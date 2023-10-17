#include <QDebug>
#include <string>
#include <cstring>
#include <QProcess>
#include <thread>
#include <chrono>

#include "dev_scanner.h"
#include "udev_wrap_utils.h"

const char* const ID_INPUT_OK = "1";
const char* const IS_KEYBOAD_PROPERTY = "ID_INPUT_KEYBOARD";
const char* const IS_MOUSE_PROPERTY = "ID_INPUT_MOUSE";
const char* const INPUT_SUBSYSTEM = "input";
const char* const VIDEO_SUBSYSTEM = "drm";
const char* const INVALID_ARG_EXCEPTION_MESSAGE = "dev subsystem isn't \"input\"";
const char* const USB_HID_PROTOCOL_PROP = "bInterfaceProtocol";
const char* const USB_HID_MOUSE_DESCRIPTOR = "02";
const char* const USB_HID_KEYBOARD_DESCRIPTOR = "01";

DevScanner::DevScanner(int num_of_monitors)
    : m_udevContext(udev_new(), &udev_unref), m_numOfMonitors(num_of_monitors) {}

void DevScanner::listen()
{
    if(!m_udevContext.raw())
        throw std::runtime_error{"udev not initialized"};
    do_first_scan();
    UdevObject<udev_monitor> monitor{udev_monitor_new_from_netlink(m_udevContext.raw(), "udev"),
                                     &udev_monitor_unref};
    udev_monitor_filter_add_match_subsystem_devtype(monitor.raw(), INPUT_SUBSYSTEM, NULL);
    udev_monitor_filter_add_match_subsystem_devtype(monitor, VIDEO_SUBSYSTEM, NULL);
    udev_monitor_enable_receiving(monitor.raw());
    qDebug() << "Started listening for udev events";
    for(;;) {
        UdevObject<udev_device> dev{udev_monitor_receive_device(monitor.raw()), &udev_device_unref};
        if (dev.raw()) {
            auto subsystem = udev_device_get_subsystem(dev);
            if(cstr_equal(subsystem, VIDEO_SUBSYSTEM)) {
                //без sleep_for вызов xrandr не успеет подхватить отсутствие монитора
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                check_monitor();
            } else {
                check_input_udev_device(dev);
            }
        }
    }
}

void DevScanner::register_observer(IDevScannerObserver *obs)
{
    m_observers.push_front(obs);
}

void DevScanner::remove_observer(IDevScannerObserver *obs)
{
    m_observers.remove_if([obs](const IDevScannerObserver* m) { return m == obs; });
}

DevStatus DevScanner::udev_action_to_dev_status(const char *status)
{
    if(!status)
        return DevStatus::Undefined;
    if(std::strcmp(status, "remove") == 0) {
        return DevStatus::NotAvailable;
    }
    if(std::strcmp(status, "add") == 0) {
        return DevStatus::Available;
    }
    return DevStatus::Undefined;
}

void DevScanner::notify(DevType type, DevStatus status)
{
    for(auto obs : m_observers) {
        obs->receive(type, status);
    }
}

void DevScanner::do_first_scan()
{
    qDebug() << "Starting first check before listening for udev events...";
    UdevObject<udev_enumerate> enumerate{udev_enumerate_new(m_udevContext), &udev_enumerate_unref};
    udev_enumerate_add_match_subsystem(enumerate, INPUT_SUBSYSTEM);
    udev_enumerate_scan_devices(enumerate);
    udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry* dev_list_entry;
    qDebug() << "Checking input devices...";
    udev_list_entry_foreach(dev_list_entry, devices) {
        const char *path;
        path = udev_list_entry_get_name(dev_list_entry);
        UdevObject<udev_device> dev{udev_device_new_from_syspath(m_udevContext, path), &udev_device_unref};
        first_check_input_devs(dev);
    }
    qDebug() << "Input devices first check result: ";
    qDebug() << "Mouse found: " << m_1stCheckResult.mouse_found;
    qDebug() << "Keyboard found: " << m_1stCheckResult.keyboard_found;
    notify(DevType::Mouse, m_1stCheckResult.mouse_found ? DevStatus::Available : DevStatus::NotAvailable);
    notify(DevType::Keyboard, m_1stCheckResult.keyboard_found ? DevStatus::Available : DevStatus::NotAvailable);
    qDebug() << "Starting screen first check...";
    check_monitor();
}

void DevScanner::check_monitor()
{
    QString programm = "xrandr";
    QStringList arguments;
    arguments << "--listmonitors";
    QProcess process;
    process.start(programm, arguments);
    if(process.waitForStarted()) {
        qDebug() << "Checking monitor availability with xrandr...";
        qDebug() << "xrandr started";
        if(process.waitForReadyRead()) {
            qDebug() << "Reading output from xrandr...";
            auto output = process.readAllStandardOutput();
            auto output_str = QString::fromLocal8Bit(output);
            qDebug() << "Output is: " << output_str;
            qDebug() << "Processing output...";
            auto num_of_monitors_begin = std::find(output_str.begin(), output_str.end(), ' ');
            if(num_of_monitors_begin != output_str.end()) {
                auto num_of_monitors_end = std::find(num_of_monitors_begin, output_str.end(), '\n');
                if(num_of_monitors_end != output_str.end()) {
                    QString nm_str;
                    ++num_of_monitors_begin;
                    while(num_of_monitors_begin != num_of_monitors_end) {
                        nm_str.append(*num_of_monitors_begin++);
                    }
                    qDebug() << "Parsed num of monitors from xrandr output: " << nm_str;
                    if(nm_str.toInt() < m_numOfMonitors) {
                        notify(DevType::Screen, DevStatus::NotAvailable);
                    } else {
                        notify(DevType::Screen, DevStatus::Available);
                    }
                }
            }

            if(process.waitForFinished()) {
                qDebug() << "xrandr finished!";
            }
        }

    } else {
        qDebug() << "Can't start xrandr, monitor status is undefined";
    }
}

void DevScanner::log_scan_device(UdevObject<udev_device> &) const
{
//    auto devpath = udev_dev
}

bool DevScanner::cstr_equal(const char * first, const char * second) const noexcept
{
    auto comp = std::strcmp(first, second);
    return comp ? false : true;
}

DevType DevScanner::get_input_device_dev_type(UdevObject<udev_device> &dev, bool first_check)
{
    auto subsystem_check = udev_device_get_subsystem(dev);
    if(!subsystem_check) throw std::invalid_argument{INVALID_ARG_EXCEPTION_MESSAGE};
    if(!cstr_equal(subsystem_check, INPUT_SUBSYSTEM)) throw std::invalid_argument{INVALID_ARG_EXCEPTION_MESSAGE};
    auto dev_raw = dev.raw();
    if(first_check) {
        dev_raw = udev_device_get_parent_with_subsystem_devtype(dev_raw, "usb", nullptr);
        if(!dev_raw) return DevType::Undefined;
        auto dev_prot = udev_device_get_sysattr_value(dev_raw, USB_HID_PROTOCOL_PROP);
        if(!dev_prot) return DevType::Undefined;
        if(cstr_equal(dev_prot, USB_HID_MOUSE_DESCRIPTOR)) return DevType::Mouse;
        else if(cstr_equal(dev_prot, USB_HID_KEYBOARD_DESCRIPTOR)) return DevType::Keyboard;
        else return DevType::Undefined;
    } else {
        auto id_prop_keyboard = udev_device_get_property_value(dev_raw, IS_KEYBOAD_PROPERTY);
        if(id_prop_keyboard && cstr_equal(id_prop_keyboard, ID_INPUT_OK)) return DevType::Keyboard;
        auto id_prop_mouse = udev_device_get_property_value(dev_raw, IS_MOUSE_PROPERTY);
        if(id_prop_mouse && cstr_equal(id_prop_mouse, ID_INPUT_OK)) return DevType::Mouse;
        return DevType::Undefined;
    }

}

void DevScanner::check_input_udev_device(UdevObject<udev_device> &dev)
{
    auto action = udev_action_to_dev_status(udev_device_get_action(dev.raw()));
    auto name = udev_device_get_sysname(dev);
    qDebug() << "/*******************";
    try {
        auto type = get_input_device_dev_type(dev, false);
        if(type == DevType::Undefined) {
            qDebug() << "Found undefined device, skip...";
        }
        else {
            qDebug() << "Udev sysname is " << (name ? name : "");
            qDebug() << "Dev type is " << dtts(type);
            qDebug() << "Status is " << dsts(action);
            notify(type, action);
        }
    } catch (const std::invalid_argument& ex) {
        qDebug() << ex.what();
    }
    qDebug() << "********************/\n";
}

void DevScanner::first_check_input_devs(UdevObject<udev_device> &dev)
{
    try {
        auto type = get_input_device_dev_type(dev, true);
        if(type == DevType::Keyboard) {
            m_1stCheckResult.keyboard_found = true;
        } else if(type == DevType::Mouse) {
            m_1stCheckResult.mouse_found = true;
        }

    } catch (const std::invalid_argument& ex) {
        qDebug() << ex.what();
    }
}

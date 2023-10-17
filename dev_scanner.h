#ifndef DEVSCANNER_H
#define DEVSCANNER_H

#include <forward_list>

#include "udev_wrap_utils.h"
#include "idev_scanner_observer.h"

struct FirstCheckInputDevsResult {
    bool mouse_found = false;
    bool keyboard_found = false;
};

class DevScanner final
{
public:
    explicit DevScanner(int num_of_monitors = 1);
    void listen();
    void register_observer(IDevScannerObserver* obs);
    void remove_observer(IDevScannerObserver* obs);
    static DevStatus udev_action_to_dev_status(const char*);

private:
    void notify(DevType type, DevStatus status);
    void do_first_scan();
    void check_input_udev_device(UdevObject<udev_device>& dev);
    void first_check_input_devs(UdevObject<udev_device>& dev);
    void check_monitor();
    void log_scan_device(UdevObject<udev_device>&) const;
    bool cstr_equal(const char*, const char*) const noexcept;
    DevType get_input_device_dev_type(UdevObject<udev_device>& dev, bool first_check = false);



private:
    FirstCheckInputDevsResult m_1stCheckResult;
    UdevObject<udev> m_udevContext;
    std::forward_list<IDevScannerObserver*> m_observers;
    int m_numOfMonitors;
};

#endif // DEVSCANNER_H

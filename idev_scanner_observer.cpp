#include <iostream>
#include <iomanip>

#include "idev_scanner_observer.h"
#include "dev_scanner.h"

void IDevScannerObserver::subscribe(DevScanner &scanner) noexcept
{
    if(!m_dev_scanner) {
        scanner.register_observer(this);
        m_dev_scanner = &scanner;
    }
}

void IDevScannerObserver::unsubscribe(DevScanner &scanner) noexcept
{
    if(m_dev_scanner) {
        scanner.remove_observer(this);
        m_dev_scanner = nullptr;
    }
}

void IDevScannerObserver::receive(DevType type, DevStatus status) noexcept
{
    handle(type, status);
}

IDevScannerObserver::~IDevScannerObserver()
{
    if(m_dev_scanner) {
        unsubscribe(*m_dev_scanner);
    }
}

ConsoleLogObserver::ConsoleLogObserver() {}

void ConsoleLogObserver::handle(DevType type, DevStatus status) noexcept
{
    std::cout << "ConsoleLogObserver: handling device of type " << std::quoted(dtts(type)) <<
        ", status " << std::quoted(dsts(status));
}



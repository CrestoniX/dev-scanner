#ifndef IDEVSCANNEROBSERVER_H
#define IDEVSCANNEROBSERVER_H

#include "dev_enum.h"

class DevScanner;


class IDevScannerObserver
{
public:
    IDevScannerObserver() = default;
    void subscribe(DevScanner& scanner) noexcept;
    void unsubscribe(DevScanner& scanner) noexcept;
    void receive(DevType, DevStatus) noexcept;
    IDevScannerObserver(const IDevScannerObserver&) = delete;
    IDevScannerObserver(IDevScannerObserver&&) = delete;
    IDevScannerObserver& operator=(const IDevScannerObserver&) = delete;
    IDevScannerObserver& operator=(IDevScannerObserver&&) = delete;
    //to do unsub on destruction
    virtual ~IDevScannerObserver();
private:
    virtual void handle(DevType, DevStatus) noexcept = 0;

private:
    DevScanner* m_dev_scanner = nullptr;
};

class ConsoleLogObserver final : public IDevScannerObserver {
public:
    explicit ConsoleLogObserver();
private:
    void handle(DevType, DevStatus) noexcept override;
    const char* resolve_arm_id() const noexcept;


};

#endif // IDEVSCANNEROBSERVER_H

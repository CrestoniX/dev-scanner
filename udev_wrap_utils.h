#ifndef UDEV_WRAP_UTILS_H
#define UDEV_WRAP_UTILS_H

#include <memory>
#include <libudev.h>

template<typename T>
using UdevUnrefFuncPtr = T*(*)(T*);

template<typename UdevEntityType, typename DeleterFunc = UdevUnrefFuncPtr<UdevEntityType>>
struct UdevDeleter final {
    explicit UdevDeleter(DeleterFunc deleter_func) : d_func{deleter_func} {}
    void operator()(UdevEntityType* obj) const noexcept {
        d_func(obj);
    }
    DeleterFunc d_func;
};

template<typename UdevEntityType, typename DeleterFunc = UdevUnrefFuncPtr<UdevEntityType>>
class UdevObject final {
public:
    UdevObject(UdevEntityType* ent, DeleterFunc del) : m_udev_obj_ptr(ent, del) {}
    UdevEntityType* raw() noexcept {return m_udev_obj_ptr.get(); }
    operator UdevEntityType*() { return m_udev_obj_ptr.get(); }
private:
    std::shared_ptr<UdevEntityType> m_udev_obj_ptr;
};

#endif // UDEV_WRAP_UTILS_H

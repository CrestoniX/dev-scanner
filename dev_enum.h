#ifndef DEV_ENUM_H
#define DEV_ENUM_H

#include <exception>
#include <stdexcept>

enum class DevStatus {
    NotAvailable = 0,
    Available,
    Undefined
};

enum class DevType {
    Mouse = 0,
    Keyboard,
    Screen,
    Undefined
};

inline int dei(DevStatus status) noexcept {
    return static_cast<int>(status);
}

inline int dei(DevType type) noexcept {
    return static_cast<int>(type);
}

inline DevStatus ids(int value) {
    if(value > dei(DevStatus::Available) || value < 0) throw std::invalid_argument{"bad int value"};
    return static_cast<DevStatus>(value);
}

inline DevType idt(int value) {
    if(value > dei(DevType::Undefined) || value < 0) throw std::invalid_argument{"bad int value"};
    return static_cast<DevType>(value);
}

static const char* dtts(DevType dt) {
    switch (dt) {
    case DevType::Mouse:
        return "mouse";
    case DevType::Keyboard:
        return "keyboard";
    case DevType::Screen:
        return "screen";
    default:
        return "undefined";
    }
}

static const char* dsts(DevStatus dt) {
    switch (dt) {
    case DevStatus::Available:
        return "available";
    case DevStatus::NotAvailable:
        return "not available";
    default:
        return "undefined";
    }
}



#endif // DEV_ENUM_H

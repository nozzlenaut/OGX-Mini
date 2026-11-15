#ifndef _DEVICE_DRIVER_TYPES_H_
#define _DEVICE_DRIVER_TYPES_H_

#include <cstdint>

enum class DeviceDriverType : uint8_t
{
    NONE = 0,
    XBOXOG,
    XBOXOG_SB,
    XBOXOG_XR,
    XINPUT,
    PS3,
    DINPUT,
    PSCLASSIC,
    SWITCH,
    XBOXONE_SERIES_BRIDGE, // Xbox 360 XInput via external Xbox One/Series converter
    WEBAPP = 100,
    UART_BRIDGE
};

#endif // _DEVICE_DRIVER_TYPES_H_
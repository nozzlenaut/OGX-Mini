#pragma once
#include "USBDevice/DeviceDriver/DeviceDriverTypes.h"
namespace standalone_ui {
    // Power-up initialization; sets buttons, OLED and last console output.
    void initialize(DeviceDriverType mode);
    // A single quick menu update during the normal gamepad loop.
    void poll();
}

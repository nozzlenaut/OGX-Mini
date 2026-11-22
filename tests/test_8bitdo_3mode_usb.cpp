// Compile and run with a normal desktop C++ compiler, no Pico SDK required.
#include <cassert>
#include "Firmware/RP2040/src/USBHost/HostDriver/XInput/EightBitDo3Mode.h"

int main() {
    using namespace EightBitDo3Mode;
    static_assert(is_receiver_id(0x2DC8, 0x200F), "waiting receiver");
    static_assert(is_receiver_id(0x2DC8, 0x201B), "connected receiver");
    static_assert(!is_receiver_id(0x2DC8, 0x310B), "other 8BitDo receiver");
    static_assert(!is_receiver_id(0x045E, 0x201B), "unrelated vendor");

    assert(is_gip_receiver_interface(VID, PID_CONNECTED, 0, 0xFF, 2));
    assert(is_gip_receiver_interface(VID, PID_WAITING, 0, 0xFF, 2));
    assert(!is_gip_receiver_interface(VID, PID_CONNECTED, 1, 0xFF, 2));
    assert(!is_gip_receiver_interface(VID, PID_CONNECTED, 0, 0x03, 2));
    assert(!is_gip_receiver_interface(VID, PID_CONNECTED, 0, 0xFF, 1));
    assert(!is_gip_receiver_interface(0x045E, PID_CONNECTED, 0, 0xFF, 2));
    return 0;
}

#ifndef _OGXM_8BITDO_3MODE_H_
#define _OGXM_8BITDO_3MODE_H_

#include <cstdint>

// 8BitDo Ultimate 3-Mode for Xbox 2.4 GHz receiver.
// The receiver appears as 2DC8:200F while disconnected/connecting,
// then re-enumerates as 2DC8:201B when the controller is paired.
// These IDs are documented by the Linux xpad patch (August 2026).
namespace EightBitDo3Mode {
    constexpr uint16_t VID = 0x2DC8;
    constexpr uint16_t PID_WAITING = 0x200F;
    constexpr uint16_t PID_CONNECTED = 0x201B;
    constexpr uint8_t USB_VENDOR_CLASS = 0xFF;

    constexpr bool is_receiver_id(uint16_t vid, uint16_t pid) {
        return vid == VID && (pid == PID_WAITING || pid == PID_CONNECTED);
    }

    // Some receiver firmware revisions use a nonstandard USB interface
    // subclass/protocol. Claim only vendor-specific interface 0 with both
    // endpoints; do not intercept its separate HID/config interfaces.
    constexpr bool is_gip_receiver_interface(uint16_t vid, uint16_t pid,
                                             uint8_t itf_number, uint8_t itf_class,
                                             uint8_t endpoint_count) {
        return is_receiver_id(vid, pid) &&
               itf_number == 0 &&
               itf_class == USB_VENDOR_CLASS &&
               endpoint_count >= 2;
    }
}

#endif // _OGXM_8BITDO_3MODE_H_

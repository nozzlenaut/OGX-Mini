#ifndef _OGXM_8BITDO_XPAD_GIP_H_
#define _OGXM_8BITDO_XPAD_GIP_H_

#include <array>
#include <cstdint>

// Mirrors Linux xpad's generic Xbox One startup command order for the
// 8BitDo Ultimate 3-Mode Xbox 2.4GHz USB receiver only:
// POWER_ON -> LED_ON -> AUTH_DONE.
// This avoids OGX-Mini's unconditional Xbox One S/Elite S_INIT (which
// xpad sends only for certain Microsoft products).
// Source: linux/drivers/input/joystick/xpad.c, xboxone_init_packets[].
namespace EightBitDoXpad {

enum class Stage : uint8_t { POWER_ON, LED_ON, AUTH_DONE, COMPLETE };

struct Packet {
    std::array<uint8_t, 13> bytes{};
    uint8_t length{0};
};

constexpr Packet make_packet(Stage stage, uint8_t serial)
{
    Packet out{};
    switch (stage) {
        case Stage::POWER_ON:
            out.bytes = {0x05, 0x20, serial, 0x01, 0x00};
            out.length = 5;
            break;
        case Stage::LED_ON:
            out.bytes = {0x0A, 0x20, serial, 0x03, 0x00, 0x01, 0x14};
            out.length = 7;
            break;
        case Stage::AUTH_DONE:
            out.bytes = {0x06, 0x20, serial, 0x02, 0x01, 0x00};
            out.length = 6;
            break;
        case Stage::COMPLETE:
            break;
    }
    return out;
}

constexpr Packet mode_ack(uint8_t sequence)
{
    // Matches Linux xpadone_ack_mode_report for GIP virtual-key packets.
    Packet out{};
    out.bytes = {0x01, 0x20, sequence, 0x09, 0x00, 0x07, 0x20,
                 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
    out.length = 13;
    return out;
}

struct InitState {
    Stage stage{Stage::COMPLETE};
    uint8_t next_serial{0};
    uint8_t failures{0};
    bool awaiting_out{false};
    uint32_t next_try_ms{0};

    void begin(uint32_t now)
    {
        stage = Stage::POWER_ON;
        next_serial = 0;
        failures = 0;
        awaiting_out = false;
        next_try_ms = now;
    }

    bool ready(uint32_t now) const
    {
        return stage != Stage::COMPLETE && !awaiting_out &&
               static_cast<int32_t>(now - next_try_ms) >= 0;
    }

    void submitted()
    {
        awaiting_out = true;
        ++next_serial;
    }

    void complete_tx(bool success, uint32_t now)
    {
        if (!awaiting_out) return;
        awaiting_out = false;
        if (success) {
            stage = static_cast<Stage>(static_cast<uint8_t>(stage) + 1);
            failures = 0;
            next_try_ms = now + 10; // let receiver process one command
        } else if (++failures >= 4) {
            stage = Stage::COMPLETE; // no infinite initialization retry loop
        } else {
            next_try_ms = now + 75;
        }
    }
};

} // namespace EightBitDoXpad

#endif // _OGXM_8BITDO_XPAD_GIP_H_

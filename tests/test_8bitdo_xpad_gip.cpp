#include <cassert>
#include "Firmware/RP2040/src/USBHost/HostDriver/XInput/EightBitDoXpadGip.h"

int main() {
    using namespace EightBitDoXpad;
    InitState s;
    assert(!s.ready(0));
    s.begin(100);
    assert(s.ready(100));
    const Packet p = make_packet(s.stage, s.next_serial);
    assert(p.length == 5 && p.bytes[0] == 0x05 && p.bytes[1] == 0x20 && p.bytes[4] == 0x00);
    s.submitted();
    assert(!s.ready(100));
    s.complete_tx(true, 100);
    assert(!s.ready(101) && s.ready(110));
    const Packet led = make_packet(s.stage, s.next_serial);
    assert(led.length == 7 && led.bytes[0] == 0x0A && led.bytes[2] == 1 && led.bytes[6] == 0x14);
    s.submitted();
    s.complete_tx(false, 110);
    assert(s.stage == Stage::LED_ON && !s.ready(180) && s.ready(185));
    s.submitted();
    s.complete_tx(true, 185);
    assert(s.stage == Stage::AUTH_DONE);
    const Packet auth = make_packet(s.stage, s.next_serial);
    assert(auth.length == 6 && auth.bytes[0] == 0x06 && auth.bytes[4] == 0x01 && auth.bytes[5] == 0);
    s.submitted();
    s.complete_tx(true, 200);
    assert(s.stage == Stage::COMPLETE && !s.ready(500));

    const Packet ack = mode_ack(0x42);
    assert(ack.length == 13 && ack.bytes[0] == 0x01 && ack.bytes[2] == 0x42);
    assert(ack.bytes[5] == 0x07 && ack.bytes[7] == 0x02);

    s.begin(500);
    for (int i = 0; i < 4; ++i) {
        assert(s.ready(500 + i * 75));
        s.submitted();
        s.complete_tx(false, 500 + i * 75);
    }
    assert(s.stage == Stage::COMPLETE); // finite retry budget
}

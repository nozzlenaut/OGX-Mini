# 8BitDo Ultimate 3-Mode for Xbox: Linux xpad USB-host compatibility trial

**Target:** wired RP2040 Zero + existing USB-A host socket, NOT the ESP32 Bluetooth/I2C variant.
**Do not merge or replace known-working firmware without testing on hardware.**

## Observed
- A normal wired Xbox One controller connects and vibrates through the original OGX-Mini on a modded Xbox 360.
- The 8BitDo Ultimate 3-Mode for Xbox 2.4 GHz USB dongle works on a Windows PC, but has not paired/produced input through the OGX adapter.
- Original first patch only matched known USB IDs; no change in symptoms.
- Earlier LED diagnostics showed a blue/green 2-second cycle, but the driver-mounted LED callback overwrote diagnostic colors, so that color loop **cannot** establish the dongle's VID/PID or USB stage.

## Examined working/reference implementations

1. **Linux xpad**: https://github.com/torvalds/linux/blob/master/drivers/input/joystick/xpad.c
   - Handles 8BitDo Ultimate 3-Mode for Xbox as `XTYPE_XBOXONE`. USB PID `2DC8:200F` represents waiting/disconnected; `2DC8:201B` appears during normal wireless operation (Aug 23 2026 kernel patch: https://lkml.rescloud.iu.edu/2608.2/12536.html).
   - Claims USB vendor-specific controller **interface 0**, with interrupt IN and OUT endpoints.
   - Generic Xbox One initialization sends `POWER_ON`, `LED_ON`, `AUTH_DONE` in this order, with sequence numbers assigned on transmission. `S_INIT` is reserved for particular Microsoft products (0x045e:02ea and 045e:0b00), **not** 8BitDo.
   - Acknowledges GIP virtual-key packets with an ACK; input transfers and OUT initialization remain asynchronous.
2. **GP2040-CE derivative**: https://github.com/tamanegitaro/8BitDoArcadeStickUSBConverter
   - Uses an RP2040 Zero with USB-A USB host and a 2.4 GHz *8BitDo Arcade Stick* receiver. Supports multiple console output profiles, but this is a **different receiver and mode**, not a confirmed Ultimate 3-Mode Xbox dongle test. It proves the general hardware architecture is viable.
3. **ogx360_t4**: https://github.com/Ryzee119/ogx360_t4
   - Supports older 8BitDo Bluetooth v1/v2 adapters and Xbox USB gamepads via Teensy USB host. Also not evidence of this proprietary Xbox 3-Mode 2.4 GHz dongle.

## Our change vs old OGX-Mini

| Aspect | Original behavior | This experiment |
| --- | --- | --- |
| IDs | 200F/201B classified with simple fallback | Keeps fallback, restricts to GIP data interface 0 |
| Initialization | POWER_ON then S_INIT on **all** Xbox One-class devices | **Only for 8BitDo 3-Mode**, POWER_ON, LED_ON, AUTH_DONE, like Linux xpad |
| Timing | Busy loop in `set_config` awaiting USB OUT | Nonblocking stage machine on TinyUSB host core; 10ms between successful commands |
| Errors | IN transfer errors could forward stale buffers; unchecked pointer | Receiver IN errors rearm without forwarding invalid data; null pointer guarded |
| Virtual key | Didn't ACK GIP virtual-key messages | Queues the xpad 13-byte GIP ACK, higher priority than init |
| Input report parsing | Compared `&prev_in_report_ + 4` against `in_report + 4` (four whole structures; undefined, out-of-bounds) | Compare 14 bytes after the 4-byte GIP header; initialize previous report buffer; reject short transfers |
| PID flip | Input unmount could reboot entire RP2040 | Ignore full-board reboot for the two known receiver PIDs; re-enumeration handled normally |
| Xbox wired | Known-good | Old USB input behavior preserved for non-8BitDo devices |

## Validation

- Standalone C++ regression tests: USB IDs and Linux-style GIP packet bytes/sequence, no endless retries.
- GitHub CI: RP2040_ZERO, RP2040 ESP32 I2C, PI_PICO, and ESP32 firmware build.
- Hardware still required for 8BitDo RF pairing, button mapping, reconnect and rumble. A successful build is **not** proof of support.
- If dongle still fails: capture actual Windows USB interfaces for the dongle **in both waiting and connected states**, and test whether PIO-USB host enumeration succeeds before trying another protocol modification. The LED colors alone have been unreliable.
- Preserve known-good `wiredopposite/OGX-Mini` v1.0.0-alpha3 fallback, and do not change ESP32 controller firmware.

## Test order

1. On the wired RP2040 Zero, first connect the normal Xbox One controller; check input and vibration on 360.
2. Power down adapter, connect the already-paired 8BitDo dongle to USB-A, then power adapter. Set controller to 2.4 GHz mode.
3. Try input and guide button; test rumble only after buttons connect.
4. Disconnect/reconnect dongle and adapter, wait 15 seconds, and repeat.
5. If it still fails, obtain USB descriptor/VID/PID info from the Windows PC. Do not claim firmware works solely from USB LEDs.

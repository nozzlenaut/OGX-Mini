# 8BitDo receiver USB attach timing experiment

## Observation on Windows
With the physical 8BitDo Ultimate 3-Mode for Xbox 2.4 GHz receiver **still plugged in** but wireless controller powered OFF, USBTreeView reports `Connection Status: 0x00 (No device is connected)` on that port. When the controller is paired and powered ON, the receiver reports `VID_2DC8&PID_201B`, USB Full-Speed, interface 0 `FF/47/D0` with 64-byte interrupt IN (`81`) and OUT (`02`) endpoints.

This means the USB connector alone cannot be treated as an always-enumerated gamepad, and seeing the LED green on OGX-Mini only proves a USB class driver mounted, NOT an RF connection or gamepad input.

## Specific change
The original wired OGX firmware waits in a loop polling D+ / D- pull-ups every 100 milliseconds **before it calls TinyUSB `tuh_init`**. A wireless dongle that electrically detaches while the controller is off may reconnect at a timing boundary that this initialization order handles poorly.

On **RP2040_ZERO only**, this experiment initializes the Pico-PIO-USB host immediately after normal board setup; the existing PIO host services USB attach and detach directly instead of delaying host initialization until an initial D+ pull-up. All other boards keep their original attach-first behavior. Based on the working original code and the Linux xpad-compatible GIP startup branch `feature/8bitdo-xpad-gip-init`.

No new hardware, no wiring change, no changes to ESP32 Bluetooth path, and no changes to console output descriptors. This is a test of **USB host timing**, not a confirmed solution.

## Test gate and cautions
- CI compiles firmware and protocol tests; cannot emulate the real receiver or prove the host can attach without a device on this board.
- First test the normal wired Xbox One controller: confirm connect, buttons, vibration on modded Xbox 360.
- Power-cycle the RP2040 with 8BitDo receiver inserted but controller off. Switch 8BitDo to 2.4GHz/on, wait 10 seconds, test buttons. Repeat after unplugging/replugging receiver.
- If it stays green without input, do **not** assume success: gather a live USB transfer capture on the PC or instrument the RP2040's actual in/out packets before modifying anything else.
- Known-good firmware should remain available as a fallback; do not merge until physical tests pass.

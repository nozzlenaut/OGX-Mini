# 8BitDo 3-Mode + console-output experiment

Branch: `feature/8bitdo-3mode-console-plus`
Base: `feature/usb-console-expansion` (the last CI-successful USB/ESP32 expansion build).
Do **not** flash over the known-working adapter until firmware and hardware tests pass.

## What changed

- Add a narrow RP2040 USB-host fallback to recognize the **8BitDo Ultimate 3-Mode for Xbox 2.4 GHz receiver** as an Xbox One GIP input device if its interface does not carry the standard Xbox One subclass/protocol.
- Match only USB VID/PIDs `2DC8:200F` (receiver searching/unpaired) and `2DC8:201B` (receiver with controller connected). Linux xpad patch: https://lkml.indiana.edu/hypermail/linux/kernel/2608.2/12536.html
- Claim only a vendor-class (0xFF), two-or-more-endpoint **interface 0**. HID/config interfaces, unrelated receivers, and existing wired Xbox controllers retain the original driver selection.
- Add an isolated C++ regression test for those cases. Full host enumeration, GIP packet exchange, rumble, and re-enumeration **still require testing with actual hardware**.

## Board / input path limitations

- **RP2040_ZERO wired board**: USB-A host adapter can accept supported wired controllers/receivers. The new 8BitDo fallback is relevant here.
- **RP2040 Zero + LilyGO ESP32 (Bluepad32 I2C hybrid)**: ESP32 is the Bluetooth input host; RP2040 USB port faces the console. This does **not** make the 2.4 GHz USB receiver usable on the existing hybrid without a separate USB-host path, and does not require changes to its wiring.
- Bluetooth rumble in the hybrid still requires a usable Bluepad32 `play_dual_rumble` callback from the controller's Bluetooth profile. The existing guard avoids calling a missing callback; we do not claim that rumble now works on the 8BitDo's Bluetooth profile.

## Console output modes inherited from USB expansion

| Output | Input/console notes | Status |
| --- | --- | --- |
| Original Xbox | Xbox USB/XID, original-console adapter cable | Inherited, needs regression test |
| Xbox 360 | XInput; XSM3 retail path imported from upstream | Compiled; console test required |
| PC / Steam Deck | XInput / DInput / Steam profile | Compiled; test host enumeration |
| Switch 1 / Switch 2 | USB Switch controller profile, docked mode | Compiled; real-console test required |
| PS3 / PlayStation Classic | Native USB modes | Compiled; hardware check required |
| Wii U | USB GameCube adapter profile; on stock Wii U generally used for Smash; broader compatibility requires compatible homebrew | Compiled; **not** a universal Pro Controller replacement |
| PS4 | DualShock 4 USB descriptor/profile | Authentication missing on this branch; **not** native PS4 support |
| Xbox One / Xbox Series | No native console USB authentication/emulation in this branch | **Unsupported output** |
| PS1/PS2, GameCube, N64, Dreamcast | GPIO console-port modes in imported upstream | **Not** plug-and-play USB outputs on the hybrid; require console wiring |

For the hybrid, hold the following on your connected controller for about 3 seconds:

- Start + Up: Xbox 360/XInput
- Start + Down: Switch
- Start + Left: PS3
- Start + Right: Original Xbox
- Start + LB + Down: Wii U GameCube USB adapter
- Start + LB + Up: Steam/PC
- Start + LB + Left: experimental PS4 HID (not authenticated)

## Tests and next steps

1. CI: `python3 tests/test_hybrid_modes.py` and `g++ -std=c++17 -Wall -Wextra -Werror -I. tests/test_8bitdo_3mode_usb.cpp -o /tmp/ogx-8bitdo-id-test && /tmp/ogx-8bitdo-id-test`.
2. CI: build RP2040 Zero, RP2040 ESP32 I2C target, and ESP32 Bluepad32 firmware.
3. Physical RP2040 Zero USB-A: confirm Xbox One wired still connects, button maps and rumble; then connect 8BitDo receiver in 2.4 GHz mode, observe 200F->201B remount, verify input, rumble, idle 10 minutes, reconnect.
4. Physical hybrid: confirm Bluetooth pairing, modes, gamepad inputs, and that non-rumble Bluetooth profiles still connect.
5. Console tests: verify Wii U only in supported USB GameCube mode, Switch/360/OG Xbox with real games. **No merge** until the known-working adapters pass.

Known-good fallback: original wiredopposite OGX-Mini v1.0.0-alpha3 on the wired RP2040 Zero; do not overwrite your existing functioning ESP/RP2040 images before validation.

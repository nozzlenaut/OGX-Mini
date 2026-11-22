# Original OGX-Mini + 8BitDo 3-Mode receiver (experimental)

This branch starts from `feature/xbox-series-8bitdo-bridge` — our **original wiredopposite firmware line**, with the existing ESP32 Bluepad32 fixes retained. It deliberately avoids importing the newer 2026 USB-host stack, because that stack failed to detect controllers on Dalton's wired RP2040 Zero.

## Firmware changes

- RP2040 USB XInput host: recognize known 8BitDo Ultimate 3-Mode for Xbox receiver VID/PID `2DC8:200F` (unpaired/waiting) and `2DC8:201B` (connected); those states re-enumerate. See https://lkml.indiana.edu/hypermail/linux/kernel/2608.2/12536.html
- If the receiver uses a nonstandard vendor-specific subclass/protocol, classify only its interface 0 with two or more endpoints as Xbox One GIP input.
- No changes to standard Xbox One/360/Xbox OG interface detection; do not claim HID/config interfaces.
- No extra hardware, pin changes, or driver-mode renumbering.

## Test gate

1. CI build: RP2040_ZERO, ESP32_BLUEPAD32_I2C, PI_PICO, ESP32; desktop tests for matching positive and negative USB IDs.
2. Real RP2040 Zero (USB-A wired host): first retest normal Xbox One controller **and vibration** on Xbox 360.
3. Then test 8BitDo 2.4 GHz receiver, re-enumeration, button/trigger mapping, rumble, disconnect/reconnect and idle timeout. A successful compilation does **not** guarantee the receiver protocol actually works.
4. Hybrid ESP32: retest Bluetooth pairing and mode switching. The 8BitDo USB receiver can't be physically plugged into the hybrid console-output USB port; this patch is primarily for the separate wired RP2040 Zero build.
5. Do not merge over working firmware or call this a completed receiver fix until hardware passes.

## Console output

This original-line branch retains the old firmware's USB output modes: Original Xbox, Xbox 360/XInput, PS3, Switch, PlayStation Classic and PC/DInput. It **does not** add Wii U, Steam/Deck-specific, or PS4 modes. Those remain on separate `feature/8bitdo-3mode-console-plus` (based on the newer 2026 console-expansion branch), where Wii U's stock support is only a GameCube adapter profile and PS4 still needs authentication. Xbox One/Series USB console output is not available without solving native console authentication.

This separation allows reliable testing without risking the ESP32 adapter already in use.

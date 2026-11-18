# USB console expansion — development and test plan

Keep RP2040 Zero + ESP32 hardware and the existing OLED concept. No extra hardware is required for the primary target modes. Preserve the original master and existing feature branches.

## Imported from OGX-Mini 2026
Source: https://github.com/MegaCadeDev/OGX-Mini-2026/tree/2c5b41096e14e2ff7b0b0f84c1b1cce8e403e34b

| Mode | Expected behavior | Validation |
| --- | --- | --- |
| Original Xbox | USB gamepad protocol, console-specific passive cable | Regression test |
| Xbox 360 | XInput with retail XSM3 authentication | Retail and modded hardware pending |
| Switch / Switch 2 | Docked USB Switch Pro | Both consoles pending |
| PS3 | USB gamepad | Hardware pending |
| PS Classic | USB gamepad | Hardware pending |
| PC / Steam Deck / Android | XInput, DInput, Steam profile as applicable | Host enumeration pending |
| Wii U | USB GameCube adapter | Stock Smash only; broader use via compatible homebrew; hardware pending |
| PS4 | USB DS4 profile | Requires authenticating intermediary; not native support |
| Xbox One / Series | No native USB output yet | Research only |

## Hybrid boards
- Reapply the existing ESP32 Bluetooth discovery, first-report, digital-trigger fallback, rumble guard and Linux build fixes from our existing feature branch.
- Build the RP2040 Zero, ESP32 Bluepad32 I2C, Pico W and ESP32 firmware in CI.
- OLED changes are on the separate Pico W branch; port to the hybrid board only after the new baseline builds and pin compatibility is checked.
- Do not increase MAX_GAMEPADS on the hybrid until the I2C transport and USB mode are validated.
- Keep all third-party licenses and upstream history intact.

## Next: Wii U and PS4
- First validate GameCube USB reports on a stock Wii U in Smash and an Aroma/compatible homebrew setup for other games.
- Explore Wii U Pro Controller Bluetooth emulation. The ESP32 is currently a Bluetooth host for input pads; simultaneous HID-device output is a separate transport problem, not an existing feature. No claim of feasibility yet.
- Investigate PS4 authentication and supported licensed controller passthrough. Do not label DS4 USB enumeration as native PS4 compatibility.
- Keep Xbox One/Series experimental until native authentication is established; no new hardware requirement for existing modes.

## Acceptance gate
Build success is not console verification. Check enumeration, mapping, analog sticks/triggers, reconnect, rumble, mode persistence, and at least 15 minutes of gameplay on real consoles. Do not merge the development branch into master until regression tests pass.

## Verification log
- Upstream merge completed without conflicts; imported tree compared against local Git merge.
- Existing ESP32 patch reapplied cleanly; Git whitespace check passed.
- Firmware compilation and console hardware testing not yet confirmed.

## Hybrid bring-up (2026-11-18)
- Existing USB expansion build passed for RP2040 Zero and ESP32 I2C RP2040 target, but failed on ESP32 BTstack configuration and Pico W. Focus CI on the actual hybrid rather than the unrelated Pico W for this iteration.
- Restored the known-compiling ESP32 IDF 5.1.5 configuration and pinned the original Bluepad32/BTstack in a separate submodule; the newer RP2040/Pico W dependencies stay untouched. No board or wiring changes. If rebuilding from a previous tree, remove generated ESP32 components/btstack and build directory first (they are generated, not source).
- Added actual ESP32 mode selection: Start+LB+Down = Wii U; Start+LB+Left = PS4 DS4 profile (auth required); Start+LB+Up = Steam. Hold for the normal three seconds. Numeric IDs match RP2040 and a regression test checks the mapping. PS4 authentication remains unimplemented.
- Hybrid input transport remains 32-byte I2C packets; motion and touchpad data are not carried on this legacy link. Do not claim motion/touchpad functionality on this hardware.
- Build verification pending on this commit; physical tests remain necessary.

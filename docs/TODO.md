# OGX-Mini software-only to-do

**Design constraint:** Keep the current RP2040 Zero + ESP32 + OLED adapter small, low-cost, and hand-assembled. **Do not require a USB-host port, external authentication dongle, USB hub, or PCB changes** for these items. Keep proven Xbox OG and Xbox 360 behavior intact; validate builds and real hardware before merging.

## Planned

- [ ] **Nintendo Switch 2 USB output compatibility** — First test whether our existing Switch Pro USB output mode already works on a retail Switch 2. Only if not, compare/port the minimal USB output changes from `MegaCadeDev/OGX-Mini-2026`. Avoid confusing Switch 2 controller **input** support with console **output** support.
- [ ] **Steam Deck / SteamOS / Bazzite USB mode** — Confirm if existing XInput or DInput is sufficient for the Steam Deck/SteamOS. If a meaningful advantage exists, selectively port the SteamOS/DualSense output handling from `MegaCadeDev/OGX-Mini-2026`, keeping the current Bluetooth and OLED workflow.
- [ ] **Multiplayer feasibility on the current hybrid, software only** — Audit Bluepad32 controller slot count, ESP32 indexed I2C transport, the RP2040 `MAX_GAMEPADS` hard limit in `Board/Config.h`, and how many USB device interfaces Switch / PS3 support. Prototype 2-player Switch / PS3 only if the existing hardware can sustain it reliably. Do not promise multiple Xbox OG / Xbox 360 players from a single USB device.
- [ ] **Wii U GameCube-adapter output** (low priority) — Only if there is a practical multiplayer use case and it works over the current USB output with no added hardware.

## Current limitations / non-goals

- The current `ESP32_BLUEPAD32_I2C` board profile explicitly clamps `MAX_GAMEPADS` to **1**. The ESP32 Bluetooth stack itself may support more devices, but a second paired controller cannot independently control a second player with this firmware.
- Stock OG Xbox and Xbox 360 multiplayer is **one OGX-Mini per player** in the current USB output implementation.
- Native Xbox One / Series and authenticated PS4 / PS5 USB output are **not** on this to-do list because they would require extra authentication hardware; keep the adapter simple.
- 8BitDo Bluetooth compatibility improvements are tracked in [PR #2](https://github.com/nozzlenaut/OGX-Mini/pull/2) and are separate from the features above.

# OGX-Mini Wireless OLED — experimental branch

Goal: plug the OLED into a Pico WH, flash one UF2, and get a standalone wireless OGX adapter with only four actions.

**This is not physically tested yet.** Keep an original working adapter / stock firmware while testing.

## Hardware
- Raspberry Pi Pico WH (headers already installed), or Pico W with two rows of headers soldered.
- Seeed 103030401 or Waveshare Pico-OLED-1.3: 64 by 128 pixels, SH1107, two physical buttons.
- The OLED's standard Pico socket fits without wiring. I2C is the usual factory setup (GP6 and GP7). The firmware also tries SPI as a fallback if the display was modified for SPI.
- Button A is GP15. Button B is GP17. The OLED reset is GP12.

## Four operations
1. PAIR — disconnect current pad and allow a new Bluetooth controller to connect. Bluetooth keys are preserved.
2. OUTPUT — choose OG Xbox, Steel Battalion, XInput, PS3, DInput, PS Classic, or Switch. XInput on Xbox 360 requires a modified console.
3. WEB APP — remember the chosen output and reboot into the original USB WebApp driver for advanced setup on a PC.
4. SAVE — store output and restart. In WebApp mode SAVE returns to the previous console output.

A means NEXT. B means SELECT. There are no profiles or stick settings in this tiny menu.

## Download the UF2
In GitHub, open Actions > Build standalone OGX-Mini OLED. Open the latest successful run for this branch. Download the artifact named ogx-mini-pico-w-oled-UNTESTED. Extract the UF2, hold BOOTSEL on the Pico while plugging it into a PC, then drag the UF2 to the RPI-RP2 USB drive.

The workflow compiles source code, but passing it does NOT prove the physical OLED or Bluetooth works. Test all four functions before gifting.

## Build locally
From the root of the repository, with Raspberry Pi Pico SDK 2.1.0 and an ARM compiler:

    cmake -S Firmware/RP2040 -B build-oled -DPICO_SDK_PATH=/path/to/pico-sdk -DCMAKE_BUILD_TYPE=Release -DOGXM_BOARD=PI_PICOW -DMAX_GAMEPADS=1 -DOGXM_STANDALONE_UI=ON
    cmake --build build-oled

The flag OGXM_STANDALONE_UI is OFF by default, so ordinary OGX-Mini firmware remains unchanged.

## Where everything is (plain English)
- MenuLogic.h — the menu's four choices and how NEXT/SELECT work.
- OledSH1107.cpp — the display driver and lettering.
- StandaloneUI.cpp — button handling, screen refresh, and four actions.
- Bluepad32.cpp — safe pairing requests on the Bluetooth processor core.
- UserSettings.cpp — saves the last output when entering WebApp, using the existing safe flash-write procedure.
- docs/index.html — simple setup instructions for a future QR code.

All new and modified code includes explanatory comments. Firmware retains the upstream project's licensing/attribution.

## Future QR code
Enable GitHub Pages from master /docs after merging. The anticipated address is https://nozzlenaut.github.io/OGX-Mini/. Verify it is live before making the 25 mm two-color printed QR. A shorter redirect improves printability.

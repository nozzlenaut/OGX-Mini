# 8BitDo 3-Mode USB-dongle diagnostic firmware (RP2040 Zero wired version)

This is a **temporary diagnostic build** based on the original OGX-Mini code line (including the previous 8BitDo ID-only patch). It does not modify master or the Bluetooth hybrid's installed firmware.

## Why this diagnostic is needed

The 8BitDo Ultimate 3-Mode Xbox dongle can re-enumerate from `2DC8:200F` while waiting to `2DC8:201B` when paired. Published 201B descriptors have a standard vendor-specific Xbox GIP interface: `FF/47/D0`, with two 64-byte endpoints on interface 0. The standard XboxOne USB class driver should recognize that already. Merely registering the product IDs did not fix detection, so we need to distinguish USB electrical attach, device enumeration, and XInput host-driver claim.

## LED meanings on the RP2040 Zero's onboard RGB LED

| LED | Meaning |
| --- | --- |
| **Red** at boot | No D+/D- USB connection detected yet; the code is still waiting to start the host stack |
| **Blue** | USB electrical attach detected, host stack initialization started, but USB enumeration / gamepad driver mounting hasn't finished |
| **Orange** | Receiver enumerated as `2DC8:200F` (waiting/connecting), but a gamepad driver has not mounted |
| **Purple** | Receiver enumerated as `2DC8:201B` (wireless controller attached), but gamepad driver has not mounted |
| **White** | Other USB device enumerated, but no gamepad driver mounted |
| **Green (usual connected color)** | Gamepad driver mounted; this alone does not prove button input or wireless pairing |

Note: some board LED implementations/firmware versions may swap red and green channels; use a known working Xbox One wired pad as a reference. If flashing this diagnostic firmware, capture the LED color *with the dongle plugged in*, then again when the controller is switched to 2.4 GHz mode. If nothing changes from the boot LED, the issue is below the controller driver (USB electrical detection/power, startup, or host enumeration).

## Important physical test

- Use the original wired RP2040 Zero with its existing USB-A host adapter (not the RP2040 + ESP32 Bluetooth hybrid).
- Connect the Zero's USB console-facing port to the powered console/PC.
- First confirm the RGB changes using the **known-working wired Xbox One controller**, then remove the controller and power-cycle the Zero to reset the host.
- Plug the 8BitDo receiver into the USB-A host port **before** powering the Zero; observe color. Repeat with hotplug after boot if necessary. Keep the 8BitDo controller set to the proper 2.4 GHz/PC position.
- No need to re-pair the already paired dongle with a PC each time. If the dongle powers on and pairs on PC, the USB detection on the OGX host remains the issue.

## Source / release

- Debug branch: `diag/8bitdo-usb-enumeration`
- CI workflow: `Verify OGX-Mini Bluetooth improvements`
- Build artifact: `ogx-bt-RP2040_ZERO` from this branch's run
- After the test, restore the known-good original alpha3 UF2 (or leave the working version if it still functions).
- Do not merge this diagnostic LED behavior into the normal code line; it is only for troubleshooting.

## Blue/green diagnostic correction — 2026-11-22

The prior blue/green cycle was **not definitive evidence of a receiver PID change**. `standard::host_mounted(true)` always set the board LED green, overwriting the orange/purple status from `tuh_mount_cb`. The receiver could have had either expected USB ID or a different one entirely.

This revision modifies only the RP2040 Zero diagnostic target:

- Don't let the generic controller-mounted callback overwrite the diagnostic LED.
- Do not reboot the entire RP2040 after **any** USB detach while diagnosing. The receiver's actual VID/PID may be unknown or changing, so a VID/PID-specific reboot exception was inadequate.
- Show **white** for ANY unexpected device ID, regardless of whether an ordinary Xbox driver recognized it.
- Orange = `2DC8:200F`, purple = `2DC8:201B`, blue = PIO USB host started / device detached, red = USB host waiting for first connection. Green should **not** appear after boot in this diagnostic build; if it does, check that the **latest** `RP2040_ZERO` artifact was flashed.

If blue is the only color, the dongle may be stalling or resetting during enumeration, and further source/serial diagnostics would be needed. Compilation does not establish that pairing or rumble work.

# 8BitDo 3-Mode USB-dongle diagnostic firmware (RP2040 Zero wired version)

This is a **temporary diagnostic build** based on the original OGX-Mini code line (including the previous 8BitDo ID-only patch). It does not modify master or the Bluetooth hybrid's installed firmware.

## Why this diagnostic is needed

The 8BitDo Ultimate 3-Mode Xbox dongle can re-enumerate from `2DC8:200F` while waiting to `2DC8:201B` when paired. Published 201B descriptors have a standard vendor-specific Xbox GIP interface: `FF/47/D0`, with two 64-byte endpoints on interface 0. The standard XboxOne USB class driver should recognize that already. Merely registering the product IDs did not fix detection, so we need to distinguish USB electrical attach, device enumeration, and XInput host-driver claim.

## LED meanings on the RP2040 Zero's onboard RGB LED

| LED | Meaning |
| --- | --- |
| **Red** at boot | No D+/D- USB connection detected yet; the code is still waiting to start the host stack |
| **Blue** | USB electrical attach detected, host stack initialization started, but USB enumeration / gamepad driver mounting hasn't finished |
| **Orange** | Receiver enumerated as `2DC8:200F` (waiting/connecting), even if an input driver has mounted |
| **Purple** | Receiver enumerated as `2DC8:201B` (connected USB identity), even if an input driver has mounted |
| **White** | Other USB device enumerated, but no gamepad driver mounted |
| **Green (usual connected color)** | Non-8BitDo gamepad driver mounted; this alone does not prove input |

Note: some board LED implementations/firmware versions may swap red and green channels; use a known working Xbox One wired pad as a reference. If flashing this diagnostic firmware, capture the LED color *with the dongle plugged in*, then again when the controller is switched to 2.4 GHz mode. If nothing changes from the boot LED, the issue is below the controller driver (USB electrical detection/power, startup, or host enumeration).

## Important physical test

- Use the original wired RP2040 Zero with its existing USB-A host adapter (not the RP2040 + ESP32 Bluetooth hybrid).
- Connect the Zero's USB console-facing port to the powered console/PC.
- First confirm the RGB changes using the **known-working wired Xbox One controller**, then remove the controller and power-cycle the Zero to reset the host.
- Plug the 8BitDo receiver into the USB-A host port **before** powering the Zero; observe color. Repeat with hotplug after boot if necessary. Keep the 8BitDo controller set to the proper 2.4 GHz/PC position.
- No need to re-pair the already paired dongle with a PC each time. If the dongle powers on and pairs on PC, the USB detection on the OGX host remains the issue.


## Follow-up: blue/green loop every two seconds

The first diagnostic build masked 8BitDo USB PIDs in the LED callback when an XInput driver had mounted. Its green indicator could therefore mean either `200F` or `201B`. Also, the original `standard::host_mounted(false)` restarts the entire RP2040 on any driver disconnect. That can interrupt receiver PID switching.

This revision keeps the RP2040 USB host running across **only known 8BitDo receiver (200F/201B) disconnects**, and shows **orange or purple even when an XInput driver mounted**. Wired Xbox controller disconnect behavior is unchanged.

- **Orange to blue repeatedly:** receiver is enumerating as waiting/unpaired, then disconnecting; no `201B` seen yet
- **Orange to purple:** receiver reached connected USB PID; test controller input and rumble
- **Purple to blue repeatedly:** the paired receiver enumerates but repeatedly drops from USB; investigate GIP init / USB transfer / power
- **Green for the 8BitDo:** if sustained, it may be an ID other than the known 200F/201B; collect VID/PID instead of guessing
- **Only blue (no orange/purple):** physical USB host starts but enumeration doesn't finish

Flashing this build does not change the 8BitDo dongle firmware or the ESP32 adapter.

## Source / release

- Debug branch: `diag/8bitdo-usb-enumeration`
- CI workflow: `Verify OGX-Mini Bluetooth improvements`
- Build artifact: `ogx-bt-RP2040_ZERO` from this branch's run
- After the test, restore the known-good original alpha3 UF2 (or leave the working version if it still functions).
- Do not merge this diagnostic LED behavior into the normal code line; it is only for troubleshooting.

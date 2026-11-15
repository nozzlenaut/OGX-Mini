# Xbox One / Series X|S bridge and 8BitDo Bluetooth work

This branch introduces an **Xbox One / Series (external converter)** output selection.
It is **not native Xbox One GIP output** and does **not bypass Xbox security**.
The selected USB output deliberately remains the existing Xbox 360 XInput implementation.
A separate, compatible **licensed Xbox One/Series USB converter** must translate that
XInput controller and handle authentication with the console.

## Use

1. Pair the controller to the ESP32 as usual.
2. Hold **Start + LB + D-pad Up** for ~3 seconds.
3. Connect the RP2040's **USB device** cable to the input port of a compatible
   converter; plug the converter into the Xbox One or Xbox Series X|S.
4. Test buttons, analog triggers and rumble.

The normal **Start + D-pad Up** selection still chooses plain Xbox 360/XInput,
with no converter-specific labelling. This new mode uses the **same USB
descriptors and data packets** as XInput; changing modes does not make XInput
directly acceptable to an Xbox One/Series console.

**Possible converter for testing:** Brook Wingman XB 3 officially supports wired
Xbox 360 controllers as inputs and Xbox One/Series consoles as outputs. Whether
it accepts this OGX-Mini's XInput emulation **still needs physical testing**.

**Do not confuse** a GP2040-CE USB authentication dongle (MagicBoots/Magic-X)
with a standalone converter. GP2040-CE-style passthrough requires a USB **host**
interface on the RP2040, a new Xbox GIP USB device class and a real auth
passthrough implementation. The `ESP32_BLUEPAD32_I2C` build currently only
uses the RP2040 USB *device* interface; its extra pins are not wired or compiled
as a USB host on the existing adapter.

## 8BitDo Ultimate 3-mode (Xbox edition)

8BitDo documents Bluetooth as Apple/Android mode; it only specifies
vibration on wired Xbox and Windows (wired / 2.4 GHz) connections.
Firmware cannot guarantee working rumble in mobile Bluetooth mode.

Fixes in this branch:
- Controllers without a Bluepad32 rumble callback can still switch output modes.
- Rumble callbacks are checked **only** when sending rumble, preventing a
  null-callback dereference; unsupported controllers skip polling.
- 8BitDo discovery may proceed even if it advertises a non-gamepad Bluetooth
  Class of Device; Bluepad32 still validates the HID protocol.
- First gamepad report after each Bluetooth reconnection is always forwarded.
- A ready event logs whether Bluepad32 has a rumble callback, to help distinguish
  a missing software rumble implementation from a missing console feedback report.

## Hardware test matrix (not tested here)

| Test | Expected outcome |
| --- | --- |
| OG Xbox mode + Xbox controller over Bluetooth | Unchanged |
| XInput/360 + Xbox controller over Bluetooth | Unchanged |
| 8BitDo Bluetooth connect/reconnect, sticks and buttons | No stale first packet |
| 8BitDo Start + D-pad Up | Mode switch even if rumble unavailable |
| 8BitDo rumble with Xbox game, Bluetooth | Capability logged; may be unsupported |
| Bridge mode + Wingman XB 3 + Series S | Pending physical authentication / input test |
| Bridge mode plugged *directly* into Series S | Not expected to work |

## Native Xbox One/Series output (follow-up)

Native GIP via USB authentication passthrough is a separate hardware/firmware
project: add a USB host path (PIO USB data pins and power arrangement) on the
RP2040, add an auth device manager, and implement GIP endpoints/handshake,
including wired rumble. It must be tested on retail Xbox One and Series
hardware. Do not mark native support as completed until tested.

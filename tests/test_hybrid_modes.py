"""Regression check for the numeric I2C protocol and hybrid mode selection."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
ESP = ROOT / "Firmware/ESP32/main/UserSettings"
RP = ROOT / "Firmware/RP2040/src"

def enum_values(path):
    body = re.search(r"enum class DeviceDriverType\s*:\s*uint8_t\s*\{(.*?)\};", path.read_text(), re.S).group(1)
    body = re.sub(r"//[^\n]*", "", body)
    values = {}
    n = -1
    for entry in body.split(","):
        entry = entry.strip()
        if not entry:
            continue
        name, sep, explicit = entry.partition("=")
        n = int(explicit.strip()) if sep else n + 1
        values[name.strip()] = n
    return values

class HybridModes(unittest.TestCase):
    def test_wire_ids(self):
        rp = enum_values(RP / "USBDevice/DeviceDriver/DeviceDriverTypes.h")
        esp = enum_values(ESP / "DeviceDriverTypes.h")
        for name, value in esp.items():
            self.assertEqual(value, rp[name], name)
        self.assertEqual({name: esp[name] for name in ("WIIU", "PS4", "STEAM")}, {"WIIU": 9, "PS4": 15, "STEAM": 16})

    def test_combo_maps(self):
        esp = (ESP / "UserSettings.cpp").read_text()
        rp = (RP / "UserSettings/UserSettings.cpp").read_text()
        for name, direction in (("WIIU", "DOWN"), ("PS4", "LEFT"), ("STEAM", "UP")):
            for source in (esp, rp):
                self.assertRegex(source, rf"{name}\s*=\s*BUTTON_COMBO\(Gamepad::BUTTON_START\s*\|\s*Gamepad::BUTTON_LB,\s*Gamepad::DPAD_{direction}\)")
                self.assertRegex(source, rf"\{{\s*ButtonCombo::{name},\s*DeviceDriverType::{name}\s*\}}")
            self.assertIn("DeviceDriverType::" + name, esp.split("struct ComboMap")[0])

if __name__ == "__main__":
    unittest.main()

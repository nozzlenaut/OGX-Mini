# Experimental local PS4 authentication — RP2040 Zero + LilyGO ESP32

Only provision authentication credentials from a controller you own. No credentials or binaries containing private keys are distributed with this project.

## Install

1. Build the `ESP32_BLUEPAD32_I2C` RP2040 firmware from this branch; it is the only supported auth target and assumes a 2 MiB RP2040 flash chip. Use the matching ESP32 firmware (no key storage on ESP32).
2. Download `docs/ps4-auth.html` and open it locally in Chrome or Edge. It is a standalone HTML file, not an online upload service. Select an unencrypted RSA-2048 PKCS#1 or PKCS#8 PEM, the corresponding 32-character hex serial, and 256-byte certificate signature.
3. The page checks format and signs/verifies a synthetic challenge locally, then creates `OGX-PRIVATE-PS4-credentials.uf2`. Put your RP2040 in BOOTSEL and copy the file onto RPI-RP2. This is a separate credential-only flash operation, not a firmware replacement. Do not share this file; delete it from Downloads when finished.
4. Reboot, select PS4 (Start + LB + Left for three seconds), attach via USB and test. The original controller or authenticator is not needed during gameplay if the local credential implementation is accepted by the console.
5. The erase button produces a UF2 that erases only the credential sector. Normal application firmware builds are checked not to overlap it.

## Layout

The 2 MiB hybrid reserves a 4 KiB sector at XIP address 0x101fb000, immediately below the existing four NVS sectors (0x101fc000 onward). All normal hybrid UF2 blocks are checked at build time and the loader checks the firmware end address. First 1024 bytes use the OGA1 format with CRC32; remaining bytes are blank. Keys are plaintext in flash; physical access can recover them. CRC is for corruption detection, not cryptographic authenticity. Browser checks cannot verify the console-issued certificate, nor guarantee that the three files are from the same controller.

## Protocol

RP2040 PS4 HID feature reports submit the challenge, return busy/ready status and return segmented response with CRC. Core 1 runs RSA-PSS signing independently of USB processing on Core 0. A generation counter prevents an outdated worker result being used after a reset. The hybrid I2C packet format is unchanged.

Protocol reference: Joypad OS (Apache-2.0; Robert Dale Smith), pinned at https://github.com/joypad-ai/joypad-os/tree/9fa79f457ae94286d43da7602ab32137303cd7d6 and GP2040-CE. Uses Pico SDK 2.1.0 Mbed TLS 2.28.8 for RSA, SHA-256 and CTR-DRBG. No actual private keys from third parties are included.

## Verification gates

- Run `node --test tests/test_ps4_provision.mjs` for synthetic PEM formats, invalid inputs, UF2 bounds and offline page parity.
- Build hybrid and standalone RP2040 targets plus original ESP32.
- Check normal firmware image does not cross reserved sector.
- Physical testing is required: BOOTSEL provisioning and erase, persistence after normal firmware update, real console authentication, continuous play over 15 minutes, repeated challenges, USB reconnect, Bluetooth pairing, rumble and input during signing. These are not established by compilation or the local browser RSA self-test.
- Browser status is structural validation and UF2 generation only; it cannot read the flashed controller status or certify PS4 acceptance. PS4 mode is experimental and stays off the stable branch pending physical verification.

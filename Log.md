## May 12 2026
Set up toolchain and created GitHub repo. Cloned repo locally. Getting used to Git workflow.

## May 14 2026
Completed research phase. Finalized spec sheet for the smart gloves project.

## May 18 2026
Finalized component specs and ordered all parts.

## May 20 2026
Parts started arriving. Received all electronic components except the Adafruit LSM6DSOX IMU (Adafruit #4438) which is still in transit.

Parts received:
- 2x ESP32-DevKitC-32E boards

Parts still pending:
- Adafruit LSM6DSOX 6 DoF Accelerometer and Gyroscope (x2)

Built and pushed the initial ESP32 firmware skeleton (smartglove_firmware.ino) to GitHub while waiting for the IMU to arrive.

Firmware completed:
- Project folder structure: firmware/smartglove_firmware/smartglove_firmware.ino
- Full NimBLE peripheral setup: server, service, NOTIFY characteristic, auto-restarts advertising on disconnect, TX power set to max
- Two-unit UUID scheme: GLOVE_ID constant (1 = left, 2 = right) with distinct 128-bit service UUIDs so a central can connect to both gloves simultaneously
- ImuSample struct: accel (g), gyro (dps), microsecond timestamp per sample
- Skeleton stubs with implementation notes for: readFIFO, detectOnset (3.0 g threshold), sendBLE (binary packet layout defined)
- Serial debug output at 115200 baud
- Pushed to GitHub (commit 0828b2e)

Blocked on: IMU delivery — cannot test or complete initIMU(), readFIFO(), or any sensor code until hardware arrives.

## May 22 2026
IMUs arrived. Verified first unit responds over I2C (imu_i2c_test.ino) — confirmed sensor is functional before moving to SPI.

## May 26 2026

Both LSM6DSOX units arrived. Soldered header pins onto both breakout boards.

Hardware verified:
- Both units confirmed working independently via SPI at 208 Hz
- Both units confirmed working simultaneously on shared VSPI bus (Unit 1 CS=GPIO 5, Unit 2 CS=GPIO 17)
- One ESP32 confirmed broadcasting over BLE as 'Glove1' using NimBLE — Serial Monitor shows "IMU OK" and "BLE broadcasting as Glove1"

Laptop BLE reception not yet verified (ble_receiver.py written but not tested end-to-end).

Firmware written and tested today:
- firmware/tests/imu_spi_single/ — single-unit SPI test, streams timestamp + accel + gyro CSV at ~200 Hz
- firmware/tests/imu_spi_dual/   — dual-unit simultaneous SPI test, streams U1/U2 accel + gyro at ~200 Hz
- firmware/ble/First_BLE_test/   — Glove1 NimBLE broadcast: IMU data packed as 6 floats (24 bytes) and notified at ~200 Hz

Receiver written:
- receiver/ble_receiver.py — Python/bleak script that scans for 'Glove1', connects, and prints incoming IMU notifications

Next steps:
- Verify laptop can receive BLE notifications from Glove1 (run ble_receiver.py)
- Flash second ESP32 as Glove2, verify both can be received simultaneously
- Begin integrating First_BLE_test logic into the main smartglove_firmware skeleton (initIMU, readFIFO, sendBLE)

## May 28 2026

Completed end-to-end BLE broadcasting verification.

Verified:
- ESP32 advertising as 'Glove1' via NimBLE confirmed working on the laptop side
- Laptop successfully receiving 48-byte IMU data packets over BLE using Python/Bleak
- Full pipeline confirmed: IMU → SPI → ESP32 → NimBLE → laptop

Next steps:
- Flash second ESP32 as Glove2 and verify simultaneous dual-glove reception
- Begin integrating verified BLE logic into main smartglove_firmware skeleton (initIMU, readFIFO, sendBLE)

## May 29 2026

Completed simultaneous BLE streaming from both gloves with real IMU data. Phase 2 Week 4 goal complete.

Hardware:
- ESP1 (GloveLeft) and ESP2 (GloveRight) each wired to one LSM6DSOX via SPI on GPIO 5
- Both ESPs broadcasting NimBLE simultaneously

Firmware:
- One IMU per ESP, 208 Hz, ±4g accel, 1000 dps gyro
- BLE packet: 24 bytes = 6 floats (gx, gy, gz, ax, ay, az)
- Only notifies when client is connected (crash fix applied)

Receiver:
- ble_dual_glove.py updated to parse real float data from both gloves simultaneously
- Confirmed live IMU data printing correctly labeled LEFT/RIGHT

Next steps:
- Integrate punch event detection into firmware: onset threshold >3g, 400ms capture window
- Hard deadline: Jun 4 2026


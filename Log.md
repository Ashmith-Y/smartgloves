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


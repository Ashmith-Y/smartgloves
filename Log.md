## May 12 2026
Set up toolchain and created GitHub repo. Cloned repo locally. Getting used to Git workflow.

## May 14 2026
Completed research phase. Finalized spec sheet for the smart gloves project.

## May 18 2026
Finalized component specs and ordered all parts.

## May 20 2026
Built and pushed the initial ESP32 firmware skeleton (smartglove_firmware.ino) to GitHub.

Hardware target: ESP32-DevKitC-32E + LSM6DSOX IMU (Adafruit #4438) over SPI (SCK=18, MOSI=23, MISO=19, CS=5), SA0 tied low to select SPI mode.

Libraries: Adafruit LSM6DSOX, Adafruit BusIO, NimBLE-Arduino.

Completed this session:
- Project folder structure: firmware/smartglove_firmware/smartglove_firmware.ino
- SPI bus setup using VSPI with the custom pin mapping, IMU detected via begin_SPI()
- ImuSample struct: accel (g), gyro (dps), microsecond timestamp per sample
- FIFO batch read skeleton (readFIFO) — reads 20 samples per cycle (~96 ms at 208 Hz); implementation notes and register references included
- Onset detection skeleton (detectOnset) — 3.0 g resultant magnitude threshold; sqrtf vs squared-compare trade-off documented
- BLE notify skeleton (sendBLE) — proposed binary packet layout defined (glove_id, sample_count, packed int16 fields); MTU negotiation path documented
- Full NimBLE peripheral setup: server, service, NOTIFY characteristic, GloveServerCallbacks (auto-restarts advertising on disconnect), TX power set to max
- Two-unit UUID scheme: GLOVE_ID constant (1 = left, 2 = right) selects between two distinct 128-bit service UUIDs so a central can connect to both gloves simultaneously
- Serial debug output at 115200 baud with labelled prefixes ([IMU], [BLE], [ONSET], [FATAL])
- loop() paces at 90 ms delay; placeholder for INT1 interrupt-driven wake once wired

Not yet implemented (pending):
- initIMU() full config: ODR/FS setters and direct register writes for FIFO stream mode + INT1 watermark interrupt (LSM6DSOX datasheet §9.14)


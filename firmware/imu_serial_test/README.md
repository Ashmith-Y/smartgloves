# imu_serial_test

Minimal Arduino sketch to verify that the LSM6DSOX IMU is wired correctly and streaming data over SPI. Once this sketch prints accel values you can move on to the full firmware (`firmware/smartglove_firmware`).

## What it does

1. Initialises the VSPI bus on the custom ESP32 pin mapping.
2. Calls `begin_SPI()` on the LSM6DSOX — prints **`LSM6DSOX found!`** on success or **`LSM6DSOX not found - check wiring`** on failure (then halts).
3. Applies the target sensor configuration:
   - Accelerometer: ±16 g full-scale, 208 Hz ODR
   - Gyroscope: ±2000 dps full-scale, 208 Hz ODR
4. Streams accel X / Y / Z in g to Serial Monitor at 115200 baud.

## SPI wiring

| LSM6DSOX pin | ESP32-DevKitC-32E GPIO | Notes |
|---|---|---|
| CS / !CS | 5 | Active-low chip select |
| SCL / SCK | 18 | VSPI clock |
| SDA / MOSI | 23 | VSPI MOSI |
| SDO / MISO | 19 | VSPI MISO |
| SA0 | GND | **Must be tied LOW** to select SPI mode on the Adafruit #4438 breakout |
| VIN | 3.3 V | Adafruit breakout has an on-board regulator |
| GND | GND | |

> **SA0 to GND is mandatory.** If SA0 is left floating or pulled HIGH the sensor stays in I²C mode and will not respond on SPI.

## Dependencies

Install all three via **Sketch → Include Library → Manage Libraries…** in the Arduino IDE:

| Library | Author | Notes |
|---|---|---|
| Adafruit LSM6DS | Adafruit | Covers LSM6DSOX; search "LSM6DS" |
| Adafruit Unified Sensor | Adafruit | Pulled in automatically as a dependency |
| Adafruit BusIO | Adafruit | SPI/I²C abstraction layer |

## Board setup

1. Boards Manager → search **esp32** by Espressif → install ≥ 2.0.
2. Tools → Board → **ESP32 Dev Module**.
3. Tools → Upload Speed → **921600** (optional, faster flashing).
4. Tools → Port → select the ESP32's COM port.

## Running the sketch

1. Open `imu_serial_test.ino` in the Arduino IDE.
2. Click **Upload**.
3. Open **Serial Monitor** (115200 baud).
4. Expected output on success:

```
LSM6DSOX found!
Config: accel ±16 g @ 208 Hz | gyro ±2000 dps @ 208 Hz
ax(g)   ay(g)   az(g)
0.012   -0.003  1.001
0.011   -0.004  1.002
...
```

With the glove lying flat, `az` should read close to **1.0 g** (Earth gravity). If all three axes read near zero or the sketch halts with "not found", recheck wiring — especially SA0 to GND and the CS pin.

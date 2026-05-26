/**
 * imu_spi_dual.ino
 * Dual LSM6DSOX simultaneous SPI test for Smart Boxing Glove project.
 * Verifies two IMU units running in parallel on a shared VSPI bus.
 *
 * Hardware
 *   MCU   : ESP32-DevKitC-32E
 *   IMU 1 : LSM6DSOX (Adafruit #4438) — SPI, CS on GPIO 5
 *   IMU 2 : LSM6DSOX (Adafruit #4438) — SPI, CS on GPIO 17
 *
 * SPI wiring (shared bus, individual chip-selects)
 *   SCK  → GPIO 18  (both units)
 *   MOSI → GPIO 23  (both units)
 *   MISO → GPIO 19  (both units)
 *   CS1  → GPIO 5   (Unit 1 only)
 *   CS2  → GPIO 17  (Unit 2 only)
 *   SA0  → GND      (must be tied LOW on each Adafruit breakout to select SPI mode)
 *
 * Libraries (install via Arduino Library Manager)
 *   • Adafruit LSM6DS           (by Adafruit)
 *   • Adafruit Unified Sensor   (by Adafruit, dependency of above)
 *   • Adafruit BusIO            (by Adafruit, dependency of above)
 *
 * Board support
 *   • ESP32 Arduino core ≥ 2.0  (Boards Manager → "esp32" by Espressif)
 *   • Target board: "ESP32 Dev Module"
 */

#include <SPI.h>
#include <Adafruit_LSM6DSOX.h>

// Shared VSPI bus pins
#define IMU_SCK  18
#define IMU_MOSI 23
#define IMU_MISO 19

// Per-unit chip-select pins
#define IMU1_CS   5
#define IMU2_CS  17

// Adafruit Unified Sensor returns accel in m/s²; divide to get g
static constexpr float G_TO_MS2 = 9.80665f;

Adafruit_LSM6DSOX imu1;
Adafruit_LSM6DSOX imu2;

void setup() {
  Serial.begin(115200);
  delay(2000);  // allow USB-serial bridge to enumerate

  SPI.begin(IMU_SCK, IMU_MISO, IMU_MOSI);

  if (!imu1.begin_SPI(IMU1_CS, &SPI)) {
    Serial.println("IMU 1 (CS=5) not found — check wiring");
    while (true) { delay(1000); }
  }
  Serial.println("IMU 1 (CS=5)  found!");

  if (!imu2.begin_SPI(IMU2_CS, &SPI)) {
    Serial.println("IMU 2 (CS=17) not found — check wiring");
    while (true) { delay(1000); }
  }
  Serial.println("IMU 2 (CS=17) found!");

  imu1.setAccelRange(LSM6DS_ACCEL_RANGE_16_G);
  imu1.setAccelDataRate(LSM6DS_RATE_208_HZ);
  imu1.setGyroRange(LSM6DS_GYRO_RANGE_2000_DPS);
  imu1.setGyroDataRate(LSM6DS_RATE_208_HZ);

  imu2.setAccelRange(LSM6DS_ACCEL_RANGE_16_G);
  imu2.setAccelDataRate(LSM6DS_RATE_208_HZ);
  imu2.setGyroRange(LSM6DS_GYRO_RANGE_2000_DPS);
  imu2.setGyroDataRate(LSM6DS_RATE_208_HZ);

  Serial.println("Config: accel ±16 g @ 208 Hz | gyro ±2000 dps @ 208 Hz");
  Serial.println("Unit\tax(g)\tay(g)\taz(g)");
}

void loop() {
  sensors_event_t accel, gyro, temp;

  imu1.getEvent(&accel, &gyro, &temp);
  float ax1 = accel.acceleration.x / G_TO_MS2;
  float ay1 = accel.acceleration.y / G_TO_MS2;
  float az1 = accel.acceleration.z / G_TO_MS2;

  imu2.getEvent(&accel, &gyro, &temp);
  float ax2 = accel.acceleration.x / G_TO_MS2;
  float ay2 = accel.acceleration.y / G_TO_MS2;
  float az2 = accel.acceleration.z / G_TO_MS2;

  Serial.print("1\t");
  Serial.print(ax1, 3);  Serial.print('\t');
  Serial.print(ay1, 3);  Serial.print('\t');
  Serial.println(az1, 3);

  Serial.print("2\t");
  Serial.print(ax2, 3);  Serial.print('\t');
  Serial.print(ay2, 3);  Serial.print('\t');
  Serial.println(az2, 3);

  // ~208 Hz → ~4.8 ms per sample; 5 ms gives a touch of headroom
  delay(5);
}

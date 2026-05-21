/**
 * imu_serial_test.ino
 * Basic IMU connectivity and accel readout for Smart Boxing Glove project.
 *
 * Hardware
 *   MCU : ESP32-DevKitC-32E
 *   IMU : LSM6DSOX (Adafruit #4438) — SPI, SA0 tied LOW (to GND)
 *
 * SPI wiring
 *   CS   → GPIO 5
 *   SCK  → GPIO 18
 *   MOSI → GPIO 23
 *   MISO → GPIO 19
 *   SA0  → GND   (selects SPI mode on the Adafruit breakout)
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

// SPI chip-select and bus pins
#define IMU_CS    5
#define IMU_SCK  18
#define IMU_MOSI 23
#define IMU_MISO 19

// Unit conversion: Adafruit Unified Sensor returns accel in m/s²
static constexpr float G_TO_MS2 = 9.80665f;

Adafruit_LSM6DSOX imu;

void setup() {
  Serial.begin(115200);
  delay(200);  // allow USB-serial bridge to enumerate

  // Initialise VSPI bus on the custom pin mapping before calling begin_SPI().
  SPI.begin(IMU_SCK, IMU_MISO, IMU_MOSI, IMU_CS);

  if (!imu.begin_SPI(IMU_CS, &SPI)) {
    Serial.println("LSM6DSOX not found - check wiring");
    while (true) { delay(1000); }
  }

  Serial.println("LSM6DSOX found!");

  // Apply sensor configuration
  imu.setAccelRange(LSM6DS_ACCEL_RANGE_16_G);
  imu.setAccelDataRate(LSM6DS_RATE_208_HZ);
  imu.setGyroRange(LSM6DS_GYRO_RANGE_2000_DPS);
  imu.setGyroDataRate(LSM6DS_RATE_208_HZ);

  Serial.println("Config: accel ±16 g @ 208 Hz | gyro ±2000 dps @ 208 Hz");
  Serial.println("ax(g)\tay(g)\taz(g)");
}

void loop() {
  sensors_event_t accel, gyro, temp;
  imu.getEvent(&accel, &gyro, &temp);

  float ax = accel.acceleration.x / G_TO_MS2;
  float ay = accel.acceleration.y / G_TO_MS2;
  float az = accel.acceleration.z / G_TO_MS2;

  Serial.print(ax, 3);  Serial.print('\t');
  Serial.print(ay, 3);  Serial.print('\t');
  Serial.println(az, 3);

  // ~208 Hz → ~4.8 ms per sample; 5 ms gives a touch of headroom
  delay(5);
}

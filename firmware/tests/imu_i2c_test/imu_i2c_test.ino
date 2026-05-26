/**
 * imu_i2c_test.ino
 * Basic IMU connectivity and accel readout for Smart Boxing Glove project.
 *
 * Hardware
 *   MCU : ESP32-DevKitC-32E
 *   IMU : LSM6DSOX (Adafruit #4438) — I2C
 *
 * I2C wiring
 *   VIN → 3V3
 *   GND → GND
 *   SCL → GPIO 22
 *   SDA → GPIO 21
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

#include <Wire.h>
#include <Adafruit_LSM6DSOX.h>

// Adafruit Unified Sensor returns accel in m/s²; divide to get g
static constexpr float G_TO_MS2 = 9.80665f;

Adafruit_LSM6DSOX imu;

void setup() {
  Serial.begin(115200);
  delay(2000);  // allow USB-serial bridge to enumerate

  if (!imu.begin_I2C()) {
    Serial.println("LSM6DSOX not found - check wiring");
    while (true) { delay(1000); }
  }

  Serial.println("LSM6DSOX found!");

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

#include <Adafruit_LSM6DSOX.h>
#include <SPI.h>

#define LSM_CS    5
#define LSM_SCK   18
#define LSM_MISO  19
#define LSM_MOSI  23

Adafruit_LSM6DSOX sox;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("LSM6DSOX SPI Test");

  if (!sox.begin_SPI(LSM_CS, LSM_SCK, LSM_MISO, LSM_MOSI)) {
    Serial.println("FAILED to find LSM6DSOX. Check wiring.");
    while (1) delay(10);
  }

  Serial.println("LSM6DSOX found!");

  // Set ranges per your spec
  sox.setAccelRange(LSM6DS_ACCEL_RANGE_16_G);
  sox.setGyroRange(LSM6DS_GYRO_RANGE_2000_DPS);

  // Set to 208Hz (closest to 200Hz the chip supports)
  sox.setAccelDataRate(LSM6DS_RATE_208_HZ);
  sox.setGyroDataRate(LSM6DS_RATE_208_HZ);

  Serial.println("Config done. Streaming...");
}

void loop() {
  sensors_event_t accel, gyro, temp;
  sox.getEvent(&accel, &gyro, &temp);

  Serial.print(millis());
  Serial.print(",");
  Serial.print(accel.acceleration.x); Serial.print(",");
  Serial.print(accel.acceleration.y); Serial.print(",");
  Serial.print(accel.acceleration.z); Serial.print(",");
  Serial.print(gyro.gyro.x); Serial.print(",");
  Serial.print(gyro.gyro.y); Serial.print(",");
  Serial.println(gyro.gyro.z);

  delay(5); // ~200Hz
}

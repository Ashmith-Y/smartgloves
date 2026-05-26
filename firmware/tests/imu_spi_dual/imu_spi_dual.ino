#include <Adafruit_LSM6DSOX.h>
#include <SPI.h>

#define LSM_SCK   18
#define LSM_MISO  19
#define LSM_MOSI  23
#define LSM_CS1   5   // Unit 1
#define LSM_CS2   17  // Unit 2

Adafruit_LSM6DSOX sox1;
Adafruit_LSM6DSOX sox2;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  // Init Unit 1
  if (!sox1.begin_SPI(LSM_CS1, LSM_SCK, LSM_MISO, LSM_MOSI)) {
    Serial.println("FAILED: Unit 1 not found");
    while (1) delay(10);
  }
  sox1.setAccelRange(LSM6DS_ACCEL_RANGE_16_G);
  sox1.setGyroRange(LSM6DS_GYRO_RANGE_2000_DPS);
  sox1.setAccelDataRate(LSM6DS_RATE_208_HZ);
  sox1.setGyroDataRate(LSM6DS_RATE_208_HZ);
  Serial.println("Unit 1 OK");

  // Init Unit 2
  if (!sox2.begin_SPI(LSM_CS2, LSM_SCK, LSM_MISO, LSM_MOSI)) {
    Serial.println("FAILED: Unit 2 not found");
    while (1) delay(10);
  }
  sox2.setAccelRange(LSM6DS_ACCEL_RANGE_16_G);
  sox2.setGyroRange(LSM6DS_GYRO_RANGE_2000_DPS);
  sox2.setAccelDataRate(LSM6DS_RATE_208_HZ);
  sox2.setGyroDataRate(LSM6DS_RATE_208_HZ);
  Serial.println("Unit 2 OK");
}

void loop() {
  sensors_event_t accel1, gyro1, temp1;
  sensors_event_t accel2, gyro2, temp2;

  sox1.getEvent(&accel1, &gyro1, &temp1);
  sox2.getEvent(&accel2, &gyro2, &temp2);

  // Unit 1
  Serial.print("U1 | ");
  Serial.print(accel1.acceleration.x); Serial.print(",");
  Serial.print(accel1.acceleration.y); Serial.print(",");
  Serial.print(accel1.acceleration.z); Serial.print(" | ");
  Serial.print(gyro1.gyro.x); Serial.print(",");
  Serial.print(gyro1.gyro.y); Serial.print(",");
  Serial.println(gyro1.gyro.z);

  // Unit 2
  Serial.print("U2 | ");
  Serial.print(accel2.acceleration.x); Serial.print(",");
  Serial.print(accel2.acceleration.y); Serial.print(",");
  Serial.print(accel2.acceleration.z); Serial.print(" | ");
  Serial.print(gyro2.gyro.x); Serial.print(",");
  Serial.print(gyro2.gyro.y); Serial.print(",");
  Serial.println(gyro2.gyro.z);

  Serial.println("---");
  delay(5);
}

#include <Adafruit_LSM6DSOX.h>
#include <NimBLEDevice.h>
#include <SPI.h>

// SPI pins
#define LSM_CS    5
#define LSM_SCK   18
#define LSM_MISO  19
#define LSM_MOSI  23

// Unique UUID for Glove 1 - do not change
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

Adafruit_LSM6DSOX sox;
NimBLECharacteristic* pCharacteristic;

void setup() {
  Serial.begin(115200);
  delay(3000); // wait 3 seconds for Serial Monitor to connect
  Serial.println("Starting...");
  // rest of your setup code
  // Init IMU
  if (!sox.begin_SPI(LSM_CS, LSM_SCK, LSM_MISO, LSM_MOSI)) {
    Serial.println("FAILED: LSM6DSOX not found");
    while (1) delay(10);
  }
  sox.setAccelRange(LSM6DS_ACCEL_RANGE_16_G);
  sox.setGyroRange(LSM6DS_GYRO_RANGE_2000_DPS);
  sox.setAccelDataRate(LSM6DS_RATE_208_HZ);
  sox.setGyroDataRate(LSM6DS_RATE_208_HZ);
  Serial.println("IMU OK");

  // Init BLE
  NimBLEDevice::init("Glove1");
  NimBLEServer* pServer = NimBLEDevice::createServer();
  NimBLEService* pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    NIMBLE_PROPERTY::NOTIFY
  );

  pService->start();
  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("BLE broadcasting as Glove1");
}

void loop() {
  sensors_event_t accel, gyro, temp;
  sox.getEvent(&accel, &gyro, &temp);

  // Pack 6 floats into a byte array and send via BLE
  float data[6] = {
    accel.acceleration.x,
    accel.acceleration.y,
    accel.acceleration.z,
    gyro.gyro.x,
    gyro.gyro.y,
    gyro.gyro.z
  };

  pCharacteristic->setValue((uint8_t*)data, sizeof(data));
  pCharacteristic->notify();

  delay(5); // 200Hz
}

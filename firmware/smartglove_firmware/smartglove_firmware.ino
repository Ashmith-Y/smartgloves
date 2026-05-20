/**
 * smartglove_firmware.ino
 * Smart Boxing Glove — ESP32-DevKitC-32E
 *
 * Hardware
 *   MCU  : ESP32-DevKitC-32E
 *   IMU  : LSM6DSOX (Adafruit #4438) — SPI, SA0 tied LOW
 *   SPI  : SCK=18  MOSI=23  MISO=19  CS=5
 *
 * Libraries required (install via Arduino Library Manager)
 *   • Adafruit LSM6DSOX        (by Adafruit)
 *   • Adafruit BusIO           (dependency of above)
 *   • NimBLE-Arduino           (by h2zero)
 *
 * Board support
 *   • ESP32 Arduino core ≥ 2.0  (Boards Manager → "esp32" by Espressif)
 *
 * Two-unit deployment
 *   Set GLOVE_ID to 1 or 2 before flashing each physical unit.
 *   Each unit advertises its own 128-bit service UUID so a central can
 *   connect to both simultaneously.
 *
 * Skeleton status
 *   All functions compile cleanly.  Sections marked TODO contain
 *   implementation notes but are not yet active.
 */

// ─── Includes ────────────────────────────────────────────────────────────────

#include <SPI.h>
#include <Adafruit_LSM6DSOX.h>   // IMU driver + Adafruit_Sensor abstraction
#include <NimBLEDevice.h>         // NimBLE-Arduino BLE stack

// ─── Glove identity ──────────────────────────────────────────────────────────
//
// Change this constant and re-flash for each physical unit.
//   1 → left glove   (uses SERVICE_UUID_GLOVE1)
//   2 → right glove  (uses SERVICE_UUID_GLOVE2)
//
#define GLOVE_ID  1

// ─── SPI pin mapping ─────────────────────────────────────────────────────────
//
// The ESP32 VSPI bus is remapped to these GPIO numbers to match the
// PCB routing.  SA0 is pulled LOW on the breakout, selecting SPI mode.
//
#define IMU_SCK   18
#define IMU_MOSI  23
#define IMU_MISO  19
#define IMU_CS     5

// ─── BLE UUIDs ───────────────────────────────────────────────────────────────
//
// 128-bit service UUIDs — one per glove unit so a central (phone / PC) can
// distinguish left from right when both are connected at the same time.
//
// Generate replacements at: https://www.uuidgenerator.net/
//
// Glove 1 (left hand)
#define SERVICE_UUID_GLOVE1  "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
// Glove 2 (right hand) — placeholder, swap in when flashing unit 2
#define SERVICE_UUID_GLOVE2  "6e400001-b5a3-f393-e0a9-e50e24dcca9e"

// IMU data characteristic — central subscribes (NOTIFY) to receive bursts
#define CHAR_UUID_IMU_DATA   "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// ─── IMU sensor settings ─────────────────────────────────────────────────────
//
// LSM6DSOX standard ODRs: 12.5, 26, 52, 104, 208, 416, 833 Hz …
// 208 Hz is the nearest standard rate to the target 200 Hz.
//
#define IMU_ODR       LSM6DS_RATE_208_HZ
#define IMU_ACCEL_FS  LSM6DS_ACCEL_RANGE_16_G
#define IMU_GYRO_FS   LSM6DS_GYRO_RANGE_2000_DPS

// ─── Onset detection ─────────────────────────────────────────────────────────
//
// A punch onset is flagged when the resultant acceleration magnitude
// |a| = sqrt(ax² + ay² + az²) exceeds this threshold.
//
static constexpr float ONSET_THRESHOLD_G = 3.0f;  // multiples of g

// ─── FIFO batch size ─────────────────────────────────────────────────────────
//
// Number of IMU samples pulled from the hardware FIFO per loop iteration.
// At 208 Hz, 20 samples span ~96 ms — a comfortable BLE notify payload.
// The LSM6DSOX FIFO holds up to 512 words (9 kB) before overflow.
//
static constexpr uint8_t FIFO_BATCH = 20;

// ─── Data types ──────────────────────────────────────────────────────────────

/**
 * One timestamped IMU sample decoded from the FIFO.
 * Accel in g, gyro in dps, timestamp from micros() at read time.
 */
struct ImuSample {
  float    ax, ay, az;   // accelerometer  [g]
  float    gx, gy, gz;   // gyroscope      [dps]
  uint32_t ts_us;        // micros() at the moment this sample was read
};

// ─── Globals ─────────────────────────────────────────────────────────────────

Adafruit_LSM6DSOX imu;

NimBLEServer         *pServer  = nullptr;
NimBLECharacteristic *pImuChar = nullptr;

bool deviceConnected = false;

// Working buffer filled by readFIFO(), consumed by detectOnset() / sendBLE()
ImuSample fifoBuffer[FIFO_BATCH];

// ─── NimBLE server callbacks ──────────────────────────────────────────────────

class GloveServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer *pSvr) override {
    deviceConnected = true;
    Serial.println("[BLE] Central connected");
  }

  void onDisconnect(NimBLEServer *pSvr) override {
    deviceConnected = false;
    Serial.println("[BLE] Central disconnected — restarting advertising");
    // Restart advertising so a new central can reconnect without a power cycle.
    NimBLEDevice::startAdvertising();
  }
};

// ─── Function prototypes ──────────────────────────────────────────────────────

bool    initIMU();
uint8_t readFIFO();
bool    detectOnset(const ImuSample *samples, uint8_t count);
void    sendBLE(const ImuSample *samples, uint8_t count);

// ═════════════════════════════════════════════════════════════════════════════
// initIMU
// ─────────────────────────────────────────────────────────────────────────────
// Brings up the VSPI bus on the custom pin assignment and verifies that the
// LSM6DSOX is present and responding.  Then applies the target sensor config:
//
//   Accelerometer : ±16 g full-scale,  208 Hz ODR
//   Gyroscope     : ±2000 dps full-scale, 208 Hz ODR
//   FIFO          : stream mode, watermark = FIFO_BATCH samples
//
// The FIFO watermark interrupt on INT1 (TODO) will replace the polling
// delay in loop() once the INT1 GPIO is wired and the interrupt handler
// is written.
//
// Returns true on success; false if the sensor is not detected on the bus.
// ═════════════════════════════════════════════════════════════════════════════
bool initIMU() {
  // Map VSPI to the board's custom pin assignment.
  // SPI.begin(sck, miso, mosi, ss) — CS is driven per-transaction.
  SPI.begin(IMU_SCK, IMU_MISO, IMU_MOSI, IMU_CS);

  if (!imu.begin_SPI(IMU_CS, &SPI)) {
    Serial.println("[IMU] ERROR: LSM6DSOX not found — check SPI wiring and CS pin");
    return false;
  }
  Serial.println("[IMU] LSM6DSOX detected on VSPI bus");

  // ── TODO: apply ODR and full-scale ranges ────────────────────────────────
  //
  // Adafruit_LSM6DSOX exposes these setters once begin_SPI() succeeds:
  //
  //   imu.setAccelDataRate(IMU_ODR);
  //   imu.setAccelRange(IMU_ACCEL_FS);
  //   imu.setGyroDataRate(IMU_ODR);
  //   imu.setGyroRange(IMU_GYRO_FS);
  //
  // Verify with:
  //   Serial.print("[IMU] Accel ODR: "); Serial.println(imu.getAccelDataRate());
  //   Serial.print("[IMU] Gyro  ODR: "); Serial.println(imu.getGyroDataRate());

  // ── TODO: configure FIFO in stream (continuous) mode ────────────────────
  //
  // The Adafruit driver does not expose FIFO control, so these are direct
  // register writes via the BusIO layer.  Key registers (SPI read = 0x80|addr):
  //
  //   FIFO_CTRL1 (0x07)  FIFO_WTM[7:0]  — low byte of watermark
  //   FIFO_CTRL2 (0x08)  FIFO_WTM[8]    — high bit + STOP_ON_WTM
  //   FIFO_CTRL3 (0x09)  BDR_GY | BDR_XL — batch rates for gyro & accel
  //   FIFO_CTRL4 (0x0A)  FIFO_MODE[2:0] = 0b110 → stream mode
  //   INT1_CTRL  (0x0D)  INT1_FIFO_TH   — route watermark flag to INT1
  //
  // See LSM6DSOX datasheet DS13285 §9.14 for full FIFO configuration table.

  Serial.println("[IMU] NOTE: ODR / FS / FIFO configuration not yet applied (TODO)");
  return true;
}

// ═════════════════════════════════════════════════════════════════════════════
// readFIFO
// ─────────────────────────────────────────────────────────────────────────────
// Drains up to FIFO_BATCH words from the LSM6DSOX 9 kB hardware FIFO and
// decodes them into fifoBuffer[].
//
// FIFO word layout (7 bytes each, see datasheet Table 107):
//   Byte 0   : tag byte — 0x01 = accel, 0x02 = gyro, others ignored
//   Bytes 1-2: X axis raw int16 (little-endian)
//   Bytes 3-4: Y axis raw int16
//   Bytes 5-6: Z axis raw int16
//
// Sensitivity (full-scale → LSB weight):
//   ±16 g    → 0.488 mg/LSB  → divide raw by 2048 for g
//   ±2000 dps → 70 mdps/LSB  → divide raw by ~14.3 for dps
//
// Returns number of samples written into fifoBuffer[] (0 on error or empty).
// ═════════════════════════════════════════════════════════════════════════════
uint8_t readFIFO() {
  // ── TODO: check FIFO_STATUS2 (0x3B) for unread sample count ─────────────
  //   uint8_t status2 = readReg(0x3B);
  //   uint16_t available = ((status2 & 0x03) << 8) | readReg(0x3A);
  //   uint8_t toRead = min((uint8_t)available, FIFO_BATCH);

  // ── TODO: burst-read `toRead` FIFO words via SPI ─────────────────────────
  //   for (uint8_t i = 0; i < toRead; i++) {
  //     uint8_t word[7];
  //     readRegBurst(0x78, word, 7);   // 0x78 = FIFO_DATA_OUT_TAG
  //     uint8_t tag = word[0] >> 3;
  //     int16_t x = (int16_t)(word[2] << 8 | word[1]);
  //     int16_t y = (int16_t)(word[4] << 8 | word[3]);
  //     int16_t z = (int16_t)(word[6] << 8 | word[5]);
  //     if (tag == 0x01) { /* accel */ }
  //     if (tag == 0x02) { /* gyro  */ }
  //   }

  // ── TODO: timestamp each completed accel+gyro pair with micros() ─────────

  Serial.println("[IMU] readFIFO() not yet implemented — returning 0 samples");
  return 0;
}

// ═════════════════════════════════════════════════════════════════════════════
// detectOnset
// ─────────────────────────────────────────────────────────────────────────────
// Scans `count` ImuSamples for a punch onset: any sample whose resultant
// acceleration magnitude exceeds ONSET_THRESHOLD_G (3.0 g).
//
// Returns true on the first sample that breaches the threshold.
//
// Performance note: sqrtf() costs ~20 cycles on the Xtensa LX6.  If profiling
// shows this is a bottleneck, replace with a squared comparison:
//   float mag2 = ax*ax + ay*ay + az*az;
//   if (mag2 >= ONSET_THRESHOLD_G * ONSET_THRESHOLD_G) …
// ═════════════════════════════════════════════════════════════════════════════
bool detectOnset(const ImuSample *samples, uint8_t count) {
  // ── TODO: iterate and compute vector magnitude ────────────────────────────
  //   for (uint8_t i = 0; i < count; i++) {
  //     const ImuSample &s = samples[i];
  //     float mag = sqrtf(s.ax*s.ax + s.ay*s.ay + s.az*s.az);
  //     if (mag >= ONSET_THRESHOLD_G) {
  //       Serial.printf("[ONSET] mag=%.2f g at t=%lu us\n", mag, s.ts_us);
  //       return true;
  //     }
  //   }

  (void)samples;  // suppress -Wunused-parameter until implemented
  (void)count;
  return false;
}

// ═════════════════════════════════════════════════════════════════════════════
// sendBLE
// ─────────────────────────────────────────────────────────────────────────────
// Serialises `count` ImuSamples into a compact binary payload and pushes it
// to the subscribed central via a BLE NOTIFY on pImuChar.
//
// Proposed packet layout (big-endian, variable length):
//
//   Offset  Size  Field
//   ──────  ────  ─────────────────────────────────────────
//   0       1     glove_id       (GLOVE_ID constant)
//   1       1     sample_count   (number of samples in this packet)
//   2+      N×14  per-sample data:
//                   int16  ax, ay, az   (raw counts, ±16 g scale)
//                   int16  gx, gy, gz   (raw counts, ±2000 dps scale)
//                   uint32 ts_us        (timestamp, little-endian)
//
// Maximum payload for 20 samples: 2 + 20×14 = 282 bytes.
// Default BLE MTU is 23 bytes (20 usable).  Negotiate a larger ATT MTU
// (up to 512 bytes on NimBLE) or chunk into multiple notifications.
//
// TODO: call NimBLEDevice::setMTU(512) before advertising, then check
//       pServer->getPeerMTU(connId) to know the actual agreed size.
// ═════════════════════════════════════════════════════════════════════════════
void sendBLE(const ImuSample *samples, uint8_t count) {
  if (!deviceConnected || pImuChar == nullptr) return;

  // ── TODO: allocate and fill byte buffer ──────────────────────────────────
  //   uint8_t buf[2 + FIFO_BATCH * 14];
  //   buf[0] = GLOVE_ID;
  //   buf[1] = count;
  //   uint16_t offset = 2;
  //   for (uint8_t i = 0; i < count; i++) { /* pack fields */ }

  // ── TODO: notify central ─────────────────────────────────────────────────
  //   pImuChar->setValue(buf, offset);
  //   pImuChar->notify();

  (void)samples;
  (void)count;
  Serial.println("[BLE] sendBLE() not yet implemented");
}

// ═════════════════════════════════════════════════════════════════════════════
// setup
// ═════════════════════════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  // Brief wait so the USB–serial bridge enumerates before first print.
  delay(200);
  Serial.printf("\n╔═══════════════════════════════════╗\n");
  Serial.printf(  "║  Smart Glove Firmware  |  Unit %d  ║\n", GLOVE_ID);
  Serial.printf(  "╚═══════════════════════════════════╝\n\n");

  // ── IMU ──────────────────────────────────────────────────────────────────
  if (!initIMU()) {
    // Hard fault — sensor absent means no data; no point continuing.
    Serial.println("[FATAL] IMU initialisation failed.  Check SPI wiring. Halting.");
    while (true) { delay(1000); }
  }

  // ── BLE ──────────────────────────────────────────────────────────────────
  // Pick the service UUID for this physical unit.
  const char *serviceUUID = (GLOVE_ID == 1) ? SERVICE_UUID_GLOVE1
                                             : SERVICE_UUID_GLOVE2;

  // Device name shown in BLE scans — human-readable.
  char devName[24];
  snprintf(devName, sizeof(devName), "SmartGlove-%d", GLOVE_ID);
  NimBLEDevice::init(devName);

  // Maximum TX power for reliable range in a noisy gym environment.
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);

  // TODO: NimBLEDevice::setMTU(512) — negotiate larger MTU for bulk payloads.

  // Create the GATT server.
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new GloveServerCallbacks());

  // Build the primary service and its notify characteristic.
  NimBLEService *pService = pServer->createService(serviceUUID);

  pImuChar = pService->createCharacteristic(
      CHAR_UUID_IMU_DATA,
      NIMBLE_PROPERTY::NOTIFY
  );
  // TODO: attach a GATT Presentation Format descriptor (0x2904) to convey
  //       the unit (g / dps) and format (sint16) to generic BLE clients.

  pService->start();

  // Advertise the service UUID so centrals can filter by glove identity.
  NimBLEAdvertising *pAd = NimBLEDevice::getAdvertising();
  pAd->addServiceUUID(serviceUUID);
  pAd->setScanResponse(true);
  NimBLEDevice::startAdvertising();

  Serial.printf("[BLE] Advertising as \"%s\"\n", devName);
  Serial.printf("[BLE] Service UUID : %s\n", serviceUUID);
  Serial.printf("[BLE] IMU Char UUID: %s\n\n", CHAR_UUID_IMU_DATA);
}

// ═════════════════════════════════════════════════════════════════════════════
// loop
// ─────────────────────────────────────────────────────────────────────────────
// Main execution cycle:
//   1. Drain FIFO_BATCH samples from the IMU hardware FIFO.
//   2. Run onset detection on the batch.
//   3. Stream the batch over BLE if a central is subscribed.
//   4. Sleep until the next FIFO watermark window.
//
// Once INT1 is wired, replace the delay() in step 4 with a semaphore that
// is released in the INT1 ISR — this removes polling jitter entirely.
// ═════════════════════════════════════════════════════════════════════════════
void loop() {
  // ── 1. Batch-read from IMU FIFO ──────────────────────────────────────────
  uint8_t nSamples = readFIFO();

  // ── 2. Onset detection ───────────────────────────────────────────────────
  if (nSamples > 0 && detectOnset(fifoBuffer, nSamples)) {
    Serial.printf("[ONSET] Impact detected! (%u samples in burst)\n", nSamples);
    // TODO: drive LED / haptic motor output here.
  }

  // ── 3. BLE notify ────────────────────────────────────────────────────────
  if (nSamples > 0 && deviceConnected) {
    sendBLE(fifoBuffer, nSamples);
  }

  // ── 4. Pace loop to FIFO watermark interval ──────────────────────────────
  // At 208 Hz × 20 samples, the watermark fills in ~96 ms.
  // 90 ms leaves a small margin for processing overhead.
  //
  // TODO: replace with:
  //   xSemaphoreTake(fifoReadySem, portMAX_DELAY);  // released in INT1 ISR
  delay(90);
}

/*
    Based on Neil Kolban example for IDF:
   https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleWrite.cpp
    Ported to Arduino ESP32 by Evandro Copercini
*/

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// capture timer info
#define CPTR_TIME_START()                                             \
  (*(uint32_t *)0x3FF5E004 | 0x03000000) & (0xFCFFFFFF | 0x00000000); \
  (*(uint32_t *)0x3FF5E008 | 0x0000001F) & (0xFFFFFFE0 | 0x00000002)
#define CPTR_TIME_CFG(value) \
  *(uint32_t *)0x3FF5E0E8 =  \
      (*(uint32_t *)0x3FF5E0E8 | 0x0000003F) & (0xFFFFFFC0 | value)
#define CPTR_CH0_CFG(value) \
  *(uint32_t *)0x3FF5E0F0 = \
      (*(uint32_t *)0x3FF5E0F0 | 0x00001FFF) & (0xFFFFE000 | value)
#define CPTR_CH1_CFG(value) \
  *(uint32_t *)0x3FF5E0F4 = \
      (*(uint32_t *)0x3FF5E0F4 | 0x00001FFF) & (0xFFFFE000 | value)
#define CPTR_CH0_VAL() *(uint32_t *)0x3FF5E0FC
#define CPTR_CH1_VAL() *(uint32_t *)0x3FF5E100
#define CAPTURE_UPDATE()                                         \
  *(uint32_t *)0x3FF5E0F0 =                                      \
      (*(uint32_t *)0x3FF5E0F0 | 0x00001000) & (0xFFFFEFFF | 1); \
  *(uint32_t *)0x3FF5E0F4 =                                      \
      (*(uint32_t *)0x3FF5E0F4 | 0x00001000) & (0xFFFFEFFF | 1)

// General purpose timer info
#define TIMER_CONFIG(value) *(uint32_t *)0x3FF5F000 = value
#define LOW32_TIMER() *(uint32_t *)0x3FF5F004
#define HIGH32_TIMER() *(uint32_t *)0x3FF5F008
#define TIMER_UPDATE() *(uint32_t *)0x3FF5F00C = 1

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string value = pCharacteristic->getValue();

    if (value.length() > 0) {
      Serial.println("*********");
      Serial.print("New value: ");
      for (int i = 0; i < value.length(); i++) Serial.print(value[i]);

      Serial.println();
      Serial.println("*********");
    }
  }
};

void setup() {
  Serial.begin(115200);
  CPTR_TIME_START();
  CPTR_TIME_CFG(0x00000001);
  TIMER_CONFIG(0xD0000000);
  CPTR_CH0_CFG(0x00000003);
  CPTR_CH1_CFG(0x000007FA);
  TIMER_UPDATE();

  Serial.print("GPT: ");
  Serial.print(HIGH32_TIMER(), HEX);
  Serial.print(",");
  Serial.println(LOW32_TIMER(), HEX);

  Serial.print("Capture: ");
  Serial.print(CPTR_CH1_VAL(), HEX);
  Serial.print(",");
  Serial.println(CPTR_CH0_VAL(), HEX);

  BLEDevice::init("MyESP32");
  BLEServer *pServer = BLEDevice::createServer();

  CAPTURE_UPDATE();
  TIMER_UPDATE();

  Serial.print("GPT: ");
  Serial.print(HIGH32_TIMER(), HEX);
  Serial.print(",");
  Serial.println(LOW32_TIMER(), HEX);

  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setCallbacks(new MyCallbacks());

  pCharacteristic->setValue("Hello World");
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();

  CAPTURE_UPDATE();
  TIMER_UPDATE();

  Serial.print("GPT: ");
  Serial.print(HIGH32_TIMER(), HEX);
  Serial.print(",");
  Serial.println(LOW32_TIMER(), HEX);

  Serial.print("Capture: ");
  Serial.print(CPTR_CH1_VAL(), HEX);
  Serial.print(",");
  Serial.println(CPTR_CH0_VAL(), HEX);
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(2000);
}
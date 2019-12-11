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

// General purpose timer info
#define TIMER_CONFIG(value) *(uint32_t *)0x3FF5F000 = value
#define LOW32_TIMER() *(uint32_t *)0x3FF5F004
#define HIGH32_TIMER() *(uint32_t *)0x3FF5F008
#define READ_TIMER()                              \
  (((uint64_t) * (uint32_t *)0x3FF5F008) << 32) + \
      (uint64_t) * (uint32_t *)0x3FF5F004
#define TIMER_UPDATE() *(uint32_t *)0x3FF5F00C = 1

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    TIMER_UPDATE();
    Serial.print(HIGH32_TIMER(), HEX);
    Serial.print(", ");
    Serial.println(LOW32_TIMER(), HEX);
    uint32_t low = READ_TIMER() % 0xFFFFFFFF;
    uint32_t high = (READ_TIMER() >> 32) % 0xFFFFFFFF;
    Serial.print(high, HEX);
    Serial.print(", ");
    Serial.println(low, HEX);
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
  TIMER_CONFIG(0xD0000000);  // initialize the GPT

  // BLE initialization
  BLEDevice::init("ESP32 NTP Client");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
  pCharacteristic->setCallbacks(new MyCallbacks());
  // starts the BLE communication
  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}

void loop() {
  // put your main code here, to run repeatedly:
  // BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  delay(2000);
}

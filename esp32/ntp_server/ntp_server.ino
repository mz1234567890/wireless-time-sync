/*
    Based on Neil Kolban example for IDF:
   https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleWrite.cpp
    Ported to Arduino ESP32 by Evandro Copercini
*/

#include <BLEAdvertisedDevice.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEServer.h>
#include <BLEUtils.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_2 "beb5483e-36e1-4688-b7f5-ea07361b26a9"

// General purpose timer info
#define TIMER_CONFIG(value) *(uint32_t *)0x3FF5F000 = value
#define LOW32_TIMER() *(uint32_t *)0x3FF5F004
#define HIGH32_TIMER() *(uint32_t *)0x3FF5F008
#define READ_TIMER()                              \
  (((uint64_t) * (uint32_t *)0x3FF5F008) << 32) + \
      (uint64_t) * (uint32_t *)0x3FF5F004
#define TIMER_UPDATE() *(uint32_t *)0x3FF5F00C = 1
#define PRINT_UINT64(data)                                \
  Serial.print((uint32_t)(*(uint64_t *)data >> 32), HEX); \
  Serial.print(", ");                                     \
  Serial.println(*(uint32_t *)data, HEX)

static BLERemoteCharacteristic *pRemoteCharacteristic;
BLECharacteristic *read_char;
static BLEAdvertisedDevice *myDevice;
BLEScan *pBLEScan;
uint64_t t1;

uint64_t isr_time;

const byte interruptPin = 5;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
void (*resetFunc)(void) = 0;  // declare reset function @ address 0

void IRAM_ATTR handleInterrupt() {
  portENTER_CRITICAL_ISR(&mux);
  TIMER_UPDATE();
  isr_time = READ_TIMER();
  portEXIT_CRITICAL_ISR(&mux);
}

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    TIMER_UPDATE();
    t1 = READ_TIMER();
    uint8_t *data = pCharacteristic->getData();
  }
};

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient *pclient) {
    Serial.println("   Connected to the server");
  }

  void onDisconnect(BLEClient *pclient) { resetFunc(); }
};

bool connectToServer() {
  Serial.print("   Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient *pClient = BLEDevice::createClient();
  Serial.println("    - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of
                               // address, it will be recognized type of peer
                               // device address (public or private)
  Serial.println("    - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService *pRemoteService = pClient->getService(SERVICE_UUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(SERVICE_UUID);
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  // Obtain a reference to the characteristic in the service of the remote BLE
  // server.
  pRemoteCharacteristic =
      pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(CHARACTERISTIC_UUID);
    pClient->disconnect();
    return false;
  }
  Serial.println("    - Found our characteristic");
  return true;
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == "ESP32 NTP Client") {
      Serial.println("    - Found our server");
      pBLEScan->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
    }  // Found our server
  }    // onResult
};     // MyAdvertisedDeviceCallbacks

void setup() {
  Serial.begin(115200);
  TIMER_CONFIG(0xD0000000);  // initialize the GPT

  // ISR setup
  Serial.println("Monitoring interrupts: ");
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt,
                  RISING);
  isr_time = 0;

  // BLE initialization
  BLEDevice::init("ESP32 NTP Server");
  Serial.println("BLE Client");
  Serial.println("    - Beginning scan for server");
  // BLE scan/connect
  pBLEScan = BLEDevice::getScan();  // create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(
      true);  // active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
  pBLEScan->start(5, false);
  connectToServer();
  pRemoteCharacteristic->writeValue(2);
  t1 = 0;
  Serial.println("BLE Server");
  Serial.println("    - Starting server");

  // BLE Server initialization
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
  pCharacteristic->setCallbacks(new MyCallbacks());
  read_char = pService->createCharacteristic(CHARACTERISTIC_UUID_2,
                                             BLECharacteristic::PROPERTY_READ);
  // starts the BLE communication
  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
  Serial.println("    - Started server");
}

void loop() {
  // put your main code here, to run repeatedly:
  if (t1) {
    TIMER_UPDATE();
    uint64_t t2 = READ_TIMER();
    // PRINT_UINT64(&t2);
    read_char->setValue((uint8_t *)&t2, 8);
    pRemoteCharacteristic->writeValue((uint8_t *)&t1, 8);
    t1 = 0;
  }
  if (isr_time) {
    Serial.print("ISR Time: ");
    PRINT_UINT64(&isr_time);
  }
}

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

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic *server_write_char;
static BLERemoteCharacteristic *server_read_char;
static BLEAdvertisedDevice *myDevice;
BLEScan *pBLEScan;
uint64_t t1;
uint64_t t3;

uint64_t isr_time;

const byte interruptPin = 5;
volatile int interruptCounter = 0;
int numberOfInterrupts = 0;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR handleInterrupt() {
  portENTER_CRITICAL_ISR(&mux);
  TIMER_UPDATE();
  isr_time = READ_TIMER();
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&mux);
}

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    TIMER_UPDATE();
    t3 = READ_TIMER();
    uint8_t *data = pCharacteristic->getData();
    t1 = *(uint64_t *)data;
  }
};

static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                           uint8_t *pData, size_t length, bool isNotify) {
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(length);
  Serial.print("data: ");
  Serial.println((char *)pData);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient *pclient) {}

  void onDisconnect(BLEClient *pclient) {
    connected = false;
    setup();
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient *pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of
                               // address, it will be recognized type of peer
                               // device address (public or private)
  Serial.println(" - Connected to server");

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
  server_write_char = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
  server_read_char = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID_2);
  if (server_write_char == nullptr || server_read_char == nullptr) {
    Serial.print("Failed to find our characteristic UUID");
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  connected = true;
  return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we
 * are looking for.
 */
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are
    // looking for.
    if (advertisedDevice.getName() == "ESP32 NTP Server") {
      pBLEScan->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

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
                  FALLING);
  isr_time = 0;

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
  t1 = 0;
  // BLE scan/connect
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();  // create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(
      true);  // active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
  pBLEScan->start(5, false);
  connectToServer();
}

void loop() {
  // make t0
  TIMER_UPDATE();
  uint64_t t0 = READ_TIMER();
  // write t0, set t1
  t1 = 0;
  server_write_char->writeValue((uint8_t *)&t0, 8);
  // wait for t1 to return
  while (t1 == 0) {
    delay(10);
  }

  // get t2 from read_char
  std::string temp = server_read_char->readValue();
  uint64_t t2 = *(uint64_t *)temp.c_str();
  uint64_t d = ((t1 - t0) + (t3 - t2)) / 2;
  uint64_t o = ((t1 - t0) - (t3 - t2)) / 2;
  TIMER_UPDATE();
  *((uint32_t *)0x3FF5F018) = LOW32_TIMER() + o;
  *((uint32_t *)0x3FF5F020) = 1;
  // Serial.print("t0: ");
  // PRINT_UINT64(&t0);
  // Serial.print("t1: ");
  // PRINT_UINT64(&t1);
  // Serial.print("t2: ");
  // PRINT_UINT64(&t2);
  // Serial.print("t3: ");
  // PRINT_UINT64(&t3);

  // Serial.print("delay: ");
  // PRINT_UINT64(&d);
  // Serial.print("offset: ");
  Serial.println((int32_t)o);
  if (isr_time) {
    portENTER_CRITICAL(&mux);
    interruptCounter--;
    portEXIT_CRITICAL(&mux);
    Serial.print("ISR Time: ");
    PRINT_UINT64(&isr_time);
  }
  delay(1 * 1000);
}

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
#include <esp32-hal-timer.h>
#include <string.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_2 "beb5483e-36e1-4688-b7f5-ea07361b26a9"

static boolean modify_offset = true;
static BLERemoteCharacteristic *server_write_char;
static BLERemoteCharacteristic *server_read_char;
static BLEAdvertisedDevice *myDevice;
BLECharacteristic *pCharacteristic;
BLEScan *pBLEScan;
static volatile uint64_t t1;
static volatile uint64_t t3;
uint64_t isr_time;
uint64_t alarm_num;
static volatile bool timestamp_received;

const byte interruptPin = 5;
int ledPin = 5;

volatile int interruptCounter;
int totalInterruptCounter;

int state = 0;

hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
void (*resetFunc)(void) = 0;  // declare reset function @ address 0

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&timerMux);
}

void IRAM_ATTR handleInterrupt() {
  portENTER_CRITICAL_ISR(&mux);
  isr_time = timerRead(timer);
  portEXIT_CRITICAL_ISR(&mux);
}

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    t3 = timerRead(timer);
    // TODO: t1 loses correct value somehow
    const char *data = pCharacteristic->getValue().c_str();
    t1 = *(uint64_t *)data;
    timestamp_received = true;
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
  Serial.println("    - Found our service");

  // Obtain a reference to the characteristic in the service of the remote BLE
  // server.
  server_write_char = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID);
  server_read_char = pRemoteService->getCharacteristic(CHARACTERISTIC_UUID_2);
  if (server_write_char == nullptr || server_read_char == nullptr) {
    Serial.print("Failed to find our characteristic UUID");
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
    if (advertisedDevice.getName() == "ESP32 NTP Server") {
      Serial.println("    - Found our server");
      pBLEScan->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
    }  // Found our server
  }    // onResult
};     // MyAdvertisedDeviceCallbacks

void setup() {
  timestamp_received = false;
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);

  // timer initialization
  timer = timerBegin(0, 0x4000, true);
  timerAttachInterrupt(timer, &onTimer, true);
  Serial.println(*(uint32_t *)0x3FF5F000, HEX);

  digitalWrite(ledPin, LOW);
  // ISR setup
  // Serial.println("\nMonitoring interrupts\n");
  // pinMode(interruptPin, INPUT_PULLUP);
  // attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt,
  // RISING);
  isr_time = 0;

  // BLE initialization
  BLEDevice::init("ESP32 NTP Client");
  t1 = 0;

  Serial.println("BLE Server");
  Serial.println("    - Starting server");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_WRITE);
  pCharacteristic->setCallbacks(new MyCallbacks());
  // starts the BLE communication
  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
  Serial.println("    - Started server");
  delay(250);
  digitalWrite(ledPin, HIGH);
  delay(250);
  digitalWrite(ledPin, LOW);
  // while (t1 == 0) {
  //   delay(10);
  // }
  Serial.println("    - Server connected");
  delay(100);
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
  delay(250);
  digitalWrite(ledPin, HIGH);
  delay(125);
  digitalWrite(ledPin, LOW);
  delay(250);
  digitalWrite(ledPin, HIGH);
  delay(125);
  digitalWrite(ledPin, LOW);
  Serial.println("Starting PTP");
  timerWrite(timer, 0);
  while (modify_offset) {
    uint64_t o = calculate_offset();
    if (modify_offset) {
      uint64_t temp = timerRead(timer);
      *(uint32_t *)0x3FF5F018 = (uint32_t)temp + (uint32_t)o;
      *(uint32_t *)0x3FF5F014 = (uint32_t)(temp >> 32) + (uint32_t)(o >> 32);
      *(uint32_t *)0x3FF5F020 = 1;
    }
    if (o < 75) {
      modify_offset = false;
    }

    // Serial.print("offset: ");
    // Serial.println((int32_t)o);
    delay(5);
  }
  digitalWrite(ledPin, LOW);
  *(uint32_t *)0x3FF5F018 = (uint32_t)0;
  *(uint32_t *)0x3FF5F014 = (uint32_t)(0 >> 32);
  timerAlarmWrite(timer, 0xD000, true);
  timerAlarmEnable(timer);
}

void loop() {
  if (isr_time) {
    Serial.print("ISR Time: ");
  }

  if (interruptCounter > 0) {
    portENTER_CRITICAL(&timerMux);
    interruptCounter--;
    portEXIT_CRITICAL(&timerMux);
    digitalWrite(ledPin, (state) ? HIGH : LOW);
    state = !state;
    timerAlarmWrite(timer, 0x500, true);
    timerAlarmEnable(timer);
  }
}

uint64_t calculate_offset() {
  // make t0
  uint64_t t0 = timerRead(timer);
  // write t0, set t1
  server_write_char->writeValue((uint8_t *)&t0, 8);

  // Serial.print("t0: ");
  // Serial.print((uint32_t)(t0 >> 32), HEX);
  // Serial.print(", ");
  // Serial.println((uint32_t)t0, HEX);

  // wait for t1 to return
  while (!timestamp_received) {
    delay(10);
  }
  timestamp_received = false;

  // Serial.print("t1: ");
  // Serial.print((uint32_t)(t1 >> 32), HEX);
  // Serial.print(", ");
  // Serial.println((uint32_t)t1, HEX);

  // get t2 from read_char
  std::string temp = server_read_char->readValue();
  uint64_t t2 = *(uint64_t *)temp.c_str();

  // Serial.print("t2: ");
  // Serial.print((uint32_t)(t2 >> 32), HEX);
  // Serial.print(", ");
  // Serial.println((uint32_t)t2, HEX);

  uint64_t d = ((t1 - t0) + (t3 - t2)) / 2;
  uint64_t o = ((t1 - t0) - (t3 - t2)) / 2;

  // Serial.print("t3: ");
  // Serial.print((uint32_t)(t1 >> 32), HEX);
  // Serial.print(", ");
  // Serial.println((uint32_t)t3, HEX);

  // Serial.print("o: ");
  // Serial.print((uint32_t)(o >> 32), HEX);
  // Serial.print(", ");
  // Serial.println((uint32_t)o, HEX);
  // Serial.print("d: ");
  // Serial.print((uint32_t)(d >> 32), HEX);
  // Serial.print(", ");
  // Serial.println((uint32_t)d, HEX);
  return o;
}

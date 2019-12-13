#include <driver/gpio.h>

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

#define GPIO_IN 5

const byte interruptPin = 5;
volatile int interruptCounter = 0;
int numberOfInterrupts = 0;
 
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
 
void IRAM_ATTR handleInterrupt() {
  portENTER_CRITICAL_ISR(&mux);
  TIMER_UPDATE();
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&mux);
}

void setup() {
  Serial.begin(115200);
  Serial.println("test");
  TIMER_CONFIG(0xD0000000);  // initialize the GPT
  Serial.println("Monitoring interrupts: ");
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, RISING);
}

void loop() {
  // put your main code here, to run repeatedly:
  if(interruptCounter>0){
 
      portENTER_CRITICAL(&mux);
      interruptCounter--;
      portEXIT_CRITICAL(&mux);
 
      numberOfInterrupts++;
      uint64_t temp = READ_TIMER();
      PRINT_UINT64(&temp);
  }
}

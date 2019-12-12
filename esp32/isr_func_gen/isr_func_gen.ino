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

static void IRAM_ATTR gpio_isr_handler(void *arg) { Serial.println("here"); }

void setup() {
  Serial.begin(115200);
  TIMER_CONFIG(0xD0000000);  // initialize the GPT
  gpio_config_t *config = new gpio_config_t;
  config->pin_bit_mask = GPIO_NUM_5;  // gpio pin 5
  config->pull_down_en = GPIO_PULLDOWN_DISABLE;
  config->pull_up_en = GPIO_PULLUP_DISABLE;
  config->mode = GPIO_MODE_INPUT;
  config->intr_type = GPIO_INTR_POSEDGE;
  gpio_config(config);
  gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
  gpio_isr_handler_add(GPIO_NUM_5, gpio_isr_handler, NULL);
}

void loop() {
  // put your main code here, to run repeatedly:
}

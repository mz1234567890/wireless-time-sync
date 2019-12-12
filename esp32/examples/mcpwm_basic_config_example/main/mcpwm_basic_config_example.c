/* MCPWM basic config example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
 * This example will show you how to use each submodule of MCPWM unit.
 * The example can't be used without modifying the code first.
 * Edit the macros at the top of mcpwm_example_basic_config.c to enable/disable
 * the submodules which are used in the example.
 */

#include <stdio.h>
#include "driver/mcpwm.h"
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"
#include "soc/rtc.h"
#include "string.h"

#define CAP0_INT_EN (BIT(27))
#define GPIO_PWM0A_OUT (19)
#define GPIO_CAP0_IN (23)
#define APB_CYC_NS (12.5)

typedef struct {
  uint32_t first_cap;
  uint32_t second_cap
} capture;

static mcpwm_dev_t *MCPWM[2] = {&MCPWM0, &MCPWM1};

static capture isr_evt = {0};

static xQueueHandle cap_queue = NULL;

static void IRAM_ATTR cap_isr_handler() {
  static bool first_cap = true;
  uint32_t mcpwm_intr_status = MCPWM[MCPWM_UNIT_0]->int_st.val;
  portBASE_TYPE HPTaskAwoken = pdFALSE;
  if (mcpwm_intr_status & CAP0_INT_EN) {
    // Post edge
    if (mcpwm_capture_signal_get_edge(MCPWM_UNIT_0, MCPWM_SELECT_CAP0) == 1) {
      if (first_cap == true) {  // first capture
        first_cap = false;
        isr_evt.first_cap =
            mcpwm_capture_signal_get_value(MCPWM_UNIT_0, MCPWM_SELECT_CAP0);
      } else {  // second capture
        MCPWM[MCPWM_UNIT_0]->int_ena.cap0_int_ena = 0;
        isr_evt.second_cap =
            mcpwm_capture_signal_get_value(MCPWM_UNIT_0, MCPWM_SELECT_CAP0);
        xQueueOverwriteFromISR(cap_queue, &isr_evt, NULL);
        first_cap = true;
      }
    } else {
      first_cap = true;
    }
  }
  if (HPTaskAwoken == pdTRUE) {
    portYIELD_FROM_ISR();
  }
  MCPWM[MCPWM_UNIT_0]->int_clr.val = mcpwm_intr_status;
  MCPWM[MCPWM_UNIT_0]->int_ena.cap0_int_ena = 1;
}

// Connect GPIO19 (MCPWM output) and GPIO23 (cap input).
void app_main(void) {
  capture evt;
  float period = 1.0;
  mcpwm_config_t pwm_config = {
      // The actual out put is abut 47.6K, (You can decrease MCPWM_CLK_PRESCL
      // and TIMER_CLK_PRESCALE to get a more accurate frequency )
      .frequency = 50000,
      .cmpr_a = 50.0,
      .cmpr_b = 50.0,
      .counter_mode = MCPWM_UP_COUNTER,
      .duty_mode = MCPWM_DUTY_MODE_0,
  };
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO_PWM0A_OUT);
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_0, GPIO_CAP0_IN);
  MCPWM[MCPWM_UNIT_0]->int_ena.val = 0;
  mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);
  cap_queue = xQueueCreate(1, sizeof(capture));
  mcpwm_capture_enable(MCPWM_UNIT_0, MCPWM_SELECT_CAP0, MCPWM_POS_EDGE, 0);
  mcpwm_isr_register(MCPWM_UNIT_0, cap_isr_handler, NULL, ESP_INTR_FLAG_IRAM,
                     NULL);  // Set ISR Handler
  MCPWM[MCPWM_UNIT_0]->int_ena.cap0_int_ena = 1;
  while (1) {
    xQueueReceive(cap_queue, &evt, portMAX_DELAY);
    period = APB_CYC_NS * (evt.second_cap - evt.first_cap) / 1000;
    printf("CAP0 : %f us\n", period);
  }
}

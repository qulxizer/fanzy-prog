#include "driver/uart.h"
#include "esp_log.h"
#include "fanzy_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "http_server.h"
#include "uart_half_duplex.h"
#include "wifi_access_point.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "main";

void print_fanzy_config(const char *tag, const fanzy_config_t *cfg) {
  if (cfg == NULL) {
    ESP_LOGE(tag, "Cannot print configuration: Pointer is NULL");
    return;
  }

  ESP_LOGI(tag, "--- fanzy_config_t Configuration ---");
  ESP_LOGI(tag, "  magic:                     0x%08X", cfg->magic);
  ESP_LOGI(tag, "  temp_divider_pu:           %s",
           cfg->temp_divider_pu ? "true" : "false");
  ESP_LOGI(tag, "  fan_pwm_inverted:          %s",
           cfg->fan_pwm_inverted ? "true" : "false");
  ESP_LOGI(tag, "  ac_pullup:                 %s",
           cfg->ac_pullup ? "true" : "false");
  ESP_LOGI(tag, "  temp_r_fixed_ohm:          %d Ohm", cfg->temp_r_fixed_ohm);
  ESP_LOGI(tag, "  temp_adc_max:              %d", cfg->temp_adc_max);
  ESP_LOGI(tag, "  temp_adc_min:              %d", cfg->temp_adc_min);
  ESP_LOGI(tag, "  temp_adc_short_threshold:  %d",
           cfg->temp_adc_short_threshold);
  ESP_LOGI(tag, "  temp_adc_open_threshold:   %d",
           cfg->temp_adc_open_threshold);
  ESP_LOGI(tag, "  temp_min_valid_c:          %d C", cfg->temp_min_valid_c);
  ESP_LOGI(tag, "  temp_max_valid_c:          %d C", cfg->temp_max_valid_c);
  ESP_LOGI(tag, "  fan_temp_on_c:             %.2f C", cfg->fan_temp_on_c);
  ESP_LOGI(tag, "  fan_temp_full_c:           %.2f C", cfg->fan_temp_full_c);
  ESP_LOGI(tag, "  fan_min_duty:              %d", cfg->fan_min_duty);
  ESP_LOGI(tag, "  fan_max_duty:              %d", cfg->fan_max_duty);
  ESP_LOGI(tag, "  ac_multiplier:             %.4f", cfg->ac_multiplier);
  ESP_LOGI(tag, "  ac_min_speed:              %d", cfg->ac_min_speed);
  ESP_LOGI(tag, "------------------------------------");
}

void app_main(void) {
  // ESP_ERROR_CHECK(wifi_access_point_start());
  ESP_ERROR_CHECK(uart_half_duplex_init());

  fanzy_config_t cfg = {
      .magic = 0x46415A59u,

      .temp_divider_pu = false,
      .fan_pwm_inverted = true,
      .ac_pullup = true,

      .temp_r_fixed_ohm = 1000,
      .temp_adc_max = 4095,
      .temp_adc_min = 0,
      .temp_adc_short_threshold = 300,
      .temp_adc_open_threshold = 3900,
      .temp_min_valid_c = -40,
      .temp_max_valid_c = 150,

      .fan_temp_on_c = 60.0f,
      .fan_temp_full_c = 65.0f,
      .fan_min_duty = 20,
      .fan_max_duty = 90,

      .ac_multiplier = 2.0f,
      .ac_min_speed = 50,
  };

  ESP_LOGI(TAG, "Press 'w' to write config, 'r' to read config.");

  while (1) {
    int c = getchar();

    switch (c) {
    case 'w':
      ESP_LOGI(TAG, "Writing config...");
      fanzy_protocol_write_config(&cfg);
      break;

    case 'r': {
      fanzy_config_t cfgg = {0};

      ESP_LOGI(TAG, "Reading config...");
      fanzy_protocol_read_config(&cfgg);
      print_fanzy_config(TAG, &cfgg);
      break;
    }

    default:
      break;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
  // ESP_ERROR_CHECK(http_server_start(queue_read_request, queue_config_write));
}

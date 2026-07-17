#include "esp_log.h"
#include "fanzy_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "uart_half_duplex.h"
#include "wifi_access_point.h"

static const char *TAG = "main";

void app_main(void) {
  ESP_ERROR_CHECK(wifi_access_point_start());
  ESP_ERROR_CHECK(uart_half_duplex_init());
  fanzy_proto_packet_t packet = fanzy_protocol_init_packet();

  uint8_t rx[64];

  ESP_ERROR_CHECK(uart_half_duplex_write((uint8_t *)&packet, sizeof(packet)));

  while (1) {
    int len = uart_half_duplex_read(rx, sizeof(rx), 100);
    if (len > 0) {
      ESP_LOG_BUFFER_HEX(TAG, rx, len);
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

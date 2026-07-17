#include "esp_log.h"
#include "fanzy_protocol.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "http_server.h"
#include "uart_half_duplex.h"
#include "wifi_access_point.h"
#include <string.h>

enum {
  PROTO_SOF = 0xAA,
  PROTO_RESP_SOF = 0xBB,
  PROTO_READ_CONFIG_INTERVAL_MS = 5000,
  PROTO_FRAME_SIZE = sizeof(fanzy_proto_packet_t),
};

static const char *TAG = "main";

static fanzy_proto_packet_t read_config_packet(void) {
  fanzy_proto_packet_t packet = {0};
  packet.sof = PROTO_SOF;
  packet.msg_id = FANZY_PROTO_MSG_READ_CONFIG;
  packet.length = 0;
  return packet;
}

static fanzy_proto_packet_t init_packet(void) {
  fanzy_proto_packet_t packet = {0};
  packet.sof = PROTO_SOF;
  packet.msg_id = FANZY_PROTO_MSG_INIT;
  packet.length = 0;
  return packet;
}

static void handle_response(const uint8_t *frame) {
  ESP_LOGD(TAG, "response header: sof=0x%02x msg=0x%02x length=%u", frame[0],
           frame[1], frame[2]);
  if (frame[0] != PROTO_RESP_SOF || frame[1] != FANZY_PROTO_MSG_READ_CONFIG ||
      frame[2] > sizeof(fanzy_config_t)) {
    ESP_LOGD(TAG, "ignored invalid response frame");
    return;
  }

  fanzy_config_t config = {0};
  memcpy(&config, &frame[3], frame[2]);
  http_server_update_config(&config);
  ESP_LOGI(TAG,
           "config: magic=%d temp_pu=%d pwm_inverted=%d ac_pu=%d "
           "temp_r=%d adc=%d..%d valid=%d..%dC fan_temp=%.1f..%.1fC "
           "duty=%d..%d ac_multiplier=%.2f ac_min_speed=%d",
           config.magic, config.temp_divider_pu, config.fan_pwm_inverted,
           config.ac_pullup, config.temp_r_fixed_ohm, config.temp_adc_min,
           config.temp_adc_max, config.temp_min_valid_c,
           config.temp_max_valid_c, config.fan_temp_on_c,
           config.fan_temp_full_c, config.fan_min_duty, config.fan_max_duty,
           config.ac_multiplier, config.ac_min_speed);
}

static void protocol_task(void *arg) {
  (void)arg;
  uint8_t input[128];
  size_t buffered = 0;

  fanzy_proto_packet_t init = init_packet();
  ESP_ERROR_CHECK(uart_half_duplex_write((uint8_t *)&init, sizeof(init)));

  while (true) {
    fanzy_proto_packet_t request = read_config_packet();
    ESP_ERROR_CHECK(
        uart_half_duplex_write((uint8_t *)&request, sizeof(request)));

    int received =
        uart_half_duplex_read(input + buffered, sizeof(input) - buffered,
                              PROTO_READ_CONFIG_INTERVAL_MS);
    if (received > 0) {
      buffered += received;
      ESP_LOGD(TAG, "protocol buffer: %u bytes", (unsigned)buffered);
    }

    while (buffered >= PROTO_FRAME_SIZE) {
      size_t offset = 0;
      while (offset < buffered && input[offset] != PROTO_RESP_SOF) {
        offset++;
      }
      if (offset > 0) {
        memmove(input, input + offset, buffered - offset);
        buffered -= offset;
      }
      if (buffered < PROTO_FRAME_SIZE) {
        break;
      }
      handle_response(input);
      memmove(input, input + PROTO_FRAME_SIZE, buffered - PROTO_FRAME_SIZE);
      buffered -= PROTO_FRAME_SIZE;
    }
  }
}

void app_main(void) {
  ESP_ERROR_CHECK(wifi_access_point_start());
  ESP_ERROR_CHECK(uart_half_duplex_init());
  ESP_ERROR_CHECK(http_server_start());
  xTaskCreate(protocol_task, "protocol", 4096, NULL, 5, NULL);
}

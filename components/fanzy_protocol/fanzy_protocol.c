#include "fanzy_protocol.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "uart_half_duplex.h"
#include <stdint.h>
#include <string.h>

static const char *TAG = "proto";

fanzy_proto_packet_t fanzy_protocol_init_packet(void) {
  return (fanzy_proto_packet_t){
      .sof = FANZY_PROTO_REQ_SOF,
      .msg_id = FANZY_PROTO_MSG_INIT,
      .length = 0,
  };
}

void fanzy_protocol_read_config(fanzy_config_t *cfg) {
  fanzy_proto_packet_t req = {
      .sof = FANZY_PROTO_REQ_SOF,
      .msg_id = FANZY_PROTO_MSG_READ_CONFIG,
  };

  ESP_LOGI(TAG, "Requesting Config");
  uart_half_duplex_write((uint8_t *)&req, sizeof(req));
  uint8_t buf[sizeof(fanzy_proto_packet_t)];
  uart_flush_input(UART_HALF_DUPLEX_PORT);

  ESP_LOGI(TAG, "Reading Config");
  uart_half_duplex_read(buf, sizeof(buf), 100);
  ESP_LOG_BUFFER_HEX(TAG, buf, sizeof(buf));
  memcpy(cfg, ((fanzy_proto_packet_t *)buf)->payload, sizeof(buf));
}

void fanzy_protocol_write_config(fanzy_config_t *cfg) {
  fanzy_proto_packet_t req = {.sof = FANZY_PROTO_REQ_SOF,
                              .msg_id = FANZY_PROTO_MSG_WRITE_CONFIG,
                              .length = sizeof(*cfg)};
  memcpy(req.payload, cfg, sizeof(*cfg));
  ESP_LOGI(TAG, "Writing Config");
  uart_half_duplex_write((uint8_t *)&req, sizeof(req));
  // uart_flush_input(UART_HALF_DUPLEX_PORT);
}

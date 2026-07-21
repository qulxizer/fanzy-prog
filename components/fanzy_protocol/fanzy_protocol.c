#include "fanzy_protocol.h"
#include "cJSON.h"
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

cJSON *fanzy_config_to_json(const fanzy_config_t *cfg) {
  if (!cfg)
    return NULL;

  cJSON *json = cJSON_CreateObject();
  if (!json)
    return NULL;
  cJSON_AddNumberToObject(json, "magic", cfg->magic);
  cJSON_AddBoolToObject(json, "temp_divider_pu", cfg->temp_divider_pu);
  cJSON_AddBoolToObject(json, "fan_pwm_inverted", cfg->fan_pwm_inverted);
  cJSON_AddBoolToObject(json, "ac_pullup", cfg->ac_pullup);
  cJSON_AddNumberToObject(json, "temp_r_fixed_ohm", cfg->temp_r_fixed_ohm);
  cJSON_AddNumberToObject(json, "temp_adc_max", cfg->temp_adc_max);
  cJSON_AddNumberToObject(json, "temp_adc_min", cfg->temp_adc_min);
  cJSON_AddNumberToObject(json, "temp_adc_short_threshold",
                          cfg->temp_adc_short_threshold);
  cJSON_AddNumberToObject(json, "temp_adc_open_threshold",
                          cfg->temp_adc_open_threshold);
  cJSON_AddNumberToObject(json, "temp_min_valid_c", cfg->temp_min_valid_c);
  cJSON_AddNumberToObject(json, "temp_max_valid_c", cfg->temp_max_valid_c);
  cJSON_AddNumberToObject(json, "fan_temp_on_c", cfg->fan_temp_on_c);
  cJSON_AddNumberToObject(json, "fan_temp_full_c", cfg->fan_temp_full_c);
  cJSON_AddNumberToObject(json, "fan_min_duty", cfg->fan_min_duty);
  cJSON_AddNumberToObject(json, "fan_max_duty", cfg->fan_max_duty);
  cJSON_AddNumberToObject(json, "ac_multiplier", cfg->ac_multiplier);
  cJSON_AddNumberToObject(json, "ac_min_speed", cfg->ac_min_speed);

  return json;
}
#include <cJSON.h>
#include <stdbool.h>

bool fanzy_json_to_config(const cJSON *json, fanzy_config_t *cfg) {
  if (!json || !cfg)
    return false;

  cJSON *item = NULL;

#define GET_INT(field)                                                         \
  if ((item = cJSON_GetObjectItemCaseSensitive(json, #field)) &&               \
      cJSON_IsNumber(item))                                                    \
    cfg->field = item->valueint;

#define GET_FLOAT(field)                                                       \
  if ((item = cJSON_GetObjectItemCaseSensitive(json, #field)) &&               \
      cJSON_IsNumber(item))                                                    \
    cfg->field = (float)item->valuedouble;

#define GET_BOOL(field)                                                        \
  if ((item = cJSON_GetObjectItemCaseSensitive(json, #field)) &&               \
      cJSON_IsBool(item))                                                      \
    cfg->field = cJSON_IsTrue(item);

  GET_INT(magic);
  GET_BOOL(temp_divider_pu);
  GET_BOOL(fan_pwm_inverted);
  GET_BOOL(ac_pullup);
  GET_INT(temp_r_fixed_ohm);
  GET_INT(temp_adc_max);
  GET_INT(temp_adc_min);
  GET_INT(temp_adc_short_threshold);
  GET_INT(temp_adc_open_threshold);
  GET_INT(temp_min_valid_c);
  GET_INT(temp_max_valid_c);
  GET_FLOAT(fan_temp_on_c);
  GET_FLOAT(fan_temp_full_c);
  GET_INT(fan_min_duty);
  GET_INT(fan_max_duty);
  GET_FLOAT(ac_multiplier);
  GET_INT(ac_min_speed);

#undef GET_INT
#undef GET_FLOAT
#undef GET_BOOL

  return true;
}

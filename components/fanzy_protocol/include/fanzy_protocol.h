#pragma once

#include "cJSON.h"
#include <stdbool.h>
#include <stdint.h>

#define FANZY_PROTO_REQ_SOF 0xAA

typedef struct __attribute__((aligned(8))) {
  int magic;
  bool temp_divider_pu;
  bool fan_pwm_inverted;
  bool ac_pullup;
  int temp_r_fixed_ohm;
  int temp_adc_max;
  int temp_adc_min;
  int temp_adc_short_threshold;
  int temp_adc_open_threshold;
  int temp_min_valid_c;
  int temp_max_valid_c;
  float fan_temp_on_c;
  float fan_temp_full_c;
  int fan_min_duty;
  int fan_max_duty;
  float ac_multiplier;
  int ac_min_speed;
} fanzy_config_t;

typedef enum {
  FANZY_PROTO_MSG_INIT = 0x67,
  FANZY_PROTO_MSG_READ_CONFIG,
  FANZY_PROTO_MSG_WRITE_CONFIG,
} fanzy_proto_msg_t;

typedef enum {

  FANZY_PROTO_OK = 0,
  FANZY_PROTO_ERR_NULL_ARG,
} fanzy_proto_status_t;

typedef struct __attribute__((packed)) {
  uint8_t sof;
  uint8_t msg_id;
  uint8_t length;
  uint8_t payload[sizeof(fanzy_config_t)];
} fanzy_proto_packet_t;

fanzy_proto_status_t fanzy_config_to_json(const fanzy_config_t *cfg,
                                          cJSON *json);
void fanzy_protocol_read_config(fanzy_config_t *cfg);
void fanzy_protocol_write_config(fanzy_config_t *cfg);
fanzy_proto_status_t fanzy_json_to_config(const cJSON *json,
                                          fanzy_config_t *cfg);
fanzy_proto_packet_t fanzy_protocol_init_packet(void);

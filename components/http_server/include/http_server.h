#pragma once

#include "esp_err.h"
#include "fanzy_protocol.h"

typedef esp_err_t (*http_server_write_callback_t)(const fanzy_config_t *config);
typedef esp_err_t (*http_server_read_callback_t)(void);

esp_err_t http_server_start(http_server_read_callback_t read_callback,
                           http_server_write_callback_t write_callback);
void http_server_update_config(const fanzy_config_t *config);

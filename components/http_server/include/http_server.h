#pragma once

#include "esp_err.h"
#include "fanzy_protocol.h"

esp_err_t http_server_start(void);
void http_server_update_config(const fanzy_config_t *config);

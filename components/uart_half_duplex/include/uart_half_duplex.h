#pragma once

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

esp_err_t uart_half_duplex_init(void);
esp_err_t uart_half_duplex_write(const uint8_t *data, size_t len);
int uart_half_duplex_read(uint8_t *buffer, size_t max_len, uint32_t timeout_ms);

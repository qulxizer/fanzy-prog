#pragma once

#include "esp_err.h"
#include "hal/uart_types.h"
#include <stddef.h>
#include <stdint.h>

enum {
  UART_HALF_DUPLEX_PORT = UART_NUM_1,
  UART_HALF_DUPLEX_PIN = 4,
  UART_HALF_DUPLEX_BAUDRATE = 115200,
  UART_HALF_DUPLEX_RX_BUFFER_SIZE = 512,
  UART_HALF_DUPLEX_TX_TIMEOUT_MS = 100,
};

esp_err_t uart_half_duplex_init(void);
esp_err_t uart_half_duplex_write(const uint8_t *data, size_t len);
int uart_half_duplex_read(uint8_t *buffer, size_t max_len, uint32_t timeout_ms);

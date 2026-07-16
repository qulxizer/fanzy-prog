#include "driver/gpio.h"
#include "driver/uart.h"

#define UART_PORT UART_NUM_1
#define UART_PIN 4
#define BAUDRATE 115200

static void uart_half_duplex_init(void) {
  uart_config_t cfg = {
      .baud_rate = BAUDRATE,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };

  ESP_ERROR_CHECK(uart_driver_install(UART_PORT,
                                      256, // RX buffer
                                      0,   // TX buffer
                                      0, NULL, 0));

  ESP_ERROR_CHECK(uart_param_config(UART_PORT, &cfg));

  // TX and RX routed to the same GPIO
  ESP_ERROR_CHECK(uart_set_pin(UART_PORT,
                               UART_PIN, // TX
                               UART_PIN, // RX
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

static esp_err_t uart_hd_write(const uint8_t *data, size_t len) {
  // Stop receiving our own transmission.
  ESP_ERROR_CHECK(uart_disable_rx_intr(UART_PORT));

  int written = uart_write_bytes(UART_PORT, data, len);
  if (written < 0) {
    uart_enable_rx_intr(UART_PORT);
    return ESP_FAIL;
  }

  // Critical: wait until the final stop bit leaves the pin.
  ESP_ERROR_CHECK(uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(100)));

  uart_flush_input(UART_PORT);
  ESP_ERROR_CHECK(uart_enable_rx_intr(UART_PORT));

  return ESP_OK;
}
static int uart_hd_read(uint8_t *buf, size_t max_len, uint32_t timeout_ms) {
  return uart_read_bytes(UART_PORT, buf, max_len, pdMS_TO_TICKS(timeout_ms));
}

void app_main(void) {
  uart_half_duplex_init();

  uint8_t tx[] = {0xAA, 0x01, 0x55};
  uint8_t rx[64];

  while (1) {
    uart_hd_write(tx, sizeof(tx));

    int len = uart_hd_read(rx, sizeof(rx), 100);

    if (len > 0) {
      ESP_LOG_BUFFER_HEX("UART", rx, len);
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

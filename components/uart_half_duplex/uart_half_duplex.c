#include "uart_half_duplex.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"

enum {
  UART_HALF_DUPLEX_PORT = UART_NUM_1,
  UART_HALF_DUPLEX_PIN = 4,
  UART_HALF_DUPLEX_BAUDRATE = 115200,
  UART_HALF_DUPLEX_RX_BUFFER_SIZE = 256,
  UART_HALF_DUPLEX_TX_TIMEOUT_MS = 100,
};

esp_err_t uart_half_duplex_init(void) {
  const uart_config_t config = {
      .baud_rate = UART_HALF_DUPLEX_BAUDRATE,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };

  esp_err_t error = uart_driver_install(
      UART_HALF_DUPLEX_PORT, UART_HALF_DUPLEX_RX_BUFFER_SIZE, 0, 0, NULL, 0);
  if (error != ESP_OK && error != ESP_ERR_INVALID_STATE) {
    return error;
  }

  ESP_RETURN_ON_ERROR(uart_param_config(UART_HALF_DUPLEX_PORT, &config),
                      "uart_half_duplex", "failed to configure UART");
  ESP_RETURN_ON_ERROR(uart_set_pin(UART_HALF_DUPLEX_PORT, UART_HALF_DUPLEX_PIN,
                                   UART_HALF_DUPLEX_PIN, UART_PIN_NO_CHANGE,
                                   UART_PIN_NO_CHANGE),
                      "uart_half_duplex", "failed to configure UART pins");
  ESP_RETURN_ON_ERROR(gpio_od_enable(UART_HALF_DUPLEX_PIN), "uart_half_duplex",
                      "failed to enable open-drain mode");
  return gpio_pullup_en(UART_HALF_DUPLEX_PIN);
}

esp_err_t uart_half_duplex_write(const uint8_t *data, size_t len) {
  ESP_RETURN_ON_ERROR(uart_disable_rx_intr(UART_HALF_DUPLEX_PORT),
                      "uart_half_duplex", "failed to disable RX interrupt");

  int written = uart_write_bytes(UART_HALF_DUPLEX_PORT, data, len);
  if (written < 0) {
    uart_enable_rx_intr(UART_HALF_DUPLEX_PORT);
    return ESP_FAIL;
  }

  esp_err_t error = uart_wait_tx_done(
      UART_HALF_DUPLEX_PORT, pdMS_TO_TICKS(UART_HALF_DUPLEX_TX_TIMEOUT_MS));
  if (error == ESP_OK) {
    uart_flush_input(UART_HALF_DUPLEX_PORT);
  }

  esp_err_t enable_error = uart_enable_rx_intr(UART_HALF_DUPLEX_PORT);
  if (error != ESP_OK) {
    return error;
  }
  return enable_error;
}

int uart_half_duplex_read(uint8_t *buffer, size_t max_len,
                          uint32_t timeout_ms) {
  return uart_read_bytes(UART_HALF_DUPLEX_PORT, buffer, max_len,
                         pdMS_TO_TICKS(timeout_ms));
}

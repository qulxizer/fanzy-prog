#include "driver/gpio.h"
#include "driver/uart.h"
#include "hal/gpio_types.h"
#include <stdint.h>

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

  // Route TX and RX to the same GPIO pin
  ESP_ERROR_CHECK(uart_set_pin(UART_PORT,
                               UART_PIN, // TX
                               UART_PIN, // RX
                               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  gpio_od_enable(UART_PIN);
  gpio_pullup_en(UART_PIN);
}

static esp_err_t uart_hd_write(const uint8_t *data, size_t len) {
  // Stop receiving our own transmission.
  ESP_ERROR_CHECK(uart_disable_rx_intr(UART_PORT));
  // gpio_set_direction(UART_PIN, GPIO_MODE_OUTPUT);

  int written = uart_write_bytes(UART_PORT, data, len);
  if (written < 0) {
    uart_enable_rx_intr(UART_PORT);
    return ESP_FAIL;
  }

  // Critical: wait until the final stop bit leaves the pin.
  ESP_ERROR_CHECK(uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(100)));
  // gpio_set_direction(UART_PIN, GPIO_MODE_INPUT);
  uart_flush_input(UART_PORT);
  ESP_ERROR_CHECK(uart_enable_rx_intr(UART_PORT));

  return ESP_OK;
}
static int uart_hd_read(uint8_t *buf, size_t max_len, uint32_t timeout_ms) {
  return uart_read_bytes(UART_PORT, buf, max_len, pdMS_TO_TICKS(timeout_ms));
}
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
} config_t;

typedef enum : uint8_t {
  PROTO_MSG_INIT = 0x67,
  PROTO_MSG_READ_CONFIG
} proto_msg_t;

typedef struct __attribute__((packed)) {
  uint8_t sof;
  proto_msg_t msg_id;
  uint8_t length;
  uint8_t payload[sizeof(config_t)];
  // uint16_t crc; // :TODO
} proto_packet_t;

void app_main(void) {
  uart_half_duplex_init();
  proto_packet_t pkt = {0};
  pkt.sof = 0xAA;
  pkt.length = 0;
  pkt.msg_id = PROTO_MSG_INIT;

  uint8_t rx[64];

  uart_hd_write((uint8_t *)&pkt, sizeof(pkt));

  while (1) {

    int len = uart_hd_read(rx, sizeof(rx), 100);
    if (len > 0) {
      ESP_LOG_BUFFER_HEX("UART", rx, len);
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

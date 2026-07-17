#include "http_server.h"

#include "esp_check.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <stdio.h>

static const char *TAG = "http_server";
static fanzy_config_t current_config;
static SemaphoreHandle_t config_mutex;

static esp_err_t index_handler(httpd_req_t *request)
{
    fanzy_config_t config;
    if (xSemaphoreTake(config_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_FAIL;
    }
    config = current_config;
    xSemaphoreGive(config_mutex);

    char page[1800];
    int length = snprintf(
        page, sizeof(page),
        "<!doctype html><html><head><meta name=viewport content='width=device-width,initial-scale=1'>"
        "<title>Fanzy</title><style>body{font:16px sans-serif;max-width:36rem;margin:2rem auto;padding:0 1rem}"
        "dt{font-weight:bold;margin-top:.7rem}dd{margin:.1rem 0}</style></head><body>"
        "<h1>Fanzy</h1><p>Read-only device configuration</p><dl>"
        "<dt>Magic</dt><dd>%d</dd>"
        "<dt>Temperature divider pull-up</dt><dd>%s</dd>"
        "<dt>Fan PWM inverted</dt><dd>%s</dd>"
        "<dt>AC pull-up</dt><dd>%s</dd>"
        "<dt>Temperature fixed resistor</dt><dd>%d ohm</dd>"
        "<dt>Temperature ADC range</dt><dd>%d to %d</dd>"
        "<dt>Temperature valid range</dt><dd>%d to %d C</dd>"
        "<dt>Fan temperature range</dt><dd>%.1f to %.1f C</dd>"
        "<dt>Fan duty range</dt><dd>%d to %d</dd>"
        "<dt>AC multiplier</dt><dd>%.2f</dd>"
        "<dt>AC minimum speed</dt><dd>%d</dd>"
        "</dl><p><small>Values are read from the controller.</small></p></body></html>",
        config.magic,
        config.temp_divider_pu ? "yes" : "no",
        config.fan_pwm_inverted ? "yes" : "no",
        config.ac_pullup ? "yes" : "no",
        config.temp_r_fixed_ohm,
        config.temp_adc_min,
        config.temp_adc_max,
        config.temp_min_valid_c,
        config.temp_max_valid_c,
        config.fan_temp_on_c,
        config.fan_temp_full_c,
        config.fan_min_duty,
        config.fan_max_duty,
        config.ac_multiplier,
        config.ac_min_speed);

    if (length < 0 || length >= (int)sizeof(page)) {
        return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "page too large");
    }
    httpd_resp_set_type(request, "text/html; charset=utf-8");
    return httpd_resp_send(request, page, length);
}

esp_err_t http_server_start(void)
{
    config_mutex = xSemaphoreCreateMutex();
    if (config_mutex == NULL) {
        return ESP_ERR_NO_MEM;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
    ESP_RETURN_ON_ERROR(httpd_start(&server, &config), TAG,
                        "failed to start HTTP server");

    const httpd_uri_t index_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_handler,
    };
    esp_err_t error = httpd_register_uri_handler(server, &index_uri);
    if (error != ESP_OK) {
        httpd_stop(server);
        return error;
    }
    ESP_LOGI(TAG, "read-only HTTP server started");
    return ESP_OK;
}

void http_server_update_config(const fanzy_config_t *config)
{
    if (config == NULL || config_mutex == NULL) {
        return;
    }
    if (xSemaphoreTake(config_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        current_config = *config;
        xSemaphoreGive(config_mutex);
    }
}

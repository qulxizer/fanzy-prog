#include "http_server.h"

#include "esp_check.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "http_server";
static fanzy_config_t current_config;
static SemaphoreHandle_t config_mutex;
static http_server_read_callback_t read_callback;
static http_server_write_callback_t write_callback;

static bool form_value(const char *body, const char *key, char *value,
                       size_t value_size)
{
    return httpd_query_key_value(body, key, value, value_size) == ESP_OK;
}

static bool form_int(const char *body, const char *key, int *result)
{
    char value[24];
    if (!form_value(body, key, value, sizeof(value))) {
        return false;
    }
    char *end = NULL;
    long parsed = strtol(value, &end, 10);
    if (end == value || *end != '\0' || parsed < INT_MIN || parsed > INT_MAX) {
        return false;
    }
    *result = (int)parsed;
    return true;
}

static bool form_float(const char *body, const char *key, float *result)
{
    char value[24];
    if (!form_value(body, key, value, sizeof(value))) {
        return false;
    }
    char *end = NULL;
    float parsed = strtof(value, &end);
    if (end == value || *end != '\0') {
        return false;
    }
    *result = parsed;
    return true;
}

static bool form_bool(const char *body, const char *key, bool *result)
{
    int value;
    if (!form_int(body, key, &value) || (value != 0 && value != 1)) {
        return false;
    }
    *result = value != 0;
    return true;
}

static esp_err_t write_handler(httpd_req_t *request)
{
    if (write_callback == NULL || request->content_len <= 0 ||
        request->content_len >= 1024) {
        return httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST,
                                   "invalid request");
    }

    char body[1024];
    size_t total = 0;
    while (total < request->content_len) {
        int received = httpd_req_recv(request, body + total,
                                      request->content_len - total);
        if (received <= 0) {
            return httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST,
                                       "missing form data");
        }
        total += received;
    }
    body[total] = '\0';

    fanzy_config_t config;
    if (xSemaphoreTake(config_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "configuration busy");
    }
    config = current_config;
    xSemaphoreGive(config_mutex);

    bool valid = form_int(body, "magic", &config.magic) &&
                 form_bool(body, "temp_divider_pu", &config.temp_divider_pu) &&
                 form_bool(body, "fan_pwm_inverted", &config.fan_pwm_inverted) &&
                 form_bool(body, "ac_pullup", &config.ac_pullup) &&
                 form_int(body, "temp_r_fixed_ohm", &config.temp_r_fixed_ohm) &&
                 form_int(body, "temp_adc_max", &config.temp_adc_max) &&
                 form_int(body, "temp_adc_min", &config.temp_adc_min) &&
                 form_int(body, "temp_adc_short_threshold",
                         &config.temp_adc_short_threshold) &&
                 form_int(body, "temp_adc_open_threshold",
                         &config.temp_adc_open_threshold) &&
                 form_int(body, "temp_min_valid_c", &config.temp_min_valid_c) &&
                 form_int(body, "temp_max_valid_c", &config.temp_max_valid_c) &&
                 form_float(body, "fan_temp_on_c", &config.fan_temp_on_c) &&
                 form_float(body, "fan_temp_full_c", &config.fan_temp_full_c) &&
                 form_int(body, "fan_min_duty", &config.fan_min_duty) &&
                 form_int(body, "fan_max_duty", &config.fan_max_duty) &&
                 form_float(body, "ac_multiplier", &config.ac_multiplier) &&
                 form_int(body, "ac_min_speed", &config.ac_min_speed);
    if (!valid) {
        return httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST,
                                   "invalid configuration value");
    }
    ESP_RETURN_ON_ERROR(write_callback(&config), TAG, "failed to queue write");
    httpd_resp_set_status(request, "303 See Other");
    httpd_resp_set_hdr(request, "Location", "/");
    return httpd_resp_send(request, NULL, 0);
}

static esp_err_t index_handler(httpd_req_t *request)
{
    if (read_callback != NULL) {
        ESP_RETURN_ON_ERROR(read_callback(), TAG, "failed to read configuration");
    }

    fanzy_config_t config;
    if (xSemaphoreTake(config_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return ESP_FAIL;
    }
    config = current_config;
    xSemaphoreGive(config_mutex);

    char *page = malloc(3600);
    if (page == NULL) {
        return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "out of memory");
    }
    int length = snprintf(
        page, 3600,
        "<!doctype html><html><head><meta name=viewport content='width=device-width,initial-scale=1'>"
        "<title>Fanzy</title><style>body{font:16px sans-serif;max-width:36rem;margin:2rem auto;padding:0 1rem}"
        "dt{font-weight:bold;margin-top:.7rem}dd{margin:.1rem 0}</style>"
        "<style>label{display:block;margin:.7rem 0}input,select{float:right;width:11rem}"
        "button{margin-top:1rem;padding:.6rem 1rem}</style></head><body>"
        "<h1>Fanzy</h1><p>Controller configuration</p><form method=post action=/config>"
        "<label>Magic <input name=magic type=number value=%d></label>"
        "<label>Temperature divider pull-up <select name=temp_divider_pu>"
        "<option value=0%s>No</option><option value=1%s>Yes</option></select></label>"
        "<label>Fan PWM inverted <select name=fan_pwm_inverted>"
        "<option value=0%s>No</option><option value=1%s>Yes</option></select></label>"
        "<label>AC pull-up <select name=ac_pullup>"
        "<option value=0%s>No</option><option value=1%s>Yes</option></select></label>"
        "<label>Temperature fixed resistor (ohm) <input name=temp_r_fixed_ohm type=number value=%d></label>"
        "<label>Temperature ADC max <input name=temp_adc_max type=number value=%d></label>"
        "<label>Temperature ADC min <input name=temp_adc_min type=number value=%d></label>"
        "<label>ADC short threshold <input name=temp_adc_short_threshold type=number value=%d></label>"
        "<label>ADC open threshold <input name=temp_adc_open_threshold type=number value=%d></label>"
        "<label>Minimum valid temperature (C) <input name=temp_min_valid_c type=number value=%d></label>"
        "<label>Maximum valid temperature (C) <input name=temp_max_valid_c type=number value=%d></label>"
        "<label>Fan start temperature (C) <input name=fan_temp_on_c type=number step=0.1 value=%.1f></label>"
        "<label>Fan full temperature (C) <input name=fan_temp_full_c type=number step=0.1 value=%.1f></label>"
        "<label>Fan minimum duty <input name=fan_min_duty type=number value=%d></label>"
        "<label>Fan maximum duty <input name=fan_max_duty type=number value=%d></label>"
        "<label>AC multiplier <input name=ac_multiplier type=number step=0.01 value=%.2f></label>"
        "<label>AC minimum speed <input name=ac_min_speed type=number value=%d></label>"
        "<button type=submit>Write configuration</button></form>"
        "<p><small>GET displays the latest controller values. POST writes the form.</small></p></body></html>",
        config.magic,
        config.temp_divider_pu ? "" : " selected",
        config.temp_divider_pu ? " selected" : "",
        config.fan_pwm_inverted ? "" : " selected",
        config.fan_pwm_inverted ? " selected" : "",
        config.ac_pullup ? "" : " selected",
        config.ac_pullup ? " selected" : "",
        config.temp_r_fixed_ohm,
        config.temp_adc_max,
        config.temp_adc_min,
        config.temp_adc_short_threshold,
        config.temp_adc_open_threshold,
        config.temp_min_valid_c,
        config.temp_max_valid_c,
        config.fan_temp_on_c,
        config.fan_temp_full_c,
        config.fan_min_duty,
        config.fan_max_duty,
        config.ac_multiplier,
        config.ac_min_speed);

    if (length < 0 || length >= 3600) {
        free(page);
        return httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR,
                                   "page too large");
    }
    httpd_resp_set_type(request, "text/html; charset=utf-8");
    esp_err_t error = httpd_resp_send(request, page, length);
    free(page);
    return error;
}

esp_err_t http_server_start(http_server_read_callback_t read,
                            http_server_write_callback_t write)
{
    read_callback = read;
    write_callback = write;
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
    const httpd_uri_t config_uri = {
        .uri = "/config",
        .method = HTTP_POST,
        .handler = write_handler,
    };
    esp_err_t error = httpd_register_uri_handler(server, &index_uri);
    if (error != ESP_OK) {
        httpd_stop(server);
        return error;
    }
    error = httpd_register_uri_handler(server, &config_uri);
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

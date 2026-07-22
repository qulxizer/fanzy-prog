#include "http_server.h"
#include "cJSON.h"
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "fanzy_protocol.h"
#include "freertos/semphr.h"
#include "http_parser.h"
#include <limits.h>

typedef struct cJSON {
  struct cJSON *next;
  struct cJSON *prev;
  struct cJSON *child;
  int type;
  char *valuestring;
  int valueint;
  double valuedouble;
  char *string;
} cJSON;

static const char *TAG = "http_server";
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");

static esp_err_t handle_get_config(httpd_req_t *req) {
  ESP_LOGD(TAG, "handle_get_config");
  if (req->method != HTTP_GET) {
    ESP_LOGE(TAG, "Only GET is supported on this endpoint");
    return httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED,
                               "Only GET is supported on this endpoint");
  }
  fanzy_config_t cfg = {
      .magic = 0x696969, .fan_pwm_inverted = 1, .temp_r_fixed_ohm = 1000};

  cJSON *jcfg = cJSON_CreateObject();
  fanzy_proto_status_t st = fanzy_config_to_json(&cfg, jcfg);
  if (st != FANZY_PROTO_OK) {
    ESP_LOGE(TAG, "Failed to convert config to json");
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                               "Failed to convert config to json");
  }
  char *scfg = cJSON_Print(jcfg);

  cJSON_Delete(jcfg);
  return httpd_resp_send(req, scfg, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t handle_post_config(httpd_req_t *req) {
  ESP_LOGD(TAG, "handle_post_config");

  if (req->method != HTTP_POST) {
    ESP_LOGE(TAG, "Only POST is supported on this endpoint");
    return httpd_resp_send_err(req, HTTPD_405_METHOD_NOT_ALLOWED,
                               "Only POST is supported on this endpoint");
  }

  int total_len = req->content_len;
  if (total_len >= 1024) {

    ESP_LOGE(TAG, "Payload too large");
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Payload too large");
  }

  char buf[1024] = {0};
  int cur_len = 0;

  while (cur_len < total_len) {
    int received = httpd_req_recv(req, buf + cur_len, total_len - cur_len);
    if (received <= 0) {
      if (received == HTTPD_SOCK_ERR_TIMEOUT)
        continue;
      return httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                                 "Receive failed");
    }
    cur_len += received;
  }
  buf[cur_len] = '\0';

  cJSON *jcfg = cJSON_Parse(buf);
  if (!jcfg) {
    ESP_LOGE(TAG, "Failed to parse json");
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
  }

  fanzy_config_t cfg = {0};
  fanzy_proto_status_t st = fanzy_json_to_config(jcfg, &cfg);
  if (st != FANZY_PROTO_OK) {
    ESP_LOGE(TAG, "Failed to convert json to config");
    return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Bad Request");
  }
  cJSON_Delete(jcfg);

  ESP_LOGI(TAG, "Magic: %d", cfg.magic);
  return httpd_resp_send(req, "Success Post", HTTPD_RESP_USE_STRLEN);
}

static esp_err_t handle_index(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");

  size_t index_html_len = index_html_end - index_html_start;
  httpd_resp_send(req, (const char *)index_html_start, index_html_len);
  return ESP_OK;
}

httpd_handle_t start_web_server(void) {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  httpd_handle_t server = NULL;

  if (httpd_start(&server, &config) == ESP_OK) {
    ESP_LOGI(TAG, "HTTP server started on port %d", config.server_port);
    ESP_LOGI(TAG, "HTTP server started on ip %d", config.if_name);

    httpd_uri_t index_get = {.uri = "/",
                             .method = HTTP_GET,
                             .handler = handle_index,
                             .user_ctx = NULL};

    httpd_uri_t config_get = {.uri = "/config",
                              .method = HTTP_GET,
                              .handler = handle_get_config,
                              .user_ctx = NULL};

    httpd_uri_t config_post = {.uri = "/config",
                               .method = HTTP_POST,
                               .handler = handle_post_config,
                               .user_ctx = NULL};

    httpd_register_uri_handler(server, &config_post);
    httpd_register_uri_handler(server, &index_get);
    httpd_register_uri_handler(server, &config_get);
    return server;
  }

  ESP_LOGE(TAG, "Failed to start HTTP server");
  return NULL;
}

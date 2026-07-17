#include "wifi_access_point.h"

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include <string.h>

static const char *TAG = "wifi_access_point";

esp_err_t wifi_access_point_start(void)
{
#if !CONFIG_FANZY_WIFI_AP_ENABLED
    ESP_LOGI(TAG, "disabled by Kconfig");
    return ESP_OK;
#else
    if (CONFIG_FANZY_WIFI_AP_SSID[0] == '\0') {
        ESP_LOGE(TAG, "access point SSID is not configured");
        return ESP_ERR_INVALID_ARG;
    }
    if (CONFIG_FANZY_WIFI_AP_PASSWORD[0] != '\0' &&
        strlen(CONFIG_FANZY_WIFI_AP_PASSWORD) < 8) {
        ESP_LOGE(TAG, "WPA2 password must contain at least 8 characters");
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t error = nvs_flash_init();
    if (error == ESP_ERR_NVS_NO_FREE_PAGES || error == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "failed to erase NVS");
        error = nvs_flash_init();
    }
    ESP_RETURN_ON_ERROR(error, TAG, "failed to initialize NVS");
    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "failed to initialize network interface");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG,
                        "failed to create default event loop");

    esp_netif_create_default_wifi_ap();
    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&init_config), TAG, "failed to initialize Wi-Fi");

    wifi_config_t wifi_config = {0};
    strlcpy((char *)wifi_config.ap.ssid, CONFIG_FANZY_WIFI_AP_SSID,
            sizeof(wifi_config.ap.ssid));
    strlcpy((char *)wifi_config.ap.password, CONFIG_FANZY_WIFI_AP_PASSWORD,
            sizeof(wifi_config.ap.password));
    wifi_config.ap.ssid_len = strlen(CONFIG_FANZY_WIFI_AP_SSID);
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.authmode = CONFIG_FANZY_WIFI_AP_PASSWORD[0] == '\0'
                                  ? WIFI_AUTH_OPEN
                                  : WIFI_AUTH_WPA2_PSK;
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_AP), TAG,
                        "failed to set access point mode");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &wifi_config), TAG,
                        "failed to set access point configuration");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "failed to start access point");
    ESP_LOGI(TAG, "access point started: %s", CONFIG_FANZY_WIFI_AP_SSID);
    return ESP_OK;
#endif
}

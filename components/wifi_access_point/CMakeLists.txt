idf_component_register(
    SRCS "wifi_access_point.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_event esp_netif esp_wifi nvs_flash esp_eth
)

idf_component_register(SRCS "main.c"
                       PRIV_REQUIRES fanzy_protocol http_server uart_half_duplex wifi_access_point
                       INCLUDE_DIRS ".")

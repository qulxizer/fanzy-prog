idf_component_register(
    SRCS "http_server.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_server fanzy_protocol
    EMBED_FILES "${CMAKE_CURRENT_LIST_DIR}/../../web/dist/index.html"
)

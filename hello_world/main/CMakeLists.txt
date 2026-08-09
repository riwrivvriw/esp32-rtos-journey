idf_component_register(SRCS "wifiHost.c" "hello_world_main.c"
                       PRIV_REQUIRES spi_flash esp_driver_gpio esp_wifi nvs_flash esp_netif esp_event
                       INCLUDE_DIRS "")
idf_component_register(
    SRCS "main.c"
         "RGB/RGB.c"
         "Wifi/Wifi.c"
         "Button/Button.c"
    INCLUDE_DIRS "."
                 "./RGB"
                 "./managed_components/espressif__led_strip/include"
                 "./Wifi"
                 "./Button"
                 "./Website"
    REQUIRES led_strip esp_wifi esp_netif esp_event nvs_flash esp_http_client esp_http_server
)
#include "RGB.h"
#include "Wifi.h"
#include "Button.h"
#include "stdio.h"
#include "string.h"
#include "esp_log.h"

uint8_t ssid[] = { 's', 'i', 'm', 0xC3, 0xB8, 'n', '\0' };
uint8_t password[] = "12345678";

wifi_settings_t my_wifi = {
    .mode = WIFI_CONFIG_MODE_STATIC,
    .static_ip = "172.20.10.5",     
    .gateway = "172.20.10.1",        
    .netmask = "255.255.255.0",     
    .dns = "8.8.8.8"                 
};

int app_main(void){

    RGB_Init();
    Button_Init();

    RGB_Mode(check);
    vTaskDelay(pdMS_TO_TICKS(200));
    RGB_Mode(pulse);

    strncpy(my_wifi.ssid, (char *)ssid, sizeof(my_wifi.ssid) - 1);
    strncpy(my_wifi.password, (char *)password, sizeof(my_wifi.password) - 1);

    do {
        if (button_pressed()){
            RGB_Mode(flash_white);
            if (wifi_init(&my_wifi) == ESP_OK){
                char ip[16];
                if (wifi_get_ip(ip, sizeof(ip))== ESP_OK){
                    ESP_LOGI("Main", "IP-Address: %s", ip);
                    if (start_webserver() == ESP_OK){
                        set_html_page("<h1>Hallo ESP32</h1><p>Diese Seite kommt aus dem Programm!</p>");
                        RGB_Mode(green);
                    }
                }
                else{
                    ESP_LOGW("Main", "Couldn't get IP-Address.");
                    RGB_Mode(red);
                }
            }
            else{
                ESP_LOGW("Main", "Wifi couldn't be started.");
                RGB_Mode(red);
            }
            vTaskDelay(pdMS_TO_TICKS(5000));
            RGB_Mode(rainbow);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    } while (1);
    
    return 0;
}
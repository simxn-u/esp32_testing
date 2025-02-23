#pragma once

#include "esp_err.h"

// Vermeidet Konflikt mit ESP-IDF: Umbenennung von `wifi_mode_t` → `wifi_config_mode_t`
typedef enum {
    WIFI_CONFIG_MODE_DHCP,
    WIFI_CONFIG_MODE_STATIC
} wifi_config_mode_t;

// Vermeidet Konflikt mit ESP-IDF: Umbenennung von `wifi_config_t` → `wifi_settings_t`
typedef struct {
    char ssid[32];
    char password[64];
    wifi_config_mode_t mode; // DHCP oder Statische IP
    char static_ip[16];
    char gateway[16];
    char netmask[16];
    char dns[16];
} wifi_settings_t;

esp_err_t wifi_init(const wifi_settings_t *config);
esp_err_t wifi_get_ip(char *result, size_t len);

esp_err_t start_webserver();  // Startet den Webserver
void set_html_page(const char *html); 
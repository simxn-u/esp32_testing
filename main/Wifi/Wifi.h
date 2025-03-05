#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    WIFI_MODE_DHCP = 0,
    WIFI_MODE_STATIC
} wifi_config_mode_t;

typedef struct {
    char ssid[32];
    char password[64];
    wifi_config_mode_t mode; // DHCP or static IP
    char static_ip[16];
    char gateway[16];
    char netmask[16];
    char dns[16];
} wifi_settings_t;

#ifdef __cplusplus
extern "C" {
#endif

void wifi_init(const wifi_settings_t *settings);

void wifi_connect(const wifi_settings_t *settings);

void wifi_serve_website(const char *html_file_path);

bool wifi_fetch_api(const char *url, char *response_buffer, size_t buffer_size);

void wifi_disconnect(void);

#ifdef __cplusplus
}
#endif

#endif // WIFI_H

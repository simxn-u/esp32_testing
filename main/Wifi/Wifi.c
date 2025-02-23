#include "wifi.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "string.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t wifi_event_group;
static const char *TAG = "WiFi";
static int retry_count = 0;
static char ip_address[16] = "0.0.0.0";
#define MAXIMUM_RETRY 5

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (retry_count < MAXIMUM_RETRY) {
            esp_wifi_connect();
            retry_count++;
            ESP_LOGI(TAG, "Verbindung erneut versuchen (%d/%d)", retry_count, MAXIMUM_RETRY);
        } else {
            xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        snprintf(ip_address, sizeof(ip_address), IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Verbunden mit IP: %s", ip_address);
        retry_count = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_init(const wifi_settings_t *config) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_event_group = xEventGroupCreate();
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();

    if (config->mode == WIFI_CONFIG_MODE_STATIC) {
        esp_netif_ip_info_t ip_info;
        esp_netif_str_to_ip4(config->static_ip, &ip_info.ip);
        esp_netif_str_to_ip4(config->gateway, &ip_info.gw);
        esp_netif_str_to_ip4(config->netmask, &ip_info.netmask);
        ESP_ERROR_CHECK(esp_netif_dhcpc_stop(sta_netif));
        ESP_ERROR_CHECK(esp_netif_set_ip_info(sta_netif, &ip_info));
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_cfg = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strncpy((char *)wifi_cfg.sta.ssid, config->ssid, sizeof(wifi_cfg.sta.ssid) - 1);
    strncpy((char *)wifi_cfg.sta.password, config->password, sizeof(wifi_cfg.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WLAN wird gestartet...");

    EventBits_t bits = xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WLAN-Verbindung erfolgreich!");
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "WLAN-Verbindung fehlgeschlagen!");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t wifi_get_ip(char *result, size_t len) {
    if (strlen(ip_address) > 0) {
        strncpy(result, ip_address, len - 1);
        result[len - 1] = '\0';
        return ESP_OK;
    }
    return ESP_FAIL;
}

static const char *html_page = "<h1>Standard HTML</h1>";  // Standardseite

// Funktion zum Setzen einer neuen HTML-Seite
void set_html_page(const char *html) {
    html_page = html;
    ESP_LOGI(TAG, "Neue HTML-Seite gesetzt!");
}

// HTTP GET-Handler, um HTML-Seite zu liefern
esp_err_t get_handler(httpd_req_t *req) {
    httpd_resp_send(req, html_page, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Webserver starten
esp_err_t start_webserver() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_uri_t page_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &page_uri);
    }

    ESP_LOGI(TAG, "Webserver gestartet!");
    return ESP_OK;
}

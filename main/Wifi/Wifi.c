#include "Wifi.h"
#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_http_client.h"
#include "esp_http_server.h"

static const char *TAG = "wifi";

#define API_KEY "cuuds91r01qlidi3enc0cuuds91r01qlidi3encg"
char symbol[] = "NVDA";

static int s_retry_num = 0;
#define MAX_RETRY_QUICK 5

static bool reconnect_enabled = true;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi started - initiating connection");
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                if (!reconnect_enabled){
                    ESP_LOGI(TAG, "Automatic reconnection disabled. No further connection attempts.");
                    break;
                }

                ESP_LOGW(TAG, "WiFi connection lost - retrying...");
                if (s_retry_num < MAX_RETRY_QUICK) {
                    s_retry_num++;
                    esp_wifi_connect();
                } else {
                    ESP_LOGW(TAG, "Maximum quick attempts reached, retrying in 10 seconds");
                    esp_wifi_connect();
                    vTaskDelay(pdMS_TO_TICKS(10000));
                }
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "Successfully connected to WiFi");
                reconnect_enabled = true;
                s_retry_num = 0;
                break;
            default:
                break;
        }
    }
}

void wifi_init(const wifi_settings_t *settings) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    ESP_EVENT_ANY_ID,
                    &wifi_event_handler,
                    NULL,
                    NULL));

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, settings->ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, settings->password, sizeof(wifi_config.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    if (settings->mode == WIFI_MODE_STATIC) {
        esp_netif_ip_info_t ip_info;

        ip4addr_aton(settings->static_ip, (ip4_addr_t *)&ip_info.ip);
        ip4addr_aton(settings->gateway, (ip4_addr_t *)&ip_info.gw);
        ip4addr_aton(settings->netmask, (ip4_addr_t *)&ip_info.netmask); 

        esp_netif_dhcpc_stop(sta_netif);
        esp_netif_set_ip_info(sta_netif, &ip_info);
    }

    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "WiFi initialized");
}

void wifi_connect(const wifi_settings_t *settings) {
    reconnect_enabled = true;
    s_retry_num = 0;
    ESP_LOGI(TAG, "Attempting connection to SSID: %s", settings->ssid);
    esp_wifi_connect();
}

void wifi_disconnect(void) {
    reconnect_enabled = false;
    esp_wifi_disconnect();
    vTaskDelay(pdMS_TO_TICKS(200));
    ESP_LOGI(TAG, "WiFi disconnected and automatic reconnection disabled.");
}

// HTTP-GET Handler zum Ausliefern der HTML-Datei
static esp_err_t website_get_handler(httpd_req_t *req) {
    // Der übergebene Kontext enthält den Pfad zur HTML-Datei
    const char *html_path = (const char *)req->user_ctx;
    FILE *file = fopen(html_path, "r");
    if (!file) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Error opening the HTML file");
        return ESP_FAIL;
    }

    char buffer[1024];
    size_t nread;
    httpd_resp_set_type(req, "text/html");
    while ((nread = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        httpd_resp_send_chunk(req, buffer, nread);
    }
    fclose(file);
    // Signalisieren, dass keine weiteren Daten folgen
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

void wifi_serve_website(const char *html_file_path) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // URI-Handler für die Startseite
    httpd_uri_t uri_get = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = website_get_handler,
        .user_ctx = (void *)html_file_path
    };

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &uri_get);
        ESP_LOGI(TAG, "HTTP server started - website available at the static IP");
    } else {
        ESP_LOGE(TAG, "Error starting the HTTP server");
    }
}

bool wifi_fetch_api(const char *url, char *response_buffer, size_t buffer_size) {
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 5000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "HTTP connection could not be initialized");
        return false;
    }
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int read_len = esp_http_client_read_response(client, response_buffer, buffer_size);
        if (read_len >= 0) {
            // Abschließen der Zeichenkette
            response_buffer[read_len] = 0;
            ESP_LOGI(TAG, "API-Antwort erhalten (Statuscode: %d)",
                     esp_http_client_get_status_code(client));
            esp_http_client_cleanup(client);
            return true;
        }
    }
    ESP_LOGE(TAG, "Fehler bei der API-Anfrage: %s", esp_err_to_name(err));
    esp_http_client_cleanup(client);
    return false;
}
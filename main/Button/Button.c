#include <stdio.h>
#include "driver/gpio.h"
#include "esp_log.h"

#define BOOT_BUTTON_GPIO 9

static const char *TAG = "BOOT_BUTTON";

void Button_Init() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BOOT_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    ESP_LOGI(TAG, "GPIO %d initialized!", BOOT_BUTTON_GPIO);
}

bool button_pressed() {
    return gpio_get_level(BOOT_BUTTON_GPIO) == 0;  
}

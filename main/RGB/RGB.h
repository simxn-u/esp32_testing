#pragma once

#include "driver/gpio.h"
#include "led_strip.h"

#define BLINK_GPIO 8

typedef enum {
    check,
    red,
    blue,
    green,
    white,
    off,
    rainbow,
    pulse,
    flash_white
} Mode;

void RGB_Init(void);
void RGB_Mode(Mode mode);

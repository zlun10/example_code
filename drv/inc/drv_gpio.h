#ifndef _DRV_GPIO_H
#define _DRV_GPIO_H

#include <stdint.h>
#include "cw32l011_gpio.h"

typedef enum {
    GPIO_ROW1,
    GPIO_ROW2,
    GPIO_ROW3,
    GPIO_ROW4,
    GPIO_COLUMN1,
    GPIO_COLUMN2,
    GPIO_COLUMN3,
    GPIO_COLUMN4,
    GPIO_MATRIX_NUM,
    GPIO_PTT = GPIO_MATRIX_NUM,
    GPIO_POWER_IN,
    GPIO_POWER_OUT,
    GPIO_RST,
    GPIO_LED,
    GPIO_AT_ISR,
    GPIO_LCD_VCC,
    GPIO_LCD_RST,
    GPIO_LCD_DC,
    GPIO_NUM,
} Gpio_e;

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    Gpio_e keyName;
} Gpio_map_t;

typedef enum {
    GPIO_RESET = 0,
    GPIO_SET,
} GpioSta_e;

typedef enum {
    GPIO_MODE_IN = 0,
    GPIO_MODE_OUT,
} GpioMode_e;

void gpio_modecfg(uint8_t io, GpioMode_e mode);
void gpio_deinit(uint8_t io);
void gpio_set(uint8_t io, GpioSta_e sta);
GpioSta_e gpio_get(uint8_t io);

#endif

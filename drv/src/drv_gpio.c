#include <stdint.h>
#include <stdbool.h>
#include "../inc/drv_gpio.h"
#include "../../lib/misc.h"

static Gpio_map_t gpio_map[] = {
    {CW_GPIOA, GPIO_PIN_0, GPIO_ROW1},
    {CW_GPIOA, GPIO_PIN_1, GPIO_ROW2},
    {CW_GPIOA, GPIO_PIN_2, GPIO_ROW3},
    {CW_GPIOA, GPIO_PIN_3, GPIO_ROW4},
    {CW_GPIOA, GPIO_PIN_4, GPIO_COLUMN1},
    {CW_GPIOA, GPIO_PIN_5, GPIO_COLUMN2},
    {CW_GPIOA, GPIO_PIN_6, GPIO_COLUMN3},
    {CW_GPIOA, GPIO_PIN_7, GPIO_COLUMN4},

    {CW_GPIOB, GPIO_PIN_7,  GPIO_PTT},
    {CW_GPIOC, GPIO_PIN_13, GPIO_POWER_IN},
    {CW_GPIOA, GPIO_PIN_8,  GPIO_POWER_OUT},
    {CW_GPIOB, GPIO_PIN_1,  GPIO_RST},
    {CW_GPIOA, GPIO_PIN_14, GPIO_LED},
    {CW_GPIOB, GPIO_PIN_0,  GPIO_AT_ISR},
    {CW_GPIOB, GPIO_PIN_6,  GPIO_LCD_VCC},
    {CW_GPIOB, GPIO_PIN_4,  GPIO_LCD_RST},
    {CW_GPIOA, GPIO_PIN_12, GPIO_LCD_DC},

};

void gpio_modecfg(uint8_t io, GpioMode_e mode)
{
    if(io >= ARRAY_SIZE(gpio_map)) {
        return;
    }
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.IT = GPIO_IT_NONE;
    if(mode == GPIO_MODE_IN)
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT_PULLUP;
    else
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pins = gpio_map[io].pin;
    GPIO_Init(gpio_map[io].port, &GPIO_InitStruct);
}

void gpio_set(uint8_t io, GpioSta_e sta)
{
    if(io >= ARRAY_SIZE(gpio_map)) {
        return;
    }
    GPIO_WritePin(gpio_map[io].port, gpio_map[io].pin, (GPIO_PinState) sta);
}

GpioSta_e gpio_get(uint8_t io)
{
    if(io >= ARRAY_SIZE(gpio_map)) {
        return GPIO_SET;
    }
    GPIO_PinState pin_state = GPIO_Pin_RESET;
    pin_state = GPIO_ReadPin(gpio_map[io].port, gpio_map[io].pin);
    return (GpioSta_e) pin_state;
}

void gpio_deinit(uint8_t io)
{
    if(io >= ARRAY_SIZE(gpio_map)) {
        return;
    }
    GPIO_DeInit(gpio_map[io].port, gpio_map[io].pin);
}

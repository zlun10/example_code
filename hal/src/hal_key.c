#include <stdint.h>
#include <stdbool.h>
#include "../inc/hal_key.h"
#include "../../drv/inc/drv_gpio.h"
#include "../../dev/inc/matrix.h"

void hal_key_init(void)
{
    matrix_init();

    // single
    gpio_modecfg(GPIO_PTT, GPIO_MODE_IN);
    gpio_modecfg(GPIO_POWER_IN, GPIO_MODE_IN);
}

void hal_key_deInit(void)
{
    matrix_deInit();

    // single
    gpio_deinit(GPIO_PTT);
    gpio_deinit(GPIO_POWER_OUT);
    gpio_deinit(GPIO_RST);
}

uint8_t hal_key_getLevel(KEY_e key_num)
{
    if(key_num >= KEY_MATRIX_NUM) {
        return gpio_get(key_num - GPIO_MATRIX_NUM);
    }
    return matrixGet((M_key_e) key_num);
}

void hal_key_scan(void)
{
    matrixScan();
}

/**
 * @brief 行列按键
 */
#include <stdint.h>
#include <stdbool.h>
#include "../inc/matrix.h"
#include "../../drv/inc/drv_gpio.h"

#define GPIO_ROW_NUM (GPIO_MATRIX_NUM/2)
#define GPIO_COL_NUM (GPIO_MATRIX_NUM/2)

typedef struct
{
    KEY_SCAN_S sta;
    bool ready;// 首次扫描完成 1：完成
    uint8_t cur_row;
    uint8_t level[M_KEY_NUM];
} Matrix_cxt_t;

static Matrix_cxt_t matrix_cxt;

Matrix_cxt_t *get_matrix_cxt(void)
{
    return &matrix_cxt;
}

void matrix_init(void)
{
    for(uint8_t i = 0; i < GPIO_ROW_NUM; i++) {
        gpio_modecfg(i, GPIO_MODE_OUT);
        gpio_set(i, GPIO_SET);
    }

    for(uint8_t i = GPIO_ROW_NUM; i < GPIO_COL_NUM + GPIO_ROW_NUM; i++) {
        gpio_modecfg(i, GPIO_MODE_IN);
    }

    //var
    memset(&matrix_cxt, 0, sizeof(matrix_cxt));
}

void matrix_deInit(void)
{
    for(uint8_t i = 0; i < GPIO_COL_NUM + GPIO_ROW_NUM; i++) {
        gpio_deinit(i);
    }
    memset(&matrix_cxt, 0, sizeof(matrix_cxt));
}

void matrixScan(void)
{
    Matrix_cxt_t *cxt = get_matrix_cxt();
    switch(cxt->sta)
    {
    case KEY_INPUT:
        if(cxt->cur_row > 0) {
            gpio_set(cxt->cur_row - 1, GPIO_SET);
        } else if(cxt->ready) {
            gpio_set(GPIO_ROW_NUM - 1, GPIO_SET);
        }

        gpio_set(cxt->cur_row, GPIO_RESET);
        cxt->sta = KEY_WAIT;
        break;

    case KEY_WAIT:
        cxt->sta = KEY_OUTPUT;
        break;

    case KEY_OUTPUT:
        for(uint8_t col = 0; col < GPIO_COL_NUM; col++)
        {
            uint8_t key_index = col + cxt->cur_row * GPIO_COL_NUM;
            uint8_t col_index = col + GPIO_ROW_NUM;

            GpioSta_e pin_state = gpio_get(col_index);
            cxt->level[key_index] = (pin_state == GPIO_RESET) ? 0 : 1;
        }

        cxt->cur_row++;
        if(cxt->cur_row >= GPIO_ROW_NUM) {
            cxt->cur_row = 0;
            cxt->ready = 1;  // 完成首次扫描
        }

        cxt->sta = KEY_INPUT;
        break;

    default:
        cxt->sta = KEY_INPUT;
        cxt->cur_row = 0;
        break;
    }
}

uint8_t matrixGet(M_key_e key_num)
{
    Matrix_cxt_t *cxt = get_matrix_cxt();
    if(cxt->ready)
        return cxt->level[key_num];
    else
        return 1;
}


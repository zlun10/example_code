#include "../inc/drv_spi.h"
#include "../inc/drv_timer.h"
#include "cw32l011_spi.h"
#include "cw32l011_gpio.h"
#include "cw32l011_sysctrl.h"

void drv_spi_init(void)
{
    // rcc
    SYSCTRL_APBPeriphClk_Enable1(SYSCTRL_APB1_PERIPH_SPI1, ENABLE);

    // af gpio
    GPIO_InitTypeDef GPIO_InitStructure = { 0 };
    GPIO_InitStructure.Pins = GPIO_PIN_3 | GPIO_PIN_5;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Init(CW_GPIOB, &GPIO_InitStructure);
    GPIO_InitStructure.Pins = GPIO_PIN_15;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Init(CW_GPIOA, &GPIO_InitStructure);

    PA15_SETHIGH();
    PB03_AFx_SPI1SCK();
    PB05_AFx_SPI1MOSI();

    // spi
    SPI_InitTypeDef SPI_InitStructure = { 0 };
    SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_TxOnly;         // 单工单发
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;                         // 主机模式
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;                     // 帧数据长度为8bit
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;                           // 时钟空闲电平为高
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;                          // 第二个边沿采样
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;                             // 片选信号由SSI寄存器控制
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;    // 波特率为PCLK的8分频
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;                    // 最高有效位 MSB 收发在前
    SPI_InitStructure.SPI_Speed = SPI_Speed_High;
    SPI_Init(CW_SPI, &SPI_InitStructure);
    SPI_Cmd(CW_SPI, ENABLE);
}

void drv_spi_deInit(void)
{
    SPI1_DeInit();
    SYSCTRL_APBPeriphClk_Enable1(SYSCTRL_APB1_PERIPH_SPI1, DISABLE);
    GPIO_DeInit(CW_GPIOB, GPIO_PIN_3 | GPIO_PIN_5);
    GPIO_DeInit(CW_GPIOA, GPIO_PIN_15);
}

void drv_spi_send(uint8_t data)
{
    uint32_t timeout;
    PA15_SETLOW();

    timeout = 10000;
    while(SPI_GetFlagStatus(CW_SPI, SPI_FLAG_TXE) == RESET)
    {
        if(--timeout == 0) {
            PA15_SETHIGH();  // 超时则释放片选
            return;
        }
    }

    SPI_SendData(CW_SPI, data);

    timeout = 10000;
    while(SPI_GetFlagStatus(CW_SPI, SPI_FLAG_BUSY) == SET)
    {
        if(--timeout == 0) {
            PA15_SETHIGH();  // 超时则释放片选
            return;
        }
    }

    PA15_SETHIGH();
}



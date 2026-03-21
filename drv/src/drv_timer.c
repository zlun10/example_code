#include "../inc/drv_timer.h"
#include "cw32l011_btim.h"
#include "cw32l011_sysctrl.h"

#define getTick() ((uint16_t)(CW_BTIM1->CNT & 0xFFFF))

void timer_init(void)
{
    BTIM_TimeBaseInitTypeDef BTIM_TimeBaseInitStruct = { 0 };
    BTIM_TimeBaseInitStruct.BTIM_Mode = BTIM_MODE_TIMER;
    BTIM_TimeBaseInitStruct.BTIM_Period = 0xffff;
    BTIM_TimeBaseInitStruct.BTIM_Prescaler = SYSCTRL_GetHClkFreq() / 1000000 - 1;
    BTIM_TimeBaseInit(CW_BTIM1, &BTIM_TimeBaseInitStruct);
    BTIM_Cmd(CW_BTIM1, ENABLE);
}

void timer_deInit(void)
{
    BTIM_Cmd(CW_BTIM1, DISABLE);
    BTIM_DeInit(CW_BTIM1);
    __SYSCTRL_BTIM123_CLK_DISABLE();
}

uint8_t inline timer_exceed(uint16_t ref, uint16_t us)
{
    return ((uint16_t) (getTick() - ref) >= (us));
}

uint16_t timer_getTick(void)
{
    return (uint16_t) getTick();
}

void timer_delayUs(uint16_t us)
{
    uint16_t t = getTick();
    while(!timer_exceed(t, us))
    {
    }
}

void timer_delayMs(uint32_t ms)
{
    for(uint32_t i = 0; i < ms; i++)
    {
        timer_delayUs(1000);
    }
}

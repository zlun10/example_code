#include "../inc/drv_rcc.h"
#include <stdint.h>
#include "cw32l011_sysctrl.h"

void rcc_init(void)
{
    SYSCTRL_ClearResetFlags();
    SYSCTRL_HSI_Enable(HSIOSC_TO_HSI96MHZ);
    SYSCTRL_LSI_Enable();
    while(SYSCTRL_GetStableFlag(SYSCTRL_FLAG_HSISTABLE) == RESET) {
        ;
    }
    SYSCTRL_SysClk_Switch(SYSCTRL_SYSCLKSRC_HSI);
    SYSCTRL_HCLKPRS_Config(SYSCTRL_HCLK_DIV1);
    SYSCTRL_PCLKPRS_Config(SYSCTRL_PCLK_DIV1);
    SYSCTRL_SystemCoreClockUpdate(SYSCTRL_GetHClkFreq());
}

void rcc_deInit(void)
{
    SYSCTRL_HSI_Enable(HSIOSC_TO_HSI4MHZ);
    SYSCTRL_LSI_Disable();
    SYSCTRL_SysClk_Switch(SYSCTRL_SYSCLKSRC_HSI);
    SYSCTRL_SystemCoreClockUpdate(SYSCTRL_GetHClkFreq());
}

#include "../inc/hal-storage.h"
#include "cw32l011_sysctrl.h"
#include "cw32l011_flash.h"

void hal_storage_init(void)
{
    SYSCTRL_AHBPeriphClk_Enable(SYSCTRL_AHB_PERIPH_FLASH, ENABLE);
}

void hal_storage_deInit(void)
{
    SYSCTRL_AHBPeriphClk_Enable(SYSCTRL_AHB_PERIPH_FLASH, DISABLE);
}

// void hal_storage_resume(void){}

void hal_storage_lockPage(void)
{
    FLASH_LockAllPages();
}

void hal_storage_unlockPage(uint16_t page_number)
{
    FLASH_UnlockPage(page_number);
}

uint8_t hal_storage_erasePage(uint16_t page_number)
{
    if(FLASH_ErasePage(page_number))
        return 1;

    return 0;
}

void hal_storage_writeBytes(uint32_t addr, uint8_t *data, uint16_t length)
{
    FLASH_WriteBytes(addr, data, length);
}

#ifndef _HAL_STORAGE_H
#define _HAL_STORAGE_H

#include <stdint.h>

void hal_storage_init(void);
void hal_storage_deInit(void);
void hal_storage_resume(void);

void hal_storage_lockPage(void);
void hal_storage_unlockPage(uint16_t page_number);
uint8_t hal_storage_erasePage(uint16_t page_number);

void hal_storage_writeBytes(uint32_t addr, uint8_t *data, uint16_t length);

#endif

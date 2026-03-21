#ifndef _STORAGE_H
#define _STORAGE_H

#define FLASH_MAGIC_ID 0x6C786C78

#include <stdint.h>

// event
void refreshNVM_EN(void);

// loop
void refresh_flash_task(void);

// app
void flash_init(void);
void flash_deInit(void);
void flash_resume(void);
void flash_addRecordId(uint8_t *id);

#endif


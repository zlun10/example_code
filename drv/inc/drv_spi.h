#ifndef _DRV_SPI_H
#define _DRV_SPI_H

#include <stdint.h>

void drv_spi_init(void);
void drv_spi_deInit(void);
void drv_spi_send(uint8_t data);

#endif

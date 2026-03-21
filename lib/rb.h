
#ifndef __RB_H
#define __RB_H
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef struct {
    uint8_t *buf;
    uint16_t size;
    volatile uint16_t wr;
    volatile uint16_t rd;
    volatile uint16_t cnt;
} rb_t;

void rb_init(rb_t *rb, uint8_t *buf, uint16_t size);
int16_t rb_cap(rb_t *rb);
int rb_write_buf(rb_t *rb, uint8_t *buf, int16_t len);
int rb_read_buf(rb_t *rb, uint8_t *buf, int16_t len, bool peek);
int rb_write(rb_t *rb, uint8_t data);
int rb_read(rb_t *rb, uint8_t *data, bool peek);

#endif





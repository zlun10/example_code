#include <stdio.h>
#include "rb.h"
#include "../dev/inc/log.h"

// ring buffer

void rb_init(rb_t *rb, uint8_t *buf, uint16_t size)
{
    rb->buf = buf;
    rb->size = size;
    rb->cnt = 0;
    rb->wr = 0;
    rb->rd = 0;
}

int16_t rb_cap(rb_t *rb)
{
    return rb->size - rb->cnt;
}

int rb_write_buf(rb_t *rb, uint8_t *buf, int16_t len)
{
    if((rb->cnt + len) > rb->size) {
        return -1;
    }


    if((rb->wr + len) <= rb->size) {
        memcpy(rb->buf + rb->wr, buf, len);
    } else {
        memcpy(rb->buf + rb->wr, buf + 0, rb->size - rb->wr);
        memcpy(rb->buf + 0, buf + (rb->size - rb->wr), len - (rb->size - rb->wr));
    }
    rb->wr = (rb->wr + len) % rb->size;
    rb->cnt += len;


    return len;
}

int rb_read_buf(rb_t *rb, uint8_t *buf, int16_t len, bool peek)
{
    if(rb->cnt == 0) {
        return -1;
    }

    if(rb->cnt < len) {
        len = rb->cnt;
        //        return -1;
    }

    if(buf != NULL) {
        if((rb->rd + len) <= rb->size) {
            memcpy(buf, rb->buf + rb->rd, len);
        } else {
            memcpy(buf, rb->buf + rb->rd, rb->size - rb->rd);
            memcpy(buf + rb->size - rb->rd, rb->buf + 0, len - (rb->size - rb->rd));
        }
    }

    if(!peek) {

        rb->rd = (rb->rd + len) % rb->size;
        rb->cnt -= len;

    }

    return len;
}

int rb_write(rb_t *rb, uint8_t data)
{
    if(rb->cnt >= rb->size) {
        LOG_ERROR("Ring buffer overflow");
        return -1;
    }

    rb->buf[rb->wr] = data;
    rb->wr = (rb->wr + 1) % rb->size;
    rb->cnt += 1;


    return 1;
}

int rb_read(rb_t *rb, uint8_t *data, bool peek)
{
    if(rb->cnt <= 0) {
        return -1;
    }

    *data = rb->buf[rb->rd];

    if(!peek) {

        rb->rd = (rb->rd + 1) % rb->size;
        rb->cnt -= 1;

    }

    return 1;
}

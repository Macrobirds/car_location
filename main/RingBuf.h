//»·ÐÎ»º³åÇø

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

typedef struct 
{
    uint8_t *buffer;
    uint32_t length;
    uint32_t read_pos;
    uint32_t write_pos;
}RingBuf;



void RingBuf_init(RingBuf * ringbuf,uint8_t * buffer,uint32_t length);

void RingBuf_push(RingBuf * ringbuf,uint8_t data);

uint8_t RingBuf_pop(RingBuf * ringbuf);

uint32_t RingBuf_get_Byte2read(RingBuf * ringbuf);

uint32_t RingBuf_get_Freesize(RingBuf * ringbuf);

uint32_t RingBuf_pop_length(RingBuf * ringbuf,uint8_t * data,uint32_t length);

void RingBuf_push_length(RingBuf * ringbuf,const uint8_t * data,uint32_t length);

#ifdef __cplusplus
}
#endif
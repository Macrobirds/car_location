#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define NBLOT_BUF_SIZE (256)
#define M5311_Enable_Pin 14
#define TX_BUF_SIZE 80


extern char M5310A_Rxbuf[NBLOT_BUF_SIZE];

uint8_t * M5310A_init(void);
void M5310A_Enalbe(void);
void M5310A_Disable(void);
uint8_t M5310A_Isconnect(void);
void M5310A_SendMsg(const uint8_t *msg,uint8_t len);


#ifdef __cplusplus
}
#endif
#pragma once

#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"




#define RA08_Enable_Pin 16
#define RA08_BUF_SIZE (256)

//ra08 发送状态
typedef enum{
    RA08_STA_JOINFAIL=0x00, //未成功入网
    RA08_STA_BUSY=0x01,     //模块忙
    RA08_STA_OVERLEN=0x02,  //发送数据过长
    RA08_STA_OK=0x03,       //发送成功
}RA08_STAT;

extern char Ra08_Rxbuf[RA08_BUF_SIZE];

//ra08模块初始化
uint8_t* Ra08_init(void);
//开启ra08模块
void Ra08_Enable(void);
void Ra08_Linkcheck(uint8_t *rx_buffer);
//关闭ra08模块
void Ra08_Disable(void);
//返回ra08连接状态
uint8_t Ra08_IsConnect(void);

//ra08发送
void Ra08_SendMsg(const uint8_t *msg,uint8_t len);
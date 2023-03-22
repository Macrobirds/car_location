#pragma once

#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"




#define RA08_Enable_Pin 16
#define RA08_BUF_SIZE (256)

//ra08 ����״̬
typedef enum{
    RA08_STA_JOINFAIL=0x00, //δ�ɹ�����
    RA08_STA_BUSY=0x01,     //ģ��æ
    RA08_STA_OVERLEN=0x02,  //�������ݹ���
    RA08_STA_OK=0x03,       //���ͳɹ�
}RA08_STAT;

extern char Ra08_Rxbuf[RA08_BUF_SIZE];

//ra08ģ���ʼ��
uint8_t* Ra08_init(void);
//����ra08ģ��
void Ra08_Enable(void);
void Ra08_Linkcheck(uint8_t *rx_buffer);
//�ر�ra08ģ��
void Ra08_Disable(void);
//����ra08����״̬
uint8_t Ra08_IsConnect(void);

//ra08����
void Ra08_SendMsg(const uint8_t *msg,uint8_t len);
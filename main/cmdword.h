#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdio.h>
#include "ATGM336.h"
#include "crc8_16.h"
#include "SC7A20.h"
#include "RingBuf.h"
#include "Ra-08.h"

#define LORAWAN
#define POI_CMD_LEN 0x05
#define CONF_CMD_LEN 0x07
#define LARGE_PACKAGE_SIZE 1024
#define PACKAGE_SIZE 32



typedef enum{
    LOCATION_CMD=0x01,
    INFO_CMD=0x02,
    LARGEPACKAGE_CMD=0x03,    
    POI_CMD=0xd1,
    CONF_CMD=0xd2
}command_word;


typedef struct 
{
    uint8_t cmmand;
    uint8_t len;
    uint32_t UTC;
    uint8_t warning;//crash sos 
    uint16_t fangweijiao;
    uint8_t CRC;
}info_t;

typedef struct 
{
    uint8_t command;
    uint8_t len;
    uint32_t UTC;
    uint8_t POI_Meg;
    uint8_t CRC;
}POI_t;

typedef struct 
{
    uint8_t command;
    uint8_t len;
    uint32_t UTC;
    int32_t latitude;
    int32_t longitude;
    uint8_t speed;
    uint8_t acc_x;
    uint8_t acc_y;
    uint8_t acc_z;
    uint16_t fangweijiao;
    uint16_t altitude;
    uint16_t PDOP;
    uint8_t power;
    uint8_t CRC;
}Location_t;

typedef struct 
{
    uint8_t command;
    uint8_t len;
    uint32_t UTC;
    uint8_t SensorConf;
    uint8_t UplinkConf;
    uint8_t CRC;
}Conf_t;





void Cmd_task_stack_Init(uint8_t *rxbuffer);
void Send_LocationCmd(esp_gps_t *esp_gps, uint8_t *acc, uint8_t power);
void Send_infoCmd(esp_gps_t *esp_gps, uint8_t warn_msg);

#ifdef __cplusplus
}
#endif
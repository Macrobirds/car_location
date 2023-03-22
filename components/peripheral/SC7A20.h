#pragma once 

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include "driver/i2c.h"
#include "i2ccommon.h"


//==========================================
//    SC7A20 Register
//==========================================
#define SC7A20_ADDR 0x19
#define SOFT_RESET    0x00
//#define CHIPID        0x01
#define CHIPID        0x0f// erichan 20150529

#define ACC_X_LSB     0x28
#define ACC_X_MSB     0x29

#define ACC_Y_LSB     0x2a
#define ACC_Y_MSB     0x2b

#define ACC_Z_LSB     0x2c
#define ACC_Z_MSB     0x2d

#define MOTION_FLAG   0x09
#define NEWDATA_FLAG  0x0A

#define TAP_ACTIVE_STATUS 0x0B  

#define RESOLUTION_RANGE 0x0F  // Resolution bit[3:2] -- 00:14bit 
                               //                        01:12bit 
                               //                        10:10bit
                               //                        11:8bit 
                               
                               // FS bit[1:0]         -- 00:+/-2g 
                               //                        01:+/-4g 
                               //                        10:+/-8g
                               //                        11:+/-16g 
#define ODR_AXIS      0x10 
#define MODE_BW       0x11                             
#define SWAP_POLARITY 0x12
#define INT_SET1      0x16
#define INT_SET2      0x17
#define INT_MAP1      0x19 
#define INT_MAP2      0x1A
#define INT_CONFIG    0x20 
#define INT_LATCH     0x21
#define FREEFALL_DUR  0x22
#define FREEFALL_THS  0x23
#define FREEFALL_HYST 0x24
#define ACTIVE_DUR    0x27
#define ACTIVE_THS    0x28
#define TAP_DUR       0x2A
#define TAP_THS       0x2B
#define ORIENT_HYST   0x2C
#define Z_BLOCK       0x2D
#define SELF_TEST     0x32
#define ENGINEERING_MODE   0x7f   


extern int16_t acc[XYZ];


uint8_t SC7A20_Init(uint8_t acc_thr);
void SC7A20_GetRaw(int16_t *acc);
uint8_t SC7A20_IsMove(void);


#ifdef __cplusplus
}
#endif
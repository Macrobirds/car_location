/*****************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software is protected by Copyright and the information and source code
 *  contained herein is confidential. The software including the source code
 *  may not be copied and the information contained herein may not be used or
 *  disclosed except with the written permission of MEMSIC Inc. (C) 2019
 *****************************************************************************/

 /**
 * @brief
 * This file implement magnetic sensor driver APIs.
 * Modified history: 
 * V1.0: Add version control on 20190508
 * V1.1: Optimize OTP read process on 20190603
 */
 
#pragma once
#ifdef __cplusplus
extern "C"
{
#endif

#include "driver/i2c.h"
#include "i2ccommon.h"

#define MMC36X0KJ_7BITI2C_ADDRESS	0x30

#define MMC36X0KJ_REG_DATA			0x00
#define MMC36X0KJ_REG_XL			0x00
#define MMC36X0KJ_REG_XH			0x01
#define MMC36X0KJ_REG_YL			0x02
#define MMC36X0KJ_REG_YH			0x03
#define MMC36X0KJ_REG_ZL			0x04
#define MMC36X0KJ_REG_ZH			0x05
#define MMC36X0KJ_REG_TEMP			0x06
#define MMC36X0KJ_REG_STATUS		0x07
#define MMC36X0KJ_REG_CTRL0			0x08
#define MMC36X0KJ_REG_CTRL1			0x09
#define MMC36X0KJ_REG_CTRL2			0x0A
#define MMC36X0KJ_REG_X_THD			0x0B
#define MMC36X0KJ_REG_Y_THD			0x0C
#define MMC36X0KJ_REG_Z_THD			0x0D
#define MMC36X0KJ_REG_SELFTEST		0x0E
#define MMC36X0KJ_REG_PASSWORD		0x0F
#define MMC36X0KJ_REG_OTPMODE		0x12
#define MMC36X0KJ_REG_TESTMODE		0x13
#define MMC36X0KJ_REG_SR_PWIDTH		0x20
#define MMC36X0KJ_REG_OTP			0x2A
#define MMC36X0KJ_REG_PRODUCTID		0x2F
 
#define MMC36X0KJ_CMD_REFILL		0x20
#define MMC36X0KJ_CMD_RESET         0x10
#define MMC36X0KJ_CMD_SET			0x08
#define MMC36X0KJ_CMD_TM_M			0x01
#define MMC36X0KJ_CMD_TM_T			0x02
#define MMC36X0KJ_CMD_START_MDT		0x04
#define MMC36X0KJ_CMD_100HZ			0x00
#define MMC36X0KJ_CMD_200HZ			0x01
#define MMC36X0KJ_CMD_400HZ			0x02
#define MMC36X0KJ_CMD_600HZ			0x03
#define MMC36X0KJ_CMD_CM_14HZ		0x01
#define MMC36X0KJ_CMD_CM_5HZ		0x02
#define MMC36X0KJ_CMD_CM_1HZ		0x04
#define MMC36X0KJ_CMD_SW_RST		0x80
#define MMC36X0KJ_CMD_PASSWORD		0xE1
#define MMC36X0KJ_CMD_OTP_OPER		0x11
#define MMC36X0KJ_CMD_OTP_MR		0x80
#define MMC36X0KJ_CMD_OTP_ACT		0x80
#define MMC36X0KJ_CMD_OTP_NACT		0x00
#define MMC36X0KJ_CMD_STSET_OPEN	0x02
#define MMC36X0KJ_CMD_STRST_OPEN	0x04
#define MMC36X0KJ_CMD_ST_CLOSE		0x00
#define MMC36X0KJ_CMD_INT_MD_EN		0x40
#define MMC36X0KJ_CMD_INT_MDT_EN	0x20

#define MMC36X0KJ_PRODUCT_ID		0x0A
#define MMC36X0KJ_OTP_READ_DONE_BIT	0x10
#define MMC36X0KJ_PUMP_ON_BIT		0x08
#define MMC36X0KJ_MDT_BIT			0x04
#define MMC36X0KJ_MEAS_T_DONE_BIT	0x02
#define MMC36X0KJ_MEAS_M_DONE_BIT	0x01

/* 16-bit mode, null field output (32768) */
#define	MMC36X0KJ_OFFSET			32768
#define	MMC36X0KJ_SENSITIVITY		1024
#define MMC36X0KJ_T_ZERO			(-75)
#define MMC36X0KJ_T_SENSITIVITY		0.8

extern float magnetic[XYZ];


/**
 * @brief Initialization
 */
int MMC36X0KJ_Initialization(void);

/**
 * @brief Enable the sensor
 */
void MMC36X0KJ_Enable(void);

/**
 * @brief Disable the sensor
 */
void MMC36X0KJ_Disable(void);

/**
 * @brief SET operation when using dual supply
 */
void MMC36X0KJ_DualPower_SET(void);

/**
 * @brief RESET operation when using dual supply
 */
void MMC36X0KJ_DualPower_RESET(void);

/**
 * @brief SET operation when using single supply
 */
void MMC36X0KJ_SinglePower_SET(void);

/**
 * @brief RESET operation when using single supply
 */
void MMC36X0KJ_SinglePower_RESET(void);

/**
 * @brief Get the temperature output
 * @param t_out[0] is the temperature, unit is degree Celsius
 */
void MMC36X0KJ_GetTemperature(float *t_out);

/**
 * @brief Get sensor data
 * @param mag_out is the magnetic field vector, unit is gauss
 */
void MMC36X0KJ_GetData(float *mag_out);

/**
 * @brief Get sensor data with SET and RESET function 
 * @param mag_out is the magnetic field vector, unit is gauss
 */
void MMC36X0KJ_GetData_With_SET_RESET(float *mag_out);

#ifdef __cplusplus
}
#endif
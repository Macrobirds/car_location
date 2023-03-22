#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"




typedef enum {
    UART_INTR_RXFIFO_FULL      = (0x1<<0),
    UART_INTR_TXFIFO_EMPTY     = (0x1<<1),
    UART_INTR_PARITY_ERR       = (0x1<<2),
    UART_INTR_FRAM_ERR         = (0x1<<3),
    UART_INTR_RXFIFO_OVF       = (0x1<<4),
    UART_INTR_DSR_CHG          = (0x1<<5),
    UART_INTR_CTS_CHG          = (0x1<<6),
    UART_INTR_BRK_DET          = (0x1<<7),
    UART_INTR_RXFIFO_TOUT      = (0x1<<8),
    UART_INTR_SW_XON           = (0x1<<9),
    UART_INTR_SW_XOFF          = (0x1<<10),
    UART_INTR_GLITCH_DET       = (0x1<<11),
    UART_INTR_TX_BRK_DONE      = (0x1<<12),
    UART_INTR_TX_BRK_IDLE      = (0x1<<13),
    UART_INTR_TX_DONE          = (0x1<<14),
    UART_INTR_RS485_PARITY_ERR = (0x1<<15),
    UART_INTR_RS485_FRM_ERR    = (0x1<<16),
    UART_INTR_RS485_CLASH      = (0x1<<17),
    UART_INTR_CMD_CHAR_DET     = (0x1<<18),
} uart_intr_t;



#define UART_INTR_CONFIG_FLAG ((UART_INTR_RXFIFO_FULL) \
                            | (UART_INTR_RXFIFO_TOUT) \
                            | (UART_INTR_RXFIFO_OVF) \
                            | (UART_INTR_BRK_DET) \
                            | (UART_INTR_PARITY_ERR))


extern QueueHandle_t CmdParserEventQueue;

#ifdef __cplusplus
}
#endif
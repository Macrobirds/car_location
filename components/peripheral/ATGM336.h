#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_types.h"
#include "esp_event.h"
#include "esp_err.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define GPS_Buffer_Length 80
#define UTCTime_Length 11
#define latitude_Length 11
#define N_S_Length 2
#define longitude_Length 12
#define E_W_Length 2 
#define UTCData_Length 12
#define ATGM336_Enable_Pin 16


ESP_EVENT_DECLARE_BASE(ESP_ATGM_EVENT);

typedef void *ATGM_parser_handle_t;

typedef enum{
    GPRMC=0x01,
    GPGGA=0x02,
    GPGSA=0x04,
    GPALL=0x07
}GPSFORM;




typedef struct {
    struct {
        uart_port_t uart_port;        /*!< UART port number */
        uint32_t rx_pin;              /*!< UART Rx Pin number */
        uint32_t tx_pin;              /*!< UART Tx Pin number */
        uint32_t baud_rate;           /*!< UART baud rate */
        uart_word_length_t data_bits; /*!< UART data bits length */
        uart_parity_t parity;         /*!< UART parity */
        uart_stop_bits_t stop_bits;   /*!< UART stop bits length */
        uint32_t event_queue_size;    /*!< UART event queue size */
    } uart;                           /*!< UART specific configuration */
} ATGM336_parser_config_t;




//GPS UART口初始化结构体
#define ATGM336_PARSER_CONFIG_DEFAULT()       \
    {                                      \
        .uart = {                          \
            .uart_port = UART_NUM_2,       \
            .rx_pin = 40,                  \
            .tx_pin= 39,                  \
            .baud_rate = 9600,             \
            .data_bits = UART_DATA_8_BITS, \
            .parity = UART_PARITY_DISABLE, \
            .stop_bits = UART_STOP_BITS_1, \
            .event_queue_size = 32         \
        }                                  \
    }


typedef void *ATGM336_parse_handle_t;

typedef struct {
       uint32_t Time;
       int32_t latitude;
       int32_t longitude;
       uint8_t speed;
       uint16_t fangweijiao;
       uint16_t altitude;
       uint16_t PDOP;
       uint8_t hour;
       uint8_t minute;
       uint8_t second;
       uint16_t year;
       uint8_t month;
       uint8_t day;
}GPSData;

typedef struct
{
    GPSData parent;
    uart_port_t uart_port;
    uint8_t *buffer;
    uint8_t statement;
    esp_event_loop_handle_t event_loop_hdl;
    TaskHandle_t tsk_hd1;
    QueueHandle_t event_queue;
} esp_gps_t;


typedef struct {
    char isGetData;
    char isuseful[2];
    char UTCtime[UTCTime_Length];
    char UTCDay[UTCData_Length];
    char latitude[latitude_Length];
    char N_S[N_S_Length];
    char longtitude[longitude_Length];
    char E_W[E_W_Length];
    char Speed[10];
    char fangweijiao[10];
} ATGM336RMC_t;

typedef struct {
    char WireHeight[10];
    char GroundHeight[10];
} ATGM336GGA_t;

typedef struct {
    char PDOP[5];
}ATGM336GAS_t;

esp_gps_t* ATGM336_parse_init(const ATGM336_parser_config_t *config);
void ATGM336_Enable(void);
void ATGM336_Disable(void);
void ATGM336_SendBuffer(const char *buffer);


#ifdef __cplusplus
}
#endif

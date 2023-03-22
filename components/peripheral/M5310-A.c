#include "M5310-A.h"
#include "esp_log.h"
#include "uartcommon.h"

#define TXD_PIN (17)
#define RXD_PIN (18)
#define UARTM5310AQUEUE_SIZE 256
#define CONFIG_M5310A_PARSER_TASK_PRIORITY 3
#define M5310A_UART_PORT UART_NUM_1

char M5310A_Rxbuf[NBLOT_BUF_SIZE];

static const char *M5310ATAG="M5310A";
static QueueHandle_t M5310AEventQueue_handle;
static TaskHandle_t M5310A_tsk_hdl=NULL;

static uint8_t m5310a_isconnect=false;


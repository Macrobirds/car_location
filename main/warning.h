#include "SC7A20.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "cmdword.h"
#include "ATGM336.h"

//¸æ¾¯ÐÅÏ¢
typedef enum{
    SOS=0x10,
    FALLOVER=0x20,
    CRASH=0x40,
    LOWPOWER=0x80
}warning_word;

void Warning_Init(esp_gps_t *esp_gps);

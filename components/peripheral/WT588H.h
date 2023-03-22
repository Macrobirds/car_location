#pragma once
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#define WT588_DATA_PIN 37
#define WT588_BUSY_PIN 36
#define WT588_Enable_Pin 34

void WT588_init(void);
void WT588_Enable(void);
void WT588_Disable(void);
void WT588_PlaySound(uint8_t soundindex);
void WT588_SendWord(uint8_t word);
void WT588_Volume(uint8_t volume);//有8级音量调节
void WT588_Mute(void);


#ifdef __cplusplus
}
#endif
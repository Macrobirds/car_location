/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "i2ccommon.h"
#include "ATGM336.h"
#include "SC7A20.h"
#include "MMC3630KJ.h"
#include "WT588H.h"
#include "Ra-08.h"
#include "cmdword.h"
#include "warning.h"


#define REF_LIPO_VOL 0x001
#define NO_OF_SAMPLES 100
#define DC_MODE_TASK_PRIORITY 5
#define LIPO_MODE_TASK_PRIORITY 5
#define CONFIG_PUM_TASK_PRIORITY 1
#define PWR_STATE_PIN 26


typedef enum
{
   DC_MODE = 0x01,
   LIPO_MODE = 0x02,
}power_mode;

static const char *TAG = "TEST";
static esp_gps_t *esp_gps_ptr = NULL;
static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;
static const adc_channel_t channel = ADC1_CHANNEL_8;

static uint8_t power = 0;
static uint8_t *cmd_buf_ptr = NULL;
static TaskHandle_t PMU_tsk_hd = NULL;
static TaskHandle_t DC_MODE_tsk_hd = NULL;
static TaskHandle_t LIPO_MODE_tsk_hd = NULL;

// DC供电任务任务
static void DC_MajorTask_entry(void *arg)
{
   ATGM336_Enable();
   Ra08_Enable();

   for (;;)
   {
      SC7A20_GetRaw(acc);
      //发送location命令字
      Send_LocationCmd(esp_gps_ptr, acc, power);
      vTaskDelay(pdMS_TO_TICKS(5000));
   }
}

// LIPO 3.7V供电任务
static void LIPO_MajorTask_entry(void *arg)
{
   for (;;)
   {
      if (SC7A20_IsMove())
      {
         //开启设备
         ATGM336_Enable();
         MMC36X0KJ_Enable();
         Ra08_Enable();
         vTaskDelay(pdMS_TO_TICKS(1000));
         //发送位置命令字
         Send_LocationCmd(esp_gps_ptr, acc, power);
         vTaskDelay(pdMS_TO_TICKS(30000));
         //关闭设备
         ATGM336_Disable();
         MMC36X0KJ_Disable();
         Ra08_Disable();
      }
      vTaskDelay(pdMS_TO_TICKS(100));
      esp_sleep_enable_timer_wakeup(1000000LL * 5 * 60);
   }
}

//监测电池电量或供电电压任务
static void PMU_SwitchMdoe_entry(void *arg)
{
   int val_raw = adc1_get_raw(channel);
   uint32_t voltage = 0;
   uint8_t loop_num = 0;
   uint8_t mode = DC_MODE;
   //配置充电状态监测gpio
   gpio_pad_select_gpio(PWR_STATE_PIN);
   gpio_set_direction(PWR_STATE_PIN,GPIO_MODE_INPUT);
   esp_adc_cal_characteristics_t *adcChar = calloc(1, sizeof(esp_adc_cal_characteristics_t));
   esp_adc_cal_characterize(unit, atten, ADC_WIDTH_12Bit, 1100, adcChar);
   //初始模式为DC供电模式
   xTaskCreate(DC_MajorTask_entry,
               "DC_Mode_tsk",
                2048,
               NULL,
               DC_MODE_TASK_PRIORITY,
               DC_MODE_tsk_hd);
   power = 0xff;
   while (1)
   {
      if (loop_num > NO_OF_SAMPLES)
      {
         loop_num = 0;
         val_raw /= NO_OF_SAMPLES;
         voltage = esp_adc_cal_raw_to_voltage(val_raw, adcChar);
         if (1) //gpio_get_level(PWR_STATE_PIN) 调试默认为5V供电
         {
            //5V供电模式
            printf("5V support voltage: %d\r\n",voltage);
            //检测到模式切换
            if (mode == LIPO_MODE)
            {
               power = 0xff;
               mode = DC_MODE;
               vTaskDelete(LIPO_MODE_tsk_hd);
               xTaskCreate(
                   DC_MajorTask_entry,
                   "DC_Mode_tsk",
                   2048,
                   NULL,
                   DC_MODE_TASK_PRIORITY,
                   DC_MODE_tsk_hd);
            }
         }
         else
         {
            //lipo 供电模式
            power = (uint8_t)((voltage / REF_LIPO_VOL) * 0xff);
            printf("lipo support voltage: %d\r\n",voltage);
            if (mode == DC_MODE)
            {
               mode = LIPO_MODE;
               vTaskDelete(DC_MODE_tsk_hd);
               xTaskCreate(
                   LIPO_MajorTask_entry,
                   "LIPO_Mode_tsk",
                   2048,
                   NULL,
                   LIPO_MODE_TASK_PRIORITY,
                   LIPO_MODE_tsk_hd);
            }
         }
      }
      else
      {
         val_raw += adc1_get_raw(channel);
         loop_num++;
      }
      vTaskDelay(pdMS_TO_TICKS(10));
   }
   vTaskDelete(NULL);
}


//模块测试函数
static void test_task(void)
{
   int i=0;
   SC7A20_GetRaw(acc);
   while(i<=9)
   {
      WT588_PlaySound(0x00+i);
      i++;
      if(i%2) WT588_Mute();
      else WT588_Volume(0x0f);
   }
   
   // MMC36X0KJ_GetData(magnetic);
   printf("acc: x%d y%d z%d magnetic: x%f y%f z%f\r\n",acc[X],acc[Y],acc[Z],
                                                      magnetic[X],magnetic[Y],magnetic[Z]);
   Ra08_SendMsg((uint8_t *)"test",strlen("test")); 
}


void app_main(void)
{
   // //初始化ADC
   adc1_config_width(ADC_WIDTH_12Bit);
   adc1_config_channel_atten(channel, atten);
   printf("adc init\r\n");
   // //初始化GPS
   // //初始化gpio 控制gps
   ATGM336_parser_config_t config = ATGM336_PARSER_CONFIG_DEFAULT();
   esp_gps_ptr = ATGM336_parse_init(&config);
   printf("gps init\r\n");
   ATGM336_Disable();

   //初始化i2c
   ESP_ERROR_CHECK(IIC_master_init());
   //初始化加速度计
   if(SC7A20_Init(0x16)) printf("sc7a20 init success\r\n");
   else printf("sc7a20 init false\r\n");

   // //初始化磁力计
   // if(MMC36X0KJ_Initialization()) printf("mmc3630 init success\r\n");
   // else printf("mmc3630 init false\r\n");

   //初始化控制语音模块gpio
   WT588_init();
   WT588_Enable();
   WT588_Volume(0x0f);

   #ifdef LORAWAN
   // 初始化Lorawn模块
   cmd_buf_ptr = Ra08_init();
   #endif
   // 开启命令字处理任务
   Cmd_task_stack_Init(cmd_buf_ptr);
   
  // 开启警告检测任务
   Warning_Init(esp_gps_ptr);

   //开启电源检测任务
   xTaskCreate(PMU_SwitchMdoe_entry,
               "PWM_SwitchMode",
               2048,
               NULL,
               CONFIG_PUM_TASK_PRIORITY,
               PMU_tsk_hd);


   //module test
   // while(1)
   // {
   //    test_task();
   //    vTaskDelay(pdMS_TO_TICKS(5000));
   // }

}

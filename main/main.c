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

// DC������������
static void DC_MajorTask_entry(void *arg)
{
   ATGM336_Enable();
   Ra08_Enable();

   for (;;)
   {
      SC7A20_GetRaw(acc);
      //����location������
      Send_LocationCmd(esp_gps_ptr, acc, power);
      vTaskDelay(pdMS_TO_TICKS(5000));
   }
}

// LIPO 3.7V��������
static void LIPO_MajorTask_entry(void *arg)
{
   for (;;)
   {
      if (SC7A20_IsMove())
      {
         //�����豸
         ATGM336_Enable();
         MMC36X0KJ_Enable();
         Ra08_Enable();
         vTaskDelay(pdMS_TO_TICKS(1000));
         //����λ��������
         Send_LocationCmd(esp_gps_ptr, acc, power);
         vTaskDelay(pdMS_TO_TICKS(30000));
         //�ر��豸
         ATGM336_Disable();
         MMC36X0KJ_Disable();
         Ra08_Disable();
      }
      vTaskDelay(pdMS_TO_TICKS(100));
      esp_sleep_enable_timer_wakeup(1000000LL * 5 * 60);
   }
}

//����ص����򹩵��ѹ����
static void PMU_SwitchMdoe_entry(void *arg)
{
   int val_raw = adc1_get_raw(channel);
   uint32_t voltage = 0;
   uint8_t loop_num = 0;
   uint8_t mode = DC_MODE;
   //���ó��״̬���gpio
   gpio_pad_select_gpio(PWR_STATE_PIN);
   gpio_set_direction(PWR_STATE_PIN,GPIO_MODE_INPUT);
   esp_adc_cal_characteristics_t *adcChar = calloc(1, sizeof(esp_adc_cal_characteristics_t));
   esp_adc_cal_characterize(unit, atten, ADC_WIDTH_12Bit, 1100, adcChar);
   //��ʼģʽΪDC����ģʽ
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
         if (1) //gpio_get_level(PWR_STATE_PIN) ����Ĭ��Ϊ5V����
         {
            //5V����ģʽ
            printf("5V support voltage: %d\r\n",voltage);
            //��⵽ģʽ�л�
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
            //lipo ����ģʽ
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


//ģ����Ժ���
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
   // //��ʼ��ADC
   adc1_config_width(ADC_WIDTH_12Bit);
   adc1_config_channel_atten(channel, atten);
   printf("adc init\r\n");
   // //��ʼ��GPS
   // //��ʼ��gpio ����gps
   ATGM336_parser_config_t config = ATGM336_PARSER_CONFIG_DEFAULT();
   esp_gps_ptr = ATGM336_parse_init(&config);
   printf("gps init\r\n");
   ATGM336_Disable();

   //��ʼ��i2c
   ESP_ERROR_CHECK(IIC_master_init());
   //��ʼ�����ٶȼ�
   if(SC7A20_Init(0x16)) printf("sc7a20 init success\r\n");
   else printf("sc7a20 init false\r\n");

   // //��ʼ��������
   // if(MMC36X0KJ_Initialization()) printf("mmc3630 init success\r\n");
   // else printf("mmc3630 init false\r\n");

   //��ʼ����������ģ��gpio
   WT588_init();
   WT588_Enable();
   WT588_Volume(0x0f);

   #ifdef LORAWAN
   // ��ʼ��Lorawnģ��
   cmd_buf_ptr = Ra08_init();
   #endif
   // ���������ִ�������
   Cmd_task_stack_Init(cmd_buf_ptr);
   
  // ��������������
   Warning_Init(esp_gps_ptr);

   //������Դ�������
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

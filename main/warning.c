#include "warning.h"

#define SC7A20_EXTI_PIN 12
#define SOS_EXTI_PIN 21
#define SC7A20_EXTI_PIN_SEL (1ULL << SC7A20_EXTI_PIN)
#define SOS_EXTI_PIN_SEL (1ULL << SOS_EXTI_PIN)
#define WARNING_EVENT_PRIORITY 10

static QueueHandle_t gpioWarnEventQueue = NULL;
static TaskHandle_t gpioWarn_tsk_hdl = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
   uint32_t gpio_num = (uint32_t)arg;
   xQueueSendFromISR(gpioWarnEventQueue, &gpio_num, NULL);
}

static void Warning_Info_entry(void *arg)
{
   esp_gps_t * esp_gps=(esp_gps_t *)arg;
   uint32_t io_num;
   for (;;)
   {
      if (xQueueReceive(gpioWarnEventQueue, &io_num, portMAX_DELAY))
      {
         //发送info命令字
        // printf("crash irt\r\n");
         printf("%d", io_num);
         switch (io_num)
         {
         case SC7A20_EXTI_PIN:
             Send_infoCmd(esp_gps,CRASH);
            break;
         case SOS_EXTI_PIN:
            Send_infoCmd(esp_gps,SOS);
            break;

         default:
            break;
         }
      }
   }
   vTaskDelete(NULL);
}

void Warning_Init(esp_gps_t *esp_gps)
{
   gpio_config_t io_conf;
   //初始化SOS告警按钮
   //初始化外部中断感知碰撞
   gpio_config(&io_conf);
   io_conf.pin_bit_mask = (SC7A20_EXTI_PIN_SEL | SOS_EXTI_PIN_SEL);
   io_conf.mode = GPIO_MODE_INPUT;
   io_conf.pull_up_en = 0;
   io_conf.pull_down_en = 1;
   io_conf.intr_type = GPIO_INTR_POSEDGE;
   gpio_config(&io_conf);
   printf("init warn queue \r\n");
   gpioWarnEventQueue = xQueueCreate(10, sizeof(uint32_t));
   gpio_install_isr_service(0);
   gpio_isr_handler_add(SC7A20_EXTI_PIN, gpio_isr_handler, (void *)SC7A20_EXTI_PIN);
   gpio_isr_handler_add(SOS_EXTI_PIN, gpio_isr_handler, (void *)SOS_EXTI_PIN);
   xTaskCreate(Warning_Info_entry,
               "waring_event",
               2048,
               esp_gps,
               WARNING_EVENT_PRIORITY,
               gpioWarn_tsk_hdl);
}
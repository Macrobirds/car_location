#include "WT588H.h"


static inline void Start(void)
{
    gpio_set_level(WT588_DATA_PIN,0);
    ets_delay_us(5000);
    gpio_set_level(WT588_DATA_PIN,1);
}

static inline void Stop(void)
{
    gpio_set_level(WT588_DATA_PIN,1);
    ets_delay_us(2000);

}

static inline void Out_High(void)
{
    gpio_set_level(WT588_DATA_PIN,1);
    ets_delay_us(600);
    gpio_set_level(WT588_DATA_PIN,0);
    ets_delay_us(200);
    gpio_set_level(WT588_DATA_PIN,1);
}

static inline void Out_Low(void)
{
    gpio_set_level(WT588_DATA_PIN,1);
    ets_delay_us(200);
    gpio_set_level(WT588_DATA_PIN,0);
    ets_delay_us(600);
    gpio_set_level(WT588_DATA_PIN,1);
}

static inline void Out_interval(void)
{
    gpio_set_level(WT588_DATA_PIN,1);
    ets_delay_us(2000);
    gpio_set_level(WT588_DATA_PIN,0);
    ets_delay_us(5000);
    gpio_set_level(WT588_DATA_PIN,1);
}

//����оƬGPIO��ʼ��
void WT588_init(void)
{
    //����Data pin
    gpio_pad_select_gpio(WT588_DATA_PIN);
    gpio_set_direction(WT588_DATA_PIN,GPIO_MODE_OUTPUT);
    //����Busy pin
    gpio_pad_select_gpio(WT588_BUSY_PIN);
    gpio_set_direction(WT588_BUSY_PIN,GPIO_MODE_INPUT);
    //����ʹ��gpio��
    //.....
    gpio_pad_select_gpio(WT588_Enable_Pin);
    gpio_set_direction(WT588_Enable_Pin,GPIO_MODE_OUTPUT);
    WT588_Disable();
}

//��������ģ��
void WT588_Enable(void)
{
    gpio_set_level(WT588_Enable_Pin,1);
}
//�ر�����ģ��
void WT588_Disable(void)
{
    gpio_set_level(WT588_Enable_Pin,0);
}




void WT588_SendWord(uint8_t word)
{
    uint8_t i;
    uint8_t temp;
    temp=word;
    Start();
    for(i=0;i<8;i++)
    {
        if(temp&0x01){
            Out_High();
        }else{
            Out_Low();
        }
        temp>>=1;
    }
    Stop();
}

//���Ŷ�Ӧ��ַ����
void WT588_PlaySound(uint8_t soundindex)
{
    while(!gpio_get_level(WT588_BUSY_PIN)) 
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        printf("audio busy\r\n");
    }   
    WT588_SendWord(soundindex);
    
}

//������������
void WT588_Volume(uint8_t volume)//��8����������
{
    while(!gpio_get_level(WT588_BUSY_PIN)) 
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        printf("audio busy\r\n");
    }
    if(volume<=0x0f){
        WT588_SendWord(0xe0+volume);
    }
}

//����ģ�龲��
void WT588_Mute(void)
{
    while(!gpio_get_level(WT588_BUSY_PIN)) 
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        printf("audio busy\r\n");
    }    
    WT588_SendWord(0xe0);
}
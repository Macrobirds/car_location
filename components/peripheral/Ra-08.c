
#include "Ra-08.h"
#include "esp_log.h"
#include "uartcommon.h"




#define TX_BUF_SIZE 80
#define TXD_PIN (17)
#define RXD_PIN (18)
#define UARTRA08QUEUE_SIZE 256
#define CONFIG_RA08_PARSER_TASK_PRIORITY 3
#define RA08_UART_PORT UART_NUM_1




//接收命令字符串
char Ra08_Rxbuf[RA08_BUF_SIZE]={0};


static const char *RA08TAG = "ra08 parser";
static QueueHandle_t Ra08EventQueue_handle;
static TaskHandle_t ra08_tsk_hdl = NULL;

const int ra08_uart_num=RA08_UART_PORT;
static uint8_t ra08_isconnect=false;
static const char * CDEVEUI="70B3D57ED0054DF0";
static const char * CAPPKEY="A7E4FF245E4372EAC17333A396A2900C";
static const char * CDEVADDR="019e4313";
static const char * CAPPEUI="0000000000000000";
static const char * CAPPSKEY="614fe5b44bd0c2e827eabae8d10d82aa";
static const char * CNWKSKEY="5cf07249a7b245b8c97fdcaae8f84bab";


static void ra08_char2byte(const uint8_t *msg,uint8_t *msg_byte,uint8_t len)
{
    while(len--){
        *msg_byte=0x30;
        *msg_byte+=(*msg/16)>9?(*msg/16)+7:(*msg/16);
        msg_byte++;
        *msg_byte=0x30;
        *msg_byte+=(*msg%16)>9?(*msg%16)+7:(*msg%16);
        msg++;
        msg_byte++;
    }
}

static void ra08_byte2char(const uint8_t *msg,uint8_t *msg_char,uint8_t len)
{
    len/=2;
    while(len--){
        *msg_char=(*msg-0x30)>9?(*msg-0x37)*16:(*msg-0x30)*16;
        msg++;
        *msg_char+=(*msg-0x30)>9?(*msg-0x37):(*msg-0x30);
        msg_char++;
        msg++;
        
        
       
    }
}
//ra08 串口接收处理函数
static void ra08_handler_uart_tout(uint8_t *rx_buf)
{
    char *strx=NULL;
    int i=0;
    memset(rx_buf,0,RA08_BUF_SIZE);
    const int rxbytes=uart_read_bytes(RA08_UART_PORT,rx_buf,RA08_BUF_SIZE,10);
    if(rxbytes>0)
    {
        rx_buf[rxbytes]=0;
        printf((char *)rx_buf);
        if((strx=strstr((char *)rx_buf,"OK+RECV:"))!=NULL)//判断是否接收到服务器数据
        {
            ra08_isconnect=RA08_STA_OK; //发送状态为发送成功
            while(i<3)
            {
                if((strx=strstr(strx,","))==NULL)//检测是否为接收数据
                {
                    return;
                }
                strx++;
                i++;
            }
            printf("receive downlink msg\r\n");
            printf(strx);
            memset(Ra08_Rxbuf,0,sizeof(Ra08_Rxbuf));
            ra08_byte2char((uint8_t *)strx,(uint8_t *)Ra08_Rxbuf,strlen(strx)-2); //去除换行回车符
            xQueueSend(CmdParserEventQueue,&Ra08_Rxbuf,NULL);//调用命令解析任务
        }
        else if((strx=strstr((char *)rx_buf,"ERR+SEND:"))!=NULL) //发送失败
        {
            printf("error string %s\r\n",strx);
            strx=strstr(strx,":");
            strx++;
            printf("%s\r\n",strx);
            ra08_byte2char((uint8_t *)strx,&ra08_isconnect,2); //得到ra08发送状态
            printf("error status : %d",ra08_isconnect);
        }
    }
    else
    {
        uart_flush_input(RA08_UART_PORT);
    }
}

static int sendData(const char *data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(RA08_UART_PORT, data, len);
    return txBytes;
}

void Ra08_SendBuffer(const char *buffer)
{
    printf(buffer);
    sendData(buffer);
}

// static void ra08_handler_uart_pattern(uint8_t *buffer)
// {
//     char *strx=NULL;
//     int pos=uart_pattern_pop_pos(RA08_UART_PORT);
//     if(pos!=-1){
        
//         int read_len = uart_read_bytes(RA08_UART_PORT, buffer, pos + 1, 100 / 1000);
//         buffer[read_len]='\0';
//         printf((char *)buffer);
//         if((strx=strstr((char *)buffer,"+RECV"))!=NULL)
//         {   

//             xQueueSend(CmdParserEventQueue,NULL,NULL);
//             strx=NULL;
//         }

//     }else{
//         uart_flush_input(RA08_UART_PORT);
//     }
// }


//ra08 解析接收任务
static void ra08_parser_task_entry(void *arg)
{
    //初始化控制ra08开关io
    uint8_t *rx_buffer = (uint8_t *)arg;
    uart_event_t event;
    while (1)
    {
        if (xQueueReceive(Ra08EventQueue_handle, &event, pdMS_TO_TICKS(200)))
        {
            switch (event.type)
            {
            case UART_DATA:
                ESP_LOGW(RA08TAG,"RECEIVE DATA");
                ra08_handler_uart_tout(rx_buffer);
                break;
            case UART_FIFO_OVF:
                ESP_LOGW(RA08TAG, "HW FIFO Overflow");
                uart_flush(RA08_UART_PORT);
                xQueueReset(Ra08EventQueue_handle);
                break;
            case UART_BUFFER_FULL:
                ESP_LOGW(RA08TAG, "Ring Buffer Full");
                uart_flush(RA08_UART_PORT);
                xQueueReset(Ra08EventQueue_handle);
                break;
            case UART_BREAK:
                ESP_LOGW(RA08TAG, "Rx Break");
               // ra08_handler_uart_pattern(rx_buffer);
               //ra08_handler_uart_tout(rx_buffer);
                break;
            case UART_PARITY_ERR:
                ESP_LOGE(RA08TAG, "Parity Error");
                break;
            case UART_FRAME_ERR:
                ESP_LOGE(RA08TAG, "Frame Error");
                break;
            case UART_PATTERN_DET:
               // ra08_handler_uart_pattern(rx_buffer);
                break;
            default:
                ESP_LOGW(RA08TAG, "unknown uart event type: %d", event.type);
                break;
            }
        }
    }
    vTaskDelete(NULL);
}

static void Ra08_Delay(void)
{
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

static void Ra08_SendATCmd(const char * cmd,uint8_t * rx_buff)
{
    char *strx;
    memset(rx_buff,0,RA08_BUF_SIZE);
    Ra08_SendBuffer(cmd);
    Ra08_Delay();
    printf((char *)rx_buff);
    strx = strstr((char *)rx_buff, "OK");
    while (!strx)
    {   
        memset(rx_buff,0,RA08_BUF_SIZE);
        Ra08_SendBuffer(cmd);
        Ra08_Delay();
        strx = strstr((char *)rx_buff, "OK");
    }
}

//ra08 APB入网
static void Ra08_APB(uint8_t *rx_buffer)
{
    char tempbuf[100]={0};
    //检测esp32是否与ra08连接
    Ra08_SendATCmd("AT+CGMI?\r\n",rx_buffer);
    //APB入网模式
    Ra08_SendATCmd("AT+CJOINMODE=1\r\n",rx_buffer);
    //配置节点信息
    sprintf(tempbuf,"AT+CDEVEUI=%s\r\n",CDEVEUI);
    Ra08_SendATCmd(tempbuf,rx_buffer);    

    memset(tempbuf,0,sizeof(tempbuf));
    sprintf(tempbuf,"AT+CDEVADDR=%s\r\n",CDEVADDR);
    Ra08_SendATCmd(tempbuf,rx_buffer);

    memset(tempbuf,0,sizeof(tempbuf));
    sprintf(tempbuf,"AT+CAPPSKEY=%s\r\n",CAPPSKEY);
    Ra08_SendATCmd(tempbuf,rx_buffer);

    memset(tempbuf,0,sizeof(tempbuf));
    sprintf(tempbuf,"AT+CNWKSKEY=%s\r\n",CNWKSKEY);
    Ra08_SendATCmd(tempbuf,rx_buffer);
    //异频模式
    Ra08_SendATCmd("AT+CULDLMODE=2\r\n",rx_buffer);
    //C class
    Ra08_SendATCmd("AT+CCLASS=2\r\n",rx_buffer);
    //
    Ra08_SendATCmd("AT+CFREQBANDMASK=0001\r\n",rx_buffer);
    //关闭自动JOIN，JOIN周期8S,最大次数8次
    Ra08_SendATCmd("AT+CJOIN=1,0,8,8\r\n",rx_buffer);
}

//ra08 OTAA入网
static void Ra08_OTAA(uint8_t *rx_buffer)
{
    
    char tempbuf[100]={0};
    //检测esp32是否与ra08连接
    Ra08_SendATCmd("AT+CGMI?\r\n",rx_buffer);
    //OTAA入网模式
    Ra08_SendATCmd("AT+CJOINMODE=0\r\n",rx_buffer);
    //配置节点三元组信息
    //CDEVEUI
    sprintf(tempbuf,"AT+CDEVEUI=%s\r\n",CDEVEUI);
    Ra08_SendATCmd(tempbuf,rx_buffer);
    //CAPPKEY
    memset(tempbuf,0,sizeof(tempbuf));
    sprintf(tempbuf,"AT+CAPPEUI=%s\r\n",CAPPEUI);
    Ra08_SendATCmd(tempbuf,rx_buffer);
    //CAPPKEY
    memset(tempbuf,0,sizeof(tempbuf));
    sprintf(tempbuf,"AT+CAPPKEY=%s\r\n",CAPPKEY);
    Ra08_SendATCmd(tempbuf,rx_buffer);
    //CFREQBANDMASK节点频组掩码设置
    Ra08_SendATCmd("AT+CFREQBANDMASK=0001\r\n",rx_buffer);
    //CULDLMODE设置上下行同异频
    Ra08_SendATCmd("AT+CULDLMODE=2\r\n",rx_buffer);
    //C class
    Ra08_SendATCmd("AT+CCLASS=2\r\n",rx_buffer);
    //关闭自动JOIN，JOIN周期8S,最大次数8次
    Ra08_SendATCmd("AT+CJOIN=1,0,8,8\r\n",rx_buffer);
    vTaskDelay(pdMS_TO_TICKS(5000));


}

void Ra08_Linkcheck(uint8_t *rx_buffer)
{   
    uint8_t i;
    char temp_value[10]={0};
    int check_value[4]={0};
    char *substring=Ra08_Rxbuf;
    char *substringnext=Ra08_Rxbuf;
    //验证网络连接
    Ra08_SendATCmd("AT+CLINKCHECK=1\r\n",(uint8_t *)rx_buffer);
    substring=strstr(substring,":");
    substring++;
    for(i=0;i<4;i++)
    {
        substringnext=strstr(substring,",");
        memset(temp_value,0,sizeof(temp_value));
        memcpy(temp_value,substring,substringnext-substring);
        check_value[i]=atoi(temp_value);
        substring=strstr(substringnext,",");
        substring++;
    }
    //判断是否连接
   
}

//Ra08 初始化
uint8_t* Ra08_init(void)
{
    //初始化Ra08使能gpio口
    gpio_pad_select_gpio(RA08_Enable_Pin);
    gpio_set_direction(RA08_Enable_Pin,GPIO_MODE_OUTPUT);
    gpio_set_level(RA08_Enable_Pin,1);
    uint8_t *rx_buffer = calloc(RA08_BUF_SIZE, sizeof(uint8_t));


    const uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    if(uart_driver_install(RA08_UART_PORT, RA08_BUF_SIZE, 0, UARTRA08QUEUE_SIZE, &Ra08EventQueue_handle, 0)!=ESP_OK)
    {
        printf("install uart2 driver failed");
    }else{
        printf("success");
    }
    if(uart_param_config(RA08_UART_PORT, &uart_config)!=ESP_OK)
    {
        printf("config uart2 parameter failde");
    }else{
        printf("success");
    }

    if(uart_set_pin(RA08_UART_PORT, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE)!=ESP_OK)
    {
        printf("config uart gpio failed");
    }else{
        printf("success");
    }

    if(uart_enable_rx_intr(ra08_uart_num)!=ESP_OK)
    {
        printf("enable rx intr fail\r\n");
    }
    if(uart_set_rx_timeout(ra08_uart_num,10)!=ESP_OK)
    {
        printf("set rx timeout faile \r\n");
    }

    uart_flush(RA08_UART_PORT);
    xTaskCreate( //创建lora接收中断任务
        ra08_parser_task_entry,
        "ra08_parser",
        2048,
        rx_buffer,
        CONFIG_RA08_PARSER_TASK_PRIORITY,
        &ra08_tsk_hdl);


    //Ra08_APB(rx_buffer);
    Ra08_OTAA(rx_buffer); //lora入网
   // Ra08_Disable();//关闭ra08
    return rx_buffer;
}


void Ra08_Enable(void)
{
    uart_flush(RA08_UART_PORT);
    gpio_set_level(RA08_Enable_Pin,1);
    uart_enable_intr_mask(RA08_UART_PORT,UART_INTR_CONFIG_FLAG);
    //Ra08_OTAA(rx_buffer);
}

void Ra08_Disable(void)
{
    uart_disable_intr_mask(RA08_UART_PORT,UART_INTR_CONFIG_FLAG);//关闭中断
    gpio_set_level(RA08_Enable_Pin,0);
}

uint8_t Ra08_IsConnect(void)
{
    return (ra08_isconnect==RA08_STA_OK?true:false);
}

void Ra08_SendMsg(const uint8_t *msg,uint8_t len)
{   
    char cmdbuf[100]={0};
    char msg_byte[TX_BUF_SIZE]={0};
    if(len>TX_BUF_SIZE) return;
    ra08_char2byte(msg,(uint8_t *)msg_byte,len);
    sprintf(cmdbuf,"AT+DTRX=1,2,%d,%s\r\n",len,msg_byte);
    Ra08_SendBuffer(cmdbuf);
    Ra08_Delay();
    // strx = strstr((char *)rx_buf, "OK");
    // while (!strx)
    // {   
    //     memset(rx_buf,0,RA08_BUF_SIZE);
    //     Ra08_SendBuffer(cmdbuf);
    //     Ra08_Delay();
    //     strx = strstr((char *)rx_buf, "OK");
    // }
}

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "ATGM336.h"
#include "uartcommon.h"

#define ATGM336_UART_PORT UART_NUM_2

#define ATGM_PARSER_RUNTIME_BUFFER_SIZE 80
#define CONFIG_ATGM_PARSER_TASK_STACK_SIZE 4096
#define CONFIG_ATGM_PARSER_TASK_PRIORITY 2
#define ATGM_MAX_STATEMENT_ITEM_LENGTH (16)
#define ATGM_EVENT_LOOP_QUEUE_SIZE (16)
#define LATITUDE_BIAS 1000000
#define LONGITUDE_BIAS 1000000
#define PDOP_BIAS 10
#define KOT_TO_KMH 1.852

ESP_EVENT_DEFINE_BASE(ESP_ATGM_EVENT);

static const char *GPS_TAG="ATGM336_parser";



static char parsegpsbuffer[ATGM_PARSER_RUNTIME_BUFFER_SIZE];
ATGM336GAS_t ATGM336GAS_Data={
    {0}
};
ATGM336RMC_t ATGM336RMC_Data={
    false,
    {0},
    {0},
    {0},
    {0},
    {0},
    {0},
    {0},
    {0},
    {0}
};
ATGM336GGA_t ATGM336GGA_Data={
    {0},
    {0}
};

//char 转换为 uint8_t
static inline uint8_t convert_two_digit2number(const char *digit_char)
{
    return 10 * (digit_char[0] - '0') + (digit_char[1] - '0');
}

//解析UTC时间
static void parse_utc_time(esp_gps_t *esp_gps)
{
    esp_gps->parent.hour=convert_two_digit2number(&ATGM336RMC_Data.UTCtime[0]);
    esp_gps->parent.minute=convert_two_digit2number(&ATGM336RMC_Data.UTCtime[2]);
    esp_gps->parent.second=convert_two_digit2number(&ATGM336RMC_Data.UTCtime[4]);
}

//解析日期
static void parse_utc_day(esp_gps_t *esp_gps)
{
    esp_gps->parent.year=convert_two_digit2number(&ATGM336RMC_Data.UTCDay[4]);
    esp_gps->parent.month=convert_two_digit2number(&ATGM336RMC_Data.UTCDay[2]);
    esp_gps->parent.day=convert_two_digit2number(&ATGM336RMC_Data.UTCDay[0]);
}

//解析得到时间戳
static uint32_t parse_utc2stamp(esp_gps_t *esp_gps)
{
    struct tm info;
    info.tm_year=esp_gps->parent.year+100;
    info.tm_mon=esp_gps->parent.month-1;
    info.tm_mday=esp_gps->parent.day;
    info.tm_hour=esp_gps->parent.hour;
    info.tm_min=esp_gps->parent.minute;
    info.tm_sec=esp_gps->parent.second;
    info.tm_isdst=-1;
    return (uint32_t)mktime(&info);
}

//解析纬度
static int32_t parse_latitude(void)
{
    double temp1;
    uint32_t temp2;
    double latitude;
    temp1=atof(ATGM336RMC_Data.latitude);
    temp2=(uint32_t)(temp1/100);
    latitude=(((temp1/100)-temp2)*5/3+temp2);
    if(ATGM336RMC_Data.N_S[0]=='S') latitude*=-1;
    return (int32_t)(latitude*LATITUDE_BIAS);
}

//解析经度
static int32_t parse_longitude(void)
{   
    double temp1;
    uint32_t temp2;
    double longitude;
    temp1=atof(ATGM336RMC_Data.longtitude);
    temp2=(uint32_t)(temp1/100);
    longitude=((temp1/100)-temp2)*5/3+temp2;
    if(ATGM336RMC_Data.E_W[0]=='W') longitude*=-1;
    return (int32_t)(longitude*LONGITUDE_BIAS);
}

//解析速度
static uint8_t parse_speed(void)
{
    float speed;
    speed=atof(ATGM336RMC_Data.Speed);
    speed*=KOT_TO_KMH;
    return (uint8_t)speed;
}

//gps信息解析函数
static void parsegpsdata(esp_gps_t *esp_gps)
{
    //获取UTC时间
    parse_utc_time(esp_gps);
    parse_utc_day(esp_gps);
    esp_gps->parent.Time=parse_utc2stamp(esp_gps);
    //获取纬度
    esp_gps->parent.latitude=parse_latitude();
    //获取经度
    esp_gps->parent.longitude=parse_longitude();
    //获取速度
    esp_gps->parent.speed=parse_speed();
    //获取方位角
    esp_gps->parent.fangweijiao=(uint16_t)(atof(ATGM336RMC_Data.fangweijiao));
    //获取海拔
    esp_gps->parent.altitude=(uint16_t)(atof(ATGM336GGA_Data.WireHeight));
    //获取PDOP参数
    esp_gps->parent.PDOP=(uint16_t)(atof(ATGM336GAS_Data.PDOP)*PDOP_BIAS);
    printf("time %x altitude %x longitude %x speed %x fangweijiao %x altitude %x PDOP %x\r\n",esp_gps->parent.Time,
                                                                                              esp_gps->parent.latitude,
                                                                                              esp_gps->parent.longitude,
                                                                                              esp_gps->parent.speed,
                                                                                              esp_gps->parent.fangweijiao,
                                                                                              esp_gps->parent.altitude,
                                                                                              esp_gps->parent.PDOP);
}



static void parseGpsBuffer_GPRMS(char *gpsbuffer, int len)
{
    
    char *substring=gpsbuffer;
    char *substringnext=gpsbuffer;
    int i;
    memset((void *)&ATGM336RMC_Data,0,sizeof(ATGM336RMC_Data));
    for (i = 0; i <= 9; i++)
    {
        if (i == 0)
        {
            substring = strstr(substring, ",");
            if ( substring== NULL)
            {
                return;
            }
        }
        else
        {
            substring++;
            if ((substringnext = strstr(substring, ",") )!= NULL)
            {   
                
                switch (i)
                {
                case 1:
                    memcpy(ATGM336RMC_Data.UTCtime, substring, substringnext - substring);
                    break;
                case 2:
                    memcpy(ATGM336RMC_Data.isuseful, substring, substringnext - substring);
                    break;
                case 3:
                    memcpy(ATGM336RMC_Data.latitude, substring, substringnext - substring);
                    //test code 
                    //memcpy(ATGM336RMC_Data.latitude,"2235.10896",strlen("2235.10896"));
                    break;
                case 4:
                    memcpy(ATGM336RMC_Data.N_S, substring, substringnext - substring);
                    break;
                case 5:
                    memcpy(ATGM336RMC_Data.longtitude, substring, substringnext - substring);
                    //test code
                    //memcpy(ATGM336RMC_Data.longtitude,"11354.79188",strlen("11354.79188"));
                    break;
                case 6:
                    memcpy(ATGM336RMC_Data.E_W, substring, substringnext - substring);
                    break;
                case 7:
                    memcpy(ATGM336RMC_Data.Speed, substring, substringnext - substring);
                    break;
                case 8:
                    memcpy(ATGM336RMC_Data.fangweijiao, substring, substringnext - substring);
                    break;
                case 9:
                    memcpy(ATGM336RMC_Data.UTCDay, substring, substringnext - substring);
                    break;
                default:
                    break;
                }
            }
            substring = substringnext;
        }
        
    }
    if (ATGM336RMC_Data.isuseful[0] == 'A')
    {
        ATGM336RMC_Data.isGetData = true;
    }
    else if (ATGM336RMC_Data.isuseful[0] == 'V')
    {
        ATGM336RMC_Data.isGetData = false;
    }
}

static void parseGpsBuffer_GPGGA(char *gpsbuffer, int len)
{
    char *substring=gpsbuffer;
    char *substringnext=gpsbuffer;
    int i;
    for (i = 0; i <= 11; i++)
    {
        if (i == 0)
        {
            substring = strstr(substring, ",");
            if ( substring== NULL)
            {
                return;
            }
        }
        else
        {
            substring++;
            if ((substringnext = strstr(substring, ",") )!= NULL)
            {
                switch (i)
                {
                case 9:
                    memcpy(ATGM336GGA_Data.WireHeight, substring, substringnext - substring); 
                    ATGM336GGA_Data.WireHeight[substringnext - substring]='\0';
                    break;
                case 11:
                    memcpy(ATGM336GGA_Data.GroundHeight, substring, substringnext - substring); 
                    ATGM336GGA_Data.GroundHeight[substringnext - substring]='\0';
                    break;
                default:
                    break;
                }
            }
            substring=substringnext;
        }
        
    }
}
static void parseGpsBuffer_GPGAS(char *gpsbuffer, int len)
{
    char *substring=gpsbuffer;
    char *substringnext=gpsbuffer;
    int i;
    for (i = 0; i <= 15; i++)
    {
        if (i == 0)
        {
            substring = strstr(substring, ",");
            if ( substring== NULL)
            {
                return;
            }
        }
        else {
            substring++;
            if ((substringnext = strstr(substring, ",") )!= NULL)
            {
                //printf("index %d:%s",i,substring);
                if(i==15)
                {
                    memcpy(ATGM336GAS_Data.PDOP,substring,substringnext-substring); 
                    break;
                }
                substring=substringnext;
            }


        }
        
    }
    
}

//gps信息接收函数
static esp_err_t gps_decode(esp_gps_t *esp_gps, size_t len)
{
    unsigned char *gpsbuffer = esp_gps->buffer;


    if (gpsbuffer[0] == '$' && gpsbuffer[4] == 'M' && gpsbuffer[5] == 'C') //确定是否收到GPRMC/GNRMC
    {
        
        memset(parsegpsbuffer, 0, sizeof(parsegpsbuffer));
        memcpy(parsegpsbuffer, gpsbuffer, len);
        parseGpsBuffer_GPRMS(parsegpsbuffer, len);
        bzero(gpsbuffer, ATGM_PARSER_RUNTIME_BUFFER_SIZE);
        esp_gps->statement |= GPRMC;
    }
    if (gpsbuffer[0] == '$' && gpsbuffer[4] == 'G' && gpsbuffer[5] == 'A') //确定是否收到GPGGA/GNGGA
    {
        
        memset(parsegpsbuffer, 0, sizeof(parsegpsbuffer));
        memcpy(parsegpsbuffer, gpsbuffer, len);
        parseGpsBuffer_GPGGA(parsegpsbuffer, len);
        bzero(gpsbuffer, ATGM_PARSER_RUNTIME_BUFFER_SIZE);
        esp_gps->statement |= GPGGA;
    }
    if (gpsbuffer[0] == '$' && gpsbuffer[4] == 'S' && gpsbuffer[5] == 'A') //确定是否收到GPGSA
    {
        
        memset(parsegpsbuffer, 0, sizeof(parsegpsbuffer));
        memcpy(parsegpsbuffer, gpsbuffer, len);
        parseGpsBuffer_GPGAS(parsegpsbuffer, len);
        bzero(gpsbuffer, ATGM_PARSER_RUNTIME_BUFFER_SIZE);
        esp_gps->statement |= GPGSA;
    }
    if(esp_gps->statement==GPALL)
    {
        parsegpsdata(esp_gps);
        esp_gps->statement=0;
    }
   // parsegpsdata(esp_gps);
    
    return ESP_OK;
}

//检测到GPS匹配字符
static void esp_handle_uart_pattern(esp_gps_t *esp_gps)
{
    int pos = uart_pattern_pop_pos(esp_gps->uart_port);
    if (pos != -1)
    {
        /* read one line(include '\n') */
        int read_len = uart_read_bytes(esp_gps->uart_port, esp_gps->buffer, pos + 1, 100 / portTICK_PERIOD_MS);
        /* make sure the line is a standard string */
        esp_gps->buffer[read_len] = '\0';
        //ESP_LOGW(GPS_TAG,"%s",esp_gps->buffer);
        /* Send new line to handle */
        if (gps_decode(esp_gps, read_len + 1) != ESP_OK)
        {
            ESP_LOGW(GPS_TAG, "GPS decode line failed");
        }
    }
    else
    {
        ESP_LOGW(GPS_TAG, "Pattern Queue Size too small");
        uart_flush_input(esp_gps->uart_port);
    }
}

//GPS信号解析任务
static void ATGM_parser_task_entry(void *arg)
{
    esp_gps_t *esp_gps = (esp_gps_t *)arg;
    uart_event_t event;
    while (1)
    {
        if (xQueueReceive(esp_gps->event_queue, &event, pdMS_TO_TICKS(200)))
        {
            switch (event.type)
            {
            case UART_DATA:
                break;
            case UART_FIFO_OVF:
                ESP_LOGW(GPS_TAG, "HW FIFO Overflow");
                uart_flush(esp_gps->uart_port);
                xQueueReset(esp_gps->event_queue);
                break;
            case UART_BUFFER_FULL:
                ESP_LOGW(GPS_TAG, "Ring Buffer Full");
                uart_flush(esp_gps->uart_port);
                xQueueReset(esp_gps->event_queue);
                break;
            case UART_BREAK:
                ESP_LOGW(GPS_TAG, "Rx Break");
                break;
            case UART_PARITY_ERR:
                ESP_LOGE(GPS_TAG, "Parity Error");
                break;
            case UART_FRAME_ERR:
                ESP_LOGE(GPS_TAG, "Frame Error");
                break;
            case UART_PATTERN_DET:
                esp_handle_uart_pattern(esp_gps);
                break;
            default:
                ESP_LOGW(GPS_TAG, "unknown uart event type: %d", event.type);
                break;
            }
        }
    }
    vTaskDelete(NULL);
}

//GPS信息解析初始化函数
esp_gps_t* ATGM336_parse_init(const ATGM336_parser_config_t *config)
{
    //初始化使能gpio口
    gpio_pad_select_gpio(ATGM336_Enable_Pin);
    gpio_set_direction(ATGM336_Enable_Pin,GPIO_MODE_OUTPUT_OD);//开漏输出
    
    esp_gps_t *esp_gps = calloc(1, sizeof(esp_gps_t));
    if (!esp_gps)
    {
        ESP_LOGE(GPS_TAG, "calloc memory for esp_fps failed");
        goto err_gps;
    }

    esp_gps->buffer = calloc(1, ATGM_PARSER_RUNTIME_BUFFER_SIZE);
    if (!esp_gps->buffer)
    {
        ESP_LOGE(GPS_TAG, "calloc memory for runtime buffer failed");
        goto err_buffer;
    }
    esp_gps->uart_port=config->uart.uart_port;
    esp_gps->statement &=0xFE;
    uart_config_t uart_config = {
        .baud_rate = config->uart.baud_rate,
        .data_bits = config->uart.data_bits,
        .parity = config->uart.parity,
        .stop_bits = config->uart.stop_bits,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    
    //安装驱动
    if (uart_driver_install(config->uart.uart_port, 256, 0, config->uart.event_queue_size,
                            &(esp_gps->event_queue), 0) != ESP_OK)
    {
        ESP_LOGE(GPS_TAG, "install uart driver failed");
        goto err_uart_install;
    }
    //设置uart串口参数
    if (uart_param_config(config->uart.uart_port, &uart_config) != ESP_OK)
    {
        ESP_LOGE(GPS_TAG, "config uart parameter failed");
        goto err_uart_config;
    }
    //设置引脚
    if (uart_set_pin(config->uart.uart_port,config->uart.tx_pin, config->uart.rx_pin,
                     UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK)
    {
        ESP_LOGE(GPS_TAG, "config uart gpio failed");
        goto err_uart_config;
    }
    //设置中断匹配模式
    uart_enable_pattern_det_baud_intr(config->uart.uart_port, '\n', 1, 9, 0, 0);
    //复位中断处理序列
    uart_pattern_queue_reset(config->uart.uart_port, config->uart.event_queue_size);
    uart_flush(config->uart.uart_port);
    //设置GPS数据发送频率 1HZ
    ATGM336_SendBuffer("$PCAS02,1000*2E");

    //创建解析GPS信息任务
    BaseType_t err = xTaskCreate(ATGM_parser_task_entry,
                                 "atgm_parser",
                                 CONFIG_ATGM_PARSER_TASK_STACK_SIZE,
                                 esp_gps,
                                 CONFIG_ATGM_PARSER_TASK_PRIORITY,
                                 &esp_gps->tsk_hd1);
    if(err != pdTRUE){
        ESP_LOGE(GPS_TAG,"craet ATGM parser task failed");
        goto err_task_create;
    }
    ESP_LOGI(GPS_TAG,"ATGM init OK");
    return esp_gps; //返回命令数据域
    /*Error Handling*/
err_task_create:
    esp_event_loop_delete(esp_gps->event_loop_hdl);
err_uart_install:
    uart_driver_delete(esp_gps->uart_port);
err_uart_config:
err_buffer:
    free(esp_gps->buffer);
err_gps:
    free(esp_gps);
    return NULL;
}

//开启GPS模块
void ATGM336_Enable(void)
{
    uart_flush(ATGM336_UART_PORT);
    uart_enable_intr_mask(ATGM336_UART_PORT,UART_INTR_CONFIG_FLAG|UART_INTR_CMD_CHAR_DET);
    gpio_set_level(ATGM336_Enable_Pin,1);
}

//关闭GPS模块
void ATGM336_Disable(void)
{
    //关闭模式匹配中断
    uart_disable_intr_mask(ATGM336_UART_PORT,UART_INTR_CONFIG_FLAG|UART_INTR_CMD_CHAR_DET);
    gpio_set_level(ATGM336_Enable_Pin,0);
}

//串口发送数据到GPS模块
static int sendData(const char *data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(ATGM336_UART_PORT, data, len);
    return txBytes;
}

void ATGM336_SendBuffer(const char *buffer){
    //printf(buffer);
    sendData(buffer);
}
#include "cmdword.h"
#include "WT588H.h"
#include "fastlz.h"
#include "uartcommon.h"

#define LORAWAN_PARSER_TASK_PRIORITY 8
#define LORAWAN_SEND_TASK_PRIORITY 7

typedef enum{
    LOCAT_CMDPOS=0,
    LOCAT_LEN=1,
    LOCAT_UTC=2,
    LOCAT_LAT=6,
    LOCAT_LON=10,
    LOCAT_SPEED=14,
    LOCAT_ACCX=15,
    LOCAT_ACCY=16,
    LOCAT_ACCZ=17,
    LOCAT_ANGLE=18,
    LOCAT_ALT=20,
    LOCAT_PDOP=22,
    LOCAT_POWER=24,
    LOCAT_CRC=25,
    LOCAT_TOTAL
}Loaction_index;

typedef enum{
    INFO_CMDPOS=0,
    INFO_LEN=1,
    INFO_UTC=2,
    INFO_WARN=6,
    INFO_ANGLE=7,
    INFO_CRC=8,
    INFO_TOTAL
}Info_index;


typedef enum{
    CONF_CMDPOS=0,
    CONF_LEN=1,
    CONF_UTC=2,
    CONF_SENSOR=6,
    CONF_UPLINK=7,
    CONF_CRC=9,
    CONF_TOTAL
}Conf_index;

typedef enum{
    POI_CMDPOS=0,
    POI_LEN=1,
    POI_UTC=2,
    POI_MESG=6,
    POI_CRC=7,
    POI_TOTAL
}POI_index;
/********************lora task **********************/
static QueueHandle_t LoraCmdsendEventQueue = NULL;
static TaskHandle_t LoraCmd_parser_tsk_hdl = NULL;
static TaskHandle_t LoraCmd_Send_tsk_hdl = NULL;

/******************message buffer*****************/
static uint8_t location_buffer[LOCAT_TOTAL]={0};
static uint8_t Info_buffer[INFO_TOTAL]={0};
static uint8_t Conf_buffer[CONF_TOTAL]={0};
static uint8_t POI_buffer[POI_TOTAL]={0};

static uint8_t package_buf[LARGE_PACKAGE_SIZE] = {0};
static RingBuf ringbuf_packet;
static uint32_t cmd=0;



POI_t POI_msg = {0};

Conf_t Conf_msg = {0};

//小端数据转换为大端数据
static void mymemcpy(void * dst,void * src,size_t size)
{
    uint8_t temp_buff[4]={0};
    uint8_t *src_ptr=(uint8_t *)src;
    uint8_t temp_cnt=0;
    uint8_t size_cnt=size;
    src_ptr+=size_cnt;
    while(size_cnt)
    {
        size_cnt--;
        src_ptr--;
        temp_buff[temp_cnt]=*src_ptr;
        temp_cnt++;
    }
    memcpy(dst,temp_buff,size);
    
}

//设置location命令字
void Send_LocationCmd(esp_gps_t *esp_gps, uint8_t *acc, uint8_t power)
{

    location_buffer[LOCAT_CMDPOS]=0x01;
    location_buffer[LOCAT_LEN]=23;
    mymemcpy(&location_buffer[LOCAT_UTC],&esp_gps->parent.Time,sizeof(uint32_t));
    mymemcpy(&location_buffer[LOCAT_LAT],&esp_gps->parent.latitude,sizeof(uint32_t));
    mymemcpy(&location_buffer[LOCAT_LON],&esp_gps->parent.longitude,sizeof(uint32_t));
    location_buffer[LOCAT_SPEED]=esp_gps->parent.speed;
    location_buffer[LOCAT_ACCX]=acc[X];
    location_buffer[LOCAT_ACCY]=acc[Y];
    location_buffer[LOCAT_ACCZ]=acc[Z];
    mymemcpy(&location_buffer[LOCAT_ANGLE],&esp_gps->parent.fangweijiao,sizeof(uint16_t));
    mymemcpy(&location_buffer[LOCAT_ALT],&esp_gps->parent.altitude,sizeof(uint16_t));
    mymemcpy(&location_buffer[LOCAT_PDOP],&esp_gps->parent.PDOP,sizeof(uint16_t));
    location_buffer[LOCAT_POWER]=power;
    location_buffer[LOCAT_CRC]=Get_Crc8(location_buffer,location_buffer[LOCAT_LEN]+2);

    cmd=0x01;
    #ifdef LORAWAN
    xQueueSend(LoraCmdsendEventQueue,&cmd,( TickType_t ) 10);
    #endif

    #ifdef NBLOT

    #endif
}

//设置info 命令字
void Send_infoCmd(esp_gps_t *esp_gps, uint8_t warn_msg)
{

    Info_buffer[INFO_CMDPOS]=0x02;
    Info_buffer[INFO_LEN]=7;
    mymemcpy(&Info_buffer[INFO_UTC],&esp_gps->parent.Time,sizeof(uint32_t));
    Info_buffer[INFO_WARN]=warn_msg;
    mymemcpy(&Info_buffer[INFO_ANGLE],&esp_gps->parent.fangweijiao,sizeof(uint16_t));
    Info_buffer[INFO_CRC]=Get_Crc8(Info_buffer,Info_buffer[INFO_LEN]+2);

    cmd=0x02;
    xQueueSend(LoraCmdsendEventQueue,&cmd,( TickType_t ) 10);
}

//解析下行数据
static uint8_t Parse_DownlinkMeg(uint8_t *meg)
{
    uint8_t len = meg[1];
    uint8_t crc = meg[len + 2];
    uint8_t i;
    uint8_t *cmd_prt;
    if (crc != Get_Crc8(meg, len + 2)) // crc校验
    {
        return false;
    }
    else
    {
        if (meg[0] == POI_CMD)
        {
            memset(POI_buffer,0,POI_TOTAL);
            cmd_prt = (uint8_t *)&POI_buffer;
            for (i = 0; i < len + 3; i++) //
            {
                cmd_prt[i] = meg[i];
            }
            //将字符串解析到对应结构体
            POI_msg.command=POI_buffer[POI_CMDPOS];
            POI_msg.len=POI_buffer[POI_LEN];
            mymemcpy(&POI_msg.UTC,&POI_buffer[POI_UTC],sizeof(uint32_t));
            POI_msg.POI_Meg=POI_buffer[POI_MESG];
            POI_msg.CRC=POI_buffer[POI_CRC];

        }
        else if (meg[0] == CONF_CMD)
        {
            memset(Conf_buffer,0,CONF_TOTAL);
            cmd_prt = (uint8_t *)&Conf_buffer;
            for (i = 0; i < len + 3; i++) //
            {
                cmd_prt[i] = meg[i];
            }
            //将字符串解析到对应结构体
            Conf_msg.command=Conf_buffer[CONF_CMDPOS];
            Conf_msg.len=Conf_buffer[CONF_LEN];
            mymemcpy(&Conf_msg.UTC,&Conf_buffer[CONF_UTC],sizeof(uint32_t));
            Conf_msg.SensorConf=Conf_buffer[CONF_SENSOR];
            mymemcpy(&Conf_msg.UplinkConf,&Conf_buffer[CONF_UPLINK],sizeof(uint16_t));
            Conf_msg.CRC=Conf_buffer[CONF_CRC];

        }
        return meg[0];
    }
}

static void Cmd_parser_entry(void *arg)
{
    uint8_t *rx_buf = (uint8_t *)arg;
    uint8_t cmd = 0;
    printf("lorawan cmd parser task\r\n");
    for (;;)
    {
        if (xQueueReceive(CmdParserEventQueue, rx_buf, portMAX_DELAY))
        {
            printf("receive downlink data\r\n");
            cmd = Parse_DownlinkMeg(rx_buf);
            switch (cmd)
            {
            case CONF_CMD:
                /* code */
                break;
            case POI_CMD:
                WT588_PlaySound(POI_msg.POI_Meg);
                break;
            default:
                printf("unknow msg\r\n");
                break;
            }
        }
    }
}


//分包发送
static uint32_t LargePacker_Send(void)
{
    
    uint8_t tempbuf[PACKAGE_SIZE+1]={0};
    uint8_t templz77buf[PACKAGE_SIZE]={0};
    uint32_t len=RingBuf_get_Byte2read(&ringbuf_packet);
    uint8_t i=len/PACKAGE_SIZE;
    while(i)
    {
        RingBuf_pop_length(&ringbuf_packet,tempbuf,PACKAGE_SIZE);
        /*按照指定格式分包发送，以包的最大大小发送
        ***********************************

        ***********************************
        */
        i--;
        bzero(tempbuf,sizeof(tempbuf));
        bzero(templz77buf,sizeof(templz77buf));
    }
    
    RingBuf_pop_length(&ringbuf_packet,tempbuf,len%PACKAGE_SIZE);
        /*按照指定格式分包发送，发送剩余数据
        ***********************************
        
        ***********************************
        */

    return true;
}



static void Cmd_Send_entry(void *arg)
{
    uint32_t cmd = 0;
   // uint8_t *rx_buffer=(uint8_t *)arg;
    printf("cmd send task\r\n");
    for (;;)
    {
        #ifdef LORAWAN
        if (xQueueReceive(LoraCmdsendEventQueue, &cmd, portMAX_DELAY))
        {
            // //检测是否重新连接
            printf("send uplink msg\r\n");
            if(!Ra08_IsConnect())
            {
                Ra08_SendMsg((uint8_t *)"test",strlen("test"));//发送测试数据，检测是否连接成功 
            }
            
            if (Ra08_IsConnect()) //是否有基站信号 Ra08_IsConnect()
            {
                printf("send msg direct\r\n");
                if (RingBuf_get_Byte2read(&ringbuf_packet)) //检测是否有分包需要发送  !RingBuf_uint8_t_get_Freesize(&ptr_package_ringbuf)
                {   
                    //直接发送
                    switch (cmd)
                    {
                    case LOCATION_CMD:
                        printf("send location cmd\r\n");
                        Ra08_SendMsg(location_buffer,location_buffer[LOCAT_LEN]+3);
                        break;
                    case INFO_CMD:
                        printf("send info cmd\r\n");
                        Ra08_SendMsg(Info_buffer,Info_buffer[INFO_LEN]+3);
                        break;

                    default:
                        printf("unknow msg\r\n");
                        break;
                    }
                }
                else 
                {
                    //分包发送
                    printf("send larget packet\r\n");
                    LargePacker_Send();
                }
            }
            else
            {
                //打包存储 放入环形数组
                printf("store msg\r\n");
                switch (cmd)
                {
                case LOCATION_CMD:
                    RingBuf_push_length(&ringbuf_packet,location_buffer,location_buffer[LOCAT_LEN]+3);
                    break;
                case INFO_CMD:
                    //LargePacket_Restore((uint8_t *)&info_buffer, info_buffer.len + 3);
                    RingBuf_push_length(&ringbuf_packet,Info_buffer,Info_buffer[INFO_LEN]+3);
                    break;
                default:
                    break;
                }
               
            }
        }
        #endif

        #ifdef NBLOT

        #endif



    }
}


static void LargePacket_Init(void)
{
    RingBuf_init(&ringbuf_packet,package_buf,LARGE_PACKAGE_SIZE);
}

void Cmd_task_stack_Init(uint8_t *rxbuffer)
{
    //初始化环形数组
    printf("init ringbuf\r\n");
    LargePacket_Init();
    //初始化命令解析队列
    printf("create parser event queue\r\n");
    CmdParserEventQueue = xQueueCreate(10, sizeof(uint8_t));
    //初始化命令发送队列
    printf("create cmd send queue\r\n");
    #ifdef LORAWAN
    LoraCmdsendEventQueue=xQueueCreate(10,sizeof(uint32_t));
    #endif

    // //开启接收命令解析任务
    xTaskCreate(
                Cmd_parser_entry,
                "lorawan_pareser_task",
                4096,
                NULL,
                LORAWAN_PARSER_TASK_PRIORITY,
                LoraCmd_parser_tsk_hdl);
    //开启命令发送任务
    xTaskCreate(
                Cmd_Send_entry,
                "lorawan_send_task",
                4096,
                NULL,
                LORAWAN_SEND_TASK_PRIORITY,
                LoraCmd_Send_tsk_hdl);
}

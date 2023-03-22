#include "SC7A20.h"
#include "esp_log.h"
#include <math.h>

#define CALIBRATE_TIME 50
#define GRAVITY 8192
#define DETECT_MOVE_TIME 10
#define DETECT_MOVE_THRESHOLD 100

int16_t acc[XYZ] = {0};

int16_t acc_bias[XYZ] = {0};

//读取sc7a20寄存器
static esp_err_t SC7A20_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, SC7A20_ADDR, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
}

//写入sc7a20寄存器
static esp_err_t SC7A20_write(uint8_t reg_addr, uint8_t data)
{
    int ret;
    uint8_t write_buf[2] = {reg_addr, data};
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, SC7A20_ADDR, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
    return ret;
}

//处理原始16bit 加速度数据
int16_t SC7A20_12bitComplement(uint8_t msb,uint8_t lsb)
{
    int16_t temp;
	temp=msb<<8|lsb;
	temp=temp>>4;   //只有高12位有效
	temp=temp & 0x0fff;
	if(temp&0x0800) //负数 补码==>原码
	{
		temp=temp&0x07ff; //屏弊最高位      
		temp=~temp;
		temp=temp+1;
		temp=temp&0x07ff;
		temp=-temp;       //还原最高位
	}	
	return temp;    
}

//获取sc7a20 加速度数据
void SC7A20_GetRaw(int16_t *acc_data)
{
    uint8_t temp[6]={0};
    int i;
    SC7A20_read(ACC_X_LSB|0x80, temp, sizeof(temp));
    // for(i=0;i<6;i++)
    // {
        
    //     printf("index %d data %x\r\n",i,temp[i]);
    // }

    
    // acc_data[X] = temp[0]+((int16_t)temp[1]<<8);
    // acc_data[Y] = temp[2]+((int16_t)temp[3]<<8);
    // acc_data[Z] = temp[4]+((int16_t)temp[5]<<8);
    acc_data[X]=SC7A20_12bitComplement(temp[1],temp[0]);
    acc_data[Y]=SC7A20_12bitComplement(temp[3],temp[2]);
    acc_data[Z]=SC7A20_12bitComplement(temp[5],temp[4]);
    // acc_data[X]=temp[1];
    // acc_data[X]<<=8;
    // acc_data[X]|=temp[0];
    // acc_data[X]>>=4;

    // acc_data[Y]=temp[3];
    // acc_data[Y]<<=8;
    // acc_data[Y]|=temp[2];
    // acc_data[Y]>>=4;

    // acc_data[Z]=temp[5];
    // acc_data[Z]<<=8;
    // acc_data[Z]|=temp[4];
    // acc_data[Z]>>=4;

}

// static void SC7A20_GetAllRaw(int16_t *data)
// {
//     uint8_t temp[6];
//     SC7A20_read(ACC_X_LSB,temp,sizeof(temp));
//     data[X]=temp[0]+((uint16_t)temp[1]<<8);
//     data[Y]=temp[2]+((uint16_t)temp[3]<<8);
//     data[Z]=temp[4]+((uint16_t)temp[5]<<8);
// }

// static void SC7A20_SetOffset(void)
// {
//     uint16_t i;
//     int32_t sum[XYZ]={0};
//     int16_t raw[XYZ]={0};
//     int16_t acc_temp[XYZ]={0};
//     for(i=0;i<CALIBRATE_TIME;i++)
//     {
//         SC7A20_GetAllRaw(raw);
//         raw[Z]-=GRAVITY;
//         for(j=0;j<XYZ;j++)
//         {
//             sum[j]+=raw[j];
//         }
//         vTaskDelay(20/portTICK_RATE_MS);
//     }
//     for(i=0;i<XYZ;i++)
//     {
//         acc_bias[i]=sum[i]/CALIBRATE_TIME;
//         acc_temp[i]=raw[i]-acc_bias[i];
//     }
//     acc_bias[Z]=sqrt(GRAVITY*GRAVITY-acc_temp[X]*acc_temp[X]-acc_temp[Y]*acc_temp[Y]);

// }

static void SC7A20_GetDate(int16_t *data)
{
    uint8_t i;
    int16_t raw[XYZ] = {0};
    SC7A20_GetRaw(raw);
    for (i = 0; i < XYZ; i++)
    {
        data[i] = raw[i] - acc_bias[i];
    }
}

//SC7A20 模块初始化
uint8_t SC7A20_Init(uint8_t acc_thr)
{
    uint8_t temp=0;

    ESP_ERROR_CHECK(SC7A20_read(CHIPID, &temp, sizeof(temp)));
    printf("chip id :%d",temp);
    if (temp != 0x11)
    {
        return false;
    }
    ESP_ERROR_CHECK(SC7A20_write(0x20, 0x37)); // 设置ODR25HZ 使能XYZ
    SC7A20_write(0x21, 0xbc); //开启高通滤波 设置截止频率
    SC7A20_write(0x23, 0x80); //输出不更新直到读取数据 ,+-2G,数字低通滤波
    // // SDO Ground
    // SC7A20_write(0x1e, 0x05);
    // SC7A20_write(0x57, 0x08);
    // set interrupt
    SC7A20_write(0x38, 0x15); //三轴单击检测中断使能
    // SC7A20_write(0x25, 0x02);    //中断低电平有效
    SC7A20_write(0x3a, acc_thr); //设置触发加速度阈值
    SC7A20_write(0x3b, acc_thr);    //检测时间阈值

    SC7A20_write(0x3c, 0x02); //延迟时间

    ESP_ERROR_CHECK(SC7A20_write(0x22, 0x80)); //中断在INT1脚

   

    return true;
}

uint8_t SC7A20_IsMove(void)
{
    //uint8_t i = 0;
    // uint32_t acc_temp[XYZ] = {0};
    // for (i = 0; i < DETECT_CRASH_TIME, i++)
    // {
    //     SC7A20_GetRaw(acc);
    //     acc_temp[X] += acc[X];
    //     acc_temp[Y] += acc[Y];
    //     acc_temp[Z] += acc[Z];
        
    // }
    // acc_temp[X] /=DETECT_CRASH_TIME;
    // acc_temp[Y] /=DETECT_CRASH_TIME;
    // acc_temp[Z] /=DETECT_CRASH_TIME;

   // return (sqrt(acc_temp[X]*acc_temp[X]+acc_temp[Y]*acc_temp[Y]+acc_temp[Z]*acc_temp[Z])>)

   SC7A20_GetRaw(acc);
   return (sqrt(acc[X]*acc[X]+acc[Y]*acc[Y]+acc[Z]*acc[Z])>DETECT_MOVE_THRESHOLD)?true:false;
}
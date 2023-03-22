#include "SC7A20.h"
#include "esp_log.h"
#include <math.h>

#define CALIBRATE_TIME 50
#define GRAVITY 8192
#define DETECT_MOVE_TIME 10
#define DETECT_MOVE_THRESHOLD 100

int16_t acc[XYZ] = {0};

int16_t acc_bias[XYZ] = {0};

//��ȡsc7a20�Ĵ���
static esp_err_t SC7A20_read(uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_write_read_device(I2C_MASTER_NUM, SC7A20_ADDR, &reg_addr, 1, data, len, I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
}

//д��sc7a20�Ĵ���
static esp_err_t SC7A20_write(uint8_t reg_addr, uint8_t data)
{
    int ret;
    uint8_t write_buf[2] = {reg_addr, data};
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, SC7A20_ADDR, write_buf, sizeof(write_buf), I2C_MASTER_TIMEOUT_MS / portTICK_RATE_MS);
    return ret;
}

//����ԭʼ16bit ���ٶ�����
int16_t SC7A20_12bitComplement(uint8_t msb,uint8_t lsb)
{
    int16_t temp;
	temp=msb<<8|lsb;
	temp=temp>>4;   //ֻ�и�12λ��Ч
	temp=temp & 0x0fff;
	if(temp&0x0800) //���� ����==>ԭ��
	{
		temp=temp&0x07ff; //�������λ      
		temp=~temp;
		temp=temp+1;
		temp=temp&0x07ff;
		temp=-temp;       //��ԭ���λ
	}	
	return temp;    
}

//��ȡsc7a20 ���ٶ�����
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

//SC7A20 ģ���ʼ��
uint8_t SC7A20_Init(uint8_t acc_thr)
{
    uint8_t temp=0;

    ESP_ERROR_CHECK(SC7A20_read(CHIPID, &temp, sizeof(temp)));
    printf("chip id :%d",temp);
    if (temp != 0x11)
    {
        return false;
    }
    ESP_ERROR_CHECK(SC7A20_write(0x20, 0x37)); // ����ODR25HZ ʹ��XYZ
    SC7A20_write(0x21, 0xbc); //������ͨ�˲� ���ý�ֹƵ��
    SC7A20_write(0x23, 0x80); //���������ֱ����ȡ���� ,+-2G,���ֵ�ͨ�˲�
    // // SDO Ground
    // SC7A20_write(0x1e, 0x05);
    // SC7A20_write(0x57, 0x08);
    // set interrupt
    SC7A20_write(0x38, 0x15); //���ᵥ������ж�ʹ��
    // SC7A20_write(0x25, 0x02);    //�жϵ͵�ƽ��Ч
    SC7A20_write(0x3a, acc_thr); //���ô������ٶ���ֵ
    SC7A20_write(0x3b, acc_thr);    //���ʱ����ֵ

    SC7A20_write(0x3c, 0x02); //�ӳ�ʱ��

    ESP_ERROR_CHECK(SC7A20_write(0x22, 0x80)); //�ж���INT1��

   

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
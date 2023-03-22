#include "RingBuf.h"

//环形数组初始化
void RingBuf_init(RingBuf * ringbuf,uint8_t * buffer,uint32_t length)
{
    ringbuf->buffer=buffer;
    ringbuf->length=length;
    ringbuf->read_pos=ringbuf->write_pos=0;
}
//环形数组入栈
void RingBuf_push(RingBuf * ringbuf,uint8_t data)
{
    ringbuf->buffer[ringbuf->write_pos]=data;
    uint32_t temp_write_pos=ringbuf->write_pos+1;
    if( temp_write_pos >= ringbuf->length ) 
    {
        temp_write_pos = 0; 
    }
    //数据满
    if(temp_write_pos==ringbuf->read_pos)
    {
        if(ringbuf->read_pos>=ringbuf->length-1)
        {
            ringbuf->read_pos=0;
        }
        else
        {
            ringbuf->read_pos++;
        }
    }
    ringbuf->write_pos=temp_write_pos;

}
//环形数组出栈
uint8_t RingBuf_pop(RingBuf * ringbuf)
{
    //数据空
    //返回上一次数据
    if(ringbuf->read_pos==ringbuf->write_pos)
    {
        uint32_t pos=ringbuf->read_pos;
        if(pos>=1) pos-=1;
        else pos=ringbuf->length-1;
        return ringbuf->buffer[pos];
    }
    uint8_t data=ringbuf->buffer[ringbuf->read_pos];
    if(ringbuf->read_pos>=ringbuf->length-1)
    {
        ringbuf->read_pos=0;
    }
    else
    {
        ringbuf->read_pos++;
    } 
    return data;
    
}
//获取可读数据数量
uint32_t RingBuf_get_Byte2read(RingBuf * ringbuf)
{
    if(ringbuf->write_pos>=ringbuf->read_pos)
    {
        return ringbuf->write_pos-ringbuf->read_pos;
    }
    else
    {
        return ringbuf->length+ringbuf->write_pos-ringbuf->read_pos;
    }
}
//获取剩余空间
uint32_t RingBuf_get_Freesize(RingBuf * ringbuf)
{
    if(ringbuf->write_pos>=ringbuf->read_pos)
    {
        return ringbuf->length+ringbuf->write_pos-ringbuf->read_pos;
    }
    else
    {
        return ringbuf->write_pos-ringbuf->read_pos;
    }
}


//读出指定数量数据
uint32_t RingBuf_pop_length(RingBuf * ringbuf,uint8_t * data,uint32_t length)
{
    uint32_t byte2read=RingBuf_get_Byte2read(ringbuf);
    if(length>byte2read)
    {
        length=byte2read;
    }
    uint32_t byte2end=ringbuf->length-ringbuf->read_pos;
    if(length<=byte2end)
    {
        memcpy(data,&ringbuf->buffer[ringbuf->read_pos],length);
        ringbuf->read_pos+=length;
    }
    else
    {
        memcpy(data,&ringbuf->buffer[ringbuf->read_pos],byte2end);
        memcpy(data,&ringbuf->buffer[0],length-byte2end);
        ringbuf->read_pos=length-byte2end;
    }
    return length;

}

//压入指定数量数据
void RingBuf_push_length(RingBuf * ringbuf,const uint8_t * data,uint32_t length)
{

    if(length>ringbuf->length)
    {
        length=ringbuf->length;
    }
    uint32_t byte2end=ringbuf->length-ringbuf->write_pos;
    if(length<=byte2end)
    {
        memcpy(&ringbuf->buffer[ringbuf->write_pos],data,length);
    }
    else
    {
        memcpy(&ringbuf->buffer[ringbuf->write_pos],data,byte2end);
        memcpy(&ringbuf->buffer[0],&data[byte2end],length-byte2end);
    }

    uint32_t temp_write_pos=ringbuf->write_pos+length;
    if(temp_write_pos>=ringbuf->length)
    {
        temp_write_pos-=length;
    }
    uint32_t freesize=RingBuf_get_Freesize(ringbuf);
    if(freesize<=length)
    {
        if(temp_write_pos+1>=ringbuf->length)
        {
            ringbuf->read_pos=0;
        }
        else
        {
            ringbuf->read_pos=temp_write_pos+1;
        }
    }
    ringbuf->write_pos=temp_write_pos;
}
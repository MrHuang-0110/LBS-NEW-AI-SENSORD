#include "kt782xx.h"
#include "spi.h"
#include "systick_delay.h"
 
static uint16_t ZeroData;
 
#define ZeroPoint 3650

 
static uint8_t KTH7823_WriteRegister_16bitSPI(uint8_t reg_addr, uint8_t reg_value)
{
    uint16_t write_frame;
    uint16_t read_back;
    
    // 构造写请求帧
    write_frame = (0x02 << 14) |          // 写命令: 10
                  ((reg_addr & 0x3F) << 8) | // 6位地址
                  (reg_value & 0xFF);       // 8位数据
    

    // 开始传输
      CS_LOW();
	     delay_us(1);
  
	    read_back = spi_write_harltByte(write_frame);
	    delay_ms(25);
	   
	    delay_us(1);
      CS_HIGH();
 
    return (uint8_t)((read_back >> 8) & 0xFF);
}

#if 0
void setKT782xxCfg(void)
{ 
	 KTH7823_WriteRegister_16bitSPI(0x09,0x80);
	 KTH7823_WriteRegister_16bitSPI(0x04,0xC0);
	 KTH7823_WriteRegister_16bitSPI(0x05,0xFF);
	 KTH7823_WriteRegister_16bitSPI(0x08,0x00); 
	 KTH7823_WriteRegister_16bitSPI(0x03,0x00);
	 KTH7823_WriteRegister_16bitSPI(0x02,0x00);
	 
	 kt_set_zero();
}
#endif
void setKT782xxCfg(void)
{ 
    // 首先，配置ABZ输出分辨率为128脉冲/圈
    // PPT值 = 期望分辨率 - 1 = 128 - 1 = 127
    // PPT[1:0] = 127的低2位 = 11 (二进制) = 0x03
    // PPT[9:2] = 127的高8位 = 00111111 (二进制) = 0x3F
    
    KTH7823_WriteRegister_16bitSPI(0x04, 0x03);  // PPT[1:0]=11, ZL=00, ZD=00
    
    // 配置PPT[9:2] = 0x3F
    KTH7823_WriteRegister_16bitSPI(0x05, 0x1F);  // PPT[9:2]=00011111
    
    // 配置ABZLIMIT = 7 (0.125MHz)
    KTH7823_WriteRegister_16bitSPI(0x08, 0x07);  // ABZLIMIT=7
    
    // 其他配置保持不变
    KTH7823_WriteRegister_16bitSPI(0x09, 0x80);
    KTH7823_WriteRegister_16bitSPI(0x03, 0x00);
    KTH7823_WriteRegister_16bitSPI(0x02, 0x00);
    
    kt_set_zero();
}
void KT782XX_Init(void)
{ 
    //_kt782.readByte = spi_red_byte;
		//_kt782.writeByte = spi_write_byte;
}

uint16_t read_angle(void)
{ 
 
  uint16_t result = 0;
   
	CS_LOW();
	MOSI_LOW();
  result = spi_write_harltByte(0x0);     
	CS_HIGH();
	
	return result;
}

void close_kt78xxcfg(void){ 
    uint16_t write_frame;
 
    // 构造写请求帧
    write_frame = (0x03 << 14) |          // 写命令: 10
                  ((0x28 & 0x3F) << 8) | // 6位地址
                  (0x02 & 0xFF);       // 8位数据
    

    // 开始传输
      CS_LOW();
	     delay_us(1);
  
	      spi_write_harltByte(write_frame);
	    delay_ms(25);
	   
	    delay_us(1);
      CS_HIGH();
       
}
// 计算设置零点后的角度输出
float calculate_angle_with_zero_offset(uint16_t spi_raw_value)
{
    // 处 理16位有符号差值
	  uint8_t hight_data = ZeroData>>8;
	  uint8_t low_data = ZeroData&0xFF;
	  uint16_t regangle = hight_data<<8|low_data;
      int32_t difference = ((int32_t)spi_raw_value - (int32_t)(regangle));
	 	if (difference < 0) {
         difference += 65536;
     } 
    // 转换为角度
    float angle = (difference * 360.0f) / 65536.0f;
    
    return angle;
}
void kt_set_zero(void)
{ 
 
	uint16_t reg;
	
	for(uint8_t i = 0;i<10;i++)
	{ 
	    reg = read_angle();
	}
	reg = read_angle();
	/*读取当前电机位置*/
  ZeroData =reg;
}  

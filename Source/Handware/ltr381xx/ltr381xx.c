#include "ltr381xx.h"
#include "math.h"
#include "bsp_i2c.h"
 
static ltr381_config_t cfg;
static ltr381_rgb_raw_t rgb; 
extern void COMXAnalysisProtocolSend(unsigned char id,unsigned char index,char *data);
 
static void ColorDiscrimiation_Write_Byte(u8 Slave_Addr,u8 Register,u8 DataToWrite)
{ 
    Sf_I2C_Start();//发送开始信号
	
	  Sf_I2C_SendOneByte(Slave_Addr<<1);//发送写命令
	
	  Sf_I2C_GetAck();//等待应答
	  
	  Sf_I2C_SendOneByte(Register);//发送寄存器地址
	
	  Sf_I2C_GetAck();//等待应答
	  
	  Sf_I2C_SendOneByte(DataToWrite);//发送写命令数据
	
	  Sf_I2C_GetAck();//等待应答
	
	  Sf_I2C_Stop();//发送停止
    
	  delay_us(5);
}
 
static uint8_t ColorDiscrimiation_Read_Byte(u8 Slave_Addr,u8 ReadAddr)
{ 
	  uint8_t tmp;
	
    Sf_I2C_Start();//发送开始信号
	  
	  Sf_I2C_SendOneByte(Slave_Addr<<1);//发送写命令
	  
	  Sf_I2C_GetAck();//等待应答
	
	  Sf_I2C_SendOneByte(ReadAddr);//发送寄存器地址
	  
	  Sf_I2C_GetAck();//等待应答
	  
	  /*接收*/
	  Sf_I2C_Start();//发送开始信号
	  
	  Sf_I2C_SendOneByte((Slave_Addr<<1)+1);//发送写命令
	  
	  Sf_I2C_GetAck();//等待应答
	
	  tmp = Sf_I2C_ReceiveOneByte(0);
	  
		Sf_I2C_Stop();//发送停止
		
    delay_us(5);	
		 
    return tmp;
}
int8_t ltr381_i2c_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len)
{ 
    for(uint8_t i = 0;i<len;i++)
	  {
		  ColorDiscrimiation_Write_Byte(dev_addr,reg_addr,data[i]);
		}
		return 0;
}
int8_t ltr381_i2c_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len)
{ 
    for(uint8_t i = 0;i<len;i++)
	  {
		  data[i] = ColorDiscrimiation_Read_Byte(dev_addr,reg_addr);
		}
		return 0;  
}
static void COLOR_LED_Init(void)
{ 
  GPIO_InitTypeDef GPIO_InitStructure;
  
  RCC_AHBPeriphClockCmd( RCC_AHBPeriph_GPIOD, ENABLE);
  
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(GPIOD, &GPIO_InitStructure);

  GPIO_ResetBits(GPIOD,GPIO_Pin_2);
	GPIO_ResetBits(GPIOD,GPIO_Pin_3);  
}
/* 内部辅助函数：读单字节 */
static int8_t _read_reg(uint8_t reg, uint8_t *val) {
    return ltr381_i2c_read(LTR381RGB_I2C_ADDR, reg, val, 1);
}

/* 内部辅助函数：写单字节 */
static int8_t _write_reg(uint8_t reg, uint8_t val) {
    return ltr381_i2c_write(LTR381RGB_I2C_ADDR, reg, &val, 1);
}

/* 内部辅助函数：读取20位数据 (从低字节开始连续读3字节) */
static int8_t _read_20bit(uint8_t reg_low, uint32_t *val) {
    uint8_t buf[3];
    int8_t ret = ltr381_i2c_read(LTR381RGB_I2C_ADDR, reg_low, buf, 3);
    if (ret == 0) {
        // 组合数据: 高4位来自 buf[2] 的低4位，中8位来自 buf[1]，低8位来自 buf[0]
        //*val = ((uint32_t)(buf[2] & 0x0F) << 16) | ((uint32_t)buf[1] << 8) | buf[0];
			  *val = ((uint32_t)((uint32_t)buf[1] << 8) | buf[0]);
    }
    return ret;
}

/* 初始化 */
int8_t ltr381_rgb_init(void) {
    uint8_t part_id;
    COLOR_LED_Init();
    // 读取 Part ID (默认应为 0xC2, Page 14)
    if (_read_reg(LTR381RGB_PART_ID, &part_id) != 0) return -1;
    
    // 检查 Part Number ID 高4位 (Page 14: 1100 = 0xC)
    if ((part_id & 0xF0) != 0xC0) return -1;
    
    // 执行软件复位，使所有寄存器恢复默认值
    return ltr381_rgb_soft_reset();
}
int8_t ltr381_cfg_init(void)
{
 // 2. 配置传感器: 使用 CS 模式 (RGB+IR), 分辨率 18bit/100ms, 增益 3x
    cfg.mode = LTR381RGB_MODE_CS;      // 色彩传感器模式，获取RGB
    cfg.enable = 1;                    // 激活测量
    cfg.resolution = LTR381RGB_16BIT_25MS;
    cfg.meas_rate = LTR381RGB_RATE_25MS;
    cfg.gain = LTR381RGB_GAIN_1;
    
    if (ltr381_rgb_configure(&cfg) != 0) {
 
        return -1;
    } 
		return 0;
} 
/* 软件复位 */
int8_t ltr381_rgb_soft_reset(void) {
    uint8_t reg;
    int8_t ret;
    
    // 读取 MAIN_CTRL
    ret = _read_reg(LTR381RGB_MAIN_CTRL, &reg);
    if (ret != 0) return ret;
    
    // 设置 SW_RESET 位
    reg |= LTR381RGB_SW_RESET;
    ret = _write_reg(LTR381RGB_MAIN_CTRL, reg);
    if (ret != 0) return ret;
    
    // 等待复位完成 (数据手册建议等待至少 10ms)
    // 嵌入式平台请替换为 HAL_Delay(10) 或 vTaskDelay(pdMS_TO_TICKS(10))
    delay_ms(10);
    
    return 0;
}

/* 配置传感器 */
int8_t ltr381_rgb_configure(const ltr381_config_t *cfg) {
    uint8_t meas_reg, gain_reg, ctrl_reg;
    int8_t ret;
    
    // 1. 配置测量速率和分辨率 (寄存器 0x04)
    meas_reg = (cfg->resolution & LTR381RGB_RESOLUTION_MASK) | (cfg->meas_rate & LTR381RGB_MEAS_RATE_MASK);
    ret = _write_reg(LTR381RGB_ALS_CS_MEAS_RATE, meas_reg);
    if (ret != 0) return ret;
    
    // 2. 配置增益 (寄存器 0x05)
    gain_reg = (cfg->gain & 0x07);  // 低3位有效
    ret = _write_reg(LTR381RGB_ALS_CS_GAIN, gain_reg);
    if (ret != 0) return ret;
    
    // 3. 配置模式和使能 (寄存器 0x00)
    ctrl_reg = 0;
    if (cfg->mode == LTR381RGB_MODE_CS) {
        ctrl_reg |= LTR381RGB_CS_MODE;   // CS模式 (RGB+IR)
    }
    if (cfg->enable) {
        ctrl_reg |= LTR381RGB_ALS_CS_ENABLE;  // 激活测量
    }
    ret = _write_reg(LTR381RGB_MAIN_CTRL, ctrl_reg);
 
uint8_t reg04, reg05, reg00;
if (ltr381_i2c_read(LTR381RGB_I2C_ADDR, 0x04, &reg04, 1) != 0) {
     ;
}
if (ltr381_i2c_read(LTR381RGB_I2C_ADDR, 0x05, &reg05, 1) != 0) {
     ;
}
if (ltr381_i2c_read(LTR381RGB_I2C_ADDR, 0x00, &reg00, 1) != 0) {
    ;
}
 
    return ret;
}

/* 获取当前配置 */
int8_t ltr381_rgb_get_config(ltr381_config_t *cfg) {
    uint8_t meas_reg, gain_reg, ctrl_reg;
    
    if (_read_reg(LTR381RGB_ALS_CS_MEAS_RATE, &meas_reg) != 0) return -1;
    if (_read_reg(LTR381RGB_ALS_CS_GAIN, &gain_reg) != 0) return -1;
    if (_read_reg(LTR381RGB_MAIN_CTRL, &ctrl_reg) != 0) return -1;
    
    cfg->resolution = (ltr381_resolution_t)(meas_reg & LTR381RGB_RESOLUTION_MASK);
    cfg->meas_rate = (ltr381_meas_rate_t)(meas_reg & LTR381RGB_MEAS_RATE_MASK);
    cfg->gain = (ltr381_gain_t)(gain_reg & 0x07);
    cfg->mode = (ctrl_reg & LTR381RGB_CS_MODE) ? LTR381RGB_MODE_CS : LTR381RGB_MODE_ALS;
    cfg->enable = (ctrl_reg & LTR381RGB_ALS_CS_ENABLE) ? 1 : 0;
    
    return 0;
}

/* 读取原始RGB和IR数据 */
int8_t ltr381_rgb_read_raw(ltr381_rgb_raw_t *rgb) {
    // 读取 IR 通道 (0x0A, 0x0B, 0x0C)
    if (_read_20bit(LTR381RGB_CS_DATA_IR_0, &rgb->ir) != 0) return -1;
    // 读取绿色通道 (0x0D, 0x0E, 0x0F)
    if (_read_20bit(LTR381RGB_CS_DATA_GREEN_0, &rgb->green) != 0) return -1;
    // 读取蓝色通道 (0x10, 0x11, 0x12)
    if (_read_20bit(LTR381RGB_CS_DATA_BLUE_0, &rgb->blue) != 0) return -1;
    // 读取红色通道 (0x13, 0x14, 0x15)
    if (_read_20bit(LTR381RGB_CS_DATA_RED_0, &rgb->red) != 0) return -1;
    
    return 0;
}

/* 检查数据就绪状态 (新数据标志) */
int8_t ltr381_rgb_data_ready(uint8_t *new_data) {
    uint8_t status;
    if (_read_reg(LTR381RGB_MAIN_STATUS, &status) != 0) return -1;
    // Bit 3: CS/ALS Data Status (Page 15: 1 = 新数据未读, 0 = 旧数据/已读)
    *new_data = (status & 0x08) ? 1 : 0;
    return 0;
}

/* 计算 Lux (依据数据手册 Page 23 公式) */
int8_t ltr381_rgb_read_lux(float *lux) {
    ltr381_rgb_raw_t rgb;
    ltr381_config_t cfg;
    float green_val, ir_val;
    float gain_factor, int_time_factor;
    float tmp;
    
    // 获取当前配置 (需要增益和分辨率来计算系数)
    if (ltr381_rgb_get_config(&cfg) != 0) return -1;
    
    // 读取原始数据 (需要绿色和IR通道)
    if (ltr381_rgb_read_raw(&rgb) != 0) return -1;
    
    // 获取增益系数 (数据手册 Page 24 表格)
    switch(cfg.gain) {
        case LTR381RGB_GAIN_1:  gain_factor = 1.0f; break;
        case LTR381RGB_GAIN_3:  gain_factor = 3.0f; break;
        case LTR381RGB_GAIN_6:  gain_factor = 6.0f; break;
        case LTR381RGB_GAIN_9:  gain_factor = 9.0f; break;
        case LTR381RGB_GAIN_18: gain_factor = 18.0f; break;
        default: gain_factor = 3.0f; break;
    }
    
    // 获取积分时间系数 (数据手册 Page 24 表格)
    switch(cfg.resolution) {
        case LTR381RGB_20BIT_400MS: int_time_factor = 4.0f; break;  // 400ms / 100ms = 4
        case LTR381RGB_19BIT_200MS: int_time_factor = 2.0f; break;
        case LTR381RGB_18BIT_100MS: int_time_factor = 1.0f; break;
        case LTR381RGB_17BIT_50MS:  int_time_factor = 0.5f; break;
        case LTR381RGB_16BIT_25MS:  int_time_factor = 0.25f; break;
        default: int_time_factor = 1.0f; break;
    }
    
    // 转换为浮点数
    green_val = (float)rgb.green;
    ir_val = (float)rgb.ir;
    
    // 应用数据手册公式: Lux = 0.74 * Green / (Gain * Int) * (1 - C1*(IR/Green)) * W_FAC
    // C1 = 0.033 (常数)
    // W_FAC = 1.0 (假设无玻璃盖或透光率已校准)
    tmp = (ir_val / green_val);
    // 避免除零和负数情况
    if (green_val < 1e-6) {
        *lux = 0.0f;
        return 0;
    }
    if (tmp > 30.0f) tmp = 30.0f;  // 限制比值范围
    
   // *lux = (0.74f * green_val) / (gain_factor * int_time_factor) * (1.0f - 0.033f * tmp);
		 *lux = (0.74f * green_val) / (gain_factor * int_time_factor);
     float comp = 1.0f - 0.033f * ir_val / green_val;
    // 确保非负
    if (*lux < 0) *lux = 0;
    
    return 0;
}
 
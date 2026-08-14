#include "rc522.h"

// SPI 外设初始化
void RC522_SPI_Init(void)
{
    // SPI1 初始化，由 CubeMX 配置
    HAL_SPI_Init(&hspi1);
}

// 初始化 RC522
void RC522_Init(void)
{
    // 复位 RC522 模块
    HAL_GPIO_WritePin(RC522_RST_GPIO_PORT, RC522_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(RC522_RST_GPIO_PORT, RC522_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
}

// 选择 RC522 片选
void RC522_Select(void)
{
    HAL_GPIO_WritePin(RC522_CS_GPIO_PORT, RC522_CS_PIN, GPIO_PIN_RESET); // 拉低片选
}

// 取消选择 RC522
void RC522_Deselect(void)
{
    HAL_GPIO_WritePin(RC522_CS_GPIO_PORT, RC522_CS_PIN, GPIO_PIN_SET); // 拉高片选
}

// 通过 SPI 发送一个字节并接收返回的数据
uint8_t RC522_SPI_Transmit(uint8_t data)
{
    uint8_t received_data = 0;
    HAL_SPI_TransmitReceive(&hspi1, &data, &received_data, 1, 100);
    return received_data;
}

// 向 RC522 写寄存器
void RC522_WriteRegister(uint8_t reg, uint8_t value)
{
    RC522_Select();
    RC522_SPI_Transmit((reg << 1) & 0x7E);   // 写寄存器命令
    RC522_SPI_Transmit(value);
    RC522_Deselect();
}

// 读取 RC522 寄存器
uint8_t RC522_ReadRegister(uint8_t reg)
{
    uint8_t value;
    RC522_Select();
    RC522_SPI_Transmit(((reg << 1) & 0x7E) | 0x80);  // 读寄存器命令
    value = RC522_SPI_Transmit(0x00);  // 发送空数据接收寄存器值
    RC522_Deselect();
    return value;
}

// 启动 RC522 命令
void RC522_Command(uint8_t command)
{
    RC522_WriteRegister(CommandReg, command); // 向 Command 寄存器写入命令
    HAL_Delay(50); // 等待指令完成
}

// 认证函数
uint8_t RC522_Authenticate(uint8_t sector, uint8_t block, uint8_t *key)
{
    uint8_t status;
    
    uint8_t authData[12] = {0};
    authData[0] = PCD_AUTHENT;  // 认证命令
    authData[1] = (sector * 4 + block) & 0x0F;  // 选择块地址
    
    for (int i = 0; i < 6; i++) 
    {
        authData[2 + i] = key[i];  // 填充密钥
    }

    // 向 RC522 发送认证命令和数据
    RC522_Select();
    for (int i = 0; i < 12; i++) 
    {
        RC522_SPI_Transmit(authData[i]);
    }
    RC522_Deselect();

    // 检查认证是否成功
    status = RC522_ReadRegister(ComIrqReg);
    if (status & 0x08)  // 认证成功标志
    {
        return 1;  // 成功
    }
    return 0;  // 失败
}

// 读取指定块的数据
uint8_t RC522_ReadBlock(uint8_t sector, uint8_t block, uint8_t *buffer)
{
    uint8_t status;
    uint8_t blockAddr = sector * SECTOR_SIZE + block;
    
    // 认证
    status = RC522_Authenticate(sector, block, (uint8_t *)KEY_A);
    if (status != 1)
    {
        // 认证失败
        return 0;
    }

    // 读取数据
    RC522_Command(PCD_TRANSCEIVE);  // 发送读取命令
    status = RC522_ReadRegister(ComIrqReg); // 检查命令执行状态
    
    if (status & 0x01)  // 成功读取
    {
        // 读取块数据
        for (int i = 0; i < 16; i++) // 每块有 16 字节
        {
            buffer[i] = RC522_ReadRegister(FIFODataReg + i);
        }
        return 1;  // 成功
    }
    return 0;  // 失败
}

// 检查是否有卡片
uint8_t RC522_CheckCard(void)
{
    uint8_t status;
    
    RC522_Command(PCD_IDLE);  // 确保 RC522 处于空闲状态
    status = RC522_ReadRegister(ComIrqReg);  // 检查中断状态
    
    if (status & 0x01)  // 卡片存在
        return 1;
    else
        return 0;
}

// 读取卡片 UID
uint8_t RC522_ReadUID(uint8_t *uid)
{
    uint8_t status;
    
    // 启动读取卡片 UID
    RC522_Command(PCD_TRANSCEIVE);
    status = RC522_ReadRegister(ComIrqReg);  // 读取中断寄存器，判断是否读取成功
    if (status & 0x01)
    {
        for (int i = 0; i < 4; i++)
        {
            uid[i] = RC522_ReadRegister(FIFODataReg + i); // 读取 UID 数据
        }
        return 1;  // 成功读取
    }
    return 0;  // 读取失败
}

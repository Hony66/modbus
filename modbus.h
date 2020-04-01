#ifndef __MODBUS_H
#define __MODBUS_H

#ifndef NULL
	#define NULL 0
#endif
          
//***** user config area start****************************************************************

//modbus 只读和可读写区域之间可以相邻，不能重叠
#define MODBUS_RW_REG_START               0U       //《自定义》
#define MODBUS_RW_REG_TOTAL               1U       //《自定义》
#define MODBUS_RO_REG_START               1U       //《自定义》
#define MODBUS_RO_REG_TOTAL               1U       //《自定义》
//钩子函数
#define MODBUS_HOOK_FCODE_RD              0U       //《自定义》
#define MODBUS_HOOK_FCODE_RD_PARAM        1U       //《自定义》
#define MODBUS_HOOK_FCODE_WT              0U       //《自定义》
//用户DIY功能码
//#define MODBUS_FUNC_CODE_DIY1                    //《自定义》
//扩展从机地址，相当于广播地址，但是从机响应此广播
#define MODBUS_EX_SLAVE_ADDR              0xF8U    //《自定义》

//***** user config area end ******************************************************************



#define MODBUS_RW_REG_END (MODBUS_RW_REG_START + MODBUS_RW_REG_TOTAL - 1)
#define MODBUS_RO_REG_END (MODBUS_RO_REG_START + MODBUS_RO_REG_TOTAL - 1)
//广播地址
#define MODBUS_SLAVE_BROADCAST_ADDR     0U   //广播地址
//功能码
#define MODBUS_FUNC_CODE_R              0x03 //读单个或多个寄存器
#define MODBUS_FUNC_CODE_W_MULTI        0x10 //写多个寄存器
#define MODBUS_FUNC_CODE_W_SINGLE       0x06 //写单个寄存器
//一次传输的寄存器最多个数
#define MODBUS_TRANS_REG_MAX            124u
//返回的错误代码
#define MODBUS_ERR_CODE_FUNC            0X01
#define MODBUS_ERR_CODE_REG             0X02
#define MODBUS_ERR_CODE_DATA            0X03
#define MODBUS_ERR_CODE_FAULT           0X04
#define MODBUS_ERR_CODE_BUSY            0X06

typedef enum{
    MODBUS_OK = 0,
    MODBUS_CRC_ERR,
    MODBUS_ADDR_ERR,
    MODBUS_LEN_ERR, 
    MODBUS_INPUT_ERR,
}Modbus_State_Enum;

typedef enum
{
    ERR = 0,
    OK = 1,
} Func_State_Enum;

typedef struct 
{
    unsigned char * pTr;//提取的数据域指针
    unsigned char offset;//寄存器偏移
    unsigned char nRegs;//提取的寄存器个数
} Modbus_Extract_TypeDef;

typedef struct
{
    unsigned char SlaveAddr;//从机地址
    const unsigned char *pRxBuffer; //接收数据缓冲地址
    unsigned char RxLen;//接收到的长度
    unsigned char * pTxBuffer;//发送缓存地址
    const unsigned short * pRW;//可读可写区域指针
    const unsigned short * pRO;//只读区域指针
    Func_State_Enum (*func_write_regs_check)(Modbus_Extract_TypeDef *); //写寄存器时，参数有效性检查
    void (*func_save_regs)(Modbus_Extract_TypeDef *);//保存写入的寄存器值
    void (*func_send_data)(const unsigned char *, unsigned short); //发送数据
    unsigned short (*func_crc16)(const unsigned char *, unsigned short); //校验函数
    Modbus_Extract_TypeDef extract_reg_data;//提取接收的寄存器数据；
}Modbus_TypeDef;

Modbus_State_Enum modbus_proc(Modbus_TypeDef *modbus);

#endif
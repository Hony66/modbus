#ifndef __MODBUS_H
#define __MODBUS_H

#ifndef NULL
#define NULL 0
#endif

typedef struct 
{
    unsigned char * pTr;//提取的数据域指针
    unsigned char offset;//寄存器偏移
    unsigned char nRegs;//提取的寄存器个数
} Modbus_Extract_TypeDef;


//***** user config area start****************************
//***** 区域之间必须不能重叠 **********************************
#include "sys.h"
#define MODBUS_RW_REG_START SYS_RW_REG_START
#define MODBUS_RW_REG_TOTAL SYS_RW_REG_TOTAL
#define MODBUS_RO_REG_START SYS_RO_REG_START
#define MODBUS_RO_REG_TOTAL SYS_RO_REG_TOTAL
//扩展数据区间，用于特殊指令
#define MODBUS_EX_REG_START SYS_EX_REG_START
#define MODBUS_EX_REG_TOTAL SYS_EX_REG_TOTAL

#define MODBUS_HOOK_FCODE_RD              0U
#define MODBUS_HOOK_FCODE_RD_PARAM        1U
#define MODBUS_HOOK_FCODE_WT              0U
//***** user config area end ****************************

#define MODBUS_RW_REG_END (MODBUS_RW_REG_START + MODBUS_RW_REG_TOTAL - 1)
#define MODBUS_RO_REG_END (MODBUS_RO_REG_START + MODBUS_RO_REG_TOTAL - 1)
//扩展数据区间，用于特殊指令
#define MODBUS_EX_REG_END (MODBUS_EX_REG_START + MODBUS_EX_REG_TOTAL - 1)

//显示器专用地址
#define MODBUS_DISPLAYER_ADDR      0xF8U

//广播地址
#define MODBUS_SLAVE_BROADCAST_ADDR      0U
//功能码
#define MODBUS_FUNC_CODE_R 0x03 //读单个或多个寄存器
#define MODBUS_FUNC_CODE_W_MULTI 0x10 //写多个寄存器
//自定义功能码
#include "boot.h"
#define MODBUS_FUNC_CODE_DIY1          GOTO_BOOT   
#define MODBUS_FUNC_CODE_DIY2          SYS_DATA_EX_FCODE   

//返回的错误代码
#define MODBUS_ERR_CODE_FUNC 0X01
#define MODBUS_ERR_CODE_REG 0X02
#define MODBUS_ERR_CODE_DATA 0X03
#define MODBUS_ERR_CODE_FAULT 0X04
#define MODBUS_ERR_CODE_BUSY 0X06

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



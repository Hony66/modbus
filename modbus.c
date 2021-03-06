/**
  ******************************************************************************
  * @file    modbus.c
  * @author  Hony
  * @version V1.1b
  * @date    2020-05-16
  * @brief   MODBUS RTU 协议文件, 支持 0x03, 0x06, 0x10功能码
  @verbatim
  2018-08-07 V1.0
    创建, 支持 0x03读保持寄存器, 0x10连续写寄存器功能码
  2019-10-21 V1.1
    支持 0x06 写单个寄存器 功能码
  2019-04-01 V1.1a
    增加DIY功能码, 目前支持03,06,10功能码；支持0地址广播；支持固定地址回复；
  2020-05-16 V1.1b
    修正发送函数指针的未检查，防止空指针
  @endverbatim
  ******************************************************************************
  * @attention
  * 
  ******************************************************************************  
  */

#include "modbus.h"


#if MODBUS_HOOK_FCODE_RD == 1u
//读命令钩子函数
__WEAK void modbus_fcode_rd_hook(void)
{
    ;
}
#endif

#if MODBUS_HOOK_FCODE_RD_PARAM == 1u
//读参数区钩子函数
__WEAK void modbus_fcode_rd_param_hook(void)
{
    ;
}
#endif

#if MODBUS_HOOK_FCODE_WT == 1u
//写命令钩子函数
__WEAK void modbus_fcode_wt_hook(void)
{
    ;
}
#endif

#ifdef MODBUS_FUNC_CODE_DIY1
__WEAK unsigned char modbus_fcode_diy1_hook(Modbus_TypeDef * modbus)
{
    (void)modbus;
    return 0;
}
#endif

/**
  * @brief  modbus_proc 数据传输协议处理
  * @param  modbus 参见Modbus_TypeDef定义
  * @retval Modbus_State_Enum  value in 
  *         MODBUS_OK = 0,
            MODBUS_CRC_ERR,
            MODBUS_ADDR_ERR,
            MODBUS_LEN_ERR, 
            MODBUS_INPUT_ERR,
  */
Modbus_State_Enum modbus_proc(Modbus_TypeDef *modbus)
{
    #define CFLAG_IS_SAVE_REG     (1<<0)
    #define CFALG_IS_RD_PARAM     (1<<1)
    #define CFALG_IS_NEED_REPLY   (1<<2)
    unsigned char i;
    unsigned char cal_crc_len;
    unsigned char cflag = 0;//bit0 是否需要保存参数, bit1 是否读了参数区
    unsigned short startReg;
    unsigned short nReg;
    unsigned short crc16;
    cflag |= CFALG_IS_NEED_REPLY;
    //参数检查
    if (modbus == NULL 
        || modbus->pRxBuffer == NULL 
        || modbus->pTxBuffer == NULL 
        || modbus->func_send_data == NULL
        || modbus->func_crc16 == NULL)
    {
        return MODBUS_INPUT_ERR;
    }
    //校验检查
    if (modbus->func_crc16(modbus->pRxBuffer, modbus->RxLen) != 0)
    {
        return MODBUS_CRC_ERR;
    }
    //地址匹配
	#ifdef MODBUS_EX_SLAVE_ADDR
	if (modbus->pRxBuffer[0] != modbus->SlaveAddr 
        && modbus->pRxBuffer[0] != MODBUS_SLAVE_BROADCAST_ADDR
        && modbus->pRxBuffer[0] != MODBUS_EX_SLAVE_ADDR)
	#else
    if (modbus->pRxBuffer[0] != modbus->SlaveAddr 
        && modbus->pRxBuffer[0] != MODBUS_SLAVE_BROADCAST_ADDR)
	#endif
    {
        return MODBUS_ADDR_ERR;
    }
    
    modbus->pTxBuffer[0] = modbus->pRxBuffer[0];
    modbus->pTxBuffer[1] = modbus->pRxBuffer[1];
    //功能码匹配
    switch (modbus->pRxBuffer[1])
    {
    case MODBUS_FUNC_CODE_R:
        //得到读寄存器起始地址
        startReg = (((unsigned short)modbus->pRxBuffer[2]) << 8) + modbus->pRxBuffer[3];
        //得到读寄存器个数
        nReg = (((unsigned short)modbus->pRxBuffer[4]) << 8) + modbus->pRxBuffer[5];
        modbus->pTxBuffer[2] = nReg * 2;
        cal_crc_len = modbus->pTxBuffer[2] + 3;
        if(nReg > MODBUS_TRANS_REG_MAX)
        {
            return MODBUS_LEN_ERR;
        }
        //长度匹配
        if (modbus->RxLen!= 8)
        {
            return MODBUS_LEN_ERR;
        }
#if MODBUS_RO_REG_START == (MODBUS_RW_REG_END + 1)//RW + RO
        //寄存器地址匹配
        #if MODBUS_RW_REG_START == 0u
        if (nReg != 0 && nReg <= (MODBUS_RO_REG_TOTAL + MODBUS_RW_REG_TOTAL)  && startReg <= MODBUS_RO_REG_END && (startReg + nReg) <= (MODBUS_RO_REG_END + 1))
        #else
        if (nReg != 0 && nReg <= (MODBUS_RO_REG_TOTAL + MODBUS_RW_REG_TOTAL) && startReg >= MODBUS_RW_REG_START && startReg <= MODBUS_RO_REG_END && (startReg + nReg) <= (MODBUS_RO_REG_END + 1))
        #endif
        { //在实时数据区
            for (i = 0; i < nReg; i++)
            {
                if ((startReg + i) > MODBUS_RW_REG_END) //在RO区
                {
                    modbus->pTxBuffer[(i << 1) + 3] = modbus->pRO[startReg + i - MODBUS_RO_REG_START]>>8;
                    modbus->pTxBuffer[(i << 1) + 4] = modbus->pRO[startReg + i - MODBUS_RO_REG_START];
                }

                else //在RW区
                {
                    modbus->pTxBuffer[(i << 1) + 3] = modbus->pRW[startReg + i - MODBUS_RW_REG_START]>>8;
                    modbus->pTxBuffer[(i << 1) + 4] = modbus->pRW[startReg + i - MODBUS_RW_REG_START];
                }
            }
#if MODBUS_HOOK_FCODE_RD_PARAM == 1u
            if(startReg  <=  MODBUS_RW_REG_END)
            {
                cflag |= CFALG_IS_RD_PARAM;
            }
#endif
            cal_crc_len = modbus->pTxBuffer[2] + 3;
        }
#elif MODBUS_RW_REG_START == (MODBUS_RO_REG_END + 1) //RO + RW
        //寄存器地址匹配
        if (nReg != 0 && nReg <= (MODBUS_RO_REG_TOTAL + MODBUS_RW_REG_TOTAL) && startReg >= MODBUS_RO_REG_START && startReg <= MODBUS_RW_REG_END && (startReg + nReg) <= (MODBUS_RW_REG_END + 1))
        { //在实时数据区
            for (i = 0; i < nReg; i++)
            {
                if ((startReg + i) > MODBUS_RO_REG_END) //在RW区
                {
                    modbus->pTxBuffer[(i << 1) + 3] = modbus->pRW[startReg + i - MODBUS_RW_REG_START] >> 8;
                    modbus->pTxBuffer[(i << 1) + 4] = modbus->pRW[startReg + i - MODBUS_RW_REG_START];
                }

                else //在RO区
                {
                    modbus->pTxBuffer[(i << 1) + 3] = modbus->pRO[startReg + i - MODBUS_RO_REG_START] >> 8;
                    modbus->pTxBuffer[(i << 1) + 4] = modbus->pRO[startReg + i - MODBUS_RO_REG_START];
                }
            }
#if MODBUS_HOOK_FCODE_RD_PARAM == 1u
            if (startReg + nReg >= MODBUS_RW_REG_START)
            {
                cflag |= CFALG_IS_RD_PARAM;
            }
#endif
            cal_crc_len = modbus->pTxBuffer[2] + 3;
        }
#else
        //寄存器地址匹配
        if (nReg != 0 && nReg <= MODBUS_RO_REG_TOTAL && startReg >= MODBUS_RO_REG_START && startReg <= MODBUS_RO_REG_END && (startReg + nReg) <= (MODBUS_RO_REG_END + 1))
        { //在实时数据区
            for (i = 0; i < nReg; i++)
            {
                modbus->pTxBuffer[(i << 1) + 3] = modbus->pRO[startReg + i - MODBUS_RO_REG_START]>>8;
                modbus->pTxBuffer[(i << 1) + 4] = modbus->pRO[startReg + i - MODBUS_RO_REG_START];
            }
            cal_crc_len = modbus->pTxBuffer[2] + 3;
        }
#if MODBUS_RW_REG_START == 0u
        else if (nReg != 0 && nReg <= MODBUS_RW_REG_TOTAL /*&& startReg >= MODBUS_RW_REG_START*/ && startReg <= MODBUS_RW_REG_END && (startReg + nReg) <= (MODBUS_RW_REG_END + 1))
#else
        else if (nReg != 0 && nReg <= MODBUS_RW_REG_TOTAL && startReg >= MODBUS_RW_REG_START && startReg <= MODBUS_RW_REG_END && (startReg + nReg) <= (MODBUS_RW_REG_END + 1))
#endif
        { //在系统参数区
            for (i = 0; i < nReg; i++)
            {
                modbus->pTxBuffer[(i << 1) + 3] = modbus->pRW[startReg + i - MODBUS_RW_REG_START]>>8;
                modbus->pTxBuffer[(i << 1) + 4] = modbus->pRW[startReg + i - MODBUS_RW_REG_START];
            }
            cal_crc_len = modbus->pTxBuffer[2] + 3;
#if MODBUS_HOOK_FCODE_RD_PARAM == 1u
            cflag |= CFALG_IS_RD_PARAM;
#endif
        }
#endif
        else
        {
            /* err reg addr */
            modbus->pTxBuffer[1] |= 0X80;
            modbus->pTxBuffer[2] = MODBUS_ERR_CODE_REG;
            cal_crc_len = 3;
        }
#if MODBUS_HOOK_FCODE_RD == 1u
        modbus_fcode_rd_hook();
#endif
#if MODBUS_HOOK_FCODE_RD_PARAM == 1u
        if(cflag & CFALG_IS_RD_PARAM)
        {
            modbus_fcode_rd_param_hook();
        }
#endif
        break;
    case MODBUS_FUNC_CODE_W_SINGLE:
        {
            //得到写寄存器地址
            startReg = (((unsigned short)modbus->pRxBuffer[2]) << 8) + modbus->pRxBuffer[3];
            #if MODBUS_RW_REG_START == 0u
            if(startReg <= MODBUS_RW_REG_END)
            #else
            if(startReg >= MODBUS_RW_REG_START && startReg <= MODBUS_RW_REG_END)
            #endif
            {
                //在系统参数区
                modbus->extract_reg_data.pTr = &((unsigned char *)modbus->pRxBuffer)[4];//寄存器值
                modbus->extract_reg_data.nRegs = 1;
                modbus->extract_reg_data.offset = startReg - MODBUS_RW_REG_START;
                if (modbus->func_write_regs_check == NULL || modbus->func_write_regs_check(&modbus->extract_reg_data) == OK)
                {
                    /* code */
                    modbus->pTxBuffer[2] = modbus->pRxBuffer[2];
                    modbus->pTxBuffer[3] = modbus->pRxBuffer[3];
                    modbus->pTxBuffer[4] = modbus->pRxBuffer[4];
                    modbus->pTxBuffer[5] = modbus->pRxBuffer[5];
                    cal_crc_len = 6;
                    //参数保存
                    cflag |= CFLAG_IS_SAVE_REG;
                }
                else
                {
                    /* err data */
                    modbus->pTxBuffer[1] |= 0X80;
                    modbus->pTxBuffer[2] = MODBUS_ERR_CODE_DATA;
                    cal_crc_len = 3;
                }
            }
            else
            {
                /* err reg addr */
                modbus->pTxBuffer[1] |= 0X80;
                modbus->pTxBuffer[2] = MODBUS_ERR_CODE_REG;
                cal_crc_len = 3;
            }
        }
        break;
    case MODBUS_FUNC_CODE_W_MULTI:
        //得到写寄存器起始地址
        startReg = (((unsigned short)modbus->pRxBuffer[2]) << 8) + modbus->pRxBuffer[3];
        //得到写寄存器个数
        nReg = (((unsigned short)modbus->pRxBuffer[4]) << 8) + modbus->pRxBuffer[5];
        if(nReg > MODBUS_TRANS_REG_MAX)
        {
            return MODBUS_LEN_ERR;
        }
        //长度匹配
        if (modbus->pRxBuffer[6] != nReg * 2 || modbus->pRxBuffer[6] != modbus->RxLen- 9)
        {
            return MODBUS_LEN_ERR;
        }
#if MODBUS_RW_REG_START == 0u
        if (nReg != 0 && (modbus->pRxBuffer[6] == (nReg << 1)) && nReg <= MODBUS_RW_REG_TOTAL /*&& startReg >= MODBUS_RW_REG_START*/ && startReg <= MODBUS_RW_REG_END && (startReg + nReg) <= (MODBUS_RW_REG_END + 1))
#else
        if (nReg != 0 && (modbus->pRxBuffer[6] == (nReg << 1)) && nReg <= MODBUS_RW_REG_TOTAL && startReg >= MODBUS_RW_REG_START && startReg <= MODBUS_RW_REG_END && (startReg + nReg) <= (MODBUS_RW_REG_END + 1))
#endif
        { //在系统参数区
            modbus->extract_reg_data.pTr = &((unsigned char *)modbus->pRxBuffer)[7];
            modbus->extract_reg_data.nRegs = nReg;
            modbus->extract_reg_data.offset = startReg - MODBUS_RW_REG_START;
            if (modbus->func_write_regs_check == NULL || modbus->func_write_regs_check(&modbus->extract_reg_data) == OK)
            {
                /* code */
                modbus->pTxBuffer[2] = modbus->pRxBuffer[2];
                modbus->pTxBuffer[3] = modbus->pRxBuffer[3];
                modbus->pTxBuffer[4] = modbus->pRxBuffer[4];
                modbus->pTxBuffer[5] = modbus->pRxBuffer[5];
                cal_crc_len = 6;
                //参数保存
                cflag |= CFLAG_IS_SAVE_REG;
            }
            else
            {
                /* err data */
                modbus->pTxBuffer[1] |= 0X80;
                modbus->pTxBuffer[2] = MODBUS_ERR_CODE_DATA;
                cal_crc_len = 3;
            }
        }

        else
        {
            /* err reg addr */
            modbus->pTxBuffer[1] |= 0X80;
            modbus->pTxBuffer[2] = MODBUS_ERR_CODE_REG;
            cal_crc_len = 3;
        }
#if MODBUS_HOOK_FCODE_WT == 1u
        modbus_fcode_wt_hook();
#endif
        break;
#ifdef MODBUS_FUNC_CODE_DIY1
    case MODBUS_FUNC_CODE_DIY1:
        if(modbus_fcode_diy1_hook(modbus) == 0)
        {
            modbus->pTxBuffer[1] |= 0X80;
            modbus->pTxBuffer[2] = MODBUS_ERR_CODE_DATA;
            cal_crc_len = 3;
        }
        break;
#endif
    default:
        /* err  func code */
        modbus->pTxBuffer[1] |= 0X80;
        modbus->pTxBuffer[2] = MODBUS_ERR_CODE_FUNC;
        cal_crc_len = 3;
        break;
    }
    if(modbus->func_send_data != NULL 
        && modbus->pRxBuffer[0] != MODBUS_SLAVE_BROADCAST_ADDR 
        && (cflag & CFALG_IS_NEED_REPLY) != 0)
    {
        //计算CRC
        crc16 = modbus->func_crc16(modbus->pTxBuffer, cal_crc_len);
        modbus->pTxBuffer[cal_crc_len] = crc16;
        modbus->pTxBuffer[cal_crc_len + 1] = crc16 >> 8;
        modbus->func_send_data(modbus->pTxBuffer, cal_crc_len + 2);
    }
    if (modbus->func_save_regs != NULL && (cflag & CFLAG_IS_SAVE_REG) != 0)
    {
        modbus->func_save_regs(&modbus->extract_reg_data);
    }
    #undef CFLAG_IS_SAVE_REG
    #undef CFALG_IS_RD_PARAM
    #undef CFALG_IS_NEED_REPLY
    return MODBUS_OK;
}

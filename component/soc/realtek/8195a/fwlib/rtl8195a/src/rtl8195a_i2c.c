/*
 *  Routines to access hardware
 *
 *  Copyright (c) 2013 Realtek Semiconductor Corp.
 *
 *  This module is a confidential and proprietary property of RealTek and
 *  possession or use of this module requires written permission of RealTek.
 */

#include "rtl8195a.h"

/* Used only for A~C Version */
#ifndef CONFIG_CHIP_E_CUT
//---------------------------------------------------------------------------------------------------
//Function Name:
//		HalI2CSendRtl8195a
//
// Description:
//		Send one byte to the I2C internal fifo, it will generate START and STOP bit 
//         automatically.
//
// Arguments:
//		[in] VOID    *Data -
//			The I2C parameter data struct.
//
// Return:
//		_EXIT_SUCCESS if the sending succeeded.
//		_EXIT_FAILURE if the sending failed.
//
// Note:
//		None
//
// See Also:
//		NA
//
// Author:
// 		By Jason Deng, 2014-02-18.
//
//---------------------------------------------------------------------------------------------------
HAL_Status
HalI2CSendRtl8195a_Patch(
    IN  VOID    *Data
){
    PHAL_I2C_INIT_DAT pHalI2CInitData = (PHAL_I2C_INIT_DAT)Data;
    u8  I2CIdx  = pHalI2CInitData->I2CIdx;
    u8  *pDat   = pHalI2CInitData->I2CRWData;
    u8  I2CCmd  = pHalI2CInitData->I2CCmd;
    u8  I2CStop = pHalI2CInitData->I2CStop;
    u8  I2CReSTR= pHalI2CInitData->I2CReSTR;
 
    DBG_I2C_INFO("HalI2CSendRtl8195a\n");
    DBG_I2C_INFO("I2C Index: %x\n",I2CIdx);

    HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_DATA_CMD, 
                    *(pDat) |
                    BIT_CTRL_IC_DATA_CMD_RESTART(I2CReSTR)|
                    BIT_CTRL_IC_DATA_CMD_CMD(I2CCmd) |
                    BIT_CTRL_IC_DATA_CMD_STOP(I2CStop));
    
    return (HAL_OK);
}

//---------------------------------------------------------------------------------------------------
//Function Name:
//		HalI2CInit8195a
//
// Description:
//		To initialize I2C module by using the given data.
//
// Arguments:
//		[in] VOID    *Data -
//			The I2C parameter data struct.
//
// Return:
//		The status of the DeInit process.
//          _EXIT_SUCCESS if the initialization succeeded.
//          _EXIT_FAILURE if the initialization failed.
//
// Note:
//		None
//
// See Also:
//		NA
//
// Author:
// 		By Jason Deng, 2014-04-02.
//
//---------------------------------------------------------------------------------------------------
HAL_Status
HalI2CInit8195a_Patch(
    IN  VOID    *Data
)
{
    PHAL_I2C_INIT_DAT pHalI2CInitData = (PHAL_I2C_INIT_DAT)Data;

    u8  Master;
    u8  I2CIdx;
    u8  SpdMd;
    u8  AddrMd;
    u8  ReSTR;
    u8  StartByte;
    u8  Specical;
    u8  GC;
    u16 I2CAckAddr;
    u16 SdaHd;
    u8  SdaSetup;
    u8  RXTL;
    u8  TXTL;
    u8  SlvNoAck;
    u32 INTRMsk;
    u8  TxDMARqLv;
    u8  RxDMARqLv;
    
    /* Get the I2C parameters*/
    I2CIdx      = pHalI2CInitData->I2CIdx;
    SpdMd       = pHalI2CInitData->I2CSpdMod;
    AddrMd      = pHalI2CInitData->I2CAddrMod;
    I2CAckAddr  = pHalI2CInitData->I2CAckAddr;
    Master      = pHalI2CInitData->I2CMaster;
    SdaHd       = pHalI2CInitData->I2CSdaHd;
    SdaSetup    = pHalI2CInitData->I2CSetup;
    
    ReSTR       = pHalI2CInitData->I2CReSTR;
    GC          = pHalI2CInitData->I2CGC;
    StartByte   = pHalI2CInitData->I2CStartB;
    SlvNoAck    = pHalI2CInitData->I2CSlvNoAck;
    
    RXTL        = pHalI2CInitData->I2CRXTL;
    TXTL        = pHalI2CInitData->I2CTXTL;

    TxDMARqLv   = pHalI2CInitData->I2CTxDMARqLv;
    RxDMARqLv   = pHalI2CInitData->I2CRxDMARqLv;
    
    /* Disable the IC first */
    HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_ENABLE,BIT_CTRL_IC_ENABLE(0));

    /* Master case*/
    if (Master) {
        /*RESTART MUST be set in these condition in Master mode. 
               But it might be NOT compatible in old slaves.*/
        if ((AddrMd == I2C_ADDR_10BIT) || (SpdMd == I2C_HS_MODE))
            ReSTR = 1;
            
        HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_CON, 
                            (BIT_CTRL_IC_CON_IC_SLAVE_DISABLE(1) |
                             BIT_CTRL_IC_CON_IC_RESTART_EN(ReSTR) |
                             BIT_CTRL_IC_CON_IC_10BITADDR_MASTER(AddrMd) |
                             BIT_CTRL_IC_CON_SPEED(SpdMd) |                            
                             BIT_CTRL_IC_CON_MASTER_MODE(Master)));
    
        DBG_I2C_INFO("Init master, IC_CON%d[%2x]: %x\n", I2CIdx, REG_DW_I2C_IC_CON, HAL_I2C_READ32(I2CIdx,REG_DW_I2C_IC_CON));


        /* To set target addr.*/
        Specical = 0;
        if ((GC!=0) || (StartByte!=0))
            Specical = 1;
        HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_TAR,
                            (BIT_CTRL_IC_TAR_IC_10BITADDR_MASTER(AddrMd) |
                             BIT_CTRL_IC_TAR_SPECIAL(Specical)           |
                             BIT_CTRL_IC_TAR_GC_OR_START(StartByte)      |
                             BIT_CTRL_IC_TAR(I2CAckAddr)));

        /* To Set I2C clock*/
        HalI2CSetCLKRtl8195a_Patch(pHalI2CInitData);
        

        DBG_I2C_INFO("Init master, IC_TAR%d[%2x]: %x\n", I2CIdx, REG_DW_I2C_IC_TAR, HAL_I2C_READ32(I2CIdx,REG_DW_I2C_IC_TAR));

    }    /*if (Master)*/
    else {         
        HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_CON,
                            BIT_CTRL_IC_CON_IC_10BITADDR_SLAVE(AddrMd) |
                            BIT_CTRL_IC_CON_IC_SLAVE_DISABLE(Master) |
                            BIT_CTRL_IC_CON_SPEED(SpdMd)|
                            BIT_CTRL_IC_CON_MASTER_MODE(Master));

        DBG_I2C_INFO("Init slave, IC_CON%d[%2x]: %x\n", I2CIdx, REG_DW_I2C_IC_CON, HAL_I2C_READ32(I2CIdx,REG_DW_I2C_IC_CON));


        /* To set  slave addr. */
        HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_SAR,BIT_CTRL_IC_SAR(I2CAckAddr));

        DBG_I2C_INFO("Init slave, IC_SAR%d[%2x]: %x\n", I2CIdx, REG_DW_I2C_IC_SAR, HAL_I2C_READ32(I2CIdx,REG_DW_I2C_IC_SAR));


        /* To set slave no ack */
        HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_SLV_DATA_NACK_ONLY,BIT_CTRL_IC_SLV_DATA_NACK_ONLY(SlvNoAck));
        
        /* Set ack general call. */
        HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_ACK_GENERAL_CALL,BIT_CTRL_IC_ACK_GENERAL_CALL(pHalI2CInitData->I2CSlvAckGC));

        

        DBG_I2C_INFO("Init slave, I2C_IC_ACK_GC%d[%2x]: %x\n", I2CIdx, REG_DW_I2C_IC_ACK_GENERAL_CALL, HAL_I2C_READ32(I2CIdx,REG_DW_I2C_IC_ACK_GENERAL_CALL));

        /* to set SDA hold time */
        HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_SDA_HOLD,BIT_CTRL_IC_SDA_HOLD(SdaHd));
        //4 
        /* to set SDA setup time */
        HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_SDA_SETUP,BIT_CTRL_IC_SDA_SETUP(SdaSetup));
    }

    /* To set TX_Empty Level */
    HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_TX_TL,TXTL);
    
    /* To set RX_Full Level */
    HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_RX_TL,RXTL);

    /* To set TX/RX FIFO level */
    HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_DMA_TDLR,TxDMARqLv);
    HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_DMA_RDLR,RxDMARqLv);
    

    DBG_I2C_INFO("Init i2c dev, I2C_IC_DMA_TDLR%d[%2x]: %x\n", I2CIdx, REG_DW_I2C_IC_DMA_TDLR, HAL_I2C_READ32(I2CIdx,REG_DW_I2C_IC_DMA_TDLR));
    DBG_I2C_INFO("Init i2c dev, I2C_IC_DMA_RDLR%d[%2x]: %x\n", I2CIdx, REG_DW_I2C_IC_DMA_RDLR, HAL_I2C_READ32(I2CIdx,REG_DW_I2C_IC_DMA_RDLR));


    /*I2C Clear all interrupts first*/
    HalI2CClrAllIntrRtl8195a(pHalI2CInitData);

    /*I2C Disable all interrupts first*/
    INTRMsk = pHalI2CInitData->I2CIntrMSK;
    pHalI2CInitData->I2CIntrMSK = 0;
    HalI2CIntrCtrl8195a(pHalI2CInitData);
    pHalI2CInitData->I2CIntrMSK = INTRMsk;
    
    return HAL_OK;
}

//---------------------------------------------------------------------------------------------------
//Function Name:
//		HalI2CSetCLKRtl8195a
//
// Description:
//		To set I2C bus clock rate.
//
// Arguments:
//		[in] VOID    *Data -
//			The I2C parameter data struct.
//
// Return:
//		The status of the enable process.
//          _EXIT_SUCCESS if the de-initialization succeeded.
//          _EXIT_FAILURE if the de-initialization failed.
//
// Note:
//		None
//
// See Also:
//		NA
//
// Author:
// 		By Jason Deng, 2014-02-18.
//
//---------------------------------------------------------------------------------------------------
HAL_Status
HalI2CSetCLKRtl8195a_Patch(
    IN  VOID    *Data
)
{
    PHAL_I2C_INIT_DAT pHalI2CInitData = (PHAL_I2C_INIT_DAT)Data;
    u8  SpdMd = pHalI2CInitData->I2CSpdMod;
    u32 I2CClk = pHalI2CInitData->I2CClk;
    u8  I2CIdx = pHalI2CInitData->I2CIdx;
    u32 ICHLcnt;
    u32 ICHtime;
    u32 ICLtime;

    /* Get the IC-Clk setting first for the following process*/
#ifdef CONFIG_FPGA
    u32 IcClk = SYSTEM_CLK/1000000;
#else
    u32 IcClk;
    u32 ClkSELTmp = 0;
    u32 CpuClkTmp = 0;
    
    #if defined(CONFIG_CHIP_A_CUT)
        CpuClkTmp = StartupHalGetCpuClk();
    #elif  (defined(CONFIG_CHIP_B_CUT) || defined(CONFIG_CHIP_C_CUT))
        CpuClkTmp = HalGetCpuClk();
    #endif
    
    DBG_I2C_INFO("%s, CPU Clk:%x\n",__func__, CpuClkTmp);
    
    ClkSELTmp = HAL_READ32(PERI_ON_BASE, REG_PESOC_CLK_SEL);
    ClkSELTmp &= (~(BIT_PESOC_PERI_SCLK_SEL(3)));
    HAL_WRITE32(PERI_ON_BASE,REG_PESOC_CLK_SEL,ClkSELTmp);
    IcClk = (CpuClkTmp/1000000)>>1;

#if 0    
    if ((I2CClk > 0) && (I2CClk <= 400)) {
        ClkSELTmp &= (~(BIT_PESOC_PERI_SCLK_SEL(3)));
        HAL_WRITE32(PERI_ON_BASE,REG_PESOC_CLK_SEL,ClkSELTmp);
        IcClk = ClkSELTmp/1000000;     /*actually it's 12.5MHz*/
    }
    else {
        ClkSELTmp &= (~(BIT_PESOC_PERI_SCLK_SEL(3)));
        HAL_WRITE32(PERI_ON_BASE,REG_PESOC_CLK_SEL,ClkSELTmp);
        IcClk = 100;    
    }
#endif
#endif

    switch (SpdMd)  
    {
        case I2C_SS_MODE:
        {
            ICHtime = ((1000000/I2CClk)*I2C_SS_MIN_SCL_HTIME)/(I2C_SS_MIN_SCL_HTIME+I2C_SS_MIN_SCL_LTIME);
            ICLtime = ((1000000/I2CClk)*I2C_SS_MIN_SCL_LTIME)/(I2C_SS_MIN_SCL_HTIME+I2C_SS_MIN_SCL_LTIME);
            
            ICHLcnt = (ICHtime * IcClk)/1000;
            HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_SS_SCL_HCNT,ICHLcnt);
            

            DBG_I2C_INFO("IC_SS_SCL_HCNT%d[%2x]: %x\n", I2CIdx,
                        REG_DW_I2C_IC_SS_SCL_HCNT, HAL_I2C_READ32(I2CIdx,REG_DW_I2C_IC_SS_SCL_HCNT));


            ICHLcnt = (ICLtime * IcClk)/1000;
            HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_SS_SCL_LCNT,ICHLcnt);


            DBG_I2C_INFO("IC_SS_SCL_LCNT%d[%2x]: %x\n", I2CIdx,
                        REG_DW_I2C_IC_SS_SCL_LCNT, HAL_I2C_READ32(I2CIdx,REG_DW_I2C_IC_SS_SCL_LCNT));

            break;
        }
        
        case I2C_FS_MODE:
        {
            ICHtime = ((1000000/I2CClk)*I2C_FS_MIN_SCL_HTIME)/(I2C_FS_MIN_SCL_HTIME+I2C_FS_MIN_SCL_LTIME);
            ICLtime = ((1000000/I2CClk)*I2C_FS_MIN_SCL_LTIME)/(I2C_FS_MIN_SCL_HTIME+I2C_FS_MIN_SCL_LTIME);
            
            ICHLcnt = (ICHtime * IcClk)/1000;
            if (ICHLcnt>4)/*this part is according to the fine-tune result*/
                ICHLcnt -= 4;
            HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_FS_SCL_HCNT,ICHLcnt);
            

            DBG_I2C_INFO("IC_FS_SCL_HCNT%d[%2x]: %x\n", I2CIdx,
                        REG_DW_I2C_IC_FS_SCL_HCNT, HAL_I2C_READ32(I2CIdx,REG_DW_I2C_IC_FS_SCL_HCNT));

            
            ICHLcnt = (ICLtime * IcClk)/1000;
            if (ICHLcnt>3)/*this part is according to the fine-tune result*/
                ICHLcnt -= 3;
            HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_FS_SCL_LCNT,ICHLcnt);
            

            DBG_I2C_INFO("IC_FS_SCL_LCNT%d[%2x]: %x\n", I2CIdx,
                        REG_DW_I2C_IC_FS_SCL_LCNT, HAL_I2C_READ32(I2CIdx,REG_DW_I2C_IC_FS_SCL_LCNT));

            break;
        }

        case I2C_HS_MODE:
        {
            ICHLcnt = 400;
            HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_SS_SCL_HCNT,ICHLcnt);
            
            ICHLcnt = 470;
            HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_SS_SCL_LCNT,ICHLcnt);
            
            ICHLcnt = 60;
            HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_FS_SCL_HCNT,ICHLcnt);
            
            ICHLcnt = 130;
            HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_FS_SCL_LCNT,ICHLcnt);

            ICHtime = ((1000000/I2CClk)*I2C_HS_MIN_SCL_HTIME_100)/(I2C_HS_MIN_SCL_HTIME_100+I2C_HS_MIN_SCL_LTIME_100);
            ICLtime = ((1000000/I2CClk)*I2C_HS_MIN_SCL_LTIME_100)/(I2C_HS_MIN_SCL_HTIME_100+I2C_HS_MIN_SCL_LTIME_100);

            
            DBG_I2C_INFO("ICHtime:%x\n",ICHtime);
            DBG_I2C_INFO("ICLtime:%x\n",ICLtime);


            ICHLcnt = (ICHtime * IcClk)/1000;
            if (ICHLcnt>8)/*this part is according to the fine-tune result*/
                ICHLcnt -= 3;
            HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_HS_SCL_HCNT,ICHLcnt);
            
            
            DBG_I2C_INFO("IC_HS_SCL_HCNT%d[%2x]: %x\n", I2CIdx,
                        REG_DW_I2C_IC_HS_SCL_HCNT, HAL_I2C_READ32(I2CIdx,REG_DW_I2C_IC_HS_SCL_HCNT));


            ICHLcnt = (ICLtime * IcClk)/1000;
            if (ICHLcnt>6)/*this part is according to the fine-tune result*/
                ICHLcnt -= 6;
            HAL_I2C_WRITE32(I2CIdx,REG_DW_I2C_IC_HS_SCL_LCNT,ICHLcnt);
            

            DBG_I2C_INFO("IC_HS_SCL_LCNT%d[%2x]: %x\n", I2CIdx,
                        REG_DW_I2C_IC_HS_SCL_LCNT, HAL_I2C_READ32(I2CIdx,REG_DW_I2C_IC_HS_SCL_LCNT));


            break;
        }    

        default:
            break;
    }

    return HAL_OK;
}
#endif
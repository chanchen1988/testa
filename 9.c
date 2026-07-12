/*******************************************************************************************************
**--------------File Info-------------------------------------------------------------------------------
** File name:			NCWlan.c
** Version:				The original version
** Descriptions：	    NCWlan C entry file for  project
** Created by:			
** Created date:		2021-11-15
** Version:				1.0
** Descriptions:		WIFI
**

WIFI 版本
AT version:2.3.0.0-dev(s-bcd64d2 - ESP8266 - Jun 23 2021 11:42:05)
SDK version:v3.4-22-g967752e2
compile time(b498b58):Jul 31 2021 11:41:32
Bin version:2.2.0(WROOM-02)

**----------------------------------------------------------------------------------------------------*/
#include "NCWlan.h"  
#include <stdio.h>
#include <stdlib.h>
#include "NCUserSetting.h"
#include <string.h>
#include "FreeRTOS.h"
#include "task.h" 

/************************************************************/
static ENLoopArray       *gSendLoop          = NULL; 
static ENLoopArray       *gRecvLoop          = NULL;
static volatile BOOL    gWifiSending      = FALSE;
//static           BOOL    gWifiSignlSending = FALSE;

static volatile BOOL    gWlanIsConfigFinish = FALSE;//针对于模块本身 于配网无关的
static volatile BOOL    gWlanChange         = FALSE;////针对于模块本身 于配网无关的

//static           UINT16  UsartUnSendCount    = 0;
static           UINT16  Uart_DMA_LastCnt    = 0;
static volatile UINT8   WlanRevBuf[WLAN_COM_RX_BUF_SIZE];


#define     NC_BLE_GET_NO_ACK     0x00
#define     NC_BLE_GET_OPENED     0x01
#define     NC_BLE_GET_CLOSEED    0x02


/*********************************************************************************************************
** Function name(函数名称):				NCIPStrToNum()
**
** Descriptions（描述）:				IP字符串转数字
**
** input parameters（输入参数）:		None
** Returned value（返回值）:			None
**         
** Used global variables（全局变量）:	None
** Calling modules（调用模块）:			None
**
** Created by（创建人）:              SummerQT				
** Created Date（创建日期）:			 2021-12-14
********************************************************************************************************/
#if 0
void NCIPStrToNum(CHAR *ipStr, CHAR *ipNum)
{
    CHAR ipStrCpy[16];  
    CHAR *pNum;
    CHAR *pPoint;
    UINT8 i;

    strcpy(ipStrCpy, ipStr);

    pNum = ipStrCpy;
    for (i=0; i<3; i++)
    {
        pPoint = strchr(pNum, '.');
        if ((pPoint <= pNum) || (pPoint > (pNum+3)))
        {
            return;
        }
        *pPoint = 0;
        ipNum[i] = atoi(pNum);
        pNum = pPoint + 1;
    }
    ipNum[i] = atoi(pNum);  // 第4节数字不带'.'
}
#endif

//===============================================
void vTdelay_ms(UINT16 time)
{
    vTaskDelay(time);
}
//================================================
void NCWlanInitBufferConfig(ENLoopArray *sendLoop, ENLoopArray *recvLoop)
{
    gSendLoop = sendLoop;
    gRecvLoop = recvLoop;   
}
//================================================
BOOL NCWlanIsSending(void)
{
    return gWifiSending;
}

BOOL NCWlanConfigFinish(void)
{
    return gWlanIsConfigFinish;
}

BOOL NCIsWlanConfigChange(void)
{
    return gWlanChange;
}

void NCWlanConfigEnableChange(void)
{
     gWlanChange = TRUE;
}


static void WlanConfigReset(void)
{
    gWlanChange = FALSE;
    gWlanIsConfigFinish = FALSE;
}

static BOOL WlanConfigModify(void)
{
    if(gWlanChange == TRUE)
    {
        WlanConfigReset();
        return TRUE;
    }
    return FALSE;
}


//===========================================================================
//串口和相关GPIO初始化
/*
初始化串口 
使能接收中断 初始化接收中断
*/
//==========================================================================
//==========================================================================
//==========================================================================
static void NCWlan_USART_Init(UINT32 BaudRateValue)
{
    GPIO_Config_T  GPIO_InitStructure;
    USART_Config_T USART_InitStructure;
    /* Initialize GPIO_InitStructure */
    GPIO_ConfigStructInit(&GPIO_InitStructure);
    /* 使能串口时钟 */
    WIFI_USART_CLK_ENABLE();
    /* Configure USARTz Tx as alternate function push-pull */
    GPIO_InitStructure.pin = WIFI_USART_TX_PIN;
    GPIO_InitStructure.speed = GPIO_SPEED_50MHz;
    GPIO_InitStructure.mode = GPIO_MODE_AF_PP;
    GPIO_Config(WIFI_USART_TX_GPIO_PORT, &GPIO_InitStructure);

    /* Configure USARTz Rx as alternate function push-pull and pull-up */
    GPIO_InitStructure.pin = WIFI_USART_RX_PIN;
    GPIO_InitStructure.speed = GPIO_SPEED_50MHz;
    GPIO_InitStructure.mode = GPIO_MODE_IN_PU;//GPIO_MODE_IN_PU;
    GPIO_Config(WIFI_USART_RX_GPIO_PORT, &GPIO_InitStructure);	 
    //GPIO_SetBit(GPIOA, GPIO_PIN_3);
    
    /* 配置串USART 模式 */
    USART_Reset(WIFI_USART);
    USART_InitStructure.baudRate     = BaudRateValue;
    USART_InitStructure.wordLength   = USART_WORD_LEN_8B;
    USART_InitStructure.stopBits     = USART_STOP_BIT_1;
    USART_InitStructure.parity       = USART_PARITY_NONE;
    USART_InitStructure.hardwareFlow = USART_HARDWARE_FLOW_NONE;
    USART_InitStructure.mode         = USART_MODE_TX_RX;//USART_MODE_TX | USART_MODE_RX;
    USART_Config(WIFI_USART, &USART_InitStructure);
    
     /*配置串口接收中断 */
    //USART_EnableInterrupt(RS485_USART, USART_INT_RXBNE);
    USART_EnableInterrupt(WIFI_USART, USART_INT_IDLE);

    /* Enable USARTy DMA Rx request */
    USART_EnableDMA(WIFI_USART, USART_DMA_RX);//使能DMA接收
    USART_EnableDMA(WIFI_USART, USART_DMA_TX);//使能DMA发送
    WIFI_USART->STS = 0;//清标志位
    USART_ClearStatusFlag(WIFI_USART, USART_FLAG_TXC);  // 清发送完成标志位
    NVIC_EnableIRQRequest(WIFI_USART_IRQ, WIFI_USART_PRIORITY, 0);

    /* Enable the USARTy */
    USART_Enable(WIFI_USART);
}

//==================================================================
//==================================================================
static void  NCWlanOtherGPIOInit(void)
{
    //NLINK
    GPIO_Config_T  GPIO_InitStructure;
    WIFI_NLINK_PIN_HIGH;
    GPIO_InitStructure.pin   = WIFI_NLINK_PIN;
    GPIO_InitStructure.speed = GPIO_SPEED_50MHz;
    GPIO_InitStructure.mode  = GPIO_MODE_IN_PU;
    GPIO_Config(WIFI_NLINK_PORT, &GPIO_InitStructure);	
    //NREADY    
    WIFI_NREADY_PIN_HIGH;
    GPIO_InitStructure.pin = WIFI_NREADY_PIN;
    GPIO_Config(WIFI_NREADY_PORT, &GPIO_InitStructure);
    
    
    //NRST
    GPIO_InitStructure.pin   = WIFI_NRST_PIN;
    GPIO_InitStructure.speed = GPIO_SPEED_50MHz;
    GPIO_InitStructure.mode  = GPIO_MODE_OUT_PP;
    GPIO_Config(WIFI_NRST_PORT, &GPIO_InitStructure);	 
    //WIFI_NRST_PIN_LOW;
    WIFI_NRST_PIN_HIGH; 
    
    //WIFI POWER
    GPIO_InitStructure.pin   = WIFI_POWER_PIN;
    GPIO_InitStructure.speed = GPIO_SPEED_50MHz;
    GPIO_InitStructure.mode  = GPIO_MODE_OUT_PP;
    GPIO_Config(WIFI_POWER_PORT, &GPIO_InitStructure);	 
    WIFI_POWER_PIN_DISABLE; 
    //WIFI_POWER_PIN_ENABLE

                


} 
//==================================================================
static void NCWlan_UARTTx_DMA_Config(UINT32 cmar,UINT16 cndtr)
{
    /* DMA clock enable */
    RCM_EnableAHBPeriphClock(RCM_AHB_PERIPH_DMA1);
    /* enable DMA1 channel6 */
    DMA_Config_T   DMA_InitStructure;
    DMA_Reset(WIFI_USART_TX_DMA_CHANNEL);
    DMA_ConfigStructInit(&DMA_InitStructure);
    DMA_InitStructure.peripheralBaseAddr    = WIFI_USART_DAT_Base;
    DMA_InitStructure.memoryBaseAddr        = (uint32_t)cmar;
    DMA_InitStructure.dir                   = DMA_DIR_PERIPHERAL_DST;//从寄存器到外设
    DMA_InitStructure.bufferSize            = cndtr;
    DMA_InitStructure.peripheralInc         = DMA_PERIPHERAL_INC_DISABLE;
    DMA_InitStructure.memoryInc             = DMA_MEMORY_INC_ENABLE;
    DMA_InitStructure.peripheralDataSize    = DMA_PERIPHERAL_DATA_SIZE_BYTE;
    DMA_InitStructure.memoryDataSize        = DMA_MEMORY_DATA_SIZE_BYTE;
    DMA_InitStructure.loopMode              = DMA_MODE_NORMAL;
    DMA_InitStructure.priority              = DMA_PRIORITY_HIGH;
    DMA_InitStructure.M2M                   = DMA_M2MEN_DISABLE;
    DMA_Config(WIFI_USART_TX_DMA_CHANNEL, &DMA_InitStructure);
    /*配置DMA接收中断 */ 
    DMA_EnableInterrupt(WIFI_USART_TX_DMA_CHANNEL,DMA_INT_TC);//开启DMA完成中断使能
    NVIC_EnableIRQRequest(WIFI_USART_DMA_IRQ, WIFI_USART_PRIORITY, 0); 
}  


//==========================================================================

/**
  * @brief RS485_UARTRx_DMA_Config
  * @param None
  * @retval None
  * @note   处理完数据后再次启动
  */
//====================================================
//====================================================
static void NCWlan_UARTRx_DMA_Config(void)
{
    DMA_Config_T   DMA_InitStructure;
    /* DMA clock enable */
    RCM_EnableAHBPeriphClock(RCM_AHB_PERIPH_DMA1);
    
    DMA_Reset(WIFI_USART_RX_DMA_CHANNEL);
    DMA_ConfigStructInit(&DMA_InitStructure);
    DMA_InitStructure.peripheralBaseAddr    = WIFI_USART_DAT_Base;
    DMA_InitStructure.memoryBaseAddr        = (uint32_t)WlanRevBuf;
    DMA_InitStructure.dir                   = DMA_DIR_PERIPHERAL_SRC;//从外设到寄存器
    DMA_InitStructure.bufferSize            = WLAN_COM_RX_BUF_SIZE;
    DMA_InitStructure.peripheralInc         = DMA_PERIPHERAL_INC_DISABLE;
    DMA_InitStructure.memoryInc             = DMA_MEMORY_INC_ENABLE;
    DMA_InitStructure.peripheralDataSize    = DMA_PERIPHERAL_DATA_SIZE_BYTE;
    DMA_InitStructure.memoryDataSize        = DMA_MEMORY_DATA_SIZE_BYTE;
    DMA_InitStructure.loopMode              = DMA_MODE_CIRCULAR;//DMA_MODE_NORMAL;//DMA_MODE_CIRCULAR;//循环
    DMA_InitStructure.priority              = DMA_PRIORITY_VERYHIGH;
    DMA_InitStructure.M2M                   = DMA_M2MEN_DISABLE;
    DMA_Config(WIFI_USART_RX_DMA_CHANNEL, &DMA_InitStructure);  
}

//====================================================================================
void NCWlan_RevDMA_Restart(void)
{
    UINT16 Uart_DMA_Data;
	/* Disable the DMA */
    WIFI_USART_RX_DMA_CHANNEL->CHCFG_B.CHEN = DISABLE;//先关闭才能设置 设置成功后再开启 
    USART_DisableInterrupt(WIFI_USART, USART_INT_IDLE); 
    ENLoopArrayClear(gRecvLoop);
    Uart_DMA_Data = WIFI_USART->STS;
    if(Uart_DMA_Data!=0)
    {
       Uart_DMA_Data = WIFI_USART->DATA;
    }
    Uart_DMA_LastCnt = 0;
    WIFI_USART_RX_DMA_CHANNEL->CHNDATA = WLAN_COM_RX_BUF_SIZE;
    
    WIFI_USART_RX_DMA_CHANNEL->CHPADDR = ((uint32_t)&WIFI_USART->DATA);
    WIFI_USART_RX_DMA_CHANNEL->CHMADDR = (uint32_t)WlanRevBuf;
    

    WIFI_USART_TX_DMA->INTFCLR = WIFI_USART_RX_DMA_GLBF | WIFI_USART_RX_DMA_TXCF |WIFI_USART_RX_DMA_HTXF| WIFI_USART_RX_DMA_ERRF;

    WIFI_USART_RX_DMA_CHANNEL->CHCFG_B.CHEN = ENABLE;
    USART_EnableDMA(WIFI_USART, USART_DMA_RX);//使能DMA接收
    USART_EnableInterrupt(WIFI_USART, USART_INT_IDLE);
}


//=====================================================================
void NCWlan_RevDMA_Disable(void)
{
	/* Disable the DMA */
    WIFI_USART_RX_DMA_CHANNEL->CHCFG_B.CHEN = DISABLE;
    //Uart_DMA_LastCnt = 0;
    USART_DisableInterrupt(WIFI_USART, USART_INT_IDLE);//RS485_USART
    ENLoopArrayClear(gRecvLoop);  
}

//=================================================
void NCWlan_TxDMA_Disable(void)
{
	/* Disable the DMA */
    WIFI_USART_TX_DMA_CHANNEL->CHCFG_B.CHEN = DISABLE;
    ENLoopArrayClear(gSendLoop);
    gWifiSending = FALSE;
}
//================================================
void NCWlan_DMA_UsartTxTransmit(UINT32 cmar, UINT16 cndtr)
{   
	/* clear */
    WIFI_USART_TX_DMA_CHANNEL->CHCFG_B.CHEN = DISABLE;//先关闭才能设置 设置成功后再开启 
    /* clear DMA Flag */
    WIFI_USART_TX_DMA->INTFCLR = WIFI_USART_TX_DMA_GLBF | WIFI_USART_TX_DMA_TXCF |WIFI_USART_TX_DMA_HTXF| WIFI_USART_TX_DMA_ERRF;
    
    WIFI_USART_TX_DMA_CHANNEL->CHNDATA = cndtr;
    /* Configure DMA Stream destination address */
    WIFI_USART_TX_DMA_CHANNEL->CHPADDR = ((uint32_t)&WIFI_USART->DATA);
    /* Configure DMA Stream source address */
    WIFI_USART_TX_DMA_CHANNEL->CHMADDR = cmar;

    WIFI_USART_TX_DMA_CHANNEL->CHCFG_B.CHEN = ENABLE;
}

//====================================================
/*
函数名称：USART1_DMA_Send(u32 cmar,u16 cndtr)
函数参数：cmar------传输数据内存地址
          cndtr-----传输数据量
函数说明：USART1以DMA的方式发送数据
*/
void NCWlan_DMA_Send(UINT8* cmar,UINT16 cndtr)
{
    if (cndtr == 0)  return ;
    NCWlan_DMA_UsartTxTransmit((UINT32)cmar,cndtr);
} 

/*********************************************************************************************************
** Function name(函数名称):	        Usart2SendStringBuffer()
**
** Descriptions（描述）:				USART1_DMA_Tx发送字符串
**
** input parameters（输入参数）:		   buffer:发送字符串的寄存器
** Returned value（返回值）:			   None
**         
** Used global variables（全局变量）:	None
** Calling modules（调用模块）:			None
**
** Created by（创建人）:				
** Created Date（创建日期）:			2021-11-16
********************************************************************************************************/
void NCWlanSendStringBuffer(const CHAR *buffer)
{
    UINT8 length=0,index;

    length = strlen(buffer);
    
    if ((gSendLoop) && (length))
    {
        for(index=0;index<length;index++)
          ENLoopArrayIn(gSendLoop, buffer[index]);      
        NCWlan_StartSend();          
    }
}

//=======================================================================================================
void NCWlan_StartSend(void)   
{    
    vTaskDelay(10);//延时发送 RS485
    if((gSendLoop) && (gSendLoop->number) && (gWifiSending == FALSE))
    {    
        while(RESET == ((WIFI_USART->STS) & USART_FLAG_TXBE))  
        {
           vTaskDelay(1); // vTdelay_ms(1); 等最后一个字节发完
        }
        gWifiSending = TRUE;
        NCWlan_DMA_Send(&(gSendLoop->buffer[gSendLoop->head]),gSendLoop->number);
        ENLoopArrayClear(gSendLoop);
    }
}

//===========================================================================
void NCWlan_TxBuffer_Deinit(void)
{
   ENLoopArrayClear(gSendLoop);
}


void NCWlan_RxBuffer_Deinit(void)
{
   ENLoopArrayClear(gRecvLoop);
}

//=======================================================================
ENLoopArray * getNCWlan_RecvLoopPtr(void)
{
    return gRecvLoop;  
}

//=======================================================================================================
void NCWlan_Init(void)
{    
    /*串口1中断初始化 */
    NCWlanOtherGPIOInit();
    NCWlan_USART_Init(115200);//115200
    NCWlan_UARTTx_DMA_Config((UINT32)&(gSendLoop->buffer[gSendLoop->head]),gSendLoop->number);
    NCWlan_UARTRx_DMA_Config();//接收
    NCWlan_RevDMA_Restart();
}

//==================================================================================
#define     WIFI_RECV_MAX               128
static CHAR gWifiUartAck[WIFI_RECV_MAX];

//需要在关闭回显的情况下

static CHAR *WifiParseRsp(void)
{
    UINT16 i = 0;
    memset(gWifiUartAck, 0, sizeof(gWifiUartAck));
    if (gRecvLoop)
    {
        // 等待WIFI 模块答复-- 该模式只适应关闭回显情况下
        for(i=0; i<1000; i++)   // 最多等待1秒
        {
            vTdelay_ms(1);
            if (ENLoopArrayEmpty(gRecvLoop) == FALSE)
            {
                break;
            }    
        }
        vTdelay_ms(10);   // 再等10秒-- 把答复收完
        
        i = 0;
        while(ENLoopArrayEmpty(gRecvLoop) == FALSE)
        {            
            gWifiUartAck[i++] = ENLoopArrayOut(gRecvLoop);
            if(i >= WIFI_RECV_MAX)
            {
                NCWlan_RevDMA_Restart();//ENLoopArrayClear(gRecvLoop);  //多余的丢弃
                break;
            }            
        }
    }
    return gWifiUartAck;
}


//========================================================================
//复位WIFI模块
//WIFI_POWER_PIN_DISABLE; 
//WIFI_POWER_PIN_ENABLE;


void NCWlanHardReset(void)
{
    WIFI_POWER_PIN_DISABLE;
    WIFI_NRST_PIN_LOW;
    vTdelay_ms(80); 
    WIFI_POWER_PIN_ENABLE;    
    WIFI_NRST_PIN_HIGH;
    vTdelay_ms(3000);   
}

//=============================================
static UINT32 timeoutCounts = 0;
// 设置超时次数
static void WlanConfigTimeout(UINT32 timeout)
{
    timeoutCounts = timeout;
}
static BOOL IsWlanConfigTimeout(void)
{
    if( timeoutCounts == 0)
        return TRUE;
    else
        timeoutCounts--;
    return FALSE;
}

#define WLAN_CONFIG_BEGIN()       WlanConfigTimeout(30)

#define WLAN_CONFIG_RETURN()        \
{                   \
    if(IsWlanConfigTimeout() == TRUE || WlanConfigModify() == TRUE) \
    {               \
        NCWlanHardReset();/*WifiReset();*/\
        return ;    \
    }               \
}

//定义：strstr(str1,str2) 函数用于判断字符串str2是否是str1的子串
//如果是，则该函数返回str2在str1中首次出现的地址；否则，返回NULL。

// 打开、关闭回显
BOOL WifiEchoOnOff(BOOL isOn)
{
    if (isOn)
    {
        NCWlanSendStringBuffer("HFAT+E=on\r\n");//\r\n  只需要回车即可
    }
    else
    {
        NCWlanSendStringBuffer("HFAT+E=off\r\n");
    }
    vTdelay_ms(300);      // 复位后默认开回显,关闭回显时,会先收到"回显命令"，等待一会后再收到"+ok"
    if(strstr(WifiParseRsp(), "+ok"))
    {
        NCWlan_RevDMA_Restart();
        return TRUE;
    }
    else
    {
        NCWlan_RevDMA_Restart();
        return FALSE;
    }
}

//================================================================
//模块重启 需要等待2S
BOOL  WlanModeRestart(void)
{
    NCWlanSendStringBuffer("HFAT+Z\r\n");
    vTdelay_ms(150);
    if(strstr(WifiParseRsp(), "+ok"))
    {
        NCWlan_RevDMA_Restart();
        return TRUE;
    }
    else
    {
        NCWlan_RevDMA_Restart();
        return FALSE;
    }
}

//=================================================================
//进入AT 指令模式
BOOL WifiEntryAtMode(void)
{
    BOOL ack = FALSE;
    UINT8 tmp;
    static UINT8 errorCount;
    
    if(errorCount > 10)         // 防止死机
	{
		errorCount = 0;
        return FALSE;
	}
    
    NCWlanSendStringBuffer("+++");
    errorCount++;
    vTdelay_ms(150);
    if(gRecvLoop)
    {
        while(ENLoopArrayEmpty(gRecvLoop) == FALSE)
        {
            tmp = ENLoopArrayOut(gRecvLoop);
            if(tmp == 'a')
            {
                ack = TRUE;
                NCWlanSendStringBuffer("a");
                vTdelay_ms(150);
                break;
            }            
        }

        if(ack)
        {
            if(strstr(WifiParseRsp(), "+ok"))
            {
                errorCount = 0;
                ack = TRUE;
            }
            else
            {
                ack = FALSE;
            }   
            ENLoopArrayClear(gRecvLoop);            
        }
        else
        {
            ack = FALSE;
        }        
    }    
    
    return ack;
}

//========================================================================
// 模块进入透传模式
BOOL WifiEntryTransMode(void)
{
    BOOL ack=FALSE;
    
    NCWlanSendStringBuffer("HFAT+ENTM\r\n");
    vTdelay_ms(150);
    if(strstr(WifiParseRsp(), "+ok"))
    {
        NCWlan_RevDMA_Restart();
        ack = TRUE;
    }
    else
    {
        NCWlan_RevDMA_Restart();
        ack = FALSE;
    }

    return ack;
}

//===========================================================================
// STA Mode
static BOOL WifiSetSTAMode(void)
{
    NCWlanSendStringBuffer("HFAT+WMODE=STA\r\n");
    vTdelay_ms(150);
    if(strstr(WifiParseRsp(), "+ok"))
    {
        NCWlan_RevDMA_Restart();
        return TRUE;
    }
    else
    {
        NCWlan_RevDMA_Restart();
        return FALSE;
    }
}


static BOOL WifiGetSTAMode(void)
{
    NCWlanSendStringBuffer("HFAT+WMODE\r\n");
    vTdelay_ms(150);
    if(strstr(WifiParseRsp(), "+ok=STA"))
    {
        NCWlan_RevDMA_Restart();
        return TRUE;
    }
    else
    {
        NCWlan_RevDMA_Restart();
        return FALSE;
    }
}

//===========================================================================
// BLE 广播参数  
//格式：AAAABBBBCD，默认值 0768128007   480  800
//格式：AAAABBBBCD，默认值 0256041607   160  260
static BOOL WifiSetBLEADP(void)
{
    NCWlanSendStringBuffer("HFAT+BLEADP=0256041607\r\n");
    vTdelay_ms(150);
    if(strstr(WifiParseRsp(), "+ok"))
    {
        NCWlan_RevDMA_Restart();
        return TRUE;
    }
    else
    {
        NCWlan_RevDMA_Restart();
        return FALSE;
    }
}


static BOOL WifiGetBLEADP(void)
{
    NCWlanSendStringBuffer("HFAT+BLEADP\r\n");
    vTdelay_ms(150);
    if(strstr(WifiParseRsp(), "+ok=0256041607"))
    {
        NCWlan_RevDMA_Restart();
        return TRUE;
    }
    else
    {
        NCWlan_RevDMA_Restart();
        return FALSE;
    }
}


//=================================================
//==========================================================================
//设置需要连接的热点的密码
static BOOL WifiSetPassWord(CHAR *psw)
{
    CHAR str[42] = {0};
    sprintf(str, "HFAT+WSKEY=WPA2PSK,AES,%s\r\n", psw);
    NCWlanSendStringBuffer(str);
    vTdelay_ms(100);
    if(strstr(WifiParseRsp(), "+ok"))
    {
        NCWlan_RevDMA_Restart();
        return TRUE;
    }
    else
    {
        NCWlan_RevDMA_Restart();
        return FALSE;
    }
}
//=============================================================================
// 设置需要连接的AP的SSID
static BOOL WifiSetSSID(CHAR *name)
{
    CHAR str[42] = {0};
    sprintf(str, "HFAT+WSSSID=%s\r\n", name);
    NCWlanSendStringBuffer(str);
    vTdelay_ms(100);
    if(strstr(WifiParseRsp(), "+ok"))
    {
        NCWlan_RevDMA_Restart();
        return TRUE;
    }
    else
    {
        NCWlan_RevDMA_Restart();
        return FALSE;
    }
}

//
//=============================================================================
// 设置需要连接的BLE 名字 (最大26字节)
static BOOL WifiSetBleName(CHAR *name)
{
    CHAR str[42] = {0};
    sprintf(str, "HFAT+BLENAME=WEB240,%s\r\n", name);
    NCWlanSendStringBuffer(str);
    vTdelay_ms(100);
    if(strstr(WifiParseRsp(), "+ok"))
    {
        NCWlan_RevDMA_Restart();
        return TRUE;
    }
    else
    {
        NCWlan_RevDMA_Restart();
        return FALSE;
    }
}


static BOOL WifiGetBleName(void)
{
    CHAR str[13] = {0};
    NCUserSetting* userSetting = NCUserSettingGet();
    memcpy(str,userSetting->deviceSN,12); //12位数SN
    NCWlanSendStringBuffer("HFAT+BLENAME\r\n");
    vTdelay_ms(100);
    if(strstr(WifiParseRsp(), str))
    {
        NCWlan_RevDMA_Restart();
        return TRUE;
    }
    else
    {
        NCWlan_RevDMA_Restart();
        return FALSE;
    }
}




//=============================================================================
// 设置DHCP   复位后设置生效!!!
static BOOL WifiSetDHCP(void)
{
    NCWlanSendStringBuffer("HFAT+WANN=DHCP\r\n");
    vTdelay_ms(150);
    if(strstr(WifiParseRsp(), "+ok"))
    {
        NCWlan_RevDMA_Restart();
        return TRUE;
    }
    else
    {
        NCWlan_RevDMA_Restart();
        return FALSE;
    }
}


static BOOL WifiGetDHCP(void)
{
    NCWlanSendStringBuffer("HFAT+WANN\r\n");
    vTdelay_ms(150);
    if(strstr(WifiParseRsp(), "+ok=DHCP"))
    {
        NCWlan_RevDMA_Restart();
        return TRUE;
    }
    else
    {
        NCWlan_RevDMA_Restart();
        return FALSE;
    }
}

//==============================================================================
//创建UDP 服务
static BOOL WifiSetUdpClient(CHAR *server_ip,UINT16 port)
{
    CHAR str[64] = {0};
    
    sprintf(str, "HFAT+NETP=UDP,CLIENT,%d,%s\r\n", port,server_ip);
    NCWlanSendStringBuffer(str);
    vTdelay_ms(100);
    if(strstr(WifiParseRsp(), "+ok"))
    {
        NCWlan_RevDMA_Restart();
        return TRUE;
    }
    else
    {
        NCWlan_RevDMA_Restart();
        return FALSE;
    }
}

//=============================================================================
//=============================================================================
// 设置wifi 功能使能
BOOL WifiModeFunConfig(BOOL enable)
{
    if(enable == ENABLE)
    {
       NCWlanSendStringBuffer("HFAT+WIFI=UP\r\n"); 
    }
    else
    {
       NCWlanSendStringBuffer("HFAT+WIFI=DOWN\r\n");
    }
    vTdelay_ms(100);
    if(strstr(WifiParseRsp(), "+ok"))
    {
        NCWlan_RevDMA_Restart();
        return TRUE;
    }
    else
    {
        NCWlan_RevDMA_Restart();
        return FALSE;
    }
}

//=============================================================================
// 设置BLE 功能使能
static BOOL BleModeFunConfig(BOOL enable)
{
    if(enable == ENABLE)
    {
       NCWlanSendStringBuffer("HFAT+BLE=on\r"); //("HFAT+BLE=on,0\r\n")
    }
    else
    {
       NCWlanSendStringBuffer("HFAT+BLE=off\r");//默认关闭 "HFAT+BLE=off,0\r\n"
    }
    vTdelay_ms(150);
    if(strstr(WifiParseRsp(), "+ok"))
    {
        NCWlan_RevDMA_Restart();
        return TRUE;
    }
    else
    {
        NCWlan_RevDMA_Restart();
        return FALSE;
    }
}




BOOL BleModeFunConfig11(BOOL enable)
{
    if(enable == ENABLE)
    {
       NCWlanSendStringBuffer("HFAT+BLE=on,0\r\n"); //
    }
    else
    {
       NCWlanSendStringBuffer("HFAT+BLE=off,1\r\n");//默认关闭
    }
    vTdelay_ms(150);
    if(strstr(WifiParseRsp(), "+ok"))
    {
        NCWlan_RevDMA_Restart();
        return TRUE;
    }
    else
    {
        NCWlan_RevDMA_Restart();
        return FALSE;
    }
}
//==========================================================================
//return: 0x00  没有响应  NC_BLE_GET_NO_ACK     0x00
//        0x01  已经开启  NC_BLE_GET_OPENED     0x01
//        0x02  已经关闭  NC_BLE_GET_CLOSEED    0x02
UINT8 GetBleModeFunEnable(void)
{
    NCWlanSendStringBuffer("HFAT+BLE\r\n"); 

    vTdelay_ms(200);
    WifiParseRsp();
    //printf("%s",gWifiUartAck);
    if(strstr(gWifiUartAck, "+ok=on"))
    {
        NCWlan_RevDMA_Restart();
        return NC_BLE_GET_OPENED;//已经开启
    }
    else if(strstr(gWifiUartAck, "+ok=off"))  
    {
        NCWlan_RevDMA_Restart();
        return  NC_BLE_GET_CLOSEED;//已经关闭
    }
    return  NC_BLE_GET_NO_ACK;//没有响应  
    
}


//=========================================================================
//获取WIFI 连接状态
BOOL WifiGetLinkState(void)
{
    NCWlanSendStringBuffer("HFAT+WSLK\r\n");
    vTdelay_ms(150);
    if(strstr(WifiParseRsp(), "+ok=Disconnected"))
    {
        NCWlan_RevDMA_Restart();//ENLoopArrayClear(gRecvLoop);
        return FALSE;
    }
    else
    {
        if(strstr(gWifiUartAck,"+ok="))
        {
            NCWlan_RevDMA_Restart();//ENLoopArrayClear(gRecvLoop);
            return TRUE;
        }
        else
        {
            NCWlan_RevDMA_Restart();//ENLoopArrayClear(gRecvLoop);
            return FALSE;
        }
        
    }
}



//=============================================================================================
//=============================================================================================
//上电和普通配置 ：
//1.先复位 蓝牙名字是否等于SN     
//2.wifi 是不是DHCP
//3.是不是STA 模式
//有配置的话 软件复位（是否关闭蓝牙待考虑）
//使用 HFAT+COM  不会有回显
//=============================================================================================
//上电或者有SN更新配置
/*
TW100

上电或者复位如果蓝牙是关闭的发送HFAT蓝牙关闭指令 会导致串口接收不到任何数据。（需要硬件复位或者上电才能）
但是如果使用非透传下的AT是没有问题的

但是如果是开启的情况下 发送 开启或者关闭不限制次数都是由回复的。

*/
void NCWanPowerOnConfigChange(void)  
{
    BOOL  IsConfigParma = FALSE;
    UINT8 BLE_Flag;
    NCUserSetting* userSetting = NCUserSettingGet();
    CHAR String[64];
    ENLoopArrayClear(gSendLoop);
    userSetting->AP_NetInfo.IsNetConfigComplete  = FALSE;
    userSetting->AP_NetInfo.IsNetConfigEnable    = FALSE;
    userSetting->AP_NetInfo.IsNetConfigComplete  = FALSE;
    userSetting->AP_NetInfo.IsNetConnect         = FALSE;
    userSetting->AP_NetInfo.IsNetCommunicating   = FALSE;  
    
    
    WlanConfigReset();//软件标记复位    
    NCWlanHardReset();//硬件复位
    NCWlan_RevDMA_Restart();

    //BleModeFunConfig(DISABLE);///不用开启也可以改变蓝牙名字 只是改变后需要复位
    
    //1.先复位 蓝牙名字是否等于SN 
    if(FALSE==WifiGetBleName())
    {
        #ifdef  IS_LOG_EN
        //printf("WifiSetBleName");
        #endif
         memset(String,0,sizeof(String));
         memcpy(String,userSetting->deviceSN,12); //12位数SN
        
         WLAN_CONFIG_BEGIN();
         while(FALSE == WifiSetBleName(String)) //配置蓝牙nanme
         {
            WLAN_CONFIG_RETURN();
         }
         IsConfigParma  = TRUE;        
    }
    //2.wifi 是不是DHCP
    if(FALSE == WifiGetDHCP())
    {
        #ifdef  IS_LOG_EN
        //printf("WifiSetDHCP");
        #endif
        WLAN_CONFIG_BEGIN();
         while(FALSE == WifiSetDHCP()) //配置DHCP
         {
            WLAN_CONFIG_RETURN();
         }
       IsConfigParma  = TRUE;  
    }
    //3.是不是STA 模式
    if(FALSE == WifiGetSTAMode())
    {
        #ifdef  IS_LOG_EN
        //printf("WifiSetSTAMode");
        #endif        
        WLAN_CONFIG_BEGIN();
         while(FALSE == WifiSetSTAMode()) //配置DHCP
         {
            WLAN_CONFIG_RETURN();
         }
       IsConfigParma  = TRUE;  
    }  
    //3.是不是STA 模式
    if(FALSE == WifiGetBLEADP())
    {
        #ifdef  IS_LOG_EN
        //printf("WifiSetSTAMode");
        #endif        
        WLAN_CONFIG_BEGIN();
         while(FALSE == WifiSetBLEADP()) //配置DHCP
         {
            WLAN_CONFIG_RETURN();
         }
       IsConfigParma  = TRUE;  
    }     


    
    //4.开启/关闭蓝牙
    #if     1//2023-11-28 关闭  上电BLE关闭 需要双击开机键打开 配网后自动关闭
//    if(NC_BLE_GET_OPENED == GetBleModeFunEnable())
//    {
//        #ifdef  IS_LOG_EN
//        printf("BleModeFunConfig");
//        #endif        
//        WLAN_CONFIG_BEGIN();
//         while(FALSE == BleModeFunConfig11(DISABLE)) //配置 11
//         {
//            WLAN_CONFIG_RETURN();
//         }
//         userSetting->AP_NetInfo.IsBluetoothEnable    = FALSE;
//       //IsConfigParma  = TRUE;  
//    } 
    #else
    if(NC_BLE_GET_CLOSEED == GetBleModeFunEnable())
    {
        #ifdef  IS_LOG_EN
        //printf("BleModeFunConfig");
        #endif        
        WLAN_CONFIG_BEGIN();
         while(FALSE == BleModeFunConfig(ENABLE)) //配置 
         {
            WLAN_CONFIG_RETURN();
         }
         userSetting->AP_NetInfo.IsBluetoothEnable    = FALSE;
       //IsConfigParma  = TRUE;  
    }     
    
    #endif    
    //复位进程
    if(TRUE==IsConfigParma)
    {
    
       NCWlanHardReset();//硬件复位
        //WlanModeRestart();
       NCWlan_RevDMA_Restart();
        //vTdelay_ms(2500);
       #ifdef  IS_LOG_EN
       //printf("WIFI Setting Compelel");
       #endif
       WifiGetBleName();
    }
    
    UINT8  Timecnt= 0;
    userSetting->AP_NetInfo.IsNetConnect = TRUE;
    while(FALSE== WifiGetLinkState()) 
    {
        vTaskDelay(300); 
        if(++Timecnt>4)      
        {
            userSetting->AP_NetInfo.IsNetConnect = FALSE;
            break;       
        }            
    
    }

    
    #ifdef  IS_LOG_EN
   // printf("BleModeFunConfig Dis");
    #endif
    
    //先检测是否BLE是开启的 如果不是开启的 关闭指令会有问题
    BLE_Flag =  GetBleModeFunEnable();
    if(BLE_Flag == NC_BLE_GET_NO_ACK)
    {
       BLE_Flag =  GetBleModeFunEnable();
       if(BLE_Flag == NC_BLE_GET_NO_ACK)
       {
          BLE_Flag =  GetBleModeFunEnable();
       }   
    }
    
    //vTaskDelay(150); //先配网等待120ms 关闭BLE   
    if(NC_BLE_GET_OPENED==BLE_Flag)
    {
        WLAN_CONFIG_BEGIN();
        while(FALSE == BleModeFunConfig(DISABLE)) //配置 
        {
           WLAN_CONFIG_RETURN();
        }    
    }
    userSetting->AP_NetInfo.IsBluetoothEnable    = FALSE;  

    gWlanIsConfigFinish = TRUE;
    gWlanChange         = FALSE;
    userSetting->AP_NetInfo.IsNetConfigComplete = TRUE;    
}


//=============================================================================================
//=============================================================================================
//配网配置 ：配网之时  关闭蓝牙 关闭wifi 然后一个一个配置  配置完后 开启 蓝牙 再开启wifi  
//1.关闭蓝牙 关闭wifi 
//2.设置需要连接的热点的密码
//3.设置需要连接的AP的SSID
//4.复位
//5.创建UDP 服务
//使用 HFAT+COM  不会有回显
//=============================================================================================

//===============================================================================
//开启 蓝牙
//升级过程不允许
void NCWanEnableBleMode(void)
{
    NCUserSetting* userSetting = NCUserSettingGet();
    BOOL   flag = FALSE;
    UINT8  BLE_Flag;
    //ENLoopArrayClear(gSendLoop);
    //NCWlan_RevDMA_Restart();
    
    //BleModeFunConfig(DISABLE);////关闭蓝牙
    vTaskDelay(150);
    ENLoopArrayClear(gSendLoop);
    //NCWlan_RevDMA_Restart();
    ENLoopArrayClear(gRecvLoop);
    
    BLE_Flag =  GetBleModeFunEnable();
    if(BLE_Flag == NC_BLE_GET_NO_ACK)
    {
       BLE_Flag =  GetBleModeFunEnable();
       if(BLE_Flag == NC_BLE_GET_NO_ACK)
       {
          BLE_Flag =  GetBleModeFunEnable();
       } 
    }
    if(BLE_Flag == NC_BLE_GET_CLOSEED)//确定是关闭下打开
    {
        flag = BleModeFunConfig(ENABLE);////开启蓝牙
        vTaskDelay(200);
        if(flag==TRUE)
        {
           userSetting->AP_NetInfo.IsBluetoothEnable    = TRUE;
           userSetting->AP_NetInfo.IsBleWaitConfig      = FALSE;
           return;
        }
        vTaskDelay(100);
        ENLoopArrayClear(gRecvLoop);
        BLE_Flag =  GetBleModeFunEnable();
        if(BLE_Flag == NC_BLE_GET_CLOSEED)
        {
           flag = BleModeFunConfig(ENABLE);////开启蓝牙
           vTaskDelay(200);
           if(flag==TRUE)
           {
              userSetting->AP_NetInfo.IsBluetoothEnable    = TRUE;
              userSetting->AP_NetInfo.IsBleWaitConfig      = FALSE;
               return;
            }
        
        }
       //开起失败//算了   
    }
    else if(BLE_Flag == NC_BLE_GET_OPENED)//本来就是开启的
    {
        userSetting->AP_NetInfo.IsBluetoothEnable    = TRUE;
        userSetting->AP_NetInfo.IsBleWaitConfig      = FALSE;
        return;
    }
    else ///if(BLE_Flag == NC_BLE_GET_NO_ACK)
    {
       //3次都不回复需要重启咯 挂了咯
       NCWlanHardReset();        
       return;//待会再进来开启吧
    }
    userSetting->AP_NetInfo.IsBleWaitConfig      = FALSE;   //必须清零不然不会解析数据WIFI数据了 
}

//=============================================================================
//关闭蓝牙
void CloseWIFIBleMode(void)
{
    NCUserSetting* userSetting = NCUserSettingGet();
    UINT8 BLE_Flag;
    ENLoopArrayClear(gSendLoop);
    NCWlan_RevDMA_Restart();
        //先检测是否BLE是开启的 如果不是开启的 关闭指令会有问题
    BLE_Flag =  GetBleModeFunEnable();
    if(BLE_Flag == NC_BLE_GET_NO_ACK)
    {
       BLE_Flag =  GetBleModeFunEnable();
       if(BLE_Flag == NC_BLE_GET_NO_ACK)
       {
          BLE_Flag =  GetBleModeFunEnable();
       }   
    }
    if(NC_BLE_GET_OPENED == BLE_Flag)
    {
        BleModeFunConfig(DISABLE);////关闭蓝牙
        vTaskDelay(200);
    }

    //WifiModeFunConfig(DISABLE);
    userSetting->AP_NetInfo.IsBluetoothEnable    = FALSE;
}


//进行配网
void NCWanNetConfigChange(void)  
{
    //BOOL  IsConfigParma = FALSE;
    UINT8  BLE_Flag;
    NCUserSetting* userSetting = NCUserSettingGet();
    
    userSetting->AP_NetInfo.IsNetConfigComplete = FALSE;
    userSetting->AP_NetInfo.IsNetConnect        = FALSE;
    userSetting->AP_NetInfo.IsNetCommunicating   = FALSE; 
    //userSetting->AP_NetInfo.IsNetConfigEnable   = FALSE;
    vTaskDelay(100);
    ENLoopArrayClear(gSendLoop);
    NCWlan_RevDMA_Restart();
    
    #if   0
    //1.关闭蓝牙 关闭wifi 
     WLAN_CONFIG_BEGIN();
     while(FALSE == BleModeFunConfig(DISABLE)) //关闭蓝牙
     {
         WLAN_CONFIG_RETURN();
     }

     WLAN_CONFIG_BEGIN();
     while(FALSE == WifiModeFunConfig(DISABLE)) //关闭WIFI
     {
         WLAN_CONFIG_RETURN();
     }   
     #endif
     //2.设置AP的SSID
     WLAN_CONFIG_BEGIN();
     while(FALSE == WifiSetSSID(userSetting->AP_NetInfo.AP_SSID)) //SSID
     {
         WLAN_CONFIG_RETURN();
     }      
     //3.设置AP的PassWord
     WLAN_CONFIG_BEGIN();
     while(FALSE == WifiSetPassWord(userSetting->AP_NetInfo.AP_Password)) //PassWord
     {
         WLAN_CONFIG_RETURN();
     } 

    //if(FALSE == WifiGetDHCP())
    {
        #ifdef  IS_LOG_EN
        //printf("WifiSetDHCP");
        #endif
        WLAN_CONFIG_BEGIN();
         while(FALSE == WifiSetDHCP()) //配置DHCP
         {
            WLAN_CONFIG_RETURN();
         } 
    }     
     //4. 复位     
     NCWlanHardReset();//硬件复位
     NCWlan_RevDMA_Restart();
     ENLoopArrayClear(gSendLoop);
//    [17:33:04.625]发→◇HFAT+WSSSID=Summer□
//    [17:33:04.688]收←◆+ok


//    [17:34:00.649]发→◇HFAT+WSKEY=WPA2PSK,AES,qt12345678□
//    [17:34:00.712]收←◆+ok


//    [17:34:36.881]发→◇HFAT+Z□
//    [17:34:36.897]收←◆+ok


//    [17:34:39.572]收←◆TW100
//    [17:34:40.720]收←◆+EVENT=CON_ON

//    [17:34:44.547]收←◆+EVENT=DHCP_OK
     //vTaskDelay(1500);//等待主机分配IP 和连接
     //5.创建UDP 服务
     WLAN_CONFIG_BEGIN();
     while(FALSE == WifiSetUdpClient(userSetting->AP_NetInfo.Server_IP,userSetting->AP_NetInfo.Server_PORT)) //关闭WIFI
     {
         WLAN_CONFIG_RETURN();         
     }
     
//     WLAN_CONFIG_BEGIN();
//     while(FALSE == BleModeFunConfig(DISABLE)) //关闭蓝牙
//     {
//         WLAN_CONFIG_RETURN();
//     }

//    vTaskDelay(8000);
//     //6.查询是否连接入网络
//     if(TRUE==WifiGetLinkState())
//     {
//        #ifdef  IS_LOG_EN
//        printf("Link Is Connect AP");
//        #endif
//        userSetting->AP_NetInfo.IsNetConnect        = TRUE;
//     }
   // vTaskDelay(4000);
     
    vTaskDelay(150);//必须要等待一段时间
    #if     0
    if(NC_BLE_GET_CLOSEED == GetBleModeFunEnable())
    {
        #ifdef  IS_LOG_EN
        //printf(" Enable Ble");
        #endif        
        WLAN_CONFIG_BEGIN();
         while(FALSE == BleModeFunConfig(ENABLE)) //配置
         {
            WLAN_CONFIG_RETURN();
         }
    }
    #else
    //先检测是否BLE是开启的 如果不是开启的 关闭指令会有问题
    BLE_Flag =  GetBleModeFunEnable();
    if(BLE_Flag == NC_BLE_GET_NO_ACK)
    {
       BLE_Flag =  GetBleModeFunEnable();
       if(BLE_Flag == NC_BLE_GET_NO_ACK)
       {
          BLE_Flag =  GetBleModeFunEnable();
       }   
    }
    if(NC_BLE_GET_OPENED == BLE_Flag)
    {       
        WLAN_CONFIG_BEGIN();
         while(FALSE == BleModeFunConfig(DISABLE)) //关闭蓝牙
         {
            WLAN_CONFIG_RETURN();
         }
    } 
       
    #endif
    userSetting->AP_NetInfo.IsBluetoothEnable    = FALSE; 

    UINT8  Timecnt= 0;    
    if((0 == memcmp(userSetting->AP_NetInfo.AP_Password,("SumQt_xx"),sizeof("SumQt_xx"))) && \
       (0 == memcmp(userSetting->AP_NetInfo.AP_SSID,("SummerQtSs"),sizeof("SummerQtSs"))))//相等
    {
       userSetting->AP_NetInfo.IsNetConnect = FALSE;
    }
    else
    {
        userSetting->AP_NetInfo.IsNetConnect = TRUE;
        while(FALSE== WifiGetLinkState()) 
        {
            vTaskDelay(300); 
            if(++Timecnt>4)      
            {
                userSetting->AP_NetInfo.IsNetConnect = FALSE;
                break;       
            }                    
        }       
    }
    //==================================================================== 
    NCWlan_RevDMA_Restart();
    
    userSetting->AP_NetInfo.IsNetConfigEnable    = FALSE;
    userSetting->AP_NetInfo.IsNetConfigComplete  = TRUE;  //在主程序 1S 判断一次是否进入   IsNetConnect
    ENLoopArrayClear(gSendLoop);
}

//================================================================================
//================================================================================
void WIFI_USART_DMA_IRQHandler(void)
{
    if((WIFI_USART_TX_DMA->INTSTS) & WIFI_USART_TX_DMA_TXCF)// 
    {
        WIFI_USART_TX_DMA->INTFCLR   = WIFI_USART_TX_DMA_GLBF | WIFI_USART_TX_DMA_TXCF |WIFI_USART_TX_DMA_HTXF| WIFI_USART_TX_DMA_ERRF;
        gWifiSending = FALSE;
    }
}


/*********************************************************************************************************
** Function name(函数名称):				USART1_IRQHandler()
**
** Descriptions（描述）:				USART2串口中断接收函数
**
** input parameters（输入参数）:		None
** Returned value（返回值）:			None
**         
** Used global variables（全局变量）:	None
** Calling modules（调用模块）:			None
**
** Created by（创建人）:				
**------------------------------------------------------------------------------------------------------
********************************************************************************************************/
//=========================================================
void WIFI_USART_IRQHandler(void)
{
    UINT16 Uart_DMA_Cnd,length;
    if(WIFI_USART->STS_B.IDLEFLG) //
    {
        Uart_DMA_Cnd = WIFI_USART->STS;
        Uart_DMA_Cnd = WIFI_USART->DATA;//读取清零UART_IT_RXNE      //RS485_USART->STS_B.IDLEFLG = FALSE; 
        Uart_DMA_Cnd = WLAN_COM_RX_BUF_SIZE - WIFI_USART_RX_DMA_CHANNEL->CHNDATA;        
        //if(gRS485RecvLoop  && gRS485RecvLoop->hasBeenInit ==TRUE)
        {
            if(Uart_DMA_LastCnt<Uart_DMA_Cnd)
            {
                  length = Uart_DMA_Cnd - Uart_DMA_LastCnt;
                  for(UINT16 i =0;i<length;++i)
                  {
                     ENLoopArrayInToISR(gRecvLoop, WlanRevBuf[Uart_DMA_LastCnt+i]); 
                  }                                                     
            }
            else if(Uart_DMA_LastCnt>Uart_DMA_Cnd)
            {
               length = WLAN_COM_RX_BUF_SIZE -  Uart_DMA_LastCnt; 
              for(UINT16 i =0;i<length;++i)
              {
                 ENLoopArrayInToISR(gRecvLoop, WlanRevBuf[Uart_DMA_LastCnt+i]); 
              }
              if(Uart_DMA_Cnd>0)
              {
                  for(UINT16 i =0;i<Uart_DMA_Cnd;++i)
                  {
                     ENLoopArrayInToISR(gRecvLoop, WlanRevBuf[i]);   
                  }
              }            
            }         
        
        }
      Uart_DMA_LastCnt = Uart_DMA_Cnd;  
    }    
    //========================================================
//    if(RS485_USART->STS_B.RXBNEFLG) //接收数据中断 
//    {       
//        //if(gRS485RecvLoop)
//        ENLoopArrayInByISR(gRS485RecvLoop, (UINT8)(RS485_USART->DATA));//读取清零UART_IT_RXNE
//        RS485_USART->STS_B.RXBNEFLG = FALSE;
//    }
    if(WIFI_USART->STS_B.TXCFLG)    //发送
    { 
        WIFI_USART->STS_B.TXCFLG = FALSE;
    } 
    ///此芯片有单独的溢出错误使能标记
    if(WIFI_USART->STS_B.OVREFLG)    //发送
    { 
        Uart_DMA_Cnd = WIFI_USART->STS;
        Uart_DMA_Cnd = WIFI_USART->DATA;//读取清零 
        
    }           
}






 



#ifndef  __NCWLAN_H_
#define  __NCWLAN_H_
#include "Typedef.h"
#include "ENLoopArray.h"
#include "apm32f103_conf.h"

#define WLAN_COM_RX_BUF_SIZE         (1600)
//====================================================================
//引脚定义
/*******************************************************/
#define WIFI_USART                          USART1
#define WIFI_USART_CLK_ENABLE()             RCM_EnableAPB2PeriphClock(RCM_APB2_PERIPH_USART1)

#define WIFI_USART_RX_GPIO_PORT             GPIOA
#define WIFI_USART_RX_PIN                   GPIO_PIN_10

#define WIFI_USART_TX_GPIO_PORT             GPIOA
#define WIFI_USART_TX_PIN                   GPIO_PIN_9

#define WIFI_NRST_PORT                      (GPIOB)
#define WIFI_NRST_PIN                       (GPIO_PIN_3) 
#define WIFI_NRST_PIN_HIGH                  (WIFI_NRST_PORT->BSC  =   WIFI_NRST_PIN)
#define WIFI_NRST_PIN_LOW                   (WIFI_NRST_PORT->BC   =   WIFI_NRST_PIN)   

#define WIFI_POWER_PORT                      (GPIOB)
#define WIFI_POWER_PIN                       (GPIO_PIN_9) 
#define WIFI_POWER_PIN_ENABLE                (WIFI_POWER_PORT->BSC  =   WIFI_POWER_PIN)
#define WIFI_POWER_PIN_DISABLE               (WIFI_POWER_PORT->BC   =   WIFI_POWER_PIN)   
    
#define WIFI_NLINK_PORT                      (GPIOB)
#define WIFI_NLINK_PIN                       (GPIO_PIN_5) //已经连接上网络
#define WIFI_NLINK_PIN_HIGH                  (WIFI_NLINK_PORT->BSC  =   WIFI_NLINK_PIN)
#define R_WIFI_NLINK_STA                     ((WIFI_NLINK_PORT->IDATA & WIFI_NLINK_PIN) ?  BIT_SET : BIT_RESET)


#define WIFI_NREADY_PORT                     (GPIOB)
#define WIFI_NREADY_PIN                      (GPIO_PIN_4)
#define WIFI_NREADY_PIN_HIGH                  (WIFI_NREADY_PORT->BSC  =   WIFI_NREADY_PIN)
#define R_WIFI_NREADY_STA                    ((WIFI_NREADY_PORT->IDATA & WIFI_NREADY_PIN) ?  BIT_SET : BIT_RESET)

//================================================================
#define WIFI_USART_IRQ                 	     USART1_IRQn
#define WIFI_USART_PRIORITY                  1
#define WIFI_USART_IRQHandler                USART1_IRQHandler
#define WIFI_USART_DAT_Base                  (USART1_BASE + 0x04)

//发送
#define WIFI_USART_DMA_IRQ                   DMA1_Channel4_IRQn
#define WIFI_USART_DMA_PRIORITY              2
#define WIFI_USART_DMA_IRQHandler            DMA1_Channel4_IRQHandler

#define WIFI_USART_TX_DMA                    DMA1
#define WIFI_USART_TX_DMA_CHANNEL            DMA1_Channel4

#define WIFI_USART_TX_DMA_GLBF               DMA1_INT_FLAG_GINT4//DMA1 Channel7 global interrupt
#define WIFI_USART_TX_DMA_TXCF               DMA1_INT_FLAG_TC4//DMA1 Channel7 transfer complete interrupt
#define WIFI_USART_TX_DMA_HTXF               DMA1_INT_FLAG_HT4//DMA1 Channel7 half transfer interrupt
#define WIFI_USART_TX_DMA_ERRF               DMA1_INT_FLAG_TERR4//DMA1 Channel7 transfer error interrupt
#define WIFI_USART_TX_DMA_FLAG               WIFI_USART_TX_DMA_TXCF//DMA_FLAG_TC1//DMA_CH1_TXCF

//接收
#define WIFI_USART_RX_DMA                    DMA1
#define WIFI_USART_RX_DMA_CHANNEL            DMA1_Channel5

#define WIFI_USART_RX_DMA_GLBF               DMA1_INT_FLAG_GINT5//DMA1 Channel6 global interrupt
#define WIFI_USART_RX_DMA_TXCF               DMA1_INT_FLAG_TC5  //DMA1 Channel6 transfer complete interrupt
#define WIFI_USART_RX_DMA_HTXF               DMA1_INT_FLAG_HT5  //DMA1 Channel6 half transfer interrupt
#define WIFI_USART_RX_DMA_ERRF               DMA1_INT_FLAG_TERR5//DMA1 Channel6 transfer error interrupt
#define WIFI_USART_Rx_DMA_FLAG               WIFI_USART_RX_DMA_TXCF


//#pragma pack(push) //保存对齐状态
//#pragma pack(1)//设定为1字节对齐
//typedef struct _NCWlanIP
//{
//    UINT8 ip1;
//    UINT8 ip2;
//    UINT8 ip3;
//    UINT8 ip4;
////    UINT8 subnetMask1;
////    UINT8 subnetMask2;
////    UINT8 subnetMask3;
////    UINT8 subnetMask4;
////    UINT8 gatway1;
////    UINT8 gatway2;
////    UINT8 gatway3;
////    UINT8 gatway4;
//}NCWlanIP;
//#pragma pack(pop)//恢复对齐状态

//======================================================
void NCWlanInitBufferConfig(ENLoopArray *sendLoop, ENLoopArray *recvLoop);
BOOL NCWlanIsSending(void);
BOOL NCWlanConfigFinish(void);
BOOL NCIsWlanConfigChange(void);
void NCWlanConfigEnableChange(void);
void NCWlan_RevDMA_Restart(void);
void NCWlan_RevDMA_Disable(void);
void NCWlan_TxDMA_Disable(void);
void NCWlan_DMA_UsartTxTransmit(UINT32 cmar, UINT16 cndtr);
void NCWlan_DMA_Send(UINT8* cmar,UINT16 cndtr);
void NCWlanSendStringBuffer(const CHAR *buffer);
void NCWlan_StartSend(void);

void NCWlan_TxBuffer_Deinit(void);
void NCWlan_RxBuffer_Deinit(void);

ENLoopArray * getNCWlan_RecvLoopPtr(void);
void NCWlan_Init(void);

void NCWanEnableBleMode(void);
void NCWanPowerOnConfigChange(void);
void NCWanNetConfigChange(void);


void CloseWIFIBleMode(void);












































#endif

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




#define MAX_DEVICE_COUNT  256
#define NI_SCHEDULE_TYPE_COUNT 2
#define TEST_TYPE_NIGHTLIGHT	2
#define DEV_GROUP_MAX        64
#define MAX_TEST_PLANS       64

typedef struct
{
   uint8_t nettype;   
   uint16_t  device_no; 
   uint8_t  logoinx; 
   uint8_t status;                              // 灯状态
   uint64_t macaddr;                            // 新的短地址 (0-127)
   char name[32+1];                         // 灯名称
   char location[32+1];                      // 位置信息
   uint8_t type;                                // 灯类型
   uint8_t config; 
   uint8_t update_flag; 
   uint64_t group_mask;    // bit0~bit63, 0=disable,1=enable
   uint64_t type_mask; 
}DRIVER_LAMP_ATTR;

typedef struct 
{
    uint8_t count;
    DRIVER_LAMP_ATTR devices[MAX_DEVICE_COUNT];
} device_list_t;
device_list_t	dev_list;

typedef struct 
{
    uint64_t addr_port;        /**< 目标参数 (端口号/短地址/组掩码) */
    time_t   trigger_time;     /**< UTC 触发时间 (秒) */
    uint8_t  state;            /**< 0=空闲, 1=待执行, 2=已执行 */
    uint8_t  nearnettype;        /* nearnet type */
    uint8_t  test_type;        /**< 测试类型 (0-3) */
    uint8_t  lamptype;         /**< 目标类型: ALL / PORT / ADDR / GROUP */
    uint8_t  onoff;            /**< 开关状态 (0/1) */
    uint8_t  update_flag;  
} dev_test_plan_t;
static dev_test_plan_t plans[MAX_TEST_PLANS] __attribute__((section(".dtcm_data"), aligned(8)));

typedef struct 
{
    uint8_t  active;           /* 0=不执行, 1=执行 */
    uint8_t  test_type;        /* TEST_TYPE_FT / DT */
    uint8_t  frequency;        /* FT:0-3, DT:0-4 (详见注释) */
    uint8_t  periodtime;             /* bit0~6: 周日~周六 */
    uint8_t  update_flag;  
    uint32_t start_sec;         /* 一天内触发时间 (0~86399) */
    time_t   next_trigger;      /* 下一次触发的绝对 UTC 时间 (秒) */
} group_schedule_t;
static group_schedule_t g_schedules[TEST_TYPE_NIGHTLIGHT][DEV_GROUP_MAX] __attribute__((section(".bss.dtcm_bss")));


typedef struct {
    uint8_t active;          // PLAN_TYPE_WAIT, PLAN_TYPE_IDLE
    uint8_t test_type;       // TEST_TYPE_NIGHTLIGHT, TEST_TYPE_INHIBIT
    //uint8_t schedule_type;   // ONCE 或 WEEKLY
    uint8_t  update_flag;  
    uint8_t frequency;       // (仅WEEKLY有效) 间隔周数
    uint8_t week_mask;       // (仅WEEKLY有效) 星期掩码
    uint32_t start_sec;      // ONCE: 绝对UTC秒; WEEKLY: 当日偏移秒
    uint32_t end_time;       // 同上
    time_t next_trigger;     // 绝对UTC秒
    time_t end_trigger;     // 绝对UTC秒
} ni_schedule_t;
static ni_schedule_t g_ni_schedules[NI_SCHEDULE_TYPE_COUNT][DEV_GROUP_MAX] __attribute__((section(".bss.dtcm_bss")));









































#endif

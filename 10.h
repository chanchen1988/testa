#ifndef PROTOCOLDEFINITION_H
#define PROTOCOLDEFINITION_H
#include "NCUtilDefine.h"
#include "Typedef.h"

#define		PCHAR		CHAR
#define		PUINT8		UINT8
#define		PINT8		INT8
#define		PUINT16		UINT16
#define		PUINT32		UINT32
#define		PINT32		INT32
#define		PINT64		INT64
#define		PUINT64		UINT64
#define		PFLOAT		FLOAT

#define HEAD_SIGN_CMD			    (0xA5)	//帧头起始符号(Head)
#define HEAD_SIGN_CMD_H			    (0x5A)	//帧头起始符号(Head)
#define HEAD_SIGN_CMD_BLEH			(0xEA)	//帧头起始符号(Head)


#define MIN_PROTOCOL_LENGTH		    (9)//
#define MAX_PROTOCOL_LENGTH		    (1040)//(1200)//由于SRAM有限 1000(Data)	+ 6
#define ERROR_CODE				    (0xFFFF)
#define MAX_MSCOM_PROTOCOL_LENGTH	(64)


//================================================

// 串口接收，解析中使用
#define CMD_HEAD          (0)
#define CMD_HEAD_H        (1)
#define CMD_CMD_L         (2)
#define CMD_CMD_H         (3)
#define CMD_DATALENGTH_L  (4)
#define CMD_DATALENGTH_H  (5)
#define CMD_RX_RESERVE_1  (6)
#define CMD_RX_RESERVE_2  (7)
#define CMD_CHECKSUM      (8)
#define CMD_RX_UID        (9)
#define CMD_DATA          (10)


#define CMD_TX_HEAD          (0)
#define CMD_TX_HEAD_H        (1)
#define CMD_TX_CMD_L         (2)
#define CMD_TX_CMD_H         (3)
#define CMD_TX_DATALENGTH_L  (4)
#define CMD_TX_DATALENGTH_H  (5)
#define CMD_TX_RESERVE_1     (6)
#define CMD_TX_RESERVE_2     (7)
#define CMD_TX_CHECKSUM      (8)
#define CMD_TX_DATA          (9)

//===================================================================

//===================================
typedef enum //_NC_PROTOCOL_CMD_FOR_WLAN
{
    g_eApp2D_PushWifiConnectInfo          = 0x0001,
    g_eApp2D_PushWifiDisConnectCmd        = 0x0002,//
    g_eApp2D_PushDeviceAutoPowerOn        = 0x000F,//USB 自动开机指令
    
    g_eApp2D_PushFirmwareUpdataInfo       = 0x0010,//App Push Firmware Updata Info
    g_eApp2D_PushFirmwarePackageData      = 0x0011,
    g_eApp2D_PushSlaveFirmwareUpdataInfo  = 0x0012,//App Push Slave Firmware Updata Info
    g_eApp2D_PushSlaveFirmwarePackageData = 0x0013,
    g_eApp2D_AckSlaveFirmwareFailInfo     = 0x0014,
    
    //====================================================================
    g_eApp2D_AskDeviceBaseInfo            = 0x0020,
    g_eApp2D_AckDeviceRealTimeReport      = 0x0021,//模组数据上报设置指令
    g_eApp2D_PushDeviceWorkCommand        = 0x0022,//模组启停运行设置指令
    //0x0023   从机指令
    g_eApp2D_PushDeviceCtrPowerCommand    = 0x0024,//模组控制电源使能/除能指令
    g_eApp2D_PushDevicePowerOffCmd        = 0x0025,//模组关机指令 
    g_eApp2D_PushDeviceCleanState         = 0x0026,//清楚容量 
    g_eApp2D_PushDeviceFallDischargeCmd   = 0x0028,
	
	g_eApp2D_AskDevAgingInfoCmd           = 0x0030,
    //=====================================================================
    g_eApp2D_PushWitreSN                  = 0x00E0,//模组设备序列号写入指令
    g_eApp2D_PushDeviceCaliParam          = 0x00E1,//模组校准参数写入指令
    g_eApp2D_PushDeviceCaliConfig         = 0x00E2,//模组校准配置操作指令
    g_eApp2D_AskDeviceCalingData          = 0x00E3,//请求读取模组校准过程中数据
    g_eApp2D_PushDeviceTestConfig         = 0x00E4,//模组测试配置操作指令
    g_eApp2D_AskDeviceTestingData         = 0x00E5,//请求读取模组校准过程中数据
    
    g_eApp2D_PushDeviceEntryAging         = 0x00F0,//下发设备进入老化
                   
    //设备到APP
    g_eD2App_AckWifiConnectInfo           = 0x8001, 
    g_eD2App_AckWifiDisConnectCmd         = 0x8002,//
    g_eD2App_AckDeviceAutoPowerOn         = 0x800F,//USB 自动开机指令
    //============================================================
    g_eD2App_AckFirmwareUpdataInfo        = 0x8010,// App Ack Firmware Updata Info
    g_eD2App_AckFirmwarePackageData       = 0x8011,
    g_eD2App_AckSlaveFirmwareUpdataInfo   = 0x8012,// App Ack Slave Firmware Updata Info
    g_eD2App_AckSlaveFirmwarePackageData  = 0x8013,
    g_eD2App_PushSlaveFirmwareFailInfo    = 0x8014,//从机升级失败
    //====================================================
    g_eD2App_AckDeviceBaseInfo            = 0x8020,
    g_eD2App_PushDeviceRealTimeReport     = 0x8021,
	g_eD2App_AckDeviceWorkCommand         = 0x8022,
    
    g_eD2App_AckDeviceCtrPowerCommand     = 0x8024,
    g_eD2App_AckDevicePowerOffCmd         = 0x8025,//模组关机指令
    g_eD2App_AckDeviceCleanState          = 0x8026,
    g_eD2App_AckAppRtDatAck               = 0x8027,//收到g_eApp2D_AckDeviceRealTimeReport      = 0x0021 应答
    g_eD2App_AckDeviceFallDischargeCmd    = 0x8028,
    
	g_eD2App_AckDevAgingInfoCmd           = 0x8030,
    //==========================================================================
    g_eD2App_AckWitreSN                   = 0x80E0,//SN写入应答
    g_eD2App_AckDeviceCaliParam           = 0x80E1,//模组校准参数写入指令
    g_eD2App_AckDeviceCaliConfig          = 0x80E2,//模组校准配置操作指令
    g_eD2App_AckDeviceCalingData          = 0x80E3,//请求读取模组校准过程中数据
    g_eD2App_AckDeviceTestConfig          = 0x80E4,//模组测试配置操作指令
    g_eD2App_AckDeviceTestingData         = 0x80E5,//请求读取模组校准过程中数据 
  
    g_eD2App_AckDeviceEntryAging          = 0x80F0,//下发设备进入老化
    
}NC_PROTOCOL_CMD_FOR_WLAN;    

//===========================================================================
typedef enum
{
    g_eM2S_PushFirmwareUpdataInfo       = 0x0010,//App Push Firmware Updata Info
    g_eM2S_PushFirmwarePackageData      = 0x0011,
    g_eM2S_AckDeviceRealTimeReport      = 0x0021,//模组数据上报设置指令
    g_eM2S_PushDeviceWorkCommand        = 0x0022,//模组启停运行设置指令
    g_eM2S_AskDeviceRunRealData         = 0x0023,//工作过程中读取数据
    g_eM2S_AckMasterFanPauseCmd         = 0x002F,//回复从机申请关闭风扇和充电5S
    //==========================================================================
    g_eM2S_PushDeviceCaliParam          = 0x00E1,//模组校准参数写入指令
    g_eM2S_PushDeviceCaliConfig         = 0x00E2,//模组校准配置操作指令
    g_eM2S_AskDeviceCalingData          = 0x00E3,//请求读取模组校准过程中数据
    g_eM2S_PushDeviceTestConfig         = 0x00E4,//模组测试配置操作指令
    g_eM2S_AskDeviceTestingData         = 0x00E5,//请求读取模组校准过程中数据
    
    //设备到APP
    g_eS2M_AckFirmwareUpdataInfo        = 0x8010,//App Ack Firmware Updata Info
    g_eS2M_AckFirmwarePackageData       = 0x8011,
    g_eS2M_PushDeviceRealTimeReport     = 0x8021,
	g_eS2M_AckDeviceWorkCommand         = 0x8022,
    g_eS2M_AckDeviceRunRealData         = 0x8023,//工作过程中读取数据
    g_eS2M_AskMasterFanPauseCmd         = 0x802F,//从机申请关闭风扇和充电5S
	//==========================================================================
    g_eS2M_AckDeviceCaliParam           = 0x80E1,//模组校准参数写入指令
    g_eS2M_AckDeviceCaliConfig          = 0x80E2,//模组校准配置操作指令
    g_eS2M_AckDeviceCalingData          = 0x80E3,//请求读取模组校准过程中数据
    g_eS2M_AckDeviceTestConfig          = 0x80E4,//模组测试配置操作指令
    g_eS2M_AckDeviceTestingData         = 0x80E5,//请求读取模组校准过程中数据
    
}NC_PROTOCOL_CMD_FOR_MSCOM;


typedef union 
{
	UINT8   cmdAllDataBuffer[MAX_MSCOM_PROTOCOL_LENGTH];
    #pragma pack(push)			//One Byte Align
    #pragma pack(1)
	struct 
	{
		UINT16    CMD_Head_Sign;/*同步头2Byte 固定0xA5A5*/
		UINT16    cmd;/*命令 2Byte*/
		UINT16    len;/*数据长度 2Byte 最大值(0~65535byte)*/
		UINT8     reserv_1;/*保留字 1Byte*/
		UINT8     reserv_2;/*保留字 1Byte*/
		UINT8     Checksum;/*检总校验  sync_header+cmd+len+reserv_1+reserv_2+udat*/
		UINT8     DataBuf[MAX_MSCOM_PROTOCOL_LENGTH-MIN_PROTOCOL_LENGTH];/*用户有效数据*/
	}frame;
    #pragma pack(pop)
}MSCOM_Protocol_un;  //接收协议数据结构   

#pragma pack(push)			//One Byte Align

#pragma pack(1)

//
typedef struct
{
    PCHAR     AP_DeviceSN[20];
    PCHAR     AP_SSID[16];
    PCHAR     AP_Password[16];
    PCHAR     Server_IP[16];
    PUINT16   Server_PORT;
}stuApp2D_PushWifiConnectInfo;


typedef struct
{
    PUINT8  IsSuccess; 
}stuD2App_AckWifiConnectInfo;

//=============================================
typedef struct
{
    PUINT8    Handle_Type;
}stuApp2D_PushWifiDisConnectCmd;


typedef struct
{
    PCHAR   DeviceSN[MAX_SN_LEN_REAL];
    PUINT8  DeviceAckInfo; 
}stuD2App_AckWifiDisConnectCmd;

//==============================================
typedef struct
{
    PUINT8  DeviceAckInfo; 
}stuD2App_AckFirmwareUpdataInfo;

typedef struct
{
    PUINT8  CurrnetPackageIndex;
    PUINT8  DeviceAckInfo;
}stuD2App_AckFirmwarePackageData; 


typedef struct
{
    PCHAR   DeviceSN[MAX_SN_LEN_REAL];
    PUINT8  DeviceAckInfo; 
}stuD2App_WIFIAckFirmwareUpdataInfo;

typedef struct
{
    PCHAR   DeviceSN[MAX_SN_LEN_REAL];
    PUINT8  CurrnetPackageIndex;
    PUINT8  DeviceAckInfo;
}stuD2App_WIFIAckFirmwarePackageData; 

//===========================================================
typedef struct
{
   PUINT32   Firmware_FileSize;
   PUINT32   Firmware_FileCheckSum;
   PUINT8    Firmware_PackageTotal;
   PCHAR     Firmware_Version[3]; 
   PUINT8    HardwareVersion;
}stuApp2D_PushFirmwareUpdataInfo;

typedef struct
{
   PUINT8     Firmware_CurrentPackage;
   PUINT16    Firmware_PackageData[512]; //512字节 
}stuApp2D_PushFirmwarePackageData;

//==============================================================
//从机
typedef struct
{
   PUINT8  TargetUid;
   PUINT32   Firmware_FileSize;
   PUINT32   Firmware_FileCheckSum;
   PUINT8    Firmware_PackageTotal;
   PCHAR     Firmware_Version[3]; 
   PUINT8    HardwareVersion;
}stuApp2D_PushSlaveFirmwareUpdataInfo;

typedef struct
{
   PUINT8     TargetUid;
   PUINT8     Firmware_CurrentPackage;
   PUINT16    Firmware_PackageData[512]; //512字节 
}stuApp2D_PushSlaveFirmwarePackageData;

typedef struct
{
    PUINT8  TargetUid;
    PUINT8  DeviceAckInfo; 
}stuD2App_AckSlaveFirmwareUpdataInfo;

typedef struct
{
    PUINT8  TargetUid;
    PUINT8  CurrnetPackageIndex;
    PUINT8  DeviceAckInfo;
}stuD2App_AckSlaveFirmwarePackageData; 

typedef struct
{
    PCHAR   DeviceSN[MAX_SN_LEN_REAL];
    PUINT8  TargetUid;
    PUINT8  DeviceAckInfo; 
}stuD2App_WIFIAckSlaveFirmwareUpdataInfo;

typedef struct
{
    PCHAR   DeviceSN[MAX_SN_LEN_REAL];
    PUINT8  TargetUid;
    PUINT8  CurrnetPackageIndex;
    PUINT8  DeviceAckInfo;
}stuD2App_WIFIAckSlaveFirmwarePackageData; 


typedef struct
{
    PUINT8  TargetUid;
    PUINT8  DeviceAckInfo;
}stuD2App_PushSlaveFirmwareFailInfo; 



//
//typedef struct
//{    
//    PCHAR   DeviceName[MAX_NAME_LEN];  
//    PCHAR   DeviceSN[MAX_SN_LEN];    
//    PCHAR   SoftVersion[3];  
//    PUINT8  HardwareVersion;
//}stuD2App_PushFirmwareUpdataFinish;


//=============================================================
typedef struct
{
    PUINT8    Handle_Type;     //
}stuApp2D_AskDeviceBaseInfo;

typedef struct
{
    PCHAR   DeviceSN[MAX_SN_LEN_REAL];
    PCHAR   SoftVersionD1[3];
    PCHAR   SoftVersionD2[3];
    PCHAR   SoftVersionD3[3];
    PCHAR   SoftVersionD4[3];
    PUINT8  HardwareVersionD1; 
//    PUINT8  HardwareVersionD2;
//    PUINT8  HardwareVersionD3;
//    PUINT8  HardwareVersionD4;            
}stuD2App_AckDeviceBaseInfo; 
//===============================================================
typedef struct
{
    PUINT8    TargetUid;
    PUINT8    DeviceWorkState;       // 运行启停状态；0x01:启动
    PUINT16   TestedCellSetVolt;     // 工作电压(单位: mV) (2字节)
    PUINT16   TestedCellSetCurrent;  // 工作电流(单位: mA) (2字节)
    PUINT8    TestedCellType;         // 电池类型(1字节)
    PUINT16   Cell_Volt_Upper;       // 上限电压(单位: mV) 根据所选择的电芯类型下发 (2字节)
    PUINT16   Cell_Volt_Lower;       // 下限电压(单位: mV) 根据所选择的电芯类型下发 (2字节)
    PUINT16   VoltUnusualChangeRate; // 电压异常变化率(单位: mV/sec) (2字节)  
}stuApp2D_PushDeviceWorkCommand;

typedef struct
{
    PCHAR     DeviceSN[MAX_SN_LEN_REAL];
    PUINT8    TargetUid;
    PUINT8    DeviceWorkState;  
    PUINT8    IsSuccessUID01;
    PUINT8    IsSuccessUID02;
    PUINT8    IsSuccessUID03;
    PUINT8    IsSuccessUID04;
}stuD2App_AckDeviceWorkCommand;

//==================================================================
typedef struct
{
    PUINT8    TargetUid;
    PUINT8    DeviceWorkState;       // 运行启停状态；0x01:启动
    PUINT8    TestedCellType;         // 电池类型(1字节)
    PUINT16   Cell_Volt_Lower;       // 下限电压(单位: mV) 根据所选择的电芯类型下发 (2字节)
    PUINT16   TestedCellSetVolt;      // 工作电压(单位: mV) (2字节)
    PUINT16   TestedCellSetCh1Current;// 工作电流(单位: mA) (2字节)
    PUINT16   TestedCellSetCh2Current;// 工作电流(单位: mA) (2字节)
    PUINT16   TestedCellSetCh3Current;// 工作电流(单位: mA) (2字节)
    PUINT16   TestedCellSetCh4Current;// 工作电流(单位: mA) (2字节)
}stuApp2D_PushDeviceFallDischargeCmd; 

typedef struct
{
    PCHAR     DeviceSN[MAX_SN_LEN_REAL];
    PUINT8    TargetUid;
    PUINT8    DeviceWorkState;  
    PUINT8    IsSuccess;
}stuD2App_AckDeviceFallDischargeCmd;

//==================================================================
typedef struct
{
    PCHAR     DeviceSN[MAX_SN_LEN_REAL];
    PUINT8    CtrlState;  
    PUINT8    IsSuccess;
}stuD2App_AckDeviceCleanState;


typedef struct
{
    PCHAR     DeviceSN[MAX_SN_LEN_REAL];
    PUINT8    IsSuccess;
}stuD2App_AckDeviceAutoPowerOn;

typedef struct
{
    PUINT8    TargetUid;
    PUINT8    DeviceCtrState;       
}stuApp2D_PushDeviceCtrPowerCommand;

typedef struct
{
    PCHAR     DeviceSN[MAX_SN_LEN_REAL];
    PUINT8    TargetUid;
    PUINT8    DeviceCtrState;  
    PUINT8    IsSuccess;
}stuD2App_AckDeviceCtrPowerCommand;

//关机指令
typedef struct
{
    PUINT8    DeviceCtrState;       
}stuApp2D_PushDevicePowerOffCmd;

typedef struct
{
    PCHAR     DeviceSN[MAX_SN_LEN_REAL];
    PUINT8    DeviceCtrState;  
    PUINT8    IsSuccess;
}stuD2App_AckDevicePowerOffCmd;


           

//============================================================
typedef struct
{
    PCHAR     DeviceSN[MAX_SN_LEN_REAL];
    PUINT8    FinAlarmState;               //散热器温度报警状态  
    //
    PUINT16   CellVoltageD1;               //运放电芯电压    mv
    PUINT16   CurrentRL_VoltD1;            //电阻网络电芯电压 mv
    PUINT16   CellCurrentD1;               //电芯电流        mA
    PUINT8    CellStateDataD1;             //运行状态
    PUINT8    DeviceAlarmStateD1;          //报警状态
    PFLOAT    CellCapacityD1;              //电池容量 
    //PFLOAT    CellCapacityWhD1;            //电池容量     
    
    PUINT16   CellVoltageD2;               //运放电芯电压    mv
    PUINT16   CurrentRL_VoltD2;            //电阻网络电芯电压 mv
    PUINT16   CellCurrentD2;               //电芯电流        mA
    PUINT8    CellStateDataD2;             //运行状态
    PUINT8    DeviceAlarmStateD2;          //报警状态
    PFLOAT    CellCapacityD2;              //电池容量 
    //PFLOAT    CellCapacityWhD2;            //电池容量 
    
    PUINT16   CellVoltageD3;               //运放电芯电压    mv
    PUINT16   CurrentRL_VoltD3;            //电阻网络电芯电压 mv
    PUINT16   CellCurrentD3;               //电芯电流        mA
    PUINT8    CellStateDataD3;             //运行状态
    PUINT8    DeviceAlarmStateD3;          //报警状态
    PFLOAT    CellCapacityD3;              //电池容量 
    //PFLOAT    CellCapacityWhD3;            //电池容量 
    
    PUINT16   CellVoltageD4;               //运放电芯电压    mv
    PUINT16   CurrentRL_VoltD4;            //电阻网络电芯电压 mv
    PUINT16   CellCurrentD4;               //电芯电流        mA
    PUINT8    CellStateDataD4;             //运行状态
    PUINT8    DeviceAlarmStateD4;          //报警状态
    PFLOAT    CellCapacityD4;              //电池容量  
    //PFLOAT    CellCapacityWhD4;            //电池容量  
    PFLOAT    Ext_NTC_Value;               //外部NTC数据   
    PINT8     FinNTC_Data;    
    PUINT8    CHxStopCode;                 //通道停止代码   
    PUINT8    CHxTotalState;               //总的状态码
}stuD2App_PushDeviceRealTimeReport;

//========================================================================================
typedef struct
{
    PCHAR     DeviceSN[MAX_SN_LEN_REAL];
}stuD2App_AckAppRtDatAck;

//========================================================================================
typedef struct
{
    PCHAR     DeviceSN[MAX_SN_LEN_REAL];
    //
    PUINT8    FinAlarmState;               //散热器温度报警状态  
    PUINT16   CellVoltageD1;               //运放电芯电压    mv
    PUINT16   CellCurrentD1;               //电芯电流        mA
    PUINT16   CellTotalCurrentD1;          //电芯总电流      mA
    PUINT8    CellStateDataD1;             //运行状态
    PUINT8    DeviceAlarmStateD1;          //报警状态
    //PFLOAT    CellCapacityD1;              //电池容量 
    //PFLOAT    CellCapacityWhD1;            //电池容量     
    
    PUINT16   CellVoltageD2;               //运放电芯电压    mv
    PUINT16   CellCurrentD2;               //电芯电流        mA
    PUINT16   CellTotalCurrentD2;          //电芯总电流      mA
    PUINT8    CellStateDataD2;             //运行状态
    PUINT8    DeviceAlarmStateD2;          //报警状态
    //PFLOAT    CellCapacityD2;              //电池容量 
    //PFLOAT    CellCapacityWhD2;            //电池容量 
    
    PUINT16   CellVoltageD3;               //运放电芯电压    mv
    PUINT16   CellCurrentD3;               //电芯电流        mA
    PUINT16   CellTotalCurrentD3;          //电芯总电流      mA
    PUINT8    CellStateDataD3;             //运行状态
    PUINT8    DeviceAlarmStateD3;          //报警状态
    //PFLOAT    CellCapacityD3;              //电池容量 
    //PFLOAT    CellCapacityWhD3;            //电池容量 
    
    PUINT16   CellVoltageD4;               //运放电芯电压    mv
    PUINT16   CellCurrentD4;               //电芯电流        mA
    PUINT16   CellTotalCurrentD4;          //电芯总电流      mA
    PUINT8    CellStateDataD4;             //运行状态
    PUINT8    DeviceAlarmStateD4;          //报警状态
    //PFLOAT    CellCapacityD4;              //电池容量  
    //PFLOAT    CellCapacityWhD4;            //电池容量  
    PFLOAT    Ext_NTC_Value;               //外部NTC数据   
    PINT8     FinNTC_Data;    
    PUINT8    CHxStopCode;                 //通道停止代码   
    PUINT8    CHxTotalState;               //总的状态码
    
}stuD2App_UsbPushDeviceRealTimeReport;
//==========================================================	

typedef struct
{
    PCHAR   PrintfInfo[128];   
}stuD2App_UsbPushPrintfInfo;
//==========================================================
typedef struct
{
   PUINT8     SN[MAX_SN_LEN]; 
}
stuApp2D_PushUsbWriterSNInfo;
//============================================================
//老化        
typedef struct
{    
    PUINT8  Handle_Type; 
	PUINT8  DevTimeStamp[12]; 
}stuApp2D_PushDeviceEntryAging; 

typedef struct
{    
    PUINT8  IsSuccess;
    PUINT8  CurrentState;
    PUINT8  CH1_State;
    PUINT8  CH2_State;
    PUINT8  CH3_State;
    PUINT8  CH4_State;    
}stuD2App_AckDeviceEntryAging;  
//============================================================
typedef struct
{  
    PCHAR   DeviceSN[MAX_SN_LEN_REAL];
    PCHAR   DevTimeStamp[12];
    PUINT8  IsComplete;
    PUINT8  CompleteInfo;          
}stuD2App_AckDevAgingInfoCmd;  
//=============================================================
typedef struct
{    
    PUINT8  IsSuccess; 
}stuD2App_AckUpdataSN_Info;
//==============================================
typedef struct
{
   PUINT8    TargetUid;
   PUINT8    Handle_Type;
   PFLOAT    BV_Cali_K_Value;  
   PFLOAT    BV_Cali_B_Value;
   //
   PFLOAT    DI_Cali_K_Value;  
   PFLOAT    DI_Cali_B_Value;
   //
   PFLOAT    DI_PWM_Cali_K_Value;  
   PFLOAT    DI_PWM_Cali_B_Value;  
    
   PFLOAT    WTI_Cali_K_Value;  
   PFLOAT    WTI_Cali_B_Value;
    
//   PFLOAT    High_BV_Cali_K_Value;  
//   PFLOAT    High_BV_Cali_B_Value;   
}stuApp2D_PushDeviceCaliParam;

typedef struct
{
   PUINT8    TargetUid;
   PUINT8    IsSuccess;  
   PFLOAT    BV_Cali_K_Value;  
   PFLOAT    BV_Cali_B_Value;
   //
   PFLOAT    DI_Cali_K_Value;  
   PFLOAT    DI_Cali_B_Value; 
   //
   PFLOAT    DI_PWM_Cali_K_Value;  
   PFLOAT    DI_PWM_Cali_B_Value; 
    
   PFLOAT    WTI_Cali_K_Value;  
   PFLOAT    WTI_Cali_B_Value; 

//   PFLOAT    High_BV_Cali_K_Value;  
//   PFLOAT    High_BV_Cali_B_Value;       
}stuD2App_AckDeviceCaliParam;
//============================================================
//===============================================
//模组校准配置操作指令
typedef struct
{    
    PUINT8    TargetUid;
    PUINT8    SettingType;
}stuApp2D_PushDeviceCaliConfig;

//--------------------------------------
typedef struct
{    
    PUINT8  TargetUid;
    PUINT8  SettingType;
    PUINT8  IsSuccess; 
}stuD2App_AckDeviceCaliConfig;
//
//=============================================================
//请求读取模组校准过程中数据
typedef struct
{   
    PUINT8    TargetUid;
    PUINT8    IsReadData;     
}stuApp2D_AskDeviceCalingData; 

//---------------------------------------
typedef struct
{    
    PUINT8    TargetUid;
    PUINT8    IsSuccess;     
    PUINT8    SettingType;
    PFLOAT    OrigContrlPWM;
    PFLOAT    OrigVolt;
    PUINT16   OrigCur;
    PUINT16   TotalOrigCur;
}stuD2App_AckDeviceCalingData;
//=============================================================
//模组测试配置操作指令
typedef struct
{   
    PUINT8    TargetUid;    
    PUINT8    SettingType;
}stuApp2D_PushDeviceTestConfig; 
//--------------------------------------------------------------
typedef struct
{   
    PUINT8    TargetUid;    
    PUINT8    SettingType;
    PUINT8    IsSuccess; 
}stuD2App_AckDeviceTestConfig;
//==============================================================
//请求读取模组测试过程中数据
typedef struct
{   
    PUINT8    TargetUid;    
    PUINT8    IsReadData; 
}stuApp2D_AskDeviceTestingData;

typedef struct
{    
    PUINT8    TargetUid;
    PUINT8    IsSuccess;
    PUINT8    SettingType;
    PUINT16   RealVolt;
    PUINT16   RealCur;
    PUINT16   RealTotalCur;
}stuD2App_AckDeviceTestingData;
//===================================================================

//===================================================================
//主模块与从模块通讯
//===========================================================
typedef struct
{
   PUINT32   Firmware_FileSize;
   PUINT32   Firmware_FileCheckSum;
   PUINT8    Firmware_PackageTotal;
   PCHAR     Firmware_Version[3]; 
   PUINT8    HardwareVersion;
}
stuM2S_PushFirmwareUpdataInfo;

typedef struct
{
    PUINT8    DeviceAckInfo; 
}stuS2M_AckFirmwareUpdataInfo;

typedef struct
{
   PUINT8     Firmware_CurrentPackage;
   PUINT16    Firmware_PackageData[512]; //512字节 
}
stuM2S_PushFirmwarePackageData;

typedef struct
{
    PUINT8    CurrnetPackageIndex;
    PUINT8    DeviceAckInfo;
}stuS2M_AckFirmwarePackageData; 
//-------------------------------------------------------------------------
//=========================================================================

#if   0
typedef struct
{
    PUINT8    DeviceWorkState;       // 运行启停状态；0x01:启动
    PUINT16   TestedCellSetVolt;     // 工作电压(单位: mV) (2字节)
    PUINT16   TestedCellSetCurrent;  // 工作电流(单位: mA) (2字节)
    PUINT16   Cell_Volt_Upper;       // 上限电压(单位: mV) 根据所选择的电芯类型下发 (2字节)
    PUINT16   Cell_Volt_Lower;       // 下限电压(单位: mV) 根据所选择的电芯类型下发 (2字节)
    PUINT16   VoltUnusualChangeRate; // 电压异常变化率(单位: mV/sec) (2字节)  
}stuM2S_PushDeviceWorkCommand;

typedef struct
{
    PUINT8    DeviceWorkState;  
    PUINT8    IsSuccess;     //    
}stuS2M_AckDeviceWorkCommand;
//============================================================
typedef struct
{
    //
    PUINT16   CellVoltage;             //运放电芯电压    mv
    PUINT16   DischCellCurrent;        //模块放电电流（忽悠模组自耗） mA
    PUINT16   CellCurrent;             //电芯总电流（加上模组自耗）   mA
    PUINT8    CellStateData;           //运行状态
    PUINT8    DeviceAlarmState;        //报警状态  
    PFLOAT    CellCapacity;            //电池容量
    PFLOAT    CellCapacityWH;          //电池容量 WH
    PCHAR     SoftVersion[3];
    PUINT8    HardwareVersion;    
}stuS2M_PushDeviceRealTimeReport;

//==========================================================
typedef struct
{    
    PUINT8    IsSuccess;
    PUINT8    MasterStatus; 
    PUINT8    FanCtrDuty;    
}stuM2S_AckDeviceRealTimeReport;

#else

typedef struct
{
    PUINT8    DeviceWorkState;       // 运行启停状态 0x00:停止（继电器也关闭） | 0x01:启动（如果继电器没有打开 先打开继电器 100ms开启输出） | 0x80:暂停（不关闭继电器）
    PUINT16   CellSetCurrent;        // 工作电流(单位: mA) (2字节)
}stuM2S_PushDeviceWorkCommand;

typedef struct
{
    PUINT8  DeviceWorkState;  
    PUINT8  IsSuccess;     //    
}stuS2M_AckDeviceWorkCommand;


typedef struct
{
    PUINT16   OPAmpCellVoltage;        //运放电芯电压    mv
    PUINT16   IdleTotalCurrent;        //电芯总电流（没有输出的时候总电流）   mA
    PUINT8    RunStateData;            //运行状态
    PCHAR     SoftVersion[3];
    //PUINT8    HardwareVersion; 
}stuS2M_PushDeviceRealTimeReport;
//=============================================================
typedef struct
{
    PUINT8  IsSuccess; 
}stuM2S_AckDeviceRealTimeReport;

typedef struct
{
    PUINT8    Handle_Type;
}stuM2S_AskDeviceRunRealData;
//=============================================================
typedef struct
{
    PUINT8    Handle_Type;            //操作标志
    PUINT16   OPAmpCellVoltage;       //运放电芯电压    mv
    PUINT16   DoingCellVoltage;       //工作中电芯电压    mv
    PUINT16   RunTotalCurrent;        //电芯总电流（没有输出的时候总电流）   mA
    PUINT16   DischCellCurrent;       //模块放电电流（忽悠模组自耗） mA
    PUINT8    RunStateData;           //运行状态
}stuS2M_AckDeviceRunRealData;
#endif


//==============================================
typedef struct
{
   PUINT8    Handle_Type;
   PFLOAT    BV_Cali_K_Value;  
   PFLOAT    BV_Cali_B_Value;
   //
   PFLOAT    DI_Cali_K_Value;  
   PFLOAT    DI_Cali_B_Value;
   //
   PFLOAT    DI_PWM_Cali_K_Value;  
   PFLOAT    DI_PWM_Cali_B_Value; 

   PFLOAT    WTI_Cali_K_Value;  
   PFLOAT    WTI_Cali_B_Value;  

//   PFLOAT    High_BV_Cali_K_Value;  
//   PFLOAT    High_BV_Cali_B_Value;       
}stuM2S_PushDeviceCaliParam;

typedef struct
{
   PUINT8    IsSuccess;  
   PFLOAT    BV_Cali_K_Value;  
   PFLOAT    BV_Cali_B_Value;
   //
   PFLOAT    DI_Cali_K_Value;  
   PFLOAT    DI_Cali_B_Value; 
   //
   PFLOAT    DI_PWM_Cali_K_Value;  
   PFLOAT    DI_PWM_Cali_B_Value; 
    
   PFLOAT    WTI_Cali_K_Value;  
   PFLOAT    WTI_Cali_B_Value; 
    
//   PFLOAT    High_BV_Cali_K_Value;  
//   PFLOAT    High_BV_Cali_B_Value;   
    
}stuS2M_AckDeviceCaliParam;
//============================================================
//===============================================
//模组校准配置操作指令
typedef struct
{    
    PUINT8    SettingType;
}stuM2S_PushDeviceCaliConfig;

//--------------------------------------
typedef struct
{    
    PUINT8    SettingType;
    PUINT8    IsSuccess; 
}stuS2M_AckDeviceCaliConfig;
//
//=============================================================
//请求读取模组校准过程中数据
typedef struct
{   
    PUINT8    IsReadData;     
}stuM2S_AskDeviceCalingData; 

//---------------------------------------
typedef struct
{    
    PUINT8    IsSuccess;     
    PUINT8    SettingType;
    PFLOAT    OrigContrlPWM;
    PFLOAT    OrigVolt;
    PUINT16   OrigCur;
    PUINT16   TotalOrigCur;        
}stuS2M_AckDeviceCalingData;
//=============================================================
//模组测试配置操作指令
typedef struct
{      
    PUINT8    SettingType;
}stuM2S_PushDeviceTestConfig; 
//---------------------------------
typedef struct
{      
    PUINT8    SettingType;
    PUINT8    IsSuccess; 
}stuS2M_AckDeviceTestConfig;
//==============================================================
//请求读取模组测试过程中数据
typedef struct
{     
    PUINT8    IsReadData; 
}stuM2S_AskDeviceTestingData;

typedef struct
{    
    PUINT8    IsSuccess;
    PUINT8    SettingType;
    PUINT16   RealVolt;
    PUINT16   RealCur;
    PUINT16   RealTotalCur;
}stuS2M_AckDeviceTestingData;

#pragma pack(pop)				//


#endif // PROTOCOLDEFINITION_H


//-----------------------------     

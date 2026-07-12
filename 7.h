#ifndef  __NCWLANPROTOCOL_H_
#define  __NCWLANPROTOCOL_H_
#include "protocoldefinition.h"
#include "ENLoopArray.h"
#include "NCMSComProtocol.h"
#define REALPACKET_TIME					(2000)


#define MAX_USB_PROTOCOL_LENGTH                       (1100) 


#if 1
// 结构体变量，转移到头文件，方便其他文件调用
// 设备向平板上传数据用
typedef union  //_ENExUpload
{
    stuD2App_AckWifiConnectInfo             AckWifiConnectInfo;
    stuD2App_AckDeviceBaseInfo              AckDeviceBaseInfo;
    stuD2App_AckDeviceWorkCommand           AckDeviceWorkCommand;
    stuD2App_PushDeviceRealTimeReport       PushDeviceRealTimeReport;
    stuD2App_AckDeviceCaliParam             AckDeviceCaliParam;
    stuD2App_AckUpdataSN_Info               AckUpdataSN_Info;
    stuD2App_AckDeviceCaliConfig            AckDeviceCaliConfig;
    stuD2App_AckDeviceCalingData            AckDeviceCalingData;
    stuD2App_AckDeviceTestConfig            AckDeviceTestConfig;
    stuD2App_AckDeviceTestingData            AckDeviceTestingData;    //
    stuD2App_AckFirmwareUpdataInfo           AckFirmwareUpdataInfo;	// 
    stuD2App_AckFirmwarePackageData           AckFirmwarePackageData;	//
    stuD2App_AckSlaveFirmwareUpdataInfo       AckSlaveFirmwareUpdataInfo;
    stuD2App_AckSlaveFirmwarePackageData      AckSlaveFirmwarePackageData;
    stuD2App_PushSlaveFirmwareFailInfo        PushSlaveFirmwareFailInfo;
    stuD2App_AckDeviceCtrPowerCommand         AckDeviceCtrPowerCommand;
    stuD2App_AckDevicePowerOffCmd             AckDevicePowerOffCmd;
    stuD2App_WIFIAckFirmwarePackageData       WIFIAckFirmwarePackageData;
    stuD2App_WIFIAckFirmwareUpdataInfo        WIFIAckFirmwareUpdataInfo;
    stuD2App_WIFIAckSlaveFirmwareUpdataInfo   WIFIAckSlaveFirmwareUpdataInfo;
    stuD2App_WIFIAckSlaveFirmwarePackageData  WIFIAckSlaveFirmwarePackageData;
    stuD2App_AckDeviceCleanState              AckDeviceCleanState;
    stuD2App_AckAppRtDatAck                   AckAppRtDatAck;
    stuD2App_AckWifiDisConnectCmd             AckWifiDisConnectCmd;
    stuD2App_AckDeviceFallDischargeCmd        AckDeviceFallDischargeCmd; 
    stuD2App_AckDevAgingInfoCmd             AckDevAgingInfoCmd;
}ENExUpload;

//=======================================================================================
// APP向设备下发命令数据用
typedef union  _ENExReceive
{
    stuApp2D_PushWifiConnectInfo           PushWifiConnectInfo;
    stuApp2D_AskDeviceBaseInfo             AskDeviceBaseInfo;	    // 
    stuApp2D_PushDeviceWorkCommand         PushDeviceWorkCommand;
    stuApp2D_PushDeviceCaliParam           PushDeviceCaliParam;
    stuApp2D_PushUsbWriterSNInfo           PushUsbWriterSNInfo;
    stuApp2D_PushDeviceCaliConfig          PushDeviceCaliConfig;
    stuApp2D_AskDeviceCalingData           AskDeviceCalingData;
    stuApp2D_PushDeviceTestConfig          PushDeviceTestConfig;
    stuApp2D_AskDeviceTestingData          AskDeviceTestingData;
    stuApp2D_PushFirmwareUpdataInfo        PushFirmwareUpdataInfo;	// 
    stuApp2D_PushFirmwarePackageData       PushFirmwarePackageData;	// 
    stuApp2D_PushSlaveFirmwareUpdataInfo   PushSlaveFirmwareUpdataInfo;
    stuApp2D_PushSlaveFirmwarePackageData  PushSlaveFirmwarePackageData;
    stuApp2D_PushDeviceCtrPowerCommand     PushDeviceCtrPowerCommand;
    stuApp2D_PushDevicePowerOffCmd         PushDevicePowerOffCmd;
    stuApp2D_PushWifiDisConnectCmd         PushWifiDisConnectCmd;
    stuApp2D_PushDeviceFallDischargeCmd    PushDeviceFallDischargeCmd;
}ENExReceive;

//===========================================================================================
typedef struct _ENExProtocol
{
	ENExUpload	upload;
	ENExReceive receive;
}ENExProtocol; 




typedef union 
{
	UINT8   cmdAllDataBuffer[MAX_PROTOCOL_LENGTH];
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
		UINT8     DataBuf[MAX_PROTOCOL_LENGTH-MIN_PROTOCOL_LENGTH];/*用户有效数据*/
	}frame;
    #pragma pack(pop)
}WLAN_Protocol_un;  //接收协议数据结构



#endif

typedef struct _NCIsSuccess0_4STU
{
	UINT8	IsSuccessUID01;
	UINT8   IsSuccessUID02;
	UINT8	IsSuccessUID03;
	UINT8   IsSuccessUID04;    
}NCIsSuccess0_4Stu; 








UINT16 NCProtocolRead(ENLoopArray *usartENLoopArray, UINT8 *cmdDataBuffer, UINT16 bufferLength);
//Recive APP Command
void NCExUnMakePushWifiConnectInfo(UINT8 *recbuff,UINT16 Datalength);  
void NCExMakeAckWifiConnectInfo(UINT8 IsSuccess);

void NCExMakeAckDeviceBaseInfo(void);

void  NCExMakePushDeviceRealTimeReport(void);
void  NCExUnMakeAckDeviceRealTimeReport(UINT8 *recbuff);

void NCExMakeAckDeviceWorkCommand(UINT8 TargetUid,UINT8 DeviceWorkState,UINT8 IsSuccess);
void NCExUnMakePushDeviceWorkCommand(UINT8 *recbuff,UINT16 datalength);

void NCExMakeAckDeviceCtrPowerCommand(UINT8 TargetUid,UINT8 DeviceCtrState,UINT8 IsSuccess);
void NCExUnMakePushDeviceCtrPowerCommand(UINT8 *recbuff);

void NCExMakeAckDevicePowerOffCmd(UINT8 DeviceCtrState,UINT8 IsSuccess);
void NCExUnMakePushDevicePowerOffCmd(UINT8 *recbuff);

void  NCExMakeAckDeviceCaliParam(UINT8    TargetUid,NC_CALI_ACK_TYPE IsSuccess,struct _NCMSComProtocol *MSComUnionProtocol);
void NCExUnMakePushDeviceCaliParam(UINT8 *recbuff);
void  NCExMakeAckUpdataSnInfo(NC_COMM_ACK_TYPE IsSuccess);
void  NCExUnMakePushWitreSN(UINT8 *recbuff,UINT16 Len);
void NCExMakeAckDeviceCaliConfig(UINT8  TargetUid,UINT8 SettingType,UINT8 IsSuccess);
void  NCExUnMakePushDeviceCaliConfig(UINT8 *recbuff);
void  NCExMakeAckDeviceCalingData(UINT8    TargetUid,CALI_TEST_DATA_STATE_TYPE IsSuccess);
void  NCExUnMakeAskDeviceCalingData(UINT8 *recbuff);
void NCExMakeAckDeviceTestConfig(UINT8  TargetUid,UINT8 SettingType,UINT8 IsSuccess);
void  NCExUnMakePushDeviceTestConfig(UINT8 *recbuff);
void  NCExMakeAckDeviceTestingData(UINT8    TargetUid,CALI_TEST_DATA_STATE_TYPE IsSuccess);
void  NCExUnMakeAskDeviceTestingData(UINT8 *recbuff);

void NCExMakeAckFirmwareUpdataInfo(UINT8 DeviceAck);
void NCExUnMakePushFirmwareUpdataInfo(UINT8 *recbuff);
void NCExMakeAckFirmwarePackageData(UINT8 PackageIndex,UINT8 DeviceAck);
void NCExUnMakePushFirmwarePackageData(UINT8 *recbuff,UINT16 Datalength);

void NCExMakeSlaveAckFirmwareUpdataInfo(UINT8  TargetUid,UINT8 DeviceAck);
void NCExUnMakeSlavePushFirmwareUpdataInfo(UINT8 *recbuff);
void NCExUnMakeSlavePushFirmwarePackageData(UINT8 *recbuff,UINT16 Datalength);
void NCExMakeSlaveAckFirmwarePackageData(UINT8  TargetUid,UINT8 PackageIndex,UINT8 DeviceAck);

void  NCExUnMakePushFirmwareUpdataInfo(UINT8 *recbuff);
void  NCExUnMakePushFirmwarePackageData(UINT8 *recbuff,UINT16 Datalength);

void NCExUnMakePushDeviceCleanState(UINT8 *recbuff);

void NCExUnMakePushWifiDisConnectCmd(void);
void NCExUnMakePushDeviceFallDischargeCmd(UINT8 *recbuff,UINT16 datalength);

void NCExUnMakeAskDevAgingInfoCmd(void);


#endif // __NCWLANPROTOCOL_H_



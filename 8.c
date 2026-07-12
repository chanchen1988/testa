/*****************************************************************
 * File: NCWlanDataHandle.c
 * Date: 2021/12/22 21:02
 *
 * Note:
 *
*****************************************************************/
#include "NCWlanDataHandle.h"
#include "NCWlanProtocol.h"
#include "ENLoopArray.h"
#include "apm32f103_conf.h"
#include "NCWlan.h"
#include "NCUsersetting.h"
#include "FreeRTOS.h"
#include "task.h" 
#include "NCOS.h" 
#include <string.h>
#include "stdio.h"
#include "NCFirmwareUpdateHandle.h"


#define DALEY_TIME                                     1000 
#define WLAN_SEND_BUF_SIZE                             (700)//(1024)
#define WLAN_REC_BUF_SIZE                              (1500)//(1024)//(600)

extern xSemaphoreHandle gMutexWlanPackage; 


volatile    UINT8             WlansendBuffer[WLAN_SEND_BUF_SIZE];   
volatile    UINT8             WlanrecvBuffer[WLAN_REC_BUF_SIZE];
volatile    WLAN_Protocol_un  Wlan_ProtocolData;
//volatile    UINT8       WlancmdWlanAllDataBuffer[MAX_PROTOCOL_LENGTH];


typedef struct _NCWlanDataStr
{
    ENLoopArray  sendLoop;
    ENLoopArray  recvLoop;
    UINT8        flagTaskWlanConfigChange;   //Wlan配置修改标志位
}NCWlanDataStr;

//static NCWlanDataStr gNCWlanData; 
//0x24000000 为ADC 16字节
static  NCWlanDataStr   gNCWlanData;


//static NCWlanDataStr gNCWlanData  __attribute__((section(".ARM.__at_0x30000100")));

// 获取WIFI配置是否修改
//static UINT8 NCWlanGetTaskBleConfigChange(void)
//{
//    return gNCWlanData.flgTaskWlanConfigChange;
//}

// WIFI配置标识清除
//static void NCWlanClearTaskBleConfig(void)
//{
//    gNCWlanData.flgTaskWlanConfigChange = 0;
//}
//=====================================================
void NCWlanDataInit(void)
{
    memset(&gNCWlanData, 0, sizeof(NCWlanDataStr));
    memset((UINT8 *)WlansendBuffer, 0, sizeof(WlansendBuffer));
    memset((UINT8 *)WlanrecvBuffer, 0, sizeof(WlanrecvBuffer));
    memset((UINT8 *)Wlan_ProtocolData.cmdAllDataBuffer, 0, sizeof(Wlan_ProtocolData.cmdAllDataBuffer));
    
    ENLoopArrayInit(&gNCWlanData.sendLoop, (UINT8	*)WlansendBuffer, sizeof(WlansendBuffer));
    ENLoopArrayInit(&gNCWlanData.recvLoop, (UINT8	*)WlanrecvBuffer, sizeof(WlanrecvBuffer));
    NCWlanInitBufferConfig(&gNCWlanData.sendLoop, &gNCWlanData.recvLoop); 

    NCSemaphoreCreateMutex(&gMutexWlanPackage); //创建信号量    
}

#if  1
//将要发送的数据传入循环数组
//sendLength 位数据长度
void NCInfoWlanParamSet(NC_PROTOCOL_CMD_FOR_WLAN cmdSend, UINT8 *sendDatabuf, UINT16 sendLength)
{
    UINT8  TimeOut=255,checkSumSend = 0;
    UINT16 checkCount, dataTxCount;
    UINT8  cmdSend_L,cmdSend_H, sendLength_H, sendLength_L;
    if(NCWlanConfigFinish()==FALSE) return; 

    cmdSend_L = cmdSend&0xFF;
    cmdSend_H = cmdSend>>8;
    sendLength_L = sendLength&0xFF;
    sendLength_H = sendLength>>8;
    
    for(checkCount = 0; checkCount < sendLength; checkCount++)
    {
        checkSumSend += sendDatabuf[checkCount];
    }
    checkSumSend += (HEAD_SIGN_CMD  + HEAD_SIGN_CMD_H + cmdSend_L + cmdSend_H + sendLength_H + sendLength_L);
    while(NCWlanIsSending()==TRUE) //vTaskDelay(10); //数据未发送完毕先不装载数据
    {
        vTaskDelay(1);
    }
    
    //=============================================================== 
    do 
    {        
        dataTxCount = ENLoopArrayGetRemainNum(&gNCWlanData.sendLoop);
        if(dataTxCount<sendLength)
        {
            vTaskDelay(1);
            if(--TimeOut==0)
            {
               //printf("NCInfoWlanParamSet RemainNum Small Wait TimeOut"); 
               return; 
            }
        }
        else
        {
           break;
        }
    }
    while(1);
    //===========================================================
    // 将要发送的数据传入循环数组
    ENLoopArrayIn(&gNCWlanData.sendLoop, HEAD_SIGN_CMD);
    ENLoopArrayIn(&gNCWlanData.sendLoop, HEAD_SIGN_CMD_H);
    ENLoopArrayIn(&gNCWlanData.sendLoop, cmdSend_L);
    ENLoopArrayIn(&gNCWlanData.sendLoop, cmdSend_H);
    ENLoopArrayIn(&gNCWlanData.sendLoop, sendLength_L); 
    ENLoopArrayIn(&gNCWlanData.sendLoop, sendLength_H); 
    ENLoopArrayIn(&gNCWlanData.sendLoop, 0x00); //CMD_TX_RESERVE_1 
    ENLoopArrayIn(&gNCWlanData.sendLoop, 0x00); //CMD_TX_RESERVE_2 
    ENLoopArrayIn(&gNCWlanData.sendLoop, checkSumSend);
    for(dataTxCount = 0; dataTxCount < sendLength; dataTxCount++)
    {
        ENLoopArrayIn(&gNCWlanData.sendLoop, sendDatabuf[dataTxCount]);
    } 
}



//==========================================================================
//==========================================================================
//WIFI模块协议解析
void NCWlanProtocolParse(NC_PROTOCOL_CMD_FOR_WLAN cmd, UINT8 *databuf, UINT16 datalength)
{
    NCUserSetting* userSetting = NCUserSettingGet();
    NCSlaveFirmwareStu *  NCSlaveFirmwareInfo = NCSlaveFirmwareInfoGet();
    NCFirmwarePackageStr * firmwarePackage    = NCFirmwarePackageGet();    
    //userSetting->WifiNoRecvTimes              = COM_NO_RECV_TIMES_MAX;    
    switch(cmd)
    {
        case g_eApp2D_PushWifiConnectInfo:
        {    
            //userSetting->WifiNoRecvTimes              = COM_NO_RECV_TIMES_MAX; 这里不能初始化该值           
            if(NC_WORK_MODE_IDLE == userSetting->DeviceWorkModeType && FALSE== userSetting->AP_NetInfo.IsNetCommunicating)//为空闲才可以配网   
            {                
               NCExUnMakePushWifiConnectInfo(databuf,datalength);
            }
            else
            {
                //不符合配网的条件 关闭BLE
                NCExMakeAckWifiConnectInfo(NC_COMM_ACK_FL);  
                vTaskDelay(1500);
                if(userSetting->AP_NetInfo.IsBluetoothEnable)
                {
                    userSetting->AP_NetInfo.IsColseBleCofig = ENABLE;
                }                
            }            
            break;
        }
        case g_eApp2D_PushWifiDisConnectCmd:
        {
            userSetting->WifiNoRecvTimes              = COM_NO_RECV_TIMES_MAX;
            NCExUnMakePushWifiDisConnectCmd();
            break;
        }
        case g_eApp2D_PushFirmwareUpdataInfo:            
        {
            userSetting->WifiNoRecvTimes              = COM_NO_RECV_TIMES_MAX;
            #ifdef IS_LOG_EN
            //printf(" WIFI PushFirmwareUpdataInfo \r\n");
            #endif
             userSetting->AP_NetInfo.IsNetCommunicating   = TRUE;
            if(FALSE==NCSlaveFirmwareInfo->IsSlaveUpdataFirm)
               NCExUnMakePushFirmwareUpdataInfo(databuf);
            else
              NCExMakeAckFirmwareUpdataInfo(FIRMWARE_SLAVE_UPDATAING);
           break;
        }            
        case g_eApp2D_PushFirmwarePackageData:
        {
            userSetting->WifiNoRecvTimes              = COM_NO_RECV_TIMES_MAX;
            userSetting->AP_NetInfo.IsNetCommunicating   = TRUE;
            if(FALSE==NCSlaveFirmwareInfo->IsSlaveUpdataFirm)
               NCExUnMakePushFirmwarePackageData(databuf,datalength);
            else
              NCExMakeAckFirmwarePackageData(firmwarePackage->FirmwarePackageIdenx,FIRMWARE_OTHER_UPDATAING);  
            break;
        }
        case g_eApp2D_PushSlaveFirmwareUpdataInfo:
        {
            userSetting->WifiNoRecvTimes              = COM_NO_RECV_TIMES_MAX;
            userSetting->AP_NetInfo.IsNetCommunicating   = TRUE;
            if(FALSE==firmwarePackage->IsFirmwareUpdataRequest)
               NCExUnMakeSlavePushFirmwareUpdataInfo(databuf);
            else
              NCExMakeSlaveAckFirmwareUpdataInfo(databuf[0],FIRMWARE_MASTER_UPDATAING);
                
            break;
        } 
        case g_eApp2D_PushSlaveFirmwarePackageData:
        {
            userSetting->WifiNoRecvTimes              = COM_NO_RECV_TIMES_MAX;
            userSetting->AP_NetInfo.IsNetCommunicating   = TRUE;
            if(FALSE==firmwarePackage->IsFirmwareUpdataRequest)
              NCExUnMakeSlavePushFirmwarePackageData(databuf,datalength);
            else 
               NCExMakeSlaveAckFirmwarePackageData(databuf[0],0x00,FIRMWARE_OTHER_UPDATAING);  
                
            break;  
        } 
        //=========================================
        case g_eApp2D_AskDeviceBaseInfo:
        {
            userSetting->WifiNoRecvTimes              = COM_NO_RECV_TIMES_MAX;
            userSetting->AP_NetInfo.IsNetCommunicating   = TRUE;
            NCExMakeAckDeviceBaseInfo();
            break;
        }         
        case g_eApp2D_AckDeviceRealTimeReport:
        {    
            userSetting->WifiNoRecvTimes              = COM_NO_RECV_TIMES_MAX;
            userSetting->AP_NetInfo.IsNetCommunicating   = TRUE;
            NCExUnMakeAckDeviceRealTimeReport(databuf);
            break;
        }  
        case g_eApp2D_PushDeviceWorkCommand:
        {
            userSetting->WifiNoRecvTimes              = COM_NO_RECV_TIMES_MAX;
            userSetting->AP_NetInfo.IsNetCommunicating   = TRUE;
            NCExUnMakePushDeviceWorkCommand(databuf,datalength);
            break;
        }
        case g_eApp2D_PushDeviceCleanState:
        {
            userSetting->WifiNoRecvTimes              = COM_NO_RECV_TIMES_MAX;
           userSetting->AP_NetInfo.IsNetCommunicating   = TRUE;
            NCExUnMakePushDeviceCleanState(databuf);
           break;
        }            
        case g_eApp2D_PushDeviceCtrPowerCommand:
        {
            userSetting->WifiNoRecvTimes              = COM_NO_RECV_TIMES_MAX;
            userSetting->AP_NetInfo.IsNetCommunicating   = TRUE;
            NCExUnMakePushDeviceCtrPowerCommand(databuf);
            break;
        }  
        case g_eApp2D_PushDevicePowerOffCmd:
        {
            userSetting->WifiNoRecvTimes                 = COM_NO_RECV_TIMES_MAX;
            userSetting->AP_NetInfo.IsNetCommunicating   = TRUE;
            NCExUnMakePushDevicePowerOffCmd(databuf);
            break;
        }
        case g_eApp2D_PushDeviceFallDischargeCmd:
        {
            userSetting->WifiNoRecvTimes                 = COM_NO_RECV_TIMES_MAX;
            userSetting->AP_NetInfo.IsNetCommunicating   = TRUE;            
            NCExUnMakePushDeviceFallDischargeCmd(databuf,datalength);  
            break;        
        } 
        case g_eApp2D_AskDevAgingInfoCmd:
        {
            userSetting->WifiNoRecvTimes                 = COM_NO_RECV_TIMES_MAX;
            userSetting->AP_NetInfo.IsNetCommunicating   = TRUE;            
            NCExUnMakeAskDevAgingInfoCmd();  
            break;                
        }            
        default:
            break;            
    }
}

//================================================
//WIFI模块协议解析进程
BOOL NCWlanProtocolParseProcess(void) 
{
	UINT16 cmdLength;
	//UINT16 cmdCMD = 0;
    ENLoopArray * RecvLoop = getNCWlan_RecvLoopPtr();
	//NCUserSetting* userSetting = NCUserSettingGet();            
    //memset((UINT8 *)Wlan_ProtocolData.cmdAllDataBuffer, 0, 40/*sizeof(Wlan_ProtocolData.cmdAllDataBuffer)*/);//大部分指令都在40字节以内 放入函数内部
	cmdLength = NCProtocolRead(RecvLoop, (UINT8 *)Wlan_ProtocolData.cmdAllDataBuffer, sizeof(Wlan_ProtocolData.cmdAllDataBuffer));
    //cmdLength = NCProtocolRead((UINT8 *)WlancmdWlanAllDataBuffer, sizeof(WlancmdWlanAllDataBuffer));
	if(cmdLength != ERROR_CODE)
	{
	    NCWlanProtocolParse((NC_PROTOCOL_CMD_FOR_WLAN)Wlan_ProtocolData.frame.cmd, (UINT8 *)Wlan_ProtocolData.frame.DataBuf, (cmdLength));
        return TRUE;
	}
    else
        return FALSE;  
}


//================================================================
//================================================================
void NCWlanDataSendProcess(void)
{
    NCUserSetting* userSetting = NCUserSettingGet();
    // 本机WIFI配置更改生效--使用AT指令发给WIFI模块
    //ProcessWlanConfigChange();  
    // 定时查询连接情况及信号强度
    // 启动发送   
    //NetworkCommInfoStu * NetworkInfo =  GetNCNetworkInfo();
    
    if(/*TRUE== NetworkInfo->IsAppConnectDevice &&*/ NCWlanConfigFinish() == TRUE && gNCWlanData.sendLoop.number)       
    {
        NCWlan_StartSend();
    }
    else
    {
        // //防错处理
       // if(FALSE== NetworkInfo->IsAppConnectDevice && NCWlanIsSending())
       // {
           // NCWlanDisableTransmit();
       // }
    }
}

//====================================================================

void NCWlanDataParseProcess(void)
{
     NCUserSetting* userSetting = NCUserSettingGet();
 	if(TRUE == NCWlanConfigFinish() && FALSE== userSetting->AP_NetInfo.IsNetConfigEnable && userSetting->AP_NetInfo.IsBleWaitConfig == FALSE)
	{
		while(NCWlanProtocolParseProcess() == TRUE);//Wait All Recive Data Pack Unmake                       
	}	
    //else
    //{
    //   printf("NCWlanConfigFinish No");
    //}
}

#endif



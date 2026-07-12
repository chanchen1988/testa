#include "NCWlanProtocol.h"
#include "NCUserSetting.h"
#include "NCDeviceManage.h"
#include "stdio.h"
#include "string.h"
#include "NCWlanDataHandle.h"
#include "NCOS.h" 
#include "NCWlan.h"
#include "NCFirmwareUpdateHandle.h"
#include "NCMSComProtocol.h"
#include "NCBalance_Adc.h" 
#include "NCBalance_Contrl.h" 
#include "NCPacketRetransmitHandle.h"
#include "NCPowerManage.h"
#include "NCBalance_Contrl.h"
#include "NCFan_Contrl.h" 
//#include "NCDomainParam.h"
//#include "NCDoPowerManage.h"

#define ONCE_READ_LENGHT		(80)

#pragma pack(push) //保存对齐状态
#pragma pack(1)//设定为1字节对齐

#pragma pack(pop)//恢复对齐状态

struct _ENExProtocol  gExProtocol; 

xSemaphoreHandle gMutexWlanPackage = NULL;

#define      LENGTH_ERR_TMR     20//20*10 = 200ms

/*********************************************************************************************************
** Function name(函数名称):		    NCProtocolRead()
**
** Descriptions（描述）:				协议读取
**
** input parameters（输入参数）:	
** Returned value（返回值）:			None
**         
** Used global variables（全局变量）:	None
** Calling modules（调用模块）:			None
**
** Created by（创建人）:				
** Created Date（创建日期）:			2021-12-22
**-------------------------------------------------------------------------------------------------------
**------------------------------------------------------------------------------------------------------
********************************************************************************************************/
//Head + datalength + cmd + data ……;(data 0--n) + checksum
//2Byte+ 2Byte      + 2Byte	+(0--n)Byte	         +1Byte	
UINT16 NCProtocolRead(ENLoopArray *usartENLoopArray, UINT8 *cmdDataBuffer, UINT16 bufferLength)
{
	static UINT8 LengthErr = LENGTH_ERR_TMR;//200ms
	UINT16 peekArrayNum, calSumCount, copyENLoopArray, dataLength = 0;
	UINT8 calCheckSum, dataLen_H, dataLen_L, checkSum; 
	peekArrayNum = calSumCount = copyENLoopArray = 0;
	calCheckSum  = dataLen_H = dataLen_L= checkSum = 0;
	if(usartENLoopArray->number < MIN_PROTOCOL_LENGTH)
		return ERROR_CODE;
	
	for(peekArrayNum = 0; peekArrayNum < usartENLoopArray->number; peekArrayNum++)	//找到帧头
	{
		if((ENLoopArrayPeek(usartENLoopArray, CMD_HEAD) == HEAD_SIGN_CMD))    
        {
            if((ENLoopArrayPeek(usartENLoopArray, CMD_HEAD + CMD_HEAD_H) == HEAD_SIGN_CMD_H)) 
            {
                break;
            }
           else ENLoopArrayOut(usartENLoopArray);           
        }            			
		else ENLoopArrayOut(usartENLoopArray);

		if(peekArrayNum == usartENLoopArray->number - 1)
			return ERROR_CODE;
	}
    dataLen_L = ENLoopArrayPeek(usartENLoopArray, (CMD_HEAD + CMD_DATALENGTH_L));
	dataLen_H = ENLoopArrayPeek(usartENLoopArray, (CMD_HEAD + CMD_DATALENGTH_H));
	dataLength = dataLen_H;
	dataLength <<= 8;
	dataLength += dataLen_L;
	if(dataLength > MAX_PROTOCOL_LENGTH)
	{
		LengthErr = LENGTH_ERR_TMR;
		ENLoopArrayOut(usartENLoopArray);
        ENLoopArrayOut(usartENLoopArray);
        #ifdef    IS_LOG_EN
        //printf("WIFI dataLength Err");
        #endif  
		return ERROR_CODE;
	}
	if(usartENLoopArray->number < (CMD_HEAD + dataLength + MIN_PROTOCOL_LENGTH))  //数据尚未接受完毕退出等待
	{
	    if(--LengthErr == 0)
        {
            LengthErr = LENGTH_ERR_TMR;
			ENLoopArrayOut(usartENLoopArray);
            ENLoopArrayOut(usartENLoopArray);
	    }
		return ERROR_CODE;
	}	
    //
	LengthErr = LENGTH_ERR_TMR;
	for(calSumCount = 0; calSumCount < (dataLength + MIN_PROTOCOL_LENGTH); calSumCount++)
	{
		if(calSumCount != CMD_CHECKSUM)
		{
			calCheckSum += ENLoopArrayPeek(usartENLoopArray, CMD_HEAD + calSumCount);
		}
	}
	checkSum = ENLoopArrayPeek(usartENLoopArray, CMD_HEAD + CMD_CHECKSUM);
	if(checkSum != calCheckSum)//校验不对 丢掉头码
	{
		ENLoopArrayOut(usartENLoopArray);
        ENLoopArrayOut(usartENLoopArray);
        #ifdef    IS_LOG_EN
        //printf("WIFI checkSum Err");
        #endif  
		return ERROR_CODE;
	}
	
	if(bufferLength < (dataLength + MIN_PROTOCOL_LENGTH))
	{
		ENLoopArrayOut(usartENLoopArray);
        ENLoopArrayOut(usartENLoopArray);
        #ifdef    IS_LOG_EN
        //printf("WIFI cmdDataBuffer Too Small");
        #endif        
		return ERROR_CODE;
	}
    memset((cmdDataBuffer+MIN_PROTOCOL_LENGTH), 0, 30/*sizeof(Wlan_ProtocolData.cmdAllDataBuffer)*/);//大部分指令都在40字节以内	
	for(copyENLoopArray = 0; copyENLoopArray < (dataLength+MIN_PROTOCOL_LENGTH); copyENLoopArray++)
	{
		cmdDataBuffer[copyENLoopArray] = ENLoopArrayOut(usartENLoopArray);
	}
	//dataLength += MIN_PROTOCOL_LENGTH;
	return (dataLength);
}
//===============================================
/*
    1.断开DMA 结束当前传输
    2.清除发送缓冲区BUF
*/
void NCWlanDisableTransmit(void)
{
    // NCUserSetting* userSetting = NCUserSettingGet();
    // NetworkCommInfoStu * NetworkInfo =  GetNCNetworkInfo();
    // NCWlan_DMA_Disable();
    // userSetting->WifiConnectIng   = FALSE;
    // userSetting->OscIsDataReStart = FALSE;  
    // userSetting->OscIsSendWave    = FALSE;
    // userSetting->DmmIsSendWave    = FALSE;
    // NetworkInfo->IsAppConnectDevice = FALSE;
}


//========================================================================
//配网解析段
void NCExMakeAckWifiConnectInfo(UINT8 IsSuccess)        
{  
    if(FALSE == NCSemaphoreTake(gMutexWlanPackage)) return;
    gExProtocol.upload.AckWifiConnectInfo.IsSuccess =   IsSuccess;
	//把数据发送出去
    NCInfoWlanParamSet(g_eD2App_AckWifiConnectInfo, (UINT8*)&gExProtocol.upload, sizeof(stuD2App_AckWifiConnectInfo));   
    NCSemaphoreGive(gMutexWlanPackage);     
}
//========================================================================

void NCExUnMakePushWifiConnectInfo(UINT8 *recbuff,UINT16 Datalength)        
{  
    NCUserSetting* userSetting = NCUserSettingGet();
    if(Datalength<70)
    {
       NCExMakeAckWifiConnectInfo(NC_COMM_ACK_FL);
    }
    else
    {
        memcpy(&gExProtocol.receive.PushWifiConnectInfo, recbuff, sizeof(stuApp2D_PushWifiConnectInfo)); 
        if(gExProtocol.receive.PushWifiConnectInfo.Server_PORT>1023)
        {
          //gExProtocol.receive.PushWifiConnectInfo.Server_PORT = 60000;
            userSetting->AP_NetInfo.IsNetConnect        = FALSE;
            userSetting->AP_NetInfo.IsNetConfigComplete = FALSE;
            userSetting->AP_NetInfo.IsNetCommunicating   = FALSE; 
            
            userSetting->AP_NetInfo.Server_PORT         = gExProtocol.receive.PushWifiConnectInfo.Server_PORT;
            
            memset(userSetting->AP_NetInfo.AP_DeviceSN,0,sizeof(userSetting->AP_NetInfo.AP_DeviceSN));
            memcpy(userSetting->AP_NetInfo.AP_DeviceSN,gExProtocol.receive.PushWifiConnectInfo.AP_DeviceSN,sizeof(userSetting->AP_NetInfo.AP_DeviceSN));
            
            memset(userSetting->AP_NetInfo.AP_Password,0,sizeof(userSetting->AP_NetInfo.AP_Password));
            memcpy(userSetting->AP_NetInfo.AP_Password,gExProtocol.receive.PushWifiConnectInfo.AP_Password,sizeof(userSetting->AP_NetInfo.AP_Password));
            
            memset(userSetting->AP_NetInfo.AP_SSID,0,sizeof(userSetting->AP_NetInfo.AP_SSID));
            memcpy(userSetting->AP_NetInfo.AP_SSID,gExProtocol.receive.PushWifiConnectInfo.AP_SSID,sizeof(userSetting->AP_NetInfo.AP_SSID));
            
            memset(userSetting->AP_NetInfo.Server_IP,0,sizeof(userSetting->AP_NetInfo.Server_IP));
            memcpy(userSetting->AP_NetInfo.Server_IP,gExProtocol.receive.PushWifiConnectInfo.Server_IP,sizeof(userSetting->AP_NetInfo.Server_IP));
            //memcpy(userSetting->AP_NetInfo.Server_IP,"192.168.1.6",sizeof(userSetting->AP_NetInfo.Server_IP));
            
            DoMianWifiSetRTMRGap(3);
            NCWlan_TxBuffer_Deinit();
            userSetting->AutoPowerOFF_Cnt =  AUTO_POWEROFF_TIME;  
            NCExMakeAckWifiConnectInfo(NC_COMM_ACK_OK);
            userSetting->Com2NoRecvTimes = SLAVE_NO_RECV_TIMES_MAX;
            userSetting->Com3NoRecvTimes = SLAVE_NO_RECV_TIMES_MAX;
            userSetting->Com4NoRecvTimes = SLAVE_NO_RECV_TIMES_MAX;
            vTaskDelay(2000);//预留时间给蓝牙断开
            //
            #ifdef IS_LOG_EN
    //        printf("\r\n");
    //        printf("Server_PORT = %d\r\n",userSetting->AP_NetInfo.Server_PORT);
    //        printf("AP_DeviceSN = %s\r\n",userSetting->AP_NetInfo.AP_DeviceSN);
    //        printf("AP_SSID = %s\r\n",userSetting->AP_NetInfo.AP_SSID);
    //        printf("AP_Password = %s\r\n",userSetting->AP_NetInfo.AP_Password);
    //        printf("Server_IP = %s\r\n",userSetting->AP_NetInfo.Server_IP);
            #endif             
            //NCExMakeAckWifiConnectInfo(NC_COMM_ACK_OK);            
            //CTR_COM_LED_ON;//亮下
            //vTaskDelay(500);//延时等待 应答信息发送完成   
            //CTR_COM_LED_ON;//亮下
            userSetting->AP_NetInfo.IsNetConfigEnable   = ENABLE;        
        }        
       else
       {
           NCExMakeAckWifiConnectInfo(NC_COMM_ACK_FL);
       }
    }       
}

//========================================================================
//========================================================================
//断开网络连接
void NCExMakeAckWifiDisConnectCmd(UINT8 IsSuccess)        
{  
    NCUserSetting* userSetting = NCUserSettingGet();
    if(FALSE == NCSemaphoreTake(gMutexWlanPackage)) return;
    
    memcpy(&(gExProtocol.upload.AckWifiDisConnectCmd.DeviceSN), userSetting->deviceSN,sizeof(userSetting->deviceSN));  
    gExProtocol.upload.AckWifiDisConnectCmd.DeviceAckInfo =   IsSuccess;
	//把数据发送出去
    NCInfoWlanParamSet(g_eD2App_AckWifiDisConnectCmd, (UINT8*)&gExProtocol.upload, sizeof(stuD2App_AckWifiDisConnectCmd));   
    NCSemaphoreGive(gMutexWlanPackage);     
}
//========================================================================

void NCExUnMakePushWifiDisConnectCmd(void)        
{  
    NCUserSetting* userSetting = NCUserSettingGet();
    
    NCExMakeAckWifiDisConnectCmd(0x01);
    vTaskDelay(900);//确保应答指令可以发出
    
    //重新配置不可能连接网络上去
    memset(userSetting->AP_NetInfo.AP_Password,0,sizeof(userSetting->AP_NetInfo.AP_Password));
    snprintf(userSetting->AP_NetInfo.AP_Password,sizeof(userSetting->AP_NetInfo.AP_Password),"SumQt_xx");
    
    memset(userSetting->AP_NetInfo.AP_SSID,0,sizeof(userSetting->AP_NetInfo.AP_SSID));
    snprintf(userSetting->AP_NetInfo.AP_SSID,sizeof(userSetting->AP_NetInfo.AP_SSID),"SummerQtSs");
    
    //memset(userSetting->AP_NetInfo.Server_IP,0,sizeof(userSetting->AP_NetInfo.Server_IP));
    
    
    userSetting->AP_NetInfo.IsNetConnect        = FALSE;
    userSetting->AP_NetInfo.IsNetConfigComplete = FALSE;
    userSetting->AP_NetInfo.IsNetCommunicating   = FALSE; 
                            
    userSetting->AP_NetInfo.IsNetConfigEnable   = ENABLE;    
    userSetting->AP_NetInfo.IsBleWaitConfig     = FALSE;    
}

//========================================================================
//App获取设备基本信息
void NCExMakeAckDeviceBaseInfo(void)
{
    NCUserSetting* userSetting = NCUserSettingGet();
    
    if(FALSE == NCSemaphoreTake(gMutexWlanPackage)) return;
    memset(&(gExProtocol.upload.AckDeviceBaseInfo),0,sizeof(stuD2App_AckDeviceBaseInfo));
   
    memcpy(&(gExProtocol.upload.AckDeviceBaseInfo.DeviceSN),        userSetting->deviceSN,      sizeof(userSetting->deviceSN));  
    memcpy(&(gExProtocol.upload.AckDeviceBaseInfo.SoftVersionD1),   userSetting->SoftVersionD1, sizeof(userSetting->SoftVersionD1));  
    memcpy(&(gExProtocol.upload.AckDeviceBaseInfo.SoftVersionD2),   userSetting->SoftVersionD2, sizeof(userSetting->SoftVersionD2));
    memcpy(&(gExProtocol.upload.AckDeviceBaseInfo.SoftVersionD3),   userSetting->SoftVersionD3, sizeof(userSetting->SoftVersionD3));
    memcpy(&(gExProtocol.upload.AckDeviceBaseInfo.SoftVersionD4),   userSetting->SoftVersionD4, sizeof(userSetting->SoftVersionD4));
    
    gExProtocol.upload.AckDeviceBaseInfo.HardwareVersionD1 =   userSetting->HardwareVersion;
//    gExProtocol.upload.AckDeviceBaseInfo.HardwareVersionD2 =   userSetting->HardwareVersionD2;
//    gExProtocol.upload.AckDeviceBaseInfo.HardwareVersionD3 =   userSetting->HardwareVersionD3;
//    gExProtocol.upload.AckDeviceBaseInfo.HardwareVersionD4 =   userSetting->HardwareVersionD4;
	//把数据发送出去
    NCInfoWlanParamSet(g_eD2App_AckDeviceBaseInfo, (UINT8*)&gExProtocol.upload, sizeof(stuD2App_AckDeviceBaseInfo));   
    NCSemaphoreGive(gMutexWlanPackage);    
}

//===============================================================================
//===============================================================================
void  NCExMakePushDeviceRealTimeReport(void) 
{
    NCUserSetting* userSetting = NCUserSettingGet();
    UINT8 TCellStateData;
    if(FALSE == NCSemaphoreTake(gMutexWlanPackage)) return;
    
    memcpy(&(gExProtocol.upload.PushDeviceRealTimeReport.DeviceSN), userSetting->deviceSN,sizeof(userSetting->deviceSN));  
    
    gExProtocol.upload.PushDeviceRealTimeReport.FinAlarmState       = userSetting->FinAlarmState; 
    
    gExProtocol.upload.PushDeviceRealTimeReport.CellVoltageD1       = gCh1ManageCtr.CurrentDispVolt;
    gExProtocol.upload.PushDeviceRealTimeReport.CurrentRL_VoltD1    = gCh1ManageCtr.CurrentRL_Volt;
    gExProtocol.upload.PushDeviceRealTimeReport.CellCurrentD1       = gCh1ManageCtr.CurrentDispCurrent;
    TCellStateData = gCh1ManageCtr.CellStateData;
    
    if(TCellStateData == NC_DISCING_STATE)
    {
        if(userSetting->DeviceCtrlCmdState == NC_CTRL_CMD_NFINISH_FIXC) 
        {
            TCellStateData = NC_DISCING_FIXC_STATE;
        }
        else if(userSetting->DeviceCtrlCmdState == NC_CTRL_CMD_QFINISH) 
        {
            TCellStateData = NC_DISCING_QF_STATE;
        }
    }
    else if(TCellStateData == NC_DISCING_FINISH_STATE)
    {
        if(userSetting->DeviceCtrlCmdState == NC_CTRL_CMD_QFINISH)
        {
            TCellStateData = NC_DISCING_QF_FINISH_STATE;
        }
    }
    gExProtocol.upload.PushDeviceRealTimeReport.CellStateDataD1     = TCellStateData;//gCh1ManageCtr.CellStateData;
    gExProtocol.upload.PushDeviceRealTimeReport.DeviceAlarmStateD1  = gCh1ManageCtr.DeviceAlarmState;
    gExProtocol.upload.PushDeviceRealTimeReport.CellCapacityD1      = gCh1ManageCtr.CellCapacityAH;//容量
    //gExProtocol.upload.PushDeviceRealTimeReport.CellCapacityWhD1    = gCh1ManageCtr.CellCapacityWH;//容量
    
    gExProtocol.upload.PushDeviceRealTimeReport.CellVoltageD2       = gCh2ManageCtr.CurrentDispVolt;
    gExProtocol.upload.PushDeviceRealTimeReport.CurrentRL_VoltD2    = gCh2ManageCtr.CurrentRL_Volt;
    gExProtocol.upload.PushDeviceRealTimeReport.CellCurrentD2       = gCh2ManageCtr.CurrentDispCurrent;
    TCellStateData = gCh2ManageCtr.CellStateData;
    
    if(TCellStateData == NC_DISCING_STATE)
    {
        if(userSetting->DeviceCtrlCmdState == NC_CTRL_CMD_NFINISH_FIXC) 
        {
            TCellStateData = NC_DISCING_FIXC_STATE;
        }
        else if(userSetting->DeviceCtrlCmdState == NC_CTRL_CMD_QFINISH) 
        {
            TCellStateData = NC_DISCING_QF_STATE;
        }
    }
    else if(TCellStateData == NC_DISCING_FINISH_STATE)
    {
        if(userSetting->DeviceCtrlCmdState == NC_CTRL_CMD_QFINISH)
        {
            TCellStateData = NC_DISCING_QF_FINISH_STATE;
        }
    }
    gExProtocol.upload.PushDeviceRealTimeReport.CellStateDataD2     = TCellStateData;//gCh2ManageCtr.CellStateData;
    gExProtocol.upload.PushDeviceRealTimeReport.DeviceAlarmStateD2  = gCh2ManageCtr.DeviceAlarmState;
    gExProtocol.upload.PushDeviceRealTimeReport.CellCapacityD2      = gCh2ManageCtr.CellCapacityAH;//容量
    //gExProtocol.upload.PushDeviceRealTimeReport.CellCapacityWhD2    = gCh2ManageCtr.CellCapacityWH;
    
    gExProtocol.upload.PushDeviceRealTimeReport.CellVoltageD3       = gCh3ManageCtr.CurrentDispVolt;
    gExProtocol.upload.PushDeviceRealTimeReport.CurrentRL_VoltD3    = gCh3ManageCtr.CurrentRL_Volt;
    gExProtocol.upload.PushDeviceRealTimeReport.CellCurrentD3       = gCh3ManageCtr.CurrentDispCurrent;
    TCellStateData = gCh3ManageCtr.CellStateData;
    
    if(TCellStateData == NC_DISCING_STATE)
    {
        if(userSetting->DeviceCtrlCmdState == NC_CTRL_CMD_NFINISH_FIXC) 
        {
            TCellStateData = NC_DISCING_FIXC_STATE;
        }
        else if(userSetting->DeviceCtrlCmdState == NC_CTRL_CMD_QFINISH) 
        {
            TCellStateData = NC_DISCING_QF_STATE;
        }
    }
    else if(TCellStateData == NC_DISCING_FINISH_STATE)
    {
        if(userSetting->DeviceCtrlCmdState == NC_CTRL_CMD_QFINISH)
        {
            TCellStateData = NC_DISCING_QF_FINISH_STATE;
        }
    }
    gExProtocol.upload.PushDeviceRealTimeReport.CellStateDataD3     = TCellStateData;//gCh3ManageCtr.CellStateData;
    gExProtocol.upload.PushDeviceRealTimeReport.DeviceAlarmStateD3  = gCh3ManageCtr.DeviceAlarmState;
    gExProtocol.upload.PushDeviceRealTimeReport.CellCapacityD3      = gCh3ManageCtr.CellCapacityAH;//容量
    //gExProtocol.upload.PushDeviceRealTimeReport.CellCapacityWhD3    = gCh3ManageCtr.CellCapacityWH;
    
    gExProtocol.upload.PushDeviceRealTimeReport.CellVoltageD4       = gCh4ManageCtr.CurrentDispVolt;
    gExProtocol.upload.PushDeviceRealTimeReport.CurrentRL_VoltD4    = gCh4ManageCtr.CurrentRL_Volt;
    gExProtocol.upload.PushDeviceRealTimeReport.CellCurrentD4       = gCh4ManageCtr.CurrentDispCurrent;
    TCellStateData = gCh4ManageCtr.CellStateData;
    
    if(TCellStateData == NC_DISCING_STATE)
    {
        if(userSetting->DeviceCtrlCmdState == NC_CTRL_CMD_NFINISH_FIXC) 
        {
            TCellStateData = NC_DISCING_FIXC_STATE;
        }
        else if(userSetting->DeviceCtrlCmdState == NC_CTRL_CMD_QFINISH) 
        {
            TCellStateData = NC_DISCING_QF_STATE;
        }
    }
    else if(TCellStateData == NC_DISCING_FINISH_STATE)
    {
        if(userSetting->DeviceCtrlCmdState == NC_CTRL_CMD_QFINISH)
        {
            TCellStateData = NC_DISCING_QF_FINISH_STATE;
        }
    }
    gExProtocol.upload.PushDeviceRealTimeReport.CellStateDataD4     = TCellStateData;//gCh4ManageCtr.CellStateData;
    gExProtocol.upload.PushDeviceRealTimeReport.DeviceAlarmStateD4  = gCh4ManageCtr.DeviceAlarmState;
    gExProtocol.upload.PushDeviceRealTimeReport.CellCapacityD4      = gCh4ManageCtr.CellCapacityAH;//容量
    //gExProtocol.upload.PushDeviceRealTimeReport.CellCapacityWhD4    = gCh4ManageCtr.CellCapacityWH;
    
    gExProtocol.upload.PushDeviceRealTimeReport.Ext_NTC_Value       = userSetting->Ext_NTC_Value;
    gExProtocol.upload.PushDeviceRealTimeReport.FinNTC_Data         = userSetting->FinNTC_Data;
    gExProtocol.upload.PushDeviceRealTimeReport.CHxStopCode         = userSetting->CHxStopCode;
    gExProtocol.upload.PushDeviceRealTimeReport.CHxTotalState       = userSetting->DeviceWorkModeType;//总的工作模式
	//把数据发送出去
	printf("disp1Volt:%d, RL1Volt:%d, disp2Volt:%d, RL2Volt:%d, disp3Volt:%d, RL3Volt:%d, disp4Volt:%d, RL4Volt:%d \n", gCh1ManageCtr.CurrentDispVolt, gCh1ManageCtr.CurrentRL_Volt, gCh2ManageCtr.CurrentDispVolt, gCh2ManageCtr.CurrentRL_Volt, gCh3ManageCtr.CurrentDispVolt, gCh3ManageCtr.CurrentRL_Volt, gCh4ManageCtr.CurrentDispVolt, gCh4ManageCtr.CurrentRL_Volt);
    NCInfoWlanParamSet(g_eD2App_PushDeviceRealTimeReport, (UINT8*)&gExProtocol.upload, sizeof(stuD2App_PushDeviceRealTimeReport));   
    NCSemaphoreGive(gMutexWlanPackage); //
}

//========================================================================
//========================================================================
void NCExUnMakeAckDeviceRealTimeReport(UINT8 *recbuff)        
{  
      NCUserSetting* userSetting = NCUserSettingGet();
      if(recbuff[0] ==0x02)
      {
         userSetting->FinAlarmState    = 0;//清除散热器温度保护
         gCh1ManageCtr.DeviceAlarmState = NC_ALARM_NONE;
         if(gCh2ManageCtr.DeviceAlarmState != NC_ALARM_NONE)
         {
             gCh2ManageCtr.DeviceAlarmState = NC_ALARM_NONE;
             NCEms02MakeAckDeviceRealTimeReport(0x02);//清除从机ID2报警标记
         }
         //========================
         if(gCh3ManageCtr.DeviceAlarmState != NC_ALARM_NONE)
         {
             gCh3ManageCtr.DeviceAlarmState = NC_ALARM_NONE;
             NCEms03MakeAckDeviceRealTimeReport(0x02);//清除从机ID3报警标记
         }
         //==============================
         if(gCh4ManageCtr.DeviceAlarmState != NC_ALARM_NONE)
         {
             gCh4ManageCtr.DeviceAlarmState = NC_ALARM_NONE;
             NCEms04MakeAckDeviceRealTimeReport(0x02);//清除从机ID4报警标记
         }                  
      }
      
    
    if(FALSE == NCSemaphoreTake(gMutexWlanPackage)) return;
    memcpy(&(gExProtocol.upload.AckAppRtDatAck.DeviceSN),        userSetting->deviceSN,      sizeof(userSetting->deviceSN));  
    NCInfoWlanParamSet(g_eD2App_AckAppRtDatAck, (UINT8*)&gExProtocol.upload, sizeof(stuD2App_AckAppRtDatAck));   
    NCSemaphoreGive(gMutexWlanPackage);     
}

//目前只要清除容量
void NCExMakeAckDeviceCleanState(UINT8 CtrlState,UINT8 IsSuccess)
{
    NCUserSetting* userSetting = NCUserSettingGet();
    if(FALSE == NCSemaphoreTake(gMutexWlanPackage))  return;
    
    memcpy(&(gExProtocol.upload.AckDeviceCleanState.DeviceSN),        userSetting->deviceSN,      sizeof(userSetting->deviceSN)); 
    
    gExProtocol.upload.AckDeviceCleanState.CtrlState = CtrlState;
    gExProtocol.upload.AckDeviceCleanState.IsSuccess = IsSuccess;
	//把数据发送出去
    NCInfoWlanParamSet(g_eD2App_AckDeviceCleanState, (UINT8*)&gExProtocol.upload, sizeof(stuD2App_AckDeviceCleanState));   
    NCSemaphoreGive(gMutexWlanPackage); // 
}

//=================================================================================
void NCExUnMakePushDeviceCleanState(UINT8 *recbuff)        
{  
    NCUserSetting* userSetting = NCUserSettingGet();
    
    gCh1ManageCtr.CellCapacityAH  = 0;
	//gCh1ManageCtr.CellCapacityWH  = 0;
	
    gCh2ManageCtr.CellCapacityAH  = 0;
	//gCh2ManageCtr.CellCapacityWH  = 0;
	
    gCh3ManageCtr.CellCapacityAH  = 0;
	//gCh3ManageCtr.CellCapacityWH  = 0;
	
    gCh4ManageCtr.CellCapacityAH  = 0;
	//gCh4ManageCtr.CellCapacityWH  = 0;
	
    NCExMakeAckDeviceCleanState(recbuff[0],0x01);
        
}


//===================================================================================
//本身就是开启 也是应答OK
//如果关闭 反正给他关闭


//    NC_WORK_MODE_DISCONNECT           = (0x00),/*未接入*/
//    NC_WORK_MODE_IDLE                 = (0x01),/*空闲*/
//    NC_WORK_MODE_START                = (0x02),/*正在启动中*/ 
//    NC_WORK_MODE_DISCING              = (0x03),/*放电*/ 
//    NC_WORK_MODE_DISC_SUCESS          = (0x04),/*放电完成*/ 
//    NC_WORK_MODE_SUSPEND              = (0x80),/*暂停*/
//    NC_WORK_MODE_RESTART              = (0x81),/*重启*/


//    HANDLE_STATE_STOP     = 0x00, //
//    HANDLE_STATE_START    = 0x01, //
//    HANDLE_STATE_SUSPEND  = 0x80,/*暂停*/
//    HANDLE_STATE_RESTART  = 0x81,/*重启*/  


//    NC_DISCONNECT_STATE               = (0x00),/*未接入状态*/
//    NC_IDLE_STATE                     = (0x01),/*空闲状态*/
//    NC_ON_STATE                       = (0x02),/*开启状态 正在启动*/
//    NC_DISCING_STATE                  = (0x03),/*放电中状态*/
//    NC_DISCING_FINISH_STATE           = (0x04),/*放电完成状态*/
//    NC_SUSPEND_STATE                  = (0x80),/*暂停*/  


void NCExMakeAckDeviceWorkCommand(UINT8 TargetUid,UINT8 DeviceWorkState,UINT8 IsSuccess)
{
    NCUserSetting* userSetting = NCUserSettingGet();
    if(FALSE == NCSemaphoreTake(gMutexWlanPackage))  return;
    
    memcpy(&(gExProtocol.upload.AckDeviceWorkCommand.DeviceSN),        userSetting->deviceSN,      sizeof(userSetting->deviceSN)); 
    
    gExProtocol.upload.AckDeviceWorkCommand.TargetUid       = TargetUid;
    gExProtocol.upload.AckDeviceWorkCommand.DeviceWorkState = DeviceWorkState;

    gExProtocol.upload.AckDeviceWorkCommand.IsSuccessUID01  = IsSuccess;
    gExProtocol.upload.AckDeviceWorkCommand.IsSuccessUID02  = IsSuccess;
    gExProtocol.upload.AckDeviceWorkCommand.IsSuccessUID03  = IsSuccess;
    gExProtocol.upload.AckDeviceWorkCommand.IsSuccessUID04  = IsSuccess;
    
	//把数据发送出去
    NCInfoWlanParamSet(g_eD2App_AckDeviceWorkCommand, (UINT8*)&gExProtocol.upload, sizeof(stuD2App_AckDeviceWorkCommand));   
    NCSemaphoreGive(gMutexWlanPackage); // 
}

NCIsSuccess0_4Stu AckIsSuccess;


//==================================================================================================
void NCExUnMakePushDeviceWorkCommand(UINT8 *recbuff,UINT16 datalength)
{
    NCUserSetting* userSetting = NCUserSettingGet();
    
    struct _NCMSComProtocol *MSComUnionProtocol  = GetMSComUnionProtocol();
    
    AckIsSuccess.IsSuccessUID01 = NC_COMM_ACK_FL;
    AckIsSuccess.IsSuccessUID02 = NC_COMM_ACK_FL;
    AckIsSuccess.IsSuccessUID03 = NC_COMM_ACK_FL;
    AckIsSuccess.IsSuccessUID04 = NC_COMM_ACK_FL;
    
    UINT8      UsableUid = 0; 
    //BOOL       IsSameState = TRUE;    
    //UINT8    IsSuccessUID01 = NC_COMM_ACK_FL;
    //UINT8    IsSuccessUID02 = NC_COMM_ACK_FL;
    //UINT8    IsSuccessUID03 = NC_COMM_ACK_FL;
    //UINT8    IsSuccessUID04 = NC_COMM_ACK_FL;
    if(datalength>1)  datalength -= 1;

    memcpy(&gExProtocol.receive.PushDeviceWorkCommand, recbuff, sizeof(stuApp2D_PushDeviceWorkCommand));
    userSetting->IsForbidFanStart = FALSE;
    //保存从机需要发送的数据
    #if      0 //2024-1-3
    MSComUnionProtocol->upload.PushDeviceWorkCommand.DeviceWorkState        = gExProtocol.receive.PushDeviceWorkCommand.DeviceWorkState;
    MSComUnionProtocol->upload.PushDeviceWorkCommand.TestedCellSetVolt      = gExProtocol.receive.PushDeviceWorkCommand.TestedCellSetVolt;
    MSComUnionProtocol->upload.PushDeviceWorkCommand.TestedCellSetCurrent   = gExProtocol.receive.PushDeviceWorkCommand.TestedCellSetCurrent;
    MSComUnionProtocol->upload.PushDeviceWorkCommand.Cell_Volt_Upper        = gExProtocol.receive.PushDeviceWorkCommand.Cell_Volt_Upper;
    MSComUnionProtocol->upload.PushDeviceWorkCommand.Cell_Volt_Lower        = gExProtocol.receive.PushDeviceWorkCommand.Cell_Volt_Lower;
    MSComUnionProtocol->upload.PushDeviceWorkCommand.VoltUnusualChangeRate  = gExProtocol.receive.PushDeviceWorkCommand.VoltUnusualChangeRate;
    #endif
    //获取在线的可用设备
    if(userSetting->IsAgingProcess  || userSetting->IsTotalMachineTest)
    {
        //userSetting->IsAgingProcess        = FALSE; //在关闭输出时候清零了这个标志
        userSetting->IsTotalMachineTest    = FALSE;
        userSetting->DeviceWorkModeType    = NC_WORK_MODE_DISABLE_CMD;
        userSetting->CHxStopCode           = CHX_STOP_CODE_CMD;
        //老化响应关闭
        NCExMakeAckDeviceWorkCommand(userSetting->SettingTargetUid,HANDLE_STATE_STOP,NC_COMM_ACK_OK); 
        vTaskDelay(150);         
        return;
    }    
    //查询本机 在线的CHx
    if(gCh1ManageCtr.CellStateData != NC_DISCONNECT_STATE)   UsableUid = 0x01; 
    if(gCh2ManageCtr.CellStateData != NC_DISCONNECT_STATE)   UsableUid |= 0x02;
    if(gCh3ManageCtr.CellStateData != NC_DISCONNECT_STATE)   UsableUid |= 0x04;
    if(gCh4ManageCtr.CellStateData != NC_DISCONNECT_STATE)   UsableUid |= 0x08;
    
    //printf("UsableUid = %d\n\r",UsableUid);
     //printf("TargetUid = %d\n\r",gExProtocol.receive.PushDeviceWorkCommand.TargetUid);
    
    if(gExProtocol.receive.PushDeviceWorkCommand.DeviceWorkState == HANDLE_STATE_STOP) //只要不是  NC_IDLE_STATE
    {
        //printf("HANDLE_STATE_STOP");
		///userSetting->SettingTargetUid      = 0x0F;//不管全部关闭掉
        userSetting->IdleTimeStamp         = 300;                           
        userSetting->DeviceWorkModeType    = NC_WORK_MODE_DISABLE_CMD;
        userSetting->CHxStopCode           = CHX_STOP_CODE_CMD;
        NCExMakeAckDeviceWorkCommand(userSetting->SettingTargetUid,HANDLE_STATE_STOP,NC_COMM_ACK_OK);   
        vTaskDelay(150);        
     }   
    else if(((gExProtocol.receive.PushDeviceWorkCommand.TargetUid) & UsableUid) != gExProtocol.receive.PushDeviceWorkCommand.TargetUid)   //
    {
        //有不在线的设备
        NCExMakeAckDeviceWorkCommand(gExProtocol.receive.PushDeviceWorkCommand.TargetUid,gExProtocol.receive.PushDeviceWorkCommand.DeviceWorkState,NC_COMM_ID_NO_MATE);
    }
    else
    {
        
        if(userSetting->DeviceCtrlCmdState  != HANDLE_STATE_NFINISH_FIXC_START)
        {        
            //正常启动 和 快速启动模式
            if(gExProtocol.receive.PushDeviceWorkCommand.DeviceWorkState == HANDLE_STATE_START     || \
               gExProtocol.receive.PushDeviceWorkCommand.DeviceWorkState == HANDLE_STATE_QFINISH_START)
            {
                 //判断状态是否合理
                 //保存 APP 配置的UID
                 //UsableUid 当前设备存在的UID
                 //===================================================================================================
                //if(IsSameState==TRUE) //     
                if(userSetting->DeviceWorkModeType == NC_WORK_MODE_IDLE || \
                  (userSetting->DeviceWorkModeType == NC_WORK_MODE_DISCING && userSetting->DeviceRunState  ==  NC_DISCING_FINISH_STATE))
                {
                     //获取所有在线从机信息
                    userSetting->SettingTargetUid = gExProtocol.receive.PushDeviceWorkCommand.TargetUid;//UsableUid;
                    userSetting->CHxStopCode    =  CHX_STOP_CODE_NONE;
                    userSetting->DeviceCtrlCmdState    = gExProtocol.receive.PushDeviceWorkCommand.DeviceWorkState;
                    gCh2ManageCtr.IsHaveNewRealData    = FALSE;
                    gCh3ManageCtr.IsHaveNewRealData    = FALSE;
                    gCh4ManageCtr.IsHaveNewRealData    = FALSE;
                    userSetting->IsBatVolSamping       = TRUE; 
                    NCExMakeAckDeviceWorkCommand(userSetting->SettingTargetUid,HANDLE_STATE_START,NC_COMM_ACK_OK);
                    userSetting->IdleTimeStamp         = 200;
                    userSetting->TestedCellSetVolt     = gExProtocol.receive.PushDeviceWorkCommand.TestedCellSetVolt;
                    //userSetting->TestedCellSetCurrent  = gExProtocol.receive.PushDeviceWorkCommand.TestedCellSetCurrent;
                    //userSetting->Cell_Volt_Upper       = gExProtocol.receive.PushDeviceWorkCommand.Cell_Volt_Upper;
                    userSetting->Cell_Volt_Lower       = gExProtocol.receive.PushDeviceWorkCommand.Cell_Volt_Lower;
                    userSetting->VoltUnusualChangeRate = gExProtocol.receive.PushDeviceWorkCommand.VoltUnusualChangeRate;
                    userSetting->TestedCellType        = gExProtocol.receive.PushDeviceWorkCommand.TestedCellType;                  
                    vTaskDelay(100);      //让数据发出 
                    if(userSetting->SettingTargetUid&0x01)    
                    {
                        gCh1ManageCtr.TestedCellSetCurrent = gExProtocol.receive.PushDeviceWorkCommand.TestedCellSetCurrent;
                    }
                    if(userSetting->SettingTargetUid&0x02)    
                    {
                        NCEms02MakeAskDeviceRunRealData(NC_READ_DATA_IDLE_VC_STATE);
                        gCh2ManageCtr.TestedCellSetCurrent = gExProtocol.receive.PushDeviceWorkCommand.TestedCellSetCurrent;
                    }
                    if(userSetting->SettingTargetUid&0x04)    
                    {
                        NCEms03MakeAskDeviceRunRealData(NC_READ_DATA_IDLE_VC_STATE);
                        gCh3ManageCtr.TestedCellSetCurrent = gExProtocol.receive.PushDeviceWorkCommand.TestedCellSetCurrent;
                    }
                    if(userSetting->SettingTargetUid&0x08)       
                    {
                        NCEms04MakeAskDeviceRunRealData(NC_READ_DATA_IDLE_VC_STATE); 
                        gCh4ManageCtr.TestedCellSetCurrent = gExProtocol.receive.PushDeviceWorkCommand.TestedCellSetCurrent;
                    }                
                    vTaskDelay(250);      //读取数据 需要退出此函数  
                                 
                    userSetting->DeviceWorkModeType    = NC_WORK_MODE_DISCING;/*放电*/
                    userSetting->DeviceRunState        = NC_ON_STATE;
                     
                    //printf("NewRealD2=%d\n\r",userSetting->IsHaveNewRealDataD2);
                    //printf("NewRealD3=%d\n\r",userSetting->IsHaveNewRealDataD2);
                    //printf("NewRealD4=%d\n\r",userSetting->IsHaveNewRealDataD4);
                     
                    gCh1ManageCtr.CellCapacityAH       = 0;//重新启动容量复位
                    //gCh1ManageCtr.CellCapacityWH       = 0;
                    gCh2ManageCtr.CellCapacityAH       = 0;//重新启动容量复位
                    //gCh2ManageCtr.CellCapacityWH       = 0;
                    gCh3ManageCtr.CellCapacityAH       = 0;//重新启动容量复位
                    //gCh3ManageCtr.CellCapacityWH       = 0;
                    gCh4ManageCtr.CellCapacityAH       = 0;//重新启动容量复位
                    //gCh4ManageCtr.CellCapacityWH       = 0;														
                }
                else
                { 
                    //指令错误 状态不一致
                    //printf("IsSameState==FALSE");
                    //NCExMakeAckDeviceWorkCommand(gExProtocol.receive.PushDeviceWorkCommand.TargetUid,HANDLE_STATE_START,NC_COMM_CMD_ER);
                    NCExMakeAckDeviceWorkCommand(gExProtocol.receive.PushDeviceWorkCommand.TargetUid,\
                                                 gExProtocol.receive.PushDeviceWorkCommand.DeviceWorkState,NC_COMM_CMD_ER);		                    
                }                           
            }
            #if      0//正常指令取消 这两个控制码
            else if(gExProtocol.receive.PushDeviceWorkCommand.DeviceWorkState == HANDLE_STATE_NFINISH_FIXC_START || \
                     gExProtocol.receive.PushDeviceWorkCommand.DeviceWorkState == HANDLE_STATE_NFINISH_AUC_START)//
            {
                 userSetting->CHxStopCode    =  CHX_STOP_CODE_NONE;
                 #if     0                                  
                 if(UsableUid&0x01)
                 {
                    if(gCh1ManageCtr.CellStateData != NC_IDLE_STATE && gCh1ManageCtr.CellStateData !=NC_DISCING_FINISH_STATE) 
                    {
                        IsSameState = FALSE;               
                    }                    
                 }
                 if(UsableUid&0x02)
                 {
                    if(gCh2ManageCtr.CellStateData != NC_IDLE_STATE && gCh2ManageCtr.CellStateData !=NC_DISCING_FINISH_STATE)
                    {
                        IsSameState = FALSE;               
                    }              
                 }
                 if(UsableUid&0x04)
                 {
                    if(gCh3ManageCtr.CellStateData != NC_IDLE_STATE && gCh3ManageCtr.CellStateData !=NC_DISCING_FINISH_STATE)
                    {
                        IsSameState = FALSE;               
                    }              
                 }
                 if(UsableUid&0x08)
                 {
                    if(gCh4ManageCtr.CellStateData != NC_IDLE_STATE && gCh4ManageCtr.CellStateData !=NC_DISCING_FINISH_STATE)
                    {
                        IsSameState = FALSE;               
                    }              
                 }
                 #endif
                 //===================================================================================================
                 //收到指令未有输出
                 if(userSetting->DeviceWorkModeType == NC_WORK_MODE_IDLE || \
                  (userSetting->DeviceWorkModeType == NC_WORK_MODE_DISCING && userSetting->DeviceRunState  ==  NC_DISCING_FINISH_STATE))
                 {
                     userSetting->SettingTargetUid = gExProtocol.receive.PushDeviceWorkCommand.TargetUid;//UsableUid;
                    //
                    //获取所有在线从机信息
                    userSetting->DeviceCtrlCmdState    = gExProtocol.receive.PushDeviceWorkCommand.DeviceWorkState;
                    gCh2ManageCtr.IsHaveNewRealData    = FALSE;
                    gCh3ManageCtr.IsHaveNewRealData    = FALSE;
                    gCh4ManageCtr.IsHaveNewRealData    = FALSE;
                    userSetting->IsBatVolSamping       = TRUE; 
                    NCExMakeAckDeviceWorkCommand(userSetting->SettingTargetUid,userSetting->DeviceCtrlCmdState,NC_COMM_ACK_OK);
                    userSetting->IdleTimeStamp         = 200;
                    userSetting->TestedCellSetVolt     = gExProtocol.receive.PushDeviceWorkCommand.TestedCellSetVolt;
                    //userSetting->TestedCellSetCurrent  = gExProtocol.receive.PushDeviceWorkCommand.TestedCellSetCurrent;
                    //userSetting->Cell_Volt_Upper       = gExProtocol.receive.PushDeviceWorkCommand.Cell_Volt_Upper;
                    userSetting->Cell_Volt_Lower       = gExProtocol.receive.PushDeviceWorkCommand.Cell_Volt_Lower;
                    userSetting->VoltUnusualChangeRate = gExProtocol.receive.PushDeviceWorkCommand.VoltUnusualChangeRate;
                    userSetting->TestedCellType        = gExProtocol.receive.PushDeviceWorkCommand.TestedCellType;                  
                    vTaskDelay(100);      //让数据发出 
                    if(userSetting->SettingTargetUid&0x01)
                    {
                        gCh1ManageCtr.TestedCellSetCurrent = gExProtocol.receive.PushDeviceWorkCommand.TestedCellSetCurrent;
                    }
                    if(userSetting->SettingTargetUid&0x02) 
                    {                    
                        NCEms02MakeAskDeviceRunRealData(NC_READ_DATA_IDLE_VC_STATE);
                        gCh2ManageCtr.TestedCellSetCurrent = gExProtocol.receive.PushDeviceWorkCommand.TestedCellSetCurrent;
                    }
                    if(userSetting->SettingTargetUid&0x04)    
                    {
                        NCEms03MakeAskDeviceRunRealData(NC_READ_DATA_IDLE_VC_STATE);
                        gCh3ManageCtr.TestedCellSetCurrent = gExProtocol.receive.PushDeviceWorkCommand.TestedCellSetCurrent;
                    }
                    if(userSetting->SettingTargetUid&0x08)   
                    {                    
                        NCEms04MakeAskDeviceRunRealData(NC_READ_DATA_IDLE_VC_STATE); 
                        gCh4ManageCtr.TestedCellSetCurrent = gExProtocol.receive.PushDeviceWorkCommand.TestedCellSetCurrent;                    
                    }                    
                    vTaskDelay(250);      //读取数据 需要退出此函数  
                                 
                    userSetting->DeviceWorkModeType    = NC_WORK_MODE_DISCING;/*放电*/
                    userSetting->DeviceRunState        = NC_ON_STATE;
                     
                     //printf("NewRealD2=%d\n\r",userSetting->IsHaveNewRealDataD2);
                     //printf("NewRealD3=%d\n\r",userSetting->IsHaveNewRealDataD2);
                     //printf("NewRealD4=%d\n\r",userSetting->IsHaveNewRealDataD4);
                     
                    gCh1ManageCtr.CellCapacityAH       = 0;//重新启动容量复位
                    //gCh1ManageCtr.CellCapacityWH       = 0;
                    gCh2ManageCtr.CellCapacityAH       = 0;//重新启动容量复位
                    //gCh2ManageCtr.CellCapacityWH       = 0;
                    gCh3ManageCtr.CellCapacityAH       = 0;//重新启动容量复位
                    //gCh3ManageCtr.CellCapacityWH       = 0;
                    gCh4ManageCtr.CellCapacityAH       = 0;//重新启动容量复位
                    //gCh4ManageCtr.CellCapacityWH       = 0;	
                 
                 }
                 else if(userSetting->DeviceWorkModeType == NC_WORK_MODE_DISCING  && \
                         (userSetting->DeviceCtrlCmdState ==  gExProtocol.receive.PushDeviceWorkCommand.DeviceWorkState))//HANDLE_STATE_NFINISH_FIXC_START  HANDLE_STATE_NFINISH_AUC_START
                 {
                     //收到指令已经处于此模式内
                     //userSetting->DeviceCtrlCmdState    = gExProtocol.receive.PushDeviceWorkCommand.DeviceWorkState;
                     if(HANDLE_STATE_NFINISH_FIXC_START == userSetting->DeviceCtrlCmdState)
                     {
                        if(gExProtocol.receive.PushDeviceWorkCommand.TargetUid & 0x01) 
                        {
                            gCh1ManageCtr.IsOverAgainStartup =FALSE;
                            gCh1ManageCtr.IsReachCurrent    = FALSE;
                            gCh1ManageCtr.TestedCellSetCurrent = gExProtocol.receive.PushDeviceWorkCommand.TestedCellSetCurrent; 
                        }
                        if(gExProtocol.receive.PushDeviceWorkCommand.TargetUid & 0x02) 
                        {
                            gCh2ManageCtr.IsOverAgainStartup =FALSE;
                            gCh2ManageCtr.IsReachCurrent    = FALSE;
                            gCh2ManageCtr.TestedCellSetCurrent = gExProtocol.receive.PushDeviceWorkCommand.TestedCellSetCurrent; 
                        }
                        if(gExProtocol.receive.PushDeviceWorkCommand.TargetUid & 0x04) 
                        {
                            gCh3ManageCtr.IsOverAgainStartup =FALSE;
                            gCh3ManageCtr.IsReachCurrent    = FALSE;
                            gCh3ManageCtr.TestedCellSetCurrent = gExProtocol.receive.PushDeviceWorkCommand.TestedCellSetCurrent; 
                        }
                        if(gExProtocol.receive.PushDeviceWorkCommand.TargetUid & 0x08) 
                        {
                            gCh4ManageCtr.IsOverAgainStartup =FALSE;
                            gCh4ManageCtr.IsReachCurrent    = FALSE;
                            gCh4ManageCtr.TestedCellSetCurrent = gExProtocol.receive.PushDeviceWorkCommand.TestedCellSetCurrent; 
                        } 
                        NCExMakeAckDeviceWorkCommand(gExProtocol.receive.PushDeviceWorkCommand.TargetUid,userSetting->DeviceCtrlCmdState,NC_COMM_ACK_OK);
                     }
                     
                 }
                 else
                 {
                    //指令错误 状态不一致
                    //printf("IsSameState==FALSE");
                    //NCExMakeAckDeviceWorkCommand(gExProtocol.receive.PushDeviceWorkCommand.TargetUid,HANDLE_STATE_START,NC_COMM_CMD_ER);
                    NCExMakeAckDeviceWorkCommand(gExProtocol.receive.PushDeviceWorkCommand.TargetUid,\
                                                 gExProtocol.receive.PushDeviceWorkCommand.DeviceWorkState,NC_COMM_CMD_ER);			             
                 }                         				
            }
            #endif
            else  if(gExProtocol.receive.PushDeviceWorkCommand.DeviceWorkState == HANDLE_STATE_SUSPEND) //HANDLE_STATE_START  NC_DISCING_FINISH_STATE
            {
                 //判断状态是否合理
                //printf("HANDLE_STATE_SUSPEND");
                //保存 APP 配置的UID
                 //UsableUid 当前设备存在的UID
                 //userSetting->SettingTargetUid = ;//UsableUid;
                 #if  0
                 if(UsableUid&0x01)
                 {
                    if(gCh1ManageCtr.CellStateData != NC_DISCING_STATE && gCh1ManageCtr.CellStateData !=NC_DISCING_FINISH_STATE)
                    {
                        IsSameState = FALSE;               
                    }                    
                 }
                 if(UsableUid&0x02)
                 {
                    if(gCh2ManageCtr.CellStateData != NC_DISCING_STATE && gCh2ManageCtr.CellStateData !=NC_DISCING_FINISH_STATE)
                    {
                        IsSameState = FALSE;               
                    }              
                 }
                 if(UsableUid&0x04)
                 {
                    if(gCh3ManageCtr.CellStateData != NC_DISCING_STATE && gCh3ManageCtr.CellStateData !=NC_DISCING_FINISH_STATE)
                    {
                        IsSameState = FALSE;               
                    }              
                 }
                 if(UsableUid&0x08)
                 {
                    if(gCh4ManageCtr.CellStateData != NC_DISCING_STATE && gCh4ManageCtr.CellStateData !=NC_DISCING_FINISH_STATE)
                    {
                        IsSameState = FALSE;               
                    }              
                 }
                 #endif
                 //===============================================================================
                 //正常的启动模式和快速完成模式才支持暂停
                 //需要与当前在工作的UID 相一致
                 if(userSetting->DeviceWorkModeType    ==  NC_WORK_MODE_DISCING \
                   && (userSetting->DeviceCtrlCmdState ==  NC_CTRL_CMD_NORMAL  || userSetting->DeviceCtrlCmdState == NC_CTRL_CMD_QFINISH) \
                   && userSetting->SettingTargetUid    ==  gExProtocol.receive.PushDeviceWorkCommand.TargetUid)//需要与当前在工作的UID 相一致  
                 {
                    userSetting->DeviceLastWorkModeType = userSetting->DeviceWorkModeType;
                    userSetting->DeviceWorkModeType     = NC_WORK_MODE_SUSPEND;
                    NCExMakeAckDeviceWorkCommand(userSetting->SettingTargetUid,HANDLE_STATE_SUSPEND,NC_COMM_ACK_OK);   
                    //printf("HANDLE_STATE_SUSPEND");   				 				 
                 }
                 else
                 {
                    //指令错误 状态不一致
                    NCExMakeAckDeviceWorkCommand(gExProtocol.receive.PushDeviceWorkCommand.TargetUid,HANDLE_STATE_SUSPEND,NC_COMM_CMD_ER);				 				 				 				 
                 }
                               
            }
            else if(gExProtocol.receive.PushDeviceWorkCommand.DeviceWorkState == HANDLE_STATE_RESTART)  //HANDLE_STATE_SUSPEND
            {
                //判断状态是否合理
                //printf("HANDLE_STATE_RESTART");
                #if  0
                if(UsableUid&0x01)
                {
                    if(gCh1ManageCtr.CellStateData != NC_SUSPEND_STATE)
                    {
                        IsSameState = FALSE;               
                    }                    
                }
                if(UsableUid&0x02)
                {
                    if(gCh2ManageCtr.CellStateData != NC_SUSPEND_STATE)
                    {
                        IsSameState = FALSE;               
                    }              
                }
                if(UsableUid&0x04)
                {
                    if(gCh3ManageCtr.CellStateData != NC_SUSPEND_STATE)
                    {
                        IsSameState = FALSE;               
                    }              
                }
                if(UsableUid&0x08)
                {
                    if(gCh4ManageCtr.CellStateData != NC_SUSPEND_STATE)
                    {
                        IsSameState = FALSE;               
                    }              
                }
                #endif
                //===================================================================================================
                if(userSetting->DeviceWorkModeType    ==  NC_WORK_MODE_SUSPEND \
                   && (userSetting->DeviceCtrlCmdState ==  NC_CTRL_CMD_NORMAL  || userSetting->DeviceCtrlCmdState == NC_CTRL_CMD_QFINISH) \
                   && userSetting->SettingTargetUid    ==  gExProtocol.receive.PushDeviceWorkCommand.TargetUid)//需要与当前在工作的UID 相一致  
                 {
                    userSetting->IsBatVolSamping          = TRUE; 
                    gCh2ManageCtr.IsHaveNewRealData      = FALSE;
                    gCh3ManageCtr.IsHaveNewRealData      = FALSE;
                    gCh4ManageCtr.IsHaveNewRealData      = FALSE;
                    NCExMakeAckDeviceWorkCommand(userSetting->SettingTargetUid,HANDLE_STATE_RESTART,NC_COMM_ACK_OK);
                    userSetting->IdleTimeStamp  = 200;                
                    vTaskDelay(100);//等待上次可能没发送完成发送完成                 
                    if(UsableUid&0x02)    NCEms02MakeAskDeviceRunRealData(NC_READ_DATA_IDLE_VC_STATE);
                    if(UsableUid&0x04)    NCEms03MakeAskDeviceRunRealData(NC_READ_DATA_IDLE_VC_STATE);
                    if(UsableUid&0x08)    NCEms04MakeAskDeviceRunRealData(NC_READ_DATA_IDLE_VC_STATE);                 
                    vTaskDelay(250);    
                    userSetting->DeviceWorkModeType = HANDLE_STATE_RESTART;    				 
                     
                 }
                 else
                 {
                    //指令错误 状态不一致
                    NCExMakeAckDeviceWorkCommand(gExProtocol.receive.PushDeviceWorkCommand.TargetUid,HANDLE_STATE_RESTART,NC_COMM_CMD_ER);			 				 				 				 
                 }									             
            }
        }
        else
        {
            //在边充边放过程中不支持此指令
            NCExMakeAckDeviceWorkCommand(gExProtocol.receive.PushDeviceWorkCommand.TargetUid,HANDLE_STATE_RESTART,NC_COMM_CMD_ER);	
        }        
    }
    //只能初次启动才需要如此操作
    if(userSetting->DeviceRunState == NC_ON_STATE)
    {
        vTaskDelay(100);
        //工作过程不允许蓝牙配网
        userSetting->AP_NetInfo.IsBleWaitConfig = FALSE;
        if(userSetting->AP_NetInfo.IsBluetoothEnable)
        {
          userSetting->AP_NetInfo.IsColseBleCofig = TRUE;
        }
    }        
}    
//========================================================================================================
//q启动时候没有电流中途加电流
static void   NC_FallDischargeReStart(NCStu_ChxManageCtr * ChxManageCtr)
{
     ChxManageCtr->CurrentconfigCurrent = ChxManageCtr->TestedCellSetCurrent;
     if(ChxManageCtr->DoingBatRealVolt > (NC_MIN_DEVICE_WORK_VOLT +400))
     {         
         if(ChxManageCtr->CurrentconfigCurrent > 2000)//
            ChxManageCtr->CurrentconfigCurrent = 2000;                       
     }
     else if(ChxManageCtr->DoingBatRealVolt > (NC_MIN_DEVICE_WORK_VOLT +300))
     {
         if(ChxManageCtr->CurrentconfigCurrent > 1000)//
            ChxManageCtr->CurrentconfigCurrent = 1000;                       
     }
     else  if(ChxManageCtr->DoingBatRealVolt > (NC_MIN_DEVICE_WORK_VOLT +200))
     {
         if(ChxManageCtr->CurrentconfigCurrent > 800)//
            ChxManageCtr->CurrentconfigCurrent = 800;                       
     } 
     else
     {
         if(ChxManageCtr->CurrentconfigCurrent>500)//加一点缓启动 
            ChxManageCtr->CurrentconfigCurrent = 500;                     
     } 
}
//==========================================================================================
//==========================================================================================
static void   NC_FallDischargeChange(NCStu_ChxManageCtr * ChxManageCtr)
{
     UINT16   D_value;
    
    if(ChxManageCtr->CurrentconfigCurrent>ChxManageCtr->TestedCellSetCurrent)
    {
       //减小
       ChxManageCtr->CurrentconfigCurrent =  ChxManageCtr->TestedCellSetCurrent;   
    }
    else
    {
        D_value  = ChxManageCtr->TestedCellSetCurrent - ChxManageCtr->CurrentconfigCurrent;
        if(ChxManageCtr->DoingBatRealVolt > (NC_MIN_DEVICE_WORK_VOLT +400))
        {         
             if(D_value > 2000)//
                D_value = 2000;                       
        }
        else if(ChxManageCtr->DoingBatRealVolt > (NC_MIN_DEVICE_WORK_VOLT +300))
        {
             if(D_value > 1000)//
                D_value = 1000;                       
        }
        else  if(ChxManageCtr->DoingBatRealVolt > (NC_MIN_DEVICE_WORK_VOLT +200))
        {
             if(D_value > 800)//
                D_value = 800;                       
        } 
        else
        {
             if(D_value>300)//加一点缓启动 
                D_value = 300;                     
        }   
        ChxManageCtr->CurrentconfigCurrent  += D_value;
        
    }     
}
//========================================================================================================
void NCExMakeAckDeviceFallDischargeCmd(UINT8 TargetUid,UINT8 DeviceWorkState,UINT8 IsSuccess)
{
    NCUserSetting* userSetting = NCUserSettingGet();
    if(FALSE == NCSemaphoreTake(gMutexWlanPackage))  return; //
    
    memcpy(&(gExProtocol.upload.AckDeviceFallDischargeCmd.DeviceSN),        userSetting->deviceSN,      sizeof(userSetting->deviceSN)); 
    gExProtocol.upload.AckDeviceFallDischargeCmd.TargetUid       = TargetUid;
    gExProtocol.upload.AckDeviceFallDischargeCmd.DeviceWorkState = DeviceWorkState;
    gExProtocol.upload.AckDeviceFallDischargeCmd.IsSuccess  = IsSuccess;
	//把数据发送出去
    NCInfoWlanParamSet(g_eD2App_AckDeviceFallDischargeCmd, (UINT8*)&gExProtocol.upload, sizeof(stuD2App_AckDeviceFallDischargeCmd));   
    NCSemaphoreGive(gMutexWlanPackage); // 
}
//========================================================================================================
void NCExUnMakePushDeviceFallDischargeCmd(UINT8 *recbuff,UINT16 datalength)
{
    NCUserSetting* userSetting = NCUserSettingGet();
    struct _NCMSComProtocol *MSComUnionProtocol  = GetMSComUnionProtocol();
    
    UINT8      UsableUid = 0; 
    if(datalength>1)  datalength -= 1;

    memcpy(&gExProtocol.receive.PushDeviceFallDischargeCmd, recbuff, sizeof(stuApp2D_PushDeviceFallDischargeCmd));
    userSetting->IsForbidFanStart = FALSE;
    //获取在线的可用设备
    if(userSetting->IsAgingProcess  || userSetting->IsTotalMachineTest)
    {
        //userSetting->IsAgingProcess        = FALSE; //在后续关闭输出 会清零这个标记
        userSetting->IsTotalMachineTest    = FALSE;
        userSetting->DeviceWorkModeType    = NC_WORK_MODE_DISABLE_CMD;
        userSetting->CHxStopCode           = CHX_STOP_CODE_CMD;
        //老化响应关闭
        NCExMakeAckDeviceFallDischargeCmd(userSetting->SettingTargetUid,HANDLE_STATE_STOP,NC_COMM_ACK_OK); 
        vTaskDelay(150); ////防止快速启动 快速关闭        
        return;
    }    
    //查询本机 在线的CHx
    if(gCh1ManageCtr.CellStateData != NC_DISCONNECT_STATE)   UsableUid  = 0x01; 
    if(gCh2ManageCtr.CellStateData != NC_DISCONNECT_STATE)   UsableUid |= 0x02;
    if(gCh3ManageCtr.CellStateData != NC_DISCONNECT_STATE)   UsableUid |= 0x04;
    if(gCh4ManageCtr.CellStateData != NC_DISCONNECT_STATE)   UsableUid |= 0x08;
    //============================================================================================
    if(gExProtocol.receive.PushDeviceFallDischargeCmd.DeviceWorkState == HANDLE_STATE_STOP) //只要不是  NC_IDLE_STATE
    {
		//userSetting->SettingTargetUid      = 0x0F;//不管全部关闭掉
        userSetting->IdleTimeStamp         = 300;                           
        userSetting->DeviceWorkModeType    = NC_WORK_MODE_DISABLE_CMD;
        userSetting->CHxStopCode           = CHX_STOP_CODE_CMD;
        NCExMakeAckDeviceFallDischargeCmd(userSetting->SettingTargetUid,HANDLE_STATE_STOP,NC_COMM_ACK_OK); 
        vTaskDelay(150); //防止快速启动 快速关闭        
     }   
    else if(((gExProtocol.receive.PushDeviceFallDischargeCmd.TargetUid) & UsableUid) != gExProtocol.receive.PushDeviceFallDischargeCmd.TargetUid)   //
    {
        //有不在线的设备
        NCExMakeAckDeviceFallDischargeCmd(gExProtocol.receive.PushDeviceFallDischargeCmd.TargetUid,gExProtocol.receive.PushDeviceFallDischargeCmd.DeviceWorkState,NC_COMM_ID_NO_MATE);
    }
    else
    {
        //正常启动 和 快速启动模式直接关闭
        if(gExProtocol.receive.PushDeviceFallDischargeCmd.DeviceWorkState == HANDLE_STATE_START)
        {
            //错误直接关闭
            ///userSetting->SettingTargetUid      = 0x0F;//不管全部关闭掉
            userSetting->IdleTimeStamp         = 300;                           
            userSetting->DeviceWorkModeType    = NC_WORK_MODE_DISABLE_CMD;
            userSetting->CHxStopCode           = CHX_STOP_CODE_CMD;
            NCExMakeAckDeviceFallDischargeCmd(userSetting->SettingTargetUid,HANDLE_STATE_STOP,NC_COMM_CMD_ER); 
        }
		else if(gExProtocol.receive.PushDeviceFallDischargeCmd.DeviceWorkState == HANDLE_STATE_NFINISH_FIXC_START)//
	    {
		     userSetting->CHxStopCode    =  CHX_STOP_CODE_NONE;
             //===================================================================================================
             //收到指令未有输出
             if(userSetting->DeviceWorkModeType == NC_WORK_MODE_IDLE || \
			  (userSetting->DeviceWorkModeType == NC_WORK_MODE_DISCING && userSetting->DeviceRunState  ==  NC_DISCING_FINISH_STATE))
             {
                 userSetting->SettingTargetUid = gExProtocol.receive.PushDeviceFallDischargeCmd.TargetUid;
                //
                //获取所有在线从机信息
				userSetting->DeviceCtrlCmdState    = gExProtocol.receive.PushDeviceFallDischargeCmd.DeviceWorkState;
                gCh2ManageCtr.IsHaveNewRealData    = FALSE;
                gCh3ManageCtr.IsHaveNewRealData    = FALSE;
                gCh4ManageCtr.IsHaveNewRealData    = FALSE;
                userSetting->IsBatVolSamping       = TRUE; 
                NCExMakeAckDeviceFallDischargeCmd(userSetting->SettingTargetUid,userSetting->DeviceCtrlCmdState,NC_COMM_ACK_OK);
                userSetting->IdleTimeStamp         = 200;
                userSetting->TestedCellSetVolt     = gExProtocol.receive.PushDeviceFallDischargeCmd.TestedCellSetVolt;
                userSetting->Cell_Volt_Lower       = gExProtocol.receive.PushDeviceFallDischargeCmd.Cell_Volt_Lower;
                userSetting->TestedCellType        = gExProtocol.receive.PushDeviceFallDischargeCmd.TestedCellType;                  
                vTaskDelay(100);      //让数据发出 
                 
                gCh1ManageCtr.TestedCellSetCurrent = 0;
                gCh2ManageCtr.TestedCellSetCurrent = 0;
                gCh3ManageCtr.TestedCellSetCurrent = 0;
                gCh4ManageCtr.TestedCellSetCurrent = 0;                 
                 
                //if(userSetting->SettingTargetUid&0x01)
                {
                    gCh1ManageCtr.TestedCellSetCurrent = gExProtocol.receive.PushDeviceFallDischargeCmd.TestedCellSetCh1Current;
                    if(gCh1ManageCtr.TestedCellSetCurrent == 0)    userSetting->SettingTargetUid &= ~0x01; 
                }
                //if(userSetting->SettingTargetUid&0x02) 
                {                    
                    NCEms02MakeAskDeviceRunRealData(NC_READ_DATA_IDLE_VC_STATE);
                    gCh2ManageCtr.TestedCellSetCurrent = gExProtocol.receive.PushDeviceFallDischargeCmd.TestedCellSetCh2Current;
                    if(gCh2ManageCtr.TestedCellSetCurrent == 0)    userSetting->SettingTargetUid &= ~0x02; 
                }
                //if(userSetting->SettingTargetUid&0x04)    
                {
                    NCEms03MakeAskDeviceRunRealData(NC_READ_DATA_IDLE_VC_STATE);
                    gCh3ManageCtr.TestedCellSetCurrent = gExProtocol.receive.PushDeviceFallDischargeCmd.TestedCellSetCh3Current;
                    if(gCh3ManageCtr.TestedCellSetCurrent == 0)    userSetting->SettingTargetUid &= ~0x04; 
                }
                //if(userSetting->SettingTargetUid&0x08)   
                {                    
                    NCEms04MakeAskDeviceRunRealData(NC_READ_DATA_IDLE_VC_STATE); 
                    gCh4ManageCtr.TestedCellSetCurrent = gExProtocol.receive.PushDeviceFallDischargeCmd.TestedCellSetCh4Current; 
                    if(gCh4ManageCtr.TestedCellSetCurrent == 0)    userSetting->SettingTargetUid &= ~0x08;                 
                }                    
                vTaskDelay(250);      //读取数据 需要退出此函数  
                             
                userSetting->DeviceWorkModeType    = NC_WORK_MODE_DISCING;/*放电*/
                userSetting->DeviceRunState        = NC_ON_STATE;
                                 
                gCh1ManageCtr.CellCapacityAH       = 0;//重新启动容量复位
                gCh2ManageCtr.CellCapacityAH       = 0;//重新启动容量复位
                gCh3ManageCtr.CellCapacityAH       = 0;//重新启动容量复位
                gCh4ManageCtr.CellCapacityAH       = 0;//重新启动容量复位             
             }
             else if(userSetting->DeviceWorkModeType == NC_WORK_MODE_DISCING  && \
                     (userSetting->DeviceCtrlCmdState ==  gExProtocol.receive.PushDeviceFallDischargeCmd.DeviceWorkState))//HANDLE_STATE_NFINISH_FIXC_START  HANDLE_STATE_NFINISH_AUC_START
             {
                 //收到指令已经处于此模式内
                 if(HANDLE_STATE_NFINISH_FIXC_START == userSetting->DeviceCtrlCmdState)
                 {
                    //if(gExProtocol.receive.PushDeviceFallDischargeCmd.TargetUid & 0x01) 
                    {
                        gCh1ManageCtr.IsOverAgainStartup =FALSE;
                        gCh1ManageCtr.IsReachCurrent    = FALSE;
                        
                        gCh1ManageCtr.TestedCellSetCurrent = gExProtocol.receive.PushDeviceFallDischargeCmd.TestedCellSetCh1Current; 
                        
                        if(gCh1ManageCtr.CellStateData  == NC_DISCING_STATE)
                        {
                            if(gCh1ManageCtr.TestedCellSetCurrent == 0)
                            {
                                gCh1ManageCtr.CellStateData       = NC_DISCING_FIXC_NOC_STATE;//NC_IDLE_STATE;
                                userSetting->SettingTargetUid     &= ~0x01;
                                NCBC_DI_Adjust_Set(0);
                                gCh1ManageCtr.CurrentconfigCurrent = 0;
                                gCh1ManageCtr.SetDischargeCurrent  = 0;
                                gCh1ManageCtr.CurrentDispCurrent   = 0; 
                                //K_CTR_PIN_DISABLE;     // 开启回路开关
                            }
                            else
                            {
                                NC_FallDischargeChange(&gCh1ManageCtr);
                            }
                        }
                        else
                        {
                            if(gCh1ManageCtr.TestedCellSetCurrent != 0 && gCh1ManageCtr.CellStateData !=NC_DISCONNECT_STATE)
                            {
                                gCh1ManageCtr.CellStateData       = NC_DISCING_STATE;
                                userSetting->SettingTargetUid     |= 0x01;
                                K_CTR_PIN_ENABLE;     // 开启回路开关
                                NC_FallDischargeReStart(&gCh1ManageCtr);
                            }                        
                        
                        }                                                
                    }
                    //if(gExProtocol.receive.PushDeviceFallDischargeCmd.TargetUid & 0x02) 
                    {
                        gCh2ManageCtr.IsOverAgainStartup =FALSE;
                        gCh2ManageCtr.IsReachCurrent    = FALSE;
                        gCh2ManageCtr.TestedCellSetCurrent = gExProtocol.receive.PushDeviceFallDischargeCmd.TestedCellSetCh2Current; 
                        if(gCh2ManageCtr.CellStateData  == NC_DISCING_STATE)
                        {
                            if(gCh2ManageCtr.TestedCellSetCurrent == 0)
                            {
                                gCh2ManageCtr.CellStateData       = NC_DISCING_FIXC_NOC_STATE;//NC_IDLE_STATE;
                                userSetting->SettingTargetUid     &= ~0x02;
                                NCEms02MakePushDeviceWorkCommand(NC_CTR_WORK_MODE_START,0); 
                                gCh2ManageCtr.CurrentconfigCurrent = 0;
                                gCh2ManageCtr.SetDischargeCurrent  = 0;   
                                gCh2ManageCtr.CurrentDispCurrent   = 0;                                
                            }
                            else
                            {
                                NC_FallDischargeChange(&gCh2ManageCtr);
                            }
                        }
                        else
                        {
                            if(gCh2ManageCtr.TestedCellSetCurrent != 0 && gCh2ManageCtr.CellStateData !=NC_DISCONNECT_STATE)
                            {
                                gCh2ManageCtr.CellStateData       = NC_DISCING_STATE;
                                userSetting->SettingTargetUid     |= 0x02;
                                NC_FallDischargeReStart(&gCh2ManageCtr);
                            }                        
                        
                        }                        
                    }
                    //if(gExProtocol.receive.PushDeviceFallDischargeCmd.TargetUid & 0x04) 
                    {
                        gCh3ManageCtr.IsOverAgainStartup =FALSE;
                        gCh3ManageCtr.IsReachCurrent    = FALSE;
                        gCh3ManageCtr.TestedCellSetCurrent = gExProtocol.receive.PushDeviceFallDischargeCmd.TestedCellSetCh3Current; 
                        if(gCh3ManageCtr.CellStateData  == NC_DISCING_STATE)
                        {
                            if(gCh3ManageCtr.TestedCellSetCurrent == 0)
                            {
                                gCh3ManageCtr.CellStateData       = NC_DISCING_FIXC_NOC_STATE;//NC_IDLE_STATE;
                                userSetting->SettingTargetUid     &= ~0x04;
                                NCEms03MakePushDeviceWorkCommand(NC_CTR_WORK_MODE_START,0); 
                                gCh3ManageCtr.CurrentconfigCurrent = 0;
                                gCh3ManageCtr.SetDischargeCurrent  = 0; 
                                gCh3ManageCtr.CurrentDispCurrent   = 0;  
                            }
                            else
                            {
                                NC_FallDischargeChange(&gCh3ManageCtr);
                            }
                        }
                        else
                        {
                            if(gCh3ManageCtr.TestedCellSetCurrent != 0 && gCh3ManageCtr.CellStateData !=NC_DISCONNECT_STATE)
                            {
                                gCh3ManageCtr.CellStateData       = NC_DISCING_STATE;
                                userSetting->SettingTargetUid     |= 0x04;
                                NC_FallDischargeReStart(&gCh3ManageCtr);
                            }                        
                        
                        }                        
                    }
                    //if(gExProtocol.receive.PushDeviceFallDischargeCmd.TargetUid & 0x08) 
                    {
                        gCh4ManageCtr.IsOverAgainStartup =FALSE;
                        gCh4ManageCtr.IsReachCurrent    = FALSE;
                        gCh4ManageCtr.TestedCellSetCurrent = gExProtocol.receive.PushDeviceFallDischargeCmd.TestedCellSetCh4Current; 
                        if(gCh4ManageCtr.CellStateData  == NC_DISCING_STATE)
                        {
                            if(gCh4ManageCtr.TestedCellSetCurrent == 0)
                            {
                                gCh4ManageCtr.CellStateData       = NC_DISCING_FIXC_NOC_STATE;//NC_IDLE_STATE;
                                userSetting->SettingTargetUid     &= ~0x08;
                                NCEms04MakePushDeviceWorkCommand(NC_CTR_WORK_MODE_START,0); 
                                gCh4ManageCtr.CurrentconfigCurrent = 0;
                                gCh4ManageCtr.SetDischargeCurrent  = 0; 
                                gCh4ManageCtr.CurrentDispCurrent   = 0; 
                            }
                            else
                            {
                                NC_FallDischargeChange(&gCh4ManageCtr);
                            }
                        }
                        else
                        {
                            if(gCh4ManageCtr.TestedCellSetCurrent != 0 && gCh4ManageCtr.CellStateData !=NC_DISCONNECT_STATE)
                            {
                                gCh4ManageCtr.CellStateData       = NC_DISCING_STATE;
                                userSetting->SettingTargetUid     |= 0x08;
                                NC_FallDischargeReStart(&gCh4ManageCtr);
                            }                        
                        
                        }                         
                    } 
                    NCExMakeAckDeviceFallDischargeCmd(gExProtocol.receive.PushDeviceFallDischargeCmd.TargetUid,userSetting->DeviceCtrlCmdState,NC_COMM_ACK_OK);
                    if(userSetting->SettingTargetUid == 0)
                    {
                      //风扇 充电关闭
                      //关闭风扇5S
                        userSetting->FanCtrDuty   =0;
                        userSetting->IsFanChange  = 0;
                        NC_Fan_PWM_Duty_Set(userSetting->FanCtrDuty);
                        //关闭CHG  
                        BAT_CHG_CTR_DISABLE;//关闭CHG                    
                        userSetting->IsLocalBatChging = FALSE; 
                        userSetting->MasterStatus &= ~MASTER_STATE_BAT_CHG;
                        userSetting->IsEnableBatBoost = TRUE;
                        BAT_UPVOLT_CTR_ENABLE;                    
                    }                                  
                 }
                 else
                 {
                     //在正常的模式下 返回失败
                     NCExMakeAckDeviceFallDischargeCmd(gExProtocol.receive.PushDeviceFallDischargeCmd.TargetUid,\
				                             gExProtocol.receive.PushDeviceFallDischargeCmd.DeviceWorkState,NC_COMM_CMD_ER);	
                 }                 
             }
             else
             {
                //指令错误 状态不一致
				NCExMakeAckDeviceFallDischargeCmd(gExProtocol.receive.PushDeviceFallDischargeCmd.TargetUid,\
				                             gExProtocol.receive.PushDeviceFallDischargeCmd.DeviceWorkState,NC_COMM_CMD_ER);			             
             }                         				
		}
        else 
        {			 
	       //指令错误 状态不一致
           NCExMakeAckDeviceFallDischargeCmd(gExProtocol.receive.PushDeviceFallDischargeCmd.TargetUid,HANDLE_STATE_RESTART,NC_COMM_CMD_ER);			 				 				 				 			 									             
        }
    }
    //只能初次启动才需要如此操作
    if(userSetting->DeviceRunState == NC_ON_STATE)
    {
        vTaskDelay(100);
        //工作过程不允许蓝牙配网
        userSetting->AP_NetInfo.IsBleWaitConfig = FALSE;
        if(userSetting->AP_NetInfo.IsBluetoothEnable)
        {
          userSetting->AP_NetInfo.IsColseBleCofig = TRUE;
        }
    } 
}
//========================================================================================================
//========================================================================================================

void NCExMakeAckDeviceCtrPowerCommand(UINT8 TargetUid,UINT8 DeviceCtrState,UINT8 IsSuccess)
{
    NCUserSetting* userSetting = NCUserSettingGet();
    if(FALSE == NCSemaphoreTake(gMutexWlanPackage))  return;
    
    memcpy(&(gExProtocol.upload.AckDeviceCtrPowerCommand.DeviceSN),        userSetting->deviceSN,      sizeof(userSetting->deviceSN)); 
    
    gExProtocol.upload.AckDeviceCtrPowerCommand.TargetUid      = TargetUid;
    gExProtocol.upload.AckDeviceCtrPowerCommand.DeviceCtrState = DeviceCtrState;

    gExProtocol.upload.AckDeviceCtrPowerCommand.IsSuccess      = IsSuccess;

    
	//把数据发送出去
    NCInfoWlanParamSet(g_eD2App_AckDeviceCtrPowerCommand, (UINT8*)&gExProtocol.upload, sizeof(stuD2App_AckDeviceCtrPowerCommand));   
    NCSemaphoreGive(gMutexWlanPackage); // 
}

//============================================================================================
//============================================================================================
void NCExUnMakePushDeviceCtrPowerCommand(UINT8 *recbuff)
{
    NCUserSetting* userSetting = NCUserSettingGet();
    
    memcpy(&gExProtocol.receive.PushDeviceCtrPowerCommand, recbuff, sizeof(stuApp2D_PushDeviceCtrPowerCommand));
    if(gExProtocol.receive.PushDeviceCtrPowerCommand.DeviceCtrState == 0x00)//关闭
    {
         if(gExProtocol.receive.PushDeviceCtrPowerCommand.TargetUid<0x10)
        {
            userSetting->DevicePowerCtrState &= ~(gExProtocol.receive.PushDeviceCtrPowerCommand.TargetUid);
            NC_BC_PowerContrl_Config(userSetting->DevicePowerCtrState);
            NCExMakeAckDeviceCtrPowerCommand(gExProtocol.receive.PushDeviceCtrPowerCommand.TargetUid,gExProtocol.receive.PushDeviceCtrPowerCommand.DeviceCtrState,NC_COMM_ACK_OK);
        }
        else
        {
            NCExMakeAckDeviceCtrPowerCommand(gExProtocol.receive.PushDeviceCtrPowerCommand.TargetUid,gExProtocol.receive.PushDeviceCtrPowerCommand.DeviceCtrState,NC_COMM_ACK_FL);
        }
    }
    else if(gExProtocol.receive.PushDeviceCtrPowerCommand.DeviceCtrState == 0x01)//开启
    {
        if(gExProtocol.receive.PushDeviceCtrPowerCommand.TargetUid<0x10)
        {
            userSetting->DevicePowerCtrState |= (gExProtocol.receive.PushDeviceCtrPowerCommand.TargetUid);
            NC_BC_PowerContrl_Config(userSetting->DevicePowerCtrState);
            NCExMakeAckDeviceCtrPowerCommand(gExProtocol.receive.PushDeviceCtrPowerCommand.TargetUid,gExProtocol.receive.PushDeviceCtrPowerCommand.DeviceCtrState,NC_COMM_ACK_OK);
        }
        else
        {
            NCExMakeAckDeviceCtrPowerCommand(gExProtocol.receive.PushDeviceCtrPowerCommand.TargetUid,gExProtocol.receive.PushDeviceCtrPowerCommand.DeviceCtrState,NC_COMM_ACK_FL);
        }
    }
    else if(gExProtocol.receive.PushDeviceCtrPowerCommand.DeviceCtrState == 0x02)//读取
    {
        NCExMakeAckDeviceCtrPowerCommand(gExProtocol.receive.PushDeviceCtrPowerCommand.TargetUid,userSetting->DevicePowerCtrState,NC_COMM_ACK_OK);
    }
    
}
//========================================================================================================
void NCExMakeAckDevicePowerOffCmd(UINT8 DeviceCtrState,UINT8 IsSuccess)
{
    NCUserSetting* userSetting = NCUserSettingGet();
    if(FALSE == NCSemaphoreTake(gMutexWlanPackage))  return;
    
    memcpy(&(gExProtocol.upload.AckDevicePowerOffCmd.DeviceSN),        userSetting->deviceSN,      sizeof(userSetting->deviceSN)); 
    
    gExProtocol.upload.AckDevicePowerOffCmd.DeviceCtrState = DeviceCtrState;
    gExProtocol.upload.AckDevicePowerOffCmd.IsSuccess      = IsSuccess;

	//把数据发送出去
    NCInfoWlanParamSet(g_eD2App_AckDevicePowerOffCmd, (UINT8*)&gExProtocol.upload, sizeof(stuD2App_AckDevicePowerOffCmd));   
    NCSemaphoreGive(gMutexWlanPackage); // 
}

//============================================================================================
//============================================================================================
void NCExUnMakePushDevicePowerOffCmd(UINT8 *recbuff)
{
    NCUserSetting* userSetting = NCUserSettingGet();
    NCFirmwarePackageStr * firmwarePackage = NCFirmwarePackageGet();
    NCSlaveFirmwareStu *  NCSlaveFirmwareInfo = NCSlaveFirmwareInfoGet();
    memcpy(&gExProtocol.receive.PushDevicePowerOffCmd, recbuff, sizeof(stuApp2D_PushDevicePowerOffCmd));
    if(gExProtocol.receive.PushDevicePowerOffCmd.DeviceCtrState == 0x00)//无操作
    {
            NCExMakeAckDevicePowerOffCmd(gExProtocol.receive.PushDevicePowerOffCmd.DeviceCtrState,NC_COMM_ACK_FL);

    }
    else if(gExProtocol.receive.PushDevicePowerOffCmd.DeviceCtrState == 0x01)//关机
    {
        //在放电过程中  主机 或者从机  升级中 禁止关机
        if((NC_DISCING_STATE != userSetting->DeviceRunState) && (FALSE==NCSlaveFirmwareInfo->IsSlaveUpdataFirm)  && (FALSE==firmwarePackage->IsFirmwareUpdataRequest))
        {
            NCExMakeAckDevicePowerOffCmd(gExProtocol.receive.PushDevicePowerOffCmd.DeviceCtrState,NC_COMM_ACK_OK); 
            vTaskDelay(2100);
            //userSetting->DevicePowerCtrState = 0x00;
            //NC_BC_PowerContrl_Config(userSetting->DevicePowerCtrState);
            userSetting->UsbForcedPowerDown = TRUE;//强制关机
        
        }
        else
        {
            NCExMakeAckDevicePowerOffCmd(gExProtocol.receive.PushDevicePowerOffCmd.DeviceCtrState,NC_COMM_ACK_FL); 
        }

    }  
}
//========================================================================================================
//读取老化信息
void NCExUnMakeAskDevAgingInfoCmd(void)
{
    uint16_t  temp;
    NCUserSetting* userSetting = NCUserSettingGet();
    
    temp =  NCFlash_RdDevAgingInfo();
    
    if(FALSE == NCSemaphoreTake(gMutexWlanPackage))  return;
    
    memcpy(&(gExProtocol.upload.AckDevAgingInfoCmd.DeviceSN),        userSetting->deviceSN,      sizeof(userSetting->deviceSN)); 
    memcpy(&(gExProtocol.upload.AckDevAgingInfoCmd.DevTimeStamp),    userSetting->DevTimeStamp,  sizeof(userSetting->DevTimeStamp)); 
    
    gExProtocol.upload.AckDevAgingInfoCmd.IsComplete    = (UINT8)(temp>>8);
    gExProtocol.upload.AckDevAgingInfoCmd.CompleteInfo  = (UINT8)temp;
	//把数据发送出去
    NCInfoWlanParamSet(g_eD2App_AckDevAgingInfoCmd, (UINT8*)&gExProtocol.upload, sizeof(stuD2App_AckDevAgingInfoCmd));   
    NCSemaphoreGive(gMutexWlanPackage); //  
}

//========================================================================================================
#if   0
void  NCExMakeAckDeviceCaliParam(UINT8    TargetUid,NC_CALI_ACK_TYPE IsSuccess,struct _NCMSComProtocol *MSComUnionProtocol) 
{
    NCUserSetting* userSetting = NCUserSettingGet();
    NC_CaliDataType * userCaliData = NCCaliDataGet();
    if(FALSE == NCSemaphoreTake(gMutexWlanPackage)) return;
    gExProtocol.upload.AckDeviceCaliParam.IsSuccess             = IsSuccess;  
    gExProtocol.upload.AckDeviceCaliParam.TargetUid             = TargetUid;
    
    gExProtocol.upload.AckDeviceCaliParam.BV_Cali_K_Value       = MSComUnionProtocol->receive.AckDeviceCaliParam.BV_Cali_K_Value;
    gExProtocol.upload.AckDeviceCaliParam.BV_Cali_B_Value       = MSComUnionProtocol->receive.AckDeviceCaliParam.BV_Cali_B_Value;
    
    gExProtocol.upload.AckDeviceCaliParam.DI_Cali_K_Value       = MSComUnionProtocol->receive.AckDeviceCaliParam.DI_Cali_K_Value;
    gExProtocol.upload.AckDeviceCaliParam.DI_Cali_B_Value       = MSComUnionProtocol->receive.AckDeviceCaliParam.DI_Cali_B_Value;
    
    gExProtocol.upload.AckDeviceCaliParam.DI_PWM_Cali_K_Value   = MSComUnionProtocol->receive.AckDeviceCaliParam.DI_PWM_Cali_K_Value;
    gExProtocol.upload.AckDeviceCaliParam.DI_PWM_Cali_B_Value   = MSComUnionProtocol->receive.AckDeviceCaliParam.DI_PWM_Cali_B_Value;
    
	//把数据发送出去
    NCInfoWlanParamSet(g_eD2App_AckDeviceCaliParam, (UINT8*)&gExProtocol.upload, sizeof(stuD2App_AckDeviceCaliParam));   
    NCSemaphoreGive(gMutexWlanPackage); //
}


//======================================================================================================================
//======================================================================================================================
void NCExUnMakePushDeviceCaliParam(UINT8 *recbuff)
{
    NC_CaliDataType * userCaliData = NCCaliDataGet();
    struct _NCMSComProtocol *MSComUnionProtocol  = GetMSComUnionProtocol();
    memcpy(&gExProtocol.receive.PushDeviceCaliParam, recbuff, sizeof(stuApp2D_PushDeviceCaliParam));
    
    MSComUnionProtocol->upload.PushDeviceCaliParam.Handle_Type         = gExProtocol.receive.PushDeviceCaliParam.Handle_Type;
    MSComUnionProtocol->upload.PushDeviceCaliParam.BV_Cali_K_Value     = gExProtocol.receive.PushDeviceCaliParam.BV_Cali_K_Value; 
    MSComUnionProtocol->upload.PushDeviceCaliParam.BV_Cali_B_Value     = gExProtocol.receive.PushDeviceCaliParam.BV_Cali_B_Value; 
            
    MSComUnionProtocol->upload.PushDeviceCaliParam.DI_Cali_K_Value     = gExProtocol.receive.PushDeviceCaliParam.DI_Cali_K_Value; 
    MSComUnionProtocol->upload.PushDeviceCaliParam.DI_Cali_B_Value     = gExProtocol.receive.PushDeviceCaliParam.DI_Cali_B_Value; 
            
    MSComUnionProtocol->upload.PushDeviceCaliParam.DI_PWM_Cali_K_Value = gExProtocol.receive.PushDeviceCaliParam.DI_PWM_Cali_K_Value; 
    MSComUnionProtocol->upload.PushDeviceCaliParam.DI_PWM_Cali_B_Value = gExProtocol.receive.PushDeviceCaliParam.DI_PWM_Cali_B_Value;  
    
    MSComUnionProtocol->upload.PushDeviceCaliParam.WTI_Cali_K_Value = gExProtocol.receive.PushDeviceCaliParam.WTI_Cali_K_Value; 
    MSComUnionProtocol->upload.PushDeviceCaliParam.WTI_Cali_B_Value = gExProtocol.receive.PushDeviceCaliParam.WTI_Cali_B_Value;  
    
    if(gExProtocol.receive.PushDeviceCaliParam.TargetUid ==0x01)
    {
    
       if(gExProtocol.receive.PushDeviceCaliParam.Handle_Type == NC_CALI_HANDLE_WR)//写入
        {
            userCaliData->BV_Cali_K_Value      = gExProtocol.receive.PushDeviceCaliParam.BV_Cali_K_Value;  
            userCaliData->BV_Cali_B_Value      = gExProtocol.receive.PushDeviceCaliParam.BV_Cali_B_Value; 
           
            userCaliData->DI_Cali_K_Value      = gExProtocol.receive.PushDeviceCaliParam.DI_Cali_K_Value;
            userCaliData->DI_Cali_B_Value      = gExProtocol.receive.PushDeviceCaliParam.DI_Cali_B_Value;       

            userCaliData->DI_PWM_Cali_K_Value  = gExProtocol.receive.PushDeviceCaliParam.DI_PWM_Cali_K_Value;
            userCaliData->DI_PWM_Cali_B_Value  = gExProtocol.receive.PushDeviceCaliParam.DI_PWM_Cali_B_Value;

            userCaliData->WTI_Cali_K_Value     = gExProtocol.receive.PushDeviceCaliParam.WTI_Cali_K_Value;
            userCaliData->WTI_Cali_B_Value     = gExProtocol.receive.PushDeviceCaliParam.WTI_Cali_B_Value;            
            
            NCFlash_WriteDeviceCaliParam();//写入  
            NCFlash_ReadDeviceCaliParam();//读取
            //对比写入是否正确  memcmp不会在\0处停下来!!!
            MSComUnionProtocol->receive.AckDeviceCaliParam.BV_Cali_K_Value = userCaliData->BV_Cali_K_Value; 
            MSComUnionProtocol->receive.AckDeviceCaliParam.BV_Cali_B_Value = userCaliData->BV_Cali_B_Value; 
            
            MSComUnionProtocol->receive.AckDeviceCaliParam.DI_Cali_K_Value = userCaliData->DI_Cali_K_Value; 
            MSComUnionProtocol->receive.AckDeviceCaliParam.DI_Cali_B_Value = userCaliData->DI_Cali_B_Value; 
            
            MSComUnionProtocol->receive.AckDeviceCaliParam.DI_PWM_Cali_K_Value = userCaliData->DI_PWM_Cali_K_Value; 
            MSComUnionProtocol->receive.AckDeviceCaliParam.DI_PWM_Cali_B_Value = userCaliData->DI_PWM_Cali_B_Value;  
            
            MSComUnionProtocol->receive.AckDeviceCaliParam.WTI_Cali_K_Value    = userCaliData->WTI_Cali_K_Value; 
            MSComUnionProtocol->receive.AckDeviceCaliParam.WTI_Cali_B_Value    = userCaliData->WTI_Cali_B_Value; 
            
            if(0 == memcmp(recbuff+1,&(userCaliData->BV_Cali_K_Value),32))//相等
            {
                NCExMakeAckDeviceCaliParam(0x01,NC_CALI_ACK_OK,MSComUnionProtocol);
            }
            else
            {
                NCExMakeAckDeviceCaliParam(0x01,NC_CALI_ACK_FL,MSComUnionProtocol);
            }
           vTaskDelay(200); //防止频繁操作       
            
        } 
       else if(gExProtocol.receive.PushDeviceCaliParam.Handle_Type == NC_CALI_HANDLE_RD)//读取
       {
            NCFlash_ReadDeviceCaliParam();
            MSComUnionProtocol->receive.AckDeviceCaliParam.BV_Cali_K_Value = userCaliData->BV_Cali_K_Value; 
            MSComUnionProtocol->receive.AckDeviceCaliParam.BV_Cali_B_Value = userCaliData->BV_Cali_B_Value; 
            
            MSComUnionProtocol->receive.AckDeviceCaliParam.DI_Cali_K_Value = userCaliData->DI_Cali_K_Value; 
            MSComUnionProtocol->receive.AckDeviceCaliParam.DI_Cali_B_Value = userCaliData->DI_Cali_B_Value; 
            
            MSComUnionProtocol->receive.AckDeviceCaliParam.DI_PWM_Cali_K_Value = userCaliData->DI_PWM_Cali_K_Value; 
            MSComUnionProtocol->receive.AckDeviceCaliParam.DI_PWM_Cali_B_Value = userCaliData->DI_PWM_Cali_B_Value;  
           
            MSComUnionProtocol->receive.AckDeviceCaliParam.WTI_Cali_K_Value    = userCaliData->WTI_Cali_K_Value; 
            MSComUnionProtocol->receive.AckDeviceCaliParam.WTI_Cali_B_Value    = userCaliData->WTI_Cali_B_Value;  
                      
            NCExMakeAckDeviceCaliParam(0x01,NC_CALI_ACK_RD,MSComUnionProtocol);
       } 
       else
       {
          NCExMakeAckDeviceCaliParam(0x01,NC_CALI_ACK_EER,MSComUnionProtocol);
       }  
    }
    else if(gExProtocol.receive.PushDeviceCaliParam.TargetUid ==0x02)
    {
         NCEms02MakePushDeviceCaliParam();    
    }
    else if(gExProtocol.receive.PushDeviceCaliParam.TargetUid ==0x04)
    {
         NCEms03MakePushDeviceCaliParam();
    }  
    else if(gExProtocol.receive.PushDeviceCaliParam.TargetUid ==0x08)
    {
        NCEms04MakePushDeviceCaliParam(); 
    }            
}
#endif


////===========================================================================
//void  NCExMakeAckUpdataSnInfo(NC_COMM_ACK_TYPE IsSuccess) 
//{
//    NCUserSetting* userSetting = NCUserSettingGet();
//    if(FALSE == NCSemaphoreTake(gMutexWlanPackage)) return;
//    gExProtocol.upload.AckUpdataSN_Info.IsSuccess  = IsSuccess;  
//	//把数据发送出去
//    NCInfoWlanParamSet(g_eD2App_AckWitreSN, (UINT8*)&gExProtocol.upload, sizeof(stuD2App_AckUpdataSN_Info));   
//    NCSemaphoreGive(gMutexWlanPackage); //
//}
////========================================================================
//void  NCExUnMakePushWitreSN(UINT8 *recbuff,UINT16 Len) 
//{
//    NCUserSetting* userSetting = NCUserSettingGet();
//    //模组设备序列号写入指令

//    if(/*(0==memcmp(DEFAULT_DEVICE_SN, databuf+1,6)) && */(Len >= DEVICE_SN_LENGTH))
//    {
//        if(0==memcmp(userSetting->deviceSN, recbuff,DEVICE_SN_LENGTH))
//        {
//           //相同不处理直接回复OK
//           NCExMakeAckUpdataSnInfo(NC_COMM_ACK_OK);  
//        }
//        else
//        {
//           memset(userSetting->deviceSN,0,sizeof(userSetting->deviceSN));
//           //snprintf(userSetting->deviceSN,(DEVICE_SN_LENGTH),"%s",recbuff); 
//           memcpy(&userSetting->deviceSN, recbuff, DEVICE_SN_LENGTH); 
//           NCFlash_WriteDeviceSN();
//           NCFlashReadDeviceSN();
//           if(0==memcmp(userSetting->deviceSN, recbuff,DEVICE_SN_LENGTH))//读取写入对比是否正确写入
//           {
//                NCExMakeAckUpdataSnInfo(NC_COMM_ACK_OK); 
//           }
//           else 
//           {
//                NCExMakeAckUpdataSnInfo(NC_COMM_ACK_FL); 
//           }
//        }                    
//    }
//    else
//    {
//       NCExMakeAckUpdataSnInfo(NC_COMM_ACK_FL);     
//    }
//    
//}

//===============================================================================================
//请求读取模组校准过程中数据
//===============================================================================================
//----------------------------------------
//    CALI_TEST_DATA_STATE_NOLMAL   = 0x00,
//    CALI_TEST_DATA_STATE_NO_CUR   = 0x01,
//    CALI_TEST_DATA_STATE_NO_VOLT  = 0x02,
//    CALI_TEST_DATA_STATE_NO_12V   = 0x03,
//    CALI_TEST_DATA_STATE_FIN_ERR  = 0x04,

#if       0
void  NCExMakeAckDeviceCalingData(UINT8    TargetUid,CALI_TEST_DATA_STATE_TYPE IsSuccess)
{
    NC_CaliTestingData* caliTestingData = NCCaliTestingDataGet();
    struct _NCMSComProtocol *MSComUnionProtocol  = GetMSComUnionProtocol();
    
    if(FALSE == NCSemaphoreTake(gMutexWlanPackage)) return;
    memset(&(gExProtocol.upload.AckDeviceCalingData),0,sizeof(stuD2App_AckDeviceCalingData));
    
    gExProtocol.upload.AckDeviceCalingData.TargetUid =  TargetUid;
    gExProtocol.upload.AckDeviceCalingData.IsSuccess =  IsSuccess;
    if(IsSuccess ==CALI_TEST_DATA_STATE_NOLMAL)
    {
        if(TargetUid == 0x01)
        {
          gExProtocol.upload.AckDeviceCalingData.SettingType   = caliTestingData->SettingType;
          gExProtocol.upload.AckDeviceCalingData.OrigContrlPWM = caliTestingData->OrigContrlPWM;
          gExProtocol.upload.AckDeviceCalingData.OrigCur       = caliTestingData->OrigCur;
          gExProtocol.upload.AckDeviceCalingData.OrigVolt      = caliTestingData->OrigVolt;           
        }
        else// if(TargetUid == 0x02)  //MSComUnionProtocol
        {
            gExProtocol.upload.AckDeviceCalingData.OrigContrlPWM   = MSComUnionProtocol->receive.AckDeviceCalingData.OrigContrlPWM;
            gExProtocol.upload.AckDeviceCalingData.OrigCur         = MSComUnionProtocol->receive.AckDeviceCalingData.OrigCur;
            gExProtocol.upload.AckDeviceCalingData.OrigVolt        = MSComUnionProtocol->receive.AckDeviceCalingData.OrigVolt;
            gExProtocol.upload.AckDeviceCalingData.SettingType     = MSComUnionProtocol->receive.AckDeviceCalingData.SettingType;
        }
    }
    else
    {
       if(TargetUid == 0x01)
       {
           gExProtocol.upload.AckDeviceCalingData.SettingType   = caliTestingData->SettingType;
       }
       else
       {
          gExProtocol.upload.AckDeviceCalingData.SettingType   = MSComUnionProtocol->receive.AckDeviceCalingData.SettingType;
       }
    }
    NCInfoWlanParamSet(g_eD2App_AckDeviceCalingData, (UINT8*)&gExProtocol.upload, sizeof(stuD2App_AckDeviceCalingData));  
    NCSemaphoreGive(gMutexWlanPackage); 
}
//===============================================================================================
void  NCExUnMakeAskDeviceCalingData(UINT8 *recbuff)
{
    NC_CaliTestingData* caliTestingData = NCCaliTestingDataGet();
    memcpy(&gExProtocol.receive.AskDeviceCalingData,recbuff,sizeof(stuApp2D_AskDeviceCalingData));
    
    if(gExProtocol.receive.AskDeviceCalingData.TargetUid == 0x01)
    {
        caliTestingData->IsHaveValidData = TRUE;//
        if(caliTestingData->IsHaveValidData) 
        {
          caliTestingData->OrigCur  = ADC_MV_CONV(DI_ADC_VAL);
          caliTestingData->OrigVolt = ADC_MV_CONV(BV_ADC_VAL);
          NCExMakeAckDeviceCalingData(0x01,CALI_TEST_DATA_STATE_NOLMAL);
        }
        else
        {
          NCExMakeAckDeviceCalingData(0x01,CALI_TEST_DATA_STATE_NO_DATA);
        }    
    }
    else if(gExProtocol.receive.AskDeviceCalingData.TargetUid == 0x02)
    {
        NCEms02MakeAskDeviceCalingData(0x01);
    }
    else if(gExProtocol.receive.AskDeviceCalingData.TargetUid == 0x04)
    {
        NCEms03MakeAskDeviceCalingData(0x01);    
    }  
    else if(gExProtocol.receive.AskDeviceCalingData.TargetUid == 0x08)
    {
        NCEms04MakeAskDeviceCalingData(0x01);    
    }     
}

#endif


//=========================================================================================
//=========================================================================================
//=========================================================================================  
//=========================================================================================
//=========================================================================================
void NCExMakeAckFirmwareUpdataInfo(UINT8 DeviceAck)
{
    NCUserSetting* userSetting = NCUserSettingGet();
    if(FALSE == NCSemaphoreTake(gMutexWlanPackage)) return;
    memcpy(&(gExProtocol.upload.WIFIAckFirmwareUpdataInfo.DeviceSN),        userSetting->deviceSN,      sizeof(userSetting->deviceSN)); 
    gExProtocol.upload.WIFIAckFirmwareUpdataInfo.DeviceAckInfo  = DeviceAck; 
	//把数据发送出去
    NCInfoWlanParamSet(g_eD2App_AckFirmwareUpdataInfo, (UINT8*)&gExProtocol.upload, sizeof(stuD2App_WIFIAckFirmwareUpdataInfo));   
    NCSemaphoreGive(gMutexWlanPackage);    
}

//=========================================================================================
//=========================================================================================
void NCExUnMakePushFirmwareUpdataInfo(UINT8 *recbuff)
{
   UINT32  Version =0;
   NCUserSetting* userSetting = NCUserSettingGet();
   NCFirmwarePackageStr * firmwarePackage = NCFirmwarePackageGet();
   UINT8    Firmware_PackageTotalMAX;
   UINT32   Firmware_FileSizeMAX;
   memcpy(&gExProtocol.receive.PushFirmwareUpdataInfo, recbuff, sizeof(stuApp2D_PushFirmwareUpdataInfo));
   
   //
    memcpy(&Version,gExProtocol.receive.PushFirmwareUpdataInfo.Firmware_Version,3);
   //Version = gExProtocol.receive.PushFirmwareUpdataInfo.Firmware_Version[2];
  // Version += ((UINT32)(gExProtocol.receive.PushFirmwareUpdataInfo.Firmware_Version[1])<<8); 
   //Version += ((UINT32)(gExProtocol.receive.PushFirmwareUpdataInfo.Firmware_Version[0])<<16);  
   
   firmwarePackage->FirmwareTimeOutTmr = 0;
   if(firmwarePackage->IsFirmwareUpdataRequest == RESET)
   {
       memset(firmwarePackage,0,sizeof(NCFirmwarePackageStr));
       if((*(__IO uint8_t *)0x080001E8) <0xC8)//如果BOOT 更新 注意要更改此处内容
       {
           Firmware_PackageTotalMAX =FIRMWARE_PACKAGES_MAX;
           Firmware_FileSizeMAX     = FIRMWARE_SIZE_MAX;         
       }
       else
       {
           Firmware_PackageTotalMAX =FIRMWARE_PACKAGES_BIG_MAX;//100
           Firmware_FileSizeMAX     = FIRMWARE_SIZE_BIG_MAX;      
       }
       
       if((gExProtocol.receive.PushFirmwareUpdataInfo.Firmware_FileSize) > Firmware_FileSizeMAX || \
          (gExProtocol.receive.PushFirmwareUpdataInfo.Firmware_PackageTotal)>Firmware_PackageTotalMAX)//  512*26*2 = 26KB
       {
            //超空间 127KB
           NCExMakeAckFirmwareUpdataInfo(FIRMWARE_FILE_TOO_BIG);
       }
//       else if(Version <= (userSetting->AppSoftVer))
//       {
//          //版本不是最新
//           NCExMakeAckFirmwareUpdataInfo(FIRMWARE_NO_NEWEST);
//       }
//       else if(gExProtocol.receive.PushFirmwareUpdataInfo.HardwareVersion != DEVICE_HARDWARE_VERSION && gExProtocol.receive.PushFirmwareUpdataInfo.HardwareVersion != DEVICE_HARDWARE_VERSION_)
//       {
//           NCExMakeAckFirmwareUpdataInfo(FIRMWARE_HARDWARE_ERR);
//       }       
       else
       {
            firmwarePackage->FirmwareFlieSize         = gExProtocol.receive.PushFirmwareUpdataInfo.Firmware_FileSize;
            firmwarePackage->FirmwareFlieChecksum     = gExProtocol.receive.PushFirmwareUpdataInfo.Firmware_FileCheckSum;
            firmwarePackage->FirmwarePackageTotal     = gExProtocol.receive.PushFirmwareUpdataInfo.Firmware_PackageTotal;
            firmwarePackage->FirmwareUpdataVersion    = Version;             
            firmwarePackage->FirmwareUpdataStep       = 0;
            
            //NCExMakeAckFirmwareUpdataInfo(FIRMWARE_ERASE_COMPLETE);//擦除成功再发送
           
            //firmwarePackage->FirmwareRetransmitCnt    = 0; 
            firmwarePackage->IsFirmwareUpdataRequest  = SET;
            firmwarePackage->IsUsbFirmwareUpdata      =  FALSE;
            //NCPacketRetransmitInit(g_eD2App_AckFirmwarePackageData,TRUE);           
       }       
   }
}

//===============================================================================
//===============================================================================
void NCExMakeAckFirmwarePackageData(UINT8 PackageIndex,UINT8 DeviceAck)
{
    NCUserSetting* userSetting = NCUserSettingGet();
    if(FALSE == NCSemaphoreTake(gMutexWlanPackage)) return;
    memcpy(&(gExProtocol.upload.WIFIAckFirmwarePackageData.DeviceSN),        userSetting->deviceSN,      sizeof(userSetting->deviceSN)); 
    gExProtocol.upload.WIFIAckFirmwarePackageData.DeviceAckInfo       = DeviceAck;
    gExProtocol.upload.WIFIAckFirmwarePackageData.CurrnetPackageIndex = PackageIndex;    
	//把数据发送出去
    NCInfoWlanParamSet(g_eD2App_AckFirmwarePackageData, (UINT8*)&gExProtocol.upload, sizeof(stuD2App_WIFIAckFirmwarePackageData));   
    NCSemaphoreGive(gMutexWlanPackage);  
}
///=================================================================================================================
void NCExUnMakePushFirmwarePackageData(UINT8 *recbuff,UINT16 Datalength)
{
   NCUserSetting* userSetting = NCUserSettingGet();
   NCFirmwarePackageStr * firmwarePackage = NCFirmwarePackageGet();
   PacketRetransmitStu * PacketRetransmit = GetgNCPacketRetransmit();
   
   if(PacketRetransmit->IsEnabnle == TRUE)
   {
      if(PacketRetransmit->Cmd == g_eD2App_AckFirmwareUpdataInfo  || \
         PacketRetransmit->Cmd == g_eD2App_AckFirmwarePackageData)
       {
          PacketRetransmit->Cmd = NULL;
          PacketRetransmit->IsEnabnle = FALSE;
       }
   }
   //===========================================
   if(firmwarePackage->IsFirmwareUpdataRequest)
   {
       memcpy(&gExProtocol.receive.PushFirmwarePackageData, recbuff, Datalength);
       if((firmwarePackage->FirmwareUpdatedPackageCnt +1) == gExProtocol.receive.PushFirmwarePackageData.Firmware_CurrentPackage)  
       {           
           firmwarePackage->FirmwarePackageIdenx = gExProtocol.receive.PushFirmwarePackageData.Firmware_CurrentPackage; 
           firmwarePackage->FirmwarePackageSize = (Datalength-1)/2;
           memcpy(&(firmwarePackage->PackageBuffer),gExProtocol.receive.PushFirmwarePackageData.Firmware_PackageData,2*(firmwarePackage->FirmwarePackageSize));   
       } 
       else
       {
            NCExMakeAckFirmwarePackageData(firmwarePackage->FirmwareUpdatedPackageCnt,FIRMWARE_PACKAGE_INDENX_ERR);
       }    
   }
   else
   {
      //8S没接收到数据
      NCExMakeAckFirmwarePackageData(firmwarePackage->FirmwarePackageIdenx,FIRMWARE_PACKAGE_TIME_OUT);
   }
}

//========================================================================================================



//==============================================
//void  NCExMakePushFirmwareUpdataFinish(void) 
//{
//    NCUserSetting* userSetting = NCUserSettingGet();

//    if(FALSE == NCSemaphoreTake(gMutexWlanPackage)) return;

//    memcpy(&(gExProtocol.upload.PushFirmwareUpdataFinish.DeviceName), userSetting->deviceName, sizeof(userSetting->deviceName));
//    memcpy(&(gExProtocol.upload.PushFirmwareUpdataFinish.DeviceSN),   userSetting->deviceSN, sizeof(userSetting->deviceSN));    
//    memcpy(&(gExProtocol.upload.PushFirmwareUpdataFinish.SoftVersion), &(userSetting->AppSoftVer),3);  
//    gExProtocol.upload.PushFirmwareUpdataFinish.HardwareVersion = userSetting->HardwareVersion;     
//	//把数据发送出去
//    NCInfoWlanParamSet(g_eD2App_PushFirmwareUpdataFinish, (UINT8*)&gExProtocol.upload, sizeof(stuD2App_PushFirmwareUpdataFinish)); 
//    userSetting->IsMustPushApp = TRUE;    
//    NCSemaphoreGive(gMutexWlanPackage); 
//     
//}

//=====================================================================
//从机 升级程序段


//==============================================================================
//===========================================================================
//==========================================================================
void NCExMakeSlaveAckFirmwareUpdataInfo(UINT8  TargetUid,UINT8 DeviceAck)
{
    NCUserSetting* userSetting = NCUserSettingGet();
    if(FALSE == NCSemaphoreTake(gMutexWlanPackage)) return;
    memcpy(&(gExProtocol.upload.WIFIAckSlaveFirmwareUpdataInfo.DeviceSN),        userSetting->deviceSN,      sizeof(userSetting->deviceSN)); 
    gExProtocol.upload.WIFIAckSlaveFirmwareUpdataInfo.DeviceAckInfo  = DeviceAck; 
    gExProtocol.upload.WIFIAckSlaveFirmwareUpdataInfo.TargetUid = TargetUid;
	//把数据发送出去
    NCInfoWlanParamSet(g_eD2App_AckSlaveFirmwareUpdataInfo, (UINT8*)&gExProtocol.upload, sizeof(stuD2App_WIFIAckSlaveFirmwareUpdataInfo));   
    NCSemaphoreGive(gMutexWlanPackage);    
}
//==================================================================================
//    FIRMWARE_SLAVE_ERASE_COMPLETE      = 0x00,// 擦除成功 请求APP发送Package 数据
//    FIRMWARE_SLAVE_FILE_TOO_BIG        = 0x02,// 固件文件超空间 
//    FIRMWARE_SLAVE_IS_UPDATAED         = 0x03,// 已经处于升级中
//    FIRMWARE_SLAVE_SLAVE_LOST          = 0x04,// 目标从机全部掉线

void NCExUnMakeSlavePushFirmwareUpdataInfo(UINT8 *recbuff)
{
    struct _NCMSComProtocol *MSComUnionProtocol  = GetMSComUnionProtocol();
    NCSlaveFirmwareStu *  NCSlaveFirmwareInfo = NCSlaveFirmwareInfoGet();
    NCUserSetting* userSetting = NCUserSettingGet();

   memcpy(&gExProtocol.receive.PushSlaveFirmwareUpdataInfo, recbuff, sizeof(stuApp2D_PushSlaveFirmwareUpdataInfo));
   
   if((gExProtocol.receive.PushSlaveFirmwareUpdataInfo.Firmware_FileSize) > FIRMWARE_SLAVE_SIZE_MAX || \
      (gExProtocol.receive.PushSlaveFirmwareUpdataInfo.Firmware_PackageTotal)>FIRMWARE_SLAVE_PACKAGES_MAX)//  512*26*2 = 26KB   
   {
        //超空间 127KB
       NCExMakeSlaveAckFirmwareUpdataInfo(gExProtocol.receive.PushSlaveFirmwareUpdataInfo.TargetUid,FIRMWARE_SLAVE_FILE_TOO_BIG);
   }      
   else if(NCSlaveFirmwareInfo->IsSlaveUpdataFirm)  
   {
       //从机已经处于升级中
       NCExMakeSlaveAckFirmwareUpdataInfo(gExProtocol.receive.PushSlaveFirmwareUpdataInfo.TargetUid,FIRMWARE_SLAVE_IS_UPDATAED);
   }
   else
   {
      MSComUnionProtocol->upload.PushFirmwareUpdataInfo.Firmware_FileSize      = gExProtocol.receive.PushSlaveFirmwareUpdataInfo.Firmware_FileSize;
      MSComUnionProtocol->upload.PushFirmwareUpdataInfo.Firmware_FileCheckSum  = gExProtocol.receive.PushSlaveFirmwareUpdataInfo.Firmware_FileCheckSum;
      MSComUnionProtocol->upload.PushFirmwareUpdataInfo.Firmware_PackageTotal  = gExProtocol.receive.PushSlaveFirmwareUpdataInfo.Firmware_PackageTotal;
      memcpy(MSComUnionProtocol->upload.PushFirmwareUpdataInfo.Firmware_Version,gExProtocol.receive.PushSlaveFirmwareUpdataInfo.Firmware_Version,3);
      MSComUnionProtocol->upload.PushFirmwareUpdataInfo.HardwareVersion       = gExProtocol.receive.PushSlaveFirmwareUpdataInfo.HardwareVersion;
      
      NCSlaveFirmwareInfo->IsSlaveUpdataFirm = FALSE;
      NCSlaveFirmwareInfo->IsUsbSlaveUpdata  = FALSE;       
      NCSlaveFirmwareInfo->NeedUpdataFirmUID = 0;
      if(gCh2ManageCtr.CellStateData != 0)  NCSlaveFirmwareInfo->NeedUpdataFirmUID |= 0x01;  
      if(gCh3ManageCtr.CellStateData != 0)  NCSlaveFirmwareInfo->NeedUpdataFirmUID |= 0x02;
      if(gCh4ManageCtr.CellStateData != 0)  NCSlaveFirmwareInfo->NeedUpdataFirmUID |= 0x04;
       
       //上位机下发的设备 和在线的设备 相与 作为可以升级的设备序列
       NCSlaveFirmwareInfo->NeedUpdataFirmUID = gExProtocol.receive.PushSlaveFirmwareUpdataInfo.TargetUid & NCSlaveFirmwareInfo->NeedUpdataFirmUID;
       if(NCSlaveFirmwareInfo->NeedUpdataFirmUID == 0)
       {
           //从机全部丢失
          NCExMakeSlaveAckFirmwareUpdataInfo(gExProtocol.receive.PushSlaveFirmwareUpdataInfo.TargetUid,FIRMWARE_SLAVE_SLAVE_LOST);
       }
       else
       {
          NCSlaveFirmwareInfo->SlaveUpdataTimeOut       = SLAVER_FIRMWARE_TIMEOUT;//16S
          NCSlaveFirmwareInfo->SlaveUpdataWaitDataTime  = SLAVER_FIRMWARE_WAIT_TIMEOUT;//5S 发出数据后开始计算
          NCSlaveFirmwareInfo->SlaveUpdata_NowCmd       = g_eApp2D_PushSlaveFirmwareUpdataInfo;
          NCSlaveFirmwareInfo->SlaveUpdata_NowPageIdenx = 0;   

          NCSlaveFirmwareInfo->SlaveUpdata_FinishIdenxD2 = 0;
          NCSlaveFirmwareInfo->SlaveUpdata_FinishIdenxD3 = 0;
          NCSlaveFirmwareInfo->SlaveUpdata_FinishIdenxD3 = 0;
          
          NCSlaveFirmwareInfo->SlaveUpdata_AckD2         = 0xFF;//因为OK的应答是0x00
          NCSlaveFirmwareInfo->SlaveUpdata_AckD3         = 0xFF;//因为OK的应答是0x00
          NCSlaveFirmwareInfo->SlaveUpdata_AckD4         = 0xFF;//因为OK的应答是0x00
          
          NCSlaveFirmwareInfo->IsSlaveUpdata_Acked      = FALSE;//放在清除标记后的地方
          NCSlaveFirmwareInfo->IsSlaveUpdataFirm        = TRUE; //从机在升级中标记
          NCSlaveFirmwareInfo->IsUsbSlaveUpdata         = FALSE;
          if(NCSlaveFirmwareInfo->NeedUpdataFirmUID&0x01)
          {
             NCEms02MakePushFirmwareUpdataInfo(); 
          }
          if(NCSlaveFirmwareInfo->NeedUpdataFirmUID&0x02)
          {
             NCEms03MakePushFirmwareUpdataInfo(); 
          }
          
          if(NCSlaveFirmwareInfo->NeedUpdataFirmUID&0x04)
          {
             NCEms04MakePushFirmwareUpdataInfo(); 
          }
       }  
   }       
   
}

//===============================================================================
void NCExUnMakeSlavePushFirmwarePackageData(UINT8 *recbuff,UINT16 Datalength)
{
   NCUserSetting* userSetting = NCUserSettingGet();
   struct _NCMSComProtocol *MSComUnionProtocol  = GetMSComUnionProtocol();
   NCSlaveFirmwareStu *  NCSlaveFirmwareInfo = NCSlaveFirmwareInfoGet();
   //===========================================
   PacketRetransmitStu * PacketRetransmit = GetgNCPacketRetransmit(); 
   if(PacketRetransmit->IsEnabnle == TRUE)
   {
      if(PacketRetransmit->Cmd == g_eD2App_AckSlaveFirmwareUpdataInfo  || \
         PacketRetransmit->Cmd == g_eD2App_AckSlaveFirmwarePackageData)
       {
          PacketRetransmit->Cmd = NULL;
          PacketRetransmit->IsEnabnle = FALSE;
       }
   }
   //
   memcpy(&gExProtocol.receive.PushSlaveFirmwarePackageData, recbuff, Datalength);
   MSComUnionProtocol->upload.PushFirmwarePackageData.Firmware_CurrentPackage  = gExProtocol.receive.PushSlaveFirmwarePackageData.Firmware_CurrentPackage; 
   memcpy(&MSComUnionProtocol->upload.PushFirmwarePackageData.Firmware_PackageData, &gExProtocol.receive.PushSlaveFirmwarePackageData.Firmware_PackageData, Datalength-2);
   
   if(NCSlaveFirmwareInfo->IsSlaveUpdataFirm == TRUE && NCSlaveFirmwareInfo->NeedUpdataFirmUID != 0)
   {
       if(NCSlaveFirmwareInfo->SlaveUpdata_NowCmd == g_eApp2D_PushSlaveFirmwareUpdataInfo)//首包数据
       {
           if(NCSlaveFirmwareInfo->NeedUpdataFirmUID & 0x01)
           {
               if(NCSlaveFirmwareInfo->SlaveUpdata_AckD2 !=FIRMWARE_SLAVE_ERASE_COMPLETE)
                NCSlaveFirmwareInfo->NeedUpdataFirmUID &= (~0x01);
           }           
           if(NCSlaveFirmwareInfo->NeedUpdataFirmUID & 0x02)
           {
               if(NCSlaveFirmwareInfo->SlaveUpdata_AckD3 !=FIRMWARE_SLAVE_ERASE_COMPLETE)
                 NCSlaveFirmwareInfo->NeedUpdataFirmUID &= (~0x02);
           }
           
          if(NCSlaveFirmwareInfo->NeedUpdataFirmUID & 0x04)
           {
               if(NCSlaveFirmwareInfo->SlaveUpdata_AckD4 !=FIRMWARE_SLAVE_ERASE_COMPLETE)
                 NCSlaveFirmwareInfo->NeedUpdataFirmUID &= (~0x04);
           }
           
          if(NCSlaveFirmwareInfo->NeedUpdataFirmUID ==0)
          {
              //所有ID 没有回复正确
             NCSlaveFirmwareInfo->IsSlaveUpdataFirm = FALSE;
             NCSlaveFirmwareInfo->IsUsbSlaveUpdata = FALSE;
             NCExMakeSlaveAckFirmwarePackageData(NCSlaveFirmwareInfo->NeedUpdataFirmUID,NCSlaveFirmwareInfo->SlaveUpdata_NowPageIdenx,FIRMWARE_PACKAGE_TIME_OUT);   
          }
          else
          {
               NCSlaveFirmwareInfo->SlaveUpdata_NowPageIdenx = MSComUnionProtocol->upload.PushFirmwarePackageData.Firmware_CurrentPackage;
               NCSlaveFirmwareInfo->SlaveUpdataTimeOut       = SLAVER_FIRMWARE_TIMEOUT;//16S
               NCSlaveFirmwareInfo->SlaveUpdataWaitDataTime  = SLAVER_FIRMWARE_WAIT_TIMEOUT;//5S 发出数据后开始计算              

               NCSlaveFirmwareInfo->SlaveUpdata_FinishIdenxD2 = 0;
               NCSlaveFirmwareInfo->SlaveUpdata_FinishIdenxD3 = 0;
               NCSlaveFirmwareInfo->SlaveUpdata_FinishIdenxD3 = 0;
              
               NCSlaveFirmwareInfo->SlaveUpdata_AckD2         = 0;
               NCSlaveFirmwareInfo->SlaveUpdata_AckD3         = 0;
               NCSlaveFirmwareInfo->SlaveUpdata_AckD4         = 0;
               NCSlaveFirmwareInfo->SlaveUpdata_NowCmd        = g_eApp2D_PushSlaveFirmwarePackageData;
               NCSlaveFirmwareInfo->IsSlaveUpdata_Acked       = FALSE;//放在清除标记后的地方  
               if(NCSlaveFirmwareInfo->NeedUpdataFirmUID&0x01)
               {
                    NCEms02MakePushFirmwarePackageData(Datalength-1); 
               }
               if(NCSlaveFirmwareInfo->NeedUpdataFirmUID&0x02)
               {
                    NCEms03MakePushFirmwarePackageData(Datalength-1); 
               }
               if(NCSlaveFirmwareInfo->NeedUpdataFirmUID&0x04)
               {
                    NCEms04MakePushFirmwarePackageData(Datalength-1);    
               }          
          }
       }
       else if(NCSlaveFirmwareInfo->SlaveUpdata_NowCmd == g_eApp2D_PushSlaveFirmwarePackageData)
       {
           if(NCSlaveFirmwareInfo->NeedUpdataFirmUID & 0x01)
           {
               if(NCSlaveFirmwareInfo->SlaveUpdata_AckD2 !=FIRMWARE_REQUEST_PACKAGE)
                  NCSlaveFirmwareInfo->NeedUpdataFirmUID &= (~0x01);
           }           
           if(NCSlaveFirmwareInfo->NeedUpdataFirmUID & 0x02)
           {
               if(NCSlaveFirmwareInfo->SlaveUpdata_AckD3 !=FIRMWARE_REQUEST_PACKAGE)
                 NCSlaveFirmwareInfo->NeedUpdataFirmUID &= (~0x02);
           }
           
          if(NCSlaveFirmwareInfo->NeedUpdataFirmUID & 0x04)
           {
               if(NCSlaveFirmwareInfo->SlaveUpdata_AckD4 !=FIRMWARE_REQUEST_PACKAGE)
                 NCSlaveFirmwareInfo->NeedUpdataFirmUID &= (~0x04);
           }
          NCSlaveFirmwareInfo->SlaveUpdata_AckD2         = 0;
          NCSlaveFirmwareInfo->SlaveUpdata_AckD3         = 0;
          NCSlaveFirmwareInfo->SlaveUpdata_AckD4         = 0;
          NCSlaveFirmwareInfo->IsSlaveUpdata_Acked       = FALSE;//放在清除标记后的地方
          if(NCSlaveFirmwareInfo->NeedUpdataFirmUID ==0)
          {
              //所有ID 没有回复正确
             NCSlaveFirmwareInfo->IsSlaveUpdataFirm = FALSE;
             NCSlaveFirmwareInfo->IsUsbSlaveUpdata = FALSE;
             NCExMakeSlaveAckFirmwarePackageData(NCSlaveFirmwareInfo->NeedUpdataFirmUID,NCSlaveFirmwareInfo->SlaveUpdata_NowPageIdenx,FIRMWARE_PACKAGE_TIME_OUT);   
          }
          else
          {
               NCSlaveFirmwareInfo->SlaveUpdata_NowPageIdenx = MSComUnionProtocol->upload.PushFirmwarePackageData.Firmware_CurrentPackage;
               NCSlaveFirmwareInfo->SlaveUpdataTimeOut       = SLAVER_FIRMWARE_TIMEOUT;//16S
               NCSlaveFirmwareInfo->SlaveUpdataWaitDataTime  = SLAVER_FIRMWARE_WAIT_TIMEOUT;//5S 发出数据后开始计算    
               if(NCSlaveFirmwareInfo->NeedUpdataFirmUID&0x01)
               {
                    NCEms02MakePushFirmwarePackageData(Datalength-1); 
               }
               if(NCSlaveFirmwareInfo->NeedUpdataFirmUID&0x02)
               {
                    NCEms03MakePushFirmwarePackageData(Datalength-1); 
               }
               if(NCSlaveFirmwareInfo->NeedUpdataFirmUID&0x04)
               {
                    NCEms04MakePushFirmwarePackageData(Datalength-1);    
               } 
           }               
       }
       else
       {
          //没有进入升级模式
          NCSlaveFirmwareInfo->IsSlaveUpdataFirm = FALSE;
          NCSlaveFirmwareInfo->IsUsbSlaveUpdata = FALSE;
          NCExMakeSlaveAckFirmwarePackageData(NCSlaveFirmwareInfo->NeedUpdataFirmUID,NCSlaveFirmwareInfo->SlaveUpdata_NowPageIdenx,FIRMWARE_PACKAGE_TIME_OUT);           
       }
   } 
   else
   {
       //不在升级中 返回超时
      NCSlaveFirmwareInfo->IsSlaveUpdataFirm = FALSE;
      NCSlaveFirmwareInfo->IsUsbSlaveUpdata = FALSE;
      NCExMakeSlaveAckFirmwarePackageData(NCSlaveFirmwareInfo->NeedUpdataFirmUID,NCSlaveFirmwareInfo->SlaveUpdata_NowPageIdenx,FIRMWARE_PACKAGE_TIME_OUT);
   }        
}

//===============================================================================
//===============================================================================
void NCExMakeSlaveAckFirmwarePackageData(UINT8  TargetUid,UINT8 PackageIndex,UINT8 DeviceAck)
{
    NCUserSetting* userSetting = NCUserSettingGet();
    if(FALSE == NCSemaphoreTake(gMutexWlanPackage)) return;
    memcpy(&(gExProtocol.upload.WIFIAckSlaveFirmwarePackageData.DeviceSN),        userSetting->deviceSN,      sizeof(userSetting->deviceSN)); 
    gExProtocol.upload.WIFIAckSlaveFirmwarePackageData.TargetUid           = TargetUid;
    gExProtocol.upload.WIFIAckSlaveFirmwarePackageData.DeviceAckInfo       = DeviceAck;
    gExProtocol.upload.WIFIAckSlaveFirmwarePackageData.CurrnetPackageIndex = PackageIndex;    
	//把数据发送出去
    NCInfoWlanParamSet(g_eD2App_AckSlaveFirmwarePackageData, (UINT8*)&gExProtocol.upload, sizeof(stuD2App_WIFIAckSlaveFirmwarePackageData));   
    NCSemaphoreGive(gMutexWlanPackage);  
}


















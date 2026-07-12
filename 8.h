/*****************************************************************
 * File: NCWlanDataHandle.h
 * Date: 2021/12/22
 *
 * Note:
 *
*****************************************************************/
#ifndef _NCWLANDATAHANDLE_H_
#define _NCWLANDATAHANDLE_H_

#include "Typedef.h"    
#include "protocoldefinition.h"  

void NCWlanDataInit(void); 

// WIFI参数被重新配置
//void NCWlanDataSetTaskBleConfig(UINT8 config);
void NCInfoWlanParamSet(NC_PROTOCOL_CMD_FOR_WLAN cmdSend, UINT8 *sendDatabuf, UINT16 sendLength);
void NCWlanProtocolParse(NC_PROTOCOL_CMD_FOR_WLAN cmd, UINT8 *databuf, UINT16 datalength);
void NCWlanDataSendProcess(void);
void NCWlanDataParseProcess(void);

#endif // _NCWLANDATAHANDLE_H_

#include <stdio.h>
#include "systemConfig.h"
#include "localNetInfo.h"
#include "inputEvent.h"
#include "commonHead.h"
#include "commandWord.h"
#include "cellInformation.h"
#include "talkCommandTranslate.h"
#include "talkAudio.h"
#include "playWav.h"
#include "inputEvent.h"
#include "tftpUpdateServer.h"
#include "watchdog.h"
#include "security.h"

T_Room localRoom;
static int talkCommandCallBack(T_Room dest,unsigned  char          cmd,
	const    unsigned char * pData, int  dataLen);
static void mySleep(int time);
static void inputEventCallBack(int keyCode,int keyValue,int keyState);
static int securityCallBackFuncTion( int securityEvent,
	const	void * data , int dataLen );



T_Room destRoom;

int main(void)
{
//初始化一系列模块服务
	int ret;
	if(getSystemConfig() == 0)
		localRoom = getConfigRoomAddr();
	
	ret  = cellInfoInit(localRoom);
	PRINT_ARRAY((char *)&localRoom, sizeof(localRoom));
	setLocalNetInfo(localRoom);
	timer_taskInit();
	
	security_init(securityCallBackFuncTion);
	talkAudioInit();
	palyWavInit();
	talkCommandInit(localRoom);
//设置对讲回调，待对讲有新事件产生时会执行此回调
	talkSetCallBackFunc(talkCommandCallBack);
//初始化按键,传入按键回调
	keypadInit(inputEventCallBack);
	palyStartingUp();
	openWTD();
	
	for(;;)
	{
	//以下代码只做测试使用,量产后会去除
		
		mySleep(1);
		keepWTDalive(5);
		
	}
	return 0;
exit0:
	return -1;
}
static void mySleep(int time)
{
	int current  = time;
	while(current > 0 )
			current = sleep(current);
}

static void inputEventCallBack(int keyCode,int keyValue,int keyState)
{
	LOGD("keyCode:%d keyValue:%d keyState:%d \n",keyCode,keyValue,keyState);
	


	switch(keyCode)
	{
		case OPEN_LOCK:
			
			if(keyValue == KEY_DOWN)
			{
				
			}else if(keyValue == KEY_UP)
			{	
				if(keyState == E_SHORT_PRESS ){
					security_stopAlarm();
					
					if(openLock(destRoom)<0)  
					{
						palyKeyTone();
						
					}
				}else if(keyState == E_LONG_PRESS)  //长按开锁键,撤防
				{
					playRepealAlarm();
					security_repealAlarm();
				}
			}
			
			break;
		case CALL_MANAGER:
			
			if(keyValue == KEY_DOWN)
			{
				
			}else if(keyValue == KEY_UP)
			{

				if(keyState == E_LONG_PRESS ){
					callManager();
				}
			}
			break;
				
		case HUANG_UP:
			if(keyValue == KEY_DOWN)
			{
				
			}else if(keyValue == KEY_UP)
			{
				if(keyState == E_SHORT_PRESS ){
					if(talkTranslateAnswerOrHuangUp(destRoom) <0)
					{
						palyKeyTone();
					}
					
				}else if(keyState == E_LONG_PRESS)
				{
					playLayoutAlarm();	
					security_layoutAlarm();
				}
				
			}
			break;
		case MQ_ALARM:
			if(keyValue == KEY_DOWN)
			{
				
			}else if(keyValue == KEY_UP)
			{
				LOGD("MQ_ALARM");
				
				security_promptlySendAlarm(E_MQ_ALARM);
				
			}
			break;
		case HW_ALARM:
			
			if(keyValue == KEY_DOWN)
			{
			}else if(keyValue == KEY_UP)
			{
				LOGD("HW_ALARM");
				
				security_delaySendAlarm(E_HW_ALARM);
			}
			break;
		case JJ_ALARM:
			if(keyValue == KEY_DOWN)
			{
				
			}else if(keyValue == KEY_UP)
			{
				LOGD("JJ_ALARM");
				security_promptlySendAlarm(E_JJ_ALARM);
			}
			break;
		case MC_ALARM:
			if(keyValue == KEY_DOWN)
			{
				
			}else if(keyValue == KEY_UP)
			{
				LOGD("MC_ALARM");
				security_promptlySendAlarm(E_MC_ALARM);
			}
			break;
		case YW_ALARM:
			if(keyValue == KEY_DOWN)
			{
				
			}else if(keyValue == KEY_UP)
			{
				LOGD("YW_ALARM");
				security_promptlySendAlarm(E_YW_ALARM);
			}
			break;
		default:
			break;
	}

}



static int securityCallBackFuncTion( int securityEvent,const	void * data , int dataLen )
{

	switch(securityEvent)
	{
		case UI_ALARM_STOP:
			stopPlayWav();
			break;
		case UI_ALARM_DELAY_TRIGGER:
			LOGI("UI_ALARM_DELAY_TRIGGER");
			palyDelayAlarm();
			break;
		case UI_ALARM_TRIGGER:
			switch(*(( E_ALARM_TYPE *) data))
			{
				case E_MQ_ALARM:
					talkTranslateHuangUp(destRoom);
					palyAlarm();
					break;
				case E_HW_ALARM:
					talkTranslateHuangUp(destRoom);
					palyAlarm();
					break;
				case E_MC_ALARM:
					talkTranslateHuangUp(destRoom);
					palyAlarm();
					break;
				case E_YW_ALARM:
					talkTranslateHuangUp(destRoom);
					palyAlarm();
					break;
				case E_JJ_ALARM:
					palyDetectAlarm();
					break;
			}
			
			break;
		case UI_ALARM_SENDTOMANAGE:{
			int ret;
			T_Room managerRoom;
			ret  = getManagerRoom(&managerRoom);
			CHECK_RET(ret<0, "fail to getManagerRoom", goto fail0 );
			ret = talkTranslateCmdToDestWaitAck(managerRoom,WARN_CMD,(const   unsigned char *)&data,sizeof(event_record_t));
			CHECK_RET(ret<0, "fail to talkTranslateCmdToDestWaitAck", goto fail0 );
		}
		break;
		default:
			break;
		
	}
	
	return 0;
fail0:
	return -1;
}
//处理回调 
static int talkCommandCallBack(T_Room dest,unsigned  char          cmd,const    unsigned char * pData, int  dataLen)
{
	
	uint32_t ipAddr;
	destRoom = dest;
	switch(cmd)
	{
		case UI_BE_CALLED_RING:
			LOGI("UI_BE_CALLED_RING!\n");
			palyRing();
			break;
		case UI_TO_CALL_RING:
			LOGI("UI_TO_CALL_RING!\n");
			palyRing();
			break;
		case UI_PEER_ANSWERED:
			LOGI("UI_PEER_ANSWERED!\n");
			stopPlayWav();
			if (0 == getRoomIpAddr(dest,&ipAddr))
			{
				talkAudioStart(ipAddr);
			}
			
			break;
		case UI_PEER_HUNG_UP:
			palyPeerHangUp();
			talkAudioStop();
			LOGI("UI_PEER_HUNG_UP!\n");
			break;
		case UI_PEER_OFF_LINE:
			palyLineFault();
			talkAudioStop();
			LOGI("UI_PEER_OFF_LINE!\n");
			break;
		case UI_PEER_NO_ANSWER:
			LOGI("UI_PEER_NO_ANSWER!\n");
			palyNoAnswer();
			break;
		case UI_PEER_LINE_BUSY:
			palyLineBusy();
			LOGI("UI_PEER_LINE_BUSY!\n");
			break;
		case UI_TALK_TIME_OUT:
			palyTalkTimeout();
			LOGI("UI_TALK_TIME_OUT!\n");
			break;
		case UI_WATCH_SUCCESS:
			LOGI("UI_WATCH_SUCCESS!\n");
			break;
		case UI_WATCH_TIME_OUT:
			palyTalkTimeout();
			LOGI("UI_WATCH_TIME_OUT!\n");
			break;
		case UI_OTHER_DEV_PROC :
			stopPlayWav();
			LOGI("UI_OTHER_DEV_PROC!\n");
			break;
		case UI_HUNG_UP_SUCCESS	:
			LOGI("UI_HUNG_UP_SUCCESS!\n");
			stopPlayWav();
			talkAudioStop();
			break;
		case UI_ANSWER_SUCESS:
			LOGI("UI_ANSWER_SUCESS!\n");
			break;
		case UI_OPEN_LOCK_SUCESS:
			stopPlayWav();
			LOGI("UI_OPEN_LOCK_SUCESS!\n");
			break;
		case UI_UPDATE_TIME_WEATHRE:
			LOGI("UI_UPDATE_TIME_WEATHRE!\n");
			break;
		case UI_UPDATE_HOUSE_INFO:
			LOGI("UI_UPDATE_HOUSE_INFO!\n");
			break;
		case UI_ELEVATOR_CALL_SUCCESS:
			LOGI("UI_ELEVATOR_CALL_SUCCESS!\n");
			break;
		case UI_ELEVATOR_CALL_FAIL:
			LOGI("UI_ELEVATOR_CALL_FAIL!\n");
			break;
		case UI_DOORBEL_TRY_SUCCESS:
			LOGI("UI_DOORBEL_TRY_SUCCESS!\n");
			break;
		case UI_NETWORK_NOT_ONLINE:
			LOGI("UI_NETWORK_NOT_ONLINE!\n");
			break;
		case UI_CONNECT_SERVER :
			LOGI("UI_CONNECT_SERVER!\n");
			break;
		case UI_UPDATE_SYSCONFIG:
			LOGI("UI_UPDATE_SYSCONFIG!\n");
			stSYS_CONFIG sysconfig_data;
			bzero(&sysconfig_data,sizeof(stSYS_CONFIG));
			memcpy(&sysconfig_data,pData,dataLen);
			if(0 == updateSysconfig(sysconfig_data)){
					palyoPerationSuccess();
					sleep(1);
					execl("/sbin/reboot", "reboot",(char *) 0);
			}
			break;
		case UI_TFTP_UPDATE_APP:
			LOGI("UI_TFTP_UPDATE_APP!!!!\n");
			T_TFTP_UPDATE tftp_update;
			bzero(&tftp_update,sizeof(T_TFTP_UPDATE));
			LOGD("dataLen :%d\n",dataLen);
			PRINT_ARRAY(pData, dataLen);
			
			memcpy(&tftp_update, pData,dataLen);
			LOGD("%s %d %d\n",tftp_update.fileName,tftp_update.serverip,ntohl(tftp_update.size));
			startTftpUpdate(&tftp_update);
			break;
			
		case UI_UPDATE_NETCONFIG:
			LOGI("UI_UPDATE_NETCONFIG!\n");
			if( 0 == updateNetconfig(pData,dataLen))
			{
					palyoPerationSuccess();
					sleep(1);
					execl("/sbin/reboot", "reboot",(char *) 0);
			}
			break;
		case UI_WAIT_LOCAL_HOOK_TIMEOUT:
			stopPlayWav();
			LOGI("UI_WAIT_LOCAL_HOOK_TIMEOUT!\n");
			break;
		case UI_PEER_RST:
			stopPlayWav();
			
			LOGI("UI_PEER_RST");
			break;
	

	
		
		default:
			break;
	
	}

	return 0;
}



















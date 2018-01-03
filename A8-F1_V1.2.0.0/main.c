#include <stdio.h>
#include <string.h>
#include <signal.h>
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

#include "memwatch.h"

#ifndef SIGSEGV
#error "SIGNAL.H does not define SIGSEGV; running this program WILL cause a core dump/crash!"
#endif

#ifndef MEMWATCH
#error "You really, really don't want to run this without memwatch. Trust me."
#endif

#if !defined(MW_STDIO) && !defined(MEMWATCH_STDIO)
#error "Define MW_STDIO and try again, please."
#endif

static T_Room localRoom;
static int talkCommandCallBack(T_Room dest,unsigned  char          cmd,
	const    unsigned char * pData, int  dataLen);
static void mySleep(int time);
static void inputEventCallBack(int keyCode,int keyValue,int keyState);
static int securityCallBackFuncTion( int securityEvent,
	const	void * data , int dataLen );

static main_exit(void);


T_Room destRoom;



static void sigroutine(int dunno) { /* 信号处理例程，其中dunno将会得到信号的值 */ 
		switch (dunno) { 
		case 1: 
			LOGE("Get a signal -- SIGHUP "); 
			main_exit();
			break; 
		case 2: 
			LOGE("Get a signal -- SIGINT "); 
			main_exit();
			break; 
		case 3: 
			LOGE("Get a signal -- SIGQUIT "); 
			main_exit();
		case SIGPIPE:
			LOGE("SIGPIPE!");
			break; 
		} 
		return; 
} 
static main_exit(void)
{
	cellInfoExit();
	talkAudioExit();
	stopPlayWav();
	talkCommandExit();
	closeWTD();
	LOGE("main_exit");
	exit(-1);
}
int main(void)
{
//初始化一系列模块服务
	int ret;
	if(getSystemConfig() == 0)
		localRoom = getConfigRoomAddr();
	ret  = cellInfoInit(localRoom);
	PRINT_ARRAY((char *)&localRoom, sizeof(localRoom));
	setLocalNetInfo(localRoom);
	
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
	signal(SIGHUP, sigroutine); 	//* 下面设置三个信号的处理方法 
	signal(SIGINT, sigroutine); 
	signal(SIGQUIT, sigroutine); 
	signal(SIGPIPE, sigroutine); 
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
			
			if(keyValue == KEY_STATE_DOWN)
			{
				
			}else if(keyValue == KEY_STATE_UP)
			{	
				if(keyState == E_SHORT_PRESS ){
					security_stopAlarm();
					
					if(openLock(destRoom)<0)  
					{
						palyKeyTone();
						
					}
				}else if(keyState == E_LONG_PRESS)  //长按开锁键,撤防
				{
					if(talkGetWorkState() <= TALK_FREE)
						security_repealAlarm();
				}
			}
			
			break;
		case CALL_MANAGER:
			
			if(keyValue == KEY_STATE_DOWN)
			{
				
			}else if(keyValue == KEY_STATE_UP)
			{
				
				if(keyState == E_LONG_PRESS ){
					security_stopAlarm();
					callManager();
				}
			}
			break;
				
		case HUANG_UP:
			if(keyValue == KEY_STATE_DOWN)
			{
				
			}else if(keyValue == KEY_STATE_UP)
			{
				if(keyState == E_SHORT_PRESS ){
					security_stopAlarm();
					if(talkTranslateAnswerOrHuangUp(destRoom) <0)
					{
						palyKeyTone();
					}
					
				}else if(keyState == E_LONG_PRESS)
				{
					if(talkGetWorkState() <= TALK_FREE)
						security_layoutAlarm();
				}
				
			}
			break;
		case MQ_ALARM:
			if(keyValue == KEY_STATE_DOWN)
			{
				
				LOGD("MQ_ALARM");
				
				security_promptlySendAlarm(E_MQ_ALARM);
				
			}else if(keyValue == KEY_STATE_UP)
			{
				
			}
			break;
		case HW_ALARM:
			
			if(keyValue == KEY_STATE_DOWN)
			{
				LOGD("HW_ALARM");
				
				security_delaySendAlarm(E_HW_ALARM);
			}else if(keyValue == KEY_STATE_UP)
			{
			
			}
			break;
		case JJ_ALARM:
			if(keyValue == KEY_STATE_DOWN)
			{
				LOGD("JJ_ALARM");
				security_promptlySendAlarm(E_JJ_ALARM);
				
			}else if(keyValue == KEY_STATE_UP)
			{
				
			}
			break;
		case MC_ALARM:
			if(keyValue == KEY_STATE_DOWN)
			{
				LOGD("MC_ALARM");
			//	security_promptlySendAlarm(E_MC_ALARM);
			}else if(keyValue == KEY_STATE_UP)
			{
				
			}
			break;
		case YW_ALARM:
			if(keyValue == KEY_STATE_DOWN)
			{
				
				LOGD("YW_ALARM");
			//	security_promptlySendAlarm(E_YW_ALARM);
				
			}else if(keyValue == KEY_STATE_UP)
			{
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
		case UI_ALARM_LAYOUT:
			playLayoutAlarm();
			break;
		case UI_ALARM_REPEAL:
			playRepealAlarm();	
			break;
		case UI_ALARM_STOP:
			
			stopPlayWav();
			break;
		case UI_ALARM_DELAY_TRIGGER:
			LOGI("UI_ALARM_DELAY_TRIGGER");
			talkTranslateHuangUp(destRoom);
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
					talkTranslateHuangUp(destRoom);
					palyDetectAlarm();
					break;
			}
			
			break;
		case UI_ALARM_SENDTOMANAGE:{
			int ret;
			T_Room managerRoom;
			ret  = getManagerRoom(&managerRoom);
			CHECK_RET(ret<0, "fail to getManagerRoom", goto fail0 );
			ret  =  userSingleServerSendDataToDestRoom(managerRoom,WARN_CMD,(const   unsigned char *)data,sizeof(event_record_t));
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
			
			talkAudioStop();
			palyPeerHangUp();
			LOGI("UI_PEER_HUNG_UP!\n");
			break;
		case UI_PEER_OFF_LINE:
			talkAudioStop();
			palyLineFault();
			
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
			LOGI("UI_TALK_TIME_OUT0!\n");
			talkAudioStop();
			usleep(1000*100);
			palyTalkTimeout();
			LOGI("UI_TALK_TIME_OUT1!\n");
			
			LOGI("UI_TALK_TIME_OUT2!\n");
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
			talkAudioStop();
			if(security_getAlarmState() != E_ALARM_RUNING )
			{
				stopPlayWav();
			}
			
			break;
		case UI_ANSWER_SUCESS:
			LOGI("UI_ANSWER_SUCESS!\n");
			break;
		case UI_OPEN_LOCK_SUCESS:
			if(security_getAlarmState != E_ALARM_RUNING)
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
			bzero((void *)&sysconfig_data,sizeof(stSYS_CONFIG));
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
			if( 0 == updateNetconfig((void*)pData,dataLen))
			{
					palyoPerationSuccess();
					sleep(1);
					execl("/sbin/reboot", "reboot",(char *) 0);
			}
			break;
		case UI_WAIT_LOCAL_HOOK_TIMEOUT:
			if(security_getAlarmState != E_ALARM_RUNING)
				stopPlayWav();
			LOGI("UI_WAIT_LOCAL_HOOK_TIMEOUT!\n");
			break;
		case UI_PEER_RST:
			if(security_getAlarmState != E_ALARM_RUNING)
				stopPlayWav();
			
			LOGI("UI_PEER_RST");
			break;
	

	
		
		default:
			break;
	
	}

	return 0;
}



















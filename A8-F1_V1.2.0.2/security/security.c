#include <time.h>
#include "timerTaskManage.h"
#include "commStructDefine.h"
#include "cellInformation.h"
#include "systemConfig.h"
#include "security.h"
#include "commonHead.h"



#define ALARM_INTERVAL_TIME 180  //安防触发间隔时间


typedef enum E_SECURITY_MODE{
		MODE_FREE = 0,
		MODE_LAYOUT,
}E_SECURITY_MODE;
static void security_sendMsgToManager(void *arg);
static void security_triggerAlarmTinkle(void *arg);
static int  sendAlarmToManager(int         type);
static void security_setAlarmState(void *arg);

static int  buildAlarmEvent(p_event_record_t alarmEvent,int type);



static E_ALARM_STATE securityState;
/*
static TimerID alarm_ringdown_TimerId;
static TimerID alarm_sendMsg_TimerId;
static TimerID alarm_state_TimerId;
*/
static E_SECURITY_MODE securityMode;
static pTimerOps alarm_ringdown_TimerId;
static pTimerOps alarm_sendMsg_TimerId;
static pTimerOps alarm_state_TimerId;


pSecurityCallBackFuncTion upSecurityMsgToUi;

/*安防服务*/
int security_init(pSecurityCallBackFuncTion       callbackFunc)
{
	int ret;
	
	securityMode = getSecurityMode();
	LOGD("securityState = %d\n",securityState);
	upSecurityMsgToUi = callbackFunc;

	return 0;
fail0:
	return -1;
}
int security_layoutAlarm(void)
{
	if(securityMode == MODE_LAYOUT )
		return -1;
	securityMode = MODE_LAYOUT;
 	setSecurityMode(securityMode);
	upSecurityMsgToUi(UI_ALARM_LAYOUT,NULL,0);
}

int security_repealAlarm(void)
{
	if(securityMode == MODE_FREE )
		return -1;
	securityMode = MODE_FREE;
	security_stopAlarm();
	setSecurityMode(securityState);
	upSecurityMsgToUi(UI_ALARM_REPEAL,NULL,0);
}

int security_delaySendAlarm(int         type)
{
	
	struct timespec  current_time;
	static struct timespec	last_time;
	const char *securityStateStr[] = {"E_ALAEM_SLEEP","E_ALARM_READY","E_ALARM_RUNING"};
		
	clock_gettime(CLOCK_MONOTONIC, &current_time);
		//180S内只能报警一次
	if ((current_time.tv_sec - last_time.tv_sec)<ALARM_INTERVAL_TIME)
	{
			LOGD(" Please waiting..");
			return -1;
	}
	clock_gettime(CLOCK_MONOTONIC, &last_time);
	if(securityMode == MODE_FREE )
	{
		LOGD("securityMode == MODE_FREE ");
		return -1;
	}
	if( (type != E_JJ_ALARM) && (securityState != E_ALAEM_SLEEP )){
		LOGD("securityState = %s",securityStateStr[securityState]);
		return -1;
	}

	//延时后产生报警铃声
	if(alarm_ringdown_TimerId )
	{
		destroyTimerTaskServer(&alarm_ringdown_TimerId);
	}
	alarm_ringdown_TimerId = createTimerTaskServer(ALAEM_DELAY_TIME,40*1000,3,security_triggerAlarmTinkle,
							&type,sizeof(type));
	alarm_ringdown_TimerId->start(alarm_ringdown_TimerId);
	
//	alarm_ringdown_TimerId  =  timer_taskRegister(ALAEM_DELAY_TIME,40*1000,3,security_triggerAlarmTinkle
//						   ,&type,sizeof(type));

						   
	//延时后发送报警信号
	LOGD("延时后报警");
	alarm_sendMsg_TimerId = createTimerTaskServer(ALAEM_DELAY_TIME,0,1,security_sendMsgToManager,
							&type,sizeof(type));
	alarm_sendMsg_TimerId->start(alarm_sendMsg_TimerId);


	//alarm_sendMsg_TimerId = timer_taskRegister(ALAEM_DELAY_TIME,0,1,security_sendMsgToManager
	//					   ,&type,sizeof(type));
	
	securityState = E_ALARM_READY;
	upSecurityMsgToUi(UI_ALARM_DELAY_TRIGGER,NULL,0);
	return 0;
}

int security_promptlySendAlarm(int        type)
{

	struct timespec  current_time;
	static struct timespec  last_time;
	clock_gettime(CLOCK_MONOTONIC, &current_time);
	//180S内只能报警一次
	if ((current_time.tv_sec - last_time.tv_sec)<ALARM_INTERVAL_TIME)
	{
		LOGD(" Please waiting..");
		return -1;
	}
	clock_gettime(CLOCK_MONOTONIC, &last_time);
	if(securityState != E_ALAEM_SLEEP ){
		LOGD("securityState IS not E_ALAEM_SLEEP! \n ");
		return -1;
	}
		
	if( type == E_JJ_ALARM)
	{
		//只上报一次
		security_triggerAlarmTinkle(&type);
	}else {
		if((securityMode == MODE_FREE ) ){
			
			return -1;
		}
		//每隔40上报一次，总上报3次

		if(alarm_ringdown_TimerId)
		{
			destroyTimerTaskServer(&alarm_ringdown_TimerId);
		}
		alarm_ringdown_TimerId = createTimerTaskServer(0,40*1000,3,security_triggerAlarmTinkle,
							&type,sizeof(type));
		alarm_ringdown_TimerId->start(alarm_ringdown_TimerId);
	}
	//只发送一次
	security_sendMsgToManager((void *)&type);
	return 0;
}

int security_stopAlarm(void)
{
	if( securityState == E_ALAEM_SLEEP)
		return 0;
	securityState = E_ALAEM_SLEEP;
	upSecurityMsgToUi(UI_ALARM_STOP,NULL,0);
	
	//ret  = timer_taskDestroy(alarm_ringdown_TimerId);
	
	destroyTimerTaskServer(&alarm_ringdown_TimerId);
//	CHECK_RET(ret < 0, "fail to timer_taskStop alarm_ringdown_TimerId", );
	destroyTimerTaskServer(&alarm_sendMsg_TimerId);
//	ret  = timer_taskDestroy(alarm_sendMsg_TimerId);
//	CHECK_RET(ret < 0, "fail to timer_taskStop alarm_sendMsg_TimerId", );
	destroyTimerTaskServer(&alarm_state_TimerId);
//	ret  = timer_taskDestroy(alarm_state_TimerId);
//	CHECK_RET(ret < 0, "fail to timer_taskStop alarm_state_TimerId",);
		
	return 0;
fail0:
	return -1;
}
 E_ALARM_STATE security_getAlarmState(void)
{
	return securityState;
}

static void security_sendMsgToManager(void *arg)
{
	LOGD("发送报警");
	if(arg == NULL)
		return;
	int type = *((int *)arg);
	
	sendAlarmToManager(type);
}
static void security_setAlarmState(void *arg)
{
	const char *securityStateStr[] = {"E_ALAEM_SLEEP","E_ALARM_READY","E_ALARM_RUNING"};
	if(arg == NULL)
		return;
	
	int type = *((int *)arg);
	switch(type){
		
		case E_ALARM_READY:
		case E_ALARM_RUNING:
				securityState = type;
			break;
		case E_ALAEM_SLEEP:
			if(securityState == E_ALARM_RUNING )
				securityState = type;
			break;
		default:
			return ;
	}
	LOGD("security_setAlarmState = %s\n",securityStateStr[type]);
			
}
static void security_triggerAlarmTinkle(void *arg)
{
	static int upCount  = 0;
	const char *securityTypeStr[] = {"E_MQ_ALARM","E_HW_ALARM","E_MC_ALARM","E_YW_ALARM","E_JJ_ALARM"};

	if(arg == NULL)
		return;
	
	int type = (*(int *)arg);
	int state = E_ALAEM_SLEEP;
	LOGD("Alarmtype = %s\n",securityTypeStr[type]);
	if(type != E_JJ_ALARM ){
		securityState = E_ALARM_RUNING;
		//timer_taskDestroy(alarm_state_TimerId);
		if(alarm_state_TimerId)
			destroyTimerTaskServer(&alarm_state_TimerId);
		
		//alarm_state_TimerId = timer_taskRegister(39*1000,0,1,security_setAlarmState
		//				   ,&type,sizeof(type));
		//这里有0.5/40的几率会产生BUG
		alarm_state_TimerId = createTimerTaskServer(39500,0,1,security_setAlarmState
					   ,&state,sizeof(state));
		alarm_state_TimerId->start(alarm_state_TimerId);
	}
	upSecurityMsgToUi(UI_ALARM_TRIGGER,arg,sizeof(int));
	
}
static int sendAlarmToManager(int         type)
{
	int ret;
	event_record_t alarmEvent;
	bzero(&alarmEvent,sizeof(event_record_t));
	switch(type)
	{
		case E_MQ_ALARM :
			buildAlarmEvent(&alarmEvent,MQ_EVENT);
			alarmEvent.room.room = EVENT_PLACE_CF;
			break;
		case E_HW_ALARM:
			buildAlarmEvent(&alarmEvent,HW_EVENT);
			alarmEvent.room.room = EVENT_PLACE_WS;
			break;
		case E_YW_ALARM:
			buildAlarmEvent(&alarmEvent,YW_EVENT);
			alarmEvent.room.room = EVENT_PLACE_CF;
			break;
		case E_JJ_ALARM:
			buildAlarmEvent(&alarmEvent,JJ_EVENT);
			alarmEvent.room.room = EVENT_PLACE_KT;
			break;
		case E_MC_ALARM:
			buildAlarmEvent(&alarmEvent,MC_EVENT);
			break;
		default:
			LOGE("Alarm type is error!");
			return -1;
	}
	upSecurityMsgToUi(UI_ALARM_SENDTOMANAGE,(void  *)&alarmEvent,sizeof(event_record_t));
	return 0;
fail0:
	return -1;
}

static int  buildAlarmEvent(p_event_record_t alarmEvent,int type)
{
	int ret;
	struct timespec tp;
	clock_gettime(CLOCK_REALTIME,&tp);
	alarmEvent->type = type;
	alarmEvent->id = htonl(1);
	alarmEvent->time = htonl(tp.tv_sec);   
	ret = getLocalRoom(&alarmEvent->room);
	CHECK_RET(ret < 0, "fail to getLocalRoom", goto fail0);
	return 0;
fail0:
	return -1;
}


























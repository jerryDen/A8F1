#include <time.h>
#include "timerTaskManage.h"
#include "commStructDefine.h"
#include "cellInformation.h"
#include "systemConfig.h"
#include "security.h"
#include "commonHead.h"



#define ALARM_INTERVAL_TIME 10   //安防触发间隔时间
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
static pTimerOps alarm_ringdown_TimerId;
static pTimerOps alarm_sendMsg_TimerId;
static pTimerOps alarm_state_TimerId;


pSecurityCallBackFuncTion upSecurityMsgToUi;

/*安防服务*/
int security_init(pSecurityCallBackFuncTion       callbackFunc)
{
	int ret;
	
	
	securityState = getSecurityMode();
	LOGD("securityState = %d\n",securityState);
	upSecurityMsgToUi = callbackFunc;

	return 0;
fail0:
	return -1;
}
int security_layoutAlarm(void)
{
	securityState = E_ALAEM_SLEEP;
 	return 	setSecurityMode(E_ALAEM_SLEEP);
}

int security_repealAlarm(void)
{
	securityState = E_ALAEM_NOT_ACTIVITY;

	return 	setSecurityMode(E_ALAEM_NOT_ACTIVITY);
}

int security_delaySendAlarm(int         type)
{
	
	struct timespec  current_time;
	static struct timespec	last_time;
	clock_gettime(CLOCK_MONOTONIC, &current_time);
		//180S内只能报警一次
	if ((current_time.tv_sec - last_time.tv_sec)<ALARM_INTERVAL_TIME)
	{
			LOGD(" Please waiting..");
			return -1;
	}
	clock_gettime(CLOCK_MONOTONIC, &last_time);

	if( (type != E_JJ_ALARM) && (securityState == E_ALAEM_NOT_ACTIVITY)||(securityState == E_ALARM_RUNING ))
		return -1;

	//延时后产生报警铃声
	alarm_ringdown_TimerId = createTimerTaskServer(ALAEM_DELAY_TIME,40*1000,3,security_triggerAlarmTinkle
							&type,sizeof(type));
	alarm_ringdown_TimerId->start(alarm_ringdown_TimerId);
	
//	alarm_ringdown_TimerId  =  timer_taskRegister(ALAEM_DELAY_TIME,40*1000,3,security_triggerAlarmTinkle
//						   ,&type,sizeof(type));

						   
	//延时后发送报警信号
	alarm_sendMsg_TimerId = createTimerTaskServer(ALAEM_DELAY_TIME,0,1,security_sendMsgToManager
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
	
	if( type == E_JJ_ALARM)
	{
		//只上报一次
		security_triggerAlarmTinkle(&type);
	}else {
		if((securityState == E_ALAEM_NOT_ACTIVITY )||(securityState == E_ALARM_RUNING ) ){
			LOGD("securityState = %d",securityState);
			return -1;
		}
		//每隔40上报一次，总上报3次
		alarm_ringdown_TimerId->changeParameter(alarm_ringdown_TimerId,0,40*1000,3);
		alarm_ringdown_TimerId->start(alarm_ringdown_TimerId);
	}
	//只发送一次
	security_sendMsgToManager((void *)&type);
	return 0;
}

int security_stopAlarm(void)
{
	int ret;
	if(securityState == E_ALAEM_NOT_ACTIVITY)
		return -1;
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
	if(arg == NULL)
		return;
	int type = *((int *)arg);
	sendAlarmToManager(type);
}
static void security_setAlarmState(void *arg)
{
	if(arg == NULL)
		return;
	int type = *((int *)arg);
	switch(type){
		case E_ALAEM_NOT_ACTIVITY :
		case E_ALARM_READY:
		case E_ALARM_RUNING:
				securityState = type;
			break;
		case E_ALAEM_SLEEP:
			if(securityState == E_ALARM_RUNING )
				securityState = type;
			break;
		default:
			break;
	}
	LOGD("security_setAlarmState = %d\n",type);
			
}
static void security_triggerAlarmTinkle(void *arg)
{
	static int upCount  = 0;
	if(arg == NULL)
		return;
	int type = E_ALAEM_SLEEP;
	if(type != E_JJ_ALARM ){
		securityState = E_ALARM_RUNING;
		//timer_taskDestroy(alarm_state_TimerId);
		if(alarm_state_TimerId)
			destroyTimerTaskServer(&alarm_state_TimerId);
		
		//alarm_state_TimerId = timer_taskRegister(39*1000,0,1,security_setAlarmState
		//				   ,&type,sizeof(type));
		//这里有0.5/40的几率会产生BUG
		alarm_state_TimerId = createTimerTaskServer(39500,0,1,security_setAlarmState
					   ,&type,sizeof(type));
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
			break;
		case E_HW_ALARM:
			buildAlarmEvent(&alarmEvent,HW_EVENT);
			break;
		case E_YW_ALARM:
			buildAlarmEvent(&alarmEvent,YW_EVENT);
			break;
		case E_JJ_ALARM:
			buildAlarmEvent(&alarmEvent,JJ_EVENT);
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
	alarmEvent->type =  htonl(type);
	alarmEvent->id = htonl(0);
	alarmEvent->time = htonl(tp.tv_sec);   
	ret = getLocalRoom(&alarmEvent->room);
	CHECK_RET(ret < 0, "fail to getLocalRoom", goto fail0);
	return 0;
fail0:
	return -1;
}


























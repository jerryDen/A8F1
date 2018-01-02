#ifndef __SECURITY_H__
#define __SECURITY_H__




#define UI_ALARM_STOP    			0x10
#define UI_ALARM_DELAY_TRIGGER    	0x11
#define UI_ALARM_TRIGGER			0x12
#define UI_ALARM_SENDTOMANAGE       0X13
#define UI_ALARM_LAYOUT             0X14
#define UI_ALARM_REPEAL            	0X15
/*安防服务*/
#define ALAEM_DELAY_TIME  120*1000
typedef enum E_ALARM_STATE{
	E_ALAEM_SLEEP = 0 ,  			//安防空闲
	E_ALARM_READY,     			//准备上报事件
	E_ALARM_RUNING,    			//正在上报
}E_ALARM_STATE;

typedef enum{
	E_MQ_ALARM  = 0,
	E_HW_ALARM,
	E_MC_ALARM,
	E_YW_ALARM,
	E_JJ_ALARM,
}E_ALARM_TYPE;
	
typedef enum e_event_place{
	EVENT_PLACE_KT = 0,
	EVENT_PLACE_WS ,
	EVENT_PLACE_CF,
}e_event_place,*pe_event_place;


typedef int  (*pSecurityCallBackFuncTion)( int ,const   void * , int );



int security_init(pSecurityCallBackFuncTion       callbackFunc);
int security_layoutAlarm(void);
int security_repealAlarm(void);
int security_delaySendAlarm(int         type);
int security_promptlySendAlarm(int        type);
E_ALARM_STATE security_getAlarmState(void);
int security_stopAlarm(void);



#endif





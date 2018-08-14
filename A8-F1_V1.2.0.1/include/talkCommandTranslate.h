#ifndef   TALK_COMMAND_TRANSLATE__H_
#define   TALK_COMMAND_TRANSLATE__H_
#include "commStructDefine.h"



#define UI_BE_CALLED_RING		0x01 //有数据，数据结构为T_Room,表示来电者的信息
#define UI_TO_CALL_RING			0x02
#define UI_PEER_ANSWERED 		0x03
#define UI_PEER_HUNG_UP			0x04
#define UI_PEER_OFF_LINE		0x05
#define UI_PEER_NO_ANSWER		0x06
#define UI_PEER_LINE_BUSY		0x07
#define UI_TALK_TIME_OUT		0x08

#define	UI_WATCH_SUCCESS		0x09
#define UI_WATCH_TIME_OUT		0x0A

#define UI_OTHER_DEV_PROC 		0x0B
#define UI_HUNG_UP_SUCCESS		0x0C
#define UI_ANSWER_SUCESS		0x0D
#define UI_OPEN_LOCK_SUCESS 	0x0E
#define UI_UPDATE_TIME_WEATHRE	0x0F  //有数据，数据结构为　time_weather_t
#define UI_UPDATE_HOUSE_INFO	0x10 //有数据，数据结构为　T_Room,表示当前设备的房号信息
#define UI_ELEVATOR_CALL_SUCCESS 0x11
#define UI_ELEVATOR_CALL_FAIL	0x12
#define UI_DOORBEL_TRY_SUCCESS  0x13

#define UI_NETWORK_NOT_ONLINE    0x14 //通知UI 服务端 或本机已经掉线
#define UI_CONNECT_SERVER        0x15
#define UI_UPDATE_SYSCONFIG      0x16
#define UI_TFTP_UPDATE_APP       0x17  
#define UI_UPDATE_NETCONFIG		 0X18
#define UI_PEER_RST       	     0X19
#define UI_WAIT_LOCAL_HOOK_TIMEOUT		0X20











typedef int  (*pTalkCallBackFuncTion)(T_Room , unsigned char ,const   unsigned char * , int );


int talkCommandInit(T_Room room);
T_eTALK_STATUS talkGetWorkState(void);
int talkSetCallBackFunc(pTalkCallBackFuncTion callBackFunc);
int talkTranslateAnswerOrHuangUp(T_Room             destRoom);
int talkTranslateHuangUp(T_Room               destRoom);
int userSingleServerSendDataToDestRoom(T_Room destRoom , unsigned char cmd,const   unsigned char * pData, int len);

int callManager(void);
int callBAJer(void);
int openLock(T_Room destRoom);


void talkCommandExit(void);


#endif


































































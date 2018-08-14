#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h> 

#include "roomNetConfig.h"
#include "Utils.h"
#include "commStructDefine.h"
#include <sys/epoll.h>
#include "commonHead.h"
#include "talkCommandTranslate.h"
#include "commandWord.h"
#include "commNetConfig.h"
#include "timerTaskManage.h"
#include "cellInformation.h"
#include "localNetInfo.h"
#include "systemConfig.h"
#include "security.h"
#include "netUdpServer.h"
#include "threadManage.h"
#include "bufferManage.h"
typedef struct  StateMachinePack{
	T_eTALK_STATUS	tackState;
	T_Room destRoom; 
	pTimerOps timerId;
	void(*timeoutCallBackFunc)(void *);
}StateMachinePack,*pStateMachinePack;

typedef struct SendDataPack{
	T_Room destRoom ; 
	unsigned char cmd ;
	DataPacket_Static buf;
}SendDataPack,*pSendDataPack;
typedef struct AckWakePack{
	int mWakeReadPipeFd;
	int mWakeWritePipeFd;
	int readPipeEpollFd;
	int timeOut;
}AckWakePack,*pAckWakePack;


#define HEARTBEAT_TIME 10*60*1000 //每隔10分钟发送一次心跳

pUdpOps udpSingleServer;

static T_Room localRoom;
T_NetInterface  localNetInfo;

pThreadOps sendUdpDataThread;

pBufferOps   udpSendBuf;




pTimerOps delaySendBusyTimerId;
pTimerOps sendHeartbeatTimerId;


pTalkCallBackFuncTion upCmdtoUiFunction;
//等待应答cmd机制
pAckWakePack ackWakeCmdPack;

static pAckWakePack  waitAckPackInit(int timeoutMs);
static int  wakeAckWait(pAckWakePack waitPack,char cmd);
static int 	waitRemoteAck(pAckWakePack waitPack ,char cmd);
static void  waitAckPackDestroy(pAckWakePack * ackWakePack);
//状态机机制
pStateMachinePack stateMachinePack ;
static pStateMachinePack stateMachineInit(void);
static void stateMachineCallBack(void *arg);
static T_eTALK_STATUS getLocalTalkState(pStateMachinePack pack);
static int  stateMachinePackDestroy(pStateMachinePack * pack);
static void setTalkState(pStateMachinePack pack,T_eTALK_STATUS	tackState);
static void setTalkDestRoom(pStateMachinePack pack,T_Room destRoom);
static int  getTalkDestRoom(pStateMachinePack pack,pT_Room destRoom);




//网络指令服务
static void * udpSendThread(void *arg);
static MsgBody UdpBuildMsg(unsigned char ackType, unsigned char cmd, T_Room srcRoom,
		T_Room destRoom, const unsigned char *pData, int dataLen);

static int  udpSingleRecvCallBack (unsigned char* ,unsigned int);


static  int udpMulitRecvCallBack(unsigned char* data,unsigned int len);

static int ackcmdParseAndHandle(pT_Comm_Head pRecv, const void *recvData,int len, pUdpOps udpserver);
static int notAckcmdParseAndHandle(pT_Comm_Head pRecv, const void *recvData,int len,pUdpOps udpServer);


static int  ackCmdToRemote(T_Room destRoom , unsigned char cmd,unsigned char ackType, struct sockaddr_in remoteInfo);
static int  ackDataToRemote(T_Room destRoom , unsigned char cmd,unsigned char ackType, const void *data,int len,struct sockaddr_in remoteInfo);
static void mySleep(int time);




static int talkTranslateCmdToDestWaitAck(pUdpOps sendServer,T_Room destRoom , unsigned char cmd,const   unsigned char * pData, int len );
static int talkTranslateCmdToDestNotWaitAck(pUdpOps sendServer,T_Room destRoom, unsigned char cmd,unsigned char ackType,const   unsigned char * pData, int len );



//分机管理服务


static int sendMulitCmd(unsigned cmd ,T_Room destRoom);



static pUdpOps udpMulticastServer;

static void delaysendAckBusy(void *arg);

//广播服务

static pUdpOps udpBoardServer;


static int udpBoardCastRecvCallBack(unsigned char* ,unsigned int);

//心跳包服务
heart_beat_t heartbeatPack;

static void sendHeartbeat(void *arg);
static int initHeartbeatPack(p_heart_beat_t heartbeatPack );







int talkCommandInit(T_Room room)
{

	int result;
	char version[64];
	
	bzero(&heartbeatPack,sizeof(heart_beat_t));
	bzero(&version,sizeof(version));
	localRoom = room;
	result = getLocalNetInfo(&localNetInfo);
	

	udpSingleServer	  = createUdpServer(COMM_UNI_PORT);

	CHECK_RET(udpSingleServer == NULL, "fail to createUdpServer udpSingleServer ", goto fail0);
	


	udpSingleServer->setHandle(udpSingleServer,udpSingleRecvCallBack,NULL,NULL);
		


	
	//超时应答3000ms后从发
	ackWakeCmdPack = waitAckPackInit(3000);
	stateMachinePack = stateMachineInit();
	
	udpSendBuf = createBufferServer(256);
	CHECK_RET(udpSendBuf == NULL, "createBufferServer", goto fail0);
	
	sendUdpDataThread = pthread_register(udpSendThread,NULL,0,NULL);

	result = sendUdpDataThread->start(sendUdpDataThread);
	CHECK_RET(result<0, "fail to pthread_start", goto fail0);

	//udp组播服务,房间内有多分机的情况下才开启
	if(isMultiExtension() == 1){
		

	//创建组播服务
		udpMulticastServer  = createUdpServer(COMM_MULIT_PORT);
		CHECK_RET(udpMulticastServer == NULL, "fail to createUdpServer COMM_MULIT_PORT", goto fail0);
		udpMulticastServer->joinMulticast(udpMulticastServer,localNetInfo.multiaddr);
	//将处理函数加入到组播服务中,有数据时会调用udpMulitRecvCallBack
		udpMulticastServer->setHandle(udpMulticastServer,udpMulitRecvCallBack,NULL,NULL);

		
		


	}
	//udp广播服务
	udpBoardServer = createUdpServer(COMM_BROADCAST_PORT);
	CHECK_RET(udpBoardServer == NULL, "fail to udpBoardServer ", goto fail0);

	udpBoardServer->setHandle(udpBoardServer,udpBoardCastRecvCallBack,NULL,NULL);



	getAppVersion(version);
	LOGD("version:%s\n",version);

	//心跳包管理s
	
	result = initHeartbeatPack(&heartbeatPack);
	CHECK_RET(result  < 0 , "initHeartbeatPack", goto fail0);
	LOGD("seq:%d",getLocalSequence());
	

	sendHeartbeatTimerId = createTimerTaskServer(getLocalSequence()%20*1000,HEARTBEAT_TIME,-1,
		sendHeartbeat,&heartbeatPack,sizeof(heartbeatPack));
	sendHeartbeatTimerId->start(sendHeartbeatTimerId);
	
	return 0;
fail0:
	return -1;
	
}
static int initHeartbeatPack(p_heart_beat_t heartbeatPack )
{
	 CellInfo cellInfo;
	 bzero(&cellInfo,sizeof(cellInfo));
	 int ret;
	 ret  = getAppVersion(heartbeatPack->app_version);
	 CHECK_RET(ret <0 , "getAppVersion", goto fail0);
	 ret  = getCellInformation(&cellInfo);
	 CHECK_RET(ret <0 , "getCellInformation", goto fail0);
	 strcpy(heartbeatPack->room_version,cellInfo.fileHead.version);
	 heartbeatPack->eventSum = htonl(0);
	 heartbeatPack->typeCode  = 4;
	 return 0;
fail0:
	return -1;
}
static void sendHeartbeat(void *arg)
{
	T_Room  managerRom;
	getManagerRoom(&managerRom);
	talkTranslateCmdToDestNotWaitAck(udpSingleServer,managerRom ,HEARTBEAT,NOT_ACK,&heartbeatPack,sizeof(heart_beat_t));
}
static void mySleep(int time)
{
	int current  = time;
	while(current > 0 )
			current = sleep(current);
}

int talkSetCallBackFunc(pTalkCallBackFuncTion callBackFunc)
{
	
	upCmdtoUiFunction = callBackFunc;
	return 0;
}
int talkTranslateCmdToDestNotWaitAck(pUdpOps sendServer,T_Room destRoom, unsigned char cmd,unsigned char ackType,const   unsigned char * pData, int len )
{
	pthread_t  thread;
	S_NetDataPackage dataPack;
	int ret = -1;
	bzero(&dataPack,sizeof(S_NetDataPackage));
	dataPack.dataBody = UdpBuildMsg(ackType,cmd,localRoom,destRoom,pData,len);
	dataPack.remoteInfo.sin_port = COMM_UNI_PORT;//
	dataPack.sendServer = sendServer;
	if( 0 == getRoomIpAddr(destRoom,&dataPack.remoteInfo.sin_addr.s_addr)){

		return sendServer->write(sendServer,dataPack.dataBody.buf,
			dataPack.dataBody.len,dataPack.remoteInfo.sin_addr.s_addr,
			dataPack.remoteInfo.sin_port);

	}else{
		LOGE("fail to getRoomIpAddr!");
		return -1;
	}
	return ret;
fail0:
	return -1;
	
}

int talkTranslateCmdToDestWaitAck(pUdpOps sendServer,T_Room destRoom , unsigned char cmd,const   unsigned char * pData, int len )
{
	pthread_t  thread;
	S_NetDataPackage dataPack;
	int ret = -1;
	bzero(&dataPack,sizeof(S_NetDataPackage));
	if( 0 == getRoomIpAddr(destRoom,&dataPack.remoteInfo.sin_addr.s_addr)){
		dataPack.dataBody = UdpBuildMsg(NOT_ACK,cmd,localRoom,destRoom,pData,len);
		
		dataPack.remoteInfo.sin_port = COMM_UNI_PORT;//
		dataPack.sendServer = sendServer;
		ret = udpSendBuf->push(udpSendBuf,&dataPack,sizeof(dataPack));
		CHECK_RET(ret<0, "fail to pushDataToQueueHandle", goto fail0);
		return 0;
	}else{
		LOGE("fail to getRoomIpAddr!");
		return -1;
	}
	return ret;
fail0:
	return -1;

}
T_eTALK_STATUS talkGetWorkState(void)
{
	return getLocalTalkState(stateMachinePack);
}
int talkTranslateAnswerOrHuangUp(T_Room               destRoom)
{
	
	if(getLocalTalkState(stateMachinePack)  == WAIT_LOCAL_HOOK  )
	{	
		
		return talkTranslateCmdToDestWaitAck(udpSingleServer,destRoom,HOOK_CMD,NULL,0 );	
	}else if(getLocalTalkState(stateMachinePack)  == TALKING || 
		getLocalTalkState(stateMachinePack)  == WAIT_DES_HOOK)
	{
		
		return talkTranslateCmdToDestWaitAck(udpSingleServer,destRoom,UNHOOK_CMD,NULL,0 );		
	}
	return -1;
}
int talkTranslateHuangUp(T_Room               destRoom)
{
	 if(getLocalTalkState(stateMachinePack)  == TALKING || 
		getLocalTalkState(stateMachinePack)  == WAIT_DES_HOOK)
	{
		return talkTranslateCmdToDestWaitAck(udpSingleServer,destRoom,UNHOOK_CMD,NULL,0 );		
	}else if(getLocalTalkState(stateMachinePack)  == WAIT_LOCAL_HOOK )
	{
		//if( destRoom.type  == X6B_ZJ)
		{
			
			if(isMultiExtension() >0){
				LOGD("send udpMulticastServer RST_CMD");
				return sendMulitCmd(RST_CMD,destRoom);
			
			}else{
				LOGD("send udpSingleServer RST_CMD");
				return talkTranslateCmdToDestWaitAck(udpSingleServer,destRoom,RST_CMD,NULL,0 );	
			}
		}
	//	return talkTranslateCmdToDestWaitAck(udpSingleServer,destRoom,UNHOOK_CMD,NULL,0 );		
	}
	return -1;
	
}

int openLock(T_Room destRoom)
{

		
		if(getLocalTalkState(stateMachinePack)  == WAIT_LOCAL_HOOK ||
			getLocalTalkState(stateMachinePack)  == TALKING ||
			getLocalTalkState(stateMachinePack)  == WATCHING )
		{
			if((destRoom.type == 8 )&& (getLocalTalkState(stateMachinePack)  == TALKING ) )
				return 0;
			return talkTranslateCmdToDestWaitAck(udpSingleServer,destRoom,OPEN_CMD,NULL,0 );
			
		}else if(getLocalTalkState(stateMachinePack)  == WAIT_DES_HOOK){
			return 0;
		}
	return -1;
}
int userSingleServerSendDataToDestRoom(T_Room destRoom , unsigned char cmd,const   unsigned char * pData, int len)
{
	
	
	return talkTranslateCmdToDestWaitAck(udpSingleServer,destRoom,cmd,pData,len);	
}
int callManager(void)
{
	T_Room  managerRoom;
	int ret;
	ret  = getManagerRoom(&managerRoom);
	CHECK_RET(ret<0, "fail to getManagerRoom", goto fail0 );
	
	if(getLocalTalkState(stateMachinePack)  == TALK_FREE  )
	{
		return talkTranslateCmdToDestWaitAck(udpSingleServer,managerRoom,CALL_CMD,NULL,0 );	
	}else
		return -1;
fail0:
	return -1;
}
int callBAJer(void)
{
	T_Room  bajRoom;
	int ret;
	ret  = getBAJRoom(&bajRoom);
	CHECK_RET(ret<0, "fail to getManagerRoom", goto fail0 );
	
	if(getLocalTalkState(stateMachinePack)  == TALK_FREE  )
	{
		return talkTranslateCmdToDestWaitAck(udpSingleServer,bajRoom,CALL_CMD,NULL,0 );	
	}else
		return -1;
fail0:
	return -1;
}

/*
static int	ackDataToRemote(T_Room destRoom , unsigned char cmd,unsigned char ackType, const void *data,int len,struct sockaddr_in remoteInfo)
{
	MsgBody dataBody;
	dataBody = UdpBuildMsg(ackType,cmd,localRoom,destRoom,data,len);
		return udp_sendData(udpSingleSocketfd,dataBody.buf,
					dataBody.len,remoteInfo.sin_addr.s_addr,
					remoteInfo.sin_port);	
}
static int ackCmdToRemote(T_Room destRoom ,unsigned char cmd,unsigned char ackType, struct sockaddr_in remoteInfo)
{
	MsgBody dataBody;
	dataBody = UdpBuildMsg(ackType,cmd,localRoom,destRoom,NULL,0);

	return udp_sendData(udpSingleSocketfd,dataBody.buf,
				dataBody.len,remoteInfo.sin_addr.s_addr,
				remoteInfo.sin_port);	

}
*/
//0xaa 0xe3 0x2 0x2	0x1 0x2 0x1 0x0	0x0 0x1 0x1 0x1	0x2 0x1 0x2 0x1	0x2 0x88 
//0xaa 0xe3 0x6 0x2 0x1 0x2 0x1 0x0 0x0 0x1 0x1 0x1 0x2 0x1 0x2 0x1 0x2 0x73
static MsgBody UdpBuildMsg(unsigned char ackType, unsigned char cmd, T_Room srcRoom,
		T_Room destRoom, const unsigned char *pData, int dataLen) {
	MsgBody tmpMsg;
	int len = sizeof(T_Comm_Head) + dataLen;
	T_Comm_Head * pHead = (T_Comm_Head *) tmpMsg.buf;
	static unsigned char seq = 0;
	pHead->ackType = ackType;
	pHead->cmd = cmd;
	pHead->sequence = seq++;
	pHead->destAddr = destRoom;
	pHead->sorAddr = srcRoom;
	if (pData != NULL && dataLen > 0) {
		memcpy(&(pHead->dataStart), pData, dataLen);
	}
	
	tmpMsg.buf[len - 1] = getUtilsOps()->NByteCrc8(0, (unsigned char *) pHead, len - 1);
	tmpMsg.len = len;
	return tmpMsg;
}


void talkCommandExit(void)
{

	destroyUdpServer(&udpSingleServer);
	destroyUdpServer(&udpMulticastServer);
	destroyUdpServer(&udpBoardServer);

	
	destroyBufferServer(&udpSendBuf);
	
	pthread_destroy(&sendUdpDataThread);	
	destroyTimerTaskServer(&delaySendBusyTimerId);
	destroyTimerTaskServer(&sendHeartbeatTimerId);
	
	return 0;
}
static int udpBoardCastRecvCallBack(unsigned char*data  ,unsigned int len)
{
	MsgBody recvDataBuf;
	LOGD("udpBoard  len:%d ",len);
	getUtilsOps()->printData(data,len);
	bzero(&recvDataBuf,sizeof(MsgBody));
	if(data == NULL)
		return ;
	
	pT_Comm_Head pRecv = (pT_Comm_Head)(data);
	//data->remoteInfo.sin_port  = htons(COMM_UNI_PORT);
	
	if(len > sizeof(T_Comm_Head))
	{
		recvDataBuf.len = len -sizeof(T_Comm_Head) +1;
		memcpy(recvDataBuf.buf,&pRecv->dataStart,recvDataBuf.len );
		
		if(pRecv->ackType == NOT_ACK){
			notAckcmdParseAndHandle(pRecv,recvDataBuf.buf,recvDataBuf.len,udpBoardServer);
		}else {
			ackcmdParseAndHandle(pRecv,recvDataBuf.buf,recvDataBuf.len,udpBoardServer);
		}
	}else{
		if(pRecv->ackType == NOT_ACK){
			notAckcmdParseAndHandle(pRecv,NULL,0,udpBoardServer);
		}else{
			ackcmdParseAndHandle(pRecv,recvDataBuf.buf,recvDataBuf.len,udpBoardServer);
		}
	}
		
	return len;
}
static  int udpMulitRecvCallBack(unsigned char* data,unsigned int len)
{	
	MsgBody recvDataBuf;
	struct sockaddr_in sourceIp;
	LOGD("udpMulit: data len:%d",len);
	getUtilsOps()->printData(data,len);
	bzero(&recvDataBuf,sizeof(MsgBody));
	if(data == NULL)
		return -1;
	//PRINT_ARRAY(data->dataBody.buf, data->dataBody.len);
	pT_Comm_Head pRecv = (pT_Comm_Head)(data);
	//data->remoteInfo.sin_port  = htons(COMM_UNI_PORT);

	udpMulticastServer->getRemoteInfo(udpMulticastServer,&sourceIp);
	sourceIp.sin_port = htons(COMM_UNI_PORT);
	udpMulticastServer->setRemoteInfo(udpMulticastServer,sourceIp);
	
	if(len > sizeof(T_Comm_Head))
	{
		recvDataBuf.len = len -sizeof(T_Comm_Head) +1;
		memcpy(recvDataBuf.buf,&pRecv->dataStart,recvDataBuf.len );
		
		if(pRecv->ackType == NOT_ACK){
			notAckcmdParseAndHandle(pRecv,recvDataBuf.buf,recvDataBuf.len,udpMulticastServer);
		}else {
			ackcmdParseAndHandle(pRecv,recvDataBuf.buf,recvDataBuf.len,udpMulticastServer);
		}
	}else{
		if(pRecv->ackType == NOT_ACK){
			notAckcmdParseAndHandle(pRecv,NULL,0,udpMulticastServer);
		}else{
			ackcmdParseAndHandle(pRecv,recvDataBuf.buf,recvDataBuf.len,udpMulticastServer);
		}
	}
		
	return len;
}

static int  udpSingleRecvCallBack (unsigned char* data ,unsigned int len)
{
	MsgBody recvDataBuf;
	struct sockaddr_in sourceIp;
	bzero(&recvDataBuf,sizeof(MsgBody));
	if(data == NULL)
		return ;
	//PRINT_ARRAY(data->dataBody.buf, data->dataBody.len);
	LOGD("udpSingle:");
	getUtilsOps()->printData(data,len);
	pT_Comm_Head pRecv = (pT_Comm_Head)(data);
	//data->remoteInfo.sin_port  = htons(COMM_UNI_PORT);
	
	
	udpSingleServer->getRemoteInfo(udpSingleServer,&sourceIp);
	sourceIp.sin_port = htons(COMM_UNI_PORT);
	udpSingleServer->setRemoteInfo(udpSingleServer,sourceIp);

	if(len > sizeof(T_Comm_Head))
	{
		recvDataBuf.len = len -sizeof(T_Comm_Head) +1;
		memcpy(recvDataBuf.buf,&pRecv->dataStart,recvDataBuf.len );
		
		if(pRecv->ackType == NOT_ACK){
			notAckcmdParseAndHandle(pRecv,recvDataBuf.buf,recvDataBuf.len,udpSingleServer);
		}else {
			ackcmdParseAndHandle(pRecv,recvDataBuf.buf,recvDataBuf.len,udpSingleServer);
		}
	}else{
		if(pRecv->ackType == NOT_ACK){
			notAckcmdParseAndHandle(pRecv,NULL,0,udpSingleServer);
		}else{
			ackcmdParseAndHandle(pRecv,recvDataBuf.buf,recvDataBuf.len,udpSingleServer);
		}
	}
		
	return len;

}
static int ackcmdParseAndHandle(pT_Comm_Head pRecv, const void *recvData,int len, pUdpOps udpserver)
{
	//发送线程若收不到此应答，则会认为对方没收到数据，则再次发送.
	wakeAckWait(ackWakeCmdPack,pRecv->cmd);
	LOGD("recv ack cmd !\n");
	switch (pRecv->cmd) {
		case CALL_CMD:
			switch(pRecv->ackType)
			{
				case ACK_OK:
					if(getLocalTalkState(stateMachinePack)  == CALL_WAIT_ACK)
					{
						setTalkState(stateMachinePack, WAIT_DES_HOOK);
						setTalkDestRoom(stateMachinePack,pRecv->sorAddr);
						upCmdtoUiFunction(pRecv->sorAddr,UI_TO_CALL_RING,recvData,len);
						
					}
					break;
				case LINE_BUSY:
						setTalkState(stateMachinePack, TALK_FREE);
						upCmdtoUiFunction(pRecv->sorAddr,UI_PEER_LINE_BUSY,recvData,len);
					break;
					
				case ACK_ERR:		
				case NOT_ONLINE:	
				case PASS_ERR:		
					break;
					
			}

			break;
		case 	RST_CMD:
			LOGI("RST_CMD!!");
			{
				
				T_Room destRoom;
				if(0 == getTalkDestRoom(stateMachinePack, &destRoom))
				{
					if(memcmp(&destRoom,&(pRecv->sorAddr),sizeof(T_Room)) != 0   )
					{
						break;
					}
				}
				//ackCmdToRemote(pRecv->sorAddr,pRecv->cmd,ACK_OK,remoteInfo);
				
			    
				if(isMultiExtension() == 1){
					if(getLocalTalkState(stateMachinePack)  == WAIT_LOCAL_HOOK  ){
						sendMulitCmd(OTHER_DEV_PROC,destRoom);
					}
				}
			
				//upCmdtoUiFunction(pRecv->sorAddr,UI_PEER_HUNG_UP,recvData,len);
				setTalkState(stateMachinePack,TALK_FREE);
			}
		break;
		case	OPEN_CMD:
			switch(pRecv->ackType){
				case ACK_OK:{
						T_Room destRoom;
						if(0 == getTalkDestRoom(stateMachinePack, &destRoom))
						{
							if(memcmp(&destRoom,&(pRecv->sorAddr),sizeof(T_Room)) != 0   )
							{
								break;
							}
						}
						
						upCmdtoUiFunction(pRecv->sorAddr,UI_OPEN_LOCK_SUCESS,recvData,len);
						if(getLocalTalkState(stateMachinePack)  == WAIT_LOCAL_HOOK  ){
							
							setTalkState(stateMachinePack, TALK_FREE);
							if(isMultiExtension() == 1)
							{
								sendMulitCmd(OTHER_DEV_PROC,destRoom);
							}
						}
					}
					break;
				case ACK_ERR:
					
					break;
			}

			break;
		case	WATCH_CMD:
			switch(pRecv->ackType)
			{
				case ACK_OK:
					if(getLocalTalkState(stateMachinePack)  == WATCH_WAIT_ACK)
					{
								setTalkState(stateMachinePack, WATCHING);
								setTalkDestRoom(stateMachinePack,pRecv->sorAddr);
								upCmdtoUiFunction(pRecv->sorAddr,UI_WATCH_SUCCESS,recvData,len);
					}
					break;
				case LINE_BUSY:
						setTalkState(stateMachinePack, TALK_FREE);
						upCmdtoUiFunction(pRecv->sorAddr,UI_PEER_LINE_BUSY,recvData,len);
					break;
			}

		
			break;
		case 	HOOK_CMD:
			switch(pRecv->ackType)
			{
				case ACK_OK:{
						T_Room destRoom;
						if(0 == getTalkDestRoom(stateMachinePack, &destRoom))
						{
							if(memcmp(&destRoom,&(pRecv->sorAddr),sizeof(T_Room)) != 0   )
							{
								break;
							}
						}
						if(isMultiExtension() == 1){
							if(getLocalTalkState(stateMachinePack)  == WAIT_LOCAL_HOOK  ){
								sendMulitCmd(OTHER_DEV_PROC,destRoom);
							}
						}
						setTalkState(stateMachinePack, TALKING);
						setTalkDestRoom(stateMachinePack,pRecv->sorAddr);
						upCmdtoUiFunction(pRecv->sorAddr,UI_PEER_ANSWERED,recvData,len);
					}
					break;
				case LINE_BUSY:
					setTalkState(stateMachinePack, TALK_FREE);
					upCmdtoUiFunction(pRecv->sorAddr,UI_PEER_LINE_BUSY,recvData,len);
					break;
			}
			break;
		case 	UNHOOK_CMD:
			LOGI("UNHOOK_CMD!!!!!!");
			{
				T_Room destRoom;
				if(0 == getTalkDestRoom(stateMachinePack, &destRoom))
				{
					if(memcmp(&destRoom,&(pRecv->sorAddr),sizeof(T_Room)) != 0   )
					{
						break;
					}
				}
				if(isMultiExtension() == 1){
					if(getLocalTalkState(stateMachinePack)  == WAIT_LOCAL_HOOK  ){
						sendMulitCmd(OTHER_DEV_PROC,destRoom);
					}
				}
				upCmdtoUiFunction(pRecv->sorAddr,UI_HUNG_UP_SUCCESS,recvData,len);
				setTalkState(stateMachinePack,TALK_FREE);
			}
			break;
		case 	UPDATE_SYSCONFIG:
			LOGD("UPDATE_SYSCONFIG");
			{
				
				stSYS_CONFIG *p_tftp_sysconfig_data;
				p_tftp_sysconfig_data = recvData;
			//	if(0 == memcmp(&p_tftp_sysconfig_data->roomAddr,&localRoom,sizeof(T_Room) )  )
				{
					upCmdtoUiFunction(pRecv->sorAddr,UI_UPDATE_SYSCONFIG,recvData,len);
				}
			
			}

			break;
		case 	DOWNLOAD_CMD:
			break;
		case 	TIME_OUT:
			LOGI("TIME_OUT");
			setTalkState(stateMachinePack,TALK_FREE);
			break;
		case    OTHER_DEV_PROC:
			LOGD("OTHER_DEV_PROC");
			break;
		case 	READ_BLOCK_ROOM:
			break;
		case 	DEL_ALL_ROOM_CMD:
			break;
		case	ELEVATOR_ENABLE_FLOORS:
			break;
		case 	ELEVATOR_CALL_FLOOR:
			break;
		case 	OUTDOORBELL_RING:
			break;
		case 	DEL_ALL_CARD_CMD:
			break;
		case 	VF_ADJ_CMD:
			break;
		case 	UPLOAD_PARAMET:
			break;
		case 	WARN_CMD:
			break;
		case 	PRE_SM_CMD:
			break;
		case 	NEXT_SM_CMD:
			break;
		case 	DEL_SM_CMD:
			break;
		case 	UPDATE_NETCONFIG:
		{
				int * updateDataLen =  (int *)recvData;
				
				len  = len - sizeof(int) - 1;//长度+校验位
				if( *updateDataLen == len)
				{
					LOGD("准备升级！ 长度:%d",*updateDataLen);
					 //偏移一个长度位
					upCmdtoUiFunction(pRecv->sorAddr,UI_UPDATE_NETCONFIG,recvData + sizeof(int),*updateDataLen);
				}else{

					LOGE("数据长度不对！ 实际接收到:%d字节,本包一共有:%d字节",len,*updateDataLen);
				}
		}
			break;
		case 	TFTP_UPDATE_APP :
			LOGD("TFTP_UPDATE_APP");
			{
				T_TFTP_UPDATE *tftpUpdate;
				tftpUpdate = recvData;
			
				if( tftpUpdate->serverip == 0)
				{
					if(udpserver != NULL )
					{
						struct sockaddr_in remoteInfo;
						if(0 == udpserver->getRemoteInfo(udpserver,&remoteInfo)){
							tftpUpdate->serverip = remoteInfo.sin_addr.s_addr;
						}
					}
					
				}
				upCmdtoUiFunction(pRecv->sorAddr,UI_TFTP_UPDATE_APP,recvData,len);
			}
		break;
		case 	READ_ROUTING_TAB:
			break;
		case 	DEL_ALL_ROUTING	:
			break;
		case  	MODIFY_ROUTING_TAB:
			break;
		case	BOARDCAST_SERVER:
			break;
		case 	BOARDCAST_STATE:
			break;
		case 	UPDATE_ONLINE_PEERS:
			break;
		case 	SET_HOUSE_INFO:
			break;
		case 	SET_DEV_INFO:
			break;
		case 	GET_OUTDOORBELL_NUM:
			break;
		default:
			break;
	}
	
	return 0; 
}

static void delaysendAckBusy(void *arg)
{
	
	LOGD("ack busy!");

	struct{
			T_Comm_Head cmd;
			struct sockaddr_in remoteinfo;
	}timerArg;
	
	memcpy(&timerArg,arg,sizeof(timerArg));
	MsgBody dataBody;
	dataBody = UdpBuildMsg(LINE_BUSY,timerArg.cmd.cmd,localRoom,timerArg.cmd.sorAddr,NULL,0);
	udpSingleServer->write(udpSingleServer,dataBody.buf,dataBody.len,
		timerArg.remoteinfo.sin_addr.s_addr ,ntohs(timerArg.remoteinfo.sin_port));
	LOGD("ack end!");
		
}

static int notAckcmdParseAndHandle(pT_Comm_Head pRecv, const void *recvData,int len,pUdpOps udpServer)
{
	
	struct timeval tv; 
	p_time_weather_t timrData;
	MsgBody dataBody;
	bzero(&dataBody,sizeof(MsgBody));
	LOGD("recv notack cmd !\n");
	switch (pRecv->cmd) {	
		case 	SET_TIME :{
			timrData = (p_time_weather_t)recvData;	
			tv.tv_sec  =  ntohl(timrData->sec);
			tv.tv_usec =  ntohl(timrData->usec);
			LOGD("SET_TIME !!");
			if(settimeofday (&tv, (struct timezone *) 0) < 0)  
		    {  
		    	LOGE("Set system datatime error!/n");  
		    	break; 
		    } 
			
			dataBody = UdpBuildMsg(ACK_OK,pRecv->cmd,localRoom,pRecv->sorAddr,(const void *)&heartbeatPack,sizeof(heartbeatPack));
			udpServer->ack(udpServer,dataBody.buf,dataBody.len);
			
		}
			break;

		case 	SET_SYS_CONFIG:
		
			
			break;
		case 	READ_SYS_CONFIG :
			break;
		case 	TFTP_UPDATE_APP :
			{
				T_TFTP_UPDATE *tftpUpdate;
				tftpUpdate = recvData;
			
				if( tftpUpdate->serverip == 0)
				{
					if(udpServer != NULL )
					{
						struct sockaddr_in remoteInfo;
						if(0 == udpServer->getRemoteInfo(udpServer,&remoteInfo)){
							tftpUpdate->serverip = remoteInfo.sin_addr.s_addr;
						}
					}
					
				}
				upCmdtoUiFunction(pRecv->sorAddr,UI_TFTP_UPDATE_APP,recvData,len);
			}
			break;
		case 	INTERNET_CALL_CMD:
			break;//add by alvin
		case 	SMS_CMD:
			break;
		case 	CALL_CMD:
			LOGI("收到呼叫命令\n");
			
			if((getLocalTalkState(stateMachinePack) == TALK_FREE ) &&(security_getAlarmState() != E_ALARM_RUNING) ){

				//ackCmdToRemote(pRecv->sorAddr,pRecv->cmd,ACK_OK,remoteInfo);

				
				dataBody = UdpBuildMsg(ACK_OK,pRecv->cmd,localRoom,pRecv->sorAddr,NULL,0);
				udpServer->ack(udpServer,dataBody.buf,dataBody.len);
		
				
				upCmdtoUiFunction(pRecv->sorAddr,UI_BE_CALLED_RING,recvData,len);
				setTalkState(stateMachinePack,WAIT_LOCAL_HOOK);
				setTalkDestRoom(stateMachinePack,pRecv->sorAddr);
			}else
			{	
				struct{
					T_Comm_Head cmd;
					struct sockaddr_in remoteinfo;
				}timerArg;
				LOGD("本机忙!\n");
				if(isMultiExtension() == 1){
	
					timerArg.cmd = *pRecv;
					 if(0 == udpServer->getRemoteInfo(udpServer,&timerArg.remoteinfo))
					 {
					 	if(delaySendBusyTimerId )
						{
							destroyTimerTaskServer(&delaySendBusyTimerId);
						}
						delaySendBusyTimerId = createTimerTaskServer(1000,0,1,delaysendAckBusy
									   ,&timerArg,sizeof(timerArg));
						delaySendBusyTimerId->start(delaySendBusyTimerId);
					 	
					 }

								   
				}else
				{
					MsgBody dataBody;
					dataBody = UdpBuildMsg(LINE_BUSY,pRecv->cmd,localRoom,pRecv->sorAddr,NULL,0);
					udpServer->ack(udpServer,dataBody.buf,dataBody.len);
				
				}
			}
			break;
		case 	RST_CMD:{
				LOGI("RST_CMD!!");
				T_Room destRoom;
				MsgBody dataBody;
				if(security_getAlarmState() == E_ALARM_RUNING)
					break;
				if(0 == getTalkDestRoom(stateMachinePack, &destRoom))
				{
					if(memcmp(&destRoom,&(pRecv->sorAddr),sizeof(T_Room)) != 0   )
					{
						break;
					}
				}
				dataBody = UdpBuildMsg(ACK_OK,pRecv->cmd,localRoom,pRecv->sorAddr,NULL,0);
				udpServer->ack(udpServer,dataBody.buf,dataBody.len);
			
				
				if(getLocalTalkState(stateMachinePack) == TALKING )
				{
					upCmdtoUiFunction(pRecv->sorAddr,UI_PEER_HUNG_UP,recvData,len);
					
				}else {
					upCmdtoUiFunction(pRecv->sorAddr,UI_PEER_RST,recvData,len);
				}
				
				setTalkState(stateMachinePack,TALK_FREE);
			}
			break;
		case 	REG_USER_PASSWORD:
			break;
		case 	DEL_ROOM_CMD:
			break;
		case 	REG_CARD_CMD:
			break;
		case 	DEL_CARD_CMD:
			break;
		case	OPEN_CMD:
			break;
		case	WATCH_CMD:
			break;
		case 	HOOK_CMD:
			LOGD(" NOTACK HOOK_CMD !");
			dataBody = UdpBuildMsg(ACK_OK,pRecv->cmd,localRoom,pRecv->sorAddr,NULL,0);
			udpServer->ack(udpServer,dataBody.buf,dataBody.len);
			if(getLocalTalkState(stateMachinePack) == WAIT_DES_HOOK)
			{
				
				setTalkState(stateMachinePack, TALKING);
				setTalkDestRoom(stateMachinePack,pRecv->sorAddr);
				
				upCmdtoUiFunction(pRecv->sorAddr,UI_PEER_ANSWERED,recvData,len);


			}


			
			break;
		case 	UNHOOK_CMD:
			LOGD("UNHOOK_CMD !");
			
			//if(memcpy(&localRoom,&(pRecv->sorAddr),sizeof(localRoom)) != 0)   break;
			T_Room destRoom;
			if(0 == getTalkDestRoom(stateMachinePack, &destRoom))
			{
				if(memcmp(&destRoom,&(pRecv->sorAddr),sizeof(T_Room)) != 0   )
				{
					break;
				}
			}

			if(getLocalTalkState(stateMachinePack) == TALKING)
			{
				dataBody = UdpBuildMsg(ACK_OK,pRecv->cmd,localRoom,pRecv->sorAddr,NULL,0);
				udpServer->ack(udpServer,dataBody.buf,dataBody.len);
			
				setTalkState(stateMachinePack, TALK_FREE);
				upCmdtoUiFunction(pRecv->sorAddr,UI_PEER_HUNG_UP,recvData,len);
			}else if(getLocalTalkState(stateMachinePack) ==WAIT_LOCAL_HOOK  ){
				
				dataBody = UdpBuildMsg(ACK_OK,pRecv->cmd,localRoom,pRecv->sorAddr,NULL,0);
				udpServer->ack(udpServer,dataBody.buf,dataBody.len);
				//ackCmdToRemote(pRecv->sorAddr,pRecv->cmd,ACK_OK,remoteInfo);
				setTalkState(stateMachinePack, TALK_FREE);
				upCmdtoUiFunction(pRecv->sorAddr,UI_PEER_HUNG_UP,recvData,len);
				
			}else {
				
				dataBody = UdpBuildMsg(ACK_ERR,pRecv->cmd,localRoom,pRecv->sorAddr,NULL,0);
				udpServer->ack(udpServer,dataBody.buf,dataBody.len);
				
				//ackCmdToRemote(pRecv->sorAddr,pRecv->cmd,ACK_ERR,remoteInfo);
			}
			break;
		case 	UPDATE_SYSCONFIG:{
				LOGD("UPDATE_SYSCONFIG");
				stSYS_CONFIG *p_tftp_sysconfig_data;
				p_tftp_sysconfig_data = recvData;
			//	if(0 == memcmp(&p_tftp_sysconfig_data->roomAddr,&localRoom,sizeof(T_Room) )  )
				{
					upCmdtoUiFunction(pRecv->sorAddr,UI_UPDATE_SYSCONFIG,recvData,len);
				}
				
			}
			break;
		case 	DOWNLOAD_CMD:
			break;
		case 	TIME_OUT:
			LOGI("TIME_OUT");
			setTalkState(stateMachinePack,TALK_FREE);
			break;
		case    OTHER_DEV_PROC:
			{
				T_Room destRoom;
				LOGI("OTHER_DEV_PROC");
				//destRoom是对讲状态时的目的IP
				//pRecv->sorAddr 是发消者的设置目的IP
				//
				if(0 == getTalkDestRoom(stateMachinePack, &destRoom))
				{
					if(memcmp(&destRoom,&(pRecv->destAddr),sizeof(T_Room)) != 0  )
					{
						//说明正在对讲状态的房号,跟所需要挂断的房号不一样
						break;
					}
				}
				
				
				if(getLocalTalkState(stateMachinePack) == WAIT_LOCAL_HOOK){
					
					upCmdtoUiFunction(pRecv->sorAddr,UI_OTHER_DEV_PROC,recvData,len);
					setTalkState(stateMachinePack,TALK_FREE);
				}
			}
			break;
		case 	READ_BLOCK_ROOM:
			break;
		case 	DEL_ALL_ROOM_CMD:
			break;
		case	ELEVATOR_ENABLE_FLOORS:
			break;
		case 	ELEVATOR_CALL_FLOOR:
			break;
		case 	OUTDOORBELL_RING:
			break;
		case 	DEL_ALL_CARD_CMD:
			break;
		case 	VF_ADJ_CMD:
			break;
		case 	UPLOAD_PARAMET:
			break;
		case 	WARN_CMD:
			break;
		case 	PRE_SM_CMD:
			break;
		case 	NEXT_SM_CMD:
			break;
		case 	DEL_SM_CMD:
			break;
		case 	UPDATE_NETCONFIG:
			{
				int * updateDataLen =  (int *)recvData;
				
				len  = len - sizeof(int) - 1;//长度+校验位
				if( *updateDataLen == len)
				{
					LOGD("准备升级！ 长度:%d",*updateDataLen);
					 //偏移一个长度位
					upCmdtoUiFunction(pRecv->sorAddr,UI_UPDATE_NETCONFIG,recvData + sizeof(int),*updateDataLen);
				}else{

					LOGE("数据长度不对！ 实际接收到:%d字节,本包一共有:%d字节",len,*updateDataLen);
				}
			}
		
			break;
		case 	READ_ROUTING_TAB:
			break;
		case 	DEL_ALL_ROUTING	:
			break;
		case  	MODIFY_ROUTING_TAB:
			break;
		case	BOARDCAST_SERVER:
			break;
		case 	BOARDCAST_STATE:
			break;
		case 	UPDATE_ONLINE_PEERS:
			break;
		case 	SET_HOUSE_INFO:
			break;
		case 	SET_DEV_INFO:
			break;
		case 	GET_OUTDOORBELL_NUM:
			break;
		case REBOOT_CMD:{
				if(memcmp(&localRoom,&(pRecv->destAddr),sizeof(T_Room)) != 0   )
				{
					break;
				}
				dataBody = UdpBuildMsg(ACK_OK,pRecv->cmd,localRoom,pRecv->sorAddr,NULL,0);
				udpServer->ack(udpServer,dataBody.buf,dataBody.len);
				system("reboot");
			}
			break;
		default:
			break;

	}

	return 0; 
	
}

static T_eTALK_STATUS  cmdToTalkState(unsigned char cmd)
{
	T_eTALK_STATUS state;
	switch(cmd)
	{
		case WATCH_CMD:
			state = WATCH_WAIT_ACK;
			break;
		case CALL_CMD:
			state = CALL_WAIT_ACK;
			break;
		default:
			state = 0;
			break;
	}
	return state;
}

static void * udpSendThread(void *arg)
{
	S_NetDataPackage  sendPack;
	int timeout;
	unsigned sendCmd;
	int waitRet;
	T_Room destRoom;
	T_eTALK_STATUS temp_state = 0;
	int pullret = 0;
	pUdpOps SendUpdServer;
	while(sendUdpDataThread->check(sendUdpDataThread ) == Thread_Run)
	{
		bzero(&sendPack,sizeof(S_NetDataPackage));
		waitRet = udpSendBuf->wait(udpSendBuf); //等待数据
		if( TRI_DATA == waitRet ){
			
			pullret = udpSendBuf->pull(udpSendBuf,&sendPack,sizeof(S_NetDataPackage)); //提取数据
			if(sizeof(S_NetDataPackage) == pullret)
			{
				udpSendBuf->deleteLeft(udpSendBuf,pullret); //删除数据
				timeout = 0;
				sendCmd = ((pT_Comm_Head)(sendPack.dataBody.buf))->cmd;
				destRoom = ((pT_Comm_Head)(sendPack.dataBody.buf))->destAddr;
				
	sendAgain:
				if(( temp_state = cmdToTalkState(sendCmd) ) >0 )
				{
					LOGD("send data to remote!");
					setTalkState(stateMachinePack,temp_state);
				}
				SendUpdServer = (pUdpOps)sendPack.sendServer;
				//getUtilsOps()->printData(sendPack.dataBody.buf,sendPack.dataBody.len);
				SendUpdServer->write(SendUpdServer,sendPack.dataBody.buf,
				sendPack.dataBody.len,sendPack.remoteInfo.sin_addr.s_addr,
				sendPack.remoteInfo.sin_port);

				
				if(waitRemoteAck(ackWakeCmdPack,(sendCmd)) == -1 ){
					
					if(timeout ++ < 1)
						goto sendAgain;
					else
						switch(sendCmd)
						{
							case HEARTBEAT:
							case WARN_CMD:
								upCmdtoUiFunction(destRoom,UI_NETWORK_NOT_ONLINE,NULL,0);
								break;
							default:
								setTalkState(stateMachinePack,TALK_FREE);
								upCmdtoUiFunction(destRoom,UI_PEER_OFF_LINE,NULL,0);
								break;
						}
						
						
					//上报异常消息到UI
					
				}
				//发送成功		
			}
		}
		else if(waitRet == TRI_EXIT)
		{
			goto exit0;
		}
	}
exit0:
	return NULL;
}

static pStateMachinePack stateMachineInit(void)
{
	
	pStateMachinePack pack = (pStateMachinePack)malloc(sizeof(StateMachinePack));
	bzero(pack,sizeof(StateMachinePack));
	pack->tackState = TALK_FREE;
	pack->timeoutCallBackFunc = stateMachineCallBack;
	return pack;
}
static void setTalkDestRoom(pStateMachinePack pack,T_Room destRoom)
{
	if(pack)
	{
		pack->destRoom = destRoom;
	}
}
static int getTalkDestRoom(pStateMachinePack pack,pT_Room destRoom)
{
	if(pack&&destRoom)
	{
		 *destRoom = pack->destRoom;
		return 0;
	}
	return -1;
}
static void setTalkState(pStateMachinePack pack,T_eTALK_STATUS	tackState)
{
	if(pack == NULL)
		return;
	char * debugStr[] = {
	"DEVICE_OFFLINE",
		"TALK_FREE", //空闲状态
	"CALL_WAIT_ACK", 
	"WAIT_LOCAL_HOOK",
	"WAIT_DES_HOOK",
	"TALKING", 
	"WATCH_WAIT_ACK",
	"WATCHING"};
//	timer_taskDestroy(pack->timerId);
	destroyTimerTaskServer(&pack->timerId);
	
	LOGD("set tackState = %s\n",debugStr[tackState]);
	pack->tackState  = tackState;
	switch(tackState )
	{
		case DEVICE_OFFLINE:
				break;
		case TALK_FREE:	
		
				break;
		case CALL_WAIT_ACK:
		
				break;
		case WAIT_LOCAL_HOOK:
			
			pack->timerId  = createTimerTaskServer(60*1000,0,1,pack->timeoutCallBackFunc,pack,sizeof(StateMachinePack));
			pack->timerId->start(pack->timerId );
			break;

		case WAIT_DES_HOOK:	
			pack->timerId  = createTimerTaskServer(60*1000,0,1,pack->timeoutCallBackFunc,pack,sizeof(StateMachinePack));
				pack->timerId->start(pack->timerId );
			break;
		case TALKING:
			pack->timerId  = createTimerTaskServer(121*1000,0,1,pack->timeoutCallBackFunc,pack,sizeof(StateMachinePack));
			pack->timerId->start(pack->timerId );
			break;
		case WATCH_WAIT_ACK:
		
				break;
		case WATCHING:
			pack->timerId  = createTimerTaskServer(60*1000,0,1,pack->timeoutCallBackFunc,pack,sizeof(StateMachinePack));
			pack->timerId->start(pack->timerId );
			break;
		default:
				break;	
		
	}
	
	
	
}
static T_eTALK_STATUS getLocalTalkState(pStateMachinePack pack)
{
	return pack->tackState;
}
static int stateMachinePackDestroy(pStateMachinePack * pack)
{
	
	if(pack != NULL &&*pack != NULL){
		free(*pack);
		*pack = NULL;
		return 0;
	}
	return -1;
}

static void stateMachineCallBack(void *arg)
{
		pStateMachinePack dataPack = (pStateMachinePack)arg;
		LOGD("dataPack->tackState = 0x%x",dataPack->tackState);
	
	switch(dataPack->tackState)
	{

	case DEVICE_OFFLINE:
			break;
	case TALK_FREE:
			break;
	case CALL_WAIT_ACK:
			upCmdtoUiFunction(dataPack->destRoom,UI_PEER_NO_ANSWER,NULL,0);
			break;
	case WAIT_LOCAL_HOOK:
			upCmdtoUiFunction(dataPack->destRoom,UI_WAIT_LOCAL_HOOK_TIMEOUT,NULL,0);
			break;
	case WAIT_DES_HOOK:
			upCmdtoUiFunction(dataPack->destRoom,UI_PEER_NO_ANSWER,NULL,0);
		    break;
	case TALKING:
			upCmdtoUiFunction(dataPack->destRoom,UI_TALK_TIME_OUT,NULL,0);
		    break;
	case WATCH_WAIT_ACK:
			upCmdtoUiFunction(dataPack->destRoom,UI_PEER_NO_ANSWER,NULL,0);
			break;
	case WATCHING:
			upCmdtoUiFunction(dataPack->destRoom,UI_WATCH_TIME_OUT,NULL,0);
			
			break;
	default:
			break;	
	}
	setTalkState(stateMachinePack,TALK_FREE);
}

static pAckWakePack waitAckPackInit(int timeoutMs )
{
	
	int wakeFds[2];
	int result;
	pAckWakePack pPack = malloc(sizeof(AckWakePack));
	CHECK_RET(pPack == NULL, "fail to mallocpAckWakePack",
            goto fail0);
	struct epoll_event eventItem;
	result = pipe(wakeFds);
	CHECK_RET(result != 0,"fail to queue_init!",goto fail0);
	pPack->mWakeReadPipeFd  = 	wakeFds[0];
	pPack->mWakeWritePipeFd =  wakeFds[1];
	result = fcntl(pPack->mWakeReadPipeFd , F_SETFL, O_NONBLOCK);
    CHECK_RET(result != 0, "Could not make wake read pipe non-blocking",
            goto fail0);
		result = fcntl(pPack->mWakeWritePipeFd , F_SETFL, O_NONBLOCK);
    CHECK_RET(result != 0, "Could not make wake read pipe non-blocking",
            goto fail0);
	pPack->timeOut = timeoutMs;//2S
	pPack->readPipeEpollFd = epoll_create(1);
	CHECK_RET(pPack->readPipeEpollFd<0, "epoll_create", goto fail0);

	memset(&eventItem, 0, sizeof(struct epoll_event));
	eventItem.data.fd = pPack->mWakeReadPipeFd;  
 	eventItem.events = EPOLLIN ;//表示句柄可读就唤醒
	result = epoll_ctl(pPack->readPipeEpollFd, EPOLL_CTL_ADD,pPack->mWakeReadPipeFd, &eventItem);
	CHECK_RET(result<0, "epoll_ctl", goto fail0);

	return pPack;
	fail0:
		return NULL;
	
}
static int waitRemoteAck(pAckWakePack waitPack,char cmd)
{
	if(waitPack == NULL)
		return -1;

	struct epoll_event eventItem;
	int pollResult ;
	char buf[16];
    ssize_t nRead;
againWait:
	pollResult = epoll_wait(waitPack->readPipeEpollFd, &eventItem, 1, waitPack->timeOut);//
	if( pollResult == 0)
	{
		LOGD("epoll_wait timeout!");
		return -1;
	}else if( pollResult >0 ){ 
	    do {
	        nRead = read(waitPack->mWakeReadPipeFd, buf, sizeof(buf));
	    } while (nRead == sizeof(buf));

		
		if(buf[nRead-1] == cmd  )
			return  0;
		else
			goto againWait;
	}else{
		LOGD(" fail to epoll_wait");
		return -1;

	}

}
static void  waitAckPackDestroy(pAckWakePack * ackWakePack)
{
	
	close((*ackWakePack)->readPipeEpollFd);
	close((*ackWakePack)->mWakeReadPipeFd);
	close((*ackWakePack)->mWakeWritePipeFd);
	free(*ackWakePack);
	*ackWakePack = NULL;

}


static int  wakeAckWait(pAckWakePack waitPack,char cmd)
{
	ssize_t nWrite;
	LOGD("wakeAckWait\n");
    do {
        nWrite = write(waitPack->mWakeWritePipeFd, &cmd, 1);
    } while (nWrite == -1 && errno == EINTR);

    if (nWrite != 1 && errno != EAGAIN) {
        LOGE("Could not write wake signal, errno=%d", errno);
		return -1;
    }
	return 0;	
}


/*组播服务*/



static int sendMulitCmd(unsigned cmd ,T_Room destRoom)
{
	MsgBody  msg;
	msg =  UdpBuildMsg(NOT_ACK,cmd, localRoom,destRoom,NULL,0);
	return udpMulticastServer->write(udpMulticastServer,msg.buf,msg.len,localNetInfo.multiaddr,COMM_MULIT_PORT);
}




























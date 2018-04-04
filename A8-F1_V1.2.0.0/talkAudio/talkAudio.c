#include "DebugLog.h"
#include "localNetInfo.h"
#include "commNetConfig.h"
#include "hiAudio.h"
#include "uscam_audio.h"
#include "commonHead.h"
#include "systemConfig.h"
#include <arpa/inet.h>
#include "netUdpServer.h"

//static int sendSocktFd;
static pUdpOps sendUdpServer;

//static int recvSocktFd;
static pUdpOps recvUdpServer;
static int destIpAddr;

static  enum {
	AUDIO_FREE = 0,
	AUDIO_TALKING,
}talkAudioState;

static pthread_mutex_t talkAudioState_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t sendUdpServer_mutex = PTHREAD_MUTEX_INITIALIZER;




static int audioDataRecvThread(unsigned char* ,unsigned int);

static int initSendSocket(void);
static int initRecvSocket(void);
static int sendTalkAudio(const struct timeval *tv,void *buf,uint32_t size);





 int usAudioOpen(void)
{
	int ret;
	UsCamAudioAttr attr;
	attr.cbm = sendTalkAudio;
	attr.sampleRate = 8000;
	attr.vqemode = 0;
	//UsSCamAudioDupluxOpen(, getConfigVol(TALKVOL));
	ret  = UsCamAudioOpen(&attr);		
	CHECK_RET(ret != 0, "fail to UsCamAudioOpen!", goto fail0);
	return 0;
fail0:
	return 1;
}
void usAudioClose(void)
{
 	 UsCamAudioClose();
}


int  talkAudioInit(void)
{
	int ret;
	ret  = initSendSocket();
	CHECK_RET(ret != 0, "fail to initSendSocket!", goto fail0);
	ret  = initRecvSocket();
	CHECK_RET(ret != 0, "fail to initRecvSocket!", goto fail0);
	
	talkAudioState = AUDIO_FREE;
	UsCamSysInit();
	return 0;
fail0:
	return -1;
}




int  talkAudioStart(int destIP)
{
	int ret ; 
	if(talkAudioState == AUDIO_TALKING)
		return -1;
	ret = UsCamSysInit();
	CHECK_RET(ret < 0, "fail to udp_startFdRecv!", goto fail0);
	ret = us_talkInitPcm();
	CHECK_RET(ret < 0, "fail to us_talkInitPcm!", goto fail0);
	ret = UsCamAudioStart();
	CHECK_RET(ret < 0, "fail to UsCamAudioStart!", goto fail0);
	destIpAddr  = destIP;
	pthread_mutex_lock(&talkAudioState_mutex);
	usAudioOpen();
	talkAudioState = AUDIO_TALKING;
	pthread_mutex_unlock(&talkAudioState_mutex);
	system("himm 0x20180200  0x80");
	return 0;
fail0:
	return -1;
}

int talkAudioStop(void)
{
	int ret ;
	if(talkAudioState == AUDIO_FREE)
		return -1;
	UsCamSysDeInit();
	ret  = UsCamAudioStop();
	
	system("himm 0x20180200  0x00");
	CHECK_RET(ret < 0, "fail to UsCamAudioStop!", goto fail0);
	pthread_mutex_lock(&talkAudioState_mutex);
	talkAudioState = AUDIO_FREE;
	pthread_mutex_unlock(&talkAudioState_mutex);

fail0:
	return -1;
}
static int sendTalkAudio(const struct timeval *tv,void *buf,uint32_t size)
{
	//发送前先设置远程主机IP地址及端口

	pthread_mutex_lock(&sendUdpServer_mutex);
	if(sendUdpServer)
	{
		pthread_mutex_unlock(&sendUdpServer_mutex);
		return sendUdpServer->write(sendUdpServer,buf,size,destIpAddr,AUDIO_PORT);

	}
	pthread_mutex_unlock(&sendUdpServer_mutex);
	return -1;
}
static int initSendSocket(void)
{
	int on,ret;
	struct in_addr  local_addr;


	sendUdpServer = createUdpServer( 0);

	
	CHECK_RET(sendUdpServer == NULL, "fail to udp_getSocketFd", goto fail0);
	on = (8 * 1024) * (2);   /* 发送缓冲区大小为8K */

	ret  = sendUdpServer->setsockopt(sendUdpServer,SOL_SOCKET, SO_SNDBUF, &on, sizeof(on));
	CHECK_RET(ret < 0, "fail to setsockopt sendsocket fd", goto fail0);
	//设置接收缓冲区大小 
	on = (8 * 1024) * (2);	  /* 接收缓冲区大小为8K */
	ret  = sendUdpServer->setsockopt(sendUdpServer,SOL_SOCKET, SO_RCVBUF, &on, sizeof(on));
	CHECK_RET(ret < 0, "fail to setsockopt sendsocket fd", goto fail0);
	on = 1;
	ret  = sendUdpServer->setsockopt(sendUdpServer,SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	CHECK_RET(ret < 0, "fail to setsockopt sendsocket fd", goto fail0);
	on = 1;
	ret  = sendUdpServer->setsockopt(sendUdpServer, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
	CHECK_RET(ret < 0, "fail to setsockopt", goto fail0);
	
	local_addr.s_addr = getLocalIpaddr();
	ret  = sendUdpServer->setsockopt(sendUdpServer,SOL_IP, IP_MULTICAST_IF, &local_addr, sizeof(struct in_addr));
	CHECK_RET(ret < 0, "fail to setsockopt", goto fail0);
	on = 0;
	ret  = sendUdpServer->setsockopt(sendUdpServer,SOL_IP, IP_MULTICAST_LOOP, &on, sizeof(on));
	CHECK_RET(ret < 0, "fail to setsockopt", goto fail0);
	
	return 0;
fail0:
	return -1;
}
static  int initRecvSocket(void)
{
	int on,ret;
	recvUdpServer = createUdpServer( AUDIO_PORT);
	CHECK_RET(recvUdpServer == NULL, "fail to udp_getSocketFd", goto fail0);
	
	on = (8 * 1024) * (2);   /* 发送缓冲区大小为8K */
	ret  = recvUdpServer->setsockopt(recvUdpServer,SOL_SOCKET, SO_SNDBUF, &on, sizeof(on));
	CHECK_RET(ret < 0, "fail to setsockopt sendsocket fd", goto fail0);
	//设置接收缓冲区大小 
	on = (8 * 1024) * (2);	  /* 接收缓冲区大小为8K */
	ret = recvUdpServer->setsockopt(recvUdpServer,SOL_SOCKET, SO_RCVBUF, &on, sizeof(on));
	CHECK_RET(ret < 0, "fail to setsockopt sendsocket fd", goto fail0);
	on = 1;
	ret = recvUdpServer->setsockopt(recvUdpServer,SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	CHECK_RET(ret < 0, "fail to setsockopt sendsocket fd", goto fail0);
	
	ret = recvUdpServer->setHandle(recvUdpServer,audioDataRecvThread,NULL,NULL);
	CHECK_RET(ret < 0, "fail to udp_addFdToRecvQueue!", goto fail0);

	return 0;
fail0:
	return -1;
	
}

static int audioDataRecvThread(unsigned char* buf,unsigned int len)
{
	if(talkAudioState == AUDIO_FREE){
		return len;
	}
	return talkWritePcm(buf,len);
}


int talkAudioExit(void)
{
	pthread_mutex_lock(&sendUdpServer_mutex);
	talkAudioStop();
	destroyUdpServer(&recvUdpServer);
	destroyUdpServer(&sendUdpServer);
	pthread_mutex_unlock(&sendUdpServer_mutex);

	return 0;
}





































#include "udpServer.h"
#include "DebugLog.h"
#include "localNetInfo.h"
#include "commNetConfig.h"
#include "hiAudio.h"
#include "uscam_audio.h"
#include "commonHead.h"
#include <arpa/inet.h>


static int sendSocktFd;
static int recvSocktFd;
static int destIpAddr;

static  enum {
	AUDIO_FREE = 0,
	AUDIO_TALKING,
}talkAudioState;

static int audioDataRecvThread(pS_NetDataPackage arg);
static int initSendSocket(void);
static int initRecvSocket(void);
static int sendTalkAudio(const struct timeval *tv,void *buf,uint32_t size);



int  talkAudioInit(void)
{
	int ret;
	ret = udp_serverInit();
	CHECK_RET(ret != 0, "fail to udp_serverInit!", goto fail0);
	ret  = initSendSocket();
	CHECK_RET(ret != 0, "fail to initSendSocket!", goto fail0);
	ret  = initRecvSocket();
	CHECK_RET(ret != 0, "fail to initRecvSocket!", goto fail0);

	talkAudioState = AUDIO_FREE;
	UsCamSysInit();
	UsCamAudioAttr attr;
	attr.cbm = sendTalkAudio;
	attr.sampleRate = 8000;
	attr.vqemode = 0;
	//UsSCamAudioDupluxOpen(, getConfigVol(TALKVOL));
	ret  = UsCamAudioOpen(&attr);			//�ص�����
	CHECK_RET(ret != 0, "fail to UsCamAudioOpen!", goto fail0);
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
	ret  = udp_startFdRecv(recvSocktFd);
	CHECK_RET(ret < 0, "fail to udp_startFdRecv!", goto fail0);
	talkAudioState = AUDIO_TALKING;
	return 0;
fail0:
	return -1;
}

int talkAudioStop(void)
{
	int ret ;
	if(talkAudioState == AUDIO_FREE)
		return -1;
	ret  = UsCamAudioStop();
	CHECK_RET(ret < 0, "fail to UsCamAudioStop!", goto fail0);
	udp_stopFdRecv(recvSocktFd);
	talkAudioState = AUDIO_FREE;

fail0:
	return -1;
}
static int sendTalkAudio(const struct timeval *tv,void *buf,uint32_t size)
{
	//发送前先设置远程主机IP地址及端口
	struct sockaddr_in remote_addr;
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(AUDIO_PORT);
	remote_addr.sin_addr.s_addr = destIpAddr;
	bzero(&(remote_addr.sin_zero),sizeof(remote_addr.sin_zero));
	return sendto( sendSocktFd, buf, size, 0,
			  (struct sockaddr*)&remote_addr, (socklen_t)sizeof(struct sockaddr));
	
	
}
static int initSendSocket(void)
{
	int on,ret;
	struct in_addr  local_addr;
	sendSocktFd = udp_getSocketFd(0);
	CHECK_RET(sendSocktFd < 0, "fail to udp_getSocketFd", goto fail0);
	on = (8 * 1024) * (2);   /* 发送缓冲区大小为8K */
	ret = setsockopt(sendSocktFd, SOL_SOCKET, SO_SNDBUF, &on, sizeof(on));
	CHECK_RET(ret < 0, "fail to setsockopt sendsocket fd", goto fail0);
	//设置接收缓冲区大小 
	on = (8 * 1024) * (2);	  /* 接收缓冲区大小为8K */
	ret = setsockopt(sendSocktFd, SOL_SOCKET, SO_RCVBUF, &on, sizeof(on));
	CHECK_RET(ret < 0, "fail to setsockopt sendsocket fd", goto fail0);
	on = 1;
	ret = setsockopt(sendSocktFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	CHECK_RET(ret < 0, "fail to setsockopt sendsocket fd", goto fail0);
	on = 1;
	ret = setsockopt(sendSocktFd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
	CHECK_RET(ret < 0, "fail to setsockopt", goto fail0);
	ret = setsockopt(sendSocktFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	CHECK_RET(ret < 0, "fail to setsockopt", goto fail0);
	
	local_addr.s_addr = getLocalIpaddr();
	ret  = setsockopt(sendSocktFd, SOL_IP, IP_MULTICAST_IF, &local_addr, sizeof(struct in_addr));
	CHECK_RET(ret < 0, "fail to setsockopt", goto fail0);
	on = 0;
	ret = setsockopt(sendSocktFd, SOL_IP, IP_MULTICAST_LOOP, &on, sizeof(on));
	CHECK_RET(ret < 0, "fail to setsockopt", goto fail0);
	
	return 0;
fail0:
	return -1;
}
static  int initRecvSocket(void)
{
	int on,ret;
	recvSocktFd = udp_getSocketFd(AUDIO_PORT);
	
	CHECK_RET(recvSocktFd < 0, "fail to udp_getSocketFd", goto fail0);
	
	on = (8 * 1024) * (2);   /* 发送缓冲区大小为8K */
	ret = setsockopt(recvSocktFd, SOL_SOCKET, SO_SNDBUF, &on, sizeof(on));
	CHECK_RET(ret < 0, "fail to setsockopt sendsocket fd", goto fail0);
	//设置接收缓冲区大小 
	on = (8 * 1024) * (2);	  /* 接收缓冲区大小为8K */
	ret = setsockopt(recvSocktFd, SOL_SOCKET, SO_RCVBUF, &on, sizeof(on));
	CHECK_RET(ret < 0, "fail to setsockopt sendsocket fd", goto fail0);
	on = 1;
	ret = setsockopt(recvSocktFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	CHECK_RET(ret < 0, "fail to setsockopt sendsocket fd", goto fail0);
	
	ret  = udp_addFdToRecvQueue(recvSocktFd,audioDataRecvThread);
	CHECK_RET(ret < 0, "fail to udp_addFdToRecvQueue!", goto fail0);

	
	return 0;
fail0:
	return -1;
	
}


static  int audioDataRecvThread(pS_NetDataPackage arg)
{
	MsgBody audioData = arg->dataBody;
	return talkWritePcm(audioData.buf,audioData.len);

}
int talkAudioExit(void)
{
	talkAudioStop();
	udp_deleteFdToRecvQueue(recvSocktFd);
	close(recvSocktFd);
	close(sendSocktFd);
	return 0;
}





































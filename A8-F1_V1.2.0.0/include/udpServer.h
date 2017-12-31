#ifndef  __UDP_SERVER__
#define  __UDP_SERVER__
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/select.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>

#include "threadManage.h"


#define MULTICAST_GROUP	0xef000000	//239.0.0.0


#define FuncMaxCount  10
#define UDP_BUF_MAXSIZE  1024*4
typedef struct S_NetDataPackage S_NetDataPackage,*pS_NetDataPackage;
typedef int (*FUNCTION)(pS_NetDataPackage );

typedef struct {
	unsigned char buf[UDP_BUF_MAXSIZE];
	int len;
	int offset;
} MsgBody, *pMsgBody;

struct S_NetDataPackage{
	int sockfd;
	struct sockaddr_in remoteInfo;
	MsgBody dataBody;
};
typedef enum{
	FD_HANG_UP = 0,
	FD_RUN,
	FD_EXIT,
}E_FdState;
typedef struct S_HandleFd{
	S_NetDataPackage netDataPack;
	FUNCTION handleFunc;
	E_FdState fdState;
}S_HandleFd,*pS_HandleFd;
typedef struct S_UdpRecv{
	int maxfdp;
	fd_set fds;
	struct timeval tv;
	int fdNum;
	Thread_ID thPid ;
	S_HandleFd handleFd[FuncMaxCount];
}S_UdpRecv,*pS_UdpRecv;


int 	udp_serverInit(void);
void 	udp_serverExit(void);
int  	udp_getSocketFd(int bandPort);
int  	udp_sendData(int udp_sockfd, const unsigned char * data, int size,
		uint32_t ipAddr, int port);
int  	udp_addFdToRecvQueue(int fd,FUNCTION callBackfun);
int 	udp_startFdRecv(int sockfd);
int 	udp_stopFdRecv(int sockfd);
int  	udp_deleteFdToRecvQueue(int fd);

int 	udp_getTid(void);











#endif













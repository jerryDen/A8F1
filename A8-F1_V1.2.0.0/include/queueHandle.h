#ifndef  QUEUE_HANDLE__H__
#define  QUEUE_HANDLE__H__
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <errno.h>
#include <assert.h>
#include <sys/inotify.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include "queueBufLib.h"


typedef struct S_QueueHandlePack{
	pQueuePack queueBuf;
	pthread_mutex_t queueBufMutex;
	int mEpollFd;
    int mWakeReadPipeFd;
    int mWakeWritePipeFd;
}QueueHandlePack,*pQueueHandlePack;
//初始化队列
pQueueHandlePack queueHandleInit(int dataTypeSize);
//将数据push到队列中,并唤醒阻塞
int  pushDataToQueueHandle(pQueueHandlePack qHandlePack ,
	const void *data,unsigned int dataNumber);
//从队列中提取数据,此函数会阻塞
int  pullDataFromQueueHandle(pQueueHandlePack qHandlePack ,
	 void *data,unsigned int dataLen);
//退出
void queueHandleExit(pQueueHandlePack * pack);




#endif














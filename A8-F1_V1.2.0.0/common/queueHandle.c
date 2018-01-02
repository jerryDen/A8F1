#include "queueBufLib.h"
#include "DebugLog.h"
#include "queueHandle.h"
#include "commonHead.h"

#define QH_TAG "QUEUE_HANDLE"
#define EPOLL_MAX_EVENTS 1
static int  wakeQueueWait(int mWakeWritePipeFd,int ch);
static int queueWaitToBeAwakened(	int mEpollFd,int mWakeReadPipeFd);

pQueueHandlePack queueHandleInit(int dataTypeSize)
{
	int result;
	int wakeFds[2];
	struct epoll_event eventItem;
	memset(&eventItem, 0, sizeof(struct epoll_event));
	pQueueHandlePack qHandlePack = (pQueueHandlePack)malloc(sizeof(QueueHandlePack)); 
	CHECK_RET(qHandlePack == NULL,"fail to queue_init!", goto fail1);
	pQueuePack qPack= queue_init(dataTypeSize);
	CHECK_RET(qPack == NULL,"fail to queue_init!", goto fail1);
	qHandlePack->queueBuf = qPack;
    result = pipe(wakeFds);
	CHECK_RET(result != 0,"fail to queue_init!", goto fail1);
	qHandlePack->mWakeReadPipeFd = 	wakeFds[0];
    qHandlePack->mWakeWritePipeFd = wakeFds[1];


	result = fcntl(qHandlePack->mWakeReadPipeFd, F_SETFL, O_NONBLOCK);
    CHECK_RET(result != 0, "Could not make wake read pipe non-blocking",
            goto fail1);

    result = fcntl( qHandlePack->mWakeWritePipeFd, F_SETFL, O_NONBLOCK);
    CHECK_RET(result != 0, "Could not make wake write pipe non-blocking",
            goto fail1);

	qHandlePack->mEpollFd = epoll_create(EPOLL_MAX_EVENTS);
	CHECK_RET(qHandlePack->mEpollFd<0, "epoll_create", goto fail1);


	eventItem.data.fd = qHandlePack->mWakeReadPipeFd ;  
 	eventItem.events = EPOLLIN ;
	result = epoll_ctl(qHandlePack->mEpollFd, EPOLL_CTL_ADD, qHandlePack->mWakeReadPipeFd, &eventItem);
	CHECK_RET(result<0, "epoll_ctl", goto fail1);
	
	pthread_mutex_init(&(qHandlePack->queueBufMutex),NULL);
	return qHandlePack;
fail1:
	free(qPack);
fail0:
	return NULL;
}


int pushDataToQueueHandle(pQueueHandlePack qHandlePack , const void *data,unsigned int dataNumber)
{
	int result;

	CHECK_RET(!(qHandlePack&&qHandlePack->queueBuf), "qHandlePack or qHandlePack->queueBuf is null", goto fail0);
		
	pthread_mutex_lock(&qHandlePack->queueBufMutex);
	result = queue_push(qHandlePack->queueBuf,data,dataNumber);
	pthread_mutex_unlock(&qHandlePack->queueBufMutex);
	
	CHECK_RET(result<0, "fail to queue_push", goto fail0);
	result = wakeQueueWait(qHandlePack->mWakeWritePipeFd,IS_DATA);
	CHECK_RET(result<0, "fail to  wakeQueueWait", goto fail0);

	return 0;
	
fail0:
	return -1;
}
int pullDataFromQueueHandle(pQueueHandlePack qHandlePack ,  void *data,unsigned int dataLen)
{
	pQueuNode qNode  = NULL;
	while(queue_isEmpty(qHandlePack->queueBuf)){
	
		if( IS_EXIT == queueWaitToBeAwakened(qHandlePack->mEpollFd,qHandlePack->mWakeReadPipeFd)){

			return IS_EXIT;
		} 
	}
	pthread_mutex_lock(&qHandlePack->queueBufMutex);
	qNode = queue_peek(qHandlePack->queueBuf);
	pthread_mutex_unlock(&qHandlePack->queueBufMutex);
	CHECK_RET(qNode == NULL, "fail to queue_peek", goto fail0);
	CHECK_RET(data == NULL||dataLen <= 0, "fail to data is null", goto fail0);
	memcpy(data,qNode->buf,(qNode->bufLen >dataLen) ?qNode->bufLen:dataLen);
	pthread_mutex_lock(&qHandlePack->queueBufMutex);
	queue_deleteTail(qHandlePack->queueBuf);
	pthread_mutex_unlock(&qHandlePack->queueBufMutex);
	
	return 0;
 fail0:
 	return -1;
}

static int  queueWaitToBeAwakened(	int mEpollFd,int mWakeReadPipeFd)
{
	struct epoll_event eventItem;
	int pollResult = epoll_wait(mEpollFd, &eventItem, EPOLL_MAX_EVENTS, -1);
	int buffer[16];
    ssize_t nRead;
	
    do {
        nRead = read(mWakeReadPipeFd, &buffer, sizeof(buffer));
		//防止缓存中存有大量的数据,所以这里要保证一次性读完,以免下次不阻塞
    } while ((nRead == -1 && errno == EINTR)||nRead == sizeof(buffer) );
	return buffer[0];	
}
static int  wakeQueueWait(int mWakeWritePipeFd,int       ch)
{	
	ssize_t nWrite;
    do {
        nWrite = write(mWakeWritePipeFd, &ch, sizeof(ch));
    } while (nWrite == -1 && errno == EINTR);

    if (nWrite != sizeof(ch) && errno != EAGAIN) {
        LOGE("Could not write wake signal, errno=%d", errno);
		return -1;
    }
	
	return 0;
}
void queueHandleExit(pQueueHandlePack * pack)
{
	if(pack	== NULL&&(*pack)->queueBuf == NULL)
		return ;
	wakeQueueWait((*pack)->mWakeWritePipeFd,IS_EXIT);
	pthread_mutex_lock(&(*pack)->queueBufMutex);
		
	queue_destroy(&((*pack)->queueBuf));
	
	
	pthread_mutex_unlock(  &(*pack)->queueBufMutex );
	free((*pack));
	*pack = NULL;
}




































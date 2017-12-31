
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "mThread.h"
#include "DebugLog.h"
#define THREAD_TAG  "PTHREAD"
static int findEmptyIndex(void);
static int get_pthread_state(ThreadpoolTid tid);
static  void clean_pthreadpool_member(ThreadpoolTid  pthId );

static S_PthPool PthPool;
ThreadpoolTid pthread_register(pthread_attr_t  *attr, THREAD_FUNC     func,  void  *arg)
{
	
	int index = findEmptyIndex();
	if(index < 0)
	{
		LOGE(THREAD_TAG," PthPool is full!\n");
		return NULL;
	}
	if(attr != NULL)
			memcpy(PthPool.PthS[index].attr,attr,sizeof(pthread_attr_t));
	
	PthPool.PthS[index].func = func;
	PthPool.PthS[index].arg = arg;	
	PthPool.PthS[index].state = ThreadWAIT;
	pthread_rwlock_init(&(PthPool.PthS[index].inside_rw_lock),NULL);
	pthread_cond_init(&(PthPool.PthS[index].cond),NULL);
	pthread_mutex_init(&(PthPool.PthS[index].mutex),NULL);
	pthread_rwlock_init(&(PthPool.PthS[index].external_rw_lock),NULL);
	PthPool.count ++;
	int * retId  = malloc(sizeof(int));
	if(retId == NULL)
	{
		LOGE(THREAD_TAG,"retId MALLOC FAIL!");
		return NULL;
	}	
	*retId = index;
	return retId;
}


int  pthread_getRwlock(pthread_rwlock_t** rwlock,ThreadpoolTid pTid)
{
	int index = *pTid;
	if(index<0 ||index >=PthPool.count )
		return -1;
	*rwlock =&PthPool.PthS[index].external_rw_lock;
	return 0;
}
int  pthread_mutex_up(ThreadpoolTid pthId)
{
	int index = (pthId==NULL)?-1:*pthId;
	if(index<0 ||index >=PthPool.count )
		return -1;	
	pthread_mutex_lock(&(PthPool.PthS[index].mutex));
	return 0;
}
int pthread_cond_awaken(ThreadpoolTid pthId){
	int index = (pthId==NULL)?-1:*pthId;
	if(index < 0)
		return -1;
	if( 0 == pthread_mutex_up(pthId)){
		pthread_cond_signal(&(PthPool.PthS[index].cond));
		pthread_mutex_un(pthId);
		return 0;
	}

	return -1;	
}
int pthread_cond_sleep(ThreadpoolTid pthId){
	int index = (pthId==NULL)?-1:*pthId;
	if(index < 0)
		return -1;
	if( 0 == pthread_mutex_up(pthId)){
		pthread_mutex_t  *mutex;
		pthread_getMutex(&mutex,pthId);
		pthread_cond_wait(&(PthPool.PthS[index].cond),mutex);
		pthread_mutex_un(pthId);
		return 0;
	}
	return -1;	
}

int  pthread_mutex_un(ThreadpoolTid pthId)
{
	int index = (pthId==NULL)?-1:*pthId;
	if(index < 0)
		return -1;
	if(index<0 ||index >=PthPool.count )
		return -1;
	pthread_mutex_unlock(&(PthPool.PthS[index].mutex));
	return 0;
}
int  pthread_rwlock_up(ThreadpoolTid pthId)
{
	int index = (pthId==NULL)?-1:*pthId;
	
	if(index<0 ||index >=PthPool.count )
		return -1;
	pthread_rwlock_wrlock(&(PthPool.PthS[index].external_rw_lock));
	return 0;
}
int  pthread_rwlock_un(ThreadpoolTid pthId)
{
	int index = (pthId==NULL)?-1:*pthId;
	
	if(index<0 ||index >=PthPool.count )
		return -1;
	pthread_rwlock_unlock(&(PthPool.PthS[index].inside_rw_lock));
	return 0;
}


int  pthread_getMutex(pthread_mutex_t  **mutex,ThreadpoolTid pthId)
{
	int index = (pthId==NULL)?-1:*pthId;
	
	if(index<0 ||index >=PthPool.count )
		return -1;
	
	if(index<0 ||index >=PthPool.count )
		return -1;
	*mutex = &PthPool.PthS[index].mutex;
			
	return 0;
}


int  pthread_getCond(pthread_cond_t ** cond,ThreadpoolTid pthId)
{
	int index = (pthId==NULL)?-1:*pthId;
	
	if(index<0 ||index >=PthPool.count )
		return -1;
	*cond = &PthPool.PthS[index].cond;
	return 0;
}
int pthread_start(ThreadpoolTid pthId)
{

	return pthread_start_bandArg(pthId,NULL);
}
int pthread_start_bandArg(ThreadpoolTid pthId,void *arg)
{
	pthread_t  thread;
	int index = (pthId==NULL)?-1:*pthId;
	if(index<0 ||index >=PthPool.count )
		return -1;
	printf("PthPool.PthS[%d].state  %d\n",index,PthPool.PthS[index].state);
	pthread_rwlock_wrlock(&(PthPool.PthS[index].inside_rw_lock));
	if(PthPool.PthS[index].state == ThreadPAUSE )
	{
		PthPool.PthS[index].arg = arg;
		PthPool.PthS[index].state = ThreadSTART;
		pthread_rwlock_unlock(&(PthPool.PthS[index].inside_rw_lock));
		return 0;
	}else if(PthPool.PthS[index].state == ThreadSTART ){  //防止重复start
		pthread_rwlock_unlock(&(PthPool.PthS[index].inside_rw_lock));
		return 0;
	}
	PthPool.PthS[index].state = ThreadSTART;
	PthPool.PthS[index].arg = arg;
	pthread_rwlock_unlock(&(PthPool.PthS[index].inside_rw_lock));
	
	if (pthread_create(&thread, PthPool.PthS[index].attr, PthPool.PthS[index].func,
			PthPool.PthS[index].arg) != 0) {
		return -1;
	}
	pthread_detach(thread);
	PthPool.PthS[index].thread =  thread;
	return 0;

}
int pthread_stop(ThreadpoolTid pthId)
{
	int cnt = 100;
	ThreadState state = get_pthread_state(pthId);
	if( pthread_setState(pthId,ThreadEXIT) == 0)
	{
			while(cnt -- >0){
				if( get_pthread_state(pthId) == ThreadWAIT ||get_pthread_state(pthId) == -1 ){
				
					return 0;
				}
				usleep(1000);
			}
			return -1;
			
	}
	pthread_setState(pthId,state);
	printf("set fail0!\n");
	return -1;
}

int pthread_setState(ThreadpoolTid pthId, ThreadState state)
{
	int index = (pthId==NULL)?-1:*pthId;
	if(index<0 ||index >=PthPool.count )
		return -1;
	pthread_rwlock_wrlock(&(PthPool.PthS[index].inside_rw_lock)); 
	PthPool.PthS[index].state = state;
	pthread_rwlock_unlock(&(PthPool.PthS[index].inside_rw_lock));
	return 0;	
}

static int findEmptyIndex(void)
{
	int i;
	for(i = 0; i < Thread_MAX ; i++)
	{			
		if(PthPool.PthS[i].func == NULL )
		{
			return i;
		}
	}	
	return -1;	
}
int pthread_destroy(ThreadpoolTid* tid)
{
	int cnt = 300;
	pthread_setState(*tid,ThreadEXIT);
	while(cnt-->0)
	{
		if( get_pthread_state(*tid) == ThreadWAIT ||get_pthread_state(*tid) == -1 ){
			
			clean_pthreadpool_member(*tid);
			free(*tid);
			*tid = NULL;
			return 0;
		}
		usleep(10);
	}
	return -1;
}
static  void clean_pthreadpool_member(ThreadpoolTid  pthId )
{

	int index = (pthId==NULL)?-1:*pthId;
	if(index<0 ||index >=PthPool.count )
		return ;
	
	if(PthPool.PthS[index].arg != NULL)
		free(PthPool.PthS[index].arg);
	if(PthPool.PthS[index].attr!= NULL)
		free(PthPool.PthS[index].attr);
	pthread_mutex_up(pthId);
	pthread_cond_destroy(&(PthPool.PthS[index].cond));
	pthread_mutex_un(pthId);
	pthread_mutex_destroy(&(PthPool.PthS[index].mutex));
	pthread_rwlock_destroy(&(PthPool.PthS[index].inside_rw_lock));
	pthread_rwlock_destroy(&(PthPool.PthS[index].external_rw_lock));
	
	
	memset(&PthPool.PthS[index],0,sizeof(S_PthMan));
}
static  int get_pthread_state(ThreadpoolTid pthId)
{
	int index = (pthId==NULL)?-1:*pthId;
	if(index<0 ||index >=PthPool.count )
		return -1;
	
	int ret = -1;
	pthread_rwlock_rdlock(&(PthPool.PthS[index].inside_rw_lock)); 
	if(PthPool.PthS[index].func != NULL)
		ret = PthPool.PthS[index].state;
	pthread_rwlock_unlock(&(PthPool.PthS[index].inside_rw_lock));
	return ret;
}
int pthread_checkState(ThreadpoolTid tid)
{
	if(tid == NULL)
		return -1;
	int index = *tid;
	
	if(index<0 ||index >=PthPool.count )
		return -1;
	pthread_rwlock_rdlock(&(PthPool.PthS[index].inside_rw_lock));   
	if(PthPool.PthS[index].state== ThreadSTART){
		pthread_rwlock_unlock(&(PthPool.PthS[index].inside_rw_lock));
		return 1;
	}
	else if(PthPool.PthS[index].state == ThreadEXIT){
		pthread_rwlock_unlock(&(PthPool.PthS[index].inside_rw_lock));
		return 0;
	}
	else if(PthPool.PthS[index].state == ThreadPAUSE )
	{	
		pthread_rwlock_unlock(&(PthPool.PthS[index].inside_rw_lock));
		pthread_rwlock_rdlock(&(PthPool.PthS[index].inside_rw_lock));
		while(PthPool.PthS[index].state == ThreadPAUSE)
		{
			
			pthread_rwlock_unlock(&(PthPool.PthS[index].inside_rw_lock));
			usleep(1000*100);
			pthread_rwlock_rdlock(&(PthPool.PthS[index].inside_rw_lock));
		}
	}
	pthread_rwlock_unlock(&(PthPool.PthS[index].inside_rw_lock));
	return 1;
}

































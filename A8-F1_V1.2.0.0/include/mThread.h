#ifndef  __MTHREAD_H_
#define  __MTHREAD_H_
#include <pthread.h>
typedef  int * ThreadpoolTid;
#define SYNC_RLOCK(lock,res) do     \
{								    \
	pthread_rwlock_rdlock(&lock);   \
	res;							\
	pthread_rwlock_unlock(&lock);	\
}while(0)
#define SYNC_WLOCK(lock,res) do     \
{								    \
	pthread_rwlock_wrlock(&lock);   \
	res;							\
	pthread_rwlock_unlock(&lock);	\
}while(0)
	
typedef void *(*THREAD_FUNC) (void*);

typedef enum ThreadState{
	ThreadWAIT = 0,
	ThreadPAUSE,
	ThreadSTART,
	ThreadEXIT,
}ThreadState; 

#define Thread_MAX  10

typedef  struct S_ThreadManage{
	pthread_t  thread;
	THREAD_FUNC func;
	void * arg;
	pthread_attr_t * attr;
	ThreadState state;
	pthread_cond_t cond;
	pthread_mutex_t  mutex;
	pthread_rwlock_t inside_rw_lock;
	pthread_rwlock_t external_rw_lock;
}S_PthMan,*pS_PthMan;
typedef  struct S_ThreadPool{
	S_PthMan PthS[Thread_MAX];
	int count;
}S_PthPool,*pS_PthPool;


ThreadpoolTid pthread_register(pthread_attr_t  *attr, THREAD_FUNC   func,  void  *arg);
int pthread_start(ThreadpoolTid id);
int pthread_start_bandArg(ThreadpoolTid pthId,void *arg);

int pthread_setState(ThreadpoolTid id, ThreadState state);
int pthread_checkState(ThreadpoolTid pthId);
int pthread_getRwlock(pthread_rwlock_t** rwlock,ThreadpoolTid pthId);
int pthread_getCond(pthread_cond_t ** cond,ThreadpoolTid pthId);
int pthread_getMutex(pthread_mutex_t ** mutex_t,ThreadpoolTid pthId);
int pthread_mutex_up(ThreadpoolTid pthId);
int pthread_mutex_un(ThreadpoolTid pthId);
int pthread_rwlock_up(ThreadpoolTid pthId);
int pthread_rwlock_un(ThreadpoolTid pthId);
int pthread_cond_awaken(ThreadpoolTid pthId);
int pthread_cond_sleep(ThreadpoolTid pthId);
int pthread_stop(ThreadpoolTid pthId);

int pthread_destroy(ThreadpoolTid* tid);



















#endif













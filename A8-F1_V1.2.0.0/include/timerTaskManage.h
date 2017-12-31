#ifndef  __TIMER_TASK_H__
#define  __TIMER_TASK_H__
#include  <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h> 
#include <sys/time.h>
#include <string.h> 
#include <pthread.h> 

typedef void (*pThreadFunc)(void *arg );
typedef struct TimerOps{
	int (*start)(struct TimerOps*);
	int (*changeParameter)(struct TimerOps*,int,int,int);
	int (*reset)(struct TimerOps*);
	int (*stop)(struct TimerOps*);
}TimerOps,*pTimerOps;
pTimerOps createTimerTaskServer( int startTime, int loopTime,int loopCount,pThreadFunc taskFunc,void *arg,int dataLen);
void destroyTimerTaskServer(pTimerOps *timerServer );





#endif

#ifndef  __QUEUE_BUF_LIB__
#define  __QUEUE_BUF_LIB__

#include<stdio.h>
#include<stdlib.h>
#include <string.h>
/*
功能：队列包中节点的最大个数
*/
#define QUEUE_NODE_MAX_NUM  10   

/*
节点类型
*/
typedef struct QueuNode{
	int bufLen;
	void *buf;
}QueuNode,*pQueuNode;
/*
包类型：一个包含有N个节点
*/
typedef struct QueuePack{
	pQueuNode node[QUEUE_NODE_MAX_NUM];
	int nodeNum;
	int typeSize;	
}QueuePack,*pQueuePack;
/*
功能：初始化函数 
参数：
	typeSize：数据类型所占内存空间的尺寸
返回值：队列包指针

*/
pQueuePack  queue_init(int typeSize);
/*
功能：判断队列是否为空
形参：
	qPack：队列包指针
返回值：0：表示非空  非0：表示空

*/

int queue_isEmpty(pQueuePack qPack);
/*
功能：判断队列是否满
形参：
	qPack：队列包指针
返回值：0：表示非满  非0：表示满

*/
int queue_isFull(pQueuePack qPack);
/*
功能：删除队列尾部
形参：
	qPack：队列包指针
返回值：void
*/

void queue_deleteTail(pQueuePack qPack);
/*
功能：提取一个节点的数据
形参：
	qPack：队列包指针
返回值：返回一个节点的指针，（使用完需释放）
*/ 

pQueuNode queue_peek(pQueuePack qPack);
/*
功能：添加一个节点的数据（数据地址+数据长度）
形参：
	qPack：队列包指针
	buf：数据首地址
	bufLen：数据长度
返回值：0：添加成功  非0：添加失败
*/

int  queue_push(pQueuePack qPack,const void *buf,int bufLen);
/*
功能：获取队列中节点的个数
参数：
	qPack：队列包指针
返回值：> 0:个数  <0:获取失败
*/

int queue_getNodeNum(pQueuePack qPack);
/*
功能：销毁整个队列
参数：
	qPack：队列包指针的地址
返回值：void
*/
void queue_destroy(pQueuePack * qPack);

#endif
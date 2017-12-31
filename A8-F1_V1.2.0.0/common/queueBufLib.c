#include "queueBufLib.h"
/*
 *  Created on: 11 14, 2016
 *      Author: silent
 */
/*��ʼ��*/
pQueuePack  queue_init(int typeSize)
{
	int i;
	pQueuePack qPack = (pQueuePack)malloc(sizeof(QueuePack)); 
	
	for(i = 0; i<QUEUE_NODE_MAX_NUM;i++)
	{
		qPack->node[i] = NULL;
	}
	
	qPack->nodeNum = 0;
	qPack->typeSize = typeSize;
	return 	qPack;
}

int queue_isEmpty(pQueuePack qPack)
{
	return qPack->nodeNum == 0;
}
int queue_isFull(pQueuePack qPack)
{
	return qPack->nodeNum == QUEUE_NODE_MAX_NUM;
}
/**/
void queue_deleteTail(pQueuePack qPack)
{
	int i = 0;

	
	if(qPack !=NULL&& qPack->nodeNum > 0)
	{
		if(qPack->node[qPack->nodeNum-1 ] !=NULL)
		{
			if(qPack->node[qPack->nodeNum -1]->buf != NULL)
			{
				free(qPack->node[qPack->nodeNum -1]->buf);
				qPack->node[qPack->nodeNum-1 ]->buf = NULL;
				qPack->node[qPack->nodeNum-1 ]->bufLen = 0;
			}
			free(qPack->node[qPack->nodeNum-1]);
			qPack->node[qPack->nodeNum-1]  = NULL;
			qPack->nodeNum --;
		} 
	}

}
pQueuNode queue_peek(pQueuePack qPack)
{

	if(qPack !=NULL&& qPack->nodeNum > 0 && qPack->nodeNum<=QUEUE_NODE_MAX_NUM)
	{
		if(qPack->node[qPack->nodeNum-1] != NULL ){
			return qPack->node[qPack->nodeNum-1];
		}
		else{
			printf("qPack->node[%d] == NULL\n",qPack->nodeNum-1);
		}
	}
	return NULL;
}
static int arrayCopy(void *dest,void *src,int size)
{
	
	
}
int  queue_push(pQueuePack qPack,const void *buf,int bufLen)
{
	if(qPack == NULL)
		goto fail1;
	if(qPack->nodeNum < QUEUE_NODE_MAX_NUM){
	
		if(qPack->nodeNum > 0){
			//右移一位
			//void *src =  &(qPack->node[0]);
			void *dest = &(qPack->node[1]);
			void *src = malloc(qPack->nodeNum*sizeof(pQueuNode));
			if(src == NULL ) goto fail1;
			memcpy(src,&(qPack->node[0]),qPack->nodeNum*sizeof(pQueuNode));
			memcpy(dest,src,qPack->nodeNum*sizeof(pQueuNode));
			free(src);
		}
		//分配包头
		qPack->node[0] = (pQueuNode)malloc(sizeof(QueuNode));
		if(qPack->node[0] == NULL )
		{
			goto fail1;
		}
		qPack->node[0]->buf = malloc(bufLen*qPack->typeSize);
		if(qPack->node[0]->buf == NULL )
		{
			goto fail2;
		}
		qPack->node[0]->bufLen = bufLen;
		memcpy(qPack->node[0]->buf,buf,bufLen*qPack->typeSize);
		qPack->nodeNum++;
		return 0;
	}
	return -1;
fail2:
	free(qPack->node[0]);
	memcpy(&qPack->node[0],&qPack->node[1],qPack->nodeNum*sizeof(pQueuNode));	
fail1:
	return -1;
	
}
int queue_getNodeNum(pQueuePack qPack)
{
	if(qPack !=NULL)
		return  qPack->nodeNum;
}
void queue_destroy(pQueuePack * qPack)
{
	int i;
	if(qPack == NULL)
	{
		return ;
	}
	for(i=(*qPack)->nodeNum;i > 0;i--)
	{
		queue_deleteTail(*qPack);
	}
	free(*qPack);
	*qPack = NULL;
	
}

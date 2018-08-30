
#include <stdlib.h>
#include <string.h>
#include "listStorage.h"

pS_ListHead list_creat(unsigned int type)
{
	pS_ListHead list_head = NULL;	
	list_head = malloc(sizeof(S_ListHead));
	if(list_head == NULL)
		 goto err1;
	list_head->head = malloc(sizeof(S_ListNode));
	if(list_head->head  == NULL)
	     goto err2;
	list_head->type = type;
	list_head->node_num = 0;
	list_head->head->next = list_head->head;
	list_head->head->prev = list_head->head;	
	return list_head;
    err2:
	    free(list_head);
		list_head = NULL;
	err1:
		return NULL;
	
}

int list_add_data(pS_ListHead list_head,void *data)
{
		if(list_head == NULL)
		{
			goto err1; 
		}
		pS_ListNode  new_node = (pS_ListNode)malloc(sizeof(S_ListNode));
		if(new_node == NULL)
				goto err1; 
		new_node->data = malloc(list_head->type);
		if(new_node->data == NULL)
		{
			goto err2;
		}	
		memcpy(new_node->data,data,list_head->type);
		pS_ListNode tail = list_head->head;	
		#if 0	//method 1		
		for(tail ;tail->next!=list_head->head; tail = tail->next)
			;
		tail->next = new_node;			
		new_node->prev = tail;
		new_node->next = list_head->head;	
		#else //method 2
			
		tail->prev->next = new_node;
		new_node->next = tail;
		new_node->prev = tail->prev;
		tail->prev = new_node;
		#endif
				
		list_head->node_num++;
		return 0;
				
	err2:
			free(new_node);
	err1:
		return -1;		
}
int list_get_data(pS_ListHead list_head,GETDATA_FUNC handFunc)
{
	if(list_head == NULL||handFunc == NULL)
	{
		return -1;
	}
	pS_ListNode tail = NULL;

	for(tail = list_head->head;tail->next != list_head->head; tail = tail->next)
	{
	  
		handFunc(tail->next->data);
	}	
	return 0;
}

int list_get_index_data(pS_ListHead list_head,int index,void *data)
{
		int temp_index = 0;
		if(index <0 ||index >= list_head->node_num||list_head == NULL)
			goto err1;
			pS_ListNode tail = list_head->head;	
		for(tail;tail->next != list_head->head&& temp_index  != index;tail = tail->next)  
			temp_index++;
		
		memcpy(data,tail->next->data,list_head->type);
		return 0;
		err1:
		return -1;
}
void* list_find_data_node(pS_ListHead list_head,void* data,CMP_FUNC compareFunc)
{
		
		pS_ListNode tail = list_head->head;	
		for(tail;tail->next != list_head->head;tail = tail->next)  
		{
			if(compareFunc(data, tail->next->data) == 0 )
			{
				return tail->next->data;
			}
		}	
		return NULL;	
}
int list_detele_data_node(pS_ListHead list_head,void* data,CMP_FUNC compareFunc)
{
		
		pS_ListNode tail = list_head->head;	
		pS_ListNode pendingNode = NULL;
		for(tail;tail->next != list_head->head;tail = tail->next)  
		{
			if(compareFunc(tail->next->data,data) == 0 )
			{
				pendingNode = tail->next;
				tail->next = pendingNode->next;
				tail->next->prev = tail;		
				free(pendingNode); 
				list_head->node_num --;
				return 0;
				
			}
		}	
		return -1;	
}


int list_delete_index(pS_ListHead list_head,int index)
{
	int temp_index = 0;
	pS_ListNode pendingNode = NULL;
	if(index <0 ||index >= list_head->node_num)
		goto err1;
	pS_ListNode tail = list_head->head;	
	for(tail;tail->next != list_head->head&& temp_index  != index;tail = tail->next)  
			temp_index++;
	pendingNode = tail->next;
	tail->next = pendingNode->next;
	tail->next->prev = tail;		
	free(pendingNode); 
	list_head->node_num --;
	return 0;
	err1:
		return -1;	
}

int list_get_node_num(pS_ListHead list_head)
{
		if( list_head !=NULL)
			return list_head->node_num;
		return 0;
}


void list_destroy(pS_ListHead *list_head)
{
	pS_ListNode tail = (*list_head)->head;	
	pS_ListNode pendingNode  = NULL; 
	for(tail; tail->next != (*list_head)->head;) 
	{
		pendingNode = tail;
		tail = tail->next;	
		free(pendingNode);
		pendingNode = NULL;
	}	
	free(tail);
	tail = NULL;
	free(*list_head);
	*list_head = NULL;

}
















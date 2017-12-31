#ifndef 	_LIST_STORAGE__H
#define 	_LIST_STORAGE__H


/*
	功能：节点类型
	data：节点数据类型
	prev：指向上一节点
	next：指向下一节点 
*/
typedef struct S_ListNode{
	void *data;
	struct S_ListNode * prev;
	struct S_ListNode * next; 
}S_ListNode,*pS_ListNode;
/*
	功能：链表头类型
	head：首节点
	node_num：节点个数 
	type：节点数据类型所占用的内存空间（如sizeof(int)） 
*/
typedef struct S_ListHead{
	pS_ListNode head;
	unsigned int node_num;
	unsigned int type;
}S_ListHead,*pS_ListHead;

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef void* (*GETDATA_FUNC)(void*);
typedef int (*CMP_FUNC)(const void *, const void *);

/*
   	功能：创建链表
	形参：
		type:节点类型的长度(如存int 就传sizeof(int)) 
	返回值： 链表头指针 
*/
pS_ListHead list_creat(unsigned int type);
/*
   	功能：添加数据到链表 
	形参：
		list_head:链表头
		data:待添加数据的地址（类型长度必须是创建时传入的type值） 
	返回值： 0：成功  -1：失败 
*/
int list_add_data(pS_ListHead list_head,void *data);
/*
   	功能：获取数据 
	形参：
		list_head:链表头
		handFunc:获取数据函数回调函数 
	返回值： 0：成功  -1：失败 
*/
int list_get_data(pS_ListHead list_head,GETDATA_FUNC handFunc);
/*
   	功能：删除链表中的数据 
	形参：
		list_head:链表头
		handFunc:链表下标号(以0开始) 
	返回值： 0：成功  -1：失败 
*/

int list_delete_index(pS_ListHead list_head,int index);
/*
   	功能：用数据在链表中查询 
	形参：
		list_head:链表头
		data:查询数据
		compareFunc：查询回调函数 
	返回值：NULL：查询失败  || 查询到的节点地址 
*/
void* list_find_data_node(pS_ListHead list_head,void* data,CMP_FUNC compareFunc);
/*
   	功能：获取链表小标对应的数据 
	形参：
		list_head:链表头
		index:链表下标 
		data：获取的数据(需在调用函数前给data分配空间) 
	返回值： 0：成功  -1：失败
*/
int list_get_index_data(pS_ListHead list_head,int index,void *data);

int list_detele_data_node(pS_ListHead list_head,void* data,CMP_FUNC compareFunc);

/*
   	功能：销毁链表 
	形参：
		list_head:链表头
	
	返回值：void
*/

void list_destroy(pS_ListHead *list_head);







#endif 

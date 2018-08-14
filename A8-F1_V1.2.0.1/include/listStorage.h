#ifndef 	_LIST_STORAGE__H
#define 	_LIST_STORAGE__H


/*
	���ܣ��ڵ�����
	data���ڵ���������
	prev��ָ����һ�ڵ�
	next��ָ����һ�ڵ� 
*/
typedef struct S_ListNode{
	void *data;
	struct S_ListNode * prev;
	struct S_ListNode * next; 
}S_ListNode,*pS_ListNode;
/*
	���ܣ�����ͷ����
	head���׽ڵ�
	node_num���ڵ���� 
	type���ڵ�����������ռ�õ��ڴ�ռ䣨��sizeof(int)�� 
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
   	���ܣ���������
	�βΣ�
		type:�ڵ����͵ĳ���(���int �ʹ�sizeof(int)) 
	����ֵ�� ����ͷָ�� 
*/
pS_ListHead list_creat(unsigned int type);
/*
   	���ܣ�������ݵ����� 
	�βΣ�
		list_head:����ͷ
		data:��������ݵĵ�ַ�����ͳ��ȱ����Ǵ���ʱ�����typeֵ�� 
	����ֵ�� 0���ɹ�  -1��ʧ�� 
*/
int list_add_data(pS_ListHead list_head,void *data);
/*
   	���ܣ���ȡ���� 
	�βΣ�
		list_head:����ͷ
		handFunc:��ȡ���ݺ����ص����� 
	����ֵ�� 0���ɹ�  -1��ʧ�� 
*/
int list_get_data(pS_ListHead list_head,GETDATA_FUNC handFunc);
/*
   	���ܣ�ɾ�������е����� 
	�βΣ�
		list_head:����ͷ
		handFunc:�����±��(��0��ʼ) 
	����ֵ�� 0���ɹ�  -1��ʧ�� 
*/

int list_delete_index(pS_ListHead list_head,int index);
/*
   	���ܣ��������������в�ѯ 
	�βΣ�
		list_head:����ͷ
		data:��ѯ����
		compareFunc����ѯ�ص����� 
	����ֵ��NULL����ѯʧ��  || ��ѯ���Ľڵ��ַ 
*/
void* list_find_data_node(pS_ListHead list_head,void* data,CMP_FUNC compareFunc);
/*
   	���ܣ���ȡ����С���Ӧ������ 
	�βΣ�
		list_head:����ͷ
		index:�����±� 
		data����ȡ������(���ڵ��ú���ǰ��data����ռ�) 
	����ֵ�� 0���ɹ�  -1��ʧ��
*/
int list_get_index_data(pS_ListHead list_head,int index,void *data);

int list_detele_data_node(pS_ListHead list_head,void* data,CMP_FUNC compareFunc);

/*
   	���ܣ��������� 
	�βΣ�
		list_head:����ͷ
	
	����ֵ��void
*/

void list_destroy(pS_ListHead *list_head);







#endif 

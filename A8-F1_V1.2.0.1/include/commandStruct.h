#ifndef	 COMMAND_STRUCT_H__
#define  COMMAND_STRUCT_H__





#define ESC 0x7d
#define END 0x7e
typedef struct T_Data_Head{
	unsigned int  sourceID;
	unsigned int  desID;
	unsigned int  dtatLen; //高字节在前
	unsigned char dataStart;
}T_Data_Head,*pT_Data_Head;




typedef struct T_Comm_Head {
	unsigned char ackType;
	unsigned char cmd;
	unsigned char sequence;
	T_Data_Head   dataStart;
} T_Comm_Head, *pT_Comm_Head;







#endif
















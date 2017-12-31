/*
 * commStructDefine.h
 *
 *  Created on: Mar 29, 2014
 *      Author: china
 */
#ifndef COMMSTRUCTDEFINE_H_
#define COMMSTRUCTDEFINE_H_
#include <stdint.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>






#define false 0
#define true 1

/*---------X6B,A8,系统公用，需保持一致.-------BEGING------*/
//const uint8_t ESC = 0x7d;
//const uint8_t END = 0x7e;
#define ESC 0x7d
#define END 0x7e
typedef struct T_NetInterface {
	uint32_t ipaddr; //网络字节序
	uint32_t netmask; //网络字节序
	uint32_t gwaddr; //网络字节序
	uint32_t broadaddr; //网络字节序
	uint32_t multiaddr;//网络字节序
	unsigned char hwaddr[6];
} T_NetInterface, *pT_NetInterface;
typedef struct CacheBuf {
	unsigned char buf[1024];
	int sockfd;
	int len;
	int isProc;
} CacheBuf, *PcacheBuf;

typedef struct T_Room {
	unsigned char type; //类型 1
	unsigned char zone; //区     1
	unsigned char build; //栋   39
	unsigned char unit; //单元 1
	unsigned char floor; //楼层 6
	unsigned char room; //房号 8
	unsigned char devno; //设备号 23
} T_Room,  *pT_Room;

/*
 服务器下发时间，天气及温度信息时的数据结构
 注意: 均须按网络序保存
 */
typedef struct time_weather_t {
	uint32_t sec; //UTC时间1970开始的秒数
	uint32_t usec; //微妙数 x (2^32/10^6)
	unsigned char weather; //-1 不显示天气，0 大晴天， 1 晴转多云， 2 阴天多云，3 有雨 ...
	uint32_t temperature; //-1 不显示温度，低2字节有效，如: 0x1012表示16℃--18℃
} time_weather_t, *p_time_weather_t;

/*
 服务器下发版本升级通知时的数据结构
 注意: 均须按网络序保存
 */
typedef struct tftp_update_t {
	char fileName[64 + 1]; //文件名，字符串
	uint32_t size; //文件大小 字节
	uint32_t serverip; //置0，表示从发出命令的PC作为服务器下载tftp
} tftp_update_t, *p_tftp_update_t;

/*
 下发缴费通知数据结构
 注意: 均须按网络序保存
 */
typedef struct payment_reg_t {
	T_Room room; //住户的房号地址
	uint32_t validTime; //缴费到期时间，UTC时间1970开始的秒数，0表示清除本户的缴费通知
} payment_reg_t, *p_payment_reg_t;

/*
 下发卡片登记数据结构
 注意: 均须按网络序保存
 */
typedef struct card_reg_t {
	uint32_t id; //卡片ID
	T_Room room; //卡片所属住户的房号地址
	uint32_t validTime; //卡片有效期，UTC时间1970开始的秒数，置0不比较有效期
	char name[8]; //姓名，字符串
} card_reg_t, *p_card_reg_t;

//事件类别定义
#define HW_EVENT			0X00000001	//红外报警
#define MC_EVENT			0X00000002	//门磁报警
#define MQ_EVENT			0X00000004	//煤气报警
#define YW_EVENT			0X00000008	//烟雾报警
#define JJ_EVENT			0X00000010	//求助
#define OPEN_EVENT			0X00000020	//分机或管理机给主机或围墙机开门，继电器打开
#define CARD_OPEN_EVENT		0X00000040	//刷卡开门事件，继电器打开
#define PASS_OPEN_EVENT		0X00000080	//住户密码开门事件，继电器打开
#define CALL_EVENT			0X00000200	//呼叫
#define OPEN_OVER_EVENT		0X00000400	//开门超时报警
#define NOT_OPEN_EVENT		0X00000800	//锁开，门未开报警
#define PUBLIC_PASS_OPEN	0X00001000	//公共密码开门事件，继电器打开
#define DOOR_OPEN_EVENT		0X00002000	//门从关闭变为打开
#define DOOR_CLOSE_EVENT	0X00004000	//门从打开变为关闭
/*
 设备实时报警或实时上报开门等事件记录数据结构
 注意: 均须按网络序保存
 */
typedef struct event_record_t {
	uint32_t type; //事件类别，注意可能有多种事件同时发生
	uint32_t time; //事件发生的机器时间，UTC时间1970开始的秒数
	T_Room room; //事件发生的房号地址
	uint32_t id; //事件发生的卡片ID
} event_record_t, *p_event_record_t;

//设备每10分钟上报一次设备在线状态数据结构





typedef struct heart_beat_t {
	uint32_t eventSum; //设备未上报成功的事件记录数量，网络序
	char app_version[64]; //程序版本，字符串
	char room_version[64]; //房号表版本，字符串
	char typeCode;
} heart_beat_t, *p_heart_beat_t;

/*
 下发短信头部结构
 下发短信时，为方便排版，将短信内容以行为单位，分成N个字符串，组成一个字符串数组，每个字符串
 有且只能有一个结束符'\0'，每个字符串在机器上将显示一行，可用在字符串前面填空格的方法进行排版，
 字符串后面则不要加空格，汉字用GBK码，第一行除去前面的空格后将会被作为标题
 短信的最大行数及每行的最大字符数不同的设备会有所不同，下发短信时须根据不同的设备进行调整
 x6b的分机最大可显示10行，每行37个汉字，x6b的其它设备待定
 */
typedef struct sms_info_t {
	uint32_t validTime; //显示到期日期时间，仅门口机、围墙机要用到，分机可不填
	char isPublic; // 1 表示公共短信，0 表示个人短信
	char lineNum; //行数，也就是字符串的个数
	//GBK码字符串数组起始，每字符串对应显示为一行，GBK每汉字2字节，UTF8每汉字最大3字节
	char GBK_lineStart;
} sms_info_t, *p_sms_info_t;

typedef struct T_Comm_Head {

	unsigned char ackType;
	unsigned char cmd;
	unsigned char sequence;

	T_Room destAddr;
	T_Room sorAddr;

//  有数据时，是数据的开始，无数据时，是效验
	unsigned char dataStart;
} T_Comm_Head, *pT_Comm_Head;
/*---------X6B,A8,系统公用，需保持一致.-------END------*/



//TCP服务端收到客户端心跳包后，应答此结构
typedef struct {
	unsigned char devNo; //心跳包接收者的房号地址中的设备号
	unsigned char zjNumPerUnit; //每单元的门口机数
	unsigned char qrjNumPerRoom; //每个住户的别墅机数
	unsigned char wqjNo[16]; //本区围墙机编号列表，-1表示无效编号
	unsigned char globalWqjNo[16]; //全局围墙机编号列表，-1表示无效编号，全局围墙机区号为0

	//当前所有在线客户端的信息，包括接收者自己的信息，但不包含与服务端同体的客户端信息
#define TCP_CLIENT_MAX	5
	struct {
		int devNo; // <=0 表示此条目无客户端在线，其它为在线客户端的设备号
		uint32_t ip; //客户端的网络序IP地址
//		uint16_t port;	//客户端的网络序端口号
	} onlineClientInfos[TCP_CLIENT_MAX];

} stCLIETN_HEART_BEAT;

/*
typedef struct {
	unsigned char buf[1024];
	int len;
	char isProc;
	uint32_t srcIP;
} MsgBody, *pMsgBody;
*/
typedef struct DevInfo {
	T_Room roomInfo;
	uint32_t devIP;
	int sock_fd;
	int first_connect;
} DevInfo, *pDevInfo;

typedef struct T_Buffer {
	void *start;

	uint32_t length;
} T_Buffer, *pT_Buffer;

typedef enum T_eTALK_STATUS {
	DEVICE_OFFLINE = 0, //设备不在线
	TALK_FREE, //空闲状态
	CALL_WAIT_ACK, //呼叫等待应答
	WAIT_LOCAL_HOOK, //等待本地接听
	WAIT_DES_HOOK, //等待对方接听状态
	TALKING, //通话状态
	WATCH_WAIT_ACK, //监听 等待应答
	WATCHING, //监听状态
	MAX_STATUS,
} T_eTALK_STATUS;



typedef struct T_Home_Peers {
	char devno;
	char state;
	T_Room remoteRoom;
	struct in_addr dev_addr;
} T_Home_Peers, *pT_Home_Peers;

typedef struct T_Talker_Pair {
	uint32_t localIP;
	uint32_t remoteIP;
	T_Room remoteAddr;
} T_Talker_Pair, *pT_Talker_Pair;
typedef struct Chcek_Ack {
	int timer;
	int ChcekType;
	int CheckStatus;
} Chcek_Ack, *pChcek_Ack;
typedef struct T_Talker_Socket {
	T_eTALK_STATUS status;
	int timer;
	int talkType;
	T_Room remoteAddr;
} T_Talker_Socket, *pT_Talker_Socket;
typedef struct {

	T_eTALK_STATUS status;
	int timer;
	T_Room DesAddr;
//	T_Room SorAddr;
	/*
	 这个用于发送命令:
	 对方多播呼叫本地时，保存本地的多播地址
	 对方单播呼叫本机时，保存的是对方的单播地址
	 本机多播呼叫其它分机时，保存的是对方的多播地址
	 本机单播呼叫其它地方时，保存的是其它地方的单播地址
	 */
	uint32_t cmdIpAddr;
	/*
	 这个主要用作比较判断，保存对方的单播地址，但要注意，
	 当本机多播呼叫其它地方时，这个IP是不确定的，只有等对方摘机后，才能确定
	 */
	uint32_t ipAddr;
	int callFrom; //0，表示主动呼叫；1，表示收到呼叫
	//心跳包机制是判断客户端离线的另一种方法，暂还没有实现
	int heartbeatTimer;
	int ackStatus; //服务端 保存各设备的应答状态
} T_TALKER_SOCKET;

typedef struct T_cmdTask {
	unsigned char isProc;
	unsigned char ackType;
	unsigned char cmd;
	T_Room roomAddr;
} T_cmdTask, *pT_cmdTask;

typedef struct {
	char fileName[64 + 1];	//�ļ������ַ���
	uint32_t size;			//�ļ���С �ֽ�
	uint32_t serverip;		//��0����ʾ�ӷ��������PC��Ϊ����������tftp
}T_TFTP_UPDATE;



#endif /* COMMSTRUCTDEFINE_H_ */

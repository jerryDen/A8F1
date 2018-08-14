#ifndef   _ROOM_NET_CONFIG_H_
#define   _ROOM_NET_CONFIG_H_ 

#include "commStructDefine.h"

typedef struct {
	char version[32 + 1/*16*/];		//2014.04.19修改
	unsigned char subnetNum;//2014.04.19增加
	unsigned char zoneNum;
	unsigned char gljNum;	//全局管理机数
	unsigned char wqjNum;	//全局围墙机数
	unsigned char bajNum;	//全局保安分机数
	
//2014.04.19增加
	uint32_t sequenceStart;
}NETCONFIG_FILE_HEAD;

//2014.04.19增加此结构
typedef struct {
	unsigned int hosts;
	uint32_t ipaddr;
	uint32_t ipmask;
	uint32_t gw;
}NET_INTERFACE;

typedef struct {
	unsigned char zoneNo;
	unsigned char gljNum;
	unsigned char wqjNum;
	unsigned char bajNum;
	unsigned char buildNum;
}NETCONFIG_ZONE_HEAD;

//管理机、围墙机、或保安分机共用这个结构
typedef struct {
	unsigned char No;
	uint32_t ipaddr;
	uint32_t ipmask;
	uint32_t gw;

//2014.04.19增加
	uint32_t sequence;
}GLJ_HASH;


typedef struct {
	unsigned char buildNo;
	unsigned char startUnitNo;
	unsigned char unitNum;
	unsigned char zjNumPerUnit;
	unsigned char startFloorNo;
	unsigned char floorNum;
	unsigned char startRoomNo;
	unsigned char roomNumPerFloor;
	unsigned char deviceNumPerRoom;
	unsigned char qrjNumPerRoom;//别墅机

	uint32_t ipaddrStart;
	uint32_t ipmask;
	uint32_t gw;

//2014.04.19增加
	uint32_t sequenceStart;

//2014.09.23 增加电梯模块
	//是否配置电梯控制模块，如果配置有电梯模块，则每单元须多分配一个电梯模块的IP
    unsigned char elevator_module;
    //电梯控制偏移，即房号地址起始层对应电梯控制器的几号输出控制端口
    unsigned char elevator_offset;
}NETCONFIG_BUILD;

/*
注意: 以上所有结构中的ip, mask, gw均按网络序，以下接口函数中的网络参数也均是网络序
*/

/*
打开网络配置文件，配置文件须放在/usr/work目录下，更换网络配置表文件后，须首先调用此函数
返回 0 成功，其它 失败
*/
int initNetConfigFile(char *file);

/*
释放关闭网络配置文件
*/
void freeNetConfigFile(void);

/*
获取网络配置文件头部结构，将其指针赋值到globalZoneHead
注意： 不要对此结构指针globalZoneHead的任何成员变量进行赋值操作
返回 0 成功，其它 失败
*/

int getGlobalZoneHead( NETCONFIG_FILE_HEAD *globalZoneHead );

/*
获取指定房号地址所在分区的分区头部结构，将其指针赋值到pMyzone
注意： 不要对此结构指针pMyzone的任何成员变量进行赋值操作
返回 0 成功，其它 失败
*/
int getMyZone( T_Room *roomAddr, NETCONFIG_ZONE_HEAD *pMyzone );

/*
获取指定房号地址所在楼栋的楼栋结构，将其指针赋值到pMybuild
注意： 不要对此结构指针pMybuild的任何成员变量进行赋值操作
返回 0 成功，其它 失败
*/
int getMyBuild( T_Room *roomAddr, NETCONFIG_BUILD *pMybuild );

/*
根据指定的楼栋结构，获取指定的房号地址的IP地址及掩码，房号地址须在指定的楼栋结构中
返回 0 成功，其它 失败
*/
int getMyBuildRoomNetInterface( T_Room *roomAddr, NETCONFIG_BUILD *mybuild,
							uint32_t *ipAddr, uint32_t *ipMask, uint32_t *gw );

/*
从整个网络配置文件中获取指定房号地址的IP地址及掩码
返回 0 成功，其它 失败
*/
int getRoomNetInterface(T_Room *roomAddr, uint32_t *ipAddr, uint32_t *ipMask, uint32_t *gw);

/*
根据房号地址中的分区号、设备类别，获取多个同类设备中编号最小的设备
的IP地址、子网掩码、最小设备号、及设备个数，用以获取多播地址
设备类别只能是管理机、围墙机、保安机，它们都使用全局统一编号
返回 0 成功，其它 失败
*/
int getMinGljInterface( T_Room *roomAddr, uint32_t *ipAddr,
						  uint32_t *ipMask, unsigned char *devno, int *num);

/*
根据房号地址中设备类别及设备号，获取其IP地址、子网掩码、并将其分区号返回填入房号地址，
设备类别只能是管理机、围墙机、保安机，它们都使用全局统一编号
返回 0 成功，其它 失败
*/
int getGljInterface( T_Room *roomAddr, uint32_t *ipAddr, uint32_t *ipMask);



//2014.04.19增加如下接口

/*
获取指定序号子网的结构，将其赋值到subNet
subNetIndex:从1开始
返回 0 成功，其它 失败
*/
int getSubnetInterface( int subNetIndex, NET_INTERFACE *subNet );

/*
获取指定房号地址对应设备的设备序列号
返回 0 成功，其它 失败
*/
int getRoomSequence( T_Room *roomAddr, uint32_t *sequence );



//2014.09.23 增加电梯模块

/*
检查指定房号地址所在的楼栋是否配置有电梯模块，如果有，获取指定单元的电梯模块的网络接口
返回 0 成功，其它 失败
*/
int getElevatorInterface(T_Room *roomAddr, uint32_t *ipAddr, uint32_t *ipMask, uint32_t *gw);







#endif

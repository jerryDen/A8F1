#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <net/route.h>

#include "roomNetConfig.h"
#include "commandWord.h"
#include "DebugLog.h"


#define DEBUG_NETCONFIG	0

//static char *netConfigFile = WORK_DIR"/netConfig.bin";
static FILE *fp = NULL;
static GLJ_HASH *globalGlj = NULL;

//下面3个参数的地址会提供给本文件外的其它函数使用，这些其它函数要保证不对其内容进行修改
static NETCONFIG_FILE_HEAD fileHead;
static NETCONFIG_ZONE_HEAD *zoneHead = NULL;
static NETCONFIG_BUILD mybuild;
static NET_INTERFACE *subnet = NULL; //2014.04.19增加

/*
 打开网络配置文件，配置文件须放在/usr/work目录下，更换网络配置表文件后，须首先调用此函数
 返回 0 成功，其它 失败
 */
int initNetConfigFile(char *file) {
	int r, len = 166;

	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
	}

	fp = fopen(file, "rb+");
	if (fp == NULL) {
		LOGE("netConfig Not found\n");
		goto fail;
	}
	LOGD("fp:%p\n",fp);



	r = fread((char*) &fileHead, 1, sizeof(NETCONFIG_FILE_HEAD), fp);
	if ((unsigned int) r < sizeof(NETCONFIG_FILE_HEAD)) {
		LOGE("netConfig error %s : %d\n", __FUNCTION__, __LINE__);
		goto fail;
	}

//2014.04.19增加
	len = sizeof(NET_INTERFACE) * fileHead.subnetNum;
	subnet = (NET_INTERFACE *) malloc(len);
	if (subnet == NULL) {
		LOGE("not enough memory %s : %d\n", __FUNCTION__, __LINE__);
		goto fail;
	}

	r = fread((char*) subnet, 1, len, fp);
	if (r < len) {
		LOGE("netConfig error %s : %d\n", __FUNCTION__, __LINE__);
		goto fail;
	}
/////

	len = sizeof(NETCONFIG_ZONE_HEAD) * fileHead.zoneNum;
	zoneHead = (NETCONFIG_ZONE_HEAD *) malloc(len);
	if (zoneHead == NULL) {
		LOGE("not enough memory %s : %d\n", __FUNCTION__, __LINE__);
		goto fail;
	}

	r = fread((char*) zoneHead, 1, len, fp);
	if (r < len) {
		LOGE("netConfig error %s : %d\n", __FUNCTION__, __LINE__);
		goto fail;
	}

	len = sizeof(GLJ_HASH)
			* (fileHead.gljNum + fileHead.wqjNum + fileHead.bajNum);
	if (len) {
		globalGlj = (GLJ_HASH *) malloc(len);
		if (globalGlj == NULL) {
			LOGE("not enough memory %s : %d\n", __FUNCTION__, __LINE__);
			goto fail;
		}

		r = fread((char*) globalGlj, 1, len, fp);
		if (r < len) {
			LOGE("netConfig error %s : %d\n", __FUNCTION__, __LINE__);
			goto fail;
		}
	}


	return 0;

	fail: if (globalGlj != NULL) {
		free(globalGlj);
		globalGlj = NULL;
	}
	if (zoneHead != NULL) {
		free(zoneHead);
		zoneHead = NULL;
	}
	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
	}
	return -1;
}

/*
 释放关闭网络配置文件
 */
void freeNetConfigFile(void) {
	if (globalGlj != NULL) {
		free(globalGlj);
		globalGlj = NULL;
	}
	if (subnet != NULL) {
		free(subnet);
		subnet = NULL;
	}
	if (zoneHead != NULL) {
		free(zoneHead);
		zoneHead = NULL;
	}
	if (fp != NULL) {
		fclose(fp);
		fp = NULL;
	}
}

/*
 获取网络配置文件头部结构，将其指针赋值到globalZoneHead
 注意： 不要对此结构指针globalZoneHead的任何成员变量进行赋值操作
 返回 0 成功，其它 失败
 */
int getGlobalZoneHead(NETCONFIG_FILE_HEAD *globalZoneHead) {
	if (fp == NULL)
		return -1;

	if (globalZoneHead)
		*globalZoneHead = fileHead;
	return 0;
}

/*
 获取指定房号地址所在分区的分区头部结构，将其指针赋值到pMyzone
 注意： 不要对此结构指针pMyzone的任何成员变量进行赋值操作
 返回 0 成功，其它 失败
 */
int getMyZone(T_Room *roomAddr, NETCONFIG_ZONE_HEAD *pMyzone) {
	int zoneIndex;

	if (fp == NULL)
		return -1;

	for (zoneIndex = 0; zoneIndex < fileHead.zoneNum; zoneIndex++) {
		if (zoneHead[zoneIndex].zoneNo == roomAddr->zone) {
			if (pMyzone != NULL)
				*pMyzone = zoneHead[zoneIndex];
			return 0;
		}
	}
	LOGE("分区号未登记 %s : %d\n", __FUNCTION__, __LINE__);
	return -1;
}

/*
 获取指定房号地址所在楼栋的楼栋结构，将其指针赋值到pMybuild
 注意： 不要对此结构指针pMybuild的任何成员变量进行赋值操作
 返回 0 成功，其它 失败
 */
int getMyBuild(T_Room *roomAddr, NETCONFIG_BUILD *pMybuild) {
	int zoneIndex, len, r;
	char buf[100 * sizeof(NETCONFIG_BUILD)]; //须保证能装下100个楼栋结构的数据
	NETCONFIG_BUILD *buildHash = (NETCONFIG_BUILD *) buf;
	int index;
	if (fp == NULL)
		return -1;
	//len计算指定分区头部到文件起始的位置偏移
	len = sizeof(NETCONFIG_FILE_HEAD)
			+ sizeof(NETCONFIG_ZONE_HEAD) * fileHead.zoneNum
			+ sizeof(GLJ_HASH)
					* (fileHead.gljNum + fileHead.wqjNum + fileHead.bajNum)
			+ sizeof(NET_INTERFACE) * fileHead.subnetNum;
	for (zoneIndex = 0; zoneIndex < fileHead.zoneNum; zoneIndex++) {
		if (zoneHead[zoneIndex].zoneNo == roomAddr->zone)
			break;
		len += (zoneHead[zoneIndex].gljNum + zoneHead[zoneIndex].wqjNum
				+ zoneHead[zoneIndex].bajNum) * sizeof(GLJ_HASH)
				+ zoneHead[zoneIndex].buildNum * sizeof(NETCONFIG_BUILD);
	}
	if (zoneIndex >= fileHead.zoneNum) {
		LOGE("分区号未登记 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (zoneHead[zoneIndex].buildNum == 0) {
		LOGE("楼栋未登记 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;
	}
	
	if (zoneHead[zoneIndex].gljNum)
		len += zoneHead[zoneIndex].gljNum * sizeof(GLJ_HASH);
	if (zoneHead[zoneIndex].wqjNum)
		len += zoneHead[zoneIndex].wqjNum * sizeof(GLJ_HASH);
	if (zoneHead[zoneIndex].bajNum)
		len += zoneHead[zoneIndex].bajNum * sizeof(GLJ_HASH);
	r = fseek(fp, len, SEEK_SET);
	if (r < 0) {
		LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	len = zoneHead[zoneIndex].buildNum * sizeof(NETCONFIG_BUILD);
	r = fread(buf, 1, len, fp);
	if (r < len) {
		LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	for (index = 0; index < zoneHead[zoneIndex].buildNum;
			index++, buildHash++) {
		if (buildHash->buildNo == roomAddr->build) {
			if (pMybuild != NULL) {
				memcpy((char*) &mybuild, (char*) buildHash,
						sizeof(NETCONFIG_BUILD));
				*pMybuild = mybuild;
			}
			return 0;
		}
	}
	LOGE("楼栋未登记 %s : %d\n", __FUNCTION__, __LINE__);
	return -1;
}

//2014.09.23 增加电梯模块

//是否配置电梯控制模块，如果配置有电梯模块，则每单元须多分配一个电梯模块的IP
static int getUnitHosts(NETCONFIG_BUILD *build) {
	int floorHost = (build->deviceNumPerRoom + build->qrjNumPerRoom)
			* build->roomNumPerFloor;
	if (build->elevator_module)
		return (build->zjNumPerUnit + floorHost * build->floorNum + 1);
	else
		return (build->zjNumPerUnit + floorHost * build->floorNum);
}

/*
 根据指定的楼栋结构，获取指定的房号地址的IP地址及掩码，房号地址须在指定的楼栋结构中
 返回 0 成功，其它 失败
 */
int getMyBuildRoomNetInterface(T_Room *roomAddr, NETCONFIG_BUILD *mybuild,
		uint32_t *ipAddr, uint32_t *ipMask, uint32_t *gw) {
	if (mybuild == NULL) {
		LOGE("mybuild is NULL\n");
		return -1;
	}

	switch (roomAddr->type) {

	case X6B_ZJ:
		if ((mybuild->startUnitNo <= roomAddr->unit)
				&& ((mybuild->startUnitNo + mybuild->unitNum) > roomAddr->unit)
				&& (mybuild->zjNumPerUnit >= roomAddr->devno)) {

			if (ipAddr) {
//2014.09.23 增加电梯模块
				int unitHost = getUnitHosts(mybuild);

				uint32_t ip;
				ip = ntohl( mybuild->ipaddrStart )
						+ (roomAddr->unit - mybuild->startUnitNo) * unitHost
						+ roomAddr->devno - 1;
				*ipAddr = htonl(ip);
			}
			if (ipMask)
				*ipMask = mybuild->ipmask;
			if (gw)
				*gw = mybuild->gw;
			return 0;
		}
		LOGE("门口主机未登记 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;

	case X6B_QRJ:
		if ((mybuild->startUnitNo <= roomAddr->unit)
				&& ((mybuild->startUnitNo + mybuild->unitNum) > roomAddr->unit)
				&&

				(mybuild->startFloorNo <= roomAddr->floor)
				&& ((mybuild->startFloorNo + mybuild->floorNum)
						> roomAddr->floor) &&

				(mybuild->startRoomNo <= roomAddr->room)
				&& ((mybuild->startRoomNo + mybuild->roomNumPerFloor)
						> roomAddr->room) &&

				(mybuild->qrjNumPerRoom >= roomAddr->devno)) {

			if (ipAddr) {
				int roomHost = mybuild->qrjNumPerRoom
						+ mybuild->deviceNumPerRoom;
				int floorHost = roomHost * mybuild->roomNumPerFloor;
//2014.09.23 增加电梯模块
				int unitHost = getUnitHosts(mybuild);

				uint32_t ip;
				ip = ntohl( mybuild->ipaddrStart )
						+ (roomAddr->unit - mybuild->startUnitNo) * unitHost
						+ mybuild->zjNumPerUnit
						+ (roomAddr->floor - mybuild->startFloorNo) * floorHost
						+ (roomAddr->room - mybuild->startRoomNo) * roomHost
						+ roomAddr->devno - 1;
				*ipAddr = htonl(ip);
			}
			if (ipMask)
				*ipMask = mybuild->ipmask;
			if (gw)
				*gw = mybuild->gw;
			return 0;
		}
		LOGE("确认机未登记 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;

	case X6B_FJ:
		if ((mybuild->startUnitNo <= roomAddr->unit)
				&& ((mybuild->startUnitNo + mybuild->unitNum) > roomAddr->unit)
				&&

				(mybuild->startFloorNo <= roomAddr->floor)
				&& ((mybuild->startFloorNo + mybuild->floorNum)
						> roomAddr->floor) &&

				(mybuild->startRoomNo <= roomAddr->room)
				&& ((mybuild->startRoomNo + mybuild->roomNumPerFloor)
						> roomAddr->room) &&

				(mybuild->deviceNumPerRoom >= roomAddr->devno)) {

			if (ipAddr) {
				int roomHost = mybuild->qrjNumPerRoom
						+ mybuild->deviceNumPerRoom;
				int floorHost = roomHost * mybuild->roomNumPerFloor;
//2014.09.23 增加电梯模块
				int unitHost = getUnitHosts(mybuild);

				uint32_t ip;
				ip = ntohl( mybuild->ipaddrStart )
						+ (roomAddr->unit - mybuild->startUnitNo) * unitHost
						+ mybuild->zjNumPerUnit
						+ (roomAddr->floor - mybuild->startFloorNo) * floorHost
						+ (roomAddr->room - mybuild->startRoomNo) * roomHost
						+ mybuild->qrjNumPerRoom + roomAddr->devno - 1;
				*ipAddr = htonl(ip);
			}
			if (ipMask)
				*ipMask = mybuild->ipmask;
			if (gw)
				*gw = mybuild->gw;
			return 0;
		}
		LOGE("室内分机未登记 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	LOGE("设备类别不是主机、分机或确认机 %s : %d\n", __FUNCTION__, __LINE__);
	return -1;
}

/*
 从整个网络配置文件中获取指定房号地址的IP地址及掩码
 返回 0 成功，其它 失败
 */
int getRoomNetInterface(T_Room *roomAddr, uint32_t *ipAddr, uint32_t *ipMask,
		uint32_t *gw) {
	int zoneIndex, len, r;
	char buf[100 * sizeof(NETCONFIG_BUILD)]; //须保证能装下100个楼栋结构的数据
	GLJ_HASH *gljHash = (GLJ_HASH *) buf; //管理机、围墙机，保安分机都使用这个变量
	NETCONFIG_BUILD *buildHash = (NETCONFIG_BUILD *) buf;
	int index;
	
	if (fp == NULL)
		return -1;
	

	//分区为0的设备只能是管理机、围墙机、保安分机，它们是全局的，不属于某个分区
	if (roomAddr->zone == 0) {
		
		switch (roomAddr->type) {
		case X6B_GLJ:
			
			if (fileHead.gljNum == 0) {
				LOGE("管理机未登记 %s : %d\n", __FUNCTION__, __LINE__);
				return -1;
			}
			
			for (index = 0; index < fileHead.gljNum; index++) {
				if (globalGlj[index].No == roomAddr->devno) {
					if (ipAddr)
						*ipAddr = globalGlj[index].ipaddr;
					if (ipMask)
						*ipMask = globalGlj[index].ipmask;
					if (gw)
						*gw = globalGlj[index].gw;
					return 0;
				}
			}
			LOGE("管理机未登记 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;

		case X6B_WQJ:
			if (fileHead.wqjNum == 0) {
				LOGE("围墙机未登记 %s : %d\n", __FUNCTION__, __LINE__);
				return -1;
			}
			for (index = fileHead.gljNum;
					index < (fileHead.gljNum + fileHead.wqjNum); index++) {
				if (globalGlj[index].No == roomAddr->devno) {
					if (ipAddr)
						*ipAddr = globalGlj[index].ipaddr;
					if (ipMask)
						*ipMask = globalGlj[index].ipmask;
					if (gw)
						*gw = globalGlj[index].gw;
					return 0;
				}
			}
			LOGE("围墙机未登记 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;

		case X6B_BAJ:
			if (fileHead.bajNum == 0) {
				LOGE("保安分机未登记 %s : %d\n", __FUNCTION__, __LINE__);
				return -1;
			}
			for (index = (fileHead.gljNum + fileHead.wqjNum);
					index
							< (fileHead.gljNum + fileHead.wqjNum
									+ fileHead.bajNum); index++) {
				if (globalGlj[index].No == roomAddr->devno) {
					if (ipAddr)
						*ipAddr = globalGlj[index].ipaddr;
					if (ipMask)
						*ipMask = globalGlj[index].ipmask;
					if (gw)
						*gw = globalGlj[index].gw;
					return 0;
				}
			}
			LOGE("保安分机未登记 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}
		LOGE("设备类别不是管理机、围墙机或保安分机 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	//len计算指定分区头部到文件起始的位置偏移
	len = sizeof(NETCONFIG_FILE_HEAD)
			+ sizeof(NETCONFIG_ZONE_HEAD) * fileHead.zoneNum
			+ sizeof(GLJ_HASH)
					* (fileHead.gljNum + fileHead.wqjNum + fileHead.bajNum)
			+ sizeof(NET_INTERFACE) * fileHead.subnetNum;
	for (zoneIndex = 0; zoneIndex < fileHead.zoneNum; zoneIndex++) {
		if (zoneHead[zoneIndex].zoneNo == roomAddr->zone)
			break;
		len += (zoneHead[zoneIndex].gljNum + zoneHead[zoneIndex].wqjNum
				+ zoneHead[zoneIndex].bajNum) * sizeof(GLJ_HASH)
				+ zoneHead[zoneIndex].buildNum * sizeof(NETCONFIG_BUILD);
	}
	if (zoneIndex >= fileHead.zoneNum) {
		LOGE("分区号未登记\n");
		return -1;
	}
	
	

	switch (roomAddr->type) {

	case X6B_GLJ:
		
		if (zoneHead[zoneIndex].gljNum == 0) {
			LOGE("管理机未登记 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		r = fseek(fp, len, SEEK_SET);
		if (r < 0) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		len = zoneHead[zoneIndex].gljNum * sizeof(GLJ_HASH);
		r = fread(buf, 1, len, fp);
		if (r < len) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}
		
		for (index = 0; index < zoneHead[zoneIndex].gljNum; index++) {
			
			if (gljHash[index].No == roomAddr->devno) {
			
				if (ipAddr)
					*ipAddr = gljHash[index].ipaddr;
				
				if (ipMask)
					*ipMask = gljHash[index].ipmask;
			
				if (gw)
					*gw = gljHash[index].gw;
			
				return 0;
			}
			
		}
		
		LOGE("管理机未登记 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;

	case X6B_WQJ:
		if (zoneHead[zoneIndex].wqjNum == 0) {
			LOGE("围墙机未登记 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		if (zoneHead[zoneIndex].gljNum)
			len += zoneHead[zoneIndex].gljNum * sizeof(GLJ_HASH);
		r = fseek(fp, len, SEEK_SET);
		if (r < 0) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		len = zoneHead[zoneIndex].wqjNum * sizeof(GLJ_HASH);
		r = fread(buf, 1, len, fp);
		if (r < len) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		for (index = 0; index < zoneHead[zoneIndex].wqjNum; index++) {
			if (gljHash[index].No == roomAddr->devno) {
				if (ipAddr)
					*ipAddr = gljHash[index].ipaddr;
				if (ipMask)
					*ipMask = gljHash[index].ipmask;
				if (gw)
					*gw = gljHash[index].gw;
				return 0;
			}
		}
		LOGE("围墙机未登记 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;

	case X6B_BAJ:
		if (zoneHead[zoneIndex].bajNum == 0) {
			LOGE("保安分机未登记 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		if (zoneHead[zoneIndex].gljNum)
			len += zoneHead[zoneIndex].gljNum * sizeof(GLJ_HASH);
		if (zoneHead[zoneIndex].wqjNum)
			len += zoneHead[zoneIndex].wqjNum * sizeof(GLJ_HASH);
		r = fseek(fp, len, SEEK_SET);
		if (r < 0) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		len = zoneHead[zoneIndex].bajNum * sizeof(GLJ_HASH);
		r = fread(buf, 1, len, fp);
		if (r < len) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		for (index = 0; index < zoneHead[zoneIndex].bajNum; index++) {
			if (gljHash[index].No == roomAddr->devno) {
				if (ipAddr)
					*ipAddr = gljHash[index].ipaddr;
				if (ipMask)
					*ipMask = gljHash[index].ipmask;
				if (gw)
					*gw = gljHash[index].gw;
				return 0;
			}
		}
		LOGE("保安分机未登记 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;

	case X6B_ZJ:
		if (zoneHead[zoneIndex].buildNum == 0) {
			LOGE("门口主机未登记 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		if (zoneHead[zoneIndex].gljNum)
			len += zoneHead[zoneIndex].gljNum * sizeof(GLJ_HASH);
		if (zoneHead[zoneIndex].wqjNum)
			len += zoneHead[zoneIndex].wqjNum * sizeof(GLJ_HASH);
		if (zoneHead[zoneIndex].bajNum)
			len += zoneHead[zoneIndex].bajNum * sizeof(GLJ_HASH);
		r = fseek(fp, len, SEEK_SET);
		if (r < 0) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		len = zoneHead[zoneIndex].buildNum * sizeof(NETCONFIG_BUILD);
		r = fread(buf, 1, len, fp);
		if (r < len) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		for (index = 0; index < zoneHead[zoneIndex].buildNum;
				index++, buildHash++) {
			if ((buildHash->buildNo == roomAddr->build) &&

			(buildHash->startUnitNo <= roomAddr->unit)
					&& ((buildHash->startUnitNo + buildHash->unitNum)
							> roomAddr->unit) &&

					(buildHash->zjNumPerUnit >= roomAddr->devno)) {
#if DEBUG_NETCONFIG
				LOGE("roomAddr: %d-%d-%d-%d-%d-%d\n", roomAddr->zone, roomAddr->build,
						roomAddr->unit, roomAddr->floor, roomAddr->room, roomAddr->devno);
				struct in_addr sin_addr;
				sin_addr.s_addr = buildHash->ipaddrStart;
				LOGE("ip %s ", inet_ntoa(sin_addr) );
				sin_addr.s_addr = buildHash->ipmask;
				LOGE( "mask:%s\n", inet_ntoa(sin_addr) );
#endif
				if (ipAddr) {
//2014.09.23 增加电梯模块
					int unitHost = getUnitHosts(buildHash);

					uint32_t ip;
					ip = ntohl( buildHash->ipaddrStart )
							+ (roomAddr->unit - buildHash->startUnitNo)
									* unitHost + roomAddr->devno - 1;
					*ipAddr = htonl(ip);
				}
				if (ipMask)
					*ipMask = buildHash->ipmask;
				if (gw)
					*gw = buildHash->gw;
				return 0;
			}
		}
		LOGE("门口主机未登记 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;

	case X6B_QRJ:
		if (zoneHead[zoneIndex].buildNum == 0) {
			LOGE("确认机未登记 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		if (zoneHead[zoneIndex].gljNum)
			len += zoneHead[zoneIndex].gljNum * sizeof(GLJ_HASH);
		if (zoneHead[zoneIndex].wqjNum)
			len += zoneHead[zoneIndex].wqjNum * sizeof(GLJ_HASH);
		if (zoneHead[zoneIndex].bajNum)
			len += zoneHead[zoneIndex].bajNum * sizeof(GLJ_HASH);
		r = fseek(fp, len, SEEK_SET);
		if (r < 0) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		len = zoneHead[zoneIndex].buildNum * sizeof(NETCONFIG_BUILD);
		r = fread(buf, 1, len, fp);
		if (r < len) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		for (index = 0; index < zoneHead[zoneIndex].buildNum;
				index++, buildHash++) {
			if ((buildHash->buildNo == roomAddr->build) &&

			(buildHash->startUnitNo <= roomAddr->unit)
					&& ((buildHash->startUnitNo + buildHash->unitNum)
							> roomAddr->unit) &&

					(buildHash->startFloorNo <= roomAddr->floor)
					&& ((buildHash->startFloorNo + buildHash->floorNum)
							> roomAddr->floor) &&

					(buildHash->startRoomNo <= roomAddr->room)
					&& ((buildHash->startRoomNo + buildHash->roomNumPerFloor)
							> roomAddr->room) &&

					(buildHash->qrjNumPerRoom >= roomAddr->devno)) {
#if DEBUG_NETCONFIG
				LOGE("roomAddr: %d-%d-%d-%d-%d-%d\n", roomAddr->zone, roomAddr->build,
						roomAddr->unit, roomAddr->floor, roomAddr->room, roomAddr->devno);
				struct in_addr sin_addr;
				sin_addr.s_addr = buildHash->ipaddrStart;
				LOGE("ip %s ", inet_ntoa(sin_addr) );
				sin_addr.s_addr = buildHash->ipmask;
				LOGE( "mask:%s\n", inet_ntoa(sin_addr) );
#endif
				if (ipAddr) {
					int roomHost = buildHash->qrjNumPerRoom
							+ buildHash->deviceNumPerRoom;
					int floorHost = roomHost * buildHash->roomNumPerFloor;
//2014.09.23 增加电梯模块
					int unitHost = getUnitHosts(buildHash);

					uint32_t ip;
					ip = ntohl( buildHash->ipaddrStart )
							+ (roomAddr->unit - buildHash->startUnitNo)
									* unitHost + buildHash->zjNumPerUnit
							+ (roomAddr->floor - buildHash->startFloorNo)
									* floorHost
							+ (roomAddr->room - buildHash->startRoomNo)
									* roomHost + roomAddr->devno - 1;
					*ipAddr = htonl(ip);
				}
				if (ipMask)
					*ipMask = buildHash->ipmask;
				if (gw)
					*gw = buildHash->gw;
				return 0;
			}
		}
		LOGE("确认机未登记 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;

	case X6B_FJ:
		if (zoneHead[zoneIndex].buildNum == 0) {
			LOGE("室内分机未登记 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		if (zoneHead[zoneIndex].gljNum)
			len += zoneHead[zoneIndex].gljNum * sizeof(GLJ_HASH);
		if (zoneHead[zoneIndex].wqjNum)
			len += zoneHead[zoneIndex].wqjNum * sizeof(GLJ_HASH);
		if (zoneHead[zoneIndex].bajNum)
			len += zoneHead[zoneIndex].bajNum * sizeof(GLJ_HASH);
		r = fseek(fp, len, SEEK_SET);
		if (r < 0) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		len = zoneHead[zoneIndex].buildNum * sizeof(NETCONFIG_BUILD);
		r = fread(buf, 1, len, fp);
		if (r < len) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		for (index = 0; index < zoneHead[zoneIndex].buildNum;
				index++, buildHash++) {
			if ((buildHash->buildNo == roomAddr->build) &&

			(buildHash->startUnitNo <= roomAddr->unit)
					&& ((buildHash->startUnitNo + buildHash->unitNum)
							> roomAddr->unit) &&

					(buildHash->startFloorNo <= roomAddr->floor)
					&& ((buildHash->startFloorNo + buildHash->floorNum)
							> roomAddr->floor) &&

					(buildHash->startRoomNo <= roomAddr->room)
					&& ((buildHash->startRoomNo + buildHash->roomNumPerFloor)
							> roomAddr->room) &&

					(buildHash->deviceNumPerRoom >= roomAddr->devno)) {
#if DEBUG_NETCONFIG
				LOGE("roomAddr: %d-%d-%d-%d-%d-%d\n", roomAddr->zone, roomAddr->build,
						roomAddr->unit, roomAddr->floor, roomAddr->room, roomAddr->devno);
				struct in_addr sin_addr;
				sin_addr.s_addr = buildHash->ipaddrStart;
				LOGE("ip %s ", inet_ntoa(sin_addr) );
				sin_addr.s_addr = buildHash->ipmask;
				LOGE( "mask:%s\n", inet_ntoa(sin_addr) );
#endif
				if (ipAddr) {
					int roomHost = buildHash->qrjNumPerRoom
							+ buildHash->deviceNumPerRoom;
					int floorHost = roomHost * buildHash->roomNumPerFloor;
//2014.09.23 增加电梯模块
					int unitHost = getUnitHosts(buildHash);

					uint32_t ip;
					ip = ntohl( buildHash->ipaddrStart )
							+ (roomAddr->unit - buildHash->startUnitNo)
									* unitHost + buildHash->zjNumPerUnit
							+ (roomAddr->floor - buildHash->startFloorNo)
									* floorHost
							+ (roomAddr->room - buildHash->startRoomNo)
									* roomHost + buildHash->qrjNumPerRoom
							+ roomAddr->devno - 1;
					*ipAddr = htonl(ip);
				}
				if (ipMask)
					*ipMask = buildHash->ipmask;
				if (gw)
					*gw = buildHash->gw;
				return 0;
			}
		}
		LOGE("室内分机未登记 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;

	case X6B_BSZJ:
		return -1;
	}

	LOGE("设备类别未登记 %s : %d\n", __FUNCTION__, __LINE__);
	return -1;
}

/*
 根据房号地址中的分区号、设备类别，获取多个同类设备中编号最小的设备
 的IP地址、子网掩码、最小设备号、及设备个数，用以获取多播地址
 设备类别只能是管理机或围墙机或保安分机
 返回 0 成功，其它 失败
 */
int getMinGljInterface(T_Room *roomAddr, uint32_t *ipAddr, uint32_t *ipMask,
		unsigned char *devno, int *num) {
	int zoneIndex, len, r;
	char buf[100 * sizeof(GLJ_HASH)]; //须保证能装下100个楼栋结构的数据
	GLJ_HASH *gljHash = (GLJ_HASH *) buf; //管理机、围墙机，保安分机都使用这个变量
	int i, minIndex;

	if (fp == NULL)
		return -1;

	//分区为0的设备只能是管理机、围墙机、保安分机，它们是全局的，不属于某个分区
	if (roomAddr->zone == 0) {
		switch (roomAddr->type) {
		case X6B_GLJ:
			if (fileHead.gljNum == 0) {
				LOGE("管理机未登记 %s : %d\n", __FUNCTION__, __LINE__);
				return -1;
			}
			//找出编号最小的管理机
			minIndex = 0;
			for (i = 1; i < fileHead.gljNum; i++)
				if (globalGlj[minIndex].No > globalGlj[i].No)
					minIndex = i;
			if (ipAddr)
				*ipAddr = globalGlj[minIndex].ipaddr;
			if (ipMask)
				*ipMask = globalGlj[minIndex].ipmask;
			if (devno)
				*devno = globalGlj[minIndex].No;
			if (num)
				*num = fileHead.gljNum;
			return 0;

		case X6B_WQJ:
			if (fileHead.wqjNum == 0) {
				LOGE("围墙机未登记 %s : %d\n", __FUNCTION__, __LINE__);
				return -1;
			}
			minIndex = fileHead.gljNum;
			for (i = (minIndex + 1); i < (fileHead.gljNum + fileHead.wqjNum);
					i++)
				if (globalGlj[minIndex].No > globalGlj[i].No)
					minIndex = i;
			if (ipAddr)
				*ipAddr = globalGlj[minIndex].ipaddr;
			if (ipMask)
				*ipMask = globalGlj[minIndex].ipmask;
			if (devno)
				*devno = globalGlj[minIndex].No;
			if (num)
				*num = fileHead.wqjNum;
			return 0;

		case X6B_BAJ:
			if (fileHead.bajNum == 0) {
				LOGE("保安分机未登记 %s : %d\n", __FUNCTION__, __LINE__);
				return -1;
			}
			minIndex = fileHead.gljNum + fileHead.wqjNum;
			for (i = (minIndex + 1);
					i < (fileHead.gljNum + fileHead.wqjNum + fileHead.bajNum);
					i++)
				if (globalGlj[minIndex].No > globalGlj[i].No)
					minIndex = i;
			if (ipAddr)
				*ipAddr = globalGlj[minIndex].ipaddr;
			if (ipMask)
				*ipMask = globalGlj[minIndex].ipmask;
			if (devno)
				*devno = globalGlj[minIndex].No;
			if (num)
				*num = fileHead.bajNum;
			return 0;
		}
		LOGE("设备类别不是管理机、围墙机或保安分机 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	//len计算指定分区头部到文件起始的位置偏移
	len = sizeof(NETCONFIG_FILE_HEAD)
			+ sizeof(NETCONFIG_ZONE_HEAD) * fileHead.zoneNum
			+ sizeof(GLJ_HASH)
					* (fileHead.gljNum + fileHead.wqjNum + fileHead.bajNum)
			+ sizeof(NET_INTERFACE) * fileHead.subnetNum;
	for (zoneIndex = 0; zoneIndex < fileHead.zoneNum; zoneIndex++) {
		if (zoneHead[zoneIndex].zoneNo == roomAddr->zone)
			break;
		len += (zoneHead[zoneIndex].gljNum + zoneHead[zoneIndex].wqjNum
				+ zoneHead[zoneIndex].bajNum) * sizeof(GLJ_HASH)
				+ zoneHead[zoneIndex].buildNum * sizeof(NETCONFIG_BUILD);
	}
	if (zoneIndex >= fileHead.zoneNum) {
		LOGE("分区号未登记\n");
		return -1;
	}

	switch (roomAddr->type) {

	case X6B_GLJ:
		if (zoneHead[zoneIndex].gljNum == 0) {
			LOGE("管理机未登记 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		r = fseek(fp, len, SEEK_SET);
		if (r < 0) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		len = zoneHead[zoneIndex].gljNum * sizeof(GLJ_HASH);
		r = fread(buf, 1, len, fp);
		if (r < len) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		//找出编号最小的管理机
		minIndex = 0;
		for (i = 1; i < zoneHead[zoneIndex].gljNum; i++) {
			if (gljHash[minIndex].No > gljHash[i].No)
				minIndex = i;
		}
		if (ipAddr)
			*ipAddr = gljHash[minIndex].ipaddr;
		if (ipMask)
			*ipMask = gljHash[minIndex].ipmask;
		if (devno)
			*devno = gljHash[minIndex].No;
		if (num)
			*num = zoneHead[zoneIndex].gljNum;
		return 0;

	case X6B_WQJ:
		if (zoneHead[zoneIndex].wqjNum == 0) {
			LOGE("围墙机未登记 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		if (zoneHead[zoneIndex].gljNum)
			len += zoneHead[zoneIndex].gljNum * sizeof(GLJ_HASH);
		r = fseek(fp, len, SEEK_SET);
		if (r < 0) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		len = zoneHead[zoneIndex].wqjNum * sizeof(GLJ_HASH);
		r = fread(buf, 1, len, fp);
		if (r < len) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		//找出编号最小的围墙机
		minIndex = 0;
		for (i = 1; i < zoneHead[zoneIndex].wqjNum; i++) {
			if (gljHash[minIndex].No > gljHash[i].No)
				minIndex = i;
		}
		if (ipAddr)
			*ipAddr = gljHash[minIndex].ipaddr;
		if (ipMask)
			*ipMask = gljHash[minIndex].ipmask;
		if (devno)
			*devno = gljHash[minIndex].No;
		if (num)
			*num = zoneHead[zoneIndex].wqjNum;
		return 0;

	case X6B_BAJ:
		if (zoneHead[zoneIndex].bajNum == 0) {
			LOGE("保安分机未登记 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		if (zoneHead[zoneIndex].gljNum)
			len += zoneHead[zoneIndex].gljNum * sizeof(GLJ_HASH);
		if (zoneHead[zoneIndex].wqjNum)
			len += zoneHead[zoneIndex].wqjNum * sizeof(GLJ_HASH);
		r = fseek(fp, len, SEEK_SET);
		if (r < 0) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		len = zoneHead[zoneIndex].bajNum * sizeof(GLJ_HASH);
		r = fread(buf, 1, len, fp);
		if (r < len) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		//找出编号最小的围墙机
		minIndex = 0;
		for (i = 1; i < zoneHead[zoneIndex].bajNum; i++) {
			if (gljHash[minIndex].No > gljHash[i].No)
				minIndex = i;
		}
		if (ipAddr)
			*ipAddr = gljHash[minIndex].ipaddr;
		if (ipMask)
			*ipMask = gljHash[minIndex].ipmask;
		if (devno)
			*devno = gljHash[minIndex].No;
		if (num)
			*num = zoneHead[zoneIndex].bajNum;
		return 0;
	}

	LOGE("设备类别不是管理机、围墙机或保安分机 %s : %d\n", __FUNCTION__, __LINE__);
	return -1;
}

/*
 根据房号地址中设备类别及设备号，获取其IP地址、子网掩码、并将其分区号返回填入房号地址，
 设备类别只能是管理机、围墙机、保安机，它们都使用全局统一编号
 返回 0 成功，其它 失败
 */
int getGljInterface(T_Room *roomAddr, uint32_t *ipAddr, uint32_t *ipMask) {
	int zoneIndex, len, r;
	char buf[100 * sizeof(GLJ_HASH)]; //须保证能装下100个楼栋结构的数据
	GLJ_HASH *gljHash = (GLJ_HASH *) buf; //管理机、围墙机，保安分机都使用这个变量
	int i;

	if (fp == NULL)
		return -1;

	switch (roomAddr->type) {
	case X6B_GLJ:
		if (fileHead.gljNum) {
			for (i = 0; i < fileHead.gljNum; i++) {
				if (globalGlj[i].No == roomAddr->devno) {
					if (ipAddr)
						*ipAddr = globalGlj[i].ipaddr;
					if (ipMask)
						*ipMask = globalGlj[i].ipmask;
					roomAddr->zone = 0;
					return 0;
				}
			}
		}
		//len计算分区头部到文件起始的位置偏移
		len = sizeof(NETCONFIG_FILE_HEAD)
				+ sizeof(NETCONFIG_ZONE_HEAD) * fileHead.zoneNum
				+ sizeof(GLJ_HASH)
						* (fileHead.gljNum + fileHead.wqjNum + fileHead.bajNum)
				+ sizeof(NET_INTERFACE) * fileHead.subnetNum;
		for (zoneIndex = 0; zoneIndex < fileHead.zoneNum; zoneIndex++) {

			if (zoneHead[zoneIndex].gljNum) {
				r = fseek(fp, len, SEEK_SET);
				if (r < 0) {
					LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
					return -1;
				}

				i = zoneHead[zoneIndex].gljNum * sizeof(GLJ_HASH);
				r = fread(buf, 1, i, fp);
				if (r < i) {
					LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
					return -1;
				}

				for (i = 0; i < zoneHead[zoneIndex].gljNum; i++) {
					if (gljHash[i].No == roomAddr->devno) {
						if (ipAddr)
							*ipAddr = gljHash[i].ipaddr;
						if (ipMask)
							*ipMask = gljHash[i].ipmask;
						roomAddr->zone = zoneHead[zoneIndex].zoneNo;
						return 0;
					}
				}
			}

			len += (zoneHead[zoneIndex].gljNum + zoneHead[zoneIndex].wqjNum
					+ zoneHead[zoneIndex].bajNum) * sizeof(GLJ_HASH)
					+ zoneHead[zoneIndex].buildNum * sizeof(NETCONFIG_BUILD);
		}
		return -1;

	case X6B_WQJ:
		if (fileHead.wqjNum) {
			for (i = fileHead.gljNum; i < (fileHead.gljNum + fileHead.wqjNum);
					i++) {
				if (globalGlj[i].No == roomAddr->devno) {
					if (ipAddr)
						*ipAddr = globalGlj[i].ipaddr;
					if (ipMask)
						*ipMask = globalGlj[i].ipmask;
					roomAddr->zone = 0;
					return 0;
				}
			}
		}
		//len计算分区头部到文件起始的位置偏移
		len = sizeof(NETCONFIG_FILE_HEAD)
				+ sizeof(NETCONFIG_ZONE_HEAD) * fileHead.zoneNum
				+ sizeof(GLJ_HASH)
						* (fileHead.gljNum + fileHead.wqjNum + fileHead.bajNum)
				+ sizeof(NET_INTERFACE) * fileHead.subnetNum;
		for (zoneIndex = 0; zoneIndex < fileHead.zoneNum; zoneIndex++) {

			if (zoneHead[zoneIndex].wqjNum) {
				i = len + zoneHead[zoneIndex].gljNum * sizeof(GLJ_HASH);
				r = fseek(fp, i, SEEK_SET);
				if (r < 0) {
					LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
					return -1;
				}

				i = zoneHead[zoneIndex].wqjNum * sizeof(GLJ_HASH);
				r = fread(buf, 1, i, fp);
				if (r < i) {
					LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
					return -1;
				}

				for (i = 0; i < zoneHead[zoneIndex].wqjNum; i++) {
					if (gljHash[i].No == roomAddr->devno) {
						if (ipAddr)
							*ipAddr = gljHash[i].ipaddr;
						if (ipMask)
							*ipMask = gljHash[i].ipmask;
						roomAddr->zone = zoneHead[zoneIndex].zoneNo;
						return 0;
					}
				}
			}

			len += (zoneHead[zoneIndex].gljNum + zoneHead[zoneIndex].wqjNum
					+ zoneHead[zoneIndex].bajNum) * sizeof(GLJ_HASH)
					+ zoneHead[zoneIndex].buildNum * sizeof(NETCONFIG_BUILD);
		}
		return -1;

	case X6B_BAJ:
		if (fileHead.bajNum) {
			for (i = (fileHead.gljNum + fileHead.wqjNum);
					i < (fileHead.gljNum + fileHead.wqjNum + fileHead.bajNum);
					i++) {
				if (globalGlj[i].No == roomAddr->devno) {
					if (ipAddr)
						*ipAddr = globalGlj[i].ipaddr;
					if (ipMask)
						*ipMask = globalGlj[i].ipmask;
					roomAddr->zone = 0;
					return 0;
				}
			}
		}
		//len计算分区头部到文件起始的位置偏移
		len = sizeof(NETCONFIG_FILE_HEAD)
				+ sizeof(NETCONFIG_ZONE_HEAD) * fileHead.zoneNum
				+ sizeof(GLJ_HASH)
						* (fileHead.gljNum + fileHead.wqjNum + fileHead.bajNum)
				+ sizeof(NET_INTERFACE) * fileHead.subnetNum;
		for (zoneIndex = 0; zoneIndex < fileHead.zoneNum; zoneIndex++) {

			if (zoneHead[zoneIndex].bajNum) {
				i = len
						+ (zoneHead[zoneIndex].gljNum
								+ zoneHead[zoneIndex].wqjNum)
								* sizeof(GLJ_HASH);
				r = fseek(fp, i, SEEK_SET);
				if (r < 0) {
					LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
					return -1;
				}

				i = zoneHead[zoneIndex].bajNum * sizeof(GLJ_HASH);
				r = fread(buf, 1, i, fp);
				if (r < i) {
					LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
					return -1;
				}

				for (i = 0; i < zoneHead[zoneIndex].bajNum; i++) {
					if (gljHash[i].No == roomAddr->devno) {
						if (ipAddr)
							*ipAddr = gljHash[i].ipaddr;
						if (ipMask)
							*ipMask = gljHash[i].ipmask;
						roomAddr->zone = zoneHead[zoneIndex].zoneNo;
						return 0;
					}
				}
			}

			len += (zoneHead[zoneIndex].gljNum + zoneHead[zoneIndex].wqjNum
					+ zoneHead[zoneIndex].bajNum) * sizeof(GLJ_HASH)
					+ zoneHead[zoneIndex].buildNum * sizeof(NETCONFIG_BUILD);
		}
		return -1;
	}
	LOGE("设备类别不是管理机、围墙机或保安分机 %s : %d\n", __FUNCTION__, __LINE__);
	return -1;
}

//2014.04.19增加如下接口

/*
 获取指定序号子网的结构，将其赋值到subNet
 subNetIndex:从1开始
 返回 0 成功，其它 失败
 */
int getSubnetInterface(int subNetIndex, NET_INTERFACE *subNet) {
	if (fp == NULL)
		return -1;
	if (subNet == NULL)
		return -1;

	if ((subNetIndex > 0) && (subNetIndex <= fileHead.subnetNum)) {
		memcpy((char*) subNet, (char*) &subnet[subNetIndex - 1],
				sizeof(NET_INTERFACE));
		return 0;
	}
	LOGE("无此子网 %s : %d\n", __FUNCTION__, __LINE__);
	return -1;
}

/*
 获取指定房号地址对应设备的设备序列号
 返回 0 成功，其它 失败
 */
int getRoomSequence(T_Room *roomAddr, uint32_t *sequence) {
	int zoneIndex, len, r;
	char buf[100 * sizeof(NETCONFIG_BUILD)]; //须保证能装下100个楼栋结构的数据
	GLJ_HASH *gljHash = (GLJ_HASH *) buf; //管理机、围墙机，保安分机都使用这个变量
	NETCONFIG_BUILD *buildHash = (NETCONFIG_BUILD *) buf;
	int index;

	if (fp == NULL)
		return -1;

	//分区为0的设备只能是管理机、围墙机、保安分机，它们是全局的，不属于某个分区
	if (roomAddr->zone == 0) {
		switch (roomAddr->type) {
		case X6B_GLJ:
			if (fileHead.gljNum == 0) {
				LOGE("管理机未登记 %s : %d\n", __FUNCTION__, __LINE__);
				return -1;
			}
			for (index = 0; index < fileHead.gljNum; index++) {
				if (globalGlj[index].No == roomAddr->devno) {
					if (sequence)
						*sequence = globalGlj[index].sequence;
					return 0;
				}
			}
			LOGE("管理机未登记 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;

		case X6B_WQJ:
			if (fileHead.wqjNum == 0) {
				LOGE("围墙机未登记 %s : %d\n", __FUNCTION__, __LINE__);
				return -1;
			}
			for (index = fileHead.gljNum;
					index < (fileHead.gljNum + fileHead.wqjNum); index++) {
				if (globalGlj[index].No == roomAddr->devno) {
					if (sequence)
						*sequence = globalGlj[index].sequence;
					return 0;
				}
			}
			LOGE("围墙机未登记 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;

		case X6B_BAJ:
			if (fileHead.bajNum == 0) {
				LOGE("保安分机未登记 %s : %d\n", __FUNCTION__, __LINE__);
				return -1;
			}
			for (index = (fileHead.gljNum + fileHead.wqjNum);
					index
							< (fileHead.gljNum + fileHead.wqjNum
									+ fileHead.bajNum); index++) {
				if (globalGlj[index].No == roomAddr->devno) {
					if (sequence)
						*sequence = globalGlj[index].sequence;
					return 0;
				}
			}
			LOGE("保安分机未登记 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}
		LOGE("设备类别不是管理机、围墙机或保安分机 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	//len计算指定分区头部到文件起始的位置偏移
	len = sizeof(NETCONFIG_FILE_HEAD)
			+ sizeof(NETCONFIG_ZONE_HEAD) * fileHead.zoneNum
			+ sizeof(GLJ_HASH)
					* (fileHead.gljNum + fileHead.wqjNum + fileHead.bajNum)
			+ sizeof(NET_INTERFACE) * fileHead.subnetNum;
	for (zoneIndex = 0; zoneIndex < fileHead.zoneNum; zoneIndex++) {
		if (zoneHead[zoneIndex].zoneNo == roomAddr->zone)
			break;
		len += (zoneHead[zoneIndex].gljNum + zoneHead[zoneIndex].wqjNum
				+ zoneHead[zoneIndex].bajNum) * sizeof(GLJ_HASH)
				+ zoneHead[zoneIndex].buildNum * sizeof(NETCONFIG_BUILD);
	}
	if (zoneIndex >= fileHead.zoneNum) {
		LOGE("分区号未登记\n");
		return -1;
	}

	switch (roomAddr->type) {

	case X6B_GLJ:
		if (zoneHead[zoneIndex].gljNum == 0) {
			LOGE("管理机未登记 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		r = fseek(fp, len, SEEK_SET);
		if (r < 0) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		len = zoneHead[zoneIndex].gljNum * sizeof(GLJ_HASH);
		r = fread(buf, 1, len, fp);
		if (r < len) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		for (index = 0; index < zoneHead[zoneIndex].gljNum; index++) {
			if (gljHash[index].No == roomAddr->devno) {
				if (sequence)
					*sequence = globalGlj[index].sequence;
				return 0;
			}
		}
		LOGE("管理机未登记 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;

	case X6B_WQJ:
		if (zoneHead[zoneIndex].wqjNum == 0) {
			LOGE("围墙机未登记 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		if (zoneHead[zoneIndex].gljNum)
			len += zoneHead[zoneIndex].gljNum * sizeof(GLJ_HASH);
		r = fseek(fp, len, SEEK_SET);
		if (r < 0) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		len = zoneHead[zoneIndex].wqjNum * sizeof(GLJ_HASH);
		r = fread(buf, 1, len, fp);
		if (r < len) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		for (index = 0; index < zoneHead[zoneIndex].wqjNum; index++) {
			if (gljHash[index].No == roomAddr->devno) {
				if (sequence)
					*sequence = globalGlj[index].sequence;
				return 0;
			}
		}
		LOGE("围墙机未登记 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;

	case X6B_BAJ:
		if (zoneHead[zoneIndex].bajNum == 0) {
			LOGE("保安分机未登记 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		if (zoneHead[zoneIndex].gljNum)
			len += zoneHead[zoneIndex].gljNum * sizeof(GLJ_HASH);
		if (zoneHead[zoneIndex].wqjNum)
			len += zoneHead[zoneIndex].wqjNum * sizeof(GLJ_HASH);
		r = fseek(fp, len, SEEK_SET);
		if (r < 0) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		len = zoneHead[zoneIndex].bajNum * sizeof(GLJ_HASH);
		r = fread(buf, 1, len, fp);
		if (r < len) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		for (index = 0; index < zoneHead[zoneIndex].bajNum; index++) {
			if (gljHash[index].No == roomAddr->devno) {
				if (sequence)
					*sequence = globalGlj[index].sequence;
				return 0;
			}
		}
		LOGE("保安分机未登记 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;

	case X6B_ZJ:
		if (zoneHead[zoneIndex].buildNum == 0) {
			LOGE("门口主机未登记 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		if (zoneHead[zoneIndex].gljNum)
			len += zoneHead[zoneIndex].gljNum * sizeof(GLJ_HASH);
		if (zoneHead[zoneIndex].wqjNum)
			len += zoneHead[zoneIndex].wqjNum * sizeof(GLJ_HASH);
		if (zoneHead[zoneIndex].bajNum)
			len += zoneHead[zoneIndex].bajNum * sizeof(GLJ_HASH);
		r = fseek(fp, len, SEEK_SET);
		if (r < 0) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		len = zoneHead[zoneIndex].buildNum * sizeof(NETCONFIG_BUILD);
		r = fread(buf, 1, len, fp);
		if (r < len) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		for (index = 0; index < zoneHead[zoneIndex].buildNum;
				index++, buildHash++) {
			if ((buildHash->buildNo == roomAddr->build) &&

			(buildHash->startUnitNo <= roomAddr->unit)
					&& ((buildHash->startUnitNo + buildHash->unitNum)
							> roomAddr->unit) &&

					(buildHash->zjNumPerUnit >= roomAddr->devno)) {
#if DEBUG_NETCONFIG
				LOGE("roomAddr: %d-%d-%d-%d-%d-%d\n", roomAddr->zone, roomAddr->build,
						roomAddr->unit, roomAddr->floor, roomAddr->room, roomAddr->devno);
				LOGE( "sequenceStart:%d\n", buildHash->sequenceStart );
#endif
				if (sequence) {
//2014.09.23 增加电梯模块
					int unitHost = getUnitHosts(buildHash);

					*sequence = buildHash->sequenceStart
							+ (roomAddr->unit - buildHash->startUnitNo)
									* unitHost + roomAddr->devno - 1; //设备号是从1开始
				}
				return 0;
			}
		}
		LOGE("门口主机未登记 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;

	case X6B_QRJ:
		if (zoneHead[zoneIndex].buildNum == 0) {
			LOGE("确认机未登记 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		if (zoneHead[zoneIndex].gljNum)
			len += zoneHead[zoneIndex].gljNum * sizeof(GLJ_HASH);
		if (zoneHead[zoneIndex].wqjNum)
			len += zoneHead[zoneIndex].wqjNum * sizeof(GLJ_HASH);
		if (zoneHead[zoneIndex].bajNum)
			len += zoneHead[zoneIndex].bajNum * sizeof(GLJ_HASH);
		r = fseek(fp, len, SEEK_SET);
		if (r < 0) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		len = zoneHead[zoneIndex].buildNum * sizeof(NETCONFIG_BUILD);
		r = fread(buf, 1, len, fp);
		if (r < len) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		for (index = 0; index < zoneHead[zoneIndex].buildNum;
				index++, buildHash++) {
			if ((buildHash->buildNo == roomAddr->build) &&

			(buildHash->startUnitNo <= roomAddr->unit)
					&& ((buildHash->startUnitNo + buildHash->unitNum)
							> roomAddr->unit) &&

					(buildHash->startFloorNo <= roomAddr->floor)
					&& ((buildHash->startFloorNo + buildHash->floorNum)
							> roomAddr->floor) &&

					(buildHash->startRoomNo <= roomAddr->room)
					&& ((buildHash->startRoomNo + buildHash->roomNumPerFloor)
							> roomAddr->room) &&

					(buildHash->qrjNumPerRoom >= roomAddr->devno)) {
#if DEBUG_NETCONFIG
				LOGE("roomAddr: %d-%d-%d-%d-%d-%d\n", roomAddr->zone, roomAddr->build,
						roomAddr->unit, roomAddr->floor, roomAddr->room, roomAddr->devno);
				LOGE( "sequenceStart:%d\n", buildHash->sequenceStart );
#endif
				if (sequence) {
					int roomHost = buildHash->qrjNumPerRoom
							+ buildHash->deviceNumPerRoom;
					int floorHost = roomHost * buildHash->roomNumPerFloor;
//2014.09.23 增加电梯模块
					int unitHost = getUnitHosts(buildHash);

					*sequence = buildHash->sequenceStart
							+ (roomAddr->unit - buildHash->startUnitNo)
									* unitHost + buildHash->zjNumPerUnit
							+ (roomAddr->floor - buildHash->startFloorNo)
									* floorHost
							+ (roomAddr->room - buildHash->startRoomNo)
									* roomHost + roomAddr->devno - 1; //设备号是从1开始
				}
				return 0;
			}
		}
		LOGE("确认机未登记 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;

	case X6B_FJ:
		if (zoneHead[zoneIndex].buildNum == 0) {
			LOGE("室内分机未登记 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		if (zoneHead[zoneIndex].gljNum)
			len += zoneHead[zoneIndex].gljNum * sizeof(GLJ_HASH);
		if (zoneHead[zoneIndex].wqjNum)
			len += zoneHead[zoneIndex].wqjNum * sizeof(GLJ_HASH);
		if (zoneHead[zoneIndex].bajNum)
			len += zoneHead[zoneIndex].bajNum * sizeof(GLJ_HASH);
		r = fseek(fp, len, SEEK_SET);
		if (r < 0) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		len = zoneHead[zoneIndex].buildNum * sizeof(NETCONFIG_BUILD);
		r = fread(buf, 1, len, fp);
		if (r < len) {
			LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
			return -1;
		}

		for (index = 0; index < zoneHead[zoneIndex].buildNum;
				index++, buildHash++) {
			if ((buildHash->buildNo == roomAddr->build) &&

			(buildHash->startUnitNo <= roomAddr->unit)
					&& ((buildHash->startUnitNo + buildHash->unitNum)
							> roomAddr->unit) &&

					(buildHash->startFloorNo <= roomAddr->floor)
					&& ((buildHash->startFloorNo + buildHash->floorNum)
							> roomAddr->floor) &&

					(buildHash->startRoomNo <= roomAddr->room)
					&& ((buildHash->startRoomNo + buildHash->roomNumPerFloor)
							> roomAddr->room) &&

					(buildHash->deviceNumPerRoom >= roomAddr->devno)) {
#if DEBUG_NETCONFIG
				LOGE("roomAddr: %d-%d-%d-%d-%d-%d\n", roomAddr->zone, roomAddr->build,
						roomAddr->unit, roomAddr->floor, roomAddr->room, roomAddr->devno);
				LOGE( "sequenceStart:%d\n", buildHash->sequenceStart );
#endif
				if (sequence) {
					int roomHost = buildHash->qrjNumPerRoom
							+ buildHash->deviceNumPerRoom;
					int floorHost = roomHost * buildHash->roomNumPerFloor;
//2014.09.23 增加电梯模块
					int unitHost = getUnitHosts(buildHash);

					*sequence = buildHash->sequenceStart
							+ (roomAddr->unit - buildHash->startUnitNo)
									* unitHost + buildHash->zjNumPerUnit
							+ (roomAddr->floor - buildHash->startFloorNo)
									* floorHost
							+ (roomAddr->room - buildHash->startRoomNo)
									* roomHost + buildHash->qrjNumPerRoom
							+ roomAddr->devno - 1; //设备号是从1开始
				}
				return 0;
			}
		}
		LOGE("室内分机未登记 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;

	case X6B_BSZJ:
		return -1;
	}

	LOGE("设备类别未登记 %s : %d\n", __FUNCTION__, __LINE__);
	return -1;
}

//2014.09.23 增加电梯模块

/*
 检查指定房号地址所在的楼栋是否配置有电梯模块，如果有，返回指定单元的电梯模块的网络接口
 返回 0 成功，其它 失败
 */
int getElevatorInterface(T_Room *roomAddr, uint32_t *ipAddr, uint32_t *ipMask,
		uint32_t *gw) {
	int zoneIndex, len, r;
	char buf[100 * sizeof(NETCONFIG_BUILD)]; //须保证能装下100个楼栋结构的数据
	NETCONFIG_BUILD *buildHash = (NETCONFIG_BUILD *) buf;
	int index;

	if (fp == NULL)
		return -1;

	//len计算指定分区头部到文件起始的位置偏移
	len = sizeof(NETCONFIG_FILE_HEAD)
			+ sizeof(NETCONFIG_ZONE_HEAD) * fileHead.zoneNum
			+ sizeof(GLJ_HASH)
					* (fileHead.gljNum + fileHead.wqjNum + fileHead.bajNum)
			+ sizeof(NET_INTERFACE) * fileHead.subnetNum;
	LOGE("本机是%d\n", roomAddr->zone);
	LOGE("登记数量%d\n", fileHead.zoneNum);
	for (zoneIndex = 0; zoneIndex < fileHead.zoneNum; zoneIndex++) {
		if (zoneHead[zoneIndex].zoneNo == roomAddr->zone)
			break;
		len += (zoneHead[zoneIndex].gljNum + zoneHead[zoneIndex].wqjNum
				+ zoneHead[zoneIndex].bajNum) * sizeof(GLJ_HASH)
				+ zoneHead[zoneIndex].buildNum * sizeof(NETCONFIG_BUILD);
		LOGE("已经登记的有%d\n", zoneHead[zoneIndex].zoneNo);
	}
	if (zoneIndex >= fileHead.zoneNum) {
		LOGE("分区号未登记\n");
		return -1;
	}

	if (zoneHead[zoneIndex].buildNum == 0) {
		LOGE("指定的房号地址错误 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (zoneHead[zoneIndex].gljNum)
		len += zoneHead[zoneIndex].gljNum * sizeof(GLJ_HASH);
	if (zoneHead[zoneIndex].wqjNum)
		len += zoneHead[zoneIndex].wqjNum * sizeof(GLJ_HASH);
	if (zoneHead[zoneIndex].bajNum)
		len += zoneHead[zoneIndex].bajNum * sizeof(GLJ_HASH);
	r = fseek(fp, len, SEEK_SET);
	if (r < 0) {
		LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	len = zoneHead[zoneIndex].buildNum * sizeof(NETCONFIG_BUILD);
	r = fread(buf, 1, len, fp);
	if (r < len) {
		LOGE("网络配置文件错误 %s : %d\n", __FUNCTION__, __LINE__);
		return -1;
	}

	for (index = 0; index < zoneHead[zoneIndex].buildNum;
			index++, buildHash++) {

		if ((buildHash->buildNo == roomAddr->build)
				&& (buildHash->startUnitNo <= roomAddr->unit)
				&& ((buildHash->startUnitNo + buildHash->unitNum)
						> roomAddr->unit)) {


			if (buildHash->elevator_module) {
				if (ipAddr) {
					int unitHost = getUnitHosts(buildHash);

					uint32_t ip;
					ip = ntohl( buildHash->ipaddrStart )
							+ (roomAddr->unit - buildHash->startUnitNo)
									* unitHost + unitHost - 1;
					*ipAddr = htonl(ip);
				}
				if (ipMask)
					*ipMask = buildHash->ipmask;
				if (gw)
					*gw = buildHash->gw;

				return 0;
			}

		}

	}

	LOGE("指定的房号地址错误 %s : %d\n", __FUNCTION__, __LINE__);
	return -1;
}

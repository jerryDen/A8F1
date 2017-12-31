/*
 * Utils.cpp
 *
 *  Created on: 2015-1-26
 *      Author: Jerry
 */

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <sys/utsname.h>
#include <limits.h>
#include <ctype.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <net/route.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

#include "Utils.h"
#include "DebugLog.h"

#define DEBUGLEVEL 7

#define ETHTOOL_GLINK  0x0000000a /* Get link status (ethtool_value) */

struct ethtool_value {
	unsigned int cmd;
	unsigned int data;
};

//控制通讯须带效验
static unsigned char CRC8_TABLE[] = { 0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65, 157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130,
		220, 35, 130, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98, 190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255, 70, 24, 250, 164, 39,
		121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7, 219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154, 101, 59, 217, 135, 4, 90, 184, 230, 167, 249,
		27, 69, 198, 152, 122, 36, 248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185, 140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147,
		205, 17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80, 175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238, 50, 108, 142, 208,
		83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115, 202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139, 87, 9, 235, 181, 54, 104, 138, 212, 149,
		203, 41, 119, 244, 170, 72, 22, 233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168, 116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137,
		107, 53 };

//////////单字节数据CRC8计算/////////////////////
unsigned char ByteCrc8(unsigned char Crc8Out, unsigned char Crc8in) {
	return CRC8_TABLE[Crc8in ^ Crc8Out];
}

////////////多字节数据CRC8计算////////////////////
unsigned char NByteCrc8(unsigned char crc_in, unsigned char *DatAdr, unsigned int DatLn) {
	unsigned char Crc8Out;

	Crc8Out = crc_in;
	while (DatLn != 0) {
		Crc8Out = CRC8_TABLE[*DatAdr ^ Crc8Out];
		DatAdr++;
		DatLn--;
	}
	return Crc8Out;
}

int get_netlink_status(const char *if_name) {
	int skfd;
	struct ifreq ifr;
	struct ethtool_value edata;

	edata.cmd = ETHTOOL_GLINK;
	edata.data = 0;

	memset(&ifr, 0, sizeof(ifr));
	strncpy(ifr.ifr_name, if_name, sizeof(ifr.ifr_name) - 1);

	ifr.ifr_data = (char *) &edata;

	if ((skfd = socket(AF_INET, SOCK_DGRAM, 0)) == 0)
		return -1;
	if (ioctl(skfd, SIOCETHTOOL, &ifr) == -1) {
		close(skfd);
		return -1;
	}
	close(skfd);
	return edata.data;
}

int get_GateWay(const char* IfName) {
	const char *route = "/proc/net/route";
	FILE *gw;

	int count;
	count = 0;

	char gateway[8];

	if ((gw = fopen(route, "r")) == NULL) {
		LOGI("Can't open %s,may be file doesn't exist or unaccess\n", route);
		return -1;
	}

	char line[1024];
	while (fgets(line, sizeof(line), gw) != NULL) {

		if (strstr(line, IfName) != NULL && strstr(line, "0003") != NULL) {
			count = 1;
			strncpy(gateway, strstr(line, "00000000") + 8 + 1, 8);

			line[3] = charToInt(gateway[0]) * 16 + charToInt(gateway[1]);
			line[2] = charToInt(gateway[2]) * 16 + charToInt(gateway[3]);
			line[1] = charToInt(gateway[4]) * 16 + charToInt(gateway[5]);
			line[0] = charToInt(gateway[6]) * 16 + charToInt(gateway[7]);
			LOGI("%s:获取网关成功:%s\n", IfName, inet_ntoa(*(struct in_addr* ) line));
		}
	}

	if (count == 0) {
		LOGD("%s:no default gateway exist!\n", IfName);
		fclose(gw);
		return -2;
	}

	fclose(gw);

	return 0;
}

int getNetIf(const char * IfName, T_NetInterface * p_net_if) {
	char strbuf[50];
	struct ifreq ifr;
	struct sockaddr_in *sin;

	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, IfName);

	int fd = 0;
	if ((fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
#if DEBUGLEVEL > 4
		LOGE("%s:获取socket失败！\n", IfName);
#endif
		return -1;
	}

	if (ioctl(fd, SIOCGIFADDR, &ifr) < 0) {
#if DEBUGLEVEL > 1
		LOGE("%s:获取IP地址失败-->%s\n", IfName, strerror(errno));
#endif
	} else {
		sin = (struct sockaddr_in *) &ifr.ifr_addr;
		strcpy(strbuf, inet_ntoa(sin->sin_addr));
#if DEBUGLEVEL > 1
		LOGI("%s:获取IP地址成功:%s\n", IfName, strbuf);
#endif
		p_net_if->ipaddr = sin->sin_addr.s_addr;
	}

	if (ioctl(fd, SIOCGIFNETMASK, &ifr) < 0) {
#if DEBUGLEVEL > 1
		LOGE("%s:获取网络掩码失败-->%s\n", IfName, strerror(errno));
#endif
	} else {
		sin = (struct sockaddr_in *) &ifr.ifr_netmask;
		strcpy(strbuf, inet_ntoa(sin->sin_addr));
#if DEBUGLEVEL > 1
		LOGI("%s:获取网络掩码成功:%s\n", IfName, strbuf);
		p_net_if->netmask = sin->sin_addr.s_addr;
#endif
	}

	if (ioctl(fd, SIOCGIFBRDADDR, &ifr) < 0) {
#if DEBUGLEVEL > 1
		LOGE("%s:获取广播地址失败-->%s\n", IfName, strerror(errno));
#endif
	} else {
		sin = (struct sockaddr_in *) &ifr.ifr_broadaddr;
		strcpy(strbuf, inet_ntoa(sin->sin_addr));
#if DEBUGLEVEL > 1
		LOGI("%s:获取广播地址成功:%s\n", IfName, strbuf);
#endif
		p_net_if->broadaddr = sin->sin_addr.s_addr;
	}

	if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
#if DEBUGLEVEL > 4
		LOGE("%s:获取MAC地址失败-->%s\n", IfName, strerror(errno));
#endif
	} else {
		memcpy(p_net_if->hwaddr, ifr.ifr_hwaddr.sa_data, 6);
#if DEBUGLEVEL > 4
		LOGI("%s:获取MAC地址成功:%02x %02x %02x %02x %02x %02x\n", IfName, (unsigned char) ifr.ifr_hwaddr.sa_data[0], (unsigned char) ifr.ifr_hwaddr.sa_data[1],
				(unsigned char) ifr.ifr_hwaddr.sa_data[2], (unsigned char) ifr.ifr_hwaddr.sa_data[3], (unsigned char) ifr.ifr_hwaddr.sa_data[4],
				(unsigned char) ifr.ifr_hwaddr.sa_data[5]);
#endif
	}

	get_GateWay(IfName);

	return 0;
}

int setNetIf(const char * IfName, T_NetInterface * p_net_if) {
	int fd = 0;
	fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0) {
		LOGE("%s:获取socket失败！\n", IfName);
		return -1;
	}

	struct ifreq ifr;
	struct sockaddr_in *sin;
	struct rtentry rm;

	memset(&ifr, 0, sizeof(ifr));
	strcpy(ifr.ifr_name, IfName);

	sin = (struct sockaddr_in *) &(ifr.ifr_addr);
	sin->sin_family = AF_INET;

	sin->sin_addr.s_addr = p_net_if->ipaddr;
	if (ioctl(fd, SIOCSIFADDR, &ifr) < 0) {
#if DEBUGLEVEL > 4
		LOGE("%s:设置IP地址%s失败\n", IfName,inet_ntoa(sin->sin_addr));
#endif
	}

	sin->sin_addr.s_addr = p_net_if->netmask;
	if (ioctl(fd, SIOCSIFNETMASK, &ifr) < 0) {
#if DEBUGLEVEL > 4
		LOGE("%s:设置网络掩码失败\n", IfName);
#endif
	}

	//设置MAC地址先要关掉网络接口
	ioctl(fd, SIOCGIFFLAGS, &ifr);
	ifr.ifr_flags &= ~IFF_UP;
	ioctl(fd, SIOCSIFFLAGS, &ifr);

	ioctl(fd, SIOCGIFHWADDR, &ifr);
	memcpy(ifr.ifr_hwaddr.sa_data, p_net_if->hwaddr, 6);
	if (ioctl(fd, SIOCSIFHWADDR, &ifr) < 0) {
#if DEBUGLEVEL >5
		LOGE("%s:设置MAC地址失败\n", IfName);
#endif
	}
	ioctl(fd, SIOCGIFFLAGS, &ifr);
	ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
	if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
		LOGE("%s:启动网络失败\n", IfName);
	} else {
		LOGI("%s:启动网络成功\n", IfName);
	}

	if ((p_net_if->gwaddr ^ p_net_if->ipaddr) & p_net_if->netmask) {
		LOGE("%s:网关和IP地址不在一个子网\n", IfName);
	}

	memset(&rm, 0, sizeof(rm));

	sin = (struct sockaddr_in *) &(rm.rt_dst);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = 0;
	sin->sin_port = 0;

	sin = (struct sockaddr_in *) &(rm.rt_genmask);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = 0;
	sin->sin_port = 0;

	sin = (struct sockaddr_in *) &(rm.rt_gateway);
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = p_net_if->gwaddr;
	sin->sin_port = 0;

	rm.rt_flags = RTF_UP | RTF_GATEWAY;

	if (ioctl(fd, SIOCADDRT, &rm) < 0) {
#if DEBUGLEVEL > 4
		LOGE("%s:设置网关失败!\n", IfName);
#endif
	}

//	system("ifconfig lo up");
//    system("mount -t nfs -o nolock 192.168.1.83:/work/nfs_root /nfs");
	return 0;
}

void YUYVToNV12(const void* yuyv, void *nv12, int width, int height) {
	int i, j;
	uint8_t* Y = (uint8_t*) nv12;
	uint8_t* UV = (uint8_t*) Y + width * height;

	for (i = 0; i < height; i += 2) {
		for (j = 0; j < width; j++) {
			*(uint8_t*) ((uint8_t*) Y + i * width + j) = *(uint8_t*) ((uint8_t*) yuyv + i * width * 2 + j * 2);
			*(uint8_t*) ((uint8_t*) Y + (i + 1) * width + j) = *(uint8_t*) ((uint8_t*) yuyv + (i + 1) * width * 2 + j * 2);
			*(uint8_t*) ((uint8_t*) UV + ((i * width) >> 1) + j) = *(uint8_t*) ((uint8_t*) yuyv + i * width * 2 + j * 2 + 1);
		}
	}
}

void YUYVToNV21(const void* yuyv, void *nv21, int width, int height) {
	int i, j;
	uint8_t* Y = (uint8_t*) nv21;
	uint8_t* VU = (uint8_t*) Y + width * height;

	for (i = 0; i < height; i += 2) {
		for (j = 0; j < width; j++) {
			*(uint8_t*) ((uint8_t*) Y + i * width + j) = *(uint8_t*) ((uint8_t*) yuyv + i * width * 2 + j * 2);
			*(uint8_t*) ((uint8_t*) Y + (i + 1) * width + j) = *(uint8_t*) ((uint8_t*) yuyv + (i + 1) * width * 2 + j * 2);

			if (j % 2) {
				if (j < width - 1) {
					*(uint8_t*) ((uint8_t*) VU + ((i * width) >> 1) + j) = *(uint8_t*) ((uint8_t*) yuyv + i * width * 2 + (j + 1) * 2 + 1);
				}
			} else {
				if (j > 1) {
					*(uint8_t*) ((uint8_t*) VU + ((i * width) >> 1) + j) = *(uint8_t*) ((uint8_t*) yuyv + i * width * 2 + (j - 1) * 2 + 1);
				}
			}
		}
	}
}

int get_chip_id(char* pChipId) {
	return 0;
}

/*************************************************
 Function: char_to_int
 Description: 字符型转化为整型
 Input:  s :要转化字符
 Output:
 Return: 成功: 返回转化后整型数;
 失败:返回-1 ;
 Others:
 *************************************************/
int charToInt(char s) {
	if (s >= '0' && s <= '9') {
		return (s - '0');
	} else if (s >= 'A' && s <= 'F') {
		return (s - 'A' + 10);
	} else if (s >= 'a' && s <= 'f') {
		return (s - 'a' + 10);
	}

	return -1;
}

int checkMuiltcastAddr(uint32_t group_addr) {
	return ((group_addr & 0x000000ff) == 0x000000ef);
}


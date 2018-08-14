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


#include "roomNetConfig.h"
#include "localNetInfo.h"
#include "DebugLog.h"
#include "Utils.h"




T_NetInterface localNetInfo;
static uint32_t getMultiaddr(uint32_t ip);
static int checkMulticast(uint32_t ip);
static int setNetIf(const char * IfName, T_NetInterface * p_net_if);
static int getNetIf(const char * IfName, T_NetInterface * p_net_if) ;
static int get_GateWay(const char* IfName);





int setLocalNetInfo(T_Room localRoom)
{
	int res;
	uint32_t tempIP;
	uint32_t broadaddr;

	res = getRoomNetInterface(&localRoom,&localNetInfo.ipaddr,
			&localNetInfo.netmask,&localNetInfo.gwaddr);
	if (res != 0) {
		LOGE("getRoomNetInterface erro!");
		return -1;
	}
	//uint32_t tempIP = ntohl(ipAddr &ipMask);

	broadaddr = (localNetInfo.ipaddr& localNetInfo.gwaddr) | (~localNetInfo.gwaddr);
	tempIP = ntohl(localNetInfo.ipaddr);
	localNetInfo.hwaddr[0] = 0x00;
	localNetInfo.hwaddr[1] = 0x1a;
	localNetInfo.hwaddr[2] = 0xa0;
	localNetInfo.hwaddr[3] = (tempIP & 0x00FF0000) >> 16;
	localNetInfo.hwaddr[4] = (tempIP & 0x0000FF00) >> 8;
	localNetInfo.hwaddr[5] = (tempIP & 0x000000FF);
	//多播地址为1号分机的多播地址
	if(localRoom.devno == 1 ){
		localNetInfo.multiaddr = getMultiaddr(localNetInfo.ipaddr);
	}else{
		uint32_t ipaddr,netmask,gwaddr;
		localRoom.devno = 1;
		res = getRoomNetInterface(&localRoom,&ipaddr,
			&netmask,&gwaddr);
		localNetInfo.multiaddr = getMultiaddr(ipaddr);
	}
	setNetIf("eth0", &localNetInfo);
	getNetIf("eth0", &localNetInfo);

	return 0;
}

int getLocalNetInfo(T_NetInterface * netInfo)
{
	if(netInfo != NULL && localNetInfo.ipaddr != 0 ){
		*netInfo =  localNetInfo;
		return 0;
	}
	return -1;
}
int getLocalIpaddr(void)
{
	return 	localNetInfo.ipaddr;
}

//形参和返回值为网络字节序
static uint32_t getMultiaddr(uint32_t ip)
{
	uint32_t multiaddr;
	multiaddr = ntohl(ip);		
	return (htonl( (multiaddr & 0x00ffffff) | MULTICAST_GROUP));
}

//形参和返回值为网络字节序
static int checkMulticast(uint32_t ip)
{
	uint32_t multiaddr;
	multiaddr = ntohl(ip);		
	if( (multiaddr & 0xff000000) == MULTICAST_GROUP )
		return 0;
	return -1;
}
int joinMulticast( int fd,unsigned int multiaddr)
{
	struct ip_mreq mreq;								
	mreq.imr_multiaddr.s_addr = multiaddr;				
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);		
	if( setsockopt( fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq) ) < 0){
		LOGE("IP_ADD_MEMBERSHIP\n");
		return -1;
	}
	int on = 1;
	//设置允许发生广播
	if( setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0 ){
		LOGE("as_socket setsockopt SO_BROADCAST:");
		return  -1;
	}
	if(setsockopt(fd,IPPROTO_IP,IP_MULTICAST_IF,&mreq.imr_multiaddr,sizeof(mreq.imr_multiaddr)) < 0)
	{
		LOGE("IP_MULTICAST_IF\n");
		return -1;
	}
	int loop = 0;
	//禁止组播回传！
	if(setsockopt(fd,IPPROTO_IP,IP_MULTICAST_LOOP,&loop,sizeof(loop)) < 0)
	{
		LOGE("IP_MULTICAST_LOOP\n");
		return -1;
	}
	return 0;
}

int quitMulticast( int fd,unsigned int  multiaddr)
{
	struct ip_mreq mreq;             					
    mreq.imr_multiaddr.s_addr = multiaddr; 			
    mreq.imr_interface.s_addr = htonl(INADDR_ANY); 		
	if( setsockopt( fd, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
		return -1;

	return 0;
}

static int getNetIf(const char * IfName, T_NetInterface * p_net_if) {
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
		LOGI("%s:获取IP地址成功:%s\n", IfName, strbuf);
		p_net_if->ipaddr = sin->sin_addr.s_addr;
	}

	if (ioctl(fd, SIOCGIFNETMASK, &ifr) < 0) {
#if DEBUGLEVEL > 1
		LOGE("%s:获取网络掩码失败-->%s\n", IfName, strerror(errno));
#endif
	} else {
		sin = (struct sockaddr_in *) &ifr.ifr_netmask;
		strcpy(strbuf, inet_ntoa(sin->sin_addr));
		LOGI("%s:获取网络掩码成功:%s\n", IfName, strbuf);
		p_net_if->netmask = sin->sin_addr.s_addr;
	}

	if (ioctl(fd, SIOCGIFBRDADDR, &ifr) < 0) {
		LOGE("%s:获取广播地址失败-->%s\n", IfName, strerror(errno));
	} else {
		sin = (struct sockaddr_in *) &ifr.ifr_broadaddr;
		strcpy(strbuf, inet_ntoa(sin->sin_addr));
		LOGI("%s:获取广播地址成功:%s\n", IfName, strbuf);
		p_net_if->broadaddr = sin->sin_addr.s_addr;
	}

	if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
		LOGE("%s:获取MAC地址失败-->%s\n", IfName, strerror(errno));
	} else {
		memcpy(p_net_if->hwaddr, ifr.ifr_hwaddr.sa_data, 6);
		LOGI("%s:获取MAC地址成功:%02x %02x %02x %02x %02x %02x\n", IfName, (unsigned char) ifr.ifr_hwaddr.sa_data[0], (unsigned char) ifr.ifr_hwaddr.sa_data[1],
				(unsigned char) ifr.ifr_hwaddr.sa_data[2], (unsigned char) ifr.ifr_hwaddr.sa_data[3], (unsigned char) ifr.ifr_hwaddr.sa_data[4],
				(unsigned char) ifr.ifr_hwaddr.sa_data[5]);
	}

	get_GateWay(IfName);

	return 0;
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

			line[3] = getUtilsOps()->charToInt(gateway[0]) * 16 + getUtilsOps()->charToInt(gateway[1]);
			line[2] = getUtilsOps()->charToInt(gateway[2]) * 16 + getUtilsOps()->charToInt(gateway[3]);
			line[1] = getUtilsOps()->charToInt(gateway[4]) * 16 + getUtilsOps()->charToInt(gateway[5]);
			line[0] = getUtilsOps()->charToInt(gateway[6]) * 16 + getUtilsOps()->charToInt(gateway[7]);
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


static int setNetIf(const char * IfName, T_NetInterface * p_net_if) {
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


























#include "roomNetConfig.h"
#include "localNetInfo.h"
#include "DebugLog.h"
#include "Utils.h"
T_NetInterface localNetInfo;
static uint32_t getMultiaddr(uint32_t ip);
static int checkMulticast(uint32_t ip);


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

























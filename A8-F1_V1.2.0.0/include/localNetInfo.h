
#ifndef LOCAL_NET_INFO_H__
#define LOCAL_NET_INFO_H__


#include "udpServer.h"
#include "commStructDefine.h"



int setLocalNetInfo(T_Room localRoom);
int getLocalIpaddr(void);

int getLocalNetInfo(T_NetInterface * netInfo);
int joinMulticast( int fd,unsigned int multiaddr);
int quitMulticast( int fd,unsigned int  multiaddr);



#endif 

































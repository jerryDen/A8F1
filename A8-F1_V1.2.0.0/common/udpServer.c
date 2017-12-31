#include "udpServer.h"
#include "DebugLog.h"
#include "threadManage.h"
#include "commStructDefine.h"


static int socketBind(const int sockfd, const int port);
static int setFdState(int sockfd, E_FdState state);
static int findIndexFromQueue(int sockfd );
static int findSecondFd(void);



static void * RecvThread(void *);
static S_UdpRecv m_udpRecv;

	

int udp_serverInit(void)
{
	if(m_udpRecv.thPid != NULL )
		return 0;
	//m_udpRecv.tv.tv_sec = 0;
	//m_udpRecv.tv.tv_usec = 10 * 1000; 
	
	m_udpRecv.thPid = pthread_Create(RecvThread,NULL,0,NULL);
	if(m_udpRecv.thPid  == NULL)
		return -1;
	return 0;
}

int udp_getSocketFd(int bandPort)
{
	int udpSocketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (udpSocketfd <= 0) {
		LOGE("fail to socket !");
		return false;
	}
	fcntl(udpSocketfd, F_SETFL, O_NONBLOCK);
	if(socketBind( udpSocketfd,bandPort) == true)
		return udpSocketfd;	
	else
		return false;
}
int udp_sendData(int udp_sockfd, const unsigned char * data, int size,
		unsigned int  ipAddr, int port) {
	int ret;
	struct sockaddr_in remote_addr;
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = (port); //htons
	remote_addr.sin_addr.s_addr = ipAddr;
	bzero(&(remote_addr.sin_zero),sizeof(remote_addr.sin_zero));
	ret = sendto(udp_sockfd, data, size, 0, (struct sockaddr*) &remote_addr,
			(socklen_t) sizeof(struct sockaddr));

	if (ret == -1) {
		LOGE("writeDatagram  failed:%s\n", strerror(errno));
	}

	return ret;
}


int udp_addFdToRecvQueue(int fd,FUNCTION callBackfun){

	int i;
	if(m_udpRecv.fdNum > FuncMaxCount-1)
	{
		LOGE("Udp_addFdToRecvQueue Fail");
		return -1;
	}
	for(i = 0;i<FuncMaxCount ; i++)
	{
		if(m_udpRecv.handleFd[i].handleFunc == NULL )
		{
			m_udpRecv.handleFd[i].netDataPack.sockfd  = fd;
			m_udpRecv.handleFd[i].handleFunc = callBackfun;
			m_udpRecv.handleFd[i].fdState = FD_HANG_UP;//挂起状态
		
			m_udpRecv.fdNum ++;
			return 0;
		}	
	}
	return -1;
}
int  udp_deleteFdToRecvQueue(int fd)
{
	int index = findIndexFromQueue(fd);
	if( index >= 0){
		setFdState(fd,FD_HANG_UP);
		bzero(&m_udpRecv.handleFd[index],sizeof(S_HandleFd));
		return 0;
	}
	return -1;
}

static void addFd(void)
{
	int i;
	FD_ZERO(&m_udpRecv.fds); 
	for( i = 0; i < m_udpRecv.fdNum; i++)
	{		
		if(m_udpRecv.handleFd[i].fdState ==  FD_RUN){
			FD_SET(m_udpRecv.handleFd[i].netDataPack.sockfd,&m_udpRecv.fds); 
		}
	}
}
static S_HandleFd findHandleFd()
{
	int i;	
	for( i = 0; i < m_udpRecv.fdNum; i++)
	{		
		if(m_udpRecv.handleFd[i].fdState ==  FD_RUN){
			if(FD_ISSET(m_udpRecv.handleFd[i].netDataPack.sockfd, &m_udpRecv.fds))
				return m_udpRecv.handleFd[i];
		}
	}
}
int udp_startFdRecv(int sockfd)
{	
	if(setFdState(sockfd,FD_RUN) == 0 ){
		return pthread_start(m_udpRecv.thPid);
	}
	return -1;
}
int udp_stopFdRecv(int sockfd)
{
	return setFdState(sockfd,FD_HANG_UP);
}

static int  setFdState(int sockfd, E_FdState state)
{
	int index = findIndexFromQueue(sockfd);
	if(index >= 0){
		m_udpRecv.handleFd[index].fdState = state;

		if( sockfd  >= m_udpRecv.maxfdp)
		{
			if(state == FD_RUN)
			{
				m_udpRecv.maxfdp = sockfd;
			}else {
				if( sockfd == m_udpRecv.maxfdp)
					m_udpRecv.maxfdp = findSecondFd();
			}
		}
		return 0;
	}
	return -1;	
}
static int findSecondFd(void)
{
	int i;
	int max = m_udpRecv.maxfdp;
	int second = 0;
	for( i = 0; i < m_udpRecv.fdNum; i++)
	{
		
		if( m_udpRecv.handleFd[i].fdState == FD_RUN )
		{
			if((m_udpRecv.handleFd[i].netDataPack.sockfd < max) &&(m_udpRecv.handleFd[i].netDataPack.sockfd>second))
			{
				second = m_udpRecv.handleFd[i].netDataPack.sockfd;
			}
		}
	}
	return second;
}
static int findIndexFromQueue(int sockfd )
{
	int i;
	for( i = 0; i < m_udpRecv.fdNum; i++)
	{		
		if(m_udpRecv.handleFd[i].netDataPack.sockfd == sockfd){
			return i;
		}		
	}
	return -1;
}

void udp_serverExit(void)
{
	int i;
	pthread_destroy(m_udpRecv.thPid);
	
	for(i = 0;i<m_udpRecv.fdNum;i++)
	{
		close(m_udpRecv.handleFd[i].netDataPack.sockfd);
	}
	memset((void *)&m_udpRecv,0,sizeof(m_udpRecv));	
}

void * RecvThread(void * data){
	unsigned char *temp_buf ;
	int  * temp_len ;
	int * temp_offset;
	int recv_len;
	int userlen = 0;

	while(pthread_checkRunState(m_udpRecv.thPid) == Thread_Run)
	{			
			addFd();
			switch (select(m_udpRecv.maxfdp+1, &m_udpRecv.fds,
				NULL, NULL, NULL)) {
			 	case -1:
					LOGE("select:%s", strerror(errno));
					break;
				case 0:
					//m_udpRecv.tv.tv_sec = 0;
					//m_udpRecv.tv.tv_usec = 10 * 1000;
				
					break;
				default:
				{
					int sockaddrLen = sizeof(struct sockaddr);
					S_HandleFd handleFd = findHandleFd();
					
					temp_buf = handleFd.netDataPack.dataBody.buf;
					temp_len =  &(handleFd.netDataPack.dataBody.len);
					temp_offset = &(handleFd.netDataPack.dataBody.offset);
					
					int maxSize = sizeof(handleFd.netDataPack.dataBody.buf);
					memset((void*)temp_buf,0,maxSize);
					
					*temp_len= recvfrom(handleFd.netDataPack.sockfd,temp_buf + *temp_offset,maxSize, 0,
					(struct sockaddr*) &handleFd.netDataPack.remoteInfo, (socklen_t*)&sockaddrLen);
					*temp_offset += *temp_len;
					if(handleFd.handleFunc != NULL )
					{
						userlen = handleFd.handleFunc((void*)&handleFd.netDataPack);	
					}
					*temp_offset -= userlen;
				}			
			}
		
		
	} 

	return NULL;
}
static int socketBind(const int sockfd, const int port) {
	struct sockaddr_in m_addr;
	m_addr.sin_family = AF_INET;
	m_addr.sin_addr.s_addr = htons(INADDR_ANY);
	m_addr.sin_port = htons(port);
	bzero(&(m_addr.sin_zero), 8);

	int opt = SO_REUSEADDR;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	int bind_return = bind(sockfd, (struct sockaddr*) &m_addr, sizeof(m_addr));

	if (bind_return == -1) {
		LOGE("bind failed: %s\n", strerror(errno));
		return false;
	}
	return true;
}






















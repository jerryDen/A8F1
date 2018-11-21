#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/sockios.h>
#include <net/route.h>
#include "roomNetConfig.h"
#include "tftpUpdateServer.h"
#include "systemConfig.h"
#include "commonHead.h"
#define X6B_ADVER             "X6B_advertise"
#define UPDATE_PACK_HEAD_NAME "wbx6b_fj_update_04_"
#define UPDATE_PACK_TAIL_NAME ".tar.bz2.des3"

static int updateState = 0; 



static int getUpdateTimeMs(uint32_t size)
{
	int timeMs;
	int sequence = getLocalSequence();
	if(sequence  > 0 ){
		timeMs = ((1.0 + 50.0 * size / 1000000.0) * sequence);
		return 	timeMs;
	}
	return 0;
}
static int checkFileName(char *fileName)
{
	char tftp_version[64];
	char local_version[64];
	int packHeadLen = strlen(UPDATE_PACK_HEAD_NAME);
	int packTailLen = strlen(UPDATE_PACK_TAIL_NAME);
	int x6bAdLen = strlen(X6B_ADVER);
	memset(tftp_version,0,sizeof(tftp_version));
	memset(local_version,0,sizeof(local_version));
	size_t namelen = strlen(fileName);
		
	if( namelen >= 65 ){
		LOGE( "tftp file name too long\n" );
		goto fail0;
	}
	//A8-F1_update_V1-0-1-0.tar.bz2.des3
	if( strncmp(fileName, X6B_ADVER, x6bAdLen) != 0){

		if( strncmp(fileName, UPDATE_PACK_HEAD_NAME, packHeadLen) != 0){
			LOGE( "tftp file name error\n" );
			goto fail0;
		}
		if( strncmp(&fileName[namelen - packTailLen], UPDATE_PACK_TAIL_NAME, packTailLen) != 0){
			LOGE( "tftp file name error\n" );
			goto fail0;
		}
		memcpy(tftp_version, &fileName[packHeadLen], (namelen - packHeadLen - packTailLen) );
		
		tftp_version[namelen - packHeadLen - packTailLen] = 0;
		getAppVersion(local_version);
		LOGD("tftp_version:%s  local_version:%s\n",tftp_version,local_version);
		if( strcmp(tftp_version,local_version ) == 0 ){
			printf( "same version update file\n" );
			goto fail0;
		}
	}
	return 0;
fail0:
	return -1;
}

int startTftpUpdate(T_TFTP_UPDATE *updatePack)
{
	int ret;
	int pid;
	int timeMs;
	updatePack->size = ntohl(updatePack->size);
	timeMs = getUpdateTimeMs(updatePack->size);
	ret = checkFileName(updatePack->fileName);
	
	CHECK_RET(ret != 0, "fail to checkFileName", goto fail0);
	
	struct in_addr sin_addr;
	sin_addr.s_addr = updatePack->serverip;
	if(updateState == 1){
		LOGD("being updated......");
		return -1;
	}
	updateState  = 1;
	pid = fork();
	if(pid == 0)
	{
		while((timeMs -= 10)>0 ){  //12500
			
			usleep(20*1000);
		}
		LOGD("updatePack->fileName %s ip:%s\n",updatePack->fileName,inet_ntoa(sin_addr));
		if(execl( "/usr/bin/.tftp_update.sh", ".tftp_update.sh", \
									updatePack->fileName, inet_ntoa(sin_addr), NULL) < 0){
									exit(-1);
		}
		exit(0);
	}
	return 0;
fail0:
		return -1;
}








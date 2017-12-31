
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
#include "systemConfig.h"


#include "commandWord.h"
//#include "A8StructDefine.h"

#define	 CFG_LINE 1024
#define  WORK_DIR "/usr/work"
#define  MEDIA_DIR WORK_DIR"/wav"
#define  DEFAULT_VOLUME 80
#define configFile WORK_DIR"/system.conf"
#define netConfigFile WORK_DIR"/netConfig.bin"
#define appVersion  WORK_DIR"/.app_version"

static stSYS_CONFIG gSysConfig;


//NETCONFIG_FILE_HEAD *pGlobalZoneHead = NULL;
//NETCONFIG_ZONE_HEAD *pMyzone = NULL;
//static NETCONFIG_BUILD myBuild;
//NETCONFIG_BUILD *pMybuild = NULL;

unsigned char myZoneWqjNo[16];	//本区围墙机编号列表，-1表示无效编号
unsigned char globalWqjNo[16];	//全局围墙机编号列表，-1表示无效编号，全局围墙机区号为0

/*

char *WAV_lineBusy =  WAV_DIR"/lineBusy.wav";
char *WAV_lineFault = WAV_DIR"/lineFault.wav";
char *WAV_noAnswer = WAV_DIR"/noAnswer.wav";
char *WAV_operationFail = WAV_DIR"/operationError.wav";
char *WAV_operationSuccess = WAV_DIR"/operationSuccess.wav";
char *WAV_roomNoUnregister = WAV_DIR"/roomNoUnregister.wav";
char *WAV_talkTimeout =  WAV_DIR"/talkTimeout.wav";
//char *WAV_unhook = WAV_DIR"/unhook.wav";
char *WAV_wrongFormat = WAV_DIR"/wrongFormat.wav";
//char *WAV_wrongPassword = WAV_DIR"/wrongPassword.wav";
//char *WAV_cardExpire = WAV_DIR"/cardExpire.wav";
*/
/*
//注意:缴费相关语音没做英文的
char *WAV_paymentExpire = WAV_DIR"/paymentExpire.wav";
char *WAV_paymentExpire_0 = WAV_DIR"/paymentExpire_0.wav";
char *WAV_paymentExpire_1 = WAV_DIR"/paymentExpire_1.wav";
char *WAV_paymentExpire_2 = WAV_DIR"/paymentExpire_2.wav";
char *WAV_paymentExpire_3 = WAV_DIR"/paymentExpire_3.wav";
char *WAV_paymentExpire_4 = WAV_DIR"/paymentExpire_4.wav";
char *WAV_paymentExpire_5 = WAV_DIR"/paymentExpire_5.wav";
char *WAV_paymentExpire_6 = WAV_DIR"/paymentExpire_6.wav";
char *WAV_paymentExpire_7 = WAV_DIR"/paymentExpire_7.wav";
*/
/*
char *WAV_alarmDelay = WAV_DIR"/alarmDelay.wav";
char *WAV_alarmSetDelay = WAV_DIR"/alarmSetDelay.wav";
*/

uint32_t localMulticast = 0;




static int readLine(char* line, FILE* stream) 
{ 
	int i;
	char buf[CFG_LINE]; 
	
	//从文件中读取一行字符，并以0结尾
	if(fgets(buf, CFG_LINE, stream) != NULL){ 

		/* Delete the last '\r ' or '\n ' or ' ' or '\t ' character */   
		for(i = strlen(buf) - 1; i >= 0; i--){ 
			if(buf[i] == '\r' || buf[i] == '\n' || buf[i] == ' ' || buf[i] == '\t') 
				buf[i] = '\0'; 
			else 
				break;
		}

		/* Delete the front '\r ' or '\n ' or ' ' or '\t ' character */ 
		for(i = 0; (uint32_t)i <= strlen(buf); i++){ 
			if(buf[i] == '\r' || buf[i] == '\n' || buf[i] == ' ' || buf[i] == '\t') 
				continue; 
			else
				break;
		}

		strcpy(line, &buf[i]);

		return 0; 
	} 

	return -1;
} 


static int isRemark(const char* line) 
{ 
	unsigned int i; 

	for(i = 0; i < strlen(line); i++){ 

		if(isgraph(line[i])){
			if(line[i] == '#') return 1; 
			else return 0; 
		} 
	} 

	return 1; 
} 


static int getKey(const char* line, char* keyname, char* keyvalue) 
{ 
	int i, start; 

	/* Find '= ' character */ 
	for(start = 0; (uint32_t)start < strlen(line); start++){ 
		if(line[start] == '=') break; 
	} 
	if((uint32_t)start >= strlen(line)) return -1;

	memcpy(keyname, line, start); 
	keyname[start] = '\0'; 

	/* Delete the last '\t ' or  ' ' character */ 
	for(i = strlen(keyname) - 1; i >= 0; i--){ 
		if(keyname[i] == ' ' || keyname[i] == '\t') 
			keyname[i] = '\0'; 
		else break; 
	}

	/* Find key value */ 
	for(start = start + 1; (uint32_t)start < strlen(line); start++){ 
		if( line[start] != ' '  && line[start] != '\t')   
			break;
	} 

	strcpy(keyvalue, line + start);

	return 0;
} 


static int charToInt(char s)
{
	if(s >= '0' && s <= '9') return (s - '0');
	else if(s >= 'A' && s <= 'F') return (s - 'A' + 10);
	else if(s >= 'a' && s <= 'f') return (s - 'a' + 10);
	return -1;
}


void resetSystemConfig()
{
	//编解码参数
	gSysConfig.resolution = PT_VGA;
	gSysConfig.fps = 30;
	gSysConfig.bitRate = 1536 * 1024;
	gSysConfig.gop = 30;	//gop选择等于fps
	gSysConfig.pip = 0;

	//用户参数，默认是分机
	gSysConfig.roomAddr.type  = X6B_FJ;
	gSysConfig.roomAddr.zone  = 99;
	gSysConfig.roomAddr.build = 99;
	gSysConfig.roomAddr.unit  = 99;
	gSysConfig.roomAddr.floor = 99;
	gSysConfig.roomAddr.room  = 99;
	gSysConfig.roomAddr.devno = 99;

	gSysConfig.roomNum = 4;

	//工程密码
	memset( gSysConfig.systemPassword, 5, sizeof(gSysConfig.systemPassword) );
	//用户开门密码最大长度多余的部分必须置为0xff
//	memset( gSysConfig.userOpenPassword, 2, OPEN_PASSWORD_LEN );
//	memset( &gSysConfig.userOpenPassword[OPEN_PASSWORD_LEN], 0xff, 
//											USER_PASSWORD_MAX - OPEN_PASSWORD_LEN );
	#if 0
	memset( gSysConfig.userOpenPassword, 0xff, USER_PASSWORD_MAX );
	gSysConfig.userPasswordEnable = 0;

	gSysConfig.promptVol = DEFAULT_VOLUME;
	gSysConfig.ringVol	 = DEFAULT_VOLUME;
	gSysConfig.talkVol 	 = DEFAULT_VOLUME;

	gSysConfig.securityMode = AT_HOME;	//QD 固定AT_HOME安防模式 CLOSE_ALERT;
	memset( gSysConfig.securityPassword, 3, sizeof(gSysConfig.securityPassword) );

	gSysConfig.languageType  = CHINESE_SIMPLE;

	gSysConfig.serverip = 0;	//inet_addr("192.168.1.198");
	gSysConfig.sequence = -1;

	//屏保模式: 0 无屏幕保护， 1 黑屏， 2 时间， 3 广告
	gSysConfig.screenSaveMode = 2;
	#endif
	strcpy( gSysConfig.ringFile, MEDIA_DIR"/ring.wav");
}


//存储修改后的全局参数gSysConfig到配置文件中
int storageSystemConfig()
{
	FILE* stream;

	if((stream = fopen(configFile, "w")) == NULL){
		printf("建立系统配置文件失败 %s\n", configFile);
		return -1; 
	}

	fprintf(stream, "#the file is created by system auto(%s)\n", "2014.03.17");

	fprintf(stream, "RESOLUTION = %d\n", gSysConfig.resolution);
	fprintf(stream, "FPS = %d\n", gSysConfig.fps);
	fprintf(stream, "BIT_RATE = %u\n", gSysConfig.bitRate);
	fprintf(stream, "GOP = %d\n", gSysConfig.gop);
	fprintf(stream, "PIP = %d\n", gSysConfig.pip);

	fprintf(stream, "\n");

	fprintf(stream, "ROOM_ADDR = %01d-%02d-%02d-%02d-%02d-%02d-%02d\n",
									gSysConfig.roomAddr.type,
                                   	gSysConfig.roomAddr.zone, 
									gSysConfig.roomAddr.build,
									gSysConfig.roomAddr.unit,
									gSysConfig.roomAddr.floor,
									gSysConfig.roomAddr.room,
									gSysConfig.roomAddr.devno);

	fprintf(stream, "ROOM_NUM = %d\n", gSysConfig.roomNum);

	fprintf(stream, "\n");

#if SYSTEM_PASSWORD_LEN == 4
	fprintf(stream, "SYSTEM_PASSWORD = %x%x%x%x\n",
                                   	gSysConfig.systemPassword[0], 
									gSysConfig.systemPassword[1],
									gSysConfig.systemPassword[2],
									gSysConfig.systemPassword[3]);

#else
	fprintf(stream, "SYSTEM_PASSWORD = %x%x%x%x%x%x\n",
                                   	gSysConfig.systemPassword[0], 
									gSysConfig.systemPassword[1],
									gSysConfig.systemPassword[2],
									gSysConfig.systemPassword[3],
									gSysConfig.systemPassword[4],
									gSysConfig.systemPassword[5]);
#endif

#if OPEN_PASSWORD_LEN == 4
	fprintf(stream, "OPEN_PASSWORD = %x%x%x%x\n",
                                   	gSysConfig.userOpenPassword[0], 
									gSysConfig.userOpenPassword[1],
									gSysConfig.userOpenPassword[2],
									gSysConfig.userOpenPassword[3]);
#else
	fprintf(stream, "OPEN_PASSWORD = %x%x%x%x%x%x\n", 
                                   	gSysConfig.userOpenPassword[0], 
									gSysConfig.userOpenPassword[1],
									gSysConfig.userOpenPassword[2],
									gSysConfig.userOpenPassword[3],
									gSysConfig.userOpenPassword[4],
									gSysConfig.userOpenPassword[5]);
#endif

	fprintf(stream, "OPEN_PASSWORD_ENABLE = %d\n", gSysConfig.userPasswordEnable);

	fprintf(stream, "\n");
	
	fprintf(stream, "#volume: 0--100\n");
	fprintf(stream, "PROMPT_VOL = %d\n", gSysConfig.promptVol);
	fprintf(stream, "RING_VOL   = %d\n", gSysConfig.ringVol);
	fprintf(stream, "TALK_VOL   = %d\n", gSysConfig.talkVol);

	fprintf(stream, "\n");

	fprintf(stream, "SECURITY_MODE = %d\n", gSysConfig.securityMode);
#if SECURITY_PASSWORD_LEN == 4	
	fprintf(stream, "SECURITY_PASSWORD = %x%x%x%x\n", 
                                   	gSysConfig.securityPassword[0], 
									gSysConfig.securityPassword[1],
									gSysConfig.securityPassword[2],
									gSysConfig.securityPassword[3]);
#else
	fprintf(stream, "SECURITY_PASSWORD = %x%x%x%x%x%x\n", 
                                   	gSysConfig.securityPassword[0], 
									gSysConfig.securityPassword[1],
									gSysConfig.securityPassword[2],
									gSysConfig.securityPassword[3],
									gSysConfig.securityPassword[4],
									gSysConfig.securityPassword[5]);
#endif

	fprintf(stream, "\n");

	fprintf(stream, "#language: 0: english 1: Simplified chinese\n");
	fprintf(stream, "LANGUAGE = %d\n", gSysConfig.languageType);

	struct in_addr sin_addr;
	sin_addr.s_addr = gSysConfig.serverip;
	fprintf(stream, "SERVER_IP = %s\n", inet_ntoa(sin_addr) );

	fprintf(stream, "SCREEN_SAVE = %d\n", gSysConfig.screenSaveMode);
	fprintf(stream, "RING_FILE = %s\n", gSysConfig.ringFile );

	fflush( stream );
	fsync( fileno(stream) );

	fclose(stream);
	return 0;
}


/*
  从配置文件中获取全局参数，赋值到全局变量gSysConfig结构中，
这里面最重要的是房号地址，因为IP地址等网络参数都是由房号地址推导出来的
返回:
	 0: 全部参数都是OK的
	-1: 配置文件不存在，自动使用默认的配置
	-2: 配置文件中，房号地址不存在
	-3: 有其它参数不存在
	注意了: 不存在的参数都会自动使用默认值
*/
int getSystemConfig(void)
{
	FILE* stream;
	char line[CFG_LINE]; 
	char keyName[CFG_LINE]; 
	char keyValue[CFG_LINE];
	int flag = 0;

	if((stream = fopen(configFile, "r")) == NULL){
		printf("系统配置文件%s不存在! 将使用默认的系统配置\n", configFile);
		resetSystemConfig();
		storageSystemConfig();
		return -1; 
	}

	resetSystemConfig();	//防止配置文件中参数不全时，就使用默认的参数
	while(!feof(stream)){

		if(readLine(line, stream) != 0)	goto fail;
		if(isRemark(line)) continue;
		if(getKey(line, keyName, keyValue) == 0){

			//编解码参数
			if(strcmp(keyName, "FPS") == 0){
				gSysConfig.fps = atoi(keyValue);
				flag |= 0x01;
			}
			else if(strcmp(keyName, "BIT_RATE") == 0){
				gSysConfig.bitRate= strtoul(keyValue, NULL, 10); 
				flag |= 0x02;
			}
			else if(strcmp(keyName, "GOP") == 0){
				gSysConfig.gop = atoi(keyValue);
				flag |= 0x04;
			}
			//用户参数，注意要跳过中间的连接线‘-’
			else if(strcmp(keyName, "ROOM_ADDR") == 0){
				gSysConfig.roomAddr.type  = charToInt( keyValue[0] );//X6B_FJ;	//
				gSysConfig.roomAddr.zone  = charToInt( keyValue[2] ) * 10 + charToInt( keyValue[3] );
				gSysConfig.roomAddr.build = charToInt( keyValue[5] ) * 10 + charToInt( keyValue[6] );
				gSysConfig.roomAddr.unit  = charToInt( keyValue[8] ) * 10 + charToInt( keyValue[9] );
				gSysConfig.roomAddr.floor = charToInt( keyValue[11] ) * 10 + charToInt( keyValue[12] );
				gSysConfig.roomAddr.room  = charToInt( keyValue[14] ) * 10 + charToInt( keyValue[15] );
				gSysConfig.roomAddr.devno = charToInt( keyValue[17] ) * 10 + charToInt( keyValue[18] );
				flag |= 0x08;
			}
			else if(strcmp(keyName, "ROOM_NUM") == 0){
				gSysConfig.roomNum = atoi(keyValue);
				flag |= 0x10;
			}

#if SYSTEM_PASSWORD_LEN == 4
			else if(strcmp(keyName, "SYSTEM_PASSWORD") == 0){
				gSysConfig.systemPassword[0] = charToInt( keyValue[0] );
				gSysConfig.systemPassword[1] = charToInt( keyValue[1] );
				gSysConfig.systemPassword[2] = charToInt( keyValue[2] );
				gSysConfig.systemPassword[3] = charToInt( keyValue[3] );
				flag |= 0x20;
			}
#else
			else if(strcmp(keyName, "SYSTEM_PASSWORD") == 0){
				gSysConfig.systemPassword[0] = charToInt( keyValue[0] );
				gSysConfig.systemPassword[1] = charToInt( keyValue[1] );
				gSysConfig.systemPassword[2] = charToInt( keyValue[2] );
				gSysConfig.systemPassword[3] = charToInt( keyValue[3] );
				gSysConfig.systemPassword[4] = charToInt( keyValue[4] );
				gSysConfig.systemPassword[5] = charToInt( keyValue[5] );
				flag |= 0x20;
			}
#endif

#if OPEN_PASSWORD_LEN == 4
			else if(strcmp(keyName, "OPEN_PASSWORD") == 0){
				gSysConfig.userOpenPassword[0] = charToInt( keyValue[0] );
				gSysConfig.userOpenPassword[1] = charToInt( keyValue[1] );
				gSysConfig.userOpenPassword[2] = charToInt( keyValue[2] );
				gSysConfig.userOpenPassword[3] = charToInt( keyValue[3] );
				flag |= 0x40;
			}
#else
			else if(strcmp(keyName, "OPEN_PASSWORD") == 0){
				gSysConfig.userOpenPassword[0] = charToInt( keyValue[0] );
				gSysConfig.userOpenPassword[1] = charToInt( keyValue[1] );
				gSysConfig.userOpenPassword[2] = charToInt( keyValue[2] );
				gSysConfig.userOpenPassword[3] = charToInt( keyValue[3] );
				gSysConfig.userOpenPassword[4] = charToInt( keyValue[4] );
				gSysConfig.userOpenPassword[5] = charToInt( keyValue[5] );
				flag |= 0x40;
			}
#endif

			else if(strcmp(keyName, "OPEN_PASSWORD_ENABLE") == 0){
				gSysConfig.userPasswordEnable = atoi(keyValue);
				flag |= 0x80;
			}
			else if(strcmp(keyName, "PROMPT_VOL") == 0){
				gSysConfig.promptVol = atoi(keyValue);
				flag |= 0x100;
			}
			else if(strcmp(keyName, "RING_VOL") == 0){
				gSysConfig.ringVol = atoi(keyValue);
				flag |= 0x200;
			}
			else if(strcmp(keyName, "TALK_VOL") == 0){
				gSysConfig.talkVol = atoi(keyValue);
				flag |= 0x400;
			}
			else if(strcmp(keyName, "SECURITY_MODE") == 0){
				gSysConfig.securityMode = atoi(keyValue);
				flag |= 0x800;
			}

#if SECURITY_PASSWORD_LEN == 4
			else if(strcmp(keyName, "SECURITY_PASSWORD") == 0){
				gSysConfig.securityPassword[0] = charToInt( keyValue[0] );
				gSysConfig.securityPassword[1] = charToInt( keyValue[1] );
				gSysConfig.securityPassword[2] = charToInt( keyValue[2] );
				gSysConfig.securityPassword[3] = charToInt( keyValue[3] );
				flag |= 0x1000;
			}
#else
			else if(strcmp(keyName, "SECURITY_PASSWORD") == 0){
				gSysConfig.securityPassword[0] = charToInt( keyValue[0] );
				gSysConfig.securityPassword[1] = charToInt( keyValue[1] );
				gSysConfig.securityPassword[2] = charToInt( keyValue[2] );
				gSysConfig.securityPassword[3] = charToInt( keyValue[3] );
				gSysConfig.securityPassword[4] = charToInt( keyValue[4] );
				gSysConfig.securityPassword[5] = charToInt( keyValue[5] );
				flag |= 0x1000;
			}
#endif

			else if(strcmp(keyName, "LANGUAGE") == 0){
				gSysConfig.languageType = atoi(keyValue);
				flag |= 0x2000;
			}
			else if(strcmp(keyName, "SERVER_IP") == 0){
				gSysConfig.serverip = inet_addr(keyValue);
				flag |= 0x4000;
			}
			else if(strcmp(keyName, "PIP") == 0){
				gSysConfig.pip = atoi(keyValue);
				flag |= 0x8000;
			}
			else if(strcmp(keyName, "RESOLUTION") == 0){
				gSysConfig.resolution = atoi(keyValue);
				flag |= 0x10000;
			}
			else if(strcmp(keyName, "SCREEN_SAVE") == 0){
				gSysConfig.screenSaveMode = atoi(keyValue);
				flag |= 0x20000;
			}
			else if(strcmp(keyName, "RING_FILE") == 0){
				strcpy( gSysConfig.ringFile, keyValue );
				flag |= 0x40000;
			}
			if(flag == 0x7ffff){
				fclose(stream);
				return 0;
			}
		} 
	} 

fail:
	printf("flag 0x%x\n", flag);
	fclose(stream);

	if( flag & 0x08 ) return -3;	//表示配置中，至少房号地址还是存在的
	return -2; 
}

 T_Room getConfigRoomAddr(void)
 {
 	return gSysConfig.roomAddr;
 }
 int setConfigRoomAddr(T_Room roomAddr)
 {
 	gSysConfig.roomAddr = roomAddr;
	return storageSystemConfig();
	
 }

 int updateSysconfig(stSYS_CONFIG updateData)
 {
 		gSysConfig = updateData;
		return storageSystemConfig();
 }
 int setgetConfigVol(ConfigVol option ,unsigned int value  )
 {
 	
	if(option == PROMPTVOL)
		gSysConfig.promptVol = value;
	if(option == RINGVOL)
		gSysConfig.ringVol  = value;
	if(option == TALKVOL)
		gSysConfig.talkVol  = value;
	return storageSystemConfig();
	
 }
int getSecurityMode(void)
{
 	return gSysConfig.securityMode;
}

int setSecurityMode(int mode)
{
	gSysConfig.securityMode  = mode;
	return storageSystemConfig();
}
int getConfigVol(ConfigVol option)
 {
 	
 	if(option == PROMPTVOL)
		return  gSysConfig.promptVol;
	if(option == RINGVOL)
		return	gSysConfig.ringVol;
	if(option == TALKVOL)
		return	gSysConfig.talkVol;
	return DEFAULT_VOLUME;
 }

 int updateNetconfig(void * buf,unsigned int len)
 {
 	FILE *fp;
	int write_len = 0;
	if(len <= 0&& buf==NULL)
		return -1;
 	fp = fopen(netConfigFile,"wb+");
	if(fp == NULL)
		return -1;	
	write_len = fwrite(buf, len, 1, fp);
	fflush(fp);
	if(write_len == 0)
		return -1;
	return 0;
 }

  int getAppVersion(char *AppVersion) {
 	 const char * default_version = "V01-00-00-00B";
	 const int  default_len = strlen(default_version);
	 int nLen = 0;
	 FILE *pF = fopen(appVersion, "r"); //鎵撳紑鏂囦欢
	 if (AppVersion == NULL || (pF == NULL))
		 return -1;
	 fseek(pF, 0, SEEK_END); //鏂囦欢鎸囬拡绉诲埌鏂囦欢灏?
	 nLen = ftell(pF); //寰楀埌褰撳墠鎸囬拡浣嶇疆, 鍗虫槸鏂囦欢鐨勯暱搴?
	 if(nLen < default_len)
	 {
	 	goto fail0;
	 }
	 rewind(pF); //鏂囦欢鎸囬拡鎭㈠鍒版枃浠跺ご浣嶇疆
	 //璇诲彇鏂囦欢鍐呭//璇诲彇鐨勯暱搴﹀拰婧愭枃浠堕暱搴︽湁鍙兘鏈夊嚭鍏ワ紝杩欓噷鑷姩璋冩暣 nLen
	 nLen = fread(AppVersion, sizeof(char), nLen, pF);
	 if(nLen > default_len)
	 	nLen = default_len;
	 AppVersion[nLen] = '\0'; //娣诲姞瀛楃涓茬粨灏炬爣蹇?
	 //鎶婅鍙栫殑鍐呭杈撳嚭鍒板睆骞曠湅鐪?
	 fclose(pF); //鍏抽棴鏂囦欢
	 return 0;
fail0:
	strcpy(AppVersion,default_version);
	fclose(pF);
	return -1;
 }


























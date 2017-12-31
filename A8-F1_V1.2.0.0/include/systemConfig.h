#ifndef   _X6B_PROTOCOL_H_
#define   _X6B_PROTOCOL_H_ 
#include "commStructDefine.h"


#ifdef   __cplusplus 
extern   "C"   { 
#endif 
typedef enum{
	PT_CIF_NTSC = 0,	//320x240
	PT_CIF_PAL,			//352x288
	PT_VGA,				//640x480
    PT_D1_NTSC,			//720x480
    PT_D1_PAL,			//720x576
    PT_WVGA,			//800x480
    PT_SVGA,			//800x600
    PT_XVGA,			//1024x600
    PT_XGA,     		//1024 * 768
	PT_720P,			//1280x720
	PT_HD1080,  		//1920 * 1080

    PT_END,
} T_ePICTURE_TYPE;
typedef enum{
	PROMPTVOL = 0,
	RINGVOL,
	TALKVOL,
    VOL_END,
} ConfigVol;
typedef  enum UDP_TYPE{
	UDP_UNICAST = 0,
	UDP_MULTICAST,	
	UDP_BROADCAST,
	
	
}UDP_TYPE;




typedef struct {

	T_ePICTURE_TYPE	resolution;	//视频采集尺寸格式
	int  fps;	//帧率
	unsigned int  bitRate;	//码率
	int gop;
	int pip;	//	0：无PIP；1：对方大图，本方小图；2：对方小图，本方大图；3：并排

	T_Room	roomAddr;		//本机房号地址
	int roomNum;			//分机位数

	char systemPassword[8];	//工程密码

	int promptVol;		//提示音音量
	int ringVol;		//铃声音量
	int talkVol;		//通话音量

	int languageType;	//0:english 1:chinese_simple 2:chinese_traditional

	uint32_t serverip;
	uint32_t sequence;	//注: 此参数未保存到文件中

	int screenSaveMode;	//屏保模式: 0 无屏幕保护， 1 黑屏， 2 时间， 3 广告
	char ringFile[192];	//用户可选的铃声文件

	int silentMode;	//1:免打扰模式；0:正常模式；免打扰模式只是不响铃而已，此参数未保存到文件中

	//仅室内机使用
	char userOpenPassword[8];	//住户开门密码
	char securityPassword[8];	//安防拆防密码
	int securityMode;

	//仅门口机、围墙机、别墅机使用
	char openPassword[8];	//门口机公共开门密码
	int openPasswordEnable;	//门口机公共密码使能
	int userPasswordEnable;	//住户开门密码使能
	int lockType;			//开锁类别，通电开还是断电开，0断电开，1通电开
	int openTime;			//开锁继电器动作时长，不能设为0
	int magneticDelay;		//开门超时，设为0则无开门超时报警

	int Crc8;
}stSYS_CONFIG;





int getSystemConfig(void);
T_Room getConfigRoomAddr(void);
int getConfigVol(ConfigVol option);
int setConfigRoomAddr(T_Room roomAddr);
int setgetConfigVol(ConfigVol option ,unsigned int value  );
int updateSysconfig(stSYS_CONFIG updateData);
int updateNetconfig(void * buf,unsigned int len);
int getSecurityMode(void);
int setSecurityMode(int mode);

int getAppVersion(char *AppVersion);




#ifdef   __cplusplus 
} 
#endif 

#endif

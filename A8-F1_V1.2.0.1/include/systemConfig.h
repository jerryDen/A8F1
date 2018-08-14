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

	T_ePICTURE_TYPE	resolution;	//��Ƶ�ɼ��ߴ��ʽ
	int  fps;	//֡��
	unsigned int  bitRate;	//����
	int gop;
	int pip;	//	0����PIP��1���Է���ͼ������Сͼ��2���Է�Сͼ��������ͼ��3������

	T_Room	roomAddr;		//�������ŵ�ַ
	int roomNum;			//�ֻ�λ��

	char systemPassword[8];	//��������

	int promptVol;		//��ʾ������
	int ringVol;		//��������
	int talkVol;		//ͨ������

	int languageType;	//0:english 1:chinese_simple 2:chinese_traditional

	uint32_t serverip;
	uint32_t sequence;	//ע: �˲���δ���浽�ļ���

	int screenSaveMode;	//����ģʽ: 0 ����Ļ������ 1 ������ 2 ʱ�䣬 3 ���
	char ringFile[192];	//�û���ѡ�������ļ�

	int silentMode;	//1:�����ģʽ��0:����ģʽ�������ģʽֻ�ǲ�������ѣ��˲���δ���浽�ļ���

	//�����ڻ�ʹ��
	char userOpenPassword[8];	//ס����������
	char securityPassword[8];	//�����������
	int securityMode;

	//���ſڻ���Χǽ����������ʹ��
	char openPassword[8];	//�ſڻ�������������
	int openPasswordEnable;	//�ſڻ���������ʹ��
	int userPasswordEnable;	//ס����������ʹ��
	int lockType;			//�������ͨ�翪���Ƕϵ翪��0�ϵ翪��1ͨ�翪
	int openTime;			//�����̵�������ʱ����������Ϊ0
	int magneticDelay;		//���ų�ʱ����Ϊ0���޿��ų�ʱ����

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

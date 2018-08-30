
/*********************************************************************
* Copyright (C) 2014-2017
**********************************************************************/
/*
 *===================================================================
 *   Full-Duplux Audio Module For Smart CAM
 *===================================================================
 */

#ifndef _US_SCAM_AUDIO_DUPLUX_H_
#define _US_SCAM_AUDIO_DUPLUX_H_

#define USCAM_OK		0
#define USCAM_ERR		(-1)


/* 
 *  定义MIC采集回调函数，每20ms回调一次，采集到的数据存放于pcm_buf中，数据长度为pcm_len（byte）
 *
 * Input:
 *       const struct timeval *tv     时间戳
 *       const void *pcm_buf          pcm数据地址
 *       const int pcm_len            pcm数据长度
 * Return: 
 *        
 *
 */
typedef void (*UsCamAudioCallback)(const struct timeval *tv, const void *pcm_buf, const int pcm_len);


typedef struct UsCamAudioAttribute_t
{
	int sampleRate;				//采样率, sample Hz（Hisi3518EV200目前只支持8k）
	int vqemode;				//0-纯软件模式(默认), 1-硬件优化模式

	UsCamAudioCallback cbm;		// callback audio
}UsCamAudioAttr;

/****************************************************************************
 * UsCamAudioOpen( )
 *  打开音频输入/出通道
 *
 * Input:
 *        UsCamAudioAttr *pAttr     音频属性数据地址
 * Return:
 *        0          打开成功
 *        1          已经打开
 *        -EINVAL    打开失败
 */
int UsCamAudioOpen(UsCamAudioAttr *pAttr);

/****************************************************************************
 * UsCamAudioClose( )
 *  关闭音频输入/出通道
 *
 * Input:
 *
 * Return:
 *        
 *
 */
void UsCamAudioClose();

/****************************************************************************
 * UsCamAudioStart( )
 *  开始采集
 *
 * Input:
 *
 * Return:
 *         USCAM_OK         启动成功
 *         USCAM_ERR        启动失败
 */
int UsCamAudioStart();

/****************************************************************************
 * UsCamAudioStop( )
 *  停止采集
 *
 * Input:
 *
 * Return:
 *        0   完成停止采集
 *
 */
int UsCamAudioStop();

/****************************************************************************
 * UsCamAudioPlay( )
 * 播放PCM音频.20毫秒PCM数据, 阻塞式播放。
 *
 * Input:
 *        char *pcm_data    播放数据地址
 *        int len           播放数据的长度
 * Return:
 *        0    播放完毕
 *
 */
int UsCamAudioPlay(char *pcm_data, int len);

/****************************************************************************
 * UsSCamAudioOutputSetVolume(...)
 * 该函数设置SPK的硬件输出音量（范围为0-100），具体数值由mediawin技术人员调试后确定
 * 
 * Input:
 *		- value         : 预设置的SPK硬件音量值
 * Return:
 *        函数处理失败返回-1，否则返回0
 *
 */
int UsCamAudioOutputSetVolume(int value);


/****************************************************************************
 * UsCamGetVersion( )
 * 直接获取版本号
 *
 * Input:
 *
 * Return:
 *        库版本号
 *
 */
char *UsCamGetVersion();

/****************************************************************************
 * UsCamSysInit
 *  初始化海思系统，必须在调用其它函数前调用
 *  如果系统其它地方已经初始化海思的系统，则可以不调用此函数以及UsCamSysDeInit函数
 *
 * Return:
 *        0          初始化成功
 *        其它        初始化失败
 */
int UsCamSysInit();


/****************************************************************************
 * UsCamSysInit()
 * 反初始化库，程序退出前调用
 * 必须调用了UsCamSysInit初始化才要调用此函数。
 *
 */
void UsCamSysDeInit();


#endif

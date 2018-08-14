
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
 *  ����MIC�ɼ��ص�������ÿ20ms�ص�һ�Σ��ɼ��������ݴ����pcm_buf�У����ݳ���Ϊpcm_len��byte��
 *
 * Input:
 *       const struct timeval *tv     ʱ���
 *       const void *pcm_buf          pcm���ݵ�ַ
 *       const int pcm_len            pcm���ݳ���
 * Return: 
 *        
 *
 */
typedef void (*UsCamAudioCallback)(const struct timeval *tv, const void *pcm_buf, const int pcm_len);


typedef struct UsCamAudioAttribute_t
{
	int sampleRate;				//������, sample Hz��Hisi3518EV200Ŀǰֻ֧��8k��
	int vqemode;				//0-�����ģʽ(Ĭ��), 1-Ӳ���Ż�ģʽ

	UsCamAudioCallback cbm;		// callback audio
}UsCamAudioAttr;

/****************************************************************************
 * UsCamAudioOpen( )
 *  ����Ƶ����/��ͨ��
 *
 * Input:
 *        UsCamAudioAttr *pAttr     ��Ƶ�������ݵ�ַ
 * Return:
 *        0          �򿪳ɹ�
 *        1          �Ѿ���
 *        -EINVAL    ��ʧ��
 */
int UsCamAudioOpen(UsCamAudioAttr *pAttr);

/****************************************************************************
 * UsCamAudioClose( )
 *  �ر���Ƶ����/��ͨ��
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
 *  ��ʼ�ɼ�
 *
 * Input:
 *
 * Return:
 *         USCAM_OK         �����ɹ�
 *         USCAM_ERR        ����ʧ��
 */
int UsCamAudioStart();

/****************************************************************************
 * UsCamAudioStop( )
 *  ֹͣ�ɼ�
 *
 * Input:
 *
 * Return:
 *        0   ���ֹͣ�ɼ�
 *
 */
int UsCamAudioStop();

/****************************************************************************
 * UsCamAudioPlay( )
 * ����PCM��Ƶ.20����PCM����, ����ʽ���š�
 *
 * Input:
 *        char *pcm_data    �������ݵ�ַ
 *        int len           �������ݵĳ���
 * Return:
 *        0    �������
 *
 */
int UsCamAudioPlay(char *pcm_data, int len);

/****************************************************************************
 * UsSCamAudioOutputSetVolume(...)
 * �ú�������SPK��Ӳ�������������ΧΪ0-100����������ֵ��mediawin������Ա���Ժ�ȷ��
 * 
 * Input:
 *		- value         : Ԥ���õ�SPKӲ������ֵ
 * Return:
 *        ��������ʧ�ܷ���-1�����򷵻�0
 *
 */
int UsCamAudioOutputSetVolume(int value);


/****************************************************************************
 * UsCamGetVersion( )
 * ֱ�ӻ�ȡ�汾��
 *
 * Input:
 *
 * Return:
 *        ��汾��
 *
 */
char *UsCamGetVersion();

/****************************************************************************
 * UsCamSysInit
 *  ��ʼ����˼ϵͳ�������ڵ�����������ǰ����
 *  ���ϵͳ�����ط��Ѿ���ʼ����˼��ϵͳ������Բ����ô˺����Լ�UsCamSysDeInit����
 *
 * Return:
 *        0          ��ʼ���ɹ�
 *        ����        ��ʼ��ʧ��
 */
int UsCamSysInit();


/****************************************************************************
 * UsCamSysInit()
 * ����ʼ���⣬�����˳�ǰ����
 * ���������UsCamSysInit��ʼ����Ҫ���ô˺�����
 *
 */
void UsCamSysDeInit();


#endif

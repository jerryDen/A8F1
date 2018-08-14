
/*
 * cmmdWord.h
 *
 *  Created on: 2013-10-10
 *      Author: Alvin
 */
#ifndef CMMANDWORD_H_
#define CMMANDWORD_H_

#define X6B_FJ		1
#define X6B_ZJ		2
#define X6B_BSZJ	3
#define X6B_QRJ		4



#define X6B_BAFJ	6
#define X6B_BAJ		6
#define X6B_WQJ		7
#define X6B_GLJ		8

#define HI3518E_FJ   4


/**
 *	系统中有如下几种节点：
 *
 *		分机，主机，别墅主机，二次确认机，保安分机，围墙机，管理机
 *
 *		所有节点均可由6字节来描述，每字节取值范围1--99
 *
 *		Byte0	Byte1	Byte2	 Byte3    Byte4 	Byte5
 * 		区号		楼栋号	单元号	     层号　 　房号		设备号
 *
 *	房号地址广播：
 *		所有节点：区号0xff，楼栋号为0xff，单元号为0xff，层号为0xff，房号为0xff，分机号0xff
 *		楼栋所有住户：区号1-99，楼栋号1-99，单元号为0xff，层号为0xff，房号为0xff
 *		单元所有住户：区号1-99，楼栋号1-99，单元号1-99，层号为0xff，房号为0xff
 *		单个住户：都不为0xff
 */

/**
 * 	网络中的几个特殊节点地址定义
 *		管理机：楼栋号，单元号，层号及房号固定为0，区号及设备号决定一个管理机
 *		围墙机：楼栋号，单元号，层号及房号固定为0，区号及设备号决定一个围墙机
 * 		主机: 层号及房号固定为0
 */
//===============================================
#define 	SET_TIME  				0X05

#define  	SET_SYS_CONFIG			0X06

#define		READ_SYS_CONFIG			0X07
#define		TFTP_UPDATE_APP			0X08

#define     HEARTBEAT       		0X09 //心跳包
#define     HEARTBEAT_TIME_OUT      0x10 //心跳包超时

#define     REBOOT_CMD				0x0A

#define     INTERNET_CALL_CMD		0xE0 //add by alvin
#define 	SMS_CMD					0XE1
#define 	CALL_CMD				0XE2

#define 	RST_CMD					0XE3
#define 	REG_USER_PASSWORD		0XE4
#define 	DEL_ROOM_CMD			0XE5
#define 	REG_CARD_CMD			0XE6
#define 	DEL_CARD_CMD			0XE7
#define 	OPEN_CMD				0XE8
#define		WATCH_CMD				0XE9
#define 	HOOK_CMD				0XEA
#define 	UNHOOK_CMD				0XEB
#define 	UPDATE_SYSCONFIG		0XEC

#define 	DOWNLOAD_CMD			0XED
#define 	TIME_OUT				0xEE // add by alvin
#define     OTHER_DEV_PROC          0xEF // add by alvin
#define 	READ_BLOCK_ROOM			0XA1
#define 	DEL_ALL_ROOM_CMD		0XA2

#define		ELEVATOR_ENABLE_FLOORS	  0XD1	    //开放指定的多个楼层
#define 	ELEVATOR_CALL_FLOOR		  0XD2	    //呼叫电梯到指定楼层
#define 	OUTDOORBELL_RING		  0xD3      //门铃

//以上命令字不能改变，因为PC软件已经使用

#define 	DEL_ALL_CARD_CMD		0XA3
#define 	VF_ADJ_CMD				0XA4
#define 	UPLOAD_PARAMET			0XA7
#define 	WARN_CMD				0XA8

#define 	PRE_SM_CMD				0XA9
#define 	NEXT_SM_CMD				0XAA
#define 	DEL_SM_CMD				0XAB


#define 	UPDATE_NETCONFIG		0XAC

#define 	READ_ROUTING_TAB		0XAD
#define 	DEL_ALL_ROUTING			0XAE
#define  	MODIFY_ROUTING_TAB		0XAF

#define		BOARDCAST_SERVER        0XB1  //保留
#define 	BOARDCAST_STATE         0xB2
#define 	UPDATE_ONLINE_PEERS     0xB3

#define 	SET_HOUSE_INFO 			0xB8   //
#define 	SET_DEV_INFO			0xB9   //
#define 	GET_OUTDOORBELL_NUM		0xBA   //
//应答类命令字
#define NOT_ACK				0XAA
#define ACK_OK				0X61
#define ACK_ERR				0X62
#define ROOMNO_ERR			0X63
#define LINE_BUSY	 		0X64
#define NOT_ONLINE			0X65
#define PASS_ERR			0X66
#define CARD_ERR			0X67

//以下记录类别定义，注意不能定义为0XAA，不能随便改变顺序
#define HW_RECORD			0X80
#define MC_RECORD			0X81
#define MQ_RECORD			0X82
#define YW_RECORD			0X83
#define JJ_RECORD			0X84

#define FJ_OPEN_RECORD		0X85
#define CARD_RECORD			0X86
#define PASS_RECORD			0X87
#define GLJ_OPEN_RECORD		0X88

#define CALL_RECORD			0X89

#define OPEN_OVER_RECORD	0X90
#define NOT_OPEN_RECORD		0X91

//以下命令用于UI与UI之间， UI与后台进程之间

//通话类型定义
#define TOCALLING       0
#define BECALLED        1
#define FREE_FREE       2

//状态时间定义
#define WAITACK_TIME      (3*1000)  //心跳包时间改成ms单位
#define RING_TIME         (60*1000)
#define TALKING_TIME      (120*1000)
#define WATCHING_TIME    (60*1000)
#define HEARTBEAT_TIMER   5  //设定心跳包发送的间隔时间

#endif /* CMMDWORD_H_ */

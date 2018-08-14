#ifndef SYSTEMCONFIGMACRO_H
#define SYSTEMCONFIGMACRO_H
/************以下端口定义必须与X6B系统一致***************/
//UDP接收端口固定
#define COMM_UNI_PORT		6768
#define COMM_MULIT_PORT     6770
#define COMM_BROADCAST_PORT 6769
#define VIDEO_PORT          6869
#define VIDEO_MULIT_PORT	6869
#define AUDIO_PORT          6970
#define VRACK_PORT			6871

/*--------------------分割线-----------------------------*/
/*********以下端口为A8系统定义本地通讯使用***************/


//#define X6BMSGS_CLIENT_UDP_PORT 46768
#define X6BMSGS_SERVER_UDP_PORT 46769
#define X6BMSGS_SERVER_TCP_PORT 46770

/******************通讯类型定义******************/
#define UNI_CAST    0
#define MULIT_CAST  1
#define BOARD_CAST  2

/*******程序版本定义，每次修改程序后必须要修改*******/
#define B1_VERSION_MAJOR 1
#define B1_VERSION_MINOR 2
#define B1_VERSION_PATCH 0

#endif // SYSTEMCONFIGMACRO_H




#ifndef _CELL_INFORMATION_H__
#define _CELL_INFORMATION_H__
#include "commStructDefine.h"
#include "roomNetConfig.h"

typedef struct CellInfo{

	T_Room    localRoom;
	NETCONFIG_FILE_HEAD fileHead;
	NETCONFIG_ZONE_HEAD zoneHead;
	NETCONFIG_BUILD     netConfigInfo;
}CellInfo,*pCellInfo;
int cellInfoInit(T_Room localRomm );
int getCellInformation(pCellInfo cellInfo );
int isMultiExtension(void);
int getManagerRoom(pT_Room room);
int getLocalRoom(pT_Room room);
int getRoomIpAddr(T_Room Romm, uint32_t *ipAddr );
int getLocalSequence(void);

void cellInfoExit(void);





#endif 












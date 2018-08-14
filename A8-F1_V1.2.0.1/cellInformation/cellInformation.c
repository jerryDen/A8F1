#include "cellInformation.h"
#include "roomNetConfig.h"
#include "commonHead.h"
#include <stdlib.h>
#include <string.h>

#define ROOM_CONFIG_FILE "/usr/work/netConfig.bin" 

pCellInfo g_cellInfo = NULL;

int cellInfoInit(T_Room localRomm )
{
	int ret;
	if(g_cellInfo  != NULL)
		return 0;
	g_cellInfo =(pCellInfo)malloc(sizeof(CellInfo));
	CHECK_RET(g_cellInfo == NULL, "fail to malloc g_cellInfo！", goto fail0);
	bzero(g_cellInfo,sizeof(CellInfo));
	ret  = initNetConfigFile(ROOM_CONFIG_FILE);
	CHECK_RET(ret<0, "fail to initNetConfigFile!", goto fail1);
	ret  = getMyBuild(&localRomm,&(g_cellInfo->netConfigInfo));
	CHECK_RET(ret<0, "fail to getMyBuild!", goto fail1);
	ret  = getGlobalZoneHead(&(g_cellInfo->fileHead));	
	CHECK_RET(ret<0, "fail to getGlobalZoneHead!", goto fail1);
	ret  = getMyZone(&localRomm,&(g_cellInfo->zoneHead));
	CHECK_RET(ret<0, "fail to getMyZone!", goto fail1);
	g_cellInfo->localRoom = localRomm;
	

	return 0;
fail1:
	free(g_cellInfo);
	g_cellInfo = NULL;
fail0:
	return -1;
	
}
int getCellInformation(pCellInfo cellInfo )
{
	if(cellInfo&& g_cellInfo){
		 *cellInfo = *g_cellInfo;
		 return 0;
	}	
	return -1;
}
int getLocalRoom(pT_Room room)
{
	if(g_cellInfo == NULL||room == NULL )
	{	
		return -1;
	}
	bzero(room,sizeof(T_Room));
	*room = g_cellInfo->localRoom;

	return 0;
}
int getManagerRoom(pT_Room room)
{
	if(g_cellInfo == NULL||room == NULL )
	{	
		return -1;
	}
	bzero(room,sizeof(T_Room));
	
	*room = g_cellInfo->localRoom;
	
	room->type = 8;
	room->unit = 0;
	room->build = 0;
	room->floor = 0;
	room->room = 0;
	room->devno = 1;


	
	
	return 0;
}
int getBAJRoom(pT_Room room)
{
	if(g_cellInfo == NULL||room == NULL )
	{	
		return -1;
	}
	bzero(room,sizeof(T_Room));
	
	*room = g_cellInfo->localRoom;
	
	room->type = 6;
	room->unit = 0;
	room->build = 0;
	room->floor = 0;
	room->room = 0;
	room->devno = 1;
	return 0;
}


int isMultiExtension(void)
{
	if(g_cellInfo == NULL )
		return -1;

	if (g_cellInfo->netConfigInfo.deviceNumPerRoom >1)
		return 1;
	return 0;
}
int getLocalSequence(void)
{
	uint32_t sequence;
	
	if( getRoomSequence(&g_cellInfo->localRoom,&sequence) == 0)
	{
		return sequence;
	}
	return -1;
}
int getRoomIpAddr( T_Room destRoom, unsigned int *ipAddr)
{
	 uint32_t ipMask,gw;
	if (0 == getRoomNetInterface(&destRoom,ipAddr,&ipMask,&gw))
	{
		return 0;
	}
	return -1;	
}
void cellInfoExit(void)
{
	if(g_cellInfo == NULL )
		return ;
	free(g_cellInfo);
}


























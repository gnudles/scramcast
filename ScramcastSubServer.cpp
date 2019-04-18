/*
 * ScramcastSubServer.cpp
 *
 *  Created on: Jul 2, 2018
 *      Author: Orr Dvory
 */

#include "ScramcastSubServer.h"

ScramcastSubServer::ScramcastSubServer(int tcp_sock, int scram_sock, int hostId):ScramcastServer(scram_sock,hostId),_clntSock(tcp_sock) {

}

ScramcastSubServer::~ScramcastSubServer() {

		closesocket(_clntSock);
		closesocket(_bcastSock);

}
int32_t ScramcastSubServer::AddMemoryWatch(u_int8_t NetId, u_int32_t Offset, u_int32_t Length, u_int32_t resolution)
{
#pragma pack(4)
	struct { char command; u_int8_t net;u_int8_t spare[2];
		struct memwatch m;

	} buf;
#pragma pack()
	if (Length % (resolution/8) != 0 || Offset % (resolution/8) != 0)
	{
	    DFATAL("Alignment error.");
	    return -1;
	}
	if (Offset >= MAX_MEMORY)
	{
		printf(TERM_RED_COLOR "ScramcastSubServer::AddMemoryWatch: bad offset was given %u\n" TERM_RESET ,Offset);
		return -1;
	}
	if (Offset + Length > MAX_MEMORY)
	{
		printf(TERM_RED_COLOR "ScramcastSubServer::AddMemoryWatch: warning: Offset (%u) + Length (%u) (=%u) is larger than %u\n" TERM_RESET ,Offset,
				Length, Offset+ Length,MAX_MEMORY);
		return -1;
	}
	buf.net = NetId;
	buf.command = 'a';
	buf.m.length = Length;
	buf.m.offset = Offset;
	buf.m.resolution = resolution;
	assert(sizeof(buf)==16);
	if (send(_clntSock,(const char*)&buf,sizeof(buf),0)!= sizeof(buf))
		return -1;
	//fsync(_clntSock);
	printf("sent memory watch command %u %u\n",Offset,Length);
	return 0;

}


#include "scramcast.h"
#ifndef __WINDOWS__
#include <unistd.h>
#else
#include <Windows.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
inline void common_sleep(int ms)
{
#ifndef __WINDOWS__
	usleep(ms*1000);
#else
	Sleep(ms);
#endif
}
int main()
{
	ScramcastServerPtr server = SC_createServer(SERV_ANY);
	void * mem = SC_getBaseMemory(0);
	if (server == 0)
	{
		fprintf(stderr, "SC_createServer failed\n");
		return -1;
	}
	int i;
#define CHUNK_SIZE 4096
	for (i=0 ; i < SCRAMCAST_MEM_SIZE/CHUNK_SIZE ; ++i)
	{
		memset(&((char*)mem)[i*CHUNK_SIZE],0,CHUNK_SIZE);
		SC_postMemory(server,0,i*CHUNK_SIZE,CHUNK_SIZE, 8);
		common_sleep(5);
	}
	SC_destroyServer(server);
	SC_destroyMemories();
	return 0;

}

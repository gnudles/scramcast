#include "scramcast.h"
#ifndef __WINDOWS__
#include <unistd.h>
#else
#include <Windows.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

bool stop_main_loop = false;
void sigterm_catch(int sig)
{
	stop_main_loop = true;

}

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
#ifndef __WINDOWS__
	sigset(SIGINT,sigterm_catch);
	sigset(SIGTERM,sigterm_catch);
#endif
	ScramcastServerPtr server = SC_createServer(SERV_MAIN);
	//void * mem = SC_getBaseMemory(0);
	if (server == 0)
	{
		fprintf(stderr, "SC_createServer failed\n");
		return -1;
	}
	while (!stop_main_loop)
	{
		common_sleep(100000);
	}
	SC_destroyServer(server);
	SC_destroyMemories();
	return 0;
}

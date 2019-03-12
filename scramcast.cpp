
#include "ScramcastSubServer.h"
#include "ScramcastMainServer.h"
#include "ScramcastMemory.h"
#define SCRAM_LIB_BUILD
#include "scramcast.h"
#include "netconfig.h"


CEXPORT ScramcastServerPtr SC_createServer(u_int32_t server_type){
#ifdef __WINDOWS__
	{
			int err;
			WORD verRequested;
			WSADATA wsaData;

			verRequested = MAKEWORD(1,1);
			err = WSAStartup(verRequested, &wsaData);
	}
#endif
	bool bind_success;
	uint32_t host_id = ScramcastServer::getSystemIP(NETMASK,DEFAULT_NET).ipv4 & (~NETMASK);
	printf("IMPORTANT: working on network %d.%d.%d.%d",host_id>>24,(host_id>>16)&0xff,(host_id>>8)&0xff,host_id&0xff);
	host_id = HASH_HOST(host_id);
	int tcp_sock = ScramcastServer::createTCPLocalSocket(server_type & SERV_MAIN ,&bind_success);
	if (tcp_sock < 0)
		return NULL;
	int scram_sock = ScramcastServer::createDatagramBroadcastSocket(ScramcastServer::getSystemIP(NETMASK,DEFAULT_NET).ipv4);
	if (scram_sock < 0)
	{
		closesocket(tcp_sock);
		return NULL;
	}
	if (bind_success) //start main server
	{
		int scram_sock_recv = ScramcastServer::createDatagramBroadcastSocket(INADDR_ANY);
		if (scram_sock_recv < 0)
		{
			closesocket(scram_sock);
			closesocket(tcp_sock);
			return NULL;
		}
		listen(tcp_sock,5);
		ScramcastMainServer* main_server =  new ScramcastMainServer(tcp_sock,scram_sock,scram_sock_recv,host_id);

#ifdef WIN_THREADS
		DWORD server_thread_id;
		HANDLE server_thread_handle;
#else
		pthread_t server_thread_id;
		int ret;
#endif



#ifdef WIN_THREADS
		server_thread_handle = CreateThread( NULL, 0 ,ScramcastMainServer::server_listen,(LPVOID)main_server,0,&server_thread_id);
		if (server_thread_handle == NULL)
		{
			delete main_server;
			return NULL;
		}
		main_server->setThreadVars(server_thread_id,server_thread_handle);
#else
		ret = pthread_create (&server_thread_id, NULL, ScramcastMainServer::server_listen, (void*)main_server);
		if (ret != 0)
		{
			delete main_server;
			return NULL;
		}
		main_server->setThreadVars(server_thread_id);
#endif
		return main_server;
	}
	else if (server_type & SERV_SLAV)//start sub server
	{
		struct sockaddr_in name;

		name.sin_family = AF_INET;
		name.sin_port = htons (SCRAMCAST_LOCAL_PORT);
		name.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

		int ret = connect(tcp_sock,(struct sockaddr*)&name,sizeof(name));
		if (ret == -1)
		{
			perror("connect");
			closesocket(tcp_sock);
			closesocket(scram_sock);
			return NULL;
		}

		ScramcastSubServer* sub_server = new ScramcastSubServer(tcp_sock,scram_sock,host_id);
		return sub_server;
	}
	return NULL;
}
/*CEXPORT int32_t SC_startServer(ScramcastServerPtr sc_server)
{
	return 0;
}
CEXPORT int32_t SC_stopServer(ScramcastServerPtr sc_server)
{
	return 0;
}*/
CEXPORT int32_t SC_destroyServer(ScramcastServerPtr sc_server)
{
	delete sc_server;
#ifdef __WINDOWS__
	WSACleanup();
	{
		int err = WSAGetLastError();

		if( WSANOTINITIALISED == err)
			return 0;
	}
#endif
	return 0;
}
CEXPORT int32_t SC_destroyMemories()
{
	ScramcastMemory::destroyAllMemories();
	return 0;
}
CEXPORT void * SC_getBaseMemory(u_int32_t net_id)
{
	ScramcastMemory* mem = ScramcastMemory::getMemoryByNet(net_id);
	if (mem)
		return mem->getBaseMemory();
	return NULL;
}
CEXPORT u_int32_t SC_getMemoryLength()
{
	return MAX_MEMORY;
}

CEXPORT u_int32_t SC_postMemory(ScramcastServerPtr sc_server, u_int8_t net_id, u_int32_t offset, u_int32_t length)
{
	if (sc_server == 0)
		return 0;
	return sc_server->postMemory(net_id,offset, length);
}
CEXPORT int32_t SC_addMemoryWatch(ScramcastServerPtr sc_server, u_int8_t net_id, u_int32_t offset, u_int32_t length, u_int8_t resolution)
{
	if (sc_server == 0)
		return -1;
	return sc_server->AddMemoryWatch(net_id,offset, length, resolution);
}

int scramcast_dbg_lvl = 0;

CEXPORT void SC_setDebugLevel(int dbg_lvl)
{
	scramcast_dbg_lvl = dbg_lvl;
}


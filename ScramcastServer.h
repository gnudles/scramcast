/*
 * ScramcastServer.h
 *
 *  Created on: Jul 2, 2018
 *      Author: Orr Dvory
 */

#ifndef SCRAMCASTSERVER_H_
#define SCRAMCASTSERVER_H_
#define NETWORKING
#include "sc_defines.h"
#define SC_SOCKET_ERROR -1
#define SC_BIND_ERROR -2
struct IPv4v6
{
	uint32_t ipv4;
	struct in6_addr ipv6;
};
#pragma pack(4)
struct memwatch
{
	u_int32_t offset;
	u_int32_t length;
	u_int32_t resolution;
};
#pragma pack()
class ScramcastServer {
protected:
	ScramcastServer(int scram_sock, int hostId);
public:

	virtual ~ScramcastServer();
	/*virtual int32_t start();
	virtual int32_t stop();*/
	virtual u_int32_t postMemory(u_int8_t NetId, u_int32_t Offset, u_int32_t Length, u_int32_t resolution);
	u_int32_t sendMemoryRequest();
	virtual int32_t AddMemoryWatch(u_int8_t NetId, u_int32_t Offset, u_int32_t Length, u_int32_t resolution)=0;
	static int createDatagramBroadcastSocket(uint32_t bind_ipv4);
	static int createTCPLocalSocket(bool try_to_bind, bool *bind_success);
	static const struct IPv4v6& getSystemIP(uint32_t net_mask, uint32_t net);
	static void overrideGetSystemIP(const struct IPv4v6 & ip);
	static struct IPv4v6 getRealSystemIP(uint32_t net_mask, uint32_t net);
private:

	static struct IPv4v6 _sysIP;
protected:
	int _bcastSock;
#ifdef __WINDOWS__

#endif
#ifdef WIN_THREADS
	DWORD _server_thread_id;
	HANDLE _server_thread_handle;
#else
	pthread_t _server_thread_id;
#endif
	u_int16_t _hostId;

};

#endif /* SCRAMCASTSERVER_H_ */

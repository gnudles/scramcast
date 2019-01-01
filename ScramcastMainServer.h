/*
 * ScramcastMainServer.h
 *
 *  Created on: Jul 2, 2018
 *      Author: Orr Dvory
 */

#ifndef SCRAMCASTMAINSERVER_H_
#define SCRAMCASTMAINSERVER_H_

#include "ScramcastServer.h"
#include <vector>
#include <algorithm>
using namespace std;

class ScramcastMainServer: public ScramcastServer {
public:
	ScramcastMainServer(int tcp_sock,int scram_sock_send,int scram_sock_recv, int hostId);
	virtual ~ScramcastMainServer();
	virtual int32_t AddMemoryWatch(u_int8_t NetId, u_int32_t Offset, u_int32_t Length, u_int32_t resolution);
#ifdef WIN_THREADS
	void setThreadVars(DWORD server_thread_id, HANDLE server_thread_handle) {
		_server_thread_id=server_thread_id;
		_server_thread_handle=server_thread_handle;
	}
#else
	void setThreadVars(pthread_t server_thread_id) {
		_server_thread_id = server_thread_id;
	}
#endif

#ifdef WIN_THREADS
static DWORD WINAPI server_listen(LPVOID args);
#else
static void* server_listen(void* args);
#endif
private:
	int32_t read_packets();
	int32_t read_command(int fd);
	int32_t accept_connections();
	int32_t remove_connection(int fd);
	static int handle_packet(struct SC_Packet* packet_buf, int32_t packet_len);
	int set_fd_set(fd_set* read_fds);
	int32_t memory_check();
	int32_t run_memwatch(u_int8_t NetId,struct memwatch watch);
	int _srvrSock;
	int _bcastRecvSock;
	vector<int> connections;
	vector<struct memwatch> _watchers [MAX_NETWORKS];
	SC_MUTEX_T _watchersMutex;
	bool _join;
};

#endif /* SCRAMCASTMAINSERVER_H_ */

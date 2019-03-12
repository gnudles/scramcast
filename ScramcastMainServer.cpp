/*
 * ScramcastMainServer.cpp
 *
 *  Created on: Jul 2, 2018
 *      Author: Orr Dvory
 */

#include "ScramcastMainServer.h"
#include "ScramcastMemory.h"

ScramcastMainServer::ScramcastMainServer(int tcp_sock, int scram_sock_send,int scram_sock_recv, int hostId):ScramcastServer(scram_sock_send, hostId),_srvrSock(tcp_sock),_bcastRecvSock(scram_sock_recv),_join(SC_FALSE) {
	_watchersMutex=SC_CREATE_MUTEX();
}

ScramcastMainServer::~ScramcastMainServer() 
{
	printf("bye bye\n");
	ATOMIC_STORE(&_join,SC_TRUE);
	#ifdef WIN_THREADS
		WaitForSingleObject(_server_thread_handle, 500); //200ms
		//TODO: check returned value.
	#else
		pthread_join(_server_thread_id, NULL);
	#endif
		closesocket(_srvrSock);
		closesocket(_bcastSock);
		closesocket(_bcastRecvSock);
		vector<int>::const_iterator it;
		vector<int>::const_iterator it_end = connections.end();
		for (it = connections.begin(); it != it_end ; ++it)
		{
			closesocket(*it);
		}
		SC_DESTROY_MUTEX(_watchersMutex);

}
static int32_t make_union(u_int32_t off1,u_int32_t len1, u_int32_t &off2, u_int32_t &len2)
{
	if ((off1 <= off2 && (off1 + len1 >= off2)) || (off2 <= off1 && (off2 + len2 >= off1)))
	{
		u_int32_t to = max(off1 + len1,off2 + len2);
		off2 = min (off1,off2);
		len2 = to - off2;
		printf(TERM_GREEN_COLOR "make_union %d %d\n" TERM_RESET,off2,len2);
		return 1;
	}
	return 0;
}
int32_t ScramcastMainServer::AddMemoryWatch(u_int8_t NetId, u_int32_t Offset, u_int32_t Length, u_int32_t resolution)
{
	if (NetId >= MAX_NETWORKS)
		return -1;
	if (Offset >= MAX_MEMORY)
	{
		printf (TERM_RED_COLOR "ScramcastMainServer::AddMemoryWatch: offset is too large, not adding watch\n" TERM_RESET);
		return -1;
	}
	if (Offset + Length > MAX_MEMORY)
	{
		Length = MAX_MEMORY - Offset;
	}
	struct memwatch w;
	w.offset=Offset;
	w.length = Length;
	w.resolution = resolution;
	SC_LOCK_MUTEX(_watchersMutex);
	if (!_watchers[NetId].empty())
	{
		vector<struct memwatch>::iterator it = _watchers[NetId].begin();
		vector<struct memwatch>::const_iterator it_end = _watchers[NetId].end();
		//check for collisions
		for (;it != it_end; ++it)
		{
			if ((*it).resolution == w.resolution)
			{
				if (make_union((*it).offset,(*it).length , w.offset, w.length))
				{
					_watchers[NetId].erase(it);
					if (_watchers[NetId].empty())
						break;
					it = _watchers[NetId].begin();
					it_end  = _watchers[NetId].end();

				}
			}
			else
			{
			//TODO:
			}
		}
	}
	_watchers[NetId].push_back(w);
	SC_UNLOCK_MUTEX(_watchersMutex);
	return 0;
}
extern "C" int64_t getTime(); // microseconds resolution
static inline uint32_t swpbl(uint32_t x) //swap bytes of long
{
	//TODO: replace with a suitable builtin
	return ((x & 0x000000FF) << 24) | ((x & 0x0000FF00) << 8)
			| ((x & 0x00FF0000) >> 8) | ((x & 0xFF000000) >> 24);
}
static inline uint16_t swpbs(uint16_t x) //swap bytes of short
{
	//TODO: replace with a suitable builtin
	return ((x & 0x00FF) << 8) | ((x & 0xFF00) >> 8);
}
static inline void swpbl_a(uint32_t* x,int n) //swap bytes of long
{
	for (int i = 0 ; i < n; ++i)
	{
		*x = swpbl(*x);
		++x;
	}
}
static inline void swpbs_a(uint16_t* x,int n) //swap bytes of long
{
	for (int i = 0 ; i < n; ++i)
	{
		*x = swpbs(*x);
		++x;
	}
}

int ScramcastMainServer::handle_packet(struct SC_Packet* packet_buf, int32_t packet_len, int fix_endianess)
{
	if (fix_endianess)
	{
		packet_buf->magic = swpbl(packet_buf->magic);
		packet_buf->net = swpbs(packet_buf->net);
		packet_buf->host = swpbs(packet_buf->host);
		packet_buf->timetag = swpbl(packet_buf->timetag);
		packet_buf->msg_id = swpbl(packet_buf->msg_id);
		packet_buf->address = swpbl(packet_buf->address);
		packet_buf->length = swpbl(packet_buf->length);
	}
	uint32_t offset = packet_buf->address;//  for memcpy
	uint32_t length = packet_buf->length;//  for memcpy
	uint32_t flags = packet_buf->magic & (~MAGIC_MASK);
	uint8_t * data= packet_buf->data;
	ScramcastMemory * mem = ScramcastMemory::getMemoryByNet(packet_buf->net);
	if (mem == NULL)
		return -1;
	bool host_in_range = (packet_buf->host < MAX_HOSTS) && (packet_buf->host >=0);
	bool offset_length_in_range = (offset < MAX_MEMORY) &&
			((offset+length) <= MAX_MEMORY) && (length <= MAX_PACKET_DATA_LEN) && (length > 0);
	bool received_match_length = (packet_len == (int32_t)(sizeof( struct SC_Packet) - MAX_PACKET_DATA_LEN + length));
	bool flags_ok = ((flags&(MAGIC_32BIT|MAGIC_16BIT)) != (MAGIC_32BIT|MAGIC_16BIT));
	if (host_in_range && offset_length_in_range && received_match_length && flags_ok)
	{
		if (fix_endianess)
		{
			if (packet_buf->magic & MAGIC_32BIT)
			{
				if ( (length % 4) != 0 )
				{
					fprintf(stderr,TERM_RED_COLOR 
					"received malformed package from host %d on network %d, offset %u, length %u. length should be divadable by 4\n"
					TERM_RESET, (int)packet_buf->host, (int)packet_buf->net, packet_buf->address, length);
					return -1;
				}
				swpbl_a((uint32_t*)data,length/4);
			}
			else if (packet_buf->magic & MAGIC_16BIT)
			{
				if ( (length % 2) != 0 )
				{
					fprintf(stderr,TERM_RED_COLOR 
					"received malformed package from host %d on network %d, offset %u, length %u. length should be divadable by 2\n"
					TERM_RESET, (int)packet_buf->host, (int)packet_buf->net, packet_buf->address, length);
					return -1;
				}
				swpbs_a((uint16_t*)data,length/2);
			}
		}
		memcpy((uint8_t*)mem->getBaseMemory() + offset, data, length);
		memcpy((uint8_t*)mem->getShadowMemory() + offset, data, length);
		return 0;
	}
	else
	{
		fprintf(stderr,TERM_RED_COLOR "received bad package from host %d on network %d, offset %u, length %u\n" TERM_RESET, (int)packet_buf->host, (int)packet_buf->net,
					packet_buf->address, packet_buf->length);
		if (!host_in_range)
			fprintf(stderr,TERM_RED_COLOR "reason: host not in range. host is: %d\n" TERM_RESET, (int)packet_buf->host);
		if (!offset_length_in_range)
			fprintf(stderr,TERM_RED_COLOR "reason: bad offset or length. offset is: %u, length is: %u, MAX_MEMORY %d\n" TERM_RESET, packet_buf->address, packet_buf->length, (int)MAX_MEMORY);
		if (!received_match_length)
			fprintf(stderr,TERM_RED_COLOR "reason: length of packet unmatches length of data. received bytes is %u, length is: %u\n" TERM_RESET, packet_len, packet_buf->length);
		if (!flags_ok)
			fprintf(stderr,TERM_RED_COLOR "reason: bad combination of flags. flags is: %08X\n" TERM_RESET, flags);
		return -1;
	}

	return 0;
}

static struct SC_Packet packet_buf;
int32_t ScramcastMainServer::read_packets()
{
	int recv_flags = 0 /*MSG_OOB*/;
	int fix_endian;
	int32_t bytes_received;
	struct sockaddr_in sockname;
	socklen_t sockname_size;
	while(1)
	{
		sockname_size = sizeof (struct sockaddr_in);
		bytes_received = recvfrom ( _bcastRecvSock, (char*) &packet_buf, sizeof(struct SC_Packet),
				recv_flags, (struct sockaddr *)&sockname, &sockname_size );
#ifdef __POSIX__
#define SOCKET_GET_ERROR (errno)
#define WOULDBLOCK_ERROR EWOULDBLOCK
#else
#define SOCKET_GET_ERROR (WSAGetLastError())
#define WOULDBLOCK_ERROR WSAEWOULDBLOCK
#endif
		if (bytes_received == -1 && SOCKET_GET_ERROR == WOULDBLOCK_ERROR)
		{
			break;
		}
		if (bytes_received == -1)
		{
			perror("recvfrom");
			return -1;
		}
#ifdef DEBUG
		printf("bytes_received %d\n",bytes_received);
#endif
		if (bytes_received < (int32_t)(sizeof( struct SC_Packet) - MAX_PACKET_DATA_LEN))
		{
			continue;
		}
		bool good_magic_correct_endian = ((packet_buf.magic & MAGIC_MASK) == MAGIC_KEY);
		bool from_me = 	good_magic_correct_endian && (_hostId == packet_buf.host);
		fix_endian = (swpbl(packet_buf.magic) & MAGIC_MASK) == MAGIC_KEY;
		if (fix_endian || good_magic_correct_endian )
		{
			if  (from_me)
			{
				DINCOME(TERM_YELLOW_COLOR "(self) ");
			}
			else
			{
				DINCOME(TERM_GREEN_COLOR);
				handle_packet(&packet_buf,bytes_received,fix_endian);
			}

			if(scramcast_dbg_lvl & LVL_INCOME ) {
			uint32_t mask = ((int)(packet_buf.length>=4))*0xffffffff|
					((int)(packet_buf.length==3))*0x00ffffff|
					((int)(packet_buf.length==2))*0x0000ffff|
					((int)(packet_buf.length==1))*0x000000ff;
			DINCOME("magic:0x%08x host:%d net:%d, length:%d, address:%d first dword: %08x (%u)\n" TERM_RESET,packet_buf.magic,
																packet_buf.host,packet_buf.net,packet_buf.length,packet_buf.address,
																mask&((uint32_t*)packet_buf.data)[0],mask&((uint32_t*)packet_buf.data)[0]);
			fflush(stdout);
			}
		}
	}
	return 0;

}

int ScramcastMainServer::set_fd_set(fd_set* read_fds)
{
	FD_ZERO ( read_fds);
	FD_SET (_srvrSock, read_fds);
	FD_SET (_bcastRecvSock, read_fds);
	vector<int>::const_iterator it;
	vector<int>::const_iterator it_end = connections.end();
	int max_fd = max(_srvrSock , _bcastRecvSock);
	for (it = connections.begin(); it != it_end ; ++it)
	{
		FD_SET (*it, read_fds);
		if (*it > max_fd)
			max_fd = *it;
	}
	return max_fd + 1;
}
int32_t ScramcastMainServer::accept_connections()
{
	/* Connection request on original socket. */
	int new_conn;
	while (1)
	{
		new_conn = accept (_srvrSock, 0, 0);
#ifdef __POSIX__
		if (new_conn < 0 && errno == EWOULDBLOCK)
		{
			break;
		}
#endif
#ifdef __WINDOWS__
		if (new_conn < 0 && WSAGetLastError() == WSAEWOULDBLOCK)
		{
			break;
		}
#endif
		if (new_conn < 0 )
		{
			perror ("accept");
			return -1;
		}
		else
		{
			connections.push_back(new_conn);
			printf(TERM_DARK_BLUE_COLOR "got connection %d\n" TERM_RESET,new_conn);
		}
	}
	return 0;
}
int32_t ScramcastMainServer::run_memwatch(u_int8_t NetId,struct memwatch watch)
{
	ScramcastMemory * mem = ScramcastMemory::getMemoryByNet(NetId);
	if (mem == NULL)
		return -1;
	void * memreal = mem->getBaseMemory();
	void * memshad = mem->getShadowMemory();
	if (watch.resolution == 32)
	{
		uint32_t offset = watch.offset/4;//round to 4 bytes
		uint32_t length = watch.length/4;//round to 4 bytes
		uint32_t offset_end = offset + length;
		//uint32_t buffer[MAX_PACKET_DATA_LEN/4];
		uint32_t temp;
		uint32_t buf_off = 0;

#define R ((uint32_t*)memreal)
#define S ((uint32_t*)memshad)
		for (; offset < offset_end ; ++offset)
		{
			//buffer[buf_off] = R[offset];
			temp = R[offset];
			if (buf_off == (MAX_PACKET_DATA_LEN/4)-1 || (temp == S[offset] && buf_off > 0))
			{
				postMemory(NetId,(offset-buf_off)*4,buf_off*4);
				buf_off=0;
			}
			if (temp != S[offset])
			{
				++buf_off;
			}


		}
		if (buf_off > 0)
		{
			postMemory(NetId,(offset-buf_off)*4,buf_off*4);
			buf_off = 0; //unuseful but for safety if one copies this if. good compiler will ignore that
		}
#undef R
#undef S
	}
	else if (watch.resolution == 8)
	{
		uint32_t offset = watch.offset;
		uint32_t length = watch.length;
		uint32_t offset_end = offset + length;
		//uint32_t buffer[MAX_PACKET_DATA_LEN/4];
		uint32_t buf_off = 0;
		uint8_t temp;

#define R ((uint8_t*)memreal)
#define S ((uint8_t*)memshad)
		for (; offset < offset_end ; ++offset)
		{
			//buffer[buf_off] = R[offset];
			temp = R[offset];
			if (buf_off == (MAX_PACKET_DATA_LEN)-1 || (temp == S[offset] && buf_off > 0))
			{
				postMemory(NetId,(offset-buf_off),buf_off);
				buf_off=0;
			}
			if (temp != S[offset])
			{
				++buf_off;
			}


		}
		if (buf_off > 0)
		{
			postMemory(NetId,(offset-buf_off),buf_off);
			buf_off = 0; //unuseful but for safety if one copies this if.
		}
#undef R
#undef S
	}

	return 0;
}
int32_t ScramcastMainServer::memory_check()
{
	SC_LOCK_MUTEX(_watchersMutex);
	for (int net=0; net < MAX_NETWORKS ; ++net)
	{
		if (!_watchers[net].empty())
		{
			vector<struct memwatch>::const_iterator it= _watchers[net].begin();
			vector<struct memwatch>::const_iterator it_end= _watchers[net].end();
			for (;it != it_end; ++it)
			{
				run_memwatch(net,(struct memwatch)*it);
			}
		}
	}
	SC_UNLOCK_MUTEX(_watchersMutex);
	return 0;
}
int32_t ScramcastMainServer::remove_connection(int fd)
{
	closesocket(fd);
	std::vector<int>::iterator position = find(connections.begin(),connections.end(),fd);
	if (position != connections.end())
		connections.erase(position);
	return 0;
}
int32_t ScramcastMainServer::read_command(int fd)
{
	char buf[256];
	struct memwatch membuf;
	int ret = recv(fd,buf,4,0);
	if (ret == 0)
	{
		DINFO("removing connection %d\n",fd);
		remove_connection(fd);
	}
	int cmd=buf[0];
	uint32_t netid=buf[1];
	switch (cmd)
	{
	case 'a': // add watch
		if (recv(fd,(char*)&membuf,sizeof(struct memwatch),0)!= sizeof(struct memwatch))
			return -1;
		DINFO("got memory watch command %u %u\n",membuf.offset,membuf.length);
		AddMemoryWatch(netid,membuf.offset,
				membuf.length,
				membuf.resolution);
		break;
	}
	return 0;
}

#ifdef WIN_THREADS
DWORD WINAPI ScramcastMainServer::server_listen(LPVOID args)
#else
void* ScramcastMainServer::server_listen(void* args)
#endif
{
#undef CARGS
#define CARGS (*(ScramcastMainServer*)args)
	fd_set read_fd_set,sel_read_fd_set;
	struct timeval timeout;
	int64_t time_elapsed = 0;

	int select_result;
	/* Initialize the timeout data structure. */

	while (CARGS._join == SC_FALSE)
	{
		int nfds = CARGS.set_fd_set(&read_fd_set);
		sel_read_fd_set = read_fd_set;
		timeout.tv_sec = 0;
		timeout.tv_usec = (long)(4000 - time_elapsed);//4ms
		int64_t tdiff = getTime();
		select_result = select(nfds,&sel_read_fd_set, NULL, NULL,&timeout);


		if (select_result > 0)
		{
			if (FD_ISSET(CARGS._bcastRecvSock, &sel_read_fd_set))
			{
				CARGS.read_packets();
				--select_result;
				FD_CLR(CARGS._bcastRecvSock, &sel_read_fd_set);
			}
			if (FD_ISSET(CARGS._srvrSock, &sel_read_fd_set))
			{
				CARGS.accept_connections();
				--select_result;
				FD_CLR(CARGS._srvrSock, &sel_read_fd_set);
			}
			for (int i = 0 ; (i < nfds) && (select_result > 0) ; ++i)
			{
				if (FD_ISSET(i, &sel_read_fd_set))
				{
					CARGS.read_command(i);
					select_result--;
					FD_CLR(i, &sel_read_fd_set);
				}
			}
		}
		else
		{
#ifdef DEBUG_ETC
			printf("empty select\n");
			fflush(stdout);
#endif
		}
		time_elapsed += (getTime()-tdiff);
		//printf("time_elapsed %lu %ld\n",time_elapsed,getTime());
		if (time_elapsed > 3000 || time_elapsed < 0) //3ms
		{
#ifdef DEBUG_ETC
			printf("run memory_check\n");
			fflush(stdout);
#endif
			time_elapsed = 0;
			CARGS.memory_check();
		}
	}
	return 0;
#undef CARGS
}


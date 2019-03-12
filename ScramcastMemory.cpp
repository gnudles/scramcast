/*
 * ScramcastMemory.cpp
 *
 *  Created on: Jul 2, 2018
 *      Author: Orr Dvory
 */

#include "ScramcastMemory.h"
#include "ScramcastServer.h"
#include "netconfig.h"
#include <stdio.h>
#define MEM_REAL_PLUS_SHADOW MAX_MEMORY*2 // multiply by 2 - first is the real memory, afterwards there's the shadow memory

ScramcastMemory* ScramcastMemory::_mappings[MAX_NETWORKS]={0};
SCSharedMemory ScramcastMemory::_countersShmem=SCSharedMemory("scramcast_counters",sizeof (struct counters_page));

SCSharedMemory::SCSharedMemory(const char* key, u_int32_t length):_key(key),_length(length)
{
#ifdef __POSIX__
	_shm_fd = shm_open(key,O_RDWR|O_CREAT,0666);
	if (_shm_fd < 0)
	{
		perror(TERM_RED_COLOR "shm_open" TERM_RESET);
		_pBuf = NULL;
		return;
	}
	int ret = ftruncate(_shm_fd,length);
	if (ret != 0)
	{
		perror(TERM_RED_COLOR "ftruncate" TERM_RESET);
		close(_shm_fd);
		_shm_fd = -1;
		shm_unlink(key);
		_pBuf = NULL;
		return;
	}
	_pBuf = mmap(0,length,PROT_READ|PROT_WRITE,MAP_SHARED,_shm_fd,0);
	if (_pBuf == MAP_FAILED)
	{
		perror(TERM_RED_COLOR "mmap" TERM_RESET);
		close(_shm_fd);
		_shm_fd = -1;
		shm_unlink(key);
		return;
	}
#endif
#ifdef __WINDOWS__
	_hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, TRUE, (LPCSTR)key);

			// if it is not allocated allocate it
			if (_hMapFile == NULL) {
				_hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, length, (LPCSTR)key);
				if (_hMapFile == NULL)
				{
					fprintf(stderr, TERM_RED_COLOR "Could not create file mapping object (%lu).\n" TERM_RESET,GetLastError());
					_pBuf = 0;
					return;
				}
			}

			// return a pointer to the shm
			_pBuf = (LPVOID)MapViewOfFile(_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, length);
			if (_pBuf == NULL)
			{
				fprintf(stderr,TERM_RED_COLOR "Could not map view of file (%lu).\n" TERM_RESET,GetLastError());
				CloseHandle(_hMapFile);
				_hMapFile = NULL;
				return;
			}
#endif
}
SCSharedMemory::~SCSharedMemory() {
#ifdef __POSIX__
	if (_shm_fd >= 0)
	{
		close(_shm_fd);
	}
	if (_pBuf != NULL && _pBuf != MAP_FAILED)
	{
		munmap(_pBuf,_length);
	}
	//shm_unlink(_key); //We don't unlink because we want to keep those objects.
#endif
#ifdef __WINDOWS__
	if (_pBuf != NULL )
	{
		UnmapViewOfFile(_pBuf);
	}
	if (_hMapFile != NULL)
	{
		CloseHandle(_hMapFile);
	}
#endif
}
#ifdef __WINDOWS__
#define SPRINTF_S sprintf_s
#else
#define SPRINTF_S snprintf
#endif
char* ScramcastMemory::createNetKeyName(u_int32_t NetworkInterface,u_int32_t NetId, char *shmFileName)
{
	assert (MAX_NETWORKS <= 1000);
#ifdef __POSIX__
const char* SC_SHMEM_FORMAT = "/scramcast_net_%08X_%03d";
#endif
#ifdef __WINDOWS__
const char* SC_SHMEM_FORMAT = "scramcast_net_%08X_%03d";
#endif

	SPRINTF_S (shmFileName,256,SC_SHMEM_FORMAT,(u_int32_t)NetworkInterface,(u_int32_t)NetId);
	return shmFileName;
}
ScramcastMemory::ScramcastMemory(u_int32_t NetId):_shmem(createNetKeyName(ScramcastServer::getSystemIP(NETMASK,DEFAULT_NET).ipv4,NetId,_shmFileName),MEM_REAL_PLUS_SHADOW) {

}

ScramcastMemory::~ScramcastMemory() {

}
ScramcastMemory* ScramcastMemory::getMemoryByNet(u_int32_t NetId)
{
	assert(MAX_NETWORKS < 256);
	if (NetId >= MAX_NETWORKS)
		return NULL;
	ScramcastMemory* mem = _mappings[NetId];
	if (mem == NULL)
	{
		mem = _mappings[NetId] = new ScramcastMemory(NetId);
		if (mem->getBaseMemory() == NULL)
		{
			_mappings[NetId] = NULL;
			delete mem;
			return NULL;
		}
	}
	return mem;
}
void ScramcastMemory::destroyMemory(u_int32_t NetId)
{
	if (NetId < MAX_NETWORKS)
	{
		if (_mappings[NetId] != NULL)
		{
			delete _mappings[NetId];
			_mappings[NetId] = NULL;
		}
	}
}
void ScramcastMemory::destroyAllMemories()
{
	for (u_int32_t i = 0 ; i < MAX_NETWORKS ; ++i)
	{
		destroyMemory(i);
	}
}
u_int32_t ScramcastMemory::fetchIncCounter(u_int32_t NetId)
{
	u_int32_t *counter =&(((struct counters_page*)_countersShmem.getAddress())->c[NetId]);
#ifdef __WINDOWS__
#ifdef MSVC6
	u_int32_t val = (u_int32_t)InterlockedIncrement((long*)counter);
#else
	u_int32_t val = InterlockedIncrement(counter);
#endif
	return val-1;
#endif
#ifdef __POSIX__
	u_int32_t val = __atomic_fetch_add(counter,1,__ATOMIC_SEQ_CST);
	return val;
#endif

}


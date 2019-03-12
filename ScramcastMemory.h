/*
 * ScramcastMemory.h
 *
 *  Created on: Jul 2, 2018
 *      Author: Orr Dvory
 */

#ifndef SCRAMCASTMEMORY_H_
#define SCRAMCASTMEMORY_H_

#define SHAREDMEM
#define NETWORKING
#include "sc_defines.h"
struct counters_page
{
	u_int32_t c[MAX_NETWORKS];
};
class SCSharedMemory
{
  public:
	SCSharedMemory(const char *key, u_int32_t length);
	~SCSharedMemory();
	inline void *getAddress() { return _pBuf; }

  private:
	const char *_key;
	u_int32_t _length;
#ifdef __WINDOWS__
	HANDLE _hMapFile;
	LPVOID _pBuf;
#endif
#ifdef __POSIX__
	int _shm_fd;
	void *_pBuf;
#endif
};
class ScramcastMemory
{
  public:
	static u_int32_t fetchIncCounter(u_int32_t NetId);
	static ScramcastMemory *getMemoryByNet(u_int32_t NetId);
	static void destroyMemory(u_int32_t NetId);
	static void destroyAllMemories();
	void *getBaseMemory() { return _shmem.getAddress(); }
	void *getShadowMemory() { return ((char *)_shmem.getAddress()) + MAX_MEMORY; }

  private:
	static char *createNetKeyName(u_int32_t NetworkInterface, u_int32_t NetId, char *shmFileName);
	ScramcastMemory(u_int32_t NetId);
	virtual ~ScramcastMemory();

	char _shmFileName[256];

	static ScramcastMemory *_mappings[MAX_NETWORKS];
	static SCSharedMemory _countersShmem;
	SCSharedMemory _shmem;
};

#endif /* SCRAMCASTMEMORY_H_ */

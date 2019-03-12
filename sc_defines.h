/*
 * sc_defines.h
 *
 *  Created on: Jul 2, 2018
 *      Author: Orr Dvory
 */

#ifndef SC_DEFINES_H_
#define SC_DEFINES_H_

#if defined(__unix__) || defined(__linux) || (defined (__APPLE__) && defined (__MACH__))
#define __POSIX__
#endif
#if defined(WIN32) || (_WIN64 == 1) || (_WIN32 == 1)
#define __WINDOWS__
#endif

#include <assert.h>
#include <stddef.h>
#include <sys/types.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef __WINDOWS__
 #ifdef MSVC6
  #include <winsock.h>
 typedef int socklen_t;
 #else
  #include <winsock2.h>
 #endif
#include <windows.h>
#include <winbase.h>
#define WIN_THREADS
#ifndef MSVC6
#include <stdint.h>
typedef uint32_t u_int32_t;
typedef uint16_t u_int16_t;
typedef uint8_t u_int8_t;
#else
typedef int int32_t;
typedef unsigned int uint32_t;
typedef unsigned int u_int32_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef unsigned short u_int16_t;
typedef char int8_t;
typedef unsigned char uint8_t;
typedef unsigned char u_int8_t;
typedef __int64 int64_t;
#endif
#endif
#ifdef __POSIX__
#include <stdint.h>
#include <sys/time.h>
#endif

#ifdef NETWORKING
#define MAX_PACKET_DATA_LEN 1404
#define SCRAMCAST_PORT 8025
#define SCRAMCAST_LOCAL_PORT 8024

#ifdef __WINDOWS__
#ifndef MSVC6
 #include <ws2tcpip.h>
 #include <ws2ipdef.h>
 #include <In6addr.h>
#endif


#endif
#ifdef __POSIX__
#include <fcntl.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <linux/if_link.h>
#define closesocket(FD) close(FD)
#endif

#endif //NETWORKING


#ifdef SHAREDMEM
#ifdef __POSIX__
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#endif
#ifdef __WINDOWS__
 #ifndef WINDOWS_H
  #define WINDOWS_H
  #include <windows.h>
 #endif
#endif

#endif

#define RESOL_DEFAULT 0
#define RESOL_BIT 1
#define RESOL_NIBBLE 4
#define RESOL_BYTE 8
#define RESOL_WORD 16
#define RESOL_DWORD 32
#define RESOL_QWORD 64

#define MAX_CALLBACKS 20
#define MAX_NETWORKS 12
#define MAX_CLIENTS 16
#define MAX_HOSTS 255
#define KILOBYTE (1024)
#define MEGABYTE (1024*KILOBYTE)

#define MAX_MEMORY (8*MEGABYTE)
#define MAX_PACKET_DATA_LEN 1404
#define MAGIC_KEY 0x4DADE
#define MAGIC_MASK 0x000FFFFF
#ifdef __POSIX__
#define TERM_UNDERLINE         "\033[04m"
#define TERM_RED_COLOR         "\033[031m"
#define TERM_GREEN_COLOR       "\033[032m"
#define TERM_YELLOW_COLOR      "\033[033m"
#define TERM_DARK_BLUE_COLOR   "\033[034m"
#define TERM_PURPLE_COLOR      "\033[035m"
#define TERM_LIGHT_BLUE_COLOR  "\033[036m"
#define TERM_RESET             "\033[0m"
#else
#define TERM_UNDERLINE         ""
#define TERM_RED_COLOR         ""
#define TERM_GREEN_COLOR       ""
#define TERM_YELLOW_COLOR      ""
#define TERM_DARK_BLUE_COLOR   ""
#define TERM_PURPLE_COLOR      ""
#define TERM_LIGHT_BLUE_COLOR  ""
#define TERM_RESET             ""
#endif


struct SC_Packet
{
	uint32_t magic; //should always be 318174 (0x4DADE) if written in same endianess.
	//if not, it should be flipped. the upper 3 nibbles used for storing preferences:
	//masks:
	//magic - 0x000FFFFF
	//T/R (0-Transmit) - 0x00100000
	//32bit alignment  - 0x00200000 if data is organized in long words.
	//16bit alignment  - 0x00400000 if data is organized in words.
	uint16_t net;
	uint16_t host;
	uint32_t timetag; // 1/1000 of a second.
	uint32_t msg_id; //
	uint32_t address;
	uint32_t length;
	uint8_t data[MAX_PACKET_DATA_LEN];
};
#define SC_TRUE 1
#define SC_FALSE 0
/* atomic operations */
#ifdef __POSIX__
	#define ATOMIC_FETCH_INC(FI_LPTR) __atomic_fetch_add(FI_LPTR,1,__ATOMIC_SEQ_CST)
	#define ATOMIC_STORE(S_LPTR,S_VAL) __atomic_store_n(S_LPTR, S_VAL, __ATOMIC_RELAXED)
	#define ATOMIC_LOAD(S_LPTR) __atomic_load_n(S_LPTR, __ATOMIC_RELAXED)
#elif defined (__WINDOWS__)
#ifdef MSVC6
	#define ATOMIC_FETCH_INC(FI_LPTR) ((u_int32_t)InterlockedIncrement((long*)FI_LPTR)-1)
	#define ATOMIC_STORE(S_LPTR,S_VAL) (*(S_LPTR) = S_VAL)
	#define ATOMIC_LOAD(S_LPTR) (*(S_LPTR))
#else
	#define ATOMIC_FETCH_INC(FI_LPTR) (InterlockedIncrement(FI_LPTR)-1)
	#define ATOMIC_STORE(S_LPTR,S_VAL) (*(S_LPTR) = S_VAL)
	#define ATOMIC_LOAD(S_LPTR) (*(S_LPTR))
#endif
#endif


/*end of atomic operations */
#ifdef __WINDOWS__
#define SC_MUTEX_T HANDLE
inline SC_MUTEX_T SC_CREATE_MUTEX()
{
	SC_MUTEX_T m = CreateMutex(NULL,FALSE,NULL);
	if (m == NULL)
	{
		printf("CreateMutex failed: %lu",GetLastError());
	}
	return m;
}
inline void SC_LOCK_MUTEX( SC_MUTEX_T m )
{
	if (WaitForSingleObject(m,INFINITE) != WAIT_OBJECT_0)
	{
		printf("fixme: mutex lock error\n");
		return;
	}
}
inline void SC_UNLOCK_MUTEX( SC_MUTEX_T m )
{
	ReleaseMutex(m);
}
inline void SC_DESTROY_MUTEX( SC_MUTEX_T m )
{
	CloseHandle(m);
}
#endif
#ifdef __POSIX__
#include <pthread.h>
#define SC_MUTEX_T pthread_mutex_t
inline SC_MUTEX_T SC_CREATE_MUTEX()
{
	SC_MUTEX_T m;
	pthread_mutex_init(&m,0);
	return m;
}
inline void SC_LOCK_MUTEX( SC_MUTEX_T& m )
{
	pthread_mutex_lock(&m);
}
inline void SC_UNLOCK_MUTEX( SC_MUTEX_T& m )
{
	pthread_mutex_unlock(&m);
}
inline void SC_DESTROY_MUTEX( SC_MUTEX_T& m )
{
	pthread_mutex_destroy(&m);
}
#endif

#define LVL_FATAL 0x00000001
#define LVL_SEVERE 0x00000002
#define LVL_INFO 0x00000004
#define LVL_INCOME 0x00000008
#define LVL_OUTGOING 0x00000010

extern int scramcast_dbg_lvl;
//__LINE__ __FILE__ __func__
#define DFATAL(x) do { if (scramcast_dbg_lvl & LVL_FATAL)  fprintf(stderr, "%s:%d (%s): %s",__FILE__,__LINE__,__func__,x);} while (0);
#define DSEVERE(...) do { if (scramcast_dbg_lvl & LVL_SEVERE)  fprintf(stderr,__VA_ARGS__);} while (0);
#define DINFO(...) do { if (scramcast_dbg_lvl & LVL_INFO)  fprintf(stderr,__VA_ARGS__);} while (0);
#define DINCOME(...) do { if (scramcast_dbg_lvl & LVL_INCOME)  fprintf(stderr,__VA_ARGS__);} while (0);

#endif /* SC_DEFINES_H_ */

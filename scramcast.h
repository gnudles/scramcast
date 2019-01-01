/*
 * scramcast.h
 *
 *  Created on: Jul 2, 2018
 *      Author: Orr Dvory
 */

#ifndef SCRAMCAST2_H_
#define SCRAMCAST2_H_

#include <stddef.h>
#include <sys/types.h>
#if defined(WIN32) || (_WIN64 == 1) || (_WIN32 == 1)
#define __WINDOWS__
#endif

#ifdef __WINDOWS__
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
#else
#include <stdint.h>
#endif
#ifdef __cplusplus
#define CEXPORT extern "C"
#else
#define CEXPORT
#endif
#ifdef SCRAM_LIB_BUILD
#include "ScramcastServer.h"
typedef ScramcastServer* ScramcastServerPtr;
#else
typedef void* ScramcastServerPtr;
#endif

#define SERV_ANY 3
#define SERV_MAIN 1
#define SERV_SLAV 2
#define SCRAMCAST_MEM_SIZE (8*1024*1024)
CEXPORT ScramcastServerPtr SC_createServer(u_int32_t server_type);
/*CEXPORT int32_t SC_startServer(ScramcastServerPtr sc_server);
CEXPORT int32_t SC_stopServer(ScramcastServerPtr sc_server);*/
CEXPORT int32_t SC_destroyServer(ScramcastServerPtr sc_server);
CEXPORT int32_t SC_destroyMemories();
CEXPORT void * SC_getBaseMemory(u_int32_t net_id);
CEXPORT u_int32_t SC_getMemoryLength();
//return number of byte that was sent.
CEXPORT u_int32_t SC_postMemory(ScramcastServerPtr sc_server, u_int8_t net_id, u_int32_t offset, u_int32_t length);
CEXPORT int32_t SC_addMemoryWatch(ScramcastServerPtr sc_server, u_int8_t net_id, u_int32_t offset, u_int32_t length, u_int8_t resolution);


#endif /* SCRAMCAST2_H_ */

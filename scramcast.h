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

/**
 * \brief Creates a scramcast server.
 * There are 2 types of servers: slave and primary. Each computer on a the network can have only one primary server.
 * The primary server listens to incoming messages,
 * and the slave server is used for sending outgoing messages and for being informed by the primary server.
 *	\param server_type specifies the type of the server to create.
 *      The value can be either SERV_MAIN, SERV_SLAV or SERV_ANY.
 *	\return The pointer to the server.
 *	\sa SC_destroyServer()
**/
CEXPORT ScramcastServerPtr SC_createServer(u_int32_t server_type);
/*CEXPORT int32_t SC_startServer(ScramcastServerPtr sc_server);
CEXPORT int32_t SC_stopServer(ScramcastServerPtr sc_server);*/
CEXPORT int32_t SC_destroyServer(ScramcastServerPtr sc_server);
CEXPORT int32_t SC_destroyMemories();
CEXPORT void * SC_getBaseMemory(u_int32_t net_id);
CEXPORT u_int32_t SC_getMemoryLength();
//CEXPORT u_int32_t SC_collectMemory(ScramcastServerPtr sc_server, u_int32_t net_id, u_int32_t offset, u_int32_t length, u_int32_t resolution);
//return number of byte that was sent.
CEXPORT u_int32_t SC_postMemory(ScramcastServerPtr sc_server, u_int32_t net_id, u_int32_t offset, u_int32_t length);
CEXPORT u_int32_t SC_postMemoryExt(ScramcastServerPtr sc_server, u_int32_t net_id, u_int32_t offset, u_int32_t length, u_int32_t resolution); //resolution = 8,16,32

CEXPORT int32_t SC_addMemoryWatch(ScramcastServerPtr sc_server, u_int32_t net_id, u_int32_t offset, u_int32_t length, u_int32_t resolution); //resolution = 8,16,32
CEXPORT void SC_setDebugLevel(int dbg_lvl);

#endif /* SCRAMCAST2_H_ */

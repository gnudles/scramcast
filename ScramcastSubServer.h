/*
 * ScramcastSubServer.h
 *
 *  Created on: Jul 2, 2018
 *      Author: Orr Dvory
 */

#ifndef SCRAMCASTSUBSERVER_H_
#define SCRAMCASTSUBSERVER_H_

#include "ScramcastServer.h"

class ScramcastSubServer: public ScramcastServer {
public:
	ScramcastSubServer(int tcp_sock, int scram_sock, int hostId);
	virtual ~ScramcastSubServer();
	virtual int32_t AddMemoryWatch(u_int8_t NetId, u_int32_t Offset, u_int32_t Length, u_int32_t resolution);
private:
	int _clntSock;
};

#endif /* SCRAMCASTSUBSERVER_H_ */

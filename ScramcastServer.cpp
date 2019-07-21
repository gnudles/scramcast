/*
 * ScramcastServer.cpp
 *
 *  Created on: Jul 2, 2018
 *      Author: Orr Dvory
 */

#include "ScramcastServer.h"
#include "ScramcastMemory.h"
#include <stdio.h>
struct IPv4v6 ScramcastServer::_sysIP={INADDR_NONE, IN6ADDR_ANY_INIT};
ScramcastServer::ScramcastServer(int scram_sock, int hostId):_bcastSock(scram_sock),_hostId(hostId){

}
const struct IPv4v6& ScramcastServer::getSystemIP(uint32_t net_mask, uint32_t net)
{
	struct in6_addr ipv6_any=IN6ADDR_ANY_INIT;
	if ((memcmp(&_sysIP.ipv6 , &ipv6_any, 16) == 0) &&
			_sysIP.ipv4 == INADDR_NONE)
	{
		_sysIP = getRealSystemIP(net_mask,net);
	}
	return _sysIP;
}
void ScramcastServer::overrideGetSystemIP(const struct IPv4v6 & ip)
{
	_sysIP = ip;
}
ScramcastServer::~ScramcastServer() {
}
#ifdef __POSIX__
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_link.h>

struct IPv4v6 ScramcastServer::getRealSystemIP(uint32_t net_mask, uint32_t net)
{

   struct ifaddrs *ifaddr, *ifa;
   int family,n;

   if (getifaddrs(&ifaddr) == -1) {
       perror("getifaddrs");
       exit(EXIT_FAILURE);
   }

   /* Walk through linked list, maintaining head pointer so we
      can free list later */
   struct IPv4v6 ret_addr={.ipv4=INADDR_NONE, .ipv6=IN6ADDR_ANY_INIT};
   uint32_t ret_ipv4;

   for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
       if (ifa->ifa_addr == NULL)
           continue;

       family = ifa->ifa_addr->sa_family;


       /* For an AF_INET* interface address, display the address */

       if (family == AF_INET) {
    	   ret_ipv4 = ntohl(((struct sockaddr_in*)ifa->ifa_addr)->sin_addr.s_addr);
           if (ret_ipv4 != INADDR_LOOPBACK && (ret_addr.ipv4 == INADDR_NONE || ((ret_ipv4 & net_mask) == net)))
        	   ret_addr.ipv4 = ret_ipv4;
       }
       else if (family == AF_INET6) {
            if (memcmp(&((struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr , &in6addr_loopback, 16) != 0 
               && memcmp(&ret_addr.ipv6 , &in6addr_any, 16) != 0)
            {
                ret_addr.ipv6 = ((struct sockaddr_in6*)ifa->ifa_addr)->sin6_addr;
            }
       }
   }
   freeifaddrs(ifaddr);
   return ret_addr;
}
#elif defined(__WINDOWS__)
#include <iphlpapi.h>


#pragma comment(lib, "IPHLPAPI.lib")


#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))


struct IPv4v6 ScramcastServer::getRealSystemIP(uint32_t net_mask, uint32_t net)
{
	struct in6_addr ipv6_any=IN6ADDR_ANY_INIT;
	struct IPv4v6 ret_addr;
    uint32_t ret_ipv4;
	ret_addr.ipv4=INADDR_NONE;
	ret_addr.ipv6=ipv6_any;
    // Declare and initialize variables

    DWORD dwSize = 0;
    DWORD dwRetVal = 0;

    unsigned int i = 0;

    // Set the flags to pass to GetAdaptersAddresses
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX|GAA_FLAG_SKIP_DNS_SERVER
|GAA_FLAG_SKIP_ANYCAST|GAA_FLAG_INCLUDE_GATEWAYS;

    // default to unspecified address family (both)
    ULONG family = AF_UNSPEC;

    LPVOID lpMsgBuf = NULL;

    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    ULONG outBufLen = 0;
    ULONG Iterations = 0;

    char buff[100];
    DWORD bufflen=100;

    PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
    PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
    PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = NULL;
    PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = NULL;
    IP_ADAPTER_DNS_SERVER_ADDRESS *pDnServer = NULL;
    PIP_ADAPTER_GATEWAY_ADDRESS_LH pGateway = NULL;
    IP_ADAPTER_PREFIX *pPrefix = NULL;

    // Allocate a 15 KB buffer to start with.
    outBufLen = WORKING_BUFFER_SIZE;

    do {

        pAddresses = (IP_ADAPTER_ADDRESSES *) MALLOC(outBufLen);
        if (pAddresses == NULL) {
            printf
                ("Memory allocation failed for IP_ADAPTER_ADDRESSES struct\n");
            exit(1);
        }

        dwRetVal =
            GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);

        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            FREE(pAddresses);
            pAddresses = NULL;
        } else {
            break;
        }

        Iterations++;

    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < MAX_TRIES));

    if (dwRetVal == NO_ERROR) {
        // If successful, output some information from the data we received
        pCurrAddresses = pAddresses;
        while (pCurrAddresses) {
            if ((pCurrAddresses->IfType != IF_TYPE_OTHER &&
                    pCurrAddresses->IfType != IF_TYPE_ETHERNET_CSMACD &&
                    pCurrAddresses->IfType != IF_TYPE_IEEE80211) || pCurrAddresses->FirstGatewayAddress == NULL || pCurrAddresses->FirstMulticastAddress == NULL)
            {
                pCurrAddresses = pCurrAddresses->Next;
                continue;
            }
            //printf("\tLength of the IP_ADAPTER_ADDRESS struct: %ld\n", pCurrAddresses->Length);
            //printf("\tIfIndex (IPv4 interface): %u\n", pCurrAddresses->IfIndex);
            //printf("\tAdapter name: %s\n", pCurrAddresses->AdapterName);

            pUnicast = pCurrAddresses->FirstUnicastAddress;
            if (pUnicast != NULL) {
                for (i = 0; pUnicast != NULL; i++)
                {
                    if (pUnicast->Address.lpSockaddr->sa_family == AF_INET)
                    {
                        sockaddr_in *sa_in = (sockaddr_in *)pUnicast->Address.lpSockaddr;
                        printf("\tIPV4:%s\n",inet_ntop(AF_INET,&(sa_in->sin_addr),buff,bufflen));
                        ret_ipv4=ntohl(sa_in->sin_addr.s_addr);
                        if (ret_ipv4 != INADDR_LOOPBACK && (ret_addr.ipv4 == INADDR_NONE || ((ret_ipv4 & net_mask) == net)))
                            ret_addr.ipv4 = ret_ipv4;
                    }
                    else if (pUnicast->Address.lpSockaddr->sa_family == AF_INET6)
                    {
                        sockaddr_in6 *sa_in6 = (sockaddr_in6 *)pUnicast->Address.lpSockaddr;
                        printf("\tIPV6:%s\n",inet_ntop(AF_INET6,&(sa_in6->sin6_addr),buff,bufflen));
                        ret_addr.ipv6 = sa_in6->sin6_addr;
                    }
                    else
                    {
                        printf("\tUNSPEC");
                    }
                    pUnicast = pUnicast->Next;
                }
                printf("\tNumber of Unicast Addresses: %d\n", i);
            }
            else printf("\tNo Unicast Addresses\n");
            /*pGateway = pCurrAddresses->FirstGatewayAddress;
            if (pGateway)
            {
                for (i = 0; pGateway != NULL; i++)
                {
                    if (pGateway->Address.lpSockaddr->sa_family == AF_INET)
                    {
                        sockaddr_in *sa_in = (sockaddr_in *)pGateway->Address.lpSockaddr;
                        printf("\tIPV4:%s\n",inet_ntop(AF_INET,&(sa_in->sin_addr),buff,bufflen));
                    }
                    else if (pGateway->Address.lpSockaddr->sa_family == AF_INET6)
                    {
                        sockaddr_in6 *sa_in6 = (sockaddr_in6 *)pGateway->Address.lpSockaddr;
                        printf("\tIPV6:%s\n",inet_ntop(AF_INET6,&(sa_in6->sin6_addr),buff,bufflen));
                    }
                    else
                    {
                        printf("\tUNSPEC");
                    }
                    pGateway = pGateway->Next;
                }
                printf("\tNumber of Anycast Addresses: %d\n", i);
            }
            pAnycast = pCurrAddresses->FirstAnycastAddress;
            if (pAnycast) {
                for (i = 0; pAnycast != NULL; i++)
                {
                    if (pAnycast->Address.lpSockaddr->sa_family == AF_INET)
                    {
                        sockaddr_in *sa_in = (sockaddr_in *)pAnycast->Address.lpSockaddr;
                        printf("\tIPV4:%s\n",inet_ntop(AF_INET,&(sa_in->sin_addr),buff,bufflen));
                    }
                    else if (pAnycast->Address.lpSockaddr->sa_family == AF_INET6)
                    {
                        sockaddr_in6 *sa_in6 = (sockaddr_in6 *)pAnycast->Address.lpSockaddr;
                        printf("\tIPV6:%s\n",inet_ntop(AF_INET6,&(sa_in6->sin6_addr),buff,bufflen));
                    }
                    else
                    {
                        printf("\tUNSPEC");
                    }
                    pAnycast = pAnycast->Next;
                }
                printf("\tNumber of Anycast Addresses: %d\n", i);
            } else
                printf("\tNo Anycast Addresses\n");

            pMulticast = pCurrAddresses->FirstMulticastAddress;
            if (pMulticast) {
                for (i = 0; pMulticast != NULL; i++)
                {
                    if (pMulticast->Address.lpSockaddr->sa_family == AF_INET)
                    {
                        sockaddr_in *sa_in = (sockaddr_in *)pMulticast->Address.lpSockaddr;
                        printf("\tIPV4:%s\n",inet_ntop(AF_INET,&(sa_in->sin_addr),buff,bufflen));
                    }
                    else if (pMulticast->Address.lpSockaddr->sa_family == AF_INET6)
                    {
                        sockaddr_in6 *sa_in6 = (sockaddr_in6 *)pMulticast->Address.lpSockaddr;
                        printf("\tIPV6:%s\n",inet_ntop(AF_INET6,&(sa_in6->sin6_addr),buff,bufflen));
                    }
                    else
                    {
                        printf("\tUNSPEC");
                    }
                    pMulticast = pMulticast->Next;
                }
                printf("\tNumber of Multicast Addresses: %d\n", i);
            } else
                printf("\tNo Multicast Addresses\n");

            pDnServer = pCurrAddresses->FirstDnsServerAddress;
            if (pDnServer) {
                for (i = 0; pDnServer != NULL; i++)
                {
                    printf("\tUnicast Address %d: %s\n", i, pDnServer->Address.lpSockaddr->sa_data);
                    pDnServer = pDnServer->Next;
                }
                printf("\tNumber of DNS Server Addresses: %d\n", i);
            } else
                printf("\tNo DNS Server Addresses\n");

            printf("\tDNS Suffix: %wS\n", pCurrAddresses->DnsSuffix);
            printf("\tDescription: %wS\n", pCurrAddresses->Description);
            printf("\tFriendly name: %wS\n", pCurrAddresses->FriendlyName);

            if (pCurrAddresses->PhysicalAddressLength != 0) {
                printf("\tPhysical address: ");
                for (i = 0; i < (int) pCurrAddresses->PhysicalAddressLength;i++)
                {
                        if (i == (pCurrAddresses->PhysicalAddressLength - 1))
                            printf("%.2X\n",
                            (int) pCurrAddresses->PhysicalAddress[i]);
                        else
                            printf("%.2X-",
                            (int) pCurrAddresses->PhysicalAddress[i]);
                }
            }
            printf("\tFlags: %ld\n", pCurrAddresses->Flags);
            printf("\tMtu: %lu\n", pCurrAddresses->Mtu);
            printf("\tIfType: %ld\n", pCurrAddresses->IfType);
            printf("\tOperStatus: %ld\n", pCurrAddresses->OperStatus);
            printf("\tIpv6IfIndex (IPv6 interface): %u\n",
                pCurrAddresses->Ipv6IfIndex);
            printf("\tZoneIndices (hex): ");
            for (i = 0; i < 16; i++)
                printf("%lx ", pCurrAddresses->ZoneIndices[i]);
            printf("\n");

            pPrefix = pCurrAddresses->FirstPrefix;
            if (pPrefix) {
                for (i = 0; pPrefix != NULL; i++)
                    pPrefix = pPrefix->Next;
                printf("\tNumber of IP Adapter Prefix entries: %d\n", i);
            } else
                printf("\tNumber of IP Adapter Prefix entries: 0\n");

            printf("\n");
*/
            pCurrAddresses = pCurrAddresses->Next;
        }
    } else {
        printf("Call to GetAdaptersAddresses failed with error: %lu\n",
            dwRetVal);
        if (dwRetVal == ERROR_NO_DATA)
            printf("\tNo addresses were found for the requested parameters\n");
        else {

            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, dwRetVal, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                // Default language
                (LPTSTR) & lpMsgBuf, 0, NULL)) {
                    printf("\tError: %s", (char*) lpMsgBuf);
                    LocalFree(lpMsgBuf);
                    if (pAddresses)
                        FREE(pAddresses);
                    exit(1);
            }
        }
    }

    if (pAddresses) {
        FREE(pAddresses);
    }

    return ret_addr;
}
#else
struct IPv4v6 ScramcastServer::getRealSystemIP()
{
	struct in6_addr ipv6_any=IN6ADDR_ANY_INIT;
	struct IPv4v6 ret_addr;
	ret_addr.ipv4=INADDR_NONE;
	ret_addr.ipv6=ipv6_any;
	return ret_addr;
}
#endif

int set_nonblock_flag (int desc, int value)
{
#ifdef __POSIX__
	int oldflags = fcntl (desc, F_GETFL, 0);
	/* If reading the flags failed, return error indication now. */
	if (oldflags == -1)
	return -1;
	/* Set just the flag we want to set. */
	if (value != 0)
	oldflags |= O_NONBLOCK;
	else
	oldflags &= ~O_NONBLOCK;
	/* Store modified flag word in the descriptor. */
	return fcntl (desc, F_SETFL, oldflags);
#elif defined(__WINDOWS__)
	u_long v = value;
	return ioctlsocket(desc,FIONBIO,&v);
#endif
}

int ScramcastServer::createDatagramBroadcastSocket(uint32_t bind_ipv4) {
	struct sockaddr_in name;
	int sock;

	/* Create the socket. */
	sock = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
#ifdef __WINDOWS__
		printf (TERM_RED_COLOR "socket: got error %d\n" TERM_RESET,WSAGetLastError());
#else
		perror(TERM_RED_COLOR "socket" TERM_RESET);
#endif
		return SC_SOCKET_ERROR;
	}
	/* Bind a name to the socket. */
	name.sin_family = AF_INET;
	name.sin_port = htons (SCRAMCAST_PORT);
	name.sin_addr.s_addr = htonl (bind_ipv4);
	/* The size of the address is
	 the offset of the start of the filename,
	 plus its length (not including the terminating null byte).
	 Alternatively you can just do:
	 size = SUN LEN (&name);
	 */

	{
		int retval;
		int opt;
		opt = 1;
		retval = setsockopt (sock,SOL_SOCKET,SO_REUSEADDR,(char*)&opt,sizeof(opt));
		if (retval < 0)
		{
#ifdef __WINDOWS__
			printf (TERM_RED_COLOR "setsockopt(SO_REUSEADDR): got error %d\n" TERM_RESET,WSAGetLastError());
#else
			perror(TERM_RED_COLOR "setsockopt(SO_REUSEADDR)" TERM_RESET);
#endif
		}
		//opt = 1;
		//setsockopt (sock,SOL_SOCKET,SO_REUSEPORT,&opt,sizeof(opt));
		opt = 1;
		retval = setsockopt (sock,SOL_SOCKET,SO_BROADCAST,(char*)&opt,sizeof(opt));
		if (retval < 0)
		{
#ifdef __WINDOWS__
			printf (TERM_RED_COLOR "setsockopt(SO_BROADCAST): got error %d\n" TERM_RESET,WSAGetLastError());
#else
			perror(TERM_RED_COLOR "setsockopt(SO_BROADCAST)" TERM_RESET);
#endif
		}
#if 0
		retval = setsockopt (sock,SOL_SOCKET,SO_BINDTODEVICE,(char*)&getSystemIP().ipv4,sizeof(getSystemIP().ipv4));
		if (retval < 0)
		{
#ifdef __WINDOWS__
			printf (TERM_RED_COLOR "setsockopt(SO_BINDTODEVICE): got error %d\n" TERM_RESET,WSAGetLastError());
#else
			perror(TERM_RED_COLOR "setsockopt(SO_BINDTODEVICE)" TERM_RESET);
#endif
		}
#endif
	}
	if (bind(sock, (struct sockaddr *) &name, sizeof(name)) < 0) {
#ifdef __WINDOWS__
		printf (TERM_RED_COLOR "bind: got error %d\n" TERM_RESET,WSAGetLastError());
#else
		perror(TERM_RED_COLOR "bind" TERM_RESET);
#endif
		closesocket(sock);
		return SC_BIND_ERROR;
	}
	set_nonblock_flag(sock,1);
	return sock;
}

int ScramcastServer::createTCPLocalSocket(bool try_to_bind, bool *bind_success) {
	struct sockaddr_in name;
	int sock;
	if (bind_success) *bind_success = false;
	/* Create the socket. */
	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
#ifdef __WINDOWS__
		printf (TERM_RED_COLOR "socket: got error %d\n" TERM_RESET,WSAGetLastError());
#else
		perror(TERM_RED_COLOR "socket" TERM_RESET);
#endif
		return SC_SOCKET_ERROR;
	}
	if (try_to_bind)
	{
		/* Bind a name to the socket. */
		name.sin_family = AF_INET;
		name.sin_port = htons (SCRAMCAST_LOCAL_PORT);
		name.sin_addr.s_addr = htonl (INADDR_ANY);
		if (bind(sock, (struct sockaddr *) &name, sizeof(name)) < 0) {
			/*
	#ifdef __WINDOWS__
			printf (TERM_RED_COLOR "bind: got error %ld\n" TERM_RESET,WSAGetLastError());
	#else
			perror(TERM_RED_COLOR "bind" TERM_RESET);
	#endif
	*/
		}
		else
		{
			set_nonblock_flag(sock,1);
			if (bind_success) *bind_success = true;
		}
	}
	return sock;
}
extern "C" int64_t getTime(); // microseconds resolution
u_int32_t ScramcastServer::sendMemoryRequest()
{
	struct sockaddr_in brdcst;
	brdcst.sin_family = AF_INET;
	brdcst.sin_port = htons (SCRAMCAST_PORT);
	brdcst.sin_addr.s_addr = htonl (INADDR_BROADCAST);
	struct SC_Packet packet_buf;

	packet_buf.net = -1;
	packet_buf.host = _hostId;
	u_int32_t magic_flags = MAGIC_KEY | MAGIC_RECEIVE;
	packet_buf.magic = magic_flags;
	packet_buf.timetag = (uint32_t)(getTime()/1000); // convert microsecond to millisecond.
	packet_buf.msg_id = 0;
	int send_flags = 0;
	ssize_t bytes_sent;
	bytes_sent = sendto(_bcastSock, (const char*) &packet_buf,
				sizeof(packet_buf) - MAX_PACKET_DATA_LEN,
				send_flags, (const struct sockaddr*) &brdcst,
				sizeof(struct sockaddr_in));
	if (bytes_sent != sizeof(packet_buf) - MAX_PACKET_DATA_LEN)
	{
	    DSEVERE("%s: could not post memory on the socket.",__func__);
	}
	return bytes_sent;
}
u_int32_t ScramcastServer::postMemory(u_int32_t NetId, u_int32_t Offset, u_int32_t Length, u_int32_t resolution)
{
    if (NetId >= MAX_NETWORKS)
    {
        DFATAL("Invalid Network ID.");
	    return -1;
    }
	ScramcastMemory *net_mem = ScramcastMemory::getMemoryByNet(NetId);
	if (net_mem == NULL)
		return 0;
	u_int32_t curr_length;
	struct sockaddr_in brdcst;
	brdcst.sin_family = AF_INET;
	brdcst.sin_port = htons (SCRAMCAST_PORT);
	brdcst.sin_addr.s_addr = htonl (INADDR_BROADCAST);
	struct SC_Packet packet_buf;
	if (Length % (resolution/8) != 0 || Offset % (resolution/8) != 0)
	{
	    DFATAL("Alignment error.");
	    return 0;
	}
	if (Offset >= MAX_MEMORY)
	{
		printf (TERM_RED_COLOR "ScramcastServer::postMemory: offset is too large!\n" TERM_RESET);
		return 0;
	}
	else
	if (Length + Offset > MAX_MEMORY)
	{
		Length = MAX_MEMORY - Offset;
	}
	//copy to shadow
	memcpy((uint8_t*) net_mem->getShadowMemory() + Offset, (uint8_t*) net_mem->getBaseMemory() + Offset,
			Length);
	packet_buf.net = NetId;
	packet_buf.host = _hostId;
	u_int32_t magic_flags = MAGIC_KEY;
	magic_flags |= (resolution == 16)? MAGIC_16BIT : 0;
	magic_flags |= (resolution == 32)? MAGIC_32BIT : 0;
	packet_buf.magic = magic_flags;
	packet_buf.timetag = (uint32_t)(getTime()/1000); // convert microsecond to millisecond.
	int send_flags = 0;
	u_int32_t remaining_length = Length;
	ssize_t bytes_sent;
	while (remaining_length > 0) {
		curr_length = remaining_length;
		if (curr_length > MAX_PACKET_DATA_LEN)
			curr_length = MAX_PACKET_DATA_LEN;
		memcpy(packet_buf.data, (uint8_t*) net_mem->getShadowMemory() + Offset,
				curr_length);

		packet_buf.address = Offset;
		packet_buf.length = curr_length;
		packet_buf.msg_id = ScramcastMemory::fetchIncCounter(NetId);
		bytes_sent = sendto(_bcastSock, (const char*) &packet_buf,
				sizeof(packet_buf) - MAX_PACKET_DATA_LEN + curr_length,
				send_flags, (const struct sockaddr*) &brdcst,
				sizeof(struct sockaddr_in));
		if (bytes_sent != (ssize_t) ( sizeof(packet_buf) - MAX_PACKET_DATA_LEN + curr_length ) )
		{
			if (bytes_sent == -1)
			{
#ifdef __WINDOWS__
				DSEVERE("ScramcastServer::postMemory: could not post memory on the socket. (%lu)\n", GetLastError());
#else
				DSEVERE("ScramcastServer::postMemory: could not post memory on the socket. %s (%d)\n", strerror(errno), errno);
#endif
			}
			else
			{
			    DSEVERE("ScramcastServer::postMemory: could not post memory on the socket. %d %d\n", (int32_t)bytes_sent, (int32_t) ( sizeof(packet_buf) - MAX_PACKET_DATA_LEN + curr_length ));
			}
			return bytes_sent;
		}
		Offset += curr_length;
		remaining_length -= curr_length;
#ifdef DEBUG_SEND
		printf("packet sent\n");
		fflush(stdout);
#endif
	}

	return Length;
}


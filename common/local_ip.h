#ifndef LOCAL_IP_H
#define LOCAL_IP_H

#include "log.hpp"
//std
#include <string>
//to get ip
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <arpa/inet.h>

std::string getHostIp(const std::string & hostname)
{
	struct hostent *he;
	struct in_addr **addr_list;
	
	//get entry
	he = gethostbyname( hostname.c_str() );
	if (he == NULL)
	{
		fprintf(stderr,"Get error on gethostbyname : %s",hstrerror(h_errno));
		abort();
	}

	//extract addr
	addr_list = (struct in_addr **) he->h_addr_list;
	
	//convert first one
	char buffer[256];
	strcpy(buffer , inet_ntoa(*addr_list[0]) );

	return buffer;
}

std::string get_local_ip(const std::string & iface)
{
	//locals
	struct ifaddrs * ifAddrStruct=NULL;
	struct ifaddrs * ifa=NULL;
	void * tmpAddrPtr=NULL;
	
	//if iface is empty, search with hostname
	if (iface.empty())
	{
		char hostname[1024];
		gethostname(hostname,sizeof(hostname));
		std::string ip = getHostIp(hostname);
		LOG(DEBUG) << "Get ip for " << hostname << " = " << ip;
		return ip;
	}

	//get addresses
	getifaddrs(&ifAddrStruct);

	//loop to seatch
	for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) 
	{
		//if no address
		if (!ifa->ifa_addr) {
			continue;
		}
		
		//check if requested one
		if (iface == ifa->ifa_name)
		{
			//check type
			if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
				// is a valid IP4 Address
				tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
				char addressBuffer[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
				LOG(DEBUG) << "Get ip for " << iface << " = " << addressBuffer;
				return addressBuffer; 
			} else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
				// is a valid IP6 Address
				tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
				char addressBuffer[INET6_ADDRSTRLEN];
				inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
				LOG(DEBUG) << "Get ip for " << iface << " = " << addressBuffer;
				return addressBuffer;
			}
		}
	}
	
	//error not found
	fprintf(stderr,"Fail to find address of interface %s\n",iface.c_str());
	abort();

	return "";
}

#endif //LOCAL_IP_H

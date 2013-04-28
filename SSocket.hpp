/*
 * SSocket.hpp
 *
 *  Created on: 28.04.2013
 *      Author: julian
 */

#ifndef SSOCKET_HPP_
#define SSOCKET_HPP_


// Network
#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "wsock32.lib");
#else
#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <fcntl.h>
#endif
// General includes
#include <string>
#include <exception>

namespace Socket
{
#ifdef _WIN32
class WSocketStartupError : public std::exception
{
public:
	const char *what()
	{
		return "could not Start WSocket";
	}
};
#endif

void GlobalInit()
{
#ifdef _WIN32
	WSADATA WsaData;
	if (WSAStartup(MAKEWORD(2,2), &WsaData)) != NO_ERROR)
		throw WSocketStartupError();

#endif

}
void GlobalShutdown()
{
#ifdef _WIN32
	WSACleanup();
#endif
}

class CRefcounter
{
protected:
	int Refcounter;
public:
	CRefcounter():
		Refcounter(0)
	{}

	~CRefcounter() {}

	void grab(){
		Refcounter++;
	}
	void drop(){
		Refcounter--;
		if (Refcounter == 0)
			delete this;
	}
};

class SocketCreateException : public std::exception
{
public:
	const char * what() {
		return "Could not create Socket";
	}
};

class SocketBindException : public std::exception
{
	in_port_t port;
public:
	SocketBindException(in_port_t port):
		port(port)
	{}
	const char * what() {
		return "Cannot bind Socket to port!";
	}

	unsigned short getPort() {
		return port;
	}
};

class SocketSwitchBlockingModeError : public std::exception
{
public:
	const char * what()
	{
		return "could not set nonBlocking-Mode";
	}
};

struct Address
{
	sockaddr_storage _address;

	Address():
		_address()
	{}

	Address(int family, const std::string &address, const in_port_t &port = 0)
	{
		_address.ss_family = family;
		inet_pton(family, address.c_str(), (family==AF_INET)?(void*)&((sockaddr_in*)&_address)->sin_addr.s_addr:(void*)&((sockaddr_in6*)&_address)->sin6_addr);

		if(_address.ss_family==AF_INET)
			((sockaddr_in*)&_address)->sin_port = port;
		else
			((sockaddr_in6*)&_address)->sin6_port = port;
	}

	Address(const sockaddr_in6 & addr)
	{
		_address.ss_family = AF_INET6;
		*((sockaddr_in6*)&_address) = addr;
	}

	Address(const sockaddr_in & addr)
	{
		_address.ss_family = AF_INET;
		*((sockaddr_in*)&_address) = addr;
	}

	sockaddr *getAddress()
	{
		return (sockaddr*)&_address;
	}

	socklen_t getAddresslen()
	{
		return (_address.ss_family == AF_INET)?sizeof(sockaddr_in):sizeof(sockaddr_in6);
	}
};

class SocketUDP : public CRefcounter
{
protected:
	int handle;
public:
	SocketUDP(in_port_t port):
		handle(0)
	{
		GlobalInit();

		handle = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

		if (handle == -1)
			throw SocketCreateException();

		sockaddr_in6 address;
		address.sin6_family = AF_INET6;
		address.sin6_addr = in6addr_any;
		address.sin6_port = port;

		if (bind(handle, (sockaddr*)&address, sizeof(address)) == -1)
			throw SocketBindException(port);
	}

	~SocketUDP()
	{
		GlobalShutdown();
	}

	void UnBlock()
	{
#ifdef _WIN32
		DWORD nonBlocking = true;
		if (ioctlsocket(handle, FIONBIO, &nonBlocking) != 0)
			throw SocketSwitchBlockingModeError();
#else
		if (fcntl(handle, F_SETFL, O_NONBLOCK, true) == -1)
			throw SocketSwitchBlockingModeError();
#endif
	}
	void Block()
	{
#ifdef _WIN32
		DWORD nonBlocking = false;
		if (ioctlsocket(handle, FIONBIO, &nonBlocking) != 0)
			throw SocketSwitchBlockingModeError();
#else
		if (fcntl(handle, F_SETFL, O_NONBLOCK, false) == -1)
			throw SocketSwitchBlockingModeError();
#endif
	}
	bool ReceiveFrom(void * buffer, size_t bufferlen,
			Address * from =0)
	{
		std::fill_n((char*)buffer, 0, bufferlen);

		Address From;
		socklen_t size = sizeof (sockaddr_in6);

		int BytesReceived = recvfrom(handle, buffer, bufferlen,
				0, From.getAddress(), &size);

		if (BytesReceived <= 0)
			return false;

		if (from)
		{
			*from = From;
		}
		return true;
	}
	bool SendTo(Address *to, const void * message, size_t message_len)
	{
		size_t bytesSend = sendto(handle, message, message_len,
				0, to->getAddress(), to->getAddresslen());

		if (bytesSend != message_len)
			return false;

		return true;
	}
};

}

#endif /* SSOCKET_HPP_ */

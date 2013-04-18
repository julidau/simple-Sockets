/*
 * main.cpp
 *
 *  Created on: 18.04.2013
 *      Author: julian
 */

// register platform
#define PLATFORM_WINDOWS 0
#define PLATFORM_MAX 1
#define PLATFORM_UNIX 2

#if defined(WIN32)
#define PLATFORM PLATFORM_WINDOWS
#elif defined(__APPLE__)
#define PLATFORM PLATFORM_MAC
#else
#define PLATFORM PLATFORM_UNIX
#endif

// Socket includes
#include <iostream>
#include <string.h>
#if PLATFORM == PLATFORM_LINUX
#include <winsock2.h>		// WINDOWS
#pragma comment(lib, "wsock32.lib");
#else
#include <sys/socket.h>		// MAX && LINUX
#include <netinet/in.h>
#include <fcntl.h>
#endif

class Socket
{
	int handle;
	sockaddr_in address;

public:
	struct Address
	{
		unsigned int addr;
		unsigned short port;
	};


	Socket(short port = 0):
		handle(0)
	{
#if PLATFORM == PLATFORM_WINDOWS
		WSADATA WsaData;
		if (WSAStartup(MAKEWORD(2,2), &WsaData)) != NO_ERROR)
		{
			throw "unable to initialize Socket";
			return;
		}
#endif

		handle = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if (handle <= 0)
		{
			throw "unable to initialize Socket";
			return;
		}

		address.sin_family = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port = htons(port);

		if (bind(handle, (const sockaddr*)&address, sizeof(sockaddr_in)) < 0)
		{
			//std::cerr << "failed to bind socket to port " << port << std::endl;
			throw "failed to bind socket to port";
			return;
		}
	}
	~Socket()
	{
#if PLATFORM == PLATFORM_WINDOWS
		WSACleanup();
#endif
	}

	// set Socket to non-Blocking Mode, so that receive() will not Wait for a
	// package
	void SetBlocking(bool Set = false)
	{
#if PLATFORM == PLATFORM_UNIX || PLATFORM == PLATFORM_MAC
		if (fcntl(handle, F_SETFL, O_NONBLOCK, !Set) == -1)
			throw "Could not set non-Blocking Mode!";

#elif PLATFORM == PLATFORM_WINDOWS
		DWORD nonBlocking = !Set;
		if (ioctlsocket(handle, FIONBIO, &nonBlocking) != 0)
			throw "Could not set non-Blocking Mode!";
#endif
	}

	/*
	 * Send an specified amount of bytes
	 * returns false if sent failed
	 */
	bool SendBytes(const char * bytes, size_t packetSize,  Address to)
	{
		sockaddr_in address;
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = htonl(to.addr);
		address.sin_port = htons(to.port);

		unsigned int Bytes_Sent = sendto(handle, (const char*)bytes, packetSize,
				0, (sockaddr*)&address, sizeof(sockaddr_in));

		if(Bytes_Sent != packetSize)
		{
			return false;
		}

		return true;
	}

	// receives a packet
	// returns false, if no packet in queue
	bool ReceivePacket (Address * From, char * Buffer, size_t BufferSize)
	{
		memset(Buffer, 0, BufferSize);

		sockaddr_in from;
		socklen_t socklen = sizeof(sockaddr_in);

		unsigned int Rec = recvfrom(handle, Buffer, BufferSize,
				0, (sockaddr*)&from,&socklen);

		if (Rec <= 0)
			return false;

		From->addr = htonl(from.sin_addr.s_addr);
		From->port = htons(from.sin_port);

		return true;
	}

};

#include <iostream>

using namespace std;
bool Server;

#include <string.h>

int main(int argc, char ** args)
{
	if (argc == 1)
		Server = true;
	else
		Server = false;

	cout << PLATFORM << endl;

	try
	{


		if (!Server)
		{
			Socket sock;

			Socket::Address addr;
			addr.port = 3000;
			addr.addr = (127 << 24) | (1);

			while(true)
			{
				string in;
				cin >> in;

				if (!sock.SendBytes(in.c_str(), (size_t)in.size(), addr))
					return -1;
			}
		} else
		{
			Socket sock(3000);
			Socket::Address from;

			while(true)
			{
				char * buffer = new char[256];

				if (sock.ReceivePacket(&from, buffer, 256))
				{
					// Print address of sender
					cout << "from :" << (int)(char)(from.addr>>24) << '.' << (int)(char)(from.addr>>16)  << '.'<< (int)(char)(from.addr>>8) << '.' << (int)(char)(from.addr);
					cout << " port " << from.port << endl;

					cout << buffer << endl;
				}
				delete buffer;
			}
		}
	} catch (exception &e)
	{
		cerr << e.what() << endl;
		return -1;
	}

	return 0;
}
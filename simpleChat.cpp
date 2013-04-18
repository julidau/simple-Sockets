/*
 * main.cpp
 *
 *  Created on: 18.04.2013
 *      Author: julian
 */

#include "simpleSocket.hpp"

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
			SSocket sock;

			SSocket::Address addr;
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
			SSocket sock(3000);
			SSocket::Address from;

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

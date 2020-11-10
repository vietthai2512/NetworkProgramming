#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <conio.h>
#include <string>
#include <stdlib.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>

#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 2048

#pragma comment (lib, "Ws2_32.lib")

std::string countWords(const char* input);

int main(int argc, char* argv[])
{
	const int SERVER_PORT = atoi(argv[1]);

	// Initialize Winsock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData))
		printf("Version is not supported\n");

	// Construct socket
	SOCKET listenSock;
	listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

	if (bind(listenSock, (sockaddr *)&serverAddr, sizeof(serverAddr)))
	{
		printf("Error! Cannot bind this address.");
		_getch();
		return 0;
	}

	// Listen request from client
	if (listen(listenSock, 10))
	{
		printf("Error! Cannot listen.");
		_getch();
		return 0;
	}
	printf("Server started.\n");

	// Communicate with client
	sockaddr_in clientAddr;
	char buff[BUFF_SIZE];
	int ret, clientAddrLen = sizeof(clientAddr);

	while (1)
	{
		SOCKET connSock;

		// Accept request
		connSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrLen);

		while (1)
		{
			// Receive the length of incoming message
			ret = recv(connSock, buff, BUFF_SIZE - 1, 0);
			if (ret == SOCKET_ERROR)
			{
				printf("Error: %d", WSAGetLastError());
				break;
			}
			// If ret = 0, the connection has been gracefully closed
			else if (ret == 0)
			{
				break;
			}
			// If ret > 0
			buff[ret] = 0;
			// Get length from buff
			int dataLength = (int)ntohl(*reinterpret_cast<uint32_t*>(buff));
			int idx = 0;
			int nLeft = dataLength;

			// Loop until all message is received
			while (nLeft > 0)
			{
				ret = recv(connSock, &buff[0], nLeft, 0);
				if (ret == SOCKET_ERROR)
				{
					break;
				}
				idx += ret;
				nLeft -= ret;
			}
			// Print received message 
			buff[dataLength] = 0;
			printf("Receive from client[%s:%d]: %s\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), buff);

			// Send the number of words to client
			std::string output = countWords(buff);
			ret = send(connSock, output.c_str(), output.size(), 0);
			if (ret == SOCKET_ERROR)
				printf("Error: %d", WSAGetLastError());
		}

		// Close connSock and waiting for a new accepted socket
		closesocket(connSock);
	}
	// Close socket
	closesocket(listenSock);

	// Terminate Winsock
	WSACleanup();
}

// If string contains number return Error
// Else return number of words
std::string countWords(const char* input)
{
	std::string output;

	// NULL string have 0 word
	if (input == NULL)
		return "Number of words: 0";

	bool inSpaces = true;
	int numWords = 0;

	// Loop until end of string
	while (*input != '\0')
	{
		// Check if string have number
		if (isdigit(*input))
		{
			numWords = -1;
			break;
		}
		else if (*input == ' ')
		{
			inSpaces = true;
		}
		else if (inSpaces)
		{
			numWords++;
			inSpaces = false;
		}
		input++;
	}

	// If numWords = -1, return Error string
	// Else return Number string
	if (numWords == -1)
	{
		return "Error: String contains number";
	}
	else
	{
		output = "Number of words: ";
		output += std::to_string(numWords);
	}
	return output;
}

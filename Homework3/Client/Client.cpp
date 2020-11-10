#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <conio.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <cstdlib>
#include <iostream>
#include <string>

#define BUFF_SIZE 2048

#pragma comment (lib, "Ws2_32.lib")

int main(int argc, char* argv[])
{
	const int SERVER_PORT = atoi(argv[2]);
	const char* SERVER_ADDR = argv[1];

	// Initialize Winsock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData))
		printf("Version is not supported\n");

	// Construct socket
	SOCKET client;
	client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Specify server address
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

	// Request to connect server
	if (connect(client, (sockaddr*)&serverAddr, sizeof(serverAddr)))
	{
		printf("Error! Cannot connect to server. %d", WSAGetLastError());
		_getch();
		return 0;
	}
	printf("Connected server!\n");

	// Communicate with server
	char buff[BUFF_SIZE];
	int ret;
	uint32_t dataLength, nLeft, idx;

	while (1)
	{
		printf("Send to server: ");
		gets_s(buff, BUFF_SIZE);

		// If user enter an empty string, close the program
		if (strcmp(buff, "") == 0)
			break;

		// Send the length of the message to server
		uint32_t dataLength = htonl(strlen(buff));
		send(client, (char*)&dataLength, sizeof(dataLength), 0);

		// Loop until the message is sent
		nLeft = strlen(buff);
		idx = 0;
		while (nLeft > 0)
		{
			int ret = send(client, &buff[0], nLeft, 0);
			if (ret == SOCKET_ERROR)
			{
				printf("Error! Cannot send.%d", WSAGetLastError());
				break;
			}
			nLeft -= ret;
			idx += ret;
		}

		// Receive and print the number of words
		ret = recv(client, buff, BUFF_SIZE, 0);
		if (ret == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAETIMEDOUT)
				printf("Time out");
			else printf("Error. Cannot recieve message.");
		}
		else if (strlen(buff) > 0)
		{
			buff[ret] = 0;
			printf("Recieve from server[%s:%d]: %s\n",
				inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port), buff);
		}
	}

	// Close socket
	closesocket(client);

	// Terminate Winsock
	WSACleanup();

	return 0;
}


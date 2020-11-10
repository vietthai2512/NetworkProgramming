// Client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <conio.h>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <cstdlib>
#include <iostream>

//#define SERVER_PORT 5500
//#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 2048

#pragma comment (lib, "Ws2_32.lib")

int main(int argc, char* argv[])
{
	const int SERVER_PORT = atoi(argv[2]);
	const char* SERVER_ADDR = argv[1];
    // Step 1: Initiative WinSock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData))
		printf("Version is not supported.\n");
	printf("Client started!\n");

	// Step 2: Construct socket 
	SOCKET client;
	client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	// (optional) Set time-out for receiving
	int tv = 10000; // Time-out interval: 10000ms
	setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)(&tv), sizeof(int));

	// Step 3: Specify server address
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

	// Step 4: Communicate with server
	char buff[BUFF_SIZE];
	int ret, serverAddrLen = sizeof(serverAddr);

	while (1)
	{
		// Input request
		printf("Send to server: ");
		gets_s(buff, BUFF_SIZE);

		// If user enter an empty string, close the program
		if (strcmp(buff, "") == 0)
			break;

		// Send request
		ret = sendto(client, buff, strlen(buff), 0, (sockaddr *)&serverAddr, serverAddrLen);
		if (ret == SOCKET_ERROR)
			printf("ERROR! Cannot send message.\n");

		// Inner while loop: Receive all responses from one request
		while(1)
		{
			// Receive response
			ret = recvfrom(client, buff, BUFF_SIZE, 0, (sockaddr *)&serverAddr, &serverAddrLen);
			if (ret == SOCKET_ERROR)
			{
				if (WSAGetLastError() == WSAETIMEDOUT)
					printf("Time-out!\n");
				else printf("Error! Cannot receive message.\n");
				break;
			}
			buff[ret] = 0;
			// If client received all responses, break the while loop and waiting for a new request
			if (strcmp(buff, "end") == 0)
			{
				printf("\n");
				break;
			}
			// Else print out all informations
			printf("Receive from server[%s:%d]: %s\n", inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port), buff);
		} 		
	} 
	// Close socket;
	closesocket(client);

	// Terminate Winsock
	WSACleanup();

	return 0;
}
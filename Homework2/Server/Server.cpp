// Server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <conio.h>
#include <winsock2.h>
#include <string>
#include <WS2tcpip.h>
#include <iostream>

#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 2048
#define SERVICENAME "http"
#define END "end"

#pragma comment (lib, "Ws2_32.lib")

void sendHostnameTo(hostent* remoteHost, SOCKET server, sockaddr_in clientAddr);

int main(int argc, char* argv[])
{
	const int SERVER_PORT = atoi(argv[1]);

    // Step 1: Initialize Winsock
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData))
		printf("Version is not supported\n");

	// Step 2: Construct socket
	SOCKET server;
	server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	// Step 3: Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERVER_PORT);
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

	if (bind(server, (sockaddr *)&serverAddr, sizeof(serverAddr)))
	{
		printf("Error! Cannot bind this address.");
		_getch();
		return 0;
	}
	printf("Server started!\n");

	// Step 4: Communicate with client
	sockaddr_in clientAddr;
	char buff[BUFF_SIZE];
	int ret, clientAddrLen = sizeof(clientAddr);

	while (1)
	{
		// Recieve message
		ret = recvfrom(server, buff, BUFF_SIZE, 0, (sockaddr *)&clientAddr, &clientAddrLen);
		if (ret == SOCKET_ERROR)
		{
			printf("Error: %d", WSAGetLastError());
			sendto(server, END, strlen(END), 0, (SOCKADDR *)&clientAddr, sizeof(clientAddr));
		}
		else if (strlen(buff) > 0)
		{
			buff[ret] = '\0';
			printf("Recieve from client[%s:%d]: %s\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), buff);
			
			in_addr inaddr;
			in6_addr in6addr;
			DWORD dwRetval;
			struct hostent* remoteHost;

			// Validate IPv4, IPv6, hostname.
			int checkV4 = inet_pton(AF_INET, buff, &inaddr);
			int checkV6 = inet_pton(AF_INET6, buff, &in6addr);

			// If buff is a hostname
			if (checkV4 == 0 && checkV6 == 0)
			{
				// Declare and initialize variables, buffer
				addrinfo* result = NULL;
				addrinfo* ptr = NULL;
				char buf6[INET6_ADDRSTRLEN];

				// Call getaddrinfo(). If the call succeeds,
				// the result variable will hold a linked list
				// of addrinfo structures containing response
				// information
				dwRetval = getaddrinfo(buff, SERVICENAME, NULL, &result);
				if (dwRetval != 0)
				{
					std::string buff = "Not found information.";
					sendto(server, buff.c_str(), buff.length(), 0, (SOCKADDR*)&clientAddr, sizeof(clientAddr));
				}
				
				// Retrieve each IPs address and send to client
				for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
				{
					if (ptr->ai_family == AF_INET)
					{
						sockaddr_in* sockaddr_ipv4 = (sockaddr_in*)ptr->ai_addr;

						char* IP4 = inet_ntoa(sockaddr_ipv4->sin_addr);
						int IP4_len = strlen(IP4);

						ret = sendto(server, IP4, IP4_len, 0, (SOCKADDR *)&clientAddr, sizeof(clientAddr));
						if (ret == SOCKET_ERROR)
							printf("Error: %d", WSAGetLastError());
					}
					else if (ptr->ai_family == AF_INET6)
					{
						sockaddr_in6* sockaddr_ipv6 = (sockaddr_in6*)ptr->ai_addr;
						inet_ntop(AF_INET6, &sockaddr_ipv6->sin6_addr, buf6, INET6_ADDRSTRLEN);

						ret = sendto(server, buf6, strlen(buf6), 0, (SOCKADDR *)&clientAddr, sizeof(clientAddr));
						if (ret == SOCKET_ERROR)
							printf("Error: %d", WSAGetLastError());
					}
				}
				freeaddrinfo(result);
			}

			// If buff is IPv4
			else if (checkV4 == 1)
			{
				remoteHost = gethostbyaddr((char*)&inaddr, sizeof(inaddr), AF_INET);
				sendHostnameTo(remoteHost, server, clientAddr);
			}

			// If buff is IPv6
			else if (checkV6 == 1)
			{
				remoteHost = gethostbyaddr((char*)&in6addr, sizeof(in6addr), AF_INET6);
				sendHostnameTo(remoteHost, server, clientAddr);
			}
			// A client's request is solved. Waiting for a new request
			sendto(server, END, strlen(END), 0, (SOCKADDR *)&clientAddr, sizeof(clientAddr));
		}
	}
	closesocket(server);
	WSACleanup();
}

// Send official name and alias name to client
void sendHostnameTo(hostent* remoteHost, SOCKET server, sockaddr_in clientAddr)
{
	DWORD dwError;
	int ret;
	char ** pAlias;
	std::string buffString;

	// If gethostbyaddr return nothing, send detail to client
	if (remoteHost == NULL) 
	{
		dwError = WSAGetLastError();
		if (dwError != 0) 
		{
			if (dwError == WSAHOST_NOT_FOUND)
			{
				char buff[] = "Host not found.";
				ret = sendto(server, buff, sizeof(buff), 0, (SOCKADDR*)&clientAddr, sizeof(clientAddr));
			}
			else if (dwError == WSANO_DATA) 
			{
				char buff[] = "No data record found.";
				ret = sendto(server, buff, sizeof(buff), 0, (SOCKADDR*)&clientAddr, sizeof(clientAddr));
			}
			else 
			{
				char buff[] = "Function failed.";
				ret = sendto(server, buff, sizeof(buff), 0, (SOCKADDR*)&clientAddr, sizeof(clientAddr));
			}
		}
	}
	else
	{
		// Send official name
		buffString = "Official name: "; buffString.append(remoteHost->h_name);

		ret = sendto(server, buffString.c_str(), buffString.length(), 0, (SOCKADDR*)&clientAddr, sizeof(clientAddr));
		if (ret == SOCKET_ERROR)
			printf("Error: %d", WSAGetLastError());

		// Send all alias names
		for (pAlias = remoteHost->h_aliases; *pAlias != 0; pAlias++)
		{
			buffString = "Alias name: "; buffString.append(*pAlias);
			ret = sendto(server, buffString.c_str(), buffString.length(), 0, (SOCKADDR*)&clientAddr, sizeof(clientAddr));
			if (ret == SOCKET_ERROR)
				printf("Error: %d", WSAGetLastError());
		}
	}
}


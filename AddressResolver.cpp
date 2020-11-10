#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <stdio.h>
#include <iostream>

// link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")

// Service name or port number represented as a string
#define SERVICENAME "http"

void printHostname(DWORD dwRetval, char hostname[]);

int main(int argc, char* argv[])
{
	//-------------------------------------------
	// Declare and initialize variables
	WSADATA wsaData;
	int iResult;

	DWORD dwRetval;

	// Store hostname
	char hostname[NI_MAXHOST];

	in_addr inaddr;
	in6_addr in6addr;

	// Validate the parameters
	if (argc != 2)
	{
		printf("Usage: %s <hostname_or_IP address>\n", argv[0]);
		return 1;
	}

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		printf("Error code: %ld", WSAGetLastError());
		return 1;
	}

	// Validate IPv4, IPv6, hostname.
	int checkV4 = inet_pton(AF_INET, argv[1], &inaddr);
	int checkV6 = inet_pton(AF_INET6, argv[1], &in6addr);

	// If argv[1] is a hostname
	if (checkV4 == 0 && checkV6 == 0)
	{
		// Declare and initialize variables, buffer
		addrinfo* result = NULL;
		addrinfo* ptr = NULL;
		char buf6[INET6_ADDRSTRLEN];

		//--------------------------------
		// Setup the hints address info structure
		// which is passed to the getaddrinfo() function
		addrinfo hints;
		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		// Call getaddrinfo(). If the call succeeds,
		// the result variable will hold a linked list
		// of addrinfo structures containing response
		// information
		dwRetval = getaddrinfo(argv[1], SERVICENAME, &hints, &result);
		if (dwRetval != 0)
		{
			printf("Not found information. getaddrinfo failed with error: %d\n", dwRetval);
			WSACleanup();
			return 1;
		}

		// Retrieve each address and print out the IPs
		for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
		{
			if (ptr->ai_family == AF_INET)
			{
				sockaddr_in* sockaddr_ipv4 = (sockaddr_in*)ptr->ai_addr;
				printf("IPv4: %s\n", inet_ntoa(sockaddr_ipv4->sin_addr));
			}
			else if (ptr->ai_family == AF_INET6)
			{
				sockaddr_in6* sockaddr_ipv6 = (sockaddr_in6*)ptr->ai_addr;
				printf("IPv6: %s\n", inet_ntop(AF_INET6, &sockaddr_ipv6->sin6_addr, buf6, INET6_ADDRSTRLEN));
			}
		}
		freeaddrinfo(result);
	}

	// If argv[1] is IPv4
	else if (checkV4 == 1)
	{
		//-----------------------------------------
		// Set up sockaddr_in structure which is passed
		// to the getnameinfo function
		sockaddr_in saGNI;
		saGNI.sin_family = AF_INET;
		saGNI.sin_addr = inaddr;

		//-----------------------------------------
		// Call getnameinfo
		dwRetval = getnameinfo((sockaddr*)&saGNI, sizeof(sockaddr), hostname,
			NI_MAXHOST, NULL, 0, NI_NAMEREQD);

		// Print the hostname or error code
		printHostname(dwRetval, hostname);
	}

	// If argv[1] is IPv6
	else if (checkV6 == 1)
	{
		//-----------------------------------------
		// Set up sockaddr_in6 structure which is passed
		// to the getnameinfo function
		sockaddr_in6 saGNI6;
		saGNI6.sin6_family = AF_INET6;
		saGNI6.sin6_addr = in6addr;

		//-----------------------------------------
		// Call getnameinfo
		dwRetval = getnameinfo((sockaddr*)&saGNI6, sizeof(saGNI6), hostname,
			NI_MAXHOST, NULL, 0, NI_NAMEREQD);

		// Print the hostname or error code
		printHostname(dwRetval, hostname);
	}
	WSACleanup();
	return 0;
}

// Print the hostname or error code
void printHostname(DWORD dwRetval, char hostname[])
{
	if (dwRetval != 0)
	{
		printf("Not found information. Error: %ld\n", WSAGetLastError());
	}
	else
	{
		printf("List of names: %s\n", hostname);
	}
}
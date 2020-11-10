// Task1_Client.cpp : Defines the entry point for the console application.
//
// Note:  I define state block: -1 and state active 1
//

#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <WS2tcpip.h>

#define BUFF_SIZE 2048
#define MAX_TRY_REQUEST 10

#define ACTIVE 1	// Account is active
#define BLOCKED -1	// Account is blocked

#define USERNAME_NOTFOUND -11
#define USERNAME_CORRECT  11

#define PASSWORD_INCORRECT	-21
#define PASSWORD_CORRECT	21

#define LOGOUT_ERROR   -31
#define LOUGOUT_SUCESS 31

#define ACCOUNT_LOGINED 41

// Link with Ws2_32.lib
#pragma comment(lib,"Ws2_32.lib")

// Prototype functions
void checkArguments(int argc, char **argv, char *serverIP, int *serverPort);
void checkIPv4Addr(char *ipAddr, char *serverIP);
int  checkPortNumber(char *port);
void connectServer(SOCKET &client, sockaddr_in serverAddr);
int  sendExt(SOCKET s, char *buff, int dataLen);
int  recvExt(SOCKET s, char *buff);
void exitExt(int exitValue);
void checkUser(char *data);
void checkPassword(char *data);
void checkLogout(char *data);
void getData(char *recvBuff, char *data);

/**
	Check arguments from keyboard

	@param argc:		Number of arguments
	@param serverIP:	Pointer to Server IP Address (string)
	@param serverPort:	Pointer to Server Port (int)

	@return:			No return value or exit (0) if Invalid arguments
*/
void checkArguments(int argc, char **argv, char *serverIP, int *serverPort) {
	if (argc != 3) {
		printf("\nUsage: %s ServerIP ServerPort.\n", argv[0]);
		printf("Example: %s 127.0.0.1 5000\n", argv[0]);
		system("pause");
		exit(0);
	}

	checkIPv4Addr(argv[1], serverIP);
	*serverPort = checkPortNumber(argv[2]);

	if (serverIP[0] == '\0' || *serverPort == -1) {
		system("pause");
		exit(0);
	}

	printf("     ------------------------------------------------\n");
	printf("		Welcome to application!\n");
}

/**
	Checking the validity of the IP address

	@param ipAddr: Pointer to argv[1].
	@param serverIP: Pointer to Server IP Address (string)

	@return: No return value.
*/
void checkIPv4Addr(char *ipAddr, char *serverIP) {
	int i = 0;
	int decValueAscii = 0;
	static bool findChar = false;
	int countDot = 0;
	int len = strlen(ipAddr);

	for (i = 0; ipAddr[i] != '\0'; i++) {
		decValueAscii = ipAddr[i] - '0';
		if (decValueAscii == -2) {
			countDot++;		// Find "."
			continue;
		}
		else if (decValueAscii < 0 || decValueAscii > 9) {
			findChar = true;	// Find character
			break;
		}
	}

	unsigned long rs = inet_addr(ipAddr);

	// Find character or octet of value greater than 255
	if (findChar == true || countDot != 3 || rs == INADDR_NONE) {
		printf("The IP address of server invalid.\n");
		serverIP[0] = '\0';	// ServerIP = NULL if IP address invalid.
	}
	else strcpy_s(serverIP, 16, ipAddr);
}

/**
	Checking the validity of the port

	@param port: Pointer to port (string from keyboard

	@return:	 port value (int) or error (-1)
*/
int checkPortNumber(char *port) {
	int i = 0, decValueAscii = 0;
	static bool findChar = false;
	int len = strlen(port);

	for (i = 0; port[i] != '\0'; i++) {
		decValueAscii = port[i] - '0';
		if (decValueAscii < 0 || decValueAscii > 9) {
			findChar = true;
			break;
		}
	}

	// Find charachter or len(port) greater than len (65535)
	if (findChar == true || len > 5) {
		printf("The port value of server invalid.\n");
		return -1;
	}

	// Caculate value port
	i = 0;
	int valuePort = 0;
	while (i < len) {
		valuePort = valuePort * 10 + (port[i] - '0');
		i++;
	}

	return valuePort;
}

/**
	Try connect to server

	@param client:	   Socket using connect to server
	@param serverAddr: socket address of server (connected)

	@return:		   No return value or exit(0) if client disconnected with server
*/
void connectServer(SOCKET &client, sockaddr_in serverAddr) {
	// Max request: 10
	int countRequest = MAX_TRY_REQUEST;
	int iResult = 0, dwError = 0;

	while (1) {
		iResult = connect(client, (sockaddr*)&serverAddr, sizeof(serverAddr));
		if (iResult == SOCKET_ERROR) {
			dwError = WSAGetLastError();
			printf("Cannot connect to server. Error: %d.\n", dwError);
			countRequest--;

			if (!countRequest) {
				exitExt(0);
			}
		}
		else break;
	}

	printf("Client started at local host!\n");
	printf("Connected with server!\n");
	printf("Sending to TCP Server ...\n");
	printf("Type USER:username to login.\n     PASS:password to login.\n     LGOU:username to logout.\n");
}

/** The send() wrapper function. */
int sendExt(SOCKET s, char *buff, int dataLen) {
	int ret = send(s, buff, dataLen, 0);
	if (ret == SOCKET_ERROR)
		printf("Cannot send message. Error: %d\n.", WSAGetLastError());

	return ret;
}

/** The recv() wrapper function. */
int recvExt(SOCKET s, char *buff) {
	int ret = recv(s, buff, BUFF_SIZE, 0);

	if (ret == SOCKET_ERROR) {
		if (ret == WSAETIMEDOUT)
			printf("Time-out!");
		else
			printf("Cannot receive message. Error: %d.\n", WSAGetLastError());
	}

	return ret;
}

/** The exit() wrapper function. */
void exitExt(int exitValue) {
	WSACleanup();
	system("pause");
	exit(exitValue);
}

/**
	Check User login

	@param data: opcode of action check user

	@return:	 No return value
*/
void checkUser(char *data) {
	if (data[0] == USERNAME_NOTFOUND)
		printf(" Username not found.\n");
	else if (data[0] == BLOCKED)
		printf(" Account is blocking.Enter other Username.\n");
	else if (data[0] == ACCOUNT_LOGINED)
		printf(" There exists a successful login account.\n You must logout before login the new account.\n");
	else
		printf(" OK! Username exist.Enter password to login.\n");
}

/**
	Check password login

	@param data: opcode of action check password

	@return:	 No return value
*/
void checkPassword(char *data) {
	if (data[0] == PASSWORD_INCORRECT)
		printf(" Password incorrect.\n");
	else if (data[0] == PASSWORD_CORRECT)
		printf(" Password correct.\n Account login sucessfull.\n");
	else if (data[0] == BLOCKED)
		printf(" Account is blocking.Enter other Username.\n");
	else if (data[0] == ACCOUNT_LOGINED)
		printf(" There exists a successful login account.\n You must logout before type password.\n");
	else
		printf(" No User wating authenticated.\n");
}

/**
	Send request logout to server

	@param data: opcode of actioc check logout

	@return:	 No return value
*/
void checkLogout(char *data) {
	char userName[32];
	int i = 1;
	while (data[i] != '\0') {
		userName[i - 1] = data[i];
		i++;
	}
	userName[i - 1] = '\0';

	if (data[0] == LOUGOUT_SUCESS)
		printf(" Logout %s sucessfull.\n", userName);
	else
		printf(" Logout failed. Because no account logined.\n");
}

/**
	Get data in message receive from client

	@param recvBuff: Pointer to recvBuff string receive from client
	@param data: Pointer to data in recvBuff

	@return: No return value
*/
void getData(char *recvBuff, char *data) {
	memset(data, 0, BUFF_SIZE);
	int i = 5;
	while (recvBuff[i] != '\0') {
		data[i - 5] = recvBuff[i];
		i++;
	}
	data[i - 5] = '\0';
}

int main(int argc, char **argv) {
	char serverIP[16];
	int serverPort = 0;
	checkArguments(argc, argv, serverIP, &serverPort);

	// Step 1: Initiate WinSock 2.2
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);
	if (WSAStartup(wVersion, &wsaData)) {
		printf("Version is not supported.\n");
		system("pause");
		return 0;
	}

	// Step 2: Construct socket	
	SOCKET client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (client == INVALID_SOCKET) {
		printf("Initiate socket fail. Error: %d.\n", WSAGetLastError());
		exitExt(0);
	}

	// (optional) Set time-out for receiving
	int tv = 1000; // Time-out interval: 1000 ms
	int iResult = setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)(&tv), sizeof(int));
	if (iResult == SOCKET_ERROR) {
		printf("Setup time-out for receiving fail. Error: %d.", WSAGetLastError());
		closesocket(client);
		exitExt(0);
	}

	// Step 3: Specify server address
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(serverPort);
	serverAddr.sin_addr.s_addr = inet_addr(serverIP);

	// Step 4: Request to connect server
	connectServer(client, serverAddr);

	// Step 5: Communicate with server
	char buff[BUFF_SIZE];
	int ret;
	char msg_type[5];
	char data[BUFF_SIZE];

	while (1) {
		// Send message
		printf("Send to server: ");
		gets_s(buff, BUFF_SIZE);
		if (buff[0] == '\0') break;

		ret = sendExt(client, buff, strlen(buff));
		if (ret == SOCKET_ERROR) break;

		// Receive echo message
		ret = recvExt(client, buff);
		if (ret == SOCKET_ERROR) break;
		else {
			buff[ret] = '\0';
			printf("Receive from server[%s:%d]:\n", inet_ntoa(serverAddr.sin_addr), ntohs(serverAddr.sin_port));
			strncpy_s(msg_type, 5, buff, 4);
			getData(buff, data);

			if (strcmp(msg_type, "USER") == 0)
				checkUser(data);
			else if (strcmp(msg_type, "PASS") == 0)
				checkPassword(data);
			else if (strcmp(msg_type, "LGOU") == 0)
				checkLogout(data);
			else
				printf(" Invalid request.\n");
		}
	}
	// Step 6: Close socket
	closesocket(client);

	// Step 7: Terminate Winsock
	exitExt(0);
}
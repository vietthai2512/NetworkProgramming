// Task1_Server.cpp : Defines the entry point for the console application.
//
// Note: "account.txt" must save to Homework06
//
// Using OverlappedEvent with MultiThread
//
// Note: I define state block: -1 and state active 1
//

#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <direct.h>
#include <WinSock2.h>
#include <Windows.h>
#include <process.h>

// Link with Ws2_32.lib
#pragma comment(lib,"Ws2_32.lib")

#define MAX_LEN_PATH 2048

#define BUFF_SIZE 2048

#define MAX_BACKLOG 10

#define RECEIVE 0
#define SEND 1

#define REQUEST_INVALID -2

#define USERNAME_SIZE 128
#define PASSWORD_SIZE 128

#define ACTIVE  1	// Account is active
#define BLOCKED -1	// Account is blocked

#define USERNAME_NOTFOUND -11
#define USERNAME_CORRECT  11

#define PASSWORD_INCORRECT -21
#define PASSWORD_CORRECT   21

#define LOGOUT_ERROR   -31
#define LOUGOUT_SUCESS 31

#define ACCOUNT_LOGINED 41

// Define state of User
#define UNIDENTIFIED	2
#define UNAUTHENTICATED 3
#define AUTHENTICATED	4

// Define user structure in database
typedef struct User {
	char userID[USERNAME_SIZE];
	char password[PASSWORD_SIZE];
	char status[2];
} User;

// Define sockInfomation structure ~ session structure
typedef struct perToOperationData{
	WSAOVERLAPPED overlapped;
	WSABUF dataBuff;
	char buff[BUFF_SIZE];
	int bufLen;
	int recvBytes;
	int sentBytes;
	int operation;
} PER_IO_OPERATION_DATA, *LPPER_IO_OPERATION_DATA;

typedef struct {
	SOCKET sockfd;
	char userID[USERNAME_SIZE];
	int tryLogin;
	int state;
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

/* ************************************************************************************************************* */
// Global variable
char pathFile[MAX_LEN_PATH];
User user[30];
DWORD countAccount = 0;	// Number of account in database
DWORD nClients = 0;		// Number of clients connected

CRITICAL_SECTION criticalSection;
/* ************************************************************************************************************* */

// Prototype functions
void checkArguments(int argc, char **argv, int &port);
int  checkPortNumber(char *port);

void exitExt(int exitValue);

void getPathFile(char *pathFile);
void getInfoAccounts(char *pathFile, User user[]);
void saveDatabase();

void getData(char *recvBuff, char *data);

void checkUser(LPPER_HANDLE_DATA perHandleData, char *data, char *out);
void checkPassword(LPPER_HANDLE_DATA perHandleData, char *data, char *out);
void checkLogout(LPPER_HANDLE_DATA perHandleData, char *data, char *out);
void processException(LPPER_HANDLE_DATA perHandleData, char *data, char *out);
void processData(LPPER_HANDLE_DATA perHandleData, LPPER_IO_OPERATION_DATA perIoData, DWORD transferfedBytes, char *in, char *out);
unsigned __stdcall serverWorkerThread(LPVOID completionPortID);

/* ************************************************************************************************************* */
/**
	Check arguments from keyboard

	@param argc: Number of arguments
	@param argv: List string of arguments
	@param port: Pointer to Server Port (int)

	@return:	 No return value or exit (0) if Invalid arguments
*/
void checkArguments(int argc, char **argv, int &port) {
	if (argc != 2) {
		printf("\nUsage: %s PortNumber\n", argv[0]);
		printf("Note: You should use port in range (1024-49151)\n");
		printf("Example: %s 5000\n", argv[0]);
		system("pause");
		exit(0);
	}

	port = checkPortNumber(argv[1]);

	if (port == -1) {
		system("pause");
		exit(0);
	}

	printf("     ------------------------------------------------\n");
	printf("		Welcome to application!\n");
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

/** The exit() wrapper function. */
void exitExt(int exitValue) {
	WSACleanup();
	system("pause");
	exit(exitValue);
}

/**
	Get path of file "account.txt" to pathFile

	@param pathFile: Pointer to path of file account (string)

	@return:		 No return value
*/
void getPathFile(char *pathFile) {
	// Get path of current directory
	char currPath[MAX_LEN_PATH];
	_getcwd(currPath, MAX_LEN_PATH);

	int i = strlen(currPath);

	// Get length of parent directory
	while (i >= 0) {
		if (currPath[i] == '\\') break;
		else i--;
	}

	// Get path of parent directory
	int j = 0;
	for (j = 0; j < i; j++) {
		pathFile[j] = currPath[j];
	}
	pathFile[i] = '\0';
	strcat_s(pathFile, MAX_LEN_PATH, "\\account.txt");
}

/**
	Get account (database) to account array

	@param pathFile: Pointer to path of file account.txt
	@param user:	 Pointer to account array (struct)

	@return: No return value or exit(0) if Not found account.txt
*/
void getInfoAccounts(char *pathFile, User user[]) {
	FILE *fr = NULL;
	errno_t error = fopen_s(&fr, pathFile, "r");

	if (fr == NULL) {
		printf("Cannot open file \"account.txt\" in database.\n");
		system("pause");
		exit(0);
	}

	int i = 0;
	char buffReads[BUFF_SIZE];
	while (!feof(fr)) {
		fgets(buffReads, BUFF_SIZE, fr);
		int j = 0;
		int k = 0;

		// Get userID
		while (buffReads[j] != '\0') {
			if (buffReads[j] != ' ') {
				user[i].userID[k] = buffReads[j];
				k++;
				j++;
			}
			else {
				user[i].userID[k] = '\0';
				j++;
				break;
			}
		}

		// Get password
		k = 0;
		while (buffReads[j] != '\0') {
			if (buffReads[j] != ' ') {
				user[i].password[k] = buffReads[j];
				k++;
				j++;
			}
			else {
				user[i].password[k] = '\0';
				j++;
				break;
			}
		}

		user[i].status[0] = buffReads[j];
		user[i].status[1] = '\0';

		i++;
	}
	fclose(fr);
	countAccount = i;
}

/**
	The saveDatabase() save state of account to database

	@return: No return value
*/
void saveDatabase() {
	FILE *fw = NULL;
	errno_t error = fopen_s(&fw, pathFile, "w");

	if (fw == NULL) {
		printf("Cannot save file \"account.txt\" to database.\n");
		system("pause");
		exit(0);
	}

	for (DWORD i = 0; i < countAccount; i++) {
		fprintf(fw, "%s %s %s", user[i].userID, user[i].password, user[i].status);
		if (i != countAccount - 1) fprintf(fw, "\n");
	}
	fclose(fw);
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

/*
	Check UserName from client

	@param sockInfo: Pointer structure to SOCKET_INFORMATION in clients[]
	@param data:	 Poiter to Username
	@param out:		 Pointer to response client

	@return:		 No return value
*/
void checkUser(LPPER_HANDLE_DATA perHandleData, char *data, char *out) {
	DWORD i = 0;

	EnterCriticalSection(&criticalSection);
	while (1) {
		// Find userID in database
		if (strcmp(user[i].userID, data) == 0) {
			if (user[i].status[0] == '0') {
				out[5] = BLOCKED;	// The user was blocked
				out[6] = '\0';
				LeaveCriticalSection(&criticalSection);
				return;
			}
			else break;	// The user is active. Break loop
		}
		else i++;

		// Not found userID in database
		if (i >= countAccount) {
			out[5] = USERNAME_NOTFOUND;
			out[6] = '\0';
			LeaveCriticalSection(&criticalSection);
			return;
		}
	}
	LeaveCriticalSection(&criticalSection);

	EnterCriticalSection(&criticalSection);
	// There exists a successful login account
	if (perHandleData->state == AUTHENTICATED) {
		out[5] = ACCOUNT_LOGINED;
		out[6] = '\0';
		LeaveCriticalSection(&criticalSection);
		return;
	}
	else if (perHandleData->state = UNIDENTIFIED) {
		// Emty userID in sockInfo
		strcpy_s(perHandleData->userID, USERNAME_SIZE, data);
		perHandleData->tryLogin = 0;
		perHandleData->state = UNAUTHENTICATED;
		out[5] = USERNAME_CORRECT;	// The user was actived
		out[6] = '\0';
		LeaveCriticalSection(&criticalSection);
		return;
	}
	else {
		// userID exist in sockInfo matching userID request login, but state is unauthenticated
		if (strcmp(perHandleData->userID, data) == 0) {
			out[5] = USERNAME_CORRECT;	// The user is acting
			out[6] = '\0';
			LeaveCriticalSection(&criticalSection);
			return;
		}
		else {
			// userID exist in sockInfo not match userID request login, but state is unauthenticated
			strcpy_s(perHandleData->userID, USERNAME_SIZE, data);	// Replace userName in sockInfo
			perHandleData->tryLogin = 0;							// Reset counter try login
			out[5] = USERNAME_CORRECT;								// The user is acting
			out[6] = '\0';
			LeaveCriticalSection(&criticalSection);
			return;
		}
	}
}

/*
	Check password in database

	@param sockInfo: Pointer structure to SOCKET_INFORMATION in clients[]
	@param data:	 Poiter to Username
	@param out:		 Pointer to response client

	@return:		 No return value
*/
void checkPassword(LPPER_HANDLE_DATA perHandleData, char *data, char *out) {
	EnterCriticalSection(&criticalSection);

	// There exists a successful login account.
	if (perHandleData->state == AUTHENTICATED) {
		out[5] = ACCOUNT_LOGINED;
		out[6] = '\0';
		LeaveCriticalSection(&criticalSection);
		return;
	}

	if (perHandleData->state == UNIDENTIFIED) {
		out[5] = REQUEST_INVALID;
		out[6] = '\0';
		LeaveCriticalSection(&criticalSection);
		return;
	}
	LeaveCriticalSection(&criticalSection);

	// Find password of user in database
	EnterCriticalSection(&criticalSection);
	DWORD i = 0;
	for (i = 0; i < countAccount; i++) {
		if (strcmp(user[i].userID, perHandleData->userID) == 0) break;
	}
	LeaveCriticalSection(&criticalSection);
	// return i: index of sockInfo->userID in array user[]

	EnterCriticalSection(&criticalSection);
	// Login more than 3 times
	if (perHandleData->tryLogin >= 3) {
		out[5] = BLOCKED;
		out[6] = '\0';
		user[i].status[0] = '0'; // Block user
		perHandleData->state = UNIDENTIFIED;
		saveDatabase();	// Save state of account to database
		LeaveCriticalSection(&criticalSection);
		return;
	}

	// Check password with database
	if (strcmp(user[i].password, data) == 0) {
		out[5] = PASSWORD_CORRECT;
		out[6] = '\0';
		perHandleData->state = AUTHENTICATED;
		LeaveCriticalSection(&criticalSection);
		return;
	}
	else {
		out[5] = PASSWORD_INCORRECT;
		out[6] = '\0';
		(perHandleData->tryLogin)++;
		LeaveCriticalSection(&criticalSection);
		return;
	}
}

/**
	Check user logout

	@param sockInfo: Pointer structure to SOCKET_INFORMATION in clients[]
	@param data:	 Pointer to username logout
	@param out:		 Pointer to sendBuff string send to client

	@return:		 No return value
*/
void checkLogout (LPPER_HANDLE_DATA perHandleData, char *data, char *out) {
	// Find socket request logout in clients[]
	EnterCriticalSection(&criticalSection);

	// The exist account login sucessful
	if (perHandleData->state == AUTHENTICATED) {
		if (strcmp(perHandleData->userID, data) == 0) {
			out[5] = LOUGOUT_SUCESS;
			out[6] = '\0';
			perHandleData->state = UNIDENTIFIED;
			LeaveCriticalSection(&criticalSection);
			return;
		}
		else {
			out[5] = LOGOUT_ERROR;
			out[6] = '\0';
			LeaveCriticalSection(&criticalSection);
			return;
		}
	}
	else {
		out[5] = LOGOUT_ERROR;
		out[6] = '\0';
		LeaveCriticalSection(&criticalSection);
		return;
	}
}

// Check request invalid
void processException (LPPER_HANDLE_DATA perHandleData, char *data, char *out) {
	out[5] = REQUEST_INVALID;
	out[6] = '\0';
	return;
}

/**

	The processData function copies the input string to ouput

	@param in:				 Pointer to input string (recvBuff)
	@param out:				 Pointer to output string (sendBuff)
	@param sockInfo:		 Pointer structure to SOCKET_INFORMATION in clients[]
	@param transferredBytes: Number of bytes transfer

	@return:				 No return value
*/
void processData(LPPER_HANDLE_DATA perHandleData, LPPER_IO_OPERATION_DATA perIoData, DWORD transferfedBytes, char *in, char *out) {
	memset(in, 0, BUFF_SIZE);
	memset(out, 0, BUFF_SIZE);
	perIoData->dataBuff.buf[transferfedBytes] = '\0';
	memcpy_s(in, BUFF_SIZE, perIoData->dataBuff.buf, BUFF_SIZE);
	// or using sprintf(in, "%s", perIoData->dataBuff.buf);
	
	memset(perIoData->buff, 0, BUFF_SIZE);
	printf("Receive message from socket [%d] %d bytes: %s\n", perHandleData->sockfd, transferfedBytes, in);

	char msg_type[5];
	char data[BUFF_SIZE];
	strncpy_s(msg_type, 5, in, 4);		// Get msg_type[5]
	strncpy_s(out, BUFF_SIZE, in, 4);	// Copy msg_type[5]
	strcat_s(out, BUFF_SIZE, ":");
	getData(in, data);

	if (strcmp(msg_type, "USER") == 0)
		checkUser(perHandleData, data, out);
	else if (strcmp(msg_type, "PASS") == 0)
		checkPassword(perHandleData, data, out);
	else if (strcmp(msg_type, "LGOU") == 0)
		checkLogout(perHandleData, data, out);
	else processException(perHandleData, data, out);
}

unsigned __stdcall serverWorkerThread(LPVOID completionPortID){
	HANDLE completionPort = (HANDLE)completionPortID;
	DWORD transferredBytes;

	LPPER_HANDLE_DATA perHandleData;
	LPPER_IO_OPERATION_DATA perIoData;
	DWORD flags;
	DWORD iResult;

	char recvBuff[BUFF_SIZE], sendBuff[BUFF_SIZE];

	while (1) {
		iResult = GetQueuedCompletionStatus(completionPort, &transferredBytes, (LPDWORD)&perHandleData, 
			(LPOVERLAPPED*)&perIoData, INFINITE);
		if (iResult == 0) {
			printf("Call GetQueuedCompletionStatus() failed. Error: %d.\n", GetLastError());
			nClients--;
			continue;
		}

		// Check to see if an error has occurred on the socket and if so
		// then close the socket and cleanup the SOCKET_INFORMATION structure associated with the socket.

		if (transferredBytes == 0) {
			printf("Closing socket [%d].\n", perHandleData->sockfd);

			iResult = closesocket(perHandleData->sockfd);
			if (iResult == SOCKET_ERROR) {
				printf("Call closesocket() failed. Error: %d\n", WSAGetLastError());
				return 0;
			}

			GlobalFree(perHandleData);
			GlobalFree(perIoData);
			nClients--;
			continue;
		}
		// Check to see if the operation field equals RECEIVE. If this is so, then
		// this means a WSARecv call just completed so update the recvBytes field
		// with the transferredBytes value from the completed WSARecv() call

		if (perIoData->operation == RECEIVE) {
			perIoData->recvBytes = transferredBytes;	// The number of bytes which is received from client
			perIoData->sentBytes = 0;					// The number of bytes which is send to client
			perIoData->operation = SEND;

			processData(perHandleData, perIoData, transferredBytes, recvBuff, sendBuff);
			memcpy_s(perIoData->buff, BUFF_SIZE, sendBuff, BUFF_SIZE);
			// or using sprintf(perIoData->buff, "%s", sendBuff);
			perIoData->dataBuff.buf = perIoData->buff;
			perIoData->dataBuff.len = strlen(sendBuff);

			printf("Send to socket [%d] %d bytes: %s\n", perHandleData->sockfd, perIoData->dataBuff.len, perIoData->dataBuff.buf);
		}
		else if (perIoData->operation == SEND) {
			perIoData->sentBytes += transferredBytes;
		}

		if (perIoData->sentBytes < 6) {
			// Post another WSASend() request.
			// Since WSASend() is not guaranteed to send all of the bytes requested,
			// continue posting WSASend() calls until all received bytes are sent.

			ZeroMemory(&(perIoData->overlapped), sizeof(OVERLAPPED));
			perIoData->dataBuff.buf = perIoData->buff + perIoData->sentBytes;	// Transfer stream in TCP protocol
			perIoData->dataBuff.len = 6 - perIoData->sentBytes;
			perIoData->operation = SEND;

			iResult = WSASend(perHandleData->sockfd, &(perIoData->dataBuff), 1, &transferredBytes, 0, &(perIoData->overlapped), NULL);
			if (iResult == SOCKET_ERROR) {
				if (WSAGetLastError() != ERROR_IO_PENDING) {
					printf("Call WSASend() failed. Error: %d.\n", WSAGetLastError());
					return 0;
				}
			}
		}
		else {
			// No more bytes to send post another WSARecv() request
			perIoData->recvBytes = 0;
			perIoData->operation = RECEIVE;
			flags = 0;
			ZeroMemory(&(perIoData->overlapped), sizeof(OVERLAPPED));

			perIoData->dataBuff.len = BUFF_SIZE;
			perIoData->dataBuff.buf = perIoData->buff;

			iResult = WSARecv(perHandleData->sockfd, &(perIoData->dataBuff), 1, &transferredBytes, &flags, &(perIoData->overlapped), NULL);
			if (iResult == SOCKET_ERROR) {
				if (WSAGetLastError() != ERROR_IO_PENDING) {
					printf("Call WSARecv() failed. Error: %d.\n", WSAGetLastError());
					return 0;
				}
			}
		}
	}
	return 0;
}
/* ************************************************************************************************************* */

int main(int argc, char **argv) {
	int serverPort = 0;
	checkArguments(argc, argv, serverPort);

	// Get info account to user[]
	getPathFile(pathFile);
	getInfoAccounts(pathFile, user);

	// Inittiate WinSock 2.2
	WSADATA wsaData;
	WORD wVersion = MAKEWORD(2, 2);

	if (WSAStartup(wVersion, &wsaData)) {
		printf("Version is not supported.\n");
		system("pause");
		return 0;
	}

	InitializeCriticalSection(&criticalSection);

	// Step 1: Setup an I/O completion port
	HANDLE completionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (completionPort == NULL) {
		printf("CreateIoCompletionPort() failed. Error: %d.\n", GetLastError());
		exitExt(0);
	}

	// Step 2: Determine how many processors are on the system
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);

	// Step 3: Create worker threads based on the number of processors available on the system. 
	// Create two worker threads for each processor	
	for (int i = 0; i < (int)systemInfo.dwNumberOfProcessors * 2; i++) {
		// Create a server worker thread and pass the completion port to the thread
		if (_beginthreadex(0, 0, serverWorkerThread, (void*)completionPort, 0, 0) == 0) {
			printf("Create thread failed. Error: %d.\n", GetLastError());
			exitExt(0);
		}
	}

	// Step 4: Create a listening socket with "WSA_FLAG_OVERLAPPED"
	SOCKET listenSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (listenSock == INVALID_SOCKET) {
		printf("Call WSASocket() failed. Error: %d.\n", WSAGetLastError());
		exitExt(0);
	}

	// Bind address to socket
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(serverPort);
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	// serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

	DWORD iResult = bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr));
	if (iResult == SOCKET_ERROR) {
		printf("Cannot bind this address. Error: %d.\n", WSAGetLastError());
		exitExt(0);
	}

	// Listen request from client
	// Max number of connection in queue was handle = MAX_BACKLOG = 10
	iResult = listen(listenSock, MAX_BACKLOG);
	if (iResult == SOCKET_ERROR) {
		printf("Cannot listen socket. Error: %d.\n", WSAGetLastError());
		exitExt(1);
	}

	printf("Server started at local host!\n");
	printf("Server connection with Multi client using Completion Port I/O.\n");
	printf("Waiting for TCP clients ...\n");

	SOCKET acceptSock;

	LPPER_HANDLE_DATA perHandleData;
	LPPER_IO_OPERATION_DATA perIoData;

	DWORD flags = 0;
	DWORD recvBytes = 0;
	DWORD transferredBytes = 0;

	while (1) {
		// Step 5: Accept new connections
		acceptSock = WSAAccept(listenSock, NULL, NULL, NULL, 0);
		if (acceptSock == SOCKET_ERROR) {
			printf("Call WSAAccept() failed. Error: %d.\n", WSAGetLastError());
			exitExt(0);
		}

		EnterCriticalSection(&criticalSection);

		// Step 6: Create a socket information structure to associate with the socket
		perHandleData = (LPPER_HANDLE_DATA)GlobalAlloc(GPTR, sizeof(PER_HANDLE_DATA));
		if (perHandleData == NULL) {
			printf("Call GlobalAlloc() failed. Error: %d.\n", GetLastError());
			exitExt(0);
		}

		// Step 7: Associate the accepted socket with the original completion port
		printf("Accept a new socket [%d] connected.\n", acceptSock);
		perHandleData->sockfd = acceptSock;
		perHandleData->userID[0] = '\0';
		perHandleData->tryLogin = 0;
		perHandleData->state = UNIDENTIFIED;

		HANDLE h = CreateIoCompletionPort((HANDLE)acceptSock, completionPort, (DWORD)perHandleData, 0);
		if (h == NULL) {
			printf("Call CreateIoCompletionPort() failed. Error: %d.\n", GetLastError());
			exitExt(0);
		}

		// Step 8: Create per I/O socket information structure to associate with the WSARecv call
		perIoData = (LPPER_IO_OPERATION_DATA)GlobalAlloc(GPTR, sizeof(PER_IO_OPERATION_DATA));
		if (perIoData == NULL) {
			printf("Call GlobalAlloc() failed. Error: %d.\n", GetLastError());
			exitExt(0);
		}

		ZeroMemory(&(perIoData->overlapped), sizeof(OVERLAPPED));
		perIoData->sentBytes = 0;
		perIoData->recvBytes = 0;
		perIoData->dataBuff.len = BUFF_SIZE;
		perIoData->dataBuff.buf = perIoData->buff;
		perIoData->operation = RECEIVE;
		flags = 0;

		iResult = WSARecv(acceptSock, &(perIoData->dataBuff), 1, &transferredBytes, &flags, &(perIoData->overlapped), NULL);
		if (iResult == SOCKET_ERROR) {
			if (WSAGetLastError() != ERROR_IO_PENDING) {
				printf("Call WSARecv() failed. Error: %d.\n", WSAGetLastError());
				exitExt(0);
			}
		}

		printf("There are %d connections.\n", nClients + 1);
		nClients++;
		LeaveCriticalSection(&criticalSection);
	}

	exitExt(0);
}
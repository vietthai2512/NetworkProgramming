#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#pragma comment (lib, "Ws2_32.lib")

#define BUFF_SIZE 2048
#define USN_PSW_SIZE 50
#define CONTENT_SIZE 200

enum msg_type { LOGIN, LOGOUT, ERR, OK };
struct msg_payload
{
	char username[USN_PSW_SIZE];
	char password[USN_PSW_SIZE];
};
struct message_struct
{
	msg_type type;
	msg_payload payload;
};
struct response_message
{
	msg_type type;
	char content[CONTENT_SIZE];
};

std::string tempUsername;

void login(SOCKET client);
void logout(SOCKET client);
message_struct craftMessage(msg_type type, const char* username, const char* password);
message_struct craftMessage(msg_type type);
void sendMessage(message_struct message, SOCKET client);
response_message recvResponse(SOCKET client);
int handleUserChoice(SOCKET client);
std::string maskPassword();

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
		return 0;
	}
	printf("Connected server!\n");

	while (1)
	{
		if (handleUserChoice(client) == 1) break;
	}
}


int handleUserChoice(SOCKET client)
{
	int choice;

	if (tempUsername.empty())
	{
		printf("\nWelcome!\n");
		printf("1. Log in\n2. Log out\n3. Exit \n");
		printf("Please choose an option: ");
		std::cin >> choice;
		
		switch ((choice))
		{
		case 1:
			login(client);
			return 0;
		case 2:
			logout(client);
			return 0;
		case 3:
			return 1;
		default:
			printf("Invalid choice.\n");
			std::cin.clear();
			std::cin.ignore(10000, '\n');
			return 0;
		}
		
	}
	else
	{
		std::cout << "\nHello " << tempUsername << std::endl;
		printf("1. Log out\n2. Exit\n");
		printf("Please choose an option: ");
		std::cin >> choice;
		
		switch ((choice))
		{
		case 1:
			logout(client);
			return 0;
		case 2:
			return 1;
		default:
			printf("Invalid choice.\n");
			std::cin.clear();
			std::cin.ignore(10000, '\n');
			return 0;
		}
	}
}

void login(SOCKET client)
{
	char username[USN_PSW_SIZE], password[USN_PSW_SIZE];
	std::string temp;
	printf("Username: ");
	std::cin >> temp; strncpy_s(username, temp.c_str(), sizeof(temp));

	printf("Password: ");
	temp = maskPassword(); strncpy_s(password, temp.c_str(), sizeof(temp));

	message_struct message = craftMessage(LOGIN, username, password);
	sendMessage(message, client);
	response_message response = recvResponse(client);
	if (response.type == OK)
	{
		tempUsername = message.payload.username;
		printf("Log in successful. Welcome %s.\n", message.payload.username);
	}
	else
	{
		std::cout << response.content;
	}
}

void logout(SOCKET client)
{
	message_struct message = craftMessage(LOGOUT);
	sendMessage(message, client);
	response_message response = recvResponse(client);
	if (response.type == OK)
	{

		std::cout << "Log out successful. Good bye " << tempUsername << ".\n";
		tempUsername.clear();
	}
	else
	{
		std::cout << response.content;
	}
}

message_struct craftMessage(msg_type type, const char* username, const char* password)
{
	message_struct message;
	message.type = type;

	strncpy_s(message.payload.username, username, strlen(username));
	strncpy_s(message.payload.password, password, strlen(password));

	return message;
}

message_struct craftMessage(msg_type type)
{
	message_struct message;
	message.type = type;
	return message;
}

void sendMessage(message_struct message, SOCKET client)
{
	char buff[BUFF_SIZE];
	memcpy(buff, &message, sizeof(message));

	uint32_t msg_len = htonl(sizeof(message));
	send(client, (char*)&msg_len, sizeof(msg_len), 0);

	int nLeft = sizeof(message);
	int idx = 0;
	while (nLeft > 0)
	{
		int ret = send(client, &buff[idx], nLeft, 0);
		if (ret == SOCKET_ERROR)
		{
			printf("Error! Cannot send.%d\n", WSAGetLastError());
			break;
		}
		nLeft -= ret;
		idx += ret;
	}
}

response_message recvResponse(SOCKET client)
{
	char buff[BUFF_SIZE];
	int ret = recv(client, buff, BUFF_SIZE - 1, 0);
	if (ret == SOCKET_ERROR)
	{
		printf("Error: %d\n", WSAGetLastError());
	}
	else if (ret > 0)
	{
		int response_len = (int)ntohl(*reinterpret_cast<uint32_t*>(buff));
		int idx = 0;
		int nLeft = response_len;

		while (nLeft > 0)
		{
			ret = recv(client, &buff[idx], nLeft, 0);
			if (ret == SOCKET_ERROR)
			{
				break;
			}
			idx += ret;
			nLeft -= ret;
		}

		response_message receive;
		memcpy(&receive, buff, sizeof(buff));
		return receive;
	}
}

std::string maskPassword()
{
	const char BACKSPACE = 8;
	const char RETURN = 13;
	const bool show_asterisk = true;

	std::string password;
	unsigned char ch = 0;

	DWORD con_mode;
	DWORD dwRead;
	DWORD fdwSaveOldMode;

	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);

	// Save the current input mode, to be restored on exit. 
	GetConsoleMode(hIn, &fdwSaveOldMode);

	// Get the standard input handle. 
	GetConsoleMode(hIn, &con_mode);
	SetConsoleMode(hIn, con_mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT));

	while (ReadConsoleA(hIn, &ch, 1, &dwRead, NULL) && ch != RETURN)
	{
		if (ch == BACKSPACE && !password.empty())
		{
			std::cout << "\b \b";
			password.pop_back();
		}
		else
		{
			password += ch;
			if (show_asterisk)
				std::cout << '*';
			else
				std::cout << ch;
		}
	}
	std::cout << std::endl;
	SetConsoleMode(hIn, fdwSaveOldMode);
	return password;
}

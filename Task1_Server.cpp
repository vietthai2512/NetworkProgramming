#include "stdafx.h"
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

#define SERVER_ADDR "127.0.0.1"
#define BUFF_SIZE 2048
#define USN_PSW_SIZE 50
#define CONTENT_SIZE 200
#define MAX_LOGIN_ATTEMPTS 3

enum msg_type { LOGIN, LOGOUT, ERR, OK };
enum acc_state { SUCCESS, ACC_LOCKED, WRONG_PW, WRONG_USN, FILE_ERR };

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

#pragma comment (lib, "Ws2_32.lib")

acc_state checkData(const char* username, const char* password, int attempted);
void processRequest(message_struct message, SOCKET connSock);
response_message processLogin(const char* username, const char* password);
response_message processLogout();
response_message craftResponse(msg_type type, std::string content);
response_message craftResponse(msg_type type);
void sendResponse(response_message response, SOCKET connSock);
void receiveRequest(SOCKET connSock);

std::map<std::string, int> LOGIN_STATE;

// Store username to logout
std::string tempUsername;
bool login = false;

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
		return 0;
	}

	// Listen request from client
	if (listen(listenSock, 10))
	{
		printf("Error! Cannot listen.");
		return 0;
	}
	printf("Server started.\n");

	// Communicate with client
	sockaddr_in clientAddr;
	int clientAddrLen = sizeof(clientAddr);

	while (1)
	{
		SOCKET connSock;
		connSock = accept(listenSock, (sockaddr*)&clientAddr, &clientAddrLen);
		
		receiveRequest(connSock);

		closesocket(connSock);
	}
	closesocket(listenSock);
	WSACleanup();
}

acc_state checkData(const char* username, const char* password, int attempted)
{
	std::fstream account_file;
	account_file.open("account.txt", std::fstream::in | std::fstream::out);

	std::string account_info;
	if (account_file.is_open())
	{
		while (std::getline(account_file, account_info))
		{
			std::istringstream iss_account_info(account_info);
			std::string temp;
			while (iss_account_info >> temp)
			{
				if (temp == username)
				{
					iss_account_info >> temp;
					if (temp == password)
					{
						iss_account_info >> temp;
						if (temp == "0")
						{
							account_file.close();
							return SUCCESS;
						}
						else
						{
							account_file.close();
							return ACC_LOCKED;
						}
					}
					else
					{
						iss_account_info >> temp;
						if (temp == "1")
						{
							return ACC_LOCKED;
						}
						if (attempted == MAX_LOGIN_ATTEMPTS - 1)
						{
							
							if (account_file.eof())
							{
								account_file.clear();
								account_file.seekg(-1, account_file.end);
							}
							else
							{
								std::streamoff pos = account_file.tellp();
								account_file.seekg(pos - 3);
							}
							account_file.put('1');
						}
						account_file.close();
						return WRONG_PW;
					}
				}
				else
				{
					break;
				}
			}
		}
		account_file.close();
		return WRONG_USN;
	}
	else
	{
		std::cout << "Unable to open account file.\n";
		return FILE_ERR;
	}
}

void processRequest(message_struct message, SOCKET connSock)
{
	switch (message.type)
	{
		case LOGIN:
			if (login == true)
			{
				sendResponse(craftResponse(ERR, "Cannot log in"), connSock);
				break;
			}
			response_message response = processLogin(message.payload.username, message.payload.password);
			if (response.type == OK)
			{
				tempUsername = message.payload.username;
				login = true;
			}
				
			sendResponse(response, connSock);
			break;
		case LOGOUT:
			sendResponse(processLogout(), connSock);
			break;
		default:
			sendResponse(craftResponse(ERR, "Request not found"), connSock);
	}
}

response_message processLogin(const char* username, const char* password)
{
	auto it = LOGIN_STATE.find(username);
	if (it != LOGIN_STATE.end())
	{
		// 
		if (it->second == 0)
		{
			return craftResponse(ERR, "Account is already logged in.");
		}
		else
		{
			acc_state state = checkData(username, password, it->second);
			if (state == SUCCESS)
			{
				it->second = 0;
				return craftResponse(OK);
			}
			else if (state == WRONG_PW)
			{
				if (it->second == MAX_LOGIN_ATTEMPTS - 1)
				{
					LOGIN_STATE.erase(it);
					return craftResponse(ERR, "The maximum number of login attempts has been reached. The account is locked.");
				}
				it->second++;
				return craftResponse(ERR, "The password is incorrect. " + std::to_string(MAX_LOGIN_ATTEMPTS - it->second) + " attempt(s) left.");
			}
			else if (state == ACC_LOCKED)
			{
				LOGIN_STATE.erase(it);
				return craftResponse(ERR, "The account is locked.");
			}
			// Account file is modified
			else if (state == WRONG_USN)
			{
				LOGIN_STATE.erase(it);
				return craftResponse(ERR, "The username is incorrect.");
			}
		}
	}
	else
	{
		acc_state state = checkData(username, password, 0);
		if (state == SUCCESS)
		{
			LOGIN_STATE[username] = 0;
			return craftResponse(OK);
		}
		else if (state == ACC_LOCKED)
		{
			return craftResponse(ERR, "The account is locked.");
		}
		else if (state == WRONG_USN)
		{
			return craftResponse(ERR, "The username is incorrect.");
		}
		else if (state == WRONG_PW)
		{
			LOGIN_STATE[username] = 1;
			return craftResponse(ERR, "The password is incorrect. " + std::to_string(MAX_LOGIN_ATTEMPTS - 1) + " attempt(s) left.");
		}
	}
}

response_message processLogout()
{
	if (tempUsername == "")
	{
		return craftResponse(ERR, "Not logged in yet\n");
	}
	if (LOGIN_STATE.find(tempUsername) == LOGIN_STATE.end())
	{
		return craftResponse(ERR, "The account is already logged out.\n");
	}
	LOGIN_STATE.erase(tempUsername);
	tempUsername.clear();
	login = false;
	return craftResponse(OK);
}

response_message craftResponse(msg_type type, std::string content)
{
	response_message response;
	response.type = type;
	strncpy_s(response.content, CONTENT_SIZE, content.c_str(), content.size());
	return response;
}

response_message craftResponse(msg_type type)
{
	response_message response;
	response.type = type;
	return response;
}

void sendResponse(response_message response, SOCKET connSock)
{
	char buff[BUFF_SIZE];
	memcpy(buff, &response, sizeof(response));
	uint32_t response_length = htonl(sizeof(response));
	send(connSock, (char*)&response_length, sizeof(response_length), 0);

	int nLeft = sizeof(response);
	int idx = 0;
	while (nLeft > 0)
	{
		int ret = send(connSock, &buff[idx], nLeft, 0);
		if (ret == SOCKET_ERROR)
		{
			printf("Error! Cannot send. %d\n", WSAGetLastError());
			break;
		}
		nLeft -= ret;
		idx += ret;
	}
}

void receiveRequest(SOCKET connSock)
{
	while (1)
	{
		char buff[BUFF_SIZE];
		int msg_len, idx, nLeft, ret;
		// Receive the length of incoming message
		ret = recv(connSock, buff, BUFF_SIZE - 1, 0);
		if (ret == SOCKET_ERROR)
		{
			printf("1Error: %d\n", WSAGetLastError());
			break;
		}
		else if (ret > 0)
		{
			// If ret > 0
			// Get length from buff
			msg_len = (int)ntohl(*reinterpret_cast<uint32_t*>(buff));
			idx = 0;
			nLeft = msg_len;

			// Loop until all message is received
			while (nLeft > 0)
			{
				ret = recv(connSock, &buff[idx], nLeft, 0);
				if (ret == SOCKET_ERROR)
				{
					break;
				}
				idx += ret;
				nLeft -= ret;
			}
			message_struct receive;
			memcpy(&receive, buff, msg_len);
			processRequest(receive, connSock);
		}
		// If ret = 0, the connection has been gracefully closed
		else if (ret == 0)
		{
			std::cout << "\n1\n";
			processLogout();
			break;
		}
	}
}
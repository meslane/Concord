#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <io.h>
#include <stdio.h>
#include <winsock2.h>
#include <vector>
#include <string.h>

#include <thread>
#include <chrono>

#pragma comment(lib,"ws2_32.lib") //Winsock Library

#define PORT 8888
#define DEFAULT_BUFLEN 512

std::vector<SOCKET> users;
std::vector<char*> usernames;

void printMsg(char* buf, unsigned short length) {
	for (unsigned short i = 0; i < length; i++) {
		putchar(buf[i]);
	}
}

void dropUser(SOCKET userPtr, char* name) {
	users.erase(std::remove(users.begin(), users.end(), userPtr), users.end());
	usernames.erase(std::remove(usernames.begin(), usernames.end(), name), usernames.end());
}

void clientSend(SOCKET sock, const char* message, int len) {
	send(sock, message, len, 0);
}

void sendToAll(const char* message, int len) {
	for (unsigned int i = 0; i < users.size(); i++) {
		clientSend(users[i], message, len);
	}
}

void disconnect(SOCKET sock, char* name) {
	char sendbuf[DEFAULT_BUFLEN + 68]; //message plus space for username
	int sendbuflen = DEFAULT_BUFLEN + 68;
	
	printf("Connection closed on ID#%d\n", sock);
	dropUser(sock, name);
	strcpy_s(sendbuf, sendbuflen, name);
	strcat_s(sendbuf, sendbuflen, " has disconnected\n");
	sendToAll(sendbuf, strlen(sendbuf));
}

void clientThread(void* param) { //runs in a thread until user disconnects
	SOCKET sock = (SOCKET)param;
	char* name = new char[64];

	char recvbuf[DEFAULT_BUFLEN];
	char sendbuf[DEFAULT_BUFLEN + 68]; //message plus space for username
	int recvbuflen = DEFAULT_BUFLEN;
	int sendbuflen = DEFAULT_BUFLEN + 68;
	char loggedIn = 0; //username is accepted

	printf("Connection accepted as ID#%d\n", sock);

	const char* welcome = "Connection accepted, please send your username for this session\n";
	clientSend(sock, welcome, strlen(welcome));

	int iResult;
	do {
		iResult = recv(sock, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			if (loggedIn == 0) { //authorize username
				if (iResult > 64) {
					clientSend(sock, "Username is too long, please resend\n", strlen("Username is too long, please resend\n"));
				}
				else {
					recvbuf[iResult] = '\0'; //null terminate
					strcpy_s(name, 64, recvbuf);
					strcpy_s(sendbuf, sendbuflen, "Username ");
					strcat_s(sendbuf, sendbuflen, name);
					strcat_s(sendbuf, sendbuflen, " accepted!\n");
					loggedIn = 1;
					clientSend(sock, sendbuf, strlen(sendbuf));

					users.push_back(sock);
					usernames.push_back(name); //allocate space for username

					strcpy_s(sendbuf, sendbuflen, name);
					strcat_s(sendbuf, sendbuflen, " has connected\n");
					sendToAll(sendbuf, strlen(sendbuf));
				}
			}
			else {
				//memset(sendbuf, 0, sendbuflen);
				strcpy_s(sendbuf, sendbuflen, "[");
				strcat_s(sendbuf, sendbuflen, name);
				strcat_s(sendbuf, sendbuflen, "] ");
				strcat_s(sendbuf, sendbuflen, recvbuf);
				strcat_s(sendbuf, sendbuflen, "\n");
				sendToAll(sendbuf, strlen(sendbuf)); //iResult + strlen(name) + strlen("[] \n")
			}
		}
		else if (iResult == 0) {
			disconnect(sock, name);
		}
		else {
			printf("recv failed: %d\n", WSAGetLastError());
			closesocket(sock);
			disconnect(sock, name);
		}

	} while (iResult > 0);

	delete[] name; //free upon exit
}

int main(int argc, char* argv[]) {
	WSADATA wsa;
	SOCKET s, new_socket;
	struct sockaddr_in server, client;
	int c;

	printf("\nInitialising Winsock...");
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		printf("Failed. Error Code : %d\n", WSAGetLastError());
		return 1;
	}

	printf("Initialised.\n");

	//Create a socket
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		printf("Could not create socket : %d\n", WSAGetLastError());
	}

	printf("Socket created\n");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(PORT);

	//Bind
	if (bind(s, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR)
	{
		printf("Bind failed with error code : %d\n", WSAGetLastError());
		exit(EXIT_FAILURE);
	}

	printf("Bind complete\n");

	//Listen to incoming connections
	listen(s, 3);
	printf("Waiting for incoming connections on port %d\n", PORT);

	c = sizeof(struct sockaddr_in);

	while ((new_socket = accept(s, (struct sockaddr*)&client, &c)) != INVALID_SOCKET) {
		_beginthread(&clientThread, 0, (LPVOID)new_socket);
	}

	if (new_socket == INVALID_SOCKET) {
		printf("accept failed with error code : %d\n", WSAGetLastError());
		return 1;
	}

	closesocket(s);
	WSACleanup();

	return 0;
}
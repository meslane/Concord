#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#include <thread>
#include <chrono>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "8888"

const char* address = "localhost";

void printMsg(char* buf, unsigned short length) {
    for (unsigned short i = 0; i < length; i++) {
        putchar(buf[i]);
    }
}

void recvThread(void* param) {
    SOCKET sock = (SOCKET)param;
    int iResult;
    char recvbuf[DEFAULT_BUFLEN + 68];
    int recvbuflen = DEFAULT_BUFLEN + 68;

    do {
        iResult = recv(sock, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            printMsg(recvbuf, iResult);
        }
        else if (iResult == 0) {
            printf("Connection closed\n");
            closesocket(sock);
            WSACleanup();
        }
        else {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(sock);
            WSACleanup();
        }

    } while (iResult > 0);
}

void sendThread(void* param) {
    SOCKET sock = (SOCKET)param;
    char sendbuf[DEFAULT_BUFLEN];
    int sendbuflen = DEFAULT_BUFLEN;
    int msgLen;

    while (1) {
        fgets(sendbuf, sendbuflen, stdin);
        sendbuf[strlen(sendbuf) - 1] = '\0';
        //strcat_s(sendbuf, sendbuflen, "\0");
        send(sock, sendbuf, strlen(sendbuf) + 1, 0);
    }
}

int main(int argc, char** argv)
{
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;
    const char* sendbuf = "this is a test";
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed with error: %d\n", WSAGetLastError());
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    if (getaddrinfo(address, DEFAULT_PORT, &hints, &result) != 0) {
        printf("getaddrinfo failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL;ptr = ptr->ai_next) {
        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    // Send an initial buffer
    /*
    iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    printf("Bytes Sent: %ld\n", iResult);
    */

    /*
    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }
    */

    // Receive until the peer closes the connection
    /*
    do {

        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0)
            printMsg(recvbuf, iResult);
        else if (iResult == 0)
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while (iResult > 0);
    */
    _beginthread(&recvThread, 0, (LPVOID)ConnectSocket);
    _beginthread(&sendThread, 0, (LPVOID)ConnectSocket);

    while (recv(ConnectSocket, NULL, 0, 0) != SOCKET_ERROR) {
        continue;
    }

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();

    return 0;
}
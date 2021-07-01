#include <winsock2.h>
#include <Windows.h>
#include <stdio.h>

#define _SOCKET(name) SOCKET WSAAPI name(int af, int type, int protocol);
typedef _SOCKET(_socket);

#define _CONNECT(name) int WSAAPI name(SOCKET s, const sockaddr *name, int namelen);
typedef _CONNECT(_connect);

#define _SEND(name) int WSAAPI name(SOCKET s, const char *buf, int len, int flags);
typedef _SEND(_send);

int CALLBACK
WinMain(HINSTANCE hInstance,
		HINSTANCE hPrevInstance,
		LPSTR lpCmdLine,
		int nCmdShow)
{
	HMODULE libraryWinsock2 = LoadLibraryA("Ws2_32.dll");
	if ( !libraryWinsock2 )
	{
		OutputDebugStringA("Loading libraryWinsock2 failed.");
		return 1;
	}
	
	WSADATA wsaData;
    if ( WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR ) {
		OutputDebugStringA("WSAStartup failed.");
        return 1;
    }

	_socket* fSocket = (_socket*)GetProcAddress(libraryWinsock2, "socket");

	SOCKET socketConnection;
	socketConnection = fSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( socketConnection == INVALID_SOCKET )
	{	
		char buffer[256];
		sprintf(buffer, "Socket failed with error: %ld\n", WSAGetLastError());
		OutputDebugStringA(buffer);
		
        WSACleanup();
        return 1;
	}

	struct sockaddr_in socketAddress;
    socketAddress.sin_family = AF_INET;
	socketAddress.sin_port = htons(65432);
	socketAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

	_connect* fConnect = (_connect*)GetProcAddress(libraryWinsock2, "connect");
	if ( fConnect(socketConnection, (SOCKADDR*) &socketAddress, sizeof(socketAddress)) == SOCKET_ERROR )
	{
		char buffer[256];
		sprintf(buffer, "Socket failed with error: %ld\nReconnecting...", WSAGetLastError());
		OutputDebugStringA(buffer);

		// Try to reconnect
		closesocket(socketConnection);
		WSACleanup();
		Sleep(2000);
	}

	_send* fSend = (_send*)GetProcAddress(libraryWinsock2, "send");
	const char* socketBuffer = "THIS IS A TEST.";
	if ( fSend(socketConnection, socketBuffer, (int)strlen(socketBuffer), 0) == SOCKET_ERROR )
	{
		char buffer[256];
		sprintf(buffer, "Socket failed with error: %ld\n", WSAGetLastError());
		OutputDebugStringA(buffer);
	}
	
	return 0;
}

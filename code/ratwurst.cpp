#include <winsock2.h>
#include <Windows.h>
#include <stdio.h>

#define _SOCKET(name) SOCKET WSAAPI name(int af, int type, int protocol);
typedef _SOCKET(_socket);

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

	SOCKET socketConnection;

	_socket* fSocket = (_socket*)GetProcAddress(libraryWinsock2, "socket");
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

	if ( connect(socketConnection, (SOCKADDR*) &socketAddress, sizeof(socketAddress)) == SOCKET_ERROR )
	{
		char buffer[256];
		sprintf(buffer, "Socket failed with error: %ld\nReconnecting...", WSAGetLastError());
		OutputDebugStringA(buffer);

		// Try to reconnect
		closesocket(socketConnection);
		WSACleanup();
		Sleep(2000);
	}
	
	const char* socketBuffer = "THIS IS A TEST.";
	if ( send(socketConnection, socketBuffer, (int)strlen(socketBuffer), 0) == SOCKET_ERROR )
	{
		char buffer[256];
		sprintf(buffer, "Socket failed with error: %ld\n", WSAGetLastError());
		OutputDebugStringA(buffer);
	}
	
	return 0;
}

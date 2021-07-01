#include <winsock2.h>
#include <Windows.h>
#include <stdio.h>

int CALLBACK
WinMain(HINSTANCE hInstance,
		HINSTANCE hPrevInstance,
		LPSTR lpCmdLine,
		int nCmdShow)
{
	WSADATA wsaData;
    if ( WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR ) {
		OutputDebugStringA("WSAStartup failed.");
        return 1;
    }

	SOCKET socketConnection;
	socketConnection = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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
	socketAddress.sin_port = 65432;
	socketAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

	if ( connect(socketConnection, (SOCKADDR*) &socketAddress, sizeof(socketAddress)) == SOCKET_ERROR )
	{
		OutputDebugStringA("Error with connecting to socket.");
	}
	
	const char* socketBuffer = "This is a test.";
	if ( send(socketConnection, socketBuffer, (int)strlen(socketBuffer), 0) == SOCKET_ERROR )
	{
		OutputDebugStringA("Error with sending socket.");
	}
	
	return 0;
}

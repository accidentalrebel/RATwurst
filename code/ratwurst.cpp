#include <winsock2.h>
#include <Windows.h>
#include <stdio.h>

// Ws2_32.dll definitions
typedef SOCKET WSAAPI _socket(int af, int type, int protocol);
typedef int WSAAPI _connect(SOCKET s, const sockaddr *name, int namelen);
typedef int WSAAPI _send(SOCKET s, const char *buf, int len, int flags);
typedef int WSAAPI _closesocket(SOCKET s);
typedef u_short _htons(u_short hostshort);
typedef unsigned long WSAAPI _inet_addr(const char *cp);
typedef int WSAAPI _WSAStartup(WORD wVersionRequested, LPWSADATA lpWSAData);
typedef int _WSACleanup();
typedef int WSAAPI _WSAGetLastError();

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

	_WSAGetLastError *fWSAGetLastError = (_WSAGetLastError*)GetProcAddress(libraryWinsock2, "WSAGetLastError");
	_WSAStartup* fWSAStartup = (_WSAStartup*)GetProcAddress(libraryWinsock2, "WSAStartup");
    if ( fWSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR ) {
		OutputDebugStringA("WSAStartup failed.");
        return 1;
    }

	_socket* fSocket = (_socket*)GetProcAddress(libraryWinsock2, "socket");

	SOCKET socketConnection;
	socketConnection = fSocket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if ( socketConnection == INVALID_SOCKET )
	{	
		char buffer[256];
		sprintf(buffer, "Socket failed with error: %ld\n", fWSAGetLastError());
		OutputDebugStringA(buffer);

		_WSACleanup* fWSACleanup = (_WSACleanup*)GetProcAddress(libraryWinsock2, "WSACleanup");
        fWSACleanup();
        return 1;
	}

	_htons* fHtons = (_htons*)GetProcAddress(libraryWinsock2, "htons");
	_inet_addr* fInet_addr = (_inet_addr*)GetProcAddress(libraryWinsock2, "inet_addr");
	
	struct sockaddr_in socketAddress;
    socketAddress.sin_family = AF_INET;
	socketAddress.sin_port = fHtons(65432);
	socketAddress.sin_addr.s_addr = fInet_addr("127.0.0.1");

	_connect* fConnect = (_connect*)GetProcAddress(libraryWinsock2, "connect");
	if ( fConnect(socketConnection, (SOCKADDR*) &socketAddress, sizeof(socketAddress)) == SOCKET_ERROR )
	{
		char buffer[256];
		sprintf(buffer, "Socket failed with error: %ld\nReconnecting...", fWSAGetLastError());
		OutputDebugStringA(buffer);

		// Try to reconnect
		_closesocket* fCloseSocket = (_closesocket*)GetProcAddress(libraryWinsock2, "closesocket");
		fCloseSocket(socketConnection);

		_WSACleanup* fWSACleanup = (_WSACleanup*)GetProcAddress(libraryWinsock2, "WSACleanup");
		fWSACleanup();
		Sleep(2000);
	}

	_send* fSend = (_send*)GetProcAddress(libraryWinsock2, "send");
	const char* socketBuffer = "THIS IS A TEST.";
	if ( fSend(socketConnection, socketBuffer, (int)strlen(socketBuffer), 0) == SOCKET_ERROR )
	{
		char buffer[256];
		sprintf(buffer, "Socket failed with error: %ld\n", fWSAGetLastError());
		OutputDebugStringA(buffer);
	}
	
	return 0;
}

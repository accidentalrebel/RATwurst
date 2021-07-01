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
	char ca_ws2_32[] = {'W','s','2','_','3','2','.','d','l','l', 0};
	HMODULE libraryWinsock2 = LoadLibraryA(ca_ws2_32);
	if ( !libraryWinsock2 )
	{
		OutputDebugStringA("Loading libraryWinsock2 failed.\n");
		return 1;
	}

	char ca_WSAGetLastError[] = { 'W','S','A','G','e','t','L','a','s','t','E','r','r','o','r', 0 };
	char ca_WSAStartup[] = { 'W','S','A','S','t','a','r','t','u','p',0 };
	_WSAGetLastError *f_WSAGetLastError = (_WSAGetLastError*)GetProcAddress(libraryWinsock2, ca_WSAGetLastError);
	
	SOCKET socketConnection;
	
	while(true)
	{
		WSADATA wsaData;

		_WSAStartup* f_WSAStartup = (_WSAStartup*)GetProcAddress(libraryWinsock2, ca_WSAStartup);
		if ( f_WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR )
		{
			OutputDebugStringA("WSAStartup failed.\n");
			return 1;
		}

		char ca_socket[] = { 's','o','c','k','e','t',0 };
		_socket* f_socket = (_socket*)GetProcAddress(libraryWinsock2, ca_socket);

		socketConnection = f_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if ( socketConnection == INVALID_SOCKET )
		{	
			char buffer[256];
			sprintf(buffer, "Socket failed with error: %ld\n", f_WSAGetLastError());
			OutputDebugStringA(buffer);

			char ca_WSACleanup[] = { 'W','S','A','C','l','e','a','n','u','p',0 };
			_WSACleanup* f_WSACleanup = (_WSACleanup*)GetProcAddress(libraryWinsock2, ca_WSACleanup);
			f_WSACleanup();
			return 1;
		}

		char ca_htons[] = { 'h','t','o','n','s',0 };
		char ca_inet_addr[] = { 'i','n','e','t','_','a','d','d','r',0 };
	
		_htons* f_htons = (_htons*)GetProcAddress(libraryWinsock2, ca_htons);
		_inet_addr* f_inet_addr = (_inet_addr*)GetProcAddress(libraryWinsock2, ca_inet_addr);

		char ca_ip_addr[] = { '1','2','7','.','0','.','0','.','1', 0 };
	
		struct sockaddr_in socketAddress;
		socketAddress.sin_family = AF_INET;
		socketAddress.sin_port = f_htons(65432);
		socketAddress.sin_addr.s_addr = f_inet_addr(ca_ip_addr);

		char ca_connect[] = { 'c','o','n','n','e','c','t', 0};
		_connect* f_connect = (_connect*)GetProcAddress(libraryWinsock2, ca_connect);

		if ( f_connect(socketConnection, (SOCKADDR*) &socketAddress, sizeof(socketAddress)) != SOCKET_ERROR )
		{
			OutputDebugStringA("Connection successful!\n");
			break;
		}
		else
		{
			char buffer[256];
			sprintf(buffer, "Socket failed with error: %ld\nReconnecting...\n", f_WSAGetLastError());
			OutputDebugStringA(buffer);

			// Try to reconnect
			// 
			char ca_closesocket[] = { 'c','l','o','s','e','s','o','c','k','e','t',0 };
			_closesocket* f_closesocket = (_closesocket*)GetProcAddress(libraryWinsock2, ca_closesocket);
			f_closesocket(socketConnection);

			char ca_WSACleanup[] = { 'W','S','A','C','l','e','a','n','u','p',0 };
			_WSACleanup* f_WSACleanup = (_WSACleanup*)GetProcAddress(libraryWinsock2, ca_WSACleanup);
			f_WSACleanup();
			
			Sleep(10000);
		}
	}

	char ca_send[] = { 's','e','n','d', 0};
	_send* f_send = (_send*)GetProcAddress(libraryWinsock2, ca_send);
	const char* socketBuffer = "THIS IS A TEST.";
	if ( f_send(socketConnection, socketBuffer, (int)strlen(socketBuffer), 0) == SOCKET_ERROR )
	{
		char buffer[256];
		sprintf(buffer, "Socket failed with error: %ld\n", f_WSAGetLastError());
		OutputDebugStringA(buffer);
	}
	
	return 0;
}

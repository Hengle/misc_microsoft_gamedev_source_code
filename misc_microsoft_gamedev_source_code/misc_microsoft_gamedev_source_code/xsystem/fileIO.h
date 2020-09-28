// File: fileIO.h
#pragma once

class BFileIO 
{
public:
   BFileIO();
   ~BFileIO();
   
   HANDLE               createFileA(const char* lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
   BOOL                 closeHandle(HANDLE hObject);

   BOOL                 readFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
   BOOL                 writeFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
   DWORD                setFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod);

   BOOL                 getFileInformationByHandle(HANDLE hFile, LPBY_HANDLE_FILE_INFORMATION lpFileInformation);
   DWORD                getFileAttributes(LPCSTR lpFileName);
   BOOL                 getFileTime(HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime);

   HANDLE               findFirstFile(LPCSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData);
   BOOL                 findNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData);
   BOOL                 findClose(HANDLE hFindFile);
   
   // Not currently implemented if XFS is active.
   BOOL                 getOverlappedResult(HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait);

   void                 setOwnerThread(HANDLE hFile, DWORD threadID);
};

extern BFileIO gFileIO;
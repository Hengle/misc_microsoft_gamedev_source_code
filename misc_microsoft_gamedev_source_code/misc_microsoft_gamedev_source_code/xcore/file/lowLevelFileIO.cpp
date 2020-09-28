//============================================================================
//
// File: lowLevelFileIO.cpp
//
// Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "lowLevelFileIO.h"

ILowLevelFileIO* ILowLevelFileIO::gpDefaultLowLevelIO = &gWin32LowLevelFileIO;

void ILowLevelFileIO::changeDefault(ILowLevelFileIO* pNewLowLevelFileIO)
{
   if (!pNewLowLevelFileIO)
      pNewLowLevelFileIO = &gWin32LowLevelFileIO;
   
   gpDefaultLowLevelIO = pNewLowLevelFileIO;
   
   trace("ILowLevelFileIO::changeDefault: Changing to %s\n", pNewLowLevelFileIO->getName());
}

HANDLE BWin32LowLevelFileIO::createFileA(const char* lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
   return CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

BOOL BWin32LowLevelFileIO::closeHandle(HANDLE hObject)
{
   return CloseHandle(hObject);
}

BOOL BWin32LowLevelFileIO::flush(HANDLE hObject)
{
   return FlushFileBuffers(hObject);
}

BOOL BWin32LowLevelFileIO::readFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
   return ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}

BOOL BWin32LowLevelFileIO::writeFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
   return WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

DWORD BWin32LowLevelFileIO::setFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
   return SetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
}

DWORD BWin32LowLevelFileIO::getFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh)
{
   return GetFileSize(hFile, lpFileSizeHigh);
}

BOOL BWin32LowLevelFileIO::getFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize)
{
   return GetFileSizeEx(hFile, lpFileSize);
}

BOOL BWin32LowLevelFileIO::getFileInformationByHandle(HANDLE hFile, LPBY_HANDLE_FILE_INFORMATION lpFileInformation)
{
   return GetFileInformationByHandle(hFile, lpFileInformation);
}

DWORD BWin32LowLevelFileIO::getFileAttributes(LPCSTR lpFileName)
{
   return GetFileAttributes(lpFileName);
}

BOOL BWin32LowLevelFileIO::getFileTime(HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime)
{
   return GetFileTime(hFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime);
}

HANDLE BWin32LowLevelFileIO::findFirstFile(LPCSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData)
{
   return FindFirstFile(lpFileName, lpFindFileData);
}

BOOL BWin32LowLevelFileIO::findNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData)
{
   return FindNextFile(hFindFile, lpFindFileData);
}

BOOL BWin32LowLevelFileIO::findClose(HANDLE hFindFile)
{
   return FindClose(hFindFile);
}

BOOL BWin32LowLevelFileIO::getOverlappedResult(HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait)
{
   return GetOverlappedResult(hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait);
}

BOOL BWin32LowLevelFileIO::setEndOfFile(HANDLE hFile)
{
   return SetEndOfFile(hFile);
}

BOOL BWin32LowLevelFileIO::setFileAttributes(LPCSTR lpFileName, DWORD dwFileAttributes)
{
   return SetFileAttributes(lpFileName, dwFileAttributes);
}

void BWin32LowLevelFileIO::setOwnerThread(HANDLE hFile, DWORD threadID)
{
   hFile;
   threadID;
}

BOOL BWin32LowLevelFileIO::hasOverlappedIoCompleted(LPOVERLAPPED lpOverlapped)
{
   return HasOverlappedIoCompleted(lpOverlapped);
}

BOOL BWin32LowLevelFileIO::getFileAttributesEx(LPCSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation)
{
   return GetFileAttributesEx(lpFileName, fInfoLevelId, lpFileInformation);
}

BOOL BWin32LowLevelFileIO::setFileTime(HANDLE hFile, CONST FILETIME *lpCreationTime, CONST FILETIME *lpLastAccessTime, CONST FILETIME *lpLastWriteTime)
{
   return SetFileTime(hFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime);
}

DWORD BWin32LowLevelFileIO::getFullPathName(LPCTSTR lpFileName, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR* lpFilePart)
{
#ifdef XBOX
   SetLastError(ERROR_INVALID_FUNCTION);
   return 0;
#else
   return GetFullPathName(lpFileName, nBufferLength, lpBuffer, lpFilePart);  
#endif   
}

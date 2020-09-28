//============================================================================
//
// File: lowLevelFileIO.h
//
// Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#pragma once

class ILowLevelFileIO
{
public:
   virtual ~ILowLevelFileIO() { }
   
   virtual HANDLE                createFileA(const char* lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) = 0;
   virtual BOOL                  closeHandle(HANDLE hObject) = 0;
   virtual BOOL                  flush(HANDLE hObject) = 0;

   virtual BOOL                  readFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) = 0;
   virtual BOOL                  writeFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) = 0;
   virtual DWORD                 setFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod) = 0;
   
   virtual DWORD                 getFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh) = 0;
   
   virtual BOOL                  getFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize) = 0;

   virtual BOOL                  getFileInformationByHandle(HANDLE hFile, LPBY_HANDLE_FILE_INFORMATION lpFileInformation) = 0;
   
   virtual BOOL                  setFileAttributes(LPCSTR lpFileName, DWORD dwFileAttributes) = 0;
   virtual DWORD                 getFileAttributes(LPCSTR lpFileName) = 0;
   
   virtual BOOL                  getFileTime(HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime) = 0;

   virtual HANDLE                findFirstFile(LPCSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData) = 0;
   virtual BOOL                  findNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData) = 0;
   virtual BOOL                  findClose(HANDLE hFindFile) = 0;

   virtual BOOL                  getOverlappedResult(HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait) = 0;

   virtual void                  setOwnerThread(HANDLE hFile, DWORD threadID) = 0;
   
   virtual BOOL                  hasOverlappedIoCompleted(LPOVERLAPPED lpOverlapped) = 0;

   virtual BOOL                  setEndOfFile(HANDLE hFile) = 0;
         
   virtual BOOL                  getFileAttributesEx(LPCSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation) = 0;

   virtual BOOL                  setFileTime(HANDLE hFile, CONST FILETIME *lpCreationTime, CONST FILETIME *lpLastAccessTime, CONST FILETIME *lpLastWriteTime) = 0;
   
   virtual DWORD                 getFullPathName(LPCTSTR lpFileName, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR* lpFilePart) = 0;

   // DO NOT return "Win32" unless the handles returned by the implementation are true Win32 handles!         
   virtual const char*           getName(void) = 0;
               
   static ILowLevelFileIO*       getDefault(void) { return gpDefaultLowLevelIO; }
   static void                   changeDefault(ILowLevelFileIO* pNewDefault = NULL);
   
protected:
   static ILowLevelFileIO* gpDefaultLowLevelIO;
};

class BWin32LowLevelFileIO : public ILowLevelFileIO
{
public:
   BWin32LowLevelFileIO() { }
   virtual ~BWin32LowLevelFileIO() { }
   
   virtual HANDLE                createFileA(const char* lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
   virtual BOOL                  closeHandle(HANDLE hObject);
   virtual BOOL                  flush(HANDLE hObject);

   virtual BOOL                  readFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
   virtual BOOL                  writeFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
   virtual DWORD                 setFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod);
      
   virtual DWORD                 getFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh);
   virtual BOOL                  getFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize);

   virtual BOOL                  getFileInformationByHandle(HANDLE hFile, LPBY_HANDLE_FILE_INFORMATION lpFileInformation);
   virtual DWORD                 getFileAttributes(LPCSTR lpFileName);
   virtual BOOL                  getFileTime(HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime);

   virtual HANDLE                findFirstFile(LPCSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData);
   virtual BOOL                  findNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData);
   virtual BOOL                  findClose(HANDLE hFindFile);
   
   virtual BOOL                  getOverlappedResult(HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait);
   
   virtual BOOL                  setEndOfFile(HANDLE hFile);

   virtual void                  setOwnerThread(HANDLE hFile, DWORD threadID);
   
   virtual BOOL                  setFileAttributes(LPCSTR lpFileName, DWORD dwFileAttributes);
   
   virtual BOOL                  getFileAttributesEx(LPCSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation);

   virtual BOOL                  setFileTime(HANDLE hFile, CONST FILETIME *lpCreationTime, CONST FILETIME *lpLastAccessTime, CONST FILETIME *lpLastWriteTime);
   
   virtual DWORD                 getFullPathName(LPCTSTR lpFileName, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR* lpFilePart);
      
   virtual const char*           getName(void) { return "Win32"; }
      
   virtual BOOL                  hasOverlappedIoCompleted(LPOVERLAPPED lpOverlapped);
};

extern BWin32LowLevelFileIO gWin32LowLevelFileIO;

// File: fileIO.cpp
#include "xsystem.h"
#include "fileIO.h"
#include "xfs.h"

//==============================================================================
// Globals
//==============================================================================
BFileIO gFileIO;

//==============================================================================
// BFileIO::BFileIO
//==============================================================================
BFileIO::BFileIO()
{
}

//==============================================================================
// BFileIO::~BFileIO
//==============================================================================
BFileIO::~BFileIO()
{

}

//==============================================================================
// BFileIO::createFileA
//==============================================================================
HANDLE BFileIO::createFileA(const char* lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
   return gXFS.createFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

//==============================================================================
// BFileIO::closeHandle
//==============================================================================
BOOL BFileIO::closeHandle(HANDLE hObject)
{
   return gXFS.closeHandle(hObject);
}

//==============================================================================
// BFileIO::readFile
//==============================================================================
BOOL BFileIO::readFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
   return gXFS.readFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}

//==============================================================================
// BFileIO::writeFile
//==============================================================================
BOOL BFileIO::writeFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
   return gXFS.writeFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

//==============================================================================
// BFileIO::setFilePointer
//==============================================================================
DWORD BFileIO::setFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
   return gXFS.setFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
}

//==============================================================================
// BFileIO::getFileInformationByHandle
//==============================================================================
BOOL BFileIO::getFileInformationByHandle(HANDLE hFile, LPBY_HANDLE_FILE_INFORMATION lpFileInformation)
{
   return gXFS.getFileInformationByHandle(hFile, lpFileInformation);
}

//==============================================================================
// 
//==============================================================================
DWORD BFileIO::getFileAttributes(LPCSTR lpFileName)
{
   return gXFS.getFileAttributes(lpFileName);
}

//==============================================================================
// BFileIO::getFileTime
//==============================================================================
BOOL BFileIO::getFileTime(HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime)
{
   return gXFS.getFileTime(hFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime);
}

//==============================================================================
// BFileIO::findFirstFile
//==============================================================================
HANDLE BFileIO::findFirstFile(LPCSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData)
{
   return gXFS.findFirstFile(lpFileName, lpFindFileData);
}

//==============================================================================
// BFileIO::findNextFile
//==============================================================================
BOOL BFileIO::findNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData)
{
   return gXFS.findNextFile(hFindFile, lpFindFileData);
}

//==============================================================================
// BFileIO::findClose
//==============================================================================
BOOL BFileIO::findClose(HANDLE hFindFile)
{
   return gXFS.findClose(hFindFile);
}

//==============================================================================
// BFileIO::getOverlappedResult
//==============================================================================
BOOL BFileIO::getOverlappedResult(HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait)
{
   return gXFS.getOverlappedResult(hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait);
}

//==============================================================================
// BFileIO::setOwnerThread
//==============================================================================
void BFileIO::setOwnerThread(HANDLE hFile, DWORD threadID)
{
   return gXFS.setOwnerThread(hFile, threadID);
}


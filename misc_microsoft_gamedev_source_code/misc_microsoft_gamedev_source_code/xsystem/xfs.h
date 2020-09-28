//============================================================================
// xfs.h
//
// Copyright (c) 2005, Ensemble Studios
//============================================================================
#pragma once

#ifndef XBOX
#include <winsock2.h>
#endif

#include "xexception\xexception.h"
#include "file\lowLevelFileIO.h"

#define USE_XFS

class BXFSFile;

//============================================================================
// BXFS
//============================================================================
class BXFS
{
   public:
                               BXFS();
                               ~BXFS();

      typedef void (*BMessageCallbackPtr)(const char* pMsg);
      bool                     setup(const char* pServerIP, bool disableNoDelay, BMessageCallbackPtr pMsgCallback = NULL, WORD connectionPort=1000, bool xfsCopy=false);
      void                     shutdown();
      bool                     isActive() const { return mActive; }

      WORD                     getConnectionPort() const { return mConnectionPort; }
      const char*              getServerIP() const { return mServerIP; }

      HANDLE                   createFile(const TCHAR* lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
      HANDLE                   createFileA(const char* lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
      HANDLE                   createFileW(const WCHAR* lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
      BOOL                     closeHandle(HANDLE hObject);
      BOOL                     readFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
      BOOL                     writeFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
      DWORD                    getFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh);
      BOOL                     getFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize);
      BOOL                     getFileInformationByHandle(HANDLE hFile, LPBY_HANDLE_FILE_INFORMATION lpFileInformation);
      DWORD                    setFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod);
      BOOL                     createDirectory(LPCSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
      DWORD                    getFileAttributes(LPCSTR lpFileName);
      BOOL                     getFileTime(HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime);
      HANDLE                   findFirstFile(LPCSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData);
      BOOL                     findNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData);
      BOOL                     findClose(HANDLE hFindFile);
      BOOL                     flushFileBuffers(HANDLE hFile);
      BOOL                     getOverlappedResult(HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait);
      void                     setOwnerThread(HANDLE hFile, DWORD threadID);
      BOOL                     hasOverlappedIoCompleted(LPOVERLAPPED lpOverlapped) const;
      BOOL                     setFileAttributes(LPCSTR lpFileName, DWORD dwFileAttributes);
      BOOL                     setEndOfFile(HANDLE hFile);
      BOOL                     getFileAttributesEx(LPCSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation);
      BOOL                     setFileTime(HANDLE hFile, CONST FILETIME *lpCreationTime, CONST FILETIME *lpLastAccessTime, CONST FILETIME *lpLastWriteTime);
      DWORD                    getFullPathName(LPCTSTR lpFileName, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR* lpFilePart);
            
      BOOL                     setFailReportDirectory( const char* cpDir );
      static const char*       getFailReportDirectory() { return mpFailReportDir; }
      static void              setFailReportFileName( const char* cpInFileName, char* cpOutFileName, const long cOutSize );
      static void              setFPSLogFileName( const char* cpInFileName, char* pOutFileName, const long cOutSize );
      XEXCEPTION_CALLBACK_FUNC getExceptionCallback() { return ExceptionCallback; }

   protected:

      bool                 setupServerIP(const char* pServerIP, BMessageCallbackPtr pMsgCallback, bool xfsCopy);

      BXFSFile*            getFile(HANDLE hFile);
      BXFSFile*            acquireFile();
      void                 releaseFile(BXFSFile* file, bool reuse = true);

      enum
      {
         cMaxFiles=16,
      };

      WORD                 mConnectionPort;
      char                 mServerIP[16];
      bool                 mActive;
      bool                 mNetStart;
      BOOL                 mEnableNoDelay;

      BXFSFile*            mFileList[cMaxFiles];
      long                 mFileCount;

      BXFSFile*            mPoolList[cMaxFiles];
      long                 mPoolCount;

      BCriticalSection     mCrit;

      static char          mpFailReportDir[256];

      static void          AssertPreCallback( const char *expression, const char *msg, const char *file, long line, bool fatal, bool noreturn, const BDebugCallstack* pCallstack, void *param1, void *param2, const char* cpFullAssertError );
      
      // Write fail report file to XFS machine
      static void          DumpFailReport( const char* cpErrorMsg, const char* cpFile, BOOL isException );

      // Exception callback function
      static void          ExceptionCallback( const char* cpMsg, BOOL die );
};

//============================================================================
// BXFSFile
//============================================================================
class BXFSFile
{
   public:
                           BXFSFile();
                           ~BXFSFile();

      bool                 setup(BOOL enableNoDelay=TRUE);
      void                 reset();
      void                 clearFindFileList();

      HANDLE               createFile(const TCHAR* lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
      HANDLE               createFileA(const char* lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
      HANDLE               createFileW(const WCHAR* lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);
      BOOL                 closeHandle(HANDLE hObject);
      BOOL                 readFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
      BOOL                 writeFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped);
      
      BOOL                 getFileInformationByHandle(HANDLE hFile, LPBY_HANDLE_FILE_INFORMATION lpFileInformation);
      DWORD                setFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod);
      BOOL                 createDirectory(LPCSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes);
      DWORD                getFileAttributes(LPCSTR lpFileName);
      DWORD                getFileAttributesEx(LPCSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation);
      BOOL                 getFileTime(HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime);
      HANDLE               findFirstFile(LPCSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData);
      BOOL                 findNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData);
      BOOL                 findClose(HANDLE hFindFile);
      BOOL                 flushFileBuffers(HANDLE hFile);
            
      bool                 resetServer(void);
      
      HANDLE               getHandle(void) const { return mHandle; }
      void                 setOwnerThread(DWORD threadID) {mOwnerThread = threadID;} //Only for special case when opening and reading from different threads. Make sure it's safe to do this.

private:      
      void                 sendBegin(BYTE func);
      void                 sendAddBYTE(BYTE val);
      void                 sendAddWORD(WORD val);
      void                 sendAddDWORD(DWORD val);
      void                 sendAddHandle(HANDLE val);
      void                 sendAddString(const TCHAR* str);
      bool                 sendEnd();

      bool                 recvData(BYTE* buffer, long offset, long size);
      bool                 recvBYTE(BYTE& data);
      bool                 recvWORD(WORD& data);
      bool                 recvDWORD(DWORD& data);
      bool                 recvUINT64(uint64& data);
      bool                 recvHandle(HANDLE& data);
      bool                 recvLARGE_INTEGER(LARGE_INTEGER& data);
      bool                 recvInt(int& data);
      bool                 recvLong(long& data);
      bool                 recvBOOL(BOOL& data);
      bool                 recvBool(bool& data);
      bool                 recvSkip(long size);
      
      bool                 waitForReply(int desiredFunc);      
      
      SOCKET               mSocket;
      BYTE*                mBuffer;
      long                 mBufferSize;

      HANDLE               mHandle;
      DWORD                mFileOffset;

      DWORD                mCacheSize;
      DWORD                mCacheStart;
      BYTE*                mCache;

      BY_HANDLE_FILE_INFORMATION*   mFileInfo;

      WIN32_FIND_DATA*     mFindFileList;
      long                 mFindFileCount;
      long                 mFindFileIndex;
      
      DWORD                mCreateFlags;
      
      DWORD                mOwnerThread;
#ifdef BUILD_DEBUG
      volatile LONG        mReadingThreadsCount;
#endif
      
      void                 setHandle(HANDLE handle) { Sync::InterlockedExchangeExport((LONG*)&mHandle, (LONG)handle); }
};

extern BXFS gXFS;

//============================================================================
// BXFSLowLevelFileIO
//============================================================================
class BXFSLowLevelFileIO : public ILowLevelFileIO
{
public:
   BXFSLowLevelFileIO() { }
   virtual ~BXFSLowLevelFileIO() { }

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

   virtual void                  setOwnerThread(HANDLE hFile, DWORD threadID);
   
   virtual BOOL                  hasOverlappedIoCompleted(LPOVERLAPPED lpOverlapped);
   
   virtual BOOL                  setEndOfFile(HANDLE hFile);
   
   virtual BOOL                  setFileAttributes(LPCSTR lpFileName, DWORD dwFileAttributes);
   
   virtual BOOL                  getFileAttributesEx(LPCSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation);
   virtual BOOL                  setFileTime(HANDLE hFile, CONST FILETIME *lpCreationTime, CONST FILETIME *lpLastAccessTime, CONST FILETIME *lpLastWriteTime);
   
   virtual DWORD                 getFullPathName(LPCTSTR lpFileName, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR* lpFilePart);
   
   virtual const char*           getName(void) { return "XFS"; }
};

extern BXFSLowLevelFileIO gXFSLowLevelFileIO;

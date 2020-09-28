//============================================================================
// xfs.cpp
//
// Copyright (c) 2005, Ensemble Studios
//============================================================================
#include "xsystem.h"
#include "xfs.h"

#if defined( XBOX )
   #include "threading\eventDispatcher.h"
   #include <xbdm.h>
   #include "xdb\xdbManager.h"
#endif

#include "config.h"

BXFSLowLevelFileIO gXFSLowLevelFileIO;

// Constants
const BYTE cCommandPrefix='@';
const BYTE cCommandSuffix='#';

const DWORD cOpenInitialRead=4;

const DWORD cBufferSize=10240;//10240;
const DWORD cCacheSize=10240;//102400;

char BXFS::mpFailReportDir[256];

enum
{
   cFuncCreateFile=1,
   cFuncCloseHandle,
   cFuncReadFile,
   cFuncWriteFile,
   cFuncSetFilePointer,
   cFuncCreateDirectory,
   cFuncGetFileAttributes,
   cFuncFindFiles,
   cFuncFlushFileBuffers,
   cFuncTest,
   cFuncReset,
   cFuncGetFileAttributesEx,
   cErrorCode=255
};

//============================================================================
// gXFS
//============================================================================
BXFS gXFS;

//============================================================================
// BXFS::BXFS
//============================================================================
BXFS::BXFS() :
   mActive(false),
   mNetStart(false),
   mEnableNoDelay(TRUE),
   mFileCount(0),
   mPoolCount(0)
{
   memset(mServerIP, NULL, sizeof(mServerIP));
   memset(mFileList, NULL, sizeof(mFileList));
   memset(mPoolList, NULL, sizeof(mPoolList));
   memset( mpFailReportDir, NULL, sizeof( mpFailReportDir ) );

   gAssertionSystem.addPreCallback(AssertPreCallback, NULL, NULL);
}

//============================================================================
// BXFS::~BXFS
//============================================================================
BXFS::~BXFS()
{
   shutdown();
}

//============================================================================
// BXFS::AssertPreCallback
//============================================================================
void BXFS::AssertPreCallback( const char *expression, const char *msg, const char *file, long line, bool fatal, bool noreturn, const BDebugCallstack* pCallstack, void *param1, void *param2, const char* cpFullAssertError )
{
   expression;
   msg;
   file;
   line;
   fatal;
   param1;
   param2;
   cpFullAssertError;

   __try
   {
      DumpFailReport( cpFullAssertError, file, false );
   }
   __except( EXCEPTION_EXECUTE_HANDLER )
   {
   }

   if (noreturn)
   {
      trace("A noreturn assert/fail has occurred -- shutting down XFS!");
      gXFS.shutdown();
   }
}

//============================================================================
// BXFS::setFailReportFileName
//
// Set path and file name for fail report
//============================================================================
void BXFS::setFailReportFileName( const char* cpInFileName, char* pOutFileName, const long cOutSize )
{
#if defined(XBOX) && !defined(BUILD_FINAL)
   const long cXboxNameSize = 256;
   char       pXboxName[cXboxNameSize];

   // Get name from xbox
   HRESULT result = DmGetXboxName( pXboxName, (LPDWORD)&cXboxNameSize );

   // Get time from system
   SYSTEMTIME sysTime;
   GetLocalTime( &sysTime );      

   // Make sure fail report directory exists
   BOOL dirSuccess = false;
   if( mpFailReportDir[0] )
   {
      dirSuccess = gXFS.createDirectory( mpFailReportDir, NULL );
   }

   // Set file name with time stamp
   char pDumpName[256];
   sprintf_s( pDumpName, sizeof( pDumpName ), "%s_%d-%d-%d.txt", cpInFileName, sysTime.wHour, sysTime.wMinute, sysTime.wSecond );

   // Pre-pend directory and 360 name to fail report file name
   if( dirSuccess && ( result == XBDM_NOERR ) )
   {
      sprintf_s( pOutFileName, cOutSize, "%s\\%s_%s", mpFailReportDir, pXboxName, pDumpName );
   }
   else if( !dirSuccess && ( result == XBDM_NOERR ) )
   {
      sprintf_s( pOutFileName, cOutSize, ".\\%s_%s", pXboxName, pDumpName );
   }
   else
   {         
      sprintf_s( pOutFileName, cOutSize, ".\\unknown_%s", pDumpName );
   }
#endif // XBOX
}

//============================================================================
// BXFS::setFPSLogFileName
//
// Set path and file name for FPS / Mem log CSV files
//============================================================================
void BXFS::setFPSLogFileName( const char* cpInFileName, char* pOutFileName, const long cOutSize )
{
#if defined(XBOX) && !defined(BUILD_FINAL)
   //const long cXboxNameSize = 256;
   //char       pXboxName[cXboxNameSize];

   // Get name from xbox
   //HRESULT result = DmGetXboxName( pXboxName, (LPDWORD)&cXboxNameSize );

   // Get time from system
   SYSTEMTIME sysTime;
   GetLocalTime( &sysTime );      

   // Make sure fail report directory exists
   BOOL dirSuccess = false;
   char logsDir[256];

   if( mpFailReportDir[0] )
   {
      sprintf_s(logsDir, "%s\\Perf", mpFailReportDir);
      dirSuccess = gXFS.createDirectory( logsDir, NULL );
   }

   // Set file name with time stamp
   char pDumpName[256];
   sprintf_s( pDumpName, sizeof( pDumpName ), "%s_%d-%d-%d.csv", cpInFileName, sysTime.wHour, sysTime.wMinute, sysTime.wSecond );

   // Pre-pend directory and 360 name to fail report file name
   if( dirSuccess ) // && ( result == XBDM_NOERR ) )
   {
      sprintf_s( pOutFileName, cOutSize, "%s\\%s", logsDir, pDumpName );
   }
   else if( !dirSuccess ) // && ( result == XBDM_NOERR ) )
   {
      sprintf_s( pOutFileName, cOutSize, ".\\%s", pDumpName );
   }
   else
   {         
      sprintf_s( pOutFileName, cOutSize, ".\\unknown_%s", pDumpName );
   }
#endif // XBOX
}

//============================================================================
// BXFS::DumpFailReport
//
// Write fail report file to XFS machine
//============================================================================
void BXFS::DumpFailReport( const char* cpErrorMsg, const char* cpFile, BOOL isException )
{
   // rg [12/6/07] - Fix in case we've been called due to the game being out of memory.
   // Try to detect if the primary heap can grow by 4k.
   void* p = gPrimaryHeap.New(16384);
   if (!p)
      return;
   gPrimaryHeap.Delete(p);
   
   cpErrorMsg;
   cpFile;
   isException;

   // Tell the file manager to flush files so that any files that haven't finished writing will (such as a record game)
   __try
   {
      gFileManager.flushOpenFiles();
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
   
   }

   // Only dump file if XFS is in a valid state
   if( gXFS.isActive() )
   {
      char pFileName[512];

      // Set appropriate file name whether it is an exception or an assertion
      if( isException )
      {
         setFailReportFileName( "PhoenixMeltdown", pFileName, sizeof( pFileName ) );
      }
      else
      {
         setFailReportFileName( "PhoenixFailReport", pFileName, sizeof( pFileName ) );
      }

      HANDLE handle = gXFS.createFileA( pFileName, GENERIC_WRITE, 0, 0, OPEN_ALWAYS, 0, 0 );
      if( INVALID_HANDLE_VALUE != handle )
      {
         gXFS.setFilePointer( handle, 0, 0, FILE_END );

         DWORD bytesWritten;

         gXFS.writeFile( handle, cpErrorMsg, strlen( cpErrorMsg ), &bytesWritten, 0 );
 
         gXFS.closeHandle( handle );
      }
      else
      {
         trace( "Failed to write fail report to dump directory look on 360 drive.\n" );
      }
   }
   else
   {
      trace( "Failed to write fail report to dump directory look on 360 drive.\n" );
   }   
}

//============================================================================
// BXFS::ExceptionCallback
//
// Callback function for exceptions
//============================================================================
void BXFS::ExceptionCallback( const char* cpMsg, BOOL die )
{
   __try
   {
      X_Exception::DumpFailReport(cpMsg, "PhoenixMeltdown.txt");

      DumpFailReport( cpMsg, NULL, true );   
   }
   __except( EXCEPTION_EXECUTE_HANDLER )
   {
   }
}

//============================================================================
// BXFS::setup
//============================================================================
bool BXFS::setup(const char* pServerIP, bool disableNoDelay, BMessageCallbackPtr pMsgCallback, WORD connectionPort, bool xfsCopy)
{
   mConnectionPort = connectionPort;

   if(mActive)
      return true;

   mEnableNoDelay = (disableNoDelay ? FALSE : TRUE);

   if(!setupServerIP(pServerIP, pMsgCallback, xfsCopy))
      return true;

   if(!networkStartup())
      return false;

   mNetStart=true;

   char name[32];
   memset(name, 0, sizeof(name));
   int retval=gethostname(name, sizeof(name));
   if(retval==0)
      blogtrace("name %s", name);

   mActive=true;
         
   const uint cTimesToRetry = 10;
   uint retryIndex;
   for (retryIndex = 0; retryIndex < cTimesToRetry; retryIndex++)
   {
      BXFSFile* file=acquireFile();
      if(!file)
      {
         blogtrace("ERROR - Could not connect to XFS server");
         
         shutdown();
         return false;
      }
      
      const bool success = file->resetServer();
                  
      releaseFile(file, false);
      
      if (!success)
         blogtrace("WARNING - Server reset failed, retry %u of %u", retryIndex + 1, cTimesToRetry);
      else
         break;
   }      

   if (retryIndex == cTimesToRetry)
   {
      blogtrace("ERROR - Could not connect to XFS server after %u retries", cTimesToRetry);
      shutdown();
      return false;
   }
   blogtrace("XFS server successfully reset.");
   
   ILowLevelFileIO::changeDefault(&gXFSLowLevelFileIO);
      
   return true;
}

//============================================================================
// BXFS::shutdown
//============================================================================
void BXFS::shutdown()
{
   // Protect file list
   {
      BScopedCriticalSection crit(mCrit);

      for(long i=0; i<mFileCount; i++)
      {
         if(mFileList[i])
         {
            delete mFileList[i];
            mFileList[i]=NULL;
         }
      }
      mFileCount=0;

      for(long i=0; i<mPoolCount; i++)
      {
         if(mPoolList[i])
         {
            delete mPoolList[i];
            mPoolList[i]=NULL;
         }
      }
      mPoolCount=0;
   }

   if(mNetStart)
   {
      networkCleanup();
      mNetStart=false;
   }

   ILowLevelFileIO::changeDefault(&gWin32LowLevelFileIO);
   
   mActive=false;
}

//============================================================================
// BXFS::setupServerIP
//============================================================================
bool BXFS::setupServerIP(const char* pServerIP, BMessageCallbackPtr pMsgCallback, bool xfsCopy)
{
   if(pServerIP==NULL || pServerIP[0]==NULL)
   {
      #ifdef XBOX
         const char* cSystemIdentifier="xbox";
         FILE* xfsFile = NULL;
         fopen_s(&xfsFile, xfsCopy ? "game:\\xfsCopy.txt" : "game:\\xfs.txt", "rt");

         #ifndef BUILD_FINAL      
         if (!xfsFile)
         {
            if (SUCCEEDED(DmMapDevkitDrive()))
            {
               fopen_s(&xfsFile, xfsCopy ? "e:\\xfsCopy.txt" : "e:\\xfs.txt", "rt");
            }
         }
         #endif         
      #else
         const char* cSystemIdentifier="win32";
         FILE* xfsFile=fopen(xfsCopy ? "xfsCopy.txt" : "xfs.txt", "rt");
      #endif

      if(!xfsFile)
      {
         blogtrace("BXFS::setupServerIP: Unable to open file xfs.txt");
         return false;
      }

      bool serverIP=false;
      char token[128];
      for(;;)
      {
         int retval=fscanf_s(xfsFile, "%s", token, sizeof(token));
         if(retval!=1)
            break;
         if(stricmp(token, cSystemIdentifier)==0)
         {
            retval=fscanf_s(xfsFile, "%s", token, sizeof(token));
            if(retval==1)
               serverIP=true;
            break;
         }
      }
      fclose(xfsFile);
      if(!serverIP)
      {
         blogtrace("BXFS::setupServerIP: Found file xfs.txt, but couldn't find server's IP");
         return false;
      }

      int len=strlen(token);
      if(len>15)
      {
         blogtrace("BXFS::setupServerIP: Found file xfs.txt, but couldn't find server's IP");
         return false;
      }
      memcpy(mServerIP, token, len);
      mServerIP[len]=NULL;
   }
   else
   {
      int len=strlen(pServerIP);
      if(len>15)
      {
         blogtrace("BXFS::setupServerIP: IP too long");
         return false;
      }
      memcpy(mServerIP, pServerIP, len);
      mServerIP[len]=NULL;
   }
   
   blogtrace("BXFS::setupServerIP: Using server IP %s", mServerIP);
   
   if (pMsgCallback)
   {
      char buf[256];
      sprintf_s(buf, sizeof(buf), "Using server IP %s", mServerIP);
      (*pMsgCallback)(buf);
   }
   
   return true;
}

//============================================================================
// BXFS::getFile
//============================================================================
BXFSFile* BXFS::getFile(HANDLE hFile)
{
   BScopedCriticalSection crit(mCrit);

   for(long i=0; i<mFileCount; i++)
   {
      BXFSFile* file=mFileList[i];
      if(!file)
         continue;
      if(file->getHandle()==hFile)
         return file;
   }
   return NULL;
}

//============================================================================
// BXFS::acquireFile
//============================================================================
BXFSFile* BXFS::acquireFile()
{
   // Protect file list
   BScopedCriticalSection crit(mCrit);

   BXFSFile* file=NULL;
   if(mPoolCount==0)
   {
      file=new BXFSFile();
      if(!file)
         return NULL;
      if(!file->setup(mEnableNoDelay))
      {
         delete file;
         return NULL;
      }
   }
   else
   {
      long poolIndex=mPoolCount-1;
      file=mPoolList[poolIndex];
      mPoolCount=poolIndex;
   }

   long fileIndex=-1;
   for(long i=0; i<mFileCount; i++)
   {
      if(mFileList[i]==NULL)
      {
         fileIndex=i;
         break;
      }
   }

   if(fileIndex==-1)
   {
      if(mFileCount<cMaxFiles)
      {
         fileIndex=mFileCount;
         mFileCount++;
      }
      else
      {
         //BASSERT(0);
         mPoolList[mPoolCount]=file;
         mPoolCount++;
         return NULL;
      }
   }

   mFileList[fileIndex]=file;

   file->reset();

   return file;
}

//============================================================================
// BXFS::releaseFile
//============================================================================
void BXFS::releaseFile(BXFSFile* file, bool reuse)
{
   if(!file)
      return;

   // Protect file list
   BScopedCriticalSection crit(mCrit);
   
   long i;
   for(i=0; i<mFileCount; i++)
   {
      if(mFileList[i]==file)
         break;
   }
   
   if (i == mFileCount)
   {
      //BASSERT(0);
      return;  
   }
   
   if((mPoolCount<cMaxFiles) &&  (reuse))
   {
      mPoolList[mPoolCount]=file;
      mPoolCount++;
   }
   else
   {
      //BASSERT(0);
      delete file;
   }
   mFileList[i]=NULL;
   
   long end=-1;
   for(long j=mFileCount-1; j>=0; j--)
   {
      if(mFileList[j])
      {
         end=j;
         break;
      }
   }
   mFileCount=end+1;
}

//============================================================================
// BXFS::createFile
//============================================================================
HANDLE BXFS::createFile(const TCHAR* lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
   if(!mActive)
      return CreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

   // Validate parameters
   if(lpFileName==NULL || lpSecurityAttributes!=NULL || hTemplateFile!=NULL)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_PARAMETER);
      return INVALID_HANDLE_VALUE;
   }
      
   // Retry in case a tool like ddxconv or gr2ugx is currently writing this file (XFS should do this, not the game).
   HANDLE handle = INVALID_HANDLE_VALUE;
   
   const uint cNumRetries = 1;
   for (uint retryIndex = 0; retryIndex < cNumRetries; retryIndex++)
   {
      // Acquire a file connection
      BXFSFile* file=acquireFile();
      if(!file)
      {
         //BASSERT(0);
         SetLastError(ERROR_OPEN_FAILED);
         return INVALID_HANDLE_VALUE;
      }
      
      handle=file->createFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
      if(handle==INVALID_HANDLE_VALUE)
         releaseFile(file);
      else
         break;
      
      //Sleep(10);   
   }
   
   if (handle==INVALID_HANDLE_VALUE)
      SetLastError(ERROR_OPEN_FAILED);

   return handle;
}

//============================================================================
// BXFS::createFileA
//============================================================================
HANDLE BXFS::createFileA(const char* lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
   if(!mActive)
      return CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

#ifdef _UNICODE
   TCHAR convName[_MAX_PATH];
   for(long i=0; i<_MAX_PATH; i++)
   {
      char c=lpFileName[i];
      convName[i]=(TCHAR)c;
      if(c==0)
         break;
   }
   return createFile(convName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
#else
   return createFile((const TCHAR*)lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
#endif
}

//============================================================================
// BXFS::createFileW
//============================================================================
HANDLE BXFS::createFileW(const WCHAR* lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
   if(!mActive)
   {
#ifdef XBOX
      lpFileName; dwDesiredAccess; dwShareMode; lpSecurityAttributes; dwCreationDisposition; dwFlagsAndAttributes; hTemplateFile;
      return INVALID_HANDLE_VALUE;
#else
      return CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
#endif
   }

#ifdef _UNICODE
   return createFile((const TCHAR*)lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
#else
   TCHAR convName[_MAX_PATH];
   for(long i=0; i<_MAX_PATH; i++)
   {
      WCHAR c=lpFileName[i];
      convName[i]=(TCHAR)c;
      if(c==0)
         break;
   }
   return createFile(convName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
#endif
}

//============================================================================
// BXFS::closeHandle
//============================================================================
BOOL BXFS::closeHandle(HANDLE hObject)
{
   if(!mActive)
      return CloseHandle(hObject);

   // Validate parameters
   if(hObject==INVALID_HANDLE_VALUE)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   // Make sure this is an actual file since CloseHandle can be used to close other types of objects.
   BXFSFile* file=getFile(hObject);
   if(file==NULL)
      return CloseHandle(hObject);

   BOOL retval=file->closeHandle(hObject);

   releaseFile(file);
   
   if (!retval)
      SetLastError(ERROR_INVALID_HANDLE);

   return retval;
}

//============================================================================
// BXFS::readFile
//============================================================================
BOOL BXFS::readFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
   if(!mActive)
      return ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);

   // Validate parameters
   if(hFile==INVALID_HANDLE_VALUE)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
   }
   
   if (lpBuffer==NULL)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   // Get the file data
   BXFSFile* file=getFile(hFile);
   if(!file)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   BOOL success = file->readFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
   if (!success)
   {
      SetLastError(ERROR_READ_FAULT);
      return FALSE;
   }
   
   return TRUE;
}

//============================================================================
// BXFS::writeFile
//============================================================================
BOOL BXFS::writeFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
   if(!mActive)
      return WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);

   // Validate parameters
   if(hFile==INVALID_HANDLE_VALUE)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
   }
   
   if (lpBuffer==NULL)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   // Return if no bytes to write
   if(nNumberOfBytesToWrite==0)
   {
      *lpNumberOfBytesWritten = 0;
      return TRUE;
   }

   // Get the file data
   BXFSFile* file=getFile(hFile);
   if(!file)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   BOOL success = file->writeFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
   if (!success)
   {
      SetLastError(ERROR_WRITE_FAULT);
      return FALSE;
   }
   
   return TRUE;
}

//============================================================================
// BXFS::getFileSize
//============================================================================
DWORD BXFS::getFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh)
{
   if (!mActive)
      return GetFileSize(hFile, lpFileSizeHigh);

   BY_HANDLE_FILE_INFORMATION fileInfo;
   if (!getFileInformationByHandle(hFile, &fileInfo))
      return (DWORD)-1;

   SetLastError(NO_ERROR);
   
   if (lpFileSizeHigh)
      *lpFileSizeHigh = fileInfo.nFileSizeHigh;
   
   // rg [4/2/07] - FIXME - This is not properly updated as the file is written, I think.   
   return fileInfo.nFileSizeLow;
}

//============================================================================
// BXFS::getFileSizeEx
//============================================================================
BOOL BXFS::getFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize)
{
   if (!mActive)
      return GetFileSizeEx(hFile, lpFileSize);
   
   if (!lpFileSize)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }
      
   BY_HANDLE_FILE_INFORMATION fileInfo;
   if (!getFileInformationByHandle(hFile, &fileInfo))
      return FALSE;
   
   lpFileSize->LowPart = fileInfo.nFileSizeLow;
   lpFileSize->HighPart = fileInfo.nFileSizeHigh;
   
   return TRUE;
}

//============================================================================
// BXFS::getFileInformationByHandle
//============================================================================
BOOL BXFS::getFileInformationByHandle(HANDLE hFile, LPBY_HANDLE_FILE_INFORMATION lpFileInformation)
{
   if(!mActive)
      return GetFileInformationByHandle(hFile, lpFileInformation);

   // Validate parameters
   if(hFile==INVALID_HANDLE_VALUE)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
   }
   
   if (!lpFileInformation)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   // Get the file data
   BXFSFile* file=getFile(hFile);
   if(!file)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   BOOL success = file->getFileInformationByHandle(hFile, lpFileInformation);
   if (!success)
   {
      SetLastError(ERROR_INVALID_FUNCTION);
      return FALSE;
   }
   
   return TRUE;
}

//============================================================================
// BXFS::setFilePointer
//============================================================================
DWORD BXFS::setFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
   if(!mActive)
      return SetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);

   // Validate parameters
   if(hFile==INVALID_HANDLE_VALUE)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_HANDLE);
      return INVALID_SET_FILE_POINTER;
   }
   
   if (lpDistanceToMoveHigh && *lpDistanceToMoveHigh!=0)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_PARAMETER);
      return INVALID_SET_FILE_POINTER;
   }

   // Get the file data
   BXFSFile* file=getFile(hFile);
   if(!file)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_HANDLE);
      return INVALID_SET_FILE_POINTER;
   }

   return file->setFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
}

//============================================================================
// BXFS::createDirectory
//============================================================================
BOOL BXFS::createDirectory(LPCSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributesLPCTSTR)
{
   if(!mActive)
      return CreateDirectory(lpPathName, lpSecurityAttributesLPCTSTR);;

   // Validate parameters
   if(!lpPathName || lpPathName[0]==NULL)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   // Get the file data
   BXFSFile* file=acquireFile();
   if(!file)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_FUNCTION);
      return FALSE;
   }

   BOOL retval=file->createDirectory(lpPathName, lpSecurityAttributesLPCTSTR);

   releaseFile(file);
   
   if (!retval)
      SetLastError(ERROR_CANNOT_MAKE);
   
   return retval;
}

//============================================================================
// BXFS::getFileAttributes
//============================================================================
DWORD BXFS::getFileAttributes(LPCTSTR lpFileName)
{
   if(!mActive)
      return GetFileAttributes(lpFileName);

   // Validate parameters
   if(!lpFileName || lpFileName[0]==NULL)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_PARAMETER);
      return (DWORD)-1;
   }

   // Get the file data
   BXFSFile* file=acquireFile();
   if(!file)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_FUNCTION);
      return (DWORD)-1;
   }

   DWORD retval=file->getFileAttributes(lpFileName);

   releaseFile(file);
   
   if (!retval)
      SetLastError(ERROR_INVALID_FUNCTION);

   return retval;
}

//============================================================================
// BXFS::getFileTime
//============================================================================
BOOL BXFS::getFileTime(HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime)
{
   if(!mActive)
      return GetFileTime(hFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime);

   // Validate parameters
   if(hFile==INVALID_HANDLE_VALUE)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   // Get the file data
   BXFSFile* file=getFile(hFile);
   if(!file)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   BOOL success = file->getFileTime(hFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime);
   if (!success)
   {
      SetLastError(ERROR_INVALID_FUNCTION);
      return FALSE;
   }
   
   return TRUE;
}

//============================================================================
// BXFS::findFirstFile
//============================================================================
HANDLE BXFS::findFirstFile(LPCSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData)
{
   if(!mActive)
      return FindFirstFile(lpFileName, lpFindFileData);

   // Validate parameters
   if(!lpFileName || lpFileName[0]==NULL || lpFindFileData==NULL)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_PARAMETER);
      return INVALID_HANDLE_VALUE;
   }

   // Get the file data
   BXFSFile* file=acquireFile();
   if(!file)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_FUNCTION);
      return INVALID_HANDLE_VALUE;
   }

   HANDLE handle=file->findFirstFile(lpFileName, lpFindFileData);

   if(handle==INVALID_HANDLE_VALUE)
   {
      releaseFile(file);
      
      SetLastError(ERROR_FILE_NOT_FOUND);
   }

   return handle;
}

//============================================================================
// BXFS::findNextFile
//============================================================================
BOOL BXFS::findNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData)
{
   if(!mActive)
      return FindNextFile(hFindFile, lpFindFileData);

   // Validate parameters
   if(hFindFile==INVALID_HANDLE_VALUE)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
   }
   
   if (lpFindFileData==NULL)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   // Get the file data
   BXFSFile* file=getFile(hFindFile);
   if(!file)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   BOOL success = file->findNextFile(hFindFile, lpFindFileData);
   
   if (!success)
   {
      SetLastError(ERROR_INVALID_FUNCTION);
      return FALSE;
   }
   
   return TRUE;
}

//============================================================================
// BXFS::findClose
//============================================================================
BOOL BXFS::findClose(HANDLE hFindFile)
{
   if(!mActive)
      return FindClose(hFindFile);

   // Validate parameters
   if(hFindFile==INVALID_HANDLE_VALUE)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   // Get the file data
   BXFSFile* file=getFile(hFindFile);
   if(!file)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   BOOL retval=file->findClose(hFindFile);

   releaseFile(file);
   
   if (!retval)
   {
      SetLastError(ERROR_INVALID_FUNCTION);
      return FALSE;
   }

   return TRUE;
}

//============================================================================
// BXFS::flushFileBuffers
//============================================================================
BOOL BXFS::flushFileBuffers(HANDLE hFile)
{
   if(!mActive)
      return FlushFileBuffers(hFile);

   // Validate parameters
   if(hFile==INVALID_HANDLE_VALUE)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   // Get the file data
   BXFSFile* file=getFile(hFile);
   if(!file)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   BOOL success = file->flushFileBuffers(hFile);
   if (!success)
   {
      SetLastError(ERROR_INVALID_FUNCTION);
      return FALSE;
   }
   
   return TRUE;
}

//============================================================================
// BXFS::hasOverlappedIoCompleted
//============================================================================
BOOL BXFS::hasOverlappedIoCompleted(LPOVERLAPPED lpOverlapped) const
{
   if (!mActive)
      return HasOverlappedIoCompleted(lpOverlapped);
   
   if (!lpOverlapped)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }
   
   return TRUE;
}

//============================================================================
// BXFS::setFileAttributes
//============================================================================
BOOL BXFS::setFileAttributes(LPCSTR lpFileName, DWORD dwFileAttributes)
{
   if (!mActive)
      return SetFileAttributes(lpFileName, dwFileAttributes);
   
   trace("BXFS::setFileAttributes: Unsupported");
   
   SetLastError(ERROR_INVALID_FUNCTION);
   return FALSE;
}

//============================================================================
// BXFS::setEndOfFile
//============================================================================
BOOL BXFS::setEndOfFile(HANDLE hFile)
{
   if (!mActive)
      return SetEndOfFile(hFile);
   
   trace("XFS::setEndOfFile: Unsupported!");
   
   // XFS doesn't support this operation yet
   SetLastError(ERROR_INVALID_FUNCTION);
   return FALSE;
}

//============================================================================
// BXFS::getFileAttributesEx
//============================================================================
BOOL BXFS::getFileAttributesEx(LPCSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation)
{
   if (!mActive)
      return GetFileAttributesEx(lpFileName, fInfoLevelId, lpFileInformation);
      
   // Validate parameters
   if(!lpFileName || lpFileName[0]==NULL || (fInfoLevelId != GetFileExInfoStandard) || (!lpFileInformation))
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   // Get the file data
   BXFSFile* file=acquireFile();
   if(!file)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_FUNCTION);
      return FALSE;
   }

   DWORD retval=file->getFileAttributesEx(lpFileName, fInfoLevelId, lpFileInformation);

   releaseFile(file);

   if (!retval)
      SetLastError(ERROR_FILE_NOT_FOUND);

   return retval;
}

//============================================================================
// BXFS::setFileTime
//============================================================================
BOOL BXFS::setFileTime(HANDLE hFile, CONST FILETIME *lpCreationTime, CONST FILETIME *lpLastAccessTime, CONST FILETIME *lpLastWriteTime)
{
   if (!mActive)
      return SetFileTime(hFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime);
   
   trace("XFS::setFileTime: Unsupported!");

   // XFS doesn't support this operation yet
   SetLastError(ERROR_INVALID_FUNCTION);
   return FALSE;
}

//============================================================================
// BXFS::getFullPathName
//============================================================================
DWORD BXFS::getFullPathName(LPCTSTR lpFileName, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR* lpFilePart)
{
   if (!mActive)
   {
#ifdef XBOX      
      SetLastError(ERROR_INVALID_FUNCTION);
      return 0;
#else
      return GetFullPathName(lpFileName, nBufferLength, lpBuffer, lpFilePart);
#endif      
   }
   
   trace("XFS::getFullPathName: Unsupported!");

   // XFS doesn't support this operation yet
   SetLastError(ERROR_INVALID_FUNCTION);
   return 0;
}

//============================================================================
// BXFS::getOverlappedResult
//============================================================================
BOOL BXFS::getOverlappedResult(HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait)
{
   if(!mActive)
      return GetOverlappedResult(hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait);

   // Validate parameters
   if(hFile==INVALID_HANDLE_VALUE)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
   }
   
   if (!lpOverlapped || !lpNumberOfBytesTransferred)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   *lpNumberOfBytesTransferred=lpOverlapped->InternalHigh;

   return TRUE;
}

//============================================================================
// BXFS::setOwnerThread
//============================================================================
void BXFS::setOwnerThread(HANDLE hFile, DWORD threadID)
{
   if(!mActive)
      return;

   // Get the file data
   BXFSFile* file=getFile(hFile);
   if(!file)
   {
      //BASSERT(0);
   }
   else
   {
      file->setOwnerThread(threadID);
   }
}

//============================================================================
// BXFS::setFailReportDirectory
//
// Set directory fail report will be written to
//============================================================================
BOOL BXFS::setFailReportDirectory( const char* cpDir )
{
   if( cpDir && ( strlen( cpDir ) < sizeof( mpFailReportDir ) ) )
   {
      memset( mpFailReportDir, NULL, sizeof( mpFailReportDir ) );
      memcpy( mpFailReportDir, cpDir, strlen( cpDir ) );

      return( TRUE );
   }

   return( FALSE );
}

//============================================================================
// BXFSFile::BXFSFile
//============================================================================
BXFSFile::BXFSFile() :
   mSocket(INVALID_SOCKET),
   mBuffer(NULL),
   mBufferSize(0),
   mFileOffset(0),
   mCacheSize(0),
   mCacheStart(0),
   mCache(NULL),
   mFileInfo(NULL),
   mFindFileList(NULL),
   mFindFileCount(0),
   mFindFileIndex(0),
   mOwnerThread(0),
   mCreateFlags(0)
{
#ifdef BUILD_DEBUG
   InterlockedExchange(&mReadingThreadsCount, 0);
#endif   
   setHandle(NULL);
}

//============================================================================
// BXFSFile::~BXFSFile
//============================================================================
BXFSFile::~BXFSFile()
{
   reset();

   if(mSocket!=INVALID_SOCKET)
   {
      closesocket(mSocket);
      mSocket=INVALID_SOCKET;
   }

   if(mBuffer)
   {
      delete[] mBuffer;
      mBuffer=NULL;
   }

   if(mFileInfo)
   {
      delete mFileInfo;
      mFileInfo=NULL;
   }

   if(mCache)
   {
      delete[] mCache;
      mCache=NULL;
   }
}

//============================================================================
// BXFSFile::setup
//============================================================================
bool BXFSFile::setup(BOOL enableNoDelay)
{
   mCache=new BYTE[cCacheSize];
   if(!mCache)
      return false;

   mFileInfo=new BY_HANDLE_FILE_INFORMATION;
   if(!mFileInfo)
      return false;

   memset(mFileInfo, 0, sizeof(BY_HANDLE_FILE_INFORMATION));

   mBuffer=new BYTE[cBufferSize];
   if(!mBuffer)
   {
      //BASSERT(0);
      return false;
   }

   SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (s == INVALID_SOCKET)
   {
      //BASSERT(0);
      return false;
   }

   BOOL optVal=enableNoDelay;
   setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (const char*)(&optVal), sizeof(optVal));

   SOCKADDR_IN sa;
   memset(&sa, 0, sizeof(sa));
   sa.sin_family = AF_INET;
   sa.sin_port = htons(gXFS.getConnectionPort());
   sa.sin_addr.s_addr = inet_addr(gXFS.getServerIP());

   if (connect(s, (struct sockaddr *)&sa, sizeof sa) == SOCKET_ERROR)
   {
      int error=WSAGetLastError();
      trace("connect error: %d", error);
      closesocket(s);
      return false;
   }

   mSocket=s;

   return true;
}

//============================================================================
// BXFSFile::reset
//============================================================================
void BXFSFile::reset()
{
   mOwnerThread = GetCurrentThreadId();
      
   setHandle(INVALID_HANDLE_VALUE);

   mFileOffset=0;
   mCacheSize=0;
   mCacheStart=0;
   memset(mFileInfo, 0, sizeof(BY_HANDLE_FILE_INFORMATION));
   mBufferSize=0;
   mCreateFlags=0;
#ifdef BUILD_DEBUG   
   InterlockedExchange(&mReadingThreadsCount, 0);
#endif   
   clearFindFileList();
}

//============================================================================
// BXFSFile::clearFindFileList
//============================================================================
void BXFSFile::clearFindFileList()
{
   if(mFindFileList)
   {
      delete[] mFindFileList;
      mFindFileList=NULL;
   }
   mFindFileCount=0;
   mFindFileIndex=0;
}

//============================================================================
// BXFSFile::waitForReply
//============================================================================
bool BXFSFile::waitForReply(int desiredFunc)
{
   BYTE prefix, func;
   
   if(!recvBYTE(prefix) || !recvBYTE(func) || prefix!=cCommandPrefix || func!=desiredFunc)
   {
      return false;
   }

   return true;
}

//============================================================================
// BXFSFile::createFile
//============================================================================
HANDLE BXFSFile::createFile(const TCHAR* lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
   BDEBUG_ASSERT(mOwnerThread == GetCurrentThreadId());
   
   hTemplateFile;
   lpSecurityAttributes;
   
   mCreateFlags = dwFlagsAndAttributes;
   
   dwFlagsAndAttributes &= ~FILE_FLAG_OVERLAPPED;
   dwFlagsAndAttributes &= ~FILE_FLAG_NO_BUFFERING;

   // Send the request
   sendBegin(cFuncCreateFile);
   sendAddString(lpFileName);
   sendAddDWORD(dwDesiredAccess);
   sendAddDWORD(dwShareMode);
   sendAddDWORD(dwCreationDisposition);
   sendAddDWORD(dwFlagsAndAttributes);
   sendEnd();

   // Read the response
   if (!waitForReply(cFuncCreateFile))
   {
      //BASSERT(0);
      return INVALID_HANDLE_VALUE;
   }
   
   HANDLE handle;
   if(!recvHandle(handle))
   {
      //BASSERT(0);
      return INVALID_HANDLE_VALUE;
   }

   LARGE_INTEGER fileSize;
   if(!recvLARGE_INTEGER(fileSize))
   {
      //BASSERT(0);
      return INVALID_HANDLE_VALUE;
   }

   LARGE_INTEGER fileTime;
   if(!recvLARGE_INTEGER(fileTime))
   {
      //BASSERT(0);
      return INVALID_HANDLE_VALUE;
   }

   BYTE data;
   if(!recvBYTE(data))
   {
      //BASSERT(0);
      return INVALID_HANDLE_VALUE;
   }
   if(data==1)
   {
      if(!recvData(mCache, 0, cOpenInitialRead))
      {
         //BASSERT(0);
         return INVALID_HANDLE_VALUE;
      }
   }

   BYTE suffix;
   if(!recvBYTE(suffix) || suffix!=cCommandSuffix)
   {
      //BASSERT(0);
      return INVALID_HANDLE_VALUE;
   }

   if(handle==INVALID_HANDLE_VALUE)
      return INVALID_HANDLE_VALUE;

   // Save off file data
   
   mFileInfo->nFileSizeHigh=fileSize.HighPart;
   mFileInfo->nFileSizeLow=fileSize.LowPart;
   mFileInfo->ftLastWriteTime.dwHighDateTime=fileTime.HighPart;
   mFileInfo->ftLastWriteTime.dwLowDateTime=fileTime.LowPart;
   mFileInfo->ftCreationTime=mFileInfo->ftLastWriteTime;
   mFileInfo->ftLastAccessTime=mFileInfo->ftLastWriteTime;
   if(data)
      mCacheSize=cOpenInitialRead;

   setHandle(handle);

   return handle;
}

//============================================================================
// BXFSFile::closeHandle
//============================================================================
BOOL BXFSFile::closeHandle(HANDLE hObject)
{  
   BDEBUG_ASSERT(mOwnerThread == GetCurrentThreadId());

   setHandle(INVALID_HANDLE_VALUE);
      
   // Send the request
   sendBegin(cFuncCloseHandle);
   sendAddHandle(hObject);
   sendEnd();

   // Read the response
   if (!waitForReply(cFuncCloseHandle))
   {
      //BASSERT(0);
      return FALSE;
   }
   
   BYTE retval;
   if(!recvBYTE(retval))
   {
      //BASSERT(0);
      return FALSE;
   }

   BYTE suffix;
   if(!recvBYTE(suffix) || suffix!=cCommandSuffix)
   {
      //BASSERT(0);
      return FALSE;
   }
   
   return retval;
}

//============================================================================
// BXFSFile::readFile
//============================================================================
BOOL BXFSFile::readFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
   BDEBUG_ASSERT(mOwnerThread == GetCurrentThreadId());
   
   if ((mCreateFlags & FILE_FLAG_OVERLAPPED) && (!lpOverlapped))
      return FALSE;
   
   if ((lpOverlapped) && (lpOverlapped->OffsetHigh))      
      return FALSE;
   
   if (lpNumberOfBytesRead)
      *lpNumberOfBytesRead = 0;
   if (lpOverlapped)
   {
      lpOverlapped->Internal = 0;
      lpOverlapped->InternalHigh = 0;
   }      
      
   DWORD overlapOfs = lpOverlapped ? lpOverlapped->Offset : 0;
   DWORD& fileOfs = lpOverlapped ? overlapOfs : mFileOffset;
      
#ifdef BUILD_DEBUG   
   LONG prevVal = InterlockedIncrement(&mReadingThreadsCount);
   prevVal;
   // rg [10/05/06] - DO NOT try to work around this assert. If it fires, it means you're trying to read from the file from multiple threads! 
   // This is not supported.
   BDEBUG_ASSERT(1 == prevVal);
#endif   
         
   // Setup some read variables
   BYTE* readPtr=reinterpret_cast<BYTE*>(lpBuffer);
   DWORD readBytesLeft=nNumberOfBytesToRead;
   DWORD readBytesTotal=0;

   // See if the data is available in the cache
   DWORD cacheBytes=0;
   DWORD cacheOffset=fileOfs-mCacheStart;
   if(cacheOffset < mCacheSize)
   {
      DWORD cacheRemaining=mCacheSize-cacheOffset;
      if(nNumberOfBytesToRead<=cacheRemaining)
         cacheBytes=nNumberOfBytesToRead;
      else
         cacheBytes=cacheRemaining;

      memcpy(readPtr, mCache+cacheOffset, cacheBytes);
      readPtr+=cacheBytes;

      readBytesTotal+=cacheBytes;
      readBytesLeft-=cacheBytes;
      if(readBytesLeft==0)
      {
         if(lpNumberOfBytesRead)
            *lpNumberOfBytesRead=readBytesTotal;
            
         if(lpOverlapped)
         {
            lpOverlapped->Internal=0;
            lpOverlapped->InternalHigh=readBytesTotal;
         }
         
         fileOfs+=readBytesTotal;
#ifdef BUILD_DEBUG            
         InterlockedDecrement(&mReadingThreadsCount);
#endif         
         return TRUE;
      }
   }

   // Reset the data cache
   mCacheSize=0;
   mCacheStart=0;

   // Request a minimum of the cache size
   DWORD requestBytes;
   if(readBytesLeft<cCacheSize-mCacheSize)
      requestBytes=cCacheSize-mCacheSize;
   else
      requestBytes=readBytesLeft;

   // Send the request
   sendBegin(cFuncReadFile);
   sendAddHandle(hFile);
   sendAddDWORD(fileOfs+readBytesTotal);
   sendAddDWORD(requestBytes);
   sendEnd();

   // Process the response
   for(;;)
   {
      if (!waitForReply(cFuncReadFile))
      {
         //BASSERT(0);
#ifdef BUILD_DEBUG            
         InterlockedDecrement(&mReadingThreadsCount);
#endif         
         return FALSE;
      }
      
      BYTE moreData;
      if(!recvBYTE(moreData))
      {
         //BASSERT(0);
#ifdef BUILD_DEBUG            
         InterlockedDecrement(&mReadingThreadsCount);
#endif         
         return FALSE;
      }

      DWORD dataSize;
      if(!recvDWORD(dataSize))
      {
         //BASSERT(0);
#ifdef BUILD_DEBUG            
         InterlockedDecrement(&mReadingThreadsCount);
#endif         
         return FALSE;
      }

      DWORD dataLeft=dataSize;

      for(;;)
      {
         if(readBytesLeft>0)
         {
            // Receive data into user supplied buffer
            DWORD recvBytes;
            if(dataLeft>readBytesLeft)
               recvBytes=readBytesLeft;
            else
               recvBytes=dataLeft;

            if(!recvData(readPtr, 0, (long)recvBytes))
            {
               //BASSERT(0);
#ifdef BUILD_DEBUG                  
               InterlockedDecrement(&mReadingThreadsCount);
#endif               
               return FALSE;
            }

            dataLeft-=recvBytes;
            readPtr+=recvBytes;
            readBytesTotal+=recvBytes;
            readBytesLeft-=recvBytes;
         }
         else if(mCacheSize<cCacheSize && dataLeft>0)
         {
            // Receive data into cache buffer
            if(mCacheSize==0)
               mCacheStart=fileOfs+readBytesTotal;

            DWORD recvBytes;
            if(dataLeft>cCacheSize-mCacheSize)
               recvBytes=cCacheSize-mCacheSize;
            else
               recvBytes=dataLeft;

            if(!recvData(mCache, mCacheSize, (long)recvBytes))
            {
               //BASSERT(0);
#ifdef BUILD_DEBUG                  
               InterlockedDecrement(&mReadingThreadsCount);
#endif               
               return FALSE;
            }

            mCacheSize+=recvBytes;
            dataLeft-=recvBytes;
         }
         else if(dataLeft>0)
         {
            // Should never get here
            //BASSERT(0);
#ifdef BUILD_DEBUG               
            InterlockedDecrement(&mReadingThreadsCount);
#endif            
            return FALSE;
         }

         if(dataLeft==0)
            break;
      }

      BYTE suffix;
      if(!recvBYTE(suffix) || suffix!=cCommandSuffix)
      {
         //BASSERT(0);
#ifdef BUILD_DEBUG            
         InterlockedDecrement(&mReadingThreadsCount);
#endif         
         return FALSE;
      }

      if(!moreData)
         break;
   }

   if(lpNumberOfBytesRead)
      *lpNumberOfBytesRead=readBytesTotal;
      
   if(lpOverlapped)
   {
      lpOverlapped->Internal=0;
      lpOverlapped->InternalHigh=readBytesTotal;
   }

   fileOfs += readBytesTotal;
   
#ifdef BUILD_DEBUG   
   InterlockedDecrement(&mReadingThreadsCount);
#endif   

   return TRUE;
}

//============================================================================
// BXFSFile::writeFile
//============================================================================
BOOL BXFSFile::writeFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
   BDEBUG_ASSERT(mOwnerThread == GetCurrentThreadId());
   
   if ((mCreateFlags & FILE_FLAG_OVERLAPPED) && (!lpOverlapped))
      return FALSE;

   if ((lpOverlapped) && (lpOverlapped->OffsetHigh))      
      return FALSE;
   
   if (lpNumberOfBytesWritten)
      *lpNumberOfBytesWritten = 0;
      
   if (lpOverlapped)
   {
      lpOverlapped->Internal = 0;
      lpOverlapped->InternalHigh = 0;
   }      
   
   DWORD overlapOfs = lpOverlapped ? lpOverlapped->Offset : 0;
   DWORD& fileOfs = lpOverlapped ? overlapOfs : mFileOffset;

   //FIXME - Should support write caching
   if(mCacheSize!=0)
   {
      mCacheSize=0;
      mCacheStart=0;
   }

   DWORD totalBytesWritten=0;
   DWORD bytesLeft=nNumberOfBytesToWrite;
//-- FIXING PREFIX BUG ID 373
   const BYTE* bufferPtr=(BYTE*)lpBuffer;
//--

   for(;;)
   {
      // Send request
      sendBegin(cFuncWriteFile);
      sendAddHandle(hFile);
      sendAddDWORD(fileOfs);

      DWORD maxBytes=cBufferSize-mBufferSize-5;
      DWORD writeBytes;
      if(bytesLeft<=maxBytes)
         writeBytes=bytesLeft;
      else
         writeBytes=maxBytes;
      sendAddDWORD(writeBytes);
      memcpy(mBuffer+mBufferSize, bufferPtr, writeBytes);
      mBufferSize+=writeBytes;
      bufferPtr+=writeBytes;
      bytesLeft-=writeBytes;

      sendEnd();

      // Process response 
      if (!waitForReply(cFuncWriteFile))
      {
         //BASSERT(0);
         return FALSE;
      }
      
      DWORD bytesWritten;
      if(!recvDWORD(bytesWritten))
      {
         //BASSERT(0);
         return FALSE;
      }

      BYTE suffix;
      if(!recvBYTE(suffix) || suffix!=cCommandSuffix)
      {
         //BASSERT(0);
         return FALSE;
      }

      totalBytesWritten+=bytesWritten;
      fileOfs+=bytesWritten;

      if(bytesLeft==0)
         break;
   }

   if(lpNumberOfBytesWritten)
      *lpNumberOfBytesWritten=totalBytesWritten;
   
   if(lpOverlapped)
   {
      lpOverlapped->Internal=0;
      lpOverlapped->InternalHigh=totalBytesWritten;
   }      

   return TRUE;
}

//============================================================================
// BXFSFile::getFileInformationByHandle
//============================================================================
BOOL BXFSFile::getFileInformationByHandle(HANDLE hFile, LPBY_HANDLE_FILE_INFORMATION lpFileInformation)
{
   BDEBUG_ASSERT(mOwnerThread == GetCurrentThreadId());
   
   hFile;

   // Copy the file info
   memcpy(lpFileInformation, mFileInfo, sizeof(BY_HANDLE_FILE_INFORMATION));
   return TRUE;
}

//============================================================================
// BXFSFile::setFilePointer
//============================================================================
DWORD BXFSFile::setFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
   BDEBUG_ASSERT(mOwnerThread == GetCurrentThreadId());
   
   lpDistanceToMoveHigh;
   SetLastError(S_OK);

   // See if this is just a request for the current file's current position
   if(dwMoveMethod==FILE_CURRENT && lDistanceToMove==0)
      return mFileOffset;

   // Send the request
   sendBegin(cFuncSetFilePointer);
   sendAddHandle(hFile);
   sendAddDWORD(mFileOffset);
   sendAddDWORD((DWORD)lDistanceToMove);
   sendAddDWORD(dwMoveMethod);
   sendEnd();

   // Read the response
   if (!waitForReply(cFuncSetFilePointer))
   {
      //BASSERT(0);
      SetLastError(1);
      return INVALID_SET_FILE_POINTER;
   }
      
   LARGE_INTEGER pos;
   if(!recvLARGE_INTEGER(pos))
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_FUNCTION);
      return INVALID_SET_FILE_POINTER;
   }

   BYTE suffix;
   if(!recvBYTE(suffix) || suffix!=cCommandSuffix)
   {
      //BASSERT(0);
      SetLastError(ERROR_INVALID_FUNCTION);
      return INVALID_SET_FILE_POINTER;
   }

   mFileOffset=pos.LowPart;
   
   return pos.LowPart;
}

//============================================================================
// BXFSFile::createDirectory
//============================================================================
BOOL BXFSFile::createDirectory(LPCSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributesLPCTSTR)
{
   BDEBUG_ASSERT(mOwnerThread == GetCurrentThreadId());
   
   lpSecurityAttributesLPCTSTR;

   // Send the request
   sendBegin(cFuncCreateDirectory);
   sendAddString(lpPathName);
   sendEnd();

   // Read the response
   if (!waitForReply(cFuncCreateDirectory))
   {
      //BASSERT(0);
      return FALSE;
   }
   
   BYTE retval;
   if(!recvBYTE(retval))
   {
      //BASSERT(0);
      return FALSE;
   }

   BYTE suffix;
   if(!recvBYTE(suffix) || suffix!=cCommandSuffix)
   {
      //BASSERT(0);
      return FALSE;
   }

   return retval;
}

//============================================================================
// BXFSFile::getFileAttributes
//============================================================================
DWORD BXFSFile::getFileAttributes(LPCTSTR lpFileName)
{
   BDEBUG_ASSERT(mOwnerThread == GetCurrentThreadId());
   
   // Send the request
   sendBegin(cFuncGetFileAttributes);
   sendAddString(lpFileName);
   sendEnd();

   // Read the response
   if (!waitForReply(cFuncGetFileAttributes))
   {
      //BASSERT(0);
      return FALSE;
   }
      
   DWORD retval;
   if(!recvDWORD(retval))
   {
      //BASSERT(0);
      return FALSE;
   }

   BYTE suffix;
   if(!recvBYTE(suffix) || suffix!=cCommandSuffix)
   {
      //BASSERT(0);
      return FALSE;
   }

   return retval;
}

//============================================================================
// BXFSFile::getFileAttributesEx
//============================================================================
DWORD BXFSFile::getFileAttributesEx(LPCSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation)
{
   BDEBUG_ASSERT(mOwnerThread == GetCurrentThreadId());
   BDEBUG_ASSERT(lpFileInformation);
   
   if (fInfoLevelId != GetFileExInfoStandard)
   {
      return FALSE;
   }
      
   // Send the request
   sendBegin(cFuncGetFileAttributesEx);
   sendAddString(lpFileName);
   sendEnd();

   // Read the response
   if (!waitForReply(cFuncGetFileAttributesEx))
   {
      //BASSERT(0);
      return FALSE;
   }
   
   BYTE success = 0;
   if (!recvBYTE(success))
   {
      //BASSERT(0);
      return FALSE;
   }
      
   DWORD attribs = 0;
   if(!recvDWORD(attribs))
   {
      //BASSERT(0);
      return FALSE;
   }
   
   uint64 createTime = 0, lastAccessTime = 0, lastWriteTime = 0;
   if (!recvUINT64(createTime))
   {
      //BASSERT(0);
      return FALSE;
   }
   if (!recvUINT64(lastAccessTime))
   {
      //BASSERT(0);
      return FALSE;
   }
   if (!recvUINT64(lastWriteTime))
   {
      //BASSERT(0);
      return FALSE;
   }
   
   DWORD fileSizeHigh = 0, fileSizeLow = 0;
   if (!recvDWORD(fileSizeHigh))
   {
      //BASSERT(0);
      return FALSE;
   }
   if (!recvDWORD(fileSizeLow))
   {
      //BASSERT(0);
      return FALSE;
   }
   
   BYTE suffix;
   if(!recvBYTE(suffix) || suffix!=cCommandSuffix)
   {
      //BASSERT(0);
      return FALSE;
   }
   
   WIN32_FILE_ATTRIBUTE_DATA* pData = (WIN32_FILE_ATTRIBUTE_DATA*)lpFileInformation;
   pData->dwFileAttributes = attribs;
   pData->ftCreationTime = Utils::UInt64ToFileTime(createTime);
   pData->ftLastAccessTime = Utils::UInt64ToFileTime(lastAccessTime);
   pData->ftLastWriteTime = Utils::UInt64ToFileTime(lastWriteTime);
   pData->nFileSizeLow = fileSizeLow;
   pData->nFileSizeHigh = fileSizeHigh;

   return success;
}

//============================================================================
// BXFSFile::getFileTime
//============================================================================
BOOL BXFSFile::getFileTime(HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime)
{
   BDEBUG_ASSERT(mOwnerThread == GetCurrentThreadId());
   
   hFile;

   if(!mFileInfo)
      return FALSE;

   if(lpCreationTime)
      *lpCreationTime=mFileInfo->ftCreationTime;

   if(lpLastAccessTime)
      *lpLastAccessTime=mFileInfo->ftLastAccessTime;

   if(lpLastWriteTime)
      *lpLastWriteTime=mFileInfo->ftLastWriteTime;

   return TRUE;
}

//============================================================================
// BXFSFile::findFirstFile
//============================================================================
HANDLE BXFSFile::findFirstFile(LPCSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData)
{
   BDEBUG_ASSERT(mOwnerThread == GetCurrentThreadId());
   
   lpFindFileData;

   // Send the request
   sendBegin(cFuncFindFiles);
   sendAddString(lpFileName);
   sendEnd();

   // Read the response
   if (!waitForReply(cFuncFindFiles))
   {
      //BASSERT(0);
      return INVALID_HANDLE_VALUE;
   }
      
   long count;
   if(!recvLong(count))
   {
      //BASSERT(0);
      return INVALID_HANDLE_VALUE;
   }

   mFindFileList=(count>0 ? new WIN32_FIND_DATA[count] : NULL);
   mFindFileCount=count;

   WIN32_FIND_DATA tempData;

   for(long i=0; i<count; i++)
   {
      long len;
      if(!recvLong(len))
      {
         //BASSERT(0);
         return INVALID_HANDLE_VALUE;
      }
      WIN32_FIND_DATA* data=(mFindFileList ? &(mFindFileList[i]) : &tempData);
      if(!recvData((BYTE*)data->cFileName, 0, len))
      {
         //BASSERT(0);
         return INVALID_HANDLE_VALUE;
      }
      data->cFileName[len]=NULL;
      if(!recvDWORD(data->dwFileAttributes))
      {
         //BASSERT(0);
         return INVALID_HANDLE_VALUE;
      }
      
      uint64 lastWriteTime;
      if(!recvUINT64(lastWriteTime))
      {
         //BASSERT(0);
         return INVALID_HANDLE_VALUE;
      }
                  
      data->ftLastWriteTime = Utils::UInt64ToFileTime(lastWriteTime);
      data->ftCreationTime = data->ftLastWriteTime;
      data->ftLastAccessTime = data->ftLastWriteTime;
      
      uint64 length;
      if(!recvUINT64(length))
      {
         //BASSERT(0);
         return INVALID_HANDLE_VALUE;
      }
      data->nFileSizeLow = (uint)length;
      data->nFileSizeHigh = (uint)(length >> 32U);
   }

   BYTE suffix;
   if(!recvBYTE(suffix) || suffix!=cCommandSuffix)
   {
      //BASSERT(0);
      return INVALID_HANDLE_VALUE;
   }

   if(count==0 || !mFindFileList)
      return INVALID_HANDLE_VALUE;

   HANDLE handle=CreateEvent(NULL, TRUE, FALSE, NULL);
   if(handle==INVALID_HANDLE_VALUE)
      return INVALID_HANDLE_VALUE;

   // Save off handle
   
   mFindFileIndex=0;

   memset(lpFindFileData, 0, sizeof(WIN32_FIND_DATA));
   lpFindFileData->dwFileAttributes=mFindFileList[0].dwFileAttributes;
   //FILETIME ftCreationTime;
   //FILETIME ftLastAccessTime;
   //FILETIME ftLastWriteTime;
   //DWORD nFileSizeHigh;
   //DWORD nFileSizeLow;
   StringCchCopyA(lpFindFileData->cFileName, MAX_PATH-1, mFindFileList[0].cFileName);
   lpFindFileData->ftCreationTime = mFindFileList[0].ftCreationTime;
   lpFindFileData->ftLastWriteTime = mFindFileList[0].ftLastWriteTime;
   lpFindFileData->ftLastAccessTime = mFindFileList[0].ftLastAccessTime;
   lpFindFileData->nFileSizeLow = mFindFileList[0].nFileSizeLow;
   lpFindFileData->nFileSizeHigh = mFindFileList[0].nFileSizeHigh;
   
   setHandle(handle);

   return handle;
}

//============================================================================
// BXFSFile::findNextFile
//============================================================================
BOOL BXFSFile::findNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData)
{
   BDEBUG_ASSERT(mOwnerThread == GetCurrentThreadId());
   
   hFindFile;

   if(!mFindFileList)
      return FALSE;

   long index=mFindFileIndex+1;
   if(index<mFindFileCount)
   {
      memset(lpFindFileData, 0, sizeof(WIN32_FIND_DATA));
      lpFindFileData->dwFileAttributes=mFindFileList[index].dwFileAttributes;
      //FILETIME ftCreationTime;
      //FILETIME ftLastAccessTime;
      //FILETIME ftLastWriteTime;
      //DWORD nFileSizeHigh;
      //DWORD nFileSizeLow;
      StringCchCopyA(lpFindFileData->cFileName, MAX_PATH-1, mFindFileList[index].cFileName);
      lpFindFileData->ftCreationTime = mFindFileList[index].ftCreationTime;
      lpFindFileData->ftLastWriteTime = mFindFileList[index].ftLastWriteTime;
      lpFindFileData->ftLastAccessTime = mFindFileList[index].ftLastAccessTime;
      lpFindFileData->nFileSizeLow = mFindFileList[index].nFileSizeLow;
      lpFindFileData->nFileSizeHigh = mFindFileList[index].nFileSizeHigh;
      
      mFindFileIndex=index;
      return TRUE;
   }

   return FALSE;
}

//============================================================================
// BXFSFile::findClose
//============================================================================
BOOL BXFSFile::findClose(HANDLE hFindFile)
{
   BDEBUG_ASSERT(mOwnerThread == GetCurrentThreadId());  
         
   setHandle(INVALID_HANDLE_VALUE);
   
   CloseHandle(hFindFile);
   
   clearFindFileList();
   
   return TRUE;
}

//============================================================================
// BXFSFile::flushFileBuffers
//============================================================================
BOOL BXFSFile::flushFileBuffers(HANDLE hFile)
{
   BDEBUG_ASSERT(mOwnerThread == GetCurrentThreadId());
   
   // Send the request
   sendBegin(cFuncFlushFileBuffers);
   sendAddHandle(hFile);
   sendEnd();

   // Read the response
   if (!waitForReply(cFuncFlushFileBuffers))
   {
      //BASSERT(0);
      SetLastError(1);
      return FALSE;
   }
      
   BOOL retval;
   if(!recvBOOL(retval))
   {
      //BASSERT(0);
      SetLastError(1);
      return FALSE;
   }

   BYTE suffix;
   if(!recvBYTE(suffix) || suffix!=cCommandSuffix)
   {
      //BASSERT(0);
      SetLastError(1);
      return INVALID_SET_FILE_POINTER;
   }

   return retval;
}

//============================================================================
// BXFSFile::sendBegin
//============================================================================
void BXFSFile::sendBegin(BYTE func)
{
   mBufferSize=0;
   sendAddBYTE(cCommandPrefix);
   sendAddBYTE(func);
}

//============================================================================
// BXFSFile::sendAddBYTE
//============================================================================
void BXFSFile::sendAddBYTE(BYTE val)
{
   //BASSERT(mBufferSize+sizeof(BYTE)<=cBufferSize);
   mBuffer[mBufferSize]=val;
   mBufferSize++;
}

//============================================================================
// BXFSFile::sendAddWORD
//============================================================================
void BXFSFile::sendAddWORD(WORD val)
{
   //BASSERT(mBufferSize+sizeof(WORD)<=cBufferSize);
//-- FIXING PREFIX BUG ID 376
   const BYTE* ptr=reinterpret_cast<const BYTE*>(&val);
//--
#ifdef XBOX
   mBuffer[mBufferSize]=ptr[1];
   mBuffer[mBufferSize+1]=ptr[0];
#else
   mBuffer[mBufferSize]=ptr[0];
   mBuffer[mBufferSize+1]=ptr[1];
#endif
   mBufferSize+=2;
}

//============================================================================
// BXFSFile::sendAddDWORD
//============================================================================
void BXFSFile::sendAddDWORD(DWORD val)
{
   //BASSERT(mBufferSize+sizeof(DWORD)<=cBufferSize);
//-- FIXING PREFIX BUG ID 377
   const BYTE* ptr=reinterpret_cast<const BYTE*>(&val);
//--
#ifdef XBOX
   mBuffer[mBufferSize]=ptr[3];
   mBuffer[mBufferSize+1]=ptr[2];
   mBuffer[mBufferSize+2]=ptr[1];
   mBuffer[mBufferSize+3]=ptr[0];
#else
   mBuffer[mBufferSize]=ptr[0];
   mBuffer[mBufferSize+1]=ptr[1];
   mBuffer[mBufferSize+2]=ptr[2];
   mBuffer[mBufferSize+3]=ptr[3];
#endif
   mBufferSize+=4;
}

//============================================================================
// BXFSFile::sendAddHandle
//============================================================================
void BXFSFile::sendAddHandle(HANDLE val)
{
   //BASSERT(mBufferSize+sizeof(HANDLE)<=cBufferSize);
//-- FIXING PREFIX BUG ID 378
   const BYTE* ptr=reinterpret_cast<const BYTE*>(&val);
//--
   mBuffer[mBufferSize]=ptr[0];
   mBuffer[mBufferSize+1]=ptr[1];
   mBuffer[mBufferSize+2]=ptr[2];
   mBuffer[mBufferSize+3]=ptr[3];
   mBufferSize+=4;
}

//============================================================================
// BXFSFile::sendAddString
//============================================================================
void BXFSFile::sendAddString(const TCHAR* str)
{
   WORD len=0;
   len=(WORD)strlen(str);
   //BASSERT(mBufferSize+sizeof(WORD)+len<=cBufferSize);
   sendAddWORD(len);
   BYTE* bufferPtr=mBuffer+mBufferSize;
   for(long i=0; i<len; i++)
      bufferPtr[i]=(BYTE)str[i];
   mBufferSize+=len;
}

//============================================================================
// BXFSFile::sendEnd
//============================================================================
bool BXFSFile::sendEnd()
{
   sendAddBYTE(cCommandSuffix);

   // send small packets up to the buffer size so we don't fragment at the tcp stack level
   int totalBytesSent = 0;
   int toSend = min(mBufferSize, 1408);
   while (totalBytesSent < mBufferSize)
   {
      int bytesSent = send(mSocket, reinterpret_cast<const char*>(mBuffer+totalBytesSent), toSend, 0);

      if (bytesSent == 0 || bytesSent == SOCKET_ERROR)
      {
         //BASSERT(0);
         trace("socket send error %d", WSAGetLastError());
         return false;
      }

      totalBytesSent += bytesSent;
      toSend = min(mBufferSize - totalBytesSent, 1408);
   }
   mBufferSize=0;
   return true;
}

//============================================================================
// BXFSFile::recvData
//============================================================================
bool BXFSFile::recvData(BYTE* buffer, long offset, long size)
{
   char* recvBuffer=(char*)buffer+offset;
   long totalRecv=0;
   long bytesLeft=size;
   while(totalRecv<size)
   {
      int bytes=recv(mSocket, recvBuffer, bytesLeft, 0);
      if(bytes==0 || bytes==SOCKET_ERROR)
      {
         //BASSERT(0);
         return false;
      }
      totalRecv+=bytes;
      bytesLeft-=bytes;
      recvBuffer+=bytes;
   }
   return true;
}

//============================================================================
// BXFSFile::recvBYTE
//============================================================================
bool BXFSFile::recvBYTE(BYTE& data)
{
   if(!recvData(&data, 0, sizeof(data)))
      return false;
   return true;
}

//============================================================================
// BXFSFile::recvWORD
//============================================================================
bool BXFSFile::recvWORD(WORD& data)
{
   if(!recvData(reinterpret_cast<BYTE*>(&data), 0, sizeof(data)))
      return false;
#ifdef XBOX
   EndianSwitchWords(&data, 1);
#endif
   return true;
}

//============================================================================
// BXFSFile::recvDWORD
//============================================================================
bool BXFSFile::recvDWORD(DWORD& data)
{
   if(!recvData(reinterpret_cast<BYTE*>(&data), 0, sizeof(data)))
      return false;
#ifdef XBOX
   EndianSwitchDWords(&data, 1);
#endif
   return true;
}

//============================================================================
// BXFSFile::recvUINT64
//============================================================================
bool BXFSFile::recvUINT64(uint64& data)
{
   if(!recvData(reinterpret_cast<BYTE*>(&data), 0, sizeof(data)))
      return false;
#ifdef XBOX
   EndianSwitchQWords(&data, 1);
#endif
   return true;
}

//============================================================================
// BXFSFile::recvHandle
//============================================================================
bool BXFSFile::recvHandle(HANDLE& data)
{
   if(!recvData(reinterpret_cast<BYTE*>(&data), 0, sizeof(data)))
      return false;
   return true;
}

//============================================================================
// BXFSFile::recvLARGE_INTEGER
//============================================================================
bool BXFSFile::recvLARGE_INTEGER(LARGE_INTEGER& data)
{
   if(!recvData(reinterpret_cast<BYTE*>(&data), 0, sizeof(data)))
      return false;
#ifdef XBOX
   EndianSwitchQWords(reinterpret_cast<unsigned __int64*>(&data), 1);
#endif
   return true;
}

//============================================================================
// BXFSFile::recvInt
//============================================================================
bool BXFSFile::recvInt(int& data)
{
   if(!recvData(reinterpret_cast<BYTE*>(&data), 0, sizeof(data)))
      return false;
#ifdef XBOX
   EndianSwitchDWords(reinterpret_cast<DWORD*>(&data), 1);
#endif
   return true;
}

//============================================================================
// BXFSFile::recvLong
//============================================================================
bool BXFSFile::recvLong(long& data)
{
   if(!recvData(reinterpret_cast<BYTE*>(&data), 0, sizeof(data)))
      return false;
#ifdef XBOX
   EndianSwitchDWords(reinterpret_cast<DWORD*>(&data), 1);
#endif
   return true;
}

//============================================================================
// BXFSFile::recvBOOL
//============================================================================
bool BXFSFile::recvBOOL(BOOL& data)
{
   if(!recvData(reinterpret_cast<BYTE*>(&data), 0, sizeof(data)))
      return false;
#ifdef XBOX
   EndianSwitchDWords(reinterpret_cast<DWORD*>(&data), 1);
#endif
   return true;
}

//============================================================================
// BXFSFile::recvBool
//============================================================================
bool BXFSFile::recvBool(bool& data)
{
   if(!recvData(reinterpret_cast<BYTE*>(&data), 0, sizeof(data)))
      return false;
   return true;
}

//============================================================================
// BXFSFile::recvSkip
//============================================================================
bool BXFSFile::recvSkip(long size)
{
   long bytesLeft=size;
   while(bytesLeft>0)
   {
      long bytesRecv=min(bytesLeft, cBufferSize);
      bytesLeft-=bytesRecv;
      if(!recvData(mBuffer, 0, bytesRecv))
         return false;
   }
   return true;
}

//============================================================================
// BXFSFile::resetServer
//============================================================================
bool BXFSFile::resetServer(void)
{
   // Tell the server to reset any previous connections from our same IP (usually left open due to client crashing or ending a debug session)
   sendBegin(cFuncReset);
   sendEnd();

   int origDelay = 0;
   int size = sizeof(origDelay);
   getsockopt(mSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&origDelay, &size);
   
   int delay=5000;
   int status=setsockopt(mSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)(&delay), sizeof(delay));   
   
   BYTE commandPrefix, funcType, version, suffix;
   bool failed = (!recvBYTE(commandPrefix) || !recvBYTE(funcType) || !recvBYTE(version) || !recvBYTE(suffix) || commandPrefix!=cCommandPrefix || funcType!=cFuncReset || version!= 1 || suffix!=cCommandSuffix);
      
   status=setsockopt(mSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)(&origDelay), sizeof(origDelay));   

   return !failed;
}      

//============================================================================
// BXFSLowLevelFileIO::createFileA
//============================================================================
HANDLE BXFSLowLevelFileIO::createFileA(const char* lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
   return gXFS.createFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

//============================================================================
// BXFSLowLevelFileIO::closeHandle
//============================================================================
BOOL BXFSLowLevelFileIO::closeHandle(HANDLE hObject)
{
   return gXFS.closeHandle(hObject);
}

//============================================================================
// BXFSLowLevelFileIO::flush
//============================================================================
BOOL BXFSLowLevelFileIO::flush(HANDLE hObject)
{
   return gXFS.flushFileBuffers(hObject);
}

//============================================================================
// BXFSLowLevelFileIO::readFile
//============================================================================
BOOL BXFSLowLevelFileIO::readFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
   return gXFS.readFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}

//============================================================================
// BXFSLowLevelFileIO::writeFile
//============================================================================
BOOL BXFSLowLevelFileIO::writeFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
   return gXFS.writeFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

//============================================================================
// BXFSLowLevelFileIO::setFilePointer
//============================================================================
DWORD BXFSLowLevelFileIO::setFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
   return gXFS.setFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
}

//============================================================================
// BXFSLowLevelFileIO::getFileSize
//============================================================================
DWORD BXFSLowLevelFileIO::getFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh)
{
   return gXFS.getFileSize(hFile, lpFileSizeHigh);
}

//============================================================================
// BWin32LowLevelFileIO::getFileSizeEx
//============================================================================
BOOL BXFSLowLevelFileIO::getFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize)
{
   return gXFS.getFileSizeEx(hFile, lpFileSize);
}

//============================================================================
// BXFSLowLevelFileIO::getFileInformationByHandle
//============================================================================
BOOL BXFSLowLevelFileIO::getFileInformationByHandle(HANDLE hFile, LPBY_HANDLE_FILE_INFORMATION lpFileInformation)
{
   return gXFS.getFileInformationByHandle(hFile, lpFileInformation);
}

//============================================================================
// BXFSLowLevelFileIO::getFileAttributes
//============================================================================
DWORD BXFSLowLevelFileIO::getFileAttributes(LPCSTR lpFileName)
{
   return gXFS.getFileAttributes(lpFileName);
}

//============================================================================
// BXFSLowLevelFileIO::getFileTime
//============================================================================
BOOL BXFSLowLevelFileIO::getFileTime(HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime)
{
   return gXFS.getFileTime(hFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime);
}

//============================================================================
// BXFSLowLevelFileIO::findFirstFile
//============================================================================
HANDLE BXFSLowLevelFileIO::findFirstFile(LPCSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData)
{
   return gXFS.findFirstFile(lpFileName, lpFindFileData);
}

//============================================================================
// BXFSLowLevelFileIO::findNextFile
//============================================================================
BOOL BXFSLowLevelFileIO::findNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData)
{
   return gXFS.findNextFile(hFindFile, lpFindFileData);
}

//============================================================================
// BXFSLowLevelFileIO::findClose
//============================================================================
BOOL BXFSLowLevelFileIO::findClose(HANDLE hFindFile)
{
   return gXFS.findClose(hFindFile);
}

//============================================================================
// BXFSLowLevelFileIO::getOverlappedResult
//============================================================================
BOOL BXFSLowLevelFileIO::getOverlappedResult(HANDLE hFile, LPOVERLAPPED lpOverlapped, LPDWORD lpNumberOfBytesTransferred, BOOL bWait)
{
   return gXFS.getOverlappedResult(hFile, lpOverlapped, lpNumberOfBytesTransferred, bWait);
}

//============================================================================
// BXFSLowLevelFileIO::setOwnerThread
//============================================================================
void BXFSLowLevelFileIO::setOwnerThread(HANDLE hFile, DWORD threadID)
{
   return gXFS.setOwnerThread(hFile, threadID);
}

//============================================================================
// BXFSLowLevelFileIO::hasOverlappedIoCompleted
//============================================================================
BOOL BXFSLowLevelFileIO::hasOverlappedIoCompleted(LPOVERLAPPED lpOverlapped)
{
   return gXFS.hasOverlappedIoCompleted(lpOverlapped);
}

//============================================================================
// BXFSLowLevelFileIO::setEndOfFile
//============================================================================
BOOL BXFSLowLevelFileIO::setEndOfFile(HANDLE hFile)
{
   return gXFS.setEndOfFile(hFile);
}

//============================================================================
// BXFSLowLevelFileIO::setFileAttributes
//============================================================================
BOOL BXFSLowLevelFileIO::setFileAttributes(LPCSTR lpFileName, DWORD dwFileAttributes)
{
   return gXFS.setFileAttributes(lpFileName, dwFileAttributes);
}

//============================================================================
// BXFSLowLevelFileIO::getFileAttributesEx
//============================================================================
BOOL BXFSLowLevelFileIO::getFileAttributesEx(LPCSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation)
{
   return gXFS.getFileAttributesEx(lpFileName, fInfoLevelId, lpFileInformation);
}

//============================================================================
// BXFSLowLevelFileIO::setFileTime
//============================================================================
BOOL BXFSLowLevelFileIO::setFileTime(HANDLE hFile, CONST FILETIME *lpCreationTime, CONST FILETIME *lpLastAccessTime, CONST FILETIME *lpLastWriteTime)
{
   return gXFS.setFileTime(hFile, lpCreationTime, lpLastAccessTime, lpLastWriteTime);
}

//============================================================================
// BXFSLowLevelFileIO::getFullPathName
//============================================================================
DWORD BXFSLowLevelFileIO::getFullPathName(LPCTSTR lpFileName, DWORD nBufferLength, LPTSTR lpBuffer, LPTSTR* lpFilePart)
{
   return gXFS.getFullPathName(lpFileName, nBufferLength, lpBuffer, lpFilePart);
}



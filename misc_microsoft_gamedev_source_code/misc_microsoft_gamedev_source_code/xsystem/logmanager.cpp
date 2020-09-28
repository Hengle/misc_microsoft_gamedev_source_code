//==============================================================================
// logmanager.cpp
//
// Copyright (c) 1999-2008 Ensemble Studios
//==============================================================================

#include "xsystem.h"
#include "logmanager.h"
#include "config.h"

#if defined(XBOX) && !defined(BUILD_FINAL)
#include <xbdm.h>
#include <xfs.h>
#endif

#ifdef USE_BUILD_INFO
#include "..\xgame\build_info.inc"
#endif

// xsystem
#include "econfigenum.h"
#include "string\bsnprintf.h"
#include "consoleOutput.h"
#include "threading\eventDispatcher.h"
#include "bfileStream.h"

// xcore
#include "file\win32File.h"
#include "file\xboxFileUtils.h"
#include "file\win32FileStream.h"
#include "file\xcontentStream.h"

// compression
#include "compressedStream.h"

//==============================================================================
// moved to crazy static segment
//BLogManager gLogManager;

long cDefaultHeader  = -1;
long cWarningsHeader = -1;
long cErrorsHeader   = -1;

// Update this number when you add an error or warning or the game will remind you.
const DWORD cNextErrorNumber = 4698;

//==============================================================================
// message
//==============================================================================
void message(const BCHAR_T  *lpszFormat, ...)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   va_list args;
   va_start(args, lpszFormat);

   messageV(lpszFormat, args);

	va_end(args);

} // message

//==============================================================================
// message
//==============================================================================
void messageV(const BCHAR_T *lpszFormat, va_list args)
{
#ifndef BUILD_FINAL
   ASSERT_THREAD(cThreadIndexSim);
   
   const int cLargeBufferSize = 8192;
   char szBuffer[cLargeBufferSize];
      
   HRESULT hr = StringCchVPrintf(szBuffer, countof(szBuffer), lpszFormat, args);

   if(!SUCCEEDED(hr))
   {
      va_end(args);
      return;
   }

   StringCchCat(szBuffer, countof(szBuffer), B("\r\n"));

   OutputDebugString(szBuffer);

#ifdef _BANG
   BSimString str = szBuffer;
	blog(str.getPtr());
#endif

#ifndef XBOX
   #ifdef _DEBUG
      MessageBox(0, szBuffer, B("Message"), MB_OK | MB_ICONWARNING);
   #endif
#endif

#endif // BUILD_FINAL
}

//==============================================================================
// DisplaySystemError
//==============================================================================
const void DisplaySystemError(bool bquiet, DWORD dwerror, const WCHAR *lpszFormat, ...)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   const int cLargeBufferSize = 8192;
   WCHAR szBufferW[cLargeBufferSize];
   
#ifdef XBOX
   // rg [6/10/05] - FIXME
   wcscpy_s(szBufferW, sizeof(szBufferW), L"doesn't work yet");
#else
	va_list args;
	va_start(args, lpszFormat);

	long nBuf = bvsnwprintf(szBufferW, cLargeBufferSize, lpszFormat, args);

   BASSERT(nBuf >= 0);               // no errors
   BASSERT(nBuf < cLargeBufferSize); // it fit

   if(nBuf>=0)
   {
	   nBuf += FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
						     dwerror,
						     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						     &szBufferW[nBuf], cLargeBufferSize - nBuf, NULL );// Display the string.

      // append a \r\n if necessary
      StringCchCatW(szBufferW, countof(szBufferW), L"\r\n");

	   OutputDebugStringW(szBufferW);
   }

#ifdef _BANG
   BSimString str(szBufferW);
   blog(str.getPtr());
#endif

   if (!bquiet)
	   MessageBoxW(0, szBufferW, L"Message", MB_OK | MB_ICONEXCLAMATION);

	va_end(args);
#endif
} // DisplaySystemError

//==============================================================================
// DisplaySystemError
//==============================================================================
const void DisplaySystemError(bool bquiet, DWORD dwerror, const char *lpszFormat, ...)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   const int cLargeBufferSize = 8192;
   char szBuffer[cLargeBufferSize];
      
#ifdef XBOX
   strcpy_s(szBuffer, sizeof(szBuffer), "doesn't work yet");
#else
	va_list args;
	va_start(args, lpszFormat);

	long nBuf = bvsnprintf(szBuffer, cLargeBufferSize, lpszFormat, args);

   BASSERT(nBuf >= 0);               // no errors
   BASSERT(nBuf < cLargeBufferSize); // it fit

   if(nBuf>=0)
   {
	   nBuf += FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL,
						     dwerror,
						     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						     &szBuffer[nBuf], cLargeBufferSize - nBuf, NULL );// Display the string.

      // append a \n if necessary
      StringCchCatA(szBuffer, countof(szBuffer), "\r\n");

	   OutputDebugStringA(szBuffer);
   }

#ifdef _BANG
   blog(szBuffer);
#endif

   if (!bquiet)
	   MessageBoxA(0, szBuffer, "Message", MB_OK | MB_ICONEXCLAMATION);

	va_end(args);
#endif
} // DisplaySystemError

//==============================================================================
// 
//==============================================================================
BLogFile::BLogFile() :
   mpStream(NULL),
   mpRawStream(NULL),
   mLogLines(NULL),
   mPostWriteAction(BLogManager::cPostWriteNone),
   miRollingLength(0),
   miLineNum(0),
   mbLineNumbering(false),
   mbValid(false),
   mbDelayedOpen(false),
   mbDelayedOpenAppend(false),
   mbDelayedOpenHeaderStamp(false)
{
}

//==============================================================================
// BLogManager::BLogManager
//==============================================================================
BLogManager::BLogManager() :
   mSourceFile(NULL),
   mSourceLine(-1),
   mBaseDirectoryID(-1),
   mCurrentHeaderID(-1),
   mCurrentErrorNumber(0),
   mLogTime(0),
   mUseTrace(false),
   mAlwaysTrace(false),
   mAlwaysPrintf(false),
   mDestructed(false),   
   mCacheEnabled(false)
{
   // Clear out headers
   for (long i = 0; i<cMaxLogHeaders; i++)
      mHeaders[i].mbValid = false;
   for (long i = 0; i<cMaxLogFiles; i++)
      mLogFiles[i].mbValid = false;
   // Get the time we were created.
   mStartupTime = timeGetTime();
} // BLogManager::BLogManager

//==============================================================================
// BLogManager::~BLogManager
//==============================================================================
BLogManager::~BLogManager()
{
   for (long i = 0; i<cMaxLogFiles; i++)
      closeLogFile(i);
   mDestructed=true;

} // BLogManager::~BLogManager


//==============================================================================
// BLogManager::logfn
//==============================================================================
bool BLogManager::logfn(const char *text, ...)  // Log text
{
   ASSERT_THREAD(cThreadIndexSim);
   
   if (mCurrentHeaderID < 0)
      return(false);

   int length;
   va_list ap;
   va_start(ap, text);
   length = bvsnprintf(mArgStr, cMaxLogTextLength, text, ap);
   va_end(ap);

   BASSERT(length >= 0);                         // no errors;
   BASSERT(length < cMaxLogTextLength);   // it fit

   // If we're doing the log of the warning or error header, also write it to the default header (as long as its not the same)

   if ((mCurrentHeaderID != cDefaultHeader) &&
      ((mCurrentHeaderID == cWarningsHeader) || (mCurrentHeaderID == cErrorsHeader)))
   {
      const char* srcFile = mSourceFile;
      long srcLine = mSourceLine;
      doLog(cDefaultHeader);
      mSourceFile = srcFile;
      mSourceLine = srcLine;
   }

   bool result = doLog(mCurrentHeaderID);

   mCurrentErrorNumber = 0;

   return(result);

} // BLogManager::logfn

//==============================================================================
// BLogManager::logfnnocrlf
// same as logfn(), except the string will not be appended with \r\n
//==============================================================================
bool BLogManager::logfnnocrlf(const char *text, ...)  // Log text
{
   ASSERT_THREAD(cThreadIndexSim);
   
   if (mCurrentHeaderID < 0)
      return(false);

   int length;
   va_list ap;
   va_start(ap, text);
   length = bvsnprintf(mArgStr, cMaxLogTextLength, text, ap);
   va_end(ap);

   BASSERT(length >= 0);                         // no errors;
   BASSERT(length < cMaxLogTextLength);   // it fit

   // If we're doing the log of the warning or error header, also write it to the default header (as long as its not the same)

   if ((mCurrentHeaderID != cDefaultHeader) &&
      ((mCurrentHeaderID == cWarningsHeader) || (mCurrentHeaderID == cErrorsHeader)))
   {
      const char* srcFile = mSourceFile;
      long srcLine = mSourceLine;
      doLog(cDefaultHeader);
      mSourceFile = srcFile;
      mSourceLine = srcLine;
   }

   bool result = doLog(mCurrentHeaderID, false);

   mCurrentErrorNumber = 0;

   return(result);

} // BLogManager::logfnnocrlf

//==============================================================================
// BLogManager::logfnh
//==============================================================================
bool BLogManager::logfnh(long headerID, const char *text, ...)  // Log text
{
   ASSERT_THREAD(cThreadIndexSim);
   
   if (headerID < 0)
      return(false);
   setCurrentHeaderID(headerID);

   int length;
   va_list ap;
   va_start(ap, text);
   length = bvsnprintf(mArgStr, cMaxLogTextLength, text, ap);
   va_end(ap);

   BASSERT(length >= 0);                         // no errors;
   BASSERT(length < cMaxLogTextLength);   // it fit

   // If we're doing the log of the warning or error header, also write it to the default header (as long as its not the same)
   if ((mCurrentHeaderID != cDefaultHeader) &&
      ((mCurrentHeaderID == cWarningsHeader) || (mCurrentHeaderID == cErrorsHeader)))
   {
      const char* srcFile = mSourceFile;
      long srcLine = mSourceLine;
      doLog(cDefaultHeader);
      mSourceFile = srcFile;
      mSourceLine = srcLine;
   }

   bool result = doLog(mCurrentHeaderID);

   mCurrentErrorNumber = 0;

   return(result);
} // BLogManager::logfnh

//==============================================================================
// BLogManager::doLog
//==============================================================================
bool BLogManager::doLog(long headerID, bool appendCRLF)
{
   // rg [6/16/06] - I'm going to use the event dispatcher to handle logs from other threads, eventually.
   ASSERT_THREAD(cThreadIndexSim);
   
   // Can't log after we're dead.
   if(mDestructed)
      return(false);
   if (headerID < 0)
      return(false);
      
   BConsoleMessageCategory msgCategory = cMsgDebug;

   if (headerID == cWarningsHeader)
      msgCategory = cMsgWarning;
   else if (headerID == cErrorsHeader)
      msgCategory = cMsgError;

   // Trace it if desired.
   // jce [1/20/2003] -- moved this first so that trace output works even if log file is not properly opened
   if(mUseTrace || mAlwaysTrace)
   {
      // Indent.
#ifndef BUILD_FINAL
      for (long i = 0; i < mHeaders[headerID].miIndentLevel; i++)
         OutputDebugStringA("   ");
#endif
      if (appendCRLF)
         trace(mArgStr);
      else
         tracenocrlf(mArgStr);
            
      if (mHeaders[headerID].mbConsoleOutput && msgCategory != cMsgDebug)
         gConsoleOutput.output(msgCategory, "%s", mArgStr);
   }
   else if (mHeaders[headerID].mbConsoleOutput)
   {
      gConsoleOutput.output(msgCategory, "%s", mArgStr);
   }

   if(mAlwaysPrintf)
   {
      // Indent.
      for (long i = 0; i < mHeaders[headerID].miIndentLevel; i++)
         puts("   ");
      
      puts(mArgStr);
      
      if (appendCRLF)
         puts("\n");
   }

   // Example log:   23  > FILE_XFER: (W:512312) (G:25343) (F: blogheader.cpp) (L: 532) File transfer completed!
   if (!mHeaders[headerID].mbValid)
   {
      mSourceFile = NULL;
      mSourceLine = -1;
      return false; // Invalid header
   }

   BLogFile* pLogFile = &(mLogFiles[mHeaders[headerID].miLogFile]);
   if (!pLogFile->mbValid)
   {
      mSourceFile = NULL;
      mSourceLine = -1;
      return false; // Invalid log file
   }

   if (pLogFile->mpStream && !pLogFile->mpStream->opened())
   {
      // if the stream is not opened then I want to delete my previous stream and allow a new one to be created
      pLogFile->mpStream = NULL;

      if (pLogFile->mpDeflateStream != NULL)
      {
         pLogFile->mpDeflateStream->close();
         delete pLogFile->mpDeflateStream;
         pLogFile->mpDeflateStream = NULL;
      }

      if (pLogFile->mpRawStream != NULL)
      {
         pLogFile->mpRawStream->close();
         delete pLogFile->mpRawStream;
         pLogFile->mpRawStream = NULL;
      }
   }

   // If the file isn't actually open (like if we are or were in close after write mode),
   // open it now.
   if (!pLogFile->mpStream)
   {
      BStream* pStream = pLogFile->mpStream;

      if (pStream == NULL)
      {
         uint flags = cSFWritable | cSFSeekable | cSFEnableBuffering;

         if (pLogFile->mbDelayedOpenAppend && !pLogFile->miRollingLength)
         {
            flags |= cSFOpenExisting;
         }

         BSimString fileName = pLogFile->mFileName;

         if (gConfig.isDefined(cConfigCompressLogs))
            fileName.append(".hwz");

         if (mCacheEnabled)
         {
            BWin32FileStream* pWin32Stream = new BWin32FileStream();

            if (!pWin32Stream->open(fileName, flags, &gWin32LowLevelFileIO))
            {
               delete pWin32Stream;
               return false;
            }

            pLogFile->mpRawStream = pWin32Stream;
            pStream = pWin32Stream;
         }
         else
         {
            BFileSystemStream* pFileSystemStream = new BFileSystemStream();

            if (!pFileSystemStream->open(mBaseDirectoryID, fileName, flags))
            {
               delete pFileSystemStream;
               return false;
            }

            pLogFile->mpRawStream = pFileSystemStream;
            pStream = pFileSystemStream;
         }

         if (gConfig.isDefined(cConfigCompressLogs))
         {
            pLogFile->mpDeflateStream = new BDeflateStream(*pStream);
            pLogFile->mpStream = pLogFile->mpDeflateStream;
         }
         else
         {
            pLogFile->mpStream = pStream;
         }

         pLogFile->mbDelayedOpen = false;
      }
   }

   writeHeaderStamp(pLogFile);

   if (strlen(mArgStr) > cMaxLogTextLength)
   {
      mSourceFile = NULL;
      mSourceLine = -1;
      return false;
   }

   BStream* pStream = pLogFile->mpStream;
   if (pStream == NULL)
   {
      mSourceFile = NULL;
      mSourceLine = -1;
      return false;
   }

   pLogFile->miLineNum++;

   // Print out the error or warning # if applicable.
   if (headerID != cDefaultHeader)
   {
      if (headerID == cWarningsHeader)
         if (mCurrentErrorNumber)
            pStream->printf("Warning(%.8d)  ", mCurrentErrorNumber);
         else
            pStream->printf("Warning(UNDEFINED)  ");
      else if(headerID == cErrorsHeader)
         if (mCurrentErrorNumber)
            pStream->printf("Error(%.8d)  ", mCurrentErrorNumber);   
         else
            pStream->printf("Error(UNDEFINED)  ");
   }

   if (pLogFile->mbLineNumbering)
      pStream->printf( "%-4d>", pLogFile->miLineNum);

   if (mHeaders[headerID].mbGameTimeStamp)
   {
      if (mLogTime > (DWORD)0)
      {
         long min=0;
         long hour=0;
         long sec=mLogTime/1000;
         if (sec > 59)
         {
            min=sec/60;
            sec=sec%60;
         }
         if (min > 59)
         {
            hour=min/60;
            min=min%60;
         }

         pStream->printf( "%-8d %02d:%02d:%02d:  ", mLogTime, hour, min, sec);
      }
      else
         pStream->printf( "PreGame  %ld:  ", timeGetTime()-mStartupTime);
   }
   else if(mHeaders[headerID].mbTimeStamp)
      pStream->printf( "Time %ld:  ", timeGetTime()-mStartupTime);

   // Indent.
   for (long i = 0; i < mHeaders[headerID].miIndentLevel; i++)
      pStream->printf( "   ");

   // File and line.
   if(mHeaders[headerID].mbFileAndLineStamp)
   {
      if(mSourceFile == NULL || mSourceLine<0)
         pStream->printf( "<invalid file & line -- please use the blog or blogh macro>");
      else
         pStream->printf( "<%s(%d)> ", mSourceFile, mSourceLine);
   }

   // Print header if desired.
   if (mHeaders[headerID].mbTitleStamp)
      pStream->printf( "#%s# ", mHeaders[headerID].msTitle);

   // Print actual logged text.
   // strip out a \r if present as either all lines should have it or none of them.
   int length = strlen(mArgStr);

   // 6/4/00 - ham - fixes problem where mArgStr is NULL (ie: length == 0)

   if (length)
   {
      if ((!appendCRLF) || (mArgStr[strlen(mArgStr)-1] == '\n'))
         pStream->printf( "%s", mArgStr);
      else
         pStream->printf( "%s\r\n", mArgStr);
   }

   // Flush or close the file as requested.
   switch (pLogFile->mPostWriteAction)
   {
      case cPostWriteClose:
         // Close the file.
         if (pLogFile->mpDeflateStream)
            pLogFile->mpDeflateStream->close();
         if (pLogFile->mpRawStream)
            pLogFile->mpRawStream->close();
         break;
   }

   mSourceFile = NULL;
   mSourceLine = -1;
   return(true);

} // BLogManager::doLog

//==============================================================================
// 
//==============================================================================
void BLogManager::writeHeaderStamp(BLogFile* pLogFile)
{
   if (!pLogFile || pLogFile->mpStream == NULL)
      return;

   if (pLogFile->mbDelayedOpenHeaderStamp && pLogFile->mpStream->size() == 0)
   {
      // always put the build info at the top of the file if we've requested the header timestamp
      time_t clock;
      time(&clock);
      struct tm tmTime;
      localtime_s(&tmTime, &clock);
      pLogFile->mpStream->printf("===============================================================\r\n");

      char t[256];
      asctime_s(t, sizeof(t), &tmTime);
      pLogFile->mpStream->printf("File '%s' opened at %s\r\n", pLogFile->mFileName.getPtr(), t);

#ifdef USE_BUILD_INFO
      pLogFile->mpStream->printf("Build Info: DEPOT:"DEPOT_REVISION"|BUILD:"BUILD_NUMBER"|CHANGELIST:"CHANGELIST_NUMBER"\r\n");
#else
      pLogFile->mpStream->printf("Build Info: NO_BUILD_INFO\r\n");
#endif

#if defined(XBOX)
   #if !defined(BUILD_FINAL)
      DM_XBE xbeInfo;
      HRESULT hr = DmGetXbeInfo(NULL, &xbeInfo);
      if (hr == XBDM_NOERR)
      {
         pLogFile->mpStream->printf("XBE Launch Path: %s\r\n", xbeInfo.LaunchPath);
         pLogFile->mpStream->printf("XBE TimeStamp: %d\r\n", xbeInfo.TimeStamp);
         pLogFile->mpStream->printf("XBE CheckSum: %d\r\n", xbeInfo.CheckSum);
      }
      char buf[256];
      DWORD size = sizeof(buf);
      hr = DmGetXboxName(buf, &size);
      if (hr == XBDM_NOERR)
      {
         pLogFile->mpStream->printf("Xbox Name: %s\r\n", buf);
      }
   #endif

      CHAR gamerTag[XUSER_NAME_SIZE];
      for (DWORD i=0; i < 4; ++i)
      {
         if (XUserGetName(i, gamerTag, XUSER_NAME_SIZE) == ERROR_SUCCESS)
         {
            pLogFile->mpStream->printf("User (%d): %s\r\n", i, gamerTag);
         }
      }
#endif

      pLogFile->mpStream->printf("===============================================================\r\n\r\n");
   }
}

//==============================================================================
// BLogManager::createHeader
//==============================================================================
long BLogManager::createHeader(const char* pName,
                               long logFile,
                               long parentHeaderID,
                               bool timestamp,
                               bool gametimestamp,
                               bool titlestamp,
                               bool logFileAndLine,
                               bool consoleOutput)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   long i;

   // Get first available header
   for (i = 0; i<cMaxLogHeaders; i++)
   {      
      if (!mHeaders[i].mbValid)
         break;
      if ((mHeaders[i].miLogFile == logFile) && (!strcmp(mHeaders[i].msTitle, pName)))
         return i; // we already have this log open
   }
   if (i >= cMaxLogHeaders)
   {
      BASSERT(0);
      return(-1); // Unable to get free header
   }

   if (strlen(pName) > BLogHeader::cMaxHeaderNameLength)
   {
      BASSERT(0);
      return(-1); // Invalid name length
   }

   if (!mHeaders[parentHeaderID].mbValid)
      parentHeaderID = cBaseHeader; // Invalid Parent, using root instead

   StringCchCopyA(mHeaders[i].msTitle, countof(mHeaders[i].msTitle), pName);
   mHeaders[i].miParentHeader = parentHeaderID;
   if (parentHeaderID == cBaseHeader)
      mHeaders[i].miIndentLevel = 0;
   else
      mHeaders[i].miIndentLevel = mHeaders[parentHeaderID].miIndentLevel+1;
   mHeaders[i].mbTitleStamp = titlestamp;
   mHeaders[i].mbGameTimeStamp = gametimestamp;
   mHeaders[i].mbTimeStamp = timestamp;
   mHeaders[i].mbFileAndLineStamp = logFileAndLine;
   if ((logFile < 0) || (logFile >= cMaxLogFiles))
   {
      BASSERT(0);
      return(-1);
   }
   mHeaders[i].miLogFile = logFile;
   mHeaders[i].mbValid = true;
   mHeaders[i].mbConsoleOutput = consoleOutput;

   return i;

} // BLogManager::createHeader

//==============================================================================
// BLogManager::headerIndent
// Ident header one collumn
//==============================================================================
void  BLogManager::headerIndent(long headerID)                  
{
   ASSERT_THREAD(cThreadIndexSim);
   
   mHeaders[headerID].miIndentLevel++;

} // BLogManager::headerIndent

//==============================================================================
// BLogManager::headerUnindent
// Decrease header indent by one collumn
//==============================================================================
void  BLogManager::headerUnindent(long headerID)                
{
   ASSERT_THREAD(cThreadIndexSim);
   
   mHeaders[headerID].miIndentLevel--;
   if (mHeaders[headerID].miIndentLevel < 0)
      mHeaders[headerID].miIndentLevel = 0;
} // BLogManager::headerUnindent

//==============================================================================
// BLogManager::setIndent
// Manually set header indent level
//==============================================================================
void  BLogManager::setIndent(long headerID, long indent)    
{
   ASSERT_THREAD(cThreadIndexSim);
   
   mHeaders[headerID].miIndentLevel = indent;
   if (mHeaders[headerID].miIndentLevel < 0)
      mHeaders[headerID].miIndentLevel = 0;
} // BLogManager::setIndent

//==============================================================================
// BLogManager::getIndent
// Get header indent level
//==============================================================================
long   BLogManager::getIndent(long headerID) const
{
   ASSERT_THREAD(cThreadIndexSim);
   
   return mHeaders[headerID].miIndentLevel;
}

//==============================================================================
// BLogManager::getParentHeader
// Get the parent of this header
//==============================================================================
long   BLogManager::getParentHeader(long headerID) const
{
   ASSERT_THREAD(cThreadIndexSim);
   
   return mHeaders[headerID].miParentHeader;

} // BLogManager::getParentHeader

//==============================================================================
// BLogManager::setPostWriteAction
// Sets action taken after each write to the log.
//==============================================================================
void  BLogManager::setPostWriteAction(long fileID, long val)  
{
   ASSERT_THREAD(cThreadIndexSim);
   
   mLogFiles[fileID].mPostWriteAction = val;
} // BLogManager::setFlushAfterWrite

//==============================================================================
// BLogManager::setLineNumbering
// Set true to number each line
//==============================================================================
void  BLogManager::setLineNumbering(long fileID, bool val)    
{
   ASSERT_THREAD(cThreadIndexSim);
   
   mLogFiles[fileID].mbLineNumbering = val;
} // BLogManager::setLineNumbering

//==============================================================================
// BLogManager::setGameTimeStamp
// Set true for per-line timestamp for this header
//==============================================================================
void  BLogManager::setGameTimeStamp(long headerID, bool val)      
{
   ASSERT_THREAD(cThreadIndexSim);  
   
   mHeaders[headerID].mbGameTimeStamp = val;
} // BLogManager::setGameTimeStamp

//==============================================================================
// BLogManager::setTitleStamp
// Set true to stamp the header's title per line
//==============================================================================
void  BLogManager::setTitleStamp(long headerID, bool val)     
{
   ASSERT_THREAD(cThreadIndexSim);
   
   mHeaders[headerID].mbTitleStamp = val;
} // BLogManager::setTitleStamp

//============================================================================== 
// BLogManager::setLogFile
// Set the file handle for logging manually
//==============================================================================
bool  BLogManager::setLogFile(long headerID, long fileID)      
{
   ASSERT_THREAD(cThreadIndexSim);
   
   if (!mLogFiles[fileID].mbValid)
      return false;

   mHeaders[headerID].miLogFile = fileID;

   return true;
} // BLogManager::setLogFile

//==============================================================================
// BLogManager::openLogFile
//==============================================================================
long  BLogManager::openLogFile(const BSimString &fileName, long postWriteAction, bool append, long rollingLength,
   bool lineNumbering, bool headerStamp, bool createFileImmediate)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   // Copy filename because we might change it.
   BSimString fullname;
   fullname.set(fileName);
#ifdef XBOX
   mCacheEnabled = BXboxFileUtils::isCacheInitialized();
#endif

   if (mCacheEnabled)
   {
      if (!CreateDirectory("cache:\\logs", NULL))
      {
         DWORD dwError = GetLastError();
         if (dwError != ERROR_ALREADY_EXISTS)
            mCacheEnabled = false;
      }
   }


#ifdef XBOX
#ifndef BUILD_FINAL
   bool logToCacheDrive = gConfig.isDefined(cConfigLogToCacheDrive);

   // if XFS is active and the cache drive is not enabled,
   // then it doesn't matter if we wanted to log to the
   // cache drive or not
   if (gXFS.isActive() && !logToCacheDrive)
   {
      // disable the cache drive if XFS is enabled and we haven't
      // defined LogToCacheDrive
      mCacheEnabled = false;
      fullname.set(fileName);
   }
   else if (logToCacheDrive && mCacheEnabled)
      fullname.format("cache:\\logs\\%s", fileName.getPtr());
   else
      fullname.format("d:\\%s", fileName.getPtr());
#else
   if (!mCacheEnabled)
      return -1;
   fullname.format("cache:\\logs\\%s", fileName.getPtr());
#endif
#endif

#ifndef XBOX
   #ifndef BUILD_FINAL
   if(gConfig.isDefined(cConfigUseAlternateLog))
   {
      BCHAR_T computerName[MAX_COMPUTERNAME_LENGTH+2];
      DWORD nameSize = MAX_COMPUTERNAME_LENGTH+1;
      BOOL ok = GetComputerName(computerName+1, &nameSize);
      if(!ok)
         computerName[1] = '0';
      computerName[0] = ' ';

      long dotPos = fullname.findRight(B('.'));
      fullname.insert(dotPos, computerName);
   }
   #endif
#endif

   // Can't open more logs when we're dead.   
   if(mDestructed)
      return(-1);

   // do we already have this log file open?
   long i;
   for (i = 0; i<cMaxLogFiles; i++)
   {
      if (mLogFiles[i].mbValid && !fullname.compare(mLogFiles[i].mFileName))
         break;
   }
   if (i < cMaxLogFiles)
      return i; // we already have this log file open

   for (i = 0; i<cMaxLogFiles; i++)
   {
      if (!mLogFiles[i].mbValid)
         break;
   }
   if (i >= cMaxLogFiles)
      return(-1); // Unable to get free header

   mLogFiles[i].mFileName = fullname;

   // make sure the file isn't read only 
   //gFileManager.setFileReadOnly(mBaseDirectoryID, fullname, false);

   // open file
   if (createFileImmediate)
   {
      BStream* pStream = mLogFiles[i].mpStream;

      uint flags = cSFWritable | cSFSeekable | cSFEnableBuffering;

      if (append && !rollingLength)
      {
         flags |= cSFOpenExisting;
      }

      if (pStream == NULL)
      {
         BSimString fileName = fullname;

         if (gConfig.isDefined(cConfigCompressLogs))
            fileName.append(".hwz");

         if (mCacheEnabled)
         {
            BWin32FileStream* pWin32Stream = new BWin32FileStream();

            if (!pWin32Stream->open(fileName, flags, &gWin32LowLevelFileIO))
            {
               delete pWin32Stream;
               return -1;
            }

            mLogFiles[i].mpRawStream = pWin32Stream;
            pStream = pWin32Stream;
         }
         else
         {
            BFileSystemStream* pFileSystemStream = new BFileSystemStream();

            if (!pFileSystemStream->open(mBaseDirectoryID, fileName, flags))
            {
               delete pFileSystemStream;
               return -1;
            }

            mLogFiles[i].mpRawStream = pFileSystemStream;
            pStream = pFileSystemStream;
         }

         if (gConfig.isDefined(cConfigCompressLogs))
         {
            mLogFiles[i].mpDeflateStream = new BDeflateStream(*pStream);
            mLogFiles[i].mpStream = mLogFiles[i].mpDeflateStream;
         }
         else
         {
            mLogFiles[i].mpStream = pStream;
         }
      }
   }

   mLogFiles[i].mbDelayedOpen = !createFileImmediate;
   mLogFiles[i].mbDelayedOpenAppend = append;
   mLogFiles[i].mbDelayedOpenHeaderStamp = headerStamp;
   mLogFiles[i].mPostWriteAction = postWriteAction;
   mLogFiles[i].mLogLines = NULL;

   if (rollingLength > BLogFile::cMaxRollingLines)
      rollingLength = BLogFile::cMaxRollingLines;

   mLogFiles[i].miRollingLength = rollingLength;
   if (rollingLength)
   {
      mLogFiles[i].mLogLines = new char[(rollingLength*BLogFile::cLogLineLength)+1];
      if (!mLogFiles[i].mLogLines)
         return(-1);
   }

   mLogFiles[i].miLineNum = 0;
   mLogFiles[i].mbLineNumbering = lineNumbering;
   mLogFiles[i].mbValid = true;

   if (createFileImmediate)
   {
      writeHeaderStamp(&mLogFiles[i]);
   }

   return i;

} // BLogManager::openLogFile

//==============================================================================
// BLogManager::closeLogFile
// Close the current log file
//==============================================================================
void  BLogManager::closeLogFile(long fileID)                  
{
   //ASSERT_THREAD(cThreadIndexSim);
   
   if (fileID < 0 || fileID >= cMaxLogFiles)
      return;

   if (!mLogFiles[fileID].mbValid)
      return;

   mLogFiles[fileID].mpStream = NULL;

   // Close the file (if not already closed).
   if (mLogFiles[fileID].mpDeflateStream != NULL)
   {
      mLogFiles[fileID].mpDeflateStream->close();
      delete mLogFiles[fileID].mpDeflateStream;
      mLogFiles[fileID].mpDeflateStream = NULL;
   }

   if (mLogFiles[fileID].mpRawStream != NULL)
   {
      mLogFiles[fileID].mpRawStream->close();
      delete mLogFiles[fileID].mpRawStream;
      mLogFiles[fileID].mpRawStream = NULL;
   }

   if (mLogFiles[fileID].mLogLines)
   {
      delete[] mLogFiles[fileID].mLogLines;
      mLogFiles[fileID].mLogLines = NULL;
   }

   mLogFiles[fileID].mbValid = false;

} // BLogManager::closeLogFile

//==============================================================================
// BLogManager::getLogFile
// Return the file id for a given header
//==============================================================================
long BLogManager::getLogFile(long headerID) const
{
   ASSERT_THREAD(cThreadIndexSim);
   
   return mHeaders[headerID].miLogFile;
} // BLogManager::getLogFile

//==============================================================================
// BLogManager::flushAllLogs
//==============================================================================
void BLogManager::flushAllLogs()
{
   ASSERT_THREAD(cThreadIndexSim);
   
   for (long i=0; i<cMaxLogFiles; i++)
   {
      if (mLogFiles[i].mbValid)
      {
         flushLogFile(i);

         mLogFiles[i].mpStream = NULL;

         if (mLogFiles[i].mpDeflateStream != NULL)
         {
            mLogFiles[i].mpDeflateStream->close();
            delete mLogFiles[i].mpDeflateStream;
            mLogFiles[i].mpDeflateStream = NULL;
         }

         if (mLogFiles[i].mpRawStream != NULL)
         {
            mLogFiles[i].mpRawStream->close();
            delete mLogFiles[i].mpRawStream;
            mLogFiles[i].mpRawStream = NULL;
         }
      }
   }
} // BLogManager::flushAllLogs

//=============================================================================
// BLogManager::setCurrentErrorNumber(DWORD newErrorNumber)
//=============================================================================
void BLogManager::setCurrentErrorNumber(DWORD newErrorNumber)
{
   ASSERT_THREAD(cThreadIndexSim);
   
   BASSERTM((newErrorNumber < cNextErrorNumber), "Someone added this error/warning without updating cNextErrorNumber in logmanager.cpp.  Please update this number to reflect what the next added error/warning number should be.  YouPlease update this number so the next person can easily tell what error number we are up to.");
   mCurrentErrorNumber = newErrorNumber;
}

//==============================================================================
// BLogManager::flushLogFile
// Flush log to disk
//==============================================================================
void  BLogManager::flushLogFile(long fileID)                  
{
   ASSERT_THREAD(cThreadIndexSim);
   
   if (!mLogFiles[fileID].mbValid)
      return;

   // Close the file (if not already closed).
   if (mLogFiles[fileID].mpRawStream != NULL)
      mLogFiles[fileID].mpRawStream->flush();

} // BLogManager::flushLogFile

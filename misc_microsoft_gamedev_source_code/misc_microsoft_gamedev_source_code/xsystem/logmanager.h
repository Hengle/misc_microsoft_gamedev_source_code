//==============================================================================
// logmanager.h
//
// Copyright (c) 1999-2008, Ensemble Studios 
//==============================================================================


//==============================================================================
#pragma once

#define LOG_ENABLED 1

//==============================================================================
#include "logheader.h"
#include "logfile.h"
//#include "bstring.h"

//==============================================================================
class BLogManager;
//class BSimString;

extern BLogManager gLogManager;
extern long cDefaultHeader;
extern long cWarningsHeader;
extern long cErrorsHeader;
extern const DWORD cNextErrorNumber;

//==============================================================================
// Macros
//==============================================================================

#ifdef LOG_ENABLED
   #define finalBlog gLogManager.setCurrentHeaderID(cDefaultHeader), gLogManager.setFileAndLine(__FILE__,__LINE__), gLogManager.setUseTrace(false), gLogManager.logfn
   #define finalBlogTrace gLogManager.setCurrentHeaderID(cDefaultHeader), gLogManager.setFileAndLine(__FILE__,__LINE__), gLogManager.setUseTrace(true), gLogManager.logfn
   #define finalBlogh gLogManager.setFileAndLine(__FILE__,__LINE__), gLogManager.setUseTrace(false), gLogManager.logfnh
   #define finalBlogWarning gLogManager.setCurrentHeaderID(cWarningsHeader), gLogManager.setFileAndLine(__FILE__,__LINE__), gLogManager.setUseTrace(false), gLogManager.logfn
   #define finalBlogError gLogManager.setCurrentHeaderID(cErrorsHeader), gLogManager.setFileAndLine(__FILE__,__LINE__), gLogManager.setUseTrace(false), gLogManager.logfn
   #define setBlogError(x) gLogManager.setCurrentErrorNumber(x)
   #define setBlogWarning setBlogError
#ifndef BUILD_FINAL
   #define blog gLogManager.setCurrentHeaderID(cDefaultHeader), gLogManager.setFileAndLine(__FILE__,__LINE__), gLogManager.setUseTrace(false), gLogManager.logfn
   #define blogh gLogManager.setFileAndLine(__FILE__,__LINE__), gLogManager.setUseTrace(false), gLogManager.logfnh
   #define blogtrace gLogManager.setCurrentHeaderID(cDefaultHeader), gLogManager.setFileAndLine(__FILE__,__LINE__), gLogManager.setUseTrace(true), gLogManager.logfn
   #define blogtracenocrlf gLogManager.setCurrentHeaderID(cDefaultHeader), gLogManager.setFileAndLine(__FILE__,__LINE__), gLogManager.setUseTrace(true), gLogManager.logfnnocrlf
   #define bloghtrace gLogManager.setFileAndLine(__FILE__,__LINE__), gLogManager.setUseTrace(true), gLogManager.logfnh
   #define blogwarning gLogManager.setCurrentHeaderID(cWarningsHeader), gLogManager.setFileAndLine(__FILE__,__LINE__), gLogManager.setUseTrace(false), gLogManager.logfn
   #define blogwarningtrace gLogManager.setCurrentHeaderID(cWarningsHeader), gLogManager.setFileAndLine(__FILE__,__LINE__), gLogManager.setUseTrace(true), gLogManager.logfn
   #define blogerror gLogManager.setCurrentHeaderID(cErrorsHeader), gLogManager.setFileAndLine(__FILE__,__LINE__), gLogManager.setUseTrace(false), gLogManager.logfn
   #define blogerrortrace gLogManager.setCurrentHeaderID(cErrorsHeader), gLogManager.setFileAndLine(__FILE__,__LINE__), gLogManager.setUseTrace(true), gLogManager.logfn
#else
   #ifdef __INTEL_COMPILER
      // rg [3/15/05] - Without this ICL blows up on blogtrace() in FINAL builds.
      #define blog 
      #define blogh
      #define blogtrace 
      #define bloghtrace 
      #define blogwarning 
      #define blogwarningtrace 
      #define blogerror 
      #define blogerrortrace 
   #else
      #define blog ((void)0)
      #define blogh ((void)0)
      #define blogtrace ((void)0)
      #define bloghtrace ((void)0)
      #define blogwarning ((void)0)
      #define blogwarningtrace ((void)0)
      #define blogerror ((void)0)
      #define blogerrortrace ((void)0)
   #endif      
#endif
#else
   #define finalBlog ((void)0)
   #define finalBlogWarning ((void)0)
   #define finalBlogError ((void)0)
   #define blog ((void)0)
   #define blogh ((void)0)
   #define blogtrace ((void)0)
   #define bloghtrace ((void)0)
   #define blogwarning ((void)0)
   #define blogwarningtrace ((void)0)
   #define blogerror ((void)0)
   #define blogerrortrace ((void)0)
   #define setBlogWarning(x) ((void)0)
   #define setBlogError(x) ((void)0)
#endif

//==============================================================================
// global functions
//==============================================================================
void message(const BCHAR_T *lpszFormat, ...);
void messageV(const BCHAR_T *lpszFormat, va_list args);

const void DisplaySystemError(bool bquiet, DWORD dwerror, const WCHAR *lpszFormat, ...);
const void DisplaySystemError(bool bquiet, DWORD dwerror, const char *lpszFormat, ...);

//=============================================================================
// BLog Class
//=============================================================================
class BLogManager
{
   public:

      //=============================================================================
      // Class defined Data Types
      //=============================================================================

      //=============================================================================
      // Class Member Functions
      //=============================================================================

                                 BLogManager();     // FIXME: Make constructor clear out the headers to HEADER_INVALID
                                 ~BLogManager();

      enum
      {
         cBaseHeader       = 0,
         cMaxLogHeaders    = 128,
         cMaxLogFiles      = 32,
         cMaxLogTextLength = 8192  // 12/29/99 - ham - changed length from 256, since 256 is obiviously too small
      };

      enum
      {
         cPostWriteNone,
         cPostWriteFlush,
         cPostWriteClose
      };

      void                       setFileAndLine(const char *file, const long line) {mSourceFile = file; mSourceLine = line;}
      bool                       logfn(const char *text, ...);
      // same as logfn(), except the string will not be appended with \r\n
      bool                       logfnnocrlf(const char *text, ...);
      bool                       logfnh(long headerID, const char *text, ...);
      
      // use this function if you want to create a log header. specify the logFileID manually
      // jce [7/1/2005] -- note that "gametimestamp" will supercede "timestamp" if both are true, showing the gametime when available.
      // jce [7/1/2005] --  defaults used to be: long parentHeaderID = cBaseHeader, bool gametimestamp = false, bool titlestamp = false, bool logFileAndLine = false
      long                       createHeader(const char* pName, long logFile, long parentHeaderID, bool timestamp, bool gametimestamp, bool titlestamp, bool logFileAndLine, bool consoleOutput=true);
      void                       setUseTrace(bool useTrace) {mUseTrace=useTrace;}
      void                       setAlwaysTrace(bool alwaysTrace) {mAlwaysTrace=alwaysTrace;}
      void                       setAlwaysPrintf(bool alwaysPrintf) {mAlwaysPrintf=alwaysPrintf;}

      // Header indentation
      void                       headerIndent(long headerID);                 
      void                       headerUnindent(long headerID);               
      void                       setIndent(long headerID, long indent);       
      long                       getIndent(long headerID) const;
      long                       getParentHeader(long headerID) const;
      void                       initLog(char *file, long line);

      // Header settings
      void                       setEnable(long headerID, bool enable);       
      bool                       getEnable(long headerID) const;
      void                       setGameTimeStamp(long headerID, bool val);   
      void                       setTitleStamp(long headerID, bool val);      

      // Log File settings
      void                       setPostWriteAction(long fileID, long val); 
      void                       setLineNumbering(long fileID, bool val);    
      bool                       setLogFile(long headerID, long fileID);     
      long                       getLogFile(long headerID) const;
                                    
      long                       openLogFile(const BSimString &fileName, long postWriteAction = cPostWriteNone, bool append = false,          
                                    long rollingLength = 0, bool lineNumbering = true, bool headerStamp = true, bool createFileImmediate = true);   
      void                       closeLogFile(long fileID);
      void                       closeLogFileForHeader(long headerID) { closeLogFile(getLogFile(headerID)); }
      void                       flushLogFile(long fileID);
      void                       flushAllLogs();
      void                       setBaseDirectoryID(long dirID) { mBaseDirectoryID = dirID; }
      long                       getBaseDirectoryID(void) const {return(mBaseDirectoryID);}
      void                       setCurrentHeaderID(long hID) { mCurrentHeaderID = hID; }
      void                       setCurrentErrorNumber(DWORD newErrorNumber);

      //Log Time.  Used to stamp lines in the log.
      DWORD                      getLogTime( void ) const { return(mLogTime); }
      void                       setLogTime( DWORD v ) { mLogTime=v; }

   protected:
      bool                       doLog(long headerID, bool appendCRLF = true);
      void                       writeHeaderStamp(BLogFile* pLogFile);

      BLogHeader                 mHeaders[cMaxLogHeaders];

      char                       mArgStr[cMaxLogTextLength];

      BLogFile                   mLogFiles[cMaxLogFiles];

      long                       mCurrentHeaderID;
      DWORD                      mCurrentErrorNumber;

      const char*                mSourceFile;
      long                       mSourceLine;
      DWORD                      mStartupTime;
      long                       mBaseDirectoryID;
      
      DWORD                      mLogTime;

      bool                       mUseTrace : 1;
      bool                       mAlwaysTrace : 1;
      bool                       mAlwaysPrintf : 1;
      bool                       mDestructed : 1;
      bool                       mCacheEnabled : 1;

}; // BLogManager

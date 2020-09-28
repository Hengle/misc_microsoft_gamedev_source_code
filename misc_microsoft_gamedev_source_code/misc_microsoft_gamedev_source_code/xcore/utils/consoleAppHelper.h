//------------------------------------------------------------------------------------------------------------------------
//
//  File: consoleAppHelper.h
//
//  Copyright (c) 2005-2007, Ensemble Studios
//
//------------------------------------------------------------------------------------------------------------------------
#pragma once

#include "commandLineParser.h"
#include "consoleOutput.h"
#include "threading\lightWeightMutex.h"

class BConsoleAppHelper
{
public:
   static void                   setup(void);
         
   static bool                   init(BCommandLineParser::BStringArray& args, int argc, const char *argv[]);
         
   static void                   deinit(void);
   
   static void                   printHelp(void);
   
   static bool                   openLogFile(const char* pFilename, bool forceAppend = false);
   static void                   closeLogFile(void);
   static bool                   openErrorLogFile(const char* pFilename, bool forceAppend = false);
   static void                   closeErrorLogFile(void);
   static FILE*                  getLogFile(void) { return mpLogFile; }
   static FILE*                  getErrorLogFile(void) { return mpErrorLogFile; }
      
   static void                   pause(void);
      
   static bool                   getQuiet(void) { return mQuietFlag; }
   static uint                   getTotalErrorMessages(void) { return mTotalErrorMessages; }
   static uint                   getTotalWarningMessages(void) { return mTotalWarningMessages; }
   
   static bool                   getErrorMessageBox(void) { return mErrorMessageBox; }
   
   static HANDLE                 getConOutHandle(void) { return mConOutHandle; }
   static bool                   getIsSTDInRedirected(void) { return mIsSTDInRedirected; }
   static bool                   getIsSTDOutRedirected(void) { return mIsSTDOutRedirected; }
   static bool                   getIsSTDErrRedirected(void) { return mIsSTDErrRedirected; }
   
   // D3D SW reference device
   static bool                   initD3D(void);
   static void                   deinitD3D(void);
   static LPDIRECT3D9            getD3D(void) { return mpD3D; }
   static LPDIRECT3DDEVICE9      getD3DDevice(void) { return mpD3DDevice; }
   static HWND                   getHWnd(void) { return mhWnd; }
   
   static bool                   checkOutputFileAttribs(const char* pDstFilename, bool P4CheckOut);
               
private:
   static bool                   mQuietFlag;
   static BString                mLogFilename;
   static BString                mErrorLogFilename;
   static bool                   mErrorMessageBox;
   static FILE*                  mpLogFile;
   static FILE*                  mpErrorLogFile;
   static uint                   mTotalErrorMessages;
   static uint                   mTotalWarningMessages;
   static bool                   mPauseOnWarnings;
   static bool                   mPauseFlag;
   static bool                   mAppend;
   
   static HWND                   mhWnd;           
   static HINSTANCE              mhInstance;        
   static LPDIRECT3D9            mpD3D;          
   static LPDIRECT3DDEVICE9      mpD3DDevice;
   static HMODULE                mD3D9DLL;
   
   static BNonRecursiveSpinlock  mConsolePrintMutex;

   static HANDLE                 mConOutHandle;
   static bool                   mIsSTDInRedirected;
   static bool                   mIsSTDOutRedirected;
   static bool                   mIsSTDErrRedirected;
   
   static void consoleOutputFunc(void* data, BConsoleMessageCategory category, const char* pMessage);
   static bool createD3DREF(void);
   static void destroyD3DREF(void);
   static void determineRedirection(void);
};  

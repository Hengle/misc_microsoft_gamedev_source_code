//============================================================================
// xsystemlib.h
//  
// Copyright (c) 2005 Ensemble Studios
//============================================================================
#pragma once

#include "xexception/xexception.h"
#include "xfs.h"

// Forward declarations
class IConsoleInterface;

// EcoreLibInfo
class XSystemInfo
{
   public:
      XSystemInfo() :
         mUseXFS(false),
         mXFSCopy(false),
         mXFSConnectionPort(1000),
         mXFSMessageCallback(NULL),
         mProductionDir(),
         mStartupDir(),
         mUserDir(),
         mPreAssertCallback(NULL),
         mPreAssertParam1(NULL),
         mPreAssertParam2(NULL),
         mPostAssertCallback(NULL),
         mPostAssertParam1(NULL),
         mPostAssertParam2(NULL),
         mpConsoleInterface(NULL),
         mExceptionCallback( NULL )
      {
      }
            
      bool                          mUseXFS;
      bool                          mXFSCopy;
      WORD                          mXFSConnectionPort;
      BXFS::BMessageCallbackPtr     mXFSMessageCallback;

      BSimString                    mProductionDir;
      BSimString                    mStartupDir;
      BSimString                    mUserDir;
      BString                       mLocEraFile;

      BASSERT_CALLBACK_FUNC         mPreAssertCallback;
      void*                         mPreAssertParam1;
      void*                         mPreAssertParam2;

      BASSERT_CALLBACK_FUNC         mPostAssertCallback;
      void*                         mPostAssertParam1;
      void*                         mPostAssertParam2;

      IConsoleInterface*            mpConsoleInterface;

      XEXCEPTION_CALLBACK_FUNC      mExceptionCallback;
};

// Functions
bool XSystemCreate(const XSystemInfo* pInfo, BHandle* pRootFileCacheHandle = NULL);
void XSystemRelease();

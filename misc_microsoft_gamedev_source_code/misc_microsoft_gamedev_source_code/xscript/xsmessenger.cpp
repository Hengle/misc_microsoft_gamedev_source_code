//==============================================================================
// BXSMessenger.cpp
//
// Copyright (c) 1999-2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xscript.h"
#include "xsmessenger.h"

//==============================================================================
// Defines


//==============================================================================
// BXSMessenger::BXSMessenger
//==============================================================================
BXSMessenger::BXSMessenger(void) :
   mErrorMsgFun(NULL),
   mWarnMsgFun(NULL),
   mInfoMsgFun(NULL),
   mRunMsgFun(NULL),
   mDebuggerMsgFun(NULL),
   mErrorCount(0),
   mWarningCount(0),
   mWarn(true),
   mInfo(false),
   mRun(true),
   mListInterpreter(false),
   mListFunctionEntry(false),
   mDebuggerInterpreter(false)
{
}

//==============================================================================
// BXSMessenger::~BXSMessenger
//==============================================================================
BXSMessenger::~BXSMessenger(void)
{
}

//==============================================================================
// BXSMessenger::errorMsg
//==============================================================================
void BXSMessenger::errorMsg(const char *lpszFormat, ...)
{
   if (mErrorMsgFun == NULL)
      return;

   //Process the message.
   va_list args;
   va_start(args, lpszFormat);

   char szTemp[1024];
   bvsnprintf(szTemp, sizeof(szTemp), lpszFormat, args);

   char szBuffer[1024];
   long nBuf=bsnprintf(szBuffer, sizeof(szBuffer), "XS: %s", szTemp);

   BASSERT(((nBuf + 1) < 1024) && ((nBuf - 1) >= 0));
   if ((szBuffer[nBuf-1] != '\n')) 
   {
      szBuffer[nBuf] = '\n';
      szBuffer[nBuf+1] = 0;
   }
   va_end(args);

   //Send it on its way.
   mErrorMsgFun(szBuffer);

   //Bump the counter.
   mErrorCount++;
}

//==============================================================================
// BXSMessenger::sourceErrorMsg
//==============================================================================
void BXSMessenger::sourceErrorMsg(const char *filename, long lineNumber, const char *lpszFormat, ...)
{
   if (mErrorMsgFun == NULL)
      return;

   //Process the message.
   va_list args;
   va_start(args, lpszFormat);

   char szTemp[1024];
   char szBuffer[1024];
   bvsnprintf(szTemp, sizeof(szTemp), lpszFormat, args);

   long nBuf;
   if (filename && (lineNumber > 0))
   {
      //#ifdef _BANG
      //nBuf = bsnprintf(szBuffer, sizeof(szBuffer), "%s%s(%d) : XS: %s", gCore->getWorkingDirectory().asANSI(), filename, lineNumber, szTemp);
      //#else
      nBuf = bsnprintf(szBuffer, sizeof(szBuffer), "%s(%d) : XS: %s", filename, lineNumber, szTemp);
      //#endif
   }
   else if (lineNumber > 0)
      nBuf = bsnprintf(szBuffer, sizeof(szBuffer), "(%d) : XS: %s", lineNumber, szTemp);
   else 
   {
      StringCchCopyA(szBuffer, 1024, szTemp);
      nBuf = strlen(szBuffer);
   }

   BASSERT(((nBuf + 1) < 1024) && ((nBuf - 1) >= 0));
   if ((szBuffer[nBuf-1] != '\n')) 
   {
      szBuffer[nBuf] = '\n';
      szBuffer[nBuf+1] = 0;
   }
   va_end(args);

   //Send it on its way.
   mErrorMsgFun(szBuffer);

   //Bump the counter.
   mErrorCount++;
}

//==============================================================================
// BXSMessenger::warningMsg
//==============================================================================
void BXSMessenger::warningMsg(const char *lpszFormat, ...)
{
   if ((mWarn == false) || (mWarnMsgFun == NULL))
      return;

   //Process the message.
   va_list args;
   va_start(args, lpszFormat);

   char szTemp[1024];
   bvsnprintf(szTemp, sizeof(szTemp), lpszFormat, args);

   char szBuffer[1024];
   long nBuf=bsnprintf(szBuffer, sizeof(szBuffer), "XS: %s", szTemp);

   BASSERT(((nBuf + 1) < 1024) && ((nBuf - 1) >= 0));
   if ((szBuffer[nBuf-1] != '\n')) 
   {
      szBuffer[nBuf] = '\n';
      szBuffer[nBuf+1] = 0;
   }
   va_end(args);

   //Send it on its way.
   mWarnMsgFun(szBuffer);

   //Bump the counter.
   mWarningCount++;
}

//==============================================================================
// BXSMessenger::sourceWarningMsg
//==============================================================================
void BXSMessenger::sourceWarningMsg(const char *filename, long lineNumber, const char *lpszFormat, ...)
{
   if ((mWarn == false) || (mWarnMsgFun == NULL))
      return;

   //Process the message.
   va_list args;
   va_start(args, lpszFormat);

   char szTemp[1024];
   char szBuffer[1024];
   bvsnprintf(szTemp, sizeof(szTemp), lpszFormat, args);

   long nBuf;
   if (filename && (lineNumber > 0))
      nBuf = bsnprintf(szBuffer, sizeof(szBuffer), "%s(%d) : XS: %s", filename , lineNumber, szTemp);
   else if (lineNumber > 0)
      nBuf = bsnprintf(szBuffer, sizeof(szBuffer), "(%d) : XS: %s", lineNumber, szTemp);
   else 
   {
      StringCchCopyA(szBuffer, 1024, szTemp);
      nBuf = strlen(szBuffer);
   }

   BASSERT(((nBuf + 1) < 1024) && ((nBuf - 1) >= 0));
   if ((szBuffer[nBuf-1] != '\n')) 
   {
      szBuffer[nBuf] = '\n';
      szBuffer[nBuf+1] = 0;
   }
   va_end(args);

   //Send it on its way.
   mWarnMsgFun(szBuffer);

   //Bump the counter.
   mWarningCount++;
}

//==============================================================================
// BXSMessenger::infoMsg
//==============================================================================
void BXSMessenger::infoMsg(const char *lpszFormat, ...)
{
   if ((mInfo == false) || (mInfoMsgFun == NULL))
      return;

   //Process the message.
   va_list args;
   va_start(args, lpszFormat);

   char szTemp[1024];
   bvsnprintf(szTemp, sizeof(szTemp), lpszFormat, args);

   char szBuffer[1024];
   long nBuf=bsnprintf(szBuffer, sizeof(szBuffer), "XS: %s", szTemp);

   BASSERT(((nBuf + 1) < 1024) && ((nBuf - 1) >= 0));
   if ((szBuffer[nBuf-1] != '\n')) 
   {
      szBuffer[nBuf] = '\n';
      szBuffer[nBuf+1] = 0;
   }
   va_end(args);

   //Send it on its way.
   mInfoMsgFun(szBuffer);
}

//==============================================================================
// BXSMessenger::sourceInfoMsg
//==============================================================================
void BXSMessenger::sourceInfoMsg(const char *filename, long lineNumber, const char *lpszFormat, ...)
{
   if ((mInfo == false) || (mInfoMsgFun == NULL))
      return;

   //Process the message.
   va_list args;
   va_start(args, lpszFormat);

   char szTemp[1024];
   char szBuffer[1024];
   bvsnprintf(szTemp, sizeof(szTemp), lpszFormat, args);

   long nBuf;
   if (filename && (lineNumber > 0))
      nBuf = bsnprintf(szBuffer, sizeof(szBuffer), "%s(%d) : XS: %s", filename , lineNumber, szTemp);
   else if (lineNumber > 0)
      nBuf = bsnprintf(szBuffer, sizeof(szBuffer), "(%d) : XS: %s", lineNumber, szTemp);
   else 
   {
      StringCchCopyA(szBuffer, 1024, szTemp);
      nBuf = strlen(szBuffer);
   }

   BASSERT(((nBuf + 1) < 1024) && ((nBuf - 1) >= 0));
   if ((szBuffer[nBuf-1] != '\n')) 
   {
      szBuffer[nBuf] = '\n';
      szBuffer[nBuf+1] = 0;
   }
   va_end(args);

   //Send it on its way.
   mInfoMsgFun(szBuffer);
}

//==============================================================================
// BXSMessenger::runMsg
//==============================================================================
void BXSMessenger::runMsg(const char *lpszFormat, ...)
{
   if ((mRun == false) || (mRunMsgFun == NULL))
      return;

   //Process the message.
   va_list args;
   va_start(args, lpszFormat);

   char szTemp[1024];
   bvsnprintf(szTemp, sizeof(szTemp), lpszFormat, args);

   char szBuffer[1024];
   long nBuf=bsnprintf(szBuffer, sizeof(szBuffer), "XS: %s", szTemp);

   BASSERT(((nBuf + 1) < 1024) && ((nBuf - 1) >= 0));
   if ((szBuffer[nBuf-1] != '\n')) 
   {
      szBuffer[nBuf] = '\n';
      szBuffer[nBuf+1] = 0;
   }
   va_end(args);

   //Send it on its way.
   mRunMsgFun(szBuffer);
}

//==============================================================================
// BXSMessenger::debuggerMsg
//==============================================================================
void BXSMessenger::debuggerMsg(const char *lpszFormat, ...)
{
   if (mDebuggerMsgFun == NULL)
      return;

   //Process the message.
   va_list args;
   va_start(args, lpszFormat);

   char szTemp[1024];
   bvsnprintf(szTemp, sizeof(szTemp), lpszFormat, args);

   char szBuffer[1024];
   long nBuf=bsnprintf(szBuffer, sizeof(szBuffer), "XS: %s", szTemp);

   BASSERT(((nBuf + 1) < 1024) && ((nBuf - 1) >= 0));
   if ((szBuffer[nBuf-1] != '\n')) 
   {
      szBuffer[nBuf] = '\n';
      szBuffer[nBuf+1] = 0;
   }
   va_end(args);

   //Send it on its way.
   mDebuggerMsgFun(szBuffer);
}

//==============================================================================
// BXSMessenger::msg
//==============================================================================
void BXSMessenger::msg(const char *lpszFormat, ...)
{
   if (mInfoMsgFun == NULL)
      return;

   //Process the message.
   va_list args;
   va_start(args, lpszFormat);

   char szTemp[1024];
   bvsnprintf(szTemp, sizeof(szTemp), lpszFormat, args);

   char szBuffer[1024];
   long nBuf=bsnprintf(szBuffer, sizeof(szBuffer), "XS: %s", szTemp);

   BASSERT(((nBuf + 1) < 1024) && ((nBuf - 1) >= 0));
   if ((szBuffer[nBuf-1] != '\n')) 
   {
      szBuffer[nBuf] = '\n';
      szBuffer[nBuf+1] = 0;
   }
   va_end(args);

   //Send it on its way.
   mInfoMsgFun(szBuffer);
}

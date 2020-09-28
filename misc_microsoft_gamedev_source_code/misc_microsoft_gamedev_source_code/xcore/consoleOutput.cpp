//==============================================================================
// File: consoleOutput.cpp
//
// Copyright (c) 2006, Ensemble Studios
//==============================================================================
#include "xcore.h"
#include "consoleOutput.h"

//==============================================================================
// Globals
//==============================================================================
BConsoleOutput gConsoleOutput;

static const uint cMaxTextBufSize = 8192;

//==============================================================================
// BConsoleOutput::BConsoleOutput
//==============================================================================
BConsoleOutput::BConsoleOutput() : 
   mpOutput(NULL),
   mOutputData(NULL),
   mDefaultCategory(cMsgStatus)
{

}

//==============================================================================
// BConsoleOutput::~BConsoleOutput
//==============================================================================
BConsoleOutput::~BConsoleOutput()
{

}

//==============================================================================
// BConsoleOutput::init
//==============================================================================
void BConsoleOutput::init(BConsoleOutputFuncPtr p, void* data)
{
   mpOutput = p;
   mOutputData = data;
}

//==============================================================================
// BConsoleOutput::deinit
//==============================================================================
void BConsoleOutput::deinit(void)
{
   mpOutput = NULL;
}

//==============================================================================
// BConsoleOutput::output
//==============================================================================
void BConsoleOutput::output(BConsoleMessageCategory category, const char* pMsg, ...)
{
   BDEBUG_ASSERT(pMsg);

   if (!mpOutput)
      return;
      
   if ((category < 0) || (category >= cMsgMax))      
      category = cMsgError;

   // Yes I know this is big but if we can't alloc 8k from the stack here the game is hosed anyway.
   char buf[cMaxTextBufSize];
   va_list arglist;
   va_start( arglist, pMsg );
   StringCchVPrintf(buf, sizeof(buf), pMsg, arglist);
   va_end(arglist);
   
   (*mpOutput)(mOutputData, category, buf);
}

//==============================================================================
// BConsoleOutput::warning
//==============================================================================
void BConsoleOutput::warning(const char* pMsg, ...)
{
   va_list args;
   va_start(args, pMsg);
   char buf[cMaxTextBufSize];
   StringCchVPrintfA(buf, sizeof(buf), pMsg, args);
   va_end(args);

   output(cMsgWarning, "%s", buf);
}

//==============================================================================
// BConsoleOutput::error
//==============================================================================
void BConsoleOutput::error(const char* pMsg, ...)
{
   va_list args;
   va_start(args, pMsg);
   char buf[cMaxTextBufSize];
   StringCchVPrintfA(buf, sizeof(buf), pMsg, args);
   va_end(args);

   output(cMsgError, "%s", buf);
}

//==============================================================================
// BConsoleOutput::debug
//==============================================================================
void BConsoleOutput::debug(const char* pMsg, ...)
{
   va_list args;
   va_start(args, pMsg);
   char buf[cMaxTextBufSize];
   StringCchVPrintfA(buf, sizeof(buf), pMsg, args);
   va_end(args);

   output(cMsgDebug, "%s", buf);
}

//==============================================================================
// BConsoleOutput::status
//==============================================================================
void BConsoleOutput::status(const char* pMsg, ...)
{
   va_list args;
   va_start(args, pMsg);
   char buf[cMaxTextBufSize];
   StringCchVPrintfA(buf, sizeof(buf), pMsg, args);
   va_end(args);

   output(cMsgStatus, "%s", buf);
}

//==============================================================================
// BConsoleOutput::resource
//==============================================================================
void BConsoleOutput::resource(const char* pMsg, ...)
{
   va_list args;
   va_start(args, pMsg);
   char buf[cMaxTextBufSize];
   StringCchVPrintfA(buf, sizeof(buf), pMsg, args);
   va_end(args);

   output(cMsgResource, "%s", buf);
}

//==============================================================================
// BConsoleOutput::fileManager
//==============================================================================
void BConsoleOutput::fileManager(const char* pMsg, ...)
{
   va_list args;
   va_start(args, pMsg);
   char buf[cMaxTextBufSize];
   StringCchVPrintfA(buf, sizeof(buf), pMsg, args);
   va_end(args);

   output(cMsgFileManager, "%s", buf);
}

//==============================================================================
// BConsoleOutput::printf
//==============================================================================
void BConsoleOutput::printf(const char* pMsg, ...)
{
   va_list args;
   va_start(args, pMsg);
   char buf[cMaxTextBufSize];
   StringCchVPrintfA(buf, sizeof(buf), pMsg, args);
   va_end(args);

   output(mDefaultCategory, "%s", buf);
}

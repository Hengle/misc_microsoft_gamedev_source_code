//==============================================================================
//
// File: consoleOutput.h
//
// Copyright (c) 2006, Ensemble Studios
//
//==============================================================================
#pragma once 

enum BConsoleMessageCategory
{
   cMsgConsole,
   cMsgDebug,
   cMsgStatus,
   cMsgWarning,
   cMsgError,
   cMsgResource,
   cMsgFileManager,
   
   cMsgMax,
};

typedef void (*BConsoleOutputFuncPtr)(void* data, BConsoleMessageCategory category, const char* pMessage);

//==============================================================================
// class BConsoleOutput
//==============================================================================
class BConsoleOutput
{
public:
   BConsoleOutput();
   ~BConsoleOutput();

   void init(BConsoleOutputFuncPtr p, void* data);
   void deinit(void);

   void output(BConsoleMessageCategory category, const char* pMsg, ...);
   
   // printf() uses the default message category.
   void printf(const char* pMsg, ...);   
   void warning(const char* pMsg, ...);
   void error(const char* pMsg, ...);
   void debug(const char* pMsg, ...);
   void status(const char* pMsg, ...);
   void resource(const char* pMsg, ...);
   void fileManager(const char* pMsg, ...);
      
   void setDefaultCategory(BConsoleMessageCategory category) { mDefaultCategory = category; }
   BConsoleMessageCategory getDefaultCategory(void) const { return mDefaultCategory; }
   
private:
   BConsoleOutputFuncPtr mpOutput;
   void* mOutputData;
   BConsoleMessageCategory mDefaultCategory;
};

// Thread-safe, system-wide console output object.
extern BConsoleOutput gConsoleOutput;

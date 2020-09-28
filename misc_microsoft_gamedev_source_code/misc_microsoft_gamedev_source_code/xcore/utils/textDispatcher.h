//==============================================================================
//
// File: textDispatcher.h
//
// Copyright (c) 2003-2006, Ensemble Studios
//
//==============================================================================
#pragma once
#include "consoleOutput.h"

enum eTextCategory
{
   cTCInfo,
   cTCStatus,
   cTCWarning,
   cTCError,
   
   cTCMax
};

class BTextDispatcher
{
public:
   BTextDispatcher() :
      mIndent(0),
      mIndentSize(2)
   {
   }
   
   virtual ~BTextDispatcher()
   {
   }
   
   virtual void setIndentSize(uint size) { mIndentSize = size; }
   virtual uint getIndentSize(void) const { return mIndentSize; }
   
   virtual void vprintf(eTextCategory level, bool printLevelDesc, const char* pMsg, va_list argptr) = 0;

   virtual void printf(const char* pMsg, ...)
   {
      va_list args;
      va_start(args, pMsg);
      vprintf(cTCInfo, true, pMsg, args);
      va_end(args);
   }
   
   virtual void printf(eTextCategory level, const char* pMsg, ...)
   {
      va_list args;
      va_start(args, pMsg);
      vprintf(level, true, pMsg, args);
      va_end(args);
   }
   
   virtual void status(const char* pMsg, ...)
   {
      va_list args;
      va_start(args, pMsg);
      vprintf(cTCStatus, true, pMsg, args);
      va_end(args);
   }
   
   virtual void warning(const char* pMsg, ...)
   {
      va_list args;
      va_start(args, pMsg);
      vprintf(cTCWarning, true, pMsg, args);
      va_end(args);
   }
   
   virtual void error(const char* pMsg, ...)
   {
      va_list args;
      va_start(args, pMsg);
      vprintf(cTCError, true, pMsg, args);
      va_end(args);
   }
   
   virtual void printf(bool printLevelDesc, eTextCategory level, const char* pMsg, ...)
   {
      va_list args;
      va_start(args, pMsg);
      vprintf(level, printLevelDesc, pMsg, args);
      va_end(args);
   }
   
   virtual void indent(int i)
   {
      mIndent += i;
      if (mIndent < 0)
         mIndent = 0;
      else if (mIndent > 30)
         mIndent = 30;
   }
                     
protected:
   int mIndent;
   uint mIndentSize;
}; // class BTextDispatcher

class BTraceTextDispatcher : public BTextDispatcher
{
public:
   // DO NOT reduce this - if we can't afford 8KB on the stack, we're screwed anyway.
   enum { cLineBufferSize = 8192 };
     
   BTraceTextDispatcher() :
      BTextDispatcher()
   {
   }
   
   virtual ~BTraceTextDispatcher()
   {
   }
   
   virtual void vprintf(eTextCategory level, bool printLevelDesc, const char* pMsg, va_list argptr)
   {
      level;
      printLevelDesc;
      
      if (mIndent*mIndentSize >= cLineBufferSize)
         return;
      
      char buf[cLineBufferSize];
      memset(buf, ' ', debugRangeCheck(mIndent*mIndentSize, sizeof(buf)));
      
      _vsnprintf_s(buf + mIndent*mIndentSize, sizeof(buf) - mIndent*mIndentSize, _TRUNCATE, pMsg, argptr);
   
      OutputDebugStringA(buf);
   }
}; // class BTraceTextDispatcher

class BFILETextDispatcher : public BTextDispatcher
{
   BFILETextDispatcher(const BFILETextDispatcher&);
   BFILETextDispatcher& operator= (const BFILETextDispatcher&);
   
public:
   // DO NOT reduce this - if we can't afford 8KB on the stack, we're screwed anyway.
   enum { cLineBufferSize = 8192 };
   
   BFILETextDispatcher() :
      BTextDispatcher(),
      mpFile(NULL),
      mAutoClose(true)
   {
   }      

   BFILETextDispatcher(const char* pFilename) :
      BTextDispatcher(),
      mpFile(NULL),
      mAutoClose(true)
   {
      open(pFilename);
   }
   
   BFILETextDispatcher(FILE* pFile, bool autoClose = false) :
      BTextDispatcher(),
      mpFile(NULL),
      mAutoClose(true)
   {
      open(pFile, autoClose);
   }

   virtual ~BFILETextDispatcher()
   {
      close();
   }
   
   bool opened(void)
   {
      return NULL != mpFile;
   }
   
   bool open(const char* pFilename)
   {
      close();
      
      mpFile = NULL;
      fopen_s(&mpFile, pFilename, "w");
      mAutoClose = true;

      return NULL != mpFile;
   }
   
   void open(FILE* pFile, bool autoClose = false)
   {
      close();
      
      if (pFile)
      {
         mpFile = pFile;
         mAutoClose = autoClose;
      }
   }
   
   void close(void)
   {
      if ((mAutoClose) && (mpFile))
         fclose(mpFile);
      
      mpFile = NULL;
      mAutoClose = true;
   }
   
   virtual void vprintf(eTextCategory level, bool printLevelDesc, const char* pMsg, va_list argptr)
   {
      if (!mpFile)
         return;
         
      level;
      printLevelDesc;
      
      if (mIndent*mIndentSize >= cLineBufferSize)
         return;
         
      char buf[cLineBufferSize];
      memset(buf, ' ', debugRangeCheck(mIndent*mIndentSize, sizeof(buf)));

      _vsnprintf_s(buf + mIndent*mIndentSize, sizeof(buf) - mIndent*mIndentSize, _TRUNCATE, pMsg, argptr);

      fputs(buf, mpFile);
   }

private:
   FILE* mpFile;
   bool mAutoClose;
      
}; // class BFILETextDispatcher

class BConsoleTextDispatcher : public BTextDispatcher
{
   BConsoleTextDispatcher(const BConsoleTextDispatcher&);
   BConsoleTextDispatcher& operator= (const BConsoleTextDispatcher&);

public:
   // DO NOT reduce this - if we can't afford 8KB on the stack, we're screwed anyway.
   enum { cLineBufferSize = 8192 };

   BConsoleTextDispatcher() :
      BTextDispatcher(),
      mpConsoleOutput(NULL)
   {
   }      

   BConsoleTextDispatcher(BConsoleOutput& consoleOutput) :
      BTextDispatcher(),
      mpConsoleOutput(&consoleOutput)
   {
   }

   virtual ~BConsoleTextDispatcher()
   {
   }

   bool opened(void)
   {
      return NULL != mpConsoleOutput;
   }

   bool open(BConsoleOutput& consoleOutput)
   {
      close();

      mpConsoleOutput = &consoleOutput;
      
      return true;
   }

   void close(void)
   {
      mpConsoleOutput = NULL;
   }

   virtual void vprintf(eTextCategory level, bool printLevelDesc, const char* pMsg, va_list argptr)
   {
      if (!mpConsoleOutput)
         return;

      level;
      printLevelDesc;

      if (mIndent*mIndentSize >= cLineBufferSize)
         return;

      char buf[cLineBufferSize];
      memset(buf, ' ', debugRangeCheck(mIndent*mIndentSize, sizeof(buf)));

      _vsnprintf_s(buf + mIndent*mIndentSize, sizeof(buf) - mIndent*mIndentSize, _TRUNCATE, pMsg, argptr);

      switch (level)
      {
         case cTCInfo:     mpConsoleOutput->printf("%s", buf); break;
         case cTCStatus:   mpConsoleOutput->printf("%s", buf); break;
         case cTCWarning:  mpConsoleOutput->warning("%s", buf); break;
         case cTCError:    mpConsoleOutput->error("%s", buf); break;
      }         
   }

private:
   BConsoleOutput* mpConsoleOutput;

}; // class BConsoleTextDispatcher

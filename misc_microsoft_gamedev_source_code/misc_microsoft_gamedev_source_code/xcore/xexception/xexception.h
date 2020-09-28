//============================================================================
// xexception.h
// Copyright (c) 2003, Blue Shift, Inc.
// Copyright (c) 2005-2007, Ensemble Studios
//============================================================================
#pragma once

//==============================================================================
// Callback function prototype.
//==============================================================================
typedef void (*XEXCEPTION_CALLBACK_FUNC)( const char* cpMsg, BOOL die );

#ifndef XEXCEPTION_H
#define XEXCEPTION_H
struct X_Exception
{
   static void Init();
   
   static long GetHandlingMeltdown() { MemoryBarrier(); return mHandlingMeltdown; }
   
   static void SetCallback( XEXCEPTION_CALLBACK_FUNC callback ) { mCallBackFunc = callback; }
         
   static void DumpFailReport(const char* cpErrorMsg, const char* cpFile);
   
private:

   static bool TestCacheDrive();

   static long WINAPI ExceptionFilter(LPEXCEPTION_POINTERS pe);
   static void ExceptionCallback(const char* cpMsg, BOOL die);
   static void Meltdown(const char* Pmsg, bool die = true);
      
   static XEXCEPTION_CALLBACK_FUNC  mCallBackFunc;
   static volatile long             mHandlingMeltdown;
   static bool                      mUseCacheDrive;
   static bool                      mFirstWrite;
};
#endif
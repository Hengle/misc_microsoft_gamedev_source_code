//==============================================================================
// WindowsTimer.h
//
// Copyright (c) 2001, Ensemble Studios
//==============================================================================

#pragma once 

//==============================================================================
// Includes

//==============================================================================
// Forward declarations

//==============================================================================
class BWindowsTimerHandler
{
public:
   virtual  void onTimerEvent(DWORD timerID) = 0;
};
typedef BDynamicSimArray<BWindowsTimerHandler*> BWindowsTimerHandlerArray;

//==============================================================================
const DWORD cInvalidTimer = 0xffffffff;
class BWindowsTimer
{
public:

   BWindowsTimer() : mEventID(cInvalidTimer), mTimerID(cInvalidTimer),mHandler(NULL) {}

   DWORD                mEventID;
   DWORD                mTimerID;
   BWindowsTimerHandler *mHandler;
};
typedef BDynamicSimArray<BWindowsTimer> BWindowsTimerArray;

//==============================================================================
class BWindowsTimerSystem
{
public:
   BWindowsTimerSystem(HWND windowHandle);
   ~BWindowsTimerSystem();

#ifndef XBOX
   static void CALLBACK onWindowsTimer(HWND hWnd, UINT nMsg, UINT nIDEvent, DWORD dwTime );
#endif   

   void  handleWindowsTimer(DWORD timerID);
   bool  addTimer(BWindowsTimerHandler* pHandler, DWORD timerID, DWORD milliseconds);
   bool  removeTimer(BWindowsTimerHandler* pHandler, DWORD timerID);

protected:
   HWND  mWindowHandle;
   BWindowsTimerArray mTimers;
};

//==============================================================================
extern BWindowsTimerSystem* gWindowsTimerSystem;
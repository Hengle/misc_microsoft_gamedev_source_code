//==============================================================================
// WindowsTimer.cpp
//
// Copyright (c) 2004, Ensemble Studios
//==============================================================================

// Includes
#include "xsystem.h"
#include "WindowsTimer.h"

//==============================================================================
// Defines

//==============================================================================
// BWindowsTimerSystem::gWindowsTimerSystem
//==============================================================================
BWindowsTimerSystem* gWindowsTimerSystem = NULL;

//==============================================================================
// BWindowsTimerSystem::BWindowsTimerSystem
//==============================================================================
BWindowsTimerSystem::BWindowsTimerSystem(HWND windowHandle) : mWindowHandle(windowHandle)
{
#ifndef XBOX
   // We have to have a valid windows handle
   BFATAL_ASSERT(mWindowHandle != INVALID_HANDLE_VALUE);
#endif   
}

//==============================================================================
// BWindowsTimerSystem::~BWindowsTimerSystem
//==============================================================================
BWindowsTimerSystem::~BWindowsTimerSystem()
{
#ifndef XBOX
   long count = mTimers.getNumber();
   for (long idx=0; idx<count; idx++)
      KillTimer(mWindowHandle, mTimers[idx].mEventID);
#endif      
   mTimers.clear();
}

#ifndef XBOX
//==============================================================================
// BWindowsTimerSystem::onWindowsTimer
// NOTE: this is a static function
//==============================================================================
void BWindowsTimerSystem::onWindowsTimer(HWND hWnd, UINT nMsg, UINT nIDEvent, DWORD dwTime )
{
   hWnd; dwTime;

   if (nMsg != WM_TIMER)
      return;
   
   if (gWindowsTimerSystem)
      gWindowsTimerSystem->handleWindowsTimer(nIDEvent);
}
#endif

//==============================================================================
// BWindowsTimerSystem::handleWindowsTimer
//==============================================================================
void  BWindowsTimerSystem::handleWindowsTimer(DWORD timerID)
{
#ifndef XBOX
   // if the timer id is out of range, ignore it
   if (timerID > (DWORD)mTimers.getNumber())
      return;

   // call the timer handler
   if (!mTimers[timerID].mHandler)
      return;

   mTimers[timerID].mHandler->onTimerEvent(mTimers[timerID].mTimerID);
#endif   
}

//==============================================================================
// BWindowsTimerSystem::addTimer
//==============================================================================
bool BWindowsTimerSystem::addTimer(BWindowsTimerHandler* pHandler, DWORD timerID, DWORD milliseconds)
{
#ifdef XBOX
   // rg [6/22/05] - FIXME
   return false;
#else   
   // We have to have a valid windows handle
   if (mWindowHandle == INVALID_HANDLE_VALUE)
      return(false);

   // why would you want to set a timer and not handle it?
   if (!pHandler)
      return(false);

   // look for an open timer
   long index = -1;
   long count = mTimers.getNumber();
   for (long idx=0; idx<count; idx++)
   {
      if (mTimers[idx].mEventID == cInvalidTimer)
      {
         index = idx;
         break;
      }
   }

   // none found, add one
   if (index<0)
   {
      index = count;
      mTimers.setNumber(index+1);
   }

   // attempt to setup the windows timer with our own timer ID (index into the array, duh)
   mTimers[index].mEventID = SetTimer(mWindowHandle, (DWORD)index, milliseconds, BWindowsTimerSystem::onWindowsTimer);
   //trace("Add timer[%x] -- index: %d, timerID: %d, eventID: %d, ms: %d", pHandler, index, timerID, mTimers[index].mEventID, milliseconds);

   // if it failed, bail
   if (mTimers[index].mEventID == 0)
   {
      mTimers[index].mEventID = cInvalidTimer;
      return(false);
   }

   // otherwise remember the handler, and his timer ID
   mTimers[index].mHandler = pHandler;
   mTimers[index].mTimerID = timerID;

   return(true);
#endif   
}

//==============================================================================
// BWindowsTimerSystem::removeTimer
//==============================================================================
bool BWindowsTimerSystem::removeTimer(BWindowsTimerHandler* pHandler, DWORD timerID)
{
#ifndef XBOX
   // We have to have a valid windows handle
   if (mWindowHandle == INVALID_HANDLE_VALUE)
      return(false);

   // search for this handler/timerID combo
   long count = mTimers.getNumber();
   for (long idx=0; idx<count; idx++)
   {
      if ( (mTimers[idx].mHandler == pHandler) && 
           (mTimers[idx].mTimerID == timerID) )
      {
//         trace("remove timer[%x] -- index: %d, timerID: %d, eventID: %d", pHandler, idx, timerID, mTimers[idx].mEventID);

         // found, so kill the windows timer
         KillTimer(mWindowHandle, mTimers[idx].mEventID);
         mTimers[idx].mHandler = NULL;
         mTimers[idx].mEventID = cInvalidTimer;
         mTimers[idx].mTimerID = cInvalidTimer;
         return(true);
      }
   }
#endif
   // boo... you suck!
   return(false);
}

//==============================================================================
//
//==============================================================================

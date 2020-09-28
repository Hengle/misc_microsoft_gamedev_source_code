//============================================================================
//
// File: win32WaitableTimer.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

//============================================================================
// class BWin32WaitableTimer
//============================================================================
class BWin32WaitableTimer
{
public:
   BWin32WaitableTimer(bool manualReset = false, LPCSTR pName = NULL)
   {
       mHandle = CreateWaitableTimer(NULL, manualReset, pName);
   }
   
   ~BWin32WaitableTimer(void)
   {
      CloseHandle(mHandle);
   }
   
   HANDLE getHandle(void) const { return mHandle; }
   
   void set(LONG periodInMilliseconds)
   {
      LARGE_INTEGER dueTime;
      dueTime.QuadPart = 0;//-(periodInMilliseconds * 10000);

      BOOL status = SetWaitableTimer(mHandle, &dueTime, periodInMilliseconds, NULL, NULL, FALSE);
      if (status == 0)
      {
         BFAIL("BWin32WaitableTimer::set: SetWaitableTimer failed");
      }
   }         
      
   void cancel(void)
   {
      CancelWaitableTimer(mHandle);
   }

private:
   HANDLE mHandle;
   
   BWin32WaitableTimer(const BWin32WaitableTimer&);
   BWin32WaitableTimer& operator= (const BWin32WaitableTimer&);
};


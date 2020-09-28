//============================================================================
//
// File: win32Event.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

class BWin32Event
{
public:
   BWin32Event(bool manualReset = false, bool initialState = false, LPCSTR pName = NULL)
   {
      mHandle = CreateEvent(
         NULL,
         manualReset,
         initialState,
         pName);

      if (NULL == mHandle)
      {
         BFAIL("BWin32Event: CreateEvent() failed");
      }
   }
   
   ~BWin32Event()
   {
      if (mHandle != INVALID_HANDLE_VALUE)
      {
         CloseHandle(mHandle);
         mHandle = INVALID_HANDLE_VALUE;
      }
   }
   
   HANDLE getHandle(void) const
   {
      return mHandle;
   }   
   
   operator HANDLE() const 
   { 
      return mHandle; 
   }
   
   void set(void)
   {
      SetEvent(mHandle);
   }
   
   void reset(void)
   {
      ResetEvent(mHandle);
   }
   
   void pulse(void)
   {
      PulseEvent(mHandle);
   }
               
   // true if the wait succeeded
   bool wait(DWORD milliseconds = INFINITE)
   {
      DWORD result = WaitForSingleObject(mHandle, milliseconds);
      
      if (result == WAIT_FAILED)
      {
         BFAIL("BWin32Event: WaitForSingleObject failed");
      }
      
      return (result == WAIT_OBJECT_0);
   }
   
   // Returns -1 if the wait timed out.
   // Returns 0 if the supplied handle became signaled.
   // Returns 1 if the event became signaled.
   int wait2(HANDLE handle, bool waitAll = false, DWORD milliseconds = INFINITE) 
   {
      HANDLE handles[2];
      handles[0] = handle;
      handles[1] = mHandle;

      DWORD result = WaitForMultipleObjects(2, handles, waitAll, milliseconds);
            
      switch (result)
      {
         case WAIT_TIMEOUT:
            return -1;      
         case WAIT_OBJECT_0:
            return 0;
         case WAIT_OBJECT_0 + 1:
            return 1;
         default:
         {
            BFAIL("BWin32Event: WaitForMultipleObjects failed");
         }
      }      
      
      return 0;
   }      
   
protected:
   HANDLE mHandle;

private:
   BWin32Event(const BWin32Event&);
   BWin32Event& operator= (const BWin32Event&);
};

// This object atomically decrements a LONG. When it reaches zero, it sets an auto-reset Win32 event object.
class BCountDownEvent
{
   BCountDownEvent(const BCountDownEvent&);
   BCountDownEvent& operator= (const BCountDownEvent&);
   
public:
   BCountDownEvent() 
   {
      set(0);
   }
   
   void clear(void)
   {
      set(0);
      mEvent.reset();
   }

   LONG set(LONG i)
   {
      return InterlockedExchange(&mValue, i);
   }

   LONG get(void) const { return mValue; }
   
   LONG increment(void)
   {
      return InterlockedIncrement(&mValue);
   }

   LONG decrement(void)
   {
      LONG newValue = InterlockedDecrement(&mValue);
      if (0 == newValue)
         mEvent.set();
      return newValue;         
   }

   BWin32Event& getEvent(void) { return mEvent; }  
   
   operator HANDLE() const { return mEvent.getHandle(); }

private:
   volatile LONG mValue;
   BWin32Event mEvent;
};

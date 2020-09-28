//============================================================================
//
// File: win32Semaphore.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#pragma once

//============================================================================
// class BWin32Semaphore
//============================================================================
class BWin32Semaphore
{
public:
   BWin32Semaphore(LONG initialCount = 0, LONG maximumCount = 1, LPCSTR pName = NULL)
   {
      mHandle = CreateSemaphore(NULL, initialCount, maximumCount, pName);
      if (NULL == mHandle)
      {
         BFAIL("BWin32Semaphore: CreateSemaphore failed");
      }
   }
   
   ~BWin32Semaphore()
   {
      if (mHandle)
         CloseHandle(mHandle);
   }

   HANDLE getHandle(void) const
   {
      return mHandle;
   }   
   
   void release(LONG releaseCount = 1, LPLONG pPreviousCount = NULL)
   {
      if (0 == ReleaseSemaphore(mHandle, releaseCount, pPreviousCount))
      {
         BFAIL("BWin32Semaphore: release failed");
      }
   }
   
   bool wait(DWORD milliseconds = INFINITE) 
   {
      DWORD result = WaitForSingleObject(mHandle, milliseconds);

      if (result == WAIT_FAILED)
      {
         BFAIL("BWin32Semaphore: WaitForSingleObject failed");
      }

      return (result == WAIT_OBJECT_0);
   }      
   
   // Returns -1 if the wait timed out.
   // Returns 0 if the supplied handle became signaled.
   // Returns 1 if the semaphore became signaled.
   int wait2(HANDLE handle, bool waitAll = false, DWORD milliseconds = INFINITE) 
   {
      HANDLE handles[2];
      handles[0] = handle;
      handles[1] = mHandle;
      
      DWORD result = WaitForMultipleObjects(2, handles, waitAll, milliseconds);

#if 0
      if ( (result == WAIT_FAILED) || 
         ((result >= WAIT_ABANDONED_0) && (result <= WAIT_ABANDONED_0 + 1)) )
      {
         BFAIL("BWin32Semaphore: WaitForSingleObject failed");
      }
#endif      
                  
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
            BFAIL("BWin32Semaphore: WaitForSingleObject failed");
         }
      }      
      
      return 0;
   }      
      
protected:   
   HANDLE mHandle;

private:
   BWin32Semaphore(const BWin32Semaphore&);
   BWin32Semaphore& operator= (const BWin32Semaphore&);
};


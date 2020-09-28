// File: readerWriterLock.h
// UNTESTED
// Originally from: http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dndllpro/html/msdn_locktest.asp
#pragma once

#if 0
class BRWLock
{
   HANDLE mReaderEvent;
   HANDLE mMutex;
   CRITICAL_SECTION mWriterMutex;
   volatile LONG mCounter;
   
public:
   BRWLock()
   {
      mReaderEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
      if (!mReaderEvent)
      {
         BFATAL_FAIL("CreateEvent() failed");
      }
      
      mMutex = CreateEvent(NULL, FALSE, TRUE, NULL);
      if (!mMutex)
      {
         BFATAL_FAIL("CreateEvent() failed");
      }  
      
      InitializeCriticalSection(&mWriterMutex);
            
      InterlockedExchange(&mCounter, -1);
   }
   
   ~BRWLock()
   {
      CloseHandle(mReaderEvent);
      CloseHandle(mMutex);
      
      DeleteCriticalSection(&mWriterMutex);
   }
   
   void claimWriter(void)
   {
      EnterCriticalSection(&mWriterMutex);

      WaitForSingleObject(mMutex, INFINITE);
   }
   
   void releaseWriter(void)
   {
      SetEvent(mMutex);

      LeaveCriticalSection(&mWriterMutex);
   }
   
   void claimReader(void)
   {
      if (InterlockedIncrement(&mCounter) == 0)
      { 
         WaitForSingleObject(mMutex, INFINITE);
         SetEvent(mReaderEvent);
      }
      
      WaitForSingleObject(mReaderEvent, INFINITE);
   }
   
   void releaseReader(void)
   {
      if (InterlockedDecrement(&mCounter) < 0)
      { 
         ResetEvent(mReaderEvent);
         SetEvent(mMutex);
      }; 
   }
};
#endif

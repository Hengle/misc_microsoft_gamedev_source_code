// File: multiprocessManager.h
#pragma once

class BMultiprocessManager
{
   BMultiprocessManager(const BMultiprocessManager&);
   BMultiprocessManager& operator= (const BMultiprocessManager&);
   
public:
   typedef bool (*BFinalizeCallbackPtr)(DWORD status, void* pUserPtr, uint64 userData);
   
   BMultiprocessManager(uint maxProcesses = cMaxSupportedProcesses, BFinalizeCallbackPtr pFinalizeCallback = NULL) :
      mMaxProcesses(0)
   {
      setMaxProcesses(maxProcesses);
      setFinalizeCallback(pFinalizeCallback);

      for (uint i = 0; i < cMaxSupportedProcesses; i++)
      {
         mActiveProcesses[i].mHandle = INVALID_HANDLE_VALUE;
         mActiveProcesses[i].mUserData = 0;
         mActiveProcesses[i].mUserPtr = NULL;
      }
   }
   
   void setFinalizeCallback(BFinalizeCallbackPtr pFinalizeCallback)
   {
      mpFinalizeCallback = pFinalizeCallback;
   }
   
   uint getNumActiveProcesses(void) const
   {
      uint total = 0;
      for (uint i = 0; i < mMaxProcesses; i++)
         if (mActiveProcesses[i].mHandle != INVALID_HANDLE_VALUE)
            total++;
      return total;            
   }
   
   void setMaxProcesses(uint maxProcesses)
   {
      if (!getNumActiveProcesses())
         mMaxProcesses = Math::Clamp<int>(maxProcesses, 1, cMaxSupportedProcesses);
   }
   
   uint getMaxProcesses() const
   {
      return mMaxProcesses;
   }

   bool queue(const char* pArgs, void* pUserPtr, uint64 userData)
   {
      BDEBUG_ASSERT(pArgs);
      
      uint processIndex;
      
      for ( ; ; )
      {
         for (processIndex = 0; processIndex < mMaxProcesses; processIndex++)
            if (mActiveProcesses[processIndex].mHandle == INVALID_HANDLE_VALUE)
               break;
         
         if (processIndex < mMaxProcesses)
            break;

         HANDLE handles[cMaxSupportedProcesses];
         for (uint i = 0; i < mMaxProcesses; i++)
            handles[i] = mActiveProcesses[i].mHandle;
         
         uint status = WaitForMultipleObjects(mMaxProcesses, handles, FALSE, INFINITE);
         if ((status >= WAIT_OBJECT_0) && (status <= (WAIT_OBJECT_0 + mMaxProcesses - 1)))
         {
            processIndex = status - WAIT_OBJECT_0;
            if (!finalizeProcess(processIndex))
               return false;
               
            break;
         }
         else 
         {
            return false;
         }
      }
      
      BActiveProcess& activeProcess = mActiveProcesses[processIndex];
      activeProcess.mArgs.set(pArgs);
      activeProcess.mUserData = userData;
      activeProcess.mUserPtr = pUserPtr;
      
      BSpawnCommand spawn(pArgs);
      activeProcess.mHandle = (HANDLE)spawn.run(_P_NOWAIT);
      if (activeProcess.mHandle == (HANDLE)-1)
      {
         activeProcess.mHandle = INVALID_HANDLE_VALUE;
         return false;
      }
      
      return true;
   }
         
   bool sync(bool tickOnce = false)
   {
      for ( ; ; )
      {
         uint numHandles = 0;
         HANDLE handles[cMaxSupportedProcesses];
         uint processIndex[cMaxSupportedProcesses];
                     
         for (uint i = 0; i < mMaxProcesses; i++)
         {
            if (mActiveProcesses[i].mHandle != INVALID_HANDLE_VALUE)
            {
               handles[numHandles] = mActiveProcesses[i].mHandle;
               processIndex[numHandles] = i;
               numHandles++;  
            }
         }

         if (!numHandles)
            break;

         uint status = WaitForMultipleObjects(numHandles, handles, FALSE, tickOnce ? 0 : INFINITE);
         
         if (status == WAIT_TIMEOUT)
            break;
         else if ((status >= WAIT_OBJECT_0) && (status <= (WAIT_OBJECT_0 + numHandles - 1)))
         {
            if (!finalizeProcess(processIndex[status - WAIT_OBJECT_0]))
               return false;
         }
         else if ((status >= WAIT_ABANDONED_0) && (status <= (WAIT_ABANDONED_0 + numHandles - 1)))
         {
            return false;
         }
         else if (status == WAIT_FAILED)
            return false;
      }
      
      return true;
   }   
               
   void cancelAll(void)
   {
      sync(true);
      
      for (uint i = 0; i < mMaxProcesses; i++)
      {
         if (mActiveProcesses[i].mHandle != INVALID_HANDLE_VALUE)
         {
            TerminateProcess(mActiveProcesses[i].mHandle, 2);
            
            finalizeProcess(i);
         }
      }
   }
   
   bool getHandles(uint& numHandles, HANDLE* pHandles, uint maxHandles)
   {
      BDEBUG_ASSERT(pHandles);

      numHandles = 0;

      for (uint i = 0; i < mMaxProcesses; i++)
      {
         if (mActiveProcesses[i].mHandle != INVALID_HANDLE_VALUE)
         {
            if (numHandles == maxHandles)
               return false;

            pHandles[numHandles] = mActiveProcesses[i].mHandle;
            numHandles++;  
         }
      }

      return true;
   }
   
private:
   enum { cMaxSupportedProcesses = 8 };

   struct BActiveProcess
   {
      uint64   mUserData;
      void*    mUserPtr;
      BString  mArgs;
      HANDLE   mHandle;
   };

   BActiveProcess mActiveProcesses[cMaxSupportedProcesses];
   uint mMaxProcesses;
   
   BFinalizeCallbackPtr mpFinalizeCallback;
   
   bool finalizeProcess(uint processIndex)
   {
      BActiveProcess& activeProcess = mActiveProcesses[processIndex];
      
      BDEBUG_ASSERT(activeProcess.mHandle != INVALID_HANDLE_VALUE);
      
      bool status = true;
      
      if (mpFinalizeCallback)
      {
         DWORD exitCode = 0;
         GetExitCodeProcess(activeProcess.mHandle, &exitCode);
         
         status = mpFinalizeCallback(exitCode, activeProcess.mUserPtr, activeProcess.mUserData);
      }
      
      CloseHandle(activeProcess.mHandle);
      
      activeProcess.mHandle = INVALID_HANDLE_VALUE;
      activeProcess.mArgs.empty();
      activeProcess.mUserData = 0;
      activeProcess.mUserPtr = 0;
      
      return status;
   }
};
//==============================================================================
// AsyncTaskManager.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#pragma once

#ifndef __ASYNCTASKMANAGER_H__
#define __ASYNCTASKMANAGER_H__

class BAsyncTaskManager;

// Global variable for the one AsyncTaskManager object
extern BAsyncTaskManager gAsyncTaskManager;

//==============================================================================
// class BAsyncNotify
//==============================================================================
class BAsyncNotify
{
public:
   virtual void notify(DWORD eventID, void* pTask) = 0;
};

//==============================================================================
// class BAsyncTask
//==============================================================================
class BAsyncTask
{
   public:

      enum eState
      {
         cStateIdle=0,
         cStateError,
         cStateDone,
         cTotalStates
      };

      enum eError                                        
      {
         cErrorSuccess=0,
         cErrorAPIFailure,
         cErrorTimedout,
         cErrorCancelled,
         cTotalErrors
      };

                        BAsyncTask();
      virtual           ~BAsyncTask();

      void              setNotify(BAsyncNotify* pNotify, DWORD eventID=0) { mpNotify = pNotify; mEventID=eventID; }

      virtual BOOL      isComplete();
      DWORD             getResult(LPDWORD pdwResult=NULL);
      DWORD             getExtendedError();

      bool              hasTimedOut() const { return mTimedOut; }

      const eState      getState() const { return mState; }
      const eError      getError() const { return mError; }

      virtual bool      update();
      virtual void      processResult();

   protected:

      bool              updateTimeout();

      void              setState(eState state, eError error=cErrorSuccess);
      void              setRetries(uint maxRetries, DWORD interval=0);
      void              setRetryInterval(DWORD interval);

#ifdef XBOX
      XOVERLAPPED       mOverlapped;
      PXOVERLAPPED      mpOverlapped;
#endif

      BAsyncNotify*     mpNotify;

      DWORD             mEventID;

      DWORD             mStartTime;
      DWORD             mTimeoutLength;

      DWORD             mLastTime;
      DWORD             mRetryTime;
      DWORD             mRetryInterval;
      uint              mNumRetries;
      uint              mMaxRetries;

      eState            mState;
      eError            mError;

      bool              mCheckTimeout : 1;
      bool              mTimedOut : 1;
};

//==============================================================================
// class AsyncTaskManager
//==============================================================================
class BAsyncTaskManager
{
   public:
               BAsyncTaskManager();
               ~BAsyncTaskManager();

      void     update();                     // Update the async events
      void     addTask(BAsyncTask* pTask);

   private:

      BDynamicArray<BAsyncTask*> mTasks;
};

#endif // __ASYNCTASKMANAGER_H__

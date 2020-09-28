//==============================================================================
// AsyncTaskManager.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

//============================================================================
//  INCLUDES
//============================================================================
#include "xsystem.h"
#include "AsyncTaskManager.h"

// Globals
BAsyncTaskManager gAsyncTaskManager;

//==============================================================================
// BAsyncTask::BAsyncTask
//==============================================================================
BAsyncTask::BAsyncTask() :
   mpNotify(NULL),
   mEventID(0),
   mStartTime(0),
   mTimeoutLength(0),
   mLastTime(0),
   mRetryTime(0),
   mRetryInterval(2500),
   mNumRetries(0),
   mMaxRetries(1),
   mState(cStateIdle),
   mError(cErrorSuccess),
   mCheckTimeout(false),
   mTimedOut(false)
{
#ifdef XBOX
   Utils::FastMemSet(&mOverlapped, 0, sizeof(mOverlapped));
   mpOverlapped = &mOverlapped;
#endif // XBOX
}

//==============================================================================
// BAsyncTask::~BAsyncTask
//==============================================================================
BAsyncTask::~BAsyncTask()
{
}

//==============================================================================
// BAsyncTask::isComplete
//==============================================================================
BOOL BAsyncTask::isComplete()
{
#ifdef XBOX
   return XHasOverlappedIoCompleted(mpOverlapped);
#else
   return false;
#endif // XBOX
}

//==============================================================================
// BAsyncTask::getResult
//==============================================================================
DWORD BAsyncTask::getResult(LPDWORD pdwResult)
{
#ifdef XBOX
   return XGetOverlappedResult(mpOverlapped, pdwResult, FALSE);
#else
   return false;
#endif // XBOX
}

//==============================================================================
// BAsyncTask::getResult
//==============================================================================
DWORD BAsyncTask::getExtendedError()
{
#ifdef XBOX
   return XGetOverlappedExtendedError(mpOverlapped);
#else
   return false;
#endif // XBOX
}

//==============================================================================
// BAsyncTask::update
//==============================================================================
bool BAsyncTask::update()
{
   if (updateTimeout())
      return true;

   switch (mState)
   {
      case cStateError:
      case cStateDone:
         return true;
   }

   return false;
}

//==============================================================================
// BAsyncTask::getResult
//==============================================================================
void BAsyncTask::processResult()
{
   if (mpNotify)
      mpNotify->notify(mEventID, this);
}

//==============================================================================
// BAsyncTask::updateTimeout
//==============================================================================
bool BAsyncTask::updateTimeout()
{
   // check timeout
   if (!mCheckTimeout)
      return false;

   // have we timed-out?
   if ((timeGetTime() - mStartTime) < mTimeoutLength)
      return false;

   // Cancel the operation
#ifdef XBOX
   HRESULT hr = XCancelOverlapped(mpOverlapped);
   if (SUCCEEDED(hr))
   {
      mTimedOut = true;
      setState(cStateError, cErrorTimedout);
      blogerrortrace("BASyncTask::updateTimeout -- Async call timed out.");
      return true;
   }
#else
   return false;
#endif // XBOX

   return false;
}

//==============================================================================
// 
//==============================================================================
void BAsyncTask::setState(eState state, eError error)
{
   mState = state;
   mError = error;

   // reset the retry values?
   // I may be transitioning to another state that also wants to start
   // checking for timeout conditions
   setRetries(10);
}

//==============================================================================
// 
//==============================================================================
void BAsyncTask::setRetries(uint maxRetries, DWORD interval)
{
   mNumRetries = 0;
   mMaxRetries = maxRetries;
   mRetryInterval = (interval == 0 ? 2500 : interval);
   mRetryTime = timeGetTime();
}

//==============================================================================
// 
//==============================================================================
void BAsyncTask::setRetryInterval(DWORD interval)
{
   mRetryInterval = interval;
   mRetryTime = timeGetTime() + mRetryInterval;
}

//==============================================================================
// BAsyncTaskManager::BAsyncTaskManager
//==============================================================================
BAsyncTaskManager::BAsyncTaskManager()
{
   mTasks.clear();
}

//==============================================================================
// BAsyncTaskManager::~BAsyncTaskManager
//==============================================================================
BAsyncTaskManager::~BAsyncTaskManager()
{
}

//==============================================================================
// BAsyncTaskManager::addTask
//==============================================================================
void BAsyncTaskManager::addTask(BAsyncTask* pTask)
{
   mTasks.add(pTask);
}

//==============================================================================
// BAsyncTaskManager::update
//==============================================================================
void BAsyncTaskManager::update()
{
   // deal with the tasks
   for (int i=0; i<mTasks.getNumber(); i++)
   {
      // did the async task complete
      BAsyncTask* pTask = mTasks[i];

      pTask->update();                  // let the task update things it needs (like timeouts)

      if (!pTask->isComplete())
         continue;

      // This task has completed, let's process it
      pTask->processResult();

      // remove and preserve order
      mTasks.removeIndex(i,true);
      i--;

      // don't want a dangling task
      delete pTask;
   }
}

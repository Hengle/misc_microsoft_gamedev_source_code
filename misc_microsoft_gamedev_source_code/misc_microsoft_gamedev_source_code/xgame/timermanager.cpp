//==============================================================================
// timerManager.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "timermanager.h"
#include "world.h"


// Globals
BTimerManager gTimerManager;

GFIMPLEMENTVERSION(BTimerManager, 1);

//==============================================================================
// BGameTimer::BGameTimer
//==============================================================================
BGameTimer::BGameTimer() :
   mCountUp(true),
   mStartTime(0),
   mStopTime(0),
   mDone(false),
   mActive(false),
   mPaused(false),
   mID(-1)
{
}

//==============================================================================
// BGameTimer::~BGameTimer
//==============================================================================
BGameTimer::~BGameTimer()
{
}

//==============================================================================
// BGameTimer::reset
//==============================================================================
void BGameTimer::reset()
{
   mCountUp = true;
   mStartTime = 0;
   mStopTime = 0;
   mDone = false;
   mActive = false;
   mPaused = false;
   mID = -1;
}


//==============================================================================
// BGameTimer::initialize
//==============================================================================
bool BGameTimer::initialize(bool countUp, DWORD startTime, DWORD stopTime, int ID)
{
   if (mActive)
      return false;

   mID = ID;

   mCountUp = countUp;
   mStartTime = startTime;
   mStopTime = stopTime;
   mPaused = false;
   mDone = false;

   return true;
}

//==============================================================================
// BGameTimer::start
//==============================================================================
void BGameTimer::start()
{
   mCurrentTime = mStartTime;
   mLastTime = gWorld->getGametime();
}

//==============================================================================
// BGameTimer::update
//==============================================================================
void BGameTimer::update(float elapsedTimeFloat)
{
   if (!mActive)
      return;

   if (mDone)
      return;

   DWORD elapsedTime = gWorld->getGametime() - mLastTime;
   mLastTime = gWorld->getGametime();

   if (mPaused)
   {
      return;
   }

   if (mCountUp)
   {
      // increment the timer
      mCurrentTime += elapsedTime;
      mCurrentTime = min(mCurrentTime, mStopTime);
   }
   else
   {
      // decrement the timer
      if (elapsedTime > mCurrentTime)
         mCurrentTime = 0;
      else
         mCurrentTime -= elapsedTime;
      mCurrentTime = max(mCurrentTime, mStopTime);
   }

   // did the timer expire?
   if (mCurrentTime == mStopTime)
   {
      // indicate we are done
      mDone = true;

      // do we want to update a trigger variable?
   }

}

//------------------------------------------------------------------------------


//==============================================================================
// BTimerManager::BTimerManager
//==============================================================================
BTimerManager::BTimerManager() : 
   mNextTimerID(0)
{
}

//==============================================================================
// BTimerManager::~BTimerManager
//==============================================================================
BTimerManager::~BTimerManager()
{
}

//==============================================================================
// BTimerManager::initialize
//==============================================================================
void BTimerManager::initialize()
{
   mNextTimerID = 0;

   for (int i=0; i<cNumGameTimers; i++)
   {
      mTimers[i].reset();
   }

   return;
}


//==============================================================================
// BTimerManager::createTimer
//==============================================================================
int BTimerManager::createTimer(bool countUp, DWORD startTime, DWORD stopTime)
{
   for (int i=0; i<cNumGameTimers; i++)
   {
      if (mTimers[i].getActive())
         continue;

      // we have a timer, grab it
      BGameTimer& timer = mTimers[i];
      timer.initialize(countUp, startTime, stopTime, mNextTimerID++);
      timer.setActive(true);
      timer.start();

      return timer.getID();
   }

   return -1;
}

//==============================================================================
// BTimerManager::update
//==============================================================================
void BTimerManager::update(float elapsedTime)
{
   for (int i=0; i<cNumGameTimers; i++)
   {
      if (!mTimers[i].getActive())
         continue;

      mTimers[i].update(elapsedTime);
   }
}

//==============================================================================
// BTimerManager::getTimer
//==============================================================================
BGameTimer* BTimerManager::getTimer(int timerID)
{
   for (int i=0; i<cNumGameTimers; i++)
   {
      if (!mTimers[i].getActive())
         continue;

      if (mTimers[i].getID() != timerID)
         continue;

      // found the timer
      return &(mTimers[i]);
   }

   return NULL;
}


//==============================================================================
// BTimerManager::destroyTimer
//==============================================================================
void BTimerManager::destroyTimer(int timerID)
{
   for (int i=0; i<cNumGameTimers; i++)
   {
      if (!mTimers[i].getActive())
         continue;

      if (mTimers[i].getID() != timerID)
         continue;

      // found the timer
      return mTimers[i].reset();
   }
}

//============================================================================
//============================================================================
bool BTimerManager::save(BStream* pStream, int saveType) const
{
   for (int i=0; i<cNumGameTimers; i++)
      GFWRITECLASS(pStream, saveType, mTimers[i])
   GFWRITEVAR(pStream, int, mNextTimerID);
   return true;
}

//============================================================================
//============================================================================
bool BTimerManager::load(BStream* pStream, int saveType)
{
   for (int i=0; i<cNumGameTimers; i++)
      GFREADCLASS(pStream, saveType, mTimers[i])
   GFREADVAR(pStream, int, mNextTimerID);
   return true;
}

//============================================================================
//============================================================================
bool BGameTimer::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, DWORD, mStartTime);
   GFWRITEVAR(pStream, DWORD, mStopTime);
   GFWRITEVAR(pStream, DWORD, mCurrentTime);
   GFWRITEVAR(pStream, DWORD, mLastTime);
   GFWRITEVAR(pStream, int,  mID);
   GFWRITEBITBOOL(pStream, mCountUp);
   GFWRITEBITBOOL(pStream, mActive);
   GFWRITEBITBOOL(pStream, mDone);
   GFWRITEBITBOOL(pStream, mPaused);
   return true;
}

//============================================================================
//============================================================================
bool BGameTimer::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, DWORD, mStartTime);
   GFREADVAR(pStream, DWORD, mStopTime);
   GFREADVAR(pStream, DWORD, mCurrentTime);
   GFREADVAR(pStream, DWORD, mLastTime);
   GFREADVAR(pStream, int,  mID);
   GFREADBITBOOL(pStream, mCountUp);
   GFREADBITBOOL(pStream, mActive);
   GFREADBITBOOL(pStream, mDone);
   GFREADBITBOOL(pStream, mPaused);
   return true;
}

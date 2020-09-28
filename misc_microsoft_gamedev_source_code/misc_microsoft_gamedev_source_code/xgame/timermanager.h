//==============================================================================
// timerManager.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#pragma once

#include "gamefilemacros.h"

// Forward Declarations
class BTimerManager;

// Global variable for the one BProfileManager object
extern BTimerManager gTimerManager;


//==============================================================================
// BGameTimer
//==============================================================================
class BGameTimer
{
public:
   BGameTimer();
   ~BGameTimer();

   bool initialize(bool countUp, DWORD startTime, DWORD stopTime, int ID);

   void update(float elapsedTimeFloat);
   void start();

   //XXXHalwes 10/4/2007 - This API is confusing.  setActive doesn't even do anything.
   bool getActive() { return mActive; }
   void setActive(bool active) { mActive = true; }

   bool getCountUp() { return mCountUp; }

   void reset();

   DWORD getCurrentTime() { return mCurrentTime; }

   int  getID() { return mID; }

   bool isDone() { return mDone; }

   bool getPaused() { return (mPaused); }
   void setPaused(bool paused) { mPaused = paused; }

   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   // Time tracking variables
   DWORD mStartTime;
   DWORD mStopTime;
   DWORD mCurrentTime;
   DWORD mLastTime;

   int  mID;

   // flags
   bool mCountUp : 1;
   bool mActive : 1;
   bool mDone : 1;
   bool mPaused:1;
};


//==============================================================================
// BTimerManager
//==============================================================================
class BTimerManager
{
   enum { cNumGameTimers = 4 };
public:

   BTimerManager();
   ~BTimerManager();

   void initialize();

   void update(float elapsedTime);

   int createTimer(bool countUp, DWORD startTime, DWORD stopTime);

   BGameTimer* getTimer(int timerID);
   void destroyTimer(int timerID);

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:
   BGameTimer  mTimers[cNumGameTimers];
   int         mNextTimerID;
};

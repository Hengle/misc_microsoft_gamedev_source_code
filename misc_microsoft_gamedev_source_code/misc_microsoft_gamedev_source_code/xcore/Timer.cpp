//============================================================================
//
//  Timer.cpp
//  
//  Copyright (c) 1999-2006, Ensemble Studios
//
//============================================================================

//============================================================================
//  INCLUDES
//============================================================================
#include "xcore.h"
#include "timer.h"

namespace 
{
   class BTimerFreq
   {
   public:
      BTimerFreq()
      {
         init();               
      }
      
      void init(void)
      {
         if (mFrequency == 0)
         {
            LARGE_INTEGER temp;
            QueryPerformanceFrequency(&temp);

            mFrequency = temp.QuadPart;
            mInverse = 1.0 / (double)mFrequency;      
         }
      }         
      
      __int64  mFrequency;
      double   mInverse;
   };
   
   BTimerFreq gTimerFreq;
} // anonymous namespace

//============================================================================
//  CONSTRUCTION/DESTRUCTION
//============================================================================
BTimer::BTimer()
{
   reset();
}

//============================================================================
//  INTERFACE
//============================================================================


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BTimer::reset()
{
   mStartTime = INT64_MAX;
   mStopTime = INT64_MAX;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BTimer::start()
{
   LARGE_INTEGER temp;
   QueryPerformanceCounter(&temp);
   mStartTime = temp.QuadPart;
   mStopTime  = INT64_MAX;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BTimer::resume()
{
   // For resume to make sense, we need to have started and then have been stopped.
   // Also, we consider it to be ok to have never been started.  In this case nothing will go wrong 
   // since elapsed will be zero and this will be just like a start operation.
   if(mStopTime == INT64_MAX && mStartTime != INT64_MAX)
   {
      BFAIL("Trying to resume when we haven't been stopped");
      return;
   }
   
   // Get the elapsed time we are storing from when we stopped.
   __int64 currElapsed = mStopTime - mStartTime;
   
   // Get the current time.
   LARGE_INTEGER currTime;
   QueryPerformanceCounter(&currTime);
   
   // Adjust start time to reflect the time when we would have started had we not paused.
   mStartTime = currTime.QuadPart - currElapsed;
   
   // Make note that we are no longer stopped.
   mStopTime = INT64_MAX;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void BTimer::stop()
{
   BDEBUG_ASSERT(isStarted());
   
   LARGE_INTEGER temp;
   QueryPerformanceCounter(&temp);
   mStopTime  = temp.QuadPart;
   
   // rg - Don't allow timer to stop at INT64_MAX, which is reserved to indicate that the timer hasn't been stopped. 
   // It's OK if this overflows because all we do is subtract stop and start times.
   // Yes the odds of me being struck by lightening right now are higher than this actually happening.
   if (mStopTime == INT64_MAX)
      mStopTime++;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
double BTimer::getElapsedSeconds() const
{
   BDEBUG_ASSERT(isStarted());

   __int64 stopTime = mStopTime;
   if (stopTime == INT64_MAX)
   {
      LARGE_INTEGER temp;
      QueryPerformanceCounter(&temp);
      stopTime = temp.QuadPart;
   }
   
   gTimerFreq.init();

   return ((double)(stopTime - mStartTime) * gTimerFreq.mInverse);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned long BTimer::getElapsedMilliseconds() const
{
   BDEBUG_ASSERT(isStarted());
   
   __int64 stopTime = mStopTime;
   if (stopTime == INT64_MAX)
   {
      LARGE_INTEGER temp;
      QueryPerformanceCounter(&temp);
      stopTime = temp.QuadPart;
   }
   
   gTimerFreq.init();
   
   return (unsigned long)(((stopTime - mStartTime) * 1000U) / gTimerFreq.mFrequency);
}


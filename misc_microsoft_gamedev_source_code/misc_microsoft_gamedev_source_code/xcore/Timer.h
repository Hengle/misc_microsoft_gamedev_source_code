//============================================================================
//
//  BTimer.h
//  
// Copyright (c) 1999-2006, Ensemble Studios
//
//============================================================================
#pragma once

//----------------------------------------------------------------------------
//  Class BTimer
//----------------------------------------------------------------------------
class BTimer
{
public:
	BTimer();
	void           reset                 ();

	void           start                 ();
	void           resume                ();   // jce [10/28/2008] -- this starts counting again maintaining the previous elapsed amount (like pausing a stopwatch and restarting)
	void           stop                  ();
   double         getElapsedSeconds     ()  const;
   unsigned long  getElapsedMilliseconds()  const;
   
   const bool     isStarted()               const { return mStartTime != INT64_MAX; }
   const bool     isStopped()               const { return mStopTime != INT64_MAX; }
   
private:
   __int64  mStartTime;
   __int64  mStopTime;
};

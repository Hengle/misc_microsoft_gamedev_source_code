//==============================================================================
// mpgametiming.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#ifndef _MPGAMETIMING_H_
#define _MPGAMETIMING_H_

//==============================================================================
// Includes
#include "Channel.h"

//==============================================================================
// Forward declarations
class BTimeSync;
class BTimeSyncStrategy;

//==============================================================================
// Const declarations

//==============================================================================
class BMPGameTiming
{
public:
   BMPGameTiming(BChannel *channel);
   virtual ~BMPGameTiming();

   BTimeSync*              getTimeSync(void) const { return(mTimeSync); }
   void                    setTimeSync(BTimeSync *timesync) { mTimeSync=timesync; }
   
   virtual DWORD           advanceGameTime(void)=0;
   virtual void            startingGame(void)=0;
   virtual void            gameStarted(void)=0;
   virtual void            gameStopped(void)=0;
   virtual void            setPaused(bool v)=0;
   virtual bool            isGameStarted(void)=0;
   virtual bool            isGameStarting(void)=0;
   virtual void            dataReceived(DWORD fromClientIndex, const void* data, DWORD size);

protected:

   BChannel                *mChannel;
   BTimeSync               *mTimeSync;
};

//==============================================================================
class BMPGameTimingFactory
{
public:
   virtual BTimeSyncStrategy*      getTimingStrategy(BMPGameTiming *timing)=0;
   virtual BMPGameTiming*          getGameTiming(BChannel *channel)=0;
};

//==============================================================================
#endif // _MPGAMETIMING_H_

//==============================================================================
// eof: mpgametiming.h
//==============================================================================

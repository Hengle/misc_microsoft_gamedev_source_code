//==============================================================================
// mpgametiming.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "multiplayercommon.h"
#include "mpgametiming.h"
#include "TimeSync.h"

//==============================================================================
// Defines

//==============================================================================
// BMPGameTiming::BMPGameTiming
//==============================================================================
BMPGameTiming::BMPGameTiming(BChannel *channel) : mChannel(channel), mTimeSync(NULL)
{
}

//==============================================================================
// BMPGameTiming::~BMPGameTiming
//==============================================================================
BMPGameTiming::~BMPGameTiming()
{
   if (mTimeSync)
   {
      mTimeSync->dispose();   
      mTimeSync = NULL;
   }
}

//==============================================================================
// BMPGameTiming::dataReceived
//==============================================================================
void BMPGameTiming::dataReceived(DWORD fromClientIndex, const void* data, DWORD size)
{
   if (mTimeSync)
      mTimeSync->timingDataReceived((long)fromClientIndex, data, size);
}

//==============================================================================
// eof: mpgametiming.cpp
//==============================================================================

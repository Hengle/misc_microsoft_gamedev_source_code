//==============================================================================
// timingtracker.cpp
//
// Copyright (c) 2001-2007, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "timingtracker.h"
#include "timesync.h"
#include "world.h"

// Const declarations
const double cDefaultUpdateLength = 0.015;
const double cDefaultOutsideUpdateLength = 0.005;
const double cDefaultFrameLength = 0.018;

const double cMinUpdateLength = 0.001;
const double cMaxUpdateLength = 0.255;

const double cMinOutsideUpdateLength = 0.001;
const double cMaxOutsideUpdateLength = 0.255;

const double cMinFrameLength = 0.001;
const double cMaxFrameLength = 0.1;

//==============================================================================
// 
//==============================================================================
void BTimingTracker::init()
{
   for (uint32 i=0; i < cUpdateHistorySize; i++)
      mUpdateHistory[i] = cDefaultUpdateLength;

   for (uint32 i=0; i < cOutsideUpdateHistorySize; i++)
      mOutsideUpdateHistory[i] = cDefaultOutsideUpdateLength;

   for (uint32 i=0; i < cFrameHistorySize; i++)
      mFrameHistory[i] = cDefaultFrameLength;

   mUpdateSum = cDefaultUpdateLength*cUpdateHistorySize;
   mOutsideUpdateSum = cDefaultOutsideUpdateLength*cOutsideUpdateHistorySize;
   mFrameSum = cDefaultFrameLength*cFrameHistorySize;

   mUpdateAverage = cDefaultUpdateLength;
   mOutsideUpdateAverage = cDefaultOutsideUpdateLength;
   mFrameAverage = cDefaultFrameLength;
}

//==============================================================================
// 
//==============================================================================
uint32 BTimingTracker::getLastUpdateLength() const
{
   if (mUpdateHistoryIndex > 0)
      return static_cast<uint32>(mUpdateHistory[mUpdateHistoryIndex-1] * 1000.0);
   else
      return static_cast<uint32>(mUpdateHistory[cUpdateHistorySize-1] * 1000.0);
}

//==============================================================================
// 
//==============================================================================
uint32 BTimingTracker::getLastOutsideUpdateLength() const
{
   if (mOutsideUpdateHistoryIndex > 0)
      return static_cast<uint32>(mOutsideUpdateHistory[mOutsideUpdateHistoryIndex-1] * 1000.0);
   else
      return static_cast<uint32>(mOutsideUpdateHistory[cOutsideUpdateHistorySize-1] * 1000.0);
}

//==============================================================================
// 
//==============================================================================
uint32 BTimingTracker::getLastFrameLength() const
{
   if (mFrameHistoryIndex > 0)
      return static_cast<uint32>(mFrameHistory[mFrameHistoryIndex-1] * 1000.0);
   else
      return static_cast<uint32>(mFrameHistory[cFrameHistorySize-1] * 1000.0);
}

//==============================================================================
// 
//==============================================================================
void BTimingTracker::addLastFrameLength(double length)
{
   mLastFrameLength = length;

   // calculate average
   length = max(cMinFrameLength, length);
   length = min(cMaxFrameLength, length);

   mFrameSum -= mFrameHistory[mFrameHistoryIndex];
   mFrameHistory[mFrameHistoryIndex++] = length;
   if (mFrameHistoryIndex == cFrameHistorySize)
      mFrameHistoryIndex = 0;
   mFrameSum += length;
   mFrameAverage = mFrameSum / cFrameHistorySize;

   double variance = 0.0;

   for (uint32 i=0; i < cFrameHistorySize; i++)
      variance += (mFrameHistory[i] - mFrameAverage) * (mFrameHistory[i] - mFrameAverage);
   variance = variance / (cFrameHistorySize - 1);

   mFrameStandardDeviation = sqrt(variance);

   //blog("BTimingTracker::addLastFrameLength:%f,%f", mLastUpdateLength, mLastFrameLength);
}

//==============================================================================
// 
//==============================================================================
void BTimingTracker::addLastUpdateLength(double length)
{
   mLastUpdateLength = length;

   length = max(cMinUpdateLength, length);
   length = min(cMaxUpdateLength, length);

   // calculate average
   mUpdateSum -= mUpdateHistory[mUpdateHistoryIndex];
   mUpdateHistory[mUpdateHistoryIndex++] = length;
   if (mUpdateHistoryIndex == cUpdateHistorySize)
      mUpdateHistoryIndex = 0;
   mUpdateSum += length;
   mUpdateAverage = mUpdateSum / cUpdateHistorySize;

   double variance = 0.0;
   for (uint32 i=0; i < cUpdateHistorySize; i++)
      variance += (mUpdateHistory[i] - mUpdateAverage) * (mUpdateHistory[i] - mUpdateAverage);
   variance = variance / (cUpdateHistorySize - 1);

   mUpdateStandardDeviation = sqrt(variance);
}

//==============================================================================
// 
//==============================================================================
void BTimingTracker::addLastOutsideUpdateLength(double length)
{
   mLastOutsideUpdateLength = length;

   length = max(cMinOutsideUpdateLength, length);
   length = min(cMaxOutsideUpdateLength, length);

   // calculate average
   mOutsideUpdateSum -= mOutsideUpdateHistory[mOutsideUpdateHistoryIndex];
   mOutsideUpdateHistory[mOutsideUpdateHistoryIndex++] = length;
   if (mOutsideUpdateHistoryIndex == cOutsideUpdateHistorySize)
      mOutsideUpdateHistoryIndex = 0;
   mOutsideUpdateSum += length;
   mOutsideUpdateAverage = mOutsideUpdateSum / cOutsideUpdateHistorySize;

   double variance = 0.0;
   for (uint32 i=0; i < cOutsideUpdateHistorySize; i++)
      variance += (mOutsideUpdateHistory[i] - mOutsideUpdateAverage) * (mOutsideUpdateHistory[i] - mOutsideUpdateAverage);
   variance = variance / (cOutsideUpdateHistorySize - 1);

   mOutsideUpdateStandardDeviation = sqrt(variance);
}

//==============================================================================
// 
//==============================================================================
uint32 BTimingTracker::getTotalTiming(double updateTiming)
{
   //if (mFrameTimingStandardDeviation == 0 || mFrameStandardDeviation < mFrameTimingStandardDeviation*2)
   //   mFrameTimingStandardDeviation = mFrameStandardDeviation;

   double pt = mFrameAverage;

   //pt += mFrameTimingStandardDeviation;

   pt = (pt * 1000.0 + 0.5);

   pt += updateTiming;
   pt = min(pt, 255.0);

   return static_cast<uint32>(pt);
}

//==============================================================================
// 
//==============================================================================
uint32 BTimingTracker::getLocalTiming(uint32 maxUpdateLength, uint32* deviationRemaining)
{
   //blog("BTimingTracker::getLocalTiming:%f,%f,%f,%f,", mUpdateAverage, mFrameAverage, mUpdateStandardDeviation, mUpdateTimingStandardDeviation);

   // make sure the std dev doesn't jump too far, arbitrarily limiting it to twice that of our previous value
   // the normal cause of a std dev spike is from xfs loads
   //if (mUpdateTimingStandardDeviation == 0 || mUpdateStandardDeviation < mUpdateTimingStandardDeviation*2)
   //   mUpdateTimingStandardDeviation = mUpdateStandardDeviation;

   double pt = mUpdateAverage;

   //pt += mUpdateTimingStandardDeviation;

   pt = (pt * 1000.0 + 0.5);
   pt = min(pt, 255.0);

   if (deviationRemaining)
      *deviationRemaining = 0;

   pt = getTotalTiming(pt);

   return static_cast<uint32>(pt);

   //DWORD pt = mUpdateAverage+mFrameAverage;
   //// how much room do we have left to add in deviation?
   //DWORD room = 0;
   //if (pt < maxUpdateLength)
   //   room = maxUpdateLength - pt;

   //// what's our perfect deviation?
   //DWORD pd =  (DWORD)(mUpdateStandardDeviation*cStandardDeviationMultiplier) +
   //            (DWORD)(mFrameStandardDeviation*cStandardDeviationMultiplier);
   //if (deviationRemaining)
   //{
   //   if (pd >= room)
   //      *deviationRemaining = pd-room;
   //   else
   //      *deviationRemaining = 0;
   //}

   //pt += min(pd, max(0,room));

   //return (unsigned char)min(pt, 255); // clamp to 255
}

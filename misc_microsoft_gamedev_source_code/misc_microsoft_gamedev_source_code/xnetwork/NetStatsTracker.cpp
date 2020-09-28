//==============================================================================
// BNetStatsTracker.cpp
//
// Copyright (c) 2007, Ensemble Studios
//==============================================================================

// Includes
#include "Precompiled.h"
#include "NetStatsTracker.h"

#include "commlog.h"

BNetStatsTracker::BNetStatsTracker(long loggingTarget, const char* text) :
mStatsBytesSent(0),
mStatsBytesRcvd(0),
mStatsBytesSentPerSecondPeak(0),
mStatsBytesRcvdPerSecondPeak(0),
mStatsPacketsSent(0),
mStatsPacketsRcvd(0),
mLogtarget(loggingTarget)
{

   mStatsStartTime = timeGetTime();
   ZeroMemory( &mStatsRcvdTrack, sizeof(mStatsRcvdTrack));
   ZeroMemory( &mStatsSentTrack, sizeof(mStatsSentTrack));
   StringCchCopyA(mDescription,254,text);
}

BNetStatsTracker::~BNetStatsTracker()
{
   float runDuration = (timeGetTime()-mStatsStartTime) / 1000.0f;
   float sentBytesPerSecondAvg = mStatsBytesSent/runDuration;
   float sentPacketsPerSecondAvg = mStatsPacketsSent/runDuration;
   float rcvdBytesPerSecondAvg = mStatsBytesRcvd/runDuration;
   float rcvdPacketsPerSecondAvg = mStatsPacketsRcvd/runDuration;

   nlog(mLogtarget, "%s running for %f seconds", mDescription, runDuration );
   nlog(mLogtarget, "Bytes sent - Total:%d   AvgPerSecond:%f  PeakPerSecond:%d", mStatsBytesSent, sentBytesPerSecondAvg, mStatsBytesSentPerSecondPeak);
   nlog(mLogtarget, "Packets sent - Total:%d   AvgPerSecond:%f", mStatsPacketsSent, sentPacketsPerSecondAvg);
   nlog(mLogtarget, "Bytes received - Total:%d   AvgPerSecond:%f  PeakPerSecond:%d", mStatsBytesRcvd, rcvdBytesPerSecondAvg, mStatsBytesRcvdPerSecondPeak);
   nlog(mLogtarget, "Packets received - Total:%d   AvgPerSecond:%f", mStatsPacketsRcvd, rcvdPacketsPerSecondAvg);
}

void BNetStatsTracker::postRcvd(DWORD bytesRcvd)
{
   mStatsBytesRcvd += bytesRcvd;
   mStatsPacketsRcvd += 1;

   //Find the correct bucket to write to
   DWORD currentTime = timeGetTime();
   DWORD bucketIndex = (int)(currentTime/100) - ((int)(currentTime/1000)*10);
   BASSERT((bucketIndex>=0) && (bucketIndex<cMaxTrackingRecords));
   if ((currentTime - mStatsRcvdTrack[bucketIndex].time) < 1000)
   {
      //Just update this bucket
      mStatsRcvdTrack[bucketIndex].amount += bytesRcvd;
   }
   else
   {
      //Check the max since we are overwritting this bucket
      DWORD totalBytes = 0;
      for (int i=0;i<cMaxTrackingRecords;i++)
         totalBytes += mStatsRcvdTrack[i].amount;
      if (totalBytes > mStatsBytesRcvdPerSecondPeak)
      {
         mStatsBytesRcvdPerSecondPeak = totalBytes;
         nlog(mLogtarget, "%s hit a new rcvd bytes per second of %d", mDescription, mStatsBytesRcvdPerSecondPeak );
      }
      mStatsRcvdTrack[bucketIndex].amount = bytesRcvd;
      DWORD bucketTime = (int)(currentTime/100);
      mStatsRcvdTrack[bucketIndex].time = bucketTime;
      if (bucketIndex>0)
      {
         for (i=bucketIndex-1;i>=0;i--)
         {
            if (mStatsRcvdTrack[i].time != (bucketTime-(bucketIndex-i)))
            {
               //This bucket was not within the same second as the just updated tracker

               mStatsRcvdTrack[i].amount = 0;
               mStatsRcvdTrack[i].time = (bucketTime-(bucketIndex-i));
            }         
         }
      }
      if (bucketIndex<cMaxTrackingRecords)
      {
         for (i=bucketIndex+1;i<cMaxTrackingRecords;i++)
         {
            if (mStatsRcvdTrack[i].time != (bucketTime+(i-bucketIndex)))
            {
               //This bucket was not within the same second as the just updated tracker

               mStatsRcvdTrack[i].amount = 0;
               mStatsRcvdTrack[i].time = (bucketTime+(i-bucketIndex));
            }         
         }
      }
   }
}

void BNetStatsTracker::postSent(DWORD bytesSent)
{
   mStatsBytesSent += bytesSent;
   mStatsPacketsSent += 1;

   //Find the correct bucket to write to
   DWORD currentTime = timeGetTime();
   DWORD bucketIndex = (int)(currentTime/100) - ((int)(currentTime/1000)*10);
   BASSERT((bucketIndex>=0) && (bucketIndex<cMaxTrackingRecords));
   if ((currentTime - mStatsSentTrack[bucketIndex].time) < 1000)
   {
      //Just update this bucket
      mStatsSentTrack[bucketIndex].amount += bytesSent;
   }
   else
   {
      //Check the max since we are overwritting this bucket
      DWORD totalBytes = 0;
      for (int i=0;i<cMaxTrackingRecords;i++)
         totalBytes += mStatsSentTrack[i].amount;
      if (totalBytes > mStatsBytesSentPerSecondPeak)
      {
         mStatsBytesSentPerSecondPeak = totalBytes;
         nlog(mLogtarget, "%s hit a new sent bytes per second of %d", mDescription, mStatsBytesSentPerSecondPeak );
      }
      mStatsSentTrack[bucketIndex].amount = bytesSent;
      DWORD bucketTime = (int)(currentTime/100);
      mStatsSentTrack[bucketIndex].time = bucketTime;
      if (bucketIndex>0)
      {
         for (i=bucketIndex-1;i>=0;i--)
         {
            if (mStatsSentTrack[i].time != (bucketTime-(bucketIndex-i)))
            {
               //This bucket was not within the same second as the just updated tracker

               mStatsSentTrack[i].amount = 0;
               mStatsSentTrack[i].time = (bucketTime-(bucketIndex-i));
            }         
         }
      }
      if (bucketIndex<cMaxTrackingRecords)
      {
         for (i=bucketIndex+1;i<cMaxTrackingRecords;i++)
         {
            if (mStatsSentTrack[i].time != (bucketTime+(i-bucketIndex)))
            {
               //This bucket was not within the same second as the just updated tracker

               mStatsSentTrack[i].amount = 0;
               mStatsSentTrack[i].time = (bucketTime+(i-bucketIndex));
            }         
         }
      }
   }
}



//==============================================================================
// BNetStatsTracker.h
//
// Copyright (c) 2007, Ensemble Studios
//==============================================================================
#pragma once

// This class tracks stats for a particular instance of a network object.
//   You just initialize it at object startup time, let it know when you send/rcvd data
//   And when it is destroyed - it logs out the stats for that object

class BNetStatsTracker 
{
public:
   BNetStatsTracker(long loggingTarget, const char* text);
   ~BNetStatsTracker();
   void postRcvd(DWORD bytesRcvd);
   void postSent(DWORD bytesSent);

private:

   enum { cMaxTrackingRecords=10};
   struct  trackingRecord
   {
      DWORD time;
      DWORD amount;
   };

   trackingRecord    mStatsRcvdTrack[cMaxTrackingRecords];
   trackingRecord    mStatsSentTrack[cMaxTrackingRecords];
   DWORD             mStatsBytesSent;
   DWORD             mStatsBytesRcvd;
   DWORD             mStatsBytesSentPerSecondPeak;
   DWORD             mStatsBytesRcvdPerSecondPeak;
   DWORD             mStatsPacketsSent;
   DWORD             mStatsPacketsRcvd;
   DWORD             mStatsStartTime;

   long              mLogtarget;
   char              mDescription[255];
};
//==============================================================================
// timingtracker.h
//
// Copyright (c) 2001-2007, Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
// BTimingTracker
//==============================================================================
class BTimingTracker
{
   public:

      BTimingTracker(void) :
         mUpdateAverage(0.0),
         mOutsideUpdateAverage(0.0),
         mFrameAverage(0.0),
         mUpdateHistoryIndex(0),
         mOutsideUpdateHistoryIndex(0),
         mFrameHistoryIndex(0),
         mUpdateSum(0.0),
         mOutsideUpdateSum(0.0),
         mFrameSum(0.0),
         mUpdateStandardDeviation(0.0),
         mOutsideUpdateStandardDeviation(0.0),
         mFrameStandardDeviation(0.0),
         //mUpdateTimingStandardDeviation(0.0),
         //mFrameTimingStandardDeviation(0.0),
         mLastUpdateLength(0.0),
         mLastOutsideUpdateLength(0.0),
         mLastFrameLength(0.0)
         {
            init();
         }

      void                       addLastUpdateLength(double length); // the length of a sim update (or sub-update, if sub-updating is turned on)
      void                       addLastOutsideUpdateLength(double length); // the length of time between exiting the bottom of the sim update, looping around and entering back into the top of it
      void                       addLastFrameLength(double length); // the length of an entire frame, including the update

      uint32                     getLocalTiming(uint32 maxUpdateLength, uint32* deviationRemaining);

      uint32                     getUpdateAverage() const { return static_cast<uint32>(mUpdateAverage * 1000.0); }
      uint32                     getOutsideUpdateAverage() const { return static_cast<uint32>(mOutsideUpdateAverage * 1000.0); }
      uint32                     getFrameAverage() const { return static_cast<uint32>(mFrameAverage * 1000.0); }
      uint32                     getLastUpdateLength() const;
      uint32                     getLastOutsideUpdateLength() const;
      uint32                     getLastFrameLength() const;
      uint32                     getUpdateDeviation() const { return static_cast<uint32>(mUpdateStandardDeviation*1000.0); }
      uint32                     getOutsideUpdateDeviation() const { return static_cast<uint32>(mOutsideUpdateStandardDeviation*1000.0); }
      uint32                     getFrameDeviation() const { return static_cast<uint32>(mFrameStandardDeviation*1000.0); }

   private:

      uint32                     getTotalTiming(double updateTiming);

      void                       init(void);

      enum 
      { 
         cUpdateHistorySize = 60,
         cOutsideUpdateHistorySize = 60,
         cFrameHistorySize = 60
      };

      // local timing history vars
      uint32                     mUpdateHistoryIndex;
      uint32                     mOutsideUpdateHistoryIndex;
      uint32                     mFrameHistoryIndex;

      double                     mUpdateHistory[cUpdateHistorySize];
      double                     mOutsideUpdateHistory[cUpdateHistorySize];
      double                     mFrameHistory[cFrameHistorySize];

      double                     mUpdateAverage;
      double                     mOutsideUpdateAverage;
      double                     mFrameAverage;

      double                     mUpdateSum;
      double                     mOutsideUpdateSum;
      double                     mFrameSum;

      double                     mUpdateStandardDeviation;
      double                     mOutsideUpdateStandardDeviation;
      double                     mFrameStandardDeviation;

      //double                     mUpdateTimingStandardDeviation;
      //double                     mFrameTimingStandardDeviation;

      double                     mLastUpdateLength;
      double                     mLastOutsideUpdateLength;
      double                     mLastFrameLength;
};
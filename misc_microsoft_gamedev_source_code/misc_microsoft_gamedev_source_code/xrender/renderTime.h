//============================================================================
//
//  renderTime.h
//  
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

//============================================================================
// class BRenderTime
//============================================================================
class BRenderTime
{
public:
   BRenderTime();
   ~BRenderTime();
   
   void init(void);
   void deinit(void);
   
   void simUpdate(uint updateNumber, bool paused, double updateRealtime, double renderRealtime, double deltaTRealtime, double lastUpdateLength, double totalRealtime, double totalGametime);
   
   // Render thread only.
   
   // Sim's update number. (Not the same as the frame count managed by gRenderThread!)
   uint getUpdateNumber(void) const                { return mState.mUpdateNumber; }
   
   // True if the sim is paused.
   bool getPaused(void) const                      { return mState.mPaused; }
   
   // The time when the sim was updated.
   double getUpdateRealtime(void) const            { return mState.mUpdateRealtime; }
   // The time when the renderer was updated (on the sim thread).
   double getRenderRealtime(void) const            { return mState.mRenderRealtime; }
   
   // The render update deltaT.
   double getDeltaTRealtime(void) const            { return mState.mDeltaTRealtime; }
   
   // The sim's update length.
   double getLastUpdateLength(void) const          { return mState.mLastUpdateLength; }
   
   // Total real/game times will stop changing when paused.
   double getTotalRealtime(void) const             { return mState.mTotalRealtime; }
   double getTotalGametime(void) const             { return mState.mTotalGametime; }

private:
   struct BStateSnapshot
   {
      void clear(void) { Utils::ClearObj(*this); }
            
      uint mUpdateNumber;
      bool mPaused;
      double mUpdateRealtime;
      double mRenderRealtime;
      double mDeltaTRealtime;
      double mLastUpdateLength;
      double mTotalRealtime;
      double mTotalGametime;
   };
   
   BStateSnapshot mState;
   BStateSnapshot mPrevState;
};

extern BRenderTime gRenderTime;

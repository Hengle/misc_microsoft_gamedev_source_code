//==============================================================================
// modeviewer.h
//
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "mode.h"

#include "displayStats.h"

// Forward declarations
class BVisual;
class BWorld;
class BSquad;
class BUnit;
class BObject;


//==============================================================================
// BAnimationInfo
//==============================================================================
class BAnimationInfo
{
   public:   
      BSimString  mName;
      long        mAnimType;
      long        mAnimIndex;
};


//==============================================================================
// BModeViewer
//==============================================================================
class BModeViewer : public BMode
{
   public:   
      enum
      {
         cStateMain,
         cStateExit,
      };

      enum
      {
         cFlagGameInit,
         cFlagTerrainInit,
         cFlagPaused,
         cFlagSingleStep,
         cFlagFixedUpdate,
      };

      enum
      {
         cBackgroundModeGrid,
         cBackgroundModeTerrain,
         cBackgroundModeEmpty,

         cBackgroundModeMax
      };


                        BModeViewer(long modeType);
      virtual           ~BModeViewer();

      virtual bool      setup();

      virtual void      preEnter(BMode* lastMode);
      virtual void      enter(BMode* lastMode);
      virtual void      leave(BMode* newMode);

      virtual void      renderBegin();
      virtual void      render();
      virtual void      renderEnd();
      virtual void      update();
      virtual void      frameStart();
      virtual void      frameEnd();
      virtual bool      handleInput(long port, long event, long controlType, BInputEventDetail& detail);

      // Timing stuff
      void              setPaused(bool paused) { setFlag(cFlagPaused, paused); }
      bool              getPaused() const { return getFlag(cFlagPaused); }
      void              setFixedUpdate(float time) { setFlag(cFlagFixedUpdate, true); mFixedUpdateTime=time; }
      void              clearFixedUpdate() { setFlag(cFlagFixedUpdate, false); }

      void              setNextState(long state) { mNextState=state; }

      bool              calcUpdateTimes(int64& currentUpdateTime, float& currentUpdateLength);
      void              updateGame(int64 currentUpdateTime, float currentUpdateLength, bool updateSim);

   protected:

      bool              initGame();
      void              deinitGame();

      bool              initWorld();
      void              deinitWorld();

      void              setupError();

      void              renderGrid();
      void              renderViewerUI();
      void              renderUIShowStats();
      
      void              initModel();
      void              initCamera();

      void              incrementSelectedModel();
      void              decrementSelectedModel();

      void              playSelectedAnimation();
      void              incrementSelectedAnim();
      void              decrementSelectedAnim();

      void              incrementBackgroundMode();

      void              resetObjectPosition();

      long              mState;
      long              mNextState;

      BMatrix           mModelMatrix;
      float             mModelRotation;

      BVector           mLookAtPosition;

      float             mViewerYaw;
      float             mViewerPitch;
      float             mViewerZoom;
      float             mViewerPanX;
      float             mViewerPanY;
      float             mViewerDistance;

      bool              mShowDebugInfo;

      BWorld*           mpWorld;
      BSquad*           mpSquad;
      BUnit*            mpUnit;
      BObject*          mpObject;

      long              mCurrentProtoVisualGen;
      
      long              mSelectedModel;
      BDynamicSimArray<BSimString>     mModelList;

      long              mSelectedAnim;
      BDynamicSimArray<BAnimationInfo> mAnimList;

      BVector           mObjectCenter;
      BVector           mObjectSize;
      float             mBoundingBoxMaxSide;
            
      DWORD             mTimingTrackerUpdate;
      BTimingTracker*   mpTimingTracker;

      int64             mTimerFrequency;
      int64             mFrameStartTime;
      int64             mLastUpdateTime;
      int64             mCounterOverhead;
      int64             mLastUpdateLength;

      double            mTimerFrequencyFloat;
      float             mUpdateLength;
      float             mFrameLength;
      float             mFixedUpdateTime;
      
      long              mBackgroundMode;

      bool              mIsMotionExtractionEnabled;
};
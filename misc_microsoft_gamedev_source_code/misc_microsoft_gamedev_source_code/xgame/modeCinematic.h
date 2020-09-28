//==============================================================================
// modecinematic.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "mode.h"
#include "ui.h"
#include "uilist.h"
#include "renderThread.h"
#include "threading\eventDispatcher.h"
#include "gfxMoviePlayer.h"
#include "math\generalVector.h"

#include "displayStats.h"

// Forward declarations
class BWorld;
class BCamera;
class BCinematicManager;

//==============================================================================
// BModeCinematic
//==============================================================================
class BModeCinematic : public BMode
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

   BModeCinematic(long modeType);
   virtual           ~BModeCinematic();

   virtual bool      setup();
   virtual void      shutdown();

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

   void              toggleShowDebugInfo() { mShowDebugInfo = !mShowDebugInfo; }
   void              toggleShowErrorMsgInfo() { mShowErrorMsgInfo = !mShowErrorMsgInfo; }



   bool              calcUpdateTimes(int64& currentUpdateTime, float& currentUpdateLength);
   void              updateGame(int64 currentUpdateTime, float currentUpdateLength, bool updateSim);


protected:

   bool              initGame();
   void              deinitGame();

   bool              initWorld();
   void              deinitWorld();

   void              renderCinematicUI();
   void              renderUIShowStats();

   bool              handleInputFreeCamera(long port, long event, long controlType, BInputEventDetail& detail);
   void              updateFreeCamera(float elapsedTime);

   long              mState;
   long              mNextState;

   BUIList           mList;
   long              mLastMainItem;
   
   BWorld*           mpWorld;
   BCinematicManager*   mpCinematicManager;
   long              mCurrentCinematicIndex;


   BSimString        mLoadErrorsMsg;

   bool              mShowDebugInfo;
   bool              mShowErrorMsgInfo;

   bool              mLoaded;

   float             mStoredNearClipPlane;
   float             mStoredFarClipPlane;
   
   // Alternate Free Camera 
   BCamera*          mpFreeCamera;
   BVec3             mFreeCameraMovement;
   BVec3             mFreeCameraRotation;
   bool              mFreeCameraLModifier : 1;
   bool              mFreeCameraRModifier : 1;
   bool              mFreeCameraReset : 1;
   BVec3             mFreeCameraCurLoc;
   BVec3             mFreeCameraCurRot;
      
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

   DWORD             mSingleStepRepeatTime;
};


//==============================================================================
// cinematicManager.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once

#include "binkvideo.h"
#include "cinematicbars.h"

class BCinematic;
class BGeneralEventSubscriber;

//============================================================================
// BCinematicObject
//============================================================================
class BCinematicObject
{
public:

   BCinematic* mpCinematic;
   int         mID;
   
   BCinematicObject(BCinematic* pCinematic, int ID);
   ~BCinematicObject();
};


//============================================================================
// BCinematicManager
//============================================================================
class BCinematicManager
{
public:

   enum
   {
      cStatePaused,
      cStatePlaying
   };

   enum
   {
      cCameraModeLocked,
      cCameraModeFree
   };

   BCinematicManager();
   ~BCinematicManager();

   void     render();
   void     postRender();

   void     update(float deltaTime);

   long     addCinematic(BCinematic* pCinematic, int cinematicID);
   void     releaseCinematics();

   void     playCinematic(uint index, BSmallDynamicSimArray<BEntityID>* pPossessSquadList = NULL, bool* pPreRendered = NULL );
   void     playCinematicByID(int cinematicID, BSmallDynamicSimArray<BEntityID>* pPossessSquadList = NULL, bool* pPreRendered = NULL  );

   bool     isPlayingCinematic() { return mFlagCinematicPlaying; }

   void     fadeInLetterBox();
   void     fadeOutLetterBox();

   void     cancelPlayback();

   long     getNumCinematics() { return mCinematics.getSize(); }
   BCinematic *getCinematic(uint index);
   BCinematic *getCinematicByID(int cinematicID);

   bool     hasTriggerTagFired(uint index, uint tagId);
   bool     hasTriggerTagFiredByID(int cinematicID, uint tagId);

   BBinkVideoHandle getVideoHandle() const; 

   BGeneralEventSubscriber* getCinematicCompletedEventSubscriber() { return (mpCinematicCompleted); }
   void setCinematicCompletedEventSubscriber(BGeneralEventSubscriber* pCinematicCompleted) { mpCinematicCompleted = pCinematicCompleted; }

   bool     areBarsVisible() { return mBars.visible(); }
private:

   void     saveUserCamera();
   void     restoreUserCamera();

   int      cinematicIDToIndex(int cinematicID);
   int      indexToCinematicID(int index);

   BSmallDynamicSimArray<BCinematicObject*>     mCinematics;
   int                     mCurrentCinematic;
   bool                    mFlagCinematicPlaying:1;
   bool                    mSaveTimingAlertsOff:1;

   BVector  mUserCameraRight;
   BVector  mUserCameraDir;
   BVector  mUserCameraUp;
   BVector  mUserCameraLoc;
   float    mUserCameraFOV;
   BCinematicBars mBars;

   BGeneralEventSubscriber* mpCinematicCompleted;
};




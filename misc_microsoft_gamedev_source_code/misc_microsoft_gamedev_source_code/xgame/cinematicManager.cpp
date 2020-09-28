//==============================================================================
// cinematicManager.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================


// Includes
#include "common.h"
#include "cinematicManager.h"
#include "cinematic.h"
#include "user.h"
#include "usermanager.h"
#include "camera.h"
#include "config.h"
#include "configsgame.h"
#include "game.h"
#include "generaleventmanager.h"
#include "renderControl.h"
#include "modemanager.h"
#include "modegame.h"
#include "uimanager.h"
#include "binkInterface.h"

//============================================================================
// BCinematicObject::BCinematicObject
//============================================================================
BCinematicObject::BCinematicObject(BCinematic* pCinematic, int ID) :
   mpCinematic(pCinematic),
   mID(ID)
{
}

//============================================================================
// BCinematicObject::BCinematicObject
//============================================================================
BCinematicObject::~BCinematicObject()
{
}


//============================================================================
// BCinematicManager::BCinematicManager
//============================================================================
BCinematicManager::BCinematicManager() :
   mpCinematicCompleted(NULL)
{
   mCurrentCinematic = -1;
   mFlagCinematicPlaying = false;
   mSaveTimingAlertsOff = false;
   mBars.init();
}


//============================================================================
// BCinematicManager::~BCinematicManager
//============================================================================
BCinematicManager::~BCinematicManager()
{
   releaseCinematics();

   // The event manager handles clean up
   mpCinematicCompleted = NULL;
}


//============================================================================
// BCinematicManager::render
//============================================================================
void BCinematicManager::render()
{
   if(!mFlagCinematicPlaying)
      return;

   // render cinematic
   mCinematics[mCurrentCinematic]->mpCinematic->render();
}


//============================================================================
// BCinematicManager::postRender
//============================================================================
void BCinematicManager::postRender()
{

#ifndef BUILD_FINAL   
   if (!gConfig.isDefined(cConfigDisableCinematicBars))
#endif   
   {
      // render cinematic bars
      mBars.postrender();
   }
}

void BCinematicManager::fadeInLetterBox()
{
   BUser* pPrimaryUser = gUserManager.getPrimaryUser();
   //BUIContext* pPrimaryUIContext = pPrimaryUser->getUIContext();

   gUIManager->setupCinematicUI(true);
   pPrimaryUser->setFlagOverrideUnitPanelDisplay(true);


#ifndef BUILD_FINAL   
   if (!gConfig.isDefined(cConfigDisableCinematicBars))
#endif   
   {
      mBars.fadeIn();
   }
}

void BCinematicManager::fadeOutLetterBox()
{

#ifndef BUILD_FINAL   
   if (!gConfig.isDefined(cConfigDisableCinematicBars))
#endif   
   {
      mBars.fadeOut();
   }
}

//============================================================================
// BCinematicManager::update
//============================================================================
void BCinematicManager::update(float currentUpdateLength)
{
   mBars.update(currentUpdateLength);

   if(!mFlagCinematicPlaying)
      return;

   mCinematics[mCurrentCinematic]->mpCinematic->update(currentUpdateLength);

   // check if cinematic has ended
   if(mCinematics[mCurrentCinematic]->mpCinematic->getState() != BCinematic::cStatePlaying)
   {
      mFlagCinematicPlaying = false;
      fadeOutLetterBox();

#ifndef BUILD_FINAL
      if (gConfig.isDefined(cConfigWowRecord) && (gModeManager.getModeType() != BModeManager::cModeCinematic))
      {
         gRenderControl.endVideo();
         gModeManager.getModeGame()->clearFixedUpdate();
      }
#endif


#ifndef BUILD_FINAL
      if (!mSaveTimingAlertsOff)
         gConfig.remove(cConfigTimingAlertsOFF);
#endif
      gUIManager->setupCinematicUI(false);

      BUser* pUser=gUserManager.getUser(BUserManager::cPrimaryUser);      
      pUser->resetUserMode();

      // restore split screen
      if (gGame.isSplitScreenAvailable() && gUserManager.getSecondaryUser()->getFlagUserActive())
         gGame.setSplitScreen(true);
      
      // restore camera location
      restoreUserCamera();

      gGeneralEventManager.eventTrigger(BEventDefinitions::cCinematicCompleted, cInvalidPlayerID);      
   }
}


//============================================================================
// BCinematicManager::addCinematic
//============================================================================
long BCinematicManager::addCinematic(BCinematic* pCinematic, int cinematicID)
{
   mCinematics.pushBack(new BCinematicObject(pCinematic, cinematicID));

   return(mCinematics.getSize() - 1);
}


//==============================================================================
// BCinematicManager::releaseCinematics
//==============================================================================
void BCinematicManager::releaseCinematics()
{
   uint count=mCinematics.getSize();
   for(uint i=0; i<count; i++)
   {
      BCinematic* pCinematic=mCinematics[i]->mpCinematic;
      delete pCinematic;
   }
   mCinematics.resize(0);
}


//==============================================================================
// BCinematicManager::playCinematic
//==============================================================================
void BCinematicManager::playCinematic(uint index, BSmallDynamicSimArray<BEntityID>* pPossessSquadList, bool* pPreRendered)
{
   if(index >= mCinematics.getSize())
      return;

   // turn off split screen
   gGame.setSplitScreen(false);

   BUser* pUser=gUserManager.getUser(BUserManager::cPrimaryUser);
   pUser->resetUserMode();
   pUser->changeMode(BUser::cUserModeCinematic);
   
   mCinematics[index]->mpCinematic->rewindToStart();
   mCinematics[index]->mpCinematic->play(pPossessSquadList, pPreRendered);

   mCurrentCinematic = index;

   mFlagCinematicPlaying = true;
   fadeInLetterBox();

   // cache camera location
   saveUserCamera();
   
#ifndef BUILD_FINAL
   if (gConfig.isDefined(cConfigWowRecord) && (gModeManager.getModeType() != BModeManager::cModeCinematic))
   {
      BSimString filename;
      strPathGetFilename(mCinematics[index]->mpCinematic->getFilename(), filename);
      filename.removeExtension();
      gModeManager.getModeGame()->setFixedUpdate(1.0f/30.0f);
      const bool rawVideo = true;
      gRenderControl.startVideo(filename, gConfig.isDefined(cConfigWowDownsample), true, 30.0f, rawVideo);
   }
#endif

#ifndef BUILD_FINAL
   mSaveTimingAlertsOff = gConfig.isDefined(cConfigTimingAlertsOFF);
   if (!mSaveTimingAlertsOff)
      gConfig.define(cConfigTimingAlertsOFF);
#endif

   gUIManager->setupCinematicUI(true);
}

//==============================================================================
// BCinematicManager::playCinematicByID
//==============================================================================
void BCinematicManager::playCinematicByID(int cinematicID, BSmallDynamicSimArray<BEntityID>* pPossessSquadList, bool* pPreRendered)
{
   int index = cinematicIDToIndex(cinematicID);
   playCinematic(index, pPossessSquadList, pPreRendered);
}



//==============================================================================
// BCinematicManager::getCinematic
//==============================================================================
BCinematic* BCinematicManager::getCinematic(uint index)
{
   if(index >= mCinematics.getSize())
      return NULL;

   return(mCinematics[index]->mpCinematic);
}

//==============================================================================
// BCinematicManager::getCinematicByID
//==============================================================================
BCinematic* BCinematicManager::getCinematicByID(int cinematicID)
{
   int index = cinematicIDToIndex(cinematicID);
   return getCinematic(index);
}

//==============================================================================
// BCinematicManager::hasTriggerTagFired
//==============================================================================
bool BCinematicManager::hasTriggerTagFired(uint index, uint tagId)
{
   if(index >= mCinematics.getSize())
      return false;

   return(mCinematics[index]->mpCinematic->hasTriggerTagFired(tagId));
}

//==============================================================================
// BCinematicManager::hasTriggerTagFiredByID
//==============================================================================
bool BCinematicManager::hasTriggerTagFiredByID(int cinematicID, uint tagId)
{
   int index = cinematicIDToIndex(cinematicID);
   return(hasTriggerTagFired(index, tagId));
}

//==============================================================================
// BCinematicManager::saveUserCamera
//==============================================================================
void BCinematicManager::saveUserCamera()
{
//-- FIXING PREFIX BUG ID 2716
   const BCamera* camera=gUserManager.getUser(BUserManager::cPrimaryUser)->getCamera();
//--

   mUserCameraRight = camera->getCameraRight();
   mUserCameraDir = camera->getCameraDir();
   mUserCameraUp = camera->getCameraUp();
   mUserCameraLoc = camera->getCameraLoc();
   mUserCameraFOV = camera->getFOV();
}


//==============================================================================
// BCinematicManager::restoreUserCamera
//==============================================================================
void BCinematicManager::restoreUserCamera()
{
   BCamera* camera=gUserManager.getUser(BUserManager::cPrimaryUser)->getCamera();

   camera->setCameraRight(mUserCameraRight);
   camera->setCameraDir(mUserCameraDir);
   camera->setCameraUp(mUserCameraUp);
   camera->setCameraLoc(mUserCameraLoc);
   camera->setFOV(mUserCameraFOV);
}


//==============================================================================
// BCinematicManager::cinematicIDToIndex
//==============================================================================
int BCinematicManager::cinematicIDToIndex(int cinematicID)
{
   if(mCinematics.getSize() <= 0)
      return -1;

   int count = mCinematics.getSize();
   for(int i = 0; i < count; i++)
   {
      if(mCinematics[i]->mID == cinematicID)
         return i;
   }

   return -1;
}

//==============================================================================
// BCinematicManager::cinematicIDToIndex
//==============================================================================
int BCinematicManager::indexToCinematicID(int index)
{   
   if((index < 0) || (index >= (int)mCinematics.getSize()))
      return -1;

   return(mCinematics[index]->mID);
}

//==============================================================================
//==============================================================================
BBinkVideoHandle BCinematicManager::getVideoHandle() const
{
   if (!mFlagCinematicPlaying)
      return cInvalidVideoHandle;
   return mCinematics[mCurrentCinematic]->mpCinematic->getVideoHandle();
}


//==============================================================================
//==============================================================================
void BCinematicManager::cancelPlayback()
{
   if (!mFlagCinematicPlaying)
      return;
   BCinematic* pCinematic = mCinematics[mCurrentCinematic]->mpCinematic;
   BBinkVideoHandle videoHandle = pCinematic->getVideoHandle();
   if (videoHandle != cInvalidVideoHandle)
      gBinkInterface.stopVideo(videoHandle,false); //CLM [11.05.08] NO ONE CALLS THIS, so i'm hard coding it to false.
}

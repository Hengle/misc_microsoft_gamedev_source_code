//==============================================================================
// worldsoundmanager.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "worldsoundmanager.h"
#include "camera.h"
#include "config.h"
#include "configsgame.h"
#include "object.h"
#include "objectmanager.h"
#include "soundmanager.h"
#include "user.h"
#include "usermanager.h"
#include "visual.h"
#include "world.h"
#include "render.h"
#include "gamedirectories.h"
#include "modemanager.h"
#include "uigame.h"
#include "protoobject.h"
#include "visiblemap.h"
#include "configsgame.h"
#include "techtree.h"
#include "ChatManager.h"
#include "game.h"
#include "protosquad.h"
#include "modegame.h"

// Constants
static const uint    cMaxWorldSoundsLower = 400;
static const uint    cMaxWorldSoundsUpper = 450;
static const float   cMaxSoundDistance = 300.0f;
static const long    cMinWWiseID = 10; //-- 0, 1 or reserved, using 2-9 for const objects
static const long    cMaxWWiseID = cMaxWorldSoundsUpper + cMinWWiseID;
static const long    cMaxExtendedBanks = 5;

//==============================================================================
// BWorldSoundManager::BWorldSoundManager
//==============================================================================
BWorldSoundManager::BWorldSoundManager() :
   mWorldSoundList(),
   mListenerPosition(cInvalidVector), 
   mListenerDirection(cInvalidVector),
   mListenerPosition2(cInvalidVector), 
   mListenerDirection2(cInvalidVector),
   mListenerFactor(55.0f),
   mRearAdjustmentFactor(0.0f),
   mPainRate(0.0f),
   mFogRTPCFadeIn(0.02f),
   mFogRTPCFadeOut(0.02f),
   mEnginePitchChangeRate(0.02f),
   mMaxPitchChangePerFrame(0.5f),
   mAttackSev1Value(0.0f),
   mAttackSev2Value(0.0f),
   mTimeToIdleChat(0)
{

   for(uint i=0; i < cSquadSoundStateMax; i++)
   {
      mMaxChatterTime[i] = -1.0f;
      mMinChatterTime[i] = -1.0f;      
      mGlobalMaxChatterTime[i] = -1.0f;
      mGlobalMinChatterTime[i] = -1.0f;

      mGlobalChatterTimer[i] = 0.0f;
   }

   reset(true);

   gSoundManager.addEventHandler(this);
}

//==============================================================================
// BWorldSoundManager::~BWorldSoundManager
//==============================================================================
BWorldSoundManager::~BWorldSoundManager()
{
   gSoundManager.playCue("unmute_music");

   reset(false);
   gSoundManager.removeEventHandler(this);
}

//==============================================================================
// BWorldSoundManager::setup
//==============================================================================
bool BWorldSoundManager::setup()
{
   ASSERT_THREAD(cThreadIndexSim);

   //-- Load limit information from xml
   bool result = loadXML();
   if(!result)
      return false;

   return true;
}

//==============================================================================
// BWorldSoundManager::reset
//==============================================================================
void BWorldSoundManager::reset(bool startup)
{
   ASSERT_THREAD(cThreadIndexSim);

   mFlagListenerSet=false;
   mFlagListenerUpdated=false;
   mFlagFirstUpdate=true;
   mFlagIntroMusicStarted=false;
   mFlagLoopMusicStarted=false;
   mFlagPaused=false;
   mFlagGameStartDelay=false;

   mCurGPUFrame = 0;
   
   gSoundManager.worldReset();
  
   //-- Call the stopall event. This may want to be specific stops for specific scenarios eventually.
   gUIGame.playSound(BSoundManager::cSoundStopAll);


   //-- Clear and remove used sound objects
   uint highWaterMark = mWorldSoundList.getHighWaterMark();
   for(uint i=0; i < highWaterMark; i++)
   {
      if(mWorldSoundList.isValidIndex(i) == false)
         continue;

      BWorldSound* pSound = mWorldSoundList.get(i);
      if(pSound && pSound->mCueHandle!=cInvalidCueHandle)
      {
         if(pSound->mCueHandle != cInvalidCueHandle)
         {
            pSound->mFlagStopped = true;
            pSound->mCueHandle=cInvalidCueHandle;
            if(pSound->mStopCueIndex!=cInvalidCueIndex)
               gSoundManager.playCue(pSound->mStopCueIndex, pSound->mWiseObjectID);
#ifdef SOUND_REREGISTER_OBJECTS
            gSoundManager.removeObject(pSound->mWiseObjectID);
#endif
         }

         //-- Reset the sound
         pSound->reset();

         //-- Mark as relesed
         mWorldSoundList.release(i);
      }
   }
   mWorldSoundList.clear();

   //-- Remove world object ID's
#ifdef SOUND_REREGISTER_OBJECTS
   for(uint i=0; i < mFreeWWiseIDs.getSize(); i++)
   {      
      gSoundManager.removeObject(mFreeWWiseIDs[i]);
   }
#endif
   mFreeWWiseIDs.clear();

   //-- Clear the playsound sound handle map
   mPlayingSoundsMap.clear();

   //-- Remove all items from the world sounds lookup table
   mWorldSoundObjectsTable.removeAll();

   mMusicManager.reset();

   //-- Unload our extended sound banks
   long numLoadedBanks = mExtendedSoundBanks.getNumber();
   for(long i = numLoadedBanks-1; i >= 0; i--)
   {
      unloadExtendedSoundBank(i);
   }

   if(startup)
   {
      //-- Create the object ID's we're going to use
      for(long i=cMaxWWiseID; i > cMinWWiseID; i--)
      {
         mFreeWWiseIDs.add(i);
#ifdef SOUND_REREGISTER_OBJECTS
         gSoundManager.addObject(i);
#endif
      }

      //-- Pause the sound manager until rendering starts
      setGameStartDelay(true);
      gSoundManager.setGameStartDelay(true);
      mMusicManager.setGameStartDelay(true);
   }
}

//==============================================================================
// BWorldSoundManager::update
//==============================================================================
void BWorldSoundManager::update(float elapsed)
{
   ASSERT_THREAD(cThreadIndexSim);

   // Early out if in cinematic mode
   if(gModeManager.getModeType() == BModeManager::cModeCinematic)
   {
      return;
   }

   bool playingVideo = gModeManager.getModeGame()->getFlagPlayingVideo();

   if(mFlagGameStartDelay)
   {
      if(gRenderThread.getCurGPUFrame() > mCurGPUFrame + 1)
      {
         //-- Play the sounds which have been queued.
         setGameStartDelay(false);         
      }
   }

   if (!playingVideo)
   {
      updateListener();

      updateSounds();
   
      updateTimers(elapsed);
   }

   mMusicManager.update();


   if(mFlagFirstUpdate)
   {
      //-- DJBFIXME: Temp for audio dev
      if(gConfig.isDefined(cConfigEnableMusic) == false)
      {
         BCueIndex cue = gSoundManager.getCueIndexByEnum(BSoundManager::cSoundMuteMusic);
         gSoundManager.playCue(cue);
      }
   }

#ifdef SOUND_TEST
   testAllSounds();
   validateData();
#endif 

   mFlagFirstUpdate = false;
}

//==============================================================================
// BWorldSoundManager::render
//==============================================================================
void BWorldSoundManager::render()
{
   //-- Draw a grid on the terrain to give a sense of distance
   const float maxDistance = 1000.0f;
   const float gridSpacing = 50.0f;
   const float gridColoringDist = 250.0f;
   const float maxDistanceSqr = maxDistance * maxDistance;   
   BVector cameraPos = gRenderDraw.getMainSceneMatrixTracker().getWorldCamPos();

   float numXdataTiles = (float)gTerrainSimRep.getNumXDataTiles();
   numXdataTiles *= gTerrainSimRep.getDataTileScale();


   for (float vx = 0; vx < numXdataTiles; vx+=gridSpacing)
   {
      for (float vz = 0; vz < numXdataTiles; vz+=gridSpacing)
      {         
         float vx2 = vx + gridSpacing;
         float vz2 = vz + gridSpacing;

         BVector p1 = BVector(vx, 0.0f, vz);
         BVector p2 = BVector(vx2, 0.0f, vz);
         BVector p3 = BVector(vx2, 0.0f, vz2);
         BVector p4 = BVector(vx, 0.0f, vz2);

         if (cameraPos.xzDistanceSqr(p1) > maxDistanceSqr)
            continue;

         if(!gRender.getViewParams().isPointOnScreen(p1) && !gRender.getViewParams().isPointOnScreen(p3))
            continue;

         gTerrainSimRep.addDebugLineOverTerrain(p1, p2, cDWORDGreen, cDWORDGreen, 0.1f);
         gTerrainSimRep.addDebugLineOverTerrain(p1, p4, cDWORDGreen, cDWORDGreen, 0.1f);
         gTerrainSimRep.addDebugLineOverTerrain(p2, p3, cDWORDGreen, cDWORDGreen, 0.1f);
         gTerrainSimRep.addDebugLineOverTerrain(p4, p3, cDWORDGreen, cDWORDGreen, 0.1f);
      }
   }

   for (float vx = 0; vx < numXdataTiles; vx+=gridColoringDist)
   {
      for (float vz = 0; vz < numXdataTiles; vz+=gridColoringDist)
      {         
         float vx2 = vx + gridColoringDist;
         float vz2 = vz + gridColoringDist;

         BVector p1 = BVector(vx, 0.0f, vz);
         BVector p2 = BVector(vx2, 0.0f, vz);
         BVector p3 = BVector(vx2, 0.0f, vz2);
         BVector p4 = BVector(vx, 0.0f, vz2);

         if (cameraPos.xzDistanceSqr(p1) > maxDistanceSqr)
            continue;

         if(!gRender.getViewParams().isPointOnScreen(p1) && !gRender.getViewParams().isPointOnScreen(p3))
            continue;

         gTerrainSimRep.addDebugThickLineOverTerrain(p1, p2, 1.0f, cDWORDRed, cDWORDRed, 0.1f);
         gTerrainSimRep.addDebugThickLineOverTerrain(p1, p4, 1.0f, cDWORDRed, cDWORDRed, 0.1f);
         gTerrainSimRep.addDebugThickLineOverTerrain(p2, p3, 1.0f, cDWORDRed, cDWORDRed, 0.1f);
         gTerrainSimRep.addDebugThickLineOverTerrain(p4, p3, 1.0f, cDWORDRed, cDWORDRed, 0.1f);
      }
   }
}

//==============================================================================
// BWorldSoundManager::updateListener
//==============================================================================
void BWorldSoundManager::updateListener()
{
   ASSERT_THREAD(cThreadIndexSim);
   
   setFlagListenerUpdated(false);

   if(!getFlagListenerSet())
   {
      BVector newPos;      
      BVector up;
      BVector velocity;      
      calcListenerPosition(gUserManager.getPrimaryUser(), mListenerPosition, newPos, mListenerDirection, up, velocity);      
      mListenerPosition = newPos;
      gSoundManager.updateListener(0, mListenerPosition, mListenerDirection, up, velocity);

      if (gGame.isSplitScreen())
      {
         calcListenerPosition(gUserManager.getSecondaryUser(), mListenerPosition2, newPos, mListenerDirection2, up, velocity);      
         mListenerPosition2 = newPos;
         gSoundManager.updateListener(1, mListenerPosition2, mListenerDirection2, up, velocity);
      }

      setFlagListenerSet(true);
      setFlagListenerUpdated(true);
   }
   else
   {
      float elapsedTime=gWorld->getLastUpdateLengthFloat();
      if(elapsedTime>0.0f)
      {         
         BVector newPos;         
         BVector up;
         BVector velocity;
         calcListenerPosition(gUserManager.getPrimaryUser(), mListenerPosition, newPos,mListenerDirection, up, velocity);
         if(newPos!=mListenerPosition)
         {            
            mListenerPosition = newPos;
            gSoundManager.updateListener(0, newPos, mListenerDirection, up, velocity);
            setFlagListenerUpdated(true);
         }

         if (gGame.isSplitScreen())
         {
            calcListenerPosition(gUserManager.getSecondaryUser(), mListenerPosition2, newPos,mListenerDirection2, up, velocity);
            if(newPos!=mListenerPosition2)
            {            
               mListenerPosition2 = newPos;
               gSoundManager.updateListener(1, newPos, mListenerDirection2, up, velocity);
               setFlagListenerUpdated(true);
            }
         }
      }
   }
}

//==============================================================================
// BWorldSoundManager::calcListenerPosition
//==============================================================================
void BWorldSoundManager::calcListenerPosition(BUser* pUser, BVector oldPos, BVector &newPos, BVector &forward, BVector &up, BVector& velocity)
{
   if(!pUser)
      return;

//-- FIXING PREFIX BUG ID 3552
   const BCamera* pCamera=pUser->getCamera();
//--
   if(!pCamera)
      return;   

   newPos = pCamera->getCameraLoc();   
   BVector cameraDir = pCamera->getCameraDir();

   if(gWorld->isPlayingCinematic() == false)
   {
      //-- Place the listener between the camera and the cursor
      BVector intersect;
      bool result = gTerrainSimRep.rayIntersectsCamera(newPos, cameraDir, intersect);   
      
      float distance = intersect.distance(newPos);
      if(result == true && distance < mListenerFactor)
         cameraDir.scale(distance);      
      else
         cameraDir.scale(mListenerFactor);
         

      newPos += cameraDir;   
   }


   //-- Velocity
   velocity=cOriginVector;

   //-- Set the listener orientation
   forward = pCamera->getCameraDir();
   forward.y = 0;
   up = pCamera->getCameraUp();

   if(gConfig.isDefined(cConfigDisplayListener))
      gpDebugPrimitives->addDebugSphere(newPos, 1.0f, cDWORDYellow);
}

//==============================================================================
// BWorldSoundManager::processSoundEvents
//==============================================================================
void BWorldSoundManager::processSoundEvent(BSoundEventParams& eventParams)
{
   ASSERT_THREAD(cThreadIndexSim);

   //mMusicManager.processSoundEvent(event);

   if (eventParams.eventType == cSoundEventStop)
   {
      // update the chat manager to let it know the sound has stopped
      if (gWorld && gWorld->getChatManager())
         gWorld->getChatManager()->processSoundStoppedEvent(eventParams);

      //-- Lookup the sound in the playing cue map
      long soundID = -1;
      BSoundHandleToSoundIDHashMap::iterator iterator = mPlayingSoundsMap.find(eventParams.in_playingID);         
      if (iterator != mPlayingSoundsMap.end())
      {
         soundID = iterator->second;         
      }

      //-- did we find it?
      if(soundID != -1)
      {  
         //-- Get the index into mWorldSoundList from the ID
         long index = WSIDINDEX(soundID);
         BASSERT(mWorldSoundList.isValidIndex(index));
         BWorldSound* pSound = mWorldSoundList.get(index);
         if(pSound)
         {
            BASSERT(pSound->mID == soundID);
            pSound->mFlagStopped = true;
         }
      }
   }
   else if(eventParams.eventType == cSoundEventBankLoaded)
   {
      //-- Go through the extended bank list and determine which one was loaded
      for (long i = mExtendedSoundBanks.getNumber() - 1; i >= 0; i--)
      {
         BExtendedSoundBankInfo& bankInfo = mExtendedSoundBanks[i];

         if(bankInfo.mLoaded == true)
            continue;

         if(bankInfo.mBankID == eventParams.in_bankID)
         {
            if(eventParams.in_status != AK_Success)
            {
               //-- Failed to load the bank, remove it from the pending bank list
               mExtendedSoundBanks.removeIndex(i);
            }
            else
            {
               onExtendedBankLoaded(bankInfo);    
            }               
         }
      }
   }
}     

//==============================================================================
// BWorldSoundManager::updateSounds
//==============================================================================
void BWorldSoundManager::updateSounds()
{
   ASSERT_THREAD(cThreadIndexSim);  
         
   BEntity* pEntity = NULL;
   bool listenerUpdated=getFlagListenerUpdated();

   uint highWaterMark = mWorldSoundList.getHighWaterMark();
   for(uint i=0; i < highWaterMark; i++)
   {
      if(mWorldSoundList.isValidIndex(i) == false)
         continue;
     
      BWorldSound* pSound = mWorldSoundList.get(i);
      if(!pSound)
         continue;

      if(pSound->mCueHandle == cInvalidCueHandle)
      {
         BDEBUG_ASSERT(0);
         pSound->mCueHandle = gSoundManager.playCue(pSound->mCueIndex, pSound->mWiseObjectID);
         if(pSound->mCueHandle != cInvalidCueHandle)
         {            
            updateSound3D(pSound);            
         }
         else
            internalRemoveSound(pSound);
         continue;
      }


      if(pSound->mFlagStopped)
      {
         internalRemoveSound(pSound);
         continue;
      }

      if(pSound->mEntityID==cInvalidObjectID)
      {
         if(listenerUpdated)            
         {
            updateSound3D(pSound);            
         }
      }
      else
      {
         pEntity = gWorld->getEntity(pSound->mEntityID);
         if (!pEntity)
         {
            internalRemoveSound(pSound);
            continue;
         }
         bool update=listenerUpdated;
         BVector position=pEntity->getPosition();
         if(pSound->mBoneHandle!=-1)
         {
            BObject *pObject = gWorld->getObject(pSound->mEntityID);
            if(pObject)
            {
//-- FIXING PREFIX BUG ID 3541
               const BVisual* pVisual=pObject->getVisual();
//--
               if(pVisual)
               {
                  BVector bonePos;
                  if(pVisual->getBone(pSound->mBoneHandle, &bonePos))
                  {                     
                     BMatrix worldMat;
                     pObject->getWorldMatrix(worldMat);
                     worldMat.transformVectorAsPoint(bonePos, position);
                  }
               }
            }
         }

         if(pSound->mPosition!=position)
         {           
            pSound->mPosition=position;
            update=true;
         }         
         
         if(update || pSound->mFlagForceUpdate)
         {            
            updateSound3D(pSound);            
         }

         if( pSound->getFlagProcessFOW() )
         {
            const BUser * const pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
            if( pUser )
            {
               gSoundManager.updateFOWvolume( pSound->mWiseObjectID, pSound->mFOWVolume, pEntity->isVisible(pUser->getTeamID()), gWorld->getLastUpdateRealtimeDelta(), mFogRTPCFadeIn, mFogRTPCFadeOut ); 
            }
         }

         if( pSound->getFlagProcessEngine() && pEntity )
         {
            //gConsoleOutput.debug("vel: %f", pEntity->getVelocity().lengthEstimate() );
            gSoundManager.updateEnginePitch( pSound->mWiseObjectID, pSound->mEnginePitch, pEntity->getVelocity().lengthEstimate(), gWorld->getLastUpdateRealtimeDelta(), mEnginePitchChangeRate, mMaxPitchChangePerFrame ); 
         }

      }

      #ifndef BUILD_FINAL
      if(gConfig.isDefined(cConfigDisplaySounds))
      {
         //-- Display Sound Position
         float distToListner = pSound->mPosition.distance(mListenerPosition);
         gpDebugPrimitives->addDebugSphere(pSound->mPosition, 1.0f, cDWORDRed);
         BSimString output;
         output.format("event: %d\n wwiseid: %d\n dist: %4.0f\n", pSound->mCueIndex, pSound->mWiseObjectID, distToListner);
         BVector pos = pSound->mPosition;
         pos.y += 1.0f;
         gpDebugPrimitives->addDebugText(output, pos, 0.4f, cDWORDRed);
      }
      #endif
   }
}

//==============================================================================
// BWorldSoundManager::addSound
//==============================================================================
void BWorldSoundManager::addSound(const BVector& position, BCueIndex cueIndex, bool checkFOW, BCueIndex stopCueIndex, bool checkSoundRadius, bool canDrop, BRTPCInitArray *rtpcArray)
{
   ASSERT_THREAD(cThreadIndexSim);
   if(gConfig.isDefined(cConfigNoSound))
      return;
   
   internalAddSound(cInvalidObjectID, -1, position, cOriginVector, cueIndex, checkFOW, stopCueIndex, checkSoundRadius, canDrop, rtpcArray);
}

//==============================================================================
// BWorldSoundManager::addSound
//==============================================================================
void BWorldSoundManager::addSound(BEntity* pEntity, long boneHandle, BCueIndex cueIndex, bool checkFOW, BCueIndex stopCueIndex, bool checkSoundRadius, bool canDrop, BRTPCInitArray *rtpcArray)
{
   //DJBFIXME: temp remove all bone handles
   boneHandle = -1;

   ASSERT_THREAD(cThreadIndexSim);  

   if(gConfig.isDefined(cConfigNoSound))
      return;

   if(!pEntity)
      return;
   bool result=internalAddSound(pEntity->getID(), boneHandle, pEntity->getPosition(), pEntity->getVelocity(), cueIndex, checkFOW, stopCueIndex, checkSoundRadius, canDrop, rtpcArray);
   if(result == false)
      return;
   pEntity->setFlagHasSounds(true);
}

//==============================================================================
// BWorldSoundManager::internalAddSound
//==============================================================================
bool BWorldSoundManager::internalAddSound(BEntityID entityID, long boneHandle, const BVector& position, const BVector& velocity, BCueIndex cueIndex, bool checkFOW, BCueIndex stopCueIndex, bool checkSoundRadius, bool canDrop, BRTPCInitArray *rtpcArray)
{
   ASSERT_THREAD(cThreadIndexSim);

   if(mFlagGameStartDelay)
   {
      BQueuedSoundInfo info;
      info.mEntityID = entityID;
      info.mBoneHandle = boneHandle;
      info.mPosition = position;
      info.mVelocity = velocity;
      info.mCueIndex = cueIndex;
      info.mCheckFOW = checkFOW;
      info.mStopCueIndex = stopCueIndex;
      info.mCheckSoundRadius = checkSoundRadius;
      info.mCanDrop = canDrop;
      mQueuedSoundsToAddWhenGameStarts.add(info);
      return true;
   }


   if(mWorldSoundList.getNumberAllocated() > cMaxWorldSoundsUpper)
   {
      float gameSpeed=1.0f;
      gConfig.get(cConfigGameSpeed, &gameSpeed);
      if(gameSpeed==1.0f)
      {
#ifndef BUILD_FINAL
         gConsoleOutput.fileManager("Too many critical world sounds!");
         dumpSoundEventCounts();
#endif
         BASSERTM(0, "Too many world sounds!");
      }
      
      return false;
   }
   //-- Cap off anim events at lower limit, so we have room for stop events which need to make it through
   if(canDrop && mWorldSoundList.getNumberAllocated() > cMaxWorldSoundsLower)
   {
#ifndef BUILD_FINAL
      if(gConfig.isDefined(cConfigCatchWorldSoundLimiting))
      {
         gConsoleOutput.fileManager("Too many regular world sounds!");
         dumpSoundEventCounts();
         BASSERTM(0, "Too many regular world sounds!");
      }
#endif
      return false;
   }
   
   if(cueIndex==cInvalidCueIndex)
      return false;      

   if(checkFOW || checkSoundRadius)
   {
      BTeamID teamID = gUserManager.getUser(BUserManager::cPrimaryUser)->getTeamID();
      BTeamID teamID2 = -1;
      if(gGame.isSplitScreen())
         teamID2 = gUserManager.getUser(BUserManager::cSecondaryUser)->getTeamID();

//-- FIXING PREFIX BUG ID 3545
      const BEntity* pEntity = NULL;
//--
      BVector checkPos;
      if(entityID==cInvalidObjectID)
      {
         checkPos=position;
      }
      else
      {
         pEntity = gWorld->getEntity(entityID);
         if (!pEntity)
            return false;
         else
            checkPos = pEntity->getPosition();
      }

       if(checkSoundRadius)
      {
         float dist = checkPos.xzDistance(mListenerPosition);
         if (gGame.isSplitScreen())
         {
            float dist2 = checkPos.xzDistance(mListenerPosition2);
            if (dist2 < dist)
               dist = dist2;
         }
         if(dist>cMaxSoundDistance)
            return false;
      }
      
      if(checkFOW)
      {
         if(pEntity)
         {
            //-- Object was specified, can we see the object?
            BObject *pObject = gWorld->getObject(entityID);
            if(pObject)
            {
               if(pObject->isVisible(teamID) == false)
               {
                  if(teamID2 == -1)
                     return false;               
                  else if(pObject->isVisible(teamID2) == false)
                     return false;
               }
            }
            else  //-- Squad?
            {
               BSquad *pSquad = gWorld->getSquad(entityID);
               if(pSquad)
               {
                  if(pSquad->isVisible(teamID) == false)
                   {
                     if(teamID2 == -1)
                        return false;               
                     else if(pSquad->isVisible(teamID2) == false)
                        return false;
                   }
               }
            }
         }
         else
         {
            //-- Determine if we have visiblity to the position, since no object was specified
            XMVECTOR simPosition = __vctsxs(XMVectorMultiply(checkPos, XMVectorReplicate(gTerrainSimRep.getReciprocalDataTileScale())), 0);
            long simX = simPosition.u[0];
            long simZ = simPosition.u[2];         
            if(gWorld->getFlagAllVisible() == false)
            {
               if (simX>=0 && simZ>=0 && simX<gVisibleMap.getMaxXTiles() && simZ<gVisibleMap.getMaxZTiles())
               {
                  if (!(gVisibleMap.getVisibility(simX, simZ) & gVisibleMap.getTeamFogOffMask(teamID)))
                  {
                     if(teamID2 == -1)
                        return false;
                     else
                        if(!(gVisibleMap.getVisibility(simX, simZ) & gVisibleMap.getTeamFogOffMask(teamID2)))
                           return false;
                  }
               }
            }
         }
      }            
   }

   //-- Aqcuire new world sound and assign ID
   uint idIndex=0;
   BWorldSound* pSound=mWorldSoundList.acquire(idIndex);      

   if(idIndex > cWSIDIndexMax || !pSound)
   {
      BASSERT(0);
      return false;      
   }

   //-- If the ID is -1, then it was never set, so set it now with a count of 0
   if(pSound->mID == -1)
      pSound->mID = WSID(0, idIndex);
   
   pSound->mEntityID = entityID;
   pSound->mBoneHandle = boneHandle;
   pSound->mCueIndex = cueIndex;
   pSound->mStopCueIndex = stopCueIndex;
   pSound->mCueHandle = cInvalidCueHandle;
   pSound->mPosition = position;   

   pSound->mDistanceToListener = mListenerPosition.distance(pSound->mPosition);

   if (gGame.isSplitScreen())
   {
      float dist2 = mListenerPosition2.distance(pSound->mPosition);
      if (dist2 < pSound->mDistanceToListener)
         pSound->mDistanceToListener = dist2;
   }

   assignWiseObjectID(pSound);

   if(pSound->mWiseObjectID == cInvalidWwiseObjectID)
   {
      BASSERT(0);
      internalRemoveSound(pSound);
      return false;
   }

   //Process the RTPC init values now that we have the wise object id.
   bool processFOW = false;
   bool processEngine = false;
   if(rtpcArray)
   {
      for(int i=0; i<rtpcArray->getNumber(); i++)
      {
         if( rtpcArray->get(i).mRTPCid == AK::GAME_PARAMETERS::FOW_DISTANCE )
            processFOW = true;
         if( rtpcArray->get(i).mRTPCid == AK::GAME_PARAMETERS::ENGINE_VELOCITY )
            processEngine = true;
         gSoundManager.setRTPCValue(pSound->mWiseObjectID, rtpcArray->get(i).mRTPCid, rtpcArray->get(i).mInitValue);
      }
   }
   pSound->setFlagProcessFOW(processFOW);
   pSound->setFlagProcessEngine(processEngine);

   //-- Play the sound
   if(pSound->mCueHandle == cInvalidCueHandle)
   {
      pSound->mCueHandle = gSoundManager.playCue(pSound->mCueIndex, pSound->mWiseObjectID);
      if(pSound->mCueHandle != cInvalidCueHandle)
      {  
         //-- Add the playing sound into the playing sound hashmap, for easy lookup when the sound ends
         mPlayingSoundsMap.insert(pSound->mCueHandle, pSound->mID); 


         #ifdef SOUND_TEST   
         validateData();
         #endif 

         //-- Update the sound position
         updateSound3D(pSound);            

         //Update fow if we are using it
         if (processFOW && entityID != cInvalidObjectID)
         {
            BEntity *pEntity = gWorld->getEntity(entityID);
            if (pEntity)
            {
               const BUser * const pUser = gUserManager.getUser(BUserManager::cPrimaryUser);        
               if (pUser)
               {
                  bool isVisible = pEntity->isVisible(pUser->getTeamID());
                  if(gGame.isSplitScreen())
                     isVisible |= pEntity->isVisible(gUserManager.getUser(BUserManager::cSecondaryUser)->getTeamID());
                  gSoundManager.updateFOWvolume(pSound->mWiseObjectID, pSound->mFOWVolume, pEntity->isVisible(pUser->getTeamID()), gWorld->getLastUpdateRealtimeDelta(), mFogRTPCFadeIn, mFogRTPCFadeOut );
               }
            }
         }
      }
      else
      {
         internalRemoveSound(pSound);
         return false;
      }
   }



   return true;
}

//==============================================================================
// BWorldSoundManager::removeSounds
//==============================================================================
void BWorldSoundManager::removeSounds(BEntityID entityID)
{
   ASSERT_THREAD(cThreadIndexSim);
   
//-- FIXING PREFIX BUG ID 3547
   const BEntity* pEntity= gWorld->getEntity(entityID);
//--
   if(pEntity && !pEntity->getFlagHasSounds())
      return;


   uint highWaterMark = mWorldSoundList.getHighWaterMark();
   for(uint i=0; i < highWaterMark; i++)
   {
      if(mWorldSoundList.isValidIndex(i) == false)
         continue;

      BWorldSound* pSound = mWorldSoundList.get(i);
      if(pSound && pSound->mEntityID==entityID)
         internalRemoveSound(pSound);

   }
}

//==============================================================================
// BWorldSoundManager::internalRemoveSound
//==============================================================================
bool BWorldSoundManager::internalRemoveSound(BWorldSound* pSound)
{
   ASSERT_THREAD(cThreadIndexSim);

   //-- Call the stop event
   if(pSound->mCueHandle!=cInvalidCueHandle)
   {
      if(pSound->mStopCueIndex!=cInvalidCueIndex)
         gSoundManager.playCue(pSound->mStopCueIndex, pSound->mWiseObjectID);
      //pSound->mCueHandle=cInvalidCueHandle;
      pSound->mFlagStopped = true;
   }   

   //-- See if the wwise object has no more sounds playing on it
   removeSoundFromWwiseObject(pSound);

   //-- Remove the sound from the playing cue hashmap
   if(pSound->mCueHandle != cInvalidCueHandle)
      mPlayingSoundsMap.erase(pSound->mCueHandle);

   //-- Updated count in the ID
   uint idIndex=WSIDINDEX(pSound->mID);
   uint idCount=WSIDCOUNT(pSound->mID);

   pSound->reset();

   idCount++;
   if(idCount >= cWSIDCountMax)
      idCount = 0;

   pSound->mID=WSID(idCount, idIndex);

   //-- Release the sound
   mWorldSoundList.release(idIndex);

   #ifdef SOUND_TEST   
   validateData();
   #endif 


   return true;
}

//==============================================================================
// BWorldSoundManager::removeSoundFromWwiseObject
//==============================================================================
void BWorldSoundManager::removeSoundFromWwiseObject(BWorldSound* pSound)
{
   BASSERT(pSound);

   //-- Find the wwise object ID associated with this sound
   BWorldSoundHashKey key;
   key.mEntityID = pSound->mEntityID.asLong();
   key.mBoneHandle = pSound->mBoneHandle;

   //-- If there is no world object ID associated with this sound, then use the wwise object ID as the key.
   if(key.mEntityID == cInvalidObjectID.asLong())
      key.mWWiseObjectID = pSound->mWiseObjectID;

   bhandle handle = mWorldSoundObjectsTable.find(key);
   if(handle != NULL)
   {
      //-- We found it, so decrement the number of sounds associated with it
      BWorldSoundHashValue value = mWorldSoundObjectsTable.get(handle);
      mWorldSoundObjectsTable.remove(handle);
      value.mPlayingSoundsCount--;

      //-- If there are no more sounds associated with this wwise ID, then add it back into the pool of free wwise IDs
      if(value.mPlayingSoundsCount <= 0)
      {         
         mFreeWWiseIDs.add(pSound->mWiseObjectID);
      }
      else
      {
         //-- There are still sounds associated, so put the updates value back into the table
         mWorldSoundObjectsTable.add(key, value);
      }
   }
   else //-- Didn't find the sound in the table?!
   {
      BASSERT(0);
   }
}

//==============================================================================
// BWorldSoundManager::handleSoundEvent
//==============================================================================
void BWorldSoundManager::handleSoundEvent(BSoundEventParams& params)
{   
   processSoundEvent(params);
}

//==============================================================================
// BWorldSoundManager::updateSound3D
//==============================================================================
void BWorldSoundManager::updateSound3D(BWorldSound *pSound)
{
   //-- Update the distance to listener
   pSound->mDistanceToListener = pSound->mPosition.distance(mListenerPosition);
   if (gGame.isSplitScreen())
   {
      float dist2 = pSound->mPosition.distance(mListenerPosition2);
      if (dist2 < pSound->mDistanceToListener)
         pSound->mDistanceToListener = dist2;
   }

   BVector adjustedPosition = pSound->mPosition;   
   
   //-- Modify sound positions --- If a sound is behind the listener, then move it further away (we want to hear things more that are in front of us)
//-- FIXING PREFIX BUG ID 3542
   const BUser* pUser=gUserManager.getUser(BUserManager::cPrimaryUser);
//--
   if(pUser)
   {
      BVector soundPos = pSound->mPosition;
      BVector listenerToObject = soundPos - mListenerPosition;                         

      float angle = mListenerDirection.angleBetweenVector(listenerToObject);         
      float unitLength = cos(angle-XM_PI);
      if(unitLength > 0)
      {
         if (gGame.isSplitScreen())
         {
            BVector listenerToObject2 = soundPos - mListenerPosition2;
            float angle2 = mListenerDirection2.angleBetweenVector(listenerToObject2);
            float unitLength2 = cos(angle2-XM_PI);
            if (unitLength2 > 0)
            {
               if (listenerToObject2.lengthSquared() < listenerToObject.lengthSquared())
               {
                  angle = angle2;
                  unitLength = unitLength2;
               }
            }
            else
               unitLength = 0;
         }
         if (unitLength > 0)
         {
            unitLength *= listenerToObject.length();                                    
            listenerToObject.normalize();
            adjustedPosition += (listenerToObject * unitLength * mRearAdjustmentFactor);
         }
      }
   }

   if(gConfig.isDefined(cConfigDisplayListener))
      gpDebugPrimitives->addDebugSphere(adjustedPosition, 1.0f, cDWORDRed);

   //-- Update position
   gSoundManager.updateSound3D(pSound->mWiseObjectID, adjustedPosition, cXAxisVector);
}

//==============================================================================
// BWorldSoundManager::assignWiseObjectID
//==============================================================================
void BWorldSoundManager::assignWiseObjectID(BWorldSound *pSound)
{
   ASSERT_THREAD(cThreadIndexSim);


   BWorldSoundHashKey key;
   key.mEntityID = pSound->mEntityID.asLong();
   key.mBoneHandle = pSound->mBoneHandle;   

   //Is this object/bone already registered?
   bhandle handle = mWorldSoundObjectsTable.find(key);
   if(handle)
   {
      BWorldSoundHashValue value = mWorldSoundObjectsTable.get(handle);
      //-- Remove the old value
      mWorldSoundObjectsTable.remove(handle);
      //-- Inc the ref count
      value.mPlayingSoundsCount++;
      //-- Add back into the hash table
      mWorldSoundObjectsTable.add(key, value);
      pSound->mWiseObjectID = value.mWWiseObjectID;      
   }
   else
   {
      BWorldSoundHashValue value;
      value.mPlayingSoundsCount = 1;
   
      //-- Assign a free ID
      if(mFreeWWiseIDs.getNumber() > 0)
      {
         value.mWWiseObjectID = mFreeWWiseIDs.get(mFreeWWiseIDs.getNumber() - 1);
         mFreeWWiseIDs.popBack();
      }

      //-- How insane is this? If we have no valid object/bone, then we're going to use one of our wwise id's as our key
      if(pSound->mEntityID == cInvalidObjectID)
      {
         key.mWWiseObjectID = value.mWWiseObjectID;
      }
         
      //-- Add the new in use WWise object to the hash table.
      mWorldSoundObjectsTable.add(key, value);
      pSound->mWiseObjectID = value.mWWiseObjectID;      
   }
}

//==============================================================================
// BWorldSoundManager::loadXML
//==============================================================================
bool BWorldSoundManager::loadXML(void)
{
   BXMLReader reader;
   if(!reader.load(cDirData, "soundInfo.xml"))
      return false;
   BXMLNode root(reader.getRootNode());
   
   long nodeCount = root.getNumberChildren();
   for(long i=0; i<nodeCount; i++)
   {
      BXMLNode node(root.getChild(i));
      const BPackedString name(node.getName());
      if(name=="painRate")
      {
         node.getTextAsFloat(mPainRate);
      }
      else if(name=="ListenerDistance")
      {
         node.getTextAsFloat(mListenerFactor);
      }
      else if(name=="FogRTPCFadeIn")
      {
         node.getTextAsFloat(mFogRTPCFadeIn);
      }
      else if(name=="FogRTPCFadeOut")
      {
         node.getTextAsFloat(mFogRTPCFadeOut);
      }
      else if(name=="EnginePitchChangeRate")
      {
         node.getTextAsFloat(mEnginePitchChangeRate);
      }
      else if(name=="MaxPitchChangePerFrame")
      {
         node.getTextAsFloat(mMaxPitchChangePerFrame);
      }
      else if(name=="MinChatterTime" || name=="MaxChatterTime" || name=="GlobalMaxChatterTime" || name=="GlobalMinChatterTime")
      {
         BSquadSoundState state = cSquadSoundStateInvalid;
         BSimString type;
         bool result = node.getAttribValueAsString("type", type);
         if(result)
         {
            if(type == "idle")
               state = cSquadSoundStateIdle;
            else if(type == "move")
               state = cSquadSoundStateMove;
            else if(type == "moveAttack")
               state = cSquadSoundStateMoveAttack;
            else if(type == "attack")
               state = cSquadSoundStateAttack;
         }
         float timer = 0.0f;
         node.getTextAsFloat(timer);

         BASSERT(state != cSquadSoundStateInvalid);

         if(name=="MinChatterTime")
            mMinChatterTime[state] = timer;
         else if(name=="MaxChatterTime")
            mMaxChatterTime[state] = timer;
         else if(name=="GlobalMaxChatterTime")
            mGlobalMaxChatterTime[state] = timer;
         else if(name=="GlobalMinChatterTime")
            mGlobalMinChatterTime[state] = timer;
      }
      else if(name=="AlertInfo")
         loadAlertInfo(node);
      else if(name=="Music")
         mMusicManager.loadXML(node);
      else if (name == "ChatterChance")
      {
         BSimString soundTypeStr;
         bool result = node.getAttribValueAsString("type", soundTypeStr);
         if(result)
         {
            BSquadSoundType soundType = BProtoSquad::getSoundType(soundTypeStr);
            if(soundType != cSquadSoundNone)
            {
               float val = 0.0f;
               //-- Get the total chance, and the specific to target chance.
               result = node.getChildValue("Total", val);
               if(result && val != 1.0f)
               {
                  mChatterChanceToPlay.insert(soundType, val); 
               }                            

               result = node.getChildValue("Specific", val);
               if(result)
               {
                  mChatterChanceToPlaySpecific.insert(soundType, val); 
               }                            
            }
         }
      }
      else if (name == "TimeToIdleChat")
      {
         float timeToIdleChatFloat = 0.0f;
         if(node.getTextAsFloat(timeToIdleChatFloat))
            mTimeToIdleChat = (DWORD)(timeToIdleChatFloat * 1000);
      }
   }   
   return true;
}

//==============================================================================
// loadAlertInfo()
//==============================================================================
void BWorldSoundManager::loadAlertInfo(const BXMLNode node)
{
   if (!node)
      return;

   long nodeCount=node.getNumberChildren();
   for (int i = 0; i < nodeCount; ++i)
   {
      const BXMLNode child(node.getChild(i));
      const BPackedString name(child.getName());
      if (name=="AttackSev1Value")
         child.getTextAsFloat(mAttackSev1Value);
      if (name=="AttackSev2Value")
         child.getTextAsFloat(mAttackSev2Value);
   }
}

//==============================================================================
// BWorldSoundManager::loadExtendedSoundBank
//==============================================================================
void BWorldSoundManager::loadExtendedSoundBank(const BSimString& bankName)
{  
   if(gConfig.isDefined(cConfigNoSound))
      return;

   if(bankName.isEmpty())
      return;

   //-- See if this bank is already loaded?
   for(long i=0; i < mExtendedSoundBanks.getNumber(); i++)
   {
      if(bankName == mExtendedSoundBanks[i].mBankName)
      {       
         if(mExtendedSoundBanks[i].mLoaded)
         {
            onExtendedBankLoaded(mExtendedSoundBanks[i]);
         }
         return;
      }      
   }

   if(mExtendedSoundBanks.getNumber() >= cMaxExtendedBanks)
   {
      unloadExtendedSoundBank(mExtendedSoundBanks.getNumber()-1);      
   }
   

   AkBankID bankID;
   bankID = gSoundManager.loadSoundBank(bankName, true);
   if(bankID != AK_INVALID_BANK_ID)
   {
      BExtendedSoundBankInfo soundBankInfo;
      soundBankInfo.mBankID = bankID;      
      soundBankInfo.mBankName = bankName;
      mExtendedSoundBanks.insertAtIndex(soundBankInfo, 0);
   }   
}

//==============================================================================
// BWorldSoundManager::unloadExtendedSoundBank
//==============================================================================
void BWorldSoundManager::unloadExtendedSoundBank(long arrayIndex, bool asynch)
{
   if(arrayIndex < 0 || arrayIndex >= mExtendedSoundBanks.getNumber())
      return;

   const BExtendedSoundBankInfo& bankInfo = mExtendedSoundBanks[arrayIndex];
   
   //-- Unload the bank from memory
   gSoundManager.unloadSoundBank(bankInfo.mBankID, asynch);
   mExtendedSoundBanks.removeIndex(arrayIndex);
}

//==============================================================================
// BWorldSoundManager::onExtendedBankLoaded
//==============================================================================
void BWorldSoundManager::onExtendedBankLoaded(BExtendedSoundBankInfo& bankInfo)
{
   //-- Mark this bank as loaded
   bankInfo.mLoaded = true;   
}

//==============================================================================
// BWorldSoundManager::onExtendedBankLoaded
//==============================================================================
bool BWorldSoundManager::getExtendedSoundBankLoaded(const BSimString& bankName)
{
   if(gConfig.isDefined(cConfigNoSound))
      return false;

   uint count = mExtendedSoundBanks.getSize();
   for(uint i=0; i < count; i++)
   {
      if(mExtendedSoundBanks[i].mBankName == bankName && mExtendedSoundBanks[i].mLoaded == true)
         return true;
   }
   return false;
}


//==============================================================================
// BWorldSoundManager::gamePaused
//==============================================================================
void BWorldSoundManager::gamePaused(bool val)
{
   if(val == mFlagPaused)
      return;
   else
      mFlagPaused = val;

   if(val)
   {
      gSoundManager.playCue("game_paused");
   }
   else
   {
      gSoundManager.playCue("game_unpaused");
   }
}

//==============================================================================
//==============================================================================
float BWorldSoundManager::getMaxChatterTime(long state) const
{
   if(state < 0 || state >= cSquadSoundStateMax)
   {
      BASSERT(0);
      return 0.0f;
   }   
   
   return mMaxChatterTime[state];   
}

//==============================================================================
//==============================================================================
float BWorldSoundManager::getMinChatterTime(long state) const
{
   if(state < 0 || state >= cSquadSoundStateMax)
   {
      BASSERT(0);
      return 0.0f;
   }

   return mMinChatterTime[state];      
}

//==============================================================================
//==============================================================================
void BWorldSoundManager::setGlobalChatterTimer(long state)
{
   if(state < 0 || state >= cSquadSoundStateMax)
   {
      BASSERT(0);
      return;
   }

   //-- Make sure the timers are set in soundinfo.xml
   if(mGlobalMinChatterTime[state] != -1 && mGlobalMaxChatterTime[state] != -1)
   {
      //-- Set the timer
      mGlobalChatterTimer[state] =  getRandRangeFloat(cUnsyncedRand, mGlobalMinChatterTime[state], mGlobalMaxChatterTime[state]);
   }
}

//==============================================================================
//==============================================================================
float BWorldSoundManager::getGlobalChatterTimer(long state) const
{
   if(state < 0 || state >= cSquadSoundStateMax)
   {
      BASSERT(0);
      return 0.0f;
   }

   return mGlobalChatterTimer[state];
}

//==============================================================================
//==============================================================================
void BWorldSoundManager::updateTimers(float elapsed)
{
   for(uint i=0; i < cSquadSoundStateMax; i++)
   {
      mGlobalChatterTimer[i] -= elapsed;
      if(mGlobalChatterTimer[i] < 0.0f)
         mGlobalChatterTimer[i] = 0.0f;
   }
}

//==============================================================================
//==============================================================================
void BWorldSoundManager::setGameStartDelay(bool val)
{
   mFlagGameStartDelay = val;

   if(val == true)
   {
      mCurGPUFrame = gRenderThread.getCurGPUFrame();
   }
   else if(val == false)
   {
      //-- Release the sound manager
      gSoundManager.setGameStartDelay(false);

      //-- Release the music manager
      mMusicManager.setGameStartDelay(false);


      //-- Play our own queued events
      for(uint i = 0; i < mQueuedSoundsToAddWhenGameStarts.getSize(); i++)
      {
         BQueuedSoundInfo info = mQueuedSoundsToAddWhenGameStarts.get(i);
         internalAddSound(info.mEntityID, info.mBoneHandle, info.mPosition, info.mVelocity, info.mCueIndex, info.mCheckFOW, info.mStopCueIndex, info.mCheckSoundRadius, info.mCanDrop);
      }
      mQueuedSoundsToAddWhenGameStarts.clear();
   }
}

//==============================================================================
//==============================================================================
float BWorldSoundManager::getChanceToPlayChatter(int32 chatterType)
{   
   float chance = 1.0f;

   if(chatterType == -1)
      return chance;

   BChatterChanceToPlay::iterator itr = mChatterChanceToPlay.find(chatterType);
   if(itr != mChatterChanceToPlay.end())
   {
      chance = itr->second;
   }   
   return chance;
}

//==============================================================================
//==============================================================================
float BWorldSoundManager::getChanceToPlaySpecificChatter(int32 chatterType)
{
   float chance = 0.0f;

   if(chatterType == -1)
      return chance;

   BChatterChanceToPlaySpecific::iterator itr = mChatterChanceToPlaySpecific.find(chatterType);
   if(itr != mChatterChanceToPlaySpecific.end())
   {
      chance = itr->second;
   }   
   return chance;
}

#ifdef SOUND_TEST

//==============================================================================
//==============================================================================
void BWorldSoundManager::validateData()
{
   //-- Go through the world sounds and make sure if they have playing ID's that they're in the playing map
   //-- Clear and remove used sound objects
   uint highWaterMark = mWorldSoundList.getHighWaterMark();
   for(uint i=0; i < highWaterMark; i++)
   {
      if(mWorldSoundList.isValidIndex(i) == false)
         continue;

      if(mWorldSoundList[i].mCueHandle != cInvalidCueHandle)
      {
         //-- Lookup the sound in the playing cue map
         BSoundHandleToSoundIDHashMap::iterator iterator = mPlayingSoundsMap.find(mWorldSoundList[i].mCueHandle);         
         if (iterator == mPlayingSoundsMap.end())
         {
            BASSERT(0);
            return;
         }

         //-- Make sure the index matches up
         uint index = WSIDINDEX(iterator->second);
         BASSERT(index == i);

         //-- Make sure the ID matches up
         BASSERT(mWorldSoundList[i].mID == iterator->second);
      }
   }

   //-- Run through all the items in the playing sounds map and make sure the data there is correct
   BSoundHandleToSoundIDHashMap::iterator iterator = mPlayingSoundsMap.begin();
   while(iterator != mPlayingSoundsMap.end())
   {
      long soundID = iterator->second;
      uint index = WSIDINDEX(soundID);
      uint count = WSIDCOUNT(soundID);
      
      if(mWorldSoundList.isValidIndex(index) == false)
      {
         BASSERT(0);
         continue;
      }

      BASSERT(mWorldSoundList[index].mID == soundID);

      iterator++;
   }
}

static uint allSoundsIndex = 0;
static uint stopAll = 0;
long mTestCues[] = { 345468634, 753098196, 598528030, 663527314, 402076089, 246092072, 56918571, 724090535, 497612135, 704552066, 1044604546, 601656205, 153712369, 728318434, 177077262, 1022459033, 474404784, 648330192, 385537771, 584892883, 653444987, 555453114, 838162015, 1051851383, 407334729, 775893609, 798210591, 999850485, 98181065, 997432040, 689172384, 915785000, 979146761, 1070817154, 592609992, 1026631601, 41285694, 677946349, 736060309, 256520019, 941592068, 143198728, 162073238, 995871708, 99785241, 883347253, 10145488, 333341135, 579742774, 118140153, 406242631, 682875372, 98425498, 368291316, 412404558, 301476268, 723730804, 109288264, 947869718, 910643443 };        

//==============================================================================
// BWorldSoundManager::testAllSounds
//==============================================================================
void BWorldSoundManager::testAllSounds()
{
   long soundsToPlay = 300 - mUsedSoundList.getNumber();

   for(long i = 0; i < soundsToPlay; i++)
   {
      float x = getRandRangeFloat(cUnsyncedRand, 0.0f, 200.0f);
      float z = getRandRangeFloat(cUnsyncedRand, 0.0f, 200.0f);
      float y = 5;

      BVector pos(x,y,z);
      BVector velocity(0,0,0);

      addSound(pos, velocity, mTestCues[allSoundsIndex], false);
      allSoundsIndex++;
      if(allSoundsIndex > (sizeof(mTestCues) / sizeof(long)))
         allSoundsIndex = 0;
   }
}
#endif

#ifndef BUILD_FINAL
//==============================================================================
// BWorldSoundManager::dumpSoundEventCounts
//==============================================================================
void BWorldSoundManager::dumpSoundEventCounts()
{
   BDynamicSimArray<BDumpSoundInfo> dumpSoundArray;
   dumpSoundArray.clear();
   uint endsound = mWorldSoundList.getNumberAllocated();
   for(uint i=0; i < endsound; i++)
   {
      if(mWorldSoundList.isValidIndex(i) == false)
         continue;

      BWorldSound* pSound = mWorldSoundList.get(i);
      if(pSound)// && pSound->mCueIndex!=cInvalidCueIndex)
      {
         uint j=0;
         for( ; j<dumpSoundArray.getNumber(); j++ )
         {
            if(dumpSoundArray.get(j).mEventID == pSound->mCueIndex)
            {
               dumpSoundArray.get(j).mCount++;
               break;
            }
         }
         if( j==dumpSoundArray.getNumber() ) //we didn't find it, so add it
         {
            BDumpSoundInfo info;
            info.mEventID = pSound->mCueIndex;
            info.mCount = 1;
            dumpSoundArray.add(info);
         }
      }
   }
   //output the list
   gConsoleOutput.fileManager("Sound event IDs currently in use:");
   for( uint k=0; k<dumpSoundArray.getNumber(); k++ )
   {
      gConsoleOutput.fileManager("%u   count %u", dumpSoundArray.get(k).mEventID, dumpSoundArray.get(k).mCount);
   }
}
#endif

//==============================================================================
// dopple.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "dopple.h"
#include "syncmacros.h"
#include "protoobject.h"
#include "config.h"
#include "configsgame.h"
#include "team.h"
#include "visual.h"
#include "world.h"
#include "worldsoundmanager.h"
#include "visiblemap.h"
#include "minimap.h"
#include "usermanager.h"
#include "user.h"
#include "uimanager.h"
#include "game.h"

//#define DEBUG_DOPPLES

GFIMPLEMENTVERSION(BDopple, 1);

//==============================================================================
// BDopple::update
//==============================================================================
bool BDopple::update( float elapsedTime ) 
{
#ifdef SYNC_Dopple
   {
      //SCOPEDSAMPLE(BDopple_update_sync);  
      syncDoppleData("BDopple::update mID", mID.asLong());
      syncDoppleData("BDopple::update name", getProtoObject()->getName());
      syncDoppleData("BDopple::update mPosition", mPosition);
      syncDoppleData("BDopple::update mParentID", mParentID);
      syncDoppleData("BDopple::update mParentID type", (DWORD)mParentID.getType());
      syncDoppleData("BDopple::update mForTeamID", mForTeamID);
   }
#endif

   // SLB: The getObject test should no longer be necessary
   //if (gWorld->getTeam(mForTeamID)->isObjectVisible(getParentID()) && gWorld->getObject(mParentID))
   if (gWorld->getTeam(mForTeamID)->isObjectVisible(getParentID()))
   {
      //SCOPEDSAMPLE(BDopple_update_kill1);  
#ifdef SYNC_Dopple
      syncDoppleCode("BDopple::update kill1");
#endif
      kill(false);
   }
   else
   {
      DWORD mask = BTeam::doesTeamHaveSpies(mForTeamID) ? cFogMask : gVisibleMap.getTeamFogOffMask(mForTeamID);

      if(getFlagBlockLOS())
      {
         DWORD visibility = gVisibleMap.getCircularEdgeVisibility(mSimX, mSimZ, getSimLOS());
         if(visibility & mask)
         {
            //SCOPEDSAMPLE(BDopple_update_kill2);  
#ifdef SYNC_Dopple
            syncDoppleCode("BDopple::update kill12");
#endif
            kill(false);
         }
      }
      else
      {
         // Sample visibility from every tile within the unit's bounding box
         BVector minCorner, maxCorner;
         BObject *pObject = gWorld->getObject(getParentID()); // Use parent's bbox if parent still exists
         const BBoundingBox *bbox = pObject ? pObject->getSimBoundingBox() : getSimBoundingBox();
         bbox->computeMinCornerAABB(minCorner);
         bbox->computeMaxCornerAABB(maxCorner);
         minCorner = __vctsxs(XMVectorMax(XMVectorMultiply(XMVectorRound(minCorner), XMVectorReplicate(gTerrainSimRep.getReciprocalDataTileScale())), XMVectorZero()), 0);
         maxCorner = __vctsxs(XMVectorMin(XMVectorMultiply(XMVectorRound(maxCorner), XMVectorReplicate(gTerrainSimRep.getReciprocalDataTileScale())), XMVectorReplicate((float) (gVisibleMap.getMaxXTiles() - 1))), 0);
         long x1 = minCorner.u[0];
         long z1 = minCorner.u[2];
         long x2 = maxCorner.u[0];
         long z2 = maxCorner.u[2];
         for (long x = x1; x <= x2; x++)
            for (long z = z1; z <= z2; z++)
               if (gVisibleMap.getVisibility(x, z) & mask)
               {
                  //SCOPEDSAMPLE(BDopple_update_kill3);  
#ifdef SYNC_Dopple
                  syncDoppleCode("BDopple::update kill13");
#endif
                  kill(false);
                  x = x2;
                  break;
               }
      }
   }

   //-- update the root object
   if (!BObject::update(elapsedTime))
      return (false);

   return (true);
}

//==============================================================================
// BDopple::init
//==============================================================================
void BDopple::init( void )
{
   //-- call the base func
   BObject::init(); 
   mParentID = cInvalidObjectID;
   mForTeamID = -1;
   mFlagIsDopple = true;
}

//==============================================================================
// BDopple::destroy
//==============================================================================
void BDopple::destroy()
{
#ifdef SYNC_Dopple
   syncDoppleData("BDopple::destroy mID", mID.asLong());
#endif
   BEntity::destroy();
}

//==============================================================================
// BDopple::kill
//==============================================================================
void BDopple::kill(bool bKillImmediately)
{
#ifdef SYNC_Dopple
   syncDoppleData("BDopple::kill mID", mID.asLong());
#endif

#ifdef DEBUG_DOPPLES
   blogtrace("Removing dopple (%u) for player %u -- %u", getID(), getPlayerID(), gWorld->getNumberDopples());
#endif

   // Play cached animation tags.
   BUser* pPrimaryUser = gUserManager.getUser(BUserManager::cPrimaryUser);BASSERT(pPrimaryUser);
//-- FIXING PREFIX BUG ID 1567
   const BPlayer* pPlayer = pPrimaryUser->getPlayer();BASSERT(pPlayer);
//--
   BTeamID playerTeamID = pPlayer->getTeamID();
   if (playerTeamID == getForTeamID())
      gWorld->flushCachedAnimEvents(mParentID, true);

   // Tell parent it's no longer doppled for this team
   BObject *pParentObject = gWorld->getObject(mParentID);
   if (pParentObject)
      pParentObject->clearDoppleObject(mForTeamID);

   stop();
   destroy();
}

//==============================================================================
// BDopple::initFromObject
//==============================================================================
bool BDopple::initFromObject(const BObject*  pObject, BTeamID teamID, bool goIdle)
{
#ifdef SYNC_Dopple
   syncDoppleData("BDopple::initFromObject mID", mID.asLong());
#endif

   const BProtoObject* pProto=pObject->getProtoObject();

   setProtoID(pProto->getID());

   mObstructionRadiusX = pObject->getObstructionRadiusX();
   mObstructionRadiusY = pObject->getObstructionRadiusY();
   mObstructionRadiusZ = pObject->getObstructionRadiusZ();

   // SLB: We don't want to do this
   //updateSimPosition();

   setFlagCollidable(pProto->getFlagCollidable());
   setFlagTiesToGround(!pProto->getFlagNoTieToGround());
   setFlagBlockLOS(pObject->getFlagBlockLOS());   

   mParentID=pObject->getID();
   mForTeamID=teamID;
   mPlayerVisibility.setAll(gVisibleMap.getTeamFogOffMask(teamID));

   setVisual(pObject->getVisual());

   updateVisualVisibility();

   if (goIdle)
   {
      // Force idle animation
      bool animationDisabled = pObject->getFlagAnimationDisabled();
      setAnimationEnabled(true);
      setAnimationState(BObjectAnimationState::cAnimationStateIdle);
      computeAnimation();
      setAnimationEnabled(!animationDisabled);
   }
   else
   {
      setAnimationEnabled(!pObject->getFlagAnimationDisabled());
   }

   //-- exist sound
   if(gUserManager.getUser(BUserManager::cPrimaryUser)->getTeamID() == teamID)
      startExistSound();

#ifdef DEBUG_DOPPLES
   blogtrace("Creating dopple (%u) for player %u -- %u", getID(), getPlayerID(), gWorld->getNumberDopples());
#endif

   return true;
}

//=============================================================================
// BDopple::updateVisibleLists
//=============================================================================
void BDopple::updateVisibleLists(void)
{
   if (gWorld->getFlagAllVisible() || 
       isVisible(gUserManager.getUser(BUserManager::cPrimaryUser)->getTeamID()) ||
       (gGame.isSplitScreen() && isVisible(gUserManager.getSecondaryUser()->getTeamID()))
      )
   {
      // mrh 8/2/07 - Be sure to check for the proto object until we make sure all objects have one.
      // slb: Dopples that don't have the dopple flag in their proto are corpses. We don't want those showing up on the minimap.
      const BProtoObject* pProtoObject = getProtoObject();
      if (pProtoObject && (pProtoObject->getFlagDopples() || pProtoObject->getFlagGrayMapDopples()))
      {
         //if (!gConfig.isDefined(cConfigFlashGameUI))
         //   gMiniMap.addUnit(*this);
         //else
         gUIManager->addMinimapIcon(this);
      }
   }
}

//=============================================================================
//=============================================================================
bool BDopple::save(BStream* pStream, int saveType) const
{
   if (!BObject::save(pStream, saveType))
      return false;
   GFWRITEVAL(pStream, int8, mForTeamID);
   return true;
}

//=============================================================================
//=============================================================================
bool BDopple::load(BStream* pStream, int saveType)
{
   if (!BObject::load(pStream, saveType))
      return false;
   GFREADVAL(pStream, int8, long, mForTeamID);
   return true;
}

//=============================================================================
//=============================================================================
bool BDopple::postLoad(int saveType)
{
   return BObject::postLoad(saveType);
}

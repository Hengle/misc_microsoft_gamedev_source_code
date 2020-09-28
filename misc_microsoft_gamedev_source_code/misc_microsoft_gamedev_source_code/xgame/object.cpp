//==============================================================================
// object.cpp
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "object.h"
#include "boundingbox.h"
#include "config.h"
#include "configsgame.h"
#include "gamecallbacks.h"
#include "kb.h"
#include "world.h"
#include "protoobject.h"
#include "obstructionmanager.h"
#include "xvisual.h"
#include "dirShadowManager.h"
#include "render.h"
#include "physicsinfo.h"
#include "physicsinfomanager.h" 
#include "team.h"
#include "worldsoundmanager.h"
#include "visiblemap.h"
#include "unit.h"
#include "minimap.h"
#include "hpbar.h"
#include "textvisualmanager.h"
#include "uigame.h"
#include "usermanager.h"
#include "user.h"
#include "tactic.h"
#include "action.h"
#include "syncmacros.h"
#include "selectionmanager.h"
#include "uimanager.h"
#include "renderDraw.h"
#include "damageHelper.h"
#include "visualitem.h"
#include "physicsgrizzlyaction.h"
#include "physicswarthogaction.h"
#include "physicsghostaction.h"
#include "physicsscorpionaction.h"
#include "physicscobraaction.h"
#include "physicswolverineaction.h"
#include "physicselephantaction.h"
#include "physicsgroundvehicleaction.h"
#include "protoimpacteffect.h"
#include "Quaternion.h"
#include "game.h"
#include "terraineffect.h"
#include "terraineffectmanager.h"
#include "corpsemanager.h"
#include "econfigenum.h"
#include "renderControl.h"

// xphysics
#include "physics.h"
//#include "physicsobject.h"
#include <Physics/Collide/Shape/Convex/Box/hkpBoxShape.h>

#include "renderHelperThread.h"

GFIMPLEMENTVERSION(BObject, 8);
enum 
{
   cSaveMarkerObject1=10000,
};

#ifndef _MOVE4
#define _MOVE4
#endif

//#define DEBUG_RENDER_BOUNDING_BOX
//#define DEBUG_RENDER_BOUNDING_SPHERE   
//#define DEBUG_RENDER_SIM_BOUNDING_BOX

#define REVEAL_SCALE 1.5f
#define BLOCK_SCALE  REVEAL_SCALE

#define HARDPOINT_TOLERANCE_STOP          0.9999f
#define HARDPOINT_SOUND_ACTIVATION_TIMER  0.2f

//#define DISABLE_NO_UPDATE_OPTIMIZATION
static float cTimeScale = 3.33f;        // seconds

#ifndef BUILD_FINAL
BEntityID sDebugObjectTempID;

//#define DEBUG_OBJECT
#endif

#ifdef DEBUG_OBJECT
#define debugObject sDebugObjectTempID=mID, dbgObjectInternalTempID
#else
#define debugObject __noop
#endif

#ifndef BUILD_FINAL
//==============================================================================
//==============================================================================
void dbgObjectInternal(BEntityID ObjectID, const char *format, ...)
{
   // Assemble caller's string.
   char buf[256];        
   va_list arglist;
   va_start(arglist, format);
   StringCchVPrintf(buf, _countof(buf), format, arglist);
   va_end(arglist);

   long lSpecificObject = -1;

   gConfig.get(cConfigDebugSpecificSquad, &lSpecificObject);
   bool bMatch = false;

   if (lSpecificObject == -1 || ((lSpecificObject != -1) && (lSpecificObject == ObjectID.asLong())))
      bMatch = true;

   if (!bMatch)
      return;

   // Output.
   gConsole.output(cChannelSim, "Object %d (%d): %s", ObjectID.asLong(), ObjectID.getIndex(), buf);
   //syncObjectData("debugMove4 --", buf);
}


//==============================================================================
//==============================================================================
void dbgObjectInternalTempID(const char *format, ...)
{
   // Assemble caller's string.
   char buf[256];        
   va_list arglist;
   va_start(arglist, format);
   StringCchVPrintf(buf, _countof(buf), format, arglist);
   va_end(arglist);

   // Call with preset ID.
   dbgObjectInternal(sDebugObjectTempID, buf);
}
#endif


//==============================================================================
void BObject::revealOverTime() 
{ 
   mLOSRevealTime = 0.0f; 
}

//==============================================================================
void BObject::setRevealOverTimePercent(float percent)
{
   mLOSRevealTime = Math::Clamp(percent, 0.0f, 1.0f);
}

//==============================================================================
BAdditionalTexture::BAdditionalTexture() :
   RenderType(cATMax),
   Texture(BMeshEffectTextures::cTTMax),
   TexUVScale(1.0f),
   TexInten(1.0f),
   TexScrollSpeed(1.0f),
   TexTimeout(-1.0f),
   TexStartTime(0),
   ModulateOffset(false),
   ModulateIntensity(false),
   ShouldBeCopied(false),
   TexClamp(false),
   TexScrollLoop(false)
{
   TexUVOfs.clear();
}

//==============================================================================
// BObject::BObject
//==============================================================================
BObject::BObject()
{
}

//==============================================================================
// BObject::~BObject
//==============================================================================
BObject::~BObject()
{
   mActions.clearActions();
}

//==============================================================================
// BObject::updatePreAsync
//==============================================================================
bool BObject::updatePreAsync(float elapsedTime)
{
   if (mpVisual)
   {
      BMatrix mat;
      getWorldMatrix(mat);

      DWORD playerColor = gWorld->getPlayerColor(getPlayerID(), BWorld::cPlayerColorContextObjects);
      mpVisual->updatePreAsync(elapsedTime, playerColor, mat, gWorld->getSubUpdate(), !getFlagAnimationDisabled());
      
      // jce [11/15/2008] -- Moved updateIK here so that during subupdating it is always occuring at the same time
      // as the animation update, since they are circularly dependent.  In theory this introduces a one update lag in
      // the terrain sampling, but in practice we could not notice this and it's way better than having IK+subupdating be
      // a stop-motion slideshow.
      updateIK(getFlagFirstUpdate());
   }

   return (true);
}

//==============================================================================
// BObject::updateAsync
//==============================================================================
bool BObject::updateAsync(float elapsedTime)
{
   if (mpVisual && !getFlagAnimationDisabled())
      mpVisual->updateAsync(elapsedTime, gWorld->getSubUpdate());  

   return (true);
}

//==============================================================================
// BObject::update
//==============================================================================
bool BObject::update(float elapsedTime)
{
   SCOPEDSAMPLE(BObject_update);  

#ifndef DISABLE_NO_UPDATE_OPTIMIZATION
   // SLB: temp hack
   if (getFlagNoUpdate())
      return true;
#endif  

   //-- do we need to update position
   bool firstUpdate = getFlagFirstUpdate();
   bool moving      = getFlagMoving() || getFlagTeleported();

#ifdef SYNC_UnitDetail
   if (getClassType() != cClassTypeObject)
   {
      syncUnitDetailData("BObject::update mID", mID.asLong());
      if (getProtoObject())
      {
         syncUnitDetailData("BObject::update name", getProtoObject()->getName());
      }
      else
      {
         syncUnitDetailData("BObject::update name", "none");
      }
//-- FIXING PREFIX BUG ID 6074
      const BPhysicsObject* pPhysicsObject = getPhysicsObject();
//--
      if (pPhysicsObject)
      {
//-- FIXING PREFIX BUG ID 6073
         const BVisual* pVis = getVisual();
//--
         if (pVis)
         {
            // SLB: Don't dereference mpName on the same frame that it gets reloaded.
            if (!BProtoVisual::mGenerationChanged && pVis->mpName)
            {
               syncUnitDetailData("BObject::update visName", *(pVis->mpName));
            }
            else if (pVis->getProtoVisual())
            {
               syncUnitDetailData("BObject::update visName", pVis->getProtoVisual()->getName());
            }
         }
         BVector pos;
         pPhysicsObject->getPosition(pos);
         syncUnitDetailData("BObject::update physics obj pos", pos);
         BPhysicsMatrix rot;
         pPhysicsObject->getRotation(rot);
         syncUnitDetailData("BObject::update physics obj fwd", rot.getForward());
      }
   }
#endif

   // mrh - 7/27/07 - MOved this from the initOnProtoObject so it happens after the unit has its parent squad attached
   // and the AI can register it (as the AI only updates on visibility changes)
   if (firstUpdate && getFlagGrayMapDopples())
   {
      long numTeams = gWorld->getNumberTeams();
      for (long i=1; i<numTeams; i++)
      {
         assertVisibility();
         makeSoftDopple(i);
         assertVisibility();
      }
   }

   setFlagMotionCollisionChecked(false);

   // we need set the visibility state on the first update right away so that the visuals
   // have their proper visual state set immediately when the object gets first created.
   // This is important for projectiles and objects with short life spans.
   if(firstUpdate && getFlagVisibility() && !getFlagIsRevealer() && !getFlagBlockLOS())
   {      
#ifdef SYNC_Visibility
      if(getClassType() != cClassTypeObject)
         syncVisibilityCode("BObject::update - updateVisibleLists");
#endif
      updateVisibleLists();
   }

   bool retval=BEntity::update(elapsedTime);
   // DLM 7-3-08 - Terrifying preE3 change.  Move the updateVisual to after the
   // Entity update, to allow animations to correctly know if they're going from
   // a previous movement to a new movement. This allows units that have been
   // recommanded to not hitch visually. If this breaks the world move it back.
   updateVisual(elapsedTime);

   // Update visual world matrix and bounding box after all movement is done
   updateMotionExtraction(elapsedTime);
   updatePhysicsVisuals(elapsedTime);
   
   // jce [11/15/2008] -- Moved updateIK to the preAsync phase.  More detailed comments there.
//   updateIK(firstUpdate);

   updateVisualVisibility();
   updateVisualWorldMatrix();
   updateBoundingBox();
   if (!firstUpdate)
   {
      updateLifespan();
   }
   updateAlpha();
   updateHardpoints(elapsedTime);


#ifdef SYNC_Visibility
   if (getClassType() != cClassTypeObject)
   {
      syncVisibilityData("BObject::update mID", mID.asLong());
      syncVisibilityData("BObject::update retval", retval);
      syncVisibilityData("BObject::update firstUpdate", firstUpdate);
      syncVisibilityData("BObject::update moving", moving);
      syncVisibilityData("BObject::update cFlagLOS", getFlagLOS());
      syncVisibilityData("BObject::update cFlagLOSDirty", getFlagLOSDirty());
      syncVisibilityData("BObject::update cFlagLOSMarked", getFlagLOSMarked());
   }
#endif

   //-- update LOS
   if(getFlagLOS())
   {
      SCOPEDSAMPLE(BObjectUpdateLOS);
      if (!retval) //-- being destroyed
      {
         if (getFlagLOSMarked())
         {
            markLOSOff();
            setFlagLOS(false);
         }
      }
      else if (!getFlagLOSMarked())
         markLOSOn();
      // ajl 6/5/08 - commenting out condition for now since the "moving" flag is not accurate
      //else if (getFlagLOSDirty() || moving)
      else
         markLOSUpdate();
   }

   //-- reveal minimap   
   if (getFlagIsRevealer())
   {
      if (mLOSRevealTime >= 0.0f)
         mLOSRevealTime = min(1.0f, mLOSRevealTime + (elapsedTime * cTimeScale));

//-- FIXING PREFIX BUG ID 6075
      const BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
//--
      BDEBUG_ASSERT(pUser);
      bool canSee = BTeam::canTeamASeeTeamB(pUser->getTeamID(), getTeamID());
//-- FIXING PREFIX BUG ID 6076
      const BUser* pUser2 = gUserManager.getUser(BUserManager::cSecondaryUser);
//--
      if (pUser2 && !canSee)
         canSee = BTeam::canTeamASeeTeamB(pUser2->getTeamID(), getTeamID());
      if (canSee)
      {
         const float los = getLOS();
         if (los > 0.0f)
         {
            //if (!gConfig.isDefined(cConfigFlashGameUI))
            //   gMiniMap.reveal(getPosition(), los);
            //else
            gUIManager->revealMinimap(getPosition(), los);
         }
         else if (los == -1.0f)
         {
            //if (!gConfig.isDefined(cConfigFlashGameUI))
            //   gMiniMap.reveal( getPosition(), REVEAL_SCALE * gTerrainSimRep.getNumXDataTiles() * gTerrainSimRep.getDataTileScale() );
            //else
            gUIManager->revealMinimap(getPosition(), REVEAL_SCALE * gTerrainSimRep.getNumXDataTiles() * gTerrainSimRep.getDataTileScale() );
         }
      }
   }
   //-- block minimap for enemies or reveal for allies
   else if (getFlagBlockLOS())
   {
//-- FIXING PREFIX BUG ID 6077
      const BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
//--
      BDEBUG_ASSERT(pUser);
      bool canSee = !BTeam::canTeamASeeTeamB(pUser->getTeamID(), getTeamID());
//-- FIXING PREFIX BUG ID 6078
      const BUser* pUser2 = gUserManager.getUser(BUserManager::cSecondaryUser);
//--
      if (pUser2 && !canSee)
         canSee = !BTeam::canTeamASeeTeamB(pUser2->getTeamID(), getTeamID());
      if (canSee)
      {
         const BVector pos = getPosition();
         const float los = getLOS();
         if (los > 0.0f)
         {
            //if (!gConfig.isDefined(cConfigFlashGameUI))
            //   gMiniMap.block(pos, los);
            //else
            gUIManager->blockMinimap(pos, los);
         }
         else if (los == -1.0f)
         {
            //if (!gConfig.isDefined(cConfigFlashGameUI))
            //   gMiniMap.block(pos, BLOCK_SCALE * gTerrainSimRep.getNumXDataTiles() * gTerrainSimRep.getDataTileScale());
            //else
            gUIManager->blockMinimap(pos, BLOCK_SCALE * gTerrainSimRep.getNumXDataTiles() * gTerrainSimRep.getDataTileScale());
         }
      }
      else if (!getFlagNoReveal())
      {
         const BVector pos = getPosition();
         const float los = getLOS();
         if (los > 0.0f)
         {  
            //if (!gConfig.isDefined(cConfigFlashGameUI))
            //   gMiniMap.reveal(pos, los);
            //else
            gUIManager->revealMinimap(pos, los);
         }
         else if (los == -1.0f)
         {
            //if (!gConfig.isDefined(cConfigFlashGameUI))
            //   gMiniMap.reveal(pos, REVEAL_SCALE * gTerrainSimRep.getNumXDataTiles() * gTerrainSimRep.getDataTileScale());
            //else
            gUIManager->revealMinimap(pos, REVEAL_SCALE * gTerrainSimRep.getNumXDataTiles() * gTerrainSimRep.getDataTileScale());
         }
      }

#ifdef SYNC_Visibility
      if(getClassType() != cClassTypeObject)
      {
         syncVisibilityCode("BObject::update - updateVisibleLists");
      }
#endif

      //-- update visible lists... blockers do need to do this so that a dopple can get created once the enemy can see the blocker
      updateVisibleLists();
   }
   //-- update visible lists...revealers don't need to do this
   else if(getFlagVisibility())
   {
#ifdef SYNC_Visibility
      if(getClassType() != cClassTypeObject)
      {
         syncVisibilityCode("BObject::update - updateVisibleLists");
      }
#endif
      updateVisibleLists();
   }

   updateAttachments(elapsedTime, moving);

#ifndef DISABLE_NO_UPDATE_OPTIMIZATION
   // SLB: temp hack
   // Halwes - 11/29/2007 - Don't set the no update flag for dopples.
   if (firstUpdate && !mFlagIsDopple && !mFlagAttached && !getProjectile() && !getFlagHasLifespan())
   {
      const BProtoObject *pProto = getProtoObject();
      if (!(pProto && pProto->getFlagUpdate()) && mpVisual)
      {
         BProtoVisual *pProtoVisual = mpVisual->getProtoVisual();

         if (!(pProtoVisual && 
            (pProtoVisual->getFlag(BProtoVisual::cFlagHasAnimation) || 
            pProtoVisual->getFlag(BProtoVisual::cFlagHasParticleSystem) || 
            pProtoVisual->getFlag(BProtoVisual::cFlagHasLight) || 
            pProtoVisual->getFlag(BProtoVisual::cFlagHasOptionalMesh) || 
            pProtoVisual->getFlag(BProtoVisual::cFlagHasAttachment))))
         {
            if (pProto && pProto->getMiniMapIcon().isEmpty() && !pProto->getTactic() && (getSimLOS() <= 0) && !pProto->getFlagIsExternalShield() && (getFlagPhysicsControl() == false) && !pProto->getFlagBuildingCommands() && !pProto->getFlagBuild() && !pProto->getFlagUseBuildingAction() && !pProto->getFlagHasHPBar())
               setFlagNoUpdate(true);
         }
      }
   }
#endif
   
   if (gEnableSubUpdating)
   {
      mOldWorldMatrix = mNewWorldMatrix;
      getWorldMatrix(mNewWorldMatrix);
      mSubUpdateNumber = gWorld->getSubUpdate();
      if(mpVisual)
         mpVisual->updateDone();
   }

#ifndef BUILD_FINAL
   if(gConfig.isDefined(cConfigDrawHardpoints))
      debugDrawHardpoints();
#endif

   return retval;
}

//==============================================================================
//==============================================================================
void BObject::updatePhysicsVisuals(float elapsedTime)
{
   // MPB - This currently assumes that the physics object we have isn't a
   // replacement physics object.  It also assumes the warthog has a single
   // physicsWarthogAction.  Fix this.
   const BProtoObject* pProto = getProtoObject();
   BPhysicsObject* pPO = getPhysicsObject();
   if (pProto && pPO && !pPO->isKeyframed())
   {
      BPhysicsInfo *pInfo = gPhysicsInfoManager.get(pProto->getPhysicsInfoID(), true);
      hkpAction* pVehicleAction = NULL;
      if (pPO->getNumActions() > 0)
         pVehicleAction = pPO->getAction(0);

      if (pInfo && pVehicleAction)
      {
         if ((pInfo->getVehicleType() == BPhysicsInfo::cWarthog) ||
             (pInfo->getVehicleType() == BPhysicsInfo::cChopper))
         {
            BPhysicsWarthogAction* pPWA = static_cast<BPhysicsWarthogAction*>(pVehicleAction);
            if (pPWA)
            {
               if (pPWA->getFlagChopper())
                  pPWA->updateChopperVisuals();
               else
                  pPWA->updateWarthogVisuals();
               pPWA->spawnPhysicsEventVisuals(pInfo->getTerrainEffectsHandle());

               // Debug rendering
               #ifndef BUILD_FINAL
                  pPWA->debugRender();
               #endif
            }
         }
         else if (pInfo->getVehicleType() == BPhysicsInfo::cGhost)
         {
            BPhysicsGhostAction* pPGA = static_cast<BPhysicsGhostAction*>(pVehicleAction);
            if (pPGA)
            {
               // Debug rendering
               #ifndef BUILD_FINAL
                  pPGA->debugRender();
               #endif
            }
         }
         else if ((pInfo->getVehicleType() == BPhysicsInfo::cCobra) ||
                  (pInfo->getVehicleType() == BPhysicsInfo::cGremlin) ||
                  (pInfo->getVehicleType() == BPhysicsInfo::cElephant) ||
                  (pInfo->getVehicleType() == BPhysicsInfo::cGrizzly) ||
                  (pInfo->getVehicleType() == BPhysicsInfo::cScorpion) ||
                  (pInfo->getVehicleType() == BPhysicsInfo::cWolverine) ||
                  (pInfo->getVehicleType() == BPhysicsInfo::cRhino) ||
                  (pInfo->getVehicleType() == BPhysicsInfo::cReactor) ||
                  (pInfo->getVehicleType() == BPhysicsInfo::cGround))
         {
            BPhysicsGroundVehicleAction* pPGVA = static_cast<BPhysicsGroundVehicleAction*>(pVehicleAction);
            if (pPGVA)
            {
               static const char* gScorpionTreadNames[4]={"bonetreadbr", "bonetreadbl", "bonetreadfr", "bonetreadfl" };
               static const char* gGrizzlyTreadNames[4]={"Bone Grizzly Tread RR", "Bone Grizzly Tread RL", "Bone Grizzly Tread FR", "Bone Grizzly Tread FL" };
               static const char* gElephantTreadNames[4]={"BoneWheelsBR", "BoneWheelsBL", "BoneWheelsFR", "BoneWheelsFL" };
               static const char* gRhinoTreadNames[4]={"bone_tread_RR", "bone_tread_RL", "bone_tread_FR", "bone_tread_FL" };
               static const char* gReactorTreadNames[4]={"bone_tread_rr", "bone_tread_rl", "bone_tread_fr", "bone_ tread_fl" };
               // TODO - setup support for middle rhino treads? bone_tread_tread_ML, bone_tread_tread_MR
               // Really this whole thing should be much more data driven, with the bone names, number of treads/wheels and such
               // in a data file

               // Update visuals
               if (pInfo->getVehicleType() == BPhysicsInfo::cElephant)
                  reinterpret_cast<BPhysicsScorpionAction*>(pPGVA)->updateVisuals(gElephantTreadNames);
               else if (pInfo->getVehicleType() == BPhysicsInfo::cGrizzly)
                  reinterpret_cast<BPhysicsScorpionAction*>(pPGVA)->updateVisuals(gGrizzlyTreadNames);
               else if (pInfo->getVehicleType() == BPhysicsInfo::cScorpion)
                  reinterpret_cast<BPhysicsScorpionAction*>(pPGVA)->updateVisuals(gScorpionTreadNames);
               else if (pInfo->getVehicleType() == BPhysicsInfo::cRhino)
                  reinterpret_cast<BPhysicsScorpionAction*>(pPGVA)->updateVisuals(gRhinoTreadNames);
               else if (pInfo->getVehicleType() == BPhysicsInfo::cReactor)
                  reinterpret_cast<BPhysicsScorpionAction*>(pPGVA)->updateVisuals(gReactorTreadNames);
               else
                  pPGVA->updateVisuals(elapsedTime);

               // Spawn trails
               pPGVA->spawnPhysicsEventVisuals(pInfo->getTerrainEffectsHandle());

               // Debug rendering
               #ifndef BUILD_FINAL
                  pPGVA->debugRender();
               #endif
            }
         }
      }
   }
}

//==============================================================================
// BObject::updateVisual
//==============================================================================
void BObject::updateVisual(float elapsedTime)
{
   if (getFlagAnimationDisabled())
      return;

   SCOPEDSAMPLE(BObjectUpdateVisual);

   computeAnimation();

   if(mpVisual)
   {
     BMatrix mat;
     getWorldMatrix(mat);

     DWORD playerColor = gWorld->getPlayerColor(getPlayerID(), BWorld::cPlayerColorContextObjects);

     mpVisual->update(elapsedTime, playerColor, mat, gWorld->getSubUpdate());

#ifdef SYNC_Anim
      if(getClassType() != cClassTypeObject)
      {
         for (int i=0; i<cNumAnimationTracks; i++)
         {
            syncAnimData("BObject::updateVisual after animPos", mpVisual->getAnimationPosition(i));
         }
      }
#endif

      // Update additional textures, removing them if the timeout is up
      if (mpAdditionalTextures)
      {
         for (int i = mpAdditionalTextures->getSize() - 1; i >= 0; i--)
         {
            BAdditionalTexture& texture = mpAdditionalTextures->get(i);
            if (texture.TexTimeout > 0.0f && texture.TexTimeout < gWorld->getGametimeFloat())
               removeAdditionalTexture(texture.RenderType);
         }
      }
   }
}

//==============================================================================
// BObject::updateIK
//==============================================================================
void BObject::updateIK(bool firstUpdate)
{
   if (mpVisual)
   {
      const BProtoObject *pProto = getProtoObject();
      if (pProto)
      {
         long numGroundIKNodes = pProto->getNumberGroundIKNodes();
         long numSweetSpotIKNodes = pProto->getNumberSweetSpotIKNodes();
         long numSingleBoneIKNodes = pProto->getNumberSingleBoneIKNodes();
         long numIKNodes = numGroundIKNodes + numSweetSpotIKNodes + numSingleBoneIKNodes;
         long nodeIndex = 0;

         // Get tilt factor
         float tiltFactor = pProto->getGroundIKTiltFactor();
         if (tiltFactor > cFloatCompareEpsilon)
         {
            numIKNodes++;
            mpVisual->setIKNodeActive(0, false);
         }

         if (gConfig.isDefined(cConfigOverrideGroundIK))
         {
            gConfig.get(cConfigOverrideGroundIKTiltFactor, &tiltFactor);
         }

         bool tilt = false;
         if (tiltFactor > cFloatCompareEpsilon)
            tilt = true;

         if (numIKNodes)
         {
            // Bypass IK
            if (getFlagIKDisabled())
            {
               // Disable all IK nodes
               for (long i = 0; i < numIKNodes; i++)
               {
                  mpVisual->setIKNodeActive(i, false);
               }

               // Exit
               return;
            }

            BMatrix worldMatrix;
            getWorldMatrix(worldMatrix);

            // Get the inverse world matrix too.  TODO: construct directly for more speed.
            BMatrix inverseWorldMatrix;
            inverseWorldMatrix = worldMatrix;
            inverseWorldMatrix.invert();

            if (numGroundIKNodes)
            {
               // Get current anim state
               long currentAnimType = getAnimationType(cMovementAnimationTrack);
               bool currentAnimIdle = !isMoveAnimType(currentAnimType) && !isTurnAnimType(currentAnimType);

               // Iterate through ground IK nodes and calculate target positions
               for (long i = 0; i < numGroundIKNodes; i++)
               {
                  long boneHandle = mpVisual->getIKNodeBoneHandle(nodeIndex);

                  // If we're sweet spotting, disable ground IK
                  bool sweetSpotting = false;
                  for (long j = 0; j < numSweetSpotIKNodes; j++)
                  {
                     long node = j + numGroundIKNodes + (tilt ? 1 : 0);
                     if ((mpVisual->isIKNodeActive(node)) && (boneHandle == mpVisual->getIKNodeBoneHandle(node)))
                     {
                        sweetSpotting = true;
                        break;
                     }
                  }

                  BVector animatedBonePos;
                  // jce [11/14/2008] -- changed this getBone to work in local space like the IK is now doing.
                  if (!sweetSpotting && mpVisual->getBone(boneHandle, &animatedBonePos, NULL, NULL, NULL, false))
                  {
                     const BGroundIKNode *pGroundIKNode = pProto->getGroundIKNode(i);

                     // Get IK influence range
                     float maxIKRange = pGroundIKNode->getIKRange();
                     if (gConfig.isDefined(cConfigOverrideGroundIK))
                     {
                        gConfig.get(cConfigOverrideGroundIKRange, &maxIKRange);
                     }

                     // Get IK anchor location
                     BVector anchorPos;
                     bool lockComplete;
                     float lockStartTime;
                     float lockEndTime;
                     BIKNode* pIKNode = mpVisual->getIKNodeFromIndex(nodeIndex);
                     BASSERT(pIKNode);
                     bool hasAnchor = mpVisual->getIKNodeAnchor(nodeIndex, anchorPos, lockStartTime, lockEndTime, lockComplete);

                     // jce [11/14/2008] -- need to move from local to world space for terrain sampling.
                     BVector worldSpaceAnimatedBonePos;
                     worldMatrix.transformVectorAsPoint(animatedBonePos, worldSpaceAnimatedBonePos);

                     // Get height of end effector at animated position
                     float height;
                     gTerrainSimRep.getHeightRaycast(worldSpaceAnimatedBonePos, height, true);

                     // Calculate target bone pos and target bone pos tied to ground
                     BVector targetBonePos;

                     // Initialize the idle transition state and anchor position if we are just now going idle
                     if (currentAnimIdle && !pIKNode->mIdleTransitioning && !pIKNode->mIdleTransitionLockStarted)
                     {
                        // Lock everything down
                        pIKNode->setAnchorPos(pIKNode->getTargetPos());
                        
                        anchorPos = pIKNode->getAnchorPos();
                        pIKNode->mIdleTransitioning = true;
                        pIKNode->mIdleTransitionLockStarted = false;
                     }
                     // If current anim is moving or turning (not idle), then reset the idleTransition state
                     else if (!currentAnimIdle)
                     {
                        pIKNode->mIdleTransitioning = false;
                        pIKNode->mIdleTransitionLockStarted = false;
                     }

                     // For first update, set it to animated position on ground
                     if (firstUpdate)
                     {
                        // jce [11/14/2008] -- IK targets are in local space now, so need to get back to world space.
                        worldMatrix.transformVectorAsPoint(animatedBonePos, targetBonePos);
                        
                        // Update the height to be on the ground.
                        targetBonePos.y = height;
                        
                        // Back to local space.
                        inverseWorldMatrix.transformVectorAsPoint(targetBonePos, targetBonePos);
                     }
                     // If going to ground, blend to ground position from anchor (starting) position
                     else if (hasAnchor)
                     {
                        // Current anim time
                        float duration = mpVisual->getAnimationDuration(cMovementAnimationTrack);
                        float time = mpVisual->getAnimationPosition(cMovementAnimationTrack);
                        float normalizedTime = time / duration;

                        // Interp factor.  If done interpolating, set the lock to complete
                        float interpFactor = 1.0f;
                        if (!lockComplete && normalizedTime >= lockStartTime && normalizedTime <= lockEndTime)
                        {
                           interpFactor = (normalizedTime - lockStartTime) / (lockEndTime - lockStartTime);
                           interpFactor *= interpFactor; // square the interp factor for some smoothing
                           if (pIKNode->mIdleTransitioning)
                              pIKNode->mIdleTransitionLockStarted = true;
                        }
                        else
                        {
                           mpVisual->setIKNodeLockComplete(nodeIndex, true);
//#define CLAMP_TO_ANCHOR
#ifdef CLAMP_TO_ANCHOR
                           if (!lockComplete)
                           {
                              // jce [11/14/2008] -- IK targets are in local space now, so need to get back to world space.
                              worldMatrix.transformVectorAsPoint(animatedBonePos, pIKNode->mAnchorPos);
                        
                              // Update the height to be on the ground.
                              pIKNode->mAnchorPos.y = height;
                        
                              // Back to local space.
                              inverseWorldMatrix.transformVectorAsPoint(pIKNode->mAnchorPos, pIKNode->mAnchorPos);
                           }
#endif
                        }

                        // If the transition to locked down has started and the lock is complete,
                        // then the idle transition for this IK node is done
                        if (pIKNode->mIdleTransitionLockStarted && lockComplete)
                           pIKNode->mIdleTransitioning = false;

                        // If not idle transitioning, then we're just interpolating 'y' from the ground to the animated position
                        if (!pIKNode->mIdleTransitioning)
                        {
                           // Lerp
#ifdef CLAMP_TO_ANCHOR
                           if (lockComplete)
                           {
                              targetBonePos = anchorPos;
                           }
                           else
#endif
                           {
                              // jce [11/14/2008] -- IK targets are in local space now, so need to get back to world space.
                              worldMatrix.transformVectorAsPoint(animatedBonePos, targetBonePos);
                              
                              // Update the height
                              float yInterp = Math::Lerp(targetBonePos.y, height, interpFactor);
                              targetBonePos.y = yInterp;
                              
                              // Back to local space.
                              inverseWorldMatrix.transformVectorAsPoint(targetBonePos, targetBonePos);

                           }
                        }
                        // Otherwise, we need to interpolate the whole position (xyz) from the anchor pos (the last position
                        // where the node was told to lock/unlock) to the animated position on the ground
                        // While the idle transition is waiting (not started), just keep the position at the anchor pos.
                        else
                        {
                           if (pIKNode->mIdleTransitionLockStarted)
                           {
                              // jce [11/14/2008] -- IK targets are in local space now, so need to get back to world space.
                              BVector animatedBonePosOnGround;
                              worldMatrix.transformVectorAsPoint(animatedBonePos, animatedBonePosOnGround);
                              
                              BVector worldAnchorPos;
                              worldMatrix.transformVectorAsPoint(anchorPos, worldAnchorPos);

                              // jce [11/14/2008] -- move height into world space.
                              animatedBonePosOnGround.y = height;
                              targetBonePos.lerpPosition(interpFactor, worldAnchorPos, animatedBonePosOnGround);

                              // Back to local space.
                              inverseWorldMatrix.transformVectorAsPoint(targetBonePos, targetBonePos);
                           }
                           else
                           {
                              // Keep locked at anchor until lock transition started
                              targetBonePos = anchorPos;
                           }
                        }
                     }
                     // If coming up from ground, blend up from anchor (starting) position
                     else
                     {
                        // TODO fix this api - Get anchor data since we 
                        anchorPos = pIKNode->getAnchorPos();
                        lockStartTime = pIKNode->mStart;
                        lockEndTime = pIKNode->mEnd;
                        lockComplete = pIKNode->mLockComplete;

                        // Current anim time
                        float duration = mpVisual->getAnimationDuration(cMovementAnimationTrack);
                        float time = mpVisual->getAnimationPosition(cMovementAnimationTrack);
                        float normalizedTime = time / duration;

                        // Interp factor.  If done interpolating, set the lock to complete
                        float interpFactor = 1.0f;
                        if (!lockComplete && normalizedTime >= lockStartTime && normalizedTime <= lockEndTime)
                        {
                           interpFactor = (normalizedTime - lockStartTime) / (lockEndTime - lockStartTime);
                           interpFactor = 1.0f - ((interpFactor - 1.0f) * (interpFactor - 1.0f)); // 1 - (x - 1)^2 smoothing
                        }
                        else
                           mpVisual->setIKNodeLockComplete(nodeIndex, true);

                        // If not idle transitioning, then we're just interpolating 'y' from the ground to the animated position
                        if (!pIKNode->mIdleTransitioning)
                        {
#ifdef CLAMP_TO_ANCHOR
                           targetBonePos.lerpPosition(interpFactor, anchorPos, animatedBonePos);
#else
                           // Lerp
                           targetBonePos = animatedBonePos;
                           float yInterp = Math::Lerp(anchorPos.y, animatedBonePos.y, interpFactor);
                           targetBonePos.y = yInterp;
#endif
                        }
                        // Otherwise, we need to interpolate to a guesstimate position since the animation won't continue
                        // the upward motion.
                        else
                        {
                           // jce [11/14/2008] -- IK targets are in local space now, so need to get back to world space.
                           BVector animatedBonePosOnGround;
                           worldMatrix.transformVectorAsPoint(animatedBonePos, animatedBonePosOnGround);
                           animatedBonePosOnGround.y = height;  // put it on ground
                           BVector worldAnchorPos;
                           worldMatrix.transformVectorAsPoint(anchorPos, worldAnchorPos);

                           // Lerp
                           BVector goalPos = (worldAnchorPos + animatedBonePosOnGround) * 0.5f;            // average of anchorPos and animatedBonePosOnGround
                           goalPos.y = worldAnchorPos.y + goalPos.xzDistance(worldAnchorPos);                   // lift amount is based on distance from anchor to goal
                           goalPos.y = Math::Clamp(goalPos.y, height, height + maxIKRange);           // clamp between groundHeight and maxIKRange
                           targetBonePos.lerpPosition(interpFactor, worldAnchorPos, goalPos);
                           
                           // Back to local space.
                           inverseWorldMatrix.transformVectorAsPoint(targetBonePos, targetBonePos);
                        }
                     }

                     // Set IK node
                     
                     // Update the IK node.
                     mpVisual->setIKNode(nodeIndex, targetBonePos);

                     // Lock nodes on first update
                     if (firstUpdate)
                     {
                        mpVisual->lockIKNodeToGround(boneHandle, true, 0.0f, 0.0f);
                        mpVisual->setIKNodeActive(nodeIndex, true);
                     }

                     // Debugging
                     #ifndef BUILD_FINAL
                        if (gConfig.isDefined(cConfigDebugIK))
                        {
                           // jce [11/14/2008] -- new positions are in local space, so we need to transform them into
                           // world space for debugging.
                           BVector temp;

                           worldMatrix.transformVectorAsPoint(anchorPos, temp);
                           gpDebugPrimitives->addDebugSphere(temp, 1.0f, cDWORDRed);

                           worldMatrix.transformVectorAsPoint(animatedBonePos, temp);
                           gpDebugPrimitives->addDebugSphere(temp, 1.0f, cDWORDGreen);

                           worldMatrix.transformVectorAsPoint(targetBonePos, temp);
                           if (hasAnchor)
                              gpDebugPrimitives->addDebugSphere(temp, 1.5f, cDWORDBlue);
                           else
                              gpDebugPrimitives->addDebugSphere(temp, 1.5f, cDWORDYellow);
                        }
                     #endif

                  }
                  else
                  {
                     mpVisual->setIKNodeActive(nodeIndex, false);
                  }

                  nodeIndex++;
               }
            }

            if (numSweetSpotIKNodes)
            {
               // Iterate through sweet spot IK nodes and calculate target positions
               for (long i = 0; i < numSweetSpotIKNodes; i++)
               {
                  BVector sweetSpotPos;
                  float start;
                  float sweetSpot;
                  float end;

                  if (mpVisual->getIKNodeSweetSpot(nodeIndex, sweetSpotPos, start, sweetSpot, end))
                  {
                     float duration = mpVisual->getAnimationDuration(cActionAnimationTrack);
                     float time = mpVisual->getAnimationPosition(cActionAnimationTrack);
                     float normalizedTime = time / duration;

                     if ((normalizedTime >= start) && (normalizedTime <= end))
                     {
                        long boneHandle = mpVisual->getIKNodeBoneHandle(nodeIndex);

                        // Get bone position from animation
                        BVector animatedBonePos;
                        if (mpVisual->getBone(boneHandle, &animatedBonePos, NULL, NULL, &worldMatrix, false))
                        {
                           float weight = (normalizedTime <= sweetSpot) ? ((normalizedTime - start) / (sweetSpot - start)) : (1.0f - ((normalizedTime - sweetSpot) / (end - sweetSpot)));
                           BVector bonePos;

                           // blending with sweet spot
                           bonePos.lerpPosition(weight, animatedBonePos, sweetSpotPos);

                           // Set bone position
                           mpVisual->setIKNode(nodeIndex, bonePos);
                        }
                     }
                     else
                     {
                        // Not in sweet spot time window. Deactivate.
                        mpVisual->setIKNodeActive(nodeIndex, false);
                     }
                  }

                  nodeIndex++;
               }
            }
         }
      }
   }
}

//==============================================================================
// BObject::updateMotionExtraction
//==============================================================================
void BObject::updateMotionExtraction(float elapsedTime)
{
   // mrh 12/17/07 - put this in an opt-in config instead.
   if (!(gConfig.isDefined(cConfigEnableMotionExtraction) || gWorld->isPlayingCinematic()))
   {
      #ifdef SYNC_Anim
         if (getClassType() != cClassTypeObject)
         {
            syncAnimData("BObject::updateMotionExtraction ConfigEnableMotionExtraction", gConfig.isDefined(cConfigEnableMotionExtraction));
            syncAnimData("BObject::updateMotionExtraction isPlayingCinematic", gWorld->isPlayingCinematic());
         }
      #endif
      return;
   }

   if (mpVisual)
   {
      #ifdef SYNC_Anim
         if (getClassType() != cClassTypeObject)
         {
            syncAnimData("BObject::updateMotionExtraction FlagSkipMotionExtraction", getFlagSkipMotionExtraction());
            syncAnimData("BObject::updateMotionExtraction FlagAnimationDisabled", getFlagAnimationDisabled());
            syncAnimData("BObject::updateMotionExtraction FlagPhysicsControl", getFlagPhysicsControl());
            syncAnimData("BObject::updateMotionExtraction model asset type", mpVisual->mModelAsset.mType);
         }
      #endif

      if (!(getFlagSkipMotionExtraction() || getFlagAnimationDisabled() || (getClassType() == cClassTypeProjectile)))
      {
         //if (!isAnimationLocked())
         //   elapsedTime *= mAnimationRate; // Apply animation rate multiplier

         if ((mpVisual->mModelAsset.mType == cVisualAssetGrannyModel) && !getFlagPhysicsControl())
         {
//-- FIXING PREFIX BUG ID 6081
            const BGrannyInstance* pGrannyInstance = (BGrannyInstance *) mpVisual->mpInstance;
//--

            #ifdef SYNC_Anim
               if (getClassType() != cClassTypeObject)
               {
                  if (pGrannyInstance)
                  {
                     syncAnimData("BObject::updateMotionExtraction granny instance hasMotionExtraction", pGrannyInstance->hasMotionExtraction());
                  }
                  else
                  {
                     syncAnimCode("BObject::updateMotionExtraction no granny instance");
                  }
               }
            #endif

            if (pGrannyInstance && pGrannyInstance->hasMotionExtraction())
            {
               BMatrix worldMatrix;
               getWorldMatrix(worldMatrix);
               BMatrix oldWorldMatrix = worldMatrix; // save off for later use below

               pGrannyInstance->getExtractedMotion(elapsedTime, worldMatrix);

               BVector newPosition;
               worldMatrix.getTranslation(newPosition);

               #ifdef SYNC_Anim
                  if (getClassType() != cClassTypeObject)
                  {
                     syncAnimData("BObject::updateMotionExtraction mPosition", mPosition);
                     syncAnimData("BObject::updateMotionExtraction newPosition", newPosition);
                  }
               #endif

               if(!Math::IsValidFloat(newPosition.x))
               {
                  #ifdef SYNC_Anim
                     if (getClassType() != cClassTypeObject)
                     {
                        syncAnimCode("BObject::updateMotionExtraction invalid float return");
                     }
                  #endif
                  return;
               }

               #ifdef SYNC_Anim
                  if (getClassType() != cClassTypeObject)
                  {
                     syncAnimData("BObject::updateMotionExtraction getFlagMotionCollisionChecked", getFlagMotionCollisionChecked());
                  }
               #endif

               ///////////////////////////////////////////////////////////////////////
               // SLB: Prevent motion extraction from moving objects onto obstructions
               if (!getFlagMotionCollisionChecked() && !getFlagIsUnderCinematicControl())
               {
                  setFlagMotionCollisionChecked(true);

                  const BUnit* pUnit = getUnit();
                  if (!(pUnit && pUnit->getFlagDoingFatality()))
                  {
                     #ifdef SYNC_Anim
                        if (getClassType() != cClassTypeObject)
                        {
                           syncAnimCode("BObject::updateMotionExtraction getFlagDoingFatality");
                        }
                     #endif
                     BVector intersection;
                     if (getNearestCollision(getPosition(), newPosition, true, true, true, intersection))
                     {
                        newPosition = intersection;
                        #ifdef SYNC_Anim
                           if (getClassType() != cClassTypeObject)
                           {
                              syncAnimData("BObject::updateMotionExtraction intersection", intersection);
                           }
                        #endif
                     }
                  }
               }
               ///////////////////////////////////////////////////////////////////////

               #ifdef SYNC_Unit
                  if (isClassType(BEntity::cClassTypeUnit))
                     syncUnitData("BObject::updateMotionExtraction", newPosition);
               #endif
               setPosition(newPosition);

               if (getFlagTiesToGround() && getFlagUseMaxHeight())
               {
                  float y;
                  gTerrainSimRep.getHeightRaycast(mPosition, y, true);

                  #ifdef SYNC_Anim
                     if (getClassType() != cClassTypeObject)
                     {
                        syncAnimData("BObject::updateMotionExtraction y", y);
                     }
                  #endif

                  if (y > mPosition.y)
                  {
                     tieToGround();
                     #ifdef SYNC_Anim
                        if (getClassType() != cClassTypeObject)
                        {
                           syncAnimData("BObject::updateMotionExtraction mPosition", mPosition);
                        }
                     #endif
                  }
               }

               setRotation(worldMatrix);

               //==============================================================================
               // If this object is attached to another, update the attachment offset for motion extraction
               BObjectAttachment* pObjectAttachment = getAttachedToObjectAttachment();
               if (pObjectAttachment && pObjectAttachment->mUseOffset)
               {
                  // Calculate motion extraction difference
                  BMatrix fromPosMatrix;
                  oldWorldMatrix.invert();
                  fromPosMatrix.mult(worldMatrix, oldWorldMatrix);

                  // Calculate new offset matrix
                  BMatrix newOffsetMatrix;
                  newOffsetMatrix.mult(fromPosMatrix, pObjectAttachment->mOffset);
                  pObjectAttachment->mOffset = newOffsetMatrix;
               }

               //==============================================================================
               // Sync
               #ifdef SYNC_Anim
                  if (getClassType() != cClassTypeObject)
                  {
                     syncAnimData("BObject::updateMotionExtraction position", getPosition());
                     syncAnimData("BObject::updateMotionExtraction forward", getForward());
                     syncAnimData("BObject::updateMotionExtraction up", getUp());
                     syncAnimData("BObject::updateMotionExtraction right", getRight());
                  }
               #endif
            }
         }
      }
      else
         setFlagSkipMotionExtraction(false);
   }

   #ifdef SYNC_Anim
      if (getClassType() != cClassTypeObject)
      {
         syncAnimCode("BObject::updateMotionExtraction end");
      }
   #endif
}

//==============================================================================
// BObject::getNearestCollision
//==============================================================================
bool BObject::getNearestCollision(BVector startPosition, BVector endPosition, bool collideWithTerrain, bool collideWithStaticObjects, bool collideWithMovingObjects, BVector &intersection) const
{
   static BObstructionNodePtrArray collisionObs;
   collisionObs.setNumber(0);

   long quadTrees = 0;
   if (collideWithTerrain)
   {
      switch (getProtoObject()->getMovementType())
      {
      case cMovementTypeLand:
         quadTrees |= BObstructionManager::cIsNewTypeBlockLandUnits;
         break;
      case cMovementTypeFlood:
         quadTrees |= BObstructionManager::cIsNewTypeBlockFloodUnits;
         break;
      case cMovementTypeScarab:
         quadTrees |= BObstructionManager::cIsNewTypeBlockScarabUnits;
         break;
      }
   }
   if (collideWithStaticObjects)
      quadTrees |= BObstructionManager::cIsNewTypeAllCollidableUnits;
   if (!collideWithMovingObjects)
      quadTrees &= ~BObstructionManager::cIsNewTypeCollidableMovingUnit;

   long nodeTypes = BObstructionManager::cObsNodeTypeAllSolid;

   // SLB: disregard collisions with object's own squad
   const BUnit* pUnit = getUnit();
   const BSquad* pSquad = (pUnit) ? pUnit->getParentSquad() : NULL;
   const BEntityIDArray* pIgnoreList = (pSquad) ? &pSquad->getChildList() : NULL;

   intersection.zero();
   gObsManager.begin(BObstructionManager::cBeginEntity, getID().asLong(), getClassType(), quadTrees, nodeTypes, 0, cDefaultRadiusSofteningFactor, pIgnoreList, canJump());
   bool intersectionFound = gObsManager.getObjectsIntersections(BObstructionManager::cGetNearestIntersect, startPosition, endPosition, false, intersection, collisionObs);
   gObsManager.end();

   // SLB: disregard collisions if we start out obstructed
   if (intersectionFound && XMVector3NearEqual(startPosition, intersection, XMVectorReplicate(cFloatCompareEpsilon)))
      intersectionFound = false;

   return intersectionFound;
}

//==============================================================================
//==============================================================================
float BObject::getInterpolation(DWORD forSubUpdate)
{
   return gWorld->getUpdateCompletion(forSubUpdate);
}

//==============================================================================
//==============================================================================
void BObject::getInterpolatedMatrix(const BMatrix &matrix1, const BMatrix &matrix2, BMatrix &interpolatedMatrix, DWORD forSubUpdate, bool interpolateScale)
{
   if (!gEnableSubUpdating)
   {
      interpolatedMatrix = matrix2;
      return;
   }

   float interpolation = getInterpolation(forSubUpdate); // FIXME: Make this code faster
   //if (interpolation == 1.0f)
   //{
   //   interpolatedMatrix = matrix2;
   //   return;
   //}

   // Get translation first in case interpolated matrix is the same as matrix1 or matrix2
   BVector pos1, pos2, pos3;
   matrix1.getTranslation(pos1);
   matrix2.getTranslation(pos2);   
   pos3.lerpPosition(interpolation, pos1, pos2);

   // Get quat1, quat2
   BQuaternion quat1, quat2;
   float interpolatedScale = 1.0f;

   // Re-normalize matrices before making the quaternions if there is scale to interpolate
   if (interpolateScale)
   {
      BVector temp;
      BMatrix scaleMtx, tempMtx1, tempMtx2;

      // This assumes uniform scale (only grabbing it from one row)
      matrix1.getRight(temp);
      float scale1 = temp.length();
      scaleMtx.makeScale(1.0f / scale1);
      tempMtx1.mult(scaleMtx, matrix1);

      // This assumes uniform scale (only grabbing it from one row)
      matrix2.getRight(temp);
      float scale2 = temp.length();
      scaleMtx.makeScale(1.0f / scale2);
      tempMtx2.mult(scaleMtx, matrix2);

      quat1.set(tempMtx1);
      quat2.set(tempMtx2);

      interpolatedScale = Math::Lerp(scale1, scale2, interpolation);
   }
   // Otherwise just make quats from the input matrices (presumably these don't have scale)
   else
   {
      quat1.set(matrix1);
      quat2.set(matrix2);
   }

   // Rotation
   BQuaternion quat3;
   quat1.slerp(quat2, interpolation, quat3);
   quat3.toMatrix(interpolatedMatrix);

   // Translation
   interpolatedMatrix.setTranslation(pos3);

   // Apply interpolated scale
   if (interpolateScale)
   {
      BMatrix scaleMtx;
      scaleMtx.makeScale(interpolatedScale);
      interpolatedMatrix.mult(scaleMtx, interpolatedMatrix);
   }
   //else
   //{
   //   // We're assuming here that if we aren't interpolating scale then we can take the existing scale from the first matrix
   //   float xScale, yScale, zScale;
   //   matrix1.getScale(xScale, yScale, zScale);
   //   BMatrix scaleMtx;
   //   scaleMtx.makeScale(xScale, yScale, zScale);
   //   interpolatedMatrix.mult(scaleMtx, interpolatedMatrix);
   //}
}

//==============================================================================
//==============================================================================
BVector BObject::getInterpolatedPosition() const
{
   if (!gEnableSubUpdating || mFlagDontInterpolate)
      return getPosition();

   float interpolation = gWorld->getUpdateCompletion(mSubUpdateNumber);
   if (interpolation == 1.0f)
   {
      BVector pos;
      mNewWorldMatrix.getTranslation(pos);
      return pos;
   }

   BVector v, p1, p2;
   mOldWorldMatrix.getTranslation(p1);
   mNewWorldMatrix.getTranslation(p2);
   v.lerpPosition(interpolation, p1, p2);
   return v;
}

//==============================================================================
//==============================================================================
BVector BObject::getInterpolatedUp() const
{
   if (!gEnableSubUpdating || mFlagDontInterpolate)
      return getUp();

   float interpolation = gWorld->getUpdateCompletion(mSubUpdateNumber);
   if (interpolation == 1.0f)
   {
      BVector up;
      mNewWorldMatrix.getUp(up);
      return up;
   }

   BQuaternion quat1(mOldWorldMatrix);
   BQuaternion quat2(mNewWorldMatrix);
   BQuaternion quat3;
   quat1.slerp(quat2, interpolation, quat3);
   BMatrix interpolatedMatrix;
   quat3.toMatrix(interpolatedMatrix);
   BVector v;
   interpolatedMatrix.getUp(v);
   return v;
}

//==============================================================================
//==============================================================================
BVector BObject::getInterpolatedRight() const
{
   if (!gEnableSubUpdating || mFlagDontInterpolate)
      return getRight();

   float interpolation = gWorld->getUpdateCompletion(mSubUpdateNumber);
   if (interpolation == 1.0f)
   {
      BVector right;
      mNewWorldMatrix.getRight(right);
      return right;
   }

   BQuaternion quat1(mOldWorldMatrix);
   BQuaternion quat2(mNewWorldMatrix);
   BQuaternion quat3;
   quat1.slerp(quat2, interpolation, quat3);
   BMatrix interpolatedMatrix;
   quat3.toMatrix(interpolatedMatrix);
   BVector v;
   interpolatedMatrix.getRight(v);
   return v;
}

//==============================================================================
//==============================================================================
BVector BObject::getInterpolatedForward() const
{
   if (!gEnableSubUpdating || mFlagDontInterpolate)
      return getForward();

   float interpolation = gWorld->getUpdateCompletion(mSubUpdateNumber);
   if (interpolation == 1.0f)
   {
      BVector fwd;
      mNewWorldMatrix.getForward(fwd);
      return fwd;
   }

   BQuaternion quat1(mOldWorldMatrix);
   BQuaternion quat2(mNewWorldMatrix);
   BQuaternion quat3;
   quat1.slerp(quat2, interpolation, quat3);
   BMatrix interpolatedMatrix;
   quat3.toMatrix(interpolatedMatrix);
   BVector v;
   interpolatedMatrix.getForward(v);
   return v;
}

//==============================================================================
//==============================================================================
void BObject::setInterpolationMatrices(const BMatrix &matrix)
{
   mOldWorldMatrix = matrix;
   mNewWorldMatrix = matrix;
}

//==============================================================================
//==============================================================================
void BObject::resetSubUpdating()
{
   getWorldMatrix(mOldWorldMatrix);
   mNewWorldMatrix = mOldWorldMatrix;
   mSubUpdateNumber = gWorld->getSubUpdate();
}

//==============================================================================
// BObject::updateVisualWorldMatrix
//==============================================================================
void BObject::updateVisualWorldMatrix()
{
   SCOPEDSAMPLE(BObject_updateVisualWorldMatrix);  
   //-- update the world matrix of the visual
   if (mpVisual && mpVisual->isWorldMatrixUpdateNeeded())
   {
      //SCOPEDSAMPLE(BObjectUpdateVisualWorldMatrix);
      if (gEnableSubUpdating && !mFlagDontInterpolate)
      {
         BMatrix matrix;
         getInterpolatedMatrix(mOldWorldMatrix, mNewWorldMatrix, matrix, mSubUpdateNumber, getFlagIsFading() /* scales on fading */);
         mpVisual->updateWorldMatrix(matrix, NULL);
      }
      else
      {
         BMatrix worldMatrix;
         getWorldMatrix(worldMatrix);      
         mpVisual->updateWorldMatrix(worldMatrix, NULL);
      }      
   }
}

//==============================================================================
// BObject::getVisualIsVisible()
//==============================================================================
bool BObject::getVisualIsVisible() const
{
   if (mpVisual)
   {
      SCOPEDSAMPLE(BObjectUpdateVisualVisibility);
      const BUser * const pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
      bool visible = false;
      if (!mFlagOccluded && !mFlagNoRender && (!isGarrisoned() || isInCover()))
      {
         if (getFlagNoRenderForOwner() && pUser->getPlayerID() == getPlayerID())
         {
            // MS 4/23/2008: putting this at top because it supersedes even the "all visible" call below.
            // The reason is that this function's return value is used to set visibility on this object's
            // visual, which goes through a rendering pathway separate from BObject::render().
            visible = false;
         }
         else if (gWorld->getFlagAllVisible())
         {
            visible = true;
         }
         else if (getFlagVisibleToAll())
         {
            visible = true;
         }
         else if (getFlagVisibleForOwnerOnly())
         {
            if (gWorld->getFlagCoop() && (pUser->getTeamID() == getPlayer()->getTeamID()))
            {
               visible =  isVisible(pUser->getTeamID());
            }
            else
            {
               visible =  (pUser->getPlayerID() == getPlayerID());
            }
         }
         else
         {
            visible = isVisible(pUser->getTeamID());
         }
      }

      if (getFlagOverrideTint() && (0 == D3DCOLOR_GETALPHA(mOverrideTint)))
      {
         visible = false;
      }

      return visible;
   }

   return false;
}

//==============================================================================
void BObject::setFlagSecondaryTurretScanToken(long hardpointIndex, bool v)
{
   if(hardpointIndex < 0 || hardpointIndex >= mHardpointState.getNumber())
      return;

   mHardpointState[hardpointIndex].mSecondaryTurretScanToken = v; 
}

//==============================================================================
bool BObject::getFlagSecondaryTurretScanToken(long hardpointIndex)
{ 
   if(hardpointIndex < 0 || hardpointIndex >= mHardpointState.getNumber())
      return false;

   return mHardpointState[hardpointIndex].mSecondaryTurretScanToken; 
}


//==============================================================================
// BObject::updateVisualVisibility()
//==============================================================================
void BObject::updateVisualVisibility()
{
   // don't update the visibility status of the objects visual if we are marked
   // for destruction.  Just retain the last visibility state from the last
   // frame.  This is a bug fix to prevent lights and particles to become invisible
   // prematurely because their parent objects dies.
   if (getFlagDestroy())
      return;

   if (mpVisual)
   {
      mpVisual->updateVisibility(getVisualIsVisible());
   }
}

//=============================================================================
// BObject::updateBoundingBox
//=============================================================================
void BObject::updateBoundingBox()
{
   //SCOPEDSAMPLE(BObjectUpdateBoundingBox);

   // Update visual BB
   if(mpVisual)
   {
      BVector maxPoint = mpVisual->getMaxCorner();
      BVector minPoint = mpVisual->getMinCorner();
      BMatrix xfrm;
      getWorldMatrix(xfrm);
      mBoundingBox.initializeTransformed(minPoint, maxPoint, xfrm);
      mRadius = mBoundingBox.getSphereRadius();

      /*
      if (mpPhantom)
      {
         hkAabb aabb;
         float minObsXZ = Math::Min(getProtoObject()->getObstructionRadiusX(), getProtoObject()->getObstructionRadiusZ());
         BVector min = BVector(-minObsXZ, 0.0f, -minObsXZ);
         BVector max = BVector(minObsXZ, getProtoObject()->getObstructionRadiusY(), minObsXZ);
         aabb.m_min = mPosition + min;
         aabb.m_max = mPosition + max;
         //aabb.m_min = minPoint + mPosition;
         //aabb.m_max = maxPoint + mPosition;
         ((hkpAabbPhantom*)mpPhantom)->setAabb(aabb);
      }
      */
   }

   //-- sim bounding box
   updateSimBoundingBox();
}

//==============================================================================
// BObject::updateLifespan
//==============================================================================
void BObject::updateLifespan()
{
   // If we don't have a lifespan, do nothing.
   if (!getFlagHasLifespan())
      return;

   // We have a lifespan and we are expired, so go away.
   if (gWorld->getGametime() >= mLifespanExpiration)
   {
#ifdef SYNC_Unit
      if(getClassType() != cClassTypeObject)
      {
         syncUnitData("BObject::updateLifespan mID", mID.asLong())
      }
#endif

      // fade out objects aftr death.  Lifetime and Fades should be actions so we don't do them for 
      // all objects / units / etc.
      if (mFlagFadeOnDeath)
      {
         if (!getFlagIsFading())
         {
            if(getProtoObject())
            {
               float fadeTime= getProtoObject()->getDeathFadeTime();
               enableAlphaFade(true, fadeTime);
            }
            else
            {
               enableAlphaFade(true, cDefaultDeathFadeTime);
            }
         }
      }
      else
      {
         kill(true);
      }
   }
}

//==============================================================================
// BObject::updateAlpha
//==============================================================================
void BObject::updateAlpha()
{
   if (!getFlagIsFading())
      return;

   mCurrentAlphaTime += gWorld->getLastUpdateLengthFloat();

   if (mCurrentAlphaTime > mAlphaFadeDuration)
   {
      mCurrentAlphaTime = mAlphaFadeDuration;

      // destroy after our fade is done because we were fading on death
      if (mFlagFadeOnDeath)
         kill(true);

      //setFlagIsFading(false);
   }
}

//==============================================================================
// BObject::updateAttachments
//==============================================================================
void BObject::updateAttachments(float elapsedTime, bool moving)
{
   //SCOPEDSAMPLE(BObject_updateAttachments);

   if(mpObjectAttachments)
   {
      for(long i=0; i<mpObjectAttachments->getNumber(); i++)
      {
         const BObjectAttachment& objectAttachment = mpObjectAttachments->get(i);
         BEntity* pAttachment=gWorld->getEntity(objectAttachment.mAttachmentObjectID);
         if(!pAttachment)
         {
            mpObjectAttachments->removeIndex(i, false);
            i--;
         }
         else
         {
            if (!objectAttachment.mIsUnitAttachment)
            {
               // Assign object's visibility info to attachments
               BObject *pObject = pAttachment->getObject();
               if (pObject)
               {
                  pObject->assertVisibility();

                  long numTeams = gWorld->getNumberTeams();
                  for (long teamID = 1; teamID < numTeams; teamID++)
                  {
                     if (isVisible(teamID))
                     {
                        pObject->makeVisible(teamID);
                     }
                     else if (mDoppleBits.isSet(teamID))
                     {
                        pObject->makeSoftDopple(teamID);
                     }
                     else if (hasDoppleObject(teamID) && !pObject->hasDoppleObject(teamID))
                     {
                        pObject->makeHardDopple(teamID);
                     }
                     else
                     {
                        pObject->makeInvisible(teamID);
                     }
                  }

                  pObject->assertVisibility();
               }
            }

            if (mpVisual)
            {
               BMatrix toWorldMatrix, toPosMatrix, fromPosMatrix, transformMatrix, attachMatrix, finalAttachMatrix;
               getWorldMatrix(toWorldMatrix);
               BObject* pAttachmentObject = pAttachment->getObject();
//-- FIXING PREFIX BUG ID 6084
               const BVisual* pAttachmentVisual = NULL;
//--
               if (pAttachmentObject)
                  pAttachmentVisual = pAttachmentObject->getVisual();
               if(mpVisual->getBone(objectAttachment.mToBoneHandle, NULL, &toPosMatrix, NULL, NULL))
               {
                  if (objectAttachment.mFromBoneHandle != -1 && pAttachmentVisual && pAttachmentVisual->getBone(objectAttachment.mFromBoneHandle, NULL, &fromPosMatrix, NULL, NULL))
                  {
                     fromPosMatrix.invert();
                     transformMatrix.mult(fromPosMatrix, objectAttachment.mOffset);
                     attachMatrix.mult(transformMatrix, toPosMatrix);
                  }
                  else
                  {
                     attachMatrix.mult(objectAttachment.mOffset, toPosMatrix);                      
                  }
               }
               else
               {
                  if (objectAttachment.mFromBoneHandle != -1 && pAttachmentVisual && pAttachmentVisual->getBone(objectAttachment.mFromBoneHandle, NULL, &fromPosMatrix, NULL, NULL))
                  {
                     fromPosMatrix.invert();
                     attachMatrix.mult(fromPosMatrix, objectAttachment.mOffset);
                  }
                  else
                  {
                     attachMatrix = objectAttachment.mOffset;
                  }
               }

               finalAttachMatrix.mult(attachMatrix, toWorldMatrix);
               pAttachment->setWorldMatrix(finalAttachMatrix);
            }
            else if (moving)
            {
               #ifdef SYNC_Unit
                  if (pAttachment->isClassType(BEntity::cClassTypeUnit))
                     syncUnitData("BObject::updateAttachments", mPosition);
               #endif
               pAttachment->setPosition(mPosition);
               pAttachment->setForward(mForward);
               pAttachment->setRight(mRight);
               pAttachment->setUp(mUp);
            }

            BObject* pObject = pAttachment->getObject();
            if (pObject)
               pObject->update(elapsedTime);
         }
      }
   }
}

#include "math\VMXIntersection.h"

//==============================================================================
// BObject::renderVisual
//==============================================================================
void BObject::renderVisual()
{
   const bool nearLayer = getFlagNearLayer();

   BVisualRenderAttributes renderAttributes;
      
   BVector min, max;
   mBoundingBox.computeWorldCorners(min, max);

   // Setup obscurable for objects or live/non-fading units
   if (getFlagObscurable() && ( (!getFlagIsFading() && isAlive()) || isClassType(BEntity::cClassTypeObject)))
   {
      renderAttributes.mObscurable = true;
      renderAttributes.mPlayerColorIndex = getColorPlayerID();
   }   

   float LODFadeFactor = 1.0f;
   bool LODFade = false;
   
   float projArea;
   bool projResult = BVMXIntersection::calculateBoxArea(
      projArea, 
      gRenderDraw.getMainActiveMatrixTracker().getWorldCamPos(), 
      min, max,
      XMMatrixIdentity(),
      gRenderDraw.getMainActiveMatrixTracker().getMatrix(cMTWorldToScreen));
         
   if ((!nearLayer) && getFlagLODFading() && (projResult))
   {
      float projRadius = sqrt(projArea * (1.0f / Math::fPi));

      static float startFade = 7.0f;
      static float endFade = 5.0f;

      if (projRadius < endFade)
      {
         // jce [11/5/2008] -- Ok this is a super confusing hack.  Basically if you are a particle attached to nothing, you will have 
         // a tiny bounding box since particle bounding boxes are not factored into mBoundingBox. (Same if you have a particle attached but are 
         // just tiny onscreen).  However, the particles do their own culling and they might (probably) still be drawing 
         // and therefore need to have their interpolated matrix updated.  In the new code, that update occurs during render, which we're 
         // wanting to skip due to this LOD fading stuff.  This means that the  particle will stop interpolating.  To address this, I'm forcing 
         // an update using the old updateWorldMatrix method which is not going to be completely correct if the particles are attached 
         // to something which has stopped rendering but are themselves still rendering.  The alternative would be doing a ton of 
         // work almost rendering to get the attach position completely right (which seems overkill since the thing you're attached 
         // to isn't drawing because it's so tiny).  In the case of a particle attached to nothing, the result is correct in either case.
         if(gEnableSubUpdating)
         {
            BMatrix matrix;
            getInterpolatedMatrix(mOldWorldMatrix, mNewWorldMatrix, matrix, mSubUpdateNumber, getFlagIsFading());
            mpVisual->updateWorldMatrix(matrix, NULL);
         }
         
         return;
      }
      else if (projRadius < startFade)
      {
         LODFade = true;
         LODFadeFactor = ((projRadius - endFade) / (startFade - endFade));
      }
   }         

   renderAttributes.setBounds(min, max);
   renderAttributes.mProjectedArea = projResult ? projArea : -1.0f;

   BMatrix matrix;
   if (gEnableSubUpdating && !mFlagDontInterpolate)
   {
      getInterpolatedMatrix(mOldWorldMatrix, mNewWorldMatrix, matrix, mSubUpdateNumber, getFlagIsFading() /* scales on fading */);
      #ifndef BUILD_FINAL
      long id = -1;
      if (gConfig.get(cConfigDecUpdTraceObj, &id) && id == mID.asLong())
      {
         static float oldInt = 0.0f;
         static DWORD oldSubUpdate = 0;
         float interpolation = getInterpolation(mSubUpdateNumber);
         if (mSubUpdateNumber == oldSubUpdate && interpolation < oldInt)
         {
            BASSERT(0);
         }
         BVector pos;
         matrix.getTranslation(pos);
         trace("   ObjRenderVisual - Upd# %5u, WorldSubUpd# %5u, ObjSubUpd# %5u, Interpolation %6.3f, int x %6.3f, act x %6.3f", gWorld->getUpdateNumber(), gWorld->getSubUpdate(), mSubUpdateNumber, interpolation, pos.x, mPosition.x);
         oldInt = interpolation;
         oldSubUpdate = mSubUpdateNumber;
      }
      #endif
   }
   else
      getWorldMatrix(matrix);
   gRender.setWorldMatrix(matrix);
   

   if (isAlive())
      renderAttributes.mPixelXFormColor = gWorld->getPlayerColor(getColorPlayerID(), BWorld::cPlayerColorContextObjects);
   else
   {
      BColor player(gWorld->getPlayerColor(getColorPlayerID(), BWorld::cPlayerColorContextObjects));
      BColor corpse(gWorld->getCorpseColor(getColorPlayerID(), BWorld::cPlayerColorContextObjects));
      float lerpVal;
      if (getFlagIsFading() || getFlagFadeOnDeath())
         lerpVal = 1.0f;
      else
         lerpVal = gCorpseManager.getCorpseDecay(mID);

      BColor result = player + ((corpse - player) * lerpVal);
      renderAttributes.mPixelXFormColor = result.asDWORD();
   }

   renderAttributes.mTintColor = 0xFF000000;

   renderAttributes.mAOTintValue = mAOTintValue;

   renderAttributes.mNearLayer = nearLayer;

   renderAttributes.mAppearsBelowDecals = mFlagAppearsBelowDecals;
   
   // Disable blackmap/FoW sampling on all near layer objects.
   renderAttributes.mSampleBlackmap = !nearLayer;
   if (nearLayer)
      renderAttributes.mShadowReceiver = false;

   BCOMPILETIMEASSERT(sizeof(mUVOffsets) == sizeof(renderAttributes.mUVOffsets));
   memcpy(renderAttributes.mUVOffsets, mUVOffsets, sizeof(renderAttributes.mUVOffsets));
   
   renderAttributes.mMultiframeTextureIndex = mMultiframeTextureIndex;

   renderAttributes.mHighlightIntensity = mHighlightIntensity;
#if 0
// HACK HACK - Emmissive and highlight intensity test
static float emmInten = 1.0f;
static float highlightInten = 1.0f;
renderAttributes.mEmissiveIntensity = emmInten;
renderAttributes.mHighlightIntensity = highlightInten;
#endif
   
#if 0      
   static bool shadowReceiver = false;
   static bool shadowCaster = false;
   static bool globalLighting = false;
   static bool localLighting = false;
   static bool farLayer = false;

   renderAttributes.mShadowReceiver = shadowReceiver;
   renderAttributes.mShadowCaster   = shadowCaster;
   renderAttributes.mGlobalLighting = globalLighting;
   renderAttributes.mLocalLighting  = localLighting;
   renderAttributes.mFarLayer       = farLayer;
#endif      

   // add in any additional textures
   if (mpAdditionalTextures && mpAdditionalTextures->getSize() > 0)
   {
      BUberVisualRenderAttributes* pUberVisualRenderAttributes = gRenderThread.allocateFrameStorageObj<BUberVisualRenderAttributes, true>();

      for (uint i = 0; i < mpAdditionalTextures->getSize(); ++i)
      {
         BAdditionalTexture& texture = mpAdditionalTextures->get(i);
         if (texture.RenderType == cATAdditive)
         {
            pUberVisualRenderAttributes->mAddTexture = BMeshEffectTextures::getInstance().get(texture.Texture);
            pUberVisualRenderAttributes->mAddTexInten = texture.TexInten;
            pUberVisualRenderAttributes->mAddTexClamp = texture.TexClamp;
            BColor color(mOverrideTint);
            pUberVisualRenderAttributes->mAddTexR = color.r;
            pUberVisualRenderAttributes->mAddTexG = color.g;
            pUberVisualRenderAttributes->mAddTexB = color.b;
            if (texture.ModulateIntensity)
            {
               const double adjustedTime = fmod(gRenderControl.getSimPrevRenderTime(), 1.0); // between 0.0 and 1.0 sec
               pUberVisualRenderAttributes->mAddTexInten *= (sin(adjustedTime * cTwoPi) * .5f + .5f);
            }
               
            if (texture.ModulateOffset)
            {
               // Calculate the uv offset depending on if this is a wrapped or clamped texture
               float uvOfs = 0.0f;
               if (texture.TexClamp)
               {
                  uvOfs = (gWorld->getSubGametime() - texture.TexStartTime) * 0.001f * texture.TexScrollSpeed;
                  if (texture.TexScrollLoop && (uvOfs < -4.0f))
                  {
                     // Reset uv offset and starttime if we're looping around
                     uvOfs = 0.0f;
                     texture.TexStartTime = gWorld->getSubGametime();

                     // Reset scale/height offset
                     float scale = 1.0f;
                     float minY;
                     if (getVisualBoundingBox())
                     {
                        BVector minCorner, maxCorner;
                        getVisualBoundingBox()->computeWorldCorners(minCorner, maxCorner);
                        float yExtent = maxCorner.y - minCorner.y;
                        if (yExtent > cFloatCompareEpsilon || yExtent < -cFloatCompareEpsilon)
                           scale = -1.0f / yExtent;
                        minY = minCorner.y;
                     }
                     else
                        minY = getPosition().y;
                     float heightOffset = 2.0f - (minY * scale);
                     texture.TexUVScale = scale;
                     texture.TexUVOfs.set(heightOffset, heightOffset);
                  }
               }
               else
               {
                  if (texture.TexScrollSpeed > 0.0f)
                  {
                     double adjustedTime = fmod(gRenderControl.getSimPrevRenderTime(), 1.0 / texture.TexScrollSpeed) * texture.TexScrollSpeed;
                     uvOfs = 1.0f - adjustedTime;
                  }
               }

               pUberVisualRenderAttributes->mAddTexUVXFormU.set(0.0f, texture.TexUVScale, 0.0f, uvOfs + texture.TexUVOfs[0]);
               pUberVisualRenderAttributes->mAddTexUVXFormV = pUberVisualRenderAttributes->mAddTexUVXFormU;
            }
            else
               pUberVisualRenderAttributes->setAddTexUVOfsAndScale(texture.TexUVOfs[0], texture.TexUVOfs[1], texture.TexUVScale);
         }
         else if (texture.RenderType == cATLerp)
         {
            const double adjustedTime = fmod(gRenderControl.getSimPrevRenderTime(), 1.0); // between 0.0 and 1.0 sec
            pUberVisualRenderAttributes->mLerpTexture = BMeshEffectTextures::getInstance().get(texture.Texture);
            pUberVisualRenderAttributes->mLerpTexClamp = texture.TexClamp;
            
            pUberVisualRenderAttributes->mLerpTexOpacity = texture.TexInten;
            if (texture.ModulateIntensity)
               pUberVisualRenderAttributes->mLerpTexOpacity *= (sin(adjustedTime * cTwoPi) * .5f + .5f);
               
            if (texture.ModulateOffset)
            {
               float uvOfs = 1.0f - fmod(adjustedTime * texture.TexScrollSpeed, 1.0);
               pUberVisualRenderAttributes->setLerpTexUVOfsAndScale(uvOfs, uvOfs, texture.TexUVScale);
            }
            else
            {
               pUberVisualRenderAttributes->setLerpTexUVOfsAndScale(texture.TexUVOfs[0], texture.TexUVOfs[1], texture.TexUVScale);
            }               
         }
      }

      renderAttributes.mpExtendedAttributes = pUberVisualRenderAttributes;
   }

   float unit_opacity = mpVisual->getAnimationOpacity();
   int unit_a = Math::iClampToByte(255 * unit_opacity);

   if (LODFade)
   {
      int a = Math::iClampToByte(255 * LODFadeFactor);
      renderAttributes.mTintColor = D3DCOLOR_ARGB(Math::Min(unit_a, a), 0, 0, 0);
   }
   else if (getFlagIsFading())
   {
      float alphaTime;
      if (gEnableSubUpdating)
      {
         float interpolation = gWorld->getUpdateCompletion(mSubUpdateNumber);
         float elapsed = gWorld->getLastUpdateLengthFloat();
         float oldTime = mCurrentAlphaTime - elapsed;
         alphaTime = oldTime + (interpolation * elapsed);
      }
      else
         alphaTime = mCurrentAlphaTime;
      int a = Math::iClampToByte(255 * (1.0f - (alphaTime / mAlphaFadeDuration)));
      renderAttributes.mTintColor = D3DCOLOR_ARGB(Math::Min(unit_a, a), 0, 0, 0);
   }
   else
   {
      renderAttributes.mTintColor = D3DCOLOR_ARGB(unit_a, 0, 0, 0);
   }

   //Used for pulsing the color of the unit when it is being selected
   if( mSelectionPulseTime > 0.00f || mSelectionFlashTime > 0.00f)
   {
      //static variables for quick debugging tweaks
      static float pulsedeltamodifier = 1.0f;
      static bool pixelxform = false;
      static bool white = false;
      static bool halfwhite = true;
      static float topclamp = 0.8f;
      static float bottomclamp = 0.2f;
      static float flashcycle = 0.2f;
      static float radiusDenominator = 75.0f;

      float deltaTime = (float)gWorld->getTotalRealtime() - mLastRealtime;
      mLastRealtime = (float)gWorld->getTotalRealtime();

      //Does this work for split screen?  May need some way of telling which user this is for.
      const BUser * const pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
      if (pUser && (pUser->getUserMode() != BUser::cUserModeCommandMenu) && (pUser->getUserMode() != BUser::cUserModeAbility) && (pUser->getUserMode() != BUser::cUserModePowerMenu))
      {
         float pulseSpeed = mSelectionPulseSpeed;
         if (mSelectionFlashTime > 0.00f)
            pulseSpeed = 6.0f;            //fast to indicate a unit recently selected

         mSelectionPulsePercent += deltaTime * pulsedeltamodifier * pulseSpeed;
         if( mSelectionPulsePercent > 1.0f )
            mSelectionPulsePercent = -1.0f;

         if( pixelxform )  //modify the unit's player color (cycle it to white)
         {
            float percent = abs(mSelectionPulsePercent);
            float invpercent = 1.0f - percent;
            DWORD r = (renderAttributes.mPixelXFormColor & 0x00FF0000) >> 16;
            DWORD g = (renderAttributes.mPixelXFormColor & 0x0000FF00) >> 8;
            DWORD b = (renderAttributes.mPixelXFormColor & 0x000000FF);
            r = (DWORD(r*invpercent) + DWORD(0xFF*percent)) & 0x000000FF;
            g = (DWORD(g*invpercent) + DWORD(0xFF*percent)) & 0x000000FF;
            b = (DWORD(b*invpercent) + DWORD(0xFF*percent)) & 0x000000FF;
            renderAttributes.mPixelXFormColor = 0xFF000000 + (r << 16) + (g << 8) + b;
         }
         else  //user the player color to tint the unit
         {
            float topclampmodifier = getVisualRadius() / radiusDenominator;  //large bases are around 40 meters, single units around 3 meters
            if (topclampmodifier > 0.7f) 
               topclampmodifier = 0.7f;
            else if (topclampmodifier < 0.0f) 
               topclampmodifier = 0.0f;
            topclampmodifier = 1.0f - topclampmodifier;
            float percent = abs(mSelectionPulsePercent) * topclamp * topclampmodifier + bottomclamp;

            DWORD a = D3DCOLOR_GETALPHA(renderAttributes.mTintColor);
            DWORD r = D3DCOLOR_GETRED(renderAttributes.mPixelXFormColor);
            DWORD g = D3DCOLOR_GETGREEN(renderAttributes.mPixelXFormColor);
            DWORD b = D3DCOLOR_GETBLUE(renderAttributes.mPixelXFormColor);
            if( white )  //test for pulsing to white instead of player color
               r = g = b = 0xFF;
            else if( halfwhite )
            {
               r = (r + 0xFF)/ 2;
               g = (g + 0xFF)/ 2;
               b = (b + 0xFF)/ 2;
            }
            r = (DWORD(r*percent)) & 0xFF;
            g = (DWORD(g*percent)) & 0xFF;
            b = (DWORD(b*percent)) & 0xFF;
            renderAttributes.mTintColor = D3DCOLOR_ARGB(a, r, g, b);
         }
      }
      else
      {
         mSelectionFlashTime = 0.0f;  //turn off flashing once a menu is opened
      }

      if( mSelectionPulseTime > 0.00f )
         mSelectionPulseTime -= (float)gWorld->getLastUpdateLengthFloat();
         //mSelectionPulseTime -= deltaTime; MWC to be added
      if( mSelectionFlashTime > 0.00f )
         mSelectionFlashTime -= (float)gWorld->getLastUpdateLengthFloat();
         //mSelectionFlashTime -= deltaTime; MWC to be added
   }
   else
      mSelectionPulsePercent = 0.0f;

   DWORD gameTime = gWorld->getGametime();
   if ((mOverrideFlashDuration != 0) && (gameTime < mOverrideFlashDuration))
   {
      if (!mFlagOverrideTint && (gameTime >= mOverrideFlashIntervalTimer))
      {
         mFlagOverrideTint = true;
         mOverrideFlashIntervalTimer = gameTime + mOverrideFlashInterval;
      }
      else if (mFlagOverrideTint && (gameTime >= mOverrideFlashIntervalTimer))
      {
         mFlagOverrideTint = false;
         mOverrideFlashIntervalTimer = gameTime + mOverrideFlashInterval;
      }
   }
   else
   {
      mOverrideFlashDuration = 0;      
      mFlagOverrideTint = false;
   }

   if (mFlagOverrideTint)
   {
      renderAttributes.mTintColor = mOverrideTint;
   }

   // do not render fully alpha visual
   if (0 == D3DCOLOR_GETALPHA(renderAttributes.mTintColor))
      return;

   // Call override render function or standard visual render
   if (mpOverrideRenderCB)
      mpOverrideRenderCB(this, &renderAttributes, gWorld->getSubUpdate(), gWorld->getAmountOfSubUpdates(), 1.0f/gWorld->getSubUpdateTimeInMsecs());
   else
      mpVisual->render(&renderAttributes);

}

//==============================================================================
// BObject::render
//==============================================================================
void BObject::render()
{
#ifndef BUILD_FINAL
   static bool bOnlyRenderGaia = false;
   if (bOnlyRenderGaia)
   {
      if (getPlayerID() != 0)
         return;
   }
#endif   

   // Physics object debug render
   BPhysicsObject* pPO = getPhysicsObject();
   if (pPO && gConfig.isDefined(cConfigDebugRenderShape))
      pPO->renderShape();

   if (mFlagNoRender)
      return;

   if (mFlagNoRenderDuringCinematic && gWorld->isPlayingCinematic())
      return;

   // early out if we don't render for owner
   if(getFlagNoRenderForOwner() && gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayerID() == getPlayerID())
      return;   

   if(mpVisual && (getFlagVisibleToAll() || !getFlagVisibleForOwnerOnly() || (gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayerID() == getPlayerID())
      || (gWorld->getFlagCoop() && (gUserManager.getUser(BUserManager::cPrimaryUser)->getTeamID() == getPlayer()->getTeamID()))))
   {
      // jce [11/3/2008] -- This now occurs in the new version of render using the correct matrices for interpolated attach points
      //if (gEnableSubUpdating && !mFlagDontInterpolate)
        // updateVisualWorldMatrix();

      renderVisual();

      //XXXHalwes 4/27/2007 - temporary until external shields get art
//-- FIXING PREFIX BUG ID 6087
      //const BUnit* pUnit = getUnit();
//--
      /*if (pUnit && pUnit->isExternalShield())
      {
         float sp = pUnit->getHPPercentage();
         DWORD shieldColor = 0;
         if (sp >= 0.75)
         {
            shieldColor = D3DCOLOR_ARGB(255, 0, 255, 0);               
         }
         else if ((sp < 0.75f) && (sp >= 0.25f))
         {
            shieldColor = D3DCOLOR_ARGB(255, 255, 255, 0);
         }
         else if (sp < 0.25f)
         {
            shieldColor = D3DCOLOR_ARGB(255, 255, 0, 0);
         }
         float radiusX = pUnit->getObstructionRadiusX();
         float yOffset = pUnit->getObstructionRadiusY() * 0.5f;
         BVector pos = pUnit->getPosition();
         pos.y -= yOffset;
         gpDebugPrimitives->addDebugSphere(pos, radiusX, shieldColor);
         //XXXHalwes - 6/5/2007 - debug
         //BVector pos2 = pUnit->getPosition();
         //BVector delta;
         //delta.set(pUnit->getObstructionRadiusX(), pUnit->getObstructionRadiusY(), pUnit->getObstructionRadiusX());
         //BVector min = pos2 - delta;
         //BVector max = pos2 + delta;
         //min.y = pos2.y;
         //gpDebugPrimitives->addDebugBox(min, max, D3DCOLOR_ARGB(255, 0, 0, 255));
      }*/
   }
   else if(getFlagBlockLOS())
   {
      BMatrix matrix;
      float radius=getLOS();
      float radiusStep=radius/20.0f;
      float heightStep=20.0f/5.0f;
      BVector pos=mPosition;
      for(long i=0; i<4; i++)
      {
         matrix.makeTranslate(pos);
         gpDebugPrimitives->addDebugCircle(matrix, radius, cDWORDRed);
         pos.y+=heightStep;
         radius-=radiusStep;
      }
   }

#ifndef BUILD_FINAL
#ifdef DEBUG_RENDER_BOUNDING_BOX
   mBoundingBox.draw(cDWORDLightGrey, false);
#endif      

#ifdef DEBUG_RENDER_BOUNDING_SPHERE   
   gpDebugPrimitives->addDebugSphere(mBoundingBox.getCenter(), mBoundingBox.getSphereRadius(), 0xFFFFFFFF, BDebugPrimitives::cCategoryNone, -1.0f);
#endif      

#ifdef DEBUG_RENDER_SIM_BOUNDING_BOX
   mSimBoundingBox.draw(cDWORDOrange, false);
#endif      

   // Debug physics phantom rendering
   /*if (gConfig.isDefined(cConfigDebugRenderShape) && mpPhantom)
   {
      BVector min, max;
      mpPhantom->getAabbMinMax(min, max);
      gpDebugPrimitives->addDebugBox(min, max, cDWORDGold);
   }*/

   // Debug area attack range
   if (mFlagDebugRenderAreaAttackRange)
   {
      BUnit* pUnit = getUnit();
      if (pUnit)
      {
//-- FIXING PREFIX BUG ID 6086
         const BAction* pAction = getActionByType(BAction::cActionTypeUnitAreaAttack);
//--
         if (pAction)
         {
            BMatrix matrix;
            float radius=pAction->getProtoAction()->getMaxRange(NULL, false);
            float step=radius/5.0f;
            BVector pos=mPosition;
            pos.y += 1.0f;
            for(long i=0; i<4; i++)
            {
               matrix.makeTranslate(pos);
               gpDebugPrimitives->addDebugCircle(matrix, radius, cDWORDRed);
               radius-=step;
            }
         }
      }
   }
   
   
   if(gConfig.isDefined(cConfigRenderInterpolationPercent))
   {
      float interpolation = gWorld->getUpdateCompletion(mSubUpdateNumber);
      
      BString str = "[";
      const cTotalTicks = 20;
      long numActiveTicks = long(cTotalTicks*interpolation);
      long i = 0;
      for(; i<numActiveTicks; i++)
      {
         str += "*";
      }
      for(; i<cTotalTicks; i++)
      {
         str += " ";
      }
      
      BString str2;
      str2.format("]  %4.1f", interpolation*100.0f);
      
      str += str2;
      
      gpDebugPrimitives->addDebugText(str.getPtr(), getInterpolatedPosition(), 0.2f, cDWORDWhite);
      
      /*
      if(isClassType(BEntity::cClassTypeUnit))
      {
         static bool started=false;
         static LARGE_INTEGER startTime = {0};
         static LARGE_INTEGER lastTime = {0};
         if(!started)
         {
            QueryPerformanceCounter(&startTime);
            QueryPerformanceCounter(&lastTime);
            started=true;
         }
         
         LARGE_INTEGER currRealTime;
         QueryPerformanceCounter(&currRealTime);
         str2.format(" delta=%5.2f currRealElapsedMS=%5.2f sub#%8d fromSub#%8d", 1000.0f*float((currRealTime.QuadPart-lastTime.QuadPart)/gWorld->getLastTimerFrequency()), 1000.0f*float((currRealTime.QuadPart-startTime.QuadPart)/gWorld->getLastTimerFrequency()), gWorld->getSubUpdate(), mSubUpdateNumber);
         str += str2;
         QueryPerformanceCounter(&lastTime);
         
         trace(str);
      }
      */
   }

   //DCP 04/20/07: Turning this off unless someone really, really needs it.
   //BMatrix worldMatrix;
   //getWorldMatrix(worldMatrix);
   //debugRender(worldMatrix);
#endif
}

//==============================================================================
// BObject::setProtoID
//==============================================================================
void BObject::setProtoID(long protoID)
{
   mProtoID = protoID;

   // Cache the proto object pointer
   BProtoObject* pProtoObject = NULL;
   if (mProtoID != cInvalidProtoID)
   {
      BPlayer *pPlayer = gWorld->getPlayer(mPlayerID);
      if (pPlayer)
         pProtoObject = pPlayer->getProtoObject(mProtoID);
   }
   setProtoObject(pProtoObject);
}

//==============================================================================
// BObject::getLOS()
//==============================================================================
float BObject::getLOS() const
{
   const BProtoObject* pProtoObject = getProtoObject();
   if (!pProtoObject)
      return 0.0f;

   float protoObjectLOS = pProtoObject->getProtoLOS();
   if (getFlagUseLOSScalar())
   {
      float scaledLOS;
      if (mLOSRevealTime < 0.0f)
         scaledLOS = mLOSScalar * protoObjectLOS;
      else
         scaledLOS = mLOSScalar * protoObjectLOS * mLOSRevealTime;

      return(scaledLOS);
   }
   else
   {
      return(protoObjectLOS);
   }
}


//==============================================================================
// BObject::getSimLOS()
//==============================================================================
long BObject::getSimLOS() const
{
   const BProtoObject* pProtoObject = getProtoObject();
   if (!pProtoObject)
      return 0.0f;

   if (getFlagUseLOSScalar())
   {
      float protoLOS = pProtoObject->getProtoLOS();
      if (protoLOS <= 0.0f)
      {
         return(0.0f);
      }
      float scaledLOS = protoLOS * mLOSScalar;
      float recipTileScale = gTerrainSimRep.getReciprocalDataTileScale();
      long scaledSimLOS = (long)(scaledLOS * recipTileScale);
      if (scaledSimLOS <= 0)
      {
         scaledSimLOS = getFlagIsRevealer() ? -1 : 1;
      }
      return(scaledSimLOS);
   }
   else
   {
      long protoObjectSimLOS = pProtoObject->getProtoSimLOS();
      return(protoObjectSimLOS);
   }
}


//==============================================================================
// BObject::setVisual
//==============================================================================
bool BObject::setVisual(long protoVisualIndex, int displayPriority, int64 userData)
{
   if(mpVisual)
      gVisualManager.releaseVisual(mpVisual);

   bool synced = (getClassType() != BEntity::cClassTypeObject);

   #ifdef SYNC_Unit
      if (synced)
      {
         const BProtoVisual* pProtoVisual = gVisualManager.getProtoVisual(protoVisualIndex, true);
         syncUnitData("BObject::setVisual protoVisual", pProtoVisual ? pProtoVisual->getName() : "");
      }
   #endif

   BMatrix worldMatrix;
   getWorldMatrix(worldMatrix);

   DWORD playerColor = gWorld->getPlayerColor(getPlayerID(), BWorld::cPlayerColorContextObjects);

   mpVisual=gVisualManager.createVisual(protoVisualIndex, synced, (userData == -1) ? mID.asLong() : userData, playerColor, worldMatrix, displayPriority);

   updateVisualWorldMatrix();

   BVector minPoint = mpVisual ? mpVisual->getMinCorner() : mPosition;
   BVector maxPoint = mpVisual ? mpVisual->getMaxCorner() : mPosition;
   mBoundingBox.initializeTransformed(minPoint, maxPoint, worldMatrix);
   mRadius = mBoundingBox.getSphereRadius();

   initIK();

   mAnimationState.setDirty();

   return(mpVisual!=NULL);
}

//==============================================================================
// BObject::setVisual
//==============================================================================
bool BObject::setVisual(const BVisual* pSource)
{
   if(mpVisual)
   {
      gVisualManager.releaseVisual(mpVisual);
      mpVisual=NULL;
   }

   BMatrix worldMatrix;
   getWorldMatrix(worldMatrix);

   DWORD playerColor = gWorld->getPlayerColor(getPlayerID(), BWorld::cPlayerColorContextObjects);

   if(pSource)
   {
      // SLB: Only create the visual from the proto if we're not a dopple and we have a proto visual, otherwise hand copy the visual from the source.
      // 1. We obviously can't create from a proto if we have no proto visual.
      // 2. We don't want to dopple from an object's proto in case the cause of the doppling is a tech upgrade, in which case the proto may be different.
      if ((getClassType() != BEntity::cClassTypeDopple) && pSource->getProtoVisualConst())
      {
         bool synced = (getClassType() != BEntity::cClassTypeObject);

         #ifdef SYNC_Unit
            if (synced)
            {
               syncAnimData("BObject::setVisual source minCorner", pSource->getMinCorner());
               syncAnimData("BObject::setVisual source maxCorner", pSource->getMaxCorner());
            }
         #endif

         mpVisual=gVisualManager.createVisual(pSource, synced, mID.asLong(), playerColor, worldMatrix);

         #ifdef SYNC_Unit
            if (synced)
            {
               syncAnimData("BObject::setVisual minCorner", mpVisual->getMinCorner());
               syncAnimData("BObject::setVisual maxCorner", mpVisual->getMaxCorner());
            }
         #endif
      }
      else
      {
         bool synced = (getClassType() != BEntity::cClassTypeObject);

         // Set flags
         setAnimationEnabled(false);

         // Create visual from visual item
         BVisual* pVisual = BVisual::getInstance();
         BASSERT(pVisual);
         if (!pVisual->clone(pSource, synced, mID.asLong(), false, playerColor, worldMatrix))
         {
            BASSERT(0);
         }

         // Set visual to the one we just created
         setVisualPtr(pVisual);
      }
   }

   updateVisualWorldMatrix();

   BVector minPoint = mpVisual ? mpVisual->getMinCorner() : mPosition;
   BVector maxPoint = mpVisual ? mpVisual->getMaxCorner() : mPosition;
   mBoundingBox.initializeTransformed(minPoint, maxPoint, worldMatrix);
   mRadius = mBoundingBox.getSphereRadius();

   initIK();

   mAnimationState.setDirty();

   return(mpVisual!=NULL);
}

//==============================================================================
//==============================================================================
bool BObject::setVisualPtr(BVisual* pVisual)
{
   if(mpVisual)
   {
      gVisualManager.releaseVisual(mpVisual);
      mpVisual=NULL;
   }

   mpVisual = pVisual;

   updateVisualWorldMatrix();

   BVector minPoint = mpVisual ? mpVisual->getMinCorner() : mPosition;
   BVector maxPoint = mpVisual ? mpVisual->getMaxCorner() : mPosition;
   BMatrix xfrm;
   getWorldMatrix(xfrm);
   mBoundingBox.initializeTransformed(minPoint, maxPoint, xfrm);
   mRadius = mBoundingBox.getSphereRadius();

   mAnimationState.setDirty();

   return(mpVisual!=NULL);
}

//==============================================================================
// BObject::initIK
//==============================================================================
void BObject::initIK()
{
   //-- Register IK nodes with visual item
   BVisual *pVisual = getVisual();
   if (pVisual)
   {
      const BProtoObject* pProto = getProtoObject();
      BASSERT(pProto);

      long numGroundIKNodes = pProto->getNumberGroundIKNodes();
      long numSweetSpotIKNodes = pProto->getNumberSweetSpotIKNodes();
      long numSingleBoneIKNodes = pProto->getNumberSingleBoneIKNodes();
      long numIKNodes = numGroundIKNodes + numSweetSpotIKNodes + numSingleBoneIKNodes;

      bool tilt = false;
      float tiltFactor = pProto->getGroundIKTiltFactor();
      if (tiltFactor > cFloatCompareEpsilon)
      {
         tilt = true;
         numIKNodes++;
      }

      if (pVisual->setNumIKNodes(numIKNodes))
      {
         long nodeIndex = 0;

         // Tilt IK
         if (tilt)
            pVisual->setIKNode(nodeIndex++, pVisual->getBoneHandle(pProto->getGroundIKTiltBoneName()), 0, XMVectorZero(), BIKNode::cIKNodeTypeSingleBone);

         // Ground IK
         for (long i = 0; i < numGroundIKNodes; i++)
         {
            const BGroundIKNode *pGroundIKNode = pProto->getGroundIKNode(i);
            pVisual->setIKNode(nodeIndex++, pVisual->getBoneHandle(pGroundIKNode->getBoneName()), pGroundIKNode->getLinkCount(), XMVectorZero(), BIKNode::cIKNodeTypeGround);
         }

         // Sweet Spot IK
         for (long i = 0; i < numSweetSpotIKNodes; i++)
         {
            const BSweetSpotIKNode *pSweetSpotIKNode = pProto->getSweetSpotIKNode(i);
            pVisual->setIKNode(nodeIndex++, pVisual->getBoneHandle(pSweetSpotIKNode->getBoneName()), pSweetSpotIKNode->getLinkCount(), XMVectorZero(), BIKNode::cIKNodeTypeSweetSpot);
         }

         // Single Bone IK
         for (long i = 0; i < numSingleBoneIKNodes; i++)
         {
            const BSingleBoneIKNode *pSingleBoneIKNode = pProto->getSingleBoneIKNode(i);
            pVisual->setIKNode(nodeIndex++, pVisual->getBoneHandle(pSingleBoneIKNode->getBoneName()), 0, XMVectorZero(), BIKNode::cIKNodeTypeSingleBone);
         }
      }
   }
}

//==============================================================================
//==============================================================================
bool BObject::ikIdleTransitionComplete() const
{
   if (!mpVisual)
      return true;
   const BProtoObject* pPO = getProtoObject();
   if (!pPO)
      return true;

   long numGroundIKNodes = pPO->getNumberGroundIKNodes();
   for (int i = 0; i < numGroundIKNodes; i++)
   {
      BIKNode* pIKNode = mpVisual->getIKNodeFromIndex(i);
      if (pIKNode)
      {
         if (pIKNode->mIdleTransitioning)
            return false;
      }
   }

   return true;
}

//==============================================================================
// BObject::computeVisual
//==============================================================================
void BObject::computeVisual()
{
   mAnimationState.setReset();
}

//=============================================================================
//=============================================================================
bool BObject::isMoveAnimType(long animType) const
{
   return ((animType == getWalkAnim()) || (animType == getJogAnim()) || (animType == getRunAnim()));
}

//=============================================================================
//=============================================================================
bool BObject::isTurnAnimType(long animType) const
{
   return ((animType == cAnimTypeTurnLeft) || (animType == cAnimTypeTurnRight));
}

//=============================================================================
//=============================================================================
long BObject::getMoveAnimType(void) const
{
   const BProtoObject* const pPO = getProtoObject();
   if (!pPO)
      return (getJogAnim());

   // Turn in place anims
   if (getFlagTurning() && pPO->getFlagTurnInPlace())
   {
      if (getFlagTurningRight())
         return cAnimTypeTurnRight;
      else
         return cAnimTypeTurnLeft;
   }

   long walkAnim=getWalkAnim();
   long jogAnim=getJogAnim();
   long runAnim=getRunAnim();

   // Pick thresholds based on current animation
   float wjScale, jrScale;
   long animation = getAnimationType(cActionAnimationTrack);
   if (animation == jogAnim)
   {
      wjScale = 0.25f;
      jrScale = 0.75f;
   }
   else if (animation == runAnim)
   {
      wjScale = 0.25f;
      jrScale = 0.25f;
   }
   else
   {
      wjScale = 0.75f;
      jrScale = 0.75f;
   }

   //Jog Velocity.
   float JV = getProtoObject()->getDesiredVelocity();
   //Run Velocity.
   float maxRV = getProtoObject()->getMaxVelocity();
   float minRV = JV + (maxRV - JV) * jrScale;
   //Walk Velocity.
   float maxWV = JV * wjScale;

   float velocity = getVelocity().length();

   //Walk.
   if (velocity <= maxWV)
      return (walkAnim);
   //Run.
   else if (velocity >= minRV)
      return (runAnim);
   //Jog.
   else
      return (jogAnim);
}

//==============================================================================
//==============================================================================
void BObject::computeAnimation()
{
   //SCOPEDSAMPLE(BObject_computeAnimation);

   debugObject("computeAnimation -->");

   bool enableAnimStateIsDirty = gConfig.isDefined(cConfigAllowAnimIsDirty);
   if (enableAnimStateIsDirty && !mAnimationState.isDirty())
   {
      if (!mAnimationState.isMoving() && !getFlagAnimationLocked())
         return;
   }

   if (!mpVisual || getFlagAnimationDisabled())
      return;

   bool synced = (getClassType() != BEntity::cClassTypeObject);

   // Get anim state
   long animState = mAnimationState.getState();
   bool movementLocked = false;
   bool actionLocked = false;

   // Process locked animation
   if (getFlagAnimationLocked())
   {
      #ifdef SYNC_Anim
      if (synced)
      {
         syncAnimData("BObject::computeAnimation mAnimationLockEnds", mAnimationLockEnds);
      }
      #endif

      if ((mAnimationLockEnds > 0) && (mAnimationLockEnds <= gWorld->getGametime()))
         unlockAnimation();
      else
      {
         actionLocked = true;
         movementLocked = !getFlagDontLockMovementAnimation();
         if (actionLocked && movementLocked)
            return;
      }
      if (enableAnimStateIsDirty && !mAnimationState.isDirty() && !mAnimationState.isMoving())
         return;
   }

   if (enableAnimStateIsDirty && !mAnimationState.isDirty())
   {
      // We can get here if the anim state is not dirty but it is moving. Need to see if move anim type needs to change.
      if (movementLocked)
         return;

      if (Math::fAbs(mVelocity.length() - mAnimationState.getMoveSpeed()) < 1.0f)
      {
         if (mFlagTurning == mAnimationState.isTurning())
            return;

         const BProtoObject* pProto = getProtoObject();
         if (!pProto || !pProto->getFlagWalkToTurn())
            return;
      }
   }

   long animType = mAnimationState.getAnimType();
   long animForceAnimID = mAnimationState.getForceAnim();
   bool applyInstantly = mAnimationState.isApplyInstantly();
   bool reset = mAnimationState.isReset();
   bool lock = mAnimationState.isLocked();

   #ifdef SYNC_Anim
   if (synced)
   {
      syncAnimData("BObject::computeAnimation animState", animState);
      syncAnimData("BObject::computeAnimation animType", gVisualManager.getAnimName(animType));
      syncAnimData("BObject::computeAnimation applyInstantly", applyInstantly);
      syncAnimData("BObject::computeAnimation reset", reset);
      syncAnimData("BObject::computeAnimation lock", lock);
   }
   #endif

   // Possibly reset idle anim
   if (reset && animState == BObjectAnimationState::cAnimationStateIdle)
   {
      animType = getIdleAnim();
      mAnimationState.setAnimType(animType);
      #ifdef SYNC_Anim
      if (synced)
      {
         syncAnimData("BObject::computeAnimation animType", gVisualManager.getAnimName(animType));
      }
      #endif
   }

   // Apply default animType if not set
   if (animType == -1)
   {
      switch (animState)
      {
         case BObjectAnimationState::cAnimationStateRangedAttack:
            animType = cAnimTypeRangedAttack;
            break;

         case BObjectAnimationState::cAnimationStateHandAttack:
            animType = cAnimTypeAttack;
            break;

         case BObjectAnimationState::cAnimationStateDeath:
            animType = getDeathAnim();
            break;

         case BObjectAnimationState::cAnimationStateWork:
            animType = cAnimTypeWork;
            break;

         case BObjectAnimationState::cAnimationStateResearch:
            animType = cAnimTypeResearch;
            break;

         case BObjectAnimationState::cAnimationStateTrain:
            animType = cAnimTypeTrain;
            break;

         default:
            animType = getIdleAnim();
            break;
      }

      mAnimationState.setAnimType(animType);
   }

   // Which animations are we using?
   long activeActionAnim = getAnimationType(cActionAnimationTrack);
   long activeMovementAnim = getAnimationType(cMovementAnimationTrack);

   // Determine if we were moving before and if we'll be moving now
   bool wasMoving = isMoveAnimType(activeMovementAnim);
   bool wasTurning = isTurnAnimType(activeMovementAnim);
   float velocity = isMoving() ? getDesiredVelocity() : 0.0f;
   bool isMoving = (!lock && (velocity > cFloatCompareEpsilon)) ? true : false;
   bool isTurning = getFlagTurning();
   const BProtoObject* pProto = getProtoObject();
   bool turnInPlace = pProto ? pProto->getFlagTurnInPlace() : false;
   bool ikTransitionToIdle = pProto ? pProto->getFlagIKTransitionToIdle() : false;
   long movementAnimType = -1;

   debugObject("ComputeAnimation - velocity: %f", velocity);
   debugObject("ComputeAnimation - isMoving: %d", (long)isMoving);
   debugObject("ComputeAnimation - wasMoving: %d", (long) wasMoving);

   #ifdef SYNC_Anim
   if (synced)
   {
      syncAnimData("BObject::computeAnimation wasMoving", wasMoving);
      syncAnimData("BObject::computeAnimation isMoving", isMoving);
      syncAnimData("BObject::computeAnimation velocity", velocity);
   }
   #endif


   bool updateVisualWorld = false;
   bool moveOverrideAction = false;
   bool actionOverrideMove = false;

   // If moving, get the move anim type and potentially override the action track anim
   if (isMoving || (turnInPlace && isTurning))
   {
      // Calculate movement anim type
      movementAnimType = getMoveAnimType();

      // Use movement to animate upper body if we're idle
      if (animState == BObjectAnimationState::cAnimationStateIdle)
      {
         animType = movementAnimType;
         moveOverrideAction = true;
      }
   }
   // If not moving, but need to transition to idle
   else if (ikTransitionToIdle && (wasMoving || wasTurning))
   {
      // Use walkIdle anim type for the transition to idle
      movementAnimType = cAnimTypeWalkIdle;

      // Use movement to animate upper body if we're idle
      if (animState == BObjectAnimationState::cAnimationStateIdle)
      {
         animType = movementAnimType;
         moveOverrideAction = true;
      }
   }
   // Otherwise, set movement to idle or action's anim
   else
   {
      if (pProto && pProto->getFlagNoActionOverrideMove() && isAlive() && (animState != BObjectAnimationState::cAnimationStateDeath))
      {
         movementAnimType = getIdleAnim(); // Use idle if not moving

         if (movementAnimType == animType)
         {
            moveOverrideAction = true;
         }
      }
      else
      {
         // Use action anim to animate lower body
         movementAnimType = animType;
         actionOverrideMove = true;
      }
   }

   bool nextAnimIdle = !isMoveAnimType(movementAnimType) && !isTurnAnimType(movementAnimType);

   // Get anim position if we were moving or syncing the ikTransitionToIdle
   if (wasMoving)
   {
      if (isMoving || (ikTransitionToIdle && nextAnimIdle))
      {
         long track = cMovementAnimationTrack;
         float duration = mpVisual->getAnimationDuration(track);
         mMoveAnimationPosition = (duration > 0.0f ? mpVisual->getAnimationPosition(track) / duration : 0.0f);
      }
      else
         mMoveAnimationPosition = 0.0f;
   }
   else
   {
      // Just started moving.

      // If we have a sync'd walk-to-idle transition, start walk back up at current anim position
      if (ikTransitionToIdle)
      {
         if (ikIdleTransitionComplete())
         {
            // If we were turning, sync up anims
            if (!isTurning && wasTurning)
            {
               long track = cMovementAnimationTrack;
               float duration = mpVisual->getAnimationDuration(track);
               mMoveAnimationPosition = (duration > 0.0f ? mpVisual->getAnimationPosition(track) / duration : 0.0f);
            }
            // If idle transition done, start move anim at 0
            else
               mMoveAnimationPosition = 0.0f;
         }
         // Otherwise, sync walk to current idle
         else
         {
            long track = cMovementAnimationTrack;
            float duration = mpVisual->getAnimationDuration(track);
            mMoveAnimationPosition = (duration > 0.0f ? mpVisual->getAnimationPosition(track) / duration : 0.0f);
         }
      }
      // Otherwise, give this a random start time so it isn't in sync with squadmates as much.  Mainly for infantry.
      else if (isMoving)
      {
         long randomTag = (synced ? cSimRand : cUnsyncedRand);
         if (pProto && pProto->getFlagRandomMoveAnimStart())
            mMoveAnimationPosition = getRandRangeFloat(randomTag, 0.0f, 1.0f);
      }
   }

   // Make sure we don't change locked animations
   if (actionLocked)
   {
      animType = activeActionAnim;
      moveOverrideAction = false;
   }
   if (movementLocked)
   {
      movementAnimType = activeMovementAnim;
      actionOverrideMove = false;
   }

   #ifdef SYNC_Anim
   if (synced)
   {
      syncAnimData("BObject::computeAnimation moveOverrideAction", moveOverrideAction);
   }
   #endif

#ifdef SYNC_Anim
   if(synced)
   {
      for (int i=0; i<cNumAnimationTracks; i++)
      {
         syncAnimData("BObject::computeAnimation before track", i);
         syncAnimData("BObject::computeAnimation before animType", gVisualManager.getAnimName(mpVisual->getAnimationType(i)));
         const BGrannyAnimation* pGrannyAnim = mpVisual->getAnimation(i);
         if (pGrannyAnim)
         {
            syncAnimData("BObject::computeAnimation before file", pGrannyAnim->getFilename());
         }
         syncAnimData("BObject::computeAnimation before numTags", mpVisual->getNumTags(i));
         syncAnimData("BObject::computeAnimation before exitAction", mpVisual->getAnimationExitAction(i));
         syncAnimData("BObject::computeAnimation before isClone", mpVisual->getAnimationIsClone(i));
         syncAnimData("BObject::computeAnimation before lock", mpVisual->getAnimationLock(i));
         syncAnimData("BObject::computeAnimation before animDur", mpVisual->getAnimationDuration(i));
         syncAnimData("BObject::computeAnimation before animPos", mpVisual->getAnimationPosition(i));
      }
   }
#endif

   if (moveOverrideAction)
   {
      #ifdef SYNC_Anim
      if (synced)
      {
         syncAnimData("BObject::computeAnimation activeMovementAnim", gVisualManager.getAnimName(activeMovementAnim));
         syncAnimData("BObject::computeAnimation movementAnimType", gVisualManager.getAnimName(movementAnimType));
         syncAnimData("BObject::computeAnimation activeActionAnim", gVisualManager.getAnimName(activeActionAnim));
         syncAnimData("BObject::computeAnimation actionOverrideMove", actionOverrideMove);
         syncAnimData("BObject::computeAnimation animType", gVisualManager.getAnimName(animType));
      }
      #endif

      // Process movement anim
      if (!movementLocked && (reset || (activeMovementAnim != movementAnimType)))
      {
         // Set new movement anim
         computeDopple();

         #ifdef SYNC_Anim
         if (synced)
         {
            syncAnimCode("BObject::computeAnimation setAnim 1");
         }
         #endif

         BMatrix worldMatrix;
         getWorldMatrix(worldMatrix);

         DWORD playerColor = gWorld->getPlayerColor(getPlayerID(), BWorld::cPlayerColorContextObjects);

         if (mAnimationState.isOverrideExitAction())
         {
            BProtoVisualAnimExitAction exitAction;
            mAnimationState.getOverrideExitAction(exitAction);
            mpVisual->setAnimation(cMovementAnimationTrack, movementAnimType, false, playerColor, worldMatrix, mMoveAnimationPosition, -1, false, NULL, &exitAction);
         }
         else
            mpVisual->setAnimation(cMovementAnimationTrack, movementAnimType, false, playerColor, worldMatrix, mMoveAnimationPosition, -1, false, NULL, NULL);
         mpVisual->copyAnimationTrack(cMovementAnimationTrack, cActionAnimationTrack, false, playerColor, worldMatrix);
         updateVisualWorld = true;
      }
      else if (!actionLocked && (activeActionAnim != movementAnimType))
      {
         computeDopple();

         #ifdef SYNC_Anim
         if (synced)
         {
            syncAnimCode("BObject::computeAnimation copyAnim 2");
         }
         #endif

         BMatrix worldMatrix;
         getWorldMatrix(worldMatrix);

         DWORD playerColor = gWorld->getPlayerColor(getPlayerID(), BWorld::cPlayerColorContextObjects);
         
         mpVisual->copyAnimationTrack(cMovementAnimationTrack, cActionAnimationTrack, applyInstantly, playerColor, worldMatrix);
         updateVisualWorld = true;
      }
   }
   else if (actionOverrideMove)
   {
      // Process action anim
      if (!actionLocked && (reset || (activeActionAnim != animType)))
      {
         // Set new action anim
         computeDopple();

         #ifdef SYNC_Anim
         if (synced)
         {
            syncAnimCode("BObject::computeAnimation setAnim 3");
         }
         #endif         

         BMatrix worldMatrix;
         getWorldMatrix(worldMatrix);

         DWORD playerColor = gWorld->getPlayerColor(getPlayerID(), BWorld::cPlayerColorContextObjects);

         if (mAnimationState.isOverrideExitAction())
         {
            BProtoVisualAnimExitAction exitAction;
            mAnimationState.getOverrideExitAction(exitAction);
            mpVisual->setAnimation(cActionAnimationTrack, animType, applyInstantly, playerColor, worldMatrix, 0.0f, animForceAnimID, reset, NULL, &exitAction);
         }
         else
            mpVisual->setAnimation(cActionAnimationTrack, animType, applyInstantly, playerColor, worldMatrix, 0.0f, animForceAnimID, reset, NULL, NULL);

         mpVisual->copyAnimationTrack(cActionAnimationTrack, cMovementAnimationTrack, applyInstantly, playerColor, worldMatrix);
         updateVisualWorld = true;
      }
      else if (!movementLocked && (activeMovementAnim != animType))
      {
         computeDopple();

         #ifdef SYNC_Anim
         if (synced)
         {
            syncAnimCode("BObject::computeAnimation copyAnim 4");
         }
         #endif

         BMatrix worldMatrix;
         getWorldMatrix(worldMatrix);

         DWORD playerColor = gWorld->getPlayerColor(getPlayerID(), BWorld::cPlayerColorContextObjects);

         mpVisual->copyAnimationTrack(cActionAnimationTrack, cMovementAnimationTrack, applyInstantly, playerColor, worldMatrix);
         updateVisualWorld = true;
      }
   }
   else
   {
      // Process movement anim
      if (!movementLocked && (reset || (activeMovementAnim != movementAnimType)))
      {
         // Set new movement anim
         computeDopple();

         #ifdef SYNC_Anim
         if (synced)
         {
            syncAnimCode("BObject::computeAnimation setAnim 5");
         }
         #endif

         BMatrix worldMatrix;
         getWorldMatrix(worldMatrix);

         DWORD playerColor = gWorld->getPlayerColor(getPlayerID(), BWorld::cPlayerColorContextObjects);

         if (mAnimationState.isOverrideExitAction())
         {
            BProtoVisualAnimExitAction exitAction;
            mAnimationState.getOverrideExitAction(exitAction);
            mpVisual->setAnimation(cMovementAnimationTrack, movementAnimType, false, playerColor, worldMatrix, mMoveAnimationPosition, -1, false, NULL, &exitAction);
         }
         else
            mpVisual->setAnimation(cMovementAnimationTrack, movementAnimType, false, playerColor, worldMatrix, mMoveAnimationPosition, -1, false, NULL, NULL);
         updateVisualWorld = true;
      }

      // Process action anim
      if (!actionLocked && (reset || (activeActionAnim != animType)))
      {
         // Set new action anim
         computeDopple();

         #ifdef SYNC_Anim
         if (synced)
         {
            syncAnimCode("BObject::computeAnimation setAnim 6");
         }
         #endif

         BMatrix worldMatrix;
         getWorldMatrix(worldMatrix);

         DWORD playerColor = gWorld->getPlayerColor(getPlayerID(), BWorld::cPlayerColorContextObjects);

         if (mAnimationState.isOverrideExitAction())
         {
            BProtoVisualAnimExitAction exitAction;
            mAnimationState.getOverrideExitAction(exitAction);
            mpVisual->setAnimation(cActionAnimationTrack, animType, applyInstantly, playerColor, worldMatrix, 0.0f, animForceAnimID, false, NULL, &exitAction);
         }
         else
            mpVisual->setAnimation(cActionAnimationTrack, animType, applyInstantly, playerColor, worldMatrix, 0.0f, animForceAnimID, false, NULL, NULL);
         updateVisualWorld = true;
      }
   }

   bool forceAnimRate = (pProto && pProto->getFlagForceAnimRate());
   if (forceAnimRate)
   {
      mpVisual->setAnimationRate(cActionAnimationTrack, mAnimationRate);
      mpVisual->setAnimationRate(cMovementAnimationTrack, mAnimationRate);
   }
   else
   {
      mpVisual->setAnimationRate(cActionAnimationTrack, moveOverrideAction ? mAnimationRate : 1.0f);
      mpVisual->setAnimationRate(cMovementAnimationTrack, actionOverrideMove ? 1.0f : mAnimationRate);
   }

   // Update visual world matrix
   if (updateVisualWorld)
   {
      setFlagNotDoppleFriendly((animState != BObjectAnimationState::cAnimationStateIdle) && (animState != BObjectAnimationState::cAnimationStateDeath) && (animState != BObjectAnimationState::cAnimationStateResearch)); //SLB: The animation we just set should not be used in a dopple
      updateVisualWorldMatrix();
      mpVisual->validateAnimationTracks();
      updateVisualVisibility();

#ifdef SYNC_Anim
      if(synced)
      {
         for (int i=0; i<cNumAnimationTracks; i++)
         {
            syncAnimData("BObject::computeAnimation after track", i);
            syncAnimData("BObject::computeAnimation after animType", gVisualManager.getAnimName(mpVisual->getAnimationType(i)));
            const BGrannyAnimation* pGrannyAnim = mpVisual->getAnimation(i);
            if (pGrannyAnim)
            {
               syncAnimData("BObject::computeAnimation after file", pGrannyAnim->getFilename());
            }
            syncAnimData("BObject::computeAnimation after numTags", mpVisual->getNumTags(i));
            syncAnimData("BObject::computeAnimation after isClone", mpVisual->getAnimationIsClone(i));
            syncAnimData("BObject::computeAnimation after lock", mpVisual->getAnimationLock(i));
            syncAnimData("BObject::computeAnimation after exitAction", mpVisual->getAnimationExitAction(i));
            syncAnimData("BObject::computeAnimation after animDur", mpVisual->getAnimationDuration(i));
         }
      }
#endif
   }

   mAnimationState.clearReset();
   mAnimationState.clearLock();

   if (!isMoving && !forceAnimRate)
      mAnimationRate = 1.0f;

   if (lock)
      lockAnimation((long)((mpVisual->getAnimationDuration(cActionAnimationTrack) - mpVisual->getAnimationTweenTime(cActionAnimationTrack)) * 1000.0f), false);

   if (enableAnimStateIsDirty)
   {
      // Set the moving status in the anim state
      if (updateVisualWorld) // this var is set whenever the anim is changed, so if it's not set then just use wasMoving.
      {
         if (isMoveAnimType(getAnimationType(cMovementAnimationTrack)))
         {
            mAnimationState.setMoving();
            mAnimationState.setMoveSpeed(mVelocity.length());
         }
         else
            mAnimationState.clearMoving();
      }
      else
      {
         if (wasMoving)
         {
            mAnimationState.setMoving();
            mAnimationState.setMoveSpeed(mVelocity.length());
         }
         else
            mAnimationState.clearMoving();
      }

      if (mFlagTurning)
         mAnimationState.setTurning();
      else
         mAnimationState.clearTurning();

      mAnimationState.clearDirty();
   }
}

//==============================================================================
// BObject::computeDopple
//==============================================================================
void BObject::computeDopple()
{
   //-- Don't dopple if we are faded out AND not alive
  /* if(this->mFlagIsFading == true &&
      this->mFlagHasLifespan == true &&
      this->mCurrentAlphaTime >= this->mAlphaFadeDuration &&
      this->isAlive() == false &&
      this->getClassType() == BEntity::cClassTypeUnit)
   {
      return;
   }*/
   //SCOPEDSAMPLE(BObject_computeDopple)
   // ajl and sle 3/26/08 - allow dopple while dead so that we don't have alpha terrain problems (black squares)
   //if (isAlive())
   {
      // Dopple if we need to
      long numTeams = gWorld->getNumberTeams();
      for (long i = 1; i < numTeams; i++)
      {
         if (mDoppleBits.isSet(i))
         {
            makeHardDopple(i);
         }
      }
   }
}

//==============================================================================
// BObject::getWorldMatrix
//==============================================================================
void BObject::getWorldMatrix(BMatrix& matrix) const
{
   matrix.makeOrient(mForward, mUp, mRight);

   if (mFlagUseCenterOffset)
   {
      BVector newPos(mPosition);
      newPos += (mCenterOffset.z * mForward);
      newPos += (mCenterOffset.y * mUp);
      newPos += (mCenterOffset.x * mRight);
      matrix.setTranslation(newPos);
   }
   else
      matrix.setTranslation(mPosition);
}

//==============================================================================
// BObject::init
//==============================================================================
void BObject::init()
{
   //-- first the base class
   BEntity::init();

   //-- Clear flags
   clearFlags();

   setProtoID(cInvalidProtoID);

   mpVisual=NULL;
   mBoundingBox.initialize(XMVectorZero(), XMVectorZero());
   mRadius = mBoundingBox.getSphereRadius();
   mAnimationRate=1.0f;
   mMoveAnimationPosition=0.0f;
   mAOTintValue=255;
   mHighlightIntensity=0.0f;
   Utils::ClearObj(mUVOffsets);
   mMultiframeTextureIndex = 0;

   mSimBoundingBox.initialize(cOriginVector, cOriginVector);

   mPlayerVisibility.zero();
   mDoppleBits.zero();
   mSimX = -1;
   mSimZ = -1;

   mLOSRevealTime=-1.0f;
   mLOSScalar = 1.0f;
   setFlagUseLOSScalar(false);
   setFlagLOSDirty(false);

   mpAdditionalTextures=NULL;
   mpObjectAttachments=NULL;

   setFlagVisibility(true);   
   setFlagIsFading(false);
   setAnimationEnabled(true);
   setFlagBlockLOS(false);
   setFlagAnimationLocked(false);
   setFlagUpdateSquadPositionOnAnimationUnlock(false);
   setFlagDontLockMovementAnimation(false);

   mCurrentAlphaTime = 0.0f;
   mAlphaFadeDuration = 0.0f;
   mAnimationLockEnds = 0;

   mSelectionPulseTime = 0.0f;   
   mSelectionPulsePercent = 0.0f;
   mSelectionFlashTime = 0.0f;
   mSelectionPulseSpeed = 1.0f;

   mAnimationState.clear();

   mIconColorSize = cInvalidVector;
   mIconColorSize.w = -1.0f;

#if defined(BOBJECT_DEBUG_VISIBLEMAP)
   mLOSX = mLOSZ = mLOSTeamID = -1;
#endif

   // Tint
   mFlagOverrideTint = false;
   mOverrideTint = 0xFF000000;
   mOverrideFlashDuration = 0;
   mOverrideFlashInterval = 0;
   mOverrideFlashIntervalTimer = 0;

   mLastRealtime = 0.0f; 

   mHardpointState.clear();

   // set no exploration group
   mExplorationGroup = -1;

   // all teams can select by default
   fillTeamSelectionMask(true);

   mCenterOffset.zero();

   mColorPlayerID = cInvalidPlayerID;
   mSubUpdateNumber = 0;

   mNewWorldMatrix.makeIdentity();
   mOldWorldMatrix.makeIdentity();

   mpOverrideRenderCB = NULL;
}

//==============================================================================
// BObject::clearFlags
//==============================================================================
void BObject::clearFlags(void)
{
   mFlagVisibility=false;
   mFlagLOS=false;   
   mFlagHasLifespan=false;
   mFlagDopples=false;
   mFlagIsFading=false;
   mFlagAnimationDisabled=false;
   mFlagIsRevealer=false;
   mFlagDontInterpolate=false;
   mFlagBlockLOS=false;
   mFlagCloaked=false;
   mFlagCloakDetected=false;
   mFlagGrayMapDopples=false;
   mFlagLOSMarked=false;
   mFlagUseLOSScalar=false;
   mFlagLOSDirty=false;
   mFlagAnimationLocked=false;
   mFlagUpdateSquadPositionOnAnimationUnlock=false;
   mFlagExistSoundPlaying=false;
   mFlagNoUpdate=false;
   mFlagSensorLockTagged=false;
   mFlagNoReveal=false;
   mFlagBuilt=false;
   mFlagBeingCaptured=false;
   mFlagInvulnerable=false;
   mFlagVisibleForOwnerOnly=false;
   mFlagIKDisabled=false;
   mFlagNearLayer=false;
   mFlagLODFading=true;
   mFlagOccluded=false;
   mFlagFadeOnDeath=false;
   mFlagHasTrackMask=false;
   mFlagObscurable=false;
   mFlagNoRender=false;
   mFlagTurning=false;
   mFlagAppearsBelowDecals=false;
   mFlagSkipMotionExtraction=false;
   mFlagMotionCollisionChecked=false;
   mFlagIsDopple = false;
   mFlagIsImpactEffect = false;
   mFlagDebugRenderAreaAttackRange = false;
   mFlagDontLockMovementAnimation = false;
   mFlagDontAutoAttackMe = false;
   mFlagAlwaysAttackReviveUnits = false;
   mFlagRemainVisible = false;
   mFlagNoRenderForOwner = false;
   mFlagNoRenderDuringCinematic = false;
   mFlagUseCenterOffset = false;
   mFlagNotDoppleFriendly = false;
   mFlagForceVisibilityUpdateNextFrame = false;
   mFlagTurningRight = false;
   mFlagIsUnderCinematicControl = false;   
   mFlagNoWorldUpdate = false;
}

//==============================================================================
// BObject::initFromProtoObject
//==============================================================================
bool BObject::initFromProtoObject(const BProtoObject *  pProto, BObjectCreateParms& parms)
{
   if (gEnableSubUpdating)
   {
      getWorldMatrix(mOldWorldMatrix);
      mNewWorldMatrix = mOldWorldMatrix;
   }

#ifdef SYNC_Unit
   if(getClassType() != BEntity::cClassTypeObject)
   {
      syncUnitData("BObject::initFromProtoObject mID", mID.asLong())
      syncUnitData("BObject::initFromProtoObject protoID", pProto->getID())
   }
#endif

#ifndef BUILD_FINAL
   mEntityName=const_cast<BSimString*>(&(pProto->getName()));
#endif

   //-- Set the obstruction radius
   mObstructionRadiusX = pProto->getObstructionRadiusX();
   mObstructionRadiusY = pProto->getObstructionRadiusY();
   mObstructionRadiusZ = pProto->getObstructionRadiusZ();

   // -- Set the AO tint value
   mAOTintValue = static_cast<uchar>(Math::Clamp<uint>(static_cast<uint>(.5f + parms.mAOTintValue * 255.0f), 0, 255)); 

   //visual variation index
   mVisualVariationIndex = parms.mVisualVariationIndex;

   if (pProto->getFlagForceToGaiaPlayer())
      setPlayerID(cGaiaPlayer);

   // Set proto ID after player so it can cache the proto object pointer
   setProtoID(pProto->getID());

   setFlagCollidable(pProto->getFlagCollidable());
   setFlagTiesToGround(!parms.mNoTieToGround && !pProto->getFlagNoTieToGround());
   setFlagVisibleForOwnerOnly(pProto->getFlagVisibleForOwnerOnly());
   setFlagVisibleForTeamOnly(pProto->getFlagVisibleForTeamOnly());
   setFlagVisibleToAll(pProto->getFlagVisibleToAll() || ((getClassType() == BEntity::cClassTypeObject) && pProto->getFlagGrayMapDopples()));
   setFlagDopples(false);
   setFlagGrayMapDopples(false);
   setFlagFadeOnDeath(pProto->getFlagFadeOnDeath());
   setFlagHasTrackMask(pProto->getFlagHasTrackMask());

   /*
   if (gConfig.isDefined(cConfigDemo))
      setFlagObscurable(false);
   else
   */
      setFlagObscurable(pProto->getFlagObscurable());

   setFlagNoRender(pProto->getFlagNoRender());
   setFlagDontAutoAttackMe(pProto->getFlagDontAutoAttackMe());
   setFlagAlwaysAttackReviveUnits(pProto->getFlagAlwaysAttackReviveUnits());
   setFlagNoRenderForOwner(pProto->getFlagNoRenderForOwner());
   mFlagAppearsBelowDecals = parms.mAppearsBelowDecals | pProto->getFlagAppearsBelowDecals();

   // This needs to be set before obstructions are evaluated
   setFlagNonMobile(pProto->getFlagNonMobile());   

   // Create the obstruction if it is either collidable or an external shield, and either a unit or destructible, but is NOT a physics replacement
   if((getFlagCollidable() || pProto->getFlagForceCreateObstruction() || pProto->getFlagIsExternalShield()) && getClassType() == BEntity::cClassTypeUnit && !parms.mPhysicsReplacement)
   {
      setFlagRotateObstruction(pProto->getFlagRotateObstruction());
      createObstruction(pProto->getFlagPlayerOwnsObstruction());
   }

   //-- sim position
   // SLB: We don't want to do this
   //updateSimPosition();

   //-- sim bounding box
   updateSimBoundingBox();

   //-- load the visual
   if(parms.mSourceVisual == cInvalidObjectID)
      setVisual(pProto->getProtoVisualIndex(), pProto->getVisualDisplayPriority());
   else
   {
      BEntity *pEntity = gWorld->getEntity(parms.mSourceVisual);
      if(pEntity)
      {
         setVisual(pEntity->getObject()->getVisual());
         copyAdditionalTextures(pEntity->getObject());
      }
   }

   //-- physics
   if (!parms.mNoPhysics)
   {
      if(parms.mPhysicsReplacement && (pProto->getPhysicsReplacementInfoID() != -1) )
      {
         createPhysicsObject(pProto->getPhysicsReplacementInfoID(), NULL, true, true);
      }
      else if ( !parms.mPhysicsReplacement && (pProto->getPhysicsInfoID() != -1) )
      {
         createPhysicsObject(pProto->getPhysicsInfoID(), NULL, false, false);
      }

      setFlagIsPhysicsReplacement(parms.mPhysicsReplacement);
   }

   //-- Lifespan
   bool hasLifespan = pProto->getFlagHasLifespan();
   setFlagHasLifespan(hasLifespan);
   mLifespanExpiration = hasLifespan ? (gWorld->getGametime() + pProto->getLifespan() + pProto->getDeathFadeDelayTimeDWORD()) : 0;

   //-- flying
   bool bFly = pProto->getFlagFlying();
   setFlagFlying(bFly);
   if (bFly)
   {
      setYDisplacement(pProto->getFlightLevel());
   }

   //-- hardpoints
   long hpcount = pProto->getNumberHardpoints();
   mHardpointState.setNumber(hpcount);

   //This is done here because the unit's anim info isn't loaded until the first time
   //it's init from the protoObject in non-archive builds.
   if(pProto->getTactic() && pProto->getTactic()->getAnimInfoLoaded() == false)
   {
      loadTacticAnimInfo(pProto);
   }

   if(pProto->getFlagSoundBehindFOW() && parms.mStartExistSound)
   {
      startExistSound();
   }

   setMultiframeTextureIndex(parms.mMultiframeTextureIndex);

   // explore groups
   if (parms.mExploreGroup != -1)
      setExplorationGroup(parms.mExploreGroup);
   else if (pProto->getFlagAutoExplorationGroup())
      setExplorationGroup(gWorld->getNewExplorationGroup());

   return true;
}

//==============================================================================
//==============================================================================
void BObject::onRelease()
{
   if (!gWorldReset && getProtoObject())
   {
      if (getFlagLOSMarked())
      {
         markLOSOff();
         setFlagLOS(false);
      }

      stopExistSound();

      // Even though we called removeAllAttachments() in ::kill() do it here too just in case there are more attachments from during death.
      removeAllAttachments();

      // SLB: Remove the object from each team's visibility list
#ifdef SYNC_Visibility
      if(getClassType() != cClassTypeObject)
      {
         syncVisibilityCode("BObject::onRelease - updateVisibleLists");
      }
#endif

      // Dopple
      computeDopple();

      // Remove object from team visibility lists
      int32 numTeams = gWorld->getNumberTeams();
      for (int32 i = 1; i < numTeams; i++)
      {
         makeInvisible(i);
      }

      assertVisibility();
   }

   if(mpObjectAttachments)
   {
      delete mpObjectAttachments;
      mpObjectAttachments=NULL;
   }
   if(mpAdditionalTextures)
   {
      delete mpAdditionalTextures;
      mpAdditionalTextures=NULL;
   }

   if(mpVisual)
   {
      gVisualManager.releaseVisual(mpVisual);
      mpVisual=NULL;
   }

   mHardpointState.clear();

   BEntity::onRelease();
}

//==============================================================================
// BObject::getAnimationType
//==============================================================================
long BObject::getAnimationType(long animationTrack) const
{
   if(mpVisual)
      return mpVisual->getAnimationType(animationTrack);
   else
      return -1;
}

//==============================================================================
// BObject::getAnimationDuration
//==============================================================================
float BObject::getAnimationDuration(long animationTrack)
{
   computeAnimation();

   if (mpVisual)
      return (mpVisual->getAnimationDuration(animationTrack));
   else
      return (-1.0f);
}

//==============================================================================
// BObject::getAnimationDurationDWORD
//==============================================================================
DWORD BObject::getAnimationDurationDWORD(long animationTrack)
{
   computeAnimation();

   if (mpVisual)
      return ((DWORD)(mpVisual->getAnimationDuration(animationTrack)*1000.0f));
   else
      return(0);
}

//==============================================================================
// BObject::setAnimationEnabled
//==============================================================================
void BObject::setAnimationEnabled(bool flag, bool includeVisualItem)
{
   mFlagAnimationDisabled = !flag;

   if (includeVisualItem && mpVisual)
   {
      mpVisual->setAnimationEnabled(flag);

      // For subupdating, if the animation is disabled, then reset the
      // interp clock values so it won't try to interpolate
      if (gWorld->getSubUpdate() && mFlagAnimationDisabled)
         mpVisual->resetInterpolatedClockValues();
   }
   
   mAnimationState.setDirty();
}

//==============================================================================
// BObject::settle
//==============================================================================
void BObject::settle( void ) 
{
   BEntity::settle();
   //setAnimation(cAnimTypeIdle, 0, true);
   clearAnimationState();
}


//=============================================================================
// BObject::markLOSOn
//=============================================================================
void BObject::markLOSOn()
{
   if(getFlagLOSMarked())
   {
#if defined(BOBJECT_DEBUG_VISIBLEMAP)
      BSimString msg;
      msg.format(B("%d %s Trying to explore %i %i when the object already has %i %i explored."), mID.asLong(), getProtoObject()->getName().getPtr(), mSimX, mSimZ, mLOSX, mLOSZ);
      BFAIL(msg.getPtr());
#endif
      return;
   }

   if (isClassType(BEntity::cClassTypeDopple) || (isGarrisoned() && !isInCover()))
      return;

   updateSimPosition();

   long los = getSimLOS();
   mLastSimLOS = los;   // set our last sim los.

   // Special case for the global revealer
   if (getFlagIsRevealer() && (los == -1))
   {
      // Set global revealer to center of current map
      float halfWorldSize = gTerrainSimRep.getNumXDataTiles() * gTerrainSimRep.getDataTileScale() * 0.5f;      
      BVector centerPos(halfWorldSize, 0.0f, halfWorldSize);
      #ifdef SYNC_Unit
         if (isClassType(BEntity::cClassTypeUnit))
            syncUnitData("BObject::markLOSOn", centerPos);
      #endif
      setPosition(centerPos);
      gVisibleMap.exploreEntireMap(getTeamID());
      setFlagLOSMarked(true);
#if defined(BOBJECT_DEBUG_VISIBLEMAP)
      mLOSX = mLOSZ = -1;
      mLOSTeamID = getTeamID();
#endif
      return;
   }

   // If LOS is zero or less, don't do anything.
   if (los <= 0)
      return;

#ifdef SYNC_Visibility
   if(getClassType() != cClassTypeObject)
   {
      syncVisibilityData("BObject::markLOSOn mID", mID.asLong());
      syncVisibilityData("BObject::markLOSOn mSimX", mSimX);
      syncVisibilityData("BObject::markLOSOn mSimZ", mSimZ);
      syncVisibilityData("BObject::markLOSOn los", los);
      syncVisibilityData("BObject::markLOSOn team", getTeamID());
   }
#endif

   gVisibleMap.exploreCircularRegion(mSimX, mSimZ, los, getTeamID());
   setFlagLOSMarked(true);

#if defined(BOBJECT_DEBUG_VISIBLEMAP)
   mLOSX = mSimX;
   mLOSZ = mSimZ;
   mLOSTeamID = getTeamID();
#endif
}

//=============================================================================
// BObject::markLOSOff
//=============================================================================
void BObject::markLOSOff()
{
   if(!getFlagLOSMarked())
   {
#if defined(BOBJECT_DEBUG_VISIBLEMAP)
      BSimString msg;
      msg.format(B("%d %s Trying to unexplore %i %i when the object doesn't have anything explored."), mID.asLong(), getProtoObject()->getName().getPtr(), mSimX, mSimZ);
      BFAIL(msg.getPtr());
#endif
      return;
   }

   long los = getSimLOS();
   mLastSimLOS = los;   // mark our last sim los.

   // Special case for the global revealer
   if (getFlagIsRevealer() && los == -1)
   {
      gVisibleMap.unexploreEntireMap(getTeamID());
      setFlagLOSMarked(false);
#if defined(BOBJECT_DEBUG_VISIBLEMAP)
      mLOSX = mLOSZ = mLOSTeamID = -1;
#endif
      return;
   }

   // If LOS is zero or less, don't do anything.
   if (los <= 0)
      return;

#if defined(BOBJECT_DEBUG_VISIBLEMAP)
   if (mLOSTeamID != getTeamID())
   {
      BSimString msg;
      msg.format(B("%d %s Trying to unexplore for team %i when the object last explored for team %i."), mID.asLong(), getProtoObject()->getName().getPtr(), getTeamID(), mLOSTeamID);
      BFAIL(msg.getPtr());
   }
   if ((mLOSX != mSimX) || (mLOSZ != mSimZ))
   {
      BSimString msg;
      msg.format(B("%d %s Trying to unexplore %i %i when the object has %i %i explored."), mID.asLong(), getProtoObject()->getName().getPtr(), mSimX, mSimZ, mLOSX, mLOSZ);
      BFAIL(msg.getPtr());
   }
#endif

#ifdef SYNC_Visibility
   if(getClassType() != cClassTypeObject)
   {
      syncVisibilityData("BObject::markLOSOff mID", mID.asLong());
      syncVisibilityData("BObject::markLOSOff mSimX", mSimX);
      syncVisibilityData("BObject::markLOSOff mSimZ", mSimZ);
      syncVisibilityData("BObject::markLOSOff los", los);
      syncVisibilityData("BObject::markLOSOff team", getTeamID());
   }
#endif

   gVisibleMap.unexploreCircularRegion(mSimX, mSimZ, los, getTeamID() );
   setFlagLOSMarked(false);
#if defined(BOBJECT_DEBUG_VISIBLEMAP)
   mLOSX = mLOSZ = mLOSTeamID = -1;
#endif
}

//=============================================================================
// BObject::markLOSUpdate
//=============================================================================
void BObject::markLOSUpdate()
{
   long oldSimX = mSimX;
   long oldSimZ = mSimZ;

   updateSimPosition();

   // If LOS is zero or less, don't do anything.
   long los = getSimLOS();

#ifdef SYNC_Visibility
   if(getClassType() != cClassTypeObject)
   {
      syncVisibilityData("BObject::markLOSUpdate oldSimX", oldSimX);
      syncVisibilityData("BObject::markLOSUpdate oldSimZ", oldSimZ);
      syncVisibilityData("BObject::markLOSUpdate mSimX", mSimX);
      syncVisibilityData("BObject::markLOSUpdate mSimZ", mSimZ);
      syncVisibilityData("BObject::markLOSUpdate los", los);
      syncVisibilityData("BObject::markLOSUpdate mLastSimLOS", mLastSimLOS);
   }
#endif

   if (los <= 0)
      return;

   bool simLosChanged = ( ( los != mLastSimLOS ) || getFlagLOSDirty() );
   if (!simLosChanged && (oldSimX == mSimX) && (oldSimZ == mSimZ))
      return;

#ifdef SYNC_Visibility
   if(getClassType() != cClassTypeObject)
   {
      syncVisibilityData("BObject::markLOSUpdate cFlagLOSMarked", getFlagLOSMarked());
   }
#endif

   if (!getFlagLOSMarked())
   {
#if defined(BOBJECT_DEBUG_VISIBLEMAP)
      BSimString msg;
      msg.format(B("%d %s Trying to update from %i %i to %i %i when the object doesn't have anything explored."), mID.asLong(), getProtoObject()->getName().getPtr(), oldSimX, oldSimZ, mSimX, mSimZ);
      BFAIL(msg.getPtr());
#endif
      return;
   }

#if defined(BOBJECT_DEBUG_VISIBLEMAP)
#ifdef SYNC_Visibility
   if(getClassType() != cClassTypeObject)
   {
      syncVisibilityData("BObject::markLOSUpdate mLOSTeamID", mLOSTeamID);
      syncVisibilityData("BObject::markLOSUpdate getTeamID", getTeamID());
   }
#endif
   if (mLOSTeamID != getTeamID())
   {
      BSimString msg;
      msg.format(B("%d %s Trying to update for team %i when the object last explored for team %i."), mID.asLong(), getProtoObject()->getName().getPtr(), getTeamID(), mLOSTeamID);
      BFAIL(msg.getPtr());
   }
   if ((mLOSX != oldSimX) || (mLOSZ != oldSimZ))
   {
      BSimString msg;
      msg.format(B("%d %s Trying to update from %i %i to %i %i when the object has %i %i explored."), mID.asLong(), getProtoObject()->getName().getPtr(), oldSimX, oldSimZ, mSimX, mSimZ, mLOSX, mLOSZ);
      BFAIL(msg.getPtr());
   }
   mLOSX = mSimX;
   mLOSZ = mSimZ;
   mLOSTeamID = getTeamID();
#endif

   if (simLosChanged)
   {
      gVisibleMap.unexploreCircularRegion(oldSimX, oldSimZ, mLastSimLOS, getTeamID());
      setFlagLOSMarked(false);
      gVisibleMap.exploreCircularRegion(mSimX, mSimZ, los, getTeamID());
      setFlagLOSMarked(true);
   }
   else
   {
      gVisibleMap.updateCircularRegion(oldSimX, oldSimZ, mSimX, mSimZ, los, getTeamID());
   }
#if defined(BOBJECT_DEBUG_VISIBLEMAP)
   mLOSX = mSimX;
   mLOSZ = mSimZ;
   mLOSTeamID = getTeamID();
#endif
   mLastSimLOS = los;
   setFlagLOSDirty(false);
}

//=============================================================================
// BObject::setLOSScalar()
//=============================================================================
void BObject::setLOSScalar(float losScalar)
{
#ifdef SYNC_Visibility
   if(getClassType() != cClassTypeObject)
   {
      syncVisibilityData("BObject::setLOSScalar mID", mID.asLong());
      syncVisibilityData("BObject::setLOSScalar losScalar", losScalar);
      syncVisibilityData("BObject::setLOSScalar mLOSScalar", mLOSScalar);
      syncVisibilityData("BObject::setLOSScalar cFlagLOSDirty", getFlagLOSDirty());
      syncVisibilityData("BObject::setLOSScalar cFlagUseLOSScalar", getFlagUseLOSScalar());
   }
#endif
   
   if (losScalar != 1.0f)
   {
      if (losScalar != mLOSScalar)
         setFlagLOSDirty(true);

      mLOSScalar = losScalar;
      setFlagUseLOSScalar(true);
   }
   else
   {
      clearLOSScalar();
   }
}

//=============================================================================
// Adjust the current LOS scalar value
//=============================================================================
void BObject::adjustLOSScalar(float adjust)
{
#ifdef SYNC_Visibility
   if (getClassType() != cClassTypeObject)
   {
      syncVisibilityData("BObject::setLOSScalar mID", mID.asLong());
      syncVisibilityData("BObject::setLOSScalar losScalar", adjust);
      syncVisibilityData("BObject::setLOSScalar mLOSScalar", mLOSScalar);
      syncVisibilityData("BObject::setLOSScalar cFlagLOSDirty", getFlagLOSDirty());
      syncVisibilityData("BObject::setLOSScalar cFlagUseLOSScalar", getFlagUseLOSScalar());
   }
#endif      

   mLOSScalar *= adjust;
   setFlagUseLOSScalar(true);
   setFlagLOSDirty(true);

   if (Math::EqualTol(mLOSScalar, 1.0f, cFloatCompareEpsilon))
   {
      clearLOSScalar();
   }
}

//=============================================================================
// BObject::clearLOSScalar
//=============================================================================
void BObject::clearLOSScalar(void)
{
#ifdef SYNC_Visibility
   if(getClassType() != cClassTypeObject)
   {
      syncVisibilityData("BObject::clearLOSScalar mID", mID.asLong());
      syncVisibilityData("BObject::setLOSScalar mLOSScalar", mLOSScalar);
   }
#endif

   if (mLOSScalar != 1.0f)
      setFlagLOSDirty(true);

   mLOSScalar = 1.0f;
   setFlagUseLOSScalar(false);
}


//=============================================================================
// BObject::getNewVisibility
//=============================================================================
DWORD BObject::getNewVisibility(void) const
{
   DWORD newVisibility;

   if (getFlagDestroy() || (isGarrisoned() && !isInCover()))
   {
      newVisibility = 0;
   }
   else if (getFlagVisibleForOwnerOnly() || getFlagVisibleForTeamOnly())
   {
      newVisibility = (0x10001 << getTeamID());
   }
   else if (getFlagVisibleToAll())
   {
      newVisibility = 0xFFFFFFFF;
   }
   else if (getFlagBlockLOS())
   {
      newVisibility = gVisibleMap.getCircularEdgeVisibility(mSimX, mSimZ, getSimLOS());
   }
   else
   {
      newVisibility = (0x10001 << getTeamID());

      // Sample visibility from every tile within the unit's bounding box
      BVector minCorner;
      BVector maxCorner;
      const BBoundingBox* bbox = getSimBoundingBox();
      bbox->computeMinCornerAABB(minCorner);
      bbox->computeMaxCornerAABB(maxCorner);
      minCorner = __vctsxs(XMVectorMax(XMVectorMultiply(minCorner, XMVectorReplicate(gTerrainSimRep.getReciprocalDataTileScale())), XMVectorZero()), 0);
      maxCorner = __vctsxs(XMVectorMin(XMVectorMultiply(maxCorner, XMVectorReplicate(gTerrainSimRep.getReciprocalDataTileScale())), XMVectorReplicate((float) (gVisibleMap.getMaxXTiles() - 1))), 0);
      int x1 = minCorner.u[0];
      int z1 = minCorner.u[2];
      int x2 = maxCorner.u[0];
      int z2 = maxCorner.u[2];
      for (int x = x1; x <= x2; x++)
      {
         for (int z = z1; z <= z2; z++)
         {
            newVisibility |= gVisibleMap.getVisibility(x, z);
         }
      }

      // Sensor locked units can be seen by all enemies at all times
      if (getFlagSensorLockTagged())
      {
         newVisibility |= getEnemyVisibility();
      }
      else  //just add the enemies that have spies
      {
         newVisibility |= getEnemySpyVisibility();
      }
   }

   // SLB: We don't allow this object to become invisible once it's visible
   if (getFlagRemainVisible())
   {
      newVisibility |= mPlayerVisibility.getValue();
   }

   return (newVisibility);
}

//=============================================================================
// BObject::assertVisibility
//=============================================================================
void BObject::assertVisibility() const
{
#ifndef BUILD_FINAL
   long numTeams = gWorld->getNumberTeams();
   for (long i = 1; i < numTeams; i++)
   {
      BTeam* pTeam = gWorld->getTeam(i);
      BASSERT((isVisible(i) || mDoppleBits.isSet(i)) == pTeam->isObjectVisible(getID()));
   }
#endif
}

//=============================================================================
// BObject::updateVisibility
//=============================================================================
void BObject::updateVisibility(DWORD newVisibility)
{
   assertVisibility();

   DWORD oldVisibility = mPlayerVisibility.getValue();
#ifdef SYNC_Visibility
   if(getClassType() != cClassTypeObject)
   {
      syncVisibilityData("BObject::updateVisibility newVisibility", newVisibility);
      syncVisibilityData("BObject::updateVisibility oldVisibility", oldVisibility);
   }
#endif
   DWORD visibilityChange = oldVisibility ^ newVisibility;
   if (visibilityChange || mFlagForceVisibilityUpdateNextFrame)
   {
      //long numPlayers = gWorld->getNumberPlayers();
      //for (long i = 1; i < numPlayers; i++)
      long numTeams = gWorld->getNumberTeams();
      for (long i = 1; i < numTeams; i++)
      {
         DWORD visible = gVisibleMap.getTeamFogOffMask(i);
         if ((visibilityChange & visible) || mFlagForceVisibilityUpdateNextFrame)
         {
            DWORD playerVisibility = visible & newVisibility;
            bool  wasVisible = (visible & oldVisibility) ? true : false;
            bool  isVisible = (playerVisibility == visible) ? true : false;

#ifdef SYNC_Visibility
            if(getClassType() != cClassTypeObject)
            {
               syncVisibilityData("BObject::updateVisibility team", i);
               syncVisibilityData("BObject::updateVisibility playerVisibility", playerVisibility);
               syncVisibilityData("BObject::updateVisibility wasVisible", wasVisible);
               syncVisibilityData("BObject::updateVisibility isVisible", isVisible);
            }
#endif

            if (wasVisible != isVisible)
            {
               //BPlayer *player = gWorld->getPlayer(i);               

               if (wasVisible)
               {
                  // ajl and sle 3/26/08 - allow dopple while dead so that we don't have alpha terrain problems (black squares)
                  if (!getFlagDestroy() && (getFlagGrayMapDopples() || (getFlagDopples())))// && isAlive())))
                     makeSoftDopple(i); // This should become a soft dopple.
                  else
                     makeInvisible(i); // Doesn't dopple. Make it invisible.

                  unselect(i);

               }
               else
               {
                  makeVisible(i);
               }
            }
            else if (mFlagForceVisibilityUpdateNextFrame)
            {
               if (isVisible)
               {
                  makeVisible(i);
               }
               else if (!getFlagDestroy() && getFlagGrayMapDopples())
               {
                  makeSoftDopple(i);
               }
               else
               {
                  makeInvisible(i);
               }
            }
         }
      }

      if (mFlagForceVisibilityUpdateNextFrame)
      {
         mFlagForceVisibilityUpdateNextFrame = false;
      }
   }

   assertVisibility();
}

//=============================================================================
// Get the object's enemy visibility bits
//=============================================================================
DWORD BObject::getEnemyVisibility() const
{
   DWORD enemyVisibility = 0;
   uint numTeams = gWorld->getNumberTeams();
   int teamID = getTeamID();
   for (uint i = 0; i < numTeams; i++)
   {
      if (gWorld->getTeamRelationType(teamID, i) == cRelationTypeEnemy)
      {
         enemyVisibility |= (0x10001 << i);
      }
   }

   return (enemyVisibility);
}

//=============================================================================
// Get the object's enemy visibility bits for enemies with spies
//=============================================================================
DWORD BObject::getEnemySpyVisibility() const
{
   DWORD enemyVisibility = 0;
   uint numTeams = gWorld->getNumberTeams();
   int teamID = getTeamID();
   for (uint i = 0; i < numTeams; i++)
   {
      if ((gWorld->getTeamRelationType(teamID, i) == cRelationTypeEnemy) && BTeam::canTeamASeeTeamB(i, teamID) )
      {
         enemyVisibility |= (0x10001 << i);
      }
   }

   return (enemyVisibility);
}

//=============================================================================
// BObject::updateVisibleLists
//=============================================================================
void BObject::updateVisibleLists(void)
{
   //SCOPEDSAMPLE(BObjectUpdateVisibleLists);
   DWORD newVisibility = getNewVisibility();
   updateVisibility(newVisibility);

   //-- send our position to the minimap
   BTeamID teamID = gUserManager.getUser(BUserManager::cPrimaryUser)->getTeamID();
   BTeamID secondaryTeamID = cInvalidTeamID;    
//-- FIXING PREFIX BUG ID 6090
   const BUser* pSecondaryUser = gUserManager.getSecondaryUser();
//--
   if (pSecondaryUser)
      secondaryTeamID = pSecondaryUser->getTeamID();

   // mrh 8/2/07 - Be sure to check for the proto object until we make sure all objects have one.
   const BProtoObject* pProtoObject = getProtoObject();
   if (pProtoObject)
   {
      if (gWorld->getFlagAllVisible()  || 
          pProtoObject->getFlagAlwaysVisibleOnMinimap() || 
          ((!getFlagVisibleForOwnerOnly() || (getTeamID() == teamID)) && (isVisible(teamID) || (isDoppled(teamID) && !hasDoppleObject(teamID)))))
      {
         //if (!gConfig.isDefined(cConfigFlashGameUI))
         //   gMiniMap.addUnit(*this);
         //else         
         gUIManager->addMinimapIcon(this);
      }
      else if (gGame.isSplitScreen())
      {
         if (!getFlagVisibleForOwnerOnly() || (getTeamID() == secondaryTeamID))
            if (isVisible(secondaryTeamID) || (isDoppled(secondaryTeamID) && !hasDoppleObject(secondaryTeamID)))
            {
               //if (!gConfig.isDefined(cConfigFlashGameUI))
               //   gMiniMap.addUnit(*this);
               //else         
               gUIManager->addMinimapIcon(this);
            }
      }
   }
}

//==============================================================================
// BObject::isVisible
//==============================================================================
bool BObject::isVisible(BTeamID teamID) const
{
   DWORD visible = gVisibleMap.getTeamFogOffMask(teamID);
   return (mPlayerVisibility.getValue() & visible) ? true : false;
}

//==============================================================================
// BObject::isExplored
//==============================================================================
bool BObject::isExplored(BTeamID teamID) const
{
   DWORD visible = gVisibleMap.getTeamBlackOffMask(teamID);
   return (mPlayerVisibility.getValue() & visible) ? true : false;
}

//==============================================================================
// BObject::isVisibleOnScreen
//==============================================================================
bool BObject::isVisibleOnScreen() const
{
   const BBoundingBox* pBoundingBox = getVisualBoundingBox();
   return gWorld->isSphereVisible(pBoundingBox->getCenter(), pBoundingBox->getSphereRadius());
}

//==============================================================================
//==============================================================================
float BObject::getDesiredVelocity() const
{
   return getProtoObject()->getDesiredVelocity();
}

//==============================================================================
//==============================================================================
float BObject::getMaxVelocity() const
{
   return getProtoObject()->getMaxVelocity();
}

//==============================================================================
//==============================================================================
float BObject::getReverseSpeed() const
{
   return getProtoObject()->getReverseSpeed();
}

//==============================================================================
// BObject::getAcceleration
//==============================================================================
float BObject::getAcceleration( void ) const
{
   return getProtoObject()->getAcceleration();
}

//==============================================================================
// BObject::kill
//==============================================================================
void BObject::kill(bool bKillImmediately)
{
   stopAllHardpointSounds();

   BSelectionManager* pSelectionManager=gUserManager.getUser(BUserManager::cPrimaryUser)->getSelectionManager();
   if(pSelectionManager->isUnitSelected(mID))
      pSelectionManager->unselectUnit(mID);

   if (gGame.isSplitScreen())
   {
      pSelectionManager=gUserManager.getUser(BUserManager::cSecondaryUser)->getSelectionManager();
      if(pSelectionManager->isUnitSelected(mID))
         pSelectionManager->unselectUnit(mID);
   }

   if (getExplorationGroup() != -1)
      gWorld->removeObjectFromExplorationGroup(*this);

   BEntity::kill(bKillImmediately);

   // Kill all of the attachments
   removeAllAttachments();

   if (getClassType() == BEntity::cClassTypeObject)
   {
      // If we're actually a cClassTypeObject, make sure we go away because BEntity::kill() doesn't do anything for us.
      destroy();
   }
}

//==============================================================================
// BObject::destroy
//==============================================================================
void BObject::destroy()
{
#ifdef SYNC_Unit
   if (getClassType() != cClassTypeObject)
   {
      syncUnitData("BObject::destroy mID", mID.asLong())
   }
#endif

   // If I am a blocker object
   if (!getUnit() && getFlagBlockLOS())
   {
      // Unblock LOS
      BTeamID teamID = getPlayer()->getTeamID();
      for (int i = 1; i < gWorld->getNumberTeams(); i++)
      {
         if (i != teamID)
         {
            gVisibleMap.unblockCircularRegion(mSimX, mSimZ, getSimLOS(), i);
         }
      }
   }

   if (getFlagLOSMarked())
   {
      markLOSOff();
      setFlagLOS(false);
   }

   // Must disable the NoUpdate flag or else the object doesn't go away (SAT)
   setFlagNoUpdate(false);


   BEntity::destroy();
}

//==============================================================================
// BObject::setLifespan
//==============================================================================
void BObject::setLifespan(DWORD lifespan, bool useDeathFadeDelay /*= false*/)
{
   if (!getFlagHasLifespan())
      enableLifespan(true);

   DWORD deathFadeDelayTime = 0;
   if (useDeathFadeDelay)
   {
      const BProtoObject* pProtoObject = getProtoObject();
      if (pProtoObject)
      {
         deathFadeDelayTime = pProtoObject->getDeathFadeDelayTimeDWORD();
      }
   }
   mLifespanExpiration = gWorld->getGametime() + lifespan + deathFadeDelayTime;
}

//==============================================================================
// BObject::enableLifespan
//==============================================================================
void BObject::enableLifespan( bool flag )
{
   setFlagHasLifespan(flag);
}

//==============================================================================
// BObject::enableAlphaFade
//==============================================================================
void BObject::enableAlphaFade(bool flag, float duration)
{
   mCurrentAlphaTime = 0.0f;

   if (!flag)
   {
      setFlagIsFading(false);
      mAlphaFadeDuration = 0.0f;
      return;
   }

   setFlagIsFading(true);
   mAlphaFadeDuration = duration;
}

//==============================================================================
// BObject::createDopple
//==============================================================================
bool BObject::createDopple(BTeamID teamID)
{
   #ifdef SYNC_Unit
      if (getClassType() != cClassTypeObject)
      {
         syncUnitData("BObject::createDopple teamID", teamID);
      }
   #endif
   return gWorld->createDopple(this, teamID, getFlagNotDoppleFriendly()) ? true : false;
}

//==============================================================================
// BObject::generateFloatingTextForResourceGather
//==============================================================================
void BObject::generateFloatingTextForResourceGather(long resourceID)
{   
   //-- Get the floating text values.
   float x, y;
   long duration, fadeout;      
   gConfig.get(cConfigRenderFloatyTextXVelocity, &x); 
   gConfig.get(cConfigRenderFloatyTextYVelocity, &y);
   gConfig.get(cConfigRenderFloatyTextDuration, &duration);
   gConfig.get(cConfigRenderFloatyTextFadeOutTime, &fadeout);

   duration*=2;
   x/=2.0f;
   y/=2.0f;

   BVector position;
   float w, h;
   gHPBar.getHPBarPosition(*this, position, w, h);
   if(position == cInvalidVector)
   {
      position = getPosition();
      position.y += 5.0f;
   }

   BUString msg(" ");
   BVector velocity;      
   velocity.set(x, y, 0.0f);

   long textVisual = gUIGame.getResourceTextVisual(resourceID);

   BDynamicSimLongArray resultIDs;
   gTextVisualManager.create(textVisual, getPlayerID(), msg, position, &resultIDs);
}

//=============================================================================
// BObject::addAttachment
//=============================================================================
BEntityID BObject::addAttachment(long protoObjectID, long toBoneHandle, long fromBoneHandle, bool isUnitAttachment)
{
   if(!mpObjectAttachments)
   {
      mpObjectAttachments=new BObjectAttachmentArray();
      if(!mpObjectAttachments)
         return cInvalidObjectID;
   }
   BEntityID attachmentID=gWorld->createEntity(protoObjectID, false, getPlayerID(), mPosition, mForward, mRight, true);
   if(attachmentID!=cInvalidObjectID)
   {
      BObjectAttachment objectAttachment(attachmentID, toBoneHandle, fromBoneHandle, isUnitAttachment);
      mpObjectAttachments->add(objectAttachment);

      BObject* pObject = gWorld->getObject(attachmentID);
      if (pObject)
         pObject->setFlagNoWorldUpdate(true);

      #ifndef BUILD_FINAL
         if (mpObjectAttachments->size() > 20)
         {
            // ajl 11/12/08 - Attempt to figure out where we are adding lots of attachments
            BSimString msg;
            msg.format("Too many attachments: Count %u, Attachment %d (%d - %s), Parent %d (%s)", 
               mpObjectAttachments->size(), attachmentID.asLong(), protoObjectID, (pObject && pObject->getName() ? pObject->getName()->getPtr() : ""), 
               mID.asLong(), (mEntityName ? mEntityName->getPtr() : ""));
            BASSERTM(0, msg.getPtr());
         }
      #endif

#ifdef DISABLE_NO_UPDATE_OPTIMIZATION
      if (!isUnitAttachment)
      {
         BObject* pObject = gWorld->getObject(attachmentID);
         if (pObject)
         {
            // Halwes 4/30/2007 - Fix for SLB: temp hack
            pObject->setFlagNoUpdate(false);

            // SLB: attachments inherit their parent's visibility
            pObject->setFlagVisibility(false);
         }
      }
#endif
   }

   return attachmentID;
}

//=============================================================================
// BObject::addAttachment( BObject* pObject )
//=============================================================================
void BObject::addAttachment(BObject* pObject, long toBoneHandle, long fromBoneHandle, bool isUnitAttachment, bool useOffset)
{
   if (!mpObjectAttachments)
   {
      mpObjectAttachments = new BObjectAttachmentArray();
      if (!mpObjectAttachments)
      {
         return;
      }
   }

   if (!pObject)
      return;

   BEntityID attachmentID = pObject->getID();
   if (attachmentID != cInvalidObjectID)
   {
      BObjectAttachment objectAttachment(attachmentID, toBoneHandle, fromBoneHandle, isUnitAttachment);

      BMatrix toPosMatrix;
      BMatrix toWorldMatrix;
      getWorldMatrix(toWorldMatrix);
      if (mpVisual && (toBoneHandle != -1))
         if (!mpVisual->getBone(toBoneHandle, NULL, &toPosMatrix, NULL, &toWorldMatrix))
            objectAttachment.mToBoneHandle = -1;
      if (objectAttachment.mToBoneHandle == -1)
         toPosMatrix = toWorldMatrix;

      BMatrix fromPosMatrix;
      BMatrix fromWorldMatrix;
      pObject->getWorldMatrix(fromWorldMatrix);
      if (pObject->getVisual() && (fromBoneHandle != -1))
         if (!pObject->getVisual()->getBone(fromBoneHandle, NULL, &fromPosMatrix, NULL, &fromWorldMatrix))
            objectAttachment.mFromBoneHandle = -1;
      if (objectAttachment.mFromBoneHandle == -1)
         fromPosMatrix = fromWorldMatrix;

      if (useOffset)
      {
         toPosMatrix.invert();
         objectAttachment.mOffset.mult(fromPosMatrix, toPosMatrix);
         objectAttachment.mUseOffset = true;
      }
      
      mpObjectAttachments->add(objectAttachment);

      pObject->setFlagNoWorldUpdate(true);

      #ifndef BUILD_FINAL
         if (mpObjectAttachments->size() > 20)
         {
            // ajl 11/12/08 - Attempt to figure out where we are adding lots of attachments
            BSimString msg;
            msg.format("Too many attachments: Count %u, Attachment %d (%d - %s), Parent %d (%s)", 
               mpObjectAttachments->size(), pObject->getID().asLong(), pObject->getProtoID(), (pObject->getName() ? pObject->getName()->getPtr() : ""), 
               mID.asLong(), (mEntityName ? mEntityName->getPtr() : ""));
            BASSERTM(0, msg.getPtr());
         }
      #endif

#ifdef DISABLE_NO_UPDATE_OPTIMIZATION
      if (!isUnitAttachment)
      {
         // Halwes 4/30/2007 - Fix for SLB: temp hack
         pObject->setFlagNoUpdate(false);

         // SLB: attachments inherit their parent's visibility
         pObject->setFlagVisibility(false);
      }
#endif
   }
}

//=============================================================================
// BObject::removeAttachment
//=============================================================================
void BObject::removeAttachment(BEntityID attachmentID)
{
   if (!mpObjectAttachments)
      return;

   uint numAttachments = mpObjectAttachments->getSize();
   for (uint i = 0; i < numAttachments; i++)
   {
      BObjectAttachment objectAttachment = mpObjectAttachments->get(i);
      if (objectAttachment.mAttachmentObjectID == attachmentID)
      {
         mpObjectAttachments->removeIndex(i, false);

         BEntity* pAttachment = gWorld->getEntity(attachmentID);
         if (pAttachment)
         {
#ifdef SYNC_Unit
            if (pAttachment->getUnit() && (getClassType() != cClassTypeObject))
            {
               syncUnitData("BObject::removeAttachment mID", mID.asLong())
            }
#endif

            BObject* pObject = pAttachment->getObject();
            if (pObject)
               pObject->setFlagNoWorldUpdate(false);

            if (!objectAttachment.mIsUnitAttachment)
            {
               pAttachment->kill(false);
            }

            // MS 9/25/2008: something else seems to have broken this so that this
            // now results in a bad visual pop on detach. Commenting this out until
            // we can find what broke and make everything work together.
/*
            // [5/15/2008 xemu] update the position on the attached object to match
            if (gConfig.isDefined("WTFBBQ") && objectAttachment.mToBoneHandle != -1)
            {
               BVector offsetPos;
               if ((getVisual() != NULL) && getVisual()->getBone(objectAttachment.mToBoneHandle, &offsetPos))
               {
                  BVector newPos = pAttachment->getPosition();
                  newPos = newPos + offsetPos;
                  #ifdef SYNC_Unit
                     if (pAttachment->isClassType(BEntity::cClassTypeUnit))
                        syncUnitData("BObject::removeAttachment", newPos);
                  #endif
                  pAttachment->setPosition(newPos, false);
               }
            }
*/
         }

         return;
      }
   }
}

//=============================================================================
// BObject::removeAllAttachments
//=============================================================================
void BObject::removeAllAttachments()
{
   if(!mpObjectAttachments)
      return;
   for(long i=0; i<mpObjectAttachments->getNumber(); i++)
   {
      BObjectAttachment objectAttachment = mpObjectAttachments->get(i);
      BEntity* pAttachment=gWorld->getEntity(objectAttachment.mAttachmentObjectID);
      if(pAttachment)
      {
#ifdef SYNC_Unit
         if (pAttachment->getUnit() && (getClassType() != cClassTypeObject))
         {
            syncUnitData("BObject::removeAttachment mID", mID.asLong())
         }
#endif

         BObject* pObject = pAttachment->getObject();
         if (pObject)
            pObject->setFlagNoWorldUpdate(false);

         if (!objectAttachment.mIsUnitAttachment)
         {
            pAttachment->kill(false);
         }
      }
   }

   mpObjectAttachments->setNumber(0);
}

//==============================================================================
// BObject::findAttachment
//==============================================================================
bool BObject::findAttachment(BEntityID attachmentID) const
{
   if (!mpObjectAttachments)
      return false;

   long numAttachments = mpObjectAttachments->getNumber();
   for (long i = 0; i < numAttachments; i++)
   {
      BObjectAttachment objectAttachment = mpObjectAttachments->get(i);
      if (objectAttachment.mAttachmentObjectID == attachmentID)
         return true;
   }

   return false;
}

//==============================================================================
//==============================================================================
BObject* BObject::getAttachedToObject() const
{
   if (!getFlagAttached())
      return NULL;

//-- FIXING PREFIX BUG ID 6091
   const BEntityRef* pRef = getFirstEntityRefByType(BEntityRef::cTypeAttachedToObject);
//--
   if (pRef)
   {
      return gWorld->getObject(pRef->mID);
   }

   return NULL;
}

//==============================================================================
//==============================================================================
BObjectAttachment* BObject::getAttachedToObjectAttachment() const
{
   BObject* pAttachedToObject = getAttachedToObject();
   if (!pAttachedToObject)
      return NULL;

   for(long i=0; i<pAttachedToObject->mpObjectAttachments->getNumber(); i++)
   {
      // Find matching BObjectAttachment
      BObjectAttachment& objectAttachment = pAttachedToObject->mpObjectAttachments->get(i);
      if (objectAttachment.mAttachmentObjectID == getID())
         return &objectAttachment;
   }

   return NULL;
}

//==============================================================================
//==============================================================================
bool BObject::isObjectAttached(BObject* pObj) const
{
   if (!pObj)
      return false;

//-- FIXING PREFIX BUG ID 6092
   const BObject* pAttachedToObj = pObj->getAttachedToObject();
//--
   if (!pAttachedToObj)
      return false;

   if (pAttachedToObj->getID() == getID())
      return true;

   return false;
}

//==============================================================================
//==============================================================================
bool BObject::isAttachedToObject(BObject* pObj) const
{
   if (!pObj)
      return false;

//-- FIXING PREFIX BUG ID 6093
   const BObject* pAttachedToObj = getAttachedToObject();
//--
   if (!pAttachedToObj)
      return false;

   if (pAttachedToObj->getID() == pObj->getID())
      return true;

   return false;
}

//==============================================================================
// BObject::removeAttachmentsOfType
//==============================================================================
void BObject::removeAttachmentsOfType(long protoObjectID)
{
   if(!mpObjectAttachments)
      return;
   for(long i=0; i<mpObjectAttachments->getNumber(); i++)
   {
      BObjectAttachment objectAttachment = mpObjectAttachments->get(i);
      BEntityID attachmentID = objectAttachment.mAttachmentObjectID;
      BEntity *pAttachment = gWorld->getEntity(attachmentID);
      if(pAttachment && pAttachment->getProtoID() == protoObjectID)
      {
         mpObjectAttachments->removeIndex(i, false);
         i--;

#ifdef SYNC_Unit
         if (pAttachment->getUnit() && (getClassType() != cClassTypeObject))
         {
            syncUnitData("BObject::removeAttachmentsOfType mID", mID.asLong())
         }
#endif

         BObject* pObject = pAttachment->getObject();
         if (pObject)
            pObject->setFlagNoWorldUpdate(false);

         if (!objectAttachment.mIsUnitAttachment)
         {
            pAttachment->kill(false);
         }
      }
   }
}

//=============================================================================
// BObject::getTactic
//=============================================================================
BTactic* BObject::getTactic( void ) const
{
   const BProtoObject *pObject = getProtoObject();
   if(pObject)
      return pObject->getTactic();

   return NULL;
}


//=============================================================================
// BObject::getNumberHardpoints
//=============================================================================
long BObject::getNumberHardpoints( void ) const
{
   return mHardpointState.getNumber();
}


//=============================================================================
// BObject::getHardpoint
//=============================================================================
const BHardpoint *BObject::getHardpoint( long index ) const
{
   const BProtoObject *pProto = getProtoObject();
   if(pProto)
      return pProto->getHardpoint(index);

   return NULL;
}

//=============================================================================
// BObject::validateHardpoint
//=============================================================================
bool BObject::validateHardpoint( long index ) const
{
   const BHardpoint *pHP = getHardpoint(index);
   if (!pHP)
      return false;

   BVisual *pVis = getVisual();
   if (!pVis)
      return false;

   if (pHP->getFlagSingleBoneIK())
   {
      // Not using a real hardpoint, just return true
      return true;
   }
   else
   {
      // Check yaw handle
      if (pHP->getYawAttachmentHandle() != -1)
      {
//-- FIXING PREFIX BUG ID 6094
         const BVisualItem* pAttachment = pVis->getAttachment(pHP->getYawAttachmentHandle());
//--
         if (!pAttachment)
            return false;
      }

      // Check pitch handle
      if (pHP->getPitchAttachmentHandle() != -1)
      {
//-- FIXING PREFIX BUG ID 6095
         const BVisualItem* pAttachment = pVis->getAttachment(pHP->getPitchAttachmentHandle());
//--
         if (!pAttachment)
            return false;
      }
   }

   return true;
}

//=============================================================================
// BObject::getHardpointYawLocation
//=============================================================================
bool BObject::getHardpointYawLocation( long index, BVector &vec, BMatrix& matrix, BMatrix* transformedMatrix)const
{
   const BHardpoint *pHP = getHardpoint(index);
   if (!pHP)
      return (false);

   BVisual *pVis = getVisual();
   if (!pVis)
      return false;

   if (pHP->getFlagSingleBoneIK())
   {
      bool retVal;
      if (pHP->getFlagRelativeToUnit())
      {
         retVal = pVis->getBone(pVis->getIKNodeBoneHandle(0), &vec, NULL, NULL, NULL, false); // SLB: HACK
         matrix.makeTranslate(vec);
      }
      else
         retVal = pVis->getBone(pVis->getIKNodeBoneHandle(0), &vec, &matrix, NULL, NULL, false); // SLB: HACK

      if (transformedMatrix)
         getHardpointYawTransform(index, *transformedMatrix);

      return retVal;
   }
   else
   {
      const BVisualItem* pAttachment = pVis->getAttachment(pHP->getYawAttachmentHandle(), NULL, &matrix);
      if (!pAttachment)
         return false;

      long boneHandle = pVis->getAttachmentToBoneHandle(pHP->getYawAttachmentHandle());

      if (boneHandle == -1)
         return false;
      BMatrix boneMatrix;
      if (!pVis->getBone(boneHandle, NULL, &boneMatrix, NULL, NULL, true, pAttachment))
         return false;

      matrix.mult(boneMatrix, matrix);
      matrix.getTranslation(vec);

      if (transformedMatrix)
      {
         if (!getHardpointYawTransform(index, *transformedMatrix))
            return false;
      }

      return true;
   }
}

//=============================================================================
// BObject::getHardpointPitchLocation
//=============================================================================
bool BObject::getHardpointPitchLocation(long index, BVector &vec, BMatrix& matrix, BMatrix* transformedMatrix)const
{
   const BHardpoint *pHP = getHardpoint(index);
   if (!pHP)
      return (false);

   BVisual *pVis = getVisual();
   if (!pVis)
      return false;

   if (pHP->getFlagSingleBoneIK() || pHP->getFlagCombined())
      return false;
   else
   {
      const BVisualItem* pAttachment = pVis->getAttachment(pHP->getPitchAttachmentHandle(), NULL, &matrix);
      if (!pAttachment)
         return false;

      long boneHandle = pVis->getAttachmentToBoneHandle(pHP->getPitchAttachmentHandle());

      if (boneHandle == -1)
         return false;
      BMatrix boneMatrix;
      if (!pVis->getBone(boneHandle, NULL, &boneMatrix, NULL, NULL, true, pAttachment))
         return false;

      matrix.mult(boneMatrix, matrix);
      matrix.getTranslation(vec);

      if (transformedMatrix)
      {
         if (!getHardpointPitchTransform(index, *transformedMatrix))
            return false;
      }

      return true;
   }
}

//=============================================================================
// BObject::getHardpointYawTransform
//=============================================================================
bool BObject::getHardpointYawTransform(long index, BMatrix &mat) const
{
   const BHardpoint *pHP = getHardpoint(index);
   if (!pHP)
      return false;

   BVisual *pVis = getVisual();
   if (!pVis)
      return false;

   if (pHP->getFlagSingleBoneIK())
   {
      BVector position;
      BQuaternion orientation;
      if (pVis->getIKNodeSingleBone(0, position, orientation)) // SLB: HACK
      {
         orientation.toMatrix(mat);
         mat.setTranslation(position);
      }
      else
         mat.makeIdentity();

      return true;
   }

   return pVis->getAttachmentTransform(pHP->getYawAttachmentHandle(), mat);
}



//=============================================================================
// BObject::getHardpointPitchTransform
//=============================================================================
bool BObject::getHardpointPitchTransform(long index, BMatrix &mat) const
{
   const BHardpoint *pHP = getHardpoint(index);
   if (!pHP)
      return false;

   BVisual *pVis = getVisual();
   if (!pVis)
      return false;

   if (pHP->getFlagSingleBoneIK() || pHP->getFlagCombined())
      return false;

   return pVis->getAttachmentTransform(pHP->getPitchAttachmentHandle(), mat);
}

//=============================================================================
// BObject::canYawHardpointToWorldPos
//=============================================================================
bool BObject::canYawHardpointToWorldPos(long index, BVector wsTargetPos, const BMatrix *pCachedYawHardpointMatrix) const
{
   //-- get visual
   if (!getVisual())
      return (true);

   //-- get hardpoint
   const BHardpoint *pHP = getHardpoint(index);
   if (!pHP)
      return (true);

   //-- redirect to orientHardpointToWorldPos if this is a combined hardpoint
   if (pHP->getFlagCombined())
      return canOrientHardpointToWorldPos(index, wsTargetPos);

   //-- get the base location and direction of the hardpoint yaw bone in model and hardpoint space
   BVector msBonePos;
   BMatrix msBoneMatrix;
   if (pCachedYawHardpointMatrix)
   {
      msBoneMatrix = *pCachedYawHardpointMatrix;
      pCachedYawHardpointMatrix->getTranslation(msBonePos);
   }
   else if (!getHardpointYawLocation(index, msBonePos, msBoneMatrix))
      return (true);

   //-- get world to model transform
   BMatrix worldToModelMatrix;
   getWorldMatrix(worldToModelMatrix);
   worldToModelMatrix.invert();

   //-- get model to hardpoint transform
   BMatrix modelToHardpointMatrix(msBoneMatrix);
   modelToHardpointMatrix.invert();

   //-- get world to hardpoint transform
   BMatrix worldToHardpointMatrix;
   worldToHardpointMatrix.mult(worldToModelMatrix, modelToHardpointMatrix);

   //-- get the target position in hardpoint space
   BVector tsTargetPos;
   worldToHardpointMatrix.transformVectorAsPoint(wsTargetPos, tsTargetPos);

   return canYawHardpointToWorldPos(pHP, tsTargetPos);
}

//=============================================================================
// BObject::capTargetOrient
//=============================================================================
bool BObject::capTargetOrient(const BHardpoint* pHP, BVector &tsTargetDir) const
{
   BASSERT(pHP);
   BASSERT(pHP->getFlagCombined());

   bool capped = false;

   if (pHP->getMaxCombinedAngle() < cPi)
   {
      //-- Calculate angle
      float dot = cZAxisVector.dot(tsTargetDir);
      float angle = XMScalarACos(dot);

      //-- Is it more than the max?
      if (angle > pHP->getMaxCombinedAngle())
      {
         capped = true;

         // Calculate rotation axis
         BVector cross;
         cross.assignCrossProduct(cZAxisVector, tsTargetDir);

         // Calculate new target direction capped at the angle limit
         BMatrix matrix;
         matrix.makeRotateArbitrary(pHP->getMaxCombinedAngle(), cross);
         matrix.transformVector(cZAxisVector, tsTargetDir);
      }
   }

   return capped;
}

//=============================================================================
// BObject::capTargetYaw
//=============================================================================
bool BObject::capTargetYaw(const BHardpoint* pHP, BVector &tsTargetDir) const
{
   BASSERT(pHP);

   bool capped = false;

   if ((pHP->getYawLeftMaxAngle() > -cPi) || (pHP->getYawRightMaxAngle() < cPi))
   {
      //-- Is the target on the left or on the right
      bool onLeft = (cXAxisVector.dot(tsTargetDir) < 0.0f);

      //-- Calculate max direction
      BVector maxDirection = cZAxisVector;
      maxDirection.rotateXZ(onLeft ? pHP->getYawLeftMaxAngle() : pHP->getYawRightMaxAngle());

      //-- Calculate max direction normal
      BVector maxNormal;
      maxNormal.assignCrossProduct(onLeft ? cYAxisVector : -cYAxisVector, maxDirection);
      maxNormal.normalize();

      //-- Make sure we're within bounds
      if (maxNormal.dot(tsTargetDir) < 0.0f)
      {
         tsTargetDir = maxDirection; // We're not. Clamp it.
         capped = true;
      }
   }

   return capped;
}

//=============================================================================
// BObject::capTargetPitch
//=============================================================================
bool BObject::capTargetPitch(const BHardpoint* pHP, BVector &tsTargetDir) const
{
   BASSERT(pHP);

   bool capped = false;

   if ((pHP->getPitchMaxAngle() < cPi) || (pHP->getPitchMinAngle() > -cPi))
   {
      //-- Is the target on top or on the bottom
      bool onBottom = (cYAxisVector.dot(tsTargetDir) < 0.0f);

      BVector forward = tsTargetDir;
      forward.y = 0.0f;
      forward.normalize();

      BVector right;
      right.assignCrossProduct(cYAxisVector, forward);
      right.normalize();

      //-- Calculate max direction
      BMatrix rotation;
      BVector maxDirection;
      rotation.makeRotateArbitrary(onBottom ? -pHP->getPitchMinAngle() : -pHP->getPitchMaxAngle(), right);
      rotation.transformVector(forward, maxDirection);

      //-- Calculate max direction normal
      BVector maxNormal;
      maxNormal.assignCrossProduct(onBottom ? -right : right, maxDirection);
      maxNormal.normalize();

      //-- Make sure we're within bounds
      if (maxNormal.dot(tsTargetDir) < 0.0f)
      {
         tsTargetDir = maxDirection; // We're not. Clamp it.
         capped = pHP->getFlagHardPitchLimits(); // Clamp but, don't let us know because we don't care.
      }
   }

   return capped;
}

//=============================================================================
// BObject::yawHardpointToWorldPos
//=============================================================================
bool BObject::yawHardpointToWorldPos(long index, BVector wsTargetPos, float time, BVector *pwsActualTargetPos, const BMatrix *pCachedYawHardpointMatrix)
{
   //-- get visual
   BVisual *pVis = getVisual();
   if (!pVis)
      return true;

   //-- get hardpoint
   const BHardpoint *pHP = getHardpoint(index);
   if (!pHP)
      return (true);

   //-- redirect to orientHardpointToWorldPos if this is a combined hardpoint
   if (pHP->getFlagCombined())
      return orientHardpointToWorldPos(index, wsTargetPos, time);

   //-- get the base location and direction of the hardpoint yaw bone in model and hardpoint space
   BVector msBonePos;
   BMatrix msBoneMatrix;
   BMatrix transformedBoneMatrix;
   if (pCachedYawHardpointMatrix)
   {
      msBoneMatrix = *pCachedYawHardpointMatrix;
      pCachedYawHardpointMatrix->getTranslation(msBonePos);
      if (!getHardpointYawTransform(index, transformedBoneMatrix))
         return true;
   }
   else
   {
      if (!getHardpointYawLocation(index, msBonePos, msBoneMatrix, &transformedBoneMatrix))
         return (true);
   }

   //-- get world to model transform
   BMatrix worldToModelMatrix;
   getWorldMatrix(worldToModelMatrix);
   worldToModelMatrix.invert();

   //-- get model to hardpoint transform
   BMatrix modelToHardpointMatrix(msBoneMatrix);
   modelToHardpointMatrix.invert();

   //-- get world to hardpoint transform
   BMatrix worldToHardpointMatrix;
   worldToHardpointMatrix.mult(worldToModelMatrix, modelToHardpointMatrix);

   //-- get the target position in hardpoint space
   BVector tsTargetPos;
   BVector tsActualTargetPos;
   worldToHardpointMatrix.transformVectorAsPoint(wsTargetPos, tsTargetPos);
   if (pwsActualTargetPos)
      worldToHardpointMatrix.transformVectorAsPoint(*pwsActualTargetPos, tsActualTargetPos);
   else
      tsActualTargetPos = tsTargetPos;

   //-- flatten to model space XZ plane
   tsTargetPos.y = 0.0f;
   tsActualTargetPos.y = 0.0f;

   //-- get the target direction in hardpoint space
   BVector tsTargetDir(tsTargetPos);
   BVector tsActualTargetDir(tsActualTargetPos);
   tsTargetDir.normalize();
   tsActualTargetDir.normalize();

   //-- cap target direction
   bool capped = capTargetYaw(pHP, tsTargetDir);

   //-- get transformed bone direction
   BVector transformedBoneDir;
   transformedBoneMatrix.getForward(transformedBoneDir);
   transformedBoneDir.normalize();

   //-- early out if we're aiming at our actual target
   float dot = transformedBoneDir.dot(tsActualTargetDir);
   if (dot >= HARDPOINT_TOLERANCE_STOP)
   {
      stopYawTurningSound(index);
      return true;
   }

   //-- get angle between vectors
   dot = transformedBoneDir.dot(tsTargetDir);
   float angle = XMScalarACos(dot);

   //-- how fast can we turn?
   float rate = pHP->getYawRotationRate() * time;

   //-- how far do we slerp?
   float k = Math::Min(rate / angle, 1.0f);

   //-- convert transformed bone direction to quaternion
   BVector transformedBoneRight;
   transformedBoneRight.assignCrossProduct(cYAxisVector, transformedBoneDir);
   transformedBoneRight.normalize();
   BQuaternion transformedBoneQuaternion(transformedBoneDir, cYAxisVector, transformedBoneRight);

   //-- convert target direction to quaternion
   BVector tsTargetRight;
   tsTargetRight.assignCrossProduct(cYAxisVector, tsTargetDir);
   tsTargetRight.normalize();
   BQuaternion targetYawQuaternion(tsTargetDir, cYAxisVector, tsTargetRight);

   //-- slerp to new direction
   transformedBoneQuaternion = transformedBoneQuaternion.slerp(targetYawQuaternion, k);
   transformedBoneQuaternion.normalize();

   //-- set yaw transform
   if (pHP->getFlagSingleBoneIK())
   {
      if (!getFlagIKDisabled())
      {
         mpVisual->setIKNodeSingleBone(0, XMVectorZero(), transformedBoneQuaternion); // SLB: HACK
         mpVisual->setIKNodeActive(0, true);
      }
   }
   else
   {
      BMatrix yawMatrix;
      transformedBoneQuaternion.toMatrix(yawMatrix);
      pVis->setAttachmentTransform(pHP->getYawAttachmentHandle(), yawMatrix);
   }

   playYawTurningSound(index);

   return !capped;
}

//=============================================================================
// BObject::canPitchHardpointToWorldPos
//=============================================================================
bool BObject::canPitchHardpointToWorldPos(long index, BVector wsTargetPos) const
{
   //-- get visual
   if (!getVisual())
      return (true);

   //-- get hardpoint
   const BHardpoint *pHP = getHardpoint(index);
   if (!pHP || pHP->getFlagSingleBoneIK() || pHP->getFlagCombined())
      return (true);

   //-- get the base location and direction of the hardpoint yaw bone in model and hardpoint space
   BVector msBonePos;
   BMatrix msBoneMatrix;
   if (!getHardpointPitchLocation(index, msBonePos, msBoneMatrix))
      return (true);

   //-- get world to model transform
   BMatrix worldToModelMatrix;
   getWorldMatrix(worldToModelMatrix);
   worldToModelMatrix.invert();

   //-- get model to hardpoint transform
   BMatrix modelToHardpointMatrix(msBoneMatrix);
   modelToHardpointMatrix.invert();

   //-- get world to hardpoint transform
   BMatrix worldToHardpointMatrix;
   worldToHardpointMatrix.mult(worldToModelMatrix, modelToHardpointMatrix);

   //-- get the target position in hardpoint space
   BVector tsTargetPos;
   worldToHardpointMatrix.transformVectorAsPoint(wsTargetPos, tsTargetPos);

   return canPitchHardpointToWorldPos(pHP, tsTargetPos);
}

//=============================================================================
// BObject::pitchHardpointToWorldPos
//=============================================================================
bool BObject::pitchHardpointToWorldPos(long index, BVector wsTargetPos, float time, BVector *pwsActualTargetPos)
{
   //-- get visual
   BVisual *pVis = getVisual();
   if (!pVis)
      return true;

   //-- get hardpoint
   const BHardpoint *pHP = getHardpoint(index);
   if (!pHP || pHP->getFlagSingleBoneIK() || pHP->getFlagCombined())
      return (true);

   //-- get the base location and direction of the hardpoint pitch bone in model and hardpoint space
   BVector msBonePos;
   BMatrix msBoneMatrix;
   BMatrix transformedBoneMatrix;
   if (!getHardpointPitchLocation(index, msBonePos, msBoneMatrix, &transformedBoneMatrix))
      return (true);

   //-- get world to model transform
   BMatrix worldToModelMatrix;
   getWorldMatrix(worldToModelMatrix);
   worldToModelMatrix.invert();

   //-- get model to hardpoint transform
   BMatrix modelToHardpointMatrix(msBoneMatrix);
   modelToHardpointMatrix.invert();

   //-- get world to hardpoint transform
   BMatrix worldToHardpointMatrix;
   worldToHardpointMatrix.mult(worldToModelMatrix, modelToHardpointMatrix);

   //-- get the target position in hardpoint space
   BVector tsTargetPos;
   BVector tsActualTargetPos;
   worldToHardpointMatrix.transformVectorAsPoint(wsTargetPos, tsTargetPos);
   if (pwsActualTargetPos)
      worldToHardpointMatrix.transformVectorAsPoint(*pwsActualTargetPos, tsActualTargetPos);
   else
      tsActualTargetPos = tsTargetPos;

   //-- get the target direction in hardpoint space
   BVector tsTargetDir(tsTargetPos);
   BVector tsActualTargetDir(tsActualTargetPos);
   tsTargetDir.normalize();
   tsActualTargetPos.normalize();

   //-- flatten to model space YZ plane
   float lengthSqr = tsTargetDir.lengthSquared();
   tsTargetDir.x = 0.0f;
   tsTargetDir.z = Math::fSqrt(lengthSqr - (tsTargetDir.y * tsTargetDir.y));
   tsTargetDir.normalize();
   lengthSqr = tsActualTargetDir.lengthSquared();
   tsActualTargetDir.x = 0.0f;
   tsActualTargetDir.z = Math::fSqrt(lengthSqr - (tsActualTargetDir.y * tsActualTargetDir.y));
   tsActualTargetDir.normalize();

   //-- cap target direction
   bool capped = capTargetPitch(pHP, tsTargetDir);

   //-- get transformed bone direction
   BVector transformedBoneDir;
   transformedBoneMatrix.getForward(transformedBoneDir);
   transformedBoneDir.normalize();

   //-- early out if we're aiming at our actual target
   float dot = transformedBoneDir.dot(tsActualTargetDir);
   if (dot >= HARDPOINT_TOLERANCE_STOP)
   {
      stopPitchTurningSound(index);
      return true;
   }

   //-- get angle between vectors
   dot = transformedBoneDir.dot(tsTargetDir);
   float angle = XMScalarACos(dot);

   //-- how fast can we turn?
   float rate = pHP->getPitchRotationRate() * time;

   //-- how far do we slerp?
   float k = Math::Min(rate / angle, 1.0f);

   //-- convert transformed bone direction to quaternion
   BVector transformedBoneUp;
   transformedBoneUp.assignCrossProduct(transformedBoneDir, cXAxisVector);
   transformedBoneUp.normalize();
   BQuaternion transformedBoneQuaternion(transformedBoneDir, transformedBoneUp, cXAxisVector);

   //-- convert target direction to quaternion
   BVector tsTargetUp;
   tsTargetUp.assignCrossProduct(tsTargetDir, cXAxisVector);
   tsTargetUp.normalize();
   BQuaternion targetPitchQuaternion(tsTargetDir, tsTargetUp, cXAxisVector);

   //-- slerp to new direction
   transformedBoneQuaternion = transformedBoneQuaternion.slerp(targetPitchQuaternion, k);
   transformedBoneQuaternion.normalize();

   //-- set pitch transform
   BMatrix pitchMatrix;
   transformedBoneQuaternion.toMatrix(pitchMatrix);
   pVis->setAttachmentTransform(pHP->getPitchAttachmentHandle(), pitchMatrix);

   playPitchTurningSound(index);

   return !capped;
}

//=============================================================================
// BObject::canOrientHardpointToWorldPos
//=============================================================================
bool BObject::canOrientHardpointToWorldPos(const BHardpoint *pHP, BVector tsTargetPos) const
{
   BASSERT(pHP);

   //-- get the target direction in hardpoint space
   BVector tsTargetDir(tsTargetPos);
   tsTargetDir.normalize();

   if (pHP->getMaxCombinedAngle() < cPi)
   {
      //-- Calculate angle
      float dot = cZAxisVector.dot(tsTargetDir);
      float angle = XMScalarACos(dot);

      //-- Is it more than the max?
      if (angle > pHP->getMaxCombinedAngle())
      {
         return false;
      }
   }
   return true;
}

//=============================================================================
// BObject::canYawHardpointToWorldPos
//=============================================================================
bool BObject::canYawHardpointToWorldPos(const BHardpoint *pHP, BVector tsTargetPos) const
{
   BASSERT(pHP);

   //-- flatten to model space XZ plane
   tsTargetPos.y = 0.0f;

   //-- get the target direction in hardpoint space
   BVector yawTargetDir(tsTargetPos);
   yawTargetDir.normalize();

   //-- cap target direction
   if ((pHP->getYawLeftMaxAngle() > -cPi) || (pHP->getYawRightMaxAngle() < cPi))
   {
      //-- Is the target on the left or on the right
      bool onLeft = (cXAxisVector.dot(yawTargetDir) < 0.0f);

      //-- Calculate max direction
      BVector maxDirection = cZAxisVector;
      maxDirection.rotateXZ(onLeft ? pHP->getYawLeftMaxAngle() : pHP->getYawRightMaxAngle());

      //-- Calculate max direction normal
      BVector maxNormal;
      maxNormal.assignCrossProduct(onLeft ? cYAxisVector : -cYAxisVector, maxDirection);
      maxNormal.normalize();

      //-- Make sure we're within bounds
      if (maxNormal.dot(yawTargetDir) < 0.0f)
         return false; // Can't rotate to target
   }

   return true;
}

//=============================================================================
// BObject::canPitchHardpointToWorldPos
//=============================================================================
bool BObject::canPitchHardpointToWorldPos(const BHardpoint *pHP, BVector tsTargetPos) const
{
   BASSERT(pHP);

   if (!pHP->getFlagHardPitchLimits())
      return true; // We don't care.

   //-- get the target direction in hardpoint space
   BVector tsTargetDir(tsTargetPos);
   tsTargetDir.normalize();

   //-- flatten to model space YZ plane
   float lengthSqr = tsTargetDir.lengthSquared();
   tsTargetDir.x = 0.0f;
   tsTargetDir.z = Math::fSqrt(lengthSqr - (tsTargetDir.y * tsTargetDir.y));
   tsTargetDir.normalize();

   //-- cap target direction
   if ((pHP->getPitchMaxAngle() < cPi) && (pHP->getPitchMinAngle() > -cPi))
   {
      //-- Is the target on top or on the bottom
      bool onBottom = (cYAxisVector.dot(tsTargetDir) < 0.0f);

      //-- Calculate max direction
      BVector maxDirection = cZAxisVector;
      maxDirection.rotateYZ(onBottom ? -pHP->getPitchMinAngle() : -pHP->getPitchMaxAngle());

      //-- Calculate max direction normal
      BVector maxNormal;
      maxNormal.assignCrossProduct(onBottom ? -cXAxisVector : cXAxisVector, maxDirection);
      maxNormal.normalize();

      //-- Make sure we're within bounds
      if (maxNormal.dot(tsTargetDir) < 0.0f)
         return false; // Can't rotate to target
   }

   return true;
}

//=============================================================================
// BObject::canOrientHardpointToWorldPos
//=============================================================================
bool BObject::canOrientHardpointToWorldPos(long index, BVector wsTargetPos) const
{
   //-- get visual
   if (!getVisual())
      return (true);

   //-- get hardpoint
   const BHardpoint *pHP = getHardpoint(index);
   if (!pHP)
      return (true);

   //-- get the base location and direction of the hardpoint yaw bone in model and hardpoint space
   BVector msBonePos;
   BMatrix msBoneMatrix;
   if (!getHardpointYawLocation(index, msBonePos, msBoneMatrix))
      return (true);

   //-- get world to model transform
   BMatrix worldToModelMatrix;
   getWorldMatrix(worldToModelMatrix);
   worldToModelMatrix.invert();

   //-- get model to hardpoint transform
   BMatrix modelToHardpointMatrix(msBoneMatrix);
   modelToHardpointMatrix.invert();

   //-- get world to hardpoint transform
   BMatrix worldToHardpointMatrix;
   worldToHardpointMatrix.mult(worldToModelMatrix, modelToHardpointMatrix);

   //-- get the target position in hardpoint space
   BVector tsTargetPos;
   worldToHardpointMatrix.transformVectorAsPoint(wsTargetPos, tsTargetPos);

   //-- check target direction vs hardpoint limits
   bool retVal = true;
   if (pHP->getYawAttachmentHandle() != pHP->getPitchAttachmentHandle())
      retVal = canOrientHardpointToWorldPos(pHP, tsTargetPos);
   else
      retVal = (canYawHardpointToWorldPos(pHP, tsTargetPos) && canPitchHardpointToWorldPos(pHP, tsTargetPos));

   return retVal;
}

//=============================================================================
// BObject::isHardpointOrientedToWorldPos
//=============================================================================
bool BObject::isHardpointOrientedToWorldPos(long index, BVector wsTargetPos, float tolerance, const BMatrix *pCachedYawHardpointMatrix) const
{
   BASSERT(tolerance <= HARDPOINT_TOLERANCE_STOP);

   //-- get visual
   if (!getVisual())
      return (true);

   //-- get hardpoint
   const BHardpoint *pHP = getHardpoint(index);
   if (!pHP)
      return (true);

   //-- get the base location and direction of the hardpoint bone in model and hardpoint space
   BVector msBonePos;
   BMatrix msBoneMatrix;
   BMatrix transformedBoneMatrix;
   bool gotYawHardpoint = false;

   if (pCachedYawHardpointMatrix)
   {
      msBoneMatrix = *pCachedYawHardpointMatrix;
      pCachedYawHardpointMatrix->getTranslation(msBonePos);
      if (getHardpointYawTransform(index, transformedBoneMatrix))
         gotYawHardpoint = true;
   }
   else
   {
      if (getHardpointYawLocation(index, msBonePos, msBoneMatrix, &transformedBoneMatrix))
         gotYawHardpoint = true;
   }

   //Use the Yaw location for the test if we have it, because that is what we care about when lining up the turret for the sim.
   if (!gotYawHardpoint)
   {
      if( !getHardpointPitchLocation(index, msBonePos, msBoneMatrix, &transformedBoneMatrix) )
         return (true);
   }

   //-- get model to world transform
   BMatrix modelToWorldMatrix;
   getWorldMatrix(modelToWorldMatrix);

   //-- get hardpoint to model transform
   BMatrix HardpointToModelMatrix(msBoneMatrix);

   //-- get hardpoint to world transform
   BMatrix HardpointToWorldMatrix;
   HardpointToWorldMatrix.mult(HardpointToModelMatrix, modelToWorldMatrix);

   //-- get world to hardpoint transform
   BMatrix worldToHardpointMatrix(HardpointToWorldMatrix);
   worldToHardpointMatrix.invert();

   //-- get transformed bone direction
   BVector transformedBoneDir;
   transformedBoneMatrix.getForward(transformedBoneDir);
   transformedBoneDir.y = 0.0f;
   transformedBoneDir.normalize();

   //-- get transformed bone position
   BVector transformedBonePos;
   transformedBoneMatrix.getTranslation(transformedBonePos);

   //-- get target direction
   BVector targetPos;
   worldToHardpointMatrix.transformVectorAsPoint(wsTargetPos, targetPos);

   BVector targetDir;
   targetDir.assignDifference(targetPos, transformedBonePos);
   targetDir.y = 0.0f;
   targetDir.normalize();

   //-- Are we oriented to the target position
   float dot = transformedBoneDir.dot(targetDir);
   return (dot >= tolerance);
}

//=============================================================================
// BObject::orientHardpointToWorldPos
//=============================================================================
bool BObject::orientHardpointToWorldPos(long index, BVector wsTargetPos, float time, BVector *pwsActualTargetPos, const BMatrix *pCachedYawHardpointMatrix)
{
   //-- get visual
   BVisual *pVis = getVisual();
   if (!pVis)
      return true;

   //-- get hardpoint
   const BHardpoint *pHP = getHardpoint(index);
   if (!pHP)
      return (true);

   //-- get the base location and direction of the hardpoint yaw bone in model and hardpoint space
   BVector msBonePos;
   BMatrix msBoneMatrix;
   BMatrix transformedBoneMatrix;
   if (pCachedYawHardpointMatrix)
   {
      msBoneMatrix = *pCachedYawHardpointMatrix;
      pCachedYawHardpointMatrix->getTranslation(msBonePos);
      if (!getHardpointYawTransform(index, transformedBoneMatrix))
         return true;
   }
   else
   {
      if (!getHardpointYawLocation(index, msBonePos, msBoneMatrix, &transformedBoneMatrix))
         return (true);
   }

   //-- get world to model transform
   BMatrix worldToModelMatrix;
   getWorldMatrix(worldToModelMatrix);
   worldToModelMatrix.invert();

   //-- get model to hardpoint transform
   BMatrix modelToHardpointMatrix(msBoneMatrix);
   modelToHardpointMatrix.invert();

   //-- get world to hardpoint transform
   BMatrix worldToHardpointMatrix;
   worldToHardpointMatrix.mult(worldToModelMatrix, modelToHardpointMatrix);

   //-- get the target position in hardpoint space
   BVector tsTargetPos;
   BVector tsActualTargetPos;
   worldToHardpointMatrix.transformVectorAsPoint(wsTargetPos, tsTargetPos);
   if (pwsActualTargetPos)
      worldToHardpointMatrix.transformVectorAsPoint(*pwsActualTargetPos, tsActualTargetPos);
   else
      tsActualTargetPos = tsTargetPos;

   //-- get the target direction in hardpoint space
   BVector tsTargetDir(tsTargetPos);
   BVector tsActualTargetDir(tsActualTargetPos);
   tsTargetDir.normalize();
   tsActualTargetDir.normalize();

   //-- cap target direction
   bool capped;
   if (pHP->getYawAttachmentHandle() != pHP->getPitchAttachmentHandle())
      capped = capTargetOrient(pHP, tsTargetDir);
   else
      capped = (capTargetYaw(pHP, tsTargetDir) | capTargetPitch(pHP, tsTargetDir));

   //-- get transformed bone direction
   BVector transformedBoneDir;
   transformedBoneMatrix.getForward(transformedBoneDir);
   transformedBoneDir.normalize();

   //-- early out if we're aiming at our actual target
   float dot = transformedBoneDir.dot(tsActualTargetDir);
   if (dot >= HARDPOINT_TOLERANCE_STOP)
   {
      stopYawTurningSound(index);
      return true;
   }

   //-- get angle between vectors
   dot = transformedBoneDir.dot(tsTargetDir);
   float angle = XMScalarACos(dot);

   //-- how fast can we turn?
   float rate = pHP->getYawRotationRate() * time;

   //-- how far do we slerp?
   float k = Math::Min(rate / angle, 1.0f);

   //-- convert transformed bone direction to quaternion
   BQuaternion transformedBoneQuaternion(transformedBoneMatrix);

   //-- convert target direction to quaternion
   BVector tsTargetRight;
   BVector tsTargetUp;
   tsTargetRight.assignCrossProduct(cYAxisVector, tsTargetDir);
   tsTargetRight.normalize();
   tsTargetUp.assignCrossProduct(tsTargetDir, tsTargetRight);
   tsTargetUp.normalize();
   BQuaternion targetQuaternion(tsTargetDir, tsTargetUp, tsTargetRight);

   //-- slerp to new direction
   transformedBoneQuaternion = transformedBoneQuaternion.slerp(targetQuaternion, k);
   transformedBoneQuaternion.normalize();

   //-- set transform
   if (pHP->getFlagSingleBoneIK())
   {
      if (!getFlagIKDisabled())
      {
         mpVisual->setIKNodeSingleBone(0, msBonePos, transformedBoneQuaternion); // SLB: HACK
         mpVisual->setIKNodeActive(0, true);
      }
   }
   else
   {
      BMatrix matrix;
      transformedBoneQuaternion.toMatrix(matrix);
      pVis->setAttachmentTransform(pHP->getYawAttachmentHandle(), matrix);
   }

   playYawTurningSound(index);

   return !capped;
}

//=============================================================================
// BObject::pitchHardpointToGoalAngle
//=============================================================================
void BObject::pitchHardpointToGoalAngle(long index, float goalAngle, float time)
{
   //-- get visual
   BVisual *pVis = getVisual();
   if (!pVis)
      return;

   //-- get hardpoint
   const BHardpoint *pHP = getHardpoint(index);
   if (!pHP)
      return;

   if (goalAngle > pHP->getPitchMaxAngle())
      goalAngle = pHP->getPitchMaxAngle();
   else if (goalAngle < pHP->getPitchMinAngle())
      goalAngle = pHP->getPitchMinAngle();

   //-- get pitch transform
   BMatrix pitchTransform;
   getHardpointPitchTransform(index, pitchTransform);

   //-- get pitch direction
   BVector pitchDir;
   pitchTransform.getForward(pitchDir);
   pitchDir.normalize();

   //-- set target direction
   BVector goalDir = cZAxisVector;

   //-- get angle between vectors
   float dot = goalDir.dot(pitchDir);
   float currAngle = XMScalarACos(dot);
   float diffAngle = goalAngle - currAngle;

   //-- do we need to pitch?
   if (fabs(diffAngle) > cFloatCompareEpsilon)
   {
      //-- how fast can we turn?
      float rate = pHP->getPitchRotationRate() * time;

      //-- how far do we slerp?
      float k = 0.0f;
      if (diffAngle >= 0.0f)
         k = Math::Min(rate / diffAngle, 1.0f);
      else
         k = Math::Min(rate / -diffAngle, 1.0f);

      //-- set pitch quaternion
      BQuaternion pitchQuaternion(pitchTransform);

      //-- set target quaternion
      BMatrix goalTransform;
      goalTransform.makeRotateX(goalAngle);

      BQuaternion goalQuaternion(goalTransform);

      //-- slerp
      pitchQuaternion = pitchQuaternion.slerp(goalQuaternion, k);
      pitchQuaternion.normalize();

      //-- set pitch transform
      BMatrix pitchMatrix;
      pitchQuaternion.toMatrix(pitchMatrix);
      pVis->setAttachmentTransform(pHP->getPitchAttachmentHandle(), pitchMatrix);

      playPitchTurningSound(index);
   }
   else
   {
      stopPitchTurningSound(index);
   }
}

//=============================================================================
//=============================================================================
bool BObject::autoCenterHardpoint(int index, float time)
{
   bool yawDone;
   bool pitchDone;

   //-- get visual
   BVisual *pVis = getVisual();
   if (!pVis)
      return true;

   //-- get hardpoint
   const BHardpoint *pHP = getHardpoint(index);
   if (!pHP)
      return (true);

   if (!isAlive())
   {
      stopAllHardpointSounds();
      return true;
   }
   else
   {
      //-- get yaw transform
      BMatrix yawTransform;
      yawDone = !getHardpointYawTransform(index, yawTransform);

      //-- center yaw
      if (!yawDone)
      {
         //-- get yaw direction
         BVector yawDir;
         yawTransform.getForward(yawDir);
         yawDir.normalize();

         //-- set target direction
         BVector targetDir = cZAxisVector;

         //-- do we need to yaw?
         float dot = targetDir.dot(yawDir);
         if (dot < HARDPOINT_TOLERANCE_STOP)
         {
            //-- get angle between vectors
            float angle = XMScalarACos(dot);

            //-- how fast can we turn?
            float rate = pHP->getYawRotationRate() * time;

            //-- how far do we slerp?
            float k = Math::Min(rate / angle, 1.0f);

            //-- set yaw quaternion
            BQuaternion yawQuaternion(yawTransform);

            //-- set target quaternion
            BQuaternion targetQuaternion = XMQuaternionIdentity();

            //-- slerp
            yawQuaternion = yawQuaternion.slerp(targetQuaternion, k);
            yawQuaternion.normalize();

            //-- set yaw transform
            if (pHP->getFlagSingleBoneIK())
            {
               if (!getFlagIKDisabled())
               {
                  BVector bonePos;
                  yawTransform.getTranslation(bonePos);
                  mpVisual->setIKNodeSingleBone(0, bonePos, yawQuaternion); // SLB: HACK
                  mpVisual->setIKNodeActive(0, true);
               }
            }
            else
            {
               BMatrix yawMatrix;
               yawQuaternion.toMatrix(yawMatrix);
               pVis->setAttachmentTransform(pHP->getYawAttachmentHandle(), yawMatrix);
            }

            playYawTurningSound(index);
         }
         else
         {
            yawDone = true;
            stopYawTurningSound(index);
         }
      }

      if (!(pHP->getFlagSingleBoneIK() || pHP->getFlagCombined()))
      {
         //-- get pitch transform
         BMatrix pitchTransform;
         pitchDone = !getHardpointPitchTransform(index, pitchTransform);

         //-- center pitch
         if (!pitchDone)
         {
            //-- get pitch direction
            BVector pitchDir;
            pitchTransform.getForward(pitchDir);
            pitchDir.normalize();

            //-- set target direction
            BVector targetDir = cZAxisVector;

            //-- get angle between vectors
            float dot = targetDir.dot(pitchDir);
            float angle = XMScalarACos(dot);

            //-- do we need to pitch?
            if (angle > cFloatCompareEpsilon)
            {
               //-- how fast can we turn?
               float rate = pHP->getPitchRotationRate() * time;

               //-- how far do we slerp?
               float k = Math::Min(rate / angle, 1.0f);

               //-- set pitch quaternion
               BQuaternion pitchQuaternion(pitchTransform);

               //-- set target quaternion
               BQuaternion targetQuaternion = XMQuaternionIdentity();

               //-- slerp
               pitchQuaternion = pitchQuaternion.slerp(targetQuaternion, k);
               pitchQuaternion.normalize();

               //-- set pitch transform
               BMatrix pitchMatrix;
               pitchQuaternion.toMatrix(pitchMatrix);
               pVis->setAttachmentTransform(pHP->getPitchAttachmentHandle(), pitchMatrix);

               playPitchTurningSound(index);
            }
            else
            {
               pitchDone = true;
               stopPitchTurningSound(index);
            }
         }
      }
      else
         pitchDone = true;
   }

   return (yawDone && pitchDone);
}

//=============================================================================
// BObject::updateHardpoints
//=============================================================================
bool BObject::updateHardpoints( float elapsed )
{
   //SCOPEDSAMPLE(BObjectUpdateHardpoints);

   BVisual *pVis = getVisual();
   if (!pVis)
      return false;

   // VT: 10/27/2008 - because we have the case where multiple hardpoints control the same pitch / yaw, 
   // we need to make sure no actions have control over ANY of the yaw / pitch attachments before autocentering
   // suffer the cost of O(2n) for simpler logic here 
   long count = mHardpointState.getNumber();
   static BSmallDynamicSimArray<long> controlledYawHandles;
   controlledYawHandles.setNumber(0);
   static BSmallDynamicSimArray<long> controlledPitchHandles;
   controlledPitchHandles.setNumber(0);
   for (long j = 0; j < count; ++j)
   {
      const BHardpoint *pHP = getHardpoint(j);
      if (!pHP)
         continue;

      BHardpointState& hardPointState = mHardpointState[j];
      if (hardPointState.mOwnerAction != -1)
      {
         if (pHP->getYawAttachmentHandle() != -1)
            controlledYawHandles.add(pHP->getYawAttachmentHandle());
         if (pHP->getPitchAttachmentHandle() != -1)
            controlledPitchHandles.add(pHP->getPitchAttachmentHandle());
      }
   }

   for (long i = 0; i < count; ++i)
   {
      const BHardpoint *pHP = getHardpoint(i);

      // AJL FIXME 6/28/07 - Added a NULl check to prevent a crash we got. Not sure how this could happen though.
      if (!pHP)
         break;

      BHardpointState& hardPointState = mHardpointState[i];

      if (pHP->getFlagAutoCenter())
      {
         //-- don't control hardpoints that an action has claimed
         if (hardPointState.mOwnerAction == -1)
         {
            if (hardPointState.mAllowAutoCentering)
            {
               // make sure that no other action may already be controlling these attachments
               if ( (pHP->getYawAttachmentHandle() == -1 || controlledYawHandles.find(pHP->getYawAttachmentHandle()) == cInvalidIndex) && 
                    (pHP->getPitchAttachmentHandle() == -1 || controlledPitchHandles.find(pHP->getPitchAttachmentHandle()) == cInvalidIndex) )
               {

                  if (hardPointState.mAutoCenteringTimer <= 0.0f)
                  {
                     if (autoCenterHardpoint(i, elapsed))
                     {
                        hardPointState.mAutoCenteringTimer = 0.0f;
                        hardPointState.mAllowAutoCentering = false;
                     }
                  }
                  else
                     hardPointState.mAutoCenteringTimer -= elapsed;
               }
            }
         }
      }

      // Start/Stop hardpoint yaw sounds
      if ((hardPointState.mYawSound != hardPointState.mYawSoundPlaying) && (hardPointState.mYawSoundActivationTimer >= cFloatCompareEpsilon))
      {
         hardPointState.mYawSoundActivationTimer -= elapsed;

         if (hardPointState.mYawSoundActivationTimer < cFloatCompareEpsilon)
         {
            hardPointState.mYawSoundActivationTimer = 0.0f;
            hardPointState.mYawSoundPlaying = hardPointState.mYawSound;
            BCueIndex cueIndex = hardPointState.mYawSound ? pHP->getStartYawSoundCue() : pHP->getStopYawSoundCue();
            bool nonCritical = hardPointState.mYawSound; //Stop sounds always need to play.  Start sounds are optional.
            gWorld->getWorldSoundManager()->addSound(this, -1, cueIndex, nonCritical, cInvalidCueIndex, nonCritical, nonCritical);
         }
      }

      // Start/Stop hardpoint pitch sounds
      if ((hardPointState.mPitchSound != hardPointState.mPitchSoundPlaying) && (hardPointState.mPitchSoundActivationTimer >= cFloatCompareEpsilon))
      {
         hardPointState.mPitchSoundActivationTimer -= elapsed;

         if (hardPointState.mPitchSoundActivationTimer < cFloatCompareEpsilon)
         {
            hardPointState.mPitchSoundActivationTimer = 0.0f;
            hardPointState.mPitchSoundPlaying = hardPointState.mPitchSound;
            BCueIndex cueIndex = hardPointState.mPitchSound ? pHP->getStartPitchSoundCue() : pHP->getStopPitchSoundCue();
            bool nonCritical = hardPointState.mYawSound; //Stop sounds always need to play.  Start sounds are optional.
            gWorld->getWorldSoundManager()->addSound(this, -1, cueIndex, nonCritical, cInvalidCueIndex, nonCritical, nonCritical);
         }
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BObject::playYawTurningSound(long hardpointIndex)
{
   const BHardpoint* pHP = getHardpoint(hardpointIndex);

   if(!pHP)
      return;

   // Play yaw turning sound
   if (pHP->getStartYawSoundCue() != cInvalidCueIndex)
   {
      BHardpointState& hardPointState = mHardpointState[hardpointIndex];

      if (hardPointState.mYawSound)
      {
         if (!hardPointState.mYawSoundPlaying && (hardPointState.mYawSoundActivationTimer < cFloatCompareEpsilon))
            hardPointState.mYawSoundActivationTimer = HARDPOINT_SOUND_ACTIVATION_TIMER;
      }
      else
      {
         hardPointState.mYawSound = true;
         hardPointState.mYawSoundActivationTimer = hardPointState.mYawSoundPlaying ? 0.0f : HARDPOINT_SOUND_ACTIVATION_TIMER;
      }
   }
}

//==============================================================================
//==============================================================================
void BObject::stopYawTurningSound(long hardpointIndex, bool immediate)
{
   const BHardpoint* pHP = getHardpoint(hardpointIndex);

   if(!pHP)
      return;

   BVisual *pVis = getVisual();
   if (!pVis)
      return;

   // Stop yaw turning sound
   if (pHP->getStartYawSoundCue() != cInvalidCueIndex)
   {
      BHardpointState& hardPointState = mHardpointState[hardpointIndex];

      if (immediate && hardPointState.mYawSoundPlaying)
      {
         hardPointState.mYawSoundActivationTimer = 0.0f;
         hardPointState.mYawSoundPlaying = false;
         hardPointState.mYawSound = false;
         gWorld->getWorldSoundManager()->addSound(this, -1, pHP->getStopYawSoundCue(), false, cInvalidCueIndex, false, false);
      }
      else if (hardPointState.mYawSound)
      {
         hardPointState.mYawSound = false;
         hardPointState.mYawSoundActivationTimer = hardPointState.mYawSoundPlaying ? HARDPOINT_SOUND_ACTIVATION_TIMER : 0.0f;
      }
      else
      {
         if (hardPointState.mYawSoundPlaying && (hardPointState.mYawSoundActivationTimer < cFloatCompareEpsilon))
            hardPointState.mYawSoundActivationTimer = HARDPOINT_SOUND_ACTIVATION_TIMER;
      }
   }
}

//==============================================================================
//==============================================================================
void BObject::playPitchTurningSound(long hardpointIndex)
{
   const BHardpoint* pHP = getHardpoint(hardpointIndex);

   if(!pHP)
      return;

   // Play Pitch turning sound
   if (pHP->getStartPitchSoundCue() != cInvalidCueIndex)
   {
      BHardpointState& hardPointState = mHardpointState[hardpointIndex];

      if (hardPointState.mPitchSound)
      {
         if (!hardPointState.mPitchSoundPlaying && (hardPointState.mPitchSoundActivationTimer < cFloatCompareEpsilon))
            hardPointState.mPitchSoundActivationTimer = HARDPOINT_SOUND_ACTIVATION_TIMER;
      }
      else
      {
         hardPointState.mPitchSound = true;
         hardPointState.mPitchSoundActivationTimer = hardPointState.mPitchSoundPlaying ? 0.0f : HARDPOINT_SOUND_ACTIVATION_TIMER;
      }
   }
}

//==============================================================================
//==============================================================================
void BObject::stopPitchTurningSound(long hardpointIndex, bool immediate)
{
   const BHardpoint* pHP = getHardpoint(hardpointIndex);

   if(!pHP)
      return;

   BVisual *pVis = getVisual();
   if (!pVis)
      return;

   // Stop Pitch turning sound
   if (pHP->getStartPitchSoundCue() != cInvalidCueIndex)
   {
      BHardpointState& hardPointState = mHardpointState[hardpointIndex];

      if (immediate && hardPointState.mPitchSoundPlaying)
      {
         hardPointState.mPitchSoundActivationTimer = 0.0f;
         hardPointState.mPitchSoundPlaying = false;
         hardPointState.mPitchSound = false;
         gWorld->getWorldSoundManager()->addSound(this, -1, pHP->getStopPitchSoundCue(), false, cInvalidCueIndex, false, false);
      }
      else if (hardPointState.mPitchSound)
      {
         hardPointState.mPitchSound = false;
         hardPointState.mPitchSoundActivationTimer = hardPointState.mPitchSoundPlaying ? HARDPOINT_SOUND_ACTIVATION_TIMER : 0.0f;
      }
      else
      {
         if (hardPointState.mPitchSoundPlaying && (hardPointState.mPitchSoundActivationTimer < cFloatCompareEpsilon))
            hardPointState.mPitchSoundActivationTimer = HARDPOINT_SOUND_ACTIVATION_TIMER;
      }
   }
}

//==============================================================================
//==============================================================================
void BObject::stopAllHardpointSounds()
{
   long numHardpoints = getNumberHardpoints();
   for (long i = 0; i < numHardpoints; i++)
   {
      stopYawTurningSound(i, true);
      stopPitchTurningSound(i, true);
   }
}

//=============================================================================
//=============================================================================
bool BObject::grabHardpoint(long actionID, long index, BUnitOppID oppID)
{
   if (index < 0 || index >= mHardpointState.getNumber())
      return (false);
   if (actionID < 0)
      return (false);

   if (getClassType() != cClassTypeObject)
   {
      syncUnitData("BObject::grabHardpoint actionID", actionID);
      syncUnitData("BObject::grabHardpoint index", index);
      syncUnitData("BObject::grabHardpoint ownerAction", mHardpointState[index].mOwnerAction);
   }

   if(hardpointHasAction(actionID, index))
      return true;

   if(!canGrabHardpoint(actionID, index, oppID))
      return false;

   // VT 10/27/2008 - we may need to add a check in here to validate that no other hardpoint
   // contains the same yaw / pitch hardpoints that we want and bail if that is controlled
   // but may require some fixing of slave / primary attack actions that depend on grabbing
   // hardpoints that control the same yaw / pitch attachment - see scorpion >_<


   //Good to go.
   mHardpointState[index].mOwnerAction=actionID;
   mHardpointState[index].mAutoCenteringTimer = 0.0f;
   mHardpointState[index].mAllowAutoCentering=false;
   mHardpointState[index].mOppID = oppID;
   return (true);
}

//=============================================================================
//=============================================================================
bool BObject::hardpointHasAction(long actionID, long index)
{
   //If we already have it, return true.
   return (mHardpointState[index].mOwnerAction == actionID);
}

//=============================================================================
//=============================================================================
bool BObject::canGrabHardpoint(long actionID, long index, BUnitOppID oppID)
{
   if (index < 0 || index >= mHardpointState.getNumber())
      return (false);
   if (actionID < 0)
      return (false);

   if (getClassType() != cClassTypeObject)
   {
      syncUnitData("BObject::canGrabHardpoint actionID", actionID);
      syncUnitData("BObject::canGrabHardpoint index", index);
      syncUnitData("BObject::canGrabHardpoint ownerAction", mHardpointState[index].mOwnerAction);
   }

   //Fail if someone else has it with higher priority.
   if (mHardpointState[index].mOwnerAction != -1)
   {
      uint currentPriority = getOppPriority(mHardpointState[index].mOppID);
      uint newPriority = getOppPriority(oppID);
      if (currentPriority >= newPriority)
         return (false);
   }

   return true;
}

//=============================================================================
//=============================================================================
long BObject::getHardpointController(long index) const
{
   if (index < 0 || index >= mHardpointState.getNumber())
      return (false);
   return (mHardpointState[index].mOwnerAction);

}

//=============================================================================
//=============================================================================
bool BObject::releaseHardpoint(long actionID, long index)
{
   if (index < 0 || index >= mHardpointState.getNumber())
      return (false);
   if (actionID < 0)
      return (false);
   if (mHardpointState[index].mOwnerAction != actionID)
      return (false);

   mHardpointState[index].mOwnerAction=-1;
   mHardpointState[index].mAutoCenteringTimer = 5.0f;
   mHardpointState[index].mAllowAutoCentering = true;
   mHardpointState[index].mOppID = BUnitOpp::cInvalidID;

   stopYawTurningSound(index);
   stopPitchTurningSound(index);

   return (true);
}

//=============================================================================
//=============================================================================
void BObject::clearHardpoint(long index)
{
   if (index < 0 || index >= mHardpointState.getNumber())
      return;
   mHardpointState[index].mOwnerAction=-1;
   mHardpointState[index].mAutoCenteringTimer = 0.0f;
   mHardpointState[index].mAllowAutoCentering = true;
   mHardpointState[index].mOppID = BUnitOpp::cInvalidID;
}

//=============================================================================
//=============================================================================
bool BObject::getHardpointYawTargetLocation(long index, BVector& wsYawPoint) const
{
   //-- get visual
   BVisual *pVis = getVisual();
   if (!pVis)
      return false;

   //-- get hardpoint
   const BHardpoint *pHP = getHardpoint(index);
   if (!pHP)
      return (false);

   //-- get the base location and direction of the hardpoint yaw bone in model space
   BVector msBonePos;
   BMatrix msBoneMatrix;
   BMatrix transformedBoneMatrix;
   if (!getHardpointYawLocation(index, msBonePos, msBoneMatrix, &transformedBoneMatrix))
      return (false);

   BMatrix modelToWorldMatrix;
   getWorldMatrix(modelToWorldMatrix);

   BVector wsBonePos;

   modelToWorldMatrix.transformVectorAsPoint(msBonePos, wsBonePos);

   BVector wsYawDir;
   transformedBoneMatrix.getForward(wsYawDir);
   wsYawDir.normalize();   
   modelToWorldMatrix.transformVector(wsYawDir, wsYawDir);
   wsYawDir.normalize();      

   wsYawDir.scale(10000.0f);

   wsYawPoint = wsBonePos + wsYawDir;
   return true;   
}

//=============================================================================
// BObject::loadTacticAnimInfo
//=============================================================================
void BObject::loadTacticAnimInfo(const BProtoObject *pProto)
{   
   BTactic *pTactic = pProto->getTactic();
   if (!pTactic)
      return;
   long numProtoActions = pTactic->getNumberProtoActions();
   for(long i = 0; i < numProtoActions; i++)
   {
      BProtoAction *pProtoAction = pTactic->getProtoAction(i);
      if(!pProtoAction)
         continue;

      // not sure why we should check this - seems like some of the damage calculations underneath could be needed by more actions... 
      if ((pProtoAction->getActionType() != BAction::cActionTypeUnitRangedAttack) && (pProtoAction->getActionType() != BAction::cActionTypeUnitSecondaryTurretAttack) && 
         (pProtoAction->getActionType() != BAction::cActionTypeUnitSlaveTurretAttack))
         continue;
      
      BProtoVisual *pProtoVis = gVisualManager.getProtoVisual(pProto->getProtoVisualIndex(), true);
      if(pProtoVis)
      {
         float reloadTime = 0.0f;
         float damagePerAttack = 0.0f;
         long maxNumAttacksPerAnim = 0;

         // Which proto visual model do we expect the tags to come from?
         long protoVisualModelIndex = -1;
         if (mpVisual)
         {
            const BHardpoint* pHP = getHardpoint(pProtoAction->getHardpointID());
            if (pHP && !pHP->getFlagSingleBoneIK())
            {
               if (pHP->getPitchAttachmentHandle() != -1)
                  protoVisualModelIndex = pProtoVis->getModelIndex(pHP->getPitchAttachmentName());

               if ((protoVisualModelIndex == -1) && (pHP->getYawAttachmentHandle() != -1))
                  protoVisualModelIndex = pProtoVis->getModelIndex(pHP->getYawAttachmentName());
            }
            else
            {
               // check if we have logic that changes the model based off the squad mode
               int squadMode = pProtoAction->getSquadMode();
               if (squadMode != -1 && pProtoVis->getFlag(BProtoVisual::cFlagHasSquadModeLogic))
               {
                  const BProtoVisualLogicNode* pLogicNode = pProtoVis->getLogicNode();
                  if (pLogicNode->mLogicType == cVisualLogicSquadMode)
                  {
                     long valueCount = pLogicNode->mLogicValues.getNumber();
                     for(long i = 0; i < valueCount; ++i)
                     {
                        const BProtoVisualLogicValue* pLogicValue=&(pLogicNode->mLogicValues[i]);
                        long logicSquadMode = (long)pLogicValue->mValueDWORD;
                        if(squadMode == logicSquadMode && pLogicValue->mpModel)
                           protoVisualModelIndex = pLogicValue->mpModel->mRefModelIndex;
                     }
                  }
               }
            }
         }

         //-- Determine cooldown average            
         float preAttack = (pProtoAction->getPreAttackCooldownMax() + pProtoAction->getPreAttackCooldownMin()) / 2;
         float postAttack = (pProtoAction->getPostAttackCooldownMax() + pProtoAction->getPostAttackCooldownMin()) / 2;
         float cooldownAverage = preAttack + postAttack;

         long animType = pProtoAction->getAnimType();
         pProtoVis->computeAttackInfo(protoVisualModelIndex, animType, pProtoAction->getDamagePerSecond(), pProtoAction->getUseDPSasDPA(), cooldownAverage, damagePerAttack, maxNumAttacksPerAnim);
         pProtoVis->computeReloadInfo(pProtoAction->getReloadAnimType(), reloadTime);
         if(damagePerAttack != 0.0f)
         {
            if (reloadTime != 0.0f && !pProtoAction->getUseDPSasDPA())
            {
               float timePerAttack = damagePerAttack / pProtoAction->getDamagePerSecond();
               float attackTime = ((float) pProtoAction->getVisualAmmo()) * timePerAttack;
               float k = 1.0f + (reloadTime / attackTime); // Scale DPA by this amount to factor in reload time

               damagePerAttack *= k;
            }

            //Now each player will do their own calculation.  Transformation of units was causing problems since the flag was stored on the base unit.  MWC 3/20/2008
            pProtoAction->setDamagePerAttack(damagePerAttack);
            pProtoAction->setMaxNumAttacksPerAnim(maxNumAttacksPerAnim);
         }
         else
         {
            gConsole.output(cMsgError, "Tactic %s -- ProtoAction %s did not find attack tags for specifed anim!", pTactic->getName().getPtr(), pProtoAction->getName().getPtr());
         }
      }        
   }      
   pTactic->setAnimInfoLoaded(true);
}

//==============================================================================
// createPhysicsReplacement
//==============================================================================
BObject* BObject::createPhysicsReplacement()
{
   if (!gWorld)
      return(NULL);

   gSimHelperThread.sync();

   //DJB 5/30/07 - Pass in mID of this object so when the replacement is created it copies the Visual information from this object.
   BEntity* pEnt = gWorld->getEntity(gWorld->createEntity(mProtoID, false, mPlayerID, mPosition, mForward, mRight, true, true, false, mID)); 
   if (!pEnt)
      return(NULL);

   BObject* pObj = (BObject*) pEnt;
   if (!pObj->getPhysicsObject())
   {
      //destory()
      //-- DJB 12/11/06 Destroy is called on the object no matter what in the unitactiondeath, it looks like this should destroy the 
      //   physics object if it doesnt have one.
      pObj->kill(true);
      return NULL;
   }

   // SLB: Update visibility on creation for audio
   if(getFlagVisibility() && !getFlagIsRevealer() && !getFlagBlockLOS())
   {
      pObj->updateVisibleLists();
   }

   pObj->setFlagSelectable(false);

   // send out a notification
   notify(BEntity::cEventUnitPhysicsReplacement, mID, (DWORD)pObj->getID(), 0);

   return(pObj);
}

//==============================================================================
// Change objejct's player ownership
//==============================================================================
void BObject::changeOwner(BPlayerID newPlayerID, BProtoObjectID newPOID)
{
   BPlayerID oldPlayerID = getPlayerID();
   if (newPlayerID == oldPlayerID)
   {
      return;
   }

   bool updateLOS = getFlagLOSMarked();
   if (updateLOS)
   {
      markLOSOff();
   }

   // cache off the old proto before we update the player
   const BProtoObject* pOldProtoObject = getProtoObject();
   sendEvent(mID, mID, BEntity::cEventSwitchToPlayerID, newPlayerID);

   // set our new proto object if one was passed in 
   if (newPOID != cInvalidProtoObjectID)
      setProtoID(newPOID);

   // Player changed so update the cached proto object
   BProtoObject* pNewProtoObject = NULL;
   BPlayer* pPlayer = getPlayer();
   if (pPlayer)
   {
      pNewProtoObject = pPlayer->getProtoObject(getProtoID());

      // if this player doesn't have this protoobject, 
      // and if the old one is a unique one, try again with the base type 
      if (!pNewProtoObject && pOldProtoObject && pOldProtoObject->getFlagUniqueInstance())
      {
         setProtoID(pOldProtoObject->getBaseType());
         pNewProtoObject = pPlayer->getProtoObject(getProtoID());
      }
   }
   setProtoObject(pNewProtoObject);

   if (updateLOS)
   {
      markLOSOn();
   }
}

//==============================================================================
//==============================================================================
bool BObject::isSelectable(BTeamID teamId) const
{
   // check against the team selection mask first
   if (~mTeamSelectionMask & (1 << teamId))
      return false;

   return (BEntity::isSelectable(teamId));
}

//==============================================================================
//==============================================================================
int BObject::getSelectType(BTeamID teamId) const
{
   if (!isSelectable(teamId))
      return (cSelectTypeNone);

   return (getProtoObject()->getSelectType());
}

//==============================================================================
//==============================================================================
int BObject::getGotoType() const
{
   return (getProtoObject()->getGotoType());
}

//==============================================================================
//==============================================================================
void BObject::setExplorationGroup(int16 newGroup)
{
   // don't support changing exploration groups right now
   BASSERT(mExplorationGroup == -1);

   mExplorationGroup = newGroup;
   gWorld->addObjectToExplorationGroup(*this);
}

//==============================================================================
//==============================================================================
int16 BObject::getExplorationGroup() const
{
   return mExplorationGroup;
}

//==============================================================================
// BObject::lockAnimation
//==============================================================================
void BObject::lockAnimation(DWORD lockDuration, bool updateSquadPositionOnAnimationUnlock)
{
   if (lockDuration > 0)
   {
      setFlagAnimationLocked(true);
      setFlagDontLockMovementAnimation(false);
      setFlagUpdateSquadPositionOnAnimationUnlock(updateSquadPositionOnAnimationUnlock);
      mAnimationLockEnds = gWorld->getGametime() + lockDuration;
      mAnimationState.setDirty();
   }
}

//==============================================================================
// BObject::unlockAnimation
//==============================================================================
void BObject::unlockAnimation()
{
   setFlagAnimationLocked(false);
   setFlagDontLockMovementAnimation(false);
   setAnimationEnabled(true, true);
   mAnimationLockEnds = 0;
   mAnimationState.setDirty();

   if (getFlagTiesToGround() && !mFlagGarrisoned)
      tieToGround();

   if (getFlagUpdateSquadPositionOnAnimationUnlock())
   {
      setFlagUpdateSquadPositionOnAnimationUnlock(false);

      BSquad *pSquad = gWorld->getSquad(getParentID());
      if (pSquad)
      {
         BVector averagePosition = XMVectorZero();
         long numUnits = pSquad->getNumberChildren();
         for (long i = 0; i < numUnits; i++)
         {
            BUnit *pUnit = gWorld->getUnit(pSquad->getChild(i));
            BASSERT(pUnit);
            averagePosition += pUnit->getPosition();
         }

         averagePosition /= float(numUnits);
         pSquad->setPosition(averagePosition);
         if (pSquad->getFlagTiesToGround() && !mFlagGarrisoned)
            pSquad->tieToGround();
      }
   }

   setFlagUseMaxHeight(false);
}

//==============================================================================
// BObject::startChain
//==============================================================================
void BObject::startChain(long fromAnimType, long toAnimType)
{
   if (mAnimationState.getAnimType() == fromAnimType)
   {
      if (mAnimationState.getState() == BObjectAnimationState::cAnimationStatePostAnim)
      {
         mAnimationState.setState(BObjectAnimationState::cAnimationStateIdle, toAnimType, false);
      }
      else
      {
         mAnimationState.setAnimType(toAnimType);
         mAnimationState.clearApplyInstantly();
      }
   }
}

//==============================================================================
// BObject::notify
//==============================================================================
void BObject::notify(DWORD eventType, BEntityID sender, DWORD data, DWORD data2)
{
   BEntity::notify(eventType, sender, data, data2);

   // Lock/unlock ground IK
   if (eventType == cEventAnimGroundIKTag)
   {
      if (mpVisual)
      {
         BProtoVisualTag* pTag = (BProtoVisualTag*) data;
         long boneHandle = pTag->mToBoneHandle;
         BIKNode* pIKNode = mpVisual->getIKNodeFromTypeBoneHandle(BIKNode::cIKNodeTypeGround, boneHandle);
         if (!pIKNode)
            return;

         // Check if idle.  Ignore any groundIK tags during idle after the idleTransition is complete
         long currentAnimType = getAnimationType(cMovementAnimationTrack);
         if (!isMoveAnimType(currentAnimType) && !isTurnAnimType(currentAnimType) && !pIKNode->mIdleTransitioning)
            return;

         bool lock = pTag->getFlag(BProtoVisualTag::cFlagLockToGround) ? 1 : 0;
         float start = pTag->mPosition;
         float end = pTag->mValue0;
         bool heightOnlyLock = pTag->mBoolValue0;

         // Set IK data
         mpVisual->lockIKNodeToGround(boneHandle, lock, start, end);
         if (pIKNode)
            pIKNode->mHeightOnlyLock = heightOnlyLock;

         //-- Stepped on the ground
         if(lock)
         {
            BVector groundPos;
            BMatrix worldMatrix;
            getWorldMatrix(worldMatrix);
            mpVisual->getBone(boneHandle, &groundPos, NULL, NULL, &worldMatrix, true);

            //-- Does this unit do trample damage?
            BTactic* pTactic = getTactic();
            BProtoAction *pProtoAction = pTactic ? pTactic->getTrampleAction() : NULL;
            if(pProtoAction)
            {
               bool unit = (getClassType() == cClassTypeUnit);
               if(unit && pProtoAction)
               {
                  BDamageHelper::doAreaEffectDamage(mPlayerID, getTeamID(), mID, pProtoAction, groundPos);
               }
            }

            //-- Does this object have a footstep sound?
            BCueIndex cueIndex = getProtoObject()->getSound(cObjectSoundStepDown);
            if(cueIndex != -1)
            {
               //DJBFIXME: Add checkSoundRadius and checkFOW to the sound info on protoobject
               gWorld->getWorldSoundManager()->addSound(this, boneHandle, cueIndex, true, cInvalidCueIndex, false, true);
            }
         }
         else if(!lock)
         {
            //-- Does this object have a footstep sound?
            BCueIndex cueIndex = getProtoObject()->getSound(cObjectSoundStepUp);
            if(cueIndex != -1)
            {               
               gWorld->getWorldSoundManager()->addSound(this, boneHandle, cueIndex, true, cInvalidCueIndex, false, true);
            }
         }
      }
   }
   else if (eventType == cEventPickedUp)
   {
      // do special stuff when you're picked up - for now, just play an animation
      setAnimationState(BObjectAnimationState::cAnimationStateMisc, cAnimTypeFlail, true, true);
      computeAnimation();
   }
}


//==============================================================================
//==============================================================================
void BObject::onDoppleBitsChanged(bool dopple, BTeamID teamID)
{
   if (getClassType() != cClassTypeUnit)
      return;

   // Bomb check.
   BTeam* pTeam = gWorld->getTeam(teamID);
   if (!pTeam)
      return;

   BSquad* pParentSquad = gWorld->getSquad(getParentID());
   if (pParentSquad)
   {
      BASSERT(isClassType(BEntity::cClassTypeUnit));
      if (dopple)
      {
         mDoppleBits.set(teamID);
         if (!pParentSquad->isVisible(teamID))
         {
            BKB* pKB = gWorld->getKB(teamID);
            if (pKB)
            {
               pKB->updateSquad(pParentSquad);
               pKB->loseVisToSquad(pParentSquad);
            }
         }
      }
      else
      {
         bool squadWasVisible = pParentSquad->isVisible(teamID);
         mDoppleBits.unset(teamID);
         if (!squadWasVisible)
         {
            BKB* pKB = gWorld->getKB(teamID);
            if (pKB)
            {
               pKB->updateSquad(pParentSquad);
               pKB->acquireVisToSquad(pParentSquad);
            }
         }
      }
   }
   else
   {
      if (dopple)
         mDoppleBits.set(teamID);
      else
         mDoppleBits.unset(teamID);
   }
}

//==============================================================================
//==============================================================================
void BObject::makeInvisible(BTeamID teamID)
{
   onDoppleBitsChanged(false, teamID);
   onVisibilityChanged(false, teamID);
   mPlayerVisibility.unset(teamID + 16);
   updateVisualVisibility();
}

//==============================================================================
//==============================================================================
void BObject::makeVisible(BTeamID teamID)
{
   onDoppleBitsChanged(false, teamID);
   onVisibilityChanged(true, teamID);
   mPlayerVisibility.set(teamID);
   mPlayerVisibility.set(teamID + 16);
   updateVisualVisibility();
}

//==============================================================================
//==============================================================================
void BObject::setTeamSelectionMask(BTeamID teamID, bool selectableByTeam)
{
   // do we support more than 8 teams? 
   BASSERT(teamID < 8);

   if (selectableByTeam)
      mTeamSelectionMask |= (1 << teamID);
   else
      mTeamSelectionMask &= ~(1 << teamID);
}

//==============================================================================
//==============================================================================
void BObject::fillTeamSelectionMask(bool selectableByAll)
{
   if (selectableByAll)
      mTeamSelectionMask = 0xFF;
   else
      mTeamSelectionMask = 0;
}

//==============================================================================
//==============================================================================
void BObject::makeSoftDopple(BTeamID teamID)
{
   if (getClassType() == cClassTypeUnit)
   {
      if (getFlagNotDoppleFriendly())
      {
         makeHardDopple(teamID);
      }
      else
      {
         onDoppleBitsChanged(true, teamID);
         onVisibilityChanged(true, teamID);
         mPlayerVisibility.unset(teamID + 16);
      }
   }
   else
   {
      onDoppleBitsChanged(false, teamID);
      onVisibilityChanged(true, teamID);
      mPlayerVisibility.set(teamID + 16);
   }
   updateVisualVisibility();
}

//==============================================================================
//==============================================================================
void BObject::makeHardDopple(BTeamID teamID)
{
   if (getClassType() == cClassTypeUnit)
   {
      onDoppleBitsChanged(false, teamID);
      onVisibilityChanged(false, teamID);
      mPlayerVisibility.unset(teamID + 16);
      createDopple(teamID);
   }
   else
   {
      onDoppleBitsChanged(false, teamID);
      onVisibilityChanged(true, teamID);
      mPlayerVisibility.set(teamID + 16);
   }
   updateVisualVisibility();
}

//==============================================================================
//==============================================================================
void BObject::onVisibilityChanged(bool visible, BTeamID teamID)
{
   // Bomb check.
   BTeam *pTeam = gWorld->getTeam(teamID);
   if (!pTeam)
      return;

   bool isObjectVisible = pTeam->isObjectVisible(mID);
   BEntityID parendSquadID = getParentID();
   BSquad* pParentSquad = gWorld->getSquad(parendSquadID);
   if (pParentSquad)
   {
      BASSERT(isClassType(BEntity::cClassTypeUnit));
      if (visible && !isObjectVisible)
      {
         // mrh/slb 11/14/07 - We don't set the bit here because we need to see if the squad was going from not-visible to visible.
         //bool visibilityBit = isVisible(teamID); 
         //if (!visibilityBit)
         //   mPlayerVisibility.set(teamID + 16);
         bool squadWasVisible = pParentSquad->isVisible(teamID);
         //if (!visibilityBit)
         //   mPlayerVisibility.unset(teamID + 16);

         pTeam->addVisibleObject(mID);
         if (!squadWasVisible)
         {
            BKB* pKB = gWorld->getKB(teamID);
            if (pKB)
            {
               pKB->updateSquad(pParentSquad);
               pKB->acquireVisToSquad(pParentSquad);
            }
         }

         if(gUserManager.getUser(BUserManager::cPrimaryUser)->getTeamID() == teamID && getProtoObject() && !getProtoObject()->getFlagSoundBehindFOW())
            startExistSound();

         // Play cached animation tags.
         gWorld->flushCachedAnimEvents(mID);
      }
      else if (!visible && isObjectVisible)
      {
         bool visibilityBit = isVisible(teamID);
         if (visibilityBit)
            mPlayerVisibility.unset(teamID + 16);
         bool squadWillBeVisible = pParentSquad->isVisible(teamID);
         if (visibilityBit)
            mPlayerVisibility.set(teamID + 16);

         pTeam->removeVisibleObject(mID);
         if (!squadWillBeVisible)
         {
            BKB* pKB = gWorld->getKB(teamID);
            if (pKB)
            {
               pKB->updateSquad(pParentSquad);
               pKB->loseVisToSquad(pParentSquad);
            }
         }
         
         //-- DJBFIXME: Removing stop exist when the unit is no longer visible so that engines don't cut in and out
         /*if(gUserManager.getUser(BUserManager::cPrimaryUser)->getTeamID() == teamID && getProtoObject() && !getProtoObject()->getFlagSoundBehindFOW())
            stopExistSound();*/
      }
   }
   else
   {
      if (visible && !isObjectVisible)
      {
         pTeam->addVisibleObject(mID);
         if(gUserManager.getUser(BUserManager::cPrimaryUser)->getTeamID() == teamID && getProtoObject() && !getProtoObject()->getFlagSoundBehindFOW())
            startExistSound();

         // Play cached animation tags.
         gWorld->flushCachedAnimEvents(mID);
      }
      else if (!visible && isObjectVisible)
      {
         pTeam->removeVisibleObject(mID);
         if(gUserManager.getUser(BUserManager::cPrimaryUser)->getTeamID() == teamID && getProtoObject() && !getProtoObject()->getFlagSoundBehindFOW())
            stopExistSound();
      }
   }

   if (mExplorationGroup != -1 && visible && !getFlagFirstUpdate())
      gWorld->makeExplorationGroupVisible(teamID, mExplorationGroup);
}

//==============================================================================
// BObject::startExistSound
//==============================================================================
void BObject::startExistSound(const BProtoObject* pProto)
{
   //-- Start the exist sound
   if(gConfig.isDefined(cConfigNoExistSound))
      return;

   if (isGarrisoned() && !isInCover())
      return;

   //-- Tell the parent Squad
   if(getClassType() != cClassTypeProjectile && getParentID() != cInvalidObjectID)
   {
      BSquad *pSquad = gWorld->getSquad(getParentID());
      if (pSquad)
         pSquad->startExistSound();
   }

   if(getFlagExistSoundPlaying())
      return;

   //-- Don't play exist sounds until the thing is built
   if(getFlagBuilt() == false && getClassType() == BEntity::cClassTypeUnit)
      return;

   //-- If a protoObject was not specified, then get it now.
   if(pProto == NULL)
      pProto = getProtoObject();

   if(pProto)
   {
      long boneHandle=-1;
      if(mpVisual)
      {
         const BSimString& boneName=pProto->getExistSoundBoneName();
         if(!boneName.isEmpty())
            boneHandle=mpVisual->getBoneHandle(boneName);
      }
      
      BCueIndex existSound=pProto->getSound(cObjectSoundExist);
      if(existSound != cInvalidCueIndex)
      {                           
         BRTPCInitArray rtpc;
         rtpc.add( BRTPCInit(AK::GAME_PARAMETERS::FOW_DISTANCE, 0.0f) );
         gWorld->getWorldSoundManager()->addSound(this, boneHandle, existSound, false, cInvalidCueIndex, false, false, &rtpc);           
      }

      //-- Set the exist sound playing flag whether there is an exist sound or not, to ensure the stop exist will get called.
      setFlagExistSoundPlaying(true);
   }
}

//==============================================================================
// BObject::stopExistSound
//==============================================================================
void BObject::stopExistSound(BEntityID parentID)
{  
   //-- Tell the parent Squad
   if(parentID == cInvalidObjectID)
      parentID = getParentID();

   if(getClassType() != cClassTypeProjectile && parentID != cInvalidObjectID)
   {
      BSquad *pSquad = gWorld->getSquad(parentID);
      if (pSquad)
         pSquad->stopExistSound();
   }

   if(getFlagExistSoundPlaying() == false)
      return;

   //-- Stop the exist sound if there is one
   const BProtoObject *pProto = getProtoObject();
   if(pProto)
   {      
      long stopCueIndex=pProto->getSound(cObjectSoundStopExist);
      if(stopCueIndex!=cInvalidCueIndex)
      {
         long boneHandle=-1;
         if(mpVisual)
         {
            const BSimString& boneName=pProto->getExistSoundBoneName();
            if(!boneName.isEmpty())
               boneHandle=mpVisual->getBoneHandle(boneName);
         }

         BRTPCInitArray rtpc;
         rtpc.add( BRTPCInit(AK::GAME_PARAMETERS::FOW_DISTANCE, 0.0f) );
         gWorld->getWorldSoundManager()->addSound(this, boneHandle, stopCueIndex, false, cInvalidCueIndex, false, false, &rtpc);         
      }      
      setFlagExistSoundPlaying(false);
   }   
}

//==============================================================================
// BObject::teleport
//==============================================================================
bool BObject::teleport( BVector location, long searchScale /*= 1*/ )
{
   BVector suggestion = cInvalidVector;
   DWORD   flags      = BWorld::cCPCheckObstructions | BWorld::cCPSetPlacementSuggestion | BWorld::cCPExpandHull | BWorld::cCPIgnoreMoving;
   if( gWorld->checkPlacement( getProtoID(), getPlayerID(), location, suggestion, getForward(), BWorld::cCPLOSDontCare, flags, searchScale ) )
   {
      #ifdef SYNC_Unit
         if (isClassType(BEntity::cClassTypeUnit))
            syncUnitData("BObject::teleport", suggestion);
      #endif
      setPosition( suggestion );
#ifdef SYNC_Visibility
      if (getClassType() != BEntity::cClassTypeObject)
      {
         syncVisibilityData( "BObject::teleport id", getID().asLong() );
      }
#endif
      setFlagLOSDirty(true );

      return( true );
   }

   return( false );   
}

//==============================================================================
//==============================================================================
void BObject::explode(int explodeType)
{
   //DJBFIXME: Total hack for rocket grunts. Data drive this!
   BObjectCreateParms parms;
   parms.mPlayerID = getPlayerID();
   parms.mPosition = getPosition();
   parms.mRight = getRight();
   parms.mForward = getForward();

   int protoID;
   if (explodeType == cExplodeTypeNormal)
      protoID = gDatabase.getProtoImpactEffectIndex("PowerExplosion");
   else if (explodeType == cExplodeTypeMethane)
      protoID = gDatabase.getProtoImpactEffectIndex("MethaneExplosion");
   else
   {
      BASSERT(0);
      return;
   }

   const BProtoImpactEffect* pData = gDatabase.getProtoImpactEffectFromIndex(protoID);
   if (pData)
   {
      BTerrainEffect* pTerrainEffect = gTerrainEffectManager.getTerrainEffect(pData->mTerrainEffectIndex, true);
      if (pTerrainEffect)
         pTerrainEffect->instantiateEffect(cInvalidSurfaceType, gDatabase.getImpactEffectSize("large"), getPosition(), getForward(), true, getPlayerID(), pData->mLifespan, getFlagVisibleToAll(), cVisualDisplayPriorityNormal, NULL);
   }

   //-- Play our impact sound 
   BCueIndex cueIndex = cInvalidCueIndex;
   const BProtoObject* pProto = getProtoObject();
   if(pProto)
      cueIndex = getProtoObject()->getSound(cObjectSoundImpactDeath);

   if(cueIndex != cInvalidCueIndex)
      gWorld->getWorldSoundManager()->addSound(mPosition, cueIndex, false, cInvalidCueIndex, false, true);
}

//==============================================================================
// Set the override flash duration ending time
//==============================================================================
void BObject::setOverrideFlashDuration(DWORD duration) 
{ 
   mOverrideFlashDuration = gWorld->getGametime() + duration; 
}

//==============================================================================
// Set the override flash interval and interval timer
//==============================================================================
void BObject::setOverrideFlashInterval(DWORD interval) 
{ 
   mOverrideFlashInterval = interval; 
   mOverrideFlashIntervalTimer = gWorld->getGametime() + interval; 
}

//==============================================================================
//==============================================================================
/*float BObject::damage(BDamage &dmg)
{
//-- For now, this method only exists to kill objects and then throw them
//-- I'm not tracking damage to units, im simply determining if a single blow is enough to kill it.
if(dmg.mDamage*dmg.mDamageMultiplier > getProtoObject()->getHitpoints())
{
// Throw a physics replacement (which destroys the original unit)
if (dmg.mpDamageInfo && dmg.mpDamageInfo->getThrowUnits())
{
createAndThrowPhysicsReplacement(dmg.mpDamageInfo, dmg.mDamagePos, dmg.mDistanceFactor);
return getProtoObject()->getHitpoints();
}
}

return 0.0f;
}*/

//=============================================================================
// BUnit::computeObstructionType
//=============================================================================
long BObject::computeObstructionType(void) 
{
   long obsType = BObstructionManager::cObsTypeUnknown;
   const BProtoObject* pProtoObject = getProtoObject();

   // DLM 6/5/08 - I need to find out how this ever worked in the old system.  
   // for now, in the new system, making flying units noncollidable through code
   // (something we like to call a hack) so units won't path around them. 
   #ifdef _MOVE4
   if (!isCollidable() || mFlagFlying)
   #else
   if (!isCollidable())
   #endif
      obsType = BObstructionManager::cObsTypeNonCollidableUnit;	
   else if ((pProtoObject != NULL) && (pProtoObject->getFlagObstructsAir()))
   {
      obsType = BObstructionManager::cObsTypeBlockAirMovement;
      mpObstructionNode->mProperties |=	BObstructionManager::cObsPropertyUpdatePatherQuadTree;
   }
   else if (pProtoObject && pProtoObject->getFlagBlockMovement())
   {
      obsType = BObstructionManager::cObsTypeCollidableNonMovableUnit;
      mpObstructionNode->mProperties |=	BObstructionManager::cObsPropertyUpdatePatherQuadTree;
   }   
   else if(isMobile() == false)
   {
      obsType = BObstructionManager::cObsTypeCollidableNonMovableUnit;
      mpObstructionNode->mProperties |=	BObstructionManager::cObsPropertyUpdatePatherQuadTree;
   }
   else if (!isMoving())
   {
      obsType = BObstructionManager::cObsTypeCollidableStationaryUnit;
      mpObstructionNode->mProperties |=	BObstructionManager::cObsPropertyMovableUnit;
   }
   else
   {
      obsType = BObstructionManager::cObsTypeCollidableMovingUnit;
      mpObstructionNode->mProperties |=	BObstructionManager::cObsPropertyMovableUnit;
   }

   return(obsType);
}

//=============================================================================
//=============================================================================
void BObject::setFlagBuilt(bool v)
{
   mFlagBuilt = v;

   BEntity::setFlagIsDoneBuilding(v);

   BEntity* pParent = getParent();
   if (pParent)
      pParent->setFlagIsDoneBuilding(getFlagIsDoneBuilding());
}

//=============================================================================
//=============================================================================
void BObject::setFlagNearLayer(bool v)
{
   mFlagNearLayer = v;
   if (mpVisual)
      mpVisual->updateNearLayer(v);
}

//=============================================================================
// Set the occluded flag for this object and all its attachments
//=============================================================================
void BObject::setFlagOccluded(bool v) 
{ 
   mFlagOccluded = v; 
   if(mpObjectAttachments)
   {
      uint numAttachments = mpObjectAttachments->getSize();
      for (uint i = 0; i < numAttachments; i++)
      {
         BObjectAttachment objectAttachment = mpObjectAttachments->get(i);         
         BObject* pAttachment = gWorld->getObject(objectAttachment.mAttachmentObjectID);
         if (pAttachment)
         {
            pAttachment->setFlagOccluded(v);
         }
      }
   }
}

//=============================================================================
// BUnit::hasAnimation
//=============================================================================
bool BObject::hasAnimation(long animType) const
{
   if (mpVisual)
   {
      return mpVisual->hasAnimation(animType);
   }

   return false;
}

//==============================================================================
//==============================================================================
bool BObject::isCapturable(BPlayerID playerID, BEntityID squadID) const
{
   //False if not capturable.
   if (!getProtoObject()->getFlagCapturable())
      return (false);   
   //False if not owned by Gaia (unless invulnerable)
   if (mPlayerID != 0 && !getProtoObject()->getFlagInvulnerable())
      return (false);

   //If it's being captured, we must be the one capturing it or else there must
   //be no one actually working on it right now.
   if (getFlagBeingCaptured())
   {
      //If we're capturing it, GTG.
      if (getCapturePlayerID() == playerID && squadID == cInvalidObjectID)
         return (true);

      //Else, no one can actively be working on it right now.
      if (mpEntityRefs)
      {
         for (uint i=0; i < mpEntityRefs->getSize(); i++)
         {
//-- FIXING PREFIX BUG ID 6070
            const BEntityRef* pEntityRef=&(*mpEntityRefs)[i];
//--
            if (pEntityRef && (pEntityRef->mType == BEntityRef::cTypeCapturingUnit))
            {
//-- FIXING PREFIX BUG ID 6066
               const BUnit* pUnit=gWorld->getUnit(pEntityRef->mID);
//--
               if (pUnit)
               {
                  if (pUnit->getPlayerID() != playerID)
                     return (false);
                  if (squadID != cInvalidObjectID)
                  {
//-- FIXING PREFIX BUG ID 6069
                     const BSquad* pSquad = pUnit->getParentSquad();
//--
                     if (pSquad && pSquad->getID() != squadID)
                        return (false);
                  }
               }
            }
         }
      }
   }

   //Else, we're GTG.
   return (true);
}

//==============================================================================
// BObject::createCorpseObstruction
//==============================================================================
void BObject::createCorpseObstruction()
{
   BASSERT(!mpObstructionNode);

   //-- create an obstruction
   mpObstructionNode = gObsManager.getNewObstructionNode();
   BASSERT(mpObstructionNode);

   gObsManager.resetObstructionNode(mpObstructionNode);
   setFlagCollidable(false);

   // Update Position
   gObsManager.fillOutRotatedPosition(mpObstructionNode, mPosition.x, mPosition.z, getVisualBoundingBox()->getExtents()[0], 
      getVisualBoundingBox()->getExtents()[2], mForward.x, mForward.z);

   // Fill out additional data   
   mpObstructionNode->mType = BObstructionManager::cObsNodeTypeCorpse;

   mpObstructionNode->mObject = this;			// This overwrites the first half of mEntity!!!
   mpObstructionNode->mEntityID = mID.asLong();

   // Put it into the quadtree
   gObsManager.installObjectObstruction(mpObstructionNode, BObstructionManager::cObsTypeNonCollidableUnit);
}

//==============================================================================
// BObject::unselect
//==============================================================================
void BObject::unselect(BTeamID teamID)
{
   BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
   if (pUser->getTeamID() == teamID)
   {
      BEntityID parendSquadID = getParentID();
//-- FIXING PREFIX BUG ID 6071
      const BSquad* pParentSquad = gWorld->getSquad(parendSquadID);
//--
      if (pParentSquad)
      {
         bool visibilityBit = isVisible(teamID);
         if (visibilityBit)
            mPlayerVisibility.unset(teamID + 16);
         bool squadWasVisible = pParentSquad->isVisible(teamID);
         if (visibilityBit)
            mPlayerVisibility.set(teamID + 16);

         if (!squadWasVisible)
         {
            BSelectionManager* pSelectionManager = pUser->getSelectionManager();
            if (pSelectionManager->isSquadSelected(parendSquadID))
               pSelectionManager->unselectSquad(parendSquadID);
         }
      }
      else
      {
         BSelectionManager* pSelectionManager = pUser->getSelectionManager();
         if (pSelectionManager->isUnitSelected(mID))
            pSelectionManager->unselectUnit(mID);
      }
   }
}

//==============================================================================
//==============================================================================
void BObject::updateGrannyInstanceSyncCallback(granny_model_instance* modelInstance, bool doSyncCheck)
{
   if (!modelInstance)
      return;
   granny_model_control_binding* pBinding = GrannyModelControlsBegin(modelInstance);
   for (;;)
   {
      if (!pBinding)
         return;
      granny_control* pControl = GrannyGetControlFromBinding(pBinding);
      if (pControl)
      {
         float clock = GrannyGetControlClampedLocalClock(pControl);

         long animIndex = -1;
         bool isBlendControl = false;
         bool isEasingOut = false;
         long animationTrack = -1;
         float weight = GrannyGetControlWeight(pControl);

         void** ppUserData = GrannyGetControlUserDataArray(pControl);
         if(ppUserData != NULL)
         {
            animIndex = (long)ppUserData[GrannyControlUserData_AnimIndex];
            isBlendControl = (bool)(ppUserData[GrannyControlUserData_IsBlendControl]!=NULL);
            isEasingOut = (bool)(ppUserData[GrannyControlUserData_EasingOut]!=NULL);
            animationTrack = (long)ppUserData[GrannyControlUserData_AnimationTrack];
         }

#ifdef SYNC_Anim
         if (doSyncCheck)
         {
            syncAnimData("BObject::updateGrannyInstanceSyncCallback animationTrack", animationTrack);
            syncAnimData("BObject::updateGrannyInstanceSyncCallback clock", clock);
            syncAnimData("BObject::updateGrannyInstanceSyncCallback weight", weight);
            syncAnimData("BObject::updateGrannyInstanceSyncCallback isBlendControl", isBlendControl);
            syncAnimData("BObject::updateGrannyInstanceSyncCallback isEasingOut", isEasingOut);
         }
#endif
      }

      if (pBinding == GrannyModelControlsEnd(modelInstance))
         break;

      pBinding = GrannyModelControlsNext(pBinding);
   }
}

//==============================================================================
//==============================================================================
void BObject::updateVisualItemSyncCallback(BVisualItem* pVisualItem, bool fullSync, bool doSyncCheck)
{
   #ifdef SYNC_Anim
   if (doSyncCheck)
   {
      syncAnimData("BObject::updateVisualItemSyncCallback mIndex", pVisualItem->mIndex);

      // SLB: Don't dereference mpName on the same frame that it gets reloaded.
      if (!BProtoVisual::mGenerationChanged && pVisualItem->mpName)
         syncAnimData("BObject::updateVisualItemSyncCallback mpName", pVisualItem->mpName->getPtr());

      syncAnimData("BObject::updateVisualItemSyncCallback asset type", pVisualItem->mModelAsset.mType);

      if (pVisualItem->mModelAsset.mType == cVisualAssetGrannyModel)
      {
//-- FIXING PREFIX BUG ID 6056
         const BGrannyModel* pModel = gGrannyManager.getModel(pVisualItem->mModelAsset.mIndex);
//--
         if (pModel)
         {
            syncAnimData("BObject::updateVisualItemSyncCallback model file", pModel->getFilename());
         }
      }            

      syncAnimData("BObject::updateVisualItemSyncCallback mMinCorner", pVisualItem->mMinCorner);
      syncAnimData("BObject::updateVisualItemSyncCallback mMaxCorner", pVisualItem->mMaxCorner);
      syncAnimData("BObject::updateVisualItemSyncCallback mCombinedMinCorner", pVisualItem->mCombinedMinCorner);
      syncAnimData("BObject::updateVisualItemSyncCallback mCombinedMaxCorner", pVisualItem->mCombinedMaxCorner);

      if (fullSync)
      {
         syncAnimData("BObject::updateVisualItemSyncCallback mFlags", *((long *) pVisualItem->mFlags.getRawValue()));

         for (int i=0; i<cNumAnimationTracks; i++)
         {
            syncAnimData("BObject::updateVisualItemSyncCallback anim track", i);
            syncAnimData("BObject::updateVisualItemSyncCallback anim asset type", pVisualItem->mAnimationTrack[i].mAnimAsset.mType);

            if (pVisualItem->mAnimationTrack[i].mAnimAsset.mType == cVisualAssetGrannyAnim)
            {
//-- FIXING PREFIX BUG ID 6057
               const BGrannyAnimation* pAnim = gGrannyManager.getAnimation(pVisualItem->mAnimationTrack[i].mAnimAsset.mIndex, false);
//--
               if (pAnim)
               {
                  syncAnimData("BObject::updateVisualItemSyncCallback anim file", pAnim->getFilename());
               }
            }

            syncAnimData("BObject::updateVisualItemSyncCallback anim position", pVisualItem->mAnimationTrack[i].mPosition);
            syncAnimData("BObject::updateVisualItemSyncCallback anim duration", pVisualItem->mAnimationTrack[i].mDuration);
            syncAnimData("BObject::updateVisualItemSyncCallback anim isClone", pVisualItem->mAnimationTrack[i].mIsClone);
            syncAnimData("BObject::updateVisualItemSyncCallback anim isLocked", pVisualItem->mAnimationTrack[i].mIsLocked);
         }
      }
   }
   #endif
}

//==============================================================================
//==============================================================================
void BObject::setCenterOffset(const BVector& offset)
{
   mCenterOffset = offset;

   if (mCenterOffset != XMVectorZero())
      mFlagUseCenterOffset = true;
   else
      mFlagUseCenterOffset = false;
}

//==============================================================================
//==============================================================================
void BObject::clearCenterOffset()
{
   mCenterOffset = XMVectorZero();
   mFlagUseCenterOffset = false;
}

//==============================================================================
//==============================================================================
void BObject::updateSimBoundingBox()
{
   // Update sim BB
   BVector simTranslation;
   BVector newBoundingBoxPosition = mPosition;
   if (getProtoObject()->getMovementType() != cMovementTypeAir)
      newBoundingBoxPosition.y += mObstructionRadiusY;

   // SimBoundingBox was not being aligned with the unit's orientation. Now it is.
   BVector maxPoint(mObstructionRadiusX, mObstructionRadiusY, mObstructionRadiusZ);
   BVector minPoint(-mObstructionRadiusX, -mObstructionRadiusY, -mObstructionRadiusZ);
   BMatrix xfrm;
   xfrm.makeOrient(mForward, mUp, mRight);
   xfrm.setTranslation(newBoundingBoxPosition);
   mSimBoundingBox.initializeTransformed(minPoint, maxPoint, xfrm);
}

//==============================================================================
//==============================================================================
bool BObject::save(BStream* pStream, int saveType) const
{
   if (!BEntity::save(pStream, saveType))
      return false;

   //BBoundingBox mBoundingBox;
   GFWRITEVECTOR(pStream, mCenterOffset);
   GFWRITEVECTOR(pStream, mIconColorSize);
   GFWRITEPTR(pStream, sizeof(BVec2)*cMaxUVOffsets, mUVOffsets);
   GFWRITEVAR(pStream, uint, mMultiframeTextureIndex);
   GFWRITEVAR(pStream, int, mVisualVariationIndex);
   if (!gVisualManager.saveVisual(pStream, saveType, mpVisual))
      return false;
   GFWRITEVAR(pStream, float, mAnimationRate);
   GFWRITEVAR(pStream, float, mRadius);
   GFWRITEVAR(pStream, float, mMoveAnimationPosition);
   GFWRITEVAR(pStream, float, mHighlightIntensity);
   GFWRITEVAR(pStream, DWORD, mSubUpdateNumber);
   GFWRITEBITVECTOR(pStream, mPlayerVisibility);
   GFWRITEBITVECTOR(pStream, mDoppleBits);
   GFWRITEVAR(pStream, long, mSimX);
   GFWRITEVAR(pStream, long, mSimZ);
   GFWRITEVAR(pStream, float, mLOSScalar);
   GFWRITEVAR(pStream, long, mLastSimLOS);
   GFWRITEVAL(pStream, bool, (mpObjectAttachments != NULL));
   if (mpObjectAttachments)
      GFWRITECLASSARRAY(pStream, saveType, *mpObjectAttachments, uint8, 250);
   GFWRITEVAL(pStream, bool, (mpAdditionalTextures != NULL));
   if (mpAdditionalTextures)
      GFWRITECLASSARRAY(pStream, saveType, *mpAdditionalTextures, uint8, 250);
   GFWRITECLASSARRAY(pStream, saveType, mHardpointState, uint8, 100);
   GFWRITECLASS(pStream, saveType, mAnimationState);
   GFWRITEVAR(pStream, DWORD, mAnimationLockEnds);
   //long mLOSX;
   //long mLOSZ;
   //long mLOSTeamID;
   GFWRITEVAR(pStream, long, mProtoID);
   GFWRITEVAR(pStream, BPlayerID, mColorPlayerID);
   GFWRITEVAR(pStream, DWORD, mOverrideTint);
   GFWRITEVAR(pStream, DWORD, mOverrideFlashInterval);
   GFWRITEVAR(pStream, DWORD, mOverrideFlashIntervalTimer);
   GFWRITEVAR(pStream, DWORD, mOverrideFlashDuration);
   GFWRITEVAR(pStream, DWORD, mLifespanExpiration);
   GFWRITEVAR(pStream, float, mCurrentAlphaTime);
   GFWRITEVAR(pStream, float, mAlphaFadeDuration);
   GFWRITEVAR(pStream, float, mSelectionPulseTime);
   GFWRITEVAR(pStream, float, mSelectionPulsePercent);
   GFWRITEVAR(pStream, float, mSelectionFlashTime);
   GFWRITEVAR(pStream, float, mSelectionPulseSpeed);
   GFWRITEVAR(pStream, float, mLastRealtime);
   GFWRITEVAR(pStream, uchar, mAOTintValue);
   //GFWRITEVAR(pStream, int16, mExplorationGroup);
   GFWRITEVAR(pStream, uchar, mTeamSelectionMask);
   GFWRITEVAR(pStream,float, mLOSRevealTime);

   GFWRITEBITBOOL(pStream, mFlagVisibility);
   GFWRITEBITBOOL(pStream, mFlagLOS);
   GFWRITEBITBOOL(pStream, mFlagHasLifespan);
   GFWRITEBITBOOL(pStream, mFlagDopples);
   GFWRITEBITBOOL(pStream, mFlagIsFading);
   GFWRITEBITBOOL(pStream, mFlagAnimationDisabled);
   GFWRITEBITBOOL(pStream, mFlagIsRevealer);
   GFWRITEBITBOOL(pStream, mFlagDontInterpolate);
   GFWRITEBITBOOL(pStream, mFlagBlockLOS);
   GFWRITEBITBOOL(pStream, mFlagCloaked);
   GFWRITEBITBOOL(pStream, mFlagCloakDetected);
   GFWRITEBITBOOL(pStream, mFlagGrayMapDopples);
   GFWRITEBITBOOL(pStream, mFlagLOSMarked);
   GFWRITEBITBOOL(pStream, mFlagUseLOSScalar);
   GFWRITEBITBOOL(pStream, mFlagLOSDirty);
   GFWRITEBITBOOL(pStream, mFlagAnimationLocked);
   GFWRITEBITBOOL(pStream, mFlagUpdateSquadPositionOnAnimationUnlock);
   GFWRITEBITBOOL(pStream, mFlagExistSoundPlaying);
   GFWRITEBITBOOL(pStream, mFlagNoUpdate);
   GFWRITEBITBOOL(pStream, mFlagSensorLockTagged);
   GFWRITEBITBOOL(pStream, mFlagNoReveal);
   GFWRITEBITBOOL(pStream, mFlagBuilt);
   GFWRITEBITBOOL(pStream, mFlagBeingCaptured);
   GFWRITEBITBOOL(pStream, mFlagInvulnerable);
   GFWRITEBITBOOL(pStream, mFlagVisibleForOwnerOnly);
   GFWRITEBITBOOL(pStream, mFlagVisibleToAll);
   GFWRITEBITBOOL(pStream, mFlagNearLayer);
   GFWRITEBITBOOL(pStream, mFlagIKDisabled);
   GFWRITEBITBOOL(pStream, mFlagHasTrackMask);
   GFWRITEBITBOOL(pStream, mFlagLODFading);
   GFWRITEBITBOOL(pStream, mFlagOccluded);
   GFWRITEBITBOOL(pStream, mFlagFadeOnDeath);
   GFWRITEBITBOOL(pStream, mFlagObscurable);
   GFWRITEBITBOOL(pStream, mFlagNoRender);
   GFWRITEBITBOOL(pStream, mFlagTurning);
   GFWRITEBITBOOL(pStream, mFlagAppearsBelowDecals);
   GFWRITEBITBOOL(pStream, mFlagSkipMotionExtraction);
   GFWRITEBITBOOL(pStream, mFlagOverrideTint);
   GFWRITEBITBOOL(pStream, mFlagMotionCollisionChecked);
   GFWRITEBITBOOL(pStream, mFlagIsDopple);
   GFWRITEBITBOOL(pStream, mFlagIsImpactEffect);
   GFWRITEBITBOOL(pStream, mFlagDebugRenderAreaAttackRange);
   GFWRITEBITBOOL(pStream, mFlagDontLockMovementAnimation);
   GFWRITEBITBOOL(pStream, mFlagRemainVisible);
   GFWRITEBITBOOL(pStream, mFlagVisibleForTeamOnly);
   GFWRITEBITBOOL(pStream, mFlagDontAutoAttackMe);
   GFWRITEBITBOOL(pStream, mFlagAlwaysAttackReviveUnits);
   GFWRITEBITBOOL(pStream, mFlagNoRenderForOwner);
   GFWRITEBITBOOL(pStream, mFlagNoRenderDuringCinematic);
   GFWRITEBITBOOL(pStream, mFlagUseCenterOffset);
   GFWRITEBITBOOL(pStream, mFlagNotDoppleFriendly);
   GFWRITEBITBOOL(pStream, mFlagForceVisibilityUpdateNextFrame);
   GFWRITEBITBOOL(pStream, mFlagTurningRight);
   GFWRITEBITBOOL(pStream, mFlagIsUnderCinematicControl);   
   GFWRITEBITBOOL(pStream, mFlagNoWorldUpdate);   

   GFWRITEVAL(pStream, bool, (mpObstructionNode != NULL));

   GFWRITEVAL(pStream, int8, (mpPhysicsObject ? mpPhysicsObject->getType() : -1));
   if (mpPhysicsObject)
      GFWRITECLASSPTR(pStream, saveType, mpPhysicsObject);

   GFWRITEMARKER(pStream, cSaveMarkerObject1);

   return true;
}

//==============================================================================
//==============================================================================
bool BObject::load(BStream* pStream, int saveType)
{
   if (!BEntity::load(pStream, saveType))
      return false;

   //BBoundingBox mBoundingBox;
   GFREADVECTOR(pStream, mCenterOffset);
   GFREADVECTOR(pStream, mIconColorSize);
   GFREADPTR(pStream, sizeof(BVec2)*cMaxUVOffsets, mUVOffsets);
   GFREADVAR(pStream, uint, mMultiframeTextureIndex);
   GFREADVAR(pStream, int, mVisualVariationIndex);

   if (mGameFileVersion >= 7)
   {
      BMatrix worldMatrix;
      getWorldMatrix(worldMatrix);
      DWORD playerColor = gWorld->getPlayerColor(mPlayerID, BWorld::cPlayerColorContextObjects);
      if (!gVisualManager.loadVisual(pStream, saveType, &mpVisual, worldMatrix, playerColor))
         return false;
   }

   GFREADVAR(pStream, float, mAnimationRate);
   GFREADVAR(pStream, float, mRadius);
   GFREADVAR(pStream, float, mMoveAnimationPosition);
   GFREADVAR(pStream, float, mHighlightIntensity);
   GFREADVAR(pStream, DWORD, mSubUpdateNumber);
   GFREADBITVECTOR(pStream, mPlayerVisibility);
   GFREADBITVECTOR(pStream, mDoppleBits);
   GFREADVAR(pStream, long, mSimX);
   GFREADVAR(pStream, long, mSimZ);
   GFREADVAR(pStream, float, mLOSScalar);
   GFREADVAR(pStream, long, mLastSimLOS);

   // mpObjectAttachments;
   bool objectAttachments;
   GFREADVAR(pStream, bool, objectAttachments);
   if (objectAttachments)
   {
      mpObjectAttachments=new BObjectAttachmentArray();
      GFREADCLASSARRAY(pStream, saveType, *mpObjectAttachments, uint8, 250);
   }

   // mpAdditionalTextures;
   bool additionalTextures;
   GFREADVAR(pStream, bool, additionalTextures);
   if (additionalTextures)
   {
      mpAdditionalTextures=new BAdditionalTextureArray();
      GFREADCLASSARRAY(pStream, saveType, *mpAdditionalTextures, uint8, 250);
   }

   GFREADCLASSARRAY(pStream, saveType, mHardpointState, uint8, 100);
   GFREADCLASS(pStream, saveType, mAnimationState);
   GFREADVAR(pStream, DWORD, mAnimationLockEnds);
   //long mLOSX;
   //long mLOSZ;
   //long mLOSTeamID;
   GFREADVAR(pStream, long, mProtoID);
   gSaveGame.remapProtoObjectID(mProtoID);
   GFREADVAR(pStream, BPlayerID, mColorPlayerID);
   GFREADVAR(pStream, DWORD, mOverrideTint);
   GFREADVAR(pStream, DWORD, mOverrideFlashInterval);
   GFREADVAR(pStream, DWORD, mOverrideFlashIntervalTimer);
   GFREADVAR(pStream, DWORD, mOverrideFlashDuration);
   GFREADVAR(pStream, DWORD, mLifespanExpiration);
   GFREADVAR(pStream, float, mCurrentAlphaTime);
   GFREADVAR(pStream, float, mAlphaFadeDuration);
   GFREADVAR(pStream, float, mSelectionPulseTime);
   GFREADVAR(pStream, float, mSelectionPulsePercent);
   GFREADVAR(pStream, float, mSelectionFlashTime);
   GFREADVAR(pStream, float, mSelectionPulseSpeed);
   GFREADVAR(pStream, float, mLastRealtime);
   GFREADVAR(pStream, uchar, mAOTintValue);
   //GFREADVAR(pStream, int16, mExplorationGroup);
   GFREADVAR(pStream, uchar, mTeamSelectionMask);
   if (mGameFileVersion >= 6)
      GFREADVAR(pStream, float, mLOSRevealTime);

   GFREADBITBOOL(pStream, mFlagVisibility);
   GFREADBITBOOL(pStream, mFlagLOS);
   GFREADBITBOOL(pStream, mFlagHasLifespan);
   GFREADBITBOOL(pStream, mFlagDopples);
   GFREADBITBOOL(pStream, mFlagIsFading);
   GFREADBITBOOL(pStream, mFlagAnimationDisabled);
   GFREADBITBOOL(pStream, mFlagIsRevealer);
   GFREADBITBOOL(pStream, mFlagDontInterpolate);
   GFREADBITBOOL(pStream, mFlagBlockLOS);
   GFREADBITBOOL(pStream, mFlagCloaked);
   GFREADBITBOOL(pStream, mFlagCloakDetected);
   GFREADBITBOOL(pStream, mFlagGrayMapDopples);
   GFREADBITBOOL(pStream, mFlagLOSMarked);
   GFREADBITBOOL(pStream, mFlagUseLOSScalar);
   GFREADBITBOOL(pStream, mFlagLOSDirty);
   GFREADBITBOOL(pStream, mFlagAnimationLocked);
   GFREADBITBOOL(pStream, mFlagUpdateSquadPositionOnAnimationUnlock);
   GFREADBITBOOL(pStream, mFlagExistSoundPlaying);
   GFREADBITBOOL(pStream, mFlagNoUpdate);
   GFREADBITBOOL(pStream, mFlagSensorLockTagged);
   GFREADBITBOOL(pStream, mFlagNoReveal);
   GFREADBITBOOL(pStream, mFlagBuilt);
   GFREADBITBOOL(pStream, mFlagBeingCaptured);
   GFREADBITBOOL(pStream, mFlagInvulnerable);
   GFREADBITBOOL(pStream, mFlagVisibleForOwnerOnly);
   GFREADBITBOOL(pStream, mFlagVisibleToAll);
   GFREADBITBOOL(pStream, mFlagNearLayer);
   GFREADBITBOOL(pStream, mFlagIKDisabled);
   GFREADBITBOOL(pStream, mFlagHasTrackMask);
   GFREADBITBOOL(pStream, mFlagLODFading);
   GFREADBITBOOL(pStream, mFlagOccluded);
   GFREADBITBOOL(pStream, mFlagFadeOnDeath);
   GFREADBITBOOL(pStream, mFlagObscurable);
   GFREADBITBOOL(pStream, mFlagNoRender);
   GFREADBITBOOL(pStream, mFlagTurning);
   GFREADBITBOOL(pStream, mFlagAppearsBelowDecals);
   GFREADBITBOOL(pStream, mFlagSkipMotionExtraction);
   GFREADBITBOOL(pStream, mFlagOverrideTint);
   GFREADBITBOOL(pStream, mFlagMotionCollisionChecked);
   GFREADBITBOOL(pStream, mFlagIsDopple);
   GFREADBITBOOL(pStream, mFlagIsImpactEffect);
   GFREADBITBOOL(pStream, mFlagDebugRenderAreaAttackRange);
   GFREADBITBOOL(pStream, mFlagDontLockMovementAnimation);
   GFREADBITBOOL(pStream, mFlagRemainVisible);
   GFREADBITBOOL(pStream, mFlagVisibleForTeamOnly);
   GFREADBITBOOL(pStream, mFlagDontAutoAttackMe);
   GFREADBITBOOL(pStream, mFlagAlwaysAttackReviveUnits);
   GFREADBITBOOL(pStream, mFlagNoRenderForOwner);
   GFREADBITBOOL(pStream, mFlagNoRenderDuringCinematic);
   GFREADBITBOOL(pStream, mFlagUseCenterOffset);
   GFREADBITBOOL(pStream, mFlagNotDoppleFriendly);
   GFREADBITBOOL(pStream, mFlagForceVisibilityUpdateNextFrame);
   GFREADBITBOOL(pStream, mFlagTurningRight);
   if (mGameFileVersion >= 4)
      GFREADBITBOOL(pStream, mFlagIsUnderCinematicControl);
   if (mGameFileVersion >= 5)
      GFREADBITBOOL(pStream, mFlagNoWorldUpdate);

   bool bObstruction;
   GFREADVAR(pStream, bool, bObstruction);

   bool haveMask = false;
   BBitArray meshRenderMask;
   if (mGameFileVersion < 7)
   {
      GFREADVAR(pStream, bool, haveMask);
      if (haveMask)
      {
         long maskCount;
         GFREADVAL(pStream, uint8, long, maskCount);
         meshRenderMask.setNumber(maskCount, false);
         meshRenderMask.clear();
         GFREADBITARRAY(pStream, meshRenderMask, uint8, 255);
      }
   }

   // mpPhysicsObject
   int8 physicsObjectType;
   GFREADVAR(pStream, int8, physicsObjectType);
   if (physicsObjectType != -1)
   {
      switch (physicsObjectType)
      {
         case BPhysicsObject::cSimple: mpPhysicsObject = new BPhysicsObject(gWorld->getPhysicsWorld()); break;
         case BPhysicsObject::cClamshell: mpPhysicsObject = new BClamshellPhysicsObject(gWorld->getPhysicsWorld()); break;
      }
      if (!mpPhysicsObject)
         return false;
      GFREADCLASSPTR(pStream, saveType, mpPhysicsObject);
   }

   GFREADMARKER(pStream, cSaveMarkerObject1);

   // Init un-saved data
   BPlayer* pPlayer = getPlayer();
   if (!pPlayer)
      return false;

   BProtoObject* pProtoObject = pPlayer->getProtoObject(mProtoID);
   if (!pProtoObject)
      return true;
   
   setProtoObject(pProtoObject);

   //if (gEnableSubUpdating)
   {
      getWorldMatrix(mOldWorldMatrix);
      mNewWorldMatrix = mOldWorldMatrix;
   }

   #ifndef BUILD_FINAL
      mEntityName=const_cast<BSimString*>(&(pProtoObject->getName()));
   #endif

   if (bObstruction)
      createObstruction(pProtoObject->getFlagPlayerOwnsObstruction());

   if (mGameFileVersion < 7)
      updateSimBoundingBox();

   if (mFlagLOSMarked)
   {
      if (mFlagIsRevealer && mLastSimLOS == -1)
         gVisibleMap.exploreEntireMap(getTeamID());
      else
         gVisibleMap.exploreCircularRegion(mSimX, mSimZ, mLastSimLOS, getTeamID());
   }

   if (mGameFileVersion < 7)
   {
      setVisual(pProtoObject->getProtoVisualIndex(), pProtoObject->getVisualDisplayPriority());

      if (haveMask && mpVisual && mpVisual->mpInstance && mpVisual->mModelAsset.mType == cVisualAssetGrannyModel)
         ((BGrannyInstance*)mpVisual->mpInstance)->setMeshRenderMask(meshRenderMask);
   }

   if (pProtoObject->getTactic() && !pProtoObject->getTactic()->getAnimInfoLoaded())
      loadTacticAnimInfo(pProtoObject);

   if (mFlagExistSoundPlaying)
   {
      mFlagExistSoundPlaying = false;
      startExistSound();
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BObject::postLoad(int saveType)
{
   if (!BEntity::postLoad(saveType))
      return false;

   if (mpVisual)
   {
      BMatrix worldMatrix;
      getWorldMatrix(worldMatrix);
      DWORD playerColor = gWorld->getPlayerColor(mPlayerID, BWorld::cPlayerColorContextObjects);
      if (!mpVisual->postLoad(saveType, worldMatrix, playerColor))
         return false;
   }

   if (getClassType() == BEntity::cClassTypeObject)
   {
      if (mpPhysicsObject && mpPhysicsObject->getLoadedKeyframed())
         mpPhysicsObject->setKeyframed(true);

      if (mFlagNoUpdate)
         gWorld->getObjectManager()->setObjectNoUpdate(mID, true);
   }

   if (mGameFileVersion >= 7)
   {
      if (mFlagNoUpdate)
         updateBoundingBox();
   }
   return true;
}

//==============================================================================
//==============================================================================
bool BAdditionalTexture::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, long, RenderType);
   GFWRITEVAR(pStream, long, Texture);
   GFWRITEVAR(pStream, BVec2, TexUVOfs);
   GFWRITEVAR(pStream, float, TexUVScale);
   GFWRITEVAR(pStream, float, TexInten);
   GFWRITEVAR(pStream, float, TexScrollSpeed);
   GFWRITEVAL(pStream, float, (TexTimeout - gWorld->getGametimeFloat()));
   GFWRITEBITBOOL(pStream, ModulateOffset);
   GFWRITEBITBOOL(pStream, ModulateIntensity);
   GFWRITEBITBOOL(pStream, ShouldBeCopied);
   GFWRITEBITBOOL(pStream, TexClamp);
   GFWRITEBITBOOL(pStream, TexScrollLoop);
   return true;
}

//==============================================================================
//==============================================================================
bool BAdditionalTexture::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, long, RenderType);
   GFREADVAR(pStream, long, Texture);
   GFREADVAR(pStream, BVec2, TexUVOfs);
   GFREADVAR(pStream, float, TexUVScale);
   GFREADVAR(pStream, float, TexInten);
   if (BObject::mGameFileVersion > 1)
   {
      GFREADVAR(pStream, float, TexScrollSpeed);
      float timeoutOffset = 0.0f;
      GFREADVAL(pStream, float, float, timeoutOffset);
      TexTimeout = (float) gWorld->getGametimeFloat() + timeoutOffset;
   }
   if (BObject::mGameFileVersion > 2)
   {
      GFREADBITBOOL(pStream, ModulateOffset);
      GFREADBITBOOL(pStream, ModulateIntensity);
      GFREADBITBOOL(pStream, ShouldBeCopied);
      GFREADBITBOOL(pStream, TexClamp);
      GFREADBITBOOL(pStream, TexScrollLoop);
   }
   TexStartTime = 0;
   return true;
}

//==============================================================================
//==============================================================================
bool BObjectAttachment::save(BStream* pStream, int saveType) const
{
   BVector vec;
   mOffset.getForward(vec);
   GFWRITEVECTOR(pStream, vec);

   mOffset.getRight(vec);
   GFWRITEVECTOR(pStream, vec);

   mOffset.getUp(vec);
   GFWRITEVECTOR(pStream, vec);

   mOffset.getTranslation(vec);
   GFWRITEVECTOR(pStream, vec);

   GFWRITEVAR(pStream, BEntityID, mAttachmentObjectID);
   GFWRITEVAR(pStream, long, mToBoneHandle);
   GFWRITEVAR(pStream, long, mFromBoneHandle);
   GFWRITEBITBOOL(pStream, mIsUnitAttachment);
   GFWRITEBITBOOL(pStream, mUseOffset);
   return true;
}

//==============================================================================
//==============================================================================
bool BObjectAttachment::load(BStream* pStream, int saveType)
{
   BVector forward, up, right, pos;
   GFREADVECTOR(pStream, forward);
   GFREADVECTOR(pStream, right);
   GFREADVECTOR(pStream, up);
   GFREADVECTOR(pStream, pos);
   mOffset.makeOrient(forward, up, right);
   mOffset.setTranslation(pos);

   GFREADVAR(pStream, BEntityID, mAttachmentObjectID);
   GFREADVAR(pStream, long, mToBoneHandle);
   GFREADVAR(pStream, long, mFromBoneHandle);
   GFREADBITBOOL(pStream, mIsUnitAttachment);
   GFREADBITBOOL(pStream, mUseOffset);
   return true;
}

//==============================================================================
//==============================================================================
bool BHardpointState::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, long, mOwnerAction);
   GFWRITEVAR(pStream, float, mAutoCenteringTimer);
   GFWRITEVAR(pStream, float, mYawSoundActivationTimer);
   GFWRITEVAR(pStream, float, mPitchSoundActivationTimer);
   GFWRITEVAR(pStream, BUnitOppID, mOppID);
   GFWRITEBITBOOL(pStream, mAllowAutoCentering);
   GFWRITEBITBOOL(pStream, mYawSound);
   GFWRITEBITBOOL(pStream, mYawSoundPlaying);
   GFWRITEBITBOOL(pStream, mPitchSound);
   GFWRITEBITBOOL(pStream, mPitchSoundPlaying);
   GFWRITEBITBOOL(pStream, mSecondaryTurretScanToken);
   return true;
}

//==============================================================================
//==============================================================================
bool BHardpointState::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, long, mOwnerAction);
   GFREADVAR(pStream, float, mAutoCenteringTimer);
   GFREADVAR(pStream, float, mYawSoundActivationTimer);
   GFREADVAR(pStream, float, mPitchSoundActivationTimer);
   GFREADVAR(pStream, BUnitOppID, mOppID);
   GFREADBITBOOL(pStream, mAllowAutoCentering);
   GFREADBITBOOL(pStream, mYawSound);
   GFREADBITBOOL(pStream, mYawSoundPlaying);
   GFREADBITBOOL(pStream, mPitchSound);
   GFREADBITBOOL(pStream, mPitchSoundPlaying);
   if(BObject::mGameFileVersion >= 8)
      GFREADBITBOOL(pStream, mSecondaryTurretScanToken);
   return true;
}

//==============================================================================
//==============================================================================
void BObject::updateGrannySync(bool doSyncCheck)
{
   if (mpVisual)
   {
      if (getClassType() != BEntity::cClassTypeObject)
      {
         //syncAnimData("BObject::updateGrannySync mID", mID.asLong());
      }
      mpVisual->updateGrannySync(doSyncCheck);
   }
}

//==============================================================================
//==============================================================================
#ifndef BUILD_FINAL
void BObject::debugDrawHardpoints() 
{
   const BProtoObject *pProto = getProtoObject();
   if(pProto)
   {
      for(long i=0; i < getNumberHardpoints(); i++)
      {
         const BHardpoint* pHP = pProto->getHardpoint(i);
         if(pHP)
         {
            //-- get the base location and direction of the hardpoint yaw bone in model and hardpoint space
            BVector msBonePos;
            BMatrix msBoneMatrix;
            BMatrix transformedBoneMatrix;
            if (!getHardpointYawLocation(i, msBonePos, msBoneMatrix, &transformedBoneMatrix))
               continue;

            //-- get model to world transform
            BMatrix modelToWorldMatrix;
            getWorldMatrix(modelToWorldMatrix);           

            //-- Get the hardpont pos in world space
            BVector hardpointWorldPos;
            modelToWorldMatrix.transformVectorAsPoint(msBonePos, hardpointWorldPos);
            gpDebugPrimitives->addDebugSphere(hardpointWorldPos, 2.0f, cDWORDRed);            

            //-- get transformed bone direction
            BVector transformedBoneDir;
            msBoneMatrix.getForward(transformedBoneDir);            
            transformedBoneDir.normalize();

            transformedBoneMatrix.transformVector(transformedBoneDir, transformedBoneDir);

            modelToWorldMatrix.transformVector(transformedBoneDir, transformedBoneDir);
            transformedBoneDir.normalize();

            BVector point2 = transformedBoneDir;
            point2.scale(20);
            point2 += hardpointWorldPos;

            gpDebugPrimitives->addDebugThickLine(hardpointWorldPos, point2, 1.0f, cDWORDGreen, cDWORDGreen);
         }
      }
   }   
}
#endif

//==============================================================================
//==============================================================================
void BObject::setFlagNoUpdate(bool v)
{ 
   mFlagNoUpdate=v;

   if (getClassType() == BEntity::cClassTypeObject)
      gWorld->getObjectManager()->setObjectNoUpdate(mID, v);
}

//==============================================================================
//==============================================================================
BAdditionalTexture*  BObject::addAdditionalTexture(const BAdditionalTexture& texture)
{
   if(!mpAdditionalTextures)
   {
      mpAdditionalTextures=new BAdditionalTextureArray();
      if(!mpAdditionalTextures)
         return NULL;
   }
   
   BASSERT(texture.RenderType != cATMax);
   BASSERT(texture.Texture != BMeshEffectTextures::cTTMax);

   // for now, just overwrite if we have another of the same type
   for (uint i = 0; i < mpAdditionalTextures->getSize(); ++i)
   {
      BAdditionalTexture& foundTexture = mpAdditionalTextures->get(i);
      if (foundTexture.RenderType == texture.RenderType)
      {
         // don't overwrite data if the incoming texture has a shorter timeout than the existing one
         if (foundTexture.TexTimeout <= texture.TexTimeout)
            foundTexture = texture;
         return &foundTexture;
      }
   }

   uint index = mpAdditionalTextures->add(texture);
   return &mpAdditionalTextures->get(index);
}
//==============================================================================
//==============================================================================
BAdditionalTexture* BObject::getAdditionalTexture(BAdditionalTextureRenderType type)
{
   if (!mpAdditionalTextures)
      return NULL;

   for (uint i = 0; i < mpAdditionalTextures->getSize(); ++i)
   {
      BAdditionalTexture& foundTexture = mpAdditionalTextures->get(i);
      if (foundTexture.RenderType == type)
         return &foundTexture;
   }

   return NULL;
}

//==============================================================================
//==============================================================================
void BObject::removeAdditionalTexture(BAdditionalTextureRenderType type)
{
   if (!mpAdditionalTextures)
      return;

   for (uint i = 0; i < mpAdditionalTextures->getSize(); ++i)
   {
      BAdditionalTexture& foundTexture = mpAdditionalTextures->get(i);
      if (foundTexture.RenderType == type)
      {
         mpAdditionalTextures->removeIndex(i);
         return;
      }  
   }
}

//==============================================================================
//==============================================================================
void BObject::copyAdditionalTextures(const BObject* pSourceObject)
{
   if (!pSourceObject || !pSourceObject->mpAdditionalTextures)
      return;

   for (uint i = 0; i < pSourceObject->mpAdditionalTextures->getSize(); ++i)
   {
      BAdditionalTexture& additionalTexture = pSourceObject->mpAdditionalTextures->get(i);
      if (additionalTexture.ShouldBeCopied)
         addAdditionalTexture(additionalTexture);
   }
}

//==============================================================================
//==============================================================================
void BObject::setTargettingSelection(bool on, float scale, float uOffset, float vOffset, float timeout, float speed, bool clamp, bool loop, float intensity, DWORD color)
{
   if (on)
   {
      setOverrideTintColor(color);

      BAdditionalTexture texture;
      texture.RenderType = cATAdditive;
      texture.Texture = BMeshEffectTextures::cTTSelection;
      texture.TexUVScale = scale;
      texture.TexUVOfs.set(uOffset, vOffset);
      texture.TexInten = intensity;
      texture.TexScrollSpeed = speed;
      texture.ModulateOffset = true;
      texture.TexClamp = clamp;
      texture.TexScrollLoop = loop;
      texture.TexStartTime = gWorld->getGametime();
      if (timeout > 0.0f)
         texture.TexTimeout = gWorld->getGametimeFloat() + timeout;
      else
         texture.TexTimeout = -1.0f;
      addAdditionalTexture(texture);
   }
   else
   {
      BAdditionalTexture* pSelectionTexture = getAdditionalTexture(cATAdditive);
      if (pSelectionTexture && pSelectionTexture->Texture == BMeshEffectTextures::cTTSelection)
         removeAdditionalTexture(cATAdditive);
   }
}

//==============================================================================
//==============================================================================
void BObject::setFlagNoRender(bool v)
{
   mFlagNoRender = v;

   // Possibly carry forward the mFlagNoRender value to this object's children.
   const BProtoObject* pProtoObject = getProtoObject();
   if (pProtoObject->getFlagCarryNoRenderToChildren())
   {
      uint numRefs = getNumberEntityRefs();
      for (uint j=0; j<numRefs; j++)
      {
         BEntityRef* pObjectRef = getEntityRefByIndex(j);
         if (pObjectRef->mType== BEntityRef::cTypeAssociatedObject)
         {
            BObject* pObject = gWorld->getObject(pObjectRef->mID);
            if (pObject)
               pObject->setFlagNoRender(v);
         }
      }
   }
}

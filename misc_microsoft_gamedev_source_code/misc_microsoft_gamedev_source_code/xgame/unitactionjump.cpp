//==============================================================================
// unitActionJump.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "unitactionJump.h"
#include "unit.h"
#include "physics.h"
#include "world.h"
#include "tactic.h"
#include "actionmanager.h"
#include "visual.h"
#include "pather.h"
#include "unitquery.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionJump, 10, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionJump::init()
{
   if (!BAction::init())
      return(false);

   mOppID = cInvalidActionID;
   mTarget.reset();
   mpParentAction = NULL;
   mXYDist = 0.0f;
   mParam = 0.0f;
   mThrowerProtoActionID = cInvalidActionID;
   mFlagOrient = true;
   mTargetHeight = 0.0f;

   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionJump::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   // Connect.
   if (!BAction::connect(pOwner, pOrder))
   {
      if (mpOwner && mpOwner->getUnit())
         mpOwner->getUnit()->completeOpp(mOppID, true);

      return (false);
   }

   // Get the controllers
   if (!grabControllers())
   {
      BAction::disconnect();
      return false;
   }

   BASSERT(mpOwner);
   BASSERT(mTarget.isPositionValid());

   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);

   // Calculate target offset based on our position within the squad
   BSquad* pSquad = pUnit->getParentSquad();

   BVector ofs(0.0f);
   if (pSquad)
   {
      ofs = pSquad->getFormationPosition(pUnit->getID()) - pSquad->getPosition();
   }

   mTarget.setPosition(mTarget.getPosition() + ofs);

   // Make sure jump target is in the world bounds
   if (gWorld->isOutsidePlayableBounds(mTarget.getPosition(), true))
   {
      BVector pos = mTarget.getPosition();

      if (pos.x < gWorld->getSimBoundsMinX()) pos.x = gWorld->getSimBoundsMinX() + 1.0f;
	   if (pos.z < gWorld->getSimBoundsMinZ()) pos.z = gWorld->getSimBoundsMinZ() + 1.0f;
	   if (pos.x > gWorld->getSimBoundsMaxX()) pos.x = gWorld->getSimBoundsMaxX() - 1.0f;
	   if (pos.z > gWorld->getSimBoundsMaxZ()) pos.z = gWorld->getSimBoundsMaxZ() - 1.0f;

      mTarget.setPosition(pos);
   }

   BASSERT(mTarget.isPositionValid());

   BVector tpos = mTarget.getPosition();
   BVector pos = pUnit->getPosition();

   if (!gTerrainSimRep.getHeight(tpos.x, tpos.z, mTargetHeight, true))
   {
      pUnit->completeOpp(mOppID, true);
      return false;
   }

   mXYDist = tpos.xzDistance(pos);

   BVector midpoint = (tpos + pos) / 2;

   // Max fly height

   // For pull adjust the midpoint to be slightly closer to the start position so it looks like it rises first
   // and heads directly towards the puller.
   if (mJumpType == BUnitOpp::cTypeJumpPull)
   {
      midpoint.x = 0.75f * pos.x + 0.25f * tpos.x;
      midpoint.z = 0.75f * pos.z + 0.25f * tpos.z;
      midpoint.y = max(tpos.y, pos.y) + mXYDist / 4.0f;
   }
   else
   {
      // @CJSTODO: Make this just slightly more than the highest point on our trajectory
      midpoint.y = max(tpos.y, pos.y) + mXYDist / 2.0f;
   }

   mCurve.init(pos, midpoint, tpos);
   mParam = 0.0f;

   // Play our jump anim
   if (mpProtoAction)
   {
      long animType = mpProtoAction->getAnimType();

      if (animType != -1)
         pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateWork, animType, true, false);

      pUnit->setFlagNotAttackable(true);
      pUnit->setFlagAllowStickyCam(false);
   }
   else if (mThrowerProtoActionID != cInvalidActionID)
   {
      BUnit* pThrowerUnit = gWorld->getUnit(mTarget.getID());
      if (pThrowerUnit)
         mpProtoAction = pThrowerUnit->getTactic()->getProtoAction(mThrowerProtoActionID);
      else
         return (false); // Time to bail - we should have a Brute (pThrowerUnit) in this block

      BASSERT(mpProtoAction); // This should be valid now.  If it's not, something is setup wrong in the attacker's action

      if (!mpProtoAction)
         return false;
      long animType = mpProtoAction->getEndAnimType();

      if (animType != -1)
         pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateWork, animType, true, false);
   }

   // Disable tie to ground and physics so we can move it manually
   pUnit->setTieToGround(false);
   pUnit->setPhysicsKeyFramed(true);

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionJump::disconnect()
{
   BASSERT(mpOwner);

   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);

   if (mJumpType != BUnitOpp::cTypeJumpPull)
   {
      long count = pUnit->getVisual()->getNumberAttachments();
      for (long i = 0; i < count; ++i)
      {
         BVisualItem* pAttach = pUnit->getVisual()->getAttachmentByIndex(i);
         removeParticles(pAttach);
      }
   }

   pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle, pUnit->getIdleAnim());

   pUnit->setFlagNotAttackable(false);
   pUnit->setFlagAllowStickyCam(true);

   //Release our controllers.
   releaseControllers();

   if (mpParentAction)
         mpParentAction->notify(BEntity::cEventActionDone, mpOwner->getID(), mOppID, 0);

   // Re-enable tie to ground and physics
   pUnit->setTieToGround(!pUnit->getProtoObject()->getFlagNoTieToGround());
   pUnit->tieToGround();
   pUnit->setPhysicsKeyFramed(false);

   pUnit->completeOpp(mOppID, true);

   // If we wound up on an obstruction unit (hello lightbridge on Terminal), kill us and make us fall
   BUnitQuery query(pUnit->getPosition(), pUnit->getObstructionRadius(), true);
   BEntityIDArray obstructionUnits(0, 64);
   query.getFlagIgnoreDead();
   gWorld->getUnitsInArea(&query, &obstructionUnits);

   uint numUnits = obstructionUnits.getSize();
   for (uint i=0; i<numUnits; i++)
   {
      const BUnit* pObs = gWorld->getUnit(obstructionUnits[i]);
      if (pObs && pObs->isType(gDatabase.getPOIDObstruction()))
      {
         BObject* pPhysReplacement = pUnit->createPhysicsReplacement();
         if (!pPhysReplacement || !pPhysReplacement->getPhysicsObject())
            continue;
         pUnit->kill(true);
         pPhysReplacement->setAnimationState(BObjectAnimationState::cAnimationStateMisc, cAnimTypeFlail, true, true);
         pPhysReplacement->computeAnimation();
      }
   }

   return (BAction::disconnect());
}

//==============================================================================
//==============================================================================
void BUnitActionJump::removeParticles(BVisualItem* pAttach)
{
   if (!pAttach)
      return;

   if (pAttach && pAttach->mModelAsset.mType == cVisualAssetParticleSystem)
      pAttach->setFlag(BVisualItem::cFlagImmediateRemove, true);

   long count = pAttach->getNumberAttachments();
   for (long i = 0; i < count; ++i)
   {
      BVisualItem* pSubAttach = pAttach->getAttachmentByIndex(i);
      removeParticles(pSubAttach);
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionJump::update(float elapsed)
{
   BASSERT(mpOwner);
   BASSERT(mTarget.isPositionValid());

   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);

   bool done = false;

   mParam += elapsed * (mpProtoAction->getVelocityScalar() / mXYDist);

   if (mParam >= 1.0f)
   {
      mParam = 1.0f;
      done = true;
   }

   BVector pos = mCurve.evaluate(mParam);

   #ifdef SYNC_Unit
      syncUnitData("BUnitActionJump::update", pos);
   #endif
   pUnit->setPosition(pos);
   pUnit->getParentSquad()->setPosition(pos);

   if (pos.y <= mTargetHeight)
   {
      pUnit->tieToGround();
   }

   if (mFlagOrient)
   {
      BVector forward = mTarget.getPosition() - pos;
      forward.y = 0;
      forward.normalize();


      if (forward.lengthSquared() > 0)
      {
         pUnit->setForward(forward);
         pUnit->calcRight();
         pUnit->calcUp();
      }
   }

   // See if we're finished
   if (done)
   {
      setState(cStateDone);

      pUnit->tieToGround();
      return true;
   }

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionJump::validateControllers() const
{
   // Return true if we have the controllers we need, false if not.
//-- FIXING PREFIX BUG ID 1649
   const BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//--
   BASSERT(pUnit);

   bool valid = (pUnit->getController(BActionController::cControllerOrient)->getActionID() == mID);
   if (!valid)
      return (false);

   return (valid);
}

//==============================================================================
//==============================================================================
bool BUnitActionJump::grabControllers()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // Try to grab the orient controller.  
   bool grabbed = pUnit->grabController(BActionController::cControllerOrient, this, getOppID());
   if (!grabbed)
   {
      return (false);
   }

   // Try to grab the animation controller (if we don't grab it, no worries)
   pUnit->grabController(BActionController::cControllerAnimation, this, getOppID());

   return (grabbed);
}

//==============================================================================
//==============================================================================
void BUnitActionJump::releaseControllers()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);

   // Release the orientation controller
   pUnit->releaseController(BActionController::cControllerOrient, this);   

   // Release the animation controller
   pUnit->releaseController(BActionController::cControllerAnimation, this);
}


//==============================================================================
//==============================================================================
bool BUnitActionJump::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;

   GFWRITEVAR(pStream, BUnitOppID, mOppID);
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEACTIONPTR(pStream, mpParentAction);
   GFWRITEVAR(pStream, float, mXYDist);
   GFWRITEVAR(pStream, float, mParam);
   GFWRITEVAR(pStream, long, mThrowerProtoActionID);
   GFWRITECLASS(pStream, saveType, mCurve);
   GFWRITEVAR(pStream, float, mTargetHeight);
   GFWRITEBITBOOL(pStream, mFlagOrient);
   GFWRITEVAR(pStream, BJumpOrderType, mJumpType);

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionJump::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   GFREADVAR(pStream, BUnitOppID, mOppID);
   GFREADCLASS(pStream, saveType, mTarget);
   GFREADACTIONPTR(pStream, mpParentAction);
   GFREADVAR(pStream, float, mXYDist);
   GFREADVAR(pStream, float, mParam);
   GFREADVAR(pStream, long, mThrowerProtoActionID);
   GFREADCLASS(pStream, saveType, mCurve);
   GFREADVAR(pStream, float, mTargetHeight);
   GFREADBITBOOL(pStream, mFlagOrient);

   if (BAction::mGameFileVersion >= 21)
   {
      GFREADVAR(pStream, BJumpOrderType, mJumpType);
   }

   return true;
}

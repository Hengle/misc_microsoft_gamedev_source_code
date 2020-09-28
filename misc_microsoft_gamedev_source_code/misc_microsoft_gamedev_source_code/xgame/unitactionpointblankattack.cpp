//==============================================================================
// unitactionpointblankattack.cpp
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#include "common.h"
#include "database.h"
#include "unit.h"
#include "unitactionPointBlankAttack.h"
#include "squad.h"
#include "objectmanager.h"
#include "world.h"
#include "commands.h"
#include "squadplotter.h"
#include "unitquery.h"
#include "SimOrderManager.h"
#include "tactic.h"
#include "physics.h"
#include "physicsobject.h"
#include "simhelper.h"
#include "visual.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionPointBlankAttack, 4, &gSimHeap);

const static float cFlareHeight = 3.0f;
const static float cFlareProjection = 10.0f;

//==============================================================================
//==============================================================================
bool BUnitActionPointBlankAttack::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   BDEBUG_ASSERT(mpProtoAction);

   //Connect.
   if (!BAction::connect(pOwner, pOrder))
      return false;

   mFlareAnimOppID = BUnitOpp::cInvalidID;
   mProjectileInstigator = cInvalidObjectID;

   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionPointBlankAttack::disconnect()
{
   mFlareAnimOppID = BUnitOpp::cInvalidID;
   mProjectileInstigator = cInvalidObjectID;

   return (BAction::disconnect());
}

//==============================================================================
//==============================================================================
bool BUnitActionPointBlankAttack::init()
{
   if (!BAction::init())
      return false;

   mFlares.clear();
   mFlareCooldownDoneTime = 0;

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionPointBlankAttack::update(float elapsed)
{
   if (mpProtoAction->getFlagDisabled())
      return true;

   // Remove dead flares from our list
   if (mFlares.size() > 0)
   {
      if (gWorld->getGametime() >= mFlareCooldownDoneTime)
      {
         mFlareCooldownDoneTime = 0;
      }

      long count = mFlares.getNumber();
      for (long i = count - 1; i >= 0; i--)
      {
         if (!gWorld->getProjectile(mFlares[i]))
            mFlares.erase(i);
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionPointBlankAttack::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   if (mpProtoAction->getFlagDisabled())
      return;

   if (!validateTag(eventType, senderID, data, data2))
      return;

   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
   BDEBUG_ASSERT(mpProtoAction);

   switch (eventType)
   {
      case BEntity::cEventAnimAttackTag:
      {
         const BHardpoint* pHP = pUnit->getHardpoint(mpProtoAction->getHardpointID());
         if (pHP && (pHP->getYawAttachmentHandle() != -1))
         {
            if (pHP->getYawAttachmentHandle() != (long)data)
               break;
            if ((pHP->getPitchAttachmentHandle() == -1) || (pHP->getPitchAttachmentHandle() != (long)data))
               break;

            BPlayerID playerID = pUnit->getPlayerID();
            BProtoObjectID protoObject = mpProtoAction->getProjectileID();
            BVector launchLoc = pUnit->getPosition();

            BPlayer* pPlayer = gWorld->getPlayer(playerID);
//-- FIXING PREFIX BUG ID 1826
            const BProtoObject* pProjProtoObject = (pPlayer ? pPlayer->getProtoObject(protoObject) : gDatabase.getGenericProtoObject(protoObject));
//--
            if (!pProjProtoObject)
               return;

            // Create the projectile
            BObjectCreateParms parms;
            parms.mPlayerID = playerID;
            parms.mProtoObjectID = mpProtoAction->getProjectileID();

            getLaunchPosition(pUnit, data, (long)data2, &parms.mPosition, &parms.mForward, &parms.mRight);

            BVector targetLoc = parms.mPosition + parms.mForward * cFlareProjection + cYAxisVector * cFlareHeight;

            float dps = mpProtoAction->getDamagePerSecond();
            BEntityID projID = gWorld->launchProjectile(parms, targetLoc, XMVectorZero(), XMVectorZero(), dps, mpProtoAction, (IDamageInfo*)mpProtoAction);
            mFlares.add(projID);

            // set the projectile instigator's target object id to this flare if it's currently pointing at us
            BProjectile* pInstigator = gWorld->getProjectile(mProjectileInstigator);
            if (pInstigator && pInstigator->getTargetObjectID() == pUnit->getID())
               pInstigator->setTargetObjectID(projID);
         }
      }
      break;

      case BEntity::cEventAttachmentAnimLoop:
      case BEntity::cEventAttachmentAnimEnd:
      case BEntity::cEventAttachmentAnimChain:
      case BEntity::cEventAnimLoop:
      case BEntity::cEventAnimEnd:
      case BEntity::cEventAnimChain:
      {
         const BHardpoint* pHP = pUnit->getHardpoint(mpProtoAction->getHardpointID());

         if (pHP && (pHP->getYawAttachmentHandle() != -1))
         {
            // Release hardpoint
            const BHardpoint* pHP = pUnit->getHardpoint(mpProtoAction->getHardpointID());
            if (pHP)
            {
               pUnit->resetAttachmentAnim(pHP->getYawAttachmentHandle());
            }
         }
      }
      break;

      case BEntity::cEventOppComplete:
      {
         if (data == mFlareAnimOppID)
         {
            mFlareAnimOppID = BUnitOpp::cInvalidID;
            mFlareCooldownDoneTime = gWorld->getGametime() + getProtoAction()->getDodgeCooldown();
            mProjectileInstigator = cInvalidObjectID;
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionPointBlankAttack::attemptLaunchFlares(BProjectile* pInstigator)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BDEBUG_ASSERT(pUnit);
   BDEBUG_ASSERT(mpProtoAction);
   BSquad *pSquad = pUnit->getParentSquad();
   BDEBUG_ASSERT(pSquad);
   
   BDEBUG_ASSERT(pInstigator);
   if (!pInstigator)
      return;

   // Already have flares out, bail
   if (mFlares.size() > 0)
      return;

   // Cooling down from last flare
   if (gWorld->getGametime() < mFlareCooldownDoneTime)
      return;

   // Don't attempt evade if moving
   if (pSquad->isSquadAPhysicsVehicle())
   {
      // Physics vehicles may always be moving at unit level so check squad instead
      if (pSquad->isMoving())
         return;
   }
   else
   {
      if (pUnit->isMoving())
      return;
   }

   // Random chance fail
   float chance = mpProtoAction->getDodgeChanceMax();
   float rand = getRandRangeFloat(cSimRand, 0.0f, 1.0f);
   if (rand > chance)
      return;

   int animType = mpProtoAction ? mpProtoAction->getAnimType() : -1;
   BASSERT(animType != -1); // If this triggers, there is no valid anim for the attack

   const BHardpoint* pHP = pUnit->getHardpoint(mpProtoAction->getHardpointID());
   BVisual* pVisual = pUnit->getVisual();

   if (pHP && pVisual)
   {
      BVisualItem* pAttachment = pVisual->getAttachment(pHP->getYawAttachmentHandle());
      if (pAttachment && !(pAttachment->getAnimationLock(cActionAnimationTrack) || pAttachment->getAnimationLock(cMovementAnimationTrack)))
      {        
         // pick an anim
         BVector directionToProjectile = pInstigator->getPosition() - pUnit->getPosition();
         directionToProjectile.normalize();
         float angleBetween = directionToProjectile.angleBetweenVector(pUnit->getRight());
         uint chosenAnim = cAnimTypeEvadeRight;
         if (angleBetween < cPiOver2)
            chosenAnim = cAnimTypeEvadeLeft;

         BUnitOpp* pNewOpp=BUnitOpp::getInstance();
         pNewOpp->init();
         pNewOpp->setSource(pUnit->getID());
         pNewOpp->setNotifySource(true);
         pNewOpp->setUserData(static_cast<uint16>(chosenAnim));
         pNewOpp->setType(BUnitOpp::cTypeEvade);
         pNewOpp->setPreserveDPS(true);
         pNewOpp->setPriority(BUnitOpp::cPriorityCritical);
         pNewOpp->generateID();
         if (!pUnit->addOpp(pNewOpp))
         {
            BUnitOpp::releaseInstance(pNewOpp);
            return;
         }
         mFlareAnimOppID = pNewOpp->getID();
      }
   }
   
   // Set next available dodge time
   mFlareCooldownDoneTime = gWorld->getGametime() + getProtoAction()->getDodgeCooldown();
   mProjectileInstigator = pInstigator->getID();
}

//==============================================================================
//==============================================================================
BEntityID BUnitActionPointBlankAttack::getFlare()
{
   if (mFlares.size() > 0)
   {
      BEntityID id = mFlares.back();
      mFlares.erase(mFlares.size() - 1);
      return id;
   }
   return cInvalidObjectID;
}

//==============================================================================
//==============================================================================
void BUnitActionPointBlankAttack::getLaunchPosition(const BUnit* pUnit, long attachmentHandle, long boneHandle, BVector* position, BVector* forward, BVector* right) const
{
   BVisual* pVisual = pUnit->getVisual();
   if (pVisual)
   {
      BMatrix attachmentMat;
      BVisualItem* pVisualItem = pVisual->getAttachment(attachmentHandle, &attachmentMat);
      if (pVisualItem)
      {
         BMatrix unitMat;
         pUnit->getWorldMatrix(unitMat);

         BMatrix mat;
         mat.mult(attachmentMat, unitMat);

         BMatrix boneMat;
         if (pVisualItem->getBone(boneHandle, position, &boneMat, NULL, &mat))
         {
            if (forward)
            {
               boneMat.getForward(*forward);
               forward->normalize();
            }
            if (right)
            {
               boneMat.getRight(*right);
               right->normalize();
            }

            return;
         }
      }
   }

   if (position)
      *position = pUnit->getSimCenter();
   if (forward)
      *forward = pUnit->getForward();
   if (right)
      *right = pUnit->getRight();
}

//==============================================================================
//==============================================================================
bool BUnitActionPointBlankAttack::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   if (BAction::mGameFileVersion >= 7)
   {
      GFREADARRAY(pStream, BEntityID, mFlares, uint8, 20);
      GFREADVAR(pStream, DWORD, mFlareCooldownDoneTime);
   }
   if (BAction::mGameFileVersion >= 29)
   {
      GFREADVAR(pStream, BUnitOppID, mFlareAnimOppID);
      GFREADVAR(pStream, BEntityID, mProjectileInstigator);
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionPointBlankAttack::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;

   GFWRITEARRAY(pStream, BEntityID, mFlares, uint8, 20);
   GFWRITEVAR(pStream, DWORD, mFlareCooldownDoneTime);
   GFWRITEVAR(pStream, BUnitOppID, mFlareAnimOppID);
   GFWRITEVAR(pStream, BEntityID, mProjectileInstigator);

   return true;
}

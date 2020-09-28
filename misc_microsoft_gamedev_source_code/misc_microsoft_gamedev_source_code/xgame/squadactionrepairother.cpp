//==============================================================================
// squadactionrepair.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "civ.h"
#include "entity.h"
#include "protoobject.h"
#include "protosquad.h"
#include "selectionmanager.h"
#include "squad.h"
#include "squadactionrepairother.h"
#include "squadactionmove.h"
#include "unit.h"
#include "user.h"
#include "usermanager.h"
#include "world.h"
#include "game.h"
#include "tactic.h"
#include "unitactionunderattack.h"
#include "unitactionavoidcollisionair.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionRepairOther, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionRepairOther::init(void)
{
   if (!BAction::init())
      return(false);

   mUnitOppID = BUnitOpp::cInvalidID;
   mFXAttachment.invalidate();
   mTargetSquad.invalidate();
   mBeamHeadID.invalidate();
   mBeamTailID.invalidate();
   mIsMoving = false;
   mBoneHandle = cInvalidObjectID;

   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionRepairOther::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   BASSERT(mpProtoAction);

   if (!BAction::connect(pOwner, pOrder))
      return(false);

   // [4/14/2008 xemu] based loosely off of BSquadActionRepair's version of this function, but using the target data out of the order
   //    instead of the local squad.

   if (!pOrder->getTarget().isIDValid())
      return false;
   BEntityID targetID = pOrder->getTarget().getID();
   BEntity *pEntity = gWorld->getEntity(targetID);
   if (!pEntity)
      return false;

//-- FIXING PREFIX BUG ID 1735
   //const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BSquad* pSquad = NULL;
   if (pEntity->getClassType() == BEntity::cClassTypeSquad)
      pSquad = pEntity->getSquad();
   else if (pEntity->getClassType() == BEntity::cClassTypeUnit)
      pSquad = pEntity->getUnit()->getParentSquad();
   if (pSquad == NULL)
      return(false);

   // [4/18/2008 xemu] repair OTHER my friend 
   if (pOwner->getID() == pSquad->getID())
      return(false);

   // If the target unit is at full health, bail
   if (pSquad->getHPPercentage() == 1.0f)
      return false;

   // If a crashing aircraft, bail
   if (pSquad && pSquad->getLeaderUnit())
   {
      BUnitActionAvoidCollisionAir* pUnitAvoidAction = reinterpret_cast<BUnitActionAvoidCollisionAir*>(pSquad->getLeaderUnit()->getActionByType(BAction::cActionTypeUnitAvoidCollisionAir));
      if (pUnitAvoidAction && pUnitAvoidAction->Crashing())
         return false;
   }

   mTargetSquad = pSquad->getID();
   mTarget.setID(mTargetSquad);

   //Figure our range.  This will end up setting the range into mTarget.
   BSquad* pOwnerSquad = reinterpret_cast<BSquad*>(mpOwner);
   BASSERT(pOwnerSquad);
   if (!mTarget.isRangeValid())
      pOwnerSquad->calculateRange(&mTarget, NULL);

   mAttackingTeamIDs.clear();

   // Cache off our bone handle
   mBoneHandle = cInvalidObjectID;

   if (pOwnerSquad && mpProtoAction)
   {
      BUnit* pUnit = pOwnerSquad->getLeaderUnit();
      if (pUnit && !mpProtoAction->getBoneName().isEmpty())
      {
         BVisual* pVis = pUnit->getVisual();
         if (pVis)
         {
            mBoneHandle = pVis->getBoneHandle(mpProtoAction->getBoneName());
         }
      }
   }

   //addUnitOpp();

   setState(cStateWorking);
   //gWorld->notify(BEntity::cEventRepair, pOwner->getID(), 0, 0);
   return(true);
}

//==============================================================================
//==============================================================================
void BSquadActionRepairOther::disconnect()
{

   removeUnitOpp();

   destroyEffect();

   //remove any remaining revealers
   BSquad *pOwnerSquad = getOwner()->getSquad();
   BASSERT(pOwnerSquad);
   BUnit *pRepairingUnit = pOwnerSquad->getLeaderUnit();
   if(pRepairingUnit)
   {
      for( int i=0; i<mAttackingTeamIDs.getNumber(); i++ )
      {
         pRepairingUnit->removeReveal(mAttackingTeamIDs.get(i));
      }
   }

   /*
   BSquad* pSquad = gWorld->getSquad(mTargetSquad);
   BASSERT(pSquad != NULL);
   if (pSquad != NULL)
   {
      pSquad->setFlagNonMobile(mSaveFlagNonMobile);
      pSquad->setFlagIsRepairing(false);
   }
   */
   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionRepairOther::update(float elapsed)
{
   BASSERT(mpProtoAction);

   BSquad* pSquad = gWorld->getSquad(mTargetSquad); 
   if (pSquad == NULL || mpProtoAction == NULL)
   {
      setState(cStateDone);
      return(BAction::update(elapsed));
   }

   bool healing = false;
   bool inRange = false;
   //float dist = pSquad->getPosition().distanceSqr(getOwner()->getPosition());
   float dist = pSquad->calculateXZDistance(getOwner());
   float maxRange = mpProtoAction->getMaxRange(pSquad->getLeaderUnit());
   if (dist < maxRange)
      inRange = true;

   switch (mState)
   {
   case cStateMoving:
      {
         if (inRange)
            setState(cStateWorking);
      }
      break;
   case cStateWorking:
      {
         if (!inRange)
         {
            BSquad *pOwnerSquad = getOwner()->getSquad();
            //pOwnerSquad->doMove(NULL, this, &mTarget, true, false, false, false);
            /*BActionID moveAction = */
            // DLM 10/22/08 - This action needs to use a platoon move if it's in a platoon.
            bool platoonMove = false;
            BEntityID parentID = pOwnerSquad->getParentID();
            if (mpOrder && (parentID != cInvalidObjectID) && (mpOrder->getOwnerID() == parentID))
               platoonMove = true;

            if (pOwnerSquad->doMove(mpOrder, this, &mTarget, platoonMove) == cInvalidActionID)
            {
               removeUnitOpp();
               setState(cStateFailed);
            }
            else
            {
               if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityUser))
               {
                  BSquadActionMove *pAction = (BSquadActionMove *)pOwnerSquad->getActionByType(cActionTypeSquadMove); // mActions.getActionByID(moveAction);
                  if (pAction)
                     pAction->setFlagForceLeashUpdate(true);
               }
               removeUnitOpp();
               mIsMoving = true;

               if( mFXAttachment.isValid() )
               {
                  destroyEffect();
               }

               setState(cStateMoving);
            }
         }
         else if (!mIsMoving)
         {
            if (mUnitOppID == BUnitOpp::cInvalidID)
            {
               BSquad *pOwnerSquad = getOwner()->getSquad();

               // Stop us from moving so we can grab the movement/orientation controllers
               pOwnerSquad->stop();
               addUnitOpp();
            }
            if (!mFXAttachment.isValid())
            {
               BObject *pSquadLeader = gWorld->getObject(pSquad->getLeader());
               if (pSquadLeader != NULL)
               {
                  createEffect(pSquadLeader);
               }
            }

            float healAmt = mpProtoAction->getWorkRate() * elapsed;
            float excessCV = 0.0f;
            pSquad->repairCombatValue(healAmt, mpProtoAction->getAllowReinforce(), excessCV);
            healing = true;
            if (excessCV > 0.0f)
            {
               setState(cStateDone);
               destroyEffect();
            }
            if (!pSquad->isAlive() || pSquad->getHPPercentage() == 0.0f)
            {
               setState(cStateDone);
               destroyEffect();
            }
            // If it's an aircraft and crashing, stop healing
            if (pSquad && pSquad->getLeaderUnit())
            {
               BUnitActionAvoidCollisionAir* pUnitAvoidAction = reinterpret_cast<BUnitActionAvoidCollisionAir*>(pSquad->getLeaderUnit()->getActionByType(BAction::cActionTypeUnitAvoidCollisionAir));
               if (pUnitAvoidAction && pUnitAvoidAction->Crashing())
               {
                  setState(cStateDone);
                  destroyEffect();
               }
            }
            // If it's on another team, stop healing (this can happen if it's jacked by a Spartan)
            if (pSquad->getTeamID() != mpOwner->getTeamID())
            {
               setState(cStateDone);
               destroyEffect();
            }
         }

         break;
      }
   }

   updateEffect();

   //Add or remove revealer
   BSquad *pOwnerSquad = getOwner()->getSquad();
   BASSERT(pOwnerSquad);
   BUnit *pRepairingUnit = pOwnerSquad->getLeaderUnit();
   BASSERT(pRepairingUnit);

   //copy currentreveal array to previous
   BDynamicSimArray<BTeamID>     previousAttackingTeamIDs;
   previousAttackingTeamIDs = mAttackingTeamIDs;
   //clear current.
   mAttackingTeamIDs.clear();

   if(healing)
   {
      BUnit *pTargetLeader = pSquad->getLeaderUnit();
      if(pTargetLeader)
      {
//-- FIXING PREFIX BUG ID 1737
         const BUnitActionUnderAttack* pUnitActionUnderAttack = reinterpret_cast<BUnitActionUnderAttack*>(pTargetLeader->getActionByType(BAction::cActionTypeUnitUnderAttack));
//--
         if( pUnitActionUnderAttack )
         {
            uint numAttackingUnits = pUnitActionUnderAttack->getNumberAttackingUnits();
            for (uint j = 0; j < numAttackingUnits; j++)
            {
//-- FIXING PREFIX BUG ID 1736
               const BUnit* pAttackingUnit = gWorld->getUnit(pUnitActionUnderAttack->getAttackingUnitByIndex(j));
//--
               if (pAttackingUnit)
               {
                  BTeamID tid = pAttackingUnit->getTeamID();

                  //if in previous array, do remove from previous array, 
                  // else only do the reveal if we haven't seen this team on this update
                  if( previousAttackingTeamIDs.contains(tid) )
                     previousAttackingTeamIDs.removeValue(tid);
                  else if (pRepairingUnit && !mAttackingTeamIDs.contains(tid))
                     pRepairingUnit->addReveal(tid);

                  //add to current array.
                  mAttackingTeamIDs.uniqueAdd(tid);
               }
            }
         }
      }
   }
   //for all left in previous array, remove the reveal
   if (pRepairingUnit)
   {
      for( int i=0; i<previousAttackingTeamIDs.getNumber(); i++ )
      {
         pRepairingUnit->removeReveal(previousAttackingTeamIDs.get(i));
      }
   }

   if(!BAction::update(elapsed))
      return false;

   return true;
}


//==============================================================================
// Give opp to our units.
//==============================================================================
bool BSquadActionRepairOther::addUnitOpp()
{
//-- FIXING PREFIX BUG ID 1738
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   //BSquad *pSquad = gWorld->getSquad(mTargetSquad);

   BUnitOpp opp;
   opp.init();
   opp.setType(BUnitOpp::cTypeHeal);
   opp.setSource(pSquad->getID());
   opp.setUserData(cAnimTypeHeal);
   opp.setTarget(mTarget);
   opp.generateID();
   if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityTrigger))
      opp.setTrigger(true);
   else if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityUser))
      opp.setPriority(BUnitOpp::cPriorityCommand);
   else
      opp.setPriority(BUnitOpp::cPrioritySquad);

   if (pSquad->addOppToChildren(opp))
   {      
      mUnitOppID = opp.getID();      
      return (true);
   }

   return (false);
}

//==============================================================================
// Remove opp that we've given our units.
//==============================================================================
void BSquadActionRepairOther::removeUnitOpp()
{
//-- FIXING PREFIX BUG ID 1739
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--

   if (mUnitOppID == BUnitOpp::cInvalidID)
      return;

   pSquad->removeOppFromChildren(mUnitOppID);
   mUnitOppID = BUnitOpp::cInvalidID;
}

//==============================================================================
//==============================================================================
void BSquadActionRepairOther::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
   // [4/18/2008 xemu] not entirely sure how this system is set up so that you can get a notify even with no owner, but it's happening... 
   if (mpOwner == NULL)
      return;

   //BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);

   switch (eventType)
   {
      case BEntity::cEventActionFailed:
         mIsMoving = false;
         setState(cStateWorking);
         //setState(cStateDone);
      break;

      case BEntity::cEventActionDone:
         mIsMoving = false;
         //mFutureState = cStateWorking;
         setState(cStateWorking);
      break;
   }
}

//==============================================================================
//==============================================================================
bool BSquadActionRepairOther::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;

   GFWRITEVAR(pStream, BEntityID, mTargetSquad);
   GFWRITEVAR(pStream, BEntityID, mFXAttachment);
   GFWRITEVAR(pStream, BUnitOppID, mUnitOppID);
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEARRAY(pStream, BTeamID, mAttackingTeamIDs, uint8, 6);
   GFWRITEVAR(pStream, BEntityID, mBeamHeadID);
   GFWRITEVAR(pStream, BEntityID, mBeamTailID);
   GFWRITEBITBOOL(pStream, mIsMoving);

   GFWRITEVAR(pStream, long, mBoneHandle);

   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionRepairOther::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   GFREADVAR(pStream, BEntityID, mTargetSquad);
   GFREADVAR(pStream, BEntityID, mFXAttachment);
   GFREADVAR(pStream, BUnitOppID, mUnitOppID);
   GFREADCLASS(pStream, saveType, mTarget);
   GFREADARRAY(pStream, BTeamID, mAttackingTeamIDs, uint8, 6);

   if (BAction::mGameFileVersion >= 9)
   {
      GFREADVAR(pStream, BEntityID, mBeamHeadID);
      GFREADVAR(pStream, BEntityID, mBeamTailID);
   }

   if (BAction::mGameFileVersion >= 18)
   {
      GFREADBITBOOL(pStream, mIsMoving);
   }

   if (BAction::mGameFileVersion >= 31)
   {
      GFREADVAR(pStream, long, mBoneHandle);
   }

   return true;
}

//==============================================================================
//==============================================================================
void BSquadActionRepairOther::createEffect(BObject* pSquadLeader)
{
   BASSERT(pSquadLeader);
   BASSERT(mpProtoAction);
   
   BSquad* pSquad = gWorld->getSquad(mTargetSquad); 
   BASSERT(pSquad);

   if (mpProtoAction->getProtoObject() != cInvalidObjectID)
   {
      BPlayer* pPlayer = gWorld->getPlayer(pSquad->getPlayerID());
      if (!pPlayer)
         return;

      BProtoObject* pProto = pPlayer->getProtoObject(mpProtoAction->getProtoObject());
      if (!pProto)
         return;

      // Create beam
      BVector pos = pSquad->getPosition();
      BUnit *pLeaderUnit = pSquad->getLeaderUnit();
      if (pLeaderUnit)
         pos = pLeaderUnit->getPosition();

      BObjectCreateParms parms;
      parms.mPlayerID = pSquad->getPlayerID();
      parms.mPosition = pos;
      parms.mRight = cXAxisVector;
      parms.mForward = cZAxisVector;
      parms.mProtoObjectID = mpProtoAction->getProtoObject();
      parms.mIgnorePop = true;
      parms.mNoTieToGround = true;
      parms.mPhysicsReplacement = false;
      parms.mType = BEntity::cClassTypeObject;   
      parms.mStartBuilt=true;
      
      BObject* pBeam = gWorld->createObject(parms);
      BASSERT(pBeam);
      mFXAttachment = pBeam->getID();
      BASSERT(mFXAttachment != cInvalidObjectID);

      // Create head
      if (pProto->getBeamHead() != cInvalidObjectID)
      {
         parms.mProtoObjectID = pProto->getBeamHead();
         BObject* pBeamHead = gWorld->createObject(parms);
         BASSERT(pBeamHead);
         mBeamHeadID = pBeamHead->getID();
         BASSERT(mBeamHeadID != cInvalidObjectID);
      }

      // Create tail
      if (pProto->getBeamTail() != cInvalidObjectID)
      {
         parms.mProtoObjectID = pProto->getBeamTail();
         BObject* pBeamTail = gWorld->createObject(parms);
         BASSERT(pBeamTail);
         mBeamTailID = pBeamTail->getID();
         BASSERT(mBeamTailID != cInvalidObjectID);
      }
   }
   else
   {
      int repairProtoID = gDatabase.getProtoObject("fx_repairing");;

      BVector offset = cOriginVector;
      if (pSquad->getProtoSquad())
         offset = pSquad->getProtoSquad()->getHPBarOffset();

      mFXAttachment = gWorld->createEntity(repairProtoID, false, pSquadLeader->getPlayerID(), pSquadLeader->getPosition() + offset, pSquadLeader->getForward(), pSquadLeader->getRight(), true);
      BObject* pObject = gWorld->getObject(mFXAttachment);
      if (pObject)
         pSquadLeader->addAttachment(pObject, -1, -1, false, true);
   }
}

//==============================================================================
//==============================================================================
void BSquadActionRepairOther::updateEffect()
{
   BSquad* pSquad = mpOwner->getSquad();
   BASSERT(pSquad);

   if (mpProtoAction->getProtoObject() != cInvalidObjectID && mFXAttachment.isValid() && pSquad && pSquad->getLeaderUnit())
   {
      BObject* pBeam = gWorld->getObject(mFXAttachment);
      if (!pBeam)
         return;

      BMatrix mtx;
      mtx.makeIdentity();

      BSquad* pTargetSquad = gWorld->getSquad(mTargetSquad);
      if (!pTargetSquad)
         return;

      if (!pTargetSquad->getLeaderUnit())
         return;

      // Calculate beam end location
      BVector endPos = pTargetSquad->getPosition();

      BVector dir = pTargetSquad->getPosition() - pBeam->getPosition();
      dir.normalize();
      BASSERT(dir.length() > -cFloatCompareEpsilon);
      BVisual* pVisual = pTargetSquad->getLeaderUnit()->getVisual();
      if (!pVisual)
         return;
      dir *= pVisual->getMaxCorner().y * 0.5f;
      endPos -= dir;

      BVector pos = pSquad->getLeaderUnit()->getPosition();
      BUnit* pUnit = pSquad->getLeaderUnit();
      if (pUnit)
      {
         BVisual* pVis = pUnit->getVisual();
         if (pVis)
         {
            BMatrix mat, unitMat;
            pUnit->getWorldMatrix(unitMat);
            pVis->getBone(mBoneHandle, NULL, &mat);
            unitMat *= mat;
            unitMat.getTranslation(pos);
         }
      }

      pVisual = pSquad->getLeaderUnit()->getVisual();
      if (!pVisual)
         return;

      pBeam->setPosition(pos);
      pVisual = pBeam->getVisual();
      if (pVisual)
      {
         mtx.setTranslation(pos);
         pVisual->updateWorldMatrix(mtx,NULL);

         mtx.setTranslation(endPos);  
         pVisual->updateSecondaryWorldMatrix(mtx);
      }

      BObject* pBeamHead = gWorld->getObject(mBeamHeadID);
      if (pBeamHead)
      {
         pBeamHead->setPosition(pos);
         mtx.setTranslation(pos);
         BVisual* pVisual = pBeamHead->getVisual();
         if (pVisual)
            pVisual->updateWorldMatrix(mtx,NULL);
      }

      BObject* pBeamTail = gWorld->getObject(mBeamTailID);
      if (pBeamTail)
      {
         pBeamTail->setPosition(endPos);
         mtx.setTranslation(endPos);
         BVisual* pVisual = pBeamTail->getVisual();
         if (pVisual)
            pVisual->updateWorldMatrix(mtx,NULL);
      }
   }
}

//==============================================================================
//==============================================================================
void BSquadActionRepairOther::destroyEffect()
{
   if (mFXAttachment.isValid())
   {
      BEntity *pAttachEntity = gWorld->getEntity(mFXAttachment);
      if (pAttachEntity != NULL)
         pAttachEntity->kill(false);
      mFXAttachment.invalidate();

      pAttachEntity = gWorld->getObject(mBeamHeadID);
      if (pAttachEntity)
         pAttachEntity->kill(false);
      mBeamHeadID.invalidate();

      pAttachEntity = gWorld->getObject(mBeamTailID);
      if (pAttachEntity)
         pAttachEntity->kill(false);
      mBeamTailID.invalidate();
   }
}

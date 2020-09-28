//==============================================================================
// unitactionbubbleshield.cpp
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#include "Common.h"
#include "UnitActionBubbleShield.h"
#include "Unit.h"
#include "User.h"
#include "UserManager.h"
#include "selectionmanager.h"
#include "World.h"                            
#include "game.h"
#include "tactic.h"
#include "protosquad.h"
#include "particlesystemmanager.h"
#include "particlegateway.h"
#include "unitactionheal.h"
#include "actionmanager.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionBubbleShield, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionBubbleShield::connect(BEntity* pOwner, BSimOrder* pOrder)
{
//-- FIXING PREFIX BUG ID 1564
   const BUnit* pUnit=reinterpret_cast<BUnit*>(pOwner);
//--
   if (!pUnit)
      return false;
   if (!mpProtoAction)
      return false;
   if (!BAction::connect(pOwner, pOrder))
      return (false);

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionBubbleShield::disconnect()
{
   if (mShieldID != cInvalidObjectID)
   {
      BSquad* pShield = gWorld->getSquad(mShieldID);
      if (pShield)
         pShield->kill(true);
      mShieldID = cInvalidObjectID;

      // Destroy beam
      BObject* pBeam = gWorld->getObject(mBeamID);
      if (pBeam)
         pBeam->kill(false);
      mBeamID = cInvalidObjectID;

      pBeam = gWorld->getObject(mBeamHeadID);
      if (pBeam)
         pBeam->kill(false);
      mBeamHeadID = cInvalidObjectID;

      pBeam = gWorld->getObject(mBeamTailID);
      if (pBeam)
         pBeam->kill(false);
      mBeamTailID = cInvalidObjectID;
   }

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionBubbleShield::init(void)
{
   if (!BAction::init())
      return (false);

   mTarget.reset();
   mShieldID = cInvalidObjectID;
   mBeamID = cInvalidObjectID;
   mBeamHeadID = cInvalidObjectID;
   mBeamTailID = cInvalidObjectID;
   mLastDamageTime = 0;
   mRender = true;

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionBubbleShield::update(float elapsed)
{
   BSquad* pTargetSquad = gWorld->getSquad(mTarget.getID());

   if (!pTargetSquad || !pTargetSquad->isAlive() || !pTargetSquad->getLeaderUnit() || !pTargetSquad->getPlayer())
      return true;

   BSquad* pShield = gWorld->getSquad(mShieldID);

   // Shield died, recreate it if we're out of combat
   if (mShieldID == cInvalidObjectID)
   {
      const DWORD lastDamagedTime = max(mLastDamageTime, pTargetSquad->getLastDamagedTime());
      const DWORD gameTime = gWorld->getGametime();
      const DWORD duration = gameTime - lastDamagedTime;

      if (lastDamagedTime == 0 || duration > pTargetSquad->getPlayer()->getShieldRegenDelay() * pTargetSquad->getLeaderUnit()->getShieldRegenDelay())
         createShield();
      else
         return (true);
   }

   // Create the beam if it doesn't exist
   if (mShieldID != cInvalidObjectID && mBeamHeadID == cInvalidObjectID)
   {
      createBeam();
   }

   // Orient the beam towards the target if our shield is up
   if (mShieldID != cInvalidObjectID && mBeamHeadID != cInvalidObjectID)
   {
      BUnit* pUnit = mpOwner->getUnit();
      BASSERT(pUnit);

      BSquad* pShield = gWorld->getSquad(mShieldID);
      BASSERT(pShield);

      BObject* pBeamHead = gWorld->getObject(mBeamHeadID);
      BASSERT(pBeamHead);

      BObject* pBeamTail = gWorld->getObject(mBeamTailID);
      BASSERT(pBeamTail);

      BObject* pBeam = gWorld->getObject(mBeamID);
      BASSERT(pBeam);
      
      if (!pShield || !pBeamHead || !pBeamTail || !pBeam)
         return true;

      BMatrix mtx;
      mtx.makeIdentity();

      // Calculate beam end location so it's on the shield
      BVector endPos = pShield->getPosition();

      BUnit *pShieldLeaderUnit = pShield->getLeaderUnit();
      if (pShieldLeaderUnit)
      {
         BVisual *pVisual = pShieldLeaderUnit->getVisual();
         if (pVisual)
         {
            BVector dir = pShield->getPosition() - pBeam->getPosition();
            dir.normalize();
            BASSERT(dir.length() > -cFloatCompareEpsilon);
            dir *= pVisual->getMaxCorner().y * 0.95f;
            endPos -= dir;
         }
      }

      pBeamHead->setPosition(pUnit->getPosition());
      /*mtx.setTranslation(pUnit->getPosition());
      BVisual* pVisual = pBeamHead->getVisual();
      if (pVisual)
         pVisual->updateWorldMatrix(mtx,NULL);*/

      pBeamTail->setPosition(endPos);
      /*mtx.setTranslation(endPos);
      pVisual = pBeamTail->getVisual();
      if (pVisual)
         pVisual->updateWorldMatrix(mtx,NULL);*/

      pBeam->setPosition(pUnit->getPosition());
      BVisual* pVisual = pBeam->getVisual();
      if (pVisual)
      {
         //mtx.setTranslation(pUnit->getPosition());
         //pVisual->updateWorldMatrix(mtx,NULL);

         mtx.setTranslation(endPos);  
         pVisual->updateSecondaryWorldMatrix(mtx);
      }

      bool oldRender = mRender;
      
      mRender = true;

      // Don't render shield if container is moving
      if (pTargetSquad && pTargetSquad->isGarrisoned())
      {
         if (pTargetSquad->getLeaderUnit())
         {
            BEntityRef* pRef = pTargetSquad->getLeaderUnit()->getFirstEntityRefByType(BEntityRef::cTypeContainingUnit);
            if (pRef)
            {
               BUnit* pContainerUnit = gWorld->getUnit(pRef->mID);

               if (pContainerUnit)
               {
                  if (pContainerUnit->isType(gDatabase.getOTIDTransporter()))
                     mRender = false;
               }
            }
         }
      }

      if (oldRender != mRender)
      {
         pBeam->setFlagNoRender(!mRender);
         pBeamHead->setFlagNoRender(!mRender);
         pBeamTail->setFlagNoRender(!mRender);

         int count = pShield->getNumberChildren();
         for (int i = 0; i < count; ++i)
         {
            BEntityID id = pShield->getChild(i);
            BUnit* pUnit = gWorld->getUnit(id);
            BASSERT(pUnit);
            if (pUnit)
               pUnit->setFlagNoRender(!mRender);
         }
      }
   }

   if (pShield && pTargetSquad->getNumberChildren() > 0)
   {
      BVector pos = pTargetSquad->getAveragePosition();

      if (pTargetSquad->getNumberChildren() == 1 && pTargetSquad->getLeaderUnit())
         pos = pTargetSquad->getLeaderUnit()->getPosition();
      
      // Keep shield synced up with squad
      pShield->setPosition(pos);
      pShield->setForward(pTargetSquad->getForward());
      pShield->setRight(pTargetSquad->getRight());
      pShield->setUp(pTargetSquad->getUp());

      int count = pShield->getNumberChildren();
      for (int i = 0; i < count; ++i)
      {
         BEntityID id = pShield->getChild(i);
         BUnit* pUnit = gWorld->getUnit(id);
         BASSERT(pUnit);
         #ifdef SYNC_Unit
            syncUnitData("BUnitActionBubbleShield::update", pShield->getPosition());
         #endif
         if (pUnit)
            pUnit->setPosition(pShield->getPosition());
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BUnitActionBubbleShield::setTarget(BSimTarget target)
{
   BASSERT(!target.getID().isValid() || (target.getID().getType() == BEntity::cClassTypeSquad));
   mTarget = target;

   // Destroy current shield if it exists
   BSquad* pShield = gWorld->getSquad(mShieldID);
   if (pShield)
      pShield->kill(true);
   mShieldID = cInvalidObjectID;

   // Destroy beam
   BObject* pBeam = gWorld->getObject(mBeamID);
   if (pBeam)
      pBeam->kill(false);
   mBeamID = cInvalidObjectID;

   pBeam = gWorld->getObject(mBeamHeadID);
   if (pBeam)
      pBeam->kill(false);
   mBeamHeadID = cInvalidObjectID;

   pBeam = gWorld->getObject(mBeamTailID);
   if (pBeam)
      pBeam->kill(false);
   mBeamTailID = cInvalidObjectID;

   createShield();
}


//==============================================================================
//==============================================================================
void BUnitActionBubbleShield::createShield()
{
   BSquad* pTargetSquad = gWorld->getSquad(mTarget.getID());
   if (!pTargetSquad)
      return;

   // We already have a shield, bail
   if (mShieldID != cInvalidObjectID)
      return;

   BSquad* pSquad = NULL;

   // Add bubble shield to the squad
   BObjectCreateParms parms;
   parms.mPlayerID = pTargetSquad->getPlayerID();
   parms.mProtoSquadID = gDatabase.getProtoShieldType(pTargetSquad->getProtoID());
   parms.mPosition = pTargetSquad->getAveragePosition();
   parms.mForward = pTargetSquad->getForward();
   parms.mRight = pTargetSquad->getRight();
   parms.mIgnorePop = true;
   parms.mNoTieToGround = true;
   parms.mPhysicsReplacement = false;
   parms.mType = BEntity::cClassTypeObject;   
   parms.mStartBuilt=true;

   pSquad = gWorld->createSquad(parms);
   BASSERT(pSquad);
   BASSERT(pSquad->getLeaderUnit());

   mShieldID = pSquad->getID();

   //pSquad->stop();
   pSquad->setFlagAttached(true);
   pSquad->setFlagCollidable(false);
   pSquad->updateObstruction();
   pSquad->setPhysicsKeyFramed(true, true);

   //pSquad->getLeaderUnit()->stop();
   pSquad->getLeaderUnit()->setFlagAttached(true);
   pSquad->getLeaderUnit()->setFlagCollidable(false);
   pSquad->getLeaderUnit()->updateObstruction();
   pSquad->getLeaderUnit()->setPhysicsKeyFramed(true, true);
   pSquad->getLeaderUnit()->setFlagHasShield(true);

   
   if (pTargetSquad->getFlagFlying() || (pTargetSquad->getLeaderUnit() && pTargetSquad->getLeaderUnit()->isPhysicsAircraft()))
   {
      pSquad->setFlagTiesToGround(false);
      pSquad->setPosition(pTargetSquad->getAveragePosition());
   }

   // Cause shield to recharge
   pSquad->getLeaderUnit()->setShieldpoints(0.1f);

   BAction* pAction = gActionManager.createAction(BAction::cActionTypeSquadShieldRegen);
   if (pAction)
   {
      pSquad->addAction(pAction);
      pSquad->setFlagShieldDamaged(true);
   }

   pTargetSquad->setDamageProxy(mShieldID);

   // Setup listener for when shield is killed
   pSquad->addEventListener(this);
}


//==============================================================================
//==============================================================================
void BUnitActionBubbleShield::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   if (eventType == BEntity::cEventDamaged)
   {
//-- FIXING PREFIX BUG ID 1565
      const BSquad* pSquad = gWorld->getSquad(mShieldID);
//--

      mLastDamageTime = gWorld->getGametime();

      if (pSquad && pSquad->getCurrentHP() <= 0.0f)
      {
         mShieldID = cInvalidObjectID;

         // Destroy beam
         BObject* pBeam = gWorld->getObject(mBeamID);
         if (pBeam)
            pBeam->kill(false);
         mBeamID = cInvalidObjectID;

         pBeam = gWorld->getObject(mBeamHeadID);
         if (pBeam)
            pBeam->kill(false);
         mBeamHeadID = cInvalidObjectID;

         pBeam = gWorld->getObject(mBeamTailID);
         if (pBeam)
            pBeam->kill(false);
         mBeamTailID = cInvalidObjectID;

         BSquad* pTargetSquad = gWorld->getSquad(mTarget.getID());
         if (!pTargetSquad)
            return;

         pTargetSquad->setDamageProxy(cInvalidObjectID);
      }
   }

   BAction::notify(eventType, senderID, data, data2);
}

//==============================================================================
//==============================================================================
void BUnitActionBubbleShield::createBeam()
{
   BASSERT(mpProtoAction);

   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);

   //mBeamID = pUnit->addAttachment(mpProtoAction->getProtoObject());
   BPlayer* pPlayer = gWorld->getPlayer(pUnit->getPlayerID());
   if (!pPlayer)
      return;

   BProtoObject* pProto = pPlayer->getProtoObject(mpProtoAction->getProtoObject());
   if (!pProto)
      return;

   BObjectCreateParms parms;
   parms.mPlayerID = pUnit->getPlayerID();
   parms.mPosition = pUnit->getPosition();
   parms.mRight = cXAxisVector;
   parms.mForward = cZAxisVector;
   parms.mProtoObjectID = pProto->getBeamHead();   
   parms.mIgnorePop = true;
   parms.mNoTieToGround = true;
   parms.mPhysicsReplacement = false;
   parms.mType = BEntity::cClassTypeObject;   
   parms.mStartBuilt=true;

   BObject* pBeamHead = gWorld->createObject(parms);
   BASSERT(pBeamHead);
   mBeamHeadID = pBeamHead->getID();
   BASSERT(mBeamHeadID != cInvalidObjectID);

   BSquad* pShield = gWorld->getSquad(mShieldID);
   BASSERT(pShield);

   BVector endPos = pShield->getPosition();

   BUnit *pShieldLeaderUnit = pShield->getLeaderUnit();
   if (pShieldLeaderUnit)
   {
      BVisual *pVisual = pShieldLeaderUnit->getVisual();
      if (pVisual)
      {
         BVector dir = pShield->getPosition() - pBeamHead->getPosition();
         dir.normalize();
         BASSERT(dir.length() > -cFloatCompareEpsilon);
         dir *= pVisual->getMaxCorner().y * 0.95f;
         endPos -= dir;
      }
   }

   parms.mPosition = cOriginVector;
   parms.mProtoObjectID = pProto->getBeamTail();
   BObject* pBeamTail = gWorld->createObject(parms);
   BASSERT(pBeamTail);
   mBeamTailID = pBeamTail->getID();
   BASSERT(mBeamTailID != cInvalidObjectID);

   parms.mPosition = pUnit->getPosition();
   parms.mProtoObjectID = mpProtoAction->getProtoObject();
   BObject* pBeam = gWorld->createObject(parms);
   BASSERT(pBeam);
   mBeamID = pBeam->getID();
   BASSERT(mBeamID != cInvalidObjectID);

   BVisual* pVisual = pBeam->getVisual();
   if (pVisual)
   {
      BMatrix mtx;
      mtx.makeIdentity();

      //mtx.setTranslation(pUnit->getPosition());
      //pVisual->updateWorldMatrix(mtx,NULL);

      mtx.setTranslation(endPos);  
      pVisual->updateSecondaryWorldMatrix(mtx);
   }
}


//==============================================================================
//==============================================================================
bool BUnitActionBubbleShield::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEVAR(pStream, BEntityID, mShieldID);
   GFWRITEVAR(pStream, BEntityID, mBeamID);
   GFWRITEVAR(pStream, BEntityID, mBeamHeadID);
   GFWRITEVAR(pStream, BEntityID, mBeamTailID);
   GFWRITEVAR(pStream, DWORD, mLastDamageTime);
   GFWRITEBITBOOL(pStream, mRender);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionBubbleShield::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADCLASS(pStream, saveType, mTarget);
   GFREADVAR(pStream, BEntityID, mShieldID);
   GFREADVAR(pStream, BEntityID, mBeamID);
   GFREADVAR(pStream, BEntityID, mBeamHeadID);
   GFREADVAR(pStream, BEntityID, mBeamTailID);
   GFREADVAR(pStream, DWORD, mLastDamageTime);
   if (BAction::mGameFileVersion >= 52)
   {
      GFREADBITBOOL(pStream, mRender);
   }
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionBubbleShield::savePtr(BStream* pStream) const
{
   GFWRITEACTIONPTR(pStream, this);
   return true;
}

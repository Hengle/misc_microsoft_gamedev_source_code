//==============================================================================
// squadactionSpiritBond.cpp
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "squad.h"
#include "squadactionSpiritBond.h"
#include "world.h"
#include "actionmanager.h"
#include "database.h"
#include "config.h"
#include "configsgame.h"
#include "SimOrderManager.h"
#include "ability.h"
#include "visual.h"
#include "grannyinstance.h"
#include "usermanager.h"
#include "user.h"
#include "worldsoundmanager.h"
#include "tactic.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionSpiritBond, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionSpiritBond::connect(BEntity* pOwner, BSimOrder* pOrder)
{  
   if (!BAction::connect(pOwner, pOrder))
      return false;

   return true;
}
//==============================================================================
//==============================================================================
void BSquadActionSpiritBond::disconnect()
{
   // Remove beam
   destroyBeam();

   // Unapply damage and health buffs
   unbuffSquad();

   //trace("d/c");

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionSpiritBond::init()
{
   if (!BAction::init())
      return(false);

   setFlagPersistent(true);

   mBeamID = cInvalidObjectID;

   return(true);
}

//==============================================================================
//==============================================================================
bool BSquadActionSpiritBond::setState(BActionState state)
{
   if (!BAction::setState(state))
      return false;

   if (state == cStateDone)
   {
      //trace("setState done");

      // Remove beam
      destroyBeam();

      // Unapply damage and health buffs
      unbuffSquad();

      setFlagPersistent(false);

      mState = cStateDone;

      BAction::disconnect();
   }
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionSpiritBond::update(float elapsed)
{
   BASSERT(mpOwner);

   /*BEntity* pEntity = NULL;
   BEntityHandle handle = cInvalidObjectID;
   while ((pEntity = gWorld->getNextEntity(handle)) != NULL)
   {
      if (pEntity->getProtoID() == mpProtoAction->getProtoObject())
         trace("Bond in world");
   }

   pEntity = gWorld->getEntity(mBeamID);
   if (pEntity)
      trace("Bond entity exists");*/

   BSquad* pSquad = mpOwner->getSquad();
   if (!pSquad || !mpProtoAction)
      return true;

   //trace("Children = %d", pSquad->getNumberChildren());

   BASSERT(mpProtoAction);
   if (mpProtoAction->getFlagDisabled())
      return (true);

   // Can't activate, bail
   if (pSquad->getNumberChildren() != 2)
   {
      setState(cStateDone);
      return true;
   }

   BUnit* pMain = gWorld->getUnit(pSquad->getChild(0));
   BUnit* pSecond = gWorld->getUnit(pSquad->getChild(1));

   // Squadmate dead, bail
   if (!pMain || !pSecond || !pMain->isAlive() || !pSecond->isAlive())
   {
      setState(cStateDone);
      return true;
   }

   BASSERT(pSquad->getNumberChildren() == 2);

   switch (mState)
   {
      // Just starting up, no beam active
      case cStateNone:
         // Activate beam
         createBeam();

         // Apply damage and health buffs
         buffSquad();

         setState(cStateWorking);
         break;

      // Beam and both squadmates are up, update the beam
      case cStateWorking:
         updateBeam();
         break;

      // One or more squadmates is down, remove beam and buffs
      case cStateDone:
         // Remove beam
         destroyBeam();

         // Unapply damage and health buffs
         unbuffSquad();
         break;
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionSpiritBond::buffSquad()
{
   BSquad* pSquad = mpOwner->getSquad();
   if (!pSquad || !mpProtoAction)
      return;

   float damageModifier = mpProtoAction->getDamageBuffFactor();

   // Apply modifiers to squadmates
   for (uint i = 0; i < pSquad->getNumberChildren(); i++)
   {
      BUnit* pSquadmate = gWorld->getUnit(pSquad->getChild(i));
      if (pSquadmate)
         pSquadmate->adjustDamageModifier(damageModifier);
   }
}

//==============================================================================
//==============================================================================
void BSquadActionSpiritBond::unbuffSquad()
{
   BSquad* pSquad = mpOwner->getSquad();
   if (!pSquad || !mpProtoAction)
      return;

   float damageModifier = 1.0f / mpProtoAction->getDamageBuffFactor();

   // Apply modifiers to squadmates
   for (uint i = 0; i < pSquad->getNumberChildren(); i++)
   {
      BUnit* pSquadmate = gWorld->getUnit(pSquad->getChild(i));
      if (pSquadmate)
         pSquadmate->adjustDamageModifier(damageModifier);
   }
}

//==============================================================================
//==============================================================================
void BSquadActionSpiritBond::createBeam()
{
   BSquad* pSquad = mpOwner->getSquad();
   if (!pSquad || !mpProtoAction)
   {
      BASSERT(pSquad);
      BASSERT(mpProtoAction);
      return;
   }

   BPlayer* pPlayer = gWorld->getPlayer(pSquad->getPlayerID());
   if (!pPlayer)
   {
      BASSERT(pPlayer);
      return;
   }

   BProtoObject* pProto = pPlayer->getProtoObject(mpProtoAction->getProtoObject());
   if (!pProto)
   {
      BASSERT(pProto);
      return;
   }

   BVector pos = pSquad->getPosition();

   BUnit* pMain = gWorld->getUnit(pSquad->getChild(0));
   if (pMain)
      pos = pMain->getSimBoundingBox()->getCenter();

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
   parms.mStartBuilt = true;

   BObject* pBeam = gWorld->createObject(parms);
   BASSERT(pBeam);
   if (pBeam)
   {
      mBeamID = pBeam->getID();
      BASSERT(mBeamID != cInvalidObjectID);

      // Make sure beam starts in valid position
      updateBeam();
   }
}

//==============================================================================
//==============================================================================
void BSquadActionSpiritBond::updateBeam()
{
   BSquad* pSquad = mpOwner->getSquad();
   if (!pSquad)
   {
      BASSERT(pSquad);
      return;
   }

   BObject* pBeam = gWorld->getObject(mBeamID);

   if (!pBeam)
   {
      mBeamID = cInvalidObjectID;
      createBeam();
      return;
   }

   BVisual* pVisual = pVisual = pBeam->getVisual();
   BASSERT(pVisual);
   if (pVisual)
   {
      BUnit* pMain = gWorld->getUnit(pSquad->getChild(0));
      BUnit* pSecond = gWorld->getUnit(pSquad->getChild(1));

      // Squadmate gone, bail
      if (!pMain || !pSecond)
         return;

      BMatrix mtx;
      mtx.makeIdentity();

      pBeam->setPosition(pMain->getSimBoundingBox()->getCenter());

      mtx.setTranslation(pSecond->getSimBoundingBox()->getCenter());  
      pVisual->updateSecondaryWorldMatrix(mtx);
   }
}

//==============================================================================
//==============================================================================
void BSquadActionSpiritBond::destroyBeam()
{
   // Destroy beam
   BObject* pBeam = gWorld->getObject(mBeamID);

   if (pBeam)
   {
      if (pBeam->getVisual())
      {
         pBeam->getVisual()->setFlag(BVisualItem::cFlagImmediateRemove, true);
      }

      pBeam->kill(true);
   }
   mBeamID = cInvalidObjectID;
}

//==============================================================================
//==============================================================================
bool BSquadActionSpiritBond::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   
   GFWRITEVAR(pStream, BEntityID, mBeamID);
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionSpiritBond::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   GFREADVAR(pStream, BEntityID, mBeamID);
   return true;
}

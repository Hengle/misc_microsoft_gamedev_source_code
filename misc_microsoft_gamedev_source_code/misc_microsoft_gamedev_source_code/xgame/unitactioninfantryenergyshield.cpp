//==============================================================================
// unitactioninfantryenergyshield.cpp
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#include "common.h"
#include "database.h"
#include "unit.h"
#include "unitactioninfantryenergyshield.h"
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
IMPLEMENT_FREELIST(BUnitActionInfantryEnergyShield, 4, &gSimHeap);


//==============================================================================
//==============================================================================
bool BUnitActionInfantryEnergyShield::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   BDEBUG_ASSERT(mpProtoAction);

   //Connect.
   if (!BAction::connect(pOwner, pOrder))
      return false;

   if (!setupShieldAttachment())
      return false;

   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionInfantryEnergyShield::disconnect()
{
   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BUnitActionInfantryEnergyShield::setupShieldAttachment()
{
   if (!mpOwner)
      return false;

   BUnit* pUnit = mpOwner->getUnit();
   if (!pUnit)
      return false;

   BVisual* pVisual = pUnit->getVisual();
   if (pVisual)
   {
      mShieldAttachment = pVisual->getAttachmentHandle("Shield");
      if (mShieldAttachment != -1)
      {
         BVisualItem* pItem = pVisual->getAttachment(mShieldAttachment);
         if (pItem)
         {
            pItem->setFlag(BVisualItem::cFlagVisible, false);
            return true;
         }
      }
   }

   return false;
}

//==============================================================================
//==============================================================================
bool BUnitActionInfantryEnergyShield::init()
{
   if (!BAction::init())
      return false;

   mShieldAttachment = -1;
   mShieldTimer = 0.0f;
   mHit = false;

   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionInfantryEnergyShield::setState(BActionState state)
{
   return BAction::setState(state);
}

//==============================================================================
//==============================================================================
bool BUnitActionInfantryEnergyShield::update(float elapsed)
{
   BASSERT(mpOwner);
   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);

   if (mShieldTimer > 0.0f)
   {
      mShieldTimer -= elapsed;
      if (mShieldTimer <= 0.0f)
      {
         BVisualItem* pItem = pUnit->getVisual()->getAttachment(mShieldAttachment);
         if (pItem)
            pItem->setFlag(BVisualItem::cFlagVisible, false);
         mShieldTimer = 0.0f;
         mHit = false;
      }
   }
   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionInfantryEnergyShield::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   BASSERT(mpOwner);

   if (eventType == BEntity::cEventDamaged || eventType == BEntity::cEventKilled)
   {
      BUnit* pUnit = mpOwner->getUnit();
      BASSERT(pUnit);

      if (pUnit->getShieldpoints() > 0.0f)
         shieldsHit();
   }
}

//==============================================================================
//==============================================================================
void BUnitActionInfantryEnergyShield::shieldsHit()
{
   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);
   BASSERT(mpProtoAction);

   if (!mHit)
   {
      BVisual* pVisual = pUnit->getVisual();
      if (pVisual)
      {
         BVisualItem* pItem = pVisual->getAttachment(mShieldAttachment);
         if (pItem)
            pItem->setFlag(BVisualItem::cFlagVisible, false);
      }
      mHit = true;
   }

   // Start shield timer
   mShieldTimer = mpProtoAction->getDuration();
}

//==============================================================================
//==============================================================================
bool BUnitActionInfantryEnergyShield::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, float, mShieldTimer);
   GFWRITEBITBOOL(pStream, mHit);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionInfantryEnergyShield::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   GFREADVAR(pStream, float, mShieldTimer);
   GFREADBITBOOL(pStream, mHit);

   setupShieldAttachment();

   return true;
}

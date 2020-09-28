//==============================================================================
// unitactionshieldregen.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "protoobject.h"
#include "unit.h"
#include "unitactionshieldregen.h"
#include "world.h"
#include "soundmanager.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionShieldRegen, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionShieldRegen::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return (false);

   mActionEndTime=(DWORD) gWorld->getGametime() + gDatabase.getShieldRegenTime();
   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionShieldRegen::init()
{
   if (!BAction::init())
      return (false);

   setFlagPersistent(true);
   mActionEndTime=0;
   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionShieldRegen::update(float elapsed)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   const BProtoObject* pProtoObject=pUnit->getProtoObject();
   BASSERT(pProtoObject);

   switch (mState)
   {
      case cStateNone:
         setState(cStateWorking);
         pUnit->playShieldSound(cObjectSoundShieldRegen);
         break;
         
      case cStateWorking:
      {
         // If parent has no recharge flag set, bail
         BSquad* pParent = pUnit->getParentSquad();
         if (pParent && pParent->getFlagStopShieldRegen())
            setState(cStateDone);

         float maxShieldPoints=pProtoObject->getShieldpoints();
         float regenRate=maxShieldPoints*pUnit->getPlayer()->getShieldRegenRate() * pUnit->getShieldRegenRate();
         float shieldPoints=Math::Min(pUnit->getShieldpoints() + regenRate * elapsed, maxShieldPoints);
         pUnit->setShieldpoints(shieldPoints);

         if ((DWORD) gWorld->getGametime() >= mActionEndTime)
            setState(cStateDone);
         break;
      }
   }

   return (true);
}


//==============================================================================
//==============================================================================
bool BUnitActionShieldRegen::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, DWORD, mActionEndTime);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionShieldRegen::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, DWORD, mActionEndTime);
   return true;
}

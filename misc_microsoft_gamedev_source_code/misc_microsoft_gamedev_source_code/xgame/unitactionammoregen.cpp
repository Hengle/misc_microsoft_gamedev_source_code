//==============================================================================
// unitactionammoregen.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "protoobject.h"
#include "unit.h"
#include "unitactionammoregen.h"
#include "game.h"
#include "world.h"


//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionAmmoRegen, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionAmmoRegen::init()
{
   if (!BAction::init())
      return(false);

   mFlagPersistent=true;
   return(true);
}

//==============================================================================
//==============================================================================
bool BUnitActionAmmoRegen::update(float elapsed)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   const BProtoObject* pProtoObject=pUnit->getProtoObject();
   BASSERT(pProtoObject);
   
   switch (mState)
   {
      case cStateNone:
         setState(cStateWorking);
         break;
         
      case cStateWorking:
      {
         float maxAmmo=pProtoObject->getMaxAmmo();
         float regenRate=pProtoObject->getAmmoRegenRate();

         if (gWorld->getFlagQuickBuild())
            pUnit->setAmmunition(maxAmmo);
         else if (pUnit->getAmmunition() < maxAmmo)
         {
            float newAmmo=pUnit->getAmmunition();         
            newAmmo+=regenRate*elapsed;
            if (newAmmo > maxAmmo)
               newAmmo=maxAmmo;
            pUnit->setAmmunition(newAmmo);
         }
         break;
      }
   }

   return (true);
}

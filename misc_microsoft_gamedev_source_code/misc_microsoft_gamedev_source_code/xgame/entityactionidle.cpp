//==============================================================================
// entityactionidle.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "actionmanager.h"
#include "entity.h"
#include "protoobject.h"
#include "squad.h"
#include "unit.h"
#include "entityactionidle.h"
#include "world.h"
#include "syncmacros.h"



//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BEntityActionIdle, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BEntityActionIdle::init()
{
   if (!BAction::init())
      return(false);

   mIdleDuration=0;
   return(true);
}

//==============================================================================
//==============================================================================
bool BEntityActionIdle::setState(BActionState state)
{
   switch (state)
   {
      case cStateDone:
      case cStateWait:
      case cStateFailed:
         break;

      case cStateWorking:
         if (mpOwner->isClassType(BEntity::cClassTypeUnit))
         {
            BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
            BASSERT(pUnit);
            if ((pUnit->getAnimationState() != BObjectAnimationState::cAnimationStateIdle) && (pUnit->getAnimationState() != BObjectAnimationState::cAnimationStatePostAnim))
            {
#ifdef SYNC_Anim
               syncAnimData("BEntityActionIdle::setState animState", pUnit->getAnimationState());
#endif
               pUnit->setAnimation(mID, BObjectAnimationState::cAnimationStateIdle);
            }
         }
         break;
   }
 
   return(BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BEntityActionIdle::update(float elapsed)
{
   switch (mState)
   {
      case cStateNone:
         setState(cStateWorking);
         break;

      case cStateWorking:
         mIdleDuration += gWorld->getLastUpdateLength();
         break;
   }
   
   //Done.
   return (true);
}

//==============================================================================
//==============================================================================
bool BEntityActionIdle::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, DWORD, mIdleDuration);
   return true;
}

//==============================================================================
//==============================================================================
bool BEntityActionIdle::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, DWORD, mIdleDuration);
   return true;
}

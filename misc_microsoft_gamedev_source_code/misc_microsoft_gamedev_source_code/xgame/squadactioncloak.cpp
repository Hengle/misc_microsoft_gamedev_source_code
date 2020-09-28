//==============================================================================
// squadactioncloak.cpp
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "squad.h"
#include "squadactioncloak.h"
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
IMPLEMENT_FREELIST(BSquadActionCloak, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionCloak::connect(BEntity* pOwner, BSimOrder* pOrder)
{  
   if (!BAction::connect(pOwner, pOrder))
      return false;

   return true;
}
//==============================================================================
//==============================================================================
void BSquadActionCloak::disconnect()
{
//-- FIXING PREFIX BUG ID 2087
   const BSquad* pSquad = mpOwner->getSquad();
//--
   BASSERT(pSquad);

   for (uint i = 0; i < mFXAttachment.size(); ++i)
   {
      if (mFXAttachment[i].isValid())
      {
         BEntity *pAttachEntity = gWorld->getEntity(mFXAttachment[i]);
         if ( pAttachEntity)
            pAttachEntity->kill(false);
         mFXAttachment[i].invalidate();
      }
   }

   for (uint i = 0; i < pSquad->getNumberChildren(); ++i)
   {
      BObject* pUnit = gWorld->getObject(pSquad->getChild(i));
      if (!pUnit || !pUnit->getVisual())
         continue;

      BGrannyInstance * pGrannyInstance = pUnit->getVisual()->getGrannyInstance();
      if (!pGrannyInstance)
         continue;

      pGrannyInstance->clearMeshRenderMaskResetCallback();
   }

   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionCloak::init()
{
   if (!BAction::init())
      return(false);

   setFlagPersistent(true);

   mActivateDelay = gDatabase.getCloakingDelay();
   mDetectedCountdown = gDatabase.getReCloakingDelay();
   mFlagPermaCloaked = false;

   return(true);
}

//==============================================================================
//==============================================================================
bool BSquadActionCloak::setState(BActionState state)
{

   return (BAction::setState(state));
}

//==============================================================================
//==============================================================================
bool BSquadActionCloak::update(float elapsed)
{
   BASSERT(mpOwner);

   BASSERT(mpProtoAction);
   if (mpProtoAction->getFlagDisabled())
      return (true);

   switch (mState)
   {
   case cStateNone:
      {
//-- FIXING PREFIX BUG ID 2094
         const BSquad* pSquad = mpOwner->getSquad();
//--
         BASSERT(pSquad);
         if (pSquad)
         {
            mFXAttachment.resize(pSquad->getNumberChildren());

            // Should we run by default?  Hacky, but I'm reusing this flag
            // to avoid bloating out units/actions even more
            if (mpProtoAction && mpProtoAction->getFlagNoAutoTarget())
            {
               mFlagPermaCloaked = true;
               cloak();
               //pSquad->setFlagWantsToCloak(true);
            }

            setState(cStateWorking);
         }
         break;
      }

   case cStateWorking:
      {
         DWORD lastUpdateLength = gWorld->getLastUpdateLength();
         bool bMovingAndMustUncloak = false;
         bool bAttackingAndMustUncloak = false;

         if (!mpOwner->getProtoObject()->getFlagCloakMove())
         {
//-- FIXING PREFIX BUG ID 2092
            const BAction* pAction = mpOwner->getActionByType(BAction::cActionTypeSquadMove);
//--
            
            if( pAction )
               bMovingAndMustUncloak = true;
         }

         // [8-28-08 CJS] Disabling as per JIRA per 10624
         /*if (!mpOwner->getProtoObject()->getFlagCloakAttack())
         {
//-- FIXING PREFIX BUG ID 2093
            const BAction* pAction = mpOwner->getActionByType(BAction::cActionTypeSquadAttack);
//--

            if( pAction )
               bAttackingAndMustUncloak = true;
         }*/

         BSquad* pSquad = mpOwner->getSquad();

         BASSERT(pSquad);

         // DMG NOTE: This could probably use some logic refactoring...
         if (pSquad && pSquad->getFlagWantsToCloak() && !pSquad->getFlagCloaked() && (!bMovingAndMustUncloak || !bAttackingAndMustUncloak) )
         {                  
            if( mActivateDelay <= lastUpdateLength )
            {
               mActivateDelay = 0;
               cloak();
            }
            else
            {
               mActivateDelay -= lastUpdateLength;
            }
         }
         else if (pSquad && pSquad->getFlagCloaked() && (bMovingAndMustUncloak || bAttackingAndMustUncloak) )
         {
            uncloak();
         }

         if (pSquad && pSquad->getFlagCloakDetected())
         {
            if ( mDetectedCountdown <= lastUpdateLength )
            {
               mDetectedCountdown = 0;
               undetect();
            }
            else
            {
               mDetectedCountdown -= lastUpdateLength;
            }
         }

         break;
      }

   case cStateDone:
      // Probably need to change the mode of the squad here.  Or do I just need to change the mode of the units?
      break;
   }

   return (true);
}

//==============================================================================
//==============================================================================
void BSquadActionCloak::notify(DWORD eventType, BEntityID senderID, DWORD data1, DWORD data2)
{
//-- FIXING PREFIX BUG ID 2096
   const BSquad* pSquad = mpOwner->getSquad();
//--
   BASSERT(pSquad);

   switch (eventType)
   {
   case BEntity::cEventDamaged:
      if (pSquad && pSquad->getFlagCloaked())
      {
         // Go ahead and reset the detection timer
         detect();
      }
      break;

   case BEntity::cEventDetected:
      {
         detect();
      }
      break;
   case BEntity::cEventSquadTimedAbilityOver:
      if (senderID == mpOwner->getID())
         uncloak();
      break;
   }  
}

//==============================================================================
//==============================================================================
void BSquadActionCloak::cloak()
{
   if (mpProtoAction->getFlagDisabled())
      return;

   BSquad* pSquad = mpOwner->getSquad();
   BASSERT(pSquad);

   if (pSquad && !pSquad->getFlagCloaked())
   {
      pSquad->setFlagCloaked(true);

      BAbilityID abilityID = gDatabase.getSquadAbilityID(pSquad, gDatabase.getAIDCommand());

      BAbility * pAbility = gDatabase.getAbilityFromID(abilityID);

      BASSERT(pAbility);
      
      if (!mFlagPermaCloaked)
         pSquad->setAbilityTimer(0, pAbility->getAbilityDuration(), abilityID);

      pSquad->setFlagCloaked(true);

      pSquad->setAbilityDamageTakenModifier(pAbility->getDamageTakenModifier(),false);
      pSquad->setAbilityDodgeModifier(pAbility->getDodgeModifier(), false);
         
      updateVisuals(pSquad, false);

      playCloakSound(true);
   }
}
//==============================================================================
//==============================================================================
void BSquadActionCloak::uncloak()
{
   if (mpProtoAction->getFlagDisabled())
      return;

   if (mFlagPermaCloaked)
      return;

   BSquad* pSquad = mpOwner->getSquad();
   BASSERT(pSquad);

   mActivateDelay=gDatabase.getCloakingDelay();
   if (pSquad && pSquad->getFlagCloaked())
   {
      BAbilityID abilityID = gDatabase.getSquadAbilityID(pSquad, gDatabase.getAIDCommand());

      BAbility * pAbility = gDatabase.getAbilityFromID(abilityID);

      BASSERT(pAbility);

      pSquad->setFlagCloaked(false);
      pSquad->setFlagWantsToCloak(false);
      pSquad->setRecover(cRecoverAbility, pSquad->getPlayer()->getAbilityRecoverTime(abilityID), abilityID);
      pSquad->setAbilityDamageTakenModifier(pAbility->getDamageTakenModifier(),true);
      pSquad->setAbilityDodgeModifier(pAbility->getDodgeModifier(), true);
         
      updateVisuals(pSquad, false);

      playCloakSound(false);
   }
}
//==============================================================================
//==============================================================================
void BSquadActionCloak::detect()
{
   if (mpProtoAction->getFlagDisabled())
      return;

   BSquad* pSquad = mpOwner->getSquad();
   BASSERT(pSquad);

   mDetectedCountdown=gDatabase.getReCloakingDelay();
   if (pSquad && !pSquad->getFlagCloakDetected() )
   {
      pSquad->setFlagCloakDetected(true);

      updateVisuals(pSquad, true);
   }
}
//==============================================================================
//==============================================================================
void BSquadActionCloak::undetect()
{
   if (mpProtoAction->getFlagDisabled())
      return;

   BSquad* pSquad = mpOwner->getSquad();
   BASSERT(pSquad);

   if (pSquad && pSquad->getFlagCloakDetected())
   {
      pSquad->setFlagCloakDetected(false);

      updateVisuals(pSquad, true);
   }
}

//==============================================================================
//==============================================================================
bool BSquadActionCloak::unitMaskReset(BGrannyInstance* pGrannyInstance, const BBitArray& previousMask)
{
   if (!pGrannyInstance)
      return false;

   // don't let the mask resets control these granny instances - set it back
   pGrannyInstance->setMeshRenderMask(previousMask);
   return true;
}

//==============================================================================
//==============================================================================
void BSquadActionCloak::updateVisuals(BSquad* pSquad, bool bEnemyOnly)
{
   if (mpProtoAction->getFlagDisabled())
      return;

   BPlayer * pSquadPlayer = pSquad->getPlayer();  
   BPlayer * pPlayer = gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayer();
   bool bIsAlly = pPlayer->isAlly(pSquadPlayer, true);

   for (uint i = 0; i < pSquad->getNumberChildren(); ++i)
   {
      BObject* pUnit = gWorld->getObject(pSquad->getChild(i));
      if (!pUnit)
         continue;

      // attach the cloak effect
      if (!bIsAlly || !bEnemyOnly)
      {
         if (mFXAttachment[i].isValid())
         {
            BEntity *pAttachEntity = gWorld->getEntity(mFXAttachment[i]);
            if (pAttachEntity != NULL)
               pAttachEntity->kill(false);
            mFXAttachment[i].invalidate();
         }

         BASSERT(mpProtoAction->getProtoObject() != cInvalidObjectID); // If this fires, the entry for this action in the tactics file is setup wrong: it must have a protoobject specified
         int cloakProtoID = mpProtoAction->getProtoObject();
         mFXAttachment[i] = pUnit->addAttachment(cloakProtoID);
      }
             
      BVisual* pVisual = pUnit->getVisual();
      if (pVisual)
      {
         BGrannyInstance * pGrannyInstance = (BGrannyInstance*)(pVisual->mpInstance);
         BASSERT(pGrannyInstance);
         if (pGrannyInstance)
         {
            BBitArray mask = pGrannyInstance->getMeshRenderMask();

            if (pSquad->getFlagCloaked())
            {
               // [11-7-08] Fix for cloaked unit OOS
               mask.clearBit(0);
               mask.setBit(1);

               if (bIsAlly || pSquad->getFlagCloakDetected())
               {
                  pUnit->setFlagNoRender(false);
               }
               else
               {
                  pUnit->setFlagNoRender(true);
               }
            }
            else
            {
               mask.setBit(0);
               mask.clearBit(1);

               pUnit->setFlagNoRender(false);
            }

            pGrannyInstance->setMeshRenderMask(mask);
            pGrannyInstance->setMeshRenderMaskResetCallback(unitMaskReset);
         }
      }
   }  
}

//==============================================================================
//==============================================================================
void BSquadActionCloak::playCloakSound(bool cloak)
{
   if (mpProtoAction->getFlagDisabled())
      return;

   //-- Play the cloak sound on our children
//-- FIXING PREFIX BUG ID 2105
   const BSquad* pSquad = mpOwner->getSquad();
//--
   BASSERT(pSquad);
   uint numChildren = pSquad->getNumberChildren();
   for(uint i=0; i < numChildren; i++)
   {
      BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
      if(!pUnit)
         continue;


      BCueIndex index = cInvalidCueIndex;
      if(cloak)
         index = pUnit->getProtoObject()->getSound(cObjectSoundCloak);                                                      
      else
         index = pUnit->getProtoObject()->getSound(cObjectSoundUncloak);                                                      

      if(index != cInvalidCueIndex)
         gWorld->getWorldSoundManager()->addSound(pUnit, -1, index, true, cInvalidCueIndex, true, true);        
   }
}

//==============================================================================
//==============================================================================
bool BSquadActionCloak::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, DWORD, mActivateDelay);
   GFWRITEVAR(pStream, DWORD, mDetectedCountdown);
   GFWRITEARRAY(pStream, BEntityID, mFXAttachment, uint8, 10);
   GFWRITEBITBOOL(pStream, mFlagPermaCloaked);
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionCloak::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, DWORD, mActivateDelay);
   GFREADVAR(pStream, DWORD, mDetectedCountdown);
   GFREADARRAY(pStream, BEntityID, mFXAttachment, uint8, 10);
   GFREADBITBOOL(pStream, mFlagPermaCloaked);
   return true;
}

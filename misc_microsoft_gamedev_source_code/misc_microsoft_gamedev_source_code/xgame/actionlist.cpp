//==============================================================================
// actionlist.cpp
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "actionlist.h"
#include "action.h"
#include "world.h"
#include "actionmanager.h"
#include "entity.h"
#include "UnitOpportunity.h"
#include "unitactionrangedattack.h"

//BTK e3 Hack include
#include "tactic.h"

//==============================================================================
//==============================================================================
bool BActionList::addAction(BAction* pAction, BEntity* pEntity, BSimOrder* pOrder)
{
   if (!pAction || !pEntity)
      return (false);

   // Class type check
   bool passedClassCheck = false;
   long classType = pEntity->getClassType();
   switch (pAction->getType())
   {
      //-- entity actions
      case BAction::cActionTypeEntityIdle:
      case BAction::cActionTypeEntityListen:
         passedClassCheck = (classType != BEntity::cClassTypeObject);
         break;

      //-- unit actions
      case BAction::cActionTypeUnitMove:
      case BAction::cActionTypeUnitMoveAir:
      case BAction::cActionTypeUnitMoveWarthog:
      case BAction::cActionTypeUnitMoveGhost:
      case BAction::cActionTypeUnitRangedAttack:
      case BAction::cActionTypeUnitBuilding:
      case BAction::cActionTypeUnitDOT:
      case BAction::cActionTypeUnitChangeMode:
      case BAction::cActionTypeUnitDeath:
      case BAction::cActionTypeUnitInfectDeath:
      case BAction::cActionTypeUnitGarrison:
      case BAction::cActionTypeUnitUngarrison:
      case BAction::cActionTypeUnitShieldRegen:
      case BAction::cActionTypeUnitHonk:
      case BAction::cActionTypeUnitSpawnSquad:
      case BAction::cActionTypeUnitCapture:
      case BAction::cActionTypeUnitJoin:
      case BAction::cActionTypeUnitChangeOwner:
      case BAction::cActionTypeUnitAmmoRegen:
      case BAction::cActionTypeUnitPhysics:
      case BAction::cActionTypeUnitPlayBlockingAnimation:
      case BAction::cActionTypeUnitMines:
      case BAction::cActionTypeUnitDetonate:
      case BAction::cActionTypeUnitGather:
      case BAction::cActionTypeUnitCollisionAttack:
      case BAction::cActionTypeUnitAreaAttack:
      case BAction::cActionTypeUnitUnderAttack:
      case BAction::cActionTypeUnitSecondaryTurretAttack:
      case BAction::cActionTypeUnitRevealToTeam:
      case BAction::cActionTypeUnitAirTrafficControl:
      case BAction::cActionTypeUnitHitch:
      case BAction::cActionTypeUnitUnhitch:
      case BAction::cActionTypeUnitSlaveTurretAttack:
      case BAction::cActionTypeUnitThrown:
      case BAction::cActionTypeUnitDodge:
      case BAction::cActionTypeUnitDeflect:
      case BAction::cActionTypeUnitAvoidCollisionAir:
      case BAction::cActionTypeUnitPlayAttachmentAnims:
      case BAction::cActionTypeUnitHeal:
      case BAction::cActionTypeUnitRevive:
      case BAction::cActionTypeUnitBuff:
      case BAction::cActionTypeUnitInfect:
      case BAction::cActionTypeUnitHotDrop:
      case BAction::cActionTypeUnitTentacleDormant:
      case BAction::cActionTypeUnitHeroDeath:
      case BAction::cActionTypeUnitStasis:
      case BAction::cActionTypeUnitBubbleShield:
      case BAction::cActionTypeUnitBomb:
      case BAction::cActionTypeUnitPlasmaShieldGen:
      case BAction::cActionTypeUnitJump:
      case BAction::cActionTypeUnitAmbientLifeSpawner:
      case BAction::cActionTypeUnitJumpGather:
      case BAction::cActionTypeUnitJumpGarrison:
      case BAction::cActionTypeUnitJumpAttack:
      case BAction::cActionTypeUnitPointBlankAttack:
      case BAction::cActionTypeUnitRoar:
      case BAction::cActionTypeUnitEnergyShield:
      case BAction::cActionTypeUnitScaleLOS:
      case BAction::cActionTypeUnitChargedRangedAttack:
      case BAction::cActionTypeUnitTowerWall:
      case BAction::cActionTypeUnitAoeHeal:
      case BAction::cActionTypeUnitCoreSlide:
      case BAction::cActionTypeUnitInfantryEnergyShield:
      case BAction::cActionTypeUnitDome:
      case BAction::cActionTypeUnitRage:
         passedClassCheck = (classType == BEntity::cClassTypeUnit);
         break;

      //-- squad actions
      case BAction::cActionTypeSquadAttack:
      case BAction::cActionTypeSquadChangeMode:
      case BAction::cActionTypeSquadRepair:
      case BAction::cActionTypeSquadRepairOther:
      case BAction::cActionTypeSquadShieldRegen:
      case BAction::cActionTypeSquadGarrison:
      case BAction::cActionTypeSquadUngarrison:
      case BAction::cActionTypeSquadTransport:
      case BAction::cActionTypeSquadPlayBlockingAnimation:
      case BAction::cActionTypeSquadMove:
      case BAction::cActionTypeSquadReinforce:
      case BAction::cActionTypeSquadWork:
      case BAction::cActionTypeSquadCarpetBomb:
      case BAction::cActionTypeSquadHitch:
      case BAction::cActionTypeSquadUnhitch:
      case BAction::cActionTypeSquadDetonate:
      case BAction::cActionTypeSquadCloak:
      case BAction::cActionTypeSquadCloakDetect:
      case BAction::cActionTypeSquadDaze:
      case BAction::cActionTypeSquadJump:
      case BAction::cActionTypeSquadAmbientLife:
      case BAction::cActionTypeSquadReflectDamage:
      case BAction::cActionTypeSquadCryo:
      case BAction::cActionTypeSquadSpiritBond:
      case BAction::cActionTypeSquadWander: // Put this here because it actually is a squad action now
         passedClassCheck = (classType == BEntity::cClassTypeSquad);
         break;

      //-- platoon actions
      case BAction::cActionTypePlatoonMove:
         passedClassCheck = (classType == BEntity::cClassTypePlatoon);
         break;

      default:  
      {
         BSimString errorMsg;
         errorMsg.format("Unknown action type %d", (int)pAction->getType());
         BFAIL(errorMsg.getPtr());
         break;
      }
   }

   if (!passedClassCheck)
   {
      BSimString className;
      switch (classType)
      {
         case BEntity::cClassTypeObject:
            return false; // SLB: Just fail gracefully.
         case BEntity::cClassTypeUnit:
            className = "Unit";
            break;
         case BEntity::cClassTypeSquad:
            className = "Squad";
            break;
         case BEntity::cClassTypeDopple:
            className = "Dopple";
            break;
         case BEntity::cClassTypeProjectile:
            className = "Projectile";
            break;
         case BEntity::cClassTypePlatoon:
            className = "Platoon";
            break;
         case BEntity::cClassTypeArmy:
            className = "Army";
            break;
         default:
            className = "Invalid";
            break;
      }

      BSimString messageString;
      const BProtoObject* pProto = pEntity->getProtoObject();
      messageString.format("Trying to add an action to a class type it doesn't belong to. Action Type: %s   Class Type: %s   Proto Object: %s", 
         pAction->getTypeName(),
         className.getPtr(),
         pProto ? pProto->getName().getPtr() : "NULL");
      BASSERTM(0, messageString.getPtr());
      return false;
   }

   // SLB: Don't accept new actions once the game is over
   if (gWorld->getFlagGameOver() && !pAction->isAllowedWhenGameOver())
      return (false);

   //Check for conflicts on the list.
   for (uint i=0; i < mActions.getSize(); i++)
   {  
      bool result=pAction->conflicts(mActions[i]);
      if (result)
      {
         //Don't let idle actions override non idle actions.
         if (pAction->getType() == BAction::cActionTypeEntityIdle)
            return (false);         
         
         //Special case for attacks. If we can, just change the target
         if ((pAction->getType() == BAction::cActionTypeUnitRangedAttack) && 
            (mActions[i]->getType() == BAction::cActionTypeUnitRangedAttack) &&
            (mActions[i]->getProtoAction() == pAction->getProtoAction()))
         {                      
            BUnitActionRangedAttack* pAttackAction = reinterpret_cast<BUnitActionRangedAttack*>(mActions[i]);
            pAttackAction->changeTarget(pAction->getTarget(), pAction->getOppID(), pOrder);

            // TRB 11/21/08 - Don't want to inherit complete flag.  This is a special case flag that will only be
            // true if the opp goes away and the attack should complete its current attack anim.
            pAttackAction->setFlagCompleteWhenDoneAttacking(false);

            gActionManager.releaseAction(pAction);
            return true;
         }
         
         //Remove the conflict and we're good to go.
         removeActionByIndex(i);
         i--;
      }
   }

   //Attempt to connect it.  Fail if we can't.
   if (!pAction->connect(pEntity, pOrder))
      return (false);

   //Add it to the back.
   mActions.pushBack(pAction);

   return (true);
}

//==============================================================================
//==============================================================================
bool BActionList::removeAction(BAction *pAction)
{
   if (!pAction)
      return (false);

   //-- find the action first
   int index=mActions.find(pAction);
   if (index == cInvalidIndex)
      return (false);

   return(removeActionByIndex(index));
}

//==============================================================================
//==============================================================================
bool BActionList::removeActionByIndex(uint index)
{
   //Making this maintain action order on purpose.
   if (index >= mActions.getSize())
      return (false);

   BAction* pAction=mActions[index];
   //Remove it from the list.
   mActions.remove(pAction);
   //Release it back to the pool.
   gActionManager.releaseAction(pAction);

   return (true);
}

//==============================================================================
//==============================================================================
bool BActionList::removeActionByID(BActionID id)
{
   for (uint i=0; i < mActions.getSize(); i++)
   {
      if (mActions[i]->getID() == id)
         return(removeActionByIndex(i));
   }
   return (false);
}

//==============================================================================
//==============================================================================
bool BActionList::removeAllActionsOfType(BActionType type)
{
   bool found=false;
   for (uint i=0; i < mActions.getSize(); i++)
   {
      if (mActions[i]->getType() == type)
      {
         removeActionByIndex(i);
         i--;
         found=true;
      }
   }
   return(found);
}

//==============================================================================
//==============================================================================
bool BActionList::pauseAllActionsForOrder(BSimOrder* pOrder)
{
   if (!pOrder)
      return (false);

   bool found=false;
   for (uint i=0; i < mActions.getSize(); i++)
   {
      if (mActions[i]->getOrder() == pOrder)
      {
         mActions[i]->pause();
         found=true;
      }
   }
   return (found);
}

//==============================================================================
//==============================================================================
bool BActionList::unpauseAllActionsForOrder(BSimOrder* pOrder)
{
   if (!pOrder)
      return (false);

   bool found=false;
   for (uint i=0; i < mActions.getSize(); i++)
   {
      if (mActions[i]->getOrder() == pOrder)
      {
         mActions[i]->unpause();
         found=true;
      }
   }
   return (found);
}

//==============================================================================
//==============================================================================
bool BActionList::removeAllActionsForOrder(BSimOrder* pOrder)
{
   if (!pOrder)
      return(false);

   bool found=false;
   for (uint i=0; i < mActions.getSize(); i++)
   {
      if (mActions[i]->getOrder() == pOrder)
      {
         removeActionByIndex(i);
         i--;
         found=true;
      }
   }
   return(found);
}

//==============================================================================
//==============================================================================
bool BActionList::removeAllActionsForOpp(BUnitOpp* pOpp)
{
   if (!pOpp)
      return(false);

   bool found=false;
   for (uint i=0; i < mActions.getSize(); i++)
   {
      if (mActions[i]->getOppID() == pOpp->getID())
      {
         if(mActions[i]->getFlagPersistent())
         {
            // jce [10/14/2008] -- even though we can't remove it, disassociate the action with this opportunity.
            mActions[i]->setOppID(BUnitOpp::cInvalidID);
            
            // jce [10/14/2008] -- also, poke in a blank target since it would have been coming from this opp.
            BSimTarget target;
            mActions[i]->setTarget(target);
            
            continue;
         }

         //-- Ask the action if it's ok to remove it.
         //-- Only calling for the removeAllActionsForOpp() method for now.
         if(mActions[i]->attemptToRemove() == false)
         {
            found = true;
            continue;
         }
        
         removeActionByIndex(i);
         i--;
         found=true;
      }
   }
   return(found);
}

//==============================================================================
//==============================================================================
bool BActionList::removeAllConflicts(BAction* pAction)
{
   if (!pAction)
      return (false);

   bool found=false;
   for (uint i=0; i < mActions.getSize(); i++)
   {
      if (!mActions[i]->getFlagPersistent() && pAction->conflicts(mActions[i]))
      {
         removeActionByIndex(i);
         i--;
         found=true;
      }
   }

   return (found);
}

//==============================================================================
//==============================================================================
void BActionList::clearActions()
{
   for (uint i=0; i < mActions.getSize(); i++)
   {
      BAction* pAction=mActions[i];
      mActions.removeIndex(i);
      i--;
      gActionManager.releaseAction(pAction);
   }
}

//==============================================================================
//==============================================================================
void BActionList::clearNonPersistentActions()
{
   for (uint i=0; i < mActions.getSize(); i++)
   {
      BAction* pAction=mActions[i];
      if (pAction->getFlagPersistent())
         continue;

      mActions.removeIndex(i);
      i--;
      gActionManager.releaseAction(pAction);
   }
}

//==============================================================================
//==============================================================================
void BActionList::clearPersistentTacticActions()
{
   for (uint i=0; i < mActions.getSize(); i++)
   {
      BAction* pAction=mActions[i];
      if (pAction->getFlagPersistent() && pAction->getFlagFromTactic())
      {
         mActions.removeIndex(i);
         i--;
         gActionManager.releaseAction(pAction);
      }
   }
}

//==============================================================================
//==============================================================================
BAction* BActionList::getActionByID(BActionID id) const
{
   for (uint i=0; i < mActions.getSize(); i++)
   {
      if (mActions[i]->getID() == id)
         return (mActions[i]);
   }
   return (NULL);
}

//==============================================================================
//==============================================================================
BAction* BActionList::getActionByType(BActionType type, bool ignoreDisabled)
{
   for (uint i=0; i < mActions.getSize(); i++)
   {
      BAction* pAction=mActions[i];
      if ((pAction->getType()==type && !pAction->getProtoAction()) || (pAction->getType()==type && pAction->getProtoAction()))
      {
         if (!pAction->getProtoAction() || ignoreDisabled || (!ignoreDisabled && !pAction->getProtoAction()->getFlagDisabled()))
            return (pAction);
      }
   }
   return (NULL);
}

//==============================================================================
//==============================================================================
const BAction* BActionList::getActionByType(BActionType type, bool ignoreDisabled) const
{
   for (uint i=0; i < mActions.getSize(); i++)
   {
      const BAction* pAction=mActions[i];
      if ((pAction->getType()==type && !pAction->getProtoAction()) || (pAction->getType()==type && pAction->getProtoAction()))
      {
         if (!pAction->getProtoAction() || ignoreDisabled || (!ignoreDisabled && !pAction->getProtoAction()->getFlagDisabled()))
            return (pAction);
      }
   }
   return (NULL);
}

//==============================================================================
//==============================================================================
void BActionList::update(float elapsed)
{
   SCOPEDSAMPLE(BActionListUpdate);

   bool gameOver = gWorld->getFlagGameOver();

   for (uint i=0; i < mActions.getSize(); i++)
   {
      BAction* pAction=mActions[i];

      //SLB: If the game is over, kill the action
      if (gameOver && !pAction->isAllowedWhenGameOver())
      {
         removeActionByIndex(i);
         i--;
         continue;
      }

      //If the action has no owner, nuke it.
      if (!pAction->getOwner())
      {
         removeActionByIndex(i);
         i--;
         continue;
      }
      //If the action has it's destroy flag set, nuke it.
      if (pAction->getFlagDestroy())
      {
         removeActionByIndex(i);
         i--;
         continue;
      }
      //If the action is inactive, skip it.
      if (!pAction->getFlagActive())
         continue;

      //Do the update.
      bool keepCurrentAction=pAction->update(elapsed);
      
      //Kill virtually everything if the owner is dead.
      if (keepCurrentAction && pAction->getOwner()==NULL)
         keepCurrentAction=false;
      if (keepCurrentAction && 
         !pAction->getOwner()->isAlive() &&
         (pAction->getType() != BAction::cActionTypeUnitDeath) &&
         (pAction->getType() != BAction::cActionTypeUnitPhysics) &&
         (pAction->getType() != BAction::cActionTypeUnitAmbientLifeSpawner) &&
         (pAction->getType() != BAction::cActionTypeUnitScaleLOS))
         keepCurrentAction=false;

      //If we overly mucked with the list during this update, bail on the rest of it.
      bool listChangedDuringUpdate=(i >= mActions.size() || mActions[i] != pAction);

      //If we're supposed to nuke the current action, do it.
      if (listChangedDuringUpdate)
      {
         if (!keepCurrentAction || pAction->getFlagDestroy())
            removeAction(pAction);

         //Bail since the list changed.
         return;
      }
      else
      {
         if (!keepCurrentAction || pAction->getFlagDestroy())
         {
            removeActionByIndex(i);
            i--;
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BActionList::notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2)
{
   for (uint i=0; i < mActions.size(); i++)
   {
      // SLB: Don't notify dead actions.
      BAction* pAction = mActions[i];
      if (!pAction->getFlagDestroy() && pAction->getOwner())
         mActions[i]->notify(eventType, senderID, data, data2);
   }
}

//==============================================================================
//==============================================================================
bool BActionList::hasConflictsWithType(BActionType type) const
{
   //Check for conflicts on the list.
   for (uint i=0; i < mActions.getSize(); i++)
   {
      bool result=mActions[i]->conflicts(type);
      if (result)
         return (true);
   }

   return (false);
}

//==============================================================================
//==============================================================================
void BActionList::debugRender()
{
   for (uint i=0; i < mActions.getSize(); i++)
      mActions[i]->debugRender();
}


//==============================================================================
//==============================================================================
bool BActionList::changeOrderForAllActions(BSimOrder* pOldOrder, BSimOrder* pNewOrder)
{
   bool found = false;
   uint numActions = mActions.getSize();
   for(uint i = 0; i < numActions; i++)
   {
      BAction* pAction = mActions[i];
      if(!pAction)
         continue;

      if(pAction->getOrder() == pOldOrder)
      {         
         pAction->setOrder(pNewOrder);
         pOldOrder->decrementRefCount();
         pNewOrder->incrementRefCount();
         found = true;
      }      
   }
   return found;
}

//==============================================================================
//==============================================================================
bool BActionList::save(BStream* pStream, int saveType) const
{
   uint actionCount = mActions.size();
   GFWRITEVAL(pStream, uint8, actionCount);
   GFVERIFYCOUNT(actionCount, 200);
   for (uint i=0; i<actionCount; i++)
   {
      BAction* pAction = mActions[i];
      if (pAction)
      {
         GFWRITEVAL(pStream, uint8, i);
         GFWRITEACTIONPTR(pStream, pAction);
      }
   }
   GFWRITEVAL(pStream, uint8, UINT8_MAX);
   return true;
}

//==============================================================================
//==============================================================================
bool BActionList::load(BStream* pStream, int saveType)
{
   uint actionCount;
   GFREADVAL(pStream, uint8, uint, actionCount);
   GFVERIFYCOUNT(actionCount, 200);
   if (!mActions.setNumber(actionCount))
      return false;
   for (uint i=0; i<actionCount; i++)
      mActions[i] = NULL;
   uint loadedCount=0;
   uint actionIndex;
   GFREADVAL(pStream, uint8, uint, actionIndex);
   while (actionIndex != UINT8_MAX)
   {
      if (actionIndex >= actionCount)
      {
         GFERROR("GameFile Error: invalid action Index %u", actionIndex);
         return false;
      }
      if (loadedCount >= actionCount)
      {
         GFERROR("GameFile Error: too many actions");
         return false;
      }
      BAction* pAction;
      GFREADACTIONPTR(pStream, pAction);
      mActions[actionIndex] = pAction;
      loadedCount++;
      GFREADVAL(pStream, uint8, uint, actionIndex);
   }
   return true;
}

//==============================================================================
//==============================================================================
bool BActionList::postLoad(int saveType)
{
   for (uint i=0; i < mActions.getSize(); i++)
   {
      if (!mActions[i]->postLoad(saveType))
         return false;
   }
   return true;
}

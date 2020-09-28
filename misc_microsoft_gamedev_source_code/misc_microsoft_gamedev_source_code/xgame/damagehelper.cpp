//==============================================================================
// damagehelper.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#include "common.h"
#include "syncmacros.h"
#include "damagehelper.h"
#include "entity.h"
#include "unit.h"
#include "visual.h"
#include "unit.h"
#include "tactic.h"
#include "weapontype.h"
#include "unitquery.h"
#include "world.h"
#include "protoobject.h"
#include "config.h"
#include "configsgame.h"
#include "usermanager.h"
#include "user.h"
#include "selectionmanager.h"
#include "physics.h"
#include "physicsobject.h"
#include "squadactionattack.h"
#include "scoremanager.h"
#include "unitactionchargedrangedattack.h"
#include "simhelper.h"

//==============================================================================
//==============================================================================
bool BDamageHelper::doesDamageWithWeaponTypeKillUnit(BPlayerID attackerPlayerID, BTeamID attackerTeamID, BEntityID targetID, IDamageInfo* pDamageInfo,
                                            float damageAmount, long weaponType, bool isDirectionalDamage, BVector direction,
                                            float distanceFactor, BVector physicsForceOrigin, BEntityID attackerID)
{
   BASSERT(targetID != cInvalidObjectID);   

   BUnit *pTarget = gWorld->getUnit(targetID);
   if(!pTarget || (pTarget->isGarrisoned() && !pTarget->isInCover()))
      return false;

   // Check for damage proxy
   if (pTarget->getParentSquad() && pTarget->getParentSquad()->hasDamageProxy())
   {
//-- FIXING PREFIX BUG ID 1659
      const BSquad* pDmgProxy = gWorld->getSquad(pTarget->getParentSquad()->getDamageProxy());
//--  
      if (pDmgProxy && pDmgProxy->isAlive() && pDmgProxy->getNumberChildren() > 0)
      {
         // If we have a damage proxy, then just return false.. no matter how much damage you going to do,
         // you're not going to kill the unit. 
         // pTarget = gWorld->getUnit(pDmgProxy->getChild(0));
         return false;
      }
   }

   if(!pTarget || (pTarget->isGarrisoned() && !pTarget->isInCover()))
      return false;

   //-- Get the weaponType modifiers to our damage
   float shieldedModifier;
   float weaponTypeModifier = gWorld->getPlayer(attackerPlayerID)->getDamageModifier(weaponType, targetID,
      isDirectionalDamage ? direction : XMVectorNegate(pTarget->getForward()), shieldedModifier);

   //-- Get a pointer to the attacker unit
   BUnit* pUnit = gWorld->getUnit(attackerID);
   //-- See if there is some extra damage in our squad's damage bank that we can dish out
   useDamageBank(pUnit, damageAmount, false);

   //-- Setup our damage
   BDamage dmg;
   dmg.mAttackerID = attackerID;
   dmg.mAttackerTeamID = attackerTeamID;
   dmg.mDamage = damageAmount;
   dmg.mDamageMultiplier = weaponTypeModifier;
   // [2 APR 08]: BSR - At Tim's request, damage to shields uses base damage modifier rather than shield modifier
//   dmg.mShieldDamageMultiplier = shieldedModifier;
   dmg.mDirectional = isDirectionalDamage;
   dmg.mDirection = direction;
   dmg.mHitZoneIndex = -1;
   dmg.mpDamageInfo = pDamageInfo;
   dmg.mDamagePos = physicsForceOrigin;
   dmg.mDistanceFactor = distanceFactor;

   //-- Apply the damage
   if (!pTarget->getFlagBuilt())         
      dmg.mDamageDealt = dmg.mDamage * dmg.mDamageMultiplier * gDatabase.getConstructionDamageMultiplier(); 
   else
   {
      if (pTarget->getFlagHasShield() && (pTarget->getFlagFullShield() || !dmg.mDirectional || (dmg.mDirection.dot(pTarget->getForward()) <= 0.0f)))
      {
//         dmg.mShieldDamageDealt = Math::Min(dmg.mDamage * dmg.mShieldDamageMultiplier * pTarget->getDamageTakenScalar(), pTarget->getShieldpoints());
//         dmg.mDamageDealt = (dmg.mDamage - (dmg.mShieldDamageDealt * (1.0f / dmg.mShieldDamageMultiplier))) * dmg.mDamageMultiplier;
         dmg.mShieldDamageDealt = Math::Min(dmg.mDamage * dmg.mDamageMultiplier * pTarget->getDamageTakenScalar(), pTarget->getShieldpoints());
         dmg.mDamageDealt = (dmg.mDamage - (dmg.mShieldDamageDealt * (1.0f / dmg.mDamageMultiplier))) * dmg.mDamageMultiplier;
      }
      else            
         dmg.mDamageDealt = dmg.mDamage * dmg.mDamageMultiplier;
   }
   dmg.mDamageDealt *= pTarget->getDamageTakenScalar();

   return (dmg.mDamageDealt >= pTarget->getHitpoints());
}

//==============================================================================
//==============================================================================
float BDamageHelper::doDamageWithWeaponType(BPlayerID attackerPlayerID, BTeamID attackerTeamID, BEntityID targetID, IDamageInfo* pDamageInfo,
   float damageAmount, long weaponType, bool isDirectionalDamage, BVector direction,
   float distanceFactor, BVector physicsForceOrigin, BEntityID projectileID, BEntityID attackerID, bool* unitKilled,
   float* excessDamageOut, long hitZoneIndex /*= -1*/, bool doLogging /*=false*/, float* actualDamageDealt /*= NULL*/)
{
   BASSERT(targetID != cInvalidObjectID);   

   syncUnitActionData("BUnitActionRangedAttack::doDamageWithWeaponType attackerID", attackerID.asLong());
   syncUnitActionData("BUnitActionRangedAttack::doDamageWithWeaponType targetID", targetID.asLong());

   BUnit *pTarget = gWorld->getUnit(targetID);
   if(!pTarget || (pTarget->isGarrisoned() && !pTarget->isInCover()))
      return 0.0f;

   // Check for damage proxy
   if (pTarget->getParentSquad() && pTarget->getParentSquad()->hasDamageProxy())
   {
//-- FIXING PREFIX BUG ID 1663
      const BSquad* pDmgProxy = gWorld->getSquad(pTarget->getParentSquad()->getDamageProxy());
//--
      if (pDmgProxy && pDmgProxy->isAlive() && pDmgProxy->getNumberChildren() > 0)
      {
         pTarget = gWorld->getUnit(pDmgProxy->getChild(0));
      }
   }

   if(!pTarget || (pTarget->isGarrisoned() && !pTarget->isInCover()))
      return 0.0f;

   //-- Get the weaponType modifiers to our damage
   float shieldedModifier = 1.0f;
   float weaponTypeModifier = 1.0f;
   // MS 10/14/2008: PHX-15154, check player against NULL before using it
   if(gWorld->getPlayer(attackerPlayerID))
   {
      weaponTypeModifier = gWorld->getPlayer(attackerPlayerID)->getDamageModifier(weaponType, targetID,
         isDirectionalDamage ? direction : XMVectorNegate(pTarget->getForward()), shieldedModifier);
   }



   //-- Get a pointer to the attacker unit
   BUnit* pUnit = gWorld->getUnit(attackerID);
   //-- See if there is some extra damage in our squad's damage bank that we can dish out
   useDamageBank(pUnit, damageAmount);

   //-- Setup our damage
   BDamage dmg;
   dmg.mAttackerID = attackerID;
   dmg.mAttackerTeamID = attackerTeamID;
   dmg.mDamage = damageAmount;
   dmg.mDamageMultiplier = weaponTypeModifier;
//   dmg.mShieldDamageMultiplier = shieldedModifier;
   dmg.mDirectional = isDirectionalDamage;
   dmg.mDirection = direction;
   dmg.mHitZoneIndex = hitZoneIndex;
   dmg.mpDamageInfo = pDamageInfo;
   dmg.mDamagePos = physicsForceOrigin;
   dmg.mDistanceFactor = distanceFactor;
   dmg.mWeaponType = weaponType;
   
   if (pDamageInfo)
      dmg.mDOTEffect = pDamageInfo->getDOTEffect(pTarget);
   else
      dmg.mDOTEffect = cInvalidObjectID;

   syncUnitActionData("BUnitActionRangedAttack::doDamageWithWeaponType damageAmount", damageAmount);

   if (pDamageInfo)
   {
      float DOTrate = pDamageInfo->getDOTrate();
      if (DOTrate > 0.0f)
      {
         dmg.mDOTrate = DOTrate;
         dmg.mDOTduration = pDamageInfo->getDOTduration();
      }

      // DMG NOTE: Holy crap I wish there was another way to do this...
//-- FIXING PREFIX BUG ID 1666
      const BProtoAction* pProtoAction = (BProtoAction*)pDamageInfo;
//--

      BUnitActionChargedRangedAttack *pAttackAction = NULL;

      if (pUnit)
      {
         pAttackAction = (BUnitActionChargedRangedAttack*)pUnit->getActionByType(BAction::cActionTypeUnitChargedRangedAttack);
      }

      if (pProtoAction)
      {
         BSquad* pSquad = pTarget->getParentSquad();
         BProjectile* pAttackProjectile = gWorld->getProjectile(projectileID);

         syncUnitActionData("BUnitActionRangedAttack::doDamageWithWeaponType projectileID", projectileID.asLong());
         if (pAttackProjectile)
            syncUnitActionData("BUnitActionRangedAttack::doDamageWithWeaponType alreadyDazed", pAttackProjectile->getFlagTargetAlreadyDazed());

         // Daze for projectile attacks
         bool daze = false;
         if (pSquad && pAttackProjectile && pProtoAction->getFlagDaze() && !pAttackProjectile->getFlagTargetAlreadyDazed() && pTarget->isType(pProtoAction->getDazeTargetTypeID())) 
         {
            daze = true;
         }
         // Daze for non-projectile attacks
         else if (pSquad && pProtoAction->getFlagDaze() && !pAttackProjectile && pTarget->isType(pProtoAction->getDazeTargetTypeID())) 
         {
            daze = true;
         }
         
         bool pull = false;
         // Pull the unit if we're supposed to and the weapon is charged
         if (pAttackAction && pSquad && pProtoAction->getPullUnits())
         {
            if (pAttackAction)
            {
               pull = pAttackAction->canPull(pProtoAction, pTarget);

               if (pull)
               {
                  // Add pull attachment to squad
                  if (pProtoAction->getProtoObject() != cInvalidObjectID)
                  {
                     uint count = pSquad->getNumberChildren();
                     for (uint i = 0; i < count; ++i)
                     {
                        BUnit* pChild = gWorld->getUnit(pSquad->getChild(i));
                        if (pChild)
                        {
                           BVector fwd = pUnit->getPosition() - pChild->getPosition();
                           fwd.normalize();

                           BObjectCreateParms parms;
                           parms.mPlayerID = cGaiaPlayer;
                           parms.mPosition = pChild->getPosition();
                           parms.mRight = cXAxisVector;
                           parms.mForward = fwd;
                           parms.mProtoObjectID = pProtoAction->getProtoObject();
                           parms.mIgnorePop = true;
                           parms.mNoTieToGround = true;
                           parms.mPhysicsReplacement = false;
                           parms.mType = BEntity::cClassTypeObject;   
                           parms.mStartBuilt=true;
                           
                           gWorld->createObject(parms);
                        }
                     }
                  }
                  pSquad->pullSquad(attackerID, pProtoAction->getID());
                  dmg.mDamage = 0.0f;
                  pAttackAction->clearCharge();
               }
            }
         }

         if (daze && !pull)
         {
            syncUnitActionData("BUnitActionRangedAttack::doDamageWithWeaponType isDazed", pSquad->isDazed());

            if (pSquad->isDazed())
               pSquad->resetDaze(pProtoAction->getDazeDuration());
            else
            {
               bool smartTarget = false;

               BTactic* pTactic = pUnit ? pUnit->getTactic() : NULL;
               if (pTactic)
               {
                  const BWeapon* pWeapon = pTactic->getWeapon(pProtoAction->getWeaponID());
                  if (pWeapon && pWeapon->getSmartTargetType() != -1)
                     smartTarget = true;
               }

               pSquad->addDaze(pProtoAction->getDazeDuration(), pProtoAction->getDazeMovementModifier(), smartTarget);
            }

            // Alert the projectiles in the world
            BEntityHandle entityHandle = cInvalidObjectID;
            BProjectile* pProjectile = gWorld->getNextProjectile(entityHandle);

//-- FIXING PREFIX BUG ID 1665
            const BUnit* pTargetUnit = NULL;
//--
//-- FIXING PREFIX BUG ID 1664
            const BSquad* pTargetSquad = NULL;
//--

            if (!pProtoAction->getFlagAOEDaze())
            {
               syncUnitActionCode("BUnitActionRangedAttack::doDamageWithWeaponType setFlagTargetAlreadyDazed");
               pAttackProjectile->setFlagTargetAlreadyDazed(true);

               while (pProjectile)
               {
                  pTargetUnit = gWorld->getUnit(pAttackProjectile->getTargetObjectID());
                  
                  if (pTargetUnit)
                     pTargetSquad = pTargetUnit->getParentSquad();

                  if (pTargetSquad)
                  {
                     pProjectile->notify(BEntity::cEventTargetDazed, projectileID, pTargetSquad->getID(), NULL);
                  }
                  pProjectile = gWorld->getNextProjectile(entityHandle);
               }
            }
         }

         syncUnitActionCode("BUnitActionRangedAttack::doDamageWithWeaponType 2");

         // Pass on revive override flag from action/weapon
         dmg.mOverrideRevive = pProtoAction->getOverridesRevive();
      }
   }

   // Get damage type and half kill data
   int mode = pTarget->getDamageTypeMode();
   BDamageTypeID damageType = cInvalidDamageTypeID;
   if (pTarget->getProtoObject())
      damageType = pTarget->getProtoObject()->getDamageType(direction, pTarget->getForward(), pTarget->getRight(), false, true, mode);
   float halfKillCutoffFactor = -1.0f;
   if (pDamageInfo && pDamageInfo->getHalfKillCutoffFactor(damageType, halfKillCutoffFactor))
   {
      dmg.mHalfKill = true;
      dmg.mHalfKillCutoffFactor = halfKillCutoffFactor;
   }
   else
      dmg.mHalfKill = false;

   //-- Apply the damage
   float ret = pTarget->damage(dmg);

   if(dmg.mKilled)
   {
      if(unitKilled)
         *unitKilled = true;

      if(excessDamageOut)
         *excessDamageOut = damageAmount - ret;

      //gScoreManager.reportUnitKilled(attackerPlayerID, targetID, 1);
   }

   //-- Notify how much dmg the unit dealt
#if DPS_TRACKER
   if(pUnit)
      pUnit->damageDealt(damageAmount);
#endif

   #ifndef BUILD_FINAL
   //-- Logging
   if(gConfig.isDefined(cConfigDisplayCombatLog))
   {
      BUser* pUser=gUserManager.getUser(BUserManager::cPrimaryUser);
      if(pUser->getSelectionManager()->isUnitSelected(attackerID) ||
         (pUser->getSelectionManager()->getNumberSelectedUnits()==0 && pUser->getHoverObject()==attackerID))
         doLogging=true;
      if(doLogging)
      {
         BUnit *pAttacker = gWorld->getUnit(attackerID);
         BUnit *pUnit = gWorld->getUnit(targetID);
         BWeaponType *pWeaponType = gDatabase.getWeaponTypeByID(weaponType);
         BSimString attackerName  = (pAttacker == NULL) ? "NULL" : pAttacker->getProtoObject()->getName();
         BSimString targetName = (pUnit == NULL) ? "NULL" : pUnit->getProtoObject()->getName();         
         BSimString weaponTypeName = (pWeaponType == NULL) ? "NULL" : pWeaponType->getName();
         gConsole.output(cChannelCombat, "Unit: %s(%d) has damaged unit: %s(%d) @ Time: %1.3f",  attackerName.getPtr(), attackerID.getIndex(), targetName.getPtr(), targetID.getIndex(), gWorld->getGametimeFloat());
         gConsole.output(cChannelCombat, "Using Weapon Type: %s", weaponTypeName.getPtr());
         gConsole.output(cChannelCombat, "Base Damage: %1.2f, Applied Damage Multipliers (Shield / HP): %1.2fx / %1.2fx.", damageAmount, shieldedModifier, weaponTypeModifier);
         gConsole.output(cChannelCombat, "Shield Damage Dealt: %1.2f, HP Damage Dealt: %1.2f, Excess Base Damage: %1.2f", dmg.mShieldDamageDealt, dmg.mDamageDealt, damageAmount - ret);
         if(dmg.mKilled)
            gConsole.output(cChannelCombat, "Unit %s(%d) was killed.", targetName.getPtr(), targetID.getIndex());
         gConsole.output(cChannelCombat, " ");
      }
   }
   #endif

   // TRB 5/2/08
   // Actual hitpoint damage applied to the unit after multiplying in the various factors.
   if (actualDamageDealt != NULL)
      *actualDamageDealt = dmg.mDamageDealt + dmg.mShieldDamageDealt;

   // TRB 5/2/08
   // ret is the damage dealt relative to the base requested damage.  This is the value before BUnit::damage() applies the various factors to it.
   // This change was made to be more consistent with BUnit::damage() which returns base damage dealt and actual damage dealt is in the BDamage struct.
   return ret;
}

//==============================================================================
//==============================================================================
float BDamageHelper::doAreaEffectDamage(BPlayerID attackerPlayerID, BTeamID attackerTeamID, BEntityID attackerID, IDamageInfo* pDamageInfo,
   BVector aoeGroundZero, BEntityID projectileID, BVector direction, BEntityIDArray* pKilledUnits, BEntityID primaryTargetID,
   long hitZoneIndex /*= -1*/, bool doLogging /*=false*/, BEntityIDArray* pDamagedUnits/*=NULL*/)
{
   if (!pDamageInfo)
      return(0.0f);
   float damageAmount = pDamageInfo->getDamagePerSecond();
   long weaponType = pDamageInfo->getWeaponType();
   float aoeRadius = pDamageInfo->getAOERadius();
   float aoeDistanceRatio = pDamageInfo->getAOEDistanceFactor();
   float aoeDamageRatio = pDamageInfo->getAOEDamageFactor();
   bool friendlyFire = pDamageInfo->getFriendlyFire();
   float primaryTargetDamageRatio = pDamageInfo->getAOEPrimaryTargetFactor();
   return (BDamageHelper::doAreaEffectDamage(attackerPlayerID, attackerTeamID, attackerID, pDamageInfo, damageAmount,
      weaponType, aoeGroundZero, direction, aoeRadius, aoeDistanceRatio, aoeDamageRatio, friendlyFire,
      pKilledUnits, projectileID, primaryTargetID, primaryTargetDamageRatio, hitZoneIndex, doLogging, pDamagedUnits));
}

//==============================================================================
//==============================================================================
float BDamageHelper::doAreaEffectDamage(BPlayerID attackerPlayerID, BTeamID attackerTeamID, BEntityID attackerID, IDamageInfo* pDamageInfo,
   float damageOverride, BVector aoeGroundZero, BEntityID projectileID, BVector direction, BEntityIDArray* pKilledUnits,
   BEntityID primaryTargetID, long hitZoneIndex /*= -1*/, bool doLogging /*=false*/, BEntityIDArray* pDamagedUnits/*=NULL*/)
{
   if (!pDamageInfo)
      return(0.0f);
   float damageAmount = damageOverride;
   long weaponType = pDamageInfo->getWeaponType();
   float aoeRadius = pDamageInfo->getAOERadius();
   float aoeDistanceRatio = pDamageInfo->getAOEDistanceFactor();
   float aoeDamageRatio = pDamageInfo->getAOEDamageFactor();
   bool friendlyFire = pDamageInfo->getFriendlyFire();
   float primaryTargetDamageRatio = pDamageInfo->getAOEPrimaryTargetFactor();
   return (BDamageHelper::doAreaEffectDamage(attackerPlayerID, attackerTeamID, attackerID, pDamageInfo, damageAmount,
      weaponType, aoeGroundZero, direction, aoeRadius, aoeDistanceRatio, aoeDamageRatio, friendlyFire,
      pKilledUnits, projectileID, primaryTargetID, primaryTargetDamageRatio, hitZoneIndex, doLogging, pDamagedUnits));
}

//==============================================================================
//==============================================================================
float BDamageHelper::doAreaEffectDamage(BPlayerID attackerPlayerID, BTeamID attackerTeamID, BEntityID attackerID, IDamageInfo* pDamageInfo,
   float damageAmount, long weaponType, BVector aoeGroundZero, BVector direction, float aoeRadius,
   float aoeDistanceRatio, float aoeDamageRatio, bool friendlyFire, BEntityIDArray* killedUnits, BEntityID projectileID,
   BEntityID primaryTargetID, float primaryTargetDmgRatio, long hitZoneIndex /*= -1*/, bool doLogging /*=false*/, BEntityIDArray* pDamagedUnits/*=NULL*/)
{  

#ifndef BUILD_FINAL
   // jce [11/12/2008] -- Lookup by string was slow.  Mr. Stark says this is not needed any more.  If you're going to put it back,
   // consider making a formal (non-string) config for this.
   /*
   if (gConfig.isDefined("aoedebug"))
   {
      gpDebugPrimitives->addDebugSphere(aoeGroundZero, aoeRadius, cDWORDRed, BDebugPrimitives::cCategoryNone, 1.0f);
   }
   */
#endif

   #ifndef BUILD_FINAL
   if (gConfig.isDefined(cConfigDisplayCombatLog))
   {
      BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
      if (pUser->getSelectionManager()->isUnitSelected(attackerID) ||
         (pUser->getSelectionManager()->getNumberSelectedUnits() == 0 && pUser->getHoverObject() == attackerID))
         doLogging=true;
      if (doLogging)
      {
//-- FIXING PREFIX BUG ID 1667
         const BUnit* pAttacker = gWorld->getUnit(attackerID);
//--
//-- FIXING PREFIX BUG ID 1675
         const BUnit* pUnit = gWorld->getUnit(primaryTargetID);
//--
         BSimString attackerName = (pAttacker ? pAttacker->getProtoObject()->getName() : "NULL");
         BSimString targetName = (pUnit ? pUnit->getProtoObject()->getName() : "NULL");
         gConsole.output(cChannelCombat, "------- Start Area Effect Damage --------");
         gConsole.output(cChannelCombat, "Unit: %s(%d) is attacking unit: %s(%d) with AOE.",
            attackerName.getPtr(), attackerID.getIndex(), targetName.getPtr(), primaryTargetID.getIndex());
         gConsole.output(cChannelCombat, "Damage Amount: %1.2f", damageAmount);
         gConsole.output(cChannelCombat, "Base Radius: %1.2f", aoeRadius);
         gConsole.output(cChannelCombat, "Friendly Fire: %s", (friendlyFire ? "On" : "Off"));         
      }
   }   
   #endif

   //-- Track our total damage dealt
   float totalDamageGiven = 0.0f;
     
   //-- Calculate our base splash pool amount
   float splashPool = 0.0f;
   //-- If there is no primary target then the pool gets all the damage
   if (primaryTargetID == cInvalidObjectID)
      splashPool = damageAmount;
   else
      splashPool = (1.0f - primaryTargetDmgRatio) * damageAmount;
      
   #ifndef BUILD_FINAL
      //-- Logging
      if (doLogging)
         gConsole.output(cChannelCombat, "Original Splash Pool %1.2f", splashPool);
   #endif

   bool killedPrimaryTarget = false;

   //-- Damage our primary target
   if (primaryTargetID != cInvalidObjectID)
   {
      //-- Only do amount of dmg specifed for primary target
      float primaryTargetDamage = damageAmount * primaryTargetDmgRatio;
      float excessDamage = 0.0f;      

      totalDamageGiven += doDamageWithWeaponType(attackerPlayerID, attackerTeamID, primaryTargetID, pDamageInfo, primaryTargetDamage,
         weaponType, false, direction, primaryTargetDmgRatio, aoeGroundZero, projectileID, attackerID,
         &killedPrimaryTarget, &excessDamage, hitZoneIndex, doLogging);

      if (pDamagedUnits)
         pDamagedUnits->add(primaryTargetID);

      //-- Did we kill him?
      if (killedPrimaryTarget)
      {
         if (killedUnits)
            killedUnits->add(primaryTargetID);         
/*
         //-- Put our leftover damage into the splash pool, but divide by weapontype modifier first
         splashPool += excessDamage; -- DJB 11/20/06 - Disabled adding excess damage to splash pool per Tim's request.

#ifndef BUILD_FINAL
         //-- Logging
         if(doLogging)
         {
            gConsole.output(cChannelCombat, "Splash Pool Rollover %1.2f", excessDamage);
            gConsole.output(cChannelCombat, "New Splash Pool %1.2f", splashPool);
         }
#endif
*/
      }
   }
      
   BEntityIDArray results(0, 100);
   long numResults = 0;

   // For linear AOE, get units along the projectile direction starting at the impact point
   if (pDamageInfo->getFlagAOELinearDamage())
   {
      BVector segmentEndPt = aoeGroundZero;
      BVector segmentDir = direction;
      segmentDir.normalize();
      segmentDir *= aoeRadius;
      segmentEndPt += segmentDir;
      BSimHelper::getUnitsAlongSegment(aoeGroundZero, segmentEndPt, results);
      numResults = results.getNumber();
   }
   //-- Get the units in area that will be effected by dmg
   else
   {
      BUnitQuery query(aoeGroundZero, aoeRadius, true);
      //query.setFlagAllowObjects(true); //-- Return objects in our query so we can knock down trees.
      numResults = gWorld->getUnitsInArea(&query, &results);      
   }

   //-- Build a list of the units we're going to damage so we can properly spread out the damage
   BDynamicArray<BDamageStorage> unitsToDamage;
   float totalDamage = 0.0f;
   float outerDistance = aoeDistanceRatio * aoeRadius;

   for (long i = 0; i < numResults; i++)
   {
      BObject* pObject = gWorld->getObject(results[i]);
      if(!pObject)
         continue;

      //-- If this is not a unit, and the object is not destructible, or the projectile doesn't throw objects then there is no reason to go on.
      if(pObject->getUnit() == NULL) 
      {
         if(pDamageInfo->getThrowUnits() == false || pObject->getProtoObject()->getFlagDestructible() == false)
            continue;
      }      
      
      // Remove invulnerable
      if (pObject->getFlagInvulnerable())
         continue;

      if (pObject->getID() == attackerID)
         continue;

      // don't bother attempting to kill the primary target again
      if (killedPrimaryTarget && pObject->getID() == primaryTargetID)
         continue;

      //-- There is no need to ever collide with projectiles.
      if (pObject->getClassType() == BEntity::cClassTypeProjectile)
         continue;

      // See how far we are from the bounding box, approximately.
      float distanceFactor = 1.0f;
      float distanceFromCenterSqr = 0.0f;
      // Check for hit zone
      BBoundingBox obb;
      BVector      obbCenter;
      if (pObject->getUnit() && pObject->getUnit()->getHitZoneOBB( hitZoneIndex, obb ) )
      {
         obbCenter = obb.getCenter();
      }
      else
      {
         obb       = * pObject->getSimBoundingBox();
         obbCenter = obb.getCenter();
      }

      // Is the primary target an external shield
      BUnit* pTarget = gWorld->getUnit(primaryTargetID);

      // Check for damage proxy
      if (pTarget && pTarget->getParentSquad() && pTarget->getParentSquad()->hasDamageProxy())
      {
//-- FIXING PREFIX BUG ID 1676
         const BSquad* pDmgProxy = gWorld->getSquad(pTarget->getParentSquad()->getDamageProxy());
//--
         if (pDmgProxy && pDmgProxy->isAlive() && pDmgProxy->getNumberChildren() > 0)
         {
            pTarget = gWorld->getUnit(pDmgProxy->getChild(0));
         }
      }

      if (pTarget && pTarget->isExternalShield() && (pObject->getID() != primaryTargetID))
      {
         const BVector pos = pTarget->getPosition();
         float radiusY = pTarget->getObstructionRadiusY();
         float maxY = pos.y + radiusY;
         float minY = pos.y - radiusY;         

         // Is the center of the unit in the right vertical range to be inside the external shield (shield is not perfect sphere)
         if ((obbCenter.y <= maxY) && (obbCenter.y >= minY))
         {
            float distSqr = pos.distanceSqr(obbCenter);
            float radiusXSq = pTarget->getObstructionRadiusX();
            radiusXSq *= radiusXSq;
            // Is the center of the unit within the horizontal radius
            if (distSqr <= radiusXSq)
            {
               // Center of the unit is inside the external shield so no area damage should be applied
               continue;
            }
         }         
      }

#if 1 // SLB: New code that works
      float aoeRadiusSqr = aoeRadius * aoeRadius;

      // MS 5/22/2008: if we're ignoring the y-axis, match them up
      bool intersected;
      BVector groundZeroToBBoxCenter;
      if(pDamageInfo->getFlagAOEIgnoresYAxis())
      {
         BVector tempGroundZero = aoeGroundZero;
         tempGroundZero.y = obbCenter.y;
         groundZeroToBBoxCenter = XMVectorSubtract(obbCenter, tempGroundZero);
         intersected = obb.raySegmentIntersects(tempGroundZero, groundZeroToBBoxCenter, true, &aoeRadiusSqr, distanceFromCenterSqr);
      }
      else
      {
         groundZeroToBBoxCenter = XMVectorSubtract(obbCenter, aoeGroundZero);
         intersected = obb.raySegmentIntersects(aoeGroundZero, groundZeroToBBoxCenter, true, &aoeRadiusSqr, distanceFromCenterSqr);
      }

      // Make sure a primary target that receives a direct hit gets full AOE damage
      if ((distanceFromCenterSqr < 0.01f) || ((pObject->getID() == primaryTargetID) && (totalDamageGiven > cFloatCompareEpsilon)))
         distanceFromCenterSqr = 0.0f;
#else // SLB: Old code that doesn't work
      bool intersected = false;
      // This unit is the primary target and it has not yet received any damage
      if ((pObject->getID() == primaryTargetID) && (totalDamageGiven != 0.0f))
      {
         // Since we already know that we have hit the primary target there is no reason to do the ray intersect test
         intersected = true;
      }
      else
      {
         intersected = obb.raySegmentIntersects(aoeGroundZero, groundZeroToBBoxCenter, true, &aoeRadiusSqr, distanceFromCenterSqr);
      }
#endif

      if (!intersected)
         continue;

      //-- Don't mess with units which are too far away to be effected
      if (distanceFromCenterSqr > aoeRadiusSqr)
         continue;

      // We're within range, so get the non-squared distance for the rest of the checks.
      float distanceFromCenter = sqrtf(distanceFromCenterSqr);

      //-- Make sure we're not attacking our own or allies units.   
      if (friendlyFire == false)
      {
         if (pObject->getPlayerID() == attackerPlayerID)
            continue;
         if (pObject->getPlayer()->isAlly(attackerPlayerID))
            continue;
      }

      //-- Determine how much damage we need to do based on the distance from ground zero and, the distance and damage percentages specified.            
      if (aoeDistanceRatio != 0.0f)
      {
         if (distanceFromCenter <= outerDistance)
         {
            //Base damage area.
            distanceFactor = 1.0f - ((distanceFromCenter / outerDistance) * (1.0f - aoeDamageRatio));
         }
         else
         {
            //Outer damage area.
            distanceFactor = (1.0f - ((distanceFromCenter - outerDistance) / (aoeRadius - outerDistance))) * aoeDamageRatio;
         }
      }
      else
      {
         distanceFactor -= aoeDistanceRatio;
         if (distanceFactor > 1.0f)
            distanceFactor = 1.0f;
      }

      #ifndef BUILD_FINAL
      //-- Logging
      if (doLogging)
         gConsole.output(cChannelCombat, "Adding Unit %s(%d) as splash recipient with Damage Factor: %1.2f which is %1.2f from ground zero.",
            pObject->getProtoObject()->getName().getPtr(), pObject->getID().getIndex(), distanceFactor, distanceFromCenter);
      #endif

      float physicsDistanceFactor = distanceFactor;
      float damageModifiedByDistance = distanceFactor * damageAmount;

      //-- If it is a gaia unit then we can just dish up the full damage without worrying about the damage cap
      if ((pObject->getPlayerID() == 0) && !pObject->getProtoObject()->isType(gDatabase.getOTIDCover()))
      {
         doDamageWithWeaponType(attackerPlayerID, attackerTeamID, pObject->getID(), pDamageInfo, damageModifiedByDistance, weaponType, false,
            XMVectorZero(), physicsDistanceFactor, aoeGroundZero, projectileID, attackerID, 0, 0, hitZoneIndex, doLogging); //-- Don't count this toward total damage
      }
      else // -- Not Gaia
      {
         // For linear damage, sort the units by distance.  Use the damage factor to temporarily store the distance since damage amounts
         // will be calculated below.
         if (pDamageInfo->getFlagAOELinearDamage())
         {
            BDamageStorage damageStorage;
            damageStorage.mUnitID = pObject->getID();
            damageStorage.mDamage = distanceFromCenter;
            totalDamage += damageModifiedByDistance;

            uint insertIndex = 0;
            for ( ; insertIndex < unitsToDamage.getSize(); insertIndex++)
            {
               if (unitsToDamage[insertIndex].mDamage > distanceFromCenter)
                  break;
            }
            unitsToDamage.insertAtIndex(damageStorage, insertIndex);
         }
         else
         {
            //-- Store the damage to be done 
            BDamageStorage& damageStorage = unitsToDamage.grow();
            damageStorage.mUnitID = pObject->getID();
            damageStorage.mDamage = damageModifiedByDistance;
            totalDamage += damageModifiedByDistance;
         }
      }      
   }


   // Linear damage
   if (pDamageInfo->getFlagAOELinearDamage())
   {
      //-- We got our damage cap, and all the units we're gonna hurt, just run through the units and hurt them!
      uint numUnitsToDamage = (uint)unitsToDamage.getNumber();
      for (uint i = 0; i < numUnitsToDamage; i++)
      {
         float physicsDistanceFactor = 1.0f;

         //-- Apply the damage to the unit
//-- FIXING PREFIX BUG ID 1677
         const BUnit* pUnit = gWorld->getUnit(unitsToDamage[i].mUnitID);
//--
         if (!pUnit)
            continue;

         bool killed = false;
         float damageGiven = doDamageWithWeaponType(attackerPlayerID, attackerTeamID, pUnit->getID(), pDamageInfo, splashPool,
            weaponType, false, direction, physicsDistanceFactor, aoeGroundZero, projectileID, attackerID, &killed, NULL, hitZoneIndex, doLogging);
         totalDamageGiven += damageGiven;

         if (pDamagedUnits)
            pDamagedUnits->add(pUnit->getID());

         //-- Did we kill him?
         if (killed && killedUnits)
            killedUnits->add(pUnit->getID());

         // Remove damage dealt from the pool.  Stop dealing damage if the pool is zero
         splashPool -= damageGiven;
         if (splashPool < cFloatCompareEpsilon)
            break;
      }
   }

   else
   {
      //-- Cap our damage, apply weapon type, then deal the damage
      //-- Note: Damage cap occurs *before* we apply the weapon type bonus
      float reductionRatio = 1.0f;
      if (totalDamage > splashPool && (totalDamage > 0.0f || totalDamage < 0.0f))
         reductionRatio = splashPool / totalDamage;

      //-- We got our damage cap, and all the units we're gonna hurt, just run through the units and hurt them!
      uint numUnitsToDamage = (uint)unitsToDamage.getNumber();
      for (uint i = 0; i < numUnitsToDamage; i++)
      {
         //-- Cap the damage
         unitsToDamage[i].mDamage *= reductionRatio;

         float physicsDistanceFactor = unitsToDamage[i].mDamage;
         if (damageAmount > 0.0f)
            physicsDistanceFactor /= damageAmount;

         //-- Apply the damage to the unit
//-- FIXING PREFIX BUG ID 1678
         const BUnit* pUnit = gWorld->getUnit(unitsToDamage[i].mUnitID);
//--
         if (!pUnit)
            continue;

         bool killed = false;
         totalDamageGiven += doDamageWithWeaponType(attackerPlayerID, attackerTeamID, pUnit->getID(), pDamageInfo, unitsToDamage[i].mDamage,
            weaponType, false, direction, physicsDistanceFactor, aoeGroundZero, projectileID, attackerID, &killed, 0, hitZoneIndex, doLogging);

         if (pDamagedUnits)
            pDamagedUnits->add(pUnit->getID());

         //-- Did we kill him?
         if (killed && killedUnits)
            killedUnits->add(pUnit->getID());
      }
   }

   #ifndef BUILD_FINAL
      //-- Logging
      if (doLogging)
      {
         gConsole.output(cChannelCombat, "Total Damage Dealt: %1.2f", totalDamageGiven);
         gConsole.output(cChannelCombat, "------- End Area Effect Damage --------");
         gConsole.output(cChannelCombat, " ");
      }
   #endif

   return (totalDamageGiven);      
}

//==============================================================================
//==============================================================================
float BDamageHelper::getDamageAmount(BEntityID sourceId, float baseDamage, BEntityID targetId, bool usesHeightBonusDamage, bool displayLog)
{
//-- FIXING PREFIX BUG ID 1679
   const BUnit* pUnit = gWorld->getUnit(sourceId);
//--
   BDEBUG_ASSERT(pUnit);

   syncUnitActionData("BUnitActionRangedAttack::getDamageAmount owner ID", pUnit->getID().asLong());

   //Get the damage modifier.
   float damageModifier=pUnit->getDamageModifier();
   syncUnitActionData("BUnitActionRangedAttack::getDamageAmount damageModifier", damageModifier);
   if (damageModifier <= 0.0f)
      return(0.0f);

   //Get the base damage amount.
   float damageAmount = baseDamage;
   syncUnitActionData("BUnitActionRangedAttack::getDamageAmount damageAmount", damageAmount);

   //Add in our modifier.
   damageAmount*=damageModifier;

   //Add Height Bonus Damage.
   float heightBonus = 0.0f;
//-- FIXING PREFIX BUG ID 1680
   const BUnit* pTarget=gWorld->getUnit(targetId);   
//--
   if (pTarget && usesHeightBonusDamage)
   {
      float heightDiff = pUnit->getPosition().y-pTarget->getPosition().y;
      syncUnitActionData("BUnitActionRangedAttack::getDamageAmount heightDiff", heightDiff);
      if (heightDiff > 0.0f)
      {
         heightBonus = damageAmount*heightDiff*gDatabase.getHeightBonusDamage();
         damageAmount += heightBonus;
         syncUnitActionData("BUnitActionRangedAttack::getDamageAmount heightBonusDamage", gDatabase.getHeightBonusDamage());
      }
   }

   // Removing this debugging output at Tim's request since it's not actually doing damage here
   // and it gets confused with the actual damage output.
/*
#ifndef BUILD_FINAL
   if (displayLog && gConfig.isDefined(cConfigDisplayCombatLog))
   {
      BEntityID unitID = pUnit->getID();
      BUser* pUser = gUserManager.getUser(BUserManager::cPrimaryUser);
      if (pUser->getSelectionManager()->isUnitSelected(unitID) ||
         (pUser->getSelectionManager()->getNumberSelectedUnits() == 0 && pUser->getHoverObject() == unitID))
      {
         BSimString attackerName = pUnit->getProtoObject()->getName();
         BSimString targetName = (pTarget ? pTarget->getProtoObject()->getName() : "NULL");
         gConsole.output(cChannelCombat, "------- getDamageAmount --------");
         gConsole.output(cChannelCombat, "Unit: %s(%d) vs unit: %s(%d).",
            attackerName.getPtr(), unitID.getIndex(), targetName.getPtr(), targetId.getIndex());
         gConsole.output(cChannelCombat, "   Damage Amount: %1.2f", baseDamage);
         if (damageModifier != 1.0f)
            gConsole.output(cChannelCombat, "   Damage Modifier: %1.2f", damageModifier);
         if (heightBonus > 0.0f)
            gConsole.output(cChannelCombat, "   Height Bonus: %1.2f", heightBonus);
         if (damageAmount != baseDamage)
            gConsole.output(cChannelCombat, "Total Base Damage: %1.2f", damageAmount);
         gConsole.output(cChannelCombat, "------- End of getDamageAmount --------");
      }
   }   
#endif
*/
   return(damageAmount);
}

//==============================================================================
//==============================================================================
void BDamageHelper::useDamageBank(BUnit* pUnit, float &damageAmount, bool adjustBank)
{
   if(!pUnit)
      return;

   BSquad* pSquad = pUnit->getParentSquad();
   if(pSquad)
   {
      float damageAdjustment = 0.0f;

      float damageBank = pSquad->getDamageBank();
      float fabsDamageBank = Math::fAbs(damageBank);
      if (fabsDamageBank > cFloatCompareEpsilon) //-- Damage banks can be + or -
      {
         //-- Allow a designer specified max increase in damage %
         float maxBankDamage = damageAmount * gDatabase.getMaxDamageBankPctAdjust();            
         if(maxBankDamage <= fabsDamageBank)
         {
            damageAdjustment = maxBankDamage * damageBank / fabsDamageBank; //Keep the sign
         }
         else
         {
            damageAdjustment = damageBank;
         }
       
         //-- Remove it from the bank
         if (adjustBank)
            pSquad->adjustDamageBank(-damageAdjustment);
      }

      damageAmount += damageAdjustment;
   }
}

//==============================================================================
//==============================================================================
/*#ifndef BUILD_FINAL
float BDamageHelper::updateAttackTimer(BUnit *pUnit, float rate, float timer)
{
   //-- Do attacks even if we don't have animation setup. Just for development
   if(!pUnit)
      return timer;   

   float gameTimeFloat = (float)gWorld->getGametimeFloat();
   
   if((gameTimeFloat - timer) >= rate || timer == -1.0f)
   {
      //-- Send the attack event
      pUnit->notify(BEntity::cEventAnimAttackTag, pUnit->getID(), NULL, 0);
      pUnit->notify(BEntity::cEventAnimLoop, pUnit->getID(), NULL, 0);
      timer = gameTimeFloat;
   }
   return timer;
}
#endif
*/


//==============================================================================
//==============================================================================
BSmallDynamicSimArray<int> BDamageAreaOverTimeInstance::mActiveIndices;
IMPLEMENT_FREELIST(BDamageAreaOverTimeInstance, 4, &gSimHeap);

//==============================================================================
//==============================================================================
BDamageAreaOverTimeInstance::BDamageAreaOverTimeInstance() :
   mpDamageInfo(NULL),
   mAttackerObjectID(cInvalidObjectID),
   mAttackerPlayerID(cInvalidPlayerID),
   mAttackerTeamID(cInvalidTeamID),
   mTimer(0),
   mEndTime(0),
   mAttackerObjectInitialized(false),
   mDurationInitialized(false)
{
}

//==============================================================================
//==============================================================================
BDamageAreaOverTimeInstance::~BDamageAreaOverTimeInstance()
{
}

//==============================================================================
//==============================================================================
void BDamageAreaOverTimeInstance::reset()
{
   mDamagedEntityIDs.setNumber(0);
   mpDamageInfo = NULL;
   mAttackerObjectID = cInvalidObjectID;
   mAttackerPlayerID = cInvalidPlayerID;
   mAttackerTeamID = cInvalidTeamID;
   mTimer = 0;
   mEndTime = 0;
   mAttackerObjectInitialized = false;
   mDurationInitialized = false;
}

//==============================================================================
//==============================================================================
bool BDamageAreaOverTimeInstance::init(DWORD curTime, IDamageInfo* pDamageInfo, BPlayerID attackerPlayerID, BTeamID attackerTeamID, /*BVector location, */BEntityID attackerObjectID)
{
   if (!pDamageInfo)
      return false;

   mDamagedEntityIDs.setNumber(0);
   mTimer = curTime;
   mpDamageInfo = pDamageInfo;
   mAttackerObjectID = attackerObjectID;
   mAttackerPlayerID = attackerPlayerID;
   mAttackerTeamID = attackerTeamID;
   mEndTime = 0;
   mDurationInitialized = false;
   mAttackerObjectInitialized = false;

   // Set end time duration if specified, otherwise wait for the object to be set to
   // use its lifetime
   if (pDamageInfo->getShockwaveDuration() > 0)
   {
      mEndTime = curTime + pDamageInfo->getShockwaveDuration();
      mDurationInitialized = true;
   }

   if (mAttackerObjectID != cInvalidObjectID)
      mAttackerObjectInitialized = true;

   return true;
}

//==============================================================================
//==============================================================================
void BDamageAreaOverTimeInstance::setObjectID(BEntityID id)
{
   mAttackerObjectID = id;
   mAttackerObjectInitialized = true;
//-- FIXING PREFIX BUG ID 1655
   const BObject* pObject = gWorld->getObject(mAttackerObjectID);
//--
   if (pObject && pObject->getProtoObject() && !mDurationInitialized)
   {
      mEndTime = mTimer + pObject->getProtoObject()->getLifespan();
      mDurationInitialized = true;
   }
}

//==============================================================================
//==============================================================================
bool BDamageAreaOverTimeInstance::update(DWORD elapsedTime)
{
   // If not init'd or no time elapsed, early out
   if (!mAttackerObjectInitialized || !mDurationInitialized)
      return true;
   if (elapsedTime <= 0)
      return true;

   // Done if timer expired
   if (mTimer >= mEndTime)
      return false;

   // Update time
   mTimer += elapsedTime;

   // Get location / radius from attacker object
   BVector loc;
   float radius;
//-- FIXING PREFIX BUG ID 1657
   const BObject* pAttackerObject = gWorld->getObject(mAttackerObjectID);
//--
   if (pAttackerObject && pAttackerObject->getSimBoundingBox())
   {
      // We're specifically using the visual data here since the damage is tied
      // to the scaling size of the shockwave model.
      radius = pAttackerObject->getVisualRadius();
      loc = pAttackerObject->getVisualCenter();
   }
   else
      return false;

   // Get all units in area
   BEntityIDArray results(0, 100);
   BUnitQuery query(loc, radius, true);
   //query.setFlagAllowObjects(true); //-- Return objects in our query so we can knock down trees.
   long numResults = gWorld->getUnitsInArea(&query, &results);      

   // Get weapon data
   float aoeRadius = mpDamageInfo->getAOERadius();
   float aoeRadiusSqr = aoeRadius * aoeRadius;
   bool friendlyFire = mpDamageInfo->getFriendlyFire();
   float aoeDistanceRatio = mpDamageInfo->getAOEDistanceFactor();
   float aoeDamageRatio = mpDamageInfo->getAOEDamageFactor();
   float damagePerAttack = mpDamageInfo->getDamagePerSecond();
   float outerDistance = aoeDistanceRatio * aoeRadius;

   // Apply damage to all new units
   for (int i = 0; i < numResults; i++)
   {
      int pos = mDamagedEntityIDs.find(results[i]);
      if (cInvalidIndex == pos)
      {
         // Damage
         BEntity* pEntity = gWorld->getEntity(results[i]);
         if (pEntity && pEntity->getUnit())
         {
            BVector dir = pEntity->getPosition() - loc;
            dir.y = 0.0f;
            float distanceFromCenterSqr = dir.lengthSquared();
            if (!dir.safeNormalize())
               dir = cXAxisVector;

            // Check distance
            if (distanceFromCenterSqr > aoeRadiusSqr)
               continue;

            //-- Make sure we're not attacking our own or allies units.   
            if (!friendlyFire)
            {
               if (pAttackerObject)
               {
                  long attackerUnitPlayerID = pAttackerObject->getPlayerID();
                  if (pEntity->getPlayerID() == attackerUnitPlayerID)
                     continue;
                  if (pEntity->getPlayer()->isAlly(attackerUnitPlayerID))
                     continue;
               }
            }

            //-- Determine how much damage we need to do based on the distance from ground zero and, the distance and damage percentages specified.            
            float distanceFactor = 1.0f;
            if (aoeDistanceRatio > 0.0f)
            {
               // We're within range, so get the non-squared distance for the rest of the checks.
               float distanceFromCenter = sqrtf(distanceFromCenterSqr);

               //Base damage area.
               if (distanceFromCenter <= outerDistance)
                  distanceFactor = 1.0f - ((distanceFromCenter / outerDistance) * (1.0f - aoeDamageRatio));
               //Outer damage area.
               else
                  distanceFactor = (1.0f - ((distanceFromCenter - outerDistance) / (aoeRadius - outerDistance))) * aoeDamageRatio;
            }

            // Do damage
            float damageAmount = damagePerAttack * distanceFactor;
            float physicsDistanceFactor = distanceFactor;
           
            BDamageHelper::doDamageWithWeaponType(mAttackerPlayerID, mAttackerTeamID, results[i], mpDamageInfo, damageAmount, mpDamageInfo->getWeaponType(), true,
               dir, physicsDistanceFactor, loc, cInvalidObjectID);

            // Add to damaged array
            mDamagedEntityIDs.add(results[i]);
         }
      }
   }

   // If timer passed end time, we're done -> return false
   if (mTimer > mEndTime)
      return false;
   else
      return true;
}

//==============================================================================
//==============================================================================
void BDamageAreaOverTimeInstance::onAcquire()
{
   // Reset data
   reset();

   // Get index for this instance and add to active list
   uint freeListIndex = 0;
   if (BDamageAreaOverTimeInstance::mFreeList.getIndex(this, freeListIndex))
      mActiveIndices.add(freeListIndex);
}

//==============================================================================
//==============================================================================
void BDamageAreaOverTimeInstance::onRelease()
{
   // Remove index from active list
   uint freeListIndex = 0;
   if (BDamageAreaOverTimeInstance::mFreeList.getIndex(this, freeListIndex))
      mActiveIndices.removeValue(freeListIndex, false);
}

//==============================================================================
//==============================================================================
void BDamageAreaOverTimeInstance::updateAllActiveInstances()
{
   for (int i = mActiveIndices.getNumber() - 1; i >= 0; i--)
   {
      uint freeListIndex = mActiveIndices[i];
      BDamageAreaOverTimeInstance* pInst = BDamageAreaOverTimeInstance::mFreeList.get(freeListIndex);
      if (!pInst)
      {
         mActiveIndices.removeIndex(i);
         continue;
      }

      // Update
      bool result = pInst->update(gWorld->getLastUpdateLength());

      // Release instance
      if (!result)
         BDamageAreaOverTimeInstance::releaseInstance(pInst);
   }
}

//==============================================================================
//==============================================================================
void BDamageAreaOverTimeInstance::removeAllInstances()
{
   BDamageAreaOverTimeInstance::mFreeList.releaseAll();
   mActiveIndices.setNumber(0);
}

//==============================================================================
//==============================================================================
void setObjectForDamageAreaOverTimeInstance(const BObject* pObject, DWORD freeListIndex)
{
   if (!pObject)
      return;

   BDamageAreaOverTimeInstance* pInst = BDamageAreaOverTimeInstance::mFreeList.get(freeListIndex);
   if (pInst)
   {
      pInst->setObjectID(pObject->getID());
   }
}

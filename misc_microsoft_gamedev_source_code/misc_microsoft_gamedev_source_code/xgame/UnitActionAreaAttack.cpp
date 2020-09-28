//==============================================================================
// UnitActionAreaAttack.cpp
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

#include "common.h"
#include "UnitActionAreaAttack.h"
#include "Squad.h"
#include "Unit.h"
#include "tactic.h"
#include "damagehelper.h"
#include "unitquery.h"
#include "world.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionAreaAttack, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionAreaAttack::init()
{
   if (!BAction::init())
      return(false);
   mFlagPersistent = true;
   mAttackTimer = 0.0f;
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionAreaAttack::update(float elapsed)
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
//-- FIXING PREFIX BUG ID 1250
   const BSquad* pSquad = pUnit->getParentSquad();
//--
   int squadMode = mpProtoAction->getSquadMode();

   switch (mState)
   {
      case cStateNone:
      {
//-- FIXING PREFIX BUG ID 1249
         const BPlayer* pPlayer = pUnit->getPlayer();
//--
         const BProtoObject* pProtoObject = pPlayer ? pPlayer->getProtoObject(mpProtoAction->getProjectileID()) : gDatabase.getGenericProtoObject(mpProtoAction->getProjectileID());
         mFlagStickyProjectile = (pProtoObject && pProtoObject->getFlagIsSticky());
         setState(cStateWait);
         // Fall through to the cStateWait case
      }

      case cStateWait:
      {
         // Start attacking once in right mode
         if (squadMode != -1 && pSquad && pSquad->getSquadMode() != squadMode)
            return true;
         pUnit->setFlagDebugRenderAreaAttackRange(false);
         setState(cStateAttacking);
         // Fall through to the cStateAttacking case
      }

      case cStateAttacking:
      {
         // Stop attacking if not in right mode
         if (squadMode != -1 && pSquad && pSquad->getSquadMode() != squadMode)
         {
            pUnit->setFlagDebugRenderAreaAttackRange(false);
            setState(cStateWait);
            return true;
         }

         mAttackTimer += elapsed;
         if (mAttackTimer >= mpProtoAction->getAttackRate())
         {
            doAttack();
            mAttackTimer = 0.0f;
         }

         break;
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
void BUnitActionAreaAttack::doAttack()
{
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);

   // Calculate the damage amount.
   float damage = mpProtoAction->getDamagePerSecond() * mAttackTimer;

   BEntityIDArray damagedUnits;

   if (mpProtoAction->getAOERadius() > 0.0f)
   {
      // Do a constant area of effect damage.
      BDamageHelper::doAreaEffectDamage(pUnit->getPlayerID(), pUnit->getTeamID(), pUnit->getID(), (IDamageInfo*)mpProtoAction, damage, pUnit->getPosition(), cInvalidObjectID, cOriginVector, NULL, cInvalidObjectID, -1, false, &damagedUnits);
   }
   else
   { 
      /*
      // Search for enemy units to damage.
      float range = mpProtoAction->getMaxRange(NULL, false);
      BEntityIDArray units(0, 100);
      BUnitQuery query(pUnit->getPosition(), range, true, BDamageHelper::cMaxUnitsDamaged);
      query.setFlagIgnoreDead(true);
      query.setRelation(pUnit->getPlayerID(), cRelationTypeEnemy);
      gWorld->getUnitsInArea(&query, &units);

      // Damage the enemy units
      int unitCount = units.getNumber();
      for (long i=0; i<unitCount; i++)
      {
      BUnit* pEnemy=gWorld->getUnit(units[i]);
      if (pEnemy)
      BDamageHelper::doDamageWithWeaponType(pUnit->getPlayerID(), pUnit->getTeamID(), pEnemy->getID(), (IDamageInfo*)mpProtoAction, damage, mpProtoAction->getWeaponType(), false, cOriginVector, 1.0f, pUnit->getPosition(), pUnit->getID(), NULL, NULL, -1, false);
      }
      */

      // Search for enemy units to damage.
      float range = mpProtoAction->getMaxRange(NULL, false);
      BEntityIDArray squads(0, 100);
      BUnitQuery query(pUnit->getPosition(), range, true);
      query.setFlagIgnoreDead(true);
      query.setRelation(pUnit->getPlayerID(), cRelationTypeEnemy);
      gWorld->getSquadsInArea(&query, &squads, false);

      // Damage the enemy units
      uint squadCount = squads.getSize();
      for (uint i=0; i<squadCount; i++)
      {
//-- FIXING PREFIX BUG ID 1252
         const BSquad* pEnemySquad = gWorld->getSquad(squads[i]);
//--
         if (pEnemySquad)
         {
            uint unitCount = pEnemySquad->getNumberChildren();
            if (unitCount > 0)
            {
               float unitDamage = damage / unitCount;
               for (uint j=0; j<unitCount; j++)
               {
//-- FIXING PREFIX BUG ID 1251
                  const BUnit* pEnemyUnit = gWorld->getUnit(pEnemySquad->getChild(j));
//--
                  if (pEnemyUnit)
                  {
                     BDamageHelper::doDamageWithWeaponType(pUnit->getPlayerID(), pUnit->getTeamID(), pEnemyUnit->getID(), (IDamageInfo*)mpProtoAction, unitDamage, mpProtoAction->getWeaponType(), false, cOriginVector, 1.0f, pUnit->getPosition(), cInvalidObjectID, pUnit->getID(), NULL, NULL, -1, false);
                     damagedUnits.add(pEnemyUnit->getID());
                  }
               }
            }
         }
      }
   }

   // Add sticky projectiles if needed.
   if (mFlagStickyProjectile && damagedUnits.getSize() > 0)
   {
      BVector startPos = pUnit->getSimCenter();
      uint damagedCount = damagedUnits.getSize();
      for (uint i=0; i<damagedCount; i++)
      {
         BUnit* pDamagedUnit = gWorld->getUnit(damagedUnits[i]);
         if (pDamagedUnit && pDamagedUnit->isAlive())
         {
//-- FIXING PREFIX BUG ID 1253
            const BPlayer* pPlayer = pUnit->getPlayer();
//--
            const BProtoObject* pProjectileProto = pPlayer ? pPlayer->getProtoObject(mpProtoAction->getProjectileID()) : gDatabase.getGenericProtoObject(mpProtoAction->getProjectileID());
            if (pProjectileProto)
            {
               if (pProjectileProto->getFlagSingleStick())
               {
                  if (pDamagedUnit->getFirstEntityRefByData1(BEntityRef::cTypeStickedProjectile, (short)pProjectileProto->getID()) != NULL)
                     continue;
               }
               BObjectCreateParms parms;
               parms.mPlayerID = pUnit->getPlayerID();
               parms.mProtoObjectID = pProjectileProto->getID();
               parms.mPosition = pDamagedUnit->getPosition();
               parms.mForward = pDamagedUnit->getPosition() - pUnit->getPosition();
               parms.mForward.normalize();
               parms.mRight.assignCrossProduct(cYAxisVector, parms.mForward);
               parms.mRight.normalize();
               BProjectile* pProjectile = gWorld->createProjectile(parms);
               if (pProjectile)
               {
                  if (!pProjectile->stickToTarget(startPos, pDamagedUnit))
                     pProjectile->kill(true);
               }
            }
         }
      }
   }
}


//==============================================================================
//==============================================================================
bool BUnitActionAreaAttack::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, float, mAttackTimer);
   GFWRITEVAR(pStream, bool, mFlagStickyProjectile);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionAreaAttack::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, float, mAttackTimer);
   GFREADVAR(pStream, bool, mFlagStickyProjectile);
   return true;
}

//==============================================================================
// UnitActionCollisionAttack.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "UnitActionCollisionAttack.h"
#include "config.h"
#include "configsgame.h"
#include "damagehelper.h"
#include "game.h"
#include "protoObject.h"
#include "Squad.h"
#include "Tactic.h"
#include "Unit.h"
#include "UnitOpportunity.h"
#include "world.h"
#include "worldsoundmanager.h"
#include "unitActionThrown.h"
#include "weapontype.h"
#include "physics.h"
#include "pather.h"
#include "plane.h"
#include "ability.h"
#include "generaleventmanager.h"

//============================================================================
// Constants
const float cRammerImpulseAngle = cPiOver4;
const float cRammerImpulseForce = 10.0f;
const float cRammedImpulseAngle = cPiOver2 * 0.75f;
const float cRammedImpulseForce = 7.0f;
const float cTopYVelOfRammedUnit = 6.0f;
const float cMedYVelOfRammedUnit = 3.0f;

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionCollisionAttack, 5, &gSimHeap);

//==============================================================================
//==============================================================================
int __cdecl sortByHitpoints(const void* elem1, const void* elem2)
{
   BEntityID entityID1 = *(BEntityID*)elem1;
   BEntityID entityID2 = *(BEntityID*)elem2;

   BUnit* pUnit1 = gWorld->getUnit(entityID1);
   BUnit* pUnit2 = gWorld->getUnit(entityID2);

   if (!pUnit1 || !pUnit2)
   {
      // Push the NULL units to the end of the list
      if (pUnit1)
         return -1;
      if (pUnit2)
         return 1;

      return 0;
   }

   // Primary sorting key
   if (pUnit1->getHitpoints() < pUnit2->getHitpoints())
      return -1;
   if (pUnit1->getHitpoints() > pUnit2->getHitpoints())
      return 1;

   // Secondary sorting key
   if (entityID1 < entityID2)
      return -1;
   if (entityID1 > entityID2)
      return 1;

   // They shouldn't be equal
   BASSERT(0);
   return 0;
}

//==============================================================================
//==============================================================================
bool BUnitActionCollisionAttack::init()
{
   if (!BAction::init())
      return(false);

   mFlagPersistent=true;
   mEvadedUnits.clear();
   mpMoveOrder = NULL;
   mAbilityID = -1;
   mFlagIgnoreTarget = false;
   mFlagCollideWithFriendlies = false;
   mFlagIgnoreAmmo = false;
   mFlagUseObstructionForCollisions = false;

   return(true);
}


//==============================================================================
//==============================================================================
bool BUnitActionCollisionAttack::update(float elapsed)
{
   // Bomb check
   BUnit* pUnit = reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
//-- FIXING PREFIX BUG ID 2211
   const BSquad* pSquad = pUnit->getParentSquad();
//--
   BASSERT(pSquad);
   if (!pSquad)
      return false;

   // Early out if not in hit and run mode
   if (pSquad->getSquadMode() != BSquadAI::cModeHitAndRun)
   {
      mTarget.reset();
      if (mpMoveOrder)
      {
         mpMoveOrder->decrementRefCount();
         mpMoveOrder = NULL;
      }
      mAbilityID = -1;
      pUnit->clearTacticState(); // Not hit-and-run, go back to regular state
      return (true);
   }
   pUnit->setTacticState(mpProtoAction->getNewTacticState()); // in hit-and-run mode, set to hit-and-run state

   //BASSERT(mTarget.isValid());
   
   switch (mState)
   {
      case cStateNone:
         // Reset ammo
         pUnit->setAmmunition(pUnit->getAmmoMax());
         setState(cStateWorking);
         break;
         
      case cStateWorking:
      {
         //Get our velocity.
         BVector unitVelocity;
         BVector unitForward;
         BVector unitPosition;
         if (pUnit->getPhysicsObject())
         {
            unitVelocity=pUnit->getPhysicsVelocity();
            unitForward=pUnit->getPhysicsForward();
            unitPosition=pUnit->getPhysicsPosition();
         }
         else
         {
            unitVelocity=pUnit->getForwardVelocity();
            unitForward=pUnit->getForward();
            unitPosition=pUnit->getPosition();
         }
         //Bail if we're not moving.  Clear our evaded units.
         if (unitVelocity.length() < 0.5f)
         {
            mEvadedUnits.clear();
            break;
         }
      
         //See if we *will* hit anything in the "near" future.
         const BEntityIDArray& ignoreUnits=pSquad->getChildList();
         static BEntityIDArray collisions;
         BVector evadePosition;
         if (pUnit->getPhysicsObject())
            evadePosition = unitPosition + unitVelocity;
         else
         {
            float evadeDistance=unitVelocity.length()*1.0f;
            evadePosition = unitPosition + unitForward*evadeDistance;
         }
         collisions.clear();
         pUnit->checkForCollisions(ignoreUnits, unitPosition, evadePosition, 0, false, collisions, NULL, false, true);
         handleEvades(collisions, unitForward, unitPosition);
         
         //See if we hit anything right now.
         collisions.clear();
         BVector collisionPosition=unitPosition+unitForward;
         if (mFlagUseObstructionForCollisions)
         {
            checkForCollisionsWithObstruction(pUnit, collisions, ignoreUnits);
         }
         else
         {
            pUnit->checkForCollisions(ignoreUnits, unitPosition, collisionPosition, 0, false, collisions, NULL, false, true, true, true);
         }
         
         // [6/12/2008 CJS] See if we hit a covenant base shield from the outside since it isn't collidable (hackorific)
         /*BUnit *pTargetUnit = gWorld->getUnit(mTarget.getID());
         if (pTargetUnit)
         {
            if (!pTargetUnit->isExternalShield())
            {
               BEntityID base = pTargetUnit->getBaseBuilding();
               if (base != cInvalidObjectID)
               {
                  BUnit* pBase = gWorld->getUnit(base);
                  int numRefs = pBase->getNumberEntityRefs();
                  for (int i = 0; i < numRefs; ++i)
                  {
//-- FIXING PREFIX BUG ID 2210
                     const BEntityRef* pRef = pBase->getEntityRefByIndex(i);
//--
                     if (pRef->mType == BEntityRef::cTypeBaseShield)
                     {
                        BEntityID shieldID = pRef->mID;
                        if (shieldID != cInvalidObjectID)
                        {
                           BUnit* pShieldUnit = gWorld->getUnit(shieldID);
                           BASSERT(pShieldUnit);
                           if (pShieldUnit)
                           {
                              pTargetUnit = pShieldUnit;
                              mTarget.setID(shieldID);
                              mTarget.setPosition(pTargetUnit->getPosition());
                           }
                           break;
                        }
                     }
                  }
               }
            }

            if (pTargetUnit && pTargetUnit->isExternalShield())
            {
               const BBoundingBox* pBounds = pTargetUnit->getSimBoundingBox();
               const float* pExtents = pBounds->getExtents();
               BVector center = pBounds->getCenter();
               center.y -= pExtents[1] / 2.0f;
               BBoundingSphere sphere(pBounds->getCenter(), max(pExtents[0], pExtents[2]));

               bool intSphere = sphere.rayIntersectsSphere(unitPosition, unitForward);
               float dist;
               float checkDist = collisionPosition.distanceSqr(unitPosition);
               if (intSphere && pBounds->raySegmentIntersects(unitPosition, unitForward, true, &checkDist, dist))
               {
                  BPlane planes[4];
                  if (pBounds->getSidePlanes(planes, false))
                  {
                     const float* pExtents = pUnit->getSimBoundingBox()->getExtents();
                     float radius = max(pExtents[0], pExtents[2]);

                     int count = 0;
                     for (int i = 0; i < 4; ++i)
                     {
                        if (planes[i].checkSphere(pUnit->getSimBoundingBox()->getCenter(), radius) == BPlane::cBehindPlane)
                           ++count;
                     }

                     // We only hit if the colliding unit is completely on the outside (in which case they will not be behind all 4 planes)
                     if (count < 4)
                        collisions.add(pTargetUnit->getID());
                     else
                        return (true);
                  }
               }
            }
         }*/
         
         collisions.sort(sortByHitpoints); // Sort collisions by hitpoints, so the weakest things get killed first
         handleCollisions(collisions);

         // Debug rendering
         #ifndef BUILD_FINAL
            if (gConfig.isDefined(cConfigDebugDodge))
            {
               gTerrainSimRep.addDebugThickLineOverTerrain(unitPosition, evadePosition, 0.5f, cDWORDGreen, cDWORDGreen, 1.0f);
               gTerrainSimRep.addDebugThickLineOverTerrain(unitPosition, collisionPosition, 0.5f, cDWORDYellow, cDWORDYellow, 1.1f);
            }
         #endif

         break;
      }
   }

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionCollisionAttack::isBowlableOrRammable(BEntity* pEntity) const
{
   if (!pEntity)
      return false;

   bool bowlable = false;
   bool rammable = false;

   // Get weapon
//-- FIXING PREFIX BUG ID 2214
   const BWeaponType* pWeaponType = gDatabase.getWeaponTypeByID(mpProtoAction->getWeaponType());
//--
   BASSERT(pWeaponType);

   if (pEntity->getClassType() == BEntity::cClassTypeUnit)
      return isUnitBowlableOrRammable(pEntity->getUnit(), bowlable, rammable, true);
   else if (pEntity->getClassType() == BEntity::cClassTypeSquad)
   {
      // If any child is bowlable or rammable, the squad is
//-- FIXING PREFIX BUG ID 2213
      const BSquad* pSquad = pEntity->getSquad();
//--
      for (uint i = 0; i < pSquad->getNumberChildren(); i++)
      {
         BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
         if (pUnit && isUnitBowlableOrRammable(pUnit, bowlable, rammable, true))
            return true;
      }
   }

   return false;
}

//==============================================================================
//==============================================================================
bool BUnitActionCollisionAttack::isBowlable(BEntity* pEntity) const
{
   if (!pEntity)
      return false;

   bool bowlable = false;
   bool rammable = false;

   // Get weapon
//-- FIXING PREFIX BUG ID 2216
   const BWeaponType* pWeaponType = gDatabase.getWeaponTypeByID(mpProtoAction->getWeaponType());
//--
   BASSERT(pWeaponType);

   if (pEntity->getClassType() == BEntity::cClassTypeUnit)
   {
      isUnitBowlableOrRammable(pEntity->getUnit(), bowlable, rammable, true);
      return bowlable;
   }
   else if (pEntity->getClassType() == BEntity::cClassTypeSquad)
   {
      // All children must be bowlable for the squad to be bowlable
//-- FIXING PREFIX BUG ID 2215
      const BSquad* pSquad = pEntity->getSquad();
//--
      for (uint i = 0; i < pSquad->getNumberChildren(); i++)
      {
         BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
         if (pUnit)
         {
            isUnitBowlableOrRammable(pUnit, bowlable, rammable, true);
            if (!bowlable)
               return false;
         }
      }
      return true;
   }

   return false;
}

//==============================================================================
//==============================================================================
void BUnitActionCollisionAttack::handleEvades(const BEntityIDArray& collisions, BVector unitForward, BVector unitPosition)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
//-- FIXING PREFIX BUG ID 2219
   const BWeaponType* pWeaponType = gDatabase.getWeaponTypeByID(mpProtoAction->getWeaponType());
//--

   // Debug rendering
   #ifndef BUILD_FINAL
      bool debugRender = false;
      if (gConfig.isDefined(cConfigDebugDodge))
         debugRender = true;
   #endif

   //If the units are far enough away, think about telling them to evade.
   for (uint i=0; i < collisions.getSize(); i++)
   {
      BUnit* pTarget=gWorld->getUnit(collisions[i]);
      if (!pTarget || (pTarget->getPlayerID() == pUnit->getPlayerID()))
         continue;
      // If already evading, continue
      if (mEvadedUnits.contains(pTarget->getID()))
      {
         // Debug render
         #ifndef BUILD_FINAL
            if (debugRender)
               gTerrainSimRep.addDebugSquareOverTerrain(pTarget->getPosition(), pTarget->getForward(), cDWORDGreen, 1.3f, pTarget->getObstructionRadius() * 2.0f);
         #endif
         continue;
      }
      // Debug render
      #ifndef BUILD_FINAL
         if (debugRender)
            gTerrainSimRep.addDebugSquareOverTerrain(pTarget->getPosition(), pTarget->getForward(), cDWORDYellow, 1.3f, pTarget->getObstructionRadius() * 2.0f);
      #endif

      // Don't trigger evade for guys in cover
      if (pTarget->isInCover())
         continue;

      // Make sure target is an enemy
      if (!mFlagCollideWithFriendlies && !pTarget->getPlayer()->isEnemy(pUnit->getPlayerID()))
         continue;

      // Only attempt evade if it's target is a bowlable unit
      BVector targetDirection=pTarget->getPosition()-unitPosition;
      int mode = (pTarget ? pTarget->getDamageTypeMode() : 0);
      BDamageTypeID damageType = pTarget->getProtoObject()->getDamageType(targetDirection, pTarget->getForward(), pTarget->getRight(), false, true, mode);
      bool bowlable = pWeaponType->getBowlable(damageType);
      if (!bowlable)
         continue;

      // Get chance to evade.  As ram juice decreases, the chance increases to 100%
      float chanceToEvade = pTarget->getProtoObject()->getRamDodgeFactor();
      chanceToEvade = Math::Lerp(1.0f, chanceToEvade, pUnit->getAmmoPercentage());

      // Roll chance to evade.  If failed add to list of evaded units so it doesn't roll again
      float rand = getRandRangeFloat(cSimRand, 0.0f, 1.0f);
      if (rand > chanceToEvade)
      {
         mEvadedUnits.add(pTarget->getID());
         continue;
      }

      // Check height to make sure unit intersects in y
      if ((pTarget->getPosition().y - pTarget->getSimBoundingBox()->getSphereRadius()) >
          (pUnit->getPosition().y + pUnit->getSimBoundingBox()->getSphereRadius()))
          continue;

      // Check to see if this unit is being thrown.  If so, can't evade
      BSimTarget simTarget(pUnit->getID());
      const BUnitOpp* pThrownOpp = pTarget->getOppByTypeAndTarget(BUnitOpp::cTypeThrown, &simTarget);
      if (pThrownOpp)
         continue;

      //Is this unit on the right or left side.
      BVector normal=unitForward.cross(targetDirection);
      bool evadeLeft=true;
      if (normal.y < 0.0f)
         evadeLeft=false;

      //Figure out if this unit has an evade or run away anim.
      static BSmallDynamicSimArray<uint> validAnims;
      validAnims.clear();
      if (evadeLeft && pTarget->hasAnimation(cAnimTypeEvadeLeft))
         validAnims.add(cAnimTypeEvadeLeft);
      if (!evadeLeft && pTarget->hasAnimation(cAnimTypeEvadeRight))
         validAnims.add(cAnimTypeEvadeRight);
      //Weight retreat a bit more.
      if (pTarget->hasAnimation(cAnimTypeRetreat))
      {
         validAnims.add(cAnimTypeRetreat);
         validAnims.add(cAnimTypeRetreat);
      }

      //If we have a valid anim, use it.
      if (validAnims.getSize() > 0)
      {
         BUnitOpp* pNewOpp=BUnitOpp::getInstance();
         pNewOpp->init();
         pNewOpp->setTarget(BSimTarget(pUnit->getID()));
         pNewOpp->setSource(pTarget->getID());
         uint8 animIndex=static_cast<uint8>(getRand(cSimRand)%validAnims.getSize());
         pNewOpp->setUserData(static_cast<uint16>(validAnims[animIndex]));
         if (validAnims[animIndex] == cAnimTypeRetreat)
            pNewOpp->setType(BUnitOpp::cTypeRetreat);
         else
            pNewOpp->setType(BUnitOpp::cTypeEvade);
         pNewOpp->setPreserveDPS(true);
         pNewOpp->setPriority(BUnitOpp::cPriorityTrigger);
         pNewOpp->generateID();
         pNewOpp->setAllowComplete(true);
         if (!pTarget->addOpp(pNewOpp))
            BUnitOpp::releaseInstance(pNewOpp);
      }
      
      //Add this to the list of stuff we don't want to consider anymore.
      mEvadedUnits.add(pTarget->getID());
   }
}

//==============================================================================
//==============================================================================
void BUnitActionCollisionAttack::handleCollisions(const BEntityIDArray& collisions)
{
   BUnit* pUnit=reinterpret_cast<BUnit*>(mpOwner);
   BASSERT(pUnit);
   BVector attackerPos = pUnit->getPosition();
   BEntityID attackerUnitID = pUnit->getID();
   BPlayerID attackerPlayerID = pUnit->getPlayerID();
   BTeamID attackerTeamID = pUnit->getTeamID();
   //float attackerVelocity = pUnit->getVelocity().length();
   float totalDamageDealt = 0.0f;
   bool rammed = false;
   float ramVelocity = 0.0f;
   BVector ramDir;
   BProtoAction* pPA = const_cast<BProtoAction*>(mpProtoAction);
   long weaponType = mpProtoAction->getWeaponType();
   float maxDamagePerHit = mpProtoAction->getMaxDamagePerRam();
//-- FIXING PREFIX BUG ID 2223
   const BUnit* pFirstUnitDamaged = NULL;
//--

   static BEntityIDArray bowledSquadsToMove;
   bowledSquadsToMove.clear();

   //Do some damage.
   uint numCollisions = collisions.getSize();
   for (uint i=0; i<numCollisions; i++)
   {
      // Due to reflection damage we need to check this.
      if (!pUnit->isAlive())
         break;

      // Squad collisions - only gather up bowlable squads as those are the only ones we move below
      BEntity* pCollideEntity = gWorld->getEntity(collisions[i]);
      if ((pCollideEntity->getClassType() == BEntity::cClassTypeSquad) && isBowlable(pCollideEntity))
      {
         bowledSquadsToMove.add(collisions[i]);
         continue;
      }

      // Make sure this target is ok.
      BUnit* pTarget = pCollideEntity->getUnit();//gWorld->getUnit(collisions[i]);
      if (!pTarget || !pTarget->isAlive() || pTarget->isInCover() || (!mFlagCollideWithFriendlies && !pTarget->getPlayer()->isEnemy(pUnit->getPlayerID())))
         continue;

      BPlayerID targetPlayerID = pTarget->getPlayerID();
      BTeamID targetTeamID = pTarget->getTeamID();

      // Check bowlable / rammable type
      bool bowlable = false;
      bool rammable = false;
      if (!isUnitBowlableOrRammable(pTarget, bowlable, rammable, false))
         continue;

      // Check height to make sure unit intersects in y
      float targetMinY = pTarget->getPosition().y - pTarget->getSimBoundingBox()->getSphereRadius();
      float targetMaxY = pTarget->getPosition().y + pTarget->getSimBoundingBox()->getSphereRadius();
      float unitMinY = pUnit->getPosition().y - pUnit->getSimBoundingBox()->getSphereRadius();
      float unitMaxY = pUnit->getPosition().y + pUnit->getSimBoundingBox()->getSphereRadius();
      if ((targetMinY > unitMaxY) || (unitMinY > targetMaxY))
          continue;

      // Check to see if this unit is evading.  If so, don't throw
      BSimTarget simTarget(pUnit->getID());
      const BUnitOpp* pEvadeOpp = pTarget->getOppByTypeAndTarget(BUnitOpp::cTypeEvade, &simTarget);
      if (pEvadeOpp)
         continue;

      //==============================================================================
      // Ram away.

      #ifdef SYNC_UnitAction
         syncUnitActionData("BUnitActionCollisionAttack::handleCollisions Ramming TargetID", pTarget->getID());
      #endif

      // Calculate desired damage
      BVector direction = pTarget->getPosition() - attackerPos;
      direction.y = 0.0f;
      if (!direction.safeNormalize())
         direction = pUnit->getForward();

      // If ignoring ammo, set the damage to an amount that will kill the unit
      float requestedDamage;
      if (mFlagIgnoreAmmo)
      {
         requestedDamage = pTarget->getHitpoints() * 10.0f; // just hack in some large amount times hitpoints and multipliers get applied later
      }
      else
      {
         // Damage is ramJuice * attackerDamageModier (vet, for instance)     ...(target's modifier multiplied in in doDamageWithWeaponType)
         // then capped by maxDamagePerRam
         float attackerDamageModifier = pUnit->getDamageModifier();
         if (attackerDamageModifier<= 0.0f)
            attackerDamageModifier = 0.0f;
         requestedDamage = pUnit->getAmmunition() * attackerDamageModifier;

         if (requestedDamage > maxDamagePerHit)
            requestedDamage = maxDamagePerHit;

         // Subtract base damage from ram juice (base damage is requestedDamage capped by target hitpoints)
         pUnit->adjustAmmunition(-Math::Min(requestedDamage, pTarget->getHitpoints()));
      }

      // Notify the target that we have begun to attack it. (must be done before we deal the damage, otherwise the notify will fail)
      pTarget->addAttackingUnit(attackerUnitID, getID());  

      // Do damage to target
      bool killed = false;
      // TRB 5/2/08:  Want actual hitpoint damage dealt after applying the various multipliers.  Note that the requested damage is a value before BUnit::damage() applies the multipliers.
      float damageDealt = 0.0f;
      BDamageHelper::doDamageWithWeaponType(attackerPlayerID, attackerTeamID, collisions[i], pPA, requestedDamage, weaponType, true, direction, 1.0f, attackerPos, cInvalidObjectID, attackerUnitID, &killed, NULL, -1, false, &damageDealt);

      // After the damage is dealt, remove the attacking unit.  The notification should have already fired.
      pTarget->removeAttackingUnit(attackerUnitID, getID());

      if(killed)
      {
         gGeneralEventManager.eventTrigger(BEventDefinitions::cGameEntityRammed, pTarget->getPlayerID(), pTarget->getID(), attackerUnitID); 
      }

      // Adjust damageDealt if the target was overkilled and update totalDamageDealt
      if (pTarget->getHitpoints() < 0.0f)
         damageDealt += pTarget->getHitpoints();
      totalDamageDealt += damageDealt;

      if(damageDealt > 0 && !pFirstUnitDamaged)
         pFirstUnitDamaged = pTarget;

      // If the unit was not killed, we still need to throw it
      if (!killed)
      {
         if (bowlable)
         {
            // TODO - apply throw impulses for units with live physics
            pTarget->throwUnit(pUnit->getID());
         }
         else if (rammable)
         {
            // TODO - apply rammed anim or something for units without live physics

            // Apply ram impulse for live physics units
            if (pTarget->getPhysicsObject() && pTarget->getFlagPhysicsControl())
            {
               float force = cRammedImpulseForce;
               float launchAngle = cRammedImpulseAngle;
               BVector ramImpulse = direction;
               ramImpulse *= cosf(launchAngle); 
               ramImpulse.y = sinf(launchAngle);
               ramImpulse *= (force * pTarget->getPhysicsObject()->getMass());

               // If the vehicle already has a certain amount of upward velocity, don't add any more
               BVector vel;
               pTarget->getPhysicsObject()->getLinearVelocity(vel);
               if(vel.y > cTopYVelOfRammedUnit)
                  ramImpulse.y = 0.0f;
               else if(vel.y > cMedYVelOfRammedUnit)  //scale the impulse down if the vel is between cMedYVelOfRammedUnit and cTopYVelOfRammedUnit;
                  ramImpulse.y = ramImpulse.y * (1.0 - (vel.y - cMedYVelOfRammedUnit)/(cTopYVelOfRammedUnit - cMedYVelOfRammedUnit));

               pTarget->getPhysicsObject()->applyImpulse(ramImpulse);
            }

            // If damaged a ramable unit, set bool and direction for later use below
            ramDir = direction;
            rammed = true;
         }
      }

      // [4/21/2008 xemu] if we are doing an overrun, give us a little up-boost 
      bool overrunBounce = false;
      if (getProtoAction()->getOverrun())
         overrunBounce = true;
      if (bowlable && overrunBounce && (pUnit->getPhysicsObject() != NULL))
      {
         BPhysicsObject *pPhys = pUnit->getPhysicsObject();
         BVector vel;
         pPhys->getLinearVelocity(vel);
         // [4/21/2008 xemu] don't jump unless we have some speed going though 
         if (vel.length() > gDatabase.getOverrunMinVel())
            pPhys->applyImpulse(BVector(0,gDatabase.getOverrunJumpForce(),0));
      }

      // Reflection damage = damageDealt * globalReflectionDamageModifier (in weapon) * per target type reflectionDamageModifier (in weaponType)
      // (veterancy damageTaken modifier gets multiplied in BUnit::damage, so no need to add it here)
      int mode = (pTarget ? pUnit->getDamageTypeMode() : 0);
      BDamageTypeID damageType = pTarget->getProtoObject()->getDamageType(direction, pTarget->getForward(), pTarget->getRight(), false, true, mode);
//-- FIXING PREFIX BUG ID 2221
      const BWeaponType* pWeaponType = gDatabase.getWeaponTypeByID(weaponType);
//--
      float reflectedDamage = damageDealt * mpProtoAction->getReflectDamageFactor() * pWeaponType->getReflectDamageFactor(damageType);
      if (reflectedDamage > 0.0f)
         BDamageHelper::doDamageWithWeaponType(targetPlayerID, targetTeamID, attackerUnitID, NULL, reflectedDamage, -1, false, cInvalidVector, 1.0f, cInvalidVector, cInvalidObjectID, pTarget->getID());

      mEvadedUnits.remove(collisions[i]);
   }

   // Push bowled squads off bowler
   // TODO - This only pushes off the bowler and terrain obstructions.  It may push
   // the squad on top of other dynamic obstructions.  This is bad.
   for (int i = 0; i < bowledSquadsToMove.getNumber(); i++)
   {
      BSquad* pSquad = gWorld->getSquad(bowledSquadsToMove[i]);
      if (pSquad && !pSquad->isMoving())
      {
         bool squadMoved = false;

         // First see if squad can be moved back to squad leash position
         float distToLeashSqr = pUnit->getPosition().distanceSqr(pSquad->getLeashPosition());
         float pushOffDist = pUnit->getObstructionRadius() + pSquad->getObstructionRadius();
         if (distToLeashSqr > (pushOffDist * pushOffDist))
         {
            pSquad->setPosition(pSquad->getLeashPosition());
            squadMoved = true;
         }
         // Otherwise, look for new unobstructed position
         else
         {
            BVector dir = pSquad->getAveragePosition() - pUnit->getPosition();
            dir.y = 0.0f;
            if (!dir.safeNormalize())
               dir = pUnit->getRight();

            float dist = pushOffDist * 1.5f;
            BVector newPos = pUnit->getPosition() + dir * dist;
            BVector result;
            if (gPather.findClosestPassableTileEx(newPos, result))
            {
               pSquad->setPosition(result);
               // TRB 10/28/08 - Rammed squads that weren't currently moving were being placed at y=0, which was below ground,
               // and they wouldn't tieToGround again until they moved again.
               pSquad->tieToGround();
               squadMoved = true;
            }
         }

         // If squad moved, update its formation and leash units to new squad position
         if (squadMoved)
         {
            pSquad->updateFormation();
            makeLeashOpps(pSquad);
         }
      }
   }

   // If we rammed, stop the movement and give the rammer an impulse.  Use the rammable bool
   if (rammed)
   {
      BSquad* pParentSquad = pUnit->getParentSquad();
      if (pParentSquad)
      {
         // Stop move action
         pParentSquad->removeAllActionsForOrder(mpMoveOrder);

         // Hack - end hit and run
         pParentSquad->getSquadAI()->setMode(BSquadAI::cModeNormal);
      }

      // Impulse rammer backwards
      if (pUnit->getPhysicsObject() && pUnit->getFlagPhysicsControl())
      {
         // Save velocity at time of ram
         BVector ramVel;
         pUnit->getPhysicsObject()->getLinearVelocity(ramVel);
         ramVelocity = ramVel.lengthEstimate();

         // Stop current movement
         pUnit->getPhysicsObject()->setLinearVelocity(cOriginVector);
         pUnit->getPhysicsObject()->setAngularVelocity(cOriginVector);

         // Apply impulse
         float force = cRammerImpulseForce;
         float launchAngle = cRammerImpulseAngle;
         BVector ramImpulse = -ramDir;
         ramImpulse *= cosf(launchAngle); 
         ramImpulse.y = sinf(launchAngle);
         ramImpulse *= (force * pUnit->getPhysicsObject()->getMass());
         pUnit->getPhysicsObject()->applyImpulse(ramImpulse);

         // Set squad position
         if (pParentSquad)
         {
            BVector newSquadPos = pUnit->getPosition() - ramDir * pUnit->getObstructionRadius();
            pParentSquad->setPosition(newSquadPos, false);
            pParentSquad->setLeashPosition(newSquadPos);
            // TRB 10/28/08 - To be on safe side, tieToGround to make sure squad isn't placed below ground here.
            pParentSquad->tieToGround();
         }
      }
      else
      {
         // Set squad position
         if (pParentSquad)
         {
            BVector newSquadPos = pUnit->getPosition();
            pParentSquad->setPosition(newSquadPos, false);
            pParentSquad->setLeashPosition(newSquadPos);
         }
      }
   }

   //-- Play meat slap sound and handle rumble / camera shake  
   if(collisions.getSize() > 0 && totalDamageDealt > 0)
   {  
      // Set the recovery timer
      if (mAbilityID != -1)
      {
//-- FIXING PREFIX BUG ID 2222
         const BAbility* pAbility = gDatabase.getAbilityFromID(mAbilityID);
//--
         BSquad* pParentSquad = pUnit->getParentSquad();
         if (pAbility && pParentSquad && pParentSquad->isAlive() && (pAbility->getRecoverStart() == cRecoverAttack) &&
             (pParentSquad->getRecoverType() != pAbility->getRecoverType()))
         {
            pParentSquad->setRecover(pAbility->getRecoverType(), pParentSquad->getPlayer()->getAbilityRecoverTime(pAbility->getID()), pAbility->getID());
         }
      }

      //-- Pick the first unit in the collision list to use for material type info.      
      if(pFirstUnitDamaged)
      {
         //-- Play the melee sound.
         pUnit->playMeleeAttackSound(pFirstUnitDamaged, rammed, ramVelocity);
      }            

      //-- Rumble controller and shake camera
      if (mpProtoAction)
         mpProtoAction->doImpactRumbleAndCameraShake(BRumbleEvent::cTypeCollision, pUnit->getPosition(), true, pUnit->getID());
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionCollisionAttack::isUnitBowlableOrRammable(BUnit* pUnit, bool& bowlable, bool& rammable, bool checkRange) const
{
   bowlable = false;
   rammable = false;

   if (!pUnit)
      return false;

   // Only bowl enemy units
   if (!mFlagCollideWithFriendlies && !mpOwner->getPlayer()->isEnemy(pUnit->getPlayer()) && !pUnit->getPlayer()->isGaia())
      return false;

   // Get damage type
   int mode = (pUnit ? pUnit->getDamageTypeMode() : 0);
   BVector dir = pUnit->getPosition() - mpOwner->getPosition();
   dir.safeNormalize();
   BDamageTypeID damageType = pUnit->getProtoObject()->getDamageType(dir, pUnit->getForward(), pUnit->getRight(), false, true, mode);

   // Get weapon
//-- FIXING PREFIX BUG ID 2225
   const BWeaponType* pWeaponType = gDatabase.getWeaponTypeByID(mpProtoAction->getWeaponType());
//--
   bool canBowl = pWeaponType->getBowlable(damageType);
   bool canRam = pWeaponType->getRammable(damageType);

   // if we want to bowl the unit, but that unit can't be thrown, let us ram it
   if (canBowl && !pUnit->canBeThrown())
   {
      canBowl = false;
      canRam = true;
   }

   // Early out on weapon check for this unit only if we're ignoring the target unit
   if (mFlagIgnoreTarget && (canBowl || canRam))
   {
      bowlable = canBowl;
      rammable = canRam;
      return true;
   }
   // TODO - determine what we want to check here
   /*
   // Just check type for now
   if (canBowl || canRam)
   {
      bowlable = canBowl;
      rammable = canRam;
      return true;
   }
   */
   // Get target squad and position
//-- FIXING PREFIX BUG ID 2226
   const BSquad* pTargetSquad = NULL;
//--
   if (mTarget.isIDValid())
   {
      BEntity* pTargetEntity = gWorld->getEntity(mTarget.getID());
      if (pTargetEntity && pTargetEntity->isClassType(BEntity::cClassTypeUnit))
         pTargetSquad = pTargetEntity->getUnit()->getParentSquad();
      else if (pTargetEntity && pTargetEntity->isClassType(BEntity::cClassTypeSquad))
         pTargetSquad = pTargetEntity->getSquad();
   }

   // Bowlable or rammable if flags set and this is a unit in the target squad
   if (pTargetSquad && pTargetSquad->containsChild(pUnit->getID()) && (canBowl || canRam))
   {
      bowlable = canBowl;
      rammable = canRam;
      return true;
   }

   // Bowlable if not in the target squad but still in range of the target position
   if (canBowl)
   {
      bool inBowlRange = false;
      if (!checkRange)
         inBowlRange = true;
      else if (mTarget.isPositionValid())
      {
         BVector targetPos = mTarget.getPosition();
         float distSqr = (pUnit->getPosition() - targetPos).lengthSquared();
         float maxBowlDistSqr = mpProtoAction->getAOERadius() * mpProtoAction->getAOERadius();
         if (distSqr <= maxBowlDistSqr)
            inBowlRange = true;
      }

      if (inBowlRange)
      {
         bowlable = true;
         rammable = false;
         return true;
      }
   }

   bowlable = false;
   rammable = false;
   return false;
}

//==============================================================================
//==============================================================================
void BUnitActionCollisionAttack::makeLeashOpps(BSquad* pSquad)
{
   BASSERT(pSquad);
   BASSERT(!pSquad->isSquadAPhysicsVehicle());
   // This will add a special "force" leash opp to the units
   // Force leash opps shouldn't be removed unless they complete
   // or a new squad move generates a new unit opp
   // MPB 2/12/08
   // TODO - the "force" leash opp feels pretty hacky.  Is there
   // a better way to get this to play nicely with BUnit::createOpps
   // (which manages the unit leashing) and BUnit::doMove?
   for (uint i = 0; i < pSquad->getNumberChildren(); i++)
   {
      BEntityID id = pSquad->getChild(i);
      BUnit* pUnit = gWorld->getUnit(id);
      if (pUnit)
      {
         BVector leashLocation;
         if (!pSquad->isChildLeashed(id, 0.0f, leashLocation))
         {
            BSimTarget leashTarget(leashLocation, 0.0f);

            // Look for existing leash opp
            BUnitOpp* pLeashOpp = NULL;
            uint numOpps = pUnit->getNumberOpps();
            for (uint i = 0; i < numOpps; i++)
            {
               const BUnitOpp* pOpp = pUnit->getOppByIndex(i);
               if (pOpp->getLeash())
               {
                  pLeashOpp = const_cast<BUnitOpp*>(pOpp);
                  break;
               }
            }

            // Update existing leash opp
            if (pLeashOpp)
            {
               pLeashOpp->setTarget(leashTarget);
               pLeashOpp->setForceLeash(true);
               pLeashOpp->setAllowComplete(true);
            }
            // Or create a new one
            else
            {
               BUnitOpp* pNewOpp=BUnitOpp::getInstance();
               pNewOpp->init();
               pNewOpp->setSource(id);
               pNewOpp->setTarget(leashTarget);
               pNewOpp->setType(BUnitOpp::cTypeMove);
               pNewOpp->setLeash(true);
               pNewOpp->setForceLeash(true);
               pNewOpp->setNotifySource(false);
               pNewOpp->generateID();
               pNewOpp->setAllowComplete(true);
               pUnit->addOpp(pNewOpp);
            }
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BUnitActionCollisionAttack::checkForCollisionsWithObstruction(BUnit* pUnit, BEntityIDArray& collisions, const BEntityIDArray& ignoreUnits)
{
   if (!pUnit)
      return;

   const bool checkMovingUnits = true;
   const bool checkSquads = true;
   const bool checkBuildings = true;

   //Actually do the collision check.  Check any terrain or non-moving units.
   // jce [10/10/2008] -- corrected flags here.  Changed to using AllCollideableUnits and take out moving if needed.
   long lObOptions = BObstructionManager::cIsNewTypeAllCollidableUnits | BObstructionManager::cIsNewTypeBlockLandUnits;
   if (!checkMovingUnits)
      lObOptions &= ~BObstructionManager::cIsNewTypeCollidableMovingUnit;
   if (checkSquads)
      lObOptions |= BObstructionManager::cIsNewTypeAllCollidableSquads;
   long lObNodeType = BObstructionManager::cObsNodeTypeAll;

   // Do the obstruction check
   gObsManager.begin(BObstructionManager::cBeginEntity, 0.0f, lObOptions, 
      lObNodeType, pUnit->getPlayerID(), 0.0f, &ignoreUnits, pUnit->canJump());

   static BObstructionNodePtrArray collisionObs;
   collisionObs.setNumber(0);
   BOPQuadHull hull;
   pUnit->getObstructionHull(hull);
   gObsManager.findObstructionsQuadHull(&hull, false, false, collisionObs);

   gObsManager.end();

   // Check the type of collisions returned
   uint obIndex;
   uint obCount = collisionObs.getNumber();
   for (obIndex = 0; obIndex < obCount; obIndex++)
   {
      BOPObstructionNode* pObstructionNode = collisionObs[obIndex];
      if (pObstructionNode == NULL)
         continue;

      BEntity* pObject = pObstructionNode->mObject;
      if (pObject == NULL)
         continue;
      if (pObstructionNode->mType == BObstructionManager::cObsNodeTypeUnit)
      {
         BUnit* pCollisionUnit = pObject->getUnit();
         if (!pCollisionUnit || !pCollisionUnit->getProtoObject())
            continue;
         long objectClass = pCollisionUnit->getProtoObject()->getObjectClass();
         if ((objectClass == cObjectClassUnit) ||
             (objectClass == cObjectClassSquad) ||
             (checkBuildings && (objectClass == cObjectClassBuilding)))
         {
            collisions.add(pObject->getID());
         }

      }
      else if (pObstructionNode->mType == BObstructionManager::cObsNodeTypeSquad)
      {
         collisions.add(pObject->getID());
      }
   }
}

//==============================================================================
//==============================================================================
bool BUnitActionCollisionAttack::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITECLASS(pStream, saveType, mTarget);
   GFWRITEFREELISTITEMPTR(pStream, BSimOrder, mpMoveOrder);
   GFWRITEARRAY(pStream, BEntityID, mEvadedUnits, uint8, 200);  
   GFWRITEVAR(pStream, int, mAbilityID);
   GFWRITEBITBOOL(pStream, mFlagIgnoreTarget);
   GFWRITEBITBOOL(pStream, mFlagCollideWithFriendlies);
   GFWRITEBITBOOL(pStream, mFlagIgnoreAmmo);
   GFWRITEBITBOOL(pStream, mFlagUseObstructionForCollisions);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionCollisionAttack::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADCLASS(pStream, saveType, mTarget);
   //GFREADFREELISTITEMPTR(pStream, BSimOrder, mpMoveOrder);


   bool haveItem;
   GFREADVAR(pStream, bool, haveItem);
   if (haveItem)
   {
      uint index;
      GFREADVAR(pStream, uint, index);
      if (index == UINT_MAX)
         mpMoveOrder = NULL;
      else
      {
         if (!BSimOrder::mFreeList.isValidIndex(index))
         {
            {GFERROR("GameFile Error: invalid sim order index, on line %s, %d", __FILE__,__LINE__);}
            return false;
         }
         else
            mpMoveOrder = BSimOrder::mFreeList.get(index);
      }
   }

   GFREADARRAY(pStream, BEntityID, mEvadedUnits, uint8, 200);  
   GFREADVAR(pStream, int, mAbilityID);

   if (BAction::mGameFileVersion >= 2)
   {
      GFREADBITBOOL(pStream, mFlagIgnoreTarget);
      GFREADBITBOOL(pStream, mFlagCollideWithFriendlies);
      GFREADBITBOOL(pStream, mFlagIgnoreAmmo);
      GFREADBITBOOL(pStream, mFlagUseObstructionForCollisions);
   }
   return true;
}

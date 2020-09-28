//==============================================================================
// squadlosvalidator.cpp
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================

//==============================================================================
//Includes
#include "common.h"
#include "squadlosvalidator.h"
#include "game.h"
#include "squad.h"
#include "terrain.h"
#include "terrainsimrep.h"
#include "config.h"
#include "configsgame.h"
#include "unitquery.h"
#include "world.h"
#include "tactic.h"
#include "segintersect.h"

#define DISABLE_UNIT_LOS_OBSTRUCTIONS


// More debug stuff
#ifndef BUILD_FINAL
   class BDebugLOSTest
   {
      public:
         
         BDebugLOSTest() {}
         BDebugLOSTest(BVector start, BVector end, BVector instersection, bool obstructed)
         {
            mStart = start;
            mEnd = end;
            mIntersection = instersection;
            mObstructed = obstructed;
         }
         BVector mStart;
         BVector mEnd;
         BVector mIntersection;
         bool mObstructed;
   };
   BDynamicSimArray<BDebugLOSTest> debugLOSTests;
#endif


BSquadLOSValidator gSquadLOSValidator;



//==============================================================================
// BSquadLOSValidator::BSquadLOSValidator
//==============================================================================
BSquadLOSValidator::BSquadLOSValidator(void)
{
   reset();
}

//==============================================================================
// BSquadLOSValidator::~BSquadLOSValidator
//==============================================================================
BSquadLOSValidator::~BSquadLOSValidator(void)
{
   reset();
}

//==============================================================================
// BSquadLOSValidator::reset
//==============================================================================
void BSquadLOSValidator::reset(void)
{
   #ifndef BUILD_FINAL
      debugLOSTests.resize(0);
   #endif
}


//==============================================================================
//==============================================================================
bool BSquadLOSValidator::validateLOS(const BSquad *pSourceSquad, const BSquad *pTargetSquad)
{
   BASSERT(pSourceSquad);
   BASSERT(pTargetSquad);

   BVector sourcePosition = pSourceSquad->getPosition();
   BVector targetPosition = pTargetSquad->getPosition();

//-- FIXING PREFIX BUG ID 2398
   const BProtoAction* pSourceSquadAction = NULL;
//--
   BUnit *pSourceLeaderUnit = pSourceSquad->getLeaderUnit();
   BUnit *pTargetLeaderUnit = pTargetSquad->getLeaderUnit();

   if(pSourceLeaderUnit && pTargetLeaderUnit)
   {
      BTactic *pTactic = pSourceLeaderUnit->getTactic();

      if(pTactic)
      {
         pSourceSquadAction = pTactic->getProtoAction(pSourceLeaderUnit->getTacticState(), pTargetLeaderUnit,
                                                            pTargetLeaderUnit->getPosition(), pSourceSquad->getPlayer(), pSourceLeaderUnit->getPosition(), pSourceLeaderUnit->getID(),
                                                            -1, false, BAction::cActionTypeUnitRangedAttack);
      }
   }

   return(validateLOS(sourcePosition, targetPosition, pSourceSquad, pSourceSquadAction, pTargetSquad));
}


//==============================================================================
//==============================================================================
bool BSquadLOSValidator::validateLOS(const BSquad *pSourceSquad, const BProtoAction *pSourceSquadAction, const BSquad *pTargetSquad)
{
   BASSERT(pSourceSquad);
   BASSERT(pTargetSquad);

   BVector sourcePosition = pSourceSquad->getPosition();
   BVector targetPosition = pTargetSquad->getPosition();

   return(validateLOS(sourcePosition, targetPosition, pSourceSquad, pSourceSquadAction, pTargetSquad));
}



//==============================================================================
//==============================================================================
bool BSquadLOSValidator::validateLOS(BVector sourcePos, BVector targetPos, const BSquad *pSourceSquad, const BProtoAction *pSourceSquadAction, const BSquad *pTargetSquad, const BEntityIDArray* ignoreList)
{
   SCOPEDSAMPLE(BSquadLOSValidator_validateLOS)
   BASSERT(pSourceSquad);
   BASSERT(pTargetSquad);

   // Offset the y components for both the source and the target's position
   //
   float sourceOffsetY = 3.0f;
   float targetOffsetY = 3.0f;

   bool checkLOSAgainstBase = false;
   BUnit * pSourceLeaderUnit = pSourceSquad->getLeaderUnit();
   if(pSourceLeaderUnit && pSourceLeaderUnit->getProtoObject())
   {
      sourceOffsetY = pSourceLeaderUnit->getProtoObject()->getTrueLOSHeight();       // Use TrueLOSHeight value here
      checkLOSAgainstBase = pSourceLeaderUnit->getProtoObject()->getFlagCheckLOSAgainstBase();
   }

   BUnit * pTargetLeaderUnit = pTargetSquad->getLeaderUnit();
   if(pTargetLeaderUnit && pTargetLeaderUnit->getProtoObject())
   {
      targetPos.y = Math::Max(targetPos.y, pTargetLeaderUnit->getPosition().y);
      targetOffsetY = pTargetLeaderUnit->getProtoObject()->getObstructionRadiusY();  // Use ObstructionRadiusY
   }

   sourcePos.y += sourceOffsetY;
   targetPos.y += targetOffsetY;

   BVector intersectionPoint;
   bool intersection = false;
   long intersectedSegment = LONG_MAX;

   // if we have a base bounding box to check, do there here first, since it's a fast check 
   // also note this is a line check only - we don't check against lobbed projectiles in this case, by design 
   if (checkLOSAgainstBase && pSourceLeaderUnit)
   {
      BUnit* pBaseUnit = gWorld->getUnit(pSourceLeaderUnit->getAssociatedBase());
      if (pBaseUnit)
      {
         const BBoundingBox* pBaseBoundingBox = pBaseUnit->getSimBoundingBox();
         if (pBaseBoundingBox)
         {
            // 2d check against bounding box
            BVector corners[8];
            pBaseBoundingBox->computeCorners(corners);

            BVector curSourcePos = sourcePos;
            BVector curTargetPos = targetPos;
            curSourcePos.y = 0.0f;
            curTargetPos.y = 0.0f;
            BVector iPoint = cOriginVector;

            BVector edgeStartPoint = cOriginVector;
            BVector edgeEndPoint = cOriginVector;

            // only check the first 4 values, segment checks in the xz plane
            long startPoint = 3;
            for(long endPoint=0; endPoint<4; endPoint++)
            {
               edgeStartPoint.set(corners[startPoint].x, 0.0f, corners[startPoint].z);
               edgeEndPoint.set(corners[endPoint].x, 0.0f, corners[endPoint].z);

               long result = segmentIntersect(curSourcePos, curTargetPos, edgeStartPoint, edgeEndPoint, iPoint);
               if(result==cIntersection)
               {
                  intersection = true;
                  break;
               }

               startPoint = endPoint;
            }

            if (intersection)
            {
               intersectionPoint = iPoint;
               intersectedSegment = 0;
            }
         }
      }
   }
   // if we've intersected before we need to build a path, go ahead and return true
   if (intersection)
      return false;

   // Create a vector array with positions that describe the projectile path.  The first an last positions
   // are the sourcePos and targetPos.  For straight line projectiles, these will be the only two entries in the 
   // array.  Lobbed projectiles will have more points.
   static BVectorArray projectilePath;

   projectilePath.resize(0);
   projectilePath.add(sourcePos);

   // Look at the action's weapon's projectile to see if it is affected by gravity.  If it is, it means that we need to
   // the more expensive lobbed ray test as opposed to a straight line test.
   // 
   if(pSourceSquadAction)
   {
      const BProtoObject* pProto = NULL;
      const BPlayer* pSourcePlayer = pSourceSquad->getPlayer();
      if (pSourcePlayer)
         pProto = pSourcePlayer->getProtoObject(pSourceSquadAction->getProjectileID());

      if(pProto && pProto->getFlagIsAffectedByGravity())
      {
         BUnit *pSourceLeaderUnit = pSourceSquad->getLeaderUnit();
         if(pSourceLeaderUnit)
         {
            BVector horizontalOffsetVector;
            horizontalOffsetVector.assignDifference(targetPos, sourcePos);
            horizontalOffsetVector.y = 0.0f;
            const float horizontalDistance = horizontalOffsetVector.length();

            const float maxRange = pSourceSquadAction->getMaxRange(pSourceLeaderUnit);
            const float range = Math::Max(horizontalDistance, 0.0f);
            const float heightScale = Math::Min(((range * range) / (maxRange * maxRange)), 1.0f);

            BVector summitPos = sourcePos + ((targetPos - sourcePos) / 2.0f);
            summitPos.y += heightScale * pProto->getMaxProjectileHeight();

            projectilePath.add(summitPos);
         }
      }
   }

   projectilePath.add(targetPos);


   long numProjectilePathSegments = projectilePath.getNumber() - 1;
   long segIndex;


   // First check for intersections against other units (buildings only) (this should be faster than the
   // testing for terrain so do it first
   //

#ifndef DISABLE_UNIT_LOS_OBSTRUCTIONS

   // Query all units that are touch by the line of fire.
   // [we don't need to do this per path segment since the BUnitQuery does not take height (y) into account,
   // so it is more optimal to do only one query from the source to the target.]
   BEntityIDArray results(0, 10);       
   BUnitQuery query(sourcePos, targetPos, 10);
   //query.addObjectTypeFilter(gDatabase.getOTIDBuilding());         // return buildings
   query.addObjectTypeFilter(gDatabase.getOTIDLOSObstructable());    // return LOS obstructables
   gWorld->getUnitsInArea(&query, &results);

   // Remove ignore units
   if(ignoreList)
   {
      for(int j = ignoreList->getNumber() - 1; j >= 0; j--)
      {
         results.removeValue(ignoreList->get(j));
      }
   }

   for(segIndex = 0; segIndex< numProjectilePathSegments; segIndex++)
   {
      BVector curSourcePos = projectilePath[segIndex];
      BVector curTargetPos = projectilePath[segIndex+1];
      BVector segment = curTargetPos - curSourcePos;


      uint numUnits = (uint)results.getNumber();
      for (uint i = 0; i < numUnits; i++)
      {
         BUnit* pUnit = gWorld->getUnit(results[i]);
         if (!pUnit)
         {
            continue;
         }

         // Skip units that are set to NoRender
         if(pUnit->getObject()->getFlagNoRender() == true)
         {
            continue;
         }

         // Skip units we are are checking the target from
         BSquad *parentSquad = pUnit->getParentSquad();
         if((parentSquad == pSourceSquad) || (parentSquad == pTargetSquad))
            continue;


         const BBoundingBox* simBox = pUnit->getSimBoundingBox();

         if(simBox->isZeroVolume())
            continue;

         // [9-17-08 CJS] Fix so that the base is not considered an obstruction for base shields
         // I can't just reduce the base's obstruction to 0, as it causes birthing to break.
         BEntityRef* pRef = pUnit->getFirstEntityRefByType(BEntityRef::cTypeBaseShield);
         if (pRef)
         {
            if (pRef->mID = pTargetSquad->getID())
               continue;
         }

         // [9-30-08 CJS] Extending the above to non-turret socketed buildings
         pRef = pUnit->getFirstEntityRefByType(BEntityRef::cTypeBuildingShield);
         if (pRef)
         {
            if (pRef->mID = pTargetSquad->getID())
               continue;
         }

         float intersectDistSqr;
         intersection = simBox->raySegmentIntersects(curSourcePos, segment, true, NULL, intersectDistSqr);

         if(intersection)
         {
            BVector trajectory = segment;
            trajectory.normalize();
            trajectory.scale(sqrt(intersectDistSqr));
            intersectionPoint = curSourcePos + trajectory;
            intersectedSegment = segIndex;
            break;
         }
      }

      if(intersection)
      {
         break;
      }
   }

#endif

   // Now check for intersections with the terrain
   //

   if(!intersection)
   {
      for(segIndex = 0; segIndex< numProjectilePathSegments; segIndex++)
      {
         BVector curSourcePos = projectilePath[segIndex];
         BVector curTargetPos = projectilePath[segIndex+1];

         intersection = gTerrainSimRep.segmentIntersects(curSourcePos, curTargetPos, intersectionPoint);

         if(intersection)
         {
            intersectedSegment = segIndex;
            break;
         }
      }
   }



   // Debug rendering for LOS test
   #ifndef BUILD_FINAL
      if (gConfig.isDefined(cConfigDebugTrueLOS))
      {
         for(segIndex = 0; segIndex < numProjectilePathSegments; segIndex++)
         {
            BVector curSourcePos = projectilePath[segIndex];
            BVector curTargetPos = projectilePath[segIndex+1];
            if(segIndex < intersectedSegment)
            {
               debugLOSTests.add(BDebugLOSTest(curSourcePos, curTargetPos, intersectionPoint, false));
            }
            else
            {
               debugLOSTests.add(BDebugLOSTest(curSourcePos, curTargetPos, intersectionPoint, intersection));
               break;
            }
               
         }
      }
   #endif

   return !intersection;
}

COLOR_CONST(DarkRed, 0.5f, 0.0f, 0.0f)
COLOR_CONST(DarkGreen, 0.0f, 0.5f, 0.0f)
COLOR_CONST(LightRed, 1.0f, 0.5f, 0.5f)
COLOR_CONST(LightGreen, 0.5f, 1.0f, 0.5f)

//==============================================================================
// BSquadLOSValidator::render
//==============================================================================
void BSquadLOSValidator::render(void) const
{
   // Surround position stuff
   #ifndef BUILD_FINAL

      // Surround positions
      for (uint i = 0; i < static_cast<uint>(debugLOSTests.getNumber()); i++)
      {
//-- FIXING PREFIX BUG ID 2406
         const BDebugLOSTest& squadLOSTest = debugLOSTests[i];
//--

         DWORD sourceColor = squadLOSTest.mObstructed ? cDWORDDarkRed : cDWORDDarkGreen;
         DWORD targetColor = squadLOSTest.mObstructed ? cDWORDLightRed : cDWORDLightGreen;

         gpDebugPrimitives->addDebugLine(squadLOSTest.mStart, squadLOSTest.mEnd, sourceColor, targetColor);

         if(squadLOSTest.mObstructed)
         {
            gpDebugPrimitives->addDebugSphere(squadLOSTest.mIntersection, 1.0f, cDWORDRed);
         }
      }
   #endif
}
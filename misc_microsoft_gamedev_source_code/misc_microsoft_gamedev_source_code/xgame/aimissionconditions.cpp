//==============================================================================
// aimissionconditions.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "ai.h"
#include "aidebug.h"
#include "aimission.h"
#include "aimissionconditions.h"
#include "database.h"
#include "kb.h"
#include "kbsquad.h"
#include "squad.h"
#include "triggercondition.h"
#include "world.h"


//==============================================================================
//==============================================================================
/*BMissionCondition::BMissionCondition() :
   mType(MissionConditionType::cInvalid),
   mSquadCount(0),
   mAreaSecureDuration(0),
   mMinPresentSquadsPercentage(0.0f),
   mAreaRadiusPercentage(1.0f),
   mTimestampAreaSecure(0),
   mFlagAreaSecure(false)
{
}

//==============================================================================
//==============================================================================
BMissionCondition::BMissionCondition(BMissionConditionType type) :
   mType(type),
   mSquadCount(0),
   mAreaSecureDuration(0),
   mMinPresentSquadsPercentage(0.0f),
   mAreaRadiusPercentage(1.0f),
   mTimestampAreaSecure(0),
   mFlagAreaSecure(false)
{
}

//==============================================================================
//==============================================================================
bool BMissionCondition::evaluate(const BAIMission* pValidAIMission)
{
   BASSERT(pValidAIMission);
   switch (mType)
   {
      case MissionConditionType::cSquadCount:
         return (evaluateSquadCount(pValidAIMission));
      case MissionConditionType::cAreaSecured:
         return (evaluateAreaSecured(pValidAIMission));
      default:
      {
         BASSERTM(false, "Evaluating Invalid MissionConditionType.");
         return (false);
      }
   }
}

//==============================================================================
//==============================================================================
bool BMissionCondition::evaluateSquadCount(const BAIMission* pValidAIMission)
{
   BASSERT(pValidAIMission);
   BASSERT(mType == MissionConditionType::cSquadCount);
   uint squadCount = pValidAIMission->getNumSquads();
   return (BTriggerCondition::compareValues(squadCount, mCompareType, mSquadCount));
}

//==============================================================================
//==============================================================================
bool BMissionCondition::evaluateAreaSecured(const BAIMission* pValidAIMission)
{
   BASSERT(pValidAIMission);
   BASSERT(mType == MissionConditionType::cAreaSecured);

   
   const BAIMissionTarget* pAIMissionTarget = pValidAIMission->getAI()->getAIMissionTarget(pValidAIMission->getTargetID());
   BASSERT(pAIMissionTarget);

   BKBSquadQuery q;
   BKBSquadIDArray results;
   BVector targetPos = pAIMissionTarget->getPosition();
   float searchRad = static_cast<float>(Math::fSelectMin(cMaxBKBBaseRadius, pAIMissionTarget->getRadius()) * mAreaRadiusPercentage);

   // Get all the buildings we know about (regardless of staleness)
   q.setPointRadius(targetPos, searchRad);
   q.setPlayerRelation(pValidAIMission->getPlayerID(), cRelationTypeEnemy);
   q.setObjectType(gDatabase.getOTIDBuilding());
   pValidAIMission->getKB()->executeKBSquadQuery(q, results, BKB::cClearExistingResults);

   // Add all the squads we know about within a staleness limitation
   q.resetQuery();
   q.setPointRadius(targetPos, searchRad);
   q.setPlayerRelation(pValidAIMission->getPlayerID(), cRelationTypeEnemy);
   //q.setMaxStaleness(mAreaSecureDuration);
   pValidAIMission->getKB()->executeKBSquadQuery(q, results, 0);

   // We still have enemies so we're not secure.
   uint numResults = results.getSize();
   if (numResults > 0)
   {
      mFlagAreaSecure = false;
      mTimestampAreaSecure = 0;
      return (false);
   }

   uint numInArea = 0;
   const BAIGroupIDArray& groupIDs = pValidAIMission->getGroups();
   uint numGroups = groupIDs.getSize();
   for (uint i=0; i<numGroups; i++)
   {
      const BAIGroup* pGroup = pValidAIMission->getAI()->getAIGroup(groupIDs[i]);
      if (!pGroup)
         continue;
      // Only count groups that are waiting in the target area as "in area"
      if (pGroup->getState() != AIGroupState::cWaiting)
         continue;      

      BVector groupPos = pGroup->getPosition();
      float groupToTargetDistSqr = groupPos.xzDistanceSqr(targetPos);
      if (groupToTargetDistSqr <= cMaxBKBBaseRadiusSqr)
         numInArea += pGroup->getNumSquads();
   }

   DWORD currentGameTime = gWorld->getGametime();
   uint numSquads = pValidAIMission->getNumSquads();
   if (numSquads > 0)
   {
      float percentInArea = static_cast<float>(numInArea) / static_cast<float>(numSquads);
      if (percentInArea >= mMinPresentSquadsPercentage)
      {
         if (!mFlagAreaSecure)
         {
            mFlagAreaSecure = true;
            mTimestampAreaSecure = currentGameTime;
         }
      }
      else
      {
         mFlagAreaSecure = false;
         mTimestampAreaSecure = 0;
      }
   }

   if (mFlagAreaSecure)
   {
      DWORD timestampSecureLongEnough = mTimestampAreaSecure + mAreaSecureDuration;
      if (currentGameTime >= timestampSecureLongEnough)
         return (true);
   }

   return (false);
}*/
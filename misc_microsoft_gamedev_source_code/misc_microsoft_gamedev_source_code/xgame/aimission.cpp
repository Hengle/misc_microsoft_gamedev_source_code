//==============================================================================
// aimission.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "ai.h"
#include "aidebug.h"
#include "aifactoid.h"
#include "aimanager.h"
#include "aimission.h"
#include "aiteleporterzone.h"
#include "bid.h"
#include "bidmgr.h"
#include "commands.h"
#include "config.h"
#include "configsgame.h"
#include "database.h"
#include "EntityGrouper.h"
#include "FontSystem2.h"
#include "game.h"
#include "kb.h"
#include "kbsquad.h"
#include "player.h"
#include "powerorbital.h"
#include "powerrage.h"
#include "powerrepair.h"
#include "powermanager.h"
#include "powerwave.h"
#include "powerodst.h"
#include "powercleansing.h"
#include "protoobject.h"
#include "protopower.h"
#include "protosquad.h"
#include "render.h"
#include "selectionmanager.h"
#include "simhelper.h"
#include "squad.h"
#include "tactic.h"
#include "unitactionhotdrop.h"
#include "unitquery.h"
#include "user.h"
#include "usermanager.h"
#include "visiblemap.h"
#include "world.h"


GFIMPLEMENTVERSION(BAIMissionCache, 4);
GFIMPLEMENTVERSION(BAIMission, 4);
GFIMPLEMENTVERSION(BAIScoreModifier, 1);
GFIMPLEMENTVERSION(BAIPlayerModifier, 1);

//==============================================================================
//==============================================================================
BSquadGroupPair::BSquadGroupPair() :
   mSquadID(cInvalidObjectID),
   mGroupID(cInvalidAIGroupID)
{
}


//==============================================================================
//==============================================================================
BSquadGroupPair::BSquadGroupPair(BEntityID squadID) :
   mSquadID(squadID),
   mGroupID(cInvalidAIGroupID)
{
}


//==============================================================================
//==============================================================================
BSquadGroupPair::BSquadGroupPair(BEntityID squadID, BAIGroupID groupID) :
   mSquadID(squadID),
   mGroupID(groupID)
{
}


//==============================================================================
//==============================================================================
void BAIMissionCache::clear()
{
   mWorkDistGatherables.resize(0);
   mWorkDistEnemies.resize(0);
   mWorkDistGarrisonedEnemies.resize(0);
   mWorkDistVisibleEnemies.resize(0);
   mWorkDistDoppledEnemies.resize(0);
   mWorkDistAllies.resize(0);
   mSecureDistEnemies.resize(0);
   mSecureDistAllies.resize(0);
   mHotZoneDistEnemies.resize(0);
   mHotZoneDistAllies.resize(0);
   mHotZoneDistSelf.resize(0);
   mGathererGroups.resize(0);
   mGroupsInsideLeash.resize(0);
   mGroupsOutsideLeash.resize(0);
   mGroupsInsideRally.resize(0);
   mGroupsOutsideRally.resize(0);
   mGroupsMeleeSquads.resize(0);
   mTeleportZones.resize(0);

   mTimestampWorkDistGatherables = 0;
   mTimestampWorkDistEnemies = 0;
   mTimestampWorkDistVisibleEnemies = 0;
   mTimestampWorkDistDoppledEnemies = 0;
   mTimestampWorkDistAllies = 0;
   mTimestampSecureDistEnemies = 0;
   mTimestampSecureDistAllies = 0;
   mTimestampHotZoneDistEnemies = 0;
   mTimestampHotZoneDistAllies = 0;
   mTimestampHotZoneDistSelf = 0;
   mTimestampGathererGroups = 0;
   mTimestampGroupsInsideLeash = 0;
   mTimestampGroupsOutsideLeash = 0;
   mTimestampGroupsInsideRally = 0;
   mTimestampGroupsOutsideRally = 0;
   mTimestampTeleportZones = 0;
}


//==============================================================================
//==============================================================================
bool BAIMissionCache::save(BStream* pStream, int saveType) const
{
   GFWRITEARRAY(pStream, BKBSquadID, mWorkDistGatherables, uint16, 1000);
   GFWRITEARRAY(pStream, BKBSquadID, mWorkDistEnemies, uint16, 1000);
   GFWRITEARRAY(pStream, BKBSquadID, mWorkDistGarrisonedEnemies, uint16, 1000);
   GFWRITEARRAY(pStream, BKBSquadID, mWorkDistVisibleEnemies, uint16, 1000);
   GFWRITEARRAY(pStream, BKBSquadID, mWorkDistDoppledEnemies, uint16, 1000);
   GFWRITEARRAY(pStream, BKBSquadID, mWorkDistAllies, uint16, 1000);
   GFWRITEARRAY(pStream, BKBSquadID, mSecureDistEnemies, uint16, 1000);
   GFWRITEARRAY(pStream, BKBSquadID, mSecureDistAllies, uint16, 1000);
   GFWRITEARRAY(pStream, BKBSquadID, mHotZoneDistEnemies, uint16, 1000);
   GFWRITEARRAY(pStream, BKBSquadID, mHotZoneDistAllies, uint16, 1000);
   GFWRITEARRAY(pStream, BKBSquadID, mHotZoneDistSelf, uint16, 1000);
   GFWRITEARRAY(pStream, BAIGroupID, mGathererGroups, uint16, 1000);
   GFWRITEARRAY(pStream, BAIGroupID, mGroupsInsideLeash, uint16, 1000);
   GFWRITEARRAY(pStream, BAIGroupID, mGroupsOutsideLeash, uint16, 1000);
   GFWRITEARRAY(pStream, BAIGroupID, mGroupsInsideRally, uint16, 1000);
   GFWRITEARRAY(pStream, BAIGroupID, mGroupsOutsideRally, uint16, 1000);
   GFWRITEARRAY(pStream, BAIGroupID, mGroupsMeleeSquads, uint16, 1000);
   GFWRITEARRAY(pStream, BAITeleporterZoneID, mTeleportZones, uint16, 1000);

   GFWRITEVAR(pStream, DWORD, mTimestampWorkDistGatherables);
   GFWRITEVAR(pStream, DWORD, mTimestampWorkDistEnemies);
   GFWRITEVAR(pStream, DWORD, mTimestampWorkDistVisibleEnemies);
   GFWRITEVAR(pStream, DWORD, mTimestampWorkDistDoppledEnemies);
   GFWRITEVAR(pStream, DWORD, mTimestampWorkDistAllies);
   GFWRITEVAR(pStream, DWORD, mTimestampSecureDistEnemies);
   GFWRITEVAR(pStream, DWORD, mTimestampSecureDistAllies);
   GFWRITEVAR(pStream, DWORD, mTimestampHotZoneDistEnemies);
   GFWRITEVAR(pStream, DWORD, mTimestampHotZoneDistAllies);
   GFWRITEVAR(pStream, DWORD, mTimestampHotZoneDistSelf);
   GFWRITEVAR(pStream, DWORD, mTimestampGathererGroups);
   GFWRITEVAR(pStream, DWORD, mTimestampGroupsInsideLeash);
   GFWRITEVAR(pStream, DWORD, mTimestampGroupsOutsideLeash);
   GFWRITEVAR(pStream, DWORD, mTimestampGroupsInsideRally);
   GFWRITEVAR(pStream, DWORD, mTimestampGroupsOutsideRally);
   GFWRITEVAR(pStream, DWORD, mTimestampTeleportZones);

   return (true);
}


//==============================================================================
//==============================================================================
bool BAIMissionCache::load(BStream* pStream, int saveType)
{
   GFREADARRAY(pStream, BKBSquadID, mWorkDistGatherables, uint16, 1000);
   if (BAIMissionCache::mGameFileVersion < 3)
      pStream->skipBytes(mWorkDistGatherables.size() * 4);

   GFREADARRAY(pStream, BKBSquadID, mWorkDistEnemies, uint16, 1000);
   if (BAIMissionCache::mGameFileVersion < 3)
      pStream->skipBytes(mWorkDistEnemies.size() * 4);

   if (BAIMissionCache::mGameFileVersion >= 4)
   GFREADARRAY(pStream, BKBSquadID, mWorkDistGarrisonedEnemies, uint16, 1000);

   GFREADARRAY(pStream, BKBSquadID, mWorkDistVisibleEnemies, uint16, 1000);
   if (BAIMissionCache::mGameFileVersion < 3)
      pStream->skipBytes(mWorkDistVisibleEnemies.size() * 4);

   GFREADARRAY(pStream, BKBSquadID, mWorkDistDoppledEnemies, uint16, 1000);
   if (BAIMissionCache::mGameFileVersion < 3)
      pStream->skipBytes(mWorkDistDoppledEnemies.size() * 4);

   GFREADARRAY(pStream, BKBSquadID, mWorkDistAllies, uint16, 1000);
   if (BAIMissionCache::mGameFileVersion < 3)
      pStream->skipBytes(mWorkDistAllies.size() * 4);

   GFREADARRAY(pStream, BKBSquadID, mSecureDistEnemies, uint16, 1000);
   if (BAIMissionCache::mGameFileVersion < 3)
      pStream->skipBytes(mSecureDistEnemies.size() * 4);

   GFREADARRAY(pStream, BKBSquadID, mSecureDistAllies, uint16, 1000);
   if (BAIMissionCache::mGameFileVersion < 3)
      pStream->skipBytes(mSecureDistAllies.size() * 4);

   GFREADARRAY(pStream, BKBSquadID, mHotZoneDistEnemies, uint16, 1000);
   if (BAIMissionCache::mGameFileVersion < 3)
      pStream->skipBytes(mHotZoneDistEnemies.size() * 4);

   GFREADARRAY(pStream, BKBSquadID, mHotZoneDistAllies, uint16, 1000);
   if (BAIMissionCache::mGameFileVersion < 3)
      pStream->skipBytes(mHotZoneDistAllies.size() * 4);

   GFREADARRAY(pStream, BKBSquadID, mHotZoneDistSelf, uint16, 1000);
   if (BAIMissionCache::mGameFileVersion < 3)
      pStream->skipBytes(mHotZoneDistSelf.size() * 4);

   GFREADARRAY(pStream, BAIGroupID, mGathererGroups, uint16, 1000);
   if (BAIMissionCache::mGameFileVersion < 3)
      pStream->skipBytes(mGathererGroups.size() * 4);

   GFREADARRAY(pStream, BAIGroupID, mGroupsInsideLeash, uint16, 1000);
   if (BAIMissionCache::mGameFileVersion < 3)
      pStream->skipBytes(mGroupsInsideLeash.size() * 4);

   GFREADARRAY(pStream, BAIGroupID, mGroupsOutsideLeash, uint16, 1000);
   if (BAIMissionCache::mGameFileVersion < 3)
      pStream->skipBytes(mGroupsOutsideLeash.size() * 4);

   GFREADARRAY(pStream, BAIGroupID, mGroupsInsideRally, uint16, 1000);
   if (BAIMissionCache::mGameFileVersion < 3)
      pStream->skipBytes(mGroupsInsideRally.size() * 4);

   GFREADARRAY(pStream, BAIGroupID, mGroupsOutsideRally, uint16, 1000);
   if (BAIMissionCache::mGameFileVersion < 3)
      pStream->skipBytes(mGroupsOutsideRally.size() * 4);

   if (BAIMissionCache::mGameFileVersion >= 4)
      GFREADARRAY(pStream, BAIGroupID, mGroupsMeleeSquads, uint16, 1000);

   GFREADARRAY(pStream, BAITeleporterZoneID, mTeleportZones, uint16, 1000);
   if (BAIMissionCache::mGameFileVersion < 3)
      pStream->skipBytes(mTeleportZones.size() * 4);

   GFREADVAR(pStream, DWORD, mTimestampWorkDistGatherables);
   GFREADVAR(pStream, DWORD, mTimestampWorkDistEnemies);
   GFREADVAR(pStream, DWORD, mTimestampWorkDistVisibleEnemies);
   GFREADVAR(pStream, DWORD, mTimestampWorkDistDoppledEnemies);
   GFREADVAR(pStream, DWORD, mTimestampWorkDistAllies);
   GFREADVAR(pStream, DWORD, mTimestampSecureDistEnemies);
   GFREADVAR(pStream, DWORD, mTimestampSecureDistAllies);
   GFREADVAR(pStream, DWORD, mTimestampHotZoneDistEnemies);
   GFREADVAR(pStream, DWORD, mTimestampHotZoneDistAllies);

   if (BAIMissionCache::mGameFileVersion >= 2)
      GFREADVAR(pStream, DWORD, mTimestampHotZoneDistSelf);

   GFREADVAR(pStream, DWORD, mTimestampGathererGroups);
   GFREADVAR(pStream, DWORD, mTimestampGroupsInsideLeash);
   GFREADVAR(pStream, DWORD, mTimestampGroupsOutsideLeash);
   GFREADVAR(pStream, DWORD, mTimestampGroupsInsideRally);
   GFREADVAR(pStream, DWORD, mTimestampGroupsOutsideRally);
   GFREADVAR(pStream, DWORD, mTimestampTeleportZones);

   return (true);
}


//==============================================================================
//==============================================================================
BAIMission::BAIMission()
{
   mID = 0;
   mType = MissionType::cInvalid;
   resetNonIDData();
}


//==============================================================================
//==============================================================================
BAIMission::~BAIMission()
{
}

//==============================================================================
//==============================================================================
void BAIMission::resetNonIDData()
{
   mSquads.reserve(32);
   mSquads.resize(0);
   mUngroupedSquads.reserve(32);
   mUngroupedSquads.resize(0);
   mSquadGroupMap.reserve(32);
   mSquadGroupMap.resize(0);
   mGroups.reserve(16);
   mGroups.resize(0);
   mWorkingGroups.reserve(16);
   mWorkingGroups.resize(0);
   mBidIDs.reserve(8);
   mBidIDs.resize(0);
   mTargetWrapperIDs.reserve(8);
   mTargetWrapperIDs.resize(0);
   mPrevUpdateWrapperID = cInvalidAIMissionTargetWrapperID;
   mPrevUpdateTargetID = cInvalidAIMissionTargetID;
   mFailConditionMinSquads = 0;
   mManualRetreatPosX = 0.0f;
   mManualRetreatPosZ = 0.0f;
   mLaunchScores.zero();
   mAITopicID.invalidate();
   mPlayerID = cInvalidPlayerID;
   mState = MissionState::cInvalid;
   mTimestampActiveToggled = 0;
   mTimestampStateChanged = 0;
   mTimestampNextOppDetect = 0;
   mTimestampNextAct = 0;
   mTimestampNextThink = 0;
   mTimestampNextPowerEval = 0;
   mTimestampNextODSTEval = 0;
   mTimestampEnemiesCleared = 0;
   mFlagActive = false;
   mFlagAllowOpportunityTargets = true;
   mFlagMoveAttack = false;
   mFlagEnemiesCleared = false;
   mFlagManualRetreatPos = false;
   mFlagRepeatTargets = false;
   mFlagEnableOdstDrop = false;

#ifndef BUILD_FINAL
   mName.empty();
   mDebugID = 0;
#endif
}


//==============================================================================
//==============================================================================
void BAIMission::payTimeCostAct(DWORD currentGameTime)
{
   const BAI* pAI = gWorld->getAI(mPlayerID);
   BASSERT(pAI);
   if (pAI)
   {
      DWORD actCostMin = pAI->getActCostMin();
      DWORD actCostMax = pAI->getActCostMax();
      BASSERT(actCostMax >= actCostMin);
      DWORD actCostNext = getRandRange(cSimRand, actCostMin, actCostMax);
      mTimestampNextAct = currentGameTime + actCostNext;
   }
}


//==============================================================================
//==============================================================================
void BAIMission::payTimeCostThink(DWORD currentGameTime)
{
   const BAI* pAI = gWorld->getAI(mPlayerID);
   BASSERT(pAI);
   if (pAI)
   {
      DWORD thinkCostMin = pAI->getThinkCostMin();
      DWORD thinkCostMax = pAI->getThinkCostMax();
      BASSERT(thinkCostMax >= thinkCostMin);
      DWORD thinkCostNext = getRandRange(cSimRand, thinkCostMin, thinkCostMax);
      mTimestampNextThink = currentGameTime + thinkCostNext;
   }
}

//==============================================================================
//==============================================================================
void BAIMission::payTimeCostPowerEval(DWORD currentGameTime)
{
   const BAI* pAI = gWorld->getAI(mPlayerID);
   BASSERT(pAI);
   if (pAI)
   {
      DWORD powerEvalCostMin = pAI->getPowerEvalIntervalMin();
      DWORD powerEvalCostMax = pAI->getPowerEvalIntervalMax();
      BASSERT(powerEvalCostMax >= powerEvalCostMin);
      DWORD powerEvalCostNext = getRandRange(cSimRand, powerEvalCostMin, powerEvalCostMax);
      mTimestampNextPowerEval = currentGameTime + powerEvalCostNext;
   }
}

//==============================================================================
//==============================================================================
void BAIMission::payTimeCostODSTEval(DWORD currentGameTime)
{
   const BAI* pAI = gWorld->getAI(mPlayerID);
   BASSERT(pAI);
   if (pAI)
   {
      DWORD odstEvalCostMin = pAI->getPowerEvalIntervalMin();
      DWORD odstEvalCostMax = pAI->getPowerEvalIntervalMax();
      BASSERT(odstEvalCostMax >= odstEvalCostMin);
      DWORD odstEvalCostNext = getRandRange(cSimRand, odstEvalCostMin, odstEvalCostMax);
      mTimestampNextODSTEval = currentGameTime + odstEvalCostNext;
   }
}

//==============================================================================
//==============================================================================
bool BAIMission::canAct(DWORD currentGameTime) const
{
   if (currentGameTime >= mTimestampNextAct)
      return (true);
   else
      return (false);
}


//==============================================================================
//==============================================================================
bool BAIMission::canThink(DWORD currentGameTime) const
{
   if (currentGameTime >= mTimestampNextThink)
      return (true);
   else
      return (false);
}

//==============================================================================
//==============================================================================
bool BAIMission::canPowerEval(DWORD currentGameTime) const
{
   if (currentGameTime >= mTimestampNextPowerEval)
      return (true);
   else
      return (false);
}

//==============================================================================
//==============================================================================
bool BAIMission::canODSTEval(DWORD currentGameTime) const
{
   if (currentGameTime >= mTimestampNextODSTEval)
      return (true);
   else
      return (false);
}

//==============================================================================
//==============================================================================
bool BAIMission::AIQExceededMaxFocusTime(DWORD currentGameTime) const
{
   // No AIQ = we cannot exceed this.
   const BAI* pAI = gWorld->getAI(mPlayerID);
   BASSERT(pAI);
   if (!pAI || !pAI->getFlagAIQ())
      return (false);

   DWORD timestampMaxFocusTime = mTimestampActiveToggled + pAI->getMaxMissionFocusTime();
   if (currentGameTime >= timestampMaxFocusTime)
      return (true);
   else
      return (false);
}


//==============================================================================
//==============================================================================
void BAIMission::update(DWORD currentGameTime)
{
   const BAI* pAI = gWorld->getAI(mPlayerID);
   if (pAI->getFlagPaused())
      return;
   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer || !pPlayer->isPlaying())
      return;

   switch (mState)
   {
      case MissionState::cSuccess:
      {
         updateStateSuccess(currentGameTime);
         break;
      }
      case MissionState::cFailure:
      {
         updateStateFailure(currentGameTime);
         break;
      }
      case MissionState::cCreate:
      {
         updateStateCreate(currentGameTime);
         break;
      }
      case MissionState::cWorking:
      {
         updateStateWorking(currentGameTime);
         break;
      }
      case MissionState::cWithdraw:
      {
         updateStateWithdraw(currentGameTime);
         break;
      }
      case MissionState::cRetreat:
      {
         updateStateRetreat(currentGameTime);
         break;
      }
      case MissionState::cInvalid:
      default:
      {
         BFAIL("Mission is in an invalid state.  Unsupported!");
         BAI* pAI = gWorld->getAI(mPlayerID);
         BASSERT(pAI);
         if (pAI)
            pAI->AIQTopicLotto(currentGameTime);
         break;
      }
   }
}


//==============================================================================
//==============================================================================
bool BAIMission::isStateTerminal()
{
   switch (mState)
   {
      case MissionState::cCreate:
      case MissionState::cWorking:
      case MissionState::cWithdraw:
      case MissionState::cRetreat:
      {
         return (false);
      }
      case MissionState::cInvalid:
      case MissionState::cSuccess:
      case MissionState::cFailure:
      default:
      {
         return (true);
      }
   }
}


//==============================================================================
//==============================================================================
float BAIMission::getRangeModifier(const BAIGroup& group, const BSquad& targetSquad) const
{
   // The distance from center of group to edge of target.
   float groupToTargetDist = targetSquad.calculateXZDistance(group.getPosition());
   float groupMaxAttackRange = group.getMaxAttackRange();
   float rangeDelta = groupToTargetDist - groupMaxAttackRange;

   // Not out of range means just return 1.0
   if (rangeDelta <= 0.0f)
      return (1.0f);

   // marchack clean up.
   if (rangeDelta >= 80.0f)
      return (0.2f);

   float lerp = rangeDelta / 80.0f;
   float modifier = Math::Lerp(0.8f, 0.2f, lerp);
   return (modifier);
}


//==============================================================================
//==============================================================================
float BAIMission::getMostlyDeadModifier(const BKBSquad& targetSquad) const
{
   // Just linearly make it a juicier target the more dead it is, for now...
   float mostlyDeadModifier = 0.5f + (0.5f * (1.0f - targetSquad.getHPPercentage()));
   return (mostlyDeadModifier);
}


//==============================================================================
//==============================================================================
void BAIMission::toggleActive(DWORD currentGameTime, bool active)
{
   // on!
   mFlagActive = active;
   mTimestampActiveToggled = currentGameTime;

   // Set our next service time to NOW because we just got toggled on!
   if (active)
   {
      mTimestampNextAct = currentGameTime;
   }
   else
   {
      mTimestampNextAct = 0;
   }
}


//==============================================================================
//==============================================================================
void BAIMission::deleteAllTasks(BAIGroupID groupID)
{
   BAIGroup* pGroup = gWorld->getAIGroup(groupID);
   if (pGroup)
      pGroup->deleteAllTasks();
}


//==============================================================================
//==============================================================================
void BAIMission::deleteAllTasks(const BAIGroupIDArray& groupIDs)
{
   uint numGroups = groupIDs.getSize();
   for (uint i=0; i<numGroups; i++)
      deleteAllTasks(groupIDs[i]);
}


//==============================================================================
//==============================================================================
void BAIMission::updateGroups(DWORD currentGameTime)
{
   uint numGroups = mGroups.getSize();
   for (uint i=0; i<numGroups; i++)
   {
      BAIGroup* pGroup = gWorld->getAIGroup(mGroups[i]);
      if (pGroup)
      {
         // Group updating that always occurs.
         pGroup->update(currentGameTime);
      }
   }
}


//==============================================================================
// Note: It is possible for both pWrapper & pTarget to be NULL.
//==============================================================================
bool BAIMission::processStateChangeConditions(DWORD currentGameTime)
{
   // Overall mission fail test.
   if (processMissionFailedConditions())
      return (true);

   // Without a target we cannot do the other tests, so bail.
   const BAIMissionTargetWrapper* pWrapper = getCurrentTargetWrapper();
   if (!pWrapper)
      return (false);
   const BAIMissionTarget* pTarget = gWorld->getAIMissionTarget(pWrapper->getTargetID());
   if (!pTarget)
      return (false);

   // Target retreat conditions.
   if (processTargetRetreatConditions(*pWrapper, *pTarget))
      return (true);

   // Target success conditions.
   if (processTargetSuccessConditions(currentGameTime, *pWrapper, *pTarget))
      return (true);

   // No state change.
   return (false);
}


//==============================================================================
//==============================================================================
bool BAIMission::processMissionFailedConditions()
{
   // ---------------------------------------------------------------
   // #1 - Overall mission fail test is fall below the minimum squads.
   // If this occurs, we immediately go to the failure state.
   if (mSquads.getSize() < mFailConditionMinSquads)
   {
      setState(MissionState::cFailure);
      BAI* pAI = gWorld->getAI(mPlayerID);
      if (pAI)
         pAI->AIQTopicLotto(gWorld->getGametime());
      return (true);
   }
   
   // No state change.
   return (false);
}


//==============================================================================
//==============================================================================
bool BAIMission::processTargetRetreatConditions(const BAIMissionTargetWrapper& wrapper, const BAIMissionTarget& target)
{
   bool bShouldRetreat = false;
   //--------------------------------------------------------------
   // #2 - Current target wrapper - Retreat ratio
   // If this occurs, we retreat, or move to next target.
   if (wrapper.getFlagAllowRetreat())
   {
      // Initialize some vars.
      const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
      if (!pPlayer)
         return (false);
      const BKB* pKB = gWorld->getKB(pPlayer->getTeamID());
      if (!pKB)
         return (false);

      // Determine our local enemies.
      BAISquadAnalysis enemiesAnalysis;
      BAI::calculateSquadAnalysis(mCache.mHotZoneDistEnemies, enemiesAnalysis);

      // Determine our global mission squads owned by us. (not our allies.)
      BAISquadAnalysis selfAnalysis;
      BAI::calculateSquadAnalysis(mSquads, selfAnalysis);

      // Determine our local allies (not owned by us).
      BAISquadAnalysis alliesAnalysis;
      BAI::calculateSquadAnalysis(mCache.mHotZoneDistAllies, alliesAnalysis);

      // Add our local allies to our global squads analysis.
      // This now contains ALL "friendly" squads we should consider.
      selfAnalysis.add(alliesAnalysis);

      // Figure out who is "better" overall. - We can skip this if the enemies have no offense.
      float friendlyOffense = BAI::calculateOffenseAToB(selfAnalysis, enemiesAnalysis);
      float enemyOffense = BAI::calculateOffenseAToB(enemiesAnalysis, selfAnalysis);
      if (enemyOffense > 0.0f)
      {
         float relativeStrength = friendlyOffense / enemyOffense;
         if (relativeStrength < wrapper.getMinRetreatRatio())
         {
            bShouldRetreat = true;
         }
      }

      // Test for all melee units attacking garrisoned units.
      if (!bShouldRetreat)
      {
         // If we have stuff, and the enemies have stuff, and all our stuff is melee, and all their stuff is garrisoned, run away.
         if (mCache.mWorkDistGarrisonedEnemies.getSize() > 0 &&
             mCache.mGroupsMeleeSquads.getSize() > 0 &&
             mCache.mWorkDistGarrisonedEnemies.getSize() == mCache.mWorkDistEnemies.getSize() &&
             mCache.mGroupsMeleeSquads.getSize() == mGroups.getSize())
         {
            bShouldRetreat = true;
         }
      }
   }

   if (bShouldRetreat)
   {
      if (wrapper.getFlagAllowSkip())
      {
         gWorld->deleteWrapper(wrapper.getID());

         if (getCurrentTargetWrapper())
         {
            // We have another target lined up, so go to working.
            setState(MissionState::cWorking);
            return (true);
         }
         else
         {
            // We don't have a next target, so retreat.
            setState(MissionState::cRetreat);
            return (true);
         }
      }
      else
      {
         setState(MissionState::cRetreat);
         return (true);
      }
   }

   // No change.
   return (false);
}


//==============================================================================
//==============================================================================
bool BAIMission::processTargetSuccessConditions(DWORD currentGameTime, const BAIMissionTargetWrapper& wrapper, const BAIMissionTarget& target)
{
   //------------------------------------------------------
   // Get our percentage of squads that are rallied up.
   //------------------------------------------------------
   float percentSquadsRallied = 0.0f;
   uint numSquads = mSquads.getSize();
   if (numSquads > 0)
   {
      uint numSquadsInArea = 0;
      uint numRalliedGroups = mCache.mGroupsInsideRally.getSize();
      for (uint i=0; i<numRalliedGroups; i++)
      {
         BAIGroup* pGroup = gWorld->getAIGroup(mCache.mGroupsInsideRally[i]);
         if (pGroup)
            numSquadsInArea += pGroup->getNumSquads();
      }
      // Figure out % of squads are "in the area" but bail if we don't have enough yet.
      percentSquadsRallied = static_cast<float>(numSquadsInArea) / static_cast<float>(numSquads);
   }
   if (percentSquadsRallied < wrapper.getMinRalliedPercent())
      return (false);

   //------------------------------------------------------
   // Do we have any enemies in the area?
   //------------------------------------------------------
   if (wrapper.getFlagRequireSecure())
   {
      // We have enemies, so reset our enemy cleared data.
      if (mCache.mSecureDistEnemies.getSize() > 0)
      {
         mFlagEnemiesCleared = false;
         mTimestampEnemiesCleared = 0;
         return (false);
      }
      // Otherwise if we weren't already clear, clear us now.
      else if (!mFlagEnemiesCleared)
      {
         mFlagEnemiesCleared = true;
         mTimestampEnemiesCleared = currentGameTime;
      }

      // We are secure, but if we are not secure long enough bail now.
      if ((currentGameTime - mTimestampEnemiesCleared) < wrapper.getMinSecureTime())
         return (false);
   }

   // Do we need to gather some crates?
   if (wrapper.getFlagGatherCrates())
   {
      // IF we have gatherers and crates to gather, we might not be able to declare success yet.
      if (mCache.mGathererGroups.getSize() > 0 && mCache.mWorkDistGatherables.getSize() > 0)
      {
         if (!wrapper.getFlagCurrentlyGathering())
         {
            return (false);
         }
         else if (currentGameTime < wrapper.getTimestampStopGathering())
         {
            return (false);
         }
      }
   }

   // Add more conditions for success here!

   //-----------------------------------------------------
   // TARGET SUCCESS!!!
   //-----------------------------------------------------
   // Destroy the current target wrapper.
   //if (mFlagRepeatTargets)
   //{
   //   BASSERTM(mTargetWrapperIDs.getSize() > 1, "Mission set to repeat target wrappers, with only 1 target wrapper specified.");
   //   reorderTargetWrapper(wrapper.getID(), mTargetWrapperIDs.getSize());
   //}
   //else
   //{
      gWorld->deleteWrapper(wrapper.getID());
   //}

   if (getCurrentTargetWrapper())
   {
      // We have another target lined up, so go to working.
      setState(MissionState::cWorking);
      return (true);
   }
   else
   {
      setState(MissionState::cSuccess);
      return (true);
   }

   // no change.
   return (false);
}


//==============================================================================
//==============================================================================
void BAIMission::updateCache(DWORD currentGameTime)
{
   // Bomb checks.
   const BAI* pAI = gWorld->getAI(mPlayerID);
   BASSERT(pAI);
   if (!pAI)
      return;
   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   BASSERT(pPlayer);
   if (!pPlayer)
      return;
   const BKB* pKB = gWorld->getKB(pPlayer->getTeamID());
   BASSERT(pKB);
   if (!pKB)
      return;
   const BAIMissionTargetWrapper* pWrapper = this->getCurrentTargetWrapper();
   if (!pWrapper)
      return;
   const BAIMissionTarget* pTarget = gWorld->getAIMissionTarget(pWrapper->getTargetID());
   if (!pTarget)
      return;

   // Get some variables from our target and wrapper.
   BVector targetPos = pTarget->getPosition();
   float workDist = pWrapper->getSearchWorkDist();
   float secureDist = pWrapper->getSecureDist();
   float hotZoneDist = pWrapper->getHotZoneDist();
   float leashDistSqr = pWrapper->getLeashDistSqr();
   float rallyDistSqr = pWrapper->getRallyDistSqr();

   // Clear the cache all at once here.
   mCache.clear();

   //--------------------------------
   // Work dist gatherables
   //--------------------------------
   BKBSquadQuery gatherableQuery;
   gatherableQuery.setPointRadius(targetPos, workDist);
   gatherableQuery.setPlayerRelation(cGaiaPlayer, cRelationTypeSelf);
   gatherableQuery.setObjectType(gDatabase.getOTIDGatherable());
   pKB->executeKBSquadQuery(gatherableQuery, mCache.mWorkDistGatherables, BKB::cClearExistingResults);
   mCache.mTimestampWorkDistGatherables = currentGameTime;

   //--------------------------
   // Work dist query.
   //--------------------------
   uint numWorkDistResults;
   BKBSquadIDArray workDistResults;
   BKBSquadQuery workDistQuery;
   workDistQuery.setPointRadius(targetPos, workDist);
   numWorkDistResults = pKB->executeKBSquadQuery(workDistQuery, workDistResults, BKB::cClearExistingResults);
   mCache.mTimestampWorkDistEnemies = currentGameTime;
   mCache.mTimestampWorkDistVisibleEnemies = currentGameTime;
   mCache.mTimestampWorkDistDoppledEnemies = currentGameTime;
   mCache.mTimestampWorkDistAllies = currentGameTime;
   for (uint i=0; i<numWorkDistResults; i++)
   {
      BKBSquadID kbSquadID = workDistResults[i];
      const BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquadID);
      if (!pKBSquad)
         continue;
      BPlayerID playerID = pKBSquad->getPlayerID();
      if (mPlayerModifier.GetPlayerModifier(playerID) <= 0.0f)
         continue;
      if (pPlayer->isEnemy(playerID))
      {
         mCache.mWorkDistEnemies.add(kbSquadID);
         if (pKBSquad->getCurrentlyVisible())
            mCache.mWorkDistVisibleEnemies.add(kbSquadID);
         else
            mCache.mWorkDistDoppledEnemies.add(kbSquadID);

         if (pKBSquad->getSquad() && pKBSquad->getSquad()->isGarrisoned())
            mCache.mWorkDistGarrisonedEnemies.add(kbSquadID);
      }
      else if (pPlayer->isAlly(playerID, false))
      {
         mCache.mWorkDistAllies.add(kbSquadID);
      }
   }

   //--------------------------
   // Secure dist query.
   //--------------------------
   uint numSecureDistResults;
   BKBSquadIDArray secureDistResults;
   if (secureDist == workDist)
   {
      numSecureDistResults = numWorkDistResults;
      secureDistResults = workDistResults;
      mCache.mSecureDistEnemies = mCache.mWorkDistEnemies;
      mCache.mSecureDistAllies = mCache.mWorkDistAllies;
   }
   else
   {
      BKBSquadQuery secureDistQuery;
      secureDistQuery.setPointRadius(targetPos, secureDist);
      numSecureDistResults = pKB->executeKBSquadQuery(secureDistQuery, secureDistResults, BKB::cClearExistingResults);
      for (uint i=0; i<numSecureDistResults; i++)
      {
         BKBSquadID kbSquadID = secureDistResults[i];
         const BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquadID);
         if (!pKBSquad)
            continue;
         BPlayerID playerID = pKBSquad->getPlayerID();
         if (mPlayerModifier.GetPlayerModifier(playerID) <= 0.0f)
            continue;
         if (pPlayer->isEnemy(playerID))
         {
            mCache.mSecureDistEnemies.add(kbSquadID);
         }
         else if (pPlayer->isAlly(playerID, false))
         {
            mCache.mSecureDistAllies.add(kbSquadID);
         }
      }
   }
   mCache.mTimestampSecureDistEnemies = currentGameTime;
   mCache.mTimestampSecureDistAllies = currentGameTime;

   //--------------------------
   // Hot zone dist query.
   //--------------------------
   uint numHotZoneDistResults;
   BKBSquadIDArray hotZoneDistResults;
   if (hotZoneDist == workDist)
   {
      numHotZoneDistResults = numWorkDistResults;
      hotZoneDistResults = workDistResults;
      mCache.mHotZoneDistEnemies = mCache.mWorkDistEnemies;
      mCache.mHotZoneDistAllies = mCache.mWorkDistAllies;
      for (uint i=0; i<numHotZoneDistResults; i++)
      {
         BKBSquadID kbSquadID = hotZoneDistResults[i];
         const BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquadID);
         if (pKBSquad && pPlayer->isSelf(pKBSquad->getPlayerID()))
            mCache.mHotZoneDistSelf.add(kbSquadID);
      }      
   }
   else if (hotZoneDist == secureDist)
   {
      numHotZoneDistResults = numSecureDistResults;
      hotZoneDistResults = secureDistResults;
      mCache.mHotZoneDistEnemies = mCache.mSecureDistEnemies;
      mCache.mHotZoneDistAllies = mCache.mSecureDistAllies;
      for (uint i=0; i<numHotZoneDistResults; i++)
      {
         BKBSquadID kbSquadID = hotZoneDistResults[i];
         const BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquadID);
         if (pKBSquad && pPlayer->isSelf(pKBSquad->getPlayerID()))
            mCache.mHotZoneDistSelf.add(kbSquadID);
      }
   }
   else
   {
      BKBSquadQuery hotZoneDistQuery;
      hotZoneDistQuery.setPointRadius(targetPos, hotZoneDist);
      numHotZoneDistResults = pKB->executeKBSquadQuery(hotZoneDistQuery, hotZoneDistResults, BKB::cClearExistingResults);
      for (uint i=0; i<numHotZoneDistResults; i++)
      {
         BKBSquadID kbSquadID = hotZoneDistResults[i];
         const BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquadID);
         if (!pKBSquad)
            continue;
         BPlayerID playerID = pKBSquad->getPlayerID();
         if (mPlayerModifier.GetPlayerModifier(playerID) <= 0.0f)
            continue;
         if (pPlayer->isEnemy(playerID))
         {
            mCache.mHotZoneDistEnemies.add(kbSquadID);
         }
         else if (pPlayer->isAlly(playerID, false))
         {
            mCache.mHotZoneDistAllies.add(kbSquadID);
         }
         else if (pPlayer->isSelf(playerID))
         {
            mCache.mHotZoneDistSelf.add(kbSquadID);
         }
      }
   }
   mCache.mTimestampHotZoneDistEnemies = currentGameTime;
   mCache.mTimestampHotZoneDistAllies = currentGameTime;
   mCache.mTimestampHotZoneDistSelf = currentGameTime;

   //--------------------------
   // Groups
   //--------------------------
   mCache.mTimestampGathererGroups = currentGameTime;
   mCache.mTimestampGroupsInsideLeash = currentGameTime;
   mCache.mTimestampGroupsOutsideLeash = currentGameTime;
   mCache.mTimestampGroupsInsideRally = currentGameTime;
   mCache.mTimestampGroupsOutsideRally = currentGameTime;

   BProtoSquadID meleePSIDs[4];
   meleePSIDs[0] = gDatabase.getProtoSquad("unsc_inf_cyclops_01");
   meleePSIDs[1] = gDatabase.getProtoSquad("cov_inf_eliteCommando_01");
   meleePSIDs[2] = gDatabase.getProtoSquad("cov_inf_arbiter_01");
   meleePSIDs[3] = gDatabase.getProtoSquad("cov_inf_bruteChief_01");

   uint numGroups = mGroups.getSize();
   for (uint i=0; i<numGroups; i++)
   {
      BAIGroupID groupID = mGroups[i];
      const BAIGroup* pGroup = gWorld->getAIGroup(groupID);
      if (!pGroup)
         continue;

      BVector groupPos = pGroup->getPosition();

      if (pGroup->getFlagCanGatherSupplies())
         mCache.mGathererGroups.add(groupID);

      if (groupPos.xzDistanceSqr(targetPos) <= leashDistSqr)
         mCache.mGroupsInsideLeash.add(groupID);
      else
         mCache.mGroupsOutsideLeash.add(groupID);

      BProtoSquadID groupProtoSquadID = pGroup->getProtoSquadID();
      if (groupProtoSquadID == meleePSIDs[0] ||
          groupProtoSquadID == meleePSIDs[1] ||
          groupProtoSquadID == meleePSIDs[2] ||
          groupProtoSquadID == meleePSIDs[3])
      {
         mCache.mGroupsMeleeSquads.add(groupID);
      }

      if (rallyDistSqr == leashDistSqr)
      {
         mCache.mGroupsInsideRally = mCache.mGroupsInsideLeash;
         mCache.mGroupsOutsideRally = mCache.mGroupsOutsideLeash;
      }
      else
      {
         if (groupPos.xzDistanceSqr(targetPos) <= rallyDistSqr)
            mCache.mGroupsInsideRally.add(groupID);
         else
            mCache.mGroupsOutsideRally.add(groupID);
      }
   }

   BAIManager* pAIManager = gWorld->getAIManager();
   if (pAIManager)
   {
      // 1.) Get the exit zones that match our target pos.
      BStaticSimArray<BEntityID> dropoffTeleporterUnitIDs;
      const BAITeleporterZoneIDArray& dropoffZones = pAIManager->getAITeleporterDropoffZoneIDs();
      uint numDropoffZones = dropoffZones.getSize();
      for (uint i=0; i<numDropoffZones; i++)
      {
         BAITeleporterZone* pZone = pAIManager->getAITeleporterZone(dropoffZones[i]);
         if (!pZone)
            continue;
         if (dropoffTeleporterUnitIDs.contains(pZone->getUnitID())) // We already grabbed this teleporter.
            continue;
         if (!pZone->containsPosition(targetPos))
            continue;
         dropoffTeleporterUnitIDs.add(pZone->getUnitID());
      }

      // 2.)  Get the zones for the teleporters linked to these dropoffs.
      BObjectTypeID otidTeleporterPickup = gDatabase.getOTIDTeleportPickup();
      const BAITeleporterZoneIDArray& pickupZones = pAIManager->getAITeleporterPickupZoneIDs();
      uint numPickupZones = pickupZones.getSize();
      for (uint i=0; i<numPickupZones; i++)
      {
         BAITeleporterZone* pZone = pAIManager->getAITeleporterZone(pickupZones[i]);
         if (!pZone)
            continue;
         BUnit* pTeleporterUnit = gWorld->getUnit(pZone->getUnitID());
         if (!pTeleporterUnit)
            continue;
         if (!pTeleporterUnit->isType(otidTeleporterPickup))
            continue;
         BUnitActionHotDrop* pUAHD = static_cast<BUnitActionHotDrop*>(pTeleporterUnit->getActionByType(BAction::cActionTypeUnitHotDrop));
         if (!pUAHD)
            continue;
         BUnit* pHotDropTarget = pUAHD->getHotdropTarget();
         if (!pHotDropTarget)
            continue;
         if (!dropoffTeleporterUnitIDs.contains(pHotDropTarget->getID()))
            continue;
         mCache.mTeleportZones.add(pickupZones[i]);
      }
   }
}


//==============================================================================
//==============================================================================
bool BAIMission::processMissionTimeout(DWORD currentGameTime)
{
   // In AIQ, the mission can timeout and we need to run the lotto.
   if (AIQExceededMaxFocusTime(currentGameTime))
   {
      BAI* pAI = gWorld->getAI(mPlayerID);
      if (pAI)
         pAI->AIQTopicLotto(currentGameTime);
      return (true);
   }

   return (false);
}


//==============================================================================
//==============================================================================
const BAIMissionTargetWrapper* BAIMission::getCurrentTargetWrapper() const
{
   if (mTargetWrapperIDs.getSize() == 0)
      return (NULL);
   return (gWorld->getWrapper(mTargetWrapperIDs[0]));
}


//==============================================================================
//==============================================================================
BAIMissionTargetWrapper* BAIMission::getCurrentTargetWrapper()
{
   if (mTargetWrapperIDs.getSize() == 0)
      return (NULL);
   return (gWorld->getWrapper(mTargetWrapperIDs[0]));
}


//==============================================================================
//==============================================================================
void BAIMission::updateStateSuccess(DWORD currentGameTime)
{
   if (!mFlagActive)
      return;

   // Don't continue to suck up AIQ in success state.
   BAI* pAI = gWorld->getAI(mPlayerID);
   if (pAI)
      pAI->AIQTopicLotto(currentGameTime);
}


//==============================================================================
//==============================================================================
void BAIMission::updateStateFailure(DWORD currentGameTime)
{
   if (!mFlagActive)
      return;

   // Don't continue to suck up AIQ in failure state.
   BAI* pAI = gWorld->getAI(mPlayerID);
   if (pAI)
      pAI->AIQTopicLotto(currentGameTime);
}


//==============================================================================
// A one-time validation, setup, etc state... not really a real update.
//==============================================================================
void BAIMission::updateStateCreate(DWORD currentGameTime)
{
   if (!mFlagActive)
      return;

   // Garbage collection.
   removeInvalidSquads();

   // Group our ungrouped squads.
   processUngroupedSquads();

   // Update the positions, etc. of our groups.
   updateGroups(currentGameTime);

   // If we're terminal, bail!
   //if (processTerminalConditions(currentGameTime))
   //   return;

   setState(MissionState::cWorking);
   return;
}


//==============================================================================
//==============================================================================
void BAIMission::updateStateWorking(DWORD currentGameTime)
{
   if (!mFlagActive)
      return;
   BAI* pAI = gWorld->getAI(mPlayerID);
   if (!pAI)
      return;
   BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return;

   // Garbage collection.
   removeInvalidSquads();

   // Group our ungrouped squads.
   processUngroupedSquads();

   // Update the positions, etc. of our groups.
   updateGroups(currentGameTime);

   if (pAI->getFlagAIQ())
   {
//-- FIXING PREFIX BUG ID 4008
      const BAIGroup* pMostRecentTaskedGroup = getMostRecentlyTaskedGroup(mGroups);
//--
      if (pMostRecentTaskedGroup)
         pPlayer->setLookAtPos(pMostRecentTaskedGroup->getPosition());
   }

   // Do we have time to act upon any groups?
   if (!canAct(currentGameTime) || !canThink(currentGameTime))
      return;

   updateCache(currentGameTime);

   if (processStateChangeConditions(currentGameTime))
      return;

   // If we're terminal, bail!
   //if (processTerminalConditions(currentGameTime))
   //   return;
   if (mSquads.getSize() == 0)
   {
      pAI->AIQTopicLotto(gWorld->getGametime());
      return;
   }

   // Have we spent too much time here?
   if (processMissionTimeout(currentGameTime))
      return;

   // If we meet the criteria (minimum cooldown period, etc...) ask the script to watch out for something better to do...
   processPossibleOpportunityTargets(currentGameTime);

   // If we have any powers ready to go, and it's a good time to use them, launch a power mission and go to that.
   if (processPowerLaunch(currentGameTime))
      return;

   // This is a special case.  :)
   if (processPowerLaunchODST(currentGameTime))
      return;

   // Update the AIEYE
   //BAIGroup* pMostRecentTaskedGroup = getMostRecentlyTaskedGroup(mGroups);
   //if (pMostRecentTaskedGroup)
      //pPlayer->setLookAtPos(pMostRecentTaskedGroup->getPosition());
   
   // Do we have time to act upon any groups?
   //if (!canAct(currentGameTime) || !canThink(currentGameTime))
      //return;

   // Make sure we have a target.
   BAIMissionTargetWrapper* pCurrentTargetWrapper = getCurrentTargetWrapper();
   if (!pCurrentTargetWrapper) // marchack - no target refs?? - just bail?
      return;

   BAIMissionTarget* pCurrentMissionTarget = gWorld->getAIMissionTarget(pCurrentTargetWrapper->getTargetID());
   if (!pCurrentMissionTarget) // marchack - no target?? - just bail?
      return;

   BVector currentMissionTargetPos = pCurrentMissionTarget->getPosition();

   // Can we get a group to task?
   BAIGroup* pGroup = getNextGroupToTask();
   if (!pGroup)
      return;

   // Increment our think AIQ so we don't infinitely spin on decisions.
   payTimeCostThink(currentGameTime);

   //BVector groupPos = pGroup->getPosition();
   //float groupToTargetXZDistSqr = groupPos.xzDistanceSqr(currentMissionTargetPos);

   //-----------------------------------------------------------------------------------------
   // We are too far from the target.  We need to get closer.  By teleporting or walking.
   //-----------------------------------------------------------------------------------------
   if (processLeashMove(*pGroup, *pCurrentMissionTarget))
      return;

   // Can we do a simple command ability?  (Grenade, rocket, etc... no complex factors.)
   if (pGroup->canDoDamageAbility())
   {
      BTaskInfoArray potentialTasks(0, 64);
      recalcDamageAbilityTasks(*pGroup, potentialTasks);
      const BTaskInfo* pTask = getHighestScoringTask(*pGroup, potentialTasks);
      if (pTask)
      {
         const BAIGroupTask* pCurrentTask = pGroup->getCurrentTask();
         if (!pCurrentTask || (pGroup->getTimestampSquadAdded() > pGroup->getTimestampLastTask()) || !pCurrentTask->isEquivalentTask(*pTask, 10.0f))
         {
            BAIGroupIDArray groupsToTask;
            groupsToTask.add(pGroup->getID());
            taskGroups(groupsToTask, *pTask);
            //pGroup->taskAttack(currentGameTime, pTask->mTarget.getID(), pTask->mScore, false, gDatabase.getAIDCommand());
            //pGroup->setState(AIGroupState::cEngaged);
            payTimeCostAct(currentGameTime);
         }

         #ifndef BUILD_FINAL
            pGroup->setPotentialTasks(potentialTasks);
         #endif
         return;
      }
   }


   // Can we do a simple command ability?  (Grenade, rocket, etc... no complex factors.)
   if (pGroup->canDoSuicideAbility())
   {
      BTaskInfoArray potentialTasks(0, 64);
      recalcSuicideAbilityTasks(*pGroup, potentialTasks);
      const BTaskInfo* pTask = getHighestScoringTask(*pGroup, potentialTasks);
      if (pTask)
      {
         const BAIGroupTask* pCurrentTask = pGroup->getCurrentTask();
         if (!pCurrentTask || (pGroup->getTimestampSquadAdded() > pGroup->getTimestampLastTask()) || !pCurrentTask->isEquivalentTask(*pTask, 10.0f))
         {
            BAIGroupIDArray groupsToTask;
            groupsToTask.add(pGroup->getID());
            taskGroups(groupsToTask, *pTask);
            //pGroup->taskAttack(currentGameTime, pTask->mTarget.getID(), pTask->mScore, false, gDatabase.getAIDCommand());
            //pGroup->setState(AIGroupState::cEngaged);
            payTimeCostAct(currentGameTime);
         }

#ifndef BUILD_FINAL
         pGroup->setPotentialTasks(potentialTasks);
#endif
         return;
      }
   }

   // Can we do a simple command ability?  (Grenade, rocket, etc... no complex factors.)
   if (pGroup->canDoJumppackAbility())
   {
      BTaskInfoArray potentialTasks(0, 64);
      recalcJumppackAbilityTasks(*pGroup, potentialTasks);
      const BTaskInfo* pTask = getHighestScoringTask(*pGroup, potentialTasks);
      if (pTask)
      {
         const BAIGroupTask* pCurrentTask = pGroup->getCurrentTask();
         if (!pCurrentTask || (pGroup->getTimestampSquadAdded() > pGroup->getTimestampLastTask()) || !pCurrentTask->isEquivalentTask(*pTask, 10.0f))
         {
            BAIGroupIDArray groupsToTask;
            groupsToTask.add(pGroup->getID());
            taskGroups(groupsToTask, *pTask);
            //pGroup->taskAttack(currentGameTime, pTask->mTarget.getID(), pTask->mScore, false, gDatabase.getAIDCommand());
            //pGroup->setState(AIGroupState::cEngaged);
            payTimeCostAct(currentGameTime);
         }

#ifndef BUILD_FINAL
         pGroup->setPotentialTasks(potentialTasks);
#endif
         return;
      }
   }


   if (pGroup->canDoOverburnAbility())
   {
      BTaskInfoArray potentialTasks(0, 64);
      recalcOverburnAbilityTasks(*pGroup, potentialTasks);
      const BTaskInfo* pTask = getHighestScoringTask(*pGroup, potentialTasks);
      if (pTask)
      {
         const BAIGroupTask* pCurrentTask = pGroup->getCurrentTask();
         if (!pCurrentTask || (pGroup->getTimestampSquadAdded() > pGroup->getTimestampLastTask()) || !pCurrentTask->isEquivalentTask(*pTask, 10.0f))
         {
            BAIGroupIDArray groupsToTask;
            groupsToTask.add(pGroup->getID());
            taskGroups(groupsToTask, *pTask);
            //pGroup->taskAttack(currentGameTime, pTask->mTarget.getID(), pTask->mScore, false, gDatabase.getAIDCommand());
            //pGroup->setState(AIGroupState::cEngaged);
            payTimeCostAct(currentGameTime);
         }

         #ifndef BUILD_FINAL
         pGroup->setPotentialTasks(potentialTasks);
         #endif
         return;
      }
   }

   if (pGroup->canDoFlashBangAbility())
   {
      BTaskInfoArray potentialTasks(0, 64);
      recalcFlashBangAbilityTasks(*pGroup, potentialTasks);
      const BTaskInfo* pTask = getHighestScoringTask(*pGroup, potentialTasks);
      if (pTask)
      {
         const BAIGroupTask* pCurrentTask = pGroup->getCurrentTask();
         if (!pCurrentTask || (pGroup->getTimestampSquadAdded() > pGroup->getTimestampLastTask()) || !pCurrentTask->isEquivalentTask(*pTask, 10.0f))
         {
            BAIGroupIDArray groupsToTask;
            groupsToTask.add(pGroup->getID());
            taskGroups(groupsToTask, *pTask);
            //pGroup->taskAttack(currentGameTime, pTask->mTarget.getID(), pTask->mScore, false, gDatabase.getAIDCommand());
            //pGroup->setState(AIGroupState::cEngaged);
            payTimeCostAct(currentGameTime);
         }

         #ifndef BUILD_FINAL
         pGroup->setPotentialTasks(potentialTasks);
         #endif
         return;
      }
   }

   if (pGroup->canDoStasisAbility())
   {
      BTaskInfoArray potentialTasks(0, 64);
      recalcStasisAbilityTasks(*pGroup, potentialTasks);
      const BTaskInfo* pTask = getHighestScoringTask(*pGroup, potentialTasks);
      if (pTask)
      {
         const BAIGroupTask* pCurrentTask = pGroup->getCurrentTask();
         if (!pCurrentTask || (pGroup->getTimestampSquadAdded() > pGroup->getTimestampLastTask()) || !pCurrentTask->isEquivalentTask(*pTask, 10.0f))
         {
            BAIGroupIDArray groupsToTask;
            groupsToTask.add(pGroup->getID());
            taskGroups(groupsToTask, *pTask);
            //pGroup->taskAttack(currentGameTime, pTask->mTarget.getID(), pTask->mScore, false, gDatabase.getAIDCommand());
            //pGroup->setState(AIGroupState::cEngaged);
            payTimeCostAct(currentGameTime);
         }

#ifndef BUILD_FINAL
         pGroup->setPotentialTasks(potentialTasks);
#endif
         return;
      }
   }


   if (pGroup->canDoRamAbility())
   {
      BTaskInfoArray potentialTasks(0, 64);
      recalcRamAbilityTasks(*pGroup, potentialTasks);
      const BTaskInfo* pTask = getHighestScoringTask(*pGroup, potentialTasks);
      if (pTask)
      {
         const BAIGroupTask* pCurrentTask = pGroup->getCurrentTask();
         if (!pCurrentTask || (pGroup->getTimestampSquadAdded() > pGroup->getTimestampLastTask()) || !pCurrentTask->isEquivalentTask(*pTask, 10.0f))
         {
            BAIGroupIDArray groupsToTask;
            groupsToTask.add(pGroup->getID());
            taskGroups(groupsToTask, *pTask);
            //pGroup->taskAttack(currentGameTime, pTask->mTarget.getID(), pTask->mScore, false, gDatabase.getAIDCommand());
            //pGroup->setState(AIGroupState::cEngaged);
            payTimeCostAct(currentGameTime);
         }

#ifndef BUILD_FINAL
         pGroup->setPotentialTasks(potentialTasks);
#endif
         return;
      }
   }

   // Can we do hijacking.
   if (pGroup->canDoHijackAbility())
   {
      BTaskInfoArray potentialTasks(0, 64);
      recalcHijackAbilityTasks(*pGroup, potentialTasks);
      const BTaskInfo* pTask = getHighestScoringTask(*pGroup, potentialTasks);
      if (pTask)
      {
         const BAIGroupTask* pCurrentTask = pGroup->getCurrentTask();
         if (!pCurrentTask || (pGroup->getTimestampSquadAdded() > pGroup->getTimestampLastTask()) || !pCurrentTask->isEquivalentTask(*pTask, 10.0f))
         {
            BAIGroupIDArray groupsToTask;
            groupsToTask.add(pGroup->getID());
            taskGroups(groupsToTask, *pTask);
            //pGroup->taskHijack(currentGameTime, pTask->mTarget.getID(), pTask->mScore, false);
            //pGroup->setState(AIGroupState::cEngaged);
            payTimeCostAct(currentGameTime);
         }

         #ifndef BUILD_FINAL
            pGroup->setPotentialTasks(potentialTasks);
         #endif
         return;
      }
   }

   // Can we do a default attack?
   {
      BTaskInfoArray potentialTasks(0, 64);
      recalcStandardAttackTasks(*pGroup, potentialTasks);
      const BTaskInfo* pTask = getHighestScoringTask(*pGroup, potentialTasks);
      if (pTask)
      {
         const BAIGroupTask* pCurrentTask = pGroup->getCurrentTask();
         if (!pCurrentTask || (pGroup->getTimestampSquadAdded() > pGroup->getTimestampLastTask()) || !pCurrentTask->isEquivalentTask(*pTask, 10.0f))
         {
            BAIGroupIDArray groupsToTask;
            groupsToTask.add(pGroup->getID());
            taskGroups(groupsToTask, *pTask);
            //pGroup->taskAttack(currentGameTime, pTask->mTarget.getID(), pTask->mScore, false, cInvalidAbilityID);
            //pGroup->setState(AIGroupState::cEngaged);
            payTimeCostAct(currentGameTime);
         }

         #ifndef BUILD_FINAL
            pGroup->setPotentialTasks(potentialTasks);
         #endif
         return;
      }
   }

   // Can we find something to scout that is stale?
   {
      BTaskInfoArray potentialTasks(0, 64);
      recalcScoutStaleTargetTasks(*pGroup, potentialTasks);
      const BTaskInfo* pTask = getHighestScoringTask(*pGroup, potentialTasks);
      if (pTask)
      {
         const BAIGroupTask* pCurrentTask = pGroup->getCurrentTask();
         if (!pCurrentTask || (pGroup->getTimestampSquadAdded() > pGroup->getTimestampLastTask()) || !pCurrentTask->isEquivalentTask(*pTask, 10.0f))
         {
            BAIGroupIDArray groupsToTask;
            groupsToTask.add(pGroup->getID());
            taskGroups(groupsToTask, *pTask);

            //pGroup->taskMoveToPos(currentGameTime, pTask->mTarget.getPosition(), false, getFlagMoveAttack());
            //pGroup->setState(AIGroupState::cMoving);
            payTimeCostAct(currentGameTime);
         }

         return;
      }
   }

   // Can we gather a crate?
   {
      bool currentlyGathering = pCurrentTargetWrapper->getFlagCurrentlyGathering();
      DWORD timestampStopGathering = pCurrentTargetWrapper->getTimestampStopGathering();
      if (pCurrentTargetWrapper->getFlagGatherCrates() && pGroup->getFlagCanGatherSupplies() && (!currentlyGathering || currentGameTime < timestampStopGathering))
      {
         BTaskInfoArray potentialTasks(0, 64);
         recalcGatherTasks(*pGroup, potentialTasks);
         const BTaskInfo* pTask = getHighestScoringTask(*pGroup, potentialTasks);
         if (pTask)
         {
            const BAIGroupTask* pCurrentTask = pGroup->getCurrentTask();
            if (!pCurrentTask || (pGroup->getTimestampSquadAdded() > pGroup->getTimestampLastTask()) || !pCurrentTask->isEquivalentTask(*pTask, 10.0f))
            {
               BAIGroupIDArray groupsToTask;
               groupsToTask.add(pGroup->getID());
               taskGroups(groupsToTask, *pTask);
               //pGroup->taskGather(currentGameTime, pTask->mTarget.getID(), pTask->mScore, false);
               //pGroup->setState(AIGroupState::cMoving);
               if (!pCurrentTargetWrapper->getFlagCurrentlyGathering())
               {
                  pCurrentTargetWrapper->setFlagCurrentlyGathering(true);
                  pCurrentTargetWrapper->setTimestampStopGathering(currentGameTime + 30000); // gather for up to 30 seconds.
               }
               payTimeCostAct(currentGameTime);
               if (mGroups.getSize() == 1)
                  pAI->AIQTopicLotto(currentGameTime);
            }
            return;
         }
      }
   }

   {
      if (pGroup->getFlagCanAutoRepair())
      {
         BTaskInfoArray potentialTasks(0, 64);
         recalcRepairOtherTasks(*pGroup, potentialTasks);
         const BTaskInfo* pTask = getHighestScoringTask(*pGroup, potentialTasks);
         if (pTask)
         {
            const BAIGroupTask* pCurrentTask = pGroup->getCurrentTask();
            if (!pCurrentTask || (pGroup->getTimestampSquadAdded() > pGroup->getTimestampLastTask()) || !pCurrentTask->isEquivalentTask(*pTask, 10.0f))
            {
               BAIGroupIDArray groupsToTask;
               groupsToTask.add(pGroup->getID());
               taskGroups(groupsToTask, *pTask);
               //pGroup->taskGather(currentGameTime, pTask->mTarget.getID(), pTask->mScore, false);
               //pGroup->setState(AIGroupState::cMoving);
               payTimeCostAct(currentGameTime);
               //if (mGroups.getSize() == 1)
               //   pAI->AIQTopicLotto(currentGameTime);
            }
            return;
         }
      }
   }

   // Nothing to do?  Should the mission still exist??
   //-----------------------------------------------------------------------------------------
   // We are too far from the target.  We need to get closer.  By teleporting or walking.
   //-----------------------------------------------------------------------------------------
   if (processRallyMove(*pGroup, *pCurrentMissionTarget))
      return;
}


//==============================================================================
//==============================================================================
bool BAIMission::processLeashMove(BAIGroup& rGroup, const BAIMissionTarget& rTarget)
{
   const BAITeleporterZoneIDArray& teleportZones = mCache.mTeleportZones;
   const BAIGroupIDArray& groupsOutsideLeash = mCache.mGroupsOutsideLeash;

   // If we are not outside the leash, do nothing at this step.
   if (!groupsOutsideLeash.contains(rGroup.getID()))
      return (false);

   BAIGroupIDArray groupsToTask;
   BTaskInfo requiredActionTask;

   // We have two options.  Figure out which we need to do.
   // 1.)  Teleport
   // 2.)  Walk

   uint numTeleportZones = teleportZones.getSize();
   for (uint i=0; i<numTeleportZones; i++)
   {
      const BAITeleporterZone* pZone = gWorld->getAITeleporterZone(mCache.mTeleportZones[i]);
      if (pZone && pZone->containsPosition(rGroup.getPosition()))
      {
         // We qualify to teleport!
         requiredActionTask.setTarget(pZone->getUnitID());
         requiredActionTask.setType(AIGroupTaskType::cGarrison);
      }
   }

   // Or should we walk?
   if (requiredActionTask.getType() == AIGroupTaskType::cInvalid)
   {
      requiredActionTask.setTarget(rTarget.getPosition());
      requiredActionTask.setType(AIGroupTaskType::cMove);
   }

   // Add all the groups who are outside the leash and are not already doing this command.
   uint numOutsideLeash = groupsOutsideLeash.getSize();
   for (uint i=0; i<numOutsideLeash; i++)
   {
      // Garbage group == skip.
      const BAIGroup* pGroup = gWorld->getAIGroup(groupsOutsideLeash[i]);
      if (!pGroup)
         continue;
      // Already doing this task == skip.
      const BAIGroupTask* pCurrentTask = pGroup->getCurrentTask();
      if (pCurrentTask && (pGroup->getTimestampLastTask() > pGroup->getTimestampSquadAdded()) && pCurrentTask->isEquivalentTask(requiredActionTask, 10.0f))
         continue;
      // Add this group to be tasked.
      groupsToTask.add(groupsOutsideLeash[i]);
   }

   // Do we have anyone to task?
   if (groupsToTask.getSize() > 0)
   {
      taskGroups(groupsToTask, requiredActionTask, false, mFlagMoveAttack);
      payTimeCostAct(gWorld->getGametime());
      return (true);
   }

   // We had nobody who needed to move towards the target.
   return (true);
}


//==============================================================================
//==============================================================================
bool BAIMission::processRallyMove(BAIGroup& rGroup, const BAIMissionTarget& rTarget)
{
   const BAITeleporterZoneIDArray& teleportZones = mCache.mTeleportZones;
   const BAIGroupIDArray& groupsOutsideRally = mCache.mGroupsOutsideRally;

   // If we are not outside the rally, do nothing at this step.
   if (!groupsOutsideRally.contains(rGroup.getID()))
      return (false);

   BAIGroupIDArray groupsToTask;
   BTaskInfo requiredActionTask;

   // We have two options.  Figure out which we need to do.
   // 1.)  Teleport
   // 2.)  Walk

   uint numTeleportZones = teleportZones.getSize();
   for (uint i=0; i<numTeleportZones; i++)
   {
      const BAITeleporterZone* pZone = gWorld->getAITeleporterZone(mCache.mTeleportZones[i]);
      if (pZone && pZone->containsPosition(rGroup.getPosition()))
      {
         // We qualify to teleport!
         requiredActionTask.setTarget(pZone->getUnitID());
         requiredActionTask.setType(AIGroupTaskType::cGarrison);
      }
   }

   // Or should we walk?
   if (requiredActionTask.getType() == AIGroupTaskType::cInvalid)
   {
      requiredActionTask.setTarget(rTarget.getPosition());
      requiredActionTask.setType(AIGroupTaskType::cMove);
   }

   // Add all the groups who are outside the rally and are not already doing this command.
   uint numOutsideRally = groupsOutsideRally.getSize();
   for (uint i=0; i<numOutsideRally; i++)
   {
      // Garbage group == skip.
      const BAIGroup* pGroup = gWorld->getAIGroup(groupsOutsideRally[i]);
      if (!pGroup)
         continue;
      // Already doing this task == skip.
      const BAIGroupTask* pCurrentTask = pGroup->getCurrentTask();
      if (pCurrentTask && (pGroup->getTimestampLastTask() > pGroup->getTimestampSquadAdded()) && pCurrentTask->isEquivalentTask(requiredActionTask, 10.0f))
         continue;
      // Add this group to be tasked.
      groupsToTask.add(groupsOutsideRally[i]);
   }

   // Do we have anyone to task?
   if (groupsToTask.getSize() > 0)
   {
      taskGroups(groupsToTask, requiredActionTask, false, mFlagMoveAttack);
      payTimeCostAct(gWorld->getGametime());
      return (true);
   }

   // We had nobody who needed to move towards the target.
   return (true);
}


//==============================================================================
//==============================================================================
void BAIMission::updateStateWithdraw(DWORD currentGameTime)
{
   BFAIL("BAIMission::updateStateWithdraw() - Not yet supported.");
}


//==============================================================================
//==============================================================================
void BAIMission::updateStateRetreat(DWORD currentGameTime)
{
   if (!mFlagActive)
      return;
   BAI* pAI = gWorld->getAI(mPlayerID);
   if (!pAI)
      return;
   BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return;

   // Garbage collection.
   removeInvalidSquads();

   // Group our ungrouped squads.
   processUngroupedSquads();

   // Update the positions, etc. of our groups.
   updateGroups(currentGameTime);

   if (pAI->getFlagAIQ())
   {
      //-- FIXING PREFIX BUG ID 4008
      const BAIGroup* pMostRecentTaskedGroup = getMostRecentlyTaskedGroup(mGroups);
      //--
      if (pMostRecentTaskedGroup)
         pPlayer->setLookAtPos(pMostRecentTaskedGroup->getPosition());
   }

   // Do we have time to act upon any groups?
   if (!canAct(currentGameTime) || !canThink(currentGameTime))
      return;

   // Overall mission fail test.
   if (processMissionFailedConditions())
      return;

   if (mSquads.getSize() == 0)
   {
      pAI->AIQTopicLotto(gWorld->getGametime());
      return;
   }

   // Increment our think AIQ so we don't infinitely spin on decisions.
   payTimeCostThink(currentGameTime);

   // Oh noes!  Run to the predetermined retreat position!
   /*if (mFlagManualRetreatPos)
   {
      BVector retreatPos(mManualRetreatPosX, 0.0f, mManualRetreatPosZ);
      gTerrainSimRep.getHeightRaycast(retreatPos, retreatPos.y, true);

      BTaskInfo retreatTask;
      retreatTask.setType(AIGroupTaskType::cMove);
      retreatTask.setTarget(retreatPos);
      taskGroups(mGroups, retreatTask);
   }
   // Oh noes!  We don't have a predetermined retreat position.  Guess!
   else*/
   {
      BKBBaseQuery kbbq;
      kbbq.setPlayerRelation(mPlayerID, cRelationTypeAlly);
      kbbq.setSelfAsAlly(true);
      kbbq.setRequireBuildings(true);

      const BKB* pKB = gWorld->getKB(pPlayer->getTeamID());
      if (pKB)
      {
         BVector centroid = BSimHelper::computeCentroid(mSquads);
         BEntityID centerSquadID = BSimHelper::computeClosestToPoint(centroid, mSquads);
         const BSquad* pSquad = gWorld->getSquad(centerSquadID);
         if (pSquad)
         {
            BKBBaseIDArray results;
            pKB->executeKBBaseQueryClosest(kbbq, pSquad->getPosition(), results);
            if (results.getSize() > 0)
            {
               const BKBBase* pKBBase = gWorld->getKBBase(results[0]);
               if (pKBBase)
               {
                  BTaskInfo retreatTask;
                  retreatTask.setType(AIGroupTaskType::cMove);
                  retreatTask.setTarget(pKBBase->getPosition());
                  taskGroups(mGroups, retreatTask);
               }
            }
         }
      }
   }

   setState(MissionState::cFailure);
   return;
}


//==============================================================================
//==============================================================================
void BAIMission::bulkGroupSetState(const BAIGroupIDArray& groupIDs, BAIGroupState groupState)
{
   uint numGroups = groupIDs.getSize();
   for (uint i=0; i<numGroups; i++)
   {
      BASSERT(mGroups.contains(groupIDs[i]));
      BAIGroup* pGroup = gWorld->getAIGroup(groupIDs[i]);
      if (pGroup)
         pGroup->setState(groupState);
   }
}


//==============================================================================
//==============================================================================
void BAIMission::taskGroups(const BAIGroupIDArray& groupIDs, const BTaskInfo& taskInfo, bool queueTask, bool moveAttack)
{
   // Storage space for the squads we are about to task.
   BEntityIDArray squadsToTask;
   squadsToTask.reserve(mSquads.getSize());

   // For "skirmish AI" do not do move attacks because our stuff is getting stalled by 'move attacking' stuff on the way to the target.
   const BAI* pAI = gWorld->getAI(mPlayerID);
   if (pAI && pAI->getFlagAIQ())
      moveAttack = false;

   // ---------------------------------------------------------------------------------
   // Step #1
   // Iterate through the groups and mark them up with the task info.
   // Collect the squads in these groups so they can be commanded in one command.
   //----------------------------------------------------------------------------------
   uint numGroups = groupIDs.getSize();
   for (uint i=0; i<numGroups; i++)
   {
      BAIGroup* pGroup = gWorld->getAIGroup(groupIDs[i]);
      if (!pGroup)
         continue;

      BASSERT(pGroup->getPlayerID() == mPlayerID);
      squadsToTask.append(pGroup->getSquads());

      // If this is not a queued order, blow away the previous tasks on all the groups.
      if (!queueTask)
         pGroup->deleteAllTasks();

      // Mark up the group with the task info.
      BAIGroupTask* pTask = gWorld->createAIGroupTask(groupIDs[i]);
      BASSERT(pTask);
      if (pTask)
      {
         pTask->setTargetPos(taskInfo.getTargetPos());
         pTask->setTargetID(taskInfo.getTargetID());
         pTask->setAbilityID(taskInfo.getAbilityID());
         pTask->setType(taskInfo.getType());
         pTask->setScore(taskInfo.getScore());
         pTask->setGroupPosWhenTasked(pGroup->getCentroid());
         pTask->setTimestampTasked(gWorld->getGametime());
      }

      pGroup->setTimestampLastTask(gWorld->getGametime());
      pGroup->setFlagTasked(true);
      pGroup->setFlagTaskedThisState(true);
   }

   // ---------------------------------------------------------------------------------
   // Step #2 - Take the squads contained in these groups and command them.
   //----------------------------------------------------------------------------------
   if (squadsToTask.getSize() > 0)
   {
      // Get them ready to task.
      BArmy* pArmy = BEntityGrouper::getOrCreateCommonArmyForSquads(squadsToTask, mPlayerID);
      BASSERT(pArmy);
      if (pArmy)
      {
         BVector targetPos = taskInfo.getTargetPos();
         BWorkCommand cmd;
         cmd.setUnitID(taskInfo.getTargetID());
         cmd.setWaypoints(&targetPos, 1);
         cmd.setAbilityID(taskInfo.getAbilityID());
         cmd.setRecipientType(BCommand::cArmy);
         cmd.setSenderType(BCommand::cPlayer);
         cmd.setFlag(BWorkCommand::cFlagAttackMove, moveAttack);
         cmd.setFlag(BCommand::cFlagAlternate, queueTask);
         pArmy->queueOrder(&cmd);
      }
   }
}


//==============================================================================
//==============================================================================
BAIGroup* BAIMission::getMostRecentlyTaskedGroup(const BAIGroupIDArray& groupIDs)
{
   BAIGroup* pMostRecent = NULL;
   DWORD mostRecentTime = 0;

   uint numGroups = groupIDs.getSize();
   for (uint i=0; i<numGroups; i++)
   {
      BAIGroup* pGroup = gWorld->getAIGroup(groupIDs[i]);
      if (pGroup && pGroup->getFlagTasked() && pGroup->getTimestampLastTask() > mostRecentTime)
      {
         pMostRecent = pGroup;
         mostRecentTime = pGroup->getTimestampLastTask();
      }
   }

   return (pMostRecent);
}


//==============================================================================
//==============================================================================
void BAIMission::getUntaskedThisStateGroups(BAIGroupIDArray& untaskedThisStateGroupIDs) const
{
   untaskedThisStateGroupIDs.resize(0);
   uint numGroups = mGroups.getSize();
   for (uint i=0; i<numGroups; i++)
   {
      const BAIGroup* pGroup = gWorld->getAIGroup(mGroups[i]);
      if (pGroup && !pGroup->getFlagTaskedThisState())
         untaskedThisStateGroupIDs.add(mGroups[i]);
   }
}


//==============================================================================
// Round-robin style...  Keep.  It.  Simple.  Stupid.  :)
//==============================================================================
BAIGroup* BAIMission::getNextGroupToTask()
{
   // Get the next working group.
   while (mWorkingGroups.getSize() > 0)
   {
      BAIGroup* pNextGroup = gWorld->getAIGroup(mWorkingGroups[0]);
      mWorkingGroups.removeIndex(0);
      if (pNextGroup)
         return (pNextGroup);
   }

   // Refresh working groups.
   mWorkingGroups = mGroups;

   // Try again.
   while (mWorkingGroups.getSize() > 0)
   {
      BAIGroup* pNextGroup = gWorld->getAIGroup(mWorkingGroups[0]);
      mWorkingGroups.removeIndex(0);
      if (pNextGroup)
         return (pNextGroup);
   }

   // Nothing left.
   return (NULL);
}


//==============================================================================
//==============================================================================
uint BAIMission::recalcStandardAttackTasks(const BAIGroup& group, BTaskInfoArray& potentialTasks)
{
   // Clear out our results.
   potentialTasks.reserve(64);
   potentialTasks.resize(0);

   // Iterate through all the targets and see what we can do.
   uint numKBSquads = mCache.mWorkDistVisibleEnemies.getSize();
   const BKBSquadIDArray& kbSquads = mCache.mWorkDistVisibleEnemies;
   BVector lastKnownPos;
   for (uint i=0; i<numKBSquads; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquads[i]);
      if (!pKBSquad)
         continue;
      if (!pKBSquad->getValidKnownPosition(lastKnownPos))
         continue;
      const BSquad* pTargetSquad = pKBSquad->getSquad();
      if (!pTargetSquad)
         continue;

      // Add a potential standard attack.
      BEntityID targetSquadID = pKBSquad->getSquadID();
//-- FIXING PREFIX BUG ID 4012
      const BProtoAction* pNormalAction = group.getProtoActionForTarget(targetSquadID, lastKnownPos, cInvalidAbilityID);
//--
      if (pNormalAction)
      {
         // Melee attacks can't be used against units in cover
         if (pNormalAction->isMeleeAttack() && pTargetSquad->isInCover())
            continue;

         // Base normalized value for how good we are at killing the target.
         float normalizedStars = group.getSquadAnalysis().getNormalizedStars(pKBSquad->getDamageType());
         // Modifier for our hatred for this type of target.
         float scoreModifier = mScoreModifier.GetScoreModifier(pTargetSquad);
         // Modifier for our player hatred.
         float playerModifier = mPlayerModifier.GetPlayerModifier(pTargetSquad->getPlayerID());
         // Modifier for being out of range by varying amounts.
         float rangeModifier = getRangeModifier(group, *pTargetSquad);
         // Modifier for the target being "only mostly dead"
         float mostlyDeadModifier = getMostlyDeadModifier(*pKBSquad);

         // Get our overall score and add the potential task.
         float overallScore = normalizedStars * scoreModifier * playerModifier * rangeModifier * mostlyDeadModifier;
         if (overallScore > 0.0f)
         {
            BTaskInfo infoToAdd;
            infoToAdd.setType(AIGroupTaskType::cAttack);
            infoToAdd.setTarget(targetSquadID);

            // Add cloaking to normal attacks.
            if (group.canDoCloakAbility())
               infoToAdd.setAbilityID(gDatabase.getAIDCommand());
            else
               infoToAdd.setAbilityID(cInvalidAbilityID);

            infoToAdd.setScore(overallScore);
            potentialTasks.add(infoToAdd);
         }
      }
   }

   // Return our count.
   return (potentialTasks.getSize());
}


//==============================================================================
//==============================================================================
const BTaskInfo* BAIMission::getHighestScoringTask(const BAIGroup& group, const BTaskInfoArray& potentialTasks)
{
   const BTaskInfo* pBestTask = NULL;
   float bestTaskScore = -cMaximumFloat;  // In case we allow negative scores.

   uint numTasks = potentialTasks.getSize();
   for (uint i=0; i<numTasks; i++)
   {
      if (potentialTasks[i].getScore() > bestTaskScore)
      {
         bestTaskScore = potentialTasks[i].getScore();
         pBestTask = &potentialTasks[i];
      }
   }

   return (pBestTask);
}


//==============================================================================
//==============================================================================
uint BAIMission::recalcDamageAbilityTasks(BAIGroup& group, BTaskInfoArray& potentialTasks)
{
   // Clear out our results.
   potentialTasks.reserve(64);
   potentialTasks.resize(0);

   // Iterate through all the targets and see what we can do.
   uint numKBSquads = mCache.mWorkDistVisibleEnemies.getSize();
   const BKBSquadIDArray& kbSquads = mCache.mWorkDistVisibleEnemies;
   for (uint i=0; i<numKBSquads; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquads[i]);
      if (!pKBSquad)
         continue;
      BVector lastKnownPos;
      if (!pKBSquad->getValidKnownPosition(lastKnownPos))
         continue;
      const BSquad* pTargetSquad = pKBSquad->getSquad();
      if (!pTargetSquad)
         continue;
      // Make sure we can do this ability to this target.
      BEntityID targetSquadID = pKBSquad->getSquadID();
      group.updateSelectionAbility(targetSquadID, lastKnownPos);
      if (!group.getSelectionAbility().mValid)
         continue;
      // Make sure we have an attack action and it does something.
//-- FIXING PREFIX BUG ID 4014
      const BProtoAction* pAbilityAction = group.getProtoActionForTarget(targetSquadID, lastKnownPos, gDatabase.getAIDCommand());
//--
      if (!pAbilityAction)
         continue;

      // Melee attacks can't be used against units in cover
      if (pAbilityAction->isMeleeAttack() && pTargetSquad->isInCover())
         continue;
      
      // Base ability DPS for killing the target.
      float abilityDPS = pAbilityAction->getDamagePerSecond();
      if (abilityDPS <= 0.0f)
         continue;
      // Modifier for our hatred for this type of target.
      float scoreModifier = mScoreModifier.GetScoreModifier(pTargetSquad);
      // Modifier for our player hatred.
      float playerModifier = mPlayerModifier.GetPlayerModifier(pTargetSquad->getPlayerID());
      // Modifier for being out of range by varying amounts.
      float rangeModifier = getRangeModifier(group, *pTargetSquad);
      // Modifier for the target being "only mostly dead"
      float mostlyDeadModifier = getMostlyDeadModifier(*pKBSquad);
      // Modifier for the amount of overkill being applied to the target.
      float overkillModifier = 1.0f;  // marchack
      // Get our overall score and add the potential task.
      float overallScore = abilityDPS * scoreModifier * playerModifier * rangeModifier * mostlyDeadModifier * overkillModifier;
      if (overallScore > 0.0f)
      {
         BTaskInfo infoToAdd;
         infoToAdd.setTarget(targetSquadID);
         infoToAdd.setAbilityID(gDatabase.getAIDCommand());
         infoToAdd.setType(AIGroupTaskType::cAttack);
         infoToAdd.setScore(overallScore);
         potentialTasks.add(infoToAdd);
      }
   }

   // Return our count.
   return (potentialTasks.getSize());
}


//==============================================================================
//==============================================================================
uint BAIMission::recalcSuicideAbilityTasks(BAIGroup& group, BTaskInfoArray& potentialTasks)
{
   // Clear out our results.
   potentialTasks.reserve(64);
   potentialTasks.resize(0);

   // We are over-binding this to be both permission and bias in scoring.
   if (mScoreModifier.mMultipliers.getSize() == 0)
      return (0);

   // Iterate through all the targets and see what we can do.
   uint numKBSquads = mCache.mWorkDistVisibleEnemies.getSize();
   const BKBSquadIDArray& kbSquads = mCache.mWorkDistVisibleEnemies;
   for (uint i=0; i<numKBSquads; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquads[i]);
      if (!pKBSquad)
         continue;
      BVector lastKnownPos;
      if (!pKBSquad->getValidKnownPosition(lastKnownPos))
         continue;
      const BSquad* pTargetSquad = pKBSquad->getSquad();
      if (!pTargetSquad)
         continue;

      float scoreModifier = mScoreModifier.GetScoreModifier(pTargetSquad, 0.0f);
      if (scoreModifier <= 0.0f)
         continue;

      float playerModifier = mPlayerModifier.GetPlayerModifier(pTargetSquad->getPlayerID(), 1.0f);
      if (playerModifier <= 0.0f)
         continue;

      // Make sure we can do this ability to this target.
      BEntityID targetSquadID = pKBSquad->getSquadID();
      group.updateSelectionAbility(targetSquadID, lastKnownPos);
      if (!group.getSelectionAbility().mValid)
         continue;
      // Make sure we have an attack action and it does something.
      BProtoAction* pAbilityAction = group.getProtoActionForTarget(targetSquadID, lastKnownPos, gDatabase.getAIDCommand());
      if (!pAbilityAction)
         continue;

      // Base ability DPS for killing the target.
      float abilityDPS = pAbilityAction->getDamagePerSecond();
      if (abilityDPS <= 0.0f)
         continue;
      // Modifier for being out of range by varying amounts.
      float rangeModifier = getRangeModifier(group, *pTargetSquad);
      // Modifier for the target being "only mostly dead"
      float mostlyDeadModifier = getMostlyDeadModifier(*pKBSquad);
      // Modifier for the amount of overkill being applied to the target.
      float overkillModifier = 1.0f;  // marchack
      // Get our overall score and add the potential task.
      float overallScore = abilityDPS * scoreModifier * playerModifier * rangeModifier * mostlyDeadModifier * overkillModifier;
      if (overallScore > 0.0f)
      {
         BTaskInfo infoToAdd;
         infoToAdd.setTarget(targetSquadID);
         infoToAdd.setAbilityID(gDatabase.getAIDCommand());
         infoToAdd.setType(AIGroupTaskType::cDetonate);
         infoToAdd.setScore(overallScore);
         potentialTasks.add(infoToAdd);
      }
   }

   // Return our count.
   return (potentialTasks.getSize());
}



//==============================================================================
//==============================================================================
uint BAIMission::recalcJumppackAbilityTasks(BAIGroup& group, BTaskInfoArray& potentialTasks)
{
   // Clear out our results.
   potentialTasks.reserve(64);
   potentialTasks.resize(0);

   // Iterate through all the targets and see what we can do.
   uint numKBSquads = mCache.mWorkDistVisibleEnemies.getSize();
   const BKBSquadIDArray& kbSquads = mCache.mWorkDistVisibleEnemies;
   for (uint i=0; i<numKBSquads; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquads[i]);
      if (!pKBSquad)
         continue;
      BVector lastKnownPos;
      if (!pKBSquad->getValidKnownPosition(lastKnownPos))
         continue;
      const BSquad* pTargetSquad = pKBSquad->getSquad();
      if (!pTargetSquad)
         continue;
      // Make sure we can do this ability to this target.
      BEntityID targetSquadID = pKBSquad->getSquadID();
      group.updateSelectionAbility(targetSquadID, lastKnownPos);
      if (!group.getSelectionAbility().mValid)
         continue;
      // Make sure we have an attack action and it does something.
      BProtoAction* pAbilityAction = group.getProtoActionForTarget(targetSquadID, lastKnownPos, gDatabase.getAIDCommand());
      if (!pAbilityAction)
         continue;

      // Base ability DPS for killing the target.
      float abilityDPS = pAbilityAction->getDamagePerSecond();
      if (abilityDPS <= 0.0f)
         continue;
      // Modifier for our hatred for this type of target.
      float scoreModifier = mScoreModifier.GetScoreModifier(pTargetSquad);
      // Modifier for our player hatred.
      float playerModifier = mPlayerModifier.GetPlayerModifier(pTargetSquad->getPlayerID());
      // Modifier for being out of range by varying amounts.
      float rangeModifier = 1.0f;//getRangeModifier(group, *pTargetSquad);
      // Modifier for the target being "only mostly dead"
      float mostlyDeadModifier = getMostlyDeadModifier(*pKBSquad);
      // Modifier for the amount of overkill being applied to the target.
      float overkillModifier = 1.0f;  // marchack
      // Get our overall score and add the potential task.
      float overallScore = abilityDPS * scoreModifier * playerModifier * rangeModifier * mostlyDeadModifier * overkillModifier;
      if (overallScore > 0.0f)
      {
         BTaskInfo infoToAdd;
         infoToAdd.setTarget(targetSquadID);
         infoToAdd.setAbilityID(gDatabase.getAIDCommand());
         infoToAdd.setType(AIGroupTaskType::cMove);
         infoToAdd.setScore(overallScore);
         potentialTasks.add(infoToAdd);
      }
   }

   // Return our count.
   return (potentialTasks.getSize());
}



//==============================================================================
//==============================================================================
uint BAIMission::recalcOverburnAbilityTasks(BAIGroup& group, BTaskInfoArray& potentialTasks)
{
   // Clear out our results.
   potentialTasks.reserve(64);
   potentialTasks.resize(0);

   // Iterate through all the targets and see what we can do.
   uint numKBSquads = mCache.mWorkDistVisibleEnemies.getSize();
   const BKBSquadIDArray& kbSquads = mCache.mWorkDistVisibleEnemies;
   for (uint i=0; i<numKBSquads; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquads[i]);
      if (!pKBSquad)
         continue;
      BVector lastKnownPos;
      if (!pKBSquad->getValidKnownPosition(lastKnownPos))
         continue;
      const BSquad* pTargetSquad = pKBSquad->getSquad();
      if (!pTargetSquad)
         continue;
      // Make sure we can do this ability to this target.
      BEntityID targetSquadID = pKBSquad->getSquadID();
      group.updateSelectionAbility(targetSquadID, lastKnownPos);
      if (!group.getSelectionAbility().mValid)
         continue;
      // Make sure we have an attack action and it does something.
      BProtoAction* pAbilityAction = group.getProtoActionForTarget(targetSquadID, lastKnownPos, gDatabase.getAIDCommand());
      if (!pAbilityAction)
         continue;

      // Base ability DPS for killing the target.
      float abilityDPS = pAbilityAction->getDamagePerSecond();
      if (abilityDPS <= 0.0f)
         continue;
      // Modifier for our hatred for this type of target.
      float scoreModifier = mScoreModifier.GetScoreModifier(pTargetSquad);
      // Modifier for our player hatred.
      float playerModifier = mPlayerModifier.GetPlayerModifier(pTargetSquad->getPlayerID());
      // Modifier for being out of range by varying amounts.
      float rangeModifier = getRangeModifier(group, *pTargetSquad);
      // Modifier for the target being "only mostly dead"
      float mostlyDeadModifier = getMostlyDeadModifier(*pKBSquad);
      // Modifier for the amount of overkill being applied to the target.
      float overkillModifier = 1.0f;  // marchack
      // Get our overall score and add the potential task.
      float overallScore = abilityDPS * scoreModifier * playerModifier * rangeModifier * mostlyDeadModifier * overkillModifier;
      if (overallScore > 0.0f)
      {
         BTaskInfo infoToAdd;
         infoToAdd.setTarget(targetSquadID);
         infoToAdd.setAbilityID(gDatabase.getAIDCommand());
         infoToAdd.setType(AIGroupTaskType::cAttack);
         infoToAdd.setScore(overallScore);
         potentialTasks.add(infoToAdd);
      }
   }

   // Return our count.
   return (potentialTasks.getSize());
}



//==============================================================================
//==============================================================================
uint BAIMission::recalcFlashBangAbilityTasks(BAIGroup& group, BTaskInfoArray& potentialTasks)
{
   // Clear out our results.
   potentialTasks.reserve(64);
   potentialTasks.resize(0);

   // Iterate through all the targets and see what we can do.
   uint numKBSquads = mCache.mWorkDistVisibleEnemies.getSize();
   const BKBSquadIDArray& kbSquads = mCache.mWorkDistVisibleEnemies;
   for (uint i=0; i<numKBSquads; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquads[i]);
      if (!pKBSquad)
         continue;
      BVector lastKnownPos;
      if (!pKBSquad->getValidKnownPosition(lastKnownPos))
         continue;
      const BSquad* pTargetSquad = pKBSquad->getSquad();
      if (!pTargetSquad)
         continue;
      // We don't want to waste extra flash bangs on already immobilized squads.
      if (pTargetSquad->getFlagDazeImmobilized())
         continue;
      // Make sure we can do this ability to this target.
      BEntityID targetSquadID = pKBSquad->getSquadID();
      group.updateSelectionAbility(targetSquadID, lastKnownPos);
      if (!group.getSelectionAbility().mValid)
         continue;
      // Make sure we have an attack action and it does something.
      BProtoAction* pAbilityAction = group.getProtoActionForTarget(targetSquadID, lastKnownPos, gDatabase.getAIDCommand());
      if (!pAbilityAction)
         continue;

      // The score for flash bang attacks on enemy infantry is simply the enemy infantry hitpoints for now (stun the healthy ones)
      // Next we probably want to stun the guys who can hurt us the most.

      // Modifier for our hatred for this type of target.
      float scoreModifier = mScoreModifier.GetScoreModifier(pTargetSquad);     
      float playerModifier = mPlayerModifier.GetPlayerModifier(pTargetSquad->getPlayerID());
      // Modifier for being out of range by varying amounts.
      float rangeModifier = getRangeModifier(group, *pTargetSquad);
      // Modifier for the overal health of the enemy.
      float enemyHealthModifier = pKBSquad->getHPPercentage();
      // Get our overall score and add the potential task.
      float overallScore = scoreModifier * playerModifier * rangeModifier * enemyHealthModifier;
      if (overallScore > 0.0f)
      {
         BTaskInfo infoToAdd;
         infoToAdd.setTarget(targetSquadID);
         infoToAdd.setAbilityID(gDatabase.getAIDCommand());
         infoToAdd.setType(AIGroupTaskType::cAttack);
         infoToAdd.setScore(overallScore);
         potentialTasks.add(infoToAdd);
      }
   }

   // Return our count.
   return (potentialTasks.getSize());
}


//==============================================================================
//==============================================================================
uint BAIMission::recalcStasisAbilityTasks(BAIGroup& group, BTaskInfoArray& potentialTasks)
{
   // Clear out our results.
   potentialTasks.reserve(64);
   potentialTasks.resize(0);

   // Iterate through all the targets and see what we can do.
   uint numKBSquads = mCache.mWorkDistVisibleEnemies.getSize();
   const BKBSquadIDArray& kbSquads = mCache.mWorkDistVisibleEnemies;
   for (uint i=0; i<numKBSquads; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquads[i]);
      if (!pKBSquad)
         continue;
      BVector lastKnownPos;
      if (!pKBSquad->getValidKnownPosition(lastKnownPos))
         continue;
      const BSquad* pTargetSquad = pKBSquad->getSquad();
      if (!pTargetSquad)
         continue;
      // We don't want to waste extra flash bangs on already immobilized squads.
      if (pTargetSquad->getFlagDazeImmobilized())
         continue;
      // Make sure we can do this ability to this target.
      BEntityID targetSquadID = pKBSquad->getSquadID();
      group.updateSelectionAbility(targetSquadID, lastKnownPos);
      if (!group.getSelectionAbility().mValid)
         continue;
      // Make sure we have an attack action and it does something.
      BProtoAction* pAbilityAction = group.getProtoActionForTarget(targetSquadID, lastKnownPos, gDatabase.getAIDCommand());
      if (!pAbilityAction)
         continue;

      // The score for flash bang attacks on enemy infantry is simply the enemy infantry hitpoints for now (stun the healthy ones)
      // Next we probably want to stun the guys who can hurt us the most.

      // Modifier for our hatred for this type of target.
      float scoreModifier = mScoreModifier.GetScoreModifier(pTargetSquad);     
      float playerModifier = mPlayerModifier.GetPlayerModifier(pTargetSquad->getPlayerID());
      // Modifier for being out of range by varying amounts.
      float rangeModifier = getRangeModifier(group, *pTargetSquad);
      // Modifier for the overal health of the enemy.
      float enemyHealthModifier = pKBSquad->getHPPercentage();
      // Get our overall score and add the potential task.
      float overallScore = scoreModifier * playerModifier * rangeModifier * enemyHealthModifier;
      if (overallScore > 0.0f)
      {
         BTaskInfo infoToAdd;
         infoToAdd.setTarget(targetSquadID);
         infoToAdd.setAbilityID(gDatabase.getAIDCommand());
         infoToAdd.setType(AIGroupTaskType::cAttack);
         infoToAdd.setScore(overallScore);
         potentialTasks.add(infoToAdd);
      }
   }

   // Return our count.
   return (potentialTasks.getSize());
}



//==============================================================================
//==============================================================================
uint BAIMission::recalcRamAbilityTasks(BAIGroup& group, BTaskInfoArray& potentialTasks)
{
   // Clear out our results.
   potentialTasks.reserve(64);
   potentialTasks.resize(0);

   BObjectTypeID otidInfantry = gDatabase.getOTIDInfantry();

   // Iterate through all the targets and see what we can do.
   uint numKBSquads = mCache.mWorkDistVisibleEnemies.getSize();
   const BKBSquadIDArray& kbSquads = mCache.mWorkDistVisibleEnemies;
   for (uint i=0; i<numKBSquads; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquads[i]);
      if (!pKBSquad)
         continue;
      BVector lastKnownPos;
      if (!pKBSquad->getValidKnownPosition(lastKnownPos))
         continue;
      const BSquad* pTargetSquad = pKBSquad->getSquad();
      if (!pTargetSquad)
         continue;
      const BUnit* pLeaderUnit = pTargetSquad->getLeaderUnit();
      if (!pLeaderUnit)
         continue;
      if (!pLeaderUnit->isType(otidInfantry))   // we only want to run over infantry or else we get hurt bad.
         continue;
      // Make sure we can do this ability to this target.
      BEntityID targetSquadID = pKBSquad->getSquadID();
      group.updateSelectionAbility(targetSquadID, lastKnownPos);
      if (!group.getSelectionAbility().mValid)
         continue;
      // Make sure we have an attack action and it does something.
      BProtoAction* pAbilityAction = group.getProtoActionForTarget(targetSquadID, lastKnownPos, gDatabase.getAIDCommand());
      if (!pAbilityAction)
         continue;

      // The score for flash bang attacks on enemy infantry is simply the enemy infantry hitpoints for now (stun the healthy ones)
      // Next we probably want to stun the guys who can hurt us the most.

      // Modifier for our hatred for this type of target.
      float scoreModifier = mScoreModifier.GetScoreModifier(pTargetSquad);     
      float playerModifier = mPlayerModifier.GetPlayerModifier(pTargetSquad->getPlayerID());
      // Modifier for being out of range by varying amounts.
      float rangeModifier = getRangeModifier(group, *pTargetSquad);
      // Modifier for the overal health of the enemy.
      float mostlyDeadModifier = getMostlyDeadModifier(*pKBSquad);

      // Get our overall score and add the potential task.
      float overallScore = scoreModifier * playerModifier * rangeModifier * mostlyDeadModifier;
      if (overallScore > 0.0f)
      {
         BTaskInfo infoToAdd;
         infoToAdd.setTarget(targetSquadID);
         infoToAdd.setAbilityID(gDatabase.getAIDCommand());
         infoToAdd.setType(AIGroupTaskType::cAttack);
         infoToAdd.setScore(overallScore);
         potentialTasks.add(infoToAdd);
      }
   }

   // Return our count.
   return (potentialTasks.getSize());
}


//==============================================================================
//==============================================================================
uint BAIMission::recalcHijackAbilityTasks(BAIGroup& group, BTaskInfoArray& potentialTasks)
{
   // Clear out our results.
   potentialTasks.reserve(64);
   potentialTasks.resize(0);

   // Iterate through all the targets and see what we can do.
   uint numKBSquads = mCache.mWorkDistVisibleEnemies.getSize();
   const BKBSquadIDArray& kbSquads = mCache.mWorkDistVisibleEnemies;
   for (uint i=0; i<numKBSquads; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquads[i]);
      if (!pKBSquad)
         continue;
      BVector lastKnownPos;
      if (!pKBSquad->getValidKnownPosition(lastKnownPos))
         continue;
      const BSquad* pTargetSquad = pKBSquad->getSquad();
      if (!pTargetSquad)
         continue;
      // Make sure we can do this ability to this target.
      BEntityID targetSquadID = pKBSquad->getSquadID();
      group.updateSelectionAbility(targetSquadID, lastKnownPos);
      if (!group.getSelectionAbility().mValid)
         continue;
      // Make sure we have an attack action and it does something.
//-- FIXING PREFIX BUG ID 4016
      const BProtoAction* pAbilityAction = group.getProtoActionForTarget(targetSquadID, lastKnownPos, gDatabase.getAIDCommand());
//--
      if (!pAbilityAction)
         continue;
      if (pAbilityAction->getActionType() != BAction::cActionTypeUnitJoin)
         continue;

      // Base ability DPS for killing the target.
      //float abilityDPS = pAbilityAction->getDamagePerSecond();
      //if (abilityDPS <= 0.0f)
      //   continue;
      // Modifier for our hatred for this type of target.
      float scoreModifier = mScoreModifier.GetScoreModifier(pTargetSquad);
      float playerModifier = mPlayerModifier.GetPlayerModifier(pTargetSquad->getPlayerID());
      // Modifier for being out of range by varying amounts.
      float rangeModifier = getRangeModifier(group, *pTargetSquad);
      // Modifier for the target being "only mostly dead"
      //float mostlyDeadModifier = getMostlyDeadModifier(*pKBSquad);
      float mostlyAliveModifier = pTargetSquad->getHPPercentage();
      // Modifier for the amount of overkill being applied to the target.
      //float overkillModifier = 1.0f;  // marchack
      // Get our overall score and add the potential task.
      //float overallScore = abilityDPS * scoreModifier * rangeModifier * mostlyDeadModifier * overkillModifier;
      float overallScore = scoreModifier * playerModifier * rangeModifier * mostlyAliveModifier;
      if (overallScore > 0.0f)
      {
         BTaskInfo infoToAdd;
         infoToAdd.setType(AIGroupTaskType::cHijack);
         infoToAdd.setTarget(targetSquadID);
         infoToAdd.setAbilityID(gDatabase.getAIDCommand());
         infoToAdd.setScore(overallScore);
         potentialTasks.add(infoToAdd);
      }
   }

   // Return our count.
   return (potentialTasks.getSize());
}


//==============================================================================
//==============================================================================
uint BAIMission::recalcScoutStaleTargetTasks(const BAIGroup& group, BTaskInfoArray& potentialTasks)
{
   // Clear out our results.
   potentialTasks.reserve(64);
   potentialTasks.resize(0);

   // Iterate through all the targets and add as a possible 
   uint numKBSquads = mCache.mWorkDistDoppledEnemies.getSize();
   const BKBSquadIDArray& kbSquads = mCache.mWorkDistDoppledEnemies;
   for (uint i=0; i<numKBSquads; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquads[i]);
      if (!pKBSquad)
         continue;
      BVector groupPos = group.getPosition();
      BVector lastKnownPos;
      if (!pKBSquad->getValidKnownPosition(lastKnownPos))
         continue;

      // Overall score is just the negative distance sqr, so the closest target will have the highest score.
      float negDistSqr = -(groupPos.xzDistanceSqr(lastKnownPos));

      BTaskInfo infoToAdd;
      infoToAdd.setType(AIGroupTaskType::cMove);
      infoToAdd.setTarget(lastKnownPos);
      infoToAdd.setScore(negDistSqr);
      potentialTasks.add(infoToAdd);
   }

   // Return our count.
   return (potentialTasks.getSize());
}


//==============================================================================
//==============================================================================
uint BAIMission::recalcGatherTasks(const BAIGroup& group, BTaskInfoArray& potentialTasks)
{
   // Clear out our results.
   potentialTasks.reserve(64);
   potentialTasks.resize(0);

   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (0);
   const BKB* pKB = gWorld->getKB(pPlayer->getTeamID());
   if (!pKB)
      return (0);

   BVector groupPos = group.getPosition();

   // Iterate through all the targets and add as a possible 
   uint numGatherables = mCache.mWorkDistGatherables.getSize();
   for (uint i=0; i<numGatherables; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(mCache.mWorkDistGatherables[i]);
      if (!pKBSquad)
         continue;

      BVector lastKnownPos;
      if (!pKBSquad->getValidKnownPosition(lastKnownPos))
         continue;

      // Overall score is just the negative distance sqr, so the closest target will have the highest score.
      float negDistSqr = -(groupPos.xzDistanceSqr(lastKnownPos));

      BTaskInfo infoToAdd;
      infoToAdd.setType(AIGroupTaskType::cGather);
      infoToAdd.setTarget(pKBSquad->getSquadID());
      infoToAdd.setScore(negDistSqr);
      potentialTasks.add(infoToAdd);
   }

   // Return our count.
   return (potentialTasks.getSize());
}


//==============================================================================
//==============================================================================
uint BAIMission::recalcRepairOtherTasks(const BAIGroup& group, BTaskInfoArray& potentialTasks)
{
   // Clear out our results.
   potentialTasks.reserve(64);
   potentialTasks.resize(0);

   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (0);
   const BKB* pKB = gWorld->getKB(pPlayer->getTeamID());
   if (!pKB)
      return (0);

   BVector groupPos = group.getPosition();

   // Iterate through all the targets and add as a possible 
   uint numMissionSquads = mSquads.getSize();
   for (uint i=0; i<numMissionSquads; i++)
   {
      // Bogus squad or squad in this group, skip.
      const BSquad* pSquad = gWorld->getSquad(mSquads[i]);
      if (!pSquad)
         continue;
      if (group.containsSquad(mSquads[i]))
         continue;
      if (pSquad->getHPPercentage() >= 1.0f)
         continue;

      // Overall score is just the negative distance sqr, so the closest target will have the highest score.
      float negDistSqr = -(groupPos.xzDistanceSqr(pSquad->getPosition()));

      BTaskInfo infoToAdd;
      infoToAdd.setType(AIGroupTaskType::cRepairOther);
      infoToAdd.setTarget(mSquads[i]);
      infoToAdd.setScore(negDistSqr);
      potentialTasks.add(infoToAdd);
   }

   // Return our count.
   return (potentialTasks.getSize());
}


//==============================================================================
//==============================================================================
bool BAIMission::recalcBestOrbitalLoc(float orbitalRadius, const BEntityIDArray& targetEnemyUnits, BVector& resultVec)
{
   BEntityGrouper* pEntityGrouper = gWorld->getEntityGrouper();
   pEntityGrouper->reset();
   pEntityGrouper->setRadius(orbitalRadius);
   pEntityGrouper->groupEntities(targetEnemyUnits);

   //const BEntityGroup* pBestGroup = NULL;
   BVector bestCentroid;   

   // Iterate through all our groups and try to find one to cast upon
   uint numGroups = pEntityGrouper->getNumberGroups();
   for (uint g=0; g<numGroups; g++)
   {
      const BEntityGroup* pGroup = pEntityGrouper->getGroupByIndex(g);
      BASSERT(pGroup);
      if (!pGroup)
         continue;

      //float totalGroupUnitsHP = 0.0f;
      const BEntityIDArray& groupUnits = pGroup->getEntities();
      uint numGroupUnits = groupUnits.getSize();
      if (numGroupUnits <= 0)
         continue;

      //for (uint s=0; s<numGroupUnits; s++)
     // {
      //   const BUnit* pGroupUnit = gWorld->getUnit(groupUnits[s]);
      //   if (pGroupUnit)
      //      totalGroupUnitsHP += pGroupUnit->getHitpoints();
      //}

      //if (totalGroupUnitsHP <= cMinHPToOrbital)
      //   continue;

      BVector groupCentroid = BSimHelper::computeCentroid(groupUnits);
      BEntityID centerUnit = BSimHelper::computeClosestToPoint(groupCentroid, groupUnits);
      const BUnit* pCenterUnit = gWorld->getUnit(centerUnit);
      if (!pCenterUnit)
         continue;



      // At this point we are going to 
      // Try to launch the power given the parameters here.
      resultVec = pCenterUnit->getPosition();
      return (true);
   }

   return (false);
}


//==============================================================================
//==============================================================================
bool BAIMission::processPowerLaunchODST(DWORD currentGameTime)
{
   if (!canODSTEval(currentGameTime))
      return (false);

   payTimeCostODSTEval(currentGameTime);

   if (!mFlagEnableOdstDrop)
      return (false);

   BProtoPowerID protoPowerID = gDatabase.getPPIDUnscOdstDrop();
   const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
   if (!pProtoPower)
      return (false);
   const BAIMissionTargetWrapper* pCurrentTargetWrapper = getCurrentTargetWrapper();
   if (!pCurrentTargetWrapper)
      return (false);
   const BAIMissionTarget* pCurrentMissionTarget = gWorld->getAIMissionTarget(pCurrentTargetWrapper->getTargetID());
   if (!pCurrentMissionTarget)
      return (false);
   BPowerManager* pPowerManager = gWorld->getPowerManager();
   if (!pPowerManager)
      return (false);
   BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (false);
   BPowerLevel powerLevel = pPlayer->getPowerLevel(protoPowerID);
   if (!pProtoPower->isValidPowerLevel(powerLevel))
      return (false);
   if (!pPlayer->canUsePower(protoPowerID))
      return (false);
   //int firstUseLimit = pPlayer->getPowerUseLimit(protoPowerID);
   //if (firstUseLimit <= 0)
   //   return (false);
   if (!pPlayer->hasAvailablePowerEntryUses(protoPowerID))
      return (false);
   if (mSquads.getSize() == 0)
      return (false);
   const BSquad* pSquad = gWorld->getSquad(mSquads[0]);
   if (!pSquad)
      return (false);
   BPower* pNewPower = pPowerManager->createNewPower(protoPowerID);
   if (!pNewPower)
      return (false);
   // Initialize it - if init() fails, it automatically flags the power to be cleaned up, so we don't have to delete anything here.
   if (!pNewPower->init(mPlayerID, powerLevel, cInvalidPowerUserID, cInvalidObjectID, pCurrentMissionTarget->getPosition()))
      return (false);


   BPowerODST* pPowerODST = static_cast<BPowerODST*>(pNewPower);
   pPowerODST->setAddToMissionID(mID);

   // marchack new stuff -- search out a bit.
   long searchScale = 8; //Math::Max(1, Math::FloatToIntTrunc(pProtoPower->getUIRadius() * gTerrainSimRep.getReciprocalDataTileScale()));

   BVector tempSuggestedLocation = XMVectorReplicate(-1.0f);
   DWORD flags = BWorld::cCPCheckObstructions | BWorld::cCPCheckSquadObstructions | BWorld::cCPExpandHull | BWorld::cCPPushOffTerrain | BWorld::cCPLOSCenterOnly | BWorld::cCPSetPlacementSuggestion;
   long losMode = BWorld::cCPLOSFullVisible; // marchack
   BProtoObjectID odstProtoObjectID = gDatabase.getProtoObject("unsc_inf_odst_01");
   BProtoSquadID odstProtoSquadID = gDatabase.getProtoSquad("unsc_inf_odst_01");
   BVector desiredPos = pSquad->getPosition();
   BPowerInput powerInput;

   if(gWorld->checkPlacement(odstProtoObjectID,  mPlayerID, desiredPos, tempSuggestedLocation, cZAxisVector, losMode, flags, searchScale, NULL, NULL, -1, odstProtoSquadID, true))
   {
      BVector suggestedLocation = tempSuggestedLocation;
      if (tempSuggestedLocation == XMVectorReplicate(-1.0f))
         suggestedLocation = desiredPos;

      powerInput.mType = PowerInputType::cUserOK;
      powerInput.mVector = suggestedLocation;
      pNewPower->submitInput(powerInput);
   }

   powerInput.mType = PowerInputType::cUserCancel;
   pNewPower->submitInput(powerInput);

   payTimeCostAct(currentGameTime);

   return (true);
}


//==============================================================================
//==============================================================================
bool BAIMission::processPowerLaunch(DWORD currentGameTime)
{
   if (!canPowerEval(currentGameTime))
      return (false);

   payTimeCostPowerEval(currentGameTime);

   // We need to have a bid for the power to continue.
   uint numBidIDs = mBidIDs.getSize();
   if (numBidIDs == 0)
      return (false);

   bool launchPowerResult = false;

   // Iterate through our power bids that have been assigned to this mission.
   for (uint i=0; i<numBidIDs; i++)
   {
      // The bid is crap - Just reset it and we'll clean up when we return.
      BBid* pBid = gWorld->getBid(mBidIDs[i]);
      if (!pBid)
      {
         mBidIDs[i] = cInvalidBidID;
         continue;
      }

      // The bid is the wrong type (this is an error case.)
      BASSERTM(pBid->isType(BidType::cPower), "Non-power BidType assigned to AIMission.");
      if (!pBid->isType(BidType::cPower))
      {
         mBidIDs[i] = cInvalidBidID;
         continue;
      }

      // The bid is not approved at this time.
      if (!pBid->isState(BidState::cApproved))
         continue;

      // Bad proto power.
      BProtoPowerID protoPowerID = pBid->getPowerToCast();
//-- FIXING PREFIX BUG ID 4018
      const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
//--
      if (!pProtoPower)
      {
         mBidIDs[i] = cInvalidBidID;
         continue;
      }

      BPowerType powerType = pProtoPower->getPowerType();
      if (powerType == PowerType::cCleansing)
      {
         if (processPowerLaunchCleansing(*pBid, protoPowerID))
         {
            mBidIDs[i] = cInvalidBidID;
            launchPowerResult = true;
            break;
         }
      }
      else if (powerType == PowerType::cOrbital)
      {
         if (processPowerLaunchOrbital(*pBid, protoPowerID))
         {
            mBidIDs[i] = cInvalidBidID;
            launchPowerResult = true;
            break;
         }
      }
      else if (powerType == PowerType::cCarpetBombing)
      {
         if (processPowerLaunchCarpetBombing(*pBid, protoPowerID))
         {
            mBidIDs[i] = cInvalidBidID;
            launchPowerResult = true;
            break;
         }
      }
      else if (powerType == PowerType::cCryo)
      {
         if (processPowerLaunchCryo(*pBid, protoPowerID))
         {
            mBidIDs[i] = cInvalidBidID;
            launchPowerResult = true;
            break;
         }
      }
      else if (powerType == PowerType::cRage)
      {
         if (processPowerLaunchRage(*pBid, protoPowerID))
         {
            mBidIDs[i] = cInvalidBidID;
            launchPowerResult = true;
            break;
         }
      }
      else if (powerType == PowerType::cWave)
      {
         if (processPowerLaunchWave(*pBid, protoPowerID))
         {
            mBidIDs[i] = cInvalidBidID;
            launchPowerResult = true;
            break;
         }
      }
      else if (powerType == PowerType::cDisruption)
      {
         if (processPowerLaunchDisruption(*pBid, protoPowerID))
         {
            mBidIDs[i] = cInvalidBidID;
            launchPowerResult = true;
            break;
         }
      }
      else if (powerType == PowerType::cRepair)
      {
         if (processPowerLaunchRepair(*pBid, protoPowerID))
         {
            mBidIDs[i] = cInvalidBidID;
            launchPowerResult = true;
            break;
         }
      }
      else
      {
         BFAIL("Unsupported power type.");
         continue;
      }
   }

   mBidIDs.removeValueAllInstances(cInvalidBidID);
   return (launchPowerResult);
}


//==============================================================================
//==============================================================================
bool BAIMission::processPowerLaunchCleansing(BBid& rBid, BProtoPowerID protoPowerID)
{
   // Step 1 - Basic verification that we can do this power.
   BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (false);

   const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
   if (!pProtoPower)
      return (false);
   BPowerLevel powerLevel = pPlayer->getPowerLevel(protoPowerID);
   if (!pProtoPower->isValidPowerLevel(powerLevel))
      return (false);
   if (!pPlayer->canUsePower(protoPowerID))
      return (false);
   const BUnit* pLeaderUnit = gWorld->getUnit(pPlayer->getLeaderUnit());
   if (!pLeaderUnit)
      return (false);
   const BSquad* pHeroSquad = pLeaderUnit->getParentSquad();
   if (!pHeroSquad)
      return (false);

   // Step 2 - Get some data.
   BVector heroSquadPos = pHeroSquad->getPosition();
   float minBeamDist = 0.0f;
   float maxBeamDist = 0.0f;
   pProtoPower->getDataFloat(powerLevel, "MinBeamDistance", minBeamDist);
   pProtoPower->getDataFloat(powerLevel, "MaxBeamDistance", maxBeamDist);

   // Step 3 - How good of an idea is this?
   //---------------------------------------

   // Don't bother with KB - only steer towards visible units for now.
   BEntityIDArray targetUnits(0, 64);
   BUnitQuery query(heroSquadPos, maxBeamDist, true);
   query.setRelation(mPlayerID, cRelationTypeEnemy);
   query.setUnitVisibility(mPlayerID);
   gWorld->getUnitsInArea(&query, &targetUnits);

   float totalHP = 0.0f;
   //float totalSP = 0.0f;
   //float totalCV = 0.0f;
   uint numTargets = targetUnits.getSize();
   for (uint i=0; i<numTargets; i++)
   {
      const BUnit* pUnit = gWorld->getUnit(targetUnits[i]);
      if (pUnit)
      {
         totalHP += pUnit->getHitpoints();
         //totalSP += pUnit->getShieldpoints();
      }
   }

   // Minimum stuff to hurt... marchack
   if (totalHP <= 12000.0f)
      return (false);


   // Where to launch?
   float useRadius = 8.0f;
   BEntityGrouper* pEntityGrouper = gWorld->getEntityGrouper();
   pEntityGrouper->reset();
   pEntityGrouper->setRadius(useRadius); // marchack (radius of cleansing)
   pEntityGrouper->groupEntities(targetUnits);
   const BEntityGroup* pGroup = pEntityGrouper->getGroupByIndex(0);
   BASSERT(pGroup);  // Since we had entities... we should have at least one group!
   if (!pGroup)
      return (false);

   const BEntityIDArray& biggestGroup = pGroup->getEntities();
   BVector groupCentroid = BSimHelper::computeCentroid(biggestGroup);
   BEntityID firstVictimUnit = BSimHelper::computeClosestToPoint(groupCentroid, biggestGroup);
   const BUnit* pVictimUnit = gWorld->getUnit(firstVictimUnit);
   if (!pVictimUnit)
      return (false);

   // [6/30/2008 xemu] make sure there are not too many allies in this area 
   // [6/30/2008 xemu] for allies, we consider their max HP, since we care about damaged units the same as healthy ones
   /*BEntityIDArray allyUnits(0, 64);
   BUnitQuery query2(pVictimUnit->getPosition(), useRadius * 2, true);
   query2.setRelation(getPlayerID(), cRelationTypeAlly);
   query2.setUnitVisibility(getPlayerID());
   gWorld->getUnitsInArea(&query2, &allyUnits);

   float totalAllyHP = 0.0f;
   float allyHPthreshold = totalHP / 4.0f;
   uint numAllies = allyUnits.getSize();
   for (uint i = 0; i < numAllies; i++)
   {
      const BUnit* pUnit = gWorld->getUnit(allyUnits[i]);
      if (pUnit)
      {
         totalAllyHP += pUnit->getHPMax();
         if (totalAllyHP > allyHPthreshold)
            return (false);
     }
   }*/

   // Try to launch the power given the parameters here.
   BVector startLocation = pVictimUnit->getPosition();
   BPowerID powerID = launchPower(protoPowerID, powerLevel, pHeroSquad->getID(), startLocation, true);
   BPower* pPower = gWorld->getPowerManager()->getPowerByID(powerID);
   if (pPower)
   {
      // Success - "Pay" for it.
      rBid.purchase(startLocation);
      gWorld->deleteBid(rBid.getID());
      return (true);
   }
   else
   {
      // Failure
      return (false);
   }
}


//==============================================================================
//==============================================================================
bool BAIMission::processPowerLaunchRage(BBid& rBid, BProtoPowerID protoPowerID)
{
   // Step 1 - Basic verification that we can do this power.
   BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (false);
   const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
   if (!pProtoPower)
      return (false);
   const BAIMissionTargetWrapper* pCurrentTargetWrapper = getCurrentTargetWrapper();
   if (!pCurrentTargetWrapper)
      return (false);
   const BAIMissionTarget* pCurrentMissionTarget = gWorld->getAIMissionTarget(pCurrentTargetWrapper->getTargetID());
   if (!pCurrentMissionTarget)
      return (false);
   BPowerLevel powerLevel = pPlayer->getPowerLevel(protoPowerID);
   if (!pProtoPower->isValidPowerLevel(powerLevel))
      return (false);
   if (!pPlayer->canUsePower(protoPowerID))
      return (false);
   const BUnit* pLeaderUnit = gWorld->getUnit(pPlayer->getLeaderUnit());
   if (!pLeaderUnit)
      return (false);
   const BSquad* pHeroSquad = pLeaderUnit->getParentSquad();
   if (!pHeroSquad)
      return (false);

   // Step 3 - How good of an idea is this?
   //---------------------------------------
   // Don't bother with KB - only steer towards visible units for now.
   BVector targetPos = pCurrentMissionTarget->getPosition();
   BVector heroSquadPos = pHeroSquad->getPosition();
   float leashDistSqr = pCurrentTargetWrapper->getLeashDistSqr();
   if (targetPos.xzDistanceSqr(heroSquadPos) > leashDistSqr)
      return (false);

   float totalHP = 0.0f;
   uint numEnemies = mCache.mWorkDistVisibleEnemies.getSize();
   for (uint i=0; i<numEnemies; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(mCache.mWorkDistVisibleEnemies[i]);
      if (pKBSquad)
         totalHP += pKBSquad->getHitpoints();
   }

   // Minimum stuff to hurt... marchack
   if (totalHP <= 12000.0f)
      return (false);

   // Where to launch?
   BVector startLocation = heroSquadPos;
   BPowerID powerID = launchPower(protoPowerID, powerLevel, pHeroSquad->getID(), startLocation, true);
   BPower* pPower = gWorld->getPowerManager()->getPowerByID(powerID);
   if (pPower)
   {
      // Success - "Pay" for it.
      rBid.purchase(startLocation);
      gWorld->deleteBid(rBid.getID());
      return (true);
   }
   else
   {
      // Failure
      return (false);
   }
}


//==============================================================================
//==============================================================================
bool BAIMission::processPowerLaunchWave(BBid& rBid, BProtoPowerID protoPowerID)
{
   // Step 1 - Basic verification that we can do this power.
   BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (false);

   const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
   if (!pProtoPower)
      return (false);
   BPowerLevel powerLevel = pPlayer->getPowerLevel(protoPowerID);
   if (!pProtoPower->isValidPowerLevel(powerLevel))
      return (false);
   if (!pPlayer->canUsePower(protoPowerID))
      return (false);
   const BUnit* pLeaderUnit = gWorld->getUnit(pPlayer->getLeaderUnit());
   if (!pLeaderUnit)
      return (false);
   const BSquad* pHeroSquad = pLeaderUnit->getParentSquad();
   if (!pHeroSquad)
      return (false);
   float minBallDist = 0.0f;
   if (!pProtoPower->getDataFloat(powerLevel, "MinBallDistance", minBallDist))
      return (false);
   float maxBallDist = 0.0f;
   if (!pProtoPower->getDataFloat(powerLevel, "MaxBallDistance", maxBallDist))
      return (false);

   // Step 3 - How good of an idea is this?
   //---------------------------------------

   // Don't bother with KB - only steer towards visible units for now.
   BVector heroSquadPos = pHeroSquad->getPosition();
   BEntityIDArray targetUnits(0, 64);
   BUnitQuery query(heroSquadPos, maxBallDist, true);
   query.setRelation(mPlayerID, cRelationTypeEnemy);
   query.setUnitVisibility(mPlayerID);
   gWorld->getUnitsInArea(&query, &targetUnits);


   float totalHP = 0.0f;
   //float totalSP = 0.0f;
   //float totalCV = 0.0f;
   uint numTargets = targetUnits.getSize();
   for (uint i=0; i<numTargets; i++)
   {
      const BUnit* pUnit = gWorld->getUnit(targetUnits[i]);
      if (pUnit)
      {
         totalHP += pUnit->getHitpoints();
         //totalSP += pUnit->getShieldpoints();
      }
   }

   // Minimum stuff to hurt... marchack
   if (totalHP <= 10000.0f)
      return (false);


   // Where to launch?
   float useRadius = 8.0f;
   BEntityGrouper* pEntityGrouper = gWorld->getEntityGrouper();
   pEntityGrouper->reset();
   pEntityGrouper->setRadius(useRadius); // marchack (radius of cleansing)
   pEntityGrouper->groupEntities(targetUnits);
   const BEntityGroup* pGroup = pEntityGrouper->getGroupByIndex(0);
   BASSERT(pGroup);  // Since we had entities... we should have at least one group!
   if (!pGroup)
      return (false);

   const BEntityIDArray& biggestGroup = pGroup->getEntities();
   BVector groupCentroid = BSimHelper::computeCentroid(biggestGroup);
   BEntityID firstVictimUnit = BSimHelper::computeClosestToPoint(groupCentroid, biggestGroup);
   const BUnit* pVictimUnit = gWorld->getUnit(firstVictimUnit);
   if (!pVictimUnit)
      return (false);

   // [6/30/2008 xemu] make sure there are not too many allies in this area 
   // [6/30/2008 xemu] for allies, we consider their max HP, since we care about damaged units the same as healthy ones
   /*BEntityIDArray allyUnits(0, 64);
   BUnitQuery query2(pVictimUnit->getPosition(), useRadius * 2, true);
   query2.setRelation(getPlayerID(), cRelationTypeAlly);
   query2.setUnitVisibility(getPlayerID());
   gWorld->getUnitsInArea(&query2, &allyUnits);

   float totalAllyHP = 0.0f;
   float allyHPthreshold = totalHP / 4.0f;
   uint numAllies = allyUnits.getSize();
   for (uint i = 0; i < numAllies; i++)
   {
      const BUnit* pUnit = gWorld->getUnit(allyUnits[i]);
      if (pUnit)
      {
         totalAllyHP += pUnit->getHPMax();
         if (totalAllyHP > allyHPthreshold)
            return (false);
      }
   }*/

   // Try to launch the power given the parameters here.
   BVector startLocation = pVictimUnit->getPosition();
   BPowerID powerID = launchPower(protoPowerID, powerLevel, pHeroSquad->getID(), startLocation, true);
   BPower* pPower = gWorld->getPowerManager()->getPowerByID(powerID);
   if (pPower)
   {
      // Success - "Pay" for it.
      rBid.purchase(startLocation);
      gWorld->deleteBid(rBid.getID());
      return (true);
   }
   else
   {
      // Failure
      return (false);
   }
}


//==============================================================================
//==============================================================================
bool BAIMission::processPowerLaunchDisruption(BBid& rBid, BProtoPowerID protoPowerID)
{
   // Step 1 - Basic verification that we can do this power.
   BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (false);
   const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
   if (!pProtoPower)
      return (false);
   BPowerManager* pPowerManager = gWorld->getPowerManager();
   if (!pPowerManager)
      return (false);
   BPowerLevel powerLevel = pPlayer->getPowerLevel(protoPowerID);
   if (!pProtoPower->isValidPowerLevel(powerLevel))
      return (false);
   if (!pPlayer->canUsePower(protoPowerID))
      return (false);
   float disruptionRadius = 0.0f;
   if (!pProtoPower->getDataFloat(powerLevel, "DisruptionRadius", disruptionRadius))
      return (false);
   if (disruptionRadius <= 0.0f)
      return (false);
   //float disruptionRadiusSqr = disruptionRadius * disruptionRadius;

   // Step #2 - Find a case where it is good to disrupt the other guy?
   // 1.)  Their leader unit is casting a power on you.
   uint numPowers = pPowerManager->getNumPowers();
   for (uint i=0; i<numPowers; i++)
   {
      // Get the existing power.
      const BPower* pExistingPower = pPowerManager->getPowerByIndex(i);
      if (!pExistingPower)
         continue;
      // If this power is not owned by an enemy, why would we disrupt it?
      if (!pPlayer->isEnemy(pExistingPower->getPlayerID()))
         continue;

      BPowerType existingPowerType = pExistingPower->getType();

      // Try to disrupt rage.
      if (existingPowerType == PowerType::cRage)
      {
         const BSquad* pOwnerSquad = pExistingPower->getOwnerSquad();
         if (!pOwnerSquad)
            continue;
         BVector covLeaderPos = pOwnerSquad->getPosition();
         float scanRadius = reinterpret_cast<const BPowerRage*>(pExistingPower)->getScanRadius();
         BEntityIDArray results;
         BUnitQuery unitQuery(covLeaderPos, scanRadius, true);
         unitQuery.setUnitVisibility(mPlayerID);
         unitQuery.setRelation(mPlayerID, cRelationTypeAlly);
         gWorld->getSquadsInArea(&unitQuery, &results, false);
         float totalHP = BSimHelper::computeTotalHP(results);
         if (totalHP > 10000.0f)
         {
            BPowerID powerID = launchPower(protoPowerID, powerLevel, cInvalidObjectID, covLeaderPos, false);
            BPower* pPower = gWorld->getPowerManager()->getPowerByID(powerID);
            if (pPower)
            {
               // Success - "Pay" for it.
               BVector crap;
               rBid.purchase(crap);
               gWorld->deleteBid(rBid.getID());
               return (true);
            }
            else
            {
               // Failure
               return (false);
            }
         }
      }

      // Try to disrupt cleansing.
      else if (existingPowerType == PowerType::cCleansing)
      {
         const BSquad* pOwnerSquad = pExistingPower->getOwnerSquad();
         if (!pOwnerSquad)
            continue;
         BVector covLeaderPos = pOwnerSquad->getPosition();
         float maxBeamDistance = reinterpret_cast<const BPowerCleansing*>(pExistingPower)->getMaxBeamDistance();
         BEntityIDArray results;
         BUnitQuery unitQuery(covLeaderPos, maxBeamDistance, true);
         unitQuery.setUnitVisibility(mPlayerID);
         unitQuery.setRelation(mPlayerID, cRelationTypeAlly);
         gWorld->getSquadsInArea(&unitQuery, &results, false);
         float totalHP = BSimHelper::computeTotalHP(results);
         if (totalHP > 10000.0f)
         {
            BPowerID powerID = launchPower(protoPowerID, powerLevel, cInvalidObjectID, covLeaderPos, false);
            BPower* pPower = gWorld->getPowerManager()->getPowerByID(powerID);
            if (pPower)
            {
               // Success - "Pay" for it.
               BVector crap;
               rBid.purchase(crap);
               gWorld->deleteBid(rBid.getID());
               return (true);
            }
            else
            {
               // Failure
               return (false);
            }
         }
      }

      // Try to disrupt wave.
      else if (existingPowerType == PowerType::cWave)
      {
         const BSquad* pOwnerSquad = pExistingPower->getOwnerSquad();
         if (!pOwnerSquad)
            continue;
         float maxBallDistance = reinterpret_cast<const BPowerWave*>(pExistingPower)->getMaxBallDistance();
         BVector covLeaderPos = pOwnerSquad->getPosition();
         BEntityIDArray results;
         BUnitQuery unitQuery(covLeaderPos, maxBallDistance, true);
         unitQuery.setUnitVisibility(mPlayerID);
         unitQuery.setRelation(mPlayerID, cRelationTypeAlly);
         gWorld->getSquadsInArea(&unitQuery, &results, false);
         float totalHP = BSimHelper::computeTotalHP(results);
         if (totalHP > 10000.0f)
         {
            BPowerID powerID = launchPower(protoPowerID, powerLevel, cInvalidObjectID, covLeaderPos, false);
            BPower* pPower = gWorld->getPowerManager()->getPowerByID(powerID);
            if (pPower)
            {
               // Success - "Pay" for it.
               BVector crap;
               rBid.purchase(crap);
               gWorld->deleteBid(rBid.getID());
               return (true);
            }
            else
            {
               // Failure
               return (false);
            }
         }
      }
   }

   // Donothing.
   return (false);
}


//==============================================================================
//==============================================================================
bool BAIMission::processPowerLaunchCarpetBombing(BBid& rBid, BProtoPowerID protoPowerID)
{
   // Step 1 - Basic verification that we can do this power.
   BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (false);
   BTeamID playerTeamID = pPlayer->getTeamID();
   // Make sure we have a target.
   const BAIMissionTargetWrapper* pCurrentTargetWrapper = getCurrentTargetWrapper();
   if (!pCurrentTargetWrapper) // marchack - no target refs?? - just bail?
      return (false);
   const BAIMissionTarget* pCurrentMissionTarget = gWorld->getAIMissionTarget(pCurrentTargetWrapper->getTargetID());
   if (!pCurrentMissionTarget) // marchack - no target?? - just bail?
      return (false);
   const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
   if (!pProtoPower)
      return (false);
   BPowerLevel powerLevel = pPlayer->getPowerLevel(protoPowerID);
   if (!pProtoPower->isValidPowerLevel(powerLevel))
      return (false);
   if (!pPlayer->canUsePower(protoPowerID))
      return (false);

   // Step 2 - Get some data.
   float maxBombOffset = 0.0f;
   if (!pProtoPower->getDataFloat(powerLevel, "MaxBombOffset", maxBombOffset))
      return (false);
   float lengthMultiplier = 0.0f;
   if (!pProtoPower->getDataFloat(powerLevel, "LengthMultiplier", lengthMultiplier))
      return (false);

   // Step 3 - How good of an idea is this?
   //---------------------------------------

   // Don't bother with KB - only steer towards visible units for now.
   BEntityIDArray targetSquads(0, 64);
   BUnitQuery query(pCurrentMissionTarget->getPosition(), pCurrentTargetWrapper->getSearchWorkDist(), true);
   query.setRelation(mPlayerID, cRelationTypeEnemy);
   query.setUnitVisibility(mPlayerID);
   gWorld->getSquadsInArea(&query, &targetSquads, false);

  
   float totalHP = 0.0f;
   uint numTargets = targetSquads.getSize();
   for (uint i=0; i<numTargets; i++)
   {
      const BSquad* pSquad = gWorld->getSquad(targetSquads[i]);
      if (pSquad)
      {
         totalHP += pSquad->getCurrentHP();
         //totalSP += pSquad->getShieldpoints();
      }
   }

   // Minimum stuff to hurt... marchack
   if (totalHP <= 12000.0f)
      return (false);

   float halfLengthMultiplier = 0.5f * lengthMultiplier;

   BEntityGrouper* pEntityGrouper = gWorld->getEntityGrouper();
   pEntityGrouper->reset();
   pEntityGrouper->setRadius(halfLengthMultiplier);
   pEntityGrouper->groupEntities(targetSquads);


   //const BEntityGroup* pBestGroup = NULL;
   BVector bestCentroid;   

   // Iterate through all our groups and try to find one to cast upon
   uint numGroups = pEntityGrouper->getNumberGroups();
   for (uint g=0; g<numGroups; g++)
   {
      const BEntityGroup* pGroup = pEntityGrouper->getGroupByIndex(g);
      BASSERT(pGroup);
      if (!pGroup)
         continue;

      BEntityIDArray targetableUnits;
      BVector testPos = pGroup->getCenter();

      float totalGroupSquadsHP = 0.0f;
      const BEntityIDArray& groupSquads = pGroup->getEntities();
      uint numGroupSquads = groupSquads.getSize();
      if (numGroupSquads <= 0)
         continue;

      for (uint s=0; s<numGroupSquads; s++)
      {
         const BSquad* pGroupSquad = gWorld->getSquad(groupSquads[s]);
         if (pGroupSquad)
         {
            totalGroupSquadsHP += pGroupSquad->getCurrentHP();

            const BEntityIDArray& childList = pGroupSquad->getChildList();
            for (uint childIndex=0; childIndex<childList.getSize(); childIndex++)
            {
               const BUnit* pChildUnit = gWorld->getUnit(childList[childIndex]);
               if (pChildUnit && pChildUnit->isPositionVisible(playerTeamID))
                  targetableUnits.add(pChildUnit->getID());
            }
         }
      }

      if (totalGroupSquadsHP < 12000.0f)
         continue;
      if (targetableUnits.getSize() == 0)
         continue;

      BEntityID closestUnitToPoint = BSimHelper::computeClosestToPoint(testPos, targetableUnits);
      const BUnit* pClosestUnitToPoint = gWorld->getUnit(closestUnitToPoint);
      if (pClosestUnitToPoint)
      {
         testPos = pClosestUnitToPoint->getPosition();
         // At this point we are going to 
         // Try to launch the power given the parameters here.
         BPowerID powerID = launchPower(protoPowerID, powerLevel, cInvalidObjectID, testPos, false);
         BPower* pPower = gWorld->getPowerManager()->getPowerByID(powerID);
         if (pPower)
         {
            // Success - "Pay" for it.
            BVector junkVector;
            rBid.purchase(junkVector);
            gWorld->deleteBid(rBid.getID());

            pPlayer->getAlertManager()->createFlareAlert(testPos, mPlayerID);

            BPowerInput powerInput;

            // Submit the position.
            powerInput.mType = PowerInputType::cPosition;
            powerInput.mVector = testPos;
            pPower->submitInput(powerInput);

            // Submit the orientation.
            BVector dir = pClosestUnitToPoint->getForward();
            dir.y = 0.0f;
            dir.normalize();
            powerInput.mType = PowerInputType::cDirection;
            powerInput.mVector = dir;
            pPower->submitInput(powerInput);
            return (true);
         }
         else
         {
            // Failure.
            return (false);
         }
      }
   }

   return (false);
}


//==============================================================================
//==============================================================================
bool BAIMission::processPowerLaunchOrbital(BBid& rBid, BProtoPowerID protoPowerID)
{
   // Step 1 - Basic verification that we can do this power.
   BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (false);
   // Make sure we have a target.
   const BAIMissionTargetWrapper* pCurrentTargetWrapper = getCurrentTargetWrapper();
   if (!pCurrentTargetWrapper) // marchack - no target refs?? - just bail?
      return (false);
   const BAIMissionTarget* pCurrentMissionTarget = gWorld->getAIMissionTarget(pCurrentTargetWrapper->getTargetID());
   if (!pCurrentMissionTarget) // marchack - no target?? - just bail?
      return (false);
   const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
   if (!pProtoPower)
      return (false);
   if (pProtoPower->getPowerType() != PowerType::cOrbital)
      return (false);
   BPowerLevel powerLevel = pPlayer->getPowerLevel(protoPowerID);
   if (!pProtoPower->isValidPowerLevel(powerLevel))
      return (false);
   if (!pPlayer->canUsePower(protoPowerID))
      return (false);
   int numShots = 0;
   if (!pProtoPower->getDataInt(powerLevel, "NumShots", numShots))
      return (false);
   if (numShots <= 0)
      return (false);

   const float cOrbitalRadius = 50.0f;
   //const float cOrbitalRadiusSqr = 50.0f * 50.0f;

   // Step 3 - How good of an idea is this?
   //---------------------------------------

   // Don't bother with KB - only steer towards visible units for now.
   BEntityIDArray targetUnits(0, 64);
   BUnitQuery query(pCurrentMissionTarget->getPosition(), pCurrentTargetWrapper->getSearchWorkDist(), true);
   query.setRelation(mPlayerID, cRelationTypeEnemy);
   query.setUnitVisibility(mPlayerID);
   gWorld->getUnitsInArea(&query, &targetUnits);

   float totalHP = 0.0f;
   uint numTargets = targetUnits.getSize();
   for (uint i=0; i<numTargets; i++)
   {
      const BUnit* pUnit = gWorld->getUnit(targetUnits[i]);
      if (pUnit)
         totalHP += pUnit->getHitpoints();
   }

   // Minimum stuff to hurt... marchack
   const float cMinHPToOrbital = 12000.0f;
   if (totalHP <= cMinHPToOrbital)
      return (false);

   BVector startLocation;
   if (!recalcBestOrbitalLoc(cOrbitalRadius, targetUnits, startLocation))
      return (false);

      BPowerID powerID = launchPower(protoPowerID, powerLevel, cInvalidObjectID, startLocation, true);
      BPower* pPower = gWorld->getPowerManager()->getPowerByID(powerID);
      if (pPower)
      {
         // Success - "Pay" for it.
         BVector junkVector;
         rBid.purchase(junkVector);
         gWorld->deleteBid(rBid.getID());
         pPlayer->getAlertManager()->createFlareAlert(startLocation, mPlayerID);
         return (true);
      }
      else
      {
         // Failure.
         return (false);
      }
 //  }

   return (false);
}




//==============================================================================
//==============================================================================
bool BAIMission::processPowerLaunchCryo(BBid& rBid, BProtoPowerID protoPowerID)
{
   //-------------------------------------------------------------------
   // Step 1 - Basic verification that we can do this power.
   //-------------------------------------------------------------------
   BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (false);
   BTeamID playerTeamID = pPlayer->getTeamID();
   const BAIMissionTargetWrapper* pCurrentTargetWrapper = getCurrentTargetWrapper();
   if (!pCurrentTargetWrapper) // marchack - no target refs?? - just bail?
      return (false);
   const BAIMissionTarget* pCurrentMissionTarget = gWorld->getAIMissionTarget(pCurrentTargetWrapper->getTargetID());
   if (!pCurrentMissionTarget) // marchack - no target?? - just bail?
      return (false);
   const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
   if (!pProtoPower)
      return (false);
   BPowerLevel powerLevel = pPlayer->getPowerLevel(protoPowerID);
   if (!pProtoPower->isValidPowerLevel(powerLevel))
      return (false);
   if (!pPlayer->canUsePower(protoPowerID))
      return (false);
   float cryoRadius = 0.0f;
   if (!pProtoPower->getDataFloat(powerLevel, "CryoRadius", cryoRadius))
      return (false);
   float maxKillHP = 0.0f;
   if (!pProtoPower->getDataFloat(powerLevel, "MaxKillHP", maxKillHP))
      return (false);
   float cryoRadiusSqr = cryoRadius * cryoRadius;

   //------------------------------------------------------------
   // Step 3 - How good of an idea is this?
   //------------------------------------------------------------
   BEntityIDArray squadIDs;
   BUnitQuery query(pCurrentMissionTarget->getPosition(), pCurrentTargetWrapper->getSearchWorkDist(), true);
   query.setUnitVisibility(mPlayerID);
   query.addObjectTypeFilter(gDatabase.getOTIDCanCryo());
   gWorld->getSquadsInArea(&query, &squadIDs, false);
   uint numSquads = squadIDs.getSize();
   if (numSquads == 0)
      return (false);


   // Tally up some basic information about the enemies and allies in this area.
   BEntityIDArray enemySquads;
   BEntityIDArray alliedSquads;
   float totalEnemyHP = 0.0f;
   float totalAlliedHP = 0.0f;
   for (uint i=0; i<numSquads; i++)
   {
      const BSquad* pSquad = gWorld->getSquad(squadIDs[i]);
      if (!pSquad)
         continue;
      if (pPlayer->isAlly(pSquad->getPlayerID()))
      {
         alliedSquads.add(squadIDs[i]);
         totalAlliedHP += pSquad->getCurrentHP();
      }
      else if (pPlayer->isEnemy(pSquad->getPlayerID()))
      {
         enemySquads.add(squadIDs[i]);
         totalEnemyHP += pSquad->getCurrentHP();
      }
   }

   uint numEnemySquads = enemySquads.getSize();
   if (numEnemySquads == 0)
      return (false);

   // Find all the enemy clusters we have.
   BEntityGrouper* pEntityGrouper = gWorld->getEntityGrouper();
   pEntityGrouper->reset();
   pEntityGrouper->setRadius(cryoRadius);
   pEntityGrouper->setFlagCommonParentShortcut(false);
   pEntityGrouper->groupEntities(enemySquads);
   uint numGroups = pEntityGrouper->getNumberGroups();

   // Consts for scoring the squads.
   const float cCSEnemy = 2.0f;
   const float cCSFreezeKillEnemy = 5.0f;
   const float cCSAlly = -3.0f;
   const float cCSFreezeKillAlly = -8.0f;
   const float cCSMinimumBar = 12.0f;

   // Get the test positions of the enemy clusters.
   for (uint i=0; i<numGroups; i++)
   {
      const BEntityGroup* pGroup = pEntityGrouper->getGroupByIndex(i);
      if (!pGroup)
         continue;

      BVector testPos = pGroup->getCenter();
      BEntityIDArray targetableUnits;

      // Score accumulators
      float cryoScoreTM = 0.0f;
      float friendlyFreezeKillHP = 0.0f;
      float enemyFreezeKillHP = 0.0f;

      // Add for enemies.
      const BEntityIDArray& localizedEnemies = pGroup->getEntities();
      uint numLocalizedEnemies = localizedEnemies.getSize();
      for (uint enemyIndex=0; enemyIndex<numLocalizedEnemies; enemyIndex++)
      {
         const BSquad* pEnemySquad = gWorld->getSquad(localizedEnemies[enemyIndex]);
         if (!pEnemySquad)
            continue;

         const BEntityIDArray& childList = pEnemySquad->getChildList();
         for (uint childIndex=0; childIndex<childList.getSize(); childIndex++)
         {
            const BUnit* pChildUnit = gWorld->getUnit(childList[childIndex]);
            if (pChildUnit && pChildUnit->isPositionVisible(playerTeamID))
               targetableUnits.add(pChildUnit->getID());
         }

         // Add some scoring for enemies this will freeze.
         if (pEnemySquad->getProtoSquad()->getFlagDiesWhenFrozen())
         {
            if (enemyFreezeKillHP <= maxKillHP)
            {
               cryoScoreTM += cCSFreezeKillEnemy;
               enemyFreezeKillHP += pEnemySquad->getCurrentHP();
            }
            else
            {
               cryoScoreTM += cCSEnemy;
            }
         }
         else
         {
            cryoScoreTM += cCSEnemy;
         }
      }

      // Penalize for allies.
      uint numAlliedSquads = alliedSquads.getSize();
      for (uint allyIndex=0; allyIndex<numAlliedSquads; allyIndex++)
      {
         const BSquad* pAllySquad = gWorld->getSquad(alliedSquads[allyIndex]);
         if (!pAllySquad)
            continue;
         if (pAllySquad->calculateXZDistanceSqr(testPos) >= cryoRadiusSqr)
            continue;
         // Add some scoring for enemies this will freeze.
         if (pAllySquad->getProtoSquad()->getFlagDiesWhenFrozen())
         {
            if (friendlyFreezeKillHP <= maxKillHP)
            {
               cryoScoreTM += cCSFreezeKillAlly;
               friendlyFreezeKillHP += pAllySquad->getCurrentHP();
            }
            else
            {
               cryoScoreTM += cCSAlly;
            }
         }
         else
         {
            cryoScoreTM += cCSAlly;
         }
      }

      // CryoScoreTM of >= 12.0 wins.
      if (cryoScoreTM >= cCSMinimumBar && targetableUnits.getSize())
      {
         BEntityID closestUnitToPoint = BSimHelper::computeClosestToPoint(testPos, targetableUnits);
         const BUnit* pClosestUnitToPoint = gWorld->getUnit(closestUnitToPoint);
         if (pClosestUnitToPoint)
         {
            testPos = pClosestUnitToPoint->getPosition();
            BPowerID powerID = launchPower(protoPowerID, powerLevel, cInvalidObjectID, testPos, false);
            BPower* pPower = gWorld->getPowerManager()->getPowerByID(powerID);
            if (pPower)
            {
               BVector junkVector;
               rBid.purchase(junkVector);
               gWorld->deleteBid(rBid.getID());
               return (true);
            }
         }
      }
   }

   return (false);
}


//==============================================================================
//==============================================================================
bool BAIMission::processPowerLaunchRepair(BBid& rBid, BProtoPowerID protoPowerID)
{
   //-------------------------------------------------------------------
   // Step 1 - Basic verification that we can do this power.
   //-------------------------------------------------------------------
   BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (false);
   const BAIMissionTargetWrapper* pCurrentTargetWrapper = getCurrentTargetWrapper();
   if (!pCurrentTargetWrapper) // marchack - no target refs?? - just bail?
      return (false);
   const BAIMissionTarget* pCurrentMissionTarget = gWorld->getAIMissionTarget(pCurrentTargetWrapper->getTargetID());
   if (!pCurrentMissionTarget) // marchack - no target?? - just bail?
      return (false);
   const BProtoPower* pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
   if (!pProtoPower)
      return (false);
   BPowerLevel powerLevel = pPlayer->getPowerLevel(protoPowerID);
   if (!pProtoPower->isValidPowerLevel(powerLevel))
      return (false);
   if (!pPlayer->canUsePower(protoPowerID))
      return (false);
   const float repairRadius = 35.0f;
   //const float repairRadiusSqr = 35.0f * 35.0f;
   //const float maxCombatRepairValue = 20.0f;
   const float minCombatRepairValue = 8.0f;

   //------------------------------------------------------------
   // Step 3 - How good of an idea is this?
   //------------------------------------------------------------
   BEntityIDArray squadIDs;
   BUnitQuery query(pCurrentMissionTarget->getPosition(), pCurrentTargetWrapper->getLeashDist(), true);
   query.setUnitVisibility(mPlayerID);
   query.setRelation(mPlayerID, cRelationTypeAlly);
   gWorld->getSquadsInArea(&query, &squadIDs, false);
   uint numSquads = squadIDs.getSize();
   if (numSquads == 0)
      return (false);
   float cvRepairCost = BSimHelper::getCombatValueRepairCost(squadIDs);
   if (cvRepairCost <= minCombatRepairValue)
      return (false);

   // Find all the enemy clusters we have.
   BEntityGrouper* pEntityGrouper = gWorld->getEntityGrouper();
   pEntityGrouper->reset();
   pEntityGrouper->setRadius(repairRadius);
   pEntityGrouper->setFlagCommonParentShortcut(false);
   pEntityGrouper->groupEntities(squadIDs);

   BVector bestGroupLocation;
   float bestGroupRepairCost = 0.0f;
   bool foundBestLocation = false;

   // Cast repair near the cluster of squads who will most benefit from it.
   // We can move the other squads into the area later.
   uint numGroups = pEntityGrouper->getNumberGroups();
   for (uint i=0; i<numGroups; i++)
   {
      const BEntityGroup* pGroup = pEntityGrouper->getGroupByIndex(i);
      if (!pGroup)
         continue;

      const BEntityIDArray& groupSquads = pGroup->getEntities();
      float groupRepairCost = BSimHelper::getCombatValueRepairCost(groupSquads);
      if (groupRepairCost <= bestGroupRepairCost)
         continue;

      bestGroupRepairCost = groupRepairCost;
      bestGroupLocation = pGroup->getCenter();
      foundBestLocation = true;
   }


   if (foundBestLocation)
   {
      BPowerID powerID = launchPower(protoPowerID, powerLevel, cInvalidObjectID, bestGroupLocation, false);
      BPower* pPower = gWorld->getPowerManager()->getPowerByID(powerID);
      if (pPower)
      {
         BVector junkVector;
         rBid.purchase(junkVector);
         gWorld->deleteBid(rBid.getID());
         return (true);
      }
   }

   // Did not cast power.
   return (false);
}



//==============================================================================
//==============================================================================
BPowerID BAIMission::launchPower(BProtoPowerID protoPowerID, BPowerLevel powerLevel, BEntityID squadID, BVector launchPos, bool bCreateMission)
{
   // Bomb check.
   BAI* pAI = gWorld->getAI(mPlayerID);
   if (!pAI)
      return (cInvalidPowerID);
   BPowerManager* pPowerManager = gWorld->getPowerManager();
   if (!pPowerManager)
      return (cInvalidPowerID);
   const BAIMissionTargetWrapper* pCurrentTargetWrapper = getCurrentTargetWrapper();
   if (!pCurrentTargetWrapper)
      return (cInvalidPowerID);

   // Create the power
   BPower* pNewPower = pPowerManager->createNewPower(protoPowerID);
   if (!pNewPower)
      return (cInvalidPowerID);
   // Initialize it - if init() fails, it automatically flags the power to be cleaned up, so we don't have to delete anything here.
   if (!pNewPower->init(mPlayerID, powerLevel, cInvalidPowerUserID, squadID, launchPos))
      return (cInvalidPowerID);

   // Some powers don't require a mission (like single-input powers.) so we can specify which is which.
   if (bCreateMission)
   {
      // Create the mission.
      BAIPowerMission* pPowerMission = static_cast<BAIPowerMission*>(gWorld->createAIMission(mPlayerID, MissionType::cPower));
      BASSERT(pPowerMission);
      if (!pPowerMission)
      {
         pNewPower->shutdown();
         return (cInvalidPowerID);
      }

      DWORD currentGameTime = gWorld->getGametime();

      // If we're in AIQ mode, create a topic as well as a mission - Leave it up to the AIQ to turn on the mission.
      if (pAI->getFlagAIQ())
      {
         BAITopic* pAITopic = gWorld->createAITopic(mPlayerID);
         BASSERT(pAITopic);
         if (!pAITopic)
         {
            pNewPower->shutdown();
            return (cInvalidPowerID);
         }

         pAITopic->setType(AITopicType::cMission);
         pAITopic->setTriggerScriptID(cInvalidTriggerScriptID);
         pAITopic->setTriggerVarID(cInvalidTriggerVarID);
         #ifndef BUILD_FINAL
         {
            BSimString topicName;
            topicName.format("Mission %u", pPowerMission->mDebugID);
            pAITopic->setName(topicName);
         }
         #endif
         pAITopic->setMinTickets(100);
         pAITopic->setMaxTickets(100);
         pAITopic->setCurrentTickets(100);
         pAITopic->setTicketInterval(100);
         pPowerMission->setPowerID(pNewPower->getID());
         pPowerMission->setOwnerID(squadID);
         pPowerMission->toggleActive(currentGameTime, false);
         pPowerMission->setAITopicID(pAITopic->getID());
         pPowerMission->setState(MissionState::cCreate);

         BAIMissionTargetWrapper* pWrapper = gWorld->createWrapper(pPowerMission->getID(), pCurrentTargetWrapper->getTargetID());
         if (pWrapper)
         {
            pWrapper->setLeashDist(pCurrentTargetWrapper->getLeashDist());
            pWrapper->setSecureDist(pCurrentTargetWrapper->getSecureDist());
            pWrapper->setRallyDist(pCurrentTargetWrapper->getRallyDist());
            pWrapper->setSearchWorkDist(pCurrentTargetWrapper->getSearchWorkDist());
            pWrapper->setHotZoneDist(pCurrentTargetWrapper->getHotZoneDist());
            pWrapper->setMinRetreatRatio(pCurrentTargetWrapper->getMinRetreatRatio());
            pWrapper->setMinRalliedPercent(pCurrentTargetWrapper->getMinRalliedPercent());
            pWrapper->setMinSecureTime(pCurrentTargetWrapper->getMinSecureTime());
            pWrapper->setFlagAllowRetreat(false);
            pWrapper->setFlagAllowSkip(false);
            pWrapper->setFlagRequireSecure(false);
            pWrapper->setFlagGatherCrates(false);
         }

         pAITopic->setAIMissionID(pPowerMission->getID());
         BAITopic* pOldTopic = gWorld->getAITopic(mAITopicID);
         if (pOldTopic)
            pOldTopic->enablePriorityRequest(currentGameTime);
         pAI->AIQSetTopic(pAITopic->getID());
      }
      // Otherwise just do a mission and turn it on.
      else
      {
         // NON AIQ
         pPowerMission->setPowerID(pNewPower->getID());
         pPowerMission->setOwnerID(squadID);
         pPowerMission->toggleActive(currentGameTime, true);
         pPowerMission->setState(MissionState::cCreate);
                  
         BAIMissionTargetWrapper* pWrapper = gWorld->createWrapper(pPowerMission->getID(), pCurrentTargetWrapper->getTargetID());
         if (pWrapper)
         {
            pWrapper->setLeashDist(pCurrentTargetWrapper->getLeashDist());
            pWrapper->setSecureDist(pCurrentTargetWrapper->getSecureDist());
            pWrapper->setRallyDist(pCurrentTargetWrapper->getRallyDist());
            pWrapper->setSearchWorkDist(pCurrentTargetWrapper->getSearchWorkDist());
            pWrapper->setHotZoneDist(pCurrentTargetWrapper->getHotZoneDist());
            pWrapper->setMinRetreatRatio(pCurrentTargetWrapper->getMinRetreatRatio());
            pWrapper->setMinRalliedPercent(pCurrentTargetWrapper->getMinRalliedPercent());
            pWrapper->setMinSecureTime(pCurrentTargetWrapper->getMinSecureTime());
            pWrapper->setFlagAllowRetreat(false);
            pWrapper->setFlagAllowSkip(false);
            pWrapper->setFlagRequireSecure(false);
            pWrapper->setFlagGatherCrates(false);
         }
      }
   }

   return (pNewPower->getID());
}


//==============================================================================
//==============================================================================
/*BEntityID BAIMission::getCovHeroSquad() const
{
   BAbilityID covLeaderGlassingID = gDatabase.getAIDCovLeaderGlassing();
   uint numSquads = mSquads.getSize();
   for (uint i=0; i<numSquads; i++)
   {
      const BSquad* pSquad = gWorld->getSquad(mSquads[i]);
      if (!pSquad)
         continue;
      const BUnit* pUnit = pSquad->getLeaderUnit();
      if (!pUnit)
         continue;
      const BProtoObject* pProtoObject = pUnit->getProtoObject();
      if (!pProtoObject)
         continue;
      if (pProtoObject->getAbilityCommand() != covLeaderGlassingID)
         continue;

      return (mSquads[i]);
   }

   return (cInvalidObjectID);
}*/


//==============================================================================
//==============================================================================
void BAIMission::processPossibleOpportunityTargets(DWORD currentGameTime)
{
   // This mission does not allow opportunity targets.
   if (!mFlagAllowOpportunityTargets)
      return;

   // Early bail if it's too soon
   if (currentGameTime >= mTimestampNextOppDetect)
   {
      BAI* pAI = gWorld->getAI(mPlayerID);
      if (!pAI)
         return;
      BAIGlobals* pAIGlobals = pAI->getAIGlobals();
      if (!pAIGlobals)
         return;
      pAIGlobals->addOpportunityRequest(mID);
      mTimestampNextOppDetect = currentGameTime + pAIGlobals->getTargetRecalculatePeriod();
   }
}


//==============================================================================
//==============================================================================
void BAIMission::addBid(BBidID bidID)
{
   const BBid* pBid = gWorld->getBid(bidID);
   if (!pBid)
      return; 

   BASSERTM(mPlayerID == pBid->getPlayerID(), "Trying to add a BidID to a mission belonging to a different player.");
   if (mPlayerID != pBid->getPlayerID())
      return;

   //BASSERTM(!mBidIDs.contains(bidID), "This mission already contains this bid ID.  Fix the script to only add a bid one time.  Ignoring for now.");
   mBidIDs.uniqueAdd(bidID);
}


//==============================================================================
//==============================================================================
void BAIMission::removeBid(BBidID bidID)
{
   mBidIDs.remove(bidID);
}


//==============================================================================
//==============================================================================
void BAIMission::destroyAllTargetWrappers()
{
   // Cache the ID's because we remove them as we destroy them.
   BAIMissionTargetWrapperIDArray wrappersToDestroy = mTargetWrapperIDs;
   uint numWrappers = wrappersToDestroy.getSize();

   // Delete them.
   for (uint i=0; i<numWrappers; i++)
      gWorld->deleteWrapper(wrappersToDestroy[i]);

   BASSERT(mTargetWrapperIDs.getSize() == 0);
}


//==============================================================================
//==============================================================================
void BAIMission::reorderTargetWrapper(BAIMissionTargetWrapperID id, uint index)
{
   // Make sure we contain this id.
   BASSERT(mTargetWrapperIDs.contains(id));
   if (!mTargetWrapperIDs.contains(id))
      return;

   // Remove it from the present location.
   mTargetWrapperIDs.remove(id);

   // Reinsert it into the new location
   if (index >= mTargetWrapperIDs.getSize())
      mTargetWrapperIDs.add(id);
   else
      mTargetWrapperIDs.insertAtIndex(id, index);
}


//==============================================================================
//==============================================================================
BAIMissionTargetID BAIMission::getFinalTargetWrapperTargetID() const
{
   BAIMissionTargetID targetID = cInvalidAIMissionTargetID;
   uint numTargetWrapperIDs = mTargetWrapperIDs.getSize();
   if (numTargetWrapperIDs > 0)
   {
      const BAIMissionTargetWrapper* pWrapper = gWorld->getWrapper(mTargetWrapperIDs[numTargetWrapperIDs-1]);
      if (pWrapper)
         targetID = pWrapper->getTargetID();
   }
   return (targetID);
}


//==============================================================================
//==============================================================================
void BAIMission::setState(BAIMissionState state)
{
   DWORD currentGameTime = gWorld->getGametime();

   mState = state;
   mTimestampStateChanged = currentGameTime;

   // This initializes the next time we can "think" about something and "act" on it to slightly after we change states.
   payTimeCostAct(currentGameTime);
   payTimeCostThink(currentGameTime);

   // Set some group flags on mission state change...
   uint numGroups = mGroups.getSize();
   for (uint i=0; i<numGroups; i++)
   {
      BAIGroup* pGroup = gWorld->getAIGroup(mGroups[i]);
      if (pGroup)
         pGroup->setFlagTaskedThisState(false);
   }

   if (isStateTerminal())
   {
      BAI* pAI = gWorld->getAI(mPlayerID);
      BASSERT(pAI);
      if (pAI)
      {
         BAIGlobals* pAIGlobals = pAI->getAIGlobals();
         BASSERT(pAIGlobals);
         if (pAIGlobals)
         {
            pAIGlobals->removeOpportunityRequest(mID);   // in case.
            pAIGlobals->addTerminalMission(mID);

            // If this mission is going terminal and it has AI focus, kill the focus entirely.      
            if (pAI->AIQGetTopic() == mAITopicID)
               pAI->AIQSetTopic(cInvalidAITopicID);
         }
      }
   }

   // Hooks
   if (mState == MissionState::cFailure)
   {
      long numPlayers = gWorld->getNumberPlayers();
      for (long i=0; i<numPlayers; i++)
      {
         BPlayer* pPlayer = gWorld->getPlayer(i);
         if (!pPlayer)
            continue;
         if (!pPlayer->isAlly(mPlayerID, false))
            continue;
         BAIFactoidManager* pFM = pPlayer->getFactoidManager();
         if (!pFM)
            continue;
         pFM->submitAIFactoidEvent(mPlayerID, BString("AllyAttackFailed"), 3, 5000, cInvalidVector, cInvalidVector, false);
      }
   }
}


//==============================================================================
//==============================================================================
void BAIMission::addSquads(BEntityIDArray squadsToAdd)
{
   uint numSquadsToAdd = squadsToAdd.getSize();
   for (uint i=0; i<numSquadsToAdd; i++)
      addSquad(squadsToAdd[i]);
}


//==============================================================================
//==============================================================================
BSquad* BAIMission::validateSquadForMission(BEntityID squadID)
{
   // Validate the ID.
   if (!squadID.isValid())
      return (NULL);
   // Validate the squad.
   BSquad* pSquad = gWorld->getSquad(squadID);
   if (!pSquad)
      return (NULL);
   // Validate the squad is owned by the player who owns this mission.
   if (pSquad->getPlayerID() != getPlayerID())
      return (NULL);
   // No buildings... this is stupid.
   const BProtoObject* pProtoObject = pSquad->getProtoObject();
   if (pProtoObject && pProtoObject->isType(gDatabase.getOTIDBuilding()))
      return (NULL);

   // The squad is valid.
   return (pSquad);
}


//==============================================================================
//==============================================================================
void BAIMission::removeInvalidSquads()
{
   for (uint i=0; i<mSquads.getSize(); )
   {
      if (!validateSquadForMission(mSquads[i]))
         removeSquad(mSquads[i]);
      else
         i++;
   }
}


//==============================================================================
//==============================================================================
uint BAIMission::getSquadsFromGroups(const BAIGroupIDArray& groupIDs, BEntityIDArray& squadIDs)
{
   squadIDs.resize(0);
   
   uint numGroupIDs = groupIDs.getSize();
   for (uint i=0; i<numGroupIDs; i++)
   {
//-- FIXING PREFIX BUG ID 4022
      const BAIGroup* pGroup = gWorld->getAIGroup(groupIDs[i]);
//--
      if (pGroup)
      {
         const BEntityIDArray& groupSquadIDs = pGroup->getSquads();
         uint numGroupSquadIDs = groupSquadIDs.getSize();
         for (uint j=0; j<numGroupSquadIDs; j++)
            squadIDs.uniqueAdd(groupSquadIDs[j]);
      }      
   }

   return (squadIDs.getSize());
}


//==============================================================================
//==============================================================================
void BAIMission::moveGroupsToPosition(const BAIGroupIDArray& groupIDs, const BVector location)
{
   BEntityIDArray squadIDs(0, 60);
   uint numSquads = getSquadsFromGroups(groupIDs, squadIDs);
   if (numSquads)
   {
      BPlayerID playerID = getPlayerID();
      BArmy* pArmy = BEntityGrouper::getOrCreateCommonArmyForSquads(squadIDs, playerID);
      if (pArmy)
      {
         BWorkCommand tempCommand;
         tempCommand.setWaypoints(&location, 1);
         tempCommand.setRecipientType(BCommand::cArmy);
         tempCommand.setFlag(BWorkCommand::cFlagAlternate, false);
         tempCommand.setFlag(BWorkCommand::cFlagAttackMove, false);
         pArmy->queueOrder(&tempCommand);
      }
   }
}


//==============================================================================
//==============================================================================
void BAIMission::setGroupState(const BAIGroupIDArray& groupIDs, BAIGroupState gs)
{
   BASSERT(gs >= AIGroupState::cMin);
   BASSERT(gs <= AIGroupState::cMax);

   uint numGroups = groupIDs.getSize();
   for (uint i=0; i<numGroups; i++)
   {
      BAIGroup* pGroup = gWorld->getAIGroup(groupIDs[i]);
      if (pGroup)
         pGroup->setState(gs);
   }
}


//==============================================================================
//==============================================================================
void BAIMission::setGroupRole(const BAIGroupIDArray& groupIDs, BAIGroupRole gr)
{
   BASSERT(gr >= AIGroupRole::cMin);
   BASSERT(gr <= AIGroupRole::cMax);

   uint numGroups = groupIDs.getSize();
   for (uint i=0; i<numGroups; i++)
   {
      BAIGroup* pGroup = gWorld->getAIGroup(groupIDs[i]);
      if (pGroup)
         pGroup->setRole(gr);
   }
}


//==============================================================================
//==============================================================================
uint BAIMission::getGroupsByState(BAIGroupState gs, BAIGroupIDArray& groupIDs)
{
   groupIDs.resize(0);
   uint numGroups = mGroups.getSize();
   for (uint i=0; i<numGroups; i++)
   {
//-- FIXING PREFIX BUG ID 4024
      const BAIGroup* pGroup = gWorld->getAIGroup(mGroups[i]);
//--
      if (pGroup && pGroup->isState(gs))
         groupIDs.add(mGroups[i]);
   }
   return (groupIDs.getSize());
}


//==============================================================================
//==============================================================================
uint BAIMission::getGroupsByRole(BAIGroupRole gr, BAIGroupIDArray& groupIDs)
{
   groupIDs.resize(0);
   uint numGroups = mGroups.getSize();
   for (uint i=0; i<numGroups; i++)
   {
//-- FIXING PREFIX BUG ID 4025
      const BAIGroup* pGroup = gWorld->getAIGroup(mGroups[i]);
//--
      if (pGroup && pGroup->isRole(gr))
         groupIDs.add(mGroups[i]);
   }
   return (groupIDs.getSize());
}


//==============================================================================
//==============================================================================
void BAIMission::addSquad(BEntityID squadToAdd)
{
   BSquad* pSquad = validateSquadForMission(squadToAdd);
   if (!pSquad)
      return;

   // Don't bother adding a squad that's already assigned to this mission!
   bool missionContainsSquad = mSquads.contains(squadToAdd);
   //APF assert disabled, this is now a feature...
   //BASSERTM(!missionContainsSquad, "Trying to add a squad to a mission that already contains the squad!");
   if (missionContainsSquad)
      return;

   // This squad is assigned to another mission, so remove it.  Note:  You should be doing this in script.
   BAIMissionID squadCurrentMissionID = pSquad->getAIMissionID();
   //APF assert disabled, this is now a feature...
   //BASSERTM(squadCurrentMissionID == cInvalidAIMissionID, "Trying to add a squad to a mission who is assigned to another mission!  Doh!");
   if (squadCurrentMissionID != cInvalidAIMissionID)
      pSquad->removeFromAIMission();

   // Add the squad to the master list in the mission.
   mSquads.add(squadToAdd);

   // Add the squad to the ungrouped squads list in the mission so we can smartly assign it to do something.
   BASSERT(!mUngroupedSquads.contains(squadToAdd));
   mUngroupedSquads.add(squadToAdd);

   // Add the mapping for the squad group map
   mSquadGroupMap.add(BSquadGroupPair(squadToAdd, cInvalidAIGroupID));

   // Set the squad mission ID
   pSquad->setAIMissionID(mID);
}


//==============================================================================
//==============================================================================
void BAIMission::removeSquads(BEntityIDArray squadsToRemove)
{
   uint numSquadsToRemove = squadsToRemove.getSize();
   for (uint i=0; i<numSquadsToRemove; i++)
      removeSquad(squadsToRemove[i]);
}


//==============================================================================
//==============================================================================
void BAIMission::removeSquad(BEntityID squadToRemove)
{
   // Early bail if this squad isn't a part of the mission.
   if (!mSquads.contains(squadToRemove))
      return;

   // 1A - Ungroup the squad if it's grouped.
   // 1B - If the group is now empty, delete it.
   // 2A - Delete the squad group map.
   uint numSquadGroupMaps = mSquadGroupMap.getSize();
   for (uint i=0; i<numSquadGroupMaps; i++)
   {
      if (mSquadGroupMap[i].getSquadID() == squadToRemove)
      {
         BAIGroup* pGroup = gWorld->getAIGroup(mSquadGroupMap[i].getGroupID());
         if (pGroup)
         {
            pGroup->removeSquad(squadToRemove);
            if (pGroup->getNumSquads() == 0)
               gWorld->deleteAIGroup(mSquadGroupMap[i].getGroupID());
         }
         mSquadGroupMap.removeIndex(i);
         break;
      }
   }

   // Remove from squad list.
   mSquads.remove(squadToRemove);

   // For good measure.
   mUngroupedSquads.remove(squadToRemove);

   BSquad* pSquad = gWorld->getSquad(squadToRemove);
   if (pSquad)
      pSquad->setAIMissionID(cInvalidAIMissionID);
}


//==============================================================================
//==============================================================================
uint BAIMission::getSquads(BEntityIDArray& squads) const
{
   squads = mSquads;
   return (mSquads.getSize());
}


//==============================================================================
//==============================================================================
uint BAIMission::getGroups(BAIGroupIDArray& groupIDs) const
{
   groupIDs = mGroups;
   return (mGroups.getSize());
}


//==============================================================================
//==============================================================================
uint BAIMission::getNumSquads() const
{
   return (mSquads.getSize());
}


//==============================================================================
//==============================================================================
uint BAIMission::getUngroupedSquads(BEntityIDArray& ungroupedSquads) const
{
   ungroupedSquads = mUngroupedSquads;
   return (mUngroupedSquads.getSize());
}


//==============================================================================
//==============================================================================
uint BAIMission::getNumUngroupedSquads() const
{
   return (mUngroupedSquads.getSize());
}


//==============================================================================
//==============================================================================
void BAIMission::ungroupSquads(const BEntityIDArray& squadIDs)
{
   if (squadIDs.getSize() == 0)
      return;

   BEntityIDArray localCopySquadIDs = squadIDs;
   uint numSquads = localCopySquadIDs.getSize();
   for (uint i=0; i<numSquads; i++)
      ungroupSquad(localCopySquadIDs[i]);
}


//==============================================================================
//==============================================================================
void BAIMission::ungroupSquad(BEntityID squadID)
{
   // The easy way out (isn't in the mission, already ungrouped, etc.)
   BASSERT(mSquads.contains(squadID));
   if (!mSquads.contains(squadID))
      return;
   if (mUngroupedSquads.contains(squadID))
      return;

   // Ungroup me.
   uint numSquadGroupMaps = mSquadGroupMap.getSize();
   for (uint i=0; i<numSquadGroupMaps; i++)
   {
      if (mSquadGroupMap[i].getSquadID() == squadID)
      {
         BAIGroup* pGroup = gWorld->getAIGroup(mSquadGroupMap[i].getGroupID());
         if (pGroup)
         {
            pGroup->removeSquad(squadID);
            if (pGroup->getNumSquads() == 0)
               gWorld->deleteAIGroup(mSquadGroupMap[i].getGroupID());
         }
         mSquadGroupMap[i].setGroupID(cInvalidAIGroupID);
         mUngroupedSquads.add(squadID);
         return;
      }
   }

   BASSERTM(false, "Should not get this far.");
}


//==============================================================================
//==============================================================================
void BAIMission::groupSquads(const BEntityIDArray& squadIDs, BAIGroupID groupID)
{
   if (squadIDs.getSize() == 0)
      return;

   BEntityIDArray localCopySquadIDs = squadIDs;
   uint numSquads = localCopySquadIDs.getSize();
   for (uint i=0; i<numSquads; i++)
      groupSquad(localCopySquadIDs[i], groupID);
}


//==============================================================================
//==============================================================================
void BAIMission::groupSquad(BEntityID squadID, BAIGroupID groupID)
{
   BASSERT(mSquads.contains(squadID));
//-- FIXING PREFIX BUG ID 4028
   const BSquad* pSquad = gWorld->getSquad(squadID);
//--
   BASSERT(pSquad);
   if (!pSquad)
      return;
   BAIGroup* pGroup = gWorld->getAIGroup(groupID);
   BASSERT(pGroup);
   if (!pGroup)
      return;

   // First get him out of his current group.
   ungroupSquad(squadID);
   BASSERT(mUngroupedSquads.contains(squadID));

   pGroup->addSquad(squadID);
   mUngroupedSquads.remove(squadID);

   // Ungroup me.
   uint numSquadGroupMaps = mSquadGroupMap.getSize();
   for (uint i=0; i<numSquadGroupMaps; i++)
   {
      if (mSquadGroupMap[i].getSquadID() == squadID)
      {
         mSquadGroupMap[i].setGroupID(groupID);
         return;
      }
   }

   BASSERTM(false, "Should not get this far.");
}


//==============================================================================
//==============================================================================
BAIGroupID BAIMission::findExistingGroupForSquad(BEntityID squadID) const
{
   const BSquad* pSquad = gWorld->getSquad(squadID);
   if (!pSquad || pSquad->getFlagContainedSpartan())
      return (cInvalidAIGroupID);

   uint numGroups = mGroups.getSize();
   for (uint i=0; i<numGroups; i++)
   {
      const BAIGroup* pAIGroup = gWorld->getAIGroup(mGroups[i]);
      if (pAIGroup && pAIGroup->canAcceptSquad(squadID))
         return (pAIGroup->getID());
   }

   return (cInvalidAIGroupID);
}


//==============================================================================
//==============================================================================
void BAIMission::processUngroupedSquads()
{
   // The easy way out.
   if (mUngroupedSquads.getSize() == 0)
      return;

   // 1st pass.
   // Go through our ungrouped squads and check existing groups and see if they will accept the squad.
   if (mGroups.getSize() > 0)
      processUngroupedSquadsForExistingGroups();

   // 2nd pass.
   // Go through the remaining ungrouped squads (if any) and bucketize them by type, the group them by proximity.
   processUngroupedSquadsForNewGroups();

   //BASSERTM(mUngroupedSquads.getSize() == 0, "BAIMission::processUngroupedSquads - some ungrouped squads remain after processing.");
   // mrh - Note that spartans can remain ungrouped while they are hijacking stuff.
}


//==============================================================================
//==============================================================================
void BAIMission::processUngroupedSquadsForExistingGroups()
{
   BEntityIDArray localUngroupedSquads = mUngroupedSquads;
   for (uint i=0; i<localUngroupedSquads.getSize(); i++)
   {
      BAIGroupID existingGroupID = findExistingGroupForSquad(localUngroupedSquads[i]);
      if (existingGroupID.isValid())
      {
         groupSquad(localUngroupedSquads[i], existingGroupID);
         continue;
      }
   }
}


//==============================================================================
//==============================================================================
void BAIMission::processUngroupedSquadsForNewGroups()
{
   uint numUngroupedSquads = mUngroupedSquads.getSize();
   if (numUngroupedSquads == 0)
      return;

   BAI* pAI = gWorld->getAI(mPlayerID);
   if (!pAI)
      return;

   BEntityIDArray localSquads(0, numUngroupedSquads);
   for (uint ungroupedSquadIndex=0; ungroupedSquadIndex<numUngroupedSquads; ungroupedSquadIndex++)
   {
      const BSquad* pSquad = gWorld->getSquad(mUngroupedSquads[ungroupedSquadIndex]);
      if (pSquad && !pSquad->getFlagContainedSpartan())
         localSquads.add(mUngroupedSquads[ungroupedSquadIndex]);
   }

   BEntityGrouper* pEntityGrouper = gWorld->getEntityGrouper();
   BASSERT(pEntityGrouper);
   pEntityGrouper->reset();
   pEntityGrouper->setRadius(50.0f);
   pEntityGrouper->groupByProtoSquad(true);
   pEntityGrouper->groupEntities(localSquads);

   // Now go through all the groups and create AIGroups for them.
   uint numGroups = pEntityGrouper->getNumberGroups();
   for (uint i=0; i<numGroups; i++)
   {
      const BEntityGroup* pGrp = pEntityGrouper->getGroupByIndex(i);
      if (!pGrp)
         continue;

      const BEntityIDArray& entities = pGrp->getEntities();
      if (entities.getSize() > 0)
      {
         const BSquad* pSquad = gWorld->getSquad(entities[0]);
         if (pSquad)
         {
            BAIGroup* pNewAIGroup = gWorld->createAIGroup(mID, pSquad->getProtoSquadID());
            if (pNewAIGroup)
               groupSquads(entities, pNewAIGroup->getID());
         }
      }
   }
}


//==============================================================================
//==============================================================================
void BAIMission::deleteAllGroups()
{
   // Cache the list because we modify it as we delete.
   BAIGroupIDArray groupsToDelete = mGroups;
   uint numGroups = groupsToDelete.getSize();

   for (uint i=0; i<numGroups; i++)
      gWorld->deleteAIGroup(groupsToDelete[i]);

   BASSERT(mGroups.getSize() == 0);
}

//==============================================================================
// BAIMission::save
//==============================================================================
bool BAIMission::save(BStream* pStream, int saveType) const
{
   GFWRITECLASS(pStream, saveType, mLaunchScores);
   GFWRITEARRAY(pStream, BEntityID, mSquads, uint16, 5000);
   GFWRITEARRAY(pStream, BEntityID, mUngroupedSquads, uint16, 5000);
   GFWRITEARRAY(pStream, BSquadGroupPair, mSquadGroupMap, uint16, 5000);
   GFWRITEARRAY(pStream, BAIGroupID, mGroups, uint16, 1000);
   GFWRITEARRAY(pStream, BAIGroupID, mWorkingGroups, uint16, 1000);
   GFWRITECLASS(pStream, saveType, mScoreModifier);
   GFWRITECLASS(pStream, saveType, mPlayerModifier);
   GFWRITEARRAY(pStream, BBidID, mBidIDs, uint16, 1000);

   GFWRITEARRAY(pStream, BAIMissionTargetWrapperID, mTargetWrapperIDs, uint16, 5000);

   GFWRITEVAR(pStream, BAIMissionTargetWrapperID, mPrevUpdateWrapperID);
   GFWRITEVAR(pStream, BAIMissionTargetID, mPrevUpdateTargetID);

   GFWRITEVAR(pStream, uint, mFailConditionMinSquads);
   GFWRITEVAR(pStream, float, mManualRetreatPosX);
   GFWRITEVAR(pStream, float, mManualRetreatPosZ);

   GFWRITECLASS(pStream, saveType, mCache);

   GFWRITEVAR(pStream, BAIMissionID, mID);
   GFWRITEVAR(pStream, BAITopicID, mAITopicID);
   GFWRITEVAR(pStream, BPlayerID, mPlayerID);
   GFWRITEVAR(pStream, BAIMissionType, mType);
   GFWRITEVAR(pStream, BAIMissionState, mState);

   GFWRITEVAR(pStream, DWORD, mTimestampActiveToggled);
   GFWRITEVAR(pStream, DWORD, mTimestampStateChanged);
   GFWRITEVAR(pStream, DWORD, mTimestampNextOppDetect);
   GFWRITEVAR(pStream, DWORD, mTimestampNextAct);
   GFWRITEVAR(pStream, DWORD, mTimestampNextThink);
   GFWRITEVAR(pStream, DWORD, mTimestampNextPowerEval);
   GFWRITEVAR(pStream, DWORD, mTimestampNextODSTEval);
   GFWRITEVAR(pStream, DWORD, mTimestampEnemiesCleared);

   GFWRITEBITBOOL(pStream, mFlagActive);
   GFWRITEBITBOOL(pStream, mFlagAllowOpportunityTargets);
   GFWRITEBITBOOL(pStream, mFlagMoveAttack);
   GFWRITEBITBOOL(pStream, mFlagEnemiesCleared);
   GFWRITEBITBOOL(pStream, mFlagManualRetreatPos);
   GFWRITEBITBOOL(pStream, mFlagRepeatTargets);
   GFWRITEBITBOOL(pStream, mFlagEnableOdstDrop);

#ifndef BUILD_FINAL  
   GFWRITESTRING(pStream, BSimString, mName, 100);
   GFWRITEVAR(pStream, uint, mDebugID);
#else
   BSimString tempStr;
   GFWRITESTRING(pStream, BSimString, tempStr, 100);
   GFWRITEVAL(pStream, uint, 0);
#endif

   return true;
}

//==============================================================================
// BAIMission::load
//==============================================================================
bool BAIMission::load(BStream* pStream, int saveType)
{  
   GFREADCLASS(pStream, saveType, mLaunchScores);
   GFREADARRAY(pStream, BEntityID, mSquads, uint16, 5000);
   GFREADARRAY(pStream, BEntityID, mUngroupedSquads, uint16, 5000);
   GFREADARRAY(pStream, BSquadGroupPair, mSquadGroupMap, uint16, 5000);
   GFREADARRAY(pStream, BAIGroupID, mGroups, uint16, 1000);
   GFREADARRAY(pStream, BAIGroupID, mWorkingGroups, uint16, 1000);
   GFREADCLASS(pStream, saveType, mScoreModifier);
   if (BAIMission::mGameFileVersion >= 2)
      GFREADCLASS(pStream, saveType, mPlayerModifier);
   GFREADARRAY(pStream, BBidID, mBidIDs, uint16, 1000);

   GFREADARRAY(pStream, BAIMissionTargetWrapperID, mTargetWrapperIDs, uint16, 5000);

   GFREADVAR(pStream, BAIMissionTargetWrapperID, mPrevUpdateWrapperID);
   GFREADVAR(pStream, BAIMissionTargetID, mPrevUpdateTargetID);

   GFREADVAR(pStream, uint, mFailConditionMinSquads);
   GFREADVAR(pStream, float, mManualRetreatPosX);
   GFREADVAR(pStream, float, mManualRetreatPosZ);

   GFREADCLASS(pStream, saveType, mCache);

   GFREADVAR(pStream, BAIMissionID, mID);
   GFREADVAR(pStream, BAITopicID, mAITopicID);
   GFREADVAR(pStream, BPlayerID, mPlayerID);
   GFREADVAR(pStream, BAIMissionType, mType);
   GFREADVAR(pStream, BAIMissionState, mState);

   GFREADVAR(pStream, DWORD, mTimestampActiveToggled);
   GFREADVAR(pStream, DWORD, mTimestampStateChanged);
   GFREADVAR(pStream, DWORD, mTimestampNextOppDetect);
   GFREADVAR(pStream, DWORD, mTimestampNextAct);
   GFREADVAR(pStream, DWORD, mTimestampNextThink);
   GFREADVAR(pStream, DWORD, mTimestampNextPowerEval);
   mTimestampNextODSTEval = 0;
   if (BAIMission::mGameFileVersion >= 4)
      GFREADVAR(pStream, DWORD, mTimestampNextODSTEval);
   GFREADVAR(pStream, DWORD, mTimestampEnemiesCleared);

   GFREADBITBOOL(pStream, mFlagActive);
   GFREADBITBOOL(pStream, mFlagAllowOpportunityTargets);
   GFREADBITBOOL(pStream, mFlagMoveAttack);
   GFREADBITBOOL(pStream, mFlagEnemiesCleared);
   GFREADBITBOOL(pStream, mFlagManualRetreatPos);
   GFREADBITBOOL(pStream, mFlagRepeatTargets);

   mFlagEnableOdstDrop = false;
   if (BAIMission::mGameFileVersion >= 3)
      GFREADBITBOOL(pStream, mFlagEnableOdstDrop);

#ifndef BUILD_FINAL  
   GFREADSTRING(pStream, BSimString, mName, 100);
   GFREADVAR(pStream, uint, mDebugID);
#else
   BSimString tempStr;
   GFREADSTRING(pStream, BSimString, tempStr, 100);
   GFREADTEMPVAL(pStream, uint);
#endif

   return true;
}


//==============================================================================
//==============================================================================
float BAIScoreModifier::GetScoreModifier(const BSquad *pSquad, float defaultResult) const
{  
   const BProtoObject* pProtoObject = pSquad->getProtoObject();
   if (!pProtoObject)
      return (defaultResult);

   bool returnDefaultResult = true;
   float modifier = 1.0f;
   uint numMultipliers = mMultipliers.size();
   for(uint i=0; i<numMultipliers; i++)
   {
      if (pProtoObject->isType(mMultipliers[i].first))
      {
         modifier *= mMultipliers[i].second;
         returnDefaultResult = false;
      }
   }

   if (modifier < 0.0f)
      modifier = 0.0f;

   if (returnDefaultResult)
      return (defaultResult);
   else
      return (modifier);
}


//==============================================================================
//==============================================================================
float BAIPlayerModifier::GetPlayerModifier(BPlayerID playerID, float defaultResult) const
{  
   bool returnDefaultResult = true;
   float modifier = 1.0f;
   uint numMultipliers = mMultipliers.size();
   for(uint i=0; i<numMultipliers; i++)
   {
      if (playerID == mMultipliers[i].first)
      {
         modifier *= mMultipliers[i].second;
         returnDefaultResult = false;
      }
   }

   if (modifier < 0.0f)
      modifier = 0.0f;

   if (returnDefaultResult)
      return (defaultResult);
   else
      return (modifier);
}

//==============================================================================
// BAIScoreModifier::save
//==============================================================================
bool BAIScoreModifier::save(BStream* pStream, int saveType) const
{
   GFWRITEARRAY(pStream, BScoreMultPair, mMultipliers, uint8, 200);
   return true;
}

//==============================================================================
// BAIScoreModifier::load
//==============================================================================
bool BAIScoreModifier::load(BStream* pStream, int saveType)
{  
   GFREADARRAY(pStream, BScoreMultPair, mMultipliers, uint8, 200);

   uint count=mMultipliers.size();
   for (uint i=0; i<count; i++)
      gSaveGame.remapObjectType(mMultipliers[i].first);

   return true;
}


//==============================================================================
//==============================================================================
bool BAIPlayerModifier::save(BStream* pStream, int saveType) const
{
   GFWRITEARRAY(pStream, BPlayerMultPair, mMultipliers, uint8, 200);
   return true;
}

//==============================================================================
//==============================================================================
bool BAIPlayerModifier::load(BStream* pStream, int saveType)
{  
   GFREADARRAY(pStream, BPlayerMultPair, mMultipliers, uint8, 200);
   return true;
}


#ifndef BUILD_FINAL
//==============================================================================
//==============================================================================
void BAIMission::debugRender()
{
   if (gGame.getAIDebugType() == AIDebugType::cNone)
      return;

   if (gGame.getAIDebugType() == AIDebugType::cAIActiveMission)
   {
      if (!mFlagActive)
         return;
//-- FIXING PREFIX BUG ID 3997
      const BUser* pUser = gUserManager.getPrimaryUser();
//--
      if (!pUser)
         return;
      if (getPlayerID() != pUser->getPlayerID())
         return;
   }

   if (gGame.getAIDebugType() == AIDebugType::cAIHoverMission)
   {
      bool renderMission = false;
      BUser* pUser = gUserManager.getPrimaryUser();
      if (!pUser)
         return;
      BSelectionManager* pSM = pUser->getSelectionManager();
      if (!pSM)
         return;
      if (pSM->getNumberSelectedSquads() > 0 && mSquads.contains(pSM->getSelectedSquad(0)))
         renderMission = true;
      if (!renderMission)
      {
         BEntityID hoverObj = pUser->getHoverObject();
//-- FIXING PREFIX BUG ID 3998
         const BSquad* pSquad = gWorld->getSquad(hoverObj);
//--
         if (pSquad && mSquads.contains(hoverObj))
            renderMission = true;
      }
      if (!renderMission)
      {
         BEntityID hoverObj = pUser->getHoverObject();
//-- FIXING PREFIX BUG ID 3999
         const BUnit* pUnit = gWorld->getUnit(hoverObj);
//--
         if (pUnit && mSquads.contains(pUnit->getParentID()))
            renderMission = true;
      }
      if (!renderMission)
         return;
   }


   const BAIMissionTargetWrapper* pWrapper = getCurrentTargetWrapper();
   if (pWrapper)
   {
      const BAIMissionTarget* pTarget = gWorld->getAIMissionTarget(pWrapper->getTargetID());
      if (pTarget)
      {
         BVector targetPos = pTarget->getPosition();
         const float cWrapperDebugTerrainOffset = 1.0f;
         
         float leashDist = pWrapper->getLeashDist() + 4.0f;
         if (gWorld->isSphereVisible(targetPos, leashDist))
            gTerrainSimRep.addDebugCircleOverTerrain(targetPos, leashDist, cDWORDRed, cWrapperDebugTerrainOffset, BDebugPrimitives::cCategoryAI);

         float secureDist = pWrapper->getSecureDist() + 3.0f;
         if (gWorld->isSphereVisible(targetPos, secureDist))
            gTerrainSimRep.addDebugCircleOverTerrain(targetPos, secureDist, cDWORDOrange, cWrapperDebugTerrainOffset, BDebugPrimitives::cCategoryAI);

         float rallyDist = pWrapper->getRallyDist() + 2.0f;
         if (gWorld->isSphereVisible(targetPos, rallyDist))
            gTerrainSimRep.addDebugCircleOverTerrain(targetPos, rallyDist, cDWORDYellow, cWrapperDebugTerrainOffset, BDebugPrimitives::cCategoryAI);

         float searchWorkDist = pWrapper->getSearchWorkDist() + 1.0f;
         if (gWorld->isSphereVisible(targetPos, searchWorkDist))
            gTerrainSimRep.addDebugCircleOverTerrain(targetPos, searchWorkDist, cDWORDGreen, cWrapperDebugTerrainOffset, BDebugPrimitives::cCategoryAI);

         float hotZoneDist = pWrapper->getHotZoneDist();
         if (gWorld->isSphereVisible(targetPos, hotZoneDist))
            gTerrainSimRep.addDebugCircleOverTerrain(targetPos, hotZoneDist, cDWORDBlue, cWrapperDebugTerrainOffset, BDebugPrimitives::cCategoryAI);
      }
   }

   uint numGroups = mGroups.getSize();
   for (uint i=0; i<numGroups; i++)
   {
      BAIGroup* pGroup = gWorld->getAIGroup(mGroups[i]);
      if (pGroup)
      {
         pGroup->debugRender();
         pGroup->debugRenderTargets();
      }
   }
}
#endif

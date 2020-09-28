//==============================================================================
// aimissiongroup.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================


// xgame
#include "common.h"
#include "ai.h"
#include "aidebug.h"
#include "aimissiongroup.h"
#include "commands.h"
#include "configsgame.h"
#include "EntityGrouper.h"
#include "FontSystem2.h"
#include "game.h"
#include "kb.h"
#include "tactic.h"
#include "protosquad.h"
#include "simhelper.h"
#include "team.h"
#include "user.h"
#include "usermanager.h"
#include "selectionmanager.h"
#include "world.h"
#include "render.h"

GFIMPLEMENTVERSION(BAIGroup, 3);
GFIMPLEMENTVERSION(BAIGroupTask, 2);


//==============================================================================
//==============================================================================
BTaskInfo::BTaskInfo()
{
   reset();
}


//==============================================================================
//==============================================================================
void BTaskInfo::reset()
{
   mTargetPos.zero();
   mTargetID = cInvalidObjectID;
   mAbilityID = cInvalidAbilityID;
   mType = AIGroupTaskType::cInvalid;
   mScore = 0.0f;
}


//==============================================================================
//==============================================================================
void BTaskInfo::setTarget(BEntityID v)
{
   mTargetID = v;
   const BEntity* pEntity = gWorld->getEntity(v);
   if (pEntity)
      mTargetPos = pEntity->getPosition();
}


//==============================================================================
//==============================================================================
void BTaskInfo::setTarget(BVector v)
{
   mTargetPos = v;
   mTargetID = cInvalidObjectID;
}


//==============================================================================
//==============================================================================
BAIGroupTask::BAIGroupTask()
{
   mID = 0;
   resetNonIDData();
}


//==============================================================================
//==============================================================================
void BAIGroupTask::resetNonIDData()
{
   mTargetPos.zero();
   mTargetID = cInvalidObjectID;
   mAbilityID = cInvalidAbilityID;
   mScore = 0.0f;
   mPlayerID = cInvalidPlayerID;
   mGroupID = cInvalidAIGroupID;
   mType = AIGroupTaskType::cInvalid;
   mGroupPosWhenTasked.zero();
   mTimestampTasked = 0;
   mTimestampValidUntil = 0;
   mFlagValidUntil = false;
}


//==============================================================================
//==============================================================================
#ifndef BUILD_FINAL
void BAIGroupTask::getDebugString(BSimString& str) const
{
   if (mType == AIGroupTaskType::cWait)
   {
      str.format("Wait");
   }
   else if (mType == AIGroupTaskType::cMove)
   {
      str.format("Move to (%.0f, %.0f)", mTargetPos.x, mTargetPos.z);
   }
   else if (mType == AIGroupTaskType::cAttack)
   {
      BEntity* pEntity = gWorld->getEntity(mTargetID);
      if (pEntity)
      {
//-- FIXING PREFIX BUG ID 4853
         const BUnit* pUnit = pEntity->getUnit();
//--
         if (pUnit)
         {
            str.format("Attack Unit %d - %s", pUnit->getID(), pUnit->getProtoObject()->getName().getPtr());
         }
         else
         {
//-- FIXING PREFIX BUG ID 4852
            const BSquad* pSquad = pEntity->getSquad();
//--
            if (pSquad)
               str.format("Attack Squad %d - %s", pSquad->getID(), pSquad->getProtoSquad()->getName().getPtr());
         }
      }
      else
      {
         str.format("Attack");
      }
   }
   else if (mType == AIGroupTaskType::cDetonate)
   {
      BEntity* pEntity = gWorld->getEntity(mTargetID);
      if (pEntity)
      {
         //-- FIXING PREFIX BUG ID 4853
         const BUnit* pUnit = pEntity->getUnit();
         //--
         if (pUnit)
         {
            str.format("Detonate Unit %d - %s", pUnit->getID(), pUnit->getProtoObject()->getName().getPtr());
         }
         else
         {
            //-- FIXING PREFIX BUG ID 4852
            const BSquad* pSquad = pEntity->getSquad();
            //--
            if (pSquad)
               str.format("Detonate Squad %d - %s", pSquad->getID(), pSquad->getProtoSquad()->getName().getPtr());
         }
      }
      else
      {
         str.format("Detonate");
      }
   }
   else if (mType == AIGroupTaskType::cGather)
   {
      BEntity* pEntity = gWorld->getEntity(mTargetID);
      if (pEntity)
      {
         BUnit* pUnit = pEntity->getUnit();
         if (pUnit)
            str.format("Gather resource (%s)", pUnit->getProtoObject()->getName().getPtr());
         else
         {
            BSquad* pSquad = pEntity->getSquad();
            if (pSquad)
               str.format("Gather resource (%s)", pSquad->getProtoSquad()->getName().getPtr());
         }
      }
      else
      {
         str.format("Gather");
      }
   }
   else if (mType == AIGroupTaskType::cRepairOther)
   {
      BEntity* pEntity = gWorld->getEntity(mTargetID);
      if (pEntity)
      {
         BUnit* pUnit = pEntity->getUnit();
         if (pUnit)
            str.format("RepairOther (%s)", pUnit->getProtoObject()->getName().getPtr());
         else
         {
            BSquad* pSquad = pEntity->getSquad();
            if (pSquad)
               str.format("RepairOther (%s)", pSquad->getProtoSquad()->getName().getPtr());
         }
      }
      else
      {
         str.format("RepairOther");
      }
   }
   else
   {
      str.format("None");
   }
}
#endif

//==============================================================================
// BAIGroupTask::save
//==============================================================================
bool BAIGroupTask::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mTargetPos);
   GFWRITEVAR(pStream, BEntityID, mTargetID);
   GFWRITEVAR(pStream, BAbilityID, mAbilityID);
   GFWRITEVAR(pStream, float, mScore);
   GFWRITEVAR(pStream, BAIGroupTaskType, mType);
   GFWRITEVAR(pStream, BAIGroupTaskID, mID);
   GFWRITEVAR(pStream, BPlayerID, mPlayerID);
   GFWRITEVAR(pStream, BAIGroupID, mGroupID);
   GFWRITEVECTOR(pStream, mGroupPosWhenTasked);
   GFWRITEVAR(pStream, DWORD, mTimestampTasked);
   GFWRITEVAR(pStream, DWORD, mTimestampValidUntil);
   GFWRITEBITBOOL(pStream, mFlagValidUntil);
   return true;
}

//==============================================================================
// BAIGroupTask::load
//==============================================================================
bool BAIGroupTask::load(BStream* pStream, int saveType)
{  
   // ajl 8/29/08 - using version from BAI since version wasn't being saved/loaded for BAIGroupTasks (but now it is).
   if (BAI::mGameFileVersion < 2)
   {
      BSimTarget tempSimTarget;
      GFREADCLASS(pStream, saveType, tempSimTarget);
      mTargetPos = tempSimTarget.getPosition();
      mTargetID = tempSimTarget.getID();
      mAbilityID = tempSimTarget.getAbilityID();
   }
   else
   {
      GFREADVECTOR(pStream, mTargetPos);
      GFREADVAR(pStream, BEntityID, mTargetID);
      GFREADVAR(pStream, BAbilityID, mAbilityID);
      gSaveGame.remapAbilityID(mAbilityID);
   }

   GFREADVAR(pStream, float, mScore);
   GFREADVAR(pStream, BAIGroupTaskType, mType);
   GFREADVAR(pStream, BAIGroupTaskID, mID);
   GFREADVAR(pStream, BPlayerID, mPlayerID);
   GFREADVAR(pStream, BAIGroupID, mGroupID);
   GFREADVECTOR(pStream, mGroupPosWhenTasked);
   GFREADVAR(pStream, DWORD, mTimestampTasked);
   GFREADVAR(pStream, DWORD, mTimestampValidUntil);
   GFREADBITBOOL(pStream, mFlagValidUntil);
   return true;
}


//==============================================================================
//==============================================================================
BAIGroup::BAIGroup()
{
   mID = 0;
   resetNonIDData();
}


//==============================================================================
//==============================================================================
void BAIGroup::resetNonIDData()
{
   mPosition.zero();
   mCentroid.zero();
   mRadius = 0.0f;
   mPlayerID = cInvalidPlayerID;
   mMissionID = cInvalidAIMissionID;
   mState = AIGroupState::cWaiting;
   mSquads.resize(0);
   mTasks.resize(0);
   mSquadsAnalysis.reset();
   mRole = AIGroupRole::cNormal;
   mTimestampCreated = 0;
   mTimestampLastEval = 0;
   mTimestampLastTask = 0;
   mTimestampSquadAdded = 0;
   mMaxAttackRange = 0.0f;
   mMaxVelocity = 0.0f;
   mProtoSquadID = cInvalidProtoSquadID;
   mFlagTasked = false;
   mFlagTaskedThisState = false;
   mFlagCanGather = false;
   mFlagCanGatherSupplies = false;
}


//==============================================================================
//==============================================================================
void BAIGroup::addSquad(BEntityID squadID)
{
   BASSERT(!mSquads.contains(squadID));
   BASSERT((mProtoSquadID == cInvalidProtoSquadID) || (gWorld->getSquad(squadID) && (gWorld->getSquad(squadID)->getProtoSquadID() == mProtoSquadID)));
   mSquads.add(squadID);
   mTimestampSquadAdded = gWorld->getGametime();
}


//==============================================================================
//==============================================================================
void BAIGroup::removeSquad(BEntityID squadID)
{
   BASSERT(mSquads.contains(squadID));
   mSquads.remove(squadID);
}


//==============================================================================
//==============================================================================
void BAIGroup::recalculatePosition()
{
   BVector centroid = BSimHelper::computeCentroid(mSquads);
   BEntityID squadID = BSimHelper::computeClosestToPoint(centroid, mSquads);
   const BSquad* pSquad = gWorld->getSquad(squadID);

   mCentroid = centroid;
   mPosition = centroid;

   if (pSquad)
   {
      BSimHelper::computeMostNearAndFarToPoint(pSquad->getPosition(), mSquads, NULL, NULL, NULL, &mRadius);
      setPosition(pSquad->getPosition());
   }
} 


//==============================================================================
//==============================================================================
void BAIGroup::recalculateSquadAnalysis()
{
   BAI::calculateSquadAnalysis(mSquads, mSquadsAnalysis);
}


//==============================================================================
//==============================================================================
void BAIGroup::recalculateAttackRanges()
{
   float maxAttackRange = 0.0f;
   float maxVelocity = 0.0f;
   float valid = -1.0f;

   uint numSquads = mSquads.getSize();
   for (uint i=0; i<numSquads; i++)
   {
      const BSquad* pSquad = gWorld->getSquad(mSquads[i]);
      if (!pSquad)
         continue;

      // Use fselect intrinsics to calculate the max range & velocity without branching.
      float range = pSquad->getAttackRange();
      float velocity = pSquad->getMaxVelocity();
      float cmpRange = static_cast<float>(Math::fSelectMax(range, maxAttackRange));
      float cmpVelocity = static_cast<float>(Math::fSelectMax(velocity, maxVelocity));
      maxAttackRange = static_cast<float>(Math::fSelect(valid, cmpRange, range));
      maxVelocity = static_cast<float>(Math::fSelect(valid, cmpVelocity, velocity));
      valid = 1.0f;
   }

   mMaxAttackRange = maxAttackRange;
   mMaxVelocity = maxVelocity;
}


//==============================================================================
//==============================================================================
void BAIGroup::deleteAllTasks()
{
   // Cache this because we modify the list as we remove them.
   BAIGroupTaskIDArray tasksToDelete = mTasks;
   uint numTasks = tasksToDelete.getSize();

   for (uint i=0; i<numTasks; i++)
      gWorld->deleteAIGroupTask(tasksToDelete[i]);

   BASSERT(mTasks.getSize() == 0);
}


//==============================================================================
//==============================================================================
#ifndef BUILD_FINAL
void BAIGroup::debugRender()
{
   int aidt = gGame.getAIDebugType();
   if (aidt != AIDebugType::cAIActiveMission && aidt != AIDebugType::cAIHoverMission)
      return;
   BAIMission* pMission = gWorld->getAIMission(mMissionID);
   if (!pMission)
      return;
  
   BHandle hFont = gFontManager.getFontCourier10();
   gFontManager.setFont(hFont);
   DWORD dwColor = cDWORDWhite;
   BAIGroup* pMostRecentGrp = pMission->getMostRecentlyTaskedGroup(pMission->getGroups());
   if (pMostRecentGrp && pMostRecentGrp->getID() == mID)
      dwColor = cDWORDGreen;

   // Draw the summary over the mission group
   BFixedString128 debugText;
   DWORD currentGameTime = gWorld->getGametime();
   float sx = 0.0f;
   float sy = 0.0f;
   float lineHeight = gFontManager.getLineHeight();
   gRender.getViewParams().calculateWorldToScreen(mCentroid, sx, sy);

   // Line #1 - Num Squads, ProtoSquad
   {
      debugText.format("ERROR");
      const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
      if (pPlayer)
      {
         const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(mProtoSquadID);
         if (pProtoSquad)
            debugText.format("(%dx) %s", mSquads.getSize(), pProtoSquad->getName().getPtr());
      }
      gFontManager.drawText(hFont, sx, sy, debugText, dwColor);
      sy += lineHeight;
   }

   // Line #2 - Group lifespan.
   {
      DWORD lifespan = currentGameTime - mTimestampCreated;
      DWORD minutes = lifespan / 60000;
      float seconds = static_cast<float>(lifespan % 60000) / 1000.0f;
      debugText.format("Lifespan: %dm:%2.1fs", minutes, seconds);
      gFontManager.drawText(hFont, sx, sy, debugText, dwColor);
      sy += lineHeight;
   }

   // Line #2 - Time since tasked
   {
      DWORD lastTasked = currentGameTime - mTimestampLastTask;
      DWORD minutes = lastTasked / 60000;
      float seconds = static_cast<float>(lastTasked % 60000) / 1000.0f;
      debugText.format("Last Tasked: %dm:%2.1fs", minutes, seconds);
      gFontManager.drawText(hFont, sx, sy, debugText, dwColor);
      sy += lineHeight;
   }

   // Line #3 - Current Task
   {
      BSimString taskDebugString = "None";
      if (mTasks.getSize() > 0)
      {
         const BAIGroupTask* pTask = gWorld->getAIGroupTask(mTasks[0]);
         if (pTask)
         {
            pTask->getDebugString(taskDebugString);
            if (pTask->getType() == AIGroupTaskType::cMove)
            {
//-- FIXING PREFIX BUG ID 4854
               const BEntity* pEntity = gWorld->getEntity(pTask->getTargetID());
//--
               BVector targetPos = pEntity ? pEntity->getPosition() : pTask->getTargetPos();
               BVector forward = targetPos - mPosition;
               gTerrainSimRep.addDebugThickCrossOverTerrain(targetPos, cZAxisVector, 0.5f, dwColor, 1.0f, 5.0f, BDebugPrimitives::cCategoryAI);
               if (!forward.almostEqualXZ(BVector(0.0f)))
               {
                  gTerrainSimRep.addDebugThickArrowOverTerrain(mPosition, forward, 0.5f, dwColor, 1.0f, 6.0f, BDebugPrimitives::cCategoryAI);
               }
            }
            else if (pTask->getType() == AIGroupTaskType::cAttack)
            {
//-- FIXING PREFIX BUG ID 4855
               const BEntity* pEntity = gWorld->getEntity(pTask->getTargetID());
//--
               BVector targetPos = pEntity ? pEntity->getPosition() : pTask->getTargetPos();
               BVector forward = targetPos - mPosition;
               gTerrainSimRep.addDebugThickCrossOverTerrain(targetPos, cZAxisVector, 0.5f, dwColor, 1.0f, 5.0f, BDebugPrimitives::cCategoryAI);
               if (!forward.almostEqualXZ(BVector(0.0f)))
               {
                  gTerrainSimRep.addDebugThickArrowOverTerrain(mPosition, forward, 0.5f, dwColor, 1.0f, 6.0f, BDebugPrimitives::cCategoryAI);
               }
            }
            else if (pTask->getType() == AIGroupTaskType::cGather)
            {
               BEntity* pEntity = gWorld->getEntity(pTask->getTargetID());
               BVector targetPos = pEntity ? pEntity->getPosition() : pTask->getTargetPos();
               BVector forward = targetPos - mPosition;
               gTerrainSimRep.addDebugThickCrossOverTerrain(targetPos, cZAxisVector, 0.5f, dwColor, 1.0f, 5.0f, BDebugPrimitives::cCategoryAI);
               if (!forward.almostEqualXZ(BVector(0.0f)))
               {
                  gTerrainSimRep.addDebugThickArrowOverTerrain(mPosition, forward, 0.5f, dwColor, 1.0f, 6.0f, BDebugPrimitives::cCategoryAI);
               }
            }
            else if (pTask->getType() == AIGroupTaskType::cRepairOther)
            {
               BEntity* pEntity = gWorld->getEntity(pTask->getTargetID());
               BVector targetPos = pEntity ? pEntity->getPosition() : pTask->getTargetPos();
               BVector forward = targetPos - mPosition;
               gTerrainSimRep.addDebugThickCrossOverTerrain(targetPos, cZAxisVector, 0.5f, dwColor, 1.0f, 5.0f, BDebugPrimitives::cCategoryAI);
               if (!forward.almostEqualXZ(BVector(0.0f)))
               {
                  gTerrainSimRep.addDebugThickArrowOverTerrain(mPosition, forward, 0.5f, dwColor, 1.0f, 6.0f, BDebugPrimitives::cCategoryAI);
               }
            }
         }
      }
      debugText.format("Task: %s", taskDebugString.getPtr());
      gFontManager.drawText(hFont, sx, sy, debugText, dwColor);
      sy += lineHeight;
   }

   // Draw the lines over the mission group.
   uint numGroupSquads = mSquads.getSize();
   if (numGroupSquads > 0)
   {
      static const float cMissionGroupLineThickness = 0.1f;
      static const float cMissionGroupLineTerrainOffset = 1.0f;
      for (uint i=0; i<numGroupSquads; i++)
      {
//-- FIXING PREFIX BUG ID 4856
         const BSquad* pSquad = gWorld->getSquad(mSquads[i]);
//--
         if (pSquad)
            gTerrainSimRep.addDebugThickLineOverTerrain(mCentroid, pSquad->getPosition(), cMissionGroupLineThickness, dwColor, dwColor, cMissionGroupLineTerrainOffset, BDebugPrimitives::cCategoryAI);
      }
   }
}
#endif


#ifndef BUILD_FINAL
//==============================================================================
//==============================================================================
void BAIGroup::debugRenderTargets()
{
   int aidt = gGame.getAIDebugType();
   if (aidt != AIDebugType::cAIActiveMission && aidt != AIDebugType::cAIHoverMission)
      return;
   BAIMission* pMission = gWorld->getAIMission(mMissionID);
   if (!pMission)
      return;
   if (pMission->getMostRecentlyTaskedGroup(pMission->getGroups()) != this)
      return;

   DWORD playerColor = cDWORDGreen;
   uint numTasks = mPotentialTasks.getSize();
   for (uint i=0; i<numTasks; i++)
   {
      if (mPotentialTasks[i].getType() != AIGroupTaskType::cAttack)
         continue;

      const BSquad* pSquad = gWorld->getSquad(mPotentialTasks[i].getTargetID());
      if (!pSquad)
         continue;

      BHandle hFont = gFontManager.getFontCourier10();
      gFontManager.setFont(hFont);
      BFixedString128 debugText;
      debugText.format("%f", mPotentialTasks[i].getScore());
      gFontManager.drawText(hFont, pSquad->getPosition(), debugText, playerColor);
   }
}
#endif


//==============================================================================
//==============================================================================
BProtoAction* BAIGroup::getProtoActionForTarget(BEntityID targetID, BVector targetLoc, BAbilityID abilityID) const
{
   uint numSquads = mSquads.getSize();
   for (uint i=0; i<numSquads; i++)
   {
      const BSquad* pSquad = gWorld->getSquad(mSquads[i]);
      if (pSquad)
      {
         BProtoAction* pProtoAction = pSquad->getProtoActionForTarget(targetID, targetLoc, abilityID, false);
         if (pProtoAction)
            return (pProtoAction);
      }
   }

   return (NULL);
}


//==============================================================================
//==============================================================================
float BAIGroup::getDPSForTarget(BEntityID targetID, BVector targetLoc, BAbilityID abilityID) const
{
   float totalDPSForTarget = 0.0f;

   uint numSquads = mSquads.getSize();
   for (uint i=0; i<numSquads; i++)
   {
      const BSquad* pSquad = gWorld->getSquad(mSquads[i]);
      if (!pSquad)
         continue;
      const BProtoAction* pPA = pSquad->getProtoActionForTarget(targetID, targetLoc, abilityID, false);
      if (!pPA)
         continue;

      totalDPSForTarget += pPA->getDamagePerSecond();
   }

   return (totalDPSForTarget);
}


//==============================================================================
//==============================================================================
bool BAIGroup::canDoDamageAbility() const
{
   // If we have an invalid ability, or we're recovering, we can't do anything right now.
   if (mSelectionAbility.mAbilityID == cInvalidAbilityID)
      return (false);
   if (mSelectionAbility.mRecovering)
      return (false);

   // Add any supported 'standard' (i.e., simple damage ability on a cooldown w/no special considerations.)
   
   // Warthog ramming is broken right now.  Don't do it.
   if (mSelectionAbility.mAbilityID == gDatabase.getAIDUnscMarineRockets())
      return (true);
   if (mSelectionAbility.mAbilityID == gDatabase.getAIDUnscWolverineBarrage())
      return (true);
   if (mSelectionAbility.mAbilityID == gDatabase.getAIDUnscBarrage())
      return (true);
   if (mSelectionAbility.mAbilityID == gDatabase.getAIDUnscHornetSpecial())
      return (true);
   if (mSelectionAbility.mAbilityID == gDatabase.getAIDUnscScorpionSpecial())
      return (true);
   if (mSelectionAbility.mAbilityID == gDatabase.getAIDCovGruntGrenade())
      return (true);
   if (mSelectionAbility.mAbilityID == gDatabase.getAIDUnscLockdown())
      return (true);
   if (mSelectionAbility.mAbilityID == gDatabase.getAIDUnscCyclopsThrow())
      return (true);
   if (mSelectionAbility.mAbilityID == gDatabase.getAIDCovWraithSpecial())
      return (true);
   if (mSelectionAbility.mAbilityID == gDatabase.getAIDUnscGremlinSpecial())
      return (true);

   // Nope.
   return (false);
}


//==============================================================================
//==============================================================================
bool BAIGroup::canDoSuicideAbility() const
{
   // If we have an invalid ability, or we're recovering, we can't do anything right now.
   if (mSelectionAbility.mAbilityID == cInvalidAbilityID)
      return (false);
   if (mSelectionAbility.mRecovering)
      return (false);

   if (mSelectionAbility.mAbilityID == gDatabase.getAIDCovGruntSuicideExplode())
      return (true);
   // Nope.
   return (false);
}


//==============================================================================
//==============================================================================
bool BAIGroup::canDoOverburnAbility() const
{
   // If we have an invalid ability, or we're recovering, we can't do anything right now.
   if (mSelectionAbility.mAbilityID == cInvalidAbilityID)
      return (false);
   if (mSelectionAbility.mRecovering)
      return (false);
   if (mSelectionAbility.mAbilityID == gDatabase.getAIDCovLocustOverburn())
      return (true);
   // Nope.
   return (false);
}


//==============================================================================
//==============================================================================
bool BAIGroup::canDoFlashBangAbility() const
{
   // If we have an invalid ability, or we're recovering, we can't do anything right now.
   if (mSelectionAbility.mAbilityID == cInvalidAbilityID)
      return (false);
   if (mSelectionAbility.mRecovering)
      return (false);

   // Add any supported 'standard' (i.e., simple damage ability on a cooldown w/no special considerations.)

   // Warthog ramming is broken right now.  Don't do it.
   if (mSelectionAbility.mAbilityID == gDatabase.getAIDUnscFlashBang())
      return (true);

   // Nope.
   return (false);
}

//==============================================================================
//==============================================================================
bool BAIGroup::canDoStasisAbility() const
{
   // If we have an invalid ability, or we're recovering, we can't do anything right now.
   if (mSelectionAbility.mAbilityID == cInvalidAbilityID)
      return (false);
   if (mSelectionAbility.mRecovering)
      return (false);

   // Add any supported 'standard' (i.e., simple damage ability on a cooldown w/no special considerations.)

   // Warthog ramming is broken right now.  Don't do it.
   if (mSelectionAbility.mAbilityID == gDatabase.getAIDCovStasis())
      return (true);

   // Nope.
   return (false);
}


//==============================================================================
//==============================================================================
bool BAIGroup::canDoRamAbility() const
{
   // If we have an invalid ability, or we're recovering, we can't do anything right now.
   if (mSelectionAbility.mAbilityID == cInvalidAbilityID)
      return (false);
   if (mSelectionAbility.mRecovering)
      return (false);

   // Add any supported 'standard' (i.e., simple damage ability on a cooldown w/no special considerations.)

   // Warthog ramming is broken right now.  Don't do it.
   if (mSelectionAbility.mAbilityID == gDatabase.getAIDUnscRam())
      return (true);
   if (mSelectionAbility.mAbilityID == gDatabase.getAIDCovGhostRam())
      return (true);
   if (mSelectionAbility.mAbilityID == gDatabase.getAIDCovChopperRunOver())
      return (true);

   // Nope.
   return (false);
}


//==============================================================================
//==============================================================================
bool BAIGroup::canDoHijackAbility() const
{
   // If we have an invalid ability, or we're recovering, we can't do anything right now.
   if (mSelectionAbility.mAbilityID == cInvalidAbilityID)
      return (false);
   if (mSelectionAbility.mRecovering)
      return (false);

   // Supported abilities of this type.
   if (mSelectionAbility.mAbilityID == gDatabase.getAIDUnscSpartanTakeOver())
      return (true);

   // Nope.
   return (false);
}


//==============================================================================
//==============================================================================
bool BAIGroup::canDoJumppackAbility() const
{
   // If we have an invalid ability, or we're recovering, we can't do anything right now.
   if (mSelectionAbility.mAbilityID == cInvalidAbilityID)
      return (false);
   if (mSelectionAbility.mRecovering)
      return (false);

   // Supported abilities of this type.
   if (mSelectionAbility.mAbilityID == gDatabase.getAIDCovJumppack())
      return (true);

   // Nope.
   return (false);
}



//==============================================================================
//==============================================================================
bool BAIGroup::canDoCloakAbility() const
{
   // If we have an invalid ability, or we're recovering, we can't do anything right now.
   if (mSelectionAbility.mAbilityID == cInvalidAbilityID)
      return (false);
   if (mSelectionAbility.mRecovering)
      return (false);
   if (mSelectionAbility.mAbilityID == gDatabase.getAIDCovCloak())
      return (true);
   if (mSelectionAbility.mAbilityID == gDatabase.getAIDCovArbCloak())
      return (true);
   // Nope.
   return (false);
}


//==============================================================================
//==============================================================================
//bool BAIGroup::demoHackIsAHumanWatching() const
//{
//   BVector groupPos = mPosition;
//   long numPlayers = gWorld->getNumberPlayers();
//   for (long i=0; i<numPlayers; i++)
//   {
//      const BPlayer* pPlayer = gWorld->getPlayer(i);
//      if (!pPlayer || !pPlayer->isHuman())
//         continue;
//      const BAI* pAI = pPlayer->getAI();
//      if (pAI && groupPos.xzDistanceSqr(pAI->getFocusPosition()) > 2500.0f)
//         continue;
//      BTeamID teamID = pPlayer->getTeamID();
//      for (uint j=0; j<mSquads.getSize(); j++)
//      {
//         const BSquad* pSquad = gWorld->getSquad(mSquads[i]);
//         if (pSquad && pSquad->isVisible(teamID))
//            return (true);
//      }
//   }
//
//   return (false);
//}


//==============================================================================
//==============================================================================
void BAIGroup::updateTasks(DWORD currentGameTime)
{
   BASSERTM(mTasks.getSize() <= 3, "BAIGroup::updateTasks() - Number of tasks for this group exceeds 3... That should not happen.");
   while (mTasks.getSize() > 0)
   {
      BAIGroupTaskID taskID = mTasks[0];

      // Task doesn't exist?  Get rid of it.
      const BAIGroupTask* pTask = gWorld->getAIGroupTask(taskID);
      if (!pTask)
      {
         gWorld->deleteAIGroupTask(taskID);
         continue;
      }

      // Passed the expiration date?  Get rid of it.
      if (pTask->getFlagValidUntil() && currentGameTime >= pTask->getTimestampValidUntil())
      {
         gWorld->deleteAIGroupTask(taskID);
         continue;
      }

      if (testGroupForTask(pTask->getType(), pTask->getAbilityID(), pTask->getTargetID(), pTask->getTargetPos(), 10.0f))
         return;

      gWorld->deleteAIGroupTask(taskID);
      continue;

      // INVALID TYPE!
      BFAIL("BAIGroup::updateTasks() - Error case!  Possible bad task type causing infinite loop.  Deleting task.");
      gWorld->deleteAIGroupTask(taskID);
   }

   // We have zero tasks... if we weren't waiting, make us start waiting.
   if (getState() != AIGroupState::cWaiting)
      setState(AIGroupState::cWaiting);
}


//==============================================================================
//==============================================================================
bool BAIGroup::testGroupForTask(BAIGroupTaskType type, BAbilityID abilityID, BEntityID targetID, BVector targetPos, float posEps) const
{
   const float posEpsilonSqr = posEps * posEps;

   uint numSquads = mSquads.getSize();
   for (uint i=0; i<numSquads; i++)
   {
      const BSquad* pSquad = gWorld->getSquad(mSquads[i]);
      if (!pSquad)
         continue;

      if (type == AIGroupTaskType::cMove)
      {
         long numActions = pSquad->getNumberActions();
         for (long actionIdx=0; actionIdx<numActions; actionIdx++)
         {
            const BAction* pAction = pSquad->getActionByIndexConst(actionIdx);
            if (!pAction || pAction->getType() != BAction::cActionTypeSquadMove)
               continue;
            const BSimTarget* pTarget = pAction->getTarget();
            if (!pTarget)
               continue;
            if (pTarget->getAbilityID() != abilityID)
               continue;
            if (pTarget->getPosition().xzDistanceSqr(targetPos) > posEpsilonSqr)
               continue;
            return (true);
         }
      }
      else if (type == AIGroupTaskType::cAttack)
      {
         long numActions = pSquad->getNumberActions();
         for (long actionIdx=0; actionIdx<numActions; actionIdx++)
         {
            const BAction* pAction = pSquad->getActionByIndexConst(actionIdx);
            if (!pAction || pAction->getType() != BAction::cActionTypeSquadAttack)
               continue;
            const BSimTarget* pTarget = pAction->getTarget();
            if (!pTarget)
               continue;
            if (pTarget->getID() != targetID)
               continue;
            if (pTarget->getAbilityID() != abilityID)
               continue;
            return (true);
         }
      }
      else if (type == AIGroupTaskType::cGather)
      {
         long numActions = pSquad->getNumberActions();
         for (long actionIdx=0; actionIdx<numActions; actionIdx++)
         {
            const BAction* pAction = pSquad->getActionByIndexConst(actionIdx);
            if (!pAction || pAction->getType() != BAction::cActionTypeSquadWork)
               continue;
            const BSimTarget* pTarget = pAction->getTarget();
            if (!pTarget)
               continue;
            if (pTarget->getID() != targetID)
               continue;
            if (pTarget->getAbilityID() != abilityID)
               continue;
            return (true);
         }
      }
      else if (type == AIGroupTaskType::cHijack)
      {
         long numActions = pSquad->getNumberActions();
         for (long actionIdx=0; actionIdx<numActions; actionIdx++)
         {
            const BAction* pAction = pSquad->getActionByIndexConst(actionIdx);
            if (!pAction || pAction->getType() != BAction::cActionTypeSquadWork)
               continue;
            const BSimTarget* pTarget = pAction->getTarget();
            if (!pTarget)
               continue;
            if (pTarget->getID() != targetID)
               continue;
            if (pTarget->getAbilityID() != abilityID)
               continue;
            return (true);
         }
      }
      else if (type == AIGroupTaskType::cRepairOther)
      {
         long numActions = pSquad->getNumberActions();
         for (long actionIdx=0; actionIdx<numActions; actionIdx++)
         {
            const BAction* pAction = pSquad->getActionByIndexConst(actionIdx);
            if (!pAction || pAction->getType() != BAction::cActionTypeSquadRepairOther)
               continue;
            const BSimTarget* pTarget = pAction->getTarget();
            if (!pTarget)
               continue;
            if (pTarget->getID() != targetID)
               continue;
            if (pTarget->getAbilityID() != abilityID)
               continue;
            return (true);
         }
      }
      else if (type == AIGroupTaskType::cDetonate)
      {
         long numActions = pSquad->getNumberActions();
         for (long actionIdx=0; actionIdx<numActions; actionIdx++)
         {
            const BAction* pAction = pSquad->getActionByIndexConst(actionIdx);
            if (!pAction || pAction->getType() != BAction::cActionTypeSquadDetonate)
               continue;
            const BSimTarget* pTarget = pAction->getTarget();
            if (!pTarget)
               continue;
            if (pTarget->getID() != targetID)
               continue;
            if (pTarget->getAbilityID() != abilityID)
               continue;
            return (true);
         }
      }
   }

   // No matches.
   return (false);
}


//==============================================================================
//==============================================================================
void BAIGroup::update(DWORD currentGameTime)
{
   recalculatePosition();
   recalculateSquadAnalysis();
   recalculateAttackRanges();
   updateTasks(currentGameTime);
   updateSelectionAbility();
   updateGroupFlags();
}


//==============================================================================
//==============================================================================
void BAIGroup::updateGroupFlags()
{
   bool canGather = false;
   bool canGatherSupplies = false;
   bool canAttack = false;
   bool canRepair = false;
   bool canAutoRepair = false;

   uint numSquads = mSquads.getSize();
   for (uint i=0; i<numSquads; i++)
   {
      const BSquad* pSquad = gWorld->getSquad(mSquads[i]);
      if (!pSquad)
         continue;
      const BUnit* pUnit = pSquad->getLeaderUnit();
      if (!pUnit)
         continue;
      const BTactic* pTactic = pUnit->getTactic();
      if (!pTactic)
         continue;
      if (pTactic->canGather())
         canGather = true;
      if (pTactic->canGatherSupplies())
         canGatherSupplies = true;
      if (pTactic->canAttack())
         canAttack = true;
      if (pTactic->canRepair())
         canRepair = true;
      if (pTactic->canAutoRepair())
         canAutoRepair = true;
      if (canGather && canAttack && canRepair && canAutoRepair)
         break;
   }

   mFlagCanGather = canGather;
   mFlagCanGatherSupplies = canGatherSupplies;
   mFlagCanAttack = canAttack;
   mFlagCanRepair = canRepair;
   mFlagCanAutoRepair = canAutoRepair;
}


//==============================================================================
//==============================================================================
void BAIGroup::updateSelectionAbility()
{
   BSimHelper::calculateSelectionAbility(mSquads, mPlayerID, mSelectionAbility);
   BSimHelper::updateSelectionAbility(mSquads, cInvalidObjectID, cInvalidVector, mSelectionAbility);
}


//==============================================================================
//==============================================================================
void BAIGroup::updateSelectionAbility(BEntityID hoverObject, BVector hoverPos)
{
   BSimHelper::calculateSelectionAbility(mSquads, mPlayerID, mSelectionAbility);
   BSimHelper::updateSelectionAbility(mSquads, hoverObject, hoverPos, mSelectionAbility);
}


//==============================================================================
//==============================================================================
BAIGroupTaskID BAIGroup::getCurrentTaskID() const
{
   if (mTasks.getSize() > 0)
      return (mTasks[0]);
   else
      return (cInvalidAIGroupTaskID);
}

//==============================================================================
//==============================================================================
const BAIGroupTask* BAIGroup::getCurrentTask() const
{
   if (mTasks.getSize() > 0)
      return (gWorld->getAIGroupTask(mTasks[0]));
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
bool BAIGroup::canAcceptSquad(BEntityID squadID) const
{
   // Cannot accept a squad beyond a certain size (5 for now.)
   //if (mSquads.getSize() >= 5)
   //   return (false);

   // Cannot accept a bogus squad.
   const BSquad* pSquad = gWorld->getSquad(squadID);
   if (!pSquad)
      return (false);

   // Cannot put a contained spartan in a group.
   if (pSquad->getFlagContainedSpartan())
      return (false);

   // Cannot accept a squad of a different type.
   BProtoSquadID protoSquadID = pSquad->getProtoSquadID();
   if (mProtoSquadID != protoSquadID)
      return (false);

   // The squad must be within the group radius to be added.
   if (mPosition.xzDistanceSqr(pSquad->getPosition()) > (mRadius * mRadius))
      return (false);

   // marchack distance check plz.

   return (true);
}


//==============================================================================
//==============================================================================
/*bool BAIGroup::canAcceptGroup(BAIGroupID groupID) const
{
   // We cannot merge ourself into...ourself.
   if (mID == groupID)
      return (false);

   // Crap group = no.
   const BAIGroup* pAIGroup = gWorld->getAIGroup(groupID);
   if (!pAIGroup)
      return (false);

   // If this is not our mission = no.
   BAIMissionID missionID = pAIGroup->getMissionID();
   if (mMissionID != missionID)
      return (false);

   // If the group is not in a valid mission = no.
   const BAIMission* pAIMission = gWorld->getAIMission(missionID);
   if (!pAIMission)
      return (false);

   // Wrong type = no.
   BProtoSquadID groupProtoSquadID = pAIGroup->getProtoSquadID();
   if (mProtoSquadID != groupProtoSquadID)
      return (false);

   // Empty group = no.
   if (pAIGroup->getNumSquads() == 0)
      return (false);

   // Too many squads = no.
   //uint currentNumSquads = getNumSquads();
   //uint additionalNumSquads = pAIGroup->getNumSquads();
   //uint potentialNumSquads = currentNumSquads + additionalNumSquads;
   //if (potentialNumSquads >= 5)
   //   return (false);

   // I guess we cannot accept them.
   return (false);
}*/


//==============================================================================
//==============================================================================
bool BAIGroup::canAcceptAdditionalSquads() const
{
   // Not if we're full.
   //if (mSquads.getSize() >= 5)
   //   return (false);

   // Yeah sure.
   return (true);
}

//==============================================================================
// BAIGroup::save
//==============================================================================
bool BAIGroup::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mPosition);
   GFWRITEVECTOR(pStream, mCentroid);
   GFWRITEVAR(pStream, float, mRadius);
   GFWRITEARRAY(pStream, BAIGroupTaskID, mTasks, uint16, 500);

   GFWRITEVAR(pStream, BAIGroupID, mID);
   GFWRITEVAR(pStream, BPlayerID, mPlayerID);
   GFWRITEVAR(pStream, BAIMissionID, mMissionID);
   GFWRITEVAR(pStream, BAIGroupState, mState);
   GFWRITEARRAY(pStream, BEntityID, mSquads, uint16, 500);
   GFWRITECLASS(pStream, saveType, mSquadsAnalysis);
   GFWRITEVAR(pStream, BAIGroupRole, mRole);
   GFWRITEVAR(pStream, DWORD, mTimestampCreated);
   GFWRITEVAR(pStream, DWORD, mTimestampLastEval);
   GFWRITEVAR(pStream, DWORD, mTimestampLastTask);
   GFWRITEVAR(pStream, DWORD, mTimestampSquadAdded);
   GFWRITEVAR(pStream, float, mMaxAttackRange);
   GFWRITEVAR(pStream, float, mMaxVelocity);
   GFWRITEVAR(pStream, BProtoSquadID, mProtoSquadID);
   GFWRITECLASS(pStream, saveType, mSelectionAbility);

   GFWRITEBITBOOL(pStream, mFlagTasked);
   GFWRITEBITBOOL(pStream, mFlagTaskedThisState);
   GFWRITEBITBOOL(pStream, mFlagCanGather);
   GFWRITEBITBOOL(pStream, mFlagCanGatherSupplies);
   GFWRITEBITBOOL(pStream, mFlagCanAttack);
   GFWRITEBITBOOL(pStream, mFlagCanRepair);
   GFWRITEBITBOOL(pStream, mFlagCanAutoRepair);
   return true;
}

//==============================================================================
// BAIGroup::load
//==============================================================================
bool BAIGroup::load(BStream* pStream, int saveType)
{  
   GFREADVECTOR(pStream, mPosition);
   GFREADVECTOR(pStream, mCentroid);
   GFREADVAR(pStream, float, mRadius);
   GFREADARRAY(pStream, BAIGroupTaskID, mTasks, uint16, 500);

   GFREADVAR(pStream, BAIGroupID, mID);
   GFREADVAR(pStream, BPlayerID, mPlayerID);
   GFREADVAR(pStream, BAIMissionID, mMissionID);
   GFREADVAR(pStream, BAIGroupState, mState);
   GFREADARRAY(pStream, BEntityID, mSquads, uint16, 500);
   GFREADCLASS(pStream, saveType, mSquadsAnalysis);
   GFREADVAR(pStream, BAIGroupRole, mRole);
   GFREADVAR(pStream, DWORD, mTimestampCreated);
   GFREADVAR(pStream, DWORD, mTimestampLastEval);
   GFREADVAR(pStream, DWORD, mTimestampLastTask);
   GFREADVAR(pStream, DWORD, mTimestampSquadAdded);
   GFREADVAR(pStream, float, mMaxAttackRange);
   GFREADVAR(pStream, float, mMaxVelocity);
   GFREADVAR(pStream, BProtoSquadID, mProtoSquadID);
   GFREADCLASS(pStream, saveType, mSelectionAbility);

   GFREADBITBOOL(pStream, mFlagTasked);
   GFREADBITBOOL(pStream, mFlagTaskedThisState);
   GFREADBITBOOL(pStream, mFlagCanGather);

   mFlagCanGatherSupplies = false;
   if (BAIGroup::mGameFileVersion >= 3)
   {
      GFREADBITBOOL(pStream, mFlagCanGatherSupplies);
   }

   mFlagCanAttack = false;
   mFlagCanRepair = false;
   mFlagCanAutoRepair = false;
   if (BAIGroup::mGameFileVersion >= 2)
   {
      GFREADBITBOOL(pStream, mFlagCanAttack);
      GFREADBITBOOL(pStream, mFlagCanRepair);
      GFREADBITBOOL(pStream, mFlagCanAutoRepair);
   }
   
   gSaveGame.remapProtoSquadID(mProtoSquadID);

   return true;
}

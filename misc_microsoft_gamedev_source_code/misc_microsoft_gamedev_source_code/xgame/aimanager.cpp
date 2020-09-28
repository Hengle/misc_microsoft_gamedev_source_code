//==============================================================================
// aimanager.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "ai.h"
#include "aimanager.h"
#include "aiteleporterzone.h"
#include "bidmgr.h"
#include "kb.h"
#include "team.h"
#include "world.h"

GFIMPLEMENTVERSION(BAIManager, 3);

enum
{
   cSaveMarkerAI=10000,
   cSaveMarkerKB,
   cSaveMarkerAIMissionTargetWrapper,
   cSaveMarkerAIGroup,
   cSaveMarkerAIGroupTask,
   cSaveMarkerAIMissionTarget,
   cSaveMarkerAITopic,
   cSaveMarkerAIPowerMission,
   cSaveMarkerAIMission,
   cSaveMarkerKBSquad,
   cSaveMarkerKBBase,
   cSaveMarkerBid,
   cSaveMarkerAIManager,
   cSaveMarkerTeleporterZone,
   cSaveMarkerTeleporterZoneIDs,
};


//==============================================================================
//==============================================================================
BAIManager::BAIManager()
{
   for (uint i=0; i<cMaximumSupportedPlayers; i++)
      mAIs[i] = NULL;
   for (uint i=0; i<cMaximumSupportedTeams; i++)
      mKBs[i] = NULL;

   // All using the sim heap.
   mWrappers.setHeap(&gSimHeap);
   mGroups.setHeap(&gSimHeap);
   mGroupTasks.setHeap(&gSimHeap);
   mMissionTargets.setHeap(&gSimHeap);
   mTopics.setHeap(&gSimHeap);
   mPowerMissions.setHeap(&gSimHeap);
   mMissions.setHeap(&gSimHeap);
   mKBSquads.setHeap(&gSimHeap);
   mKBBases.setHeap(&gSimHeap);
   mBids.setHeap(&gSimHeap);


#ifndef BUILD_FINAL
   mNextWrapperDebugID = 0;
   mNextGroupDebugID = 0;
   mNextGroupTaskDebugID = 0;
   mNextMissionTargetDebugID = 0;
   mNextTopicDebugID = 0;
   mNextMissionDebugID = 0;
   mNextBidDebugID = 0;
   mNextZoneDebugID = 0;
   mNextKBSquadDebugID = 0;
   mNextKBBaseDebugID = 0;
#endif
}


//==============================================================================
//==============================================================================
BAIManager::~BAIManager()
{
   for (uint i=0; i<cMaximumSupportedPlayers; i++)
   {
      if (mAIs[i])
      {
         HEAP_DELETE(mAIs[i], gSimHeap);
         mAIs[i] = NULL;
      }
   }

   for (uint i=0; i<cMaximumSupportedTeams; i++)
   {
      if (mKBs[i])
      {
         HEAP_DELETE(mKBs[i], gSimHeap);
         mKBs[i] = NULL;
      }
   }
}


//==============================================================================
//==============================================================================
BAI* BAIManager::createAI(BPlayerID playerID, BTeamID teamID)
{
   if ((playerID < 0) || (playerID >= cMaximumSupportedPlayers))
      return (NULL);
   if ((teamID < 0) || (teamID >= cMaximumSupportedTeams))
      return (NULL);
   
   BASSERT(!mAIs[playerID]);
   if (mAIs[playerID])
   {
      HEAP_DELETE(mAIs[playerID], gSimHeap);
      mAIs[playerID] = NULL;
   }

   mAIs[playerID] = new(gSimHeap) BAI(playerID, teamID);
   return (mAIs[playerID]);
}


//==============================================================================
//==============================================================================
BAI* BAIManager::getAI(BPlayerID playerID)
{
   if ((playerID < 0) || (playerID >= cMaximumSupportedPlayers))
      return (NULL);
   return (mAIs[playerID]);
}


//==============================================================================
//==============================================================================
const BAI* BAIManager::getAI(BPlayerID playerID) const
{
   if ((playerID < 0) || (playerID >= cMaximumSupportedPlayers))
      return (NULL);
   return (mAIs[playerID]);
}


//==============================================================================
//==============================================================================
void BAIManager::deleteAI(BPlayerID playerID)
{
   if ((playerID >= 0) && (playerID < cMaximumSupportedPlayers) && mAIs[playerID])
   {
      HEAP_DELETE(mAIs[playerID], gSimHeap);
      mAIs[playerID] = NULL;

      // Possibly kill his team's KB also.
      const BPlayer* pKilledAIPlayer = gWorld->getPlayer(playerID);
      if (!pKilledAIPlayer)
         return;

      BTeamID killedAIPlayerTeamID = pKilledAIPlayer->getTeamID();
      bool bAITeammatesStillPlaying = false;
      for (uint i=0; i<cMaximumSupportedPlayers; i++)
      {
         const BPlayer* pPlayer = gWorld->getPlayer(i);
         if (!pPlayer)
            continue;
         if (pPlayer->getTeamID() != killedAIPlayerTeamID)
            continue;
         if (pPlayer->getPlayerState() != BPlayer::cPlayerStatePlaying)
            continue;
         if (!this->getAI(i))
            continue;
         bAITeammatesStillPlaying = true;
         break;
      }

      if (!bAITeammatesStillPlaying)
         this->deleteKB(killedAIPlayerTeamID);
   }
}


//==============================================================================
//==============================================================================
BKB* BAIManager::createKB(BTeamID teamID)
{
   if ((teamID < 0) || (teamID >= cMaximumSupportedTeams))
      return (NULL);

   BASSERT(!mKBs[teamID]);
   if (mKBs[teamID])
   {
      HEAP_DELETE(mKBs[teamID], gSimHeap);
      mKBs[teamID] = NULL;
   }

   mKBs[teamID] = new(gSimHeap) BKB(teamID);
   return (mKBs[teamID]);
}


//==============================================================================
//==============================================================================
BKB* BAIManager::getKB(BTeamID teamID)
{
   if ((teamID < 0) || (teamID >= cMaximumSupportedTeams))
      return (NULL);
   return (mKBs[teamID]);
}


//==============================================================================
//==============================================================================
const BKB* BAIManager::getKB(BTeamID teamID) const
{
   if ((teamID < 0) || (teamID >= cMaximumSupportedTeams))
      return (NULL);
   return (mKBs[teamID]);
}


//==============================================================================
//==============================================================================
void BAIManager::deleteKB(BTeamID teamID)
{
   if ((teamID >= 0) && (teamID < cMaximumSupportedTeams) && mKBs[teamID])
   {
      HEAP_DELETE(mKBs[teamID], gSimHeap);
      mKBs[teamID] = NULL;
   }
}


//==============================================================================
//==============================================================================
BAIMissionTargetWrapper* BAIManager::createWrapper(BAIMissionID missionID, BAIMissionTargetID targetID)
{
   BAIMission* pMission = gWorld->getAIMission(missionID);
   if (!pMission)
      return (NULL);

   uint newIndex = BAIMissionTargetWrapperID::cMaxIndex;
   BAIMissionTargetWrapper* pWrapper = mWrappers.acquire(newIndex, true);  // fatal on alloc failure, so don't check return.
   BASSERT(newIndex < BAIMissionTargetWrapperID::cMaxIndex);
   uint refCount = pWrapper->getID().getRefCount();

   pWrapper->setID(refCount, newIndex);
   pWrapper->setMissionID(missionID);
   pWrapper->setTargetID(targetID);
   pMission->addWrapperID(pWrapper->getID());

   return (pWrapper);
}


//==============================================================================
//==============================================================================
BAIMissionTargetWrapper* BAIManager::getWrapper(BAIMissionTargetWrapperID id)
{
   // Look at the index and make sure that's ok.
   uint index = id.getIndex();
   if (index >= mWrappers.getHighWaterMark() || !mWrappers.isInUse(index))
      return (NULL);

   BAIMissionTargetWrapper* pWrapper = mWrappers.get(index);
   if (pWrapper->getID() == id)
      return (pWrapper);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
const BAIMissionTargetWrapper* BAIManager::getWrapper(BAIMissionTargetWrapperID id) const
{
   // Look at the index and make sure that's ok.
   uint index = id.getIndex();
   if (index >= mWrappers.getHighWaterMark() || !mWrappers.isInUse(index))
      return (NULL);

   const BAIMissionTargetWrapper* pWrapper = mWrappers.get(index);
   if (pWrapper->getID() == id)
      return (pWrapper);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
void BAIManager::deleteWrapper(BAIMissionTargetWrapperID id)
{
   BAIMissionTargetWrapper* pWrapper = getWrapper(id);
   BASSERT(pWrapper);
   if (pWrapper)
   {
      BAIMission* pMission = gWorld->getAIMission(pWrapper->getMissionID());
      BASSERT(pMission);
      if (pMission)
      {
         BASSERT(pMission->containsWrapperID(id));
         pMission->removeWrapperID(id);
      }

      pWrapper->setTargetID(cInvalidAIMissionTargetID);
      pWrapper->setMissionID(cInvalidAIMissionID);
      pWrapper->resetNonIDData();
      pWrapper->getID().bumpRefCount();
      mWrappers.release(pWrapper);
   }
}



//==============================================================================
//==============================================================================
BAIGroup* BAIManager::createAIGroup(BAIMissionID missionID, BProtoSquadID protoSquadID)
{
   BAIMission* pMission = gWorld->getAIMission(missionID);
   if (!pMission)
      return (NULL);
   BPlayerID playerID = pMission->getPlayerID();
   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return (NULL);
   BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(protoSquadID);
   if (!pProtoSquad)
      return (NULL);

   uint newGroupIndex = BAIGroupID::cMaxIndex;
   BAIGroup* pNewGroup = mGroups.acquire(newGroupIndex, true);  // fatal on alloc failure, so don't check return.
   BASSERT(newGroupIndex < BAIGroupID::cMaxIndex);
   uint refCount = pNewGroup->getID().getRefCount();

#ifndef BUILD_FINAL
   pNewGroup->mDebugID = mNextGroupDebugID;
   mNextGroupDebugID++;
#endif

   pNewGroup->setID(refCount, newGroupIndex);
   pNewGroup->setTimestampCreated(gWorld->getGametime());
   pNewGroup->setProtoSquadID(protoSquadID);
   pNewGroup->setPlayerID(playerID);
   pNewGroup->setMissionID(missionID);
   pNewGroup->setState(AIGroupState::cWaiting);
   pNewGroup->setRole(AIGroupRole::cNormal);
   pMission->addGroupID(pNewGroup->getID());

   return (pNewGroup);
}


//==============================================================================
//==============================================================================
BAIGroup* BAIManager::getAIGroup(BAIGroupID groupID)
{
   // Look at the index and make sure that's ok.
   uint index = groupID.getIndex();
   if (index >= mGroups.getHighWaterMark() || !mGroups.isInUse(index))
      return (NULL);

   // Make sure the refcount matches.
   BAIGroup* pGroup = mGroups.get(index);
   if (pGroup->getID() == groupID)
      return (pGroup);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
const BAIGroup* BAIManager::getAIGroup(BAIGroupID groupID) const
{
   // Look at the index and make sure that's ok.
   uint index = groupID.getIndex();
   if (index >= mGroups.getHighWaterMark() || !mGroups.isInUse(index))
      return (NULL);

   // Make sure the refcount matches.
   const BAIGroup* pGroup = mGroups.get(index);
   if (pGroup->getID() == groupID)
      return (pGroup);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
void BAIManager::deleteAIGroup(BAIGroupID groupID)
{
   BAIGroup* pGroup = getAIGroup(groupID);
   BASSERT(pGroup);
   if (pGroup)
   {
      BAIMission* pMission = gWorld->getAIMission(pGroup->getMissionID());
      BASSERT(pMission);
      if (pMission)
      {
         BASSERT(pMission->containsGroupID(groupID));
         pMission->removeGroupID(groupID);
      }

      pGroup->deleteAllTasks();
      pGroup->resetNonIDData();
      pGroup->getID().bumpRefCount();
      mGroups.release(pGroup);
   }
}


//==============================================================================
//==============================================================================
BAIGroupTask* BAIManager::createAIGroupTask(BAIGroupID groupID)
{
   BAIGroup* pGroup = gWorld->getAIGroup(groupID);
   if (!pGroup)
      return (NULL);

   uint newGroupTaskIndex = BAIGroupTaskID::cMaxIndex;
   BAIGroupTask* pNewGroupTask = mGroupTasks.acquire(newGroupTaskIndex, true);  // fatal on alloc failure, so don't check return.
   BASSERT(newGroupTaskIndex < BAIGroupTaskID::cMaxIndex);
   uint refCount = pNewGroupTask->getID().getRefCount();

#ifndef BUILD_FINAL
   pNewGroupTask->mDebugID = mNextGroupTaskDebugID;
   mNextGroupTaskDebugID++;
#endif

   pNewGroupTask->setID(refCount, newGroupTaskIndex);
   pNewGroupTask->setPlayerID(pGroup->getPlayerID());
   pNewGroupTask->setGroupID(pGroup->getID());
   BASSERT(pGroup->getNumTasks() == 0);
   pGroup->addTaskID(pNewGroupTask->getID());

   return (pNewGroupTask);
}


//==============================================================================
//==============================================================================
BAIGroupTask* BAIManager::getAIGroupTask(BAIGroupTaskID groupTaskID)
{
   // Look at the index and make sure that's ok.
   uint index = groupTaskID.getIndex();
   if (index >= mGroupTasks.getHighWaterMark() || !mGroupTasks.isInUse(index))
      return (NULL);

   // Make sure the refcount matches.
   BAIGroupTask* pGroupTask = mGroupTasks.get(index);
   if (pGroupTask->getID() == groupTaskID)
      return (pGroupTask);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
const BAIGroupTask* BAIManager::getAIGroupTask(BAIGroupTaskID groupTaskID) const
{
   // Look at the index and make sure that's ok.
   uint index = groupTaskID.getIndex();
   if (index >= mGroupTasks.getHighWaterMark() || !mGroupTasks.isInUse(index))
      return (NULL);

   // Make sure the refcount matches.
   const BAIGroupTask* pGroupTask = mGroupTasks.get(index);
   if (pGroupTask->getID() == groupTaskID)
      return (pGroupTask);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
void BAIManager::deleteAIGroupTask(BAIGroupTaskID groupTaskID)
{
   BAIGroupTask* pGroupTask = getAIGroupTask(groupTaskID);
   BASSERT(pGroupTask);
   if (pGroupTask)
   {
      BAIGroup* pGroup = gWorld->getAIGroup(pGroupTask->getGroupID());
      BASSERT(pGroup);
      if (pGroup)
      {
         BASSERT(pGroup->containsTaskID(groupTaskID));
         pGroup->removeTaskID(groupTaskID);
      }

      pGroupTask->resetNonIDData();
      pGroupTask->getID().bumpRefCount();
      mGroupTasks.release(pGroupTask);
   }
}


//==============================================================================
//==============================================================================
BAIMissionTarget* BAIManager::createAIMissionTarget(BPlayerID playerID)
{
   BAI* pAI = gWorld->getAI(playerID);
   if (!pAI)
      return (NULL);

   uint newTargetIndex = BAIMissionTargetID::cMaxIndex;
   BAIMissionTarget* pNewTarget = mMissionTargets.acquire(newTargetIndex, true);  // fatal on alloc failure, so don't check return.
   BASSERT(newTargetIndex < BAIMissionTargetID::cMaxIndex);
   uint refCount = pNewTarget->getID().getRefCount();

#ifndef BUILD_FINAL
   pNewTarget->mDebugID = mNextMissionTargetDebugID;
   mNextMissionTargetDebugID++;
#endif

   pNewTarget->setID(refCount, newTargetIndex);
   pNewTarget->setPlayerID(playerID);
   pAI->addTargetID(pNewTarget->getID());

   return (pNewTarget);
}


//==============================================================================
//==============================================================================
BAIMissionTarget* BAIManager::getAIMissionTarget(BAIMissionTargetID missionTargetID)
{
   // Look at the index and make sure that's ok.
   uint index = missionTargetID.getIndex();
   if (index >= mMissionTargets.getHighWaterMark() || !mMissionTargets.isInUse(index))
      return (NULL);

   // Make sure the refcount matches.
   BAIMissionTarget* pTarget = mMissionTargets.get(index);
   if (pTarget->getID() == missionTargetID)
      return (pTarget);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
const BAIMissionTarget* BAIManager::getAIMissionTarget(BAIMissionTargetID missionTargetID) const
{
   // Look at the index and make sure that's ok.
   uint index = missionTargetID.getIndex();
   if (index >= mMissionTargets.getHighWaterMark() || !mMissionTargets.isInUse(index))
      return (NULL);

   // Make sure the refcount matches.
   const BAIMissionTarget* pTarget = mMissionTargets.get(index);
   if (pTarget->getID() == missionTargetID)
      return (pTarget);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
void BAIManager::deleteAIMissionTarget(BAIMissionTargetID missionTargetID)
{
   BAIMissionTarget* pTarget = getAIMissionTarget(missionTargetID);
   BASSERT(pTarget);
   if (pTarget)
   {
      BAI* pAI = gWorld->getAI(pTarget->getPlayerID());
      BASSERT(pAI);
      if (pAI)
      {
         BASSERT(pAI->containsTargetID(missionTargetID));
         pAI->removeTargetID(missionTargetID);
      }

      pTarget->resetNonIDData();
      pTarget->getID().bumpRefCount();
      mMissionTargets.release(pTarget);
   }
}


//==============================================================================
//==============================================================================
BAITopic* BAIManager::createAITopic(BPlayerID playerID)
{
   BAI* pAI = gWorld->getAI(playerID);
   if (!pAI)
      return (NULL);

   uint newTopicIndex = BAITopicID::cMaxIndex;
   BAITopic* pNewTopic = mTopics.acquire(newTopicIndex, true);  // fatal on alloc failure, so don't check return.
   BASSERT(newTopicIndex < BAITopicID::cMaxIndex);
   uint refCount = pNewTopic->getID().getRefCount();

#ifndef BUILD_FINAL
   pNewTopic->mDebugID = mNextTopicDebugID;
   mNextTopicDebugID++;
#endif

   pNewTopic->setID(refCount, newTopicIndex);
   pNewTopic->setPlayerID(playerID);
   pAI->setFlagAIQ(true);
   pAI->addTopicID(pNewTopic->getID());

   return (pNewTopic);
}


//==============================================================================
//==============================================================================
BAITopic* BAIManager::getAITopic(BAITopicID topicID)
{
   // Look at the index and make sure that's ok.
   uint index = topicID.getIndex();
   if (index >= mTopics.getHighWaterMark() || !mTopics.isInUse(index))
      return (NULL);

   // Make sure the refcount matches.
   BAITopic* pTopic = mTopics.get(index);
   if (pTopic->getID() == topicID)
      return (pTopic);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
const BAITopic* BAIManager::getAITopic(BAITopicID topicID) const
{
   // Look at the index and make sure that's ok.
   uint index = topicID.getIndex();
   if (index >= mTopics.getHighWaterMark() || !mTopics.isInUse(index))
      return (NULL);

   // Make sure the refcount matches.
   const BAITopic* pTopic = mTopics.get(index);
   if (pTopic->getID() == topicID)
      return (pTopic);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
void BAIManager::deleteAITopic(BAITopicID topicID)
{
   BAITopic* pTopic = getAITopic(topicID);
   BASSERT(pTopic);
   if (pTopic)
   {
      BAI* pAI = gWorld->getAI(pTopic->getPlayerID());
      BASSERT(pAI);
      if (pAI)
      {
         BASSERT(pAI->containsTopicID(topicID));
         pAI->removeTopicID(topicID);
      }

      pTopic->resetNonIDData();
      pTopic->getID().bumpRefCount();
      mTopics.release(pTopic);
   }
}


//==============================================================================
//==============================================================================
BAIMission* BAIManager::createAIMission(BPlayerID playerID, BAIMissionType missionType)
{
   BAI* pAI = gWorld->getAI(playerID);
   if (!pAI)
      return (NULL);

   uint newMissionIndex = BAIMissionID::cMaxIndex;
   BAIMission* pNewMission = NULL;

   if (missionType == MissionType::cAttack || missionType == MissionType::cDefend)
   {
      pNewMission = mMissions.acquire(newMissionIndex, true);
      BASSERT(newMissionIndex < BAIMissionID::cMaxIndex);
   }
   else if (missionType == MissionType::cPower)
   {
      pNewMission = mPowerMissions.acquire(newMissionIndex, true);
      BASSERT(newMissionIndex < BAIMissionID::cMaxIndex);
   }
   else
   {
      //BASSERTM(false, "Trying to create an invalid mission type.");
      return (NULL);
   }

   uint refCount = pNewMission->getID().getRefCount();

#ifndef BUILD_FINAL
   pNewMission->mDebugID = mNextMissionDebugID;
   mNextMissionDebugID++;
#endif

   pNewMission->setID(refCount, missionType, newMissionIndex);
   pNewMission->setType(missionType);
   pNewMission->setPlayerID(playerID);
   pAI->addMissionID(pNewMission->getID());

   return (pNewMission);
}


//==============================================================================
//==============================================================================
BAIMission* BAIManager::getAIMission(BAIMissionID missionID)
{
   if (!missionID.isValid())
      return (NULL);

   // Do the easy ID & player checks.
   uint index = missionID.getIndex();
   BAIMission* pMission = NULL;

   BAIMissionType missionType = missionID.getType();
   if (missionType == MissionType::cAttack || missionType == MissionType::cDefend)
   {
      if (index >= mMissions.getHighWaterMark() || !mMissions.isInUse(index))
         return (NULL);
      pMission = mMissions.get(index);
      BASSERT(pMission);
   }
   else if (missionType == MissionType::cPower)
   {
      if (index >= mPowerMissions.getHighWaterMark() || !mPowerMissions.isInUse(index))
         return (NULL);
      pMission = mPowerMissions.get(index);
      BASSERT(pMission);
   }
   else
   {
      //BASSERTM(false, "Trying to get an invalid mission type.");
      return (NULL);
   }

   // Make sure the refcount matches.
   if (pMission->getID() == missionID)
      return (pMission);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
const BAIMission* BAIManager::getAIMission(BAIMissionID missionID) const
{
   if (!missionID.isValid())
      return (NULL);

   // Do the easy ID & player checks.
   uint index = missionID.getIndex();
   const BAIMission* pMission = NULL;

   BAIMissionType missionType = missionID.getType();
   if (missionType == MissionType::cAttack || missionType == MissionType::cDefend)
   {
      if (index >= mMissions.getHighWaterMark() || !mMissions.isInUse(index))
         return (NULL);
      pMission = mMissions.get(index);
      BASSERT(pMission);
   }
   else if (missionType == MissionType::cPower)
   {
      if (index >= mPowerMissions.getHighWaterMark() || !mPowerMissions.isInUse(index))
         return (NULL);
      pMission = mPowerMissions.get(index);
      BASSERT(pMission);
   }
   else
   {
      //BASSERTM(false, "Trying to get an invalid mission type.");
      return (NULL);
   }

   // Make sure the refcount matches.
   if (pMission->getID() == missionID)
      return (pMission);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
void BAIManager::deleteAIMission(BAIMissionID missionID)
{
   BAIMission* pMission = getAIMission(missionID);
   if (pMission)
   {
      BAI* pAI = gWorld->getAI(pMission->getPlayerID());
      BASSERT(pAI);
      if (pAI)
      {
         BASSERT(pAI->containsMissionID(missionID));
         pAI->removeMissionID(missionID);
         BAIGlobals* pAIGlobals = pAI->getAIGlobals();
         BASSERT(pAIGlobals);
         if (pAIGlobals)
         {
            pAIGlobals->removeOpportunityRequest(missionID);
            pAIGlobals->removeTerminalMission(missionID);
         }
      }

      uint index = missionID.getIndex();
      BAIMissionType missionType = missionID.getType();
      pMission->deleteAllGroups();

      if (pMission->getAITopicID() != cInvalidAITopicID)
      {
         deleteAITopic(pMission->getAITopicID());
         pMission->setAITopicID(cInvalidAITopicID);
      }

      pMission->destroyAllTargetWrappers();
      pMission->resetNonIDData();
      pMission->getID().bumpRefCount();

      if (missionType == MissionType::cAttack || missionType == MissionType::cDefend)
      {
         mMissions.release(index);
      }
      else if (missionType == MissionType::cPower)
      {
         mPowerMissions.release(index);
      }
      else
      {
         //BASSERTM(false, "Trying to delete an invalid mission type!");
      }
   }
}


//==============================================================================
//==============================================================================
BKBSquad* BAIManager::createKBSquad(BTeamID teamID)
{
   BKB* pKB = gWorld->getKB(teamID);
   if (!pKB)
      return (NULL);

   uint newIndex = BAITopicID::cMaxIndex;
   BKBSquad* pKBSquad = mKBSquads.acquire(newIndex, true);  // fatal on alloc failure, so don't check return.
   BASSERT(newIndex < BKBSquadID::cMaxIndex);
   uint refCount = pKBSquad->getID().getRefCount();

   BKBSquadID kbSquadID(refCount, newIndex);

   #ifndef BUILD_FINAL
   pKBSquad->mDebugID = mNextKBSquadDebugID;
   mNextKBSquadDebugID++;
   #endif

   pKBSquad->setID(kbSquadID);
   pKBSquad->setTeamID(teamID);
   BASSERT(!pKB->containsKBSquad(kbSquadID));
   pKB->addKBSquad(kbSquadID);

   return (pKBSquad);
}


//==============================================================================
//==============================================================================
BKBSquad* BAIManager::getKBSquad(BKBSquadID kbSquadID)
{
   // Look at the index and make sure that's ok.
   uint index = kbSquadID.getIndex();
   if (index >= mKBSquads.getHighWaterMark() || !mKBSquads.isInUse(index))
      return (NULL);

   // Make sure the refcount matches.
   BKBSquad* pKBSquad = mKBSquads.get(index);
   if (pKBSquad->getID() == kbSquadID)
      return (pKBSquad);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
const BKBSquad* BAIManager::getKBSquad(BKBSquadID kbSquadID) const
{
   // Look at the index and make sure that's ok.
   uint index = kbSquadID.getIndex();
   if (index >= mKBSquads.getHighWaterMark() || !mKBSquads.isInUse(index))
      return (NULL);

   // Make sure the refcount matches.
   const BKBSquad* pKBSquad = mKBSquads.get(index);
   if (pKBSquad->getID() == kbSquadID)
      return (pKBSquad);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
void BAIManager::deleteKBSquad(BKBSquadID kbSquadID)
{
   BKBSquad* pKBSquad = getKBSquad(kbSquadID);
   BASSERT(pKBSquad);
   if (pKBSquad)
   {
      // Remove the Squad entry from the base
      BKBBase* pKBBase = gWorld->getKBBase(pKBSquad->getKBBaseID());
      if (pKBBase)
         pKBBase->removeKBSquad(kbSquadID);

      // Get rid of the reference to this KBSquad on the actual Squad.
      BSquad* pSquad = gWorld->getSquad(pKBSquad->getSquadID());
      if (pSquad)
         pSquad->setKBSquadID(cInvalidKBSquadID, pKBSquad->getTeamID());

      // Remove the Squad entry from the kb player.
      BKB* pKB = gWorld->getKB(pKBSquad->getTeamID());
      BASSERT(pKB);
      if (pKB)
      {
         BKBPlayer* pKBPlayer = pKB->getKBPlayer(pKBSquad->getPlayerID());
         BASSERT(pKBPlayer);
         if (pKBPlayer)
         {
            BASSERT(pKBPlayer->containsKBSquad(kbSquadID));
            pKBPlayer->removeKBSquad(kbSquadID);
         }

         BASSERT(pKB->containsKBSquad(kbSquadID));
         pKB->removeKBSquad(kbSquadID);
      }

      pKBSquad->resetNonIDData();
      pKBSquad->getID().bumpRefCount();
      mKBSquads.release(pKBSquad);
   }
}


//==============================================================================
//==============================================================================
BKBBase* BAIManager::createKBBase(BTeamID teamID, BPlayerID playerID, BVector position, float radius)
{
   BKB* pKB = gWorld->getKB(teamID);
   BASSERT(pKB);
   if (!pKB)
      return (NULL);

   BKBPlayer* pKBPlayer = pKB->getKBPlayer(playerID);
   BASSERT(pKBPlayer);
   if (!pKBPlayer)
      return (NULL);

   // Allocate a new BKBBase from the free list.
   uint newKBBaseIndex = BKBBaseID::cMaxIndex;
   BKBBase* pNewKBBaseEntry = mKBBases.acquire(newKBBaseIndex, true); // fatal on allocation failure, so don't check for null.
   BASSERT(newKBBaseIndex < BKBBaseID::cMaxIndex);
   uint refCount = pNewKBBaseEntry->getID().getRefCount();

   #ifndef BUILD_FINAL
   pNewKBBaseEntry->mDebugID = mNextKBBaseDebugID;
   mNextKBBaseDebugID++;
   #endif

   pNewKBBaseEntry->setID(refCount, newKBBaseIndex);
   pNewKBBaseEntry->setTeamID(teamID);
   pNewKBBaseEntry->setPlayerID(playerID);
   pNewKBBaseEntry->setPosition(position);
   pNewKBBaseEntry->setRadius(radius);

   BKBBaseID baseID = pNewKBBaseEntry->getID();
   BASSERT(!pKB->containsKBBase(baseID));
   pKB->addKBBase(baseID);
   BASSERT(!pKBPlayer->containsKBBase(baseID));
   pKBPlayer->addKBBase(baseID);

   const BTeam* pTeam = gWorld->getTeam(teamID);
   if (pTeam)
   {
      long numPlayers = pTeam->getNumberPlayers();
      for (long i=0; i<numPlayers; i++)
      {
         BAI* pAI = gWorld->getAI(pTeam->getPlayerID(i));
         if (pAI)
            pAI->generateBaseMissionTarget(pNewKBBaseEntry);
      }
   }

   return (pNewKBBaseEntry);
}


//==============================================================================
//==============================================================================
BKBBase* BAIManager::getKBBase(BKBBaseID kbBaseID)
{
   // Look at the index and make sure that's ok.
   uint index = kbBaseID.getIndex();
   if (index >= mKBBases.getHighWaterMark() || !mKBBases.isInUse(index))
      return (NULL);

   // Make sure the refcount matches.
   BKBBase* pKBBase = mKBBases.get(index);
   if (pKBBase->getID() == kbBaseID)
      return (pKBBase);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
const BKBBase* BAIManager::getKBBase(BKBBaseID kbBaseID) const
{
   // Look at the index and make sure that's ok.
   uint index = kbBaseID.getIndex();
   if (index >= mKBBases.getHighWaterMark() || !mKBBases.isInUse(index))
      return (NULL);

   // Make sure the refcount matches.
   const BKBBase* pKBBase = mKBBases.get(index);
   if (pKBBase->getID() == kbBaseID)
      return (pKBBase);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
void BAIManager::deleteKBBase(BKBBaseID kbBaseID)
{
   BKBBase* pKBBase = gWorld->getKBBase(kbBaseID);
   BASSERT(pKBBase);
   if (!pKBBase)
      return;

   // Remove all the mission targets this base represents to those players who share this KB.
   BAIMissionTargetIDArray missionTargetIDs = pKBBase->getAIMissionTargetIDs();
   uint numMissionTargetIDs = missionTargetIDs.getSize();
   for (uint i=0; i<numMissionTargetIDs; i++)
   {
      BAIMissionTarget* pTarget = gWorld->getAIMissionTarget(missionTargetIDs[i]);
      if (!pTarget)
         continue;

      if (pTarget->getNumWrapperRefs() == 0)
      {
         gWorld->deleteAIMissionTarget(missionTargetIDs[i]);
      }
      else
      {
         pTarget->setPosition(pKBBase->getPosition());
         pTarget->setRadius(pKBBase->getRadius());
         pTarget->setKBBaseID(cInvalidKBBaseID);
         pTarget->setDestroyOnNoRefs(true);
      }
   }

   pKBBase->clearAIMissionTargetIDs();

   // Remove all the unit entries from the base.
   pKBBase->removeAllKBSquads();

   // Remove the base entry from the KB player.
   BKB* pKB = gWorld->getKB(pKBBase->getTeamID());
   BASSERT(pKB);
   if (pKB)
   {
      BKBPlayer* pKBPlayer = pKB->getKBPlayer(pKBBase->getPlayerID());
      BASSERT(pKBPlayer);
      if (pKBPlayer)
      {
         BASSERT(pKBPlayer->containsKBBase(kbBaseID));
         pKBPlayer->removeKBBase(kbBaseID);
      }
      BASSERT(pKB->containsKBBase(kbBaseID));
      pKB->removeKBBase(kbBaseID);
   }

   // Nuke it.
   pKBBase->resetNonIDData();
   pKBBase->getID().bumpRefCount();
   mKBBases.release(pKBBase);
}


//==============================================================================
//==============================================================================
BBid* BAIManager::createBid(BPlayerID playerID)
{
   BAI* pAI = gWorld->getAI(playerID);
   if (!pAI)
      return (NULL);
   BBidManager* pBidManager = pAI->getBidManager();
   if (!pBidManager)
      return (NULL);

   uint newBidIndex = BBidID::cMaxIndex;
   BBid* pNewBid = mBids.acquire(newBidIndex, true);  // fatal on alloc failure, so don't check return.
   BASSERT(newBidIndex < BBidID::cMaxIndex);
   uint refCount = pNewBid->getID().getRefCount();

   pNewBid->setID(refCount, newBidIndex);
   pNewBid->setPlayerID(playerID);
   pBidManager->addBid(pNewBid->getID());

#ifndef BUILD_FINAL
   pNewBid->mDebugID = mNextBidDebugID;
   mNextBidDebugID++;
#endif
   
   return (pNewBid);
}


//==============================================================================
//==============================================================================
BBid* BAIManager::getBid(BBidID bidID)
{
   // Look at the index and make sure that's ok.
   uint index = bidID.getIndex();
   if (index >= mBids.getHighWaterMark() || !mBids.isInUse(index))
      return (NULL);

   // Make sure the refcount matches.
   BBid* pBid = mBids.get(index);
   if (pBid->getID() == bidID)
      return (pBid);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
const BBid* BAIManager::getBid(BBidID bidID) const
{
   // Look at the index and make sure that's ok.
   uint index = bidID.getIndex();
   if (index >= mBids.getHighWaterMark() || !mBids.isInUse(index))
      return (NULL);

   // Make sure the refcount matches.
   const BBid* pBid = mBids.get(index);
   if (pBid->getID() == bidID)
      return (pBid);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
void BAIManager::deleteBid(BBidID bidID)
{
   BBid* pBid = gWorld->getBid(bidID);
   if (pBid)
   {
      BAI* pAI = gWorld->getAI(pBid->getPlayerID());
      BASSERT(pAI);
      if (pAI)
      {
         BBidManager* pBidManager = pAI->getBidManager();
         BASSERT(pBidManager);
         if (pBidManager)
         {
            BASSERT(pBidManager->containsBid(bidID));
            pBidManager->removeBid(bidID);
         }
      }
      pBid->resetNonIDData();
      pBid->getID().bumpRefCount();
      mBids.release(pBid);
   }
}


//==============================================================================
//==============================================================================
BAITeleporterZone* BAIManager::createAITeleporterZone(BEntityID unitID, BVector boxExtent1, BVector boxExtent2)
{
   const BUnit* pUnit = gWorld->getUnit(unitID);
   if (!pUnit)
   {
      BFAIL("Creating a teleporter zone with invalid unitID.");
      return (NULL);
   }
   bool isPickup = pUnit->isType(gDatabase.getOTIDTeleportPickup());
   bool isDropoff = pUnit->isType(gDatabase.getOTIDTeleportDropoff());
   if (!isPickup && !isDropoff)
   {
      BFAIL("Creating a teleporter zone with a non-teleporter unitID.");
      return (NULL);
   }
   if (boxExtent1.x == boxExtent2.x)
   {
      BFAIL("Creating a teleporter zone with zero x-thickness.");
      return (NULL);
   }
   if (boxExtent1.z == boxExtent2.z)
   {
      BFAIL("Creating a teleporter zone with zero z-thickness.");
      return (NULL);
   }

   uint newZoneIndex = BAITeleporterZoneID::cMaxIndex;
   BAITeleporterZone* pNewZone = mTeleporterZones.acquire(newZoneIndex, true); // fatal on alloc failure, so don't check return.
   BASSERT(newZoneIndex < BAITeleporterZoneID::cMaxIndex);
   uint refCount = pNewZone->getID().getRefCount();

#ifndef BUILD_FINAL
   pNewZone->mDebugID = mNextZoneDebugID;
   mNextZoneDebugID++;
#endif

   BAITeleporterZoneID newZoneID(refCount, AITeleporterZoneType::cAABB, newZoneIndex);
   pNewZone->setID(newZoneID);
   pNewZone->setUnitID(unitID);
   pNewZone->setExtents(boxExtent1, boxExtent2);
   mTeleporterZoneIDs.add(newZoneID);
   if (isPickup)
      mTeleporterPickupZoneIDs.add(newZoneID);
   if (isDropoff)
      mTeleporterDropoffZoneIDs.add(newZoneID);
   return (pNewZone);
}


//==============================================================================
//==============================================================================
BAITeleporterZone* BAIManager::createAITeleporterZone(BEntityID unitID, BVector spherePos, float sphereRad)
{
   const BUnit* pUnit = gWorld->getUnit(unitID);
   if (!pUnit)
   {
      BFAIL("Creating a teleporter zone with invalid unitID.");
      return (NULL);
   }
   bool isPickup = pUnit->isType(gDatabase.getOTIDTeleportPickup());
   bool isDropoff = pUnit->isType(gDatabase.getOTIDTeleportDropoff());
   if (!isPickup && !isDropoff)
   {
      BFAIL("Creating a teleporter zone with a non-teleporter unitID.");
      return (NULL);
   }
   if (sphereRad <= 0.0f)
   {
      BFAIL("Creating a teleporter zone with zero radius.");
      return (NULL);
   }

   uint newZoneIndex = BAITeleporterZoneID::cMaxIndex;
   BAITeleporterZone* pNewZone = mTeleporterZones.acquire(newZoneIndex, true); // fatal on alloc failure, so don't check return.
   BASSERT(newZoneIndex < BAITeleporterZoneID::cMaxIndex);
   uint refCount = pNewZone->getID().getRefCount();

#ifndef BUILD_FINAL
   pNewZone->mDebugID = mNextZoneDebugID;
   mNextZoneDebugID++;
#endif

   BAITeleporterZoneID newZoneID(refCount, AITeleporterZoneType::cSphere, newZoneIndex);
   pNewZone->setID(newZoneID);
   pNewZone->setUnitID(unitID);
   pNewZone->setPointRadius(spherePos, sphereRad);
   mTeleporterZoneIDs.add(newZoneID);
   if (isPickup)
      mTeleporterPickupZoneIDs.add(newZoneID);
   if (isDropoff)
      mTeleporterDropoffZoneIDs.add(newZoneID);
   return (pNewZone);
}


//==============================================================================
//==============================================================================
BAITeleporterZone* BAIManager::getAITeleporterZone(BAITeleporterZoneID zoneID)
{
   // Look at the index and make sure that's ok.
   uint index = zoneID.getIndex();
   if (index >= mTeleporterZones.getHighWaterMark() || !mTeleporterZones.isInUse(index))
      return (NULL);

   // Make sure the refcount matches.
   BAITeleporterZone* pZone = mTeleporterZones.get(index);
   if (pZone->getID() == zoneID)
      return (pZone);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
const BAITeleporterZone* BAIManager::getAITeleporterZone(BAITeleporterZoneID zoneID) const
{
   // Look at the index and make sure that's ok.
   uint index = zoneID.getIndex();
   if (index >= mTeleporterZones.getHighWaterMark() || !mTeleporterZones.isInUse(index))
      return (NULL);

   // Make sure the refcount matches.
   const BAITeleporterZone* pZone = mTeleporterZones.get(index);
   if (pZone->getID() == zoneID)
      return (pZone);
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
void BAIManager::deleteAITeleporterZone(BAITeleporterZoneID zoneID)
{
   BAITeleporterZone* pZone = gWorld->getAITeleporterZone(zoneID);
   if (pZone)
   {
      pZone->resetNonIDData();
      pZone->getID().bumpRefCount();
      mTeleporterZones.release(pZone);
      mTeleporterZoneIDs.remove(zoneID);
      mTeleporterPickupZoneIDs.remove(zoneID);
      mTeleporterDropoffZoneIDs.remove(zoneID);
   }
}


//==============================================================================
//==============================================================================
void BAIManager::updateAIs()
{
   for (uint i=0; i<cMaximumSupportedPlayers; i++)
   {
      if (mAIs[i])
      {
         const BPlayer* pPlayer = gWorld->getPlayer(i);
         if (pPlayer && pPlayer->isPlaying())
            mAIs[i]->update();
         else
            deleteAI(i);
      }
   }
}


//==============================================================================
//==============================================================================
void BAIManager::updateAIMissions()
{
   DWORD currentGameTime = gWorld->getGametime();

   // Normal Missions
   uint highWaterMark = mMissions.getHighWaterMark();
   for (uint i=0; i<highWaterMark; i++)
   {
      if (mMissions.isInUse(i))
         mMissions[i].update(currentGameTime);
   }

   // Power Missions
   highWaterMark = mPowerMissions.getHighWaterMark();
   for (uint i=0; i<highWaterMark; i++)
   {
      if (mPowerMissions.isInUse(i))
         mPowerMissions[i].update(currentGameTime);
   }
}


//==============================================================================
//==============================================================================
void BAIManager::updateKBs()
{
   BKB* pKB = mKBs[gWorld->getUpdateNumber() % cMaximumSupportedTeams];
   if (pKB)
      pKB->update();   

   //for (uint i=0; i<cMaximumSupportedTeams; i++)
   //{
   //   if (mKBs[i])
   //      mKBs[i]->update();
   //}
}


//==============================================================================
//==============================================================================
void BAIManager::updateAllKBsForSquad(BSquad* pSquad)
{
   for (uint i=0; i<cMaximumSupportedTeams; i++)
   {
      if (mKBs[i])
         mKBs[i]->updateSquad(pSquad);
   }
}

//==============================================================================
// BAIManager::save
//==============================================================================
bool BAIManager::save(BStream* pStream, int saveType) const
{
   int i=0;
   for (i=0; i<cMaximumSupportedPlayers; i++)
   {
      GFWRITEVAL(pStream, bool, mAIs[i]!=NULL);
      if (mAIs[i])
         GFWRITECLASSPTR(pStream, saveType, mAIs[i]);
   }
   GFWRITEMARKER(pStream, cSaveMarkerAI);

   for (i=0; i<cMaximumSupportedTeams; i++)
   {
      GFWRITEVAL(pStream, bool, mKBs[i]!=NULL);
      if (mKBs[i])
         GFWRITECLASSPTR(pStream, saveType, mKBs[i]);
   }
   GFWRITEMARKER(pStream, cSaveMarkerKB);

   GFWRITEFREELIST(pStream, saveType, BAIMissionTargetWrapper, mWrappers, uint16, 1000);
   GFWRITEMARKER(pStream, cSaveMarkerAIMissionTargetWrapper);

   GFWRITEFREELIST(pStream, saveType, BAIGroup, mGroups, uint16, 1000);
   GFWRITEMARKER(pStream, cSaveMarkerAIGroup);

   GFWRITEFREELIST(pStream, saveType, BAIGroupTask, mGroupTasks, uint16, 1000);
   GFWRITEMARKER(pStream, cSaveMarkerAIGroupTask);

   GFWRITEFREELIST(pStream, saveType, BAIMissionTarget, mMissionTargets, uint16, 1000);
   GFWRITEMARKER(pStream, cSaveMarkerAIMissionTarget);

   GFWRITEFREELIST(pStream, saveType, BAITopic, mTopics, uint16, 1000);
   GFWRITEMARKER(pStream, cSaveMarkerAITopic);

   GFWRITEFREELIST(pStream, saveType, BAIPowerMission, mPowerMissions, uint16, 1000);
   GFWRITEMARKER(pStream, cSaveMarkerAIPowerMission);

   GFWRITEFREELIST(pStream, saveType, BAIMission, mMissions, uint16, 1000);
   GFWRITEMARKER(pStream, cSaveMarkerAIMission);

   GFWRITEFREELIST(pStream, saveType, BKBSquad, mKBSquads, uint16, 10000);
   GFWRITEMARKER(pStream, cSaveMarkerKBSquad);

   GFWRITEFREELIST(pStream, saveType, BKBBase, mKBBases, uint16, 1000);
   GFWRITEMARKER(pStream, cSaveMarkerKBBase);

   GFWRITEFREELIST(pStream, saveType, BBid, mBids, uint16, 1000);
   GFWRITEMARKER(pStream, cSaveMarkerBid);

   GFWRITEFREELIST(pStream, saveType, BAITeleporterZone, mTeleporterZones, uint16, 1000);
   GFWRITEMARKER(pStream, cSaveMarkerTeleporterZone);

   GFWRITEARRAY(pStream, BAITeleporterZoneID, mTeleporterZoneIDs, uint16, 1000);
   GFWRITEARRAY(pStream, BAITeleporterZoneID, mTeleporterPickupZoneIDs, uint16, 1000);
   GFWRITEARRAY(pStream, BAITeleporterZoneID, mTeleporterDropoffZoneIDs, uint16, 1000);
   GFWRITEMARKER(pStream, cSaveMarkerTeleporterZoneIDs);

#ifndef BUILD_FINAL
   GFWRITEVAR(pStream, uint, mNextWrapperDebugID);
   GFWRITEVAR(pStream, uint, mNextGroupDebugID);
   GFWRITEVAR(pStream, uint, mNextGroupTaskDebugID);
   GFWRITEVAR(pStream, uint, mNextMissionTargetDebugID);
   GFWRITEVAR(pStream, uint, mNextTopicDebugID);
   GFWRITEVAR(pStream, uint, mNextMissionDebugID);
   GFWRITEVAR(pStream, uint, mNextBidDebugID);
   GFWRITEVAR(pStream, uint, mNextZoneDebugID);
   GFWRITEVAR(pStream, uint, mNextKBSquadDebugID);
   GFWRITEVAR(pStream, uint, mNextKBBaseDebugID);
#else
   GFWRITEVAL(pStream, uint, 0);
   GFWRITEVAL(pStream, uint, 0);
   GFWRITEVAL(pStream, uint, 0);
   GFWRITEVAL(pStream, uint, 0);
   GFWRITEVAL(pStream, uint, 0);
   GFWRITEVAL(pStream, uint, 0);
   GFWRITEVAL(pStream, uint, 0);
   GFWRITEVAL(pStream, uint, 0);
   GFWRITEVAL(pStream, uint, 0);
   GFWRITEVAL(pStream, uint, 0);
#endif

   GFWRITEMARKER(pStream, cSaveMarkerAIManager);
   return true;
}

//==============================================================================
// BAIManager::load
//==============================================================================
bool BAIManager::load(BStream* pStream, int saveType)
{  
   int i=0;
   bool entry;
   for (i=0; i<cMaximumSupportedPlayers; i++)
   {
      GFREADVAR(pStream, bool, entry);
      if (entry)
      {
         // If we have a save entry for an AI but one is not allocated, it is probably an older save game from when
         // All players had an AI regardless of if they needed it.  Allocate one to accomodate the save game if necessary.
         if (mAIs[i])
         {
            GFREADCLASSPTR(pStream, saveType, mAIs[i]);
         }
         else
         {
            createAI(i, gWorld->getPlayer(i)->getTeamID());
            GFREADCLASSPTR(pStream, saveType, mAIs[i]);
         }
      }
   }
   GFREADMARKER(pStream, cSaveMarkerAI);

   for (i=0; i<cMaximumSupportedTeams; i++)
   {
      GFREADVAR(pStream, bool, entry);
      if (entry)
      {
         // If we have a save entry for an KB but one is not allocated, it is probably an older save game from when
         // All teams had a KB regardless of if they needed it.  Allocate one to accomodate the save game if necessary.
         if (mKBs[i])
         {
            GFREADCLASSPTR(pStream, saveType, mKBs[i]);
         }
         else
         {
            createKB(i);
            GFREADCLASSPTR(pStream, saveType, mKBs[i]);
         }
      }
   }
   GFREADMARKER(pStream, cSaveMarkerKB);

   GFREADFREELIST(pStream, saveType, BAIMissionTargetWrapper, mWrappers, uint16, 1000);
   GFREADMARKER(pStream, cSaveMarkerAIMissionTargetWrapper);

   GFREADFREELIST(pStream, saveType, BAIGroup, mGroups, uint16, 1000);
   GFREADMARKER(pStream, cSaveMarkerAIGroup);

   GFREADFREELIST(pStream, saveType, BAIGroupTask, mGroupTasks, uint16, 1000);
   GFREADMARKER(pStream, cSaveMarkerAIGroupTask);

   GFREADFREELIST(pStream, saveType, BAIMissionTarget, mMissionTargets, uint16, 1000);
   GFREADMARKER(pStream, cSaveMarkerAIMissionTarget);

   GFREADFREELIST(pStream, saveType, BAITopic, mTopics, uint16, 1000);
   GFREADMARKER(pStream, cSaveMarkerAITopic);

   GFREADFREELIST(pStream, saveType, BAIPowerMission, mPowerMissions, uint16, 1000);
   GFREADMARKER(pStream, cSaveMarkerAIPowerMission);

   GFREADFREELIST(pStream, saveType, BAIMission, mMissions, uint16, 1000);
   GFREADMARKER(pStream, cSaveMarkerAIMission);

   GFREADFREELIST(pStream, saveType, BKBSquad, mKBSquads, uint16, 10000);
   GFREADMARKER(pStream, cSaveMarkerKBSquad);

   GFREADFREELIST(pStream, saveType, BKBBase, mKBBases, uint16, 1000);
   GFREADMARKER(pStream, cSaveMarkerKBBase);

   GFREADFREELIST(pStream, saveType, BBid, mBids, uint16, 1000);
   GFREADMARKER(pStream, cSaveMarkerBid);

   GFREADFREELIST(pStream, saveType, BAITeleporterZone, mTeleporterZones, uint16, 1000);
   GFREADMARKER(pStream, cSaveMarkerTeleporterZone);

   GFREADARRAY(pStream, BAITeleporterZoneID, mTeleporterZoneIDs, uint16, 1000);
   if (BAIManager::mGameFileVersion < 3)
      pStream->skipBytes(mTeleporterZoneIDs.size() * 4);

   GFREADARRAY(pStream, BAITeleporterZoneID, mTeleporterPickupZoneIDs, uint16, 1000);
   if (BAIManager::mGameFileVersion < 3)
      pStream->skipBytes(mTeleporterPickupZoneIDs.size() * 4);

   GFREADARRAY(pStream, BAITeleporterZoneID, mTeleporterDropoffZoneIDs, uint16, 1000);
   if (BAIManager::mGameFileVersion < 3)
      pStream->skipBytes(mTeleporterDropoffZoneIDs.size() * 4);

   GFREADMARKER(pStream, cSaveMarkerTeleporterZoneIDs);

#ifndef BUILD_FINAL
   GFREADVAR(pStream, uint, mNextWrapperDebugID);
   GFREADVAR(pStream, uint, mNextGroupDebugID);
   GFREADVAR(pStream, uint, mNextGroupTaskDebugID);
   GFREADVAR(pStream, uint, mNextMissionTargetDebugID);
   GFREADVAR(pStream, uint, mNextTopicDebugID);
   GFREADVAR(pStream, uint, mNextMissionDebugID);
   GFREADVAR(pStream, uint, mNextBidDebugID);
   GFREADVAR(pStream, uint, mNextZoneDebugID);
   if (BAIManager::mGameFileVersion >= 2)
   {
      GFREADVAR(pStream, uint, mNextKBSquadDebugID);
      GFREADVAR(pStream, uint, mNextKBBaseDebugID);
   }
   else
   {
      mNextKBSquadDebugID = 0;
      mNextKBBaseDebugID = 0;
   }
#else
   uint temp;
   GFREADVAR(pStream, uint, temp);
   GFREADVAR(pStream, uint, temp);
   GFREADVAR(pStream, uint, temp);
   GFREADVAR(pStream, uint, temp);
   GFREADVAR(pStream, uint, temp);
   GFREADVAR(pStream, uint, temp);
   GFREADVAR(pStream, uint, temp);
   GFREADVAR(pStream, uint, temp);
   GFREADVAR(pStream, uint, temp);
   GFREADVAR(pStream, uint, temp);
#endif

   GFREADMARKER(pStream, cSaveMarkerAIManager);
   return true;
}


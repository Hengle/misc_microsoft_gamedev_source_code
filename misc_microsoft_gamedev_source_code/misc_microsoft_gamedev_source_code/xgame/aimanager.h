//==============================================================================
// aimanager.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once

#include "aitypes.h"
#include "gamefilemacros.h"

class BAIMissionTargetWrapper;
class BAIGroup;
class BAIGroupTask;
class BAIMissionTarget;
class BAITeleporterZone;
class BAITopic;
class BAIPowerMission;
class BAIMission;


//==============================================================================
// class BAIManager
// Resource manager for all things AI.
//==============================================================================
class BAIManager
{
public:
   BAIManager();
   ~BAIManager();

   BAI* createAI(BPlayerID playerID, BTeamID teamID);
   BAI* getAI(BPlayerID playerID);
   const BAI* getAI(BPlayerID playerID) const;
   void deleteAI(BPlayerID playerID);

   BKB* createKB(BTeamID teamID);
   BKB* getKB(BTeamID teamID);
   const BKB* getKB(BTeamID teamID) const;
   void deleteKB(BTeamID teamID);

   BAIMissionTargetWrapper* createWrapper(BAIMissionID missionID, BAIMissionTargetID targetID);
   BAIMissionTargetWrapper* getWrapper(BAIMissionTargetWrapperID missionTargetWrapperID);
   const BAIMissionTargetWrapper* getWrapper(BAIMissionTargetWrapperID missionTargetWrapperID) const;
   void deleteWrapper(BAIMissionTargetWrapperID missionTargetWrapperID);

   BAIGroup* createAIGroup(BAIMissionID missionID, BProtoSquadID protoSquadID);
   BAIGroup* getAIGroup(BAIGroupID groupID);
   const BAIGroup* getAIGroup(BAIGroupID groupID) const;
   void deleteAIGroup(BAIGroupID groupID);

   BAIGroupTask* createAIGroupTask(BAIGroupID groupID);
   BAIGroupTask* getAIGroupTask(BAIGroupTaskID groupTaskID);
   const BAIGroupTask* getAIGroupTask(BAIGroupTaskID groupTaskID) const;
   void deleteAIGroupTask(BAIGroupTaskID groupTaskID);

   BAIMissionTarget* createAIMissionTarget(BPlayerID playerID);
   BAIMissionTarget* getAIMissionTarget(BAIMissionTargetID missionTargetID);
   const BAIMissionTarget* getAIMissionTarget(BAIMissionTargetID missionTargetID) const;
   void deleteAIMissionTarget(BAIMissionTargetID missionTargetID);

   BAITopic* createAITopic(BPlayerID playerID);
   BAITopic* getAITopic(BAITopicID topicID);
   const BAITopic* getAITopic(BAITopicID topicID) const;
   void deleteAITopic(BAITopicID topicID);

   BAIMission* createAIMission(BPlayerID playerID, BAIMissionType missionType);
   BAIMission* getAIMission(BAIMissionID missionID);
   const BAIMission* getAIMission(BAIMissionID missionID) const;
   void deleteAIMission(BAIMissionID missionID);

   BKBSquad* createKBSquad(BTeamID teamID);
   BKBSquad* getKBSquad(BKBSquadID kbSquadID);
   const BKBSquad* getKBSquad(BKBSquadID kbSquadID) const;
   void deleteKBSquad(BKBSquadID kbSquadID);

   BKBBase* createKBBase(BTeamID teamID, BPlayerID playerID, BVector position, float radius);
   BKBBase* getKBBase(BKBBaseID kbBaseID);
   const BKBBase* getKBBase(BKBBaseID kbBaseID) const;
   void deleteKBBase(BKBBaseID kbBaseID);

   BBid* createBid(BPlayerID playerID);
   BBid* getBid(BBidID bidID);
   const BBid* getBid(BBidID bidID) const;
   void deleteBid(BBidID bidID);

   BAITeleporterZone* createAITeleporterZone(BEntityID unitID, BVector boxExtent1, BVector boxExtent2);
   BAITeleporterZone* createAITeleporterZone(BEntityID unitID, BVector spherePos, float sphereRad);
   BAITeleporterZone* getAITeleporterZone(BAITeleporterZoneID zoneID);
   const BAITeleporterZone* getAITeleporterZone(BAITeleporterZoneID zoneID) const;
   void deleteAITeleporterZone(BAITeleporterZoneID zoneID);
   const BAITeleporterZoneIDArray& getAITeleporterZoneIDs() const { return (mTeleporterZoneIDs); }
   const BAITeleporterZoneIDArray& getAITeleporterPickupZoneIDs() const { return (mTeleporterPickupZoneIDs); }
   const BAITeleporterZoneIDArray& getAITeleporterDropoffZoneIDs() const { return (mTeleporterDropoffZoneIDs); }

   void updateAIs();
   void updateAIMissions();
   void updateKBs();
   void updateAllKBsForSquad(BSquad* pSquad);

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   BAI* mAIs[cMaximumSupportedPlayers];
   BKB* mKBs[cMaximumSupportedTeams];
   
   BFreeList<BAIMissionTargetWrapper, 4>  mWrappers;
   BFreeList<BAIGroup, 4>                 mGroups;
   BFreeList<BAIGroupTask, 4>             mGroupTasks;
   BFreeList<BAIMissionTarget, 4>         mMissionTargets;
   BFreeList<BAITopic, 4>                 mTopics;
   BFreeList<BAIPowerMission, 3>          mPowerMissions;
   BFreeList<BAIMission, 3>               mMissions;
   BFreeList<BKBSquad, 4>                 mKBSquads;
   BFreeList<BKBBase, 4>                  mKBBases;
   BFreeList<BBid, 4>                     mBids;
   BFreeList<BAITeleporterZone, 4>        mTeleporterZones;

   BAITeleporterZoneIDArray               mTeleporterZoneIDs;
   BAITeleporterZoneIDArray               mTeleporterPickupZoneIDs;
   BAITeleporterZoneIDArray               mTeleporterDropoffZoneIDs;

#ifndef BUILD_FINAL
   uint mNextWrapperDebugID;
   uint mNextGroupDebugID;
   uint mNextGroupTaskDebugID;
   uint mNextMissionTargetDebugID;
   uint mNextTopicDebugID;
   uint mNextMissionDebugID;
   uint mNextBidDebugID;
   uint mNextZoneDebugID;
   uint mNextKBSquadDebugID;
   uint mNextKBBaseDebugID;
#endif
};
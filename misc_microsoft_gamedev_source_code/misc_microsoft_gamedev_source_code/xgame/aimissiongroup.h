//==============================================================================
// aimissiongroup.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once

// xgame includes
#include "aitypes.h"
#include "SimTarget.h"

// Forward declarations
class BKB;
class BProtoAction;


//==============================================================================
// class BAIGroupTask
//==============================================================================
class BTaskInfo
{
public:

   BTaskInfo();
   void reset();

   BVector getTargetPos() const { return (mTargetPos); }
   BEntityID getTargetID() const { return (mTargetID); }
   BAbilityID getAbilityID() const { return (mAbilityID); }
   BAIGroupTaskType getType() const { return (mType); }
   float getScore() const { return (mScore); }

   void setTarget(BVector v);
   void setTarget(BEntityID v);
   void setAbilityID(BAbilityID v) { mAbilityID = v; }
   void setType(BAIGroupTaskType v) { mType = v; }
   void setScore(float v) { mScore = v; }

protected:

   // Stuff that we save out.
   BVector mTargetPos;
   BEntityID mTargetID;
   BAbilityID mAbilityID;
   BAIGroupTaskType mType;
   float mScore;
};


typedef BSmallDynamicSimArray<BTaskInfo> BTaskInfoArray;


//==============================================================================
// class BAIGroupTask
//==============================================================================
class BAIGroupTask
{
public:

   BAIGroupTask();
   void resetNonIDData();

   bool isEquivalentTask(const BTaskInfo& ti, float posEps = 0.0f) const
   {
      if (mType != ti.getType())
         return (false);
      if (mTargetID != ti.getTargetID())
         return (false);
      if (mAbilityID != ti.getAbilityID())
         return (false);
      float epsilonSqr = posEps * posEps;
      if (mTargetPos.xzDistanceSqr(ti.getTargetPos()) > epsilonSqr)
         return (false);
      return (true);
   }

   BVector getTargetPos() const { return (mTargetPos); }
   BEntityID getTargetID() const { return (mTargetID); }
   BAbilityID getAbilityID() const { return (mAbilityID); }
   float getScore() const { return (mScore); }
   BAIGroupTaskType getType() const { return (mType); }
   BAIGroupTaskID getID() const { return (mID); }
   BAIGroupTaskID& getID() { return (mID); }
   BPlayerID getPlayerID() const { return (mPlayerID); }
   BAIGroupID getGroupID() const { return (mGroupID); }
   BVector getGroupPosWhenTasked() const { return (mGroupPosWhenTasked); }
   DWORD getTimestampTasked() const { return (mTimestampTasked); }
   DWORD getTimestampValidUntil() const { return (mTimestampValidUntil); }
   bool getFlagValidUntil() const { return (mFlagValidUntil); }

   // set
   void setTargetPos(BVector v) { mTargetPos = v; }
   void setTargetID(BEntityID v) { mTargetID = v; }
   void setAbilityID(BAbilityID v) { mAbilityID = v; }
   void setScore(float v) { mScore = v; }
   void setType(BAIGroupTaskType v) { mType = v; }
   void setID(BAIGroupTaskID v) { mID = v; }
   void setID(uint refCount, uint index) { mID.set(refCount, index); }
   void setPlayerID(BPlayerID v) { mPlayerID = v; }
   void setGroupID(BAIGroupID v) { mGroupID = v; }
   void setGroupPosWhenTasked(BVector v) { mGroupPosWhenTasked = v; }
   void setTimestampTasked(DWORD v) { mTimestampTasked = v; }
   void setTimestampValidUntil(DWORD v) { mTimestampValidUntil = v; setFlagValidUntil(true); }
   void clearValidUntil() { mTimestampValidUntil = 0; setFlagValidUntil(false); }
   void setFlagValidUntil(bool v) { mFlagValidUntil = v; }

   #ifndef BUILD_FINAL
      uint mDebugID;
      void getDebugString(BSimString& str) const;
   #endif

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   BVector mTargetPos;
   BEntityID mTargetID;
   BAbilityID mAbilityID;
   BAIGroupTaskType mType;
   float mScore;
   BAIGroupTaskID mID;
   BPlayerID mPlayerID;
   BAIGroupID mGroupID;
   BVector mGroupPosWhenTasked;
   DWORD mTimestampTasked;
   DWORD mTimestampValidUntil;
   bool mFlagValidUntil    : 1;
};

typedef std::pair<BProtoSquadID, BEntityIDArray> BAIGroupSubtype;


//==============================================================================
// class BAIGroup
// BAIGroup is a collection of same-type squads assigned to a mission for a
// role.  They never contain multiple types.
//==============================================================================
class BAIGroup
{
public:

   BAIGroup();
   void resetNonIDData();

   // Get accessors
   BVector getPosition() const { return (mPosition); }
   BVector getCentroid() const { return (mCentroid); }
   float getRadius() const { return (mRadius); }
   BAIGroupID& getID() { return (mID); }
   BAIGroupID getID() const { return (mID); }
   BPlayerID getPlayerID() const { return (mPlayerID); }
   BAIMissionID getMissionID() const { return (mMissionID); }
   BAIGroupState getState() const { return (mState); }
   const BEntityIDArray& getSquads() const { return (mSquads); }
   uint getNumSquads() const { return (mSquads.getSize()); }
   const BAIGroupTaskIDArray& getTasks() const { return (mTasks); }
   uint getNumTasks() const { return (mTasks.getSize()); }
   BAIGroupTaskID getCurrentTaskID() const;
   const BAIGroupTask* getCurrentTask() const;
   const BAISquadAnalysis& getSquadAnalysis() const { return (mSquadsAnalysis); }
   BAIGroupRole getRole() const { return (mRole); }
   bool isState(BAIGroupState gs) const { return (mState == gs); }
   bool isRole(BAIGroupRole gr) const { return (mRole == gr); }
   DWORD getTimestampCreated() const { return (mTimestampCreated); }
   DWORD getTimestampLastEval() const { return (mTimestampLastEval); }
   DWORD getTimestampLastTask() const { return (mTimestampLastTask); }
   DWORD getTimestampSquadAdded() const { return (mTimestampSquadAdded); }
   float getMaxAttackRange() const { return (mMaxAttackRange); }
   float getMaxVelocity() const { return (mMaxVelocity); }
   BProtoSquadID getProtoSquadID() const { return (mProtoSquadID); }
   bool getFlagTasked() const { return (mFlagTasked); }
   bool getFlagTaskedThisState() const { return (mFlagTaskedThisState); }
   //bool getFlagCanGather() const { return (mFlagCanGather); }
   bool getFlagCanGatherSupplies() const { return (mFlagCanGatherSupplies); }
   bool getFlagCanAttack() const { return (mFlagCanAttack); }
   bool getFlagCanRepair() const { return (mFlagCanRepair); }
   bool getFlagCanAutoRepair() const { return (mFlagCanAutoRepair); }

   // Set accessors
   void setPosition(BVector v) { mPosition = v; }
   void setCentroid(BVector v) { mCentroid = v; }
   void setRadius(float v) { mRadius = v; }
   void setID(BAIGroupID v) { mID = v; }
   void setID(uint refCount, uint index) { mID.set(refCount, index); }
   void setPlayerID(BPlayerID v) { mPlayerID = v; }
   void setMissionID(BAIMissionID v) { mMissionID = v; }
   void setState(BAIGroupState v) { mState = v; }
   void setRole(BAIGroupRole v) { mRole = v; }
   void setTimestampCreated(DWORD v) { mTimestampCreated = v; }
   void setTimestampLastEval(DWORD v) { mTimestampLastEval = v; }
   void setTimestampLastTask(DWORD v) { mTimestampLastTask = v; }
   void setTimestampSquadAdded(DWORD v) { mTimestampSquadAdded = v; }
   void setMaxAttackRange(float v) { mMaxAttackRange = v; }
   void setMaxVelocity(float v) { mMaxVelocity = v; }
   void setProtoSquadID(BProtoSquadID v) { mProtoSquadID = v; }
   void setFlagTasked(bool v) { mFlagTasked = v; }
   void setFlagTaskedThisState(bool v) { mFlagTaskedThisState = v; }
   void setFlagCanGather(bool v) { mFlagCanGather = v; }
   void setFlagCanGatherSupplies(bool v) { mFlagCanGatherSupplies = v; }
   void setFlagCanAttack(bool v) { mFlagCanAttack = v; }
   void setFlagCanRepair(bool v) { mFlagCanRepair = v; }
   void setFlagCanAutoRepair(bool v) { mFlagCanAutoRepair = v; }

   // More stuff.
   bool containsSquad(BEntityID squadID) const { return (mSquads.contains(squadID)); }
   void addSquad(BEntityID squadID);
   void removeSquad(BEntityID squadID);
   void recalculatePosition();
   void recalculateSquadAnalysis();
   void recalculateAttackRanges();

   void update(DWORD currentGameTime);
   void updateTasks(DWORD currentGameTime);
   void updateGroupFlags();
   void updateSelectionAbility();
   void updateSelectionAbility(BEntityID hoverObject, BVector hoverPos);

   bool testGroupForTask(BAIGroupTaskType type, BAbilityID abilityID, BEntityID targetID, BVector targetPos, float posEps = 0.0f) const;

   void addTaskID(BAIGroupTaskID taskID) { mTasks.add(taskID); }
   void removeTaskID(BAIGroupTaskID taskID) { mTasks.remove(taskID); }
   bool containsTaskID(BAIGroupTaskID taskID) { return (mTasks.contains(taskID)); }

   void deleteAllTasks();

   const BSelectionAbility& getSelectionAbility() const { return (mSelectionAbility); }
   
   // Tasks stuff.
   bool canAcceptSquad(BEntityID squadID) const;
   //bool canAcceptGroup(BAIGroupID groupID) const;
   bool canAcceptAdditionalSquads() const;

   #ifndef BUILD_FINAL
      uint mDebugID;
      void debugRender();
      void debugRenderTargets();
      void clearPotentialTasks() { mPotentialTasks.resize(0); }
      void addPotentialTask(BTaskInfo ti) { mPotentialTasks.add(ti); }
      void reservePotentialTasks(uint num) { mPotentialTasks.reserve(num); }
      void setPotentialTasks(const BTaskInfoArray v) { mPotentialTasks = v; }
      const BTaskInfoArray& getPotentialTasks() const { return (mPotentialTasks); }
   #endif

   BProtoAction* getProtoActionForTarget(BEntityID targetID, BVector targetLoc, BAbilityID abilityID) const;
   float getDPSForTarget(BEntityID targetID, BVector targetLoc, BAbilityID abilityID) const;

   bool canDoDamageAbility() const;
   bool canDoSuicideAbility() const;
   bool canDoOverburnAbility() const;
   bool canDoFlashBangAbility() const;
   bool canDoStasisAbility() const;
   bool canDoRamAbility() const;
   bool canDoHijackAbility() const;
   bool canDoCloakAbility() const;
   bool canDoJumppackAbility() const;

   // Demo stuff
   //bool demoHackIsAHumanWatching() const;

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   BVector mPosition;                     // The position of the group.  Determined via entity grouper I guess.
   BVector mCentroid;                     // The center of the group.
   float mRadius;                         // The radius of the group
   BAIGroupTaskIDArray mTasks;            // The things the AI has told this group to do (might not be what they are actually doing.)
   #ifndef BUILD_FINAL
   BTaskInfoArray mPotentialTasks;        // Possible tasks
   #endif
   BAIGroupID mID;                        // The ID of the mission group.
   BPlayerID mPlayerID;
   BAIMissionID mMissionID;               // The ID of the mission we belong to.
   BAIGroupState mState;                  // The state of the mission group.
   BEntityIDArray mSquads;                // The squads that are assigned to this group currently.
   BAISquadAnalysis mSquadsAnalysis;      // The analysis of our current squads.
   BAIGroupRole mRole;                    // The role we want this group to be playing.
   DWORD mTimestampCreated;               // The time when this was created.
   DWORD mTimestampLastEval;              // Last time we considered what to do.
   DWORD mTimestampLastTask;              // Last time we did something.
   DWORD mTimestampSquadAdded;            // Last time we added a squad.
   float mMaxAttackRange;                 // Max attack range.
   float mMaxVelocity;                    // Max velocity.
   BProtoSquadID mProtoSquadID;           // Restricted by proto squad?
   BSelectionAbility mSelectionAbility;   // The selection ability.

   bool mFlagTasked : 1;
   bool mFlagTaskedThisState : 1;
   bool mFlagCanGather : 1;
   bool mFlagCanGatherSupplies : 1;
   bool mFlagCanAttack : 1;
   bool mFlagCanRepair : 1;
   bool mFlagCanAutoRepair : 1;
};
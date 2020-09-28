//==============================================================================
// aitopic.h
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================
#pragma once

#include "aitypes.h"
#include "gamefilemacros.h"

class BAI;

//==============================================================================
// class BAITopic
//==============================================================================
class BAITopic
{
public:

   BAITopic();
   ~BAITopic();

   void resetNonIDData();
   void toggleActive(DWORD currentGameTime, bool active);

   // Get accessors
   BAIMissionID getAIMissionID() const { return (mAIMissionID); }
   BAITopicID& getID() { return (mID); }
   BAITopicID getID() const { return (mID); }
   BPlayerID getPlayerID() const { return (mPlayerID); }
   DWORD getTicketInterval() const { return (mTicketInterval); }
   BTriggerScriptID getTriggerScriptID() const;
   BTriggerScriptID getTriggerScriptID();
   BTriggerVarID getTriggerVarID() const;
   BTriggerVarID getTriggerVarID();
   uint getMinTickets() const { return (mMinTickets); }
   uint getMaxTickets() const { return (mMaxTickets); }
   uint getCurrentTickets() const { return (mCurrentTickets); }
   uint getType() const { return (mType); }
   bool getFlagActive() const { return (mFlagActive); }
   bool getFlagPriority() const { return (mFlagPriority); }
   bool isType(uint type) const { return (mType == type); }

   DWORD getTimestampActiveToggled() const { return (mTimestampActiveToggled); }
   DWORD getTimestampNextTicketUpdate() const { return (mTimestampNextTicketUpdate); }
   DWORD getTimestampPriorityRequest() const { return (mTimestampPriorityRequest); }

   void enablePriorityRequest(DWORD currentGameTime) { mTimestampPriorityRequest = currentGameTime; mFlagPriority = true; }
   void disablePriorityRequest() { mTimestampPriorityRequest = 0; mFlagPriority = false; }
   void setNextTicketTime(DWORD nextTicketTime) { mTimestampNextTicketUpdate = nextTicketTime; }


   // Set accessors
   void setID(BAITopicID id) { mID = id; }
   void setID(uint refCount, uint index) { mID.set(refCount, index); }
   void setPlayerID(BPlayerID playerID) { mPlayerID = playerID; }
   void setTicketInterval(DWORD v) { BASSERT(v > 0); mTicketInterval = v; }
   void setTriggerScriptID(BTriggerScriptID v) { mTriggerScriptID = v; }
   void setTriggerVarID(BTriggerVarID v) { mTriggerVarID = v; }
   void setAIMissionID(BAIMissionID v) { mAIMissionID = v; }
   void setMinTickets(uint v) { mMinTickets = v; if (mMinTickets > mCurrentTickets) mCurrentTickets = mMinTickets; }
   void setMaxTickets(uint v) { mMaxTickets = v; if (mCurrentTickets > mMaxTickets) mCurrentTickets = mMaxTickets; }
   void setCurrentTickets(uint v) { mCurrentTickets = v; }
   void setCurrentTicketsToMin() { mCurrentTickets = mMinTickets; }
   void setType(uint v) { mType = v; }

   // Helper stuff.
   void addTickets(uint v);


   #ifndef BUILD_FINAL
      const BSimString& getName() const { return (mName); }
      void setName(const BSimString& name) { mName = name; }
      BSimString mName;
      uint mDebugID;
   #endif

   GFDECLAREVERSION();
   bool save(BStream* pStream, int saveType) const;
   bool load(BStream* pStream, int saveType);

protected:

   BAITopicID mID;
   DWORD mTicketInterval;
   BTriggerScriptID mTriggerScriptID;
   BTriggerVarID mTriggerVarID;
   BAIMissionID mAIMissionID;
   BPlayerID mPlayerID;
   
   uint mMinTickets;
   uint mMaxTickets;
   uint mCurrentTickets;
   uint mType;

   DWORD mTimestampActiveToggled;
   DWORD mTimestampNextTicketUpdate;
   DWORD mTimestampPriorityRequest;

   bool mFlagActive     : 1;
   bool mFlagPriority   : 1;
};
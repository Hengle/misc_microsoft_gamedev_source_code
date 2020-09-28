//==============================================================================
// aitopic.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "ai.h"
#include "aidebug.h"
#include "aimission.h"
#include "aitopic.h"
#include "triggermanager.h"
#include "triggerscript.h"
#include "triggervar.h"
#include "world.h"

GFIMPLEMENTVERSION(BAITopic, 2);

//==============================================================================
//==============================================================================
BAITopic::BAITopic()
{
   mID = 0;
   resetNonIDData();
}

//==============================================================================
//==============================================================================
BAITopic::~BAITopic()
{
}

//==============================================================================
//==============================================================================
void BAITopic::resetNonIDData()
{
   mTicketInterval = 0;
   mTriggerScriptID = cInvalidTriggerScriptID;
   mTriggerVarID = cInvalidTriggerVarID;
   mAIMissionID = cInvalidAIMissionID;
   mMinTickets = 0;
   mMaxTickets = 0;
   mCurrentTickets = 0;
   mType = AITopicType::cInvalid;
   mFlagActive = false;
   mFlagPriority = false;
   mTimestampActiveToggled = 0;
   mTimestampNextTicketUpdate = 0;
   mTimestampPriorityRequest = 0;

   #ifndef BUILD_FINAL
      mName = "";
      mDebugID = 0;
   #endif
}

//==============================================================================
//==============================================================================
void BAITopic::toggleActive(DWORD currentGameTime, bool active)
{
   // on!
   mFlagActive = active;
   mTimestampActiveToggled = currentGameTime;
   
   // If this is a script module topic, toggle that on in script.
   if (isType(AITopicType::cScriptModule))
   {
      BTriggerScriptID triggerScriptID = getTriggerScriptID();
      BTriggerVarID triggerVarID = getTriggerVarID();
      BTriggerScript* pTriggerScript = gTriggerManager.getTriggerScript(triggerScriptID);
      BASSERT(pTriggerScript);
      if (!pTriggerScript)
         return;
      BTriggerVar* pTriggerVar = pTriggerScript->getTriggerVar(triggerVarID);
      BASSERT(pTriggerVar);
      if (!pTriggerVar)
         return;
      pTriggerVar->asBool()->writeVar(active);
   }
   else if (isType(AITopicType::cMission))
   {
      BAIMission* pMission = gWorld->getAIMission(mAIMissionID);
      BASSERT(pMission);
      if (!pMission)
         return;
      pMission->toggleActive(currentGameTime, active);
   }
}


//==============================================================================
//==============================================================================
BTriggerScriptID BAITopic::getTriggerScriptID() const
{
   if (isType(AITopicType::cScriptModule))
      return (mTriggerScriptID);
   else
      return (cInvalidTriggerScriptID);
}

//==============================================================================
//==============================================================================
BTriggerScriptID BAITopic::getTriggerScriptID()
{
   if (isType(AITopicType::cScriptModule))
      return (mTriggerScriptID);
   else
      return (cInvalidTriggerScriptID);
}

//==============================================================================
//==============================================================================
BTriggerVarID BAITopic::getTriggerVarID() const
{
   if (isType(AITopicType::cScriptModule))
      return (mTriggerVarID);
   else
      return (cInvalidTriggerVarID);
}

//==============================================================================
//==============================================================================
BTriggerVarID BAITopic::getTriggerVarID()
{
   if (isType(AITopicType::cScriptModule))
      return (mTriggerVarID);
   else
      return (cInvalidTriggerVarID);
}


//==============================================================================
//==============================================================================
void BAITopic::addTickets(uint v)
{
   mCurrentTickets += v;
   if (mCurrentTickets > mMaxTickets)
      mCurrentTickets = mMaxTickets;
}

//==============================================================================
// BAITopic::save
//==============================================================================
bool BAITopic::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, BAITopicID, mID);
   GFWRITEVAR(pStream, DWORD, mTicketInterval);
   GFWRITEVAR(pStream, BTriggerScriptID, mTriggerScriptID);
   GFWRITEVAR(pStream, BTriggerVarID, mTriggerVarID);
   GFWRITEVAR(pStream, BAIMissionID, mAIMissionID);
   GFWRITEVAR(pStream, BPlayerID, mPlayerID);

   GFWRITEVAR(pStream, uint, mMinTickets);
   GFWRITEVAR(pStream, uint, mMaxTickets);
   GFWRITEVAR(pStream, uint, mCurrentTickets);
   GFWRITEVAR(pStream, uint, mType);

   GFWRITEVAR(pStream, DWORD, mTimestampActiveToggled);
   GFWRITEVAR(pStream, DWORD, mTimestampNextTicketUpdate);
   GFWRITEVAR(pStream, DWORD, mTimestampPriorityRequest);

   GFWRITEBITBOOL(pStream, mFlagActive);
   GFWRITEBITBOOL(pStream, mFlagPriority);

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
// BAITopic::load
//==============================================================================
bool BAITopic::load(BStream* pStream, int saveType)
{  
   GFREADVAR(pStream, BAITopicID, mID);
   GFREADVAR(pStream, DWORD, mTicketInterval);
   GFREADVAR(pStream, BTriggerScriptID, mTriggerScriptID);
   GFREADVAR(pStream, BTriggerVarID, mTriggerVarID);
   GFREADVAR(pStream, BAIMissionID, mAIMissionID);
   GFREADVAR(pStream, BPlayerID, mPlayerID);

   GFREADVAR(pStream, uint, mMinTickets);
   GFREADVAR(pStream, uint, mMaxTickets);
   GFREADVAR(pStream, uint, mCurrentTickets);
   GFREADVAR(pStream, uint, mType);

   GFREADVAR(pStream, DWORD, mTimestampActiveToggled);
   GFREADVAR(pStream, DWORD, mTimestampNextTicketUpdate);
   GFREADVAR(pStream, DWORD, mTimestampPriorityRequest);

   GFREADBITBOOL(pStream, mFlagActive);
   GFREADBITBOOL(pStream, mFlagPriority);

#ifndef BUILD_FINAL  
   GFREADSTRING(pStream, BSimString, mName, 100);
   if (mGameFileVersion < 2)
      mName.empty();
   GFREADVAR(pStream, uint, mDebugID);
#else
   BSimString tempStr;
   GFREADSTRING(pStream, BSimString, tempStr, 100);
   GFREADTEMPVAL(pStream, uint);
#endif

   return true;
}

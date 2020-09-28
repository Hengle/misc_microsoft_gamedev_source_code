//==============================================================================
// trigger.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "ai.h"
#include "world.h"
#include "trigger.h"
#include "triggercondition.h"
#include "triggereffect.h"
#include "triggerscript.h"
#include "triggermanager.h"
// xsystem
#include "xmlreader.h" 


IMPLEMENT_FREELIST(BTrigger, 6, &gSimHeap);

GFIMPLEMENTVERSION(BTrigger, 1);

//==============================================================================
//==============================================================================
void BTrigger::onAcquire()
{
   mID = cInvalidTriggerID;
   mEditorID = cInvalidTriggerID;
   mpParentTriggerScript = NULL;
   mActivatedTime = 0;
   mNextEvaluateTime = 0;
   mEvaluateFrequency = 0;
   mEvaluateCount = 0;
   mEvaluateLimit = 0;
   mbStartActive = false;
   mbConditional = false;
   mbOrConditions = false;
   #ifndef BUILD_FINAL
      mName.empty();
   #endif
}


//==============================================================================
//==============================================================================
void BTrigger::onRelease()
{
   mID = cInvalidTriggerID;
   mEditorID = cInvalidTriggerID;
   mpParentTriggerScript = NULL;
   mActivatedTime = 0;
   mNextEvaluateTime = 0;
   mEvaluateFrequency = 0;
   mEvaluateCount = 0;
   mEvaluateLimit = 0;
   mbStartActive = false;
   mbConditional = false;
   mbOrConditions = false;
   
   uint numConditions = mConditions.getSize();
   for (uint i=0; i<numConditions; i++)
      BTriggerCondition::releaseInstance(mConditions[i]);
   mConditions.clear();

   uint numEffectsOnTrue = mEffectsOnTrue.getSize();
   for (uint i=0; i<numEffectsOnTrue; i++)
      BTriggerEffect::releaseInstance(mEffectsOnTrue[i]);
   mEffectsOnTrue.clear();

   uint numEffectsOnFalse = mEffectsOnFalse.getSize();
   for (uint i=0; i<numEffectsOnFalse; i++)
      BTriggerEffect::releaseInstance(mEffectsOnFalse[i]);
   mEffectsOnFalse.clear();
}


//==============================================================================
// BTrigger::onActivated
//==============================================================================
void BTrigger::onActivated()
{
   mActivatedTime = gWorld->getGametime();
   uint numConditions = mConditions.getSize();
   for (uint i=0; i<numConditions; i++)
      mConditions[i]->initAsyncSettings();
}


//==============================================================================
// BTrigger::loadFromXML
//==============================================================================
bool BTrigger::loadFromXML(BXMLNode  node)
{
   // TODO: add check for duplicate ID's.
   BFATAL_ASSERT(node);
   mEditorID = cInvalidTriggerID;
   if (!node.getAttribValueAsUInt("ID", mEditorID))
      return (false);
   if (mEditorID == cInvalidTriggerID)
      return (false);

   bool startActive = false;
   if (!node.getAttribValueAsBool("Active", startActive))
      return (false);
   mbStartActive = startActive;
   if (!node.getAttribValueAsDWORD("EvaluateFrequency", mEvaluateFrequency))
      return (false);
   mEvaluateLimit = 0;
   node.getAttribValueAsDWORD("EvalLimit", mEvaluateLimit);
   bool conditionalTrigger = false;
   if (!node.getAttribValueAsBool("ConditionalTrigger", conditionalTrigger))
      return (false);
   #ifndef BUILD_FINAL
   if(gTriggerManager.getFlagLogTriggerNames())
   {
      node.getAttribValueAsString("Name", mName);
      node.getAttribValueAsLong("GroupID", mGroupID);
   }
   #endif
   mbConditional = conditionalTrigger;

   // Read in the trigger conditions.
   BXMLNode conditionsNode(node.getChildNode("TriggerConditions"));
   if (!conditionsNode)
      return (false);

   // We should only have one child node (or zero if no conditions)
   if (conditionsNode.getNumberChildren() == 1)
   {
      BXMLNode andOrNode(conditionsNode.getChild(0L));
      if (!andOrNode)
         return (false);
      if (andOrNode.getName() == "And")
         mbOrConditions = false;
      else if (andOrNode.getName() == "Or")
         mbOrConditions = true;
      else
         return (false);

      long numConditions = andOrNode.getNumberChildren();
      for (long i=0; i<numConditions; i++)
      {
         BXMLNode condNode(andOrNode.getChild(i));
         if (!condNode || condNode.getName() != "Condition")
            return (false);
         BTriggerCondition* pCond = BTriggerCondition::getInstance();
         pCond->setParentTriggerScript(this->getParentTriggerScript());
         pCond->setParentTrigger(this);
         mConditions.add(pCond);
         if (!pCond->loadFromXML(condNode, this))
            return (false);
      }
   }
  
   // Read in the onTrue trigger effects.
   BXMLNode  effectsOnTrueNode(node.getChildNode("TriggerEffectsOnTrue"));
   if (!effectsOnTrueNode)
      return (false);
   long numEffectsOnTrueNodes = effectsOnTrueNode.getNumberChildren();
   for (long i=0; i<numEffectsOnTrueNodes; i++)
   {
      BXMLNode  effectNode(effectsOnTrueNode.getChild(i));
      if (effectNode.getName() != "Effect")
         return (false);
      BTriggerEffect* pEffect = BTriggerEffect::getInstance();
      pEffect->setParentTriggerScript(this->getParentTriggerScript());
      mEffectsOnTrue.add(pEffect);
      if (!pEffect->loadFromXML(effectNode))
         return (false);
   }

   // Read in the onFalse trigger effects.
   BXMLNode  effectsOnFalseNode(node.getChildNode("TriggerEffectsOnFalse"));
   if (!effectsOnFalseNode)
      return (false);
   long numEffectsOnFalseNodes = effectsOnFalseNode.getNumberChildren();
   for (long i=0; i<numEffectsOnFalseNodes; i++)
   {
      BXMLNode  effectNode(effectsOnFalseNode.getChild(i));
      if (effectNode.getName() != "Effect")
         return (false);
      BTriggerEffect* pEffect = BTriggerEffect::getInstance();
      pEffect->setParentTriggerScript(this->getParentTriggerScript());
      mEffectsOnFalse.add(pEffect);
      if (!pEffect->loadFromXML(effectNode))
         return (false);
   }

   return (true);
}


//==============================================================================
// BTrigger::timeToEvaluate
//==============================================================================
bool BTrigger::timeToEvaluate(DWORD currentUpdateTime)
{
   // Is it time to do the next evaluation.
   return (currentUpdateTime >= mNextEvaluateTime);
}


//==============================================================================
// BTrigger::updateAsyncConditions
//==============================================================================
void BTrigger::updateAsyncConditions()
{
   uint numConditions = mConditions.getSize();
   for (uint i=0; i<numConditions; i++)
      mConditions[i]->updateAsyncCondition();
}


//==============================================================================
// BTrigger::evaluateConditions
//==============================================================================
bool BTrigger::evaluateConditions(DWORD currentUpdateTime)
{   
   SCOPEDSAMPLE(BTrigger_evaluateConditions);

   // We just evaluated, so update the next time to evaluate based on our evaluation frequency and the current time.
   mNextEvaluateTime = currentUpdateTime + mEvaluateFrequency;

   // Also increment the evaluate counter
   mEvaluateCount++;

   // NOTE: Zero conditions is automatically true and fires the effects.
   uint numConditions = mConditions.getSize();
   if (numConditions == 0)
   {
      #ifndef BUILD_FINAL
      //  long playerID;
      //  if ( ((playerID = this->getParentTriggerScript()->mPlayerID) > 0) 
      //  &&   ((this->getParentTriggerScript()->mAILogFilterByGroup == false) || (this->getParentTriggerScript()->mAILogGroup == this->mGroupID)))
      //     gWorld->getPlayer(playerID)->getAI()->aiLog("Trigger %4d %-24s PASS: No conditions.\n",this->getEditorID(),this->getName().getPtr());
      #endif
      return (true);
   }

   // We have some conditions so figure out the right way to handle them:
   // OR the conditions together.
   if (mbOrConditions)
   {
      for (uint i=0; i<numConditions; i++)
      {
         if (mConditions[i]->evaluate())
         {
            #ifndef BUILD_FINAL
            //   long playerID;
            //   if ( ((playerID = this->getParentTriggerScript()->mPlayerID) > 0) 
            //   &&   ((this->getParentTriggerScript()->mAILogFilterByGroup == false) || (this->getParentTriggerScript()->mAILogGroup == this->mGroupID)))
            //      gWorld->getPlayer(playerID)->getAI()->aiLog("Trigger %4d %-24s PASS: %d OR conditions, #%d was true.\n",this->getEditorID(),this->getName().getPtr(),numConditions, i+1);
            #endif
            return (true);
         }
      }
      #ifndef BUILD_FINAL
      //   long playerID;
      //   if ( ((playerID = this->getParentTriggerScript()->mPlayerID) > 0) 
      //   &&   ((this->getParentTriggerScript()->mAILogFilterByGroup == false) || (this->getParentTriggerScript()->mAILogGroup == this->mGroupID)))
      //      gWorld->getPlayer(playerID)->getAI()->aiLog("Trigger %4d %-24s FAIL: %d OR conditions.\n",this->getEditorID(),this->getName().getPtr(),numConditions);
      #endif
      return (false);
   }
   // AND the conditions together
   else
   {
      for (uint i=0; i<numConditions; i++)
      {
         if (!mConditions[i]->evaluate())
         {
            #ifndef BUILD_FINAL
            //   long playerID;
            //   if ( ((playerID = this->getParentTriggerScript()->mPlayerID) > 0) 
            //   &&   ((this->getParentTriggerScript()->mAILogFilterByGroup == false) || (this->getParentTriggerScript()->mAILogGroup == this->mGroupID)))
            //      gWorld->getPlayer(playerID)->getAI()->aiLog("Trigger %4d %-24s FAIL: %d AND conditions, #%d was false.\n",this->getEditorID(),this->getName().getPtr(),numConditions, i+1);
            #endif
            return (false);
         }
      }
      #ifndef BUILD_FINAL
      //   long playerID;
      //   if ( ((playerID = this->getParentTriggerScript()->mPlayerID) > 0) 
      //   &&   ((this->getParentTriggerScript()->mAILogFilterByGroup == false) || (this->getParentTriggerScript()->mAILogGroup == this->mGroupID)))
      //      gWorld->getPlayer(playerID)->getAI()->aiLog("Trigger %4d %-24s PASS: %d AND conditions.\n",this->getEditorID(),this->getName().getPtr(),numConditions);
      #endif
      return (true);
   }
}


//==============================================================================
// BTrigger::fireEffects
//==============================================================================
void BTrigger::fireEffects(bool onTrue)
{
   SCOPEDSAMPLE(BTrigger_fireEffects);

   // A non-conditional trigger can only fire the onTrue effects
   // A conditional trigger can fire the onTrue or onFalse effects.
   BASSERT(onTrue || mbConditional);

   // When we fire the trigger, deactivate it first, so that if it's wired into itself, it can re-activate itself.
   getParentTriggerScript()->deactivateTrigger(mID);

   #ifndef BUILD_FINAL
      bool loggedTrigger = false;
   #endif

   if (onTrue)
   {
      long numEffectsOnTrue = mEffectsOnTrue.getNumber();
      for (long i=0; i<numEffectsOnTrue; i++)
      {
         BASSERT(mEffectsOnTrue[i]);
         #ifndef BUILD_FINAL
            if (!loggedTrigger && mpParentTriggerScript->getLogEffects())
            {
               gConsole.output(cChannelTriggers, "Trigger %u '%s', time %.1f, group %d, script '%s'", mEditorID, mName.getPtr(), gWorld->getGametimeFloat(), mGroupID, mpParentTriggerScript->getName().getPtr());
               loggedTrigger = true;
            }
         #endif
         mEffectsOnTrue[i]->fire();
      }
   }
   else
   {
      long numEffectsOnFalse = mEffectsOnFalse.getNumber();
      for (long i=0; i<numEffectsOnFalse; i++)
      {
         BASSERT(mEffectsOnFalse[i]);
         #ifndef BUILD_FINAL
            if (!loggedTrigger && mpParentTriggerScript->getLogEffects())
            {
               gConsole.output(cChannelTriggers, "Trigger %u '%s', time %.1f, group %d, script '%s'", mEditorID, mName.getPtr(), gWorld->getGametimeFloat(), mGroupID, mpParentTriggerScript->getName().getPtr());
               loggedTrigger = true;
            }
         #endif
         mEffectsOnFalse[i]->fire();
      }
   }
}


//==============================================================================
// BTrigger::setAsyncConditionState
//==============================================================================
void BTrigger::setAsyncConditionState(long conditionID, bool state)
{
   uint numConditions = mConditions.getSize();
   for (uint i=0; i<numConditions; i++)
   {
      if (mConditions[i]->getID() == conditionID)
      {
         BASSERT(mConditions[i]->getAsyncCondition());
         mConditions[i]->setConsensusState(state);
         return;
      }
   }
}


//==============================================================================
// BTrigger::hasEvaluationsRemaining()
//==============================================================================
bool BTrigger::hasEvaluationsRemaining()
{
#ifndef BUILD_FINAL
   if (mEvaluateCount == BTrigger::cPerformanceWarningThreshold)
   {
      BSimString debugDataString;
      debugDataString.format("PERFORMANCE WARNING! - Trigger ID = %d has evaluated %d times this update.  Consider restructuring triggers to be more efficient.  ScriptName = %s, ScriptID = %d", getEditorID(), BTrigger::cPerformanceWarningThreshold, gTriggerManager.getCurrentScriptName().getPtr(), gTriggerManager.getCurrentTriggerScriptID());
      BASSERTM(mEvaluateCount != BTrigger::cPerformanceWarningThreshold, debugDataString);
   }
   else if (mEvaluateCount == BTrigger::cInfiniteLoopWarningThreshold)
   {
      BSimString debugDataString;
      debugDataString.format("POSSIBLE INFINITE LOOP! - Trigger ID = %d has evaluated %d times this update.  This is an extremely high number indicating your system may be about to lock up.  Go fix it ASAP!  Consider restructuring triggers to be more efficient.  ScriptName = %s, ScriptID = %d", getEditorID(), BTrigger::cInfiniteLoopWarningThreshold, gTriggerManager.getCurrentScriptName().getPtr(), gTriggerManager.getCurrentTriggerScriptID());
      BASSERTM(mEvaluateCount != BTrigger::cInfiniteLoopWarningThreshold, debugDataString);
   }
#endif
   return (mEvaluateLimit == 0 || mEvaluateCount < mEvaluateLimit);
}

//==============================================================================
//==============================================================================
bool BTrigger::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, BTriggerID, mID);
   GFWRITEVAR(pStream, BTriggerID, mEditorID);

   #ifndef BUILD_FINAL
      GFWRITESTRING(pStream, BSimString, mName, 200);
      GFWRITEVAR(pStream, long, mGroupID);
   #else
      BSimString tempStr;
      GFWRITESTRING(pStream, BSimString, tempStr, 200);
      GFWRITEVAL(pStream, long, -1);
   #endif

   uint8 conditionCount = (uint8)mConditions.size();
   GFVERIFYCOUNT(conditionCount, 200);
   GFWRITEVAR(pStream, uint8, conditionCount);
   for (uint8 i=0; i<conditionCount; i++)
   {
      if (!mConditions[i]->save(pStream, saveType))
         return false;
   }

   uint8 effectCount = (uint8)mEffectsOnTrue.size();
   GFVERIFYCOUNT(effectCount, 200);
   GFWRITEVAR(pStream, uint8, effectCount);
   for (uint8 i=0; i<effectCount; i++)
   {
      if (!mEffectsOnTrue[i]->save(pStream, saveType))
         return false;
   }

   effectCount = (uint8)mEffectsOnFalse.size();
   GFVERIFYCOUNT(effectCount, 200);
   GFWRITEVAR(pStream, uint8, effectCount);
   for (uint8 i=0; i<effectCount; i++)
   {
      if (!mEffectsOnFalse[i]->save(pStream, saveType))
         return false;
   }

   GFWRITEVAR(pStream, DWORD, mActivatedTime);
   GFWRITEVAR(pStream, DWORD, mNextEvaluateTime );
   GFWRITEVAR(pStream, DWORD, mEvaluateFrequency);
   GFWRITEVAR(pStream, DWORD, mEvaluateCount);
   GFWRITEVAR(pStream, DWORD, mEvaluateLimit);

   GFWRITEBITBOOL(pStream, mbStartActive);
   GFWRITEBITBOOL(pStream, mbConditional);
   GFWRITEBITBOOL(pStream, mbOrConditions);

   return true;
}

//==============================================================================
//==============================================================================
bool BTrigger::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, BTriggerID, mID);
   GFREADVAR(pStream, BTriggerID, mEditorID);

   #ifndef BUILD_FINAL
      GFREADSTRING(pStream, BSimString, mName, 200);
      GFREADVAR(pStream, long, mGroupID);
   #else
      BSimString tempStr;
      GFREADSTRING(pStream, BSimString, tempStr, 200);
      long tempLong;
      GFREADVAR(pStream, long, tempLong);
   #endif

   uint8 conditionCount;
   GFREADVAR(pStream, uint8, conditionCount);
   GFVERIFYCOUNT(conditionCount, 200);
   mConditions.reserve(conditionCount);
   for (uint8 i=0; i<conditionCount; i++)
   {
      BTriggerCondition* pCondition = BTriggerCondition::getInstance();
      if (!pCondition)
         return false;
      pCondition->setParentTriggerScript(mpParentTriggerScript);
      pCondition->setParentTrigger(this);
      mConditions.add(pCondition);
      if (!pCondition->load(pStream, saveType))
         return false;
   }

   uint8 effectCount;
   GFREADVAR(pStream, uint8, effectCount);
   GFVERIFYCOUNT(effectCount, 200);
   mEffectsOnTrue.reserve(effectCount);
   for (uint8 i=0; i<effectCount; i++)
   {
      BTriggerEffect* pEffect = BTriggerEffect::getInstance();
      if (!pEffect)
         return false;
      pEffect->setParentTriggerScript(mpParentTriggerScript);
      mEffectsOnTrue.add(pEffect);
      if (!pEffect->load(pStream, saveType))
         return false;
   }

   GFREADVAR(pStream, uint8, effectCount);
   GFVERIFYCOUNT(effectCount, 200);
   mEffectsOnFalse.reserve(effectCount);
   for (uint8 i=0; i<effectCount; i++)
   {
      BTriggerEffect* pEffect = BTriggerEffect::getInstance();
      if (!pEffect)
         return false;
      pEffect->setParentTriggerScript(mpParentTriggerScript);
      if (!pEffect->load(pStream, saveType))
      {
         BTriggerEffect::releaseInstance(pEffect);
         return false;
      }
      mEffectsOnFalse.add(pEffect);
   }

   GFREADVAR(pStream, DWORD, mActivatedTime);
   GFREADVAR(pStream, DWORD, mNextEvaluateTime );
   GFREADVAR(pStream, DWORD, mEvaluateFrequency);
   GFREADVAR(pStream, DWORD, mEvaluateCount);
   GFREADVAR(pStream, DWORD, mEvaluateLimit);

   GFREADBITBOOL(pStream, mbStartActive);
   GFREADBITBOOL(pStream, mbConditional);
   GFREADBITBOOL(pStream, mbOrConditions);

   return true;
}

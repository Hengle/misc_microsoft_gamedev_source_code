//==============================================================================
// triggereffectai.cpp
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "ai.h"
#include "aifactoid.h"
#include "aitypes.h"
#include "bidMgr.h"
#include "configsGame.h"
#include "kb.h"
#include "player.h"
#include "protosquad.h"
#include "simtypes.h"
#include "team.h"
#include "triggereffect.h"
#include "triggermanager.h"
#include "world.h"
#include "user.h"
//#include "userprofilemanager.h"
#include "humanPlayerAITrackingData.h"

//==============================================================================
//==============================================================================
void BTriggerEffect::teAIGenerateMissionTargets()
{
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIScoreMissionTargets()
{
   enum { cPlayer = 1, cTargetIDs = 2, };

   BAI* pAI = gWorld->getAI(getVar(cPlayer)->asPlayer()->readVar());
   BASSERT(pAI);
   if (!pAI)
      return;
   const BInt32Array& targetIDs = getVar(cTargetIDs)->asIntegerList()->readVar();
   uint numTargetIDs = targetIDs.getSize();
   for (uint i=0; i<numTargetIDs; i++)
      pAI->scoreMissionTarget(targetIDs[i]);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAISortMissionTargets()
{
   enum { cPlayer = 1, cTargetIDs = 2, };

   BTriggerVarIntegerList* pVarTargetList = getVar(cTargetIDs)->asIntegerList();
   BInt32Array targetIDs = pVarTargetList->readVar();
   std::sort(targetIDs.begin(), targetIDs.end(), BMissionTargetScoreSortFunctor());
   pVarTargetList->writeVar(targetIDs);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIMissionLaunch()
{
   uint version = getVersion();
   switch (version)
   {
   case 1:
      teAIMissionLaunchV1();
      break;
   case 2:
      teAIMissionLaunchV2();
      break;
   default:
      BASSERTM(false, "Invalid Version!");
      break;
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIMissionLaunchV1()
{
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIMissionLaunchV2()
{
}




//==============================================================================
//==============================================================================
void BTriggerEffect::teAIMissionCancel()
{
   enum { cMissionID = 1, };
   
   BAIMission* pMission = gWorld->getAIMission(getVar(cMissionID)->asInteger()->readVar());
   if (pMission)
   {
      // mrh - Removing assert for now - and auto cleaning this up I hope.
      //BTRIGGER_ASSERTM(pMission->getNumSquads() == 0, "A mission still containing squads is being destroyed!  Fix the AI script!");
      BEntityIDArray squadsStillInMission = pMission->getSquads();
      uint numSquadsInMission = squadsStillInMission.getSize();
      for (uint i=0; i<numSquadsInMission; i++)
      {
         BSquad* pSquad = gWorld->getSquad(squadsStillInMission[i]);
         if (pSquad)
            pSquad->removeFromAIMission();
      }
   }

   gWorld->deleteAIMission(getVar(cMissionID)->asInteger()->readVar());
}


//==============================================================================
// Get the mission targets
//==============================================================================
void BTriggerEffect::teAIGetMissionTargets()
{
   switch (getVersion())
   {
      case 1:
         teAIGetMissionTargetsV1();
         break;

      case 2:
         teAIGetMissionTargetsV2();
         break;

      case 3:
         teAIGetMissionTargetsV3();
         break;

      case 4:
         teAIGetMissionTargetsV4();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }
}

//==============================================================================
// Get the mission targets
//==============================================================================
void BTriggerEffect::teAIGetMissionTargetsV1()
{
   enum { cPlayer = 1, cMissionTargetIDArray = 2, };
   const BAI* pAI = gWorld->getAI(getVar(cPlayer)->asPlayer()->readVar());
   if (!pAI)
      return;
   const BAIMissionTargetIDArray& targetIDArray = pAI->getMissionTargetIDs();
   uint numTargets = targetIDArray.getSize();

   BInt32Array targetIDsAsInts(numTargets);
   for (uint i=0; i<numTargets; i++)
      targetIDsAsInts[i] = targetIDArray[i];

   getVar(cMissionTargetIDArray)->asIntegerList()->writeVar(targetIDsAsInts);
}

//==============================================================================
// Get the mission targets
//==============================================================================
void BTriggerEffect::teAIGetMissionTargetsV2()
{
   enum 
   { 
      cPlayer = 1, 
      cMissionTargetIDArray = 2, 
      cTargetType = 3,
      cMissionType = 4,
   };

   BInt32Array targetIDsAsInts;

   const BAI* pAI = gWorld->getAI(getVar(cPlayer)->asPlayer()->readVar());
   if (pAI)
   {
      const BAIMissionTargetIDArray& targetIDArray = pAI->getMissionTargetIDs();
      uint numTargets = targetIDArray.getSize();         
      for (uint i = 0; i < numTargets; i++)
      {
         targetIDsAsInts.add(targetIDArray[i]);
      }

      bool testTargetType = getVar(cTargetType)->isUsed();
      bool testMissionType = getVar(cMissionType)->isUsed();
      if (testTargetType || testMissionType)
      {
         BAIMissionTargetType missionTargetType = testTargetType ? getVar(cTargetType)->asMissionTargetType()->readVar() : MissionTargetType::cInvalid;
         BAIMissionType missionType = testMissionType ? getVar(cMissionType)->asMissionType()->readVar() : MissionType::cInvalid;
         //int numTargets2 = targetIDsAsInts.getNumber();
         for (int i = (numTargets - 1); i >= 0; i--)
         {
            const BAIMissionTarget* pAIMissionTarget = gWorld->getAIMissionTarget(targetIDsAsInts[i]);
            if (pAIMissionTarget)
            {
               if (testTargetType && !pAIMissionTarget->isTargetType(missionTargetType))
               {
                  targetIDsAsInts.removeIndex(i);
                  continue;
               }

               if (testMissionType && !pAIMissionTarget->isMissionType(missionType))
               {
                  targetIDsAsInts.removeIndex(i);
               }
            }
         }
      }
   }

   getVar(cMissionTargetIDArray)->asIntegerList()->writeVar(targetIDsAsInts);
}


//==============================================================================
// Get the mission targets
//==============================================================================
void BTriggerEffect::teAIGetMissionTargetsV3()
{
   enum 
   { 
      cPlayer = 1, 
      cMissionTargetIDArray = 2, 
      cTargetType = 3,
      cMissionType = 4,
      cIgnoreNonScoringTargets = 5,
   };

   BInt32Array targetIDsAsInts;

   bool ignoreNonScoring = false;
   if (getVar(cIgnoreNonScoringTargets)->isUsed())
      ignoreNonScoring = getVar(cIgnoreNonScoringTargets)->asBool()->readVar();

   const BAI* pAI = gWorld->getAI(getVar(cPlayer)->asPlayer()->readVar());
   if (pAI)
   {
      const BAIMissionTargetIDArray& targetIDArray = pAI->getMissionTargetIDs();
      uint numTargets = targetIDArray.getSize();         
      for (uint i = 0; i < numTargets; i++)
      {
         BAIMissionTarget* pTarget = gWorld->getAIMissionTarget(targetIDArray[i]);
         if (pTarget)
         {
            if ( !pTarget->getFlagAllowScoring() && ignoreNonScoring )
               continue;
            targetIDsAsInts.add(targetIDArray[i]);
         }
      }

      bool testTargetType = getVar(cTargetType)->isUsed();
      bool testMissionType = getVar(cMissionType)->isUsed();
      if (testTargetType || testMissionType)
      {
         BAIMissionTargetType missionTargetType = testTargetType ? getVar(cTargetType)->asMissionTargetType()->readVar() : MissionTargetType::cInvalid;
         BAIMissionType missionType = testMissionType ? getVar(cMissionType)->asMissionType()->readVar() : MissionType::cInvalid;
         //int numTargets2 = targetIDsAsInts.getNumber();
         for (int i = (numTargets - 1); i >= 0; i--)
         {
            const BAIMissionTarget* pAIMissionTarget = gWorld->getAIMissionTarget(targetIDsAsInts[i]);
            if (pAIMissionTarget)
            {
               if (testTargetType && !pAIMissionTarget->isTargetType(missionTargetType))
               {
                  targetIDsAsInts.removeIndex(i);
                  continue;
               }

               if (testMissionType && !pAIMissionTarget->isMissionType(missionType))
               {
                  targetIDsAsInts.removeIndex(i);
               }
            }
         }
      }
   }

   getVar(cMissionTargetIDArray)->asIntegerList()->writeVar(targetIDsAsInts);
}


//==============================================================================
// Get the mission targets
//==============================================================================
void BTriggerEffect::teAIGetMissionTargetsV4()
{
   enum 
   { 
      cPlayer = 1, 
      cMissionTargetIDArray = 2, 
      cTargetType = 3,
      cMissionType = 4,
      cIgnoreNonScoringTargets = 5,
      cIgnoreGaiaBases = 6,
   };

   BInt32Array targetIDsAsInts;

   bool ignoreNonScoring = false;
   if (getVar(cIgnoreNonScoringTargets)->isUsed())
      ignoreNonScoring = getVar(cIgnoreNonScoringTargets)->asBool()->readVar();

   bool ignoreGaiaBases = false;
   if (getVar(cIgnoreGaiaBases)->isUsed())
      ignoreGaiaBases = getVar(cIgnoreGaiaBases)->asBool()->readVar();

   const BAI* pAI = gWorld->getAI(getVar(cPlayer)->asPlayer()->readVar());
   if (pAI)
   {
      const BAIMissionTargetIDArray& targetIDArray = pAI->getMissionTargetIDs();
      uint numTargets = targetIDArray.getSize();         
      for (uint i = 0; i < numTargets; i++)
      {
         BAIMissionTarget* pTarget = gWorld->getAIMissionTarget(targetIDArray[i]);
         if (pTarget)
         {
            if ( !pTarget->getFlagAllowScoring() && ignoreNonScoring )
               continue;
            targetIDsAsInts.add(targetIDArray[i]);
         }
      }

      bool testTargetType = getVar(cTargetType)->isUsed();
      bool testMissionType = getVar(cMissionType)->isUsed();
      if (testTargetType || testMissionType)
      {
         BAIMissionTargetType missionTargetType = testTargetType ? getVar(cTargetType)->asMissionTargetType()->readVar() : MissionTargetType::cInvalid;
         BAIMissionType missionType = testMissionType ? getVar(cMissionType)->asMissionType()->readVar() : MissionType::cInvalid;
         //int numTargets2 = targetIDsAsInts.getNumber();
         for (int i = (numTargets - 1); i >= 0; i--)
         {
            const BAIMissionTarget* pAIMissionTarget = gWorld->getAIMissionTarget(targetIDsAsInts[i]);
            if (pAIMissionTarget)
            {
               if (testTargetType && !pAIMissionTarget->isTargetType(missionTargetType))
               {
                  targetIDsAsInts.removeIndex(i);
                  continue;
               }

               if (ignoreGaiaBases && (missionTargetType == MissionTargetType::cKBBase))
               {
                  const BKBBase* pKBBase = gWorld->getKBBase(pAIMissionTarget->getKBBaseID());
                  if (pKBBase && pKBBase->getPlayerID() == cGaiaPlayer)
                  {
                     targetIDsAsInts.removeIndex(i);
                     continue;
                  }
               }

               if (testMissionType && !pAIMissionTarget->isMissionType(missionType))
               {
                  targetIDsAsInts.removeIndex(i);
                  continue;
               }
            }
         }
      }
   }

   getVar(cMissionTargetIDArray)->asIntegerList()->writeVar(targetIDsAsInts);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIMissionTargetGetScores()
{
   enum { cMissionTargetID = 1, cScoreTotal = 2, cScoreInstance = 3, cScoreClass = 4, cScoreAfford = 5, cScorePermission = 6, };
   BAIMissionTargetID missionTargetID = getVar(cMissionTargetID)->asInteger()->readVar();
   BAIMissionScore score;
   BTriggerVarFloat *pTotal = getVar(cScoreTotal)->asFloat();
   BTriggerVarFloat *pInstance = getVar(cScoreInstance)->asFloat();
   BTriggerVarFloat *pClass = getVar(cScoreClass)->asFloat();
   BTriggerVarFloat *pAfford = getVar(cScoreAfford)->asFloat();
   BTriggerVarFloat *pPermission = getVar(cScorePermission)->asFloat();

   // Get the score for this mission target.
   const BAIMissionTarget* pMissionTarget = gWorld->getAIMissionTarget(missionTargetID);
   if (pMissionTarget)
      score = pMissionTarget->getScore();

   // Stuff the appropriate score subcomponents into output vars.
   // Note that if we were unable to get a valid target, these will all be zero due to the default constructor of the BAIMissionScore class.
   if (pTotal)
      pTotal->writeVar(score.getTotal());
   if (pInstance)
      pInstance->writeVar(score.getInstance());
   if (pClass)
      pClass->writeVar(score.getClass());
   if (pAfford)
      pAfford->writeVar(score.getAfford());
   if (pPermission)
      pPermission->writeVar(score.getPermission());
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAISetScoringParms()
{
   enum { cPlayer = 1, cStartLocation = 2, cSquadList = 3, };
   BAI* pAI = gWorld->getAI(getVar(cPlayer)->asPlayer()->readVar());
   if (!pAI)
      return;
   BAIGlobals* pAIGlobals = pAI->getAIGlobals();
   if (!pAIGlobals)
      return;

   pAIGlobals->clearScoringParms();
//-- FIXING PREFIX BUG ID 2731
   const BTriggerVarVector* pVarVector = getVar(cStartLocation)->asVector();
//--
   if (pVarVector->isUsed())
      pAIGlobals->setScoringStartLoc(pVarVector->readVar());
//-- FIXING PREFIX BUG ID 2732
   const BTriggerVarSquadList* pVarSquadList = getVar(cSquadList)->asSquadList();
//--
   if (pVarSquadList->isUsed())
      pAIGlobals->setScoringSquadList(pVarSquadList->readVar());
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAITopicCreate()
{
   enum
   {
      cPlayer           = 1,
      cTopicName        = 2,
      cTopicBool        = 3,
      cMinTickets       = 4,
      cMaxTickets       = 5,
      cTicketInterval   = 6,
      cCommitmentTime   = 7,
      cServiceInterval  = 8,
      cAITopicID        = 9,
   };

   BTriggerVarBool* pVarTopicBool = getVar(cTopicBool)->asBool();
   BTriggerVarInteger* pVarAITopicID = getVar(cAITopicID)->asInteger();
   BAITopicID topicID = cInvalidAITopicID;
   BAITopic* pAITopic = gWorld->createAITopic(getVar(cPlayer)->asPlayer()->readVar());
   if (pAITopic)
   {
      // Register the topic as a script module.
      pAITopic->setType(AITopicType::cScriptModule);
      pAITopic->setTriggerScriptID(getParentTriggerScript()->getID());
      pAITopic->setTriggerVarID(pVarTopicBool->getID());
      #ifndef BUILD_FINAL
         pAITopic->setName(getVar(cTopicName)->asString()->readVar());
      #endif
      pVarTopicBool->writeVar(false);


      // Set up the tickets.
      int32 minTickets = getVar(cMinTickets)->asInteger()->readVar();
      BTRIGGER_ASSERT(minTickets >= UINT16_MIN && minTickets <= UINT16_MAX);
      int32 maxTickets = getVar(cMaxTickets)->asInteger()->readVar();
      BTRIGGER_ASSERT(maxTickets >= UINT16_MIN && maxTickets <= UINT16_MAX);
      BTRIGGER_ASSERT(minTickets <= maxTickets);
      pAITopic->setMinTickets(static_cast<uint>(minTickets));
      pAITopic->setMaxTickets(static_cast<uint>(maxTickets));
      DWORD ticketInterval = getVar(cTicketInterval)->asTime()->readVar();
      BTRIGGER_ASSERT(ticketInterval > 0);
      pAITopic->setTicketInterval(ticketInterval);
      topicID = pAITopic->getID();
   }

   // Write out the ID.
   pVarAITopicID->writeVar(topicID);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAITopicDestroy()
{
   enum { cTopicID = 1, };
   BAITopicID topicID = getVar(cTopicID)->asInteger()->readVar();
   BAITopic* pTopic = gWorld->getAITopic(topicID);
   if (!pTopic)
      return;

   BTRIGGER_ASSERT(pTopic->isType(AITopicType::cScriptModule));
   BTriggerScript *pScript = gTriggerManager.getTriggerScript(pTopic->getTriggerScriptID());
   BTRIGGER_ASSERT(pScript);
   if (pScript)
   {
      BTriggerVar* pVar = pScript->getTriggerVar(pTopic->getTriggerVarID());
      BTRIGGER_ASSERT(pVar);
      if (pVar)
         pVar->asBool()->writeVar(false);
   }

   BAI* pAI = gWorld->getAI(pTopic->getPlayerID());
   BASSERT(pAI);
   if (pAI)
   {
      if (pAI->AIQGetTopic() == topicID)
         pAI->AIQSetTopic(cInvalidAITopicID);
   }
   gWorld->deleteAITopic(topicID);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAITopicModifyTickets()
{
   enum { cTopicID = 1, cMinTickets = 2, cMaxTickets = 3, cTicketInterval = 4, };
//-- FIXING PREFIX BUG ID 2733
   const BTriggerVarInteger* pMinTickets = getVar(cMinTickets)->asInteger();
//--
//-- FIXING PREFIX BUG ID 2734
   const BTriggerVarInteger* pMaxTickets = getVar(cMaxTickets)->asInteger();
//--
//-- FIXING PREFIX BUG ID 2735
   const BTriggerVarTime* pTicketInterval = getVar(cTicketInterval)->asTime();
//--

   BAITopic* pTopic = gWorld->getAITopic(getVar(cTopicID)->asInteger()->readVar());
   if (!pTopic)
      return;

   uint minTickets = pTopic->getMinTickets();
   uint maxTickets = pTopic->getMaxTickets();

   if (pMinTickets->isUsed())
   {
      int32 minVal = pMinTickets->readVar();
      BTRIGGER_ASSERT(minVal >= UINT16_MIN && minVal <= UINT16_MAX);
      minTickets = static_cast<uint>(minVal);
   }
   if (pMaxTickets->isUsed())
   {
      int32 maxVal = pMaxTickets->readVar();
      BTRIGGER_ASSERT(maxVal >= UINT16_MIN && maxVal <= UINT16_MAX);
      maxTickets = static_cast<uint>(maxVal);
   }
   BTRIGGER_ASSERT(minTickets <= maxTickets);
   pTopic->setMinTickets(minTickets);
   pTopic->setMaxTickets(maxTickets);
   if (pTicketInterval->isUsed())
   {
      DWORD ticketInterval = pTicketInterval->readVar();
      BTRIGGER_ASSERT(ticketInterval > 0);
      pTopic->setTicketInterval(ticketInterval);
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIMissionModifyTickets()
{
   enum { cAIMissionID = 1, cMinTickets = 2, cMaxTickets = 3, cTicketInterval = 4, };
//-- FIXING PREFIX BUG ID 2737
   const BAIMission* pAIMission = gWorld->getAIMission(getVar(cAIMissionID)->asInteger()->readVar());
//--
   if (!pAIMission)
      return;
   BAITopic* pAITopic = gWorld->getAITopic(pAIMission->getAITopicID());
   if (!pAITopic)
      return;
   // Note that modifying tickets only makes sense for missions with AIQ topics.

   uint minTickets = pAITopic->getMinTickets();
   uint maxTickets = pAITopic->getMaxTickets();

//-- FIXING PREFIX BUG ID 2738
   const BTriggerVarInteger* pMinTickets = getVar(cMinTickets)->asInteger();
//--
   if (pMinTickets->isUsed())
   {
      int32 minVal = pMinTickets->readVar();
      BTRIGGER_ASSERT(minVal >= UINT16_MIN && minVal <= UINT16_MAX);
      minTickets = static_cast<uint>(minVal);
   }

//-- FIXING PREFIX BUG ID 2739
   const BTriggerVarInteger* pMaxTickets = getVar(cMaxTickets)->asInteger();
//--
   if (pMaxTickets->isUsed())
   {
      int32 maxVal = pMaxTickets->readVar();
      BTRIGGER_ASSERT(maxVal >= UINT16_MIN && maxVal <= UINT16_MAX);
      maxTickets = static_cast<uint>(maxVal);
   }
   BTRIGGER_ASSERT(minTickets <= maxTickets);
   pAITopic->setMinTickets(minTickets);
   pAITopic->setMaxTickets(maxTickets);

//-- FIXING PREFIX BUG ID 2740
   const BTriggerVarTime* pTicketInterval = getVar(cTicketInterval)->asTime();
//--
   if (pTicketInterval->isUsed())
   {
      DWORD ticketInterval = pTicketInterval->readVar();
      BTRIGGER_ASSERT(ticketInterval > 0);
      pAITopic->setTicketInterval(ticketInterval);
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIMissionAddSquads()
{
   enum { cAIMission = 1, cAddSquad = 2, cAddSquads = 3, };
//-- FIXING PREFIX BUG ID 2741
   const BTriggerVarSquad* pVarSquad = getVar(cAddSquad)->asSquad();
//--
//-- FIXING PREFIX BUG ID 2742
   const BTriggerVarSquadList* pVarSquadList = getVar(cAddSquads)->asSquadList();
//--
   BAIMission* pMission = gWorld->getAIMission(getVar(cAIMission)->asInteger()->readVar());
   if (!pMission)
      return;

   // the BAIMission::addSquad/s() call does all the squad validity checking... so don't dupe it here.
   if (pVarSquad->isUsed())
      pMission->addSquad(pVarSquad->readVar());
   if (pVarSquadList->isUsed())
      pMission->addSquads(pVarSquadList->readVar());
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIMissionRemoveSquads()
{
   enum { cAIMission = 1, cRemoveSquad = 2, cRemoveSquads = 3, };
//-- FIXING PREFIX BUG ID 2743
   const BTriggerVarSquad* pVarSquad = getVar(cRemoveSquad)->asSquad();
//--
//-- FIXING PREFIX BUG ID 2744
   const BTriggerVarSquadList* pVarSquadList = getVar(cRemoveSquads)->asSquadList();
//--
   BAIMission* pMission = gWorld->getAIMission(getVar(cAIMission)->asInteger()->readVar());
   if (!pMission)
      return;

   // the BAIMission::removeSquad/s() call does all the squad validity checking... so don't dupe it here.
   if (pVarSquad->isUsed())
      pMission->removeSquad(pVarSquad->readVar());
   if (pVarSquadList->isUsed())
      pMission->removeSquads(pVarSquadList->readVar());
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIMissionGetSquads()
{
   uint version = getVersion();
   switch (version)
   {
   case 1:
      teAIMissionGetSquadsV1();
      break;

   case 2:
      teAIMissionGetSquadsV2();
      break;

   default:
      BTRIGGER_ASSERTM(false, "This version is not supported!");
      break;
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIMissionGetSquadsV1()
{
   enum { cAIMission = 1, cContainedSquads = 2, };
   BTriggerVarSquadList* pContainedSquads = getVar(cContainedSquads)->asSquadList();
   BAIMission* pMission = gWorld->getAIMission(getVar(cAIMission)->asInteger()->readVar());
   if (pMission)
      pContainedSquads->writeVar(pMission->getSquads());
   else
      pContainedSquads->clear();
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIMissionGetSquadsV2()
{
   enum { cAIMission = 1, cSquads = 2, cSquadCount = 3, };
   BTriggerVarSquadList* pSquads = getVar(cSquads)->asSquadList();
   BTriggerVarInteger* pSquadCount = getVar(cSquadCount)->asInteger();
   BAIMission* pMission = gWorld->getAIMission(getVar(cAIMission)->asInteger()->readVar());
   if (pMission)
   {
      if (pSquads->isUsed())
         pSquads->writeVar(pMission->getSquads());
      if (pSquadCount->isUsed())
         pSquadCount->writeVar(pMission->getSquads().getSize());
   }
   else
   {
      if (pSquads->isUsed())
         pSquads->clear();
      if (pSquadCount->isUsed())
         pSquadCount->writeVar(0);
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIScnMissionAttackArea()
{
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIScnMissionDefendArea()
{
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIGetMissions()
{
   enum { cPlayer = 1, cMissionType = 2, cMissionIDs = 3, cMissionCount = 4, };
//-- FIXING PREFIX BUG ID 2748
   const BTriggerVarMissionType* pVarMissionType = getVar(cMissionType)->asMissionType();
//--
   BTriggerVarIntegerList* pVarMissionIDs = getVar(cMissionIDs)->asIntegerList();
   BTriggerVarInteger* pVarMissionCount = getVar(cMissionCount)->asInteger();

   BAIMissionIDArray missionIDs;

//-- FIXING PREFIX BUG ID 2747
   const BAI* pAI = gWorld->getAI(getVar(cPlayer)->asPlayer()->readVar());
//--
   if (pAI)
   {
      if (pVarMissionType->isUsed())
         pAI->getMissionIDsByType(pVarMissionType->readVar(), missionIDs);
      else
         missionIDs = pAI->getMissionIDs();
   }

   uint numMissions = missionIDs.getSize();
   if (pVarMissionIDs->isUsed())
   {

      BInt32Array missionIDsAsInts(numMissions);
      for (uint i=0; i<numMissions; i++)
         missionIDsAsInts[i] = missionIDs[i];

      pVarMissionIDs->writeVar(missionIDsAsInts);
   }
   if (pVarMissionCount->isUsed())
   {
      pVarMissionCount->writeVar(numMissions);
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIMissionGetLaunchScores()
{
   enum { cMissionID = 1, cScoreTotal = 2, cScoreInstance = 3, cScoreClass = 4, cScoreAfford = 5, cScorePermission = 6, };
   BAIMissionID missionID = getVar(cMissionID)->asInteger()->readVar();
   BAIMissionScore score;
   BTriggerVarFloat* pTotal = getVar(cScoreTotal)->asFloat();
   BTriggerVarFloat* pInstance = getVar(cScoreInstance)->asFloat();
   BTriggerVarFloat* pClass = getVar(cScoreClass)->asFloat();
   BTriggerVarFloat* pAfford = getVar(cScoreAfford)->asFloat();
   BTriggerVarFloat* pPermission = getVar(cScorePermission)->asFloat();

   // Get the score for this mission target.
   const BAIMission* pMission = gWorld->getAIMission(missionID);
   if (pMission)
      score = pMission->getLaunchScores();

   // Stuff the appropriate score subcomponents into output vars.
   // Note that if we were unable to get a valid target, these will all be zero due to the default constructor of the BAIMissionScore class.
   if (pTotal)
      pTotal->writeVar(score.getTotal());
   if (pInstance)
      pInstance->writeVar(score.getInstance());
   if (pClass)
      pClass->writeVar(score.getClass());
   if (pAfford)
      pAfford->writeVar(score.getAfford());
   if (pPermission)
      pPermission->writeVar(score.getPermission());
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIRemoveFromMissions()
{
   enum { cSquadList = 1, };
   const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
   uint numSquads = squadList.getSize();
   for (uint i=0; i<numSquads; i++)
   {
      BSquad* pSquad = gWorld->getSquad(squadList[i]);
      if (pSquad)
         pSquad->removeFromAIMission();
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teGetBidsMatching()
{
   if (mVersion == 1)
   {
      teGetBidsMatchingV1();
   }
   else if (mVersion == 2)
   {
      teGetBidsMatchingV2();
   }
   else
   {
      BFAIL("Unsupported version!");
   }

}

//==============================================================================
//==============================================================================
void BTriggerEffect::teGetBidsMatchingV1()
{
   enum { cPlayerID = 1, cProtoObject = 2, cProtoSquad = 3, cTech = 4, cBidList = 5, cBidCount = 6, };
   BTriggerVarIntegerList* pBidList = getVar(cBidList)->asIntegerList();
   BTriggerVarInteger* pBidCount = getVar(cBidCount)->asInteger();
   const BAI* pAI = gWorld->getAI(getVar(cPlayerID)->asPlayer()->readVar());
   const BBidManager* pBidManager = pAI ? pAI->getBidManager() : NULL;
   if (!pBidManager)
   {
      if (pBidList->isUsed())
         pBidList->clear();
      if (pBidCount->isUsed())
         pBidCount->writeVar(0);
      return;
   }

//-- FIXING PREFIX BUG ID 2749
   const BTriggerVarProtoObject* pVarBuilding = getVar(cProtoObject)->asProtoObject();
//--
//-- FIXING PREFIX BUG ID 2750
   const BTriggerVarProtoSquad* pVarSquad = getVar(cProtoSquad)->asProtoSquad();
//--
//-- FIXING PREFIX BUG ID 2751
   const BTriggerVarTech* pVarTech = getVar(cTech)->asTech();
//--

   BInt32Array matchingBidIDs;
   int32 matchingBidCount = 0;

   const BBidIDArray& bidIDs = pBidManager->getBidIDs();
   uint numBids = bidIDs.getSize();
   for (uint i=0; i<numBids; i++)
   {
      const BBid* pBid = gWorld->getBid(bidIDs[i]);
      if (!pBid)
         continue;
      if (pVarBuilding->isUsed() && pVarBuilding->readVar() != pBid->getBuildingToBuy())
         continue;
      if (pVarSquad->isUsed() && pVarSquad->readVar() != pBid->getSquadToBuy())
         continue;
      if (pVarTech->isUsed() && pVarTech->readVar() != pBid->getTechToBuy())
         continue;

      matchingBidIDs.add(bidIDs[i]);
      matchingBidCount++;
   }

   if (pBidList->isUsed())
      pBidList->writeVar(matchingBidIDs);
   if (pBidCount->isUsed())
      pBidCount->writeVar(matchingBidCount);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teGetBidsMatchingV2()
{
   enum { cPlayerID = 1, cProtoObject = 2, cProtoSquad = 3, cTech = 4, cBidList = 5, cBidCount = 6, cProtoPower = 7, };
   BTriggerVarIntegerList* pBidList = getVar(cBidList)->asIntegerList();
   BTriggerVarInteger* pBidCount = getVar(cBidCount)->asInteger();
   const BAI* pAI = gWorld->getAI(getVar(cPlayerID)->asPlayer()->readVar());
   const BBidManager* pBidManager = pAI ? pAI->getBidManager() : NULL;
   if (!pBidManager)
   {
      if (pBidList->isUsed())
         pBidList->clear();
      if (pBidCount->isUsed())
         pBidCount->writeVar(0);
      return;
   }
//-- FIXING PREFIX BUG ID 2752
   const BTriggerVarProtoObject* pVarBuilding = getVar(cProtoObject)->asProtoObject();
//--
//-- FIXING PREFIX BUG ID 2753
   const BTriggerVarProtoSquad* pVarSquad = getVar(cProtoSquad)->asProtoSquad();
//--
//-- FIXING PREFIX BUG ID 2754
   const BTriggerVarTech* pVarTech = getVar(cTech)->asTech();
//--
//-- FIXING PREFIX BUG ID 2755
   const BTriggerVarPower* pVarPower = getVar(cProtoPower)->asPower();
//--

   BInt32Array matchingBidIDs;
   int32 matchingBidCount = 0;

   const BBidIDArray& bidIDs = pBidManager->getBidIDs();
   uint numBids = bidIDs.getSize();
   for (uint i=0; i<numBids; i++)
   {
      const BBid* pBid = gWorld->getBid(bidIDs[i]);
      if (!pBid)
         continue;
      if (pVarBuilding->isUsed() && pVarBuilding->readVar() != pBid->getBuildingToBuy())
         continue;
      if (pVarSquad->isUsed() && pVarSquad->readVar() != pBid->getSquadToBuy())
         continue;
      if (pVarTech->isUsed() && pVarTech->readVar() != pBid->getTechToBuy())
         continue;
      if (pVarPower->isUsed() && pVarPower->readVar() != pBid->getPowerToCast())
         continue;

      matchingBidIDs.add(bidIDs[i]);
      matchingBidCount++;
   }

   if (pBidList->isUsed())
      pBidList->writeVar(matchingBidIDs);
   if (pBidCount->isUsed())
      pBidCount->writeVar(matchingBidCount);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIMissionGetTarget()
{
   enum { cMissionID = 1, cMissionTarget = 2, };
   BAIMissionID missionID = getVar(cMissionID)->asInteger()->readVar();
   BAIMissionTargetID targetID = cInvalidAIMissionTargetID;
   const BAIMission* pMission = gWorld->getAIMission(missionID);
   if (pMission)
      targetID = pMission->getFinalTargetWrapperTargetID();   
   const BAIMissionTarget* pTarget = gWorld->getAIMissionTarget(targetID);
   //BTRIGGER_ASSERTM(pTarget, "Invalid mission target!");
   if (!pTarget)
      targetID = cInvalidAIMissionTargetID;
   //BTRIGGER_ASSERTM(targetID != cInvalidAIMissionTargetID, "Invalid mission target ID being returned!");
   getVar(cMissionTarget)->asInteger()->writeVar(targetID);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teBidSetTargetLocation()
{
   enum { cBidID = 1, cTargetLocation = 2, };
   BBidID bidID = getVar(cBidID)->asInteger()->readVar();
   BBid* pBid = gWorld->getBid(bidID);
   if (!pBid)
      return;
//-- FIXING PREFIX BUG ID 2756
   const BTriggerVarVector* pTargetLocation = getVar(cTargetLocation)->asVector();
//--
   if (pTargetLocation->isUsed())
      pBid->setTargetLocation(pTargetLocation->readVar());
   else
      pBid->setUseTargetLocation(false);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teBidSetBuilder()
{
   enum { cBidID = 1, cBuilderUnit = 2, };
   BBidID bidID = getVar(cBidID)->asInteger()->readVar();
   BBid* pBid = gWorld->getBid(bidID);
   if (!pBid)
      return;
//-- FIXING PREFIX BUG ID 2757
   const BTriggerVarUnit* pBuilderUnit = getVar(cBuilderUnit)->asUnit();
//--
   if (pBuilderUnit->isUsed())
      pBid->setBuilderID(pBuilderUnit->readVar());
   else
      pBid->setUseBuilderID(false);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAITopicLotto()
{
   enum { cPlayer = 1, };
   BAI* pAI = gWorld->getAI(getVar(cPlayer)->asPlayer()->readVar());
   if (pAI)
      pAI->AIQTopicLotto(gWorld->getGametime());
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAICalculations()
{
   enum
   {
      cCalculationNumber        = 5,  //pick which calculation to run 1-3 
      cSquadList                = 1, 
      cKBSquadList              = 2, 
      cUnarmored                = 4,
      cVehicle                  = 6,
      cAir                      = 7,
      cBuilding                 = 8,
   };

   if(getVar(cSquadList)->isUsed())
   {

   }
   if(getVar(cKBSquadList)->isUsed())
   {

   }
   //This handles "calculations" 1-3


   //int calculationNumber = getVar(cCalculationNumber)->asInteger()->readVar();

   //TODO :  Run the specific calculation and return 4 floats.


}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAICalculations2()
{
   enum
   {
      cSquadListA               = 1, 
      cKBSquadListA             = 2, 
      cSquadListB               = 3, 
      cKBSquadListB             = 4, 
      cAvsB                     = 5,
      cBvsA                     = 6, 
   };
   //This handles "calculation" 4
   //   calculations 5-6 are just guesses, and we can get them easily from 4 in the script

   //Input: All parameters are optional, but we need at least one entry from A and one from B (we can constrain this to matching if that make things easier

   //Output:  AvsB   is A team's damage outputs (calculation 1)   weighted by B team's %build out.   If B has 0 air, then A's air damage counts 0
   //         BvsA   same thing, other direction
   //  these are optional, to avoid extra calculation if we only want one or the other.

   //

}

//==============================================================================
//==============================================================================
void BTriggerEffect::teAIAnalyzeSquadList()
{
   enum { cSquadList = 1, cSquadAnalysis = 2, };
   const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
   BAISquadAnalysis sa;
   BAI::calculateSquadAnalysis(squadList, sa);
   getVar(cSquadAnalysis)->asAISquadAnalysis()->writeVar(sa);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIAnalyzeOffenseAToB()
{
   enum { cAnalysisA = 1, cAnalysisB = 2, cOffenseAToB = 3, };
   const BAISquadAnalysis& analysisA = getVar(cAnalysisA)->asAISquadAnalysis()->readVar();
   const BAISquadAnalysis& analysisB = getVar(cAnalysisB)->asAISquadAnalysis()->readVar();
   float offenseAToB = BAI::calculateOffenseAToB(analysisA, analysisB);
   getVar(cOffenseAToB)->asFloat()->writeVar(offenseAToB);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAISAGetComponent()
{
   enum { cSquadAnalysis = 1, cSquadAnalysisComponent = 2, cComponent = 3, };
   const BAISquadAnalysis& squadAnalysis = getVar(cSquadAnalysis)->asAISquadAnalysis()->readVar();
   BAISquadAnalysisComponent sac = getVar(cSquadAnalysisComponent)->asAISquadAnalysisComponent()->readVar();
   float component = squadAnalysis.getComponent(sac);
   getVar(cComponent)->asFloat()->writeVar(component);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIAnalyzeKBSquadList()
{
   enum { cKBSquadList = 1, cSquadAnalysis = 2, };
   const BKBSquadIDArray& kbSquadList = getVar(cKBSquadList)->asKBSquadList()->readVar();
   BAISquadAnalysis sa;
   BAI::calculateSquadAnalysis(kbSquadList, sa);
   getVar(cSquadAnalysis)->asAISquadAnalysis()->writeVar(sa);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIAnalyzeProtoSquadList()
{
   enum { cProtoSquadList = 1, cPlayer = 2, cSquadAnalysis = 3, };
   const BProtoSquadIDArray& protoSquadList = getVar(cProtoSquadList)->asProtoSquadList()->readVar();
   BPlayerID playerID = getVar(cPlayer)->asPlayer()->readVar();
   BAISquadAnalysis sa;
   BAI::calculateSquadAnalysis(protoSquadList, playerID, sa);
   getVar(cSquadAnalysis)->asAISquadAnalysis()->writeVar(sa);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIUnopposedTimeToKill()
{
   enum { cAttackerAnalysis = 1, cTargetAnalysis = 2, cMaxTime = 3, cResultTime = 4, };
   const BAISquadAnalysis& attackerAnalysis = getVar(cAttackerAnalysis)->asAISquadAnalysis()->readVar();
   const BAISquadAnalysis& targetAnalysis = getVar(cTargetAnalysis)->asAISquadAnalysis()->readVar();
   DWORD maxTime = getVar(cMaxTime)->asTime()->readVar();
   float maxTimeAsFloat = maxTime * 0.001f;
   float timeToKill = BAI::calculateUnopposedTimeToKill(attackerAnalysis, targetAnalysis, maxTimeAsFloat);
   DWORD resultTime = static_cast<DWORD>(timeToKill * 1000.0f);
   getVar(cResultTime)->asTime()->writeVar(resultTime);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAISetFocus()
{
   uint version = getVersion();
   switch (version)
   {
   case 2:
      {
         teAISetFocusV2();
         break;
      }
   default:
      {
         BASSERTM(false, "Invalid version!");
      }
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAISetFocusV2()
{
   enum { cPlayer = 1, cFocusPos = 2, cFocusUnit = 3, cFocusSquad = 4, };
   BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
   if (!pPlayer)
      return;

   BTriggerVarVector *pFocusPos = getVar(cFocusPos)->asVector();
   if (pFocusPos->isUsed())
   {
      pPlayer->setLookAtPos(pFocusPos->readVar());
      return;
   }

//-- FIXING PREFIX BUG ID 2759
   const BTriggerVarSquad* pFocusSquad = getVar(cFocusSquad)->asSquad();
//--
   if (pFocusSquad->isUsed())
   {
      BEntityID entityID = pFocusSquad->readVar();
      const BSquad* pSquad = gWorld->getSquad(entityID);
      if (pSquad)
      {
         pPlayer->setLookAtPos(pSquad->getPosition());
         return;
      }
   }

//-- FIXING PREFIX BUG ID 2760
   const BTriggerVarUnit* pFocusUnit = getVar(cFocusUnit)->asUnit();
//--
   if (pFocusUnit->isUsed())
   {
      BEntityID entityID = pFocusUnit->readVar();
      const BUnit* pUnit = gWorld->getUnit(entityID);
      if (pUnit)
      {
         pPlayer->setLookAtPos(pUnit->getPosition());
         return;
      }
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIBindLog()
{
   switch (getVersion())
   {
   case 1:
      teAIBindLogV1();
      break;

   case 2:
      teAIBindLogV2();
      break;

   case 3:
      teAIBindLogV3();
      break;

   default:
      BTRIGGER_ASSERTM(false, "This version is not supported!");
      break;
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teAIBindLogV1()
{
   enum { cInputPlayer=3, };

   if (gConfig.isDefined("aiLog"))
   {
      int playerID = getVar(cInputPlayer)->asPlayer()->readVar();
      getParentTriggerScript()->mPlayerID = playerID;
   }
   else
   {
      getParentTriggerScript()->mPlayerID = -1;
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teAIBindLogV2()
{
   enum { cInputPlayer=3, cTraceOn=4,  };

   if (gConfig.isDefined("aiLog"))
   {
      int playerID = getVar(cInputPlayer)->asPlayer()->readVar();

      bool traceon = (getVar(cTraceOn)->isUsed())?getVar(cTraceOn)->asBool()->readVar():true;
      if(traceon == false)
      {
         playerID = -1;
      }

      getParentTriggerScript()->mPlayerID = playerID;
   }
   else
   {
      getParentTriggerScript()->mPlayerID = -1;
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teAIBindLogV3()
{
   enum { cInputPlayer=3, cTraceOn=4, cGroupToLog=5, };

   if (gConfig.isDefined("aiLog"))
   {
      int playerID = getVar(cInputPlayer)->asPlayer()->readVar();
      bool traceon = (getVar(cTraceOn)->isUsed())?getVar(cTraceOn)->asBool()->readVar():true;
      if(traceon == false)
      {
         playerID = -1;
      }

      long groupToLog;

      if(getVar(cGroupToLog)->isUsed())
      {
         groupToLog = (getVar(cGroupToLog)->isUsed())?getVar(cGroupToLog)->asGroup()->readVar():cInvalidTriggerGroupID;

         getParentTriggerScript()->mAILogFilterByGroup = true;
         getParentTriggerScript()->mAILogGroup = groupToLog;
      }
      else
      {
         getParentTriggerScript()->mAILogFilterByGroup = false;
      }

      getParentTriggerScript()->mPlayerID = playerID;
   }
   else
   {
      getParentTriggerScript()->mPlayerID = -1;
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIChat()
{
   enum 
   { 
      cSender=6,
      cPlayerList=5,
      cChatSound = 1, 
      cQueue = 2,
      cChatStringID=3,
      cChatSpeaker=4,
   };

   bool isSoundUsed = getVar(cChatSound)->isUsed();
   bool isStringUsed = getVar(cChatStringID)->isUsed();
   bool isChatSpeakerUsed = getVar(cChatSpeaker)->isUsed();


   BSimString soundCue;
   soundCue.empty();
   bool queueSound = false;
   long stringID = -1;
   int chatSpeaker = -1;

   if (isSoundUsed)
   {
      // play the chat sound
      soundCue = getVar(cChatSound)->asSound()->readVar();
      queueSound = getVar(cQueue)->isUsed() ? getVar(cQueue)->asBool()->readVar() : false;
      // gSoundManager.playCue(soundCue, -1, queue);
   }

   if (isChatSpeakerUsed)
   {
      chatSpeaker = getVar(cChatSpeaker)->asChatSpeaker()->readVar();
   }

   if (isStringUsed)
   {
      stringID = getVar(cChatStringID)->asLocStringID()->readVar();
   }

   const BPlayerIDArray& playerList = getVar(cPlayerList)->asPlayerList()->readVar();
   bool allPlayers = false;

   float duration=10.0f;

   // queue this chat up.
   gWorld->getChatManager()->addChat(stringID, soundCue, queueSound, allPlayers, chatSpeaker, playerList, duration);
}


//==============================================================================
// BTriggerEffect::teAIMissionTargetGetLocation()
//==============================================================================
void BTriggerEffect::teAIMissionTargetGetLocation()
{
   enum 
   { 
      cMissionTargetID = 1, 
      cLocation = 2, 
      cRadius = 3, 
   };

   BVector location = cInvalidVector;
   float radius = -1.0f;
   const BAIMissionTarget* pTarget = gWorld->getAIMissionTarget(getVar(cMissionTargetID)->asInteger()->readVar());
   //BTRIGGER_ASSERTM(pTarget, "Invalid mission target!");
   BTriggerVarVector* pLocation = getVar(cLocation)->asVector();
   if (pLocation->isUsed())
   {
      if (pTarget)
         location = pTarget->getPosition();
      //BTRIGGER_ASSERTM(!location.almostEqual(cInvalidVector), "Invalid mission target location!");
      pLocation->writeVar(location);
   }

   BTriggerVarFloat* pRadius = getVar(cRadius)->asFloat();
   if (pRadius->isUsed())
   {
      if (pTarget)
         radius = pTarget->getRadius();
      //BTRIGGER_ASSERTM(radius >= 0.0f, "Invalid mission target radius!");
      pRadius->writeVar(radius);
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIGetFlareAlerts()
{
   enum { cPlayer = 1, cAlerts = 2, cNumAlerts = 3, };
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
   BTriggerVarIntegerList* pList = getVar(cAlerts)->asIntegerList();
   BTriggerVarInteger* pCount = getVar(cNumAlerts)->asInteger();
   if (pPlayer)
   {
      BAlertIDArray flareAlerts;
      uint numAlerts = pPlayer->getAlertManager()->getFlareAlertIDs(flareAlerts);
      if (pList->isUsed())
      {
         pList->clear();
         for (uint i=0; i<numAlerts; i++)
            pList->addInt(flareAlerts[i]);
      }
      if (pCount->isUsed())
      {
         pCount->writeVar(flareAlerts.getSize());
      }
   }
   else
   {
      if (pList->isUsed())
         pList->clear();
      if (pCount->isUsed())
         pCount->writeVar(0);
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIGetAttackAlerts()
{
   enum { cPlayer = 1, cAlerts = 2, cNumAlerts = 3, };
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
   BTriggerVarIntegerList* pList = getVar(cAlerts)->asIntegerList();
   BTriggerVarInteger* pCount = getVar(cNumAlerts)->asInteger();
   if (pPlayer)
   {
      BAlertIDArray attackAlerts;
      uint numAlerts = pPlayer->getAlertManager()->getAttackAlertIDs(attackAlerts);
      if (pList->isUsed())
      {
         pList->clear();
         for (uint i=0; i<numAlerts; i++)
            pList->addInt(attackAlerts[i]);
      }
      if (pCount->isUsed())
      {
         pCount->writeVar(attackAlerts.getSize());
      }
   }
   else
   {
      if (pList->isUsed())
         pList->clear();
      if (pCount->isUsed())
         pCount->writeVar(0);
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIGetLastFlareAlert()
{
   enum { cPlayer = 1, cAlert = 2, };
   BAlertID lastFlareAlert = cInvalidAlertID;
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
   if (pPlayer)
      lastFlareAlert = pPlayer->getAlertManager()->getLastAllyFlareAlertID();
   getVar(cAlert)->asInteger()->writeVar(lastFlareAlert);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIGetLastAttackAlert()
{
   enum { cPlayer = 1, cAlert = 2, };
   BAlertID lastAttackAlert = cInvalidAlertID;
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
   if (pPlayer)
      lastAttackAlert = pPlayer->getAlertManager()->getLastAttackAlertID();
   getVar(cAlert)->asInteger()->writeVar(lastAttackAlert);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIGetAlertData()
{
   switch (getVersion())
   {
   case 1:
      teAIGetAlertDataV1();
      break;

   case 2:
      teAIGetAlertDataV2();
      break;

   default:
      BTRIGGER_ASSERTM(false, "This version is not supported!");
      break;
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIGetAlertDataV1()
{
   enum { cAlert = 1, cLocation = 2, cFlaringPlayer = 3, cCreatedTime = 4, cExpiresTime = 5, };
   BAlertID alertID = getVar(cAlert)->asInteger()->readVar();
   if (!alertID.isValid())
      return;
   const BPlayer* pPlayer = gWorld->getPlayer(alertID.getPlayerID());
   if (!pPlayer)
      return;
   const BAlert* pAlert = pPlayer->getAlertManager()->getAlert(alertID);
   if (!pAlert)
      return;

   BTriggerVarVector* pLocation = getVar(cLocation)->asVector();
   if (pLocation->isUsed())
      pLocation->writeVar(pAlert->getLocation());

   BTriggerVarPlayer* pFlaringPlayer = getVar(cFlaringPlayer)->asPlayer();
   if (pFlaringPlayer->isUsed())
      pFlaringPlayer->writeVar(pAlert->getFlaringPlayerID());

   BTriggerVarTime* pCreatedTime = getVar(cCreatedTime)->asTime();
   if (pCreatedTime->isUsed())
      pCreatedTime->writeVar(pAlert->getCreateTime());

   BTriggerVarTime* pExpiresTime = getVar(cExpiresTime)->asTime();
   if (pExpiresTime->isUsed())
      pExpiresTime->writeVar(pAlert->getExpireTime());
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teAIGetAlertDataV2()
{
   enum { cAlert = 1, cLocation = 2, cFlaringPlayer = 3, cCreatedTime = 4, cExpiresTime = 5, cWasUnitClicked = 6, cClickedUnit = 7};
   BAlertID alertID = getVar(cAlert)->asInteger()->readVar();
   if (!alertID.isValid())
      return;
   const BPlayer* pPlayer = gWorld->getPlayer(alertID.getPlayerID());
   if (!pPlayer)
      return;
   const BAlert* pAlert = pPlayer->getAlertManager()->getAlert(alertID);
   if (!pAlert)
      return;

   BTriggerVarVector* pLocation = getVar(cLocation)->asVector();
   if (pLocation->isUsed())
      pLocation->writeVar(pAlert->getLocation());

   BTriggerVarPlayer* pFlaringPlayer = getVar(cFlaringPlayer)->asPlayer();
   if (pFlaringPlayer->isUsed())
      pFlaringPlayer->writeVar(pAlert->getFlaringPlayerID());

   BTriggerVarTime* pCreatedTime = getVar(cCreatedTime)->asTime();
   if (pCreatedTime->isUsed())
      pCreatedTime->writeVar(pAlert->getCreateTime());

   BTriggerVarTime* pExpiresTime = getVar(cExpiresTime)->asTime();
   if (pExpiresTime->isUsed())
      pExpiresTime->writeVar(pAlert->getExpireTime());

//-- FIXING PREFIX BUG ID 2762
   const BUnit* pUnit = gWorld->getUnit(pAlert->getUnitID());
//--
   if(pUnit == NULL)
   {
      if(getVar(cWasUnitClicked)->isUsed())
      {
         getVar(cWasUnitClicked)->asBool()->writeVar(false);
      }
      if(getVar(cClickedUnit)->isUsed())
      {
         getVar(cClickedUnit)->asUnit()->writeVar(cInvalidObjectID);

      }
   }
   else
   {
      if(getVar(cWasUnitClicked)->isUsed())
      {
         getVar(cWasUnitClicked)->asBool()->writeVar(true);
      }
      if(getVar(cClickedUnit)->isUsed())
      {
         getVar(cClickedUnit)->asUnit()->writeVar(pAlert->getUnitID());   
      }
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAISetBiases()
{
   enum { cOffense = 1, cNodes = 2, cMainBases = 3, cHatedEnemy = 4, 
      cOKToAttack = 5, cOKToDefend = 6, cResetToDefaults = 7, cPlayerID = 8, };
   BAI* pAI = gWorld->getAI(getVar(cPlayerID)->asPlayer()->readVar());
   if (!pAI)
      return;
   BAIGlobals* pAIGlobals = pAI->getAIGlobals();
   if (!pAIGlobals)
      return;

   if (getVar(cOffense)->isUsed())
      pAIGlobals->setBiasOffense(getVar(cOffense)->asFloat()->readVar());
   if (getVar(cNodes)->isUsed())
      pAIGlobals->setBiasNodes(getVar(cNodes)->asFloat()->readVar());
   if (getVar(cMainBases)->isUsed())
      pAIGlobals->setBiasMainBases(getVar(cMainBases)->asFloat()->readVar());
   if (getVar(cHatedEnemy)->isUsed())
      pAIGlobals->setHatedPlayer(getVar(cHatedEnemy)->asPlayer()->readVar());
   if (getVar(cOKToAttack)->isUsed())
      pAIGlobals->setOKToAttack(getVar(cOKToAttack)->asBool()->readVar());
   if (getVar(cOKToDefend)->isUsed())
      pAIGlobals->setOKToDefend(getVar(cOKToDefend)->asBool()->readVar());

   if (getVar(cResetToDefaults)->isUsed())
      if (getVar(cResetToDefaults)->asBool()->readVar())
         pAIGlobals->initBiases();
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAITopicPriorityRequest()
{
   enum { cTopicID = 1, };
   BAITopic* pTopic = gWorld->getAITopic(getVar(cTopicID)->asInteger()->readVar());
   if (pTopic)
      pTopic->enablePriorityRequest(gWorld->getGametime());
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIRegisterHook()
{
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAISetAssetMultipliers()
{
   enum 
   { 
      cPlayer = 9,
      cMissionID = 1, 
      cClear = 10,
      cObjectType1 = 2,
      cValue1 = 3,
      cObjectType2 = 4,
      cValue2 = 5,
      cObjectType3 = 6,
      cValue3 = 7,
   };

   BAIScoreModifier* pScoreSettings = NULL;
   BAI* pAI = gWorld->getAI(getVar(cPlayer)->asPlayer()->readVar());
   if (!pAI)
      return;
   //set on mission
   if(getVar(cMissionID)->isUsed())
   {      
      BAIMission* pMission = gWorld->getAIMission(getVar(cMissionID)->asInteger()->readVar());
      if (pMission)
         pScoreSettings = pMission->getScoreModifier();
   }
   //set on ai
   else
   {
      pScoreSettings = pAI->getScoreModifier();
   }

   if(pScoreSettings != NULL)
   {
      if(getVar(cClear)->isUsed() && getVar(cClear)->asBool()->readVar())
      {
         pScoreSettings->mMultipliers.clear();
      }

      BObjectTypeID type;
      float value;
      if(getVar(cObjectType1)->isUsed() && getVar(cValue1)->isUsed())
      {
         type = getVar(cObjectType1)->asObjectType()->readVar();
         value = getVar(cValue1)->asFloat()->readVar();
         pScoreSettings->mMultipliers.add(std::make_pair(type, value));
      }
      if(getVar(cObjectType2)->isUsed() && getVar(cValue2)->isUsed())
      {
         type = getVar(cObjectType2)->asObjectType()->readVar();
         value = getVar(cValue2)->asFloat()->readVar();
         pScoreSettings->mMultipliers.add(std::make_pair(type, value));
      }
      if(getVar(cObjectType3)->isUsed() && getVar(cValue3)->isUsed())
      {
         type = getVar(cObjectType3)->asObjectType()->readVar();
         value = getVar(cValue3)->asFloat()->readVar();
         pScoreSettings->mMultipliers.add(std::make_pair(type, value));
      }
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAISetPlayerMultipliers()
{
   enum 
   { 
      cPlayer = 1,
      cMissionID = 2, 
      cClear = 3,
      cPlayerID1 = 4,
      cValue1 = 5,
      cPlayerID2 = 6,
      cValue2 = 7,
      cPlayerID3 = 8,
      cValue3 = 9,
   };

   BAIPlayerModifier* pPlayerSettings = NULL;
   BAI* pAI = gWorld->getAI(getVar(cPlayer)->asPlayer()->readVar());
   if (!pAI)
      return;
   //set on mission
   if(getVar(cMissionID)->isUsed())
   {      
      BAIMission* pMission = gWorld->getAIMission(getVar(cMissionID)->asInteger()->readVar());
      if (pMission)
         pPlayerSettings = pMission->getPlayerModifier();
   }
   //set on ai
   else
   {
      pPlayerSettings = pAI->getPlayerModifier();
   }

   if(pPlayerSettings != NULL)
   {
      if(getVar(cClear)->isUsed() && getVar(cClear)->asBool()->readVar())
      {
         pPlayerSettings->mMultipliers.clear();
      }

      BPlayerID type;
      float value;
      if(getVar(cPlayerID1)->isUsed() && getVar(cValue1)->isUsed())
      {
         type = getVar(cPlayerID1)->asPlayer()->readVar();
         value = getVar(cValue1)->asFloat()->readVar();
         pPlayerSettings->mMultipliers.add(std::make_pair(type, value));
      }
      if(getVar(cPlayerID2)->isUsed() && getVar(cValue2)->isUsed())
      {
         type = getVar(cPlayerID2)->asPlayer()->readVar();
         value = getVar(cValue2)->asFloat()->readVar();
         pPlayerSettings->mMultipliers.add(std::make_pair(type, value));
      }
      if(getVar(cPlayerID3)->isUsed() && getVar(cValue3)->isUsed())
      {
         type = getVar(cPlayerID3)->asPlayer()->readVar();
         value = getVar(cValue3)->asFloat()->readVar();
         pPlayerSettings->mMultipliers.add(std::make_pair(type, value));
      }
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIGetOpportunityRequests()
{
   enum { cPlayer = 1, cMissionIDs = 2, cMissionCount = 3, };
   BAI* pAI = gWorld->getAI(getVar(cPlayer)->asPlayer()->readVar());
   if (!pAI)
      return;
   BAIGlobals* pAIGlobals = pAI->getAIGlobals();
   if (!pAIGlobals)
      return;

   const BAIMissionIDArray& opportunityRequestMissions = pAIGlobals->getOpportunityRequests();
   uint numOpportunityRequests = opportunityRequestMissions.getSize();
   BInt32Array opportunityRequestsAsInts(numOpportunityRequests);
   for (uint i=0; i<numOpportunityRequests; i++)
      opportunityRequestsAsInts[i] = opportunityRequestMissions[i];

   BTriggerVarIntegerList* pMissionIDs = getVar(cMissionIDs)->asIntegerList();
   if (pMissionIDs->isUsed())
      pMissionIDs->writeVar(opportunityRequestsAsInts);

   BTriggerVarInteger* pMissionCount = getVar(cMissionCount)->asInteger();
   if (pMissionCount->isUsed())
      pMissionCount->writeVar(static_cast<int>(numOpportunityRequests));
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIGetTerminalMissions()
{
   enum { cPlayer = 1, cMissionIDs = 2, cMissionCount = 3, };
   BAI* pAI = gWorld->getAI(getVar(cPlayer)->asPlayer()->readVar());
   if (!pAI)
      return;
   BAIGlobals* pAIGlobals = pAI->getAIGlobals();
   if (!pAIGlobals)
      return;

   const BAIMissionIDArray& terminalMissions = pAIGlobals->getTerminalMissions();
   uint numTerminalMissions = terminalMissions.getSize();
   BInt32Array terminalMissionsAsInts(numTerminalMissions);
   for (uint i=0; i<numTerminalMissions; i++)
      terminalMissionsAsInts[i] = terminalMissions[i];

   BTriggerVarIntegerList* pMissionIDs = getVar(cMissionIDs)->asIntegerList();
   if (pMissionIDs->isUsed())
      pMissionIDs->writeVar(terminalMissionsAsInts);

   BTriggerVarInteger* pMissionCount = getVar(cMissionCount)->asInteger();
   if (pMissionCount->isUsed())
      pMissionCount->writeVar(static_cast<int>(numTerminalMissions));
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIClearOpportunityRequests()
{
   enum { cPlayer = 1, };
   BAI* pAI = gWorld->getAI(getVar(cPlayer)->asPlayer()->readVar());
   if (!pAI)
      return;
   BAIGlobals* pAIGlobals = pAI->getAIGlobals();
   if (pAIGlobals)
      pAIGlobals->clearOpportunityRequests();
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAITopicSetFocus()
{
   enum { cTopicID = 1, };
   BAITopicID topicID = getVar(cTopicID)->asInteger()->readVar();
   BAITopic* pTopic = gWorld->getAITopic(topicID);
   if (!pTopic)
      return;
   BAI* pAI = gWorld->getAI(pTopic->getPlayerID());
   if (pAI)
      pAI->AIQSetTopic(topicID);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIQueryMissionTargets()
{
   enum { cPlayer = 1, cLocation = 2, cRadius = 3, cMissionTargetType = 4, cPassed = 5, cFailed = 6 };
   BAI* pAI = gWorld->getAI(getVar(cPlayer)->asPlayer()->readVar());
   if (!pAI)
      return;

   const BAIMissionTargetIDArray& targetIDArray = pAI->getMissionTargetIDs();
   uint numTargets = targetIDArray.getSize();

   BVector location;
   float radius = 100.0f;
   float radiusSqr = radius * radius;
   bool checkLocation = getVar(cLocation)->isUsed() && getVar(cRadius)->isUsed();
   if(checkLocation)
   {
      location = getVar(cLocation)->asVector()->readVar();
      radius = getVar(cRadius)->asFloat()->readVar();
      radiusSqr = radius * radius;
   }

   bool checkTargetType = getVar(cMissionTargetType)->isUsed();
   BAIMissionTargetType targetType = MissionTargetType::cInvalid;
   if(checkTargetType)
   {
      targetType = getVar(cMissionTargetType)->asMissionTargetType()->readVar();
   }

   BInt32Array passedTargetIDsAsInts;
   BInt32Array failedTargetIDsAsInts;
   for (uint i=0; i<numTargets; i++)
   {
      bool passed = true;

      const BAIMissionTarget* pTarget = gWorld->getAIMissionTarget(targetIDArray[i]);
      if (!pTarget)
         continue;

      if(checkLocation)
      {
         if (pTarget->getPosition().xzDistanceSqr(location) > radiusSqr)
         {
            passed = false;
         }
      }
      if(checkTargetType)
      {
         if(targetType != pTarget->getTargetType())
         {
            passed = false;
         }         
      }

      if(passed)
      {
         passedTargetIDsAsInts.add( targetIDArray[i] );
      }
      else
      {
         failedTargetIDsAsInts.add( targetIDArray[i] );
      }
   }

   if(getVar(cPassed)->isUsed())
   {
      getVar(cPassed)->asIntegerList()->writeVar(passedTargetIDsAsInts);
   }
   if(getVar(cFailed)->isUsed())
   {
      getVar(cFailed)->asIntegerList()->writeVar(failedTargetIDsAsInts);
   }  
}


//==============================================================================
// Calculate the offense ratio of group A to group B
//==============================================================================
void BTriggerEffect::teAICalculateOffenseRatioAToB()
{
   enum 
   { 
      cInAnalysisA = 1, 
      cInAnalysisB = 2, 
      cOutRatioAToB = 3, 
   };

   const BAISquadAnalysis& analysisA = getVar(cInAnalysisA)->asAISquadAnalysis()->readVar();
   const BAISquadAnalysis& analysisB = getVar(cInAnalysisB)->asAISquadAnalysis()->readVar();
   float ratioAToB = BAI::calculateOffenseRatioAToB(analysisA, analysisB);
   getVar(cOutRatioAToB)->asFloat()->writeVar(ratioAToB);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teBidCreateBlank()
{
   uint version = getVersion();
   switch (version)
   {
   case 2:
      {
         teBidCreateBlankV2();
         break;
      }
   default:
      {
         BTRIGGER_ASSERTM(false, "Unsupported version!");
         break;
      }
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teBidCreateBlankV2()
{
   enum { cPlayer = 1, cBidID = 2, cPriority = 3, };
   BBidID bidID = cInvalidBidID;
   BBid* pBid = gWorld->createBid(getVar(cPlayer)->asPlayer()->readVar());
   if (pBid)
   {
      bidID = pBid->getID();
      pBid->setPriority(getVar(cPriority)->asFloat()->readVar());
   }
   getVar(cBidID)->asInteger()->writeVar(bidID);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teBidCreateBuilding()
{
   uint version = getVersion();
   switch (version)
   {
   case 3:
      {
         teBidCreateBuildingV3();
         break;
      }
   default:
      {
         BTRIGGER_ASSERTM(false, "Unsupported version!");
         break;
      }
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teAIMissionSetFlags()
{
   enum { cMissionID = 1, cEnableOdstDrop = 2, };

   BAIMission* pMission = gWorld->getAIMission(getVar(cMissionID)->asInteger()->readVar());
   if (!pMission)
      return;

   BTriggerVarBool* pVarOdstDrop = getVar(cEnableOdstDrop)->asBool();
   if (pVarOdstDrop->isUsed())
      pMission->setFlagEnableOdstDrop(pVarOdstDrop->readVar());  
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teBidCreateBuildingV3()
{
   enum { cPlayer = 1, cBidID = 2, cProtoObjectID = 3, cPriority = 4, cBuilder = 5, cTargetLoc = 6, }; 
   BBidID bidID = cInvalidBidID;
   BBid* pBid = gWorld->createBid(getVar(cPlayer)->asPlayer()->readVar());
   if (pBid)
   {
      bidID = pBid->getID();
      pBid->setPriority(getVar(cPriority)->asFloat()->readVar());
      pBid->setBuildingToBuy(getVar(cProtoObjectID)->asProtoObject()->readVar());

//-- FIXING PREFIX BUG ID 2764
      const BTriggerVarUnit* pBuilder = getVar(cBuilder)->asUnit();
//--
      if (pBuilder->isUsed())
         pBid->setBuilderID(pBuilder->readVar());

//-- FIXING PREFIX BUG ID 2765
      const BTriggerVarVector* pTargetLoc = getVar(cTargetLoc)->asVector();
//--
      if (pTargetLoc->isUsed())
         pBid->setTargetLocation(pTargetLoc->readVar());
   }
   getVar(cBidID)->asInteger()->writeVar(bidID);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teBidCreateTech()
{
   uint version = getVersion();
   switch (version)
   {
   case 3:
      {
         teBidCreateTechV3();
         break;
      }
   default:
      {
         BTRIGGER_ASSERTM(false, "Unsupported version!");
         break;
      }
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teBidCreateTechV3()
{
   enum { cPlayer = 1, cTechID = 2, cBidID = 3, cPriority = 4, cBuilder = 5, cTargetLoc = 6, }; 
   BBidID bidID = cInvalidBidID;
   BBid* pBid = gWorld->createBid(getVar(cPlayer)->asPlayer()->readVar());
   if (pBid)
   {
      bidID = pBid->getID();
      pBid->setPriority(getVar(cPriority)->asFloat()->readVar());
      pBid->setTechToBuy(getVar(cTechID)->asTech()->readVar());

//-- FIXING PREFIX BUG ID 2766
      const BTriggerVarUnit* pBuilder = getVar(cBuilder)->asUnit();
//--
      if (pBuilder->isUsed())
         pBid->setBuilderID(pBuilder->readVar());

//-- FIXING PREFIX BUG ID 2767
      const BTriggerVarVector* pTargetLoc = getVar(cTargetLoc)->asVector();
//--
      if (pTargetLoc->isUsed())
         pBid->setTargetLocation(pTargetLoc->readVar());
   }
   getVar(cBidID)->asInteger()->writeVar(bidID);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teBidCreateSquad()
{
   uint version = getVersion();
   switch (version)
   {
   case 3:
      {
         teBidCreateSquadV3();
         break;
      }
   default:
      {
         BTRIGGER_ASSERTM(false, "Unsupported version!");
         break;
      }
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teBidCreateSquadV3()
{
   enum { cPlayer = 1, cSquadID = 2, cPriority = 3, cBidID = 4, cBuilder = 5, cTargetLoc = 6, }; 
   BBidID bidID = cInvalidBidID;
   BBid* pBid = gWorld->createBid(getVar(cPlayer)->asPlayer()->readVar());
   if (pBid)
   {
      bidID = pBid->getID();
      pBid->setPriority(getVar(cPriority)->asFloat()->readVar());
      pBid->setSquadToBuy(getVar(cSquadID)->asProtoSquad()->readVar());

//-- FIXING PREFIX BUG ID 2768
      const BTriggerVarUnit* pBuilder = getVar(cBuilder)->asUnit();
//--
      if (pBuilder->isUsed())
         pBid->setBuilderID(pBuilder->readVar());

//-- FIXING PREFIX BUG ID 2769
      const BTriggerVarVector* pTargetLoc = getVar(cTargetLoc)->asVector();
//--
      if (pTargetLoc->isUsed())
         pBid->setTargetLocation(pTargetLoc->readVar());
   }
   getVar(cBidID)->asInteger()->writeVar(bidID);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teBidDelete()
{
   uint version = getVersion();
   switch (version)
   {
   case 2:
      {
         teBidDeleteV2();
         break;
      }
   default:
      {
         BTRIGGER_ASSERTM(false, "Unsupported version!");
         break;
      }
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teBidDeleteV2()
{
   enum { cBidID = 1,};
   gWorld->deleteBid(getVar(cBidID)->asInteger()->readVar());
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teBidSetBuilding()
{
   uint version = getVersion();
   switch (version)
   {
   case 2:
      {
         teBidSetBuildingV2();
         break;
      }
   default:
      {
         BTRIGGER_ASSERTM(false, "Unsupported version!");
         break;
      }
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teBidSetBuildingV2()
{
   enum { cBidID = 1, cProtoObjectID = 2, };
   BBidID bidID = getVar(cBidID)->asInteger()->readVar();
   BBid* pBid = gWorld->getBid(bidID);
   if (pBid)
      pBid->setBuildingToBuy(getVar(cProtoObjectID)->asProtoObject()->readVar());
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teBidSetTech()
{
   uint version = getVersion();
   switch (version)
   {
   case 2:
      {
         teBidSetTechV2();
         break;
      }
   default:
      {
         BTRIGGER_ASSERTM(false, "Unsupported version!");
         break;
      }
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teBidSetTechV2()
{
   enum { cBidID = 1, cTechID = 2, };
   BBidID bidID = getVar(cBidID)->asInteger()->readVar();
   BBid* pBid = gWorld->getBid(bidID);
   if (pBid)
      pBid->setTechToBuy(getVar(cTechID)->asTech()->readVar());
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teBidSetSquad()
{
   uint version = getVersion();
   switch (version)
   {
   case 2:
      {
         teBidSetSquadV2();
         break;
      }
   default:
      {
         BTRIGGER_ASSERTM(false, "Unsupported version!");
         break;
      }
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teBidSetSquadV2()
{
   enum { cBidID = 1, cProtoSquadID = 2, };
   BBidID bidID = getVar(cBidID)->asInteger()->readVar();
   BBid* pBid = gWorld->getBid(bidID);
   if (pBid)
      pBid->setSquadToBuy(getVar(cProtoSquadID)->asProtoSquad()->readVar());
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teBidClear()
{
   uint version = getVersion();
   switch (version)
   {
   case 2:
      {
         teBidClearV2();
         break;
      }
   default:
      {
         BTRIGGER_ASSERTM(false, "Unsupported version!");
         break;
      }
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teBidClearV2()
{
   enum { cBidID = 1, };
   BBidID bidID = getVar(cBidID)->asInteger()->readVar();
   BBid* pBid = gWorld->getBid(bidID);
   if (pBid)
      pBid->clear();
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teBidPurchase()
{
   uint version = getVersion();
   switch (version)
   {
   case 3:
      {
         teBidPurchaseV3();
         break;
      }
   case 4:
      {
         teBidPurchaseV4();
         break;
      }
   default:
      {
         BTRIGGER_ASSERTM(false, "Unsupported version!");
         break;
      }
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teBidPurchaseV3()
{
   enum { cBidID = 2, cSuccess = 5, };
   BBidID bidID = getVar(cBidID)->asInteger()->readVar();
   BBid* pBid = gWorld->getBid(bidID);
   bool result = false;
   BVector resultLocation = cOriginVector;
   if (pBid)
      result = pBid->purchase(resultLocation);

   getVar(cSuccess)->asBool()->writeVar(result);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teBidPurchaseV4()
{
   enum { cBidID = 2, cSuccess = 5, cResultLocation = 6, };
   BBidID bidID = getVar(cBidID)->asInteger()->readVar();
   BBid* pBid = gWorld->getBid(bidID);
   bool result = false;
   BVector resultLocation = cOriginVector;
   if (pBid)
      result = pBid->purchase(resultLocation);

   BTriggerVarBool* pSuccess = getVar(cSuccess)->asBool();
   if (pSuccess->isUsed())
      pSuccess->writeVar(result);

   BTriggerVarVector* pResultLocation = getVar(cResultLocation)->asVector();
   if (pResultLocation->isUsed())
      pResultLocation->writeVar(resultLocation);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teBidSetPriority()
{
   uint version = getVersion();
   switch (version)
   {
   case 2:
      {
         teBidSetPriorityV2();
         break;
      }
   default:
      {
         BTRIGGER_ASSERTM(false, "Unsupported version!");
         break;
      }
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teBidSetPriorityV2()
{
   enum { cBidID = 1, cPriority = 2, };
   BBidID bidID = getVar(cBidID)->asInteger()->readVar();
   BBid* pBid = gWorld->getBid(bidID);
   if (pBid)
      pBid->setPriority(getVar(cPriority)->asFloat()->readVar());
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teIteratorKBBaseList()
{
   enum { cKBBaseList = 1, cIterator = 2, };
   BTriggerVarID kbBaseListVarID = getVar(cKBBaseList)->getID();
   getVar(cIterator)->asIterator()->attachIterator(kbBaseListVarID);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBBaseGetDistance()
{
   enum { cDistance = 1, cLocation = 2, cKBBase = 3, };
   float distance = 0.0f;
   const BKBBase* pKBBase = gWorld->getKBBase(getVar(cKBBase)->asInteger()->readVar());
   if (pKBBase)
   {
      BVector testLocation = getVar(cLocation)->asVector()->readVar();
      distance = testLocation.distance(pKBBase->getPosition());
   }
   getVar(cDistance)->asFloat()->writeVar(distance);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teKBBaseGetMass()
{
   enum { cMass = 1, cKBBase = 2, };
   float mass = 0.0f;
   const BKBBase* pKBBase = gWorld->getKBBase(getVar(cKBBase)->asKBBase()->readVar());
   if (pKBBase)
      mass = pKBBase->getMass();
   getVar(cMass)->asFloat()->writeVar(mass);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teKBBQReset()
{
   enum { cKBBaseQuery = 1, };
   getVar(cKBBaseQuery)->asKBBaseQuery()->resetQuery();
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teKBBQExecute()
{
   enum
   {
      cQueryingTeam        = 1,
      cKBBaseQuery         = 2,
      cResultKBBaseList    = 3,
      cResultKBBaseCount   = 4,
   };

   // BASSERT for stupidity.
   BTRIGGER_ASSERTM(getVar(cResultKBBaseList)->isUsed() || getVar(cResultKBBaseCount)->isUsed(), "If you are going to do a query, at least use the results!");
   BKBBaseIDArray resultKBBases;
   uint numKBBases = 0;
   const BKBBaseQuery& kbBaseQuery = getVar(cKBBaseQuery)->asKBBaseQuery()->readVar();
   BKB* pKB = gWorld->getKB(getVar(cQueryingTeam)->asTeam()->readVar());
   if (pKB)
      numKBBases = pKB->executeKBBaseQuery(kbBaseQuery, resultKBBases, BKB::cClearExistingResults);

   if (getVar(cResultKBBaseList)->isUsed())
      getVar(cResultKBBaseList)->asKBBaseList()->writeVar(resultKBBases);
   if (getVar(cResultKBBaseCount)->isUsed())
      getVar(cResultKBBaseCount)->asInteger()->writeVar(numKBBases);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teKBBQPointRadius()
{
   enum { cParmPosition = 1, cParmRadius = 2, cOutputKBBaseQuery = 3, };
   // Get our KBBaseQuery variable so we can put the data in it.
   BTriggerVarKBBaseQuery* pKBBaseQueryVar = getVar(cOutputKBBaseQuery)->asKBBaseQuery();
   BVector pos = getVar(cParmPosition)->asVector()->readVar();
   float rad = getVar(cParmRadius)->asFloat()->readVar();
   pKBBaseQueryVar->setPointRadius(pos, rad);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBBQPlayerRelation()
{
   uint version = getVersion();
   switch (version)
   {
   case 2:
      {
         teKBBQPlayerRelationV2();
         break;
      }
   default:
      {
         teKBBQPlayerRelationV1();

         break;
      }
   }

}

//==============================================================================
//==============================================================================
void BTriggerEffect::teKBBQPlayerRelationV1()
{
   enum { cParmPlayer = 1, cParmRelation = 2, cOutputKBBaseQuery = 3, };
   // Get our KBBaseQuery variable so we can put the data in it.
   BTriggerVarKBBaseQuery* pKBBaseQueryVar = getVar(cOutputKBBaseQuery)->asKBBaseQuery();
   BPlayerID playerID = getVar(cParmPlayer)->asPlayer()->readVar();
   BRelationType relationType = getVar(cParmRelation)->asRelationType()->readVar();
   pKBBaseQueryVar->setPlayerRelation(playerID, relationType);


}

//==============================================================================
//==============================================================================
void BTriggerEffect::teKBBQPlayerRelationV2()
{
   enum { cParmPlayer = 1, cParmRelation = 2, cOutputKBBaseQuery = 3, cSelfAsAlly = 4};
   // Get our KBBaseQuery variable so we can put the data in it.
   BTriggerVarKBBaseQuery* pKBBaseQueryVar = getVar(cOutputKBBaseQuery)->asKBBaseQuery();
   BPlayerID playerID = getVar(cParmPlayer)->asPlayer()->readVar();
   BRelationType relationType = getVar(cParmRelation)->asRelationType()->readVar();
   pKBBaseQueryVar->setPlayerRelation(playerID, relationType);

   if(getVar(cSelfAsAlly)->isUsed())
   {
      pKBBaseQueryVar->setSelfAsAlly(getVar(cSelfAsAlly)->asBool()->readVar());
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBBQExecuteClosest()
{
   enum
   {
      cQueryingTeam        = 1,
      cKBBaseQuery         = 2,
      cTestPosition        = 3,
      cResultKBBaseList    = 4,
      cResultKBBaseCount   = 5,
   };

   // BASSERT for stupidity.
   BTRIGGER_ASSERTM(getVar(cResultKBBaseList)->isUsed() || getVar(cResultKBBaseCount)->isUsed(), "If you are going to do a query, at least use the results!");
   BKBBaseIDArray resultKBBases;
   const BKBBaseQuery& kbBaseQuery = getVar(cKBBaseQuery)->asKBBaseQuery()->readVar();
   uint numKBBases = 0;
   BVector testPosition = getVar(cTestPosition)->asVector()->readVar();
   BKB* pKB = gWorld->getKB(getVar(cQueryingTeam)->asTeam()->readVar());
   if (pKB)
      numKBBases = pKB->executeKBBaseQueryClosest(kbBaseQuery, testPosition, resultKBBases);

   if (getVar(cResultKBBaseList)->isUsed())
      getVar(cResultKBBaseList)->asKBBaseList()->writeVar(resultKBBases);
   if (getVar(cResultKBBaseCount)->isUsed())
      getVar(cResultKBBaseCount)->asInteger()->writeVar(numKBBases);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teBidGetData()
{
   if (mVersion == 1)
   {
      teBidGetDataV1();
   }
   else if (mVersion == 2)
   {
      teBidGetDataV2();
   }
   else
   {
      BFAIL("Unsupported version!");
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teBidGetDataV1()
{
   enum { cBidID = 1, cProtoObject = 2, cProtoSquad = 3, cTech = 4, cPriority = 5, cScore = 6, };
   BBidID bidID = getVar(cBidID)->asInteger()->readVar();
   const BBid* pBid = gWorld->getBid(bidID);
   BTriggerVarProtoObject* pVarProtoObject = getVar(cProtoObject)->asProtoObject();
   BTriggerVarProtoSquad* pVarProtoSquad = getVar(cProtoSquad)->asProtoSquad();
   BTriggerVarTech* pVarTech = getVar(cTech)->asTech();
   BTriggerVarFloat* pVarPriority = getVar(cPriority)->asFloat();
   BTriggerVarFloat* pVarScore = getVar(cScore)->asFloat();
   if (pVarProtoObject->isUsed())
   {
      if (pBid && pBid->isType(BidType::cBuilding))
         pVarProtoObject->writeVar(pBid->getBuildingToBuy());
      else
         pVarProtoObject->writeVar(cInvalidProtoObjectID);
   }
   if (pVarProtoSquad->isUsed())
   {
      if (pBid && pBid->isType(BidType::cSquad))
         pVarProtoSquad->writeVar(pBid->getSquadToBuy());
      else
         pVarProtoSquad->writeVar(cInvalidProtoSquadID);
   }
   if (pVarTech->isUsed())
   {
      if (pBid && pBid->isType(BidType::cTech))
         pVarTech->writeVar(pBid->getTechToBuy());
      else
         pVarTech->writeVar(cInvalidTechID);
   }
   if (pVarPriority->isUsed())
   {
      if (pBid)
         pVarPriority->writeVar(pBid->getPriority());
      else
         pVarPriority->writeVar(0.0f);
   }
   if (pVarScore->isUsed())
   {
      if (pBid)
         pVarScore->writeVar(pBid->getScore());
      else
         pVarScore->writeVar(0.0f);
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teBidGetDataV2()
{
   enum { cBidID = 1, cProtoObject = 2, cProtoSquad = 3, cTech = 4, cPriority = 5, cScore = 6, cProtoPower = 7, };
   BBidID bidID = getVar(cBidID)->asInteger()->readVar();
   const BBid* pBid = gWorld->getBid(bidID);
   BTriggerVarProtoObject* pVarProtoObject = getVar(cProtoObject)->asProtoObject();
   BTriggerVarProtoSquad* pVarProtoSquad = getVar(cProtoSquad)->asProtoSquad();
   BTriggerVarPower* pVarPower = getVar(cProtoPower)->asPower();
   BTriggerVarTech* pVarTech = getVar(cTech)->asTech();
   BTriggerVarFloat* pVarPriority = getVar(cPriority)->asFloat();
   BTriggerVarFloat* pVarScore = getVar(cScore)->asFloat();
   if (pVarProtoObject->isUsed())
   {
      if (pBid && pBid->isType(BidType::cBuilding))
         pVarProtoObject->writeVar(pBid->getBuildingToBuy());
      else
         pVarProtoObject->writeVar(cInvalidProtoObjectID);
   }
   if (pVarProtoSquad->isUsed())
   {
      if (pBid && pBid->isType(BidType::cSquad))
         pVarProtoSquad->writeVar(pBid->getSquadToBuy());
      else
         pVarProtoSquad->writeVar(cInvalidProtoSquadID);
   }
   if (pVarTech->isUsed())
   {
      if (pBid && pBid->isType(BidType::cTech))
         pVarTech->writeVar(pBid->getTechToBuy());
      else
         pVarTech->writeVar(cInvalidTechID);
   }
   if (pVarPower->isUsed())
   {
      if (pBid && pBid->isType(BidType::cPower))
         pVarPower->writeVar(pBid->getPowerToCast());
      else
         pVarPower->writeVar(cInvalidProtoPowerID);
   }
   if (pVarPriority->isUsed())
   {
      if (pBid)
         pVarPriority->writeVar(pBid->getPriority());
      else
         pVarPriority->writeVar(0.0f);
   }
   if (pVarScore->isUsed())
   {
      if (pBid)
         pVarScore->writeVar(pBid->getScore());
      else
         pVarScore->writeVar(0.0f);
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teBidQuery()
{
   if (mVersion == 1)
   {
      teBidQueryV1();
   }
   else if (mVersion == 2)
   {
      teBidQueryV2();
   }
   else
   {
      BFAIL("Unsupported Version!");
   }

}

//==============================================================================
//==============================================================================
void BTriggerEffect::teBidQueryV1()
{
   enum
   {
      cBidList = 1,
      cBidCount = 2,
      cPlayer = 3,
      cBidState = 4,
      cBidType = 5,
      cFilterBidList = 6,
      cProtoSquad = 7,
      cProtoObject = 8,
      cTech = 9,
      cMinTime = 10,
      cMaxTime = 11,
      cObjectType = 12,
   };

   BPlayerID playerID = getVar(cPlayer)->asPlayer()->readVar();
//-- FIXING PREFIX BUG ID 2728
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
//--
   if (!pPlayer)
      return;
   BAI* pAI = gWorld->getAI(playerID);
   if (!pAI)
      return;
   BBidManager* pBidManager = pAI->getBidManager();
   if (!pBidManager)
      return;

   BTriggerVarIntegerList* pBidList = getVar(cBidList)->asIntegerList();
   BTriggerVarInteger* pBidCount = getVar(cBidCount)->asInteger();
   if (!pBidList->isUsed() && !pBidCount->isUsed())
      return;
//-- FIXING PREFIX BUG ID 2773
   const BTriggerVarBidState* pBidState = getVar(cBidState)->asBidState();
//--
//-- FIXING PREFIX BUG ID 2774
   const BTriggerVarBidType* pBidType = getVar(cBidType)->asBidType();
//--
   BTriggerVarIntegerList* pFilterBidList = getVar(cFilterBidList)->asIntegerList();
//-- FIXING PREFIX BUG ID 2775
   const BTriggerVarProtoSquad* pProtoSquad = getVar(cProtoSquad)->asProtoSquad();
//--
//-- FIXING PREFIX BUG ID 2776
   const BTriggerVarProtoObject* pProtoObject = getVar(cProtoObject)->asProtoObject();
//--
//-- FIXING PREFIX BUG ID 2777
   const BTriggerVarTech* pTech = getVar(cTech)->asTech();
//--
//-- FIXING PREFIX BUG ID 2778
   const BTriggerVarTime* pMinTime = getVar(cMinTime)->asTime();
//--
//-- FIXING PREFIX BUG ID 2779
   const BTriggerVarTime* pMaxTime = getVar(cMaxTime)->asTime();
//--
//-- FIXING PREFIX BUG ID 2780
   const BTriggerVarObjectType* pObjectType = getVar(cObjectType)->asObjectType();
//--

   BInt32Array bidList;
   int bidCount = 0;

   const BBidIDArray& currentBids = pBidManager->getBidIDs();
   uint numBids = currentBids.getSize();
   for (uint i=0; i<numBids; i++)
   {
      BBidID bidID = currentBids[i];
//-- FIXING PREFIX BUG ID 2772
      const BBid* pBid = gWorld->getBid(bidID);
//--
      if (!pBid)
         continue;
      if (pFilterBidList->isUsed() && !pFilterBidList->readVar().contains(bidID))
         continue;
      if (pBidState->isUsed() && !pBid->isState(pBidState->readVar()))
         continue;
      if (pBidType->isUsed() && !pBid->isType(pBidType->readVar()))
         continue;
      if (pProtoSquad->isUsed() && pBid->getSquadToBuy() != pProtoSquad->readVar())
         continue;
      if (pProtoObject->isUsed() && pBid->getBuildingToBuy() != pProtoObject->readVar())
         continue;
      if (pTech->isUsed() && pBid->getTechToBuy() != pTech->readVar())
         continue;
      if (pMinTime->isUsed() && pBid->getElapsedTime() < pMinTime->readVar())
         continue;
      if (pMaxTime->isUsed() && pBid->getElapsedTime() > pMaxTime->readVar())
         continue;

      // OBJECT TYPE NOT IMPLEMENTED
      if(pObjectType->isUsed())
      {
         long objectType = pObjectType->readVar();

         if(pBid->isType(BidType::cSquad))
         {
//-- FIXING PREFIX BUG ID 2770
            const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(pBid->getSquadToBuy());
//--
            if(!pProtoSquad)
               continue;
            long numUnitNodes = pProtoSquad->getNumberUnitNodes();
            for (long i=0; i<numUnitNodes; i++)
            {
               const BProtoSquadUnitNode& node = pProtoSquad->getUnitNode(i);
               const BProtoObject* pProtoObject = pPlayer->getProtoObject(node.mUnitType);
               if (!pProtoObject)
                  continue;
               if(pProtoObject->isType(objectType) == false)               
                  continue;               
            }
         }
         else if(pBid->isType(BidType::cBuilding))
         {
//-- FIXING PREFIX BUG ID 2771
            const BProtoObject* pProtoObject = pPlayer->getProtoObject(pBid->getBuildingToBuy());
//--
            if(!pProtoObject)
               continue;
            if(pProtoObject->isType(objectType) == false)
               continue;
         }
         else 
         {
            continue;
         }

      }

      // Must be good enough, add it or something.
      bidList.add(bidID);
      bidCount++;      
   }

   if (pBidList->isUsed())
      pBidList->writeVar(bidList);
   if (pBidCount->isUsed())
      pBidCount->writeVar(bidCount);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teBidQueryV2()
{
   enum
   {
      cBidList = 1,
      cBidCount = 2,
      cPlayer = 3,
      cBidState = 4,
      cBidType = 5,
      cFilterBidList = 6,
      cProtoSquad = 7,
      cProtoObject = 8,
      cTech = 9,
      cMinTime = 10,
      cMaxTime = 11,
      cObjectType = 12,
      cProtoPower = 13,
   };

   BPlayerID playerID = getVar(cPlayer)->asPlayer()->readVar();
   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return;
   BAI* pAI = gWorld->getAI(playerID);
   if (!pAI)
      return;
   BBidManager* pBidManager = pAI->getBidManager();
   if (!pBidManager)
      return;
   BTriggerVarIntegerList* pBidList = getVar(cBidList)->asIntegerList();
   BTriggerVarInteger* pBidCount = getVar(cBidCount)->asInteger();
   if (!pBidList->isUsed() && !pBidCount->isUsed())
      return;
//-- FIXING PREFIX BUG ID 2788
   const BTriggerVarBidState* pBidState = getVar(cBidState)->asBidState();
//--
//-- FIXING PREFIX BUG ID 2789
   const BTriggerVarBidType* pBidType = getVar(cBidType)->asBidType();
//--
   BTriggerVarIntegerList* pFilterBidList = getVar(cFilterBidList)->asIntegerList();
//-- FIXING PREFIX BUG ID 2790
   const BTriggerVarProtoSquad* pProtoSquad = getVar(cProtoSquad)->asProtoSquad();
//--
//-- FIXING PREFIX BUG ID 2791
   const BTriggerVarProtoObject* pProtoObject = getVar(cProtoObject)->asProtoObject();
//--
//-- FIXING PREFIX BUG ID 2792
   const BTriggerVarTech* pTech = getVar(cTech)->asTech();
//--
//-- FIXING PREFIX BUG ID 2793
   const BTriggerVarTime* pMinTime = getVar(cMinTime)->asTime();
//--
//-- FIXING PREFIX BUG ID 2794
   const BTriggerVarTime* pMaxTime = getVar(cMaxTime)->asTime();
//--
//-- FIXING PREFIX BUG ID 2795
   const BTriggerVarObjectType* pObjectType = getVar(cObjectType)->asObjectType();
//--
//-- FIXING PREFIX BUG ID 2796
   const BTriggerVarPower* pVarPower = getVar(cProtoPower)->asPower();
//--


   BInt32Array bidList;
   int bidCount = 0;

   const BBidIDArray& currentBids = pBidManager->getBidIDs();
   uint numBids = currentBids.getSize();
   for (uint i=0; i<numBids; i++)
   {
      BBidID bidID = currentBids[i];
//-- FIXING PREFIX BUG ID 2787
      const BBid* pBid = gWorld->getBid(bidID);
//--
      if (!pBid)
         continue;
      if (pFilterBidList->isUsed() && !pFilterBidList->readVar().contains(bidID))
         continue;
      if (pBidState->isUsed() && !pBid->isState(pBidState->readVar()))
         continue;
      if (pBidType->isUsed() && !pBid->isType(pBidType->readVar()))
         continue;
      if (pProtoSquad->isUsed() && pBid->getSquadToBuy() != pProtoSquad->readVar())
         continue;
      if (pProtoObject->isUsed() && pBid->getBuildingToBuy() != pProtoObject->readVar())
         continue;
      if (pTech->isUsed() && pBid->getTechToBuy() != pTech->readVar())
         continue;
      if (pMinTime->isUsed() && pBid->getElapsedTime() < pMinTime->readVar())
         continue;
      if (pMaxTime->isUsed() && pBid->getElapsedTime() > pMaxTime->readVar())
         continue;
      if (pVarPower->isUsed() && pBid->getPowerToCast() != pVarPower->readVar())
         continue;

      // OBJECT TYPE NOT IMPLEMENTED
      if(pObjectType->isUsed())
      {
         long objectType = pObjectType->readVar();

         if(pBid->isType(BidType::cSquad))
         {
//-- FIXING PREFIX BUG ID 2785
            const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(pBid->getSquadToBuy());
//--
            if(!pProtoSquad)
               continue;
            long numUnitNodes = pProtoSquad->getNumberUnitNodes();
            for (long i=0; i<numUnitNodes; i++)
            {
               const BProtoSquadUnitNode& node = pProtoSquad->getUnitNode(i);
               const BProtoObject* pProtoObject = pPlayer->getProtoObject(node.mUnitType);
               if (!pProtoObject)
                  continue;
               if(pProtoObject->isType(objectType) == false)               
                  continue;               
            }
         }
         else if(pBid->isType(BidType::cBuilding))
         {
//-- FIXING PREFIX BUG ID 2786
            const BProtoObject* pProtoObject = pPlayer->getProtoObject(pBid->getBuildingToBuy());
//--
            if(!pProtoObject)
               continue;
            if(pProtoObject->isType(objectType) == false)
               continue;
         }
         else 
         {
            continue;
         }

      }

      // Must be good enough, add it or something.
      bidList.add(bidID);
      bidCount++;      
   }

   if (pBidList->isUsed())
      pBidList->writeVar(bidList);
   if (pBidCount->isUsed())
      pBidCount->writeVar(bidCount);
}


//==============================================================================
// teBidSetBlockedBuilders  sets builders that are not allowed to be used by the bid manager
//==============================================================================
void BTriggerEffect::teBidSetBlockedBuilders()
{
   enum  {	cPlayerID = 1, cBlockedBuilders = 2, cClearAll = 3  };
   BAI* pAI = gWorld->getAI(getVar(cPlayerID)->asPlayer()->readVar());
   if (!pAI)
      return;
   BBidManager* pBidManager = pAI->getBidManager();
   if (!pBidManager)
      return;
   if(getVar(cClearAll)->isUsed() && (getVar(cClearAll)->asBool()->readVar() == true))
   {
      pBidManager->clearBlockedBuildings();
   }

   if(getVar(cBlockedBuilders)->isUsed())
   {
      const BEntityIDArray buildders = getVar(cBlockedBuilders)->asUnitList()->readVar();

      pBidManager->setBlockedBuildings(buildders);
   }
}


//==============================================================================
// Settings for the bid manager
//==============================================================================
void BTriggerEffect::teBidSetQueueLimits()
{
   enum 
   { 
      cPlayerID = 1, 
      cSquadGlobalQueueLimt = 2, 
      cSquadTrainQueueLimit = 2, 
   };

   BAI* pAI = gWorld->getAI(getVar(cPlayerID)->asPlayer()->readVar());
   if (!pAI)
      return;
   BBidManager* pBidManager = pAI->getBidManager();
   if (!pBidManager)
      return;

   float squadGlobal = getVar(cSquadGlobalQueueLimt)->asTime()->readVar() / 1000.0f;
   float squadTrain = getVar(cSquadTrainQueueLimit)->asTime()->readVar() / 1000.0f;

   pBidManager->setQueueSettings(squadGlobal, squadTrain);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBBQMinStaleness()
{
   enum { cMinStaleness = 1, cOutputKBBaseQuery = 2, };
   // Get our KBBaseQuery variable so we can put the data in it.
   BTriggerVarKBBaseQuery* pKBBaseQueryVar = getVar(cOutputKBBaseQuery)->asKBBaseQuery();
   DWORD minStaleness = getVar(cMinStaleness)->asTime()->readVar();
   pKBBaseQueryVar->setMinStaleness(minStaleness);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBBQMaxStaleness()
{
   enum { cMaxStaleness = 1, cOutputKBBaseQuery = 2, };
   // Get our KBBaseQuery variable so we can put the data in it.
   BTriggerVarKBBaseQuery* pKBBaseQueryVar = getVar(cOutputKBBaseQuery)->asKBBaseQuery();
   DWORD maxStaleness = getVar(cMaxStaleness)->asTime()->readVar();
   pKBBaseQueryVar->setMaxStaleness(maxStaleness);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSQInit()
{
   enum
   {
      cParmResetQuery      = 1,
      cParmPosition        = 2,
      cParmRadius          = 3,
      cParmPlayer          = 4,
      cParmPlayerRelation  = 5,
      cParmObjectType      = 6,
      cParmBase            = 7,
      cParmMinStaleness    = 8,
      cParmMaxStaleness    = 9,
      cOutputKBSquadQuery   = 11,
   };

   // Get our KBSquadQuery variable so we can put the data in it.
   BTriggerVarKBSquadQuery* pKBSquadQueryVar = getVar(cOutputKBSquadQuery)->asKBSquadQuery();

   // Do we want to clear it out first?
   bool resetQuery = getVar(cParmResetQuery)->isUsed() ? getVar(cParmResetQuery)->asBool()->readVar() : false;
   if (resetQuery)
      pKBSquadQueryVar->resetQuery();

   // Do we have a point radius component?  (Need both of these.)
   if (getVar(cParmPosition)->isUsed() && getVar(cParmRadius)->isUsed())
   {
      BVector pos = getVar(cParmPosition)->asVector()->readVar();
      float rad = getVar(cParmRadius)->asFloat()->readVar();
      pKBSquadQueryVar->setPointRadius(pos, rad);
   }

   // Do we have a player relation component?
   if (getVar(cParmPlayer)->isUsed() && getVar(cParmPlayerRelation)->isUsed())
   {
      BPlayerID playerID = getVar(cParmPlayer)->asPlayer()->readVar();
      BRelationType relationType = getVar(cParmPlayerRelation)->asRelationType()->readVar();
      pKBSquadQueryVar->setPlayerRelation(playerID, relationType);
   }

   // Do we have an object type component
   if (getVar(cParmObjectType)->isUsed())
   {
      BObjectTypeID objectType = getVar(cParmObjectType)->asObjectType()->readVar();
      pKBSquadQueryVar->setObjectType(objectType);
   }

   // Do we have a base component
   if (getVar(cParmBase)->isUsed())
   {
      BKBBaseID baseID = getVar(cParmBase)->asKBBase()->readVar();
      pKBSquadQueryVar->setBase(baseID);
   }

   // Do we have a min staleness component
   if (getVar(cParmMinStaleness)->isUsed())
   {
      DWORD minStaleness = getVar(cParmMinStaleness)->asTime()->readVar();
      pKBSquadQueryVar->setMinStaleness(minStaleness);
   }

   // Do we have a max staleness component
   if (getVar(cParmMaxStaleness)->isUsed())
   {
      DWORD maxStaleness = getVar(cParmMaxStaleness)->asTime()->readVar();
      pKBSquadQueryVar->setMaxStaleness(maxStaleness);
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSQReset()
{
   enum { cKBSquadQuery = 1, };
   getVar(cKBSquadQuery)->asKBSquadQuery()->resetQuery();
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSQExecute()
{
   enum
   {
      cQueryingTeam        = 1,
      cKBSquadQuery         = 2,
      cResultKBSquadList    = 3,
      cResultKBSquadCount   = 4,
   };

   // BASSERT for stupidity.
   BTRIGGER_ASSERTM(getVar(cResultKBSquadList)->isUsed() || getVar(cResultKBSquadCount)->isUsed(), "If you are going to do a query, at least use the results!");
   uint numKBSquads = 0;
   const BKBSquadQuery& KBSquadQuery = getVar(cKBSquadQuery)->asKBSquadQuery()->readVar();
   BKBSquadIDArray resultKBSquads;
   BKB* pKB = gWorld->getKB(getVar(cQueryingTeam)->asTeam()->readVar());
   if (pKB)
      numKBSquads = pKB->executeKBSquadQuery(KBSquadQuery, resultKBSquads, BKB::cClearExistingResults);
   if (getVar(cResultKBSquadList)->isUsed())
      getVar(cResultKBSquadList)->asKBSquadList()->writeVar(resultKBSquads);
   if (getVar(cResultKBSquadCount)->isUsed())
      getVar(cResultKBSquadCount)->asInteger()->writeVar(numKBSquads);

}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSquadFilterClear()
{
   enum { cKBSquadFilterSet = 1, };
   getVar(cKBSquadFilterSet)->asKBSquadFilterSet()->clearFilters();
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSFAddCurrentlyVisible()
{
   enum { cKBSquadFilterSet = 3, cInvert = 2, };
   bool invertFilter = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;
   getVar(cKBSquadFilterSet)->asKBSquadFilterSet()->addKBSquadFilterCurrentlyVisible(invertFilter);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSFAddObjectTypes()
{
   enum { cKBSquadFilterSet = 4, cObjectTypeList = 2, cInvert = 3, };
   bool invertFilter = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;
   const BObjectTypeIDArray &objectTypeList = getVar(cObjectTypeList)->asObjectTypeList()->readVar();
   getVar(cKBSquadFilterSet)->asKBSquadFilterSet()->addKBSquadFilterObjectTypes(invertFilter, objectTypeList);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSFAddPlayers()
{
   enum { cKBSquadFilterSet = 4, cPlayerList = 2, cInvert = 3, };
   bool invertFilter = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;
   const BPlayerIDArray &playerList = getVar(cPlayerList)->asPlayerList()->readVar();
   getVar(cKBSquadFilterSet)->asKBSquadFilterSet()->addKBSquadFilterPlayers(invertFilter, playerList);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSFAddInList()
{
   enum { cKBSquadFilterSet = 4, cKBSquadList = 2, cInvert = 3, };
   bool invertFilter = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;

   const BKBSquadIDArray &KBSquadList = getVar(cKBSquadList)->asKBSquadList()->readVar();
   getVar(cKBSquadFilterSet)->asKBSquadFilterSet()->addKBSquadFilterInList(invertFilter, KBSquadList);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSFAddPlayerRelation()
{
   uint version = getVersion();
   switch (version)
   {
   case 2:
      {
         teKBSFAddPlayerRelationV2();
         break;
      }
   default:
      {
         teKBSFAddPlayerRelationV1();

         break;
      }
   }

}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSFAddPlayerRelationV1()
{
   enum { cKBSquadFilterSet = 1, cPlayer = 2, cRelationType = 3, cInvert = 4, };
   bool invertFilter = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;
   BPlayerID playerID = getVar(cPlayer)->asPlayer()->readVar();
   BRelationType relationType = getVar(cRelationType)->asRelationType()->readVar();
   getVar(cKBSquadFilterSet)->asKBSquadFilterSet()->addKBSquadFilterPlayerRelation(invertFilter, playerID, relationType, false);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSFAddPlayerRelationV2()
{
   enum { cKBSquadFilterSet = 6, cPlayer = 2, cRelationType = 3, cInvert = 4, cSelfAsAlly = 5};
   bool invertFilter = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;
   BPlayerID playerID = getVar(cPlayer)->asPlayer()->readVar();
   BRelationType relationType = getVar(cRelationType)->asRelationType()->readVar();

   if(getVar(cSelfAsAlly)->isUsed())
   {
      getVar(cKBSquadFilterSet)->asKBSquadFilterSet()->addKBSquadFilterPlayerRelation(invertFilter, playerID, relationType,getVar(cSelfAsAlly)->asBool()->readVar() );

   }
   else
   {
      getVar(cKBSquadFilterSet)->asKBSquadFilterSet()->addKBSquadFilterPlayerRelation(invertFilter, playerID, relationType, false);
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSquadListFilter()
{
   enum { cQueryingTeam = 1, cSourceKBSquadList = 2, cKBSquadFilterSet = 3, cPassedKBSquadList = 5, cFailedKBSquadList = 6, };
   bool usePassedKBSquadList = getVar(cPassedKBSquadList)->isUsed();
   bool useFailedKBSquadList = getVar(cFailedKBSquadList)->isUsed();
   if (!usePassedKBSquadList && !useFailedKBSquadList)
      return;

   BKB* pKB = gWorld->getKB(getVar(cQueryingTeam)->asTeam()->readVar());
   if (!pKB)
      return;

   BKBSquadIDArray sourceKBSquadList = getVar(cSourceKBSquadList)->asKBSquadList()->readVar();
   BKBSquadIDArray passedKBSquads;
   BKBSquadIDArray failedKBSquads;
   BKBSquadIDArray invalidKBSquads;

   BKBSquadFilterSet *pFilter = getVar(cKBSquadFilterSet)->asKBSquadFilterSet()->readVar();
   pFilter->filterKBSquads(pKB, sourceKBSquadList, &passedKBSquads, &failedKBSquads, &invalidKBSquads);

   if (usePassedKBSquadList)
      getVar(cPassedKBSquadList)->asKBSquadList()->writeVar(passedKBSquads);
   if (useFailedKBSquadList)
      getVar(cFailedKBSquadList)->asKBSquadList()->writeVar(failedKBSquads);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSFAddMinStaleness()
{
   enum { cKBSquadFilterSet = 4, cMinStaleness = 2, cInvert = 3, };
   bool invertFilter = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;
   DWORD minStaleness = getVar(cMinStaleness)->asTime()->readVar();
   getVar(cKBSquadFilterSet)->asKBSquadFilterSet()->addKBSquadFilterMinStaleness(invertFilter, minStaleness);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSFAddMaxStaleness()
{
   enum { cKBSquadFilterSet = 4, cMaxStaleness = 2, cInvert = 3, };
   bool invertFilter = getVar(cInvert)->isUsed() ? getVar(cInvert)->asBool()->readVar() : false;
   DWORD maxStaleness = getVar(cMaxStaleness)->asTime()->readVar();
   getVar(cKBSquadFilterSet)->asKBSquadFilterSet()->addKBSquadFilterMaxStaleness(invertFilter, maxStaleness);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSquadListGetSize()
{
   enum { cKBSquadList = 1, cCount = 2, };
   const BKBSquadIDArray& KBSquadList = getVar(cKBSquadList)->asKBSquadList()->readVar();
   getVar(cCount)->asInteger()->writeVar(KBSquadList.getNumber());
}



//==============================================================================
//==============================================================================
void BTriggerEffect::teIteratorKBSquadList()
{
   enum { cKBSquadList = 1, cIterator = 2, };
   BTriggerVarID KBSquadListVarID = getVar(cKBSquadList)->getID();
   getVar(cIterator)->asIterator()->attachIterator(KBSquadListVarID);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSquadGetOwner()
{
   enum { cKBSquad = 1, cOwner = 2, }; 
   const BKBSquad* pKBSquad = gWorld->getKBSquad(getVar(cKBSquad)->asKBSquad()->readVar());
   if (pKBSquad)
      getVar(cOwner)->asPlayer()->writeVar(pKBSquad->getPlayerID());
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSquadGetLocation()
{
   enum { cKBSquad = 1, cLocation = 2, }; 
   const BKBSquad* pKBSquad = gWorld->getKBSquad(getVar(cKBSquad)->asKBSquad()->readVar());
   if (pKBSquad)
      getVar(cLocation)->asVector()->writeVar(pKBSquad->getPosition());     
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teConvertKBSquadsToSquads()
{
   enum { cKBSquadList = 1, cSquadList = 2, }; 
   const BKBSquadIDArray& KBSquadList = getVar(cKBSquadList)->asKBSquadList()->readVar();
   BTriggerVarSquadList* pVarSquadList = getVar(cSquadList)->asSquadList();
   pVarSquadList->clear();

   uint numKBSquads = KBSquadList.getSize();
   for (uint i=0; i<numKBSquads; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(KBSquadList[i]);
      if (pKBSquad && pKBSquad->getCurrentlyVisible())
         pVarSquadList->addSquad(pKBSquad->getSquadID());
   }
}



//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSquadGetProtoSquad()
{
   enum { cKBSquad = 1, cProtoSquad = 2, }; 
   const BKBSquad* pKBSquad = gWorld->getKBSquad(getVar(cKBSquad)->asKBSquad()->readVar());
   BProtoObjectID protoSquadID = pKBSquad ? pKBSquad->getProtoSquadID() : cInvalidProtoSquadID;
   getVar(cProtoSquad)->asProtoSquad()->writeVar(protoSquadID);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teKBBaseGetKBSquads()
{
   enum { cKBBase = 1, cKBSquadList = 2, }; 
   BTriggerVarKBSquadList* pTriggerVarKBSquadList = getVar(cKBSquadList)->asKBSquadList();
   pTriggerVarKBSquadList->clear();

   const BKBBase* pKBBase = gWorld->getKBBase(getVar(cKBBase)->asKBBase()->readVar());
   if (pKBBase)
      pTriggerVarKBSquadList->writeVar(pKBBase->getKBSquadIDs());
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSQExecuteClosest()
{

   enum
   {
      cQueryingTeam        = 1,
      cKBSquadQuery         = 2,
      cTestPosition        = 3,
      cResultKBSquadList    = 4,
      cResultKBSquadCount   = 5,
   };

   // BASSERT for stupidity.
   BTRIGGER_ASSERTM(getVar(cResultKBSquadList)->isUsed() || getVar(cResultKBSquadCount)->isUsed(), "If you are going to do a query, at least use the results!");
   uint numKBSquads = 0;
   BKBSquadIDArray resultKBSquads;
   const BKBSquadQuery& KBSquadQuery = getVar(cKBSquadQuery)->asKBSquadQuery()->readVar();
   BVector testPosition = getVar(cTestPosition)->asVector()->readVar();
   BKB* pKB = gWorld->getKB(getVar(cQueryingTeam)->asTeam()->readVar());
   if (pKB)
      numKBSquads = pKB->executeKBSquadQueryClosest(KBSquadQuery, testPosition, resultKBSquads);

   if (getVar(cResultKBSquadList)->isUsed())
      getVar(cResultKBSquadList)->asKBSquadList()->writeVar(resultKBSquads);
   if (getVar(cResultKBSquadCount)->isUsed())
      getVar(cResultKBSquadCount)->asInteger()->writeVar(numKBSquads);

}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSQPointRadius()
{
   enum { cParmPosition = 1, cParmRadius = 2, cOutputKBSquadQuery = 3, };
   // Get our KBSquadQuery variable so we can put the data in it.
   BTriggerVarKBSquadQuery* pKBSquadQueryVar = getVar(cOutputKBSquadQuery)->asKBSquadQuery();
   BVector pos = getVar(cParmPosition)->asVector()->readVar();
   float rad = getVar(cParmRadius)->asFloat()->readVar();
   pKBSquadQueryVar->setPointRadius(pos, rad);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSQPlayerRelation()
{
   uint version = getVersion();
   switch (version)
   {
   case 2:
      {
         teKBSQPlayerRelationV2();
         break;
      }
   default:
      {
         teKBSQPlayerRelationV1();

         break;
      }
   }

}

//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSQPlayerRelationV1()
{
   enum { cParmPlayer = 1, cParmRelation = 2, cOutputKBSquadQuery = 3, };
   // Get our KBSquadQuery variable so we can put the data in it.
   BTriggerVarKBSquadQuery* pKBSquadQueryVar = getVar(cOutputKBSquadQuery)->asKBSquadQuery();
   BPlayerID playerID = getVar(cParmPlayer)->asPlayer()->readVar();
   BRelationType relationType = getVar(cParmRelation)->asRelationType()->readVar();
   pKBSquadQueryVar->setPlayerRelation(playerID, relationType);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSQPlayerRelationV2()
{
   enum { cParmPlayer = 1, cParmRelation = 2, cOutputKBSquadQuery = 3, cSelfAsAlly = 4};
   // Get our KBSquadQuery variable so we can put the data in it.
   BTriggerVarKBSquadQuery* pKBSquadQueryVar = getVar(cOutputKBSquadQuery)->asKBSquadQuery();
   BPlayerID playerID = getVar(cParmPlayer)->asPlayer()->readVar();
   BRelationType relationType = getVar(cParmRelation)->asRelationType()->readVar();
   pKBSquadQueryVar->setPlayerRelation(playerID, relationType);

   if(getVar(cSelfAsAlly)->isUsed())
   {
      pKBSquadQueryVar->setSelfAsAlly(getVar(cSelfAsAlly)->asBool()->readVar());
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSQObjectType()
{
   enum { cParmObjectType = 1, cOutputKBSquadQuery = 2, };
   // Get our KBSquadQuery variable so we can put the data in it.
   BTriggerVarKBSquadQuery* pKBSquadQueryVar = getVar(cOutputKBSquadQuery)->asKBSquadQuery();
   BObjectTypeID objectTypeID = getVar(cParmObjectType)->asObjectType()->readVar();
   pKBSquadQueryVar->setObjectType(objectTypeID);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSQBase()
{
   enum { cParmBase = 1, cOutputKBSquadQuery = 2, };
   // Get our KBSquadQuery variable so we can put the data in it.
   BTriggerVarKBSquadQuery* pKBSquadQueryVar = getVar(cOutputKBSquadQuery)->asKBSquadQuery();
   BKBBaseID baseID = getVar(cParmBase)->asKBBase()->readVar();
   pKBSquadQueryVar->setBase(baseID);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSQMinStaleness()
{
   enum { cParmMinStaleness = 1, cOutputKBSquadQuery = 2, };
   // Get our KBSquadQuery variable so we can put the data in it.
   BTriggerVarKBSquadQuery* pKBSquadQueryVar = getVar(cOutputKBSquadQuery)->asKBSquadQuery();
   DWORD minStaleness = getVar(cParmMinStaleness)->asTime()->readVar();
   pKBSquadQueryVar->setMinStaleness(minStaleness);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSQMaxStaleness()
{
   enum { cParmMaxStaleness = 1, cOutputKBSquadQuery = 2, };
   // Get our KBSquadQuery variable so we can put the data in it.
   BTriggerVarKBSquadQuery* pKBSquadQueryVar = getVar(cOutputKBSquadQuery)->asKBSquadQuery();
   DWORD maxStaleness = getVar(cParmMaxStaleness)->asTime()->readVar();
   pKBSquadQueryVar->setMaxStaleness(maxStaleness);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teKBSQCurrentlyVisible()
{
   enum { cParmCurrentlyVisible = 1, cOutputKBSquadQuery = 2, };
   // Get our KBSquadQuery variable so we can put the data in it.
   BTriggerVarKBSquadQuery* pKBSquadQueryVar = getVar(cOutputKBSquadQuery)->asKBSquadQuery();
   bool currentlyVisible = getVar(cParmCurrentlyVisible)->asBool()->readVar();
   pKBSquadQueryVar->setCurrentlyVisible(currentlyVisible);
}

//==============================================================================
// KBSquadListDiff
//==============================================================================
void BTriggerEffect::teKBSquadListDiff()
{
   enum { cListA = 1, cListB = 2, cOnlyInA = 3, cOnlyInB = 4, cInBoth = 5, };
   bool useOnlyInA = getVar(cOnlyInA)->isUsed();
   bool useOnlyInB = getVar(cOnlyInB)->isUsed();
   bool useInBoth = getVar(cInBoth)->isUsed();
   if (!useOnlyInA && !useOnlyInB && !useInBoth)
      return;

   // It's a logic error to use the same variable for two of the outputs because one with bash the other.
   //BTRIGGER_ASSERT(getVar(cOnlyInA) != getVar(cOnlyInB) \
   //   && (getVar(cOnlyInA) != getVar(cInBoth) || !useInBoth) \
   //   && (getVar(cOnlyInB) != getVar(cInBoth) || !useInBoth));

   // Get local copies of our source data.
   BKBSquadIDArray listA = getVar(cListA)->asKBSquadList()->readVar();     
   BKBSquadIDArray listB = getVar(cListB)->asKBSquadList()->readVar();
   long numInA = listA.getNumber();
   long numInB = listB.getNumber();

   // Clear our results lists.
   BTriggerVarKBSquadList *pOnlyInA = getVar(cOnlyInA)->asKBSquadList();
   if (useOnlyInA)
      pOnlyInA->clear();
   BTriggerVarKBSquadList *pOnlyInB = getVar(cOnlyInB)->asKBSquadList();
   if (useOnlyInB)
      pOnlyInB->clear();
   BTriggerVarKBSquadList *pInBoth = getVar(cInBoth)->asKBSquadList();
   if (useInBoth)
      pInBoth->clear();

   // Easiest case:  Both lists empty.
   if (numInA == 0 && numInB == 0)
   {
      return;
   }
   // Easy case:  ListA empty.
   else if (numInA == 0)
   {
      for (long i=0; i<numInB; i++)
      {
         if (useOnlyInB)
            pOnlyInB->addKBSquad(listB[i]);
         // Halwes - 10/7/2008 - This is incorrect logic since one list is empty none of the entities are in both lists.  If fixing this breaks to much stuff
         //                      we will want to add a flag to change the effect's behavior.
         //if (useInBoth)
         //   pInBoth->addKBSquad(listB[i]);
      }
   }
   // Easy case:  ListB empty.
   else if (numInB == 0)
   {
      for (long i=0; i<numInA; i++)
      {
         if (useOnlyInA)
            pOnlyInA->addKBSquad(listA[i]);
         // Halwes - 10/7/2008 - This is incorrect logic since one list is empty none of the entities are in both lists.  If fixing this breaks to much stuff
         //                      we will want to add a flag to change the effect's behavior.
         //if (useInBoth)
         //   pInBoth->addKBSquad(listA[i]);
      }
   }
   // Both lists have something.
   else
   {
      // Check listA's stuff.
      for (long i=0; i<numInA; i++)
      {
         // Couldn't find it, so it's only in A.
         if (listB.find(listA[i]) == cInvalidIndex)
         {
            if (useOnlyInA)
               pOnlyInA->addKBSquad(listA[i]);
         }
         // Otherwise, it's in both lists.
         else
         {
            if (useInBoth)
               pInBoth->addKBSquad(listA[i]);
         }
      }

      // Check listB's stuff.
      for (long i=0; i<numInB; i++)
      {
         // Couldn't find it, so it's only in B.
         if (listA.find(listB[i]) == cInvalidIndex)
         {
            if (useOnlyInB)
               pOnlyInB->addKBSquad(listB[i]);
         }
         // And we already handled the case of being in both lists.
      }
   }   

}


//==============================================================================
//==============================================================================
void BTriggerEffect::teGetKBBaseLocation()
{
   enum { cKBBase = 1, cLocation = 2, };
   const BKBBase* pKBBase = gWorld->getKBBase(getVar(cKBBase)->asKBBase()->readVar());
   if (pKBBase)
      getVar(cLocation)->asVector()->writeVar(pKBBase->getPosition());
}


//==============================================================================
// BTriggerEffect::teTableLoad()
//==============================================================================
void BTriggerEffect::teTableLoad()
{
   enum { cUserClassType = 1, cFilename = 2, cTableName =3, cShared = 4, cID = 5};


   BSimString filename = getVar(cFilename)->asString()->readVar();
   BSimString tablename = getVar(cTableName)->asString()->readVar();

   //BTriggerUserTable* pTable = new BTriggerUserTable();
   //pTable->load(filename);
   //gTriggerManager.getTableManager()->mData.push_back(pTable);

   //ignore shared

   BTriggerUserTableXML* pTable = gTriggerManager.getTableManager()->getTable(filename, tablename);
   
   uint id = gTriggerManager.getTableManager()->getTableID(pTable);


   getVar(cID)->asInteger()->writeVar(id);


}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIGetMemory()
{
   enum { cDidPlayerWin = 1, cPlayer = 2,  cCost = 3, cNumGamesPlayed = 4, cNumGamesWon = 5, cTimePlayerAttacked = 6, cTimeAIAttacked = 7, cPreviousStratsUsed = 8, };
   
//-- FIXING PREFIX BUG ID 2801
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
//--
   BTRIGGER_ASSERT(pPlayer);
   if (!pPlayer || !pPlayer->getTrackingData()) //!pPlayer->getUser() || !pPlayer->getUser()->getProfile())
      return;

   //BTriggerVarBool* pVarDidPlayerWin = getVar(cDidPlayerWin)->asBool();
   //if(pVarDidPlayerWin->isUsed())
   //   pVarDidPlayerWin->writeVar(pPlayer->getTrackingData()->getDidIWinLastGameVSAI());
   
   //BTriggerVarInteger* pVarNumGamesPlayed = getVar(cNumGamesPlayed)->asInteger();
   //if(pVarNumGamesPlayed->isUsed())
   //   pVarNumGamesPlayed->writeVar(pPlayer->getTrackingData()->getNumGamesPlayedVSAI());
   
   //BTriggerVarInteger* pVarNumGamesAIWonVSMe = getVar(cNumGamesWon)->asInteger();
   //if(pVarNumGamesAIWonVSMe->isUsed())
   //   pVarNumGamesAIWonVSMe->writeVar(pPlayer->getTrackingData()->getNumGamesTheAIWonVSMe());
   
   //BTriggerVarTime* pVarTimePlayerFirstAttackedAI = getVar(cTimePlayerAttacked)->asTime();
   //if(pVarTimePlayerFirstAttackedAI->isUsed())
   //   pVarTimePlayerFirstAttackedAI->writeVar(pPlayer->getTrackingData()->getFirstTimeIAttackedAI());
   
   //BTriggerVarTime* pVarTimeAIFirstAttackedPlayer = getVar(cTimeAIAttacked)->asTime();
   //if(pVarTimeAIFirstAttackedPlayer->isUsed())
   //   pVarTimeAIFirstAttackedPlayer->writeVar(pPlayer->getTrackingData()->getFirstTimeAIAttackedMe());

   //BTriggerVarIntegerList* pVarPreviousStratsUsedVsPlayer = getVar(cPreviousStratsUsed)->asIntegerList();
   //if(pVarPreviousStratsUsedVsPlayer->isUsed())
   //{
   //   BInt32Array stratList(1);
   //   for (uint i=0; i<1; i++)
   //      stratList[i] = pPlayer->getTrackingData()->getPreviousStratsAIUsedOnMe();
//
 //     pVarPreviousStratsUsedVsPlayer->writeVar(stratList);
  // }

   // [5/6/2008 JRuediger] We don't pull info from these yet.
   // DEFINE_USERPROFILE_DATA(BStatRecorder::BStatTotalHashMap, RolledupSquadTotals, 2);
   // DEFINE_USERPROFILE_DATA(BStatRecorder::BStatTotalHashMap, RolledupTechTotals, 2);
   // DEFINE_USERPROFILE_DATA(BStatRecorder::BStatTotalHashMap, RolledupObjectTotals, 2);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teBidCreatePower()
{
   enum { cPlayerID = 1, cProtoPowerID = 2, cPriority = 3, cBidID = 4, };

   BBidID bidID = cInvalidBidID;
   BBid* pBid = gWorld->createBid(getVar(cPlayerID)->asPlayer()->readVar());
   if (pBid)
   {
      bidID = pBid->getID();
      pBid->setPriority(getVar(cPriority)->asFloat()->readVar());
      pBid->setPowerToCast(getVar(cProtoPowerID)->asPower()->readVar());
   }
   getVar(cBidID)->asInteger()->writeVar(bidID);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teBidSetPower()
{
   enum { cBidID = 1, cProtoPowerID = 2, };
   BBid* pBid = gWorld->getBid(getVar(cBidID)->asInteger()->readVar());
   if (pBid)
      pBid->setPowerToCast(getVar(cProtoPowerID)->asPower()->readVar());
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teBidAddToMissions()
{
   enum { cBid = 1, cBidList = 2, cMission = 3, cMissionList = 4, };
//-- FIXING PREFIX BUG ID 2803
   const BTriggerVarInteger* pVarBid = getVar(cBid)->asInteger();
//--
   BTriggerVarIntegerList* pVarBidList = getVar(cBidList)->asIntegerList();
//-- FIXING PREFIX BUG ID 2804
   const BTriggerVarInteger* pVarMission = getVar(cMission)->asInteger();
//--
   BTriggerVarIntegerList* pVarMissionList = getVar(cMissionList)->asIntegerList();

   BInt32Array workingBids;
   if (pVarBidList->isUsed())
      workingBids = pVarBidList->readVar();
   if (pVarBid->isUsed())
      workingBids.add(pVarBid->readVar());

   BInt32Array workingMissions;
   if (pVarMissionList->isUsed())
      workingMissions = pVarMissionList->readVar();
   if (pVarMission->isUsed())
      workingMissions.add(pVarMission->readVar());

   uint numBids = workingBids.getSize();
   uint numMissions = workingMissions.getSize();
   for (uint i=0; i<numBids; i++)
   {
      BBidID bidID = workingBids[i];
      if (!gWorld->getBid(bidID))
         continue;

      for (uint j=0; j<numMissions; j++)
      {
         BAIMissionID missionID = workingMissions[j];
         BAIMission* pMission = gWorld->getAIMission(missionID);
         if (pMission)
            pMission->addBid(bidID);
      }
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teBidRemoveFromMissions()
{
   enum { cBid = 1, cBidList = 2, cMission = 3, cMissionList = 4, };
//-- FIXING PREFIX BUG ID 2805
   const BTriggerVarInteger* pVarBid = getVar(cBid)->asInteger();
//--
   BTriggerVarIntegerList* pVarBidList = getVar(cBidList)->asIntegerList();
//-- FIXING PREFIX BUG ID 2806
   const BTriggerVarInteger* pVarMission = getVar(cMission)->asInteger();
//--
   BTriggerVarIntegerList* pVarMissionList = getVar(cMissionList)->asIntegerList();

   BInt32Array workingBids;
   if (pVarBidList->isUsed())
      workingBids = pVarBidList->readVar();
   if (pVarBid->isUsed())
      workingBids.add(pVarBid->readVar());

   BInt32Array workingMissions;
   if (pVarMissionList->isUsed())
      workingMissions = pVarMissionList->readVar();
   if (pVarMission->isUsed())
      workingMissions = pVarMission->readVar();

   uint numBids = workingBids.getSize();
   uint numMissions = workingMissions.getSize();
   for (uint i=0; i<numBids; i++)
   {
      BBidID bidID = workingBids[i];
      if (!gWorld->getBid(bidID))
         continue;

      for (uint j=0; j<numMissions; j++)
      {
         BAIMissionID missionID = workingMissions[j];
         BAIMission* pMission = gWorld->getAIMission(missionID);
         if (pMission)
            pMission->removeBid(bidID);
      }
   }
}


//==============================================================================
// Set or clear the MoveAttack attribute on a mission
//==============================================================================
void BTriggerEffect::teAIMissionSetMoveAttack()
{
   enum
   {
      cPlayerID = 1,
      cMissionID = 2,
      cMoveAttack = 3,
   };

   bool moveAttack = getVar(cMoveAttack)->asBool()->readVar();
   BAIMission* pMission = gWorld->getAIMission(getVar(cMissionID)->asInteger()->readVar());
   if (pMission)
      pMission->setFlagMoveAttack(moveAttack);
}


//==============================================================================
// Add squads to the KB manually.  ONLY do this if it is not cheating i.e. deterministic creeps
//==============================================================================
void BTriggerEffect::teKBAddSquadsToKB()
{
   enum
   {
      cPlayer           = 1,
      cSquadList        = 2,
   };
   BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
   if (!pPlayer)
      return;
   BKB* pKB = gWorld->getKB(pPlayer->getTeamID());
   if (!pKB)
      return;

   const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
   for(unsigned int i=0; i< squadList.size(); i++)
   {
      BEntityID id = squadList[i];
      BSquad *pSquad = gWorld->getSquad(id);
      if (!pSquad)
         continue;
      pKB->updateSquad(pSquad);
   }
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teBidSetPadSupplies()
{
   enum { cBidID = 1, cPadSupplies = 2, };
   BBidID bidID = getVar(cBidID)->asInteger()->readVar();
   BBid* pBid = gWorld->getBid(bidID);
   if (pBid)
   {
      float padSupplies = getVar(cPadSupplies)->asFloat()->readVar();
      if (padSupplies >= 0.0f)
         pBid->setPadSupplies(padSupplies);
      else
         pBid->setPadSupplies(0.0f);
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teAIFactoidSubmit()
{
   enum { cPlayerList = 8, cFromPlayer=1, cEventName=2, cPriority=3, cDeadline=4, cDestination=5, cSource=6, cShowArrow=7  };
 
   bool isPriorityUsed = getVar(cPriority)->isUsed();
   bool isDestinationUsed = getVar(cDestination)->isUsed();
   bool isSourceUsed = getVar(cSource)->isUsed();
   bool isShowArrowUsed = getVar(cShowArrow)->isUsed();
   
   const BPlayerIDArray &playerList = getVar(cPlayerList)->asPlayerList()->readVar();
   long numPlayers = playerList.getNumber();
   for (long i=0; i<numPlayers; i++)
   {
      long playerID = playerList[i];
      BPlayer* pPlayer = gWorld->getPlayer(playerID);
      if(pPlayer == NULL)
         continue;     
      BAIFactoidManager* pFactoidManager = pPlayer->getFactoidManager();
      if(pFactoidManager == NULL)
         continue;

      long fromPlayer = getVar(cFromPlayer)->asPlayer()->readVar();
      BSimString eventName = getVar(cEventName)->asString()->readVar();
      long priority = -1;
      if(isPriorityUsed)
         priority = getVar(cPriority)->asInteger()->readVar();      
      DWORD deadLine = getVar(cDeadline)->asTime()->readVar();
      BVector source = BVector(-1,-1,-1);
      if(isSourceUsed)
         source = getVar(cSource)->asVector()->readVar();
      BVector dest = BVector(-1,-1,-1);
      if(isDestinationUsed)
         dest = getVar(cDestination)->asVector()->readVar();
      bool bShowArrow = false;
      if(isShowArrowUsed)
         bShowArrow = getVar(cShowArrow)->asBool()->readVar();
   
      BString name = eventName;
      pFactoidManager->submitAIFactoidEvent(fromPlayer, name, priority, deadLine, dest, source, bShowArrow);

   }      
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAICreateAreaTarget()
{
   enum { cPlayerID = 1, cPosition = 2, cRadius = 3, cAllowScoring = 4, cDestroyOnNoRefs = 5, cMissionTargetID = 6, };

   BVector pos = getVar(cPosition)->asVector()->readVar();
   float rad = getVar(cRadius)->asFloat()->readVar();
   bool allowScoring = getVar(cAllowScoring)->isUsed() ? getVar(cAllowScoring)->asBool()->readVar() : true;
   bool destroyOnNoRefs = getVar(cDestroyOnNoRefs)->isUsed() ? getVar(cDestroyOnNoRefs)->asBool()->readVar() : false;

   BAIMissionTargetID targetID = cInvalidAIMissionTargetID;
   BAIMissionTarget* pTarget = gWorld->createAIMissionTarget(getVar(cPlayerID)->asPlayer()->readVar());
   if (pTarget)
   {
      targetID = pTarget->getID();
      pTarget->setTargetType(MissionTargetType::cArea);
      pTarget->setMissionType(MissionType::cAttack);
      pTarget->setKBBaseID(cInvalidKBBaseID);
      pTarget->setPosition(pos);
      pTarget->setRadius(rad);
      pTarget->setFlagAllowScoring(allowScoring);
      pTarget->setDestroyOnNoRefs(destroyOnNoRefs);
      #ifndef BUILD_FINAL
         pTarget->mName.format("Trigger Created Area Target At (%4.0f,%4.0f)", pos.x, pos.z);
      #endif
   }

   getVar(cMissionTargetID)->asInteger()->writeVar(targetID);
} 


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIMissionCreate()
{
   enum { cPlayerID = 1, cMissionID = 2, cTopicID = 3, };

   BTriggerVarInteger* pMissionID = getVar(cMissionID)->asInteger();
   BTriggerVarInteger* pTopicID = getVar(cTopicID)->asInteger();
   BPlayerID playerID = getVar(cPlayerID)->asPlayer()->readVar();
   bool bTopicIDUsed = pTopicID->isUsed();

   BAIMissionID missionID = cInvalidAIMissionID;
   BAITopicID topicID = cInvalidAITopicID;

   BAIMission* pMission = gWorld->createAIMission(playerID, MissionType::cAttack);
   BASSERT(pMission);
   if (pMission)
   {
      missionID = pMission->getID();
      if (bTopicIDUsed)
      {
         BAITopic* pAITopic = gWorld->createAITopic(playerID);
         BASSERT(pAITopic);
         if (pAITopic)
         {
            topicID = pAITopic->getID();
            pAITopic->setType(AITopicType::cMission);
            pAITopic->setTriggerScriptID(cInvalidTriggerScriptID);
            pAITopic->setTriggerVarID(cInvalidTriggerVarID);
            pAITopic->setAIMissionID(pMission->getID());
            pMission->setAITopicID(pAITopic->getID());
         }
      }
      // If we have a topic, start inactive and let AIQ activate us.
      pMission->toggleActive(gWorld->getGametime(), !bTopicIDUsed);
      pMission->setState(MissionState::cCreate);
   }

   // Write our results.
   pMissionID->writeVar(missionID);
   if (bTopicIDUsed)
      pTopicID->writeVar(topicID);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAICreateWrapper()
{
   enum { cMissionID = 1, cTargetID = 2, cWrapperID = 3, };
   BAIMissionTargetWrapperID wrapperID = cInvalidAIMissionTargetWrapperID;
   BAIMissionTargetWrapper* pWrapper = gWorld->createWrapper(getVar(cMissionID)->asInteger()->readVar(), getVar(cTargetID)->asInteger()->readVar());
   if (pWrapper)
      wrapperID = pWrapper->getID();
   getVar(cWrapperID)->asInteger()->writeVar(wrapperID);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teAIReorderWrapper()
{
   enum { cWrapperID = 1, cNewPosition = 2, };
   BAIMissionTargetWrapperID wrapperID = getVar(cWrapperID)->asInteger()->readVar();
   const BAIMissionTargetWrapper* pWrapper = gWorld->getWrapper(wrapperID);
   BASSERT(pWrapper);
   if (!pWrapper)
      return;
   BAIMission* pMission = gWorld->getAIMission(pWrapper->getMissionID());
   BASSERT(pMission);
   if (!pMission)
      return;
   uint newIndex = Math::Max(getVar(cNewPosition)->asInteger()->readVar(), 0);
   pMission->reorderTargetWrapper(wrapperID, newIndex);
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teAIDestroyWrapper()
{
   enum { cWrapperID = 1, };
   gWorld->deleteWrapper(getVar(cWrapperID)->asInteger()->readVar());
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIWrapperModifyRadius()
{
   enum { cWrapperID = 1, cLeashDist = 2, cSecureDist = 3, cRallyDist = 4, cSearchWorkDist = 5, cHotZoneDist = 6, };
   BAIMissionTargetWrapperID wrapperID = getVar(cWrapperID)->asInteger()->readVar();
   BAIMissionTargetWrapper* pWrapper = gWorld->getWrapper(wrapperID);
   if (!pWrapper)
      return;

   BTriggerVarFloat* pLeashDist = getVar(cLeashDist)->asFloat();
   if (pLeashDist->isUsed())
      pWrapper->setLeashDist(static_cast<float>(Math::fSelectMax(pLeashDist->readVar(), 0.0f)));

   BTriggerVarFloat* pSecureDist = getVar(cSecureDist)->asFloat();
   if (pSecureDist->isUsed())
      pWrapper->setSecureDist(static_cast<float>(Math::fSelectMax(pSecureDist->readVar(), 0.0f)));

   BTriggerVarFloat* pRallyDist = getVar(cRallyDist)->asFloat();
   if (pRallyDist->isUsed())
      pWrapper->setRallyDist(static_cast<float>(Math::fSelectMax(pRallyDist->readVar(), 0.0f)));

   BTriggerVarFloat* pSearchWorkDist = getVar(cSearchWorkDist)->asFloat();
   if (pSearchWorkDist->isUsed())
      pWrapper->setSearchWorkDist(static_cast<float>(Math::fSelectMax(pSearchWorkDist->readVar(), 0.0f)));

   BTriggerVarFloat* pHotZoneDist = getVar(cHotZoneDist)->asFloat();
   if (pHotZoneDist->isUsed())
      pWrapper->setHotZoneDist(static_cast<float>(Math::fSelectMax(pHotZoneDist->readVar(), 0.0f)));
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIWrapperModifyFlags()
{
   enum { cWrapperID = 1, cFlagAllowRetreat = 2, cFlagAllowSkip = 3, cFlagRequireSecure = 4, cFlagGatherCrates = 5, };
   BAIMissionTargetWrapperID wrapperID = getVar(cWrapperID)->asInteger()->readVar();
   BAIMissionTargetWrapper* pWrapper = gWorld->getWrapper(wrapperID);
   if (!pWrapper)
      return;

   BTriggerVarBool* pFlagAllowRetreat = getVar(cFlagAllowRetreat)->asBool();
   if (pFlagAllowRetreat->isUsed())
      pWrapper->setFlagAllowRetreat(pFlagAllowRetreat->readVar());

   BTriggerVarBool* pFlagAllowSkip = getVar(cFlagAllowSkip)->asBool();
   if (pFlagAllowSkip->isUsed())
      pWrapper->setFlagAllowSkip(pFlagAllowSkip->readVar());

   BTriggerVarBool* pFlagRequireSecure = getVar(cFlagRequireSecure)->asBool();
   if (pFlagRequireSecure->isUsed())
      pWrapper->setFlagRequireSecure(pFlagRequireSecure->readVar());

   BTriggerVarBool* pFlagGatherCrates = getVar(cFlagGatherCrates)->asBool();
   if (pFlagGatherCrates->isUsed())
      pWrapper->setFlagGatherCrates(pFlagGatherCrates->readVar());
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIWrapperModifyParms()
{
   enum { cWrapperID = 1, cTargetID = 2, cMinRetreatRatio = 3, cMinRalliedPercent = 4, cMinSecureTime = 5, };
   BAIMissionTargetWrapperID wrapperID = getVar(cWrapperID)->asInteger()->readVar();
   BAIMissionTargetWrapper* pWrapper = gWorld->getWrapper(wrapperID);
   if (!pWrapper)
      return;

   BTriggerVarInteger* pTargetID = getVar(cTargetID)->asInteger();
   if (pTargetID->isUsed())
      pWrapper->setTargetID(pTargetID->readVar());

   BTriggerVarFloat* pMinRetreatRatio = getVar(cMinRetreatRatio)->asFloat();
   if (pMinRetreatRatio->isUsed())
      pWrapper->setMinRetreatRatio(pMinRetreatRatio->readVar());

   BTriggerVarFloat* pMinRalliedPercent = getVar(cMinRalliedPercent)->asFloat();
   if (pMinRalliedPercent->isUsed())
      pWrapper->setMinRalliedPercent(pMinRalliedPercent->readVar());

   BTriggerVarTime* pMinSecureTime = getVar(cMinSecureTime)->asTime();
   if (pMinSecureTime->isUsed())
      pWrapper->setMinSecureTime(pMinSecureTime->readVar());
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAIAddTeleporterZone()
{
   enum { cTeleporterUnit = 1, cBoxExtent1 = 2, cBoxExtent2 = 3, cSpherePos = 4, cSphereRad = 5, };
   BEntityID unitID = getVar(cTeleporterUnit)->asUnit()->readVar();
   const BUnit* pUnit = gWorld->getUnit(unitID);
   if (!pUnit)
   {
      BFAIL("teAIAddTeleporterZone() - unitID is invalid.");
      return;
   }
   if (!pUnit->isType(gDatabase.getOTIDTeleportPickup()) && !pUnit->isType(gDatabase.getOTIDTeleportDropoff()))
   {
      BFAIL("teAIAddTeleporterZone() - a non-teleporter unitID.");
      return;
   }

   bool useBoxExtent1 = getVar(cBoxExtent1)->isUsed();
   bool useBoxExtent2 = getVar(cBoxExtent2)->isUsed();
   bool useSpherePos = getVar(cSpherePos)->isUsed();
   bool useSphereRad = getVar(cSphereRad)->isUsed();
   BASSERT((useBoxExtent1 && useBoxExtent2) || (useSpherePos && useSphereRad));

   if (useBoxExtent1 && useBoxExtent2)
   {
      BASSERT(!useSpherePos && !useSphereRad);
      BVector boxExtent1 = getVar(cBoxExtent1)->asVector()->readVar();
      BVector boxExtent2 = getVar(cBoxExtent2)->asVector()->readVar();
      gWorld->createAITeleporterZone(unitID, boxExtent1, boxExtent2);
      return;
   }
   else if (useSpherePos && useSphereRad)
   {
      BASSERT(!useBoxExtent1 && !useBoxExtent2);
      BVector spherePos = getVar(cSpherePos)->asVector()->readVar();
      float sphereRad = getVar(cSphereRad)->asFloat()->readVar();
      gWorld->createAITeleporterZone(unitID, spherePos, sphereRad);
      return;
   }   
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAISetPlayerAssetModifier()
{
   enum { cPlayerID = 4, cModifyPlayer = 1, cModifyPlayerList = 2, cModifier = 3 };

   BAI* pAI = gWorld->getAI(getVar(cPlayerID)->asPlayer()->readVar());
   if (!pAI)
      return;
   BAIGlobals* pAIGlobals = pAI->getAIGlobals();
   if (!pAIGlobals)
      return;

   float modifier = getVar(cModifier)->asFloat()->readVar();

   if(getVar(cModifyPlayer)->isUsed())
   {
      BPlayerID playerId = getVar(cModifyPlayer)->asPlayer()->readVar();
      pAIGlobals->setPlayerModifier(playerId, modifier);
   }
   if(getVar(cModifyPlayerList)->isUsed())
   {
      const BPlayerIDArray& playerList = getVar(cModifyPlayerList)->asPlayerList()->readVar();
      for(uint i=0; i<playerList.size(); i++)
      {
         pAIGlobals->setPlayerModifier(playerList.get(i), modifier);         
      }
   }
}

//==============================================================================
//==============================================================================
void BTriggerEffect::teAISetPlayerDamageModifiers()
{
   enum { cPlayerID = 1, cDamageMultiplier = 2, cDamageTakenMultiplier = 3 };

   BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayerID)->asPlayer()->readVar());
   if (!pPlayer)
      return;

   float damageMultiplier = getVar(cDamageMultiplier)->asFloat()->readVar();
   float damageTakenMultiplier = getVar(cDamageTakenMultiplier)->asFloat()->readVar();

   pPlayer->setAIDamageMultiplier(damageMultiplier);
   pPlayer->setAIDamageTakenMultiplier(damageTakenMultiplier); 

}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAISetPlayerBuildSpeedModifiers()
{
   enum { cPlayerID = 1, cBuildSpeedMultiplier = 2, };

   BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayerID)->asPlayer()->readVar());
   if (!pPlayer)
      return;

   float buildSpeedMultiplier = getVar(cBuildSpeedMultiplier)->asFloat()->readVar();
   pPlayer->setAIBuildSpeedMultiplier(buildSpeedMultiplier);
}


//==============================================================================
//==============================================================================
void BTriggerEffect::teAISetWinRange()
{
   enum { cPlayerID = 1, cMin = 2, cMax = 3 };

   BAI* pAI = gWorld->getAI(getVar(cPlayerID)->asPlayer()->readVar());
   if (!pAI)
      return;

   bool bUseMin = getVar(cMin)->isUsed();
   bool bUseMax = getVar(cMax)->isUsed();
   float min = (bUseMin)?getVar(cMin)->asFloat()->readVar():0.0f;
   float max = (bUseMax)?getVar(cMax)->asFloat()->readVar():0.0f;

   pAI->getAIGlobals()->setWinRange(bUseMin,min,bUseMax,max);

}
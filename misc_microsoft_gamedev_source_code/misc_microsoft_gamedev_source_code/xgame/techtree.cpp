//==============================================================================
// techtree.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "techtree.h"
#include "civ.h"
#include "config.h"
#include "configsgame.h"
#include "econfigenum.h"
#include "database.h"
#include "player.h"
#include "protoobject.h"
#include "prototech.h"
#include "team.h"
#include "world.h"
#include "worldsoundmanager.h"

GFIMPLEMENTVERSION(BTechTree, 1);
enum
{
   cSaveMarkerTechTree=10000,
};

//==============================================================================
// BTechTree::BTechTree
//==============================================================================
BTechTree::BTechTree() :
   mpPlayer(NULL),
   mTechCount(0),
   mpTechNodes(NULL),
   mInitTechID(-1)
{
}

//==============================================================================
// BTechTree::~BTechTree
//==============================================================================
BTechTree::~BTechTree()
{
   if (mpTechNodes)
   {
      for (int i=0; i<mTechCount; i++)
      {
         BTechNode* pTechNode=&(mpTechNodes[i]);
         if (pTechNode->mpUniqueNodes)
         {
            delete pTechNode->mpUniqueNodes;
            pTechNode->mpUniqueNodes=NULL;
         }
      }
      delete[] mpTechNodes;
      mpTechNodes=NULL;
   }
}

//==============================================================================
// BTechTree::init
//==============================================================================
bool BTechTree::init(BPlayer* pPlayer, bool fromSave)
{
   mpPlayer=pPlayer;

   // Build a list of tech nodes
   mTechCount=gDatabase.getNumberProtoTechs();
   if(mTechCount==0)
      return true;

   mpTechNodes=new BTechNode[mTechCount];
   if(!mpTechNodes)
      return false;

#ifdef SYNC_Tech
   syncTechData("BTechTree::init playerID", mpPlayer->getID());
#endif

   if (!fromSave)
   {
      for(long techID=0; techID<mTechCount; techID++)
      {
         mInitTechID=techID;
         BTechNode* pTechNode=&(mpTechNodes[techID]);
//-- FIXING PREFIX BUG ID 5074
         const BProtoTech* pProtoTech=gDatabase.getProtoTech(techID);
//--
         #ifdef SYNC_Tech
            syncTechData("BTechTree::init playerID", mpPlayer->getID());
            syncTechData("BTechTree::init techID", techID);
            syncTechData("BTechTree::init techName", pProtoTech->getName());
            syncTechData("BTechTree::init getFlagUnobtainable", pProtoTech->getFlagUnobtainable());
            syncTechData("BTechTree::init getFlagUnique", pProtoTech->getFlagUnique());
         #endif
         pTechNode->mUnique=pProtoTech->getFlagUnique();
         if(pProtoTech->getFlagUnobtainable())
            pTechNode->mStatus=cStatusUnobtainable;
         else
         {
            pTechNode->mStatus=cStatusObtainable;
            if(!pTechNode->mUnique)
               checkTech(techID, -1, pTechNode);
         }
      }
   }

   mInitTechID=-1;

   return true;
}

//==============================================================================
// BTechTree::getTechNode
//==============================================================================
BTechNode* BTechTree::getTechNode(long techID, long unitID, bool forceAdd)
{
   if(techID<0 || techID>=mTechCount)
      return NULL;

   BTechNode* pTechNode=&(mpTechNodes[techID]);
   if(!pTechNode->mUnique || unitID==-1)
      return pTechNode;

   if(pTechNode->mpUniqueNodes!=NULL)
   {
      long count=pTechNode->mpUniqueNodes->getNumber();
      for(long i=0; i<count; i++)
      {
         BTechUnique* pUniqueNode=&(pTechNode->mpUniqueNodes->get(i));
         if(pUniqueNode->mUnitID==unitID)
         {
            if(forceAdd)
            {
               if(pTechNode->mStatus==cStatusUnobtainable)
                  pUniqueNode->mStatus=cStatusUnobtainable;
               else if(pTechNode->mStatus==cStatusAvailable)
                  pUniqueNode->mStatus=cStatusAvailable;
               else
                  pUniqueNode->mStatus=cStatusObtainable;
               pUniqueNode->mResearchPoints=0.0f;
               pUniqueNode->mResearchBuilding=-1;
               if(pUniqueNode->mStatus==cStatusObtainable)
                  checkTech(techID, unitID, pUniqueNode);
            }
            return pUniqueNode;
         }
      }
   }
   else
   {
      pTechNode->mpUniqueNodes=new BDynamicSimArray<BTechUnique>;
      if(!pTechNode->mpUniqueNodes)
         return NULL;
   }

   BTechUnique node;
   node.mUnitID=unitID;
   node.mUnique=true;
   if(pTechNode->mStatus==cStatusUnobtainable)
      node.mStatus=cStatusUnobtainable;
   else if(pTechNode->mStatus==cStatusAvailable)
      node.mStatus=cStatusAvailable;
   else
      node.mStatus=cStatusObtainable;
   node.mResearchPoints=0.0f;
   node.mResearchBuilding=-1;
   node.mpUniqueNodes=NULL;
   long id=pTechNode->mpUniqueNodes->add(node);
   if(id==-1)
      return NULL;

   BTechNode* pUniqueNode=&(pTechNode->mpUniqueNodes->get(id));

   if(node.mStatus==cStatusObtainable)
      checkTech(techID, unitID, pUniqueNode);

   return pUniqueNode;
}

//==============================================================================
// BTechTree::getTechNodeConst
//==============================================================================
const BTechNode* BTechTree::getTechNodeConst(long techID, long unitID) const
{
   if(techID<0 || techID>=mTechCount)
      return NULL;

   const BTechNode* pTechNode=&(mpTechNodes[techID]);
   if(!pTechNode->mUnique || unitID==-1)
      return pTechNode;

   if(pTechNode->mpUniqueNodes!=NULL)
   {
      long count=pTechNode->mpUniqueNodes->getNumber();
      for(long i=0; i<count; i++)
      {
         const BTechUnique* pUniqueNode=&(pTechNode->mpUniqueNodes->get(i));
         if(pUniqueNode->mUnitID==unitID)
            return pUniqueNode;
      }
   }

   return NULL;
}

//==============================================================================
// BTechTree::addUnitRef
//==============================================================================
void BTechTree::addUnitRef(long techID, long unitID)
{
   getTechNode(techID, unitID, true);
}

//==============================================================================
// BTechTree::activateTech
//==============================================================================
bool BTechTree::activateTech(long techID, long unitID, bool noCost, bool fromCoop)
{
   BTechNode* pTechNode=getTechNode(techID, unitID);
   if(!pTechNode)
   {
#ifndef BUILD_FINAL
      if(gConfig.isDefined(cConfigTechLog))
         gConsole.output(cChannelCombat, "tech activate playerID=%d techID=%d unitID=%d error - tech node not found", mpPlayer->getID(), techID, unitID);
#endif
      return false;
   }

#ifndef BUILD_FINAL
   if(gConfig.isDefined(cConfigTechLog))
   {
//-- FIXING PREFIX BUG ID 5075
      const BProtoTech* pProtoTech=mpPlayer->getProtoTech(techID);
//--
      gConsole.output(cChannelCombat, "tech activate playerID=%d techID=%d (%s) unitID=%d oldStatus=%d newStatus=%d", mpPlayer->getID(), techID, pProtoTech ? pProtoTech->getName().getPtr() : "Unknown", unitID, pTechNode->mStatus, cStatusActive);
   }
#endif

   pTechNode->mStatus=cStatusActive;

   BProtoTech* pProtoTech=mpPlayer->getProtoTech(techID);

#ifdef SYNC_Tech
   syncTechData("BTechTree::activateTech playerID", mpPlayer->getID());
   syncTechData("BTechTree::activateTech techID", techID);
   syncTechData("BTechTree::activateTech techName", pProtoTech->getName());
#endif

   if (!fromCoop)
      pProtoTech->applyEffects(this, unitID, noCost);

   // Update the visual for any units that use this tech in their logic tree
   if (pProtoTech->getNumberVisualLogicProtoUnits() > 0)
   {
      for (uint i=0; i<pProtoTech->getNumberVisualLogicProtoUnits(); i++)
         mpPlayer->recomputeUnitVisuals(pProtoTech->getVisualLogicProtoUnit(i));
   }
   gWorld->notify2(BEntity::cEventTechResearched, mpPlayer->getID(), pProtoTech->getID());
   gWorld->notify(BEntity::cEventTechResearched, BEntityID(unitID), techID, mpPlayer->getID());

   // Make dependent techs available if all prereqs are met
   long techCount=pProtoTech->getNumberDependentTechs();
   for(long i=0; i<techCount; i++)
   {
      long dependentTechID=pProtoTech->getDependentTech(i);
//-- FIXING PREFIX BUG ID 5076
      const BProtoTech* pDependentProtoTech=mpPlayer->getProtoTech(dependentTechID);
//--
      if(pDependentProtoTech->getFlagUnique())
      {
         BTechNode* pDependentTechNode=&(mpTechNodes[dependentTechID]);
         if(pDependentTechNode->mpUniqueNodes)
         {
            uint uniqueCount=pDependentTechNode->mpUniqueNodes->getNumber();
            for(uint j=0; j<uniqueCount; j++)
            {
               BTechUnique* pUniqueNode=&(pDependentTechNode->mpUniqueNodes->get(j));
               if(pUniqueNode->mStatus==cStatusObtainable && pUniqueNode->mUnitID!=unitID)
                  checkTech(dependentTechID, pUniqueNode->mUnitID, pUniqueNode, fromCoop);
            }
         }
      }
      else
         checkTech(dependentTechID, unitID, NULL, fromCoop);
   }

   if(pProtoTech->getFlagPerpetual())
      pTechNode->mStatus=cStatusAvailable;

   return true;
}

//==============================================================================
// BTechTree::deactivateTech
//==============================================================================
bool BTechTree::deactivateTech(long techID, long unitID)
{
   BTechNode* pTechNode=getTechNode(techID, unitID);
   if(!pTechNode)
      return false;

   pTechNode->mStatus=cStatusUnobtainable;

   BProtoTech* pProtoTech=mpPlayer->getProtoTech(techID);
   pProtoTech->unapplyEffects(this, unitID);

   //FIXME AJL 5/1/06 - Possibly need to check dependent techs to see if they
   // should be made no longer available. What about if they have already been
   // researched?

   return true;
}

//==============================================================================
// BTechTree::researchTech
//==============================================================================
bool BTechTree::researchTech(long techID, long unitID, bool fromCoop)
{
   BTechNode* pTechNode=getTechNode(techID, unitID);
   if(!pTechNode)
      return false;
   if (fromCoop)
      pTechNode->mStatus=cStatusCoopResearching;
   else
      pTechNode->mStatus=cStatusResearching;
   pTechNode->mResearchBuilding=unitID;
   return true;
}

//==============================================================================
// BTechTree::unresearchTech
//==============================================================================
bool BTechTree::unresearchTech(long techID, long unitID)
{
   BTechNode* pTechNode=getTechNode(techID, unitID);
   if(!pTechNode)
      return false;
   pTechNode->mStatus=cStatusAvailable;
   pTechNode->mResearchBuilding=-1;
   pTechNode->mResearchPoints=0.0f;
   return true;
}

//==============================================================================
// BTechTree::makeObtainable
//==============================================================================
bool BTechTree::makeObtainable(long techID, long unitID)
{
   BTechNode* pTechNode=getTechNode(techID, unitID);
   if(!pTechNode)
      return false;
   pTechNode->mStatus=cStatusObtainable;
   checkTech(techID, unitID, NULL);
   return true;
}

//==============================================================================
// BTechTree::getTechStatus
//==============================================================================
long BTechTree::getTechStatus(long techID, long unitID) const
{
   const BTechNode* pTechNode=getTechNodeConst(techID, unitID);
   if(!pTechNode)
      return cStatusUnobtainable;
   return pTechNode->mStatus;
}

//==============================================================================
// BTechTree::checkTech
//==============================================================================
bool BTechTree::checkTech(long techID, long unitID, BTechNode* pTechNode, bool fromCoop)
{
   // During tech tree init, don't check techs that are later in the list than the curren tech.
   if (mInitTechID!=-1 && techID>mInitTechID)
      return false;

//-- FIXING PREFIX BUG ID 5078
   const BProtoTech* pProtoTech=mpPlayer->getProtoTech(techID);
//--

#ifdef SYNC_Tech
   syncTechData("BTechTree::checkTech playerID", mpPlayer->getID());
   syncTechData("BTechTree::checkTech techID", techID);
   syncTechData("BTechTree::checkTech techName", pProtoTech->getName());
   syncTechData("BTechTree::checkTech unitID", unitID);
   syncTechData("BTechTree::checkTech getFlagForbid", pProtoTech->getFlagForbid());
   syncTechData("BTechTree::checkTech getFlagOrPrereqs", pProtoTech->getFlagOrPrereqs());
   if (pTechNode)
   {
      syncTechData("BTechTree::checkTech techStatus", pTechNode->mStatus);
   }
#endif

   if(pProtoTech->getFlagForbid())
      return false;

   if(pProtoTech->getFlagUnique() && unitID==-1)
      return false;

   int alpha = pProtoTech->getAlpha();
   if (alpha != -1)
   {
      if (alpha == 0)
      {
         if (gConfig.isDefined(cConfigAlpha))
            return false;
      }
      if (alpha == 1)
      {
         if (!gConfig.isDefined(cConfigAlpha))
            return false;
      }
   }

   bool orPrereqs=pProtoTech->getFlagOrPrereqs();
   bool anyMet=false;

   // First check tech prereqs
   long prereqCount=pProtoTech->getNumberTechPrereqs();
   for(long k=0; k<prereqCount; k++)
   {
      long techPrereq=pProtoTech->getTechPrereq(k);
      long techStatus=getTechStatus(techPrereq, unitID);
#ifdef SYNC_Tech
      syncTechData("BTechTree::checkTech techPrereq", techPrereq);
      syncTechData("BTechTree::checkTech techStatus", techStatus);
#endif
      if(techStatus!=cStatusActive)
      {
         if(!orPrereqs)
            return false;
      }
      else
      {
         if(orPrereqs)
         {
            anyMet=true;
            break;
         }
      }
   }

#ifdef SYNC_Tech
   syncTechData("BTechTree::checkTech anyMet", anyMet);
#endif

   if(!orPrereqs || !anyMet)
   {
      // Now check unit prereqs
      prereqCount=pProtoTech->getNumberUnitPrereqs();
      for(long k=0; k<prereqCount; k++)
      {
         DWORD unitPrereq=pProtoTech->getUnitPrereq(k);
         long opType=unitPrereq>>28;
         uint prereqCount=((long)((unitPrereq&0x0FFF0000)>>16))-2047;
         BProtoObjectID protoObjectID=(unitPrereq&0x0000FFFF);
         uint unitCount=mpPlayer->getNumUnitsOfType(protoObjectID);
         BPlayerID coopPlayerID=mpPlayer->getCoopID();
         // If this is a co-op game and the unit is a building, then add in the co-op players unit count
         if (coopPlayerID!=cInvalidPlayerID)
         {
//-- FIXING PREFIX BUG ID 5077
            const BProtoObject* pProtoObject=mpPlayer->getProtoObject(protoObjectID);
//--
            if (pProtoObject->getObjectClass()==cObjectClassBuilding)
               unitCount += gWorld->getPlayer(coopPlayerID)->getNumUnitsOfType(protoObjectID);
         }
         bool met=false;
         if(opType==1) // gt (greater than)
         {
            if(unitCount>prereqCount)
               met=true;
         }
         else if(opType==2) // lt (less than)
         {
            if(unitCount<prereqCount)
               met=true;
         }
         else // e (equal)
         {
            if(unitCount==prereqCount)
               met=true;
         }
#ifdef SYNC_Tech
         syncTechData("BTechTree::checkTech k", k);
         syncTechData("BTechTree::checkTech met", met);
         syncTechData("BTechTree::checkTech protoObjectID", protoObjectID);
         syncTechData("BTechTree::checkTech unitCount", (DWORD)unitCount);
#endif
         if(!met)
         {
            if(!orPrereqs)
               return false;
         }
         else
         {
            if(orPrereqs)
            {
               anyMet=true;
               break;
            }
         }
      }
   }

#ifdef SYNC_Tech
   syncTechData("BTechTree::checkTech anyMet", anyMet);
#endif

   if(orPrereqs && !anyMet)
      return false;

   if(!pTechNode)
   {
      pTechNode=getTechNode(techID, unitID);
      if(!pTechNode)
         return false;
   }
#ifdef SYNC_Tech
   syncTechData("BTechTree::checkTech pTechNode->mStatus", pTechNode->mStatus);
   syncTechData("BTechTree::checkTech pProtoTech->getFlagShadow", pProtoTech->getFlagShadow());
#endif
   if(pTechNode->mStatus!=cStatusObtainable)
      return false;
   if(pProtoTech->getFlagShadow())
      activateTech(techID, unitID, false, fromCoop);
   else
      pTechNode->mStatus=cStatusAvailable;
   return true;
}

//==============================================================================
// BTechTree::setResearchPoints
//==============================================================================
void BTechTree::setResearchPoints(long techID, long unitID, float points)
{
   BTechNode* pTechNode=getTechNode(techID, unitID);
   if(pTechNode)
      pTechNode->mResearchPoints=points;
}

//==============================================================================
// BTechTree::getResearchPercent
//==============================================================================
float BTechTree::getResearchPercent(long techID, long unitID) const
{
   const BTechNode* pTechNode=getTechNodeConst(techID, unitID);
   if(pTechNode)
   {
      const BProtoTech* pProtoTech=mpPlayer->getProtoTech(techID);
      float totalPoints=pProtoTech->getResearchPoints();
      if(totalPoints==0.0f)
         return 0.0f;
      else
         return pTechNode->mResearchPoints/totalPoints;
   }
   else
      return 0.0f;
}

//==============================================================================
// BTechTree::getResearchBuilding
//==============================================================================
long BTechTree::getResearchBuilding(long techID, long unitID) const
{
   const BTechNode* pTechNode=getTechNodeConst(techID, unitID);
   if(pTechNode)
      return pTechNode->mResearchBuilding;
   else
      return -1;
}

//==============================================================================
// BTechTree::checkUnitPrereq
//==============================================================================
void BTechTree::checkUnitPrereq(long protoUnitID)
{
   BTechUnitDependencyArray* pArray=gDatabase.getTechUnitDependencies(protoUnitID);
   if(!pArray)
      return;

   uint count=pArray->getNumber();
   for(uint i=0; i<count; i++)
   {
      long techID=pArray->get(i);
//-- FIXING PREFIX BUG ID 5080
      const BProtoTech* pProtoTech=mpPlayer->getProtoTech(techID);
//--
      BTechNode* pTechNode=&(mpTechNodes[techID]);
      if(pProtoTech->getFlagUnique())
      {
         if (pTechNode->mpUniqueNodes)
         {
            uint uniqueCount=pTechNode->mpUniqueNodes->getNumber();
            for(uint i=0; i<uniqueCount; i++)
            {
               BTechUnique* pUniqueNode=&(pTechNode->mpUniqueNodes->get(i));
               if(pUniqueNode->mStatus==cStatusObtainable)
                  checkTech(techID, pUniqueNode->mUnitID, pUniqueNode);
            }
         }
      }
      else
      {
         if(pTechNode->mStatus==cStatusObtainable)
            checkTech(techID, -1, pTechNode);
      }
   }

   // Co-op support
   if (gWorld->getFlagCoop())
   {
      BPlayerID coopPlayerID=mpPlayer->getCoopID();
      if (coopPlayerID==cInvalidPlayerID)
      {
//-- FIXING PREFIX BUG ID 5081
         const BTeam* pTeam=mpPlayer->getTeam();
//--
         BPlayerID thisPlayerID=mpPlayer->getID();
         for (int i=0; i<pTeam->getNumberPlayers(); i++)
         {
            BPlayerID teamPlayerID=pTeam->getPlayerID(i);
            if (teamPlayerID==thisPlayerID)
               continue;
            BPlayer* pTeamPlayer=gWorld->getPlayer(teamPlayerID);
            if (pTeamPlayer->getCoopID()==thisPlayerID)
            {
               BTechTree* pTechTree=pTeamPlayer->getTechTree();
               if (pTechTree)
                  pTechTree->checkUnitPrereq(protoUnitID);
            }
         }
      }
   }
}

//==============================================================================
//==============================================================================
bool BTechTree::save(BStream* pStream, int saveType) const
{
   int16 techCount = (int16)mTechCount;
   GFWRITEVAR(pStream, int16, techCount);
   GFVERIFYCOUNT(techCount, 10000);
   for (int16 i=0; i<techCount; i++)
   {
      const BTechNode* pTechNode=&(mpTechNodes[i]);
      int16 uniqueCount = (pTechNode->mpUniqueNodes ? (int16)pTechNode->mpUniqueNodes->getNumber() : 0);
      GFWRITEVAR(pStream, int16, uniqueCount);
      GFVERIFYCOUNT(uniqueCount, 10000);
      if (uniqueCount > 0)
      {
         for (int16 j=0; j<uniqueCount; j++)
         {
            const BTechUnique& uniqueNode = pTechNode->mpUniqueNodes->get(j);
            GFWRITEVAR(pStream, long, uniqueNode.mUnitID);
            GFWRITEVAR(pStream, float, uniqueNode.mResearchPoints);
            GFWRITEVAR(pStream, long, uniqueNode.mResearchBuilding);
            GFWRITEVAR(pStream, BYTE, uniqueNode.mStatus);
            GFWRITEVAR(pStream, bool, uniqueNode.mUnique);
         }
      }
      GFWRITEVAR(pStream, float, pTechNode->mResearchPoints);
      GFWRITEVAR(pStream, long, pTechNode->mResearchBuilding);
      GFWRITEVAR(pStream, BYTE, pTechNode->mStatus);
      GFWRITEVAR(pStream, bool, pTechNode->mUnique);
   }
   GFWRITEMARKER(pStream, cSaveMarkerTechTree);
   return true;
}

//==============================================================================
//==============================================================================
bool BTechTree::load(BStream* pStream, int saveType)
{
   BTechNode tempTechNode;
   int16 techCount;
   GFREADVAR(pStream, int16, techCount);
   GFVERIFYCOUNT(techCount, 10000);
   for (int16 i=0; i<techCount; i++)
   {
      int techID = gSaveGame.getProtoTechID(i);
      BTechNode* pTechNode = (techID != -1 ? &(mpTechNodes[techID]) : &tempTechNode);
      int16 uniqueCount;
      GFREADVAR(pStream, int16, uniqueCount);
      GFVERIFYCOUNT(uniqueCount, 10000);
      if (uniqueCount > 0)
      {
         if (!pTechNode->mpUniqueNodes)
         {
            pTechNode->mpUniqueNodes=new BDynamicSimArray<BTechUnique>;
            if(!pTechNode->mpUniqueNodes)
               return false;
         }
         if (!pTechNode->mpUniqueNodes->setNumber(uniqueCount))
            return false;
         for (int16 j=0; j<uniqueCount; j++)
         {
            BTechUnique& uniqueNode = pTechNode->mpUniqueNodes->get(j);
            GFREADVAR(pStream, long, uniqueNode.mUnitID);
            GFREADVAR(pStream, float, uniqueNode.mResearchPoints);
            GFREADVAR(pStream, long, uniqueNode.mResearchBuilding);
            GFREADVAR(pStream, BYTE, uniqueNode.mStatus);
            GFREADVAR(pStream, bool, uniqueNode.mUnique);
         }
      }
      else
      {
         if (pTechNode->mpUniqueNodes)
            pTechNode->mpUniqueNodes->clear();
      }
      GFREADVAR(pStream, float, pTechNode->mResearchPoints);
      GFREADVAR(pStream, long, pTechNode->mResearchBuilding);
      GFREADVAR(pStream, BYTE, pTechNode->mStatus);
      GFREADVAR(pStream, bool, pTechNode->mUnique);
   }
   GFREADMARKER(pStream, cSaveMarkerTechTree);
   return true;
}

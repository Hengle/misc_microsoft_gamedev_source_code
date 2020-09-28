//==============================================================================
// squadactionrepair.cpp
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "civ.h"
#include "entity.h"
#include "protoobject.h"
#include "protosquad.h"
#include "selectionmanager.h"
#include "squad.h"
#include "squadactionrepair.h"
#include "unit.h"
#include "user.h"
#include "usermanager.h"
#include "world.h"
#include "game.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BSquadActionRepair, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BSquadActionRepair::init(void)
{
   if (!BAction::init())
      return(false);
   mUnits.clear();
   mReinforcements.clear();
   mTimer = 0.0f;
   mUnitOppID = BUnitOpp::cInvalidID;
   mDamagedTime = 0;
   mRepairPercent = 0.0f;
   mReinforceIndex = 0;
   mSaveFlagNonMobile = false;
   mTargetSquad.invalidate();
   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionRepair::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return(false);
   BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
   InitFromSquad(pSquad);

   addUnitOpp();
   setState(cStateWorking);
   gWorld->notify(BEntity::cEventRepair, pOwner->getID(), 0, 0);
   return(true);
}

//==============================================================================
void BSquadActionRepair::InitFromSquad(BSquad* pSquad)
{
   mTargetSquad = pSquad->getID();

   mSaveFlagNonMobile = pSquad->getFlagNonMobile();
   pSquad->setFlagNonMobile(true);
   pSquad->setFlagIsRepairing(true);
   mDamagedTime = pSquad->getLastDamagedTime();

   // Build a list of damaged units.
   float damagedHitpoints = 0.0f;
   uint numChildren=pSquad->getNumberChildren();
   BSmallDynamicSimArray<bool> childCounted;
   childCounted.setNumber(numChildren);
   for (uint j=0; j<numChildren; j++)
   {
      childCounted[j]=false;
//-- FIXING PREFIX BUG ID 2121
      const BUnit* pUnit=gWorld->getUnit(pSquad->getChild(j));
//--
      if (!pUnit) 
         continue;
      float unitHitpoints = pUnit->getHitpoints();
      float baseHitpoints = pUnit->getProtoObject()->getHitpoints();
      if (unitHitpoints < baseHitpoints)
      {
         damagedHitpoints += (baseHitpoints - unitHitpoints);
         BSquadActionRepairUnit unit;
         unit.mUnitID = pUnit->getID();
         unit.mHitpoints = pUnit->getHitpoints();
         mUnits.add(unit);
      }
   }

   // Build a list of dead units to reinforce.
   float reinforceHitpoints = 0.0f;
   const BProtoSquad* pProtoSquad=pSquad->getProtoSquad();
   int nodeCount = pProtoSquad->getNumberUnitNodes();
   for (int i=0; i<nodeCount; i++)
   {
      const BProtoSquadUnitNode& node=pProtoSquad->getUnitNode(i);
//-- FIXING PREFIX BUG ID 2123
      const BProtoObject* pProtoObject = pSquad->getPlayer()->getProtoObject(node.mUnitType);
//--
      if (!pProtoObject)
         continue;
      int unitCount = 0;
      for (uint j=0; j<numChildren; j++)
      {
         if (childCounted[j])
            continue;
//-- FIXING PREFIX BUG ID 2122
         const BUnit* pUnit=gWorld->getUnit(pSquad->getChild(j));
//--
         if (pUnit && pUnit->getProtoID()==node.mUnitType)
         {
            unitCount++;
            childCounted[j]=true;
         }
      }
      if (unitCount < node.mUnitCount)
      {
         int reinforceCount = node.mUnitCount - unitCount;
         for (int j=0; j<reinforceCount; j++)
            mReinforcements.add(node.mUnitType);
         reinforceHitpoints += (reinforceCount * pProtoObject->getHitpoints());
      }
   }

   // Compute the percentage of time that should be spent repairing verses reinforcing.
   mRepairPercent = 1.0f;
   if (mReinforcements.getNumber() > 0 && (damagedHitpoints + reinforceHitpoints) > 0.0f)
      mRepairPercent = damagedHitpoints / (damagedHitpoints + reinforceHitpoints);
}

//==============================================================================
//==============================================================================
void BSquadActionRepair::disconnect()
{
   removeUnitOpp();
   BSquad* pSquad = gWorld->getSquad(mTargetSquad);
   BASSERT(pSquad != NULL);
   if (pSquad != NULL)
   {
      pSquad->setFlagNonMobile(mSaveFlagNonMobile);
      pSquad->setFlagIsRepairing(false);
   }
   BAction::disconnect();
}

//==============================================================================
//==============================================================================
bool BSquadActionRepair::update(float elapsed)
{
   BSquad* pSquad = gWorld->getSquad(mTargetSquad); 

   switch (mState)
   {
      case cStateWorking:
      {
         if (pSquad->getLastDamagedTime() > mDamagedTime)
         {
            setState(cStateFailed);
            break;
         }

         mTimer += gWorld->getLastUpdateLengthFloat();

         float pct = mTimer / mpOwner->getPlayer()->getRepairTime();
         if (pct > 1.0f)
            pct = 1.0f;

         // Restore hit points for existing units.
         uint numUnits = mUnits.getSize();
         if (numUnits > 0 && mRepairPercent > 0.0f)
         {
            float repairPct = pct / mRepairPercent;
            if (repairPct > 1.0f)
               repairPct = 1.0f;
            for (uint j=0; j<numUnits; j++)
            {
//-- FIXING PREFIX BUG ID 2126
               const BSquadActionRepairUnit& unit=mUnits[j];
//--
               BUnit* pUnit = gWorld->getUnit(unit.mUnitID);
               if (pUnit)
               {
                  float baseHitpoints = pUnit->getProtoObject()->getHitpoints();
                  if (pUnit->getHitpoints() < baseHitpoints)
                  {
                     float startHitpoints = unit.mHitpoints;
                     float damagedHitpoints = baseHitpoints - startHitpoints;
                     float repairHitpoints = damagedHitpoints * repairPct;
                     float newHitpoints = startHitpoints + repairHitpoints;
                     if (newHitpoints > baseHitpoints)
                        newHitpoints = baseHitpoints;
                     pUnit->setHitpoints(newHitpoints);
                  }
               }
            }
         }

         // Reinforce dead units.
         if (mReinforcements.getNumber() > 0 && pct >= mRepairPercent && mRepairPercent < 1.0f)
         {
            float reinforcePct = pct / (1.0f - mRepairPercent);
            if (reinforcePct > 1.0f)
               reinforcePct = 1.0f;
            int restoreCount = (int)(pct * mReinforcements.getNumber());
            if (restoreCount > mReinforceIndex)
            {
               for (int j=mReinforceIndex; j<restoreCount; j++)
               {
                  BObjectCreateParms unitParms;
                  unitParms.mType = BEntity::cClassTypeUnit;
                  unitParms.mPlayerID = pSquad->getPlayerID();
                  unitParms.mProtoObjectID = mReinforcements[j];
                  unitParms.mStartBuilt = true;
                  unitParms.mPosition=pSquad->getPosition();
                  unitParms.mForward=pSquad->getForward();
                  unitParms.mRight=pSquad->getRight();
                  unitParms.mLevel=pSquad->getLevel();
                  BUnit* pUnit = gWorld->createUnit(unitParms);
                  if (pUnit)
                     pSquad->addChild(pUnit->getID());
               }
               pSquad->settle();
               if (gUserManager.getPrimaryUser()->getSelectionManager()->isSquadSelected(pSquad->getID()))
                  gUserManager.getPrimaryUser()->getSelectionManager()->selectSquad(pSquad->getID());
               if (gGame.isSplitScreen() && gUserManager.getSecondaryUser()->getSelectionManager()->isSquadSelected(pSquad->getID()))
                  gUserManager.getSecondaryUser()->getSelectionManager()->selectSquad(pSquad->getID());
               mReinforceIndex = restoreCount;
            }
         }

         if (pct == 1.0f)
            setState(cStateDone);
         break;
      }
   }

   if(!BAction::update(elapsed))
      return false;

   return true;
}

//==============================================================================
// Give opp to our units.
//==============================================================================
bool BSquadActionRepair::addUnitOpp()
{
//-- FIXING PREFIX BUG ID 2128
   //const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--
   BSquad *pSquad = gWorld->getSquad(mTargetSquad);

   BUnitOpp opp;
   opp.init();
   opp.setType(BUnitOpp::cTypeRepair);
   opp.setSource(pSquad->getID());
   opp.setUserData(cAnimTypeRepair);
   opp.generateID();
   if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityTrigger))
      opp.setTrigger(true);
   else if (mpOrder && (mpOrder->getPriority() == BSimOrder::cPriorityUser))
      opp.setPriority(BUnitOpp::cPriorityCommand);
   else
      opp.setPriority(BUnitOpp::cPrioritySquad);

   if (pSquad->addOppToChildren(opp))
   {      
      mUnitOppID = opp.getID();      
      return (true);
   }
   
   return (false);
}

//==============================================================================
// Remove opp that we've given our units.
//==============================================================================
void BSquadActionRepair::removeUnitOpp()
{
//-- FIXING PREFIX BUG ID 2129
   const BSquad* pSquad = reinterpret_cast<BSquad*>(mpOwner);
//--

   if (mUnitOppID == BUnitOpp::cInvalidID)
      return;

   pSquad->removeOppFromChildren(mUnitOppID);
   mUnitOppID = BUnitOpp::cInvalidID;
}

//==============================================================================
//==============================================================================
float BSquadActionRepair::getRepairPercent() const
{
   float repairTime=mpOwner->getPlayer()->getRepairTime();
   if (repairTime==0.0f)
      return 1.0f;
   else
      return mTimer/repairTime;
}

//==============================================================================
//==============================================================================
bool BSquadActionRepair::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;

   GFWRITEARRAY(pStream, BSquadActionRepairUnit, mUnits, uint8, 200);
   GFWRITEARRAY(pStream, int, mReinforcements, uint8, 100);
   GFWRITEVAR(pStream, float, mTimer);
   GFWRITEVAR(pStream, BUnitOppID, mUnitOppID);
   GFWRITEVAR(pStream, DWORD, mDamagedTime);
   GFWRITEVAR(pStream, float, mRepairPercent);
   GFWRITEVAR(pStream, int, mReinforceIndex);
   GFWRITEVAR(pStream, bool, mSaveFlagNonMobile);
   GFWRITEVAR(pStream, BEntityID, mTargetSquad);

   return true;
}

//==============================================================================
//==============================================================================
bool BSquadActionRepair::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;

   GFREADARRAY(pStream, BSquadActionRepairUnit, mUnits, uint8, 200);
   GFREADARRAY(pStream, int, mReinforcements, uint8, 100);
   GFREADVAR(pStream, float, mTimer);
   GFREADVAR(pStream, BUnitOppID, mUnitOppID);
   GFREADVAR(pStream, DWORD, mDamagedTime);
   GFREADVAR(pStream, float, mRepairPercent);
   GFREADVAR(pStream, int, mReinforceIndex);
   GFREADVAR(pStream, bool, mSaveFlagNonMobile);
   GFREADVAR(pStream, BEntityID, mTargetSquad);

   return true;
}

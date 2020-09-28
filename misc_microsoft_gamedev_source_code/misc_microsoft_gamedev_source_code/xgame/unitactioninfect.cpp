//==============================================================================
// unitactioninfect.cpp
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================

#include "common.h"
#include "civ.h"
#include "config.h"
#include "configsgame.h"
#include "entity.h"
#include "protoobject.h"
#include "protosquad.h"
#include "squad.h"
#include "unitactioninfect.h"
#include "tactic.h"
#include "unit.h"
#include "world.h"
#include "unitquery.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BUnitActionInfect, 4, &gSimHeap);

//==============================================================================
//==============================================================================
bool BUnitActionInfect::init(void)
{
   if (!BAction::init())
      return(false);

   mInfectedCount = 0;
   mTimeUntilNextScan = 0.0f;

   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionInfect::connect(BEntity* pOwner, BSimOrder* pOrder)
{
   if (!BAction::connect(pOwner, pOrder))
      return(false);

   setState(cStateWait);
   return(true);
}

//==============================================================================
//==============================================================================
void BUnitActionInfect::disconnect()
{
   // Remove any particle effects that are leftover
   for (uint ofs = 0; ofs < mInfectees.size(); ++ofs)
   {
      for (uint unitOfs = 0; unitOfs < mInfectees[ofs].units.size(); ++unitOfs)
      {
         BUnit* pTempUnit = gWorld->getUnit(mInfectees[ofs].units[unitOfs].first);
         if (pTempUnit)
            pTempUnit->removeAttachment(mInfectees[ofs].units[unitOfs].second);
      }
   }
   BAction::disconnect();
}


//==============================================================================
//==============================================================================
bool BUnitActionInfect::setState(BActionState state)
{
   return BAction::setState(state);
}

//==============================================================================
//==============================================================================
bool BUnitActionInfect::update(float elapsed)
{
   BASSERT(mpOwner);
   BASSERT(mpOwner->isClassType(BEntity::cClassTypeUnit) == true);
   if (mpProtoAction->getFlagDisabled())
      return (true);

   BUnit* pUnit = mpOwner->getUnit();
   BASSERT(pUnit);
   if (!pUnit)
      return false;
   BSquad* pSquad = pUnit->getParentSquad();
   BASSERT(pSquad);
   if (!pSquad)
      return false;

   // Update any squads in the infectee list, and infect units in it if they've been in the cloud long enough
   for (uint ofs = 0; ofs < mInfectees.size(); ++ofs)
   {
      mInfectees[ofs].time += elapsed;

      if (mInfectees[ofs].time * 1000 >= mpProtoAction->getMinIdleDuration())
      {
         mInfectees[ofs].combatValue += elapsed * mpProtoAction->getWorkRate();

         // See if there is a unit in the squad that is less than or equal to the amount of combat value we've stored
         BSquad *pTargetSquad = gWorld->getSquad(mInfectees[ofs].squadId);

         if (pTargetSquad)
         {
            const BEntityIDArray& children = pTargetSquad->getChildList();

            for (uint squadOfs = 0; squadOfs < children.size(); ++squadOfs)
            {
               BUnit *pTempUnit = pTempUnit = gWorld->getUnit(children[squadOfs]);

               if (!pTempUnit || !pTempUnit->getProtoObject() || pTempUnit->getFlagFatalityVictim() || pTempUnit->getFlagDoingFatality())
                  continue;

               if (pTempUnit->getProtoObject()->getCombatValue() > mInfectees[ofs].combatValue)
                  continue;

               for (uint unitOfs = 0; unitOfs < mInfectees[ofs].units.size(); ++unitOfs)
               {
                  if (mInfectees[ofs].units[unitOfs].first == pTempUnit->getID())
                  {
                     pTempUnit->removeAttachment(mInfectees[ofs].units[unitOfs].second);
                     mInfectees[ofs].units.eraseUnordered(unitOfs);
                     break;
                  }
               }

               #ifdef SYNC_UnitAction
                  syncUnitActionData("BUnitActionInfect::update kill", pTempUnit->getID());
               #endif

               pTempUnit->infect(pUnit->getPlayerID());
               pTempUnit->setHitpoints(0.0f);

               ++mInfectedCount;

               mInfectees[ofs].time = -1.0f;
               mInfectees[ofs].combatValue -= pTempUnit->getProtoObject()->getCombatValue();

               // If we've hit our infection limit, kill off the squad
               if (pUnit->getProtoObject() && 
                  pUnit->getProtoObject()->getNumConversions() > 0 &&
                  mInfectedCount >= pUnit->getProtoObject()->getNumConversions())
               {
                  pSquad->kill(true);
               }
            }
         }
      }
   }

   for (uint ofs = 0; ofs < mInfectees.size(); ++ofs)
   {
      if ((mInfectees[ofs].time + 1.0f) < cFloatCompareEpsilon)
      {
         for (uint unitOfs = 0; unitOfs < mInfectees[ofs].units.size(); ++unitOfs)
         {
            BUnit* pTempUnit = gWorld->getUnit(mInfectees[ofs].units[unitOfs].first);
            if (pTempUnit)
               pTempUnit->removeAttachment(mInfectees[ofs].units[unitOfs].second);
         }

         mInfectees.eraseUnordered(ofs);
         if (ofs > 0)
            --ofs;
      }
   }

   if (mInfectees.size() > 0 && mInfectees[0].time == -1.0f)
   {
      for (uint unitOfs = 0; unitOfs < mInfectees[0].units.size(); ++unitOfs)
      {
         BUnit* pTempUnit = gWorld->getUnit(mInfectees[0].units[unitOfs].first);
         if (pTempUnit)
            pTempUnit->removeAttachment(mInfectees[0].units[unitOfs].second);
      }

      mInfectees.eraseUnordered(0);
   }

   // see if enough time has passed to warrant another rescan 
   mTimeUntilNextScan -= elapsed;
   if (mTimeUntilNextScan <= 0.0f)
   {
      // whee! hardcoded time for this at the moment
      mTimeUntilNextScan = 0.5f;

      // Add any new units to the infectee list
      BEntityIDArray results(0, 100);
      BUnitQuery query(pUnit->getPosition(), mpProtoAction->getWorkRange(), true);
      gWorld->getSquadsInArea(&query, &results, false);
      const BProtoObjectID attachmentId = mpProtoAction->getProtoObject();

      for (uint i = 0; i < results.getSize(); i++)
      {
         BSquad* pTempSquad = gWorld->getSquad(results[i]);
         if (!pTempSquad || pTempSquad == pSquad)
         {
            continue;
         }

         if (pTempSquad->getPlayerID() == pUnit->getPlayerID() ||
            pTempSquad->getPlayerID() == cGaiaPlayer)
         {
            continue;
         }

         const BEntityIDArray& children = pTempSquad->getChildList();
         uint numChildren = children.getSize();
         bool canInfectSquad = false;
         for (uint j = 0; j < numChildren; ++j)
         {
            BUnit*  pTempUnit = gWorld->getUnit(children[j]);

            if (!pTempUnit)
            {
               continue;
            }

            if (pTempUnit->getFlagTakeInfectionForm())
            {
               continue;
            }

            if (pTempUnit->getFlagInfected())
            {
               continue;
            }

            if (!pTempUnit->isInfectable())
            {
               continue;
            }

            // Make sure this isn't a disallowed type
            if (mpProtoAction->getInvalidTargets().size() > 0 &&
               mpProtoAction->getInvalidTargets().find(pTempUnit->getProtoID()) >= 0)
            {
               continue;
            }

            canInfectSquad = true;
            break;
         }

         if (canInfectSquad)
         {
            BInfectee target;
            target.squadId = pTempSquad->getID();
            target.time = 0.0f;
            target.combatValue = 0.0f;

            if (mInfectees.find(target) == -1)
            {
               for (uint j = 0; j < numChildren; ++j)
               {
                  BUnit* pUnit = gWorld->getUnit(children[j]);
                  if (pUnit)
                  {
                     BInfectee::UnitFxPair unitFx;

                     unitFx.first = pUnit->getID();
                     unitFx.second = pUnit->addAttachment(attachmentId);

                     target.units.add(unitFx);
                  }
               }

               mInfectees.add(target);
            }
         }
      }
   }

   if(!BAction::update(elapsed))
      return (false);
   return (true);
}

//==============================================================================
//==============================================================================
bool BUnitActionInfect::save(BStream* pStream, int saveType) const
{
   if (!BAction::save(pStream, saveType))
      return false;
   GFWRITEVAR(pStream, int, mInfectedCount);
   GFWRITECLASSARRAY(pStream, saveType, mInfectees, uint8, 200);
   GFWRITEVAR(pStream, float, mTimeUntilNextScan);
   return true;
}

//==============================================================================
//==============================================================================
bool BUnitActionInfect::load(BStream* pStream, int saveType)
{
   if (!BAction::load(pStream, saveType))
      return false;
   GFREADVAR(pStream, int, mInfectedCount);
   GFREADCLASSARRAY(pStream, saveType, mInfectees, uint8, 200);
   if (BAction::mGameFileVersion >= 41)
   {
      GFREADVAR(pStream, float, mTimeUntilNextScan);
   }
   return true;
}

//==============================================================================
//==============================================================================
bool BInfectee::save(BStream* pStream, int saveType) const
{
   GFWRITEARRAY(pStream, UnitFxPair, units, uint8, 100);
   GFWRITEVAR(pStream, BEntityID, squadId);
   GFWRITEVAR(pStream, float, time);
   GFWRITEVAR(pStream, float, combatValue);
   return true;
}

//==============================================================================
//==============================================================================
bool BInfectee::load(BStream* pStream, int saveType)
{
   GFREADARRAY(pStream, UnitFxPair, units, uint8, 100);
   GFREADVAR(pStream, BEntityID, squadId);
   GFREADVAR(pStream, float, time);
   GFREADVAR(pStream, float, combatValue);
   return true;
}

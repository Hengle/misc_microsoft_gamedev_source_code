//==============================================================================
// army.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

#include "common.h"
#include "Army.h"
#include "ConfigsGame.h"
#include "EntityGrouper.h"
#include "ObjectManager.h"
#include "Platoon.h"
#include "World.h"
#include "protoobject.h"

//#define DEBUGMANAGEMENT
#define PLATOON_MERGING

#ifndef BUILD_FINAL
BEntityID sDebugArmyTempID;

//#define DEBUG_MOVE4
#endif

#ifdef DEBUG_MOVE4
#define debugMove4 sDebugArmyTempID=mID, dbgArmyInternalTempID
#else
#define debugMove4 __noop
#endif

#ifndef BUILD_FINAL
//==============================================================================
//==============================================================================
void dbgArmyInternal(BEntityID armyID, const char *format, ...)
{
   // Assemble caller's string.
   char buf[256];        
   va_list arglist;
   va_start(arglist, format);
   StringCchVPrintf(buf, _countof(buf), format, arglist);
   va_end(arglist);

   // Output.
   gConsole.output(cChannelSim, "ARMY %d (%d): %s", armyID.asLong(), armyID.getIndex(), buf);
   //syncSquadData("debugMove4 --", buf);
}


//==============================================================================
//==============================================================================
void dbgArmyInternalTempID(const char *format, ...)
{
   // Assemble caller's string.
   char buf[256];        
   va_list arglist;
   va_start(arglist, format);
   StringCchVPrintf(buf, _countof(buf), format, arglist);
   va_end(arglist);

   // Call with preset ID.
   dbgArmyInternal(sDebugArmyTempID, buf);
}
#endif


GFIMPLEMENTVERSION(BArmy,1);
enum
{
   cSaveMarkerArmy1=10000,
};


//==============================================================================
//==============================================================================
void BArmy::onRelease()
{
   mChildren.setNumber(0);
   BEntity::onRelease();
}

//==============================================================================
//==============================================================================
void BArmy::init()
{
    BEntity::init();
    mChildren.clear();
}

//==============================================================================
//==============================================================================
bool BArmy::updatePreAsync(float elapsedTime)
{
   if (mChildren.getNumber() == 0)
   {
#ifdef DEBUGMANAGEMENT
      gConsole.output(cChannelSim, "ArmyID=%d: No Children, going away.", getID().asLong());
#endif
      return(false);
   }
   // DLM - check my platoons.. see if I can merge any of them together.  If so,
   // then do so.  
   #ifdef PLATOON_MERGING
   long nNumChildren = mChildren.getNumber();
   if (nNumChildren <= 1)
      return true;

   float platoonRadius;
   gConfig.get(cConfigPlatoonRadius, &platoonRadius);
   // We want the decision to merge to be smaller than the general area we use to gather up
   // squads, so cut platoonRadius in half.
   platoonRadius *= 0.5f;
   float platoonDistChk = platoonRadius * platoonRadius;
   for (long n = 0; n < nNumChildren; n++)
   {
      BPlatoon *pSrcPlatoon = reinterpret_cast<BPlatoon *>(gWorld->getEntity(mChildren[n]));
      if (!pSrcPlatoon)
         continue;

      debugMove4("Analyzing Platoon ID %d to see if it can merge..", pSrcPlatoon->getID().asLong());
      // If this platoon can't merge, might as well move on..
      if (!pSrcPlatoon->canMerge())
      {
         debugMove4("  the platoon cannot merge.  Continueing..");
         continue;
      }

      // Only platoons not currently moving can absorb other platoons.
      if (!gConfig.isDefined(cConfigClassicPlatoonGrouping))
      {
         if (!pSrcPlatoon->isPlatoonDoneMoving_4())
         {
            debugMove4("  the platoon is still moving.  Continueing..");
            continue;
         }
      }
      long nSrcMovementType = pSrcPlatoon->getMovementType();

      for (long m = 0; m < nNumChildren; m++)
      {
         if (m == n)
            continue;

         BPlatoon *pComparePlatoon = reinterpret_cast<BPlatoon *>(gWorld->getEntity(mChildren[m]));
         if (!pComparePlatoon)
            continue;

         // if compare platoon can't merge either, move along..
         if (!pComparePlatoon->canMerge())
            continue;

         // if they're different movement types, don't merge.
         if (nSrcMovementType != pComparePlatoon->getMovementType())
            continue;

         // Before I even consider merging, I need to look at my current destination, and
         // see how close I am to it.  If I'm not very close, then don't merge. 
         // If classic grouping is defined, then allow merges to occur regardless of distance to
         // destination..
         if (!gConfig.isDefined(cConfigClassicPlatoonGrouping))
         {
            const BPath &userPath = pComparePlatoon->getCurrentUserPath_4();
            if (userPath.getNumberWaypoints() == 0)
               continue;
            BVector compareDestination = userPath.getWaypoint(userPath.getNumberWaypoints() - 1);
            float fDistToDestSqr = pComparePlatoon->getPosition().xzDistanceSqr(compareDestination);
            if (fDistToDestSqr > platoonDistChk)
               continue;
         }

         BVector dir = pComparePlatoon->getPosition() - pSrcPlatoon->getPosition();
         float dist = dir.lengthSquared();
         // Don't do the canMerge check down here anymore, it's been done up above.
         // DON'T remove the check up above dufus.
         if (dist < platoonDistChk /* && pComparePlatoon->canMerge() */)
         {
            pSrcPlatoon->mergePlatoon(pComparePlatoon);
            // Update the children.. 
            mChildren.removeValue(pComparePlatoon->getID(), true);
            // destroy the old platoon
            pComparePlatoon->BPlatoon::destroy();
            gWorld->releaseObject(pComparePlatoon);
            // DLM 9/25/09 - If we've done a merge.. the arrays are in disarray (hah) 
            // for continueing to loop anyway, so just exit after one merge,
            // and we'll check again next update.
            return true;
         }
      }
   }
   #endif

   return(true);
}

//==============================================================================
//==============================================================================
bool BArmy::updateAsync(float elapsedTime)
{
   return true;
}

//==============================================================================
//==============================================================================
bool BArmy::update(float elapsedTime)
{
   return true;
}

//==============================================================================
//==============================================================================
void BArmy::kill(bool bKillImmediately)
{
}

//==============================================================================
//==============================================================================
void BArmy::destroy()
{
}

//==============================================================================
//==============================================================================
long BArmy::addChild(BPlatoon *platoon)
{
   #ifdef DEBUGMANAGEMENT
   gConsole.output(cChannelSim, "ArmyID=%d: Adding Child: PlatoonID=%d.", getID().asLong(), platoon->getID().asLong());
   #endif

   //Remove the platoon from our old army.
   BArmy *oldArmy=gWorld->getArmy(platoon->getParentID());
   if (oldArmy)
      oldArmy->removeChild(platoon);

   //Set us as the parent to the platoon and add it to our child list.
   platoon->setParentID(mID);
   return(mChildren.add(platoon->getID()));
}

//==============================================================================
//==============================================================================
bool BArmy::removeChild(BPlatoon* platoon)
{
   #ifdef DEBUGMANAGEMENT
   gConsole.output(cChannelSim, "  ArmyID=%d: REMOVING Child: PlatoonID=%d.", getID().asLong(), platoon->getID().asLong());
   #endif

   long index=mChildren.find(platoon->getID());
   if (index == -1)
      return(false);

   mChildren.removeIndex(index);
   return(true);
}

//==============================================================================
//==============================================================================
BEntityID BArmy::getChild(uint index) const
{

   if (index >= (unsigned int) mChildren.getNumber())
      return cInvalidObjectID;

   return mChildren[index];
}

//==============================================================================
//==============================================================================
bool BArmy::addSquads(const BEntityIDArray& squads, bool groupBySpeed /*= true*/)
{
   BASSERT(mChildren.getSize() == 0);


   //Group up the squads.
   BEntityGrouper *egr=gWorld->getEntityGrouper();
   egr->reset();
   //Set the radius.
   float platoonRadius;
   gConfig.get(cConfigPlatoonRadius, &platoonRadius);
   egr->setRadius(platoonRadius);
   // DLM 6/30/08 -- added grouping by speed
   egr->groupBySpeed(groupBySpeed);
   egr->groupEntities(squads);

   //Create a platoon for each group of squads.
   BObjectCreateParms objectParms;
   objectParms.mPlayerID=mPlayerID;
   for (uint i=0; i < egr->getNumberGroups(); i++)
   {
      const BEntityGroup *entityGroup=egr->getGroupByIndex(i);
      if (!entityGroup)
         continue;
   
      BPlatoon *platoon=gWorld->createPlatoon(objectParms);
      if (!platoon)
         return(false);
      addChild(platoon);
      
      const BEntityIDArray& tempSquads = entityGroup->getEntities();
      for (uint j = 0; j < tempSquads.getSize(); j++)
      {
         BSquad* pSquad = gWorld->getSquad(tempSquads[j]);
         if (pSquad)
         {
            BASSERTM(pSquad->getPlayerID() == getPlayerID(), "Trying to add a squad to an army owned by a different player!");
            // Do not add squads that are buildings
//-- FIXING PREFIX BUG ID 1718
            const BUnit* pLeaderUnit = pSquad->getLeaderUnit();
//--
            if (pLeaderUnit)
            {
               const BProtoObject* pProtoObject = pLeaderUnit->getProtoObject();
               if (pProtoObject && !pProtoObject->isType(gDatabase.getOTIDBuilding()))
               {
                  platoon->addChild(pSquad);                  
               }
            }
         }
      }
   }

   return(true);
}

//==============================================================================
//==============================================================================
bool BArmy::rePlatoon()
{
   BEntityIDArray armySquads;
   uint numPlatoons = (uint)mChildren.getNumber();
   for (uint i = 0; i < numPlatoons; i++)
   {
//-- FIXING PREFIX BUG ID 1720
      const BPlatoon* pPlatoon = gWorld->getPlatoon(mChildren[i]);
//--
      if (pPlatoon)
      {
         uint numSquads = pPlatoon->getNumberChildren();
         for (uint j = 0; j < numSquads; j++)
         {
            BEntityID squadID = pPlatoon->getChild(j);
//-- FIXING PREFIX BUG ID 1719
            const BSquad* pSquad = gWorld->getSquad(squadID);
//--
            if (pSquad)
            {
               armySquads.add(squadID);            
            }
         }         
      }
   }

   mChildren.clear();
   if (gConfig.isDefined(cConfigClassicPlatoonGrouping))
      return (addSquads(armySquads, false));

   return (addSquads(armySquads));
}

//==============================================================================
// DLM 3/11/08 - Basically is a "super" version of rePlatoon, this performs a "pre" check to
// see if the platoon's we'd get back would be the same, and if so, then doesn't
// replatoon.  This will allow command metering to work a little more efficiently,
// as platoon's can stay around long enough to actually keep existing commands
// Returns true if a replatoon was performed, false if not. 
//==============================================================================
bool BArmy::checkAndReplatoon()
{
   // So similar to rePlatoon above, create an EntityIDArray of all the squads 
   // currently in the army..
   BEntityIDArray armySquads;
   uint numPlatoons = (uint)mChildren.getNumber();
   for (uint i = 0; i < numPlatoons; i++)
   {
//-- FIXING PREFIX BUG ID 1722
      const BPlatoon* pPlatoon = gWorld->getPlatoon(mChildren[i]);
//--
      if (pPlatoon)
      {
         uint numSquads = pPlatoon->getNumberChildren();
         for (uint j = 0; j < numSquads; j++)
         {
            BEntityID squadID = pPlatoon->getChild(j);
//-- FIXING PREFIX BUG ID 1721
            const BSquad* pSquad = gWorld->getSquad(squadID);
//--
            if (pSquad)
            {
               armySquads.add(squadID);            
            }
         }         
      }
   }
   // Now use the unitgrouper to see what platoons we would create..
   BEntityGrouper *egr=gWorld->getEntityGrouper();
   egr->reset();
   //Set the radius.
   float platoonRadius;
   gConfig.get(cConfigPlatoonRadius, &platoonRadius);
   egr->setRadius(platoonRadius);
   // DLM 6/30/08 -- added grouping by speed
   // DLM 9/24/8 - added a config to turn off group by speed.. 
   if (gConfig.isDefined(cConfigClassicPlatoonGrouping))
      egr->groupBySpeed(false);
   else
      egr->groupBySpeed(true);
   egr->groupEntities(armySquads);

   // First, compare counts.  If they're different, then replatoon is necessary. 
   bool bReplatoon = false;
   if (egr->getNumberGroups() != mChildren.getSize())
      bReplatoon = true;
   if (!bReplatoon)
   {
      // For each group in the entity group, see if we have a platoon to match.. 
      for (uint i=0; i < egr->getNumberGroups(); i++)
      {
         const BEntityGroup *entityGroup=egr->getGroupByIndex(i);
         if (!entityGroup)
         {
            bReplatoon = true;
            break;
         }
         const BEntityIDArray &entityList = entityGroup->getEntities();

         // Walk through platoons, and see if we have one that matches this entity group.
         bool bPlatoonFound = false;
         for (uint p = 0; p < numPlatoons; p++)
         {
//-- FIXING PREFIX BUG ID 1723
            const BPlatoon* pPlatoon = gWorld->getPlatoon(mChildren[p]);
//--
            if (pPlatoon)
            {
               uint numSquads = pPlatoon->getNumberChildren();
               // if the counts are different, this is definitely not the right platoon.. check the next one
               if (numSquads != entityList.getSize())
                  continue;
               // Now for each entityID in the list, see if we have a squad that matches.
               bool bAllSquadsFound = true;
               for (uint n = 0; n < entityList.getSize(); n++)
               {
                  bool bSquadFound = false;
                  for (uint s = 0; s < numSquads; s++)
                  {
                     BEntityID squadID = pPlatoon->getChild(s);
                     if (squadID == entityList[n])
                     {
                        bSquadFound = true;
                        break;
                     }
                  }
                  // If we didn't find a matching squad, then this is the wrong platoon.
                  if (!bSquadFound)
                  {
                     bAllSquadsFound = false;
                     break;
                  }
               }
               // If we found all the squads, then this is a platoon match.
               if (bAllSquadsFound)
               {
                  bPlatoonFound = true;
                  break;
               }
            }
         }
         // If we went through the platoon list and didn't find a match, then we're done.
         if (!bPlatoonFound)
         {
            bReplatoon = true;
            break;
         }
      } // end of for i = 0 through entity group count
   } // end of if !bReplatoon already

   if (bReplatoon)
   {
      mChildren.clear();
      if (gConfig.isDefined(cConfigClassicPlatoonGrouping))
         return addSquads(armySquads, false);

      return (addSquads(armySquads));
   }

   return bReplatoon;
}

//==============================================================================
//==============================================================================
bool BArmy::queueOrder(const BCommand* pCommand)
{
   //TEMP: Give this to all our platoons.
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      BPlatoon *platoon=gWorld->getPlatoon(mChildren[i]);
      if (platoon)
         platoon->queueOrder(pCommand);
   }

   return(true);
}

//==============================================================================
//==============================================================================
void BArmy::propagateAttackMove()
{
   //Give this to all our platoons.
   for (uint i=0; i < mChildren.getSize(); i++)
   {
      BPlatoon *platoon=gWorld->getPlatoon(mChildren[i]);
      if (platoon)
         platoon->propagateAttackMove();
   }
}

//==============================================================================
// Check to see if the whole army is outside the playable bounds
//==============================================================================
bool BArmy::isOutsidePlayableBounds(bool forceCheckWorldBoundaries) const
{
   uint numChildren = getNumberChildren();
   for (uint i = 0; i < numChildren; i++)
   {
//-- FIXING PREFIX BUG ID 1724
      const BPlatoon* pPlatoon = gWorld->getPlatoon(getChild(i));
//--
      if (pPlatoon && pPlatoon->isOutsidePlayableBounds(forceCheckWorldBoundaries))
      {
         return (true);
      }
   }

   return (BEntity::isOutsidePlayableBounds(forceCheckWorldBoundaries));
}

//==============================================================================
//==============================================================================
bool BArmy::save(BStream* pStream, int saveType) const
{
   if (!BEntity::save(pStream, saveType))
      return false;

   GFWRITEARRAY(pStream, BEntityID, mChildren, uint16, 2000);

   GFWRITEMARKER(pStream, cSaveMarkerArmy1);
   return true;
}

//==============================================================================
//==============================================================================
bool BArmy::load(BStream* pStream, int saveType)
{
   if (!BEntity::load(pStream, saveType))
      return false;

   GFREADARRAY(pStream, BEntityID, mChildren, uint16, 2000);

   GFREADMARKER(pStream, cSaveMarkerArmy1);
   return true;
}

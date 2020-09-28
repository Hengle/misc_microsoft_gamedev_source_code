//==============================================================================
// EntityGrouper.cpp
// Copyright (c) 1999-2007 Ensemble Studios
//==============================================================================

//Includes
#include "common.h"
#include "EntityGrouper.h"
#include "Army.h"
#include "Entity.h"
#include "Platoon.h"
#include "Squad.h"
#include "TerrainSimRep.h"
#include "World.h"
#include "simhelper.h"
#include "configsgame.h"

//==============================================================================
//==============================================================================
IMPLEMENT_FREELIST(BEntityGroup, 5, &gSimHeap);

//==============================================================================
//==============================================================================
bool BEntityGroup::addEntity(BEntityID entityID)
{
   //TODO: Impose some size limit if we have one.

   //See if we already have this in here.  Skip if we do.
   if (mEntities.contains(entityID))
      return(false);

   //Add it.
   mEntities.add(entityID);
   return(true);
}

//==============================================================================
//==============================================================================
bool BEntityGroup::addEntities(const BEntityIDArray &entityIDs)
{
   bool rVal=true;
   for (uint i=0; i < entityIDs.getSize(); i++)
   {
      if (!addEntity(entityIDs[i]))
         rVal=false;
   }
   return(rVal);
}

//==============================================================================
//==============================================================================
void BEntityGroup::render(void)
{
   //Slow and dumb, but it's fine:)
   if (mEntities.getSize() <= 0)
      return;

   BVector centroid(0.0f);
   uint count=0;
   for (uint i=0; i < mEntities.getSize(); i++)
   {
      BEntity *entity=gWorld->getEntity(mEntities[i]);
      if (entity)
      {
         centroid+=entity->getPosition();
         count++;
      }
   }
   centroid/=(float)count;
   //Centroid.
   gTerrainSimRep.addDebugThickCircleOverTerrain(centroid, 0.5f, 0.1f, cDWORDWhite, 0.75f);

   //Lines to each entity.
   for (uint i=0; i < mEntities.getSize(); i++)
   {
      BEntity *entity=gWorld->getEntity(mEntities[i]);
      if (entity)
         gTerrainSimRep.addDebugLineOverTerrain(centroid, entity->getPosition(), cDWORDWhite, cDWORDGreen, 0.75f);
   }
}



//==============================================================================
//==============================================================================
BEntityGrouper::BEntityGrouper() :
   //mGroups doesn't need any ctor args.
   mReset(true),
   mModifiedCommand(false),
   mSpeed(false),
   mProtoSquad(false),
   mFlagCommonParentShortcut(true)
   //mEntityIDs doesn't need any ctor args.
   //mCurrentBest doesn't need any ctor args.
   //mOverallBest doesn't need any ctor args.
{
   mTargetID.invalidate();
}

//==============================================================================
//==============================================================================
BEntityGrouper::~BEntityGrouper(void)
{
   cleanUp();
}

//==============================================================================
//==============================================================================
void BEntityGrouper::reset(void)
{
   //Rip through the groups that we have and release them.
   for (uint i=0; i < mGroups.getSize(); i++)
   {
      BEntityGroup* eg=mGroups[i];
      mGroups[i]=NULL;
      BEntityGroup::releaseInstance(eg);
   }
   mGroups.clear();

   mEntityIDs.clear();
   mCurrentBest.clear();
   mOverallBest.clear();

   //Reset our filters.
   mRadiusSqr=1.0f;
   mReset=true;
   mTargetID.invalidate();
   mModifiedCommand=false;
   mSpeed = false;
   mProtoSquad = false;
   mFlagCommonParentShortcut = true;
}

//==============================================================================
//==============================================================================
void BEntityGrouper::groupEntities(const BEntityIDArray &entityIDs)
{
   //If we haven't been reset, fail.
   if (!mReset)
      return;
   mReset=false;

   //Make a copy of the entities; we'll assume they're the same type for now.
   mEntityIDs.clear();
   for (uint i=0; i < entityIDs.getSize(); i++)
      mEntityIDs.add(entityIDs[i]);

   // Do the actual group.
   // mrh - Added separate method for the AI's grouping by proto squad to keep things cleaner and fix bugs.
   if (mProtoSquad)
      actualGroupSquadsByProtoSquad();
   else
      actualGroupEntities();
}

//==============================================================================
// BEntityGrouper::getGroupByIndex
//==============================================================================
const BEntityGroup* BEntityGrouper::getGroupByIndex(uint index) const
{
   if (index >= mGroups.getSize())
      return(NULL);
   return(mGroups[index]);
}

//=============================================================================
// BEntityGrouper::commonParentID
//=============================================================================
BEntityID BEntityGrouper::commonParentID(const BEntityIDArray &entityIDs)
{
   //This function looks at the entities and returns the ID of the parent that
   //they all belong to (if one exists).  All given entities must belong to the same
   //parent for this to return a valid entityID and the parent's number of children
   //must equal the number of entities used here.

   BEntityID commonID;
   for (uint a=0; a < entityIDs.getSize(); a++)
   {
      BEntity *entity=gWorld->getEntity(entityIDs[a]);
      if (!entity)
         continue;
      //If this doesn't have a parent, we have a mismatch.
      if (!entity->getParentID().isValid())
         return(cInvalidObjectID);
      //If we have the first entity, set the commonID.
      if (!commonID.isValid())
         commonID=entity->getParentID();
      //Else, if we have a mismatch, bail.
      else if (commonID != entity->getParentID())
         return(cInvalidObjectID);
   }
   
   //Make sure that this parent's number of children matches the list we have.
   uint parentChildCount=0;
   switch (commonID.getType())
   {
      case BEntity::cClassTypeUnit:
      {
         BSquad *squad=gWorld->getSquad(commonID);
         if (squad)
            parentChildCount=squad->getNumberChildren();
         break;
      }
      case BEntity::cClassTypeSquad:
      {
         BPlatoon *platoon=gWorld->getPlatoon(commonID);
         if (platoon)
            parentChildCount=platoon->getNumberChildren();
         break;
      }
      case BEntity::cClassTypeArmy:
      {
         BArmy *army=gWorld->getArmy(commonID);
         if (army)
            parentChildCount=army->getNumberChildren();
         break;
      }
   }
   if (parentChildCount != entityIDs.getSize())
      return(cInvalidObjectID);

   //If we're here, return the common ID.
   return(commonID);
}

//=============================================================================
// BEntityGrouper::commonArmyIDForSquads
//=============================================================================
BEntityID BEntityGrouper::commonArmyIDForSquads(const BEntityIDArray &entityIDs, bool failOnInvalidSquads)
{
   //This function assumes the entityIDs passed in are squads and returns the
   //common ArmyID they all share if every squad is in the same army and every squad
   //in that army is in this list.  If there is squad w/o an army (even if they
   //all have no army), this fails.

   //Come up with the list of platoons represented by this list of squads.  Make
   //sure that each platoon is fully represented.
   BEntityID armyID;
   armyID.invalidate();
   BEntityIDArray platoonIDs;
   for (uint i=0; i < entityIDs.getSize(); i++)
   {
      BSquad* pSquad=gWorld->getSquad(entityIDs[i]);
      if (!pSquad)
      {
         //If the squad is gone, we can either fail or continue based on what the user wants.
         if (failOnInvalidSquads)
            return(cInvalidObjectID);
         continue;
      }
//-- FIXING PREFIX BUG ID 2431
      const BPlatoon* pPlatoon=pSquad->getParentPlatoon();
//--
      if (!pPlatoon)
         return(cInvalidObjectID);

      //Skip platoons that have already been added.
      if (platoonIDs.contains(pPlatoon->getID()))
         continue;
      
      //Before we add this, make sure each squad in the platoon is in the entityID list.
      //If there is one that is not, fail.
      for (uint j=0; j < pPlatoon->getNumberChildren(); j++)
      {
         if (!entityIDs.contains(pPlatoon->getChild(j)))
            return(cInvalidObjectID);
      }
      
      //If we have a valid armyID already, this new platoon must match that.
      if (armyID.isValid() && (pPlatoon->getParentID() != armyID))
         return(cInvalidObjectID);
      else if (!armyID.isValid())
         armyID=pPlatoon->getParentID();
      
      //Now we know this platoon is fully represented in the recipient list, so add it.
      platoonIDs.add(pPlatoon->getID());
   }
   
   //If we're here, we have a list of platoon IDs that is fully represented in
   //the entityID list.  We know, also, that each of the platoons is in the same
   //army.  The last check we need to do is to make sure that all of the army's
   //platoons are here.
//-- FIXING PREFIX BUG ID 2432
   const BArmy* pArmy=gWorld->getArmy(armyID);
//--
   if (!pArmy)
      return(cInvalidObjectID);
   if (pArmy->getNumberChildren() != platoonIDs.getSize())
      return(cInvalidObjectID);
   return(pArmy->getID());
}

//=============================================================================
//=============================================================================
BArmy* BEntityGrouper::getOrCreateCommonArmyForSquads(const BEntityIDArray &squadIDs, BPlayerID playerID)
{
   BArmy* pResultArmy = NULL;

   // See if there is already a common army ID
   BEntityID commonArmyID = BEntityGrouper::commonArmyIDForSquads(squadIDs);
   if (commonArmyID != cInvalidObjectID)
   {
      BArmy* pArmy = gWorld->getArmy(commonArmyID);
      if (pArmy)
      {
         pResultArmy = pArmy;
      }
   }

   // If no common army
   if (!pResultArmy)
   {
      // Group squads per player
      BPlayerSpecificEntityIDsArray playerEntityIDs = BSimHelper::groupEntityIDsByPlayer(squadIDs);
      uint numPlayerEntityIDs = playerEntityIDs.getSize();
      for (uint i = 0; i < numPlayerEntityIDs; i++)
      {
         uint numEntities = playerEntityIDs[i].mEntityIDs.getSize();
         if (numEntities > 0)
         {
            BObjectCreateParms objectParms;
            objectParms.mPlayerID = playerEntityIDs[i].mPlayerID;
            BArmy* pArmy = gWorld->createArmy(objectParms);
            if (pArmy)
            {
               // Platoon the squads
               if (gConfig.isDefined(cConfigClassicPlatoonGrouping))
                  pArmy->addSquads(playerEntityIDs[i].mEntityIDs, false);
               else
                  pArmy->addSquads(playerEntityIDs[i].mEntityIDs);

               // Is this the player we care about?
               if (playerID == playerEntityIDs[i].mPlayerID)
               {
                  pResultArmy = pArmy;
               }
            }               
         }
      }
   }

   return (pResultArmy);
}


//==============================================================================
//==============================================================================
void BEntityGrouper::actualGroupEntities()
{
   BASSERTM(!mProtoSquad, "actualGroupEntities called when mProtoSquad setting is true.");
   //Make sure we have something in our internal entity list.
   if (mEntityIDs.getSize() == 0)
      return;

   //If we have a common parent ID, just add the entire group and be done.
   if (mFlagCommonParentShortcut)
   {
      BEntityID commonID = commonParentID(mEntityIDs);
      if (commonID.isValid())
      {
         createGroup(mEntityIDs);
         return;
      }
   }
   // First, see if we're using the relaxed speed grouping flag.  If any of the entities in question have that flag set,
   // then we will set the flag.
   bool bUseRelaxedSpeedGrouping = false;
   for (uint i = 0; i < mEntityIDs.getSize(); i++)
   {
      //Grab the "center" entity.
//-- FIXING PREFIX BUG ID 2423
      const BEntity* entity = gWorld->getEntity(mEntityIDs[i]);
//--
      if (!entity)
      {
         continue;
      }
      const BSquad* pSquad = entity->getSquad();
      if (pSquad)
      {
         
         BUnit *pUnit = pSquad->getLeaderUnit();
         if (pUnit && pUnit->getProtoObject())
         {
            if (pUnit->getProtoObject()->getFlagUsedRelaxedSpeedGroup())
            {
               bUseRelaxedSpeedGrouping = true;
               break;
            }
         }
      }
      else
      {
         const BUnit *pUnit = entity->getUnit();
         if (pUnit && pUnit->getProtoObject())
         {
            if (pUnit->getProtoObject()->getFlagUsedRelaxedSpeedGroup())
            {
               bUseRelaxedSpeedGrouping = true;
               break;
            }
         }
      }
   }


   BVector overallBestCenter;

   //Do the whole deal for the rest of the list.
   while (mEntityIDs.getSize() > 0)
   {
      BEntityID bestID;
      mCurrentBest.clear();
      mOverallBest.clear();

      for (uint i = 0; i < mEntityIDs.getSize(); i++)
      {
         //Grab the "center" entity.
//-- FIXING PREFIX BUG ID 2424
         const BEntity* entity = gWorld->getEntity(mEntityIDs[i]);
//--
         if (!entity)
         {
            continue;
         }
         // DLM 6/30/08 - if we're grouping by speed, then get the current speed we're working on. 
         float currentVelocity = entity->getDesiredVelocity();
         float speedDif = 1.0f;
         if (bUseRelaxedSpeedGrouping)
            speedDif = 2.0f;

         const BSquad* pSquad = entity->getSquad();
         long nCurrentMovementType = -1;
         if (pSquad)
         {
            const BUnit* pLeaderUnit = pSquad->getLeaderUnit();
            if (pLeaderUnit)
            {
               nCurrentMovementType = pLeaderUnit->getProtoObject()->getMovementType();
            }
         }


         //Create the list of entities that would be added with this "center" entity.
         for (uint j = 0; j < mEntityIDs.getSize(); j++)
         {
//-- FIXING PREFIX BUG ID 2425
            const BEntity* tempEntity = gWorld->getEntity(mEntityIDs[j]);
//--
            if (!tempEntity)
            {
               continue;
            }

            if (entity != tempEntity)
            {
               //Ignore entities that have too large of a speed differential.
               //float vD=(float)fabs(unit->getMaximumVelocity()-tempUnit->getMaximumVelocity());
               //if (vD > game->getWorld()->getAutoFormationVelocityDifferentialCutoff())
               //   continue;

               //Units with different movement types don't mix.
               //if (unit->getProtoUnit()->getMovementType() != tempUnit->getProtoUnit()->getMovementType())
               //   continue;
               // Units with different movement types REALLY shouldn't be together, so updating this for a new unit/squad paradigm.
               const BSquad* pTempEntitySquad = tempEntity->getSquad();
               if (pTempEntitySquad && pTempEntitySquad->getLeaderUnit() && pTempEntitySquad->getLeaderUnit()->getProtoObject()->getMovementType() != nCurrentMovementType)
                  continue;
               
               // Grouping by speed?
               if (mSpeed)
               {
                  if (fabs(tempEntity->getDesiredVelocity() - currentVelocity) > speedDif)
                  {
                     continue;
                  }
               }
               
               //Ignore entities that are too far away.
               float d = entity->calculateXZDistanceSqr(tempEntity->getPosition());
               if (d > mRadiusSqr)
                  continue;

            }

            //Add it.
            mCurrentBest.add(tempEntity->getID());
         }

         //If the current list of entities is more than the previous best, save the new list.
         if (mCurrentBest.getSize() > mOverallBest.getSize())
         {
            bestID = entity->getID();
            mOverallBest.clear();
            overallBestCenter = entity->getPosition();
            for (uint k = 0; k < mCurrentBest.getSize(); k++)
               mOverallBest.add(mCurrentBest[k]);
         }

         //Reset the current best count.
         mCurrentBest.clear();
      }

      //If we don't have a best entity here, just break out.
      if (!bestID.isValid())
         break;

      //Now, add the group that was "created".
      createGroup(mOverallBest, overallBestCenter);

      //If we added all of the entities that we had left, just break out.
      if (mOverallBest.getSize() >= mEntityIDs.getSize())
         break;
      //Remove the entities that we added.
      for (uint j = 0; j < mOverallBest.getSize(); j++)
         mEntityIDs.remove(mOverallBest[j]);
   }
}


//==============================================================================
//==============================================================================
void BEntityGrouper::actualGroupSquadsByProtoSquad()
{
   BASSERTM(mProtoSquad, "actualGroupSquadsByProtoSquad called without mProtoSquad option enabled.");

   //Make sure we have something in our internal entity list.
   if (mEntityIDs.getSize() == 0)
      return;

   BVector overallBestCenterPos;

   //Do the whole deal for the rest of the list.
   while (mEntityIDs.getSize() > 0)
   {
      BEntityID bestCenterSquadID;
      mCurrentBest.resize(0);
      mOverallBest.resize(0);

      for (uint i = 0; i < mEntityIDs.getSize(); i++)
      {
         //Grab the "center" entity.
         const BSquad* pCenterSquad = gWorld->getSquad(mEntityIDs[i]);
         if (!pCenterSquad)
            continue;

         BProtoSquadID currentProtoSquadID = pCenterSquad->getProtoSquadID();

         //Create the list of entities that would be added with this "center" entity.
         uint numEntityIDs = mEntityIDs.getSize();
         for (uint j=0; j<numEntityIDs; j++)
         {
            const BSquad* pTempSquad = gWorld->getSquad(mEntityIDs[j]);
            if (!pTempSquad)
               continue;

            if (pCenterSquad != pTempSquad)
            {
               // We must match proto squads.
               if (currentProtoSquadID != pTempSquad->getProtoSquadID())
                  continue;

               // We must be close enough.
               if (pCenterSquad->calculateXZDistanceSqr(pTempSquad->getPosition()) > mRadiusSqr)
                  continue;
            }

            mCurrentBest.add(pTempSquad->getID());
         }

         // Is this list the best?  Save it.
         if (mCurrentBest.getSize() > mOverallBest.getSize())
         {
            bestCenterSquadID = pCenterSquad->getID();
            overallBestCenterPos = pCenterSquad->getPosition();
            mOverallBest = mCurrentBest;
         }

         //Reset the current best count.
         mCurrentBest.resize(0);
      }

      //If we don't have a best entity here, just break out.
      if (!bestCenterSquadID.isValid())
         break;

      //Now, add the group that was "created".
      createGroup(mOverallBest, overallBestCenterPos);

      //If we added all of the entities that we had left, just break out.
      if (mOverallBest.getSize() >= mEntityIDs.getSize())
         break;

      //Remove the entities that we added.
      for (uint j = 0; j < mOverallBest.getSize(); j++)
         mEntityIDs.remove(mOverallBest[j], false);
   }
}

//==============================================================================
//==============================================================================
bool BEntityGrouper::createGroup(BEntityID entityID, BVector center)
{
   BEntityGroup* gg=BEntityGroup::getInstance();
   if (!gg)
      return(false);

   gg->addEntity(entityID);
   gg->setCenter(center);

   mGroups.add(gg);

   return(true);
}


//==============================================================================
//==============================================================================
bool BEntityGrouper::createGroup(BEntityID entityID)
{
   BEntityGroup* gg=BEntityGroup::getInstance();
   if (!gg)
      return(false);

   gg->addEntity(entityID);

   mGroups.add(gg);

   return(true);
}


//==============================================================================
//==============================================================================
bool BEntityGrouper::createGroup(const BEntityIDArray &entityIDs, BVector center)
{
   BEntityGroup* gg=BEntityGroup::getInstance();
   if (!gg)
      return(false);

   gg->addEntities(entityIDs);
   gg->setCenter(center);

   mGroups.add(gg);

   return(true);
}

//==============================================================================
//==============================================================================
bool BEntityGrouper::createGroup(const BEntityIDArray &entityIDs)
{
   BEntityGroup* gg=BEntityGroup::getInstance();
   if (!gg)
      return(false);

   gg->addEntities(entityIDs);

   mGroups.add(gg);

   return(true);
}

//==============================================================================
//==============================================================================
void BEntityGrouper::render(void) const
{
   for (uint i=0; i < mGroups.getSize(); i++)
      mGroups[i]->render();
}

//==============================================================================
//==============================================================================
void BEntityGrouper::cleanUp(void)
{
   //Rip through group list and release them all.
   for (uint i=0; i < mGroups.getSize(); i++)
   {
      BEntityGroup *eg=mGroups[i];
      mGroups[i]=NULL;
      BEntityGroup::releaseInstance(eg);
   }
   mGroups.clear();
}

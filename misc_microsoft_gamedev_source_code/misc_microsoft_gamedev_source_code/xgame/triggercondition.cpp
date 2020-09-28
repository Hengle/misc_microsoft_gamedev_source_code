//==============================================================================
// triggercondition.cpp
//
// Copyright (c) Ensemble Studios, 2006-2007
//==============================================================================

// xgame
#include "common.h"
#include "config.h"
#include "ai.h"
#include "bidmgr.h"
#include "commandmanager.h"
#include "commands.h"
#include "database.h"
#include "entityactionidle.h"
#include "generaleventmanager.h"
#include "hintengine.h"
#include "player.h"
#include "protoobject.h"
#include "protopower.h"
#include "selectionmanager.h"
#include "syncmacros.h"
#include "techtree.h"
#include "trigger.h"
#include "triggercondition.h"
#include "triggermanager.h"
#include "triggerscript.h"
#include "triggervar.h"
#include "unitquery.h"
#include "user.h"
#include "usermanager.h"
#include "world.h"
#include "protosquad.h"
#include "math\collision.h"
#include "Quaternion.h"
#include "gamesettings.h"
#include "timermanager.h"
#include "squadactionattack.h"
#include "unitactionrangedattack.h"
#include "unitactionunderattack.h"
#include "game.h"
#include "unitactionbuilding.h"
#include "corpsemanager.h"
#include "visual.h"
#include "transitionManager.h"
#include "prototech.h"
#include "simhelper.h"
#include "pather.h"
#include "unitactionrevive.h"

//#include "Multiplayer.h"
#include "liveSystem.h"

// xsystem
#include "xmlreader.h"

IMPLEMENT_FREELIST(BTriggerCondition, 6, &gSimHeap);

GFIMPLEMENTVERSION(BTriggerCondition, 2);

#define RIGHT_ANGLE_RADIANS Math::fDegToRad(90.0f)
#define TC_RANDOM_LOCATION_SEARCH_MAX 1000


//==============================================================================
// BTriggerCondition::tcCanGetUnits()
//==============================================================================
bool BTriggerCondition::tcCanGetUnits()
{
   bool result = false;
   switch (getVersion())
   {
      case 5:
         result = tcCanGetUnitsV5();
         break;

      case 6:
         result = tcCanGetUnitsV6();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not suported!");
         break;
   }

   return (result);
}

//==============================================================================
// Get units based on filter parameters
//==============================================================================
bool BTriggerCondition::tcCanGetUnitsV5()
{
   enum 
   { 
      cInputLocation = 1, 
      cInputDistance = 2, 
      cInputPlayer = 3, 
      cInputObjectType = 4, 
      cOutputUnitList = 8, 
      cOutputCount = 6, 
      cInputFilterList = 9, 
      cInputPlayerList = 10,
   };

   int numUnits = 0;
   bool usePlayer = getVar(cInputPlayer)->isUsed();
   bool usePlayerList = getVar(cInputPlayerList)->isUsed();
   bool useObjectType = getVar(cInputObjectType)->isUsed();
   bool useLocation = getVar(cInputLocation)->isUsed();
   bool useDistance = getVar(cInputDistance)->isUsed();
   bool useOutputUnitList = getVar(cOutputUnitList)->isUsed();
   bool useOutputCount = getVar(cOutputCount)->isUsed();
   bool useFilterList = getVar(cInputFilterList)->isUsed();
   bool onlyCountUnits = (!useFilterList && !useOutputUnitList); // if I'm not attempting to filter or use the output list, then simply count the # of units
   BEntityIDArray workingList(0,50);
   BEntityIDArray filteredResults(0,50);
//-- FIXING PREFIX BUG ID 3833
   const BEntityIDArray* pResults = &workingList;
//--

   // Populate our list of units to filter.
   // Option #1, get units in an area.
   if (useLocation && useDistance)
   {
      BVector loc = getVar(cInputLocation)->asVector()->readVar();
      float dist = getVar(cInputDistance)->asFloat()->readVar();
      BUnitQuery query(loc, dist, true);
      if (usePlayer)
      {
         BPlayerID playerID = getVar(cInputPlayer)->asPlayer()->readVar();
         query.addPlayerFilter(playerID);
      }

      if (usePlayerList)
      {
         BPlayerIDArray playerIDs = getVar(cInputPlayerList)->asPlayerList()->readVar();
         uint numPlayerIDs = playerIDs.getSize();
         for (uint i = 0; i < numPlayerIDs; i++)
         {
            query.addPlayerFilter(playerIDs[i]);
         }
      }

      if (useObjectType)
      {
         long objectType = getVar(cInputObjectType)->asObjectType()->readVar();
         query.addObjectTypeFilter(objectType);
      }

      query.setFlagIgnoreDead(true);
      numUnits = gWorld->getUnitsInArea(&query, &workingList);
   }
   // Option #2, get all units in the game.
   else
   {      
      BPlayerIDArray playerIDs;
      if (usePlayerList)
      {
         playerIDs = getVar(cInputPlayerList)->asPlayerList()->readVar();
      }

      if (usePlayer)
      {
         playerIDs.uniqueAdd(getVar(cInputPlayer)->asPlayer()->readVar());
      }

      long objectType = useObjectType ? getVar(cInputObjectType)->asObjectType()->readVar() : -1;
      BEntityHandle h = cInvalidObjectID;
//-- FIXING PREFIX BUG ID 3832
      const BUnit* pUnit = gWorld->getNextUnit(h);
//--
      while (pUnit)
      {
         if (!pUnit->isAlive())
         {
            pUnit = gWorld->getNextUnit(h);
            continue;
         }

         bool playerMatch = true;
         uint numPlayerIDs = playerIDs.getSize();
         if (numPlayerIDs > 0)
         {
            playerMatch = false;
            for (uint i = 0; i < numPlayerIDs; i++)
            {
               if (pUnit->getPlayerID() == playerIDs[i])
               {
                  playerMatch = true;
                  break;
               }
            }
         }

         if (!playerMatch)
         {
            pUnit = gWorld->getNextUnit(h);
            continue;
         }

         if (useObjectType && !pUnit->isType(objectType))
         {
            pUnit = gWorld->getNextUnit(h);
            continue;
         }

         // if I'm only here to count units but I don't care about
         // the exact number then return now
         if (onlyCountUnits && !useOutputCount)
            return (true);
         else if (!onlyCountUnits)
            workingList.add(pUnit->getID());

         ++numUnits;
         pUnit = gWorld->getNextUnit(h);
      }
   }

   if (useFilterList)
   {
      const BEntityIDArray& filterList = getVar(cInputFilterList)->asUnitList()->readVar();
      uint numInWorkingList = workingList.getSize();
      for (uint i = 0; i < numInWorkingList; i++)
      {
         BEntityID entityID = workingList[i];
         if (filterList.find(entityID) != cInvalidIndex)
         {
            filteredResults.add(entityID);
            continue;
         }
      }
      numUnits = filteredResults.getNumber();
      pResults = &filteredResults;
   }

   if (useOutputCount)
   {
      getVar(cOutputCount)->asInteger()->writeVar(numUnits);
   }

   if (useOutputUnitList)
   {
      getVar(cOutputUnitList)->asUnitList()->writeVar(*pResults);
   }

   return (numUnits > 0);
}

//==============================================================================
// Get units based on filter parameters
//==============================================================================
bool BTriggerCondition::tcCanGetUnitsV6()
{
   enum 
   { 
      cInputLocation = 1, 
      cInputDistance = 2, 
      cInputPlayer = 3, 
      cInputObjectType = 4, 
      cOutputUnitList = 8, 
      cOutputCount = 6, 
      cInputFilterList = 9, 
      cInputPlayerList = 10,
      cInputBoxForward = 11,
      cInputBoxHalfX = 13,
      cInputBoxHalfY = 14,
      cInputBoxHalfZ = 15,
   };

   bool usePlayer = getVar(cInputPlayer)->isUsed();
   bool usePlayerList = getVar(cInputPlayerList)->isUsed();
   bool useObjectType = getVar(cInputObjectType)->isUsed();
   bool useLocation = getVar(cInputLocation)->isUsed();
   bool useDistance = getVar(cInputDistance)->isUsed();
   bool useOutputUnitList = getVar(cOutputUnitList)->isUsed();
   bool useOutputCount = getVar(cOutputCount)->isUsed();
   bool useFilterList = getVar(cInputFilterList)->isUsed();
   bool onlyCountUnits = (!useFilterList && !useOutputUnitList); // if I'm not attempting to filter or use the output list, then simply count the # of units
   BEntityIDArray results(0,50);
   bool useBoxForward = getVar(cInputBoxForward)->isUsed();
   bool useBoxHalfX = getVar(cInputBoxHalfX)->isUsed();
   bool useBoxHalfY = getVar(cInputBoxHalfY)->isUsed();
   bool useBoxHalfZ = getVar(cInputBoxHalfZ)->isUsed();

   bool sphereSearch = (useLocation && useDistance);
   bool boxSearch = (useLocation && useBoxForward && useBoxHalfX && useBoxHalfY && useBoxHalfZ);

   // Populate our list of units to filter.
   // Option #1, get units in an area.
   if (sphereSearch || boxSearch)
   {
      BUnitQuery query;
      if (sphereSearch)
      {
         BVector loc = getVar(cInputLocation)->asVector()->readVar();
         float dist = getVar(cInputDistance)->asFloat()->readVar();
         query = BUnitQuery(loc, dist, true);
      }
      else
      {
         BVector loc = getVar(cInputLocation)->asVector()->readVar();         
         BVector forward = getVar(cInputBoxForward)->asVector()->readVar();
         BVector up = cYAxisVector;         
         BVector right;        
         right.assignCrossProduct(up, forward);
         right.normalize();

         float extX = getVar(cInputBoxHalfX)->asFloat()->readVar();
         float extY = getVar(cInputBoxHalfY)->asFloat()->readVar();
         float extZ = getVar(cInputBoxHalfZ)->asFloat()->readVar();

         BVector upScale = up * extY;
         BVector center = loc + upScale;
         query = BUnitQuery(center, forward, right, extX, extY, extZ);
      }

      if (usePlayer)
      {
         BPlayerID playerID = getVar(cInputPlayer)->asPlayer()->readVar();
         query.addPlayerFilter(playerID);
      }

      if (usePlayerList)
      {
         BPlayerIDArray playerIDs = getVar(cInputPlayerList)->asPlayerList()->readVar();
         uint numPlayerIDs = playerIDs.getSize();
         for (uint i = 0; i < numPlayerIDs; i++)
         {
            query.addPlayerFilter(playerIDs[i]);
         }
      }

      if (useObjectType)
      {
         long objectType = getVar(cInputObjectType)->asObjectType()->readVar();
         query.addObjectTypeFilter(objectType);
      }

      query.setFlagIgnoreDead(true);
      gWorld->getUnitsInArea(&query, &results);

      if (useFilterList)
      {
         const BEntityIDArray& filterList = getVar(cInputFilterList)->asUnitList()->readVar();
         for (uint i=0; i<results.getSize(); /*i++*/)
         {
            if (!filterList.contains(results[i]))
            {
               results.removeIndex(i, false);
               continue;
            }
            ++i;
         }
      }
   }
   // Option #2, get all units in the game.
   else
   {
      BEntityIDArray workingUnitIDs;
      DWORD playerIDBitArray = 0;
      if (usePlayerList || usePlayer)
      {
         if (usePlayerList)
         {
            const BPlayerIDArray& rPlayerIDs = getVar(cInputPlayerList)->asPlayerList()->readVar();
            for (uint i=0; i<rPlayerIDs.getSize(); i++)
            {
               playerIDBitArray |= (1 << rPlayerIDs[i]);
            }
         }
         if (usePlayer)
         {
            BPlayerID pid = getVar(cInputPlayer)->asPlayer()->readVar();
            playerIDBitArray |= (1 << pid);
         }
      }
      else
      {
         playerIDBitArray = 0xFFFFFFFF;
      }


      // Units of all players.
      BObjectTypeID objectTypeID = cInvalidObjectTypeID;
      if (useObjectType)
         objectTypeID = getVar(cInputObjectType)->asObjectType()->readVar();
      long numPlayers = gWorld->getNumberPlayers();
      BEntityIDArray playerUnitIDs;
      for (long pid=0; pid<numPlayers; pid++)
      {
         if (!(playerIDBitArray & (1 << pid)))
            continue;
         const BPlayer* pPlayer = gWorld->getPlayer(pid);
         if (!pPlayer)
            continue;
         if (useObjectType)
         {
            pPlayer->getUnitsOfType(objectTypeID, playerUnitIDs);
            workingUnitIDs.append(playerUnitIDs);
         }
         else
         {
            pPlayer->getUnits(playerUnitIDs);
            workingUnitIDs.append(playerUnitIDs);
         }
      }

      for (uint i=0; i<workingUnitIDs.getSize(); /*i++*/)
      {
         const BUnit* pUnit = gWorld->getUnit(workingUnitIDs[i]);
         if (!pUnit || !pUnit->isAlive())
         {
            workingUnitIDs.removeIndex(i, false);
            continue;
         }

         if (useFilterList)
         {
            const BEntityIDArray& filterList = getVar(cInputFilterList)->asUnitList()->readVar();
            if (!filterList.contains(workingUnitIDs[i]))
            {
               workingUnitIDs.removeIndex(i, false);
               continue;
            }
         }

         if (onlyCountUnits && !useOutputCount)
            return (true);

         ++i;
      }

      results = workingUnitIDs;
   }

   int numUnits = results.getNumber();
   if (useOutputCount)
   {
      getVar(cOutputCount)->asInteger()->writeVar(numUnits);
   }

   if (useOutputUnitList)
   {
      getVar(cOutputUnitList)->asUnitList()->writeVar(results);
   }

   return (numUnits > 0);
}

//==============================================================================
// BTriggerCondition::tcCanGetSquads()
//==============================================================================
bool BTriggerCondition::tcCanGetSquads()
{
   bool result = false;
   switch (getVersion())
   {
      case 8:
         result = tcCanGetSquadsV8();
         break;

      case 9:
         result = tcCanGetSquadsV9();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }

   return (result);
}

//==============================================================================
// Get squads based on filter parameters
//==============================================================================
bool BTriggerCondition::tcCanGetSquadsV8()
{
   enum   
   { 
      cInputLocation = 1, 
      cInputDistance = 2, 
      cInputPlayer = 3, 
      cOutputSquadList = 9, 
      cInputProtoSquad = 6, 
      cOutputCount = 7, 
      cInputFilterList = 10, 
      cObjectType = 11, 
      cInputPlayerList = 12,
      cInputBoxForward = 15,
      cInputBoxHalfX = 16,
      cInputBoxHalfY = 17,
      cInputBoxHalfZ = 18,
   };

   bool usePlayer = getVar(cInputPlayer)->isUsed();
   bool usePlayerList = getVar(cInputPlayerList)->isUsed();
   bool useProtoSquad = getVar(cInputProtoSquad)->isUsed();
   bool useLocation = getVar(cInputLocation)->isUsed();
   bool useDistance = getVar(cInputDistance)->isUsed();
   bool useOutputSquadList = getVar(cOutputSquadList)->isUsed();
   bool useOutputCount = getVar(cOutputCount)->isUsed();
   bool useFilterList = getVar(cInputFilterList)->isUsed();
   bool useObjectType = getVar(cObjectType)->isUsed();
   long objectType = (useObjectType) ? getVar(cObjectType)->asObjectType()->readVar() : -1;
   BEntityIDArray results(0,50);
   bool useBoxForward = getVar(cInputBoxForward)->isUsed();
   bool useBoxHalfX = getVar(cInputBoxHalfX)->isUsed();
   bool useBoxHalfY = getVar(cInputBoxHalfY)->isUsed();
   bool useBoxHalfZ = getVar(cInputBoxHalfZ)->isUsed();

   bool sphereSearch = (useLocation && useDistance);
   bool boxSearch = (useLocation && useBoxForward && useBoxHalfX && useBoxHalfY && useBoxHalfZ);

   // Populate our list of squads to filter.
   // Option #1, get squads in an area.
   if (sphereSearch || boxSearch)
   {
      BEntityIDArray workingList(0,50);
      BUnitQuery query;
      if (sphereSearch)
      {
         BVector loc = getVar(cInputLocation)->asVector()->readVar();
         float dist = getVar(cInputDistance)->asFloat()->readVar();
         query = BUnitQuery(loc, dist, true);
      }
      else
      {
         BVector loc = getVar(cInputLocation)->asVector()->readVar();         
         BVector forward = getVar(cInputBoxForward)->asVector()->readVar();
         BVector up = cYAxisVector;         
         BVector right;        
         right.assignCrossProduct(up, forward);
         right.normalize();

         float extX = getVar(cInputBoxHalfX)->asFloat()->readVar();
         float extY = getVar(cInputBoxHalfY)->asFloat()->readVar();
         float extZ = getVar(cInputBoxHalfZ)->asFloat()->readVar();

         BVector upScale = up * extY;
         BVector center = loc + upScale;
         query = BUnitQuery(center, forward, right, extX, extY, extZ);
      }

      if (usePlayer)
      {
         BPlayerID playerID = getVar(cInputPlayer)->asPlayer()->readVar();
         query.addPlayerFilter(playerID);
      }

      if (usePlayerList)
      {
         BPlayerIDArray playerIDs = getVar(cInputPlayerList)->asPlayerList()->readVar();
         uint numPlayerIDs = playerIDs.getSize();
         for (uint i = 0; i < numPlayerIDs; i++)
         {
            query.addPlayerFilter(playerIDs[i]);
         }
      }

      if (useObjectType)
      {
         query.addObjectTypeFilter(objectType);
      }

      query.setFlagIgnoreDead(true);
      gWorld->getSquadsInArea(&query, &workingList);

      uint numInWorkingList = workingList.getSize();
      for (uint i = 0; i < numInWorkingList; i++)
      {
         BEntityID squadID = workingList[i];
         BSquad* pSquad = gWorld->getSquad(squadID);
         if (useProtoSquad)
         {
            BProtoSquadID protoSquadID = getVar(cInputProtoSquad)->asProtoSquad()->readVar();
            if (!pSquad || (pSquad->getProtoSquadID() != protoSquadID))
               continue;
         }

         if (useFilterList)
         {
            const BEntityIDArray& filterList = getVar(cInputFilterList)->asSquadList()->readVar();
            if (filterList.find(squadID) == cInvalidIndex)
               continue;
         }

         if (useObjectType)
         {
            bool allChildrenQualify = true;
            uint numSquadChildren = pSquad->getNumberChildren();
            for (uint child = 0; child < numSquadChildren; child++)
            {
               BUnit* pChildUnit = gWorld->getUnit(pSquad->getChild(child));
               if (!pChildUnit)
               {
                  allChildrenQualify = false;
                  break;
               }

               if (!pChildUnit->isType(objectType))
               {
                  allChildrenQualify = false;
                  break;
               }
            }

            if (!allChildrenQualify)
               continue;
         }

         results.add(squadID);
      }
   }
   // Option #2, get all squads in the game.
   else
   {
      BPlayerIDArray playerIDs;
      if (usePlayerList)
      {
         playerIDs = getVar(cInputPlayerList)->asPlayerList()->readVar();
      }

      if (usePlayer)
      {
         playerIDs.uniqueAdd(getVar(cInputPlayer)->asPlayer()->readVar());
      }

      BProtoSquadID protoSquadID = useProtoSquad ? getVar(cInputProtoSquad)->asProtoSquad()->readVar() : cInvalidProtoSquadID;
      BEntityHandle h = cInvalidObjectID;
      BSquad* pSquad = gWorld->getNextSquad(h);
      while (pSquad)
      {
         if (!pSquad->isAlive())
         {
            pSquad = gWorld->getNextSquad(h);
            continue;
         }

         bool playerMatch = true;
         uint numPlayerIDs = playerIDs.getSize();
         if (numPlayerIDs > 0)
         {
            playerMatch = false;
            for (uint i = 0; i < numPlayerIDs; i++)
            {
               if (pSquad->getPlayerID() == playerIDs[i])
               {
                  playerMatch = true;
                  break;
               }
            }
         }

         if (!playerMatch)
         {
            pSquad = gWorld->getNextSquad(h);
            continue;
         }

         if (useProtoSquad && (pSquad->getProtoSquadID() != protoSquadID))
         {
            pSquad = gWorld->getNextSquad(h);
            continue;
         }

         if (useFilterList)
         {
            BEntityID squadID = pSquad->getID();
            const BEntityIDArray& filterList = getVar(cInputFilterList)->asSquadList()->readVar();
            if (filterList.find(squadID) == cInvalidIndex)
            {
               pSquad = gWorld->getNextSquad(h);
               continue;
            }
         }

         if (useObjectType)
         {
            bool allChildrenQualify = true;
            uint numSquadChildren = pSquad->getNumberChildren();
            for (uint child = 0; child < numSquadChildren; child++)
            {
               BUnit* pChildUnit = gWorld->getUnit(pSquad->getChild(child));
               if (!pChildUnit)
               {
                  allChildrenQualify = false;
                  break;
               }

               if (!pChildUnit->isType(objectType))
               {
                  allChildrenQualify = false;
                  break;
               }
            }

            if (!allChildrenQualify)
            {
               pSquad = gWorld->getNextSquad(h);
               continue;
            }
         }

         results.add(pSquad->getID());
         pSquad = gWorld->getNextSquad(h);
      }
   }

   int numSquads = results.getNumber();
   if (useOutputCount)
   {
      getVar(cOutputCount)->asInteger()->writeVar(numSquads);
   }

   if (useOutputSquadList)
   {
      getVar(cOutputSquadList)->asSquadList()->writeVar(results);
   }

   return (numSquads > 0);
}

//==============================================================================
// Get squads based on filter parameters
//==============================================================================
bool BTriggerCondition::tcCanGetSquadsV9()
{
   enum   
   { 
      cInputLocation = 1, 
      cInputDistance = 2, 
      cInputPlayer = 3, 
      cOutputSquadList = 9, 
      cInputProtoSquad = 6, 
      cOutputCount = 7, 
      cInputFilterList = 10, 
      cObjectType = 11, 
      cInputPlayerList = 12,
      cInputBoxForward = 15,
      cInputBoxHalfX = 16,
      cInputBoxHalfY = 17,
      cInputBoxHalfZ = 18,
      cInputIgnoreAir = 19,
   };

   bool usePlayer = getVar(cInputPlayer)->isUsed();
   bool usePlayerList = getVar(cInputPlayerList)->isUsed();
   bool useProtoSquad = getVar(cInputProtoSquad)->isUsed();
   bool useLocation = getVar(cInputLocation)->isUsed();
   bool useDistance = getVar(cInputDistance)->isUsed();
   bool useOutputSquadList = getVar(cOutputSquadList)->isUsed();
   bool useOutputCount = getVar(cOutputCount)->isUsed();
   bool useFilterList = getVar(cInputFilterList)->isUsed();
   bool useObjectType = getVar(cObjectType)->isUsed();
   long objectType = (useObjectType) ? getVar(cObjectType)->asObjectType()->readVar() : -1;
   BEntityIDArray results(0,50);
   bool useBoxForward = getVar(cInputBoxForward)->isUsed();
   bool useBoxHalfX = getVar(cInputBoxHalfX)->isUsed();
   bool useBoxHalfY = getVar(cInputBoxHalfY)->isUsed();
   bool useBoxHalfZ = getVar(cInputBoxHalfZ)->isUsed();
   bool ignoreAir = getVar(cInputIgnoreAir)->isUsed() && getVar(cInputIgnoreAir)->asBool()->readVar();

   bool sphereSearch = (useLocation && useDistance);
   bool boxSearch = (useLocation && useBoxForward && useBoxHalfX && useBoxHalfY && useBoxHalfZ);

   // Populate our list of squads to filter.
   // Option #1, get squads in an area.
   if (sphereSearch || boxSearch)
   {
      BEntityIDArray workingList(0,50);
      BUnitQuery query;
      if (sphereSearch)
      {
         BVector loc = getVar(cInputLocation)->asVector()->readVar();
         float dist = getVar(cInputDistance)->asFloat()->readVar();
         query = BUnitQuery(loc, dist, true);
      }
      else
      {
         BVector loc = getVar(cInputLocation)->asVector()->readVar();         
         BVector forward = getVar(cInputBoxForward)->asVector()->readVar();
         BVector up = cYAxisVector;         
         BVector right;        
         right.assignCrossProduct(up, forward);
         right.normalize();

         float extX = getVar(cInputBoxHalfX)->asFloat()->readVar();
         float extY = getVar(cInputBoxHalfY)->asFloat()->readVar();
         float extZ = getVar(cInputBoxHalfZ)->asFloat()->readVar();

         BVector upScale = up * extY;
         BVector center = loc + upScale;
         query = BUnitQuery(center, forward, right, extX, extY, extZ);
      }

      if (usePlayer)
      {
         BPlayerID playerID = getVar(cInputPlayer)->asPlayer()->readVar();
         query.addPlayerFilter(playerID);
      }

      if (usePlayerList)
      {
         BPlayerIDArray playerIDs = getVar(cInputPlayerList)->asPlayerList()->readVar();
         uint numPlayerIDs = playerIDs.getSize();
         for (uint i = 0; i < numPlayerIDs; i++)
         {
            query.addPlayerFilter(playerIDs[i]);
         }
      }

      if (useObjectType)
      {
         query.addObjectTypeFilter(objectType);
      }

      query.setFlagIgnoreDead(true);
      gWorld->getSquadsInArea(&query, &workingList);

      uint numInWorkingList = workingList.getSize();
      for (uint i = 0; i < numInWorkingList; i++)
      {
         BEntityID squadID = workingList[i];
         BSquad* pSquad = gWorld->getSquad(squadID);
         if (useProtoSquad)
         {
            BProtoSquadID protoSquadID = getVar(cInputProtoSquad)->asProtoSquad()->readVar();
            if (!pSquad || (pSquad->getProtoSquadID() != protoSquadID))
               continue;
         }

         if (useFilterList)
         {
            const BEntityIDArray& filterList = getVar(cInputFilterList)->asSquadList()->readVar();
            if (filterList.find(squadID) == cInvalidIndex)
               continue;
         }

         if (useObjectType)
         {
            bool allChildrenQualify = true;
            uint numSquadChildren = pSquad->getNumberChildren();
            for (uint child = 0; child < numSquadChildren; child++)
            {
               BUnit* pChildUnit = gWorld->getUnit(pSquad->getChild(child));
               if (!pChildUnit)
               {
                  allChildrenQualify = false;
                  break;
               }

               if (!pChildUnit->isType(objectType))
               {
                  allChildrenQualify = false;
                  break;
               }
            }

            if (!allChildrenQualify)
               continue;
         }

         if (ignoreAir && pSquad)
         {
            BUnit* pLeader = pSquad->getLeaderUnit();
            if (pSquad->getFlagFlying() || !pLeader || pLeader->isPhysicsAircraft())
               continue;
         }

         results.add(squadID);
      }
   }
   // Option #2, get all squads in the game.
   else
   {
      BEntityIDArray workingSquadIDs;
      DWORD playerIDBitArray = 0;
      if (usePlayerList || usePlayer)
      {
         if (usePlayerList)
         {
            const BPlayerIDArray& rPlayerIDs = getVar(cInputPlayerList)->asPlayerList()->readVar();
            for (uint i=0; i<rPlayerIDs.getSize(); i++)
            {
               playerIDBitArray |= (1 << rPlayerIDs[i]);
            }
         }
         if (usePlayer)
         {
            BPlayerID pid = getVar(cInputPlayer)->asPlayer()->readVar();
            playerIDBitArray |= (1 << pid);
         }
      }
      else
      {
         playerIDBitArray = 0xFFFFFFFF;
      }


      // Squads of all players.
      BProtoSquadID protoSquadID = cInvalidProtoSquadID;
      if (useProtoSquad)
         protoSquadID = getVar(cInputProtoSquad)->asProtoSquad()->readVar();
      long numPlayers = gWorld->getNumberPlayers();
      BEntityIDArray playerSquadIDs;
      for (long pid=0; pid<numPlayers; pid++)
      {
         if (!(playerIDBitArray & (1 << pid)))
            continue;
         const BPlayer* pPlayer = gWorld->getPlayer(pid);
         if (!pPlayer)
            continue;
         if (useProtoSquad)
         {
            pPlayer->getSquadsOfType(protoSquadID, playerSquadIDs);
            workingSquadIDs.append(playerSquadIDs);
         }
         else
         {
            pPlayer->getSquads(playerSquadIDs);
            workingSquadIDs.append(playerSquadIDs);
         }
      }

      for (uint i=0; i<workingSquadIDs.getSize(); /*i++*/)
      {
         const BSquad* pSquad = gWorld->getSquad(workingSquadIDs[i]);
         if (!pSquad || !pSquad->isAlive())
         {
            workingSquadIDs.removeIndex(i, false);
            continue;
         }

         if (ignoreAir)
         {
            if (pSquad->getFlagFlying())
            {
               workingSquadIDs.removeIndex(i, false);
               continue;
            }
            const BUnit* pLeader = pSquad->getLeaderUnit();
            if (!pLeader || pLeader->isPhysicsAircraft())
            {
               workingSquadIDs.removeIndex(i, false);
               continue;
            }
         }

         if (useFilterList)
         {
            const BEntityIDArray& filterList = getVar(cInputFilterList)->asSquadList()->readVar();
            if (!filterList.contains(workingSquadIDs[i]))
            {
               workingSquadIDs.removeIndex(i, false);
               continue;
            }
         }

         if (useObjectType)
         {
            bool allChildrenQualify = true;
            uint numSquadChildren = pSquad->getNumberChildren();
            for (uint child = 0; child < numSquadChildren; child++)
            {
               BUnit* pChildUnit = gWorld->getUnit(pSquad->getChild(child));
               if (!pChildUnit)
               {
                  allChildrenQualify = false;
                  break;
               }

               if (!pChildUnit->isType(objectType))
               {
                  allChildrenQualify = false;
                  break;
               }
            }

            if (!allChildrenQualify)
            {
               workingSquadIDs.removeIndex(i, false);
               continue;
            }
         }

         ++i;
      }

      results = workingSquadIDs;
   }

   int numSquads = results.getNumber();
   if (useOutputCount)
   {
      getVar(cOutputCount)->asInteger()->writeVar(numSquads);
   }

   if (useOutputSquadList)
   {
      getVar(cOutputSquadList)->asSquadList()->writeVar(results);
   }

   return (numSquads > 0);
}

//==============================================================================
// BTriggerCondition::tcTechStatus()
//==============================================================================
bool BTriggerCondition::tcTechStatus()
{
   uint version = getVersion();
   switch (version)
   {
      case 1:
      {
         return (tcTechStatusV1());
      }
      case 2:
      {
         return (tcTechStatusV2());
      }
      default:
      {
         BTRIGGER_ASSERT(false);
         return (false);
      }
   }
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcTechStatusV1()
{
   enum { cInputPlayer = 1, cInputTech = 2, cInputTechStatus = 3, };
   long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   long techID = getVar(cInputTech)->asTech()->readVar();
   long techStatus = getVar(cInputTechStatus)->asTechStatus()->readVar();

   // Check our tech status.
   BPlayer *pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return (false);
   BTechTree *pTechTree = pPlayer->getTechTree();
   if (!pTechTree)
      return (false);
   long actualTechStatus = pTechTree->getTechStatus(techID, -1);
   return (techStatus == actualTechStatus);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcTechStatusV2()
{
   enum { cInputPlayer = 1, cInputTech = 2, cInputTechStatus = 3, cUnit = 4, };
   long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   long techID = getVar(cInputTech)->asTech()->readVar();
   long techStatus = getVar(cInputTechStatus)->asTechStatus()->readVar();
   BEntityID unitID = cInvalidObjectID;
//-- FIXING PREFIX BUG ID 3843
   const BTriggerVarUnit* pUnitVar = getVar(cUnit)->asUnit();
//--
   if (pUnitVar->isUsed())
   {
      BEntityID testUnitID = pUnitVar->readVar();
      if (gWorld->getUnit(testUnitID))
         unitID = testUnitID;
   }

   // Check our tech status.
   BPlayer *pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return (false);
   BTechTree *pTechTree = pPlayer->getTechTree();
   if (!pTechTree)
      return (false);
   long actualTechStatus = pTechTree->getTechStatus(techID, unitID);
   return (techStatus == actualTechStatus);
}


//==============================================================================
// BTriggerCondition::tcTriggerActiveTime()
//==============================================================================
bool BTriggerCondition::tcTriggerActiveTime()
{
   enum { cInputOperator = 1, cInputTime = 2, };
   long op = getVar(cInputOperator)->asOperator()->readVar();
   DWORD time = getVar(cInputTime)->asTime()->readVar();

   BFATAL_ASSERT(mpParentTrigger);
   DWORD activatedTime = mpParentTrigger->getActivatedTime();
   DWORD currentTime = gWorld->getGametime();
   DWORD timeDifference = currentTime - activatedTime;

   bool result = compareValues(timeDifference, op, time);
   return (result);
}


//==============================================================================
// BTriggerCondition::tcUnitUnitDistance()
//==============================================================================
bool BTriggerCondition::tcUnitUnitDistance()
{
   uint version = getVersion();
   if (version == 2)
   {
      return (tcUnitUnitDistanceV2());
   }
   else
   {
      BTRIGGER_ASSERT(false);
      return (false);
   }   
}


//==============================================================================
// Compare against the unit to unit distance
//==============================================================================
bool BTriggerCondition::tcUnitUnitDistanceV2()
{
   enum 
   { 
      cInputUnit1 = 1, 
      cInputUnit2 = 2, 
      cInputOperator = 3, 
      cInputDistance = 4, 
   };

   BEntityID unit1ID = getVar(cInputUnit1)->asUnit()->readVar();
   BEntityID unit2ID = getVar(cInputUnit2)->asUnit()->readVar();
   long op = getVar(cInputOperator)->asOperator()->readVar();
   float compareDist = getVar(cInputDistance)->asFloat()->readVar();

//-- FIXING PREFIX BUG ID 3845
   const BUnit* pUnit1 = gWorld->getUnit(unit1ID);
//--
   if (!pUnit1)
      return (false);
//-- FIXING PREFIX BUG ID 3846
   const BUnit* pUnit2 = gWorld->getUnit(unit2ID);
//--
   if (!pUnit2)
      return (false);

   float dist = pUnit1->BEntity::calculateXZDistance(pUnit1->getPosition(), pUnit2);

   bool result = compareValues(dist, op, compareDist);
   return (result);
}


//==============================================================================
// BTriggerCondition::tcUnitLocationDistance()
//==============================================================================
bool BTriggerCondition::tcUnitLocationDistance()
{
   uint version = getVersion();
   if (version == 2)
   {
      return (tcUnitLocationDistanceV2());
   }
   else
   {
      BTRIGGER_ASSERT(false);
      return (false);
   }   
}


//==============================================================================
// Compare against the unit to location distance
//==============================================================================
bool BTriggerCondition::tcUnitLocationDistanceV2()
{
   enum 
   { 
      cInputUnit = 1, 
      cInputLocation = 2, 
      cInputOperator = 3, 
      cInputDistance = 4, 
   };

   BEntityID unitID = getVar(cInputUnit)->asUnit()->readVar();
   BVector comparePos = getVar(cInputLocation)->asVector()->readVar();
   long op = getVar(cInputOperator)->asOperator()->readVar();
   float compareDist = getVar(cInputDistance)->asFloat()->readVar();

//-- FIXING PREFIX BUG ID 3847
   const BUnit* pUnit = gWorld->getUnit(unitID);
//--
   if (!pUnit)
      return (false);

   float dist = pUnit->BEntity::calculateXZDistance(comparePos);

   bool result = compareValues(dist, op, compareDist);
   return (result);
}


//==============================================================================
// BTriggerCondition::tcIsAlive()
//==============================================================================
bool BTriggerCondition::tcIsAlive()
{
   uint version = getVersion();
   if (version == 3)
   {
      return (tcIsAliveV3());
   }
   else
   {
      BTRIGGER_ASSERT(false);
      return (false);
   }
}


//==============================================================================
// BTriggerCondition::tcIsAliveV3()
//==============================================================================
bool BTriggerCondition::tcIsAliveV3()
{
   enum 
   { 
      cUnit = 3, 
      cUnitList = 4, 
      cSquad = 5, 
      cSquadList = 6, 
   };

   BEntityIDArray testEntities;
   if (getVar(cUnitList)->isUsed())
   {
      testEntities = getVar(cUnitList)->asUnitList()->readVar();
   }

   if (getVar(cUnit)->isUsed())
   {
      testEntities.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }

   uint numEntities = testEntities.getSize();
   for (uint i = 0; i < numEntities; i++)
   {
      BUnit* pUnit = gWorld->getUnit(testEntities[i]);
      if (pUnit && pUnit->isAlive() && !pUnit->getFlagDown() && !pUnit->isHibernating())
      {
         return (true);
      }
   }

   if (getVar(cSquadList)->isUsed())
   {
      testEntities = getVar(cSquadList)->asSquadList()->readVar();
   }

   if (getVar(cSquad)->isUsed())
   {
      testEntities.uniqueAdd(getVar(cSquad)->asSquad()->readVar());
   }

   numEntities = testEntities.getSize();
   for (uint i = 0; i < numEntities; i++)
   {
//-- FIXING PREFIX BUG ID 3848
      const BSquad* pSquad = gWorld->getSquad(testEntities[i]);
//--
      if (pSquad && pSquad->isAlive() && !pSquad->isDown() && !pSquad->isHibernating())
      {
         return (true);
      }
   }

   // No alive units or squads.
   return (false);
}


//==============================================================================
// BTriggerCondition::tcIsDead
//==============================================================================
bool BTriggerCondition::tcIsDead()
{
   uint version = getVersion();
   if (version == 3)
   {
      return (tcIsDeadV3());
   }
   else
   {
      BTRIGGER_ASSERT(false);
      return (false);
   }
}


//==============================================================================
// BTriggerCondition::tcIsDeadV3
//==============================================================================
bool BTriggerCondition::tcIsDeadV3()
{
   enum 
   { 
      cUnit = 3, 
      cUnitList = 4, 
      cSquad = 5, 
      cSquadList = 6, 
   };

   // Get units
   BEntityIDArray unitList;
   if (getVar(cUnitList)->isUsed())
   {
      unitList = getVar(cUnitList)->asUnitList()->readVar();
   }

   if (getVar(cUnit)->isUsed())
   {
      unitList.uniqueAdd(getVar(cUnit)->asUnit()->readVar());
   }
   
   // Test units
   uint numUnits = unitList.getSize();
   for (uint i = 0; i < numUnits; i++)
   {
      BUnit* pUnit = gWorld->getUnit(unitList[i]);
      // If the unit is alive and not downed
      if (pUnit && pUnit->isAlive() && !pUnit->getFlagDown() && !pUnit->isHibernating())
      {
         return (false);
      }
   }

   // Get squads
   BEntityIDArray squadIDs;
   if (getVar(cSquadList)->isUsed())
   {
      squadIDs = getVar(cSquadList)->asSquadList()->readVar();
   }

   if (getVar(cSquad)->isUsed())
   {
      squadIDs.uniqueAdd(getVar(cSquad)->asSquad()->readVar());
   }

   // Test squads
   uint numSquads = squadIDs.getSize();
   for (uint i = 0; i < numSquads; i++)
   {
//-- FIXING PREFIX BUG ID 3849
      const BSquad* pSquad = gWorld->getSquad(squadIDs[i]);
//--
      // If the squad is alive and not downed and not hibernating
      if (pSquad && pSquad->isAlive() && !pSquad->isDown() && !pSquad->isHibernating())
      {
         return (false);
      }
   }

   // No alive units or squads.
   return (true);
}


//==============================================================================
// BTriggerCondition::tcUILocationOK()
//==============================================================================
bool BTriggerCondition::tcUILocationOK()
{
   enum { cInputUILocation = 1, cOutputLocation = 2, };
   BTriggerVarUILocation *pVar = getVar(cInputUILocation)->asUILocation();
   if (pVar->readResult() == BTriggerVarUILocation::cUILocationResultOK)
   {
      getVar(cOutputLocation)->asVector()->writeVar(pVar->readLocation());
      return (true);
   }
   else
      return (false);
}


//==============================================================================
// BTriggerCondition::tcUILocationCancel()
//==============================================================================
bool BTriggerCondition::tcUILocationCancel()
{
   enum { cInputUILocation = 1, };
   BTriggerVarUILocation *pVar = getVar(cInputUILocation)->asUILocation();
   if (pVar->readResult() == BTriggerVarUILocation::cUILocationResultCancel)
      return (true);
   else
      return (false);
}


//==============================================================================
// BTriggerCondition::tcCompareCount()
//==============================================================================
bool BTriggerCondition::tcCompareCount()
{
   enum { cInputFirstCount = 1, cInputCompareType = 2, cInputSecondCount = 3, };
   long firstCount = getVar(cInputFirstCount)->asInteger()->readVar();
   long compareType = getVar(cInputCompareType)->asOperator()->readVar();
   long secondCount = getVar(cInputSecondCount)->asInteger()->readVar();

   bool result = BTriggerCondition::compareValues(firstCount, compareType, secondCount);
   return (result);
}


//==============================================================================
// BTriggerCondition::tcCanPayCost()
//==============================================================================
bool BTriggerCondition::tcCanPayCost()
{
   enum { cInputPlayer = 1, cInputCost = 2, };
   long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   BCost cost = getVar(cInputCost)->asCost()->readVar();

   BPlayer *pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return (false);

   bool result = pPlayer->checkCost(&cost);
   return (result);
}


//==============================================================================
// BTriggerCondition::tcCheckPlacement()
//==============================================================================
bool BTriggerCondition::tcCheckPlacement()
{
   switch(getVersion())
   {
      case 5:
         return (tcCheckPlacementV5());
         break;

      case 6:
         return (tcCheckPlacementV6());
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         return (false);
         break;
   }
}

//==============================================================================
// Check the placement of a point or object
//==============================================================================
bool BTriggerCondition::tcCheckPlacementV5()
{
   enum 
   { 
      cInputPlayer = 1, 
      cInputLocation = 2, 
      cInputProtoObject = 3, 
      cInputCheckObstruction = 4, 
      cLOSType = 5, 
      cSearchScale = 6,
      cOutSuggestion = 7,
      cInputLOSCenterOnly = 8,
      cInputProtoSquad = 9,
   };

   long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   BVector location = getVar(cInputLocation)->asVector()->readVar();
   bool protoObjectUsed = getVar(cInputProtoObject)->isUsed();
   BProtoObjectID protoObjectType = protoObjectUsed ? getVar(cInputProtoObject)->asProtoObject()->readVar() : cInvalidProtoObjectID;
   bool protoSquadUsed = getVar(cInputProtoSquad)->isUsed();
   BProtoSquadID protoSquadType = protoSquadUsed ? getVar(cInputProtoSquad)->asProtoSquad()->readVar() : cInvalidProtoSquadID;
   bool useCheckObstruction = getVar(cInputCheckObstruction)->isUsed();
   bool checkObstruction = useCheckObstruction ? getVar(cInputCheckObstruction)->asBool()->readVar() : false;
   bool useLOSType = getVar(cLOSType)->isUsed();
   long losType = useLOSType ? getVar(cLOSType)->asLOSType()->readVar() : BWorld::cCPLOSDontCare;
   long searchScale = getVar(cSearchScale)->isUsed() ? getVar(cSearchScale)->asInteger()->readVar() : 1;
   bool losCenterOnly = getVar(cInputLOSCenterOnly)->isUsed() ? getVar(cInputLOSCenterOnly)->asBool()->readVar() : false;

   DWORD flags = 0;
   if (!protoObjectUsed && !protoSquadUsed)
   {
      flags = BWorld::cCPIgnorePlacementRules;
   }
   if (checkObstruction)
   {
      flags |= (BWorld::cCPCheckObstructions | BWorld::cCPIgnoreMoving | BWorld::cCPExpandHull | BWorld::cCPPushOffTerrain);
   }
   if (losCenterOnly)
   {
      flags |= BWorld::cCPLOSCenterOnly;
   }

   bool useSuggestion = getVar(cOutSuggestion)->isUsed();
   if (useSuggestion)
   {
      flags |= (BWorld::cCPSetPlacementSuggestion | BWorld::cCPIgnoreParkingAndFlatten);
   }

   BVector placementSuggestion = cInvalidVector;
   bool checkPlacementResult = gWorld->checkPlacement(protoObjectType, playerID, location, placementSuggestion, cZAxisVector, losType, flags, searchScale, NULL, NULL, -1, protoSquadType, protoSquadUsed);

   if (useSuggestion)
   {
      getVar(cOutSuggestion)->asVector()->writeVar(placementSuggestion);
   }

   return (checkPlacementResult);
}

//==============================================================================
// Check the placement of a point or object
//==============================================================================
bool BTriggerCondition::tcCheckPlacementV6()
{
   enum 
   { 
      cInputPlayer = 1, 
      cInputLocation = 2, 
      cInputProtoObject = 3, 
      cInputCheckObstruction = 4, 
      cLOSType = 5, 
      cSearchScale = 6,
      cOutSuggestion = 7,
      cInputLOSCenterOnly = 8,
      cInputProtoSquad = 9,
      cInputCheckUnitObstructions = 10,
      cInputCheckSquadObstructions = 11,
   };

   long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   BVector location = getVar(cInputLocation)->asVector()->readVar();
   bool protoObjectUsed = getVar(cInputProtoObject)->isUsed();
   BProtoObjectID protoObjectType = protoObjectUsed ? getVar(cInputProtoObject)->asProtoObject()->readVar() : cInvalidProtoObjectID;
   bool protoSquadUsed = getVar(cInputProtoSquad)->isUsed();
   BProtoSquadID protoSquadType = protoSquadUsed ? getVar(cInputProtoSquad)->asProtoSquad()->readVar() : cInvalidProtoSquadID;
   bool useCheckObstruction = getVar(cInputCheckObstruction)->isUsed();
   bool checkObstruction = useCheckObstruction ? getVar(cInputCheckObstruction)->asBool()->readVar() : false;
   bool useLOSType = getVar(cLOSType)->isUsed();
   long losType = useLOSType ? getVar(cLOSType)->asLOSType()->readVar() : BWorld::cCPLOSDontCare;
   long searchScale = getVar(cSearchScale)->isUsed() ? getVar(cSearchScale)->asInteger()->readVar() : 1;
   bool losCenterOnly = getVar(cInputLOSCenterOnly)->isUsed() ? getVar(cInputLOSCenterOnly)->asBool()->readVar() : false;
   bool checkUnitObstructions = getVar(cInputCheckUnitObstructions)->isUsed() ? getVar(cInputCheckUnitObstructions)->asBool()->readVar() : false;
   bool checkSquadObstructions = getVar(cInputCheckSquadObstructions)->isUsed() ? getVar(cInputCheckSquadObstructions)->asBool()->readVar() : false;

   DWORD flags = 0;
   if (!protoObjectUsed && !protoSquadUsed)
   {
      flags = BWorld::cCPIgnorePlacementRules;
   }
   if (checkObstruction)
   {
      flags |= (BWorld::cCPCheckObstructions | BWorld::cCPIgnoreMoving | BWorld::cCPExpandHull | BWorld::cCPPushOffTerrain);
   }
   if (losCenterOnly)
   {
      flags |= BWorld::cCPLOSCenterOnly;
   }
   if (checkUnitObstructions)
   {
      flags &= ~BWorld::cCPIgnoreMoving;
   }
   if (checkSquadObstructions)
   {
      flags |= BWorld::cCPCheckSquadObstructions;
   }


   bool useSuggestion = getVar(cOutSuggestion)->isUsed();
   if (useSuggestion)
   {
      flags |= (BWorld::cCPSetPlacementSuggestion | BWorld::cCPIgnoreParkingAndFlatten);
   }

   BVector placementSuggestion = cInvalidVector;
   bool checkPlacementResult = gWorld->checkPlacement(protoObjectType, playerID, location, placementSuggestion, cZAxisVector, losType, flags, searchScale, NULL, NULL, -1, protoSquadType, protoSquadUsed);

   if (useSuggestion)
   {
      getVar(cOutSuggestion)->asVector()->writeVar(placementSuggestion);
   }

   return (checkPlacementResult);
}

//==============================================================================
// BTriggerCondition::tcCanUsePower()
//==============================================================================
bool BTriggerCondition::tcCanUsePower()
{
   enum { cInputPlayer = 1, cInputPower = 2, };
   long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
//-- FIXING PREFIX BUG ID 3852
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
//--
   if (!pPlayer)
      return (false);

   long protoPowerID = getVar(cInputPower)->asPower()->readVar();
   return (pPlayer->canUsePower(protoPowerID));
}

//XXXHalwes - 7/17/2007 - This has been tagged do not use, do we need to implement/obsolete this?
//==============================================================================
// BTriggerCondition::tcIsUnitPower()
//==============================================================================
bool BTriggerCondition::tcIsUnitPower()
{
   enum { cInputPower = 1, };
   long protoPowerID = getVar(cInputPower)->asPower()->readVar();
   BProtoPower*  pProtoPower = gDatabase.getProtoPowerByID(protoPowerID);
   if (!pProtoPower)
      return (false);

   bool isUnitPower = pProtoPower->getFlagUnitPower();
   return (isUnitPower);
}


//==============================================================================
// BTriggerCondition::tcIsOwnedBy()
//==============================================================================
bool BTriggerCondition::tcIsOwnedBy()
{
   bool result = false;
   switch (getVersion())
   {
      case 3:
         result = tcIsOwnedByV3();
         break;

      case 4:
         result = tcIsOwnedByV4();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }

   return (result);
}


//============================================================================================
// Test to see if the Unit, UnitList, Squad, and/or SquadList is owned by a particular player
//============================================================================================
bool BTriggerCondition::tcIsOwnedByV3()
{
   enum { cPlayer = 2, cUnit = 4, cUnitList = 5, cSquad = 6, cSquadList = 7, };
   long playerID = getVar(cPlayer)->asPlayer()->readVar();

   bool useUnit = getVar(cUnit)->isUsed();
   if (useUnit)
   {
      BEntityID unitID = getVar(cUnit)->asUnit()->readVar();
      BUnit *pUnit = gWorld->getUnit(unitID);
      if (!pUnit || pUnit->getPlayerID() != playerID)
         return (false);
   }

   bool useSquad = getVar(cSquad)->isUsed();
   if (useSquad)
   {
      BEntityID squadID = getVar(cSquad)->asSquad()->readVar();
      BSquad *pSquad = gWorld->getSquad(squadID);
      if (!pSquad || pSquad->getPlayerID() != playerID)
         return (false);
   }

   bool useUnitList = getVar(cUnitList)->isUsed();
   if (useUnitList)
   {
      const BEntityIDArray& unitList = getVar(cUnitList)->asUnitList()->readVar();
      long numUnits = unitList.getNumber();
      for (long i=0; i<numUnits; i++)
      {
         BEntityID unitID = unitList[i];
         BUnit *pUnit = gWorld->getUnit(unitID);
         if (!pUnit || pUnit->getPlayerID() != playerID)
            return (false);
      }
   }

   bool useSquadList = getVar(cSquadList)->isUsed();
   if (useSquadList)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      long numSquads = squadList.getNumber();
      for (long i=0; i<numSquads; i++)
      {
         BEntityID squadID = squadList[i];
         BSquad *pSquad = gWorld->getSquad(squadID);
         if (!pSquad || pSquad->getPlayerID() != playerID)
            return (false);
      }
   }

   return (true);
}

//====================================================================================================
// Test to see if the Unit, UnitList, Squad, and/or SquadList is owned by a particular player or team
//====================================================================================================
bool BTriggerCondition::tcIsOwnedByV4()
{
   enum 
   { 
      cPlayer = 2, 
      cUnit = 4, 
      cUnitList = 5, 
      cSquad = 6, 
      cSquadList = 7, 
      cTeam = 8,
   };

   bool usePlayer = getVar(cPlayer)->isUsed();
   bool useTeam = getVar(cTeam)->isUsed();
   BTRIGGER_ASSERTM(usePlayer || useTeam, "No Player or Team designated!");
   BPlayerID playerID = usePlayer ? getVar(cPlayer)->asPlayer()->readVar() : -1;
   BTeamID teamID = useTeam ? getVar(cTeam)->asTeam()->readVar() : -1;

   // Collect entities
   BEntityIDArray entityList;           
   if (getVar(cUnitList)->isUsed())
   {
      entityList = getVar(cUnitList)->asUnitList()->readVar();
   }

   if (getVar(cUnit)->isUsed())
   {
      entityList.add(getVar(cUnit)->asUnit()->readVar());
   }
   
   if (getVar(cSquadList)->isUsed())
   {
      entityList.append(getVar(cSquadList)->asSquadList()->readVar());
   }

   if (getVar(cSquad)->isUsed())
   {
      entityList.add(getVar(cSquad)->asSquad()->readVar());
   }

   uint numEntities = entityList.getSize();
   for (uint i = 0; i < numEntities; i++)
   {
//-- FIXING PREFIX BUG ID 3858
      const BEntity* pEntity = gWorld->getEntity(entityList[i]);
//--
      if (pEntity)
      {
         // If either the player ID or the team ID does not match
         if ((!usePlayer || (pEntity->getPlayerID() != playerID)) && (!useTeam || (pEntity->getTeamID() != teamID)))
         {
            return (false);
         }         
      }
      else
      {
         return (false);
      }
   }

   return (true);
}

//==============================================================================
// BTriggerCondition::tcIsProtoObject()
//==============================================================================
bool BTriggerCondition::tcIsProtoObject()
{
   uint version = getVersion();
   if (version == 2)
   {
      return (tcIsProtoObjectV2());
   }
   else
   {
      BTRIGGER_ASSERT(false);
      return (false);
   }
}


//==============================================================================
// BTriggerCondition::tcIsProtoObjectV2()
//==============================================================================
bool BTriggerCondition::tcIsProtoObjectV2()
{
   enum { cUnit = 3, cProtoObject = 2, };
   BEntityID unitID = getVar(cUnit)->asUnit()->readVar();
   long protoObjectID = getVar(cProtoObject)->asProtoObject()->readVar();
   BUnit *pUnit = gWorld->getUnit(unitID);
   if (!pUnit)
      return (false);
   return (pUnit->getProtoID() == protoObjectID);
}


//==============================================================================
// BTriggerCondition::tcNextPlayer()
//==============================================================================
bool BTriggerCondition::tcNextPlayer()
{
   enum { cInputIterator = 1, cOutputNextPlayer = 2, };

   // Get our list var, and assert that it exists (or we're using an uninitialized iterator)
   BTriggerVarIterator *pIterator = getVar(cInputIterator)->asIterator();
   BTRIGGER_ASSERT(pIterator);
   BTriggerVarID listVarID = pIterator->getListVarID();
   BTriggerVar *pListVar = getParentTriggerScript()->getTriggerVar(listVarID);
   BTRIGGER_ASSERT(pListVar);
   const BPlayerIDArray& playerList = pListVar->asPlayerList()->readVar();

   long numPlayers = playerList.getNumber();
   for (long i=0; i<numPlayers; i++)
   {
      if (!pIterator->isPlayerVisited(playerList[i]))
      {
         pIterator->visitPlayer(playerList[i]);
         getVar(cOutputNextPlayer)->asPlayer()->writeVar(playerList[i]);
         return (true);
      }
   }

   return (false);
}


//==============================================================================
// BTriggerCondition::tcNextTeam()
//==============================================================================
bool BTriggerCondition::tcNextTeam()
{
   enum { cInputIterator = 1, cOutputNextTeam = 2, };

   // Get our list var, and assert that it exists (or we're using an uninitialized iterator)
   BTriggerVarIterator *pIterator = getVar(cInputIterator)->asIterator();
   BTRIGGER_ASSERT(pIterator);
   BTriggerVarID listVarID = pIterator->getListVarID();
   BTriggerVar *pListVar = getParentTriggerScript()->getTriggerVar(listVarID);
   BTRIGGER_ASSERT(pListVar);
   const BTeamIDArray& teamList = pListVar->asTeamList()->readVar();

   long numTeams = teamList.getNumber();
   for (long i=0; i<numTeams; i++)
   {
      if (!pIterator->isTeamVisited(teamList[i]))
      {
         pIterator->visitTeam(teamList[i]);
         getVar(cOutputNextTeam)->asTeam()->writeVar(teamList[i]);
         return (true);
      }
   }

   return (false);
}


//==============================================================================
// BTriggerCondition::tcUnitListLocationDistance
//==============================================================================
bool BTriggerCondition::tcUnitListLocationDistance()
{
   uint version = getVersion();
   if (version == 2)
   {
      return (tcUnitListLocationDistanceV2());
   }
   else
   {
      BTRIGGER_ASSERT(false);
      return (false);
   }
}


//==============================================================================
// Compare a list of units to a location distances
//==============================================================================
bool BTriggerCondition::tcUnitListLocationDistanceV2()
{
   enum 
   { 
      cUnitList = 1, 
      cLocation = 2, 
      cCompare = 3, 
      cDistance = 4, 
      cTestAll = 5, 
   };

   const BEntityIDArray& unitList = getVar(cUnitList)->asUnitList()->readVar();
   BVector comparePos = getVar(cLocation)->asVector()->readVar();
   bool testAll = getVar(cTestAll)->asBool()->readVar();
   long op = getVar(cCompare)->asOperator()->readVar();
   float compareDist = getVar(cDistance)->asFloat()->readVar();

   uint numUnits = (uint)unitList.getNumber();
   for (uint i = 0; i < numUnits; i++)
   {
//-- FIXING PREFIX BUG ID 3854
      const BUnit* pUnit = gWorld->getUnit(unitList[i]);
//--
      if (pUnit)
      {
         float dist = pUnit->BEntity::calculateXZDistance(comparePos);
         bool result = compareValues(dist, op, compareDist);
         if (testAll && !result)
            return (false);
         if (!testAll && result)
            return (true);
      }
   }

   // Return the default case.
   return (testAll);
}


//==============================================================================
// Select correct version
//==============================================================================
bool BTriggerCondition::tcPlayerInState()
{
   bool result = false;
   switch (getVersion())
   {
      case 2:
         result = tcPlayerInStateV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }

   return (result);
}

//==============================================================================
// Check to see if a player is in a particular state
//==============================================================================
bool BTriggerCondition::tcPlayerInStateV2()
{
   enum 
   { 
      cPlayer = 1, 
      cPlayerState = 3, 
   };

   BPlayerID playerID = getVar(cPlayer)->asPlayer()->readVar();
   BPlayerState playerState = getVar(cPlayerState)->asPlayerState()->readVar();

//-- FIXING PREFIX BUG ID 3861
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
//--
   if (pPlayer)
      return (pPlayer->getPlayerState() == playerState);
   else
      return (false);
}

//==============================================================================
// BTriggerCondition::tcContainsGarrisoned()
//==============================================================================
bool BTriggerCondition::tcContainsGarrisoned()
{
   uint version = getVersion();
   if (version == 3)
   {
      return (tcContainsGarrisonedV3());
   }
   else
   {
      BTRIGGER_ASSERT(false);
      return (false);
   }
}


//==============================================================================
// BTriggerCondition::tcContainsGarrisonedV3()
//==============================================================================
bool BTriggerCondition::tcContainsGarrisonedV3()
{
   enum { cUnit = 1, cPlayer = 2, cObjectType = 3, };

   BUnit *pContainerUnit = gWorld->getUnit(getVar(cUnit)->asUnit()->readVar());
   if (!pContainerUnit)
      return (false);

   bool usePlayer = getVar(cPlayer)->isUsed();
   long playerID = usePlayer ? getVar(cPlayer)->asPlayer()->readVar() : -1;
   bool useObjectType = getVar(cObjectType)->isUsed();
   long objectTypeID = useObjectType ? getVar(cObjectType)->asObjectType()->readVar() : -1;

   long numEntityRefs = pContainerUnit->getNumberEntityRefs();
   for (long i=0; i<numEntityRefs; i++)
   {
      BEntityRef *pEntityRef = pContainerUnit->getEntityRefByIndex(i);
      if (pEntityRef && pEntityRef->mType == BEntityRef::cTypeContainUnit)
      {
         BUnit *pUnit = gWorld->getUnit(pEntityRef->mID);
         if (!pUnit)
            continue;
         if (usePlayer && pUnit->getPlayerID() != playerID)
            continue;
         if (useObjectType && !pUnit->isType(objectTypeID))
            continue;
         // Passed
         return (true);
      }
   }

   return (false);
}


//==============================================================================
// BTriggerCondition::tcCompareBool
//==============================================================================
bool BTriggerCondition::tcCompareBool()
{
   enum { cFirstBool = 1, cCompare = 2, cSecondBool = 3, };
   bool firstBool = getVar(cFirstBool)->asBool()->readVar();
   long op = getVar(cCompare)->asOperator()->readVar();
   bool secondBool = getVar(cSecondBool)->asBool()->readVar();
   bool result = compareValues(firstBool, op, secondBool);
   return (result);
}


//==============================================================================
// BTriggerCondition::tcNextUnit
//==============================================================================
bool BTriggerCondition::tcNextUnit()
{
   enum { cIterator = 1, cOutputNextUnit = 2, };

   // Get our list var, and assert that it exists (or we're using an uninitialized iterator)
   BTriggerVarIterator *pIterator = getVar(cIterator)->asIterator();
   BTRIGGER_ASSERT(pIterator);
   BTriggerVarID listVarID = pIterator->getListVarID();
   BTriggerVar *pListVar = getParentTriggerScript()->getTriggerVar(listVarID);
   BTRIGGER_ASSERT(pListVar);
   const BEntityIDArray& unitList = pListVar->asUnitList()->readVar();

   long numUnits = unitList.getNumber();
   for (long i=0; i<numUnits; i++)
   {
      if (!pIterator->isUnitVisited(unitList[i]))
      {
         pIterator->visitUnit(unitList[i]);
         getVar(cOutputNextUnit)->asUnit()->writeVar(unitList[i]);
         return (true);
      }
   }

   return (false);
}


//==============================================================================
// BTriggerCondition::tcNextSquad
//==============================================================================
bool BTriggerCondition::tcNextSquad()
{
   enum { cIterator = 1, cOutputNextSquad = 2, };

   // Get our list var, and assert that it exists (or we're using an uninitialized iterator)
   BTriggerVarIterator *pIterator = getVar(cIterator)->asIterator();
   BTRIGGER_ASSERT(pIterator);
   BTriggerVarID listVarID = pIterator->getListVarID();
   BTriggerVar *pListVar = getParentTriggerScript()->getTriggerVar(listVarID);
   BTRIGGER_ASSERT(pListVar);
   const BEntityIDArray& squadList = pListVar->asSquadList()->readVar();

   long numSquads = squadList.getNumber();
   for (long i=0; i<numSquads; i++)
   {
      if (!pIterator->isSquadVisited(squadList[i]))
      {
         pIterator->visitSquad(squadList[i]);
         getVar(cOutputNextSquad)->asSquad()->writeVar(squadList[i]);
         return (true);
      }
   }

   return (false);
}


//==============================================================================
// BTriggerCondition::tcUIUnitOK
//==============================================================================
bool BTriggerCondition::tcUIUnitOK()
{
   enum { cInputUIUnit = 1, cOutputUnit = 2, };
   BTriggerVarUIUnit *pVar = getVar(cInputUIUnit)->asUIUnit();
   if (pVar->readResult() == BTriggerVarUIUnit::cUIUnitResultOK)
   {
      getVar(cOutputUnit)->asUnit()->writeVar(pVar->readUnit());
      return (true);
   }
   else
      return (false);
}


//==============================================================================
// BTriggerCondition::tcUIUnitCancel
//==============================================================================
bool BTriggerCondition::tcUIUnitCancel()
{
   enum { cInputUIUnit = 1, };
   BTriggerVarUIUnit *pVar = getVar(cInputUIUnit)->asUIUnit();
   if (pVar->readResult() == BTriggerVarUIUnit::cUIUnitResultCancel)
      return (true);
   else
      return (false);
}


//==============================================================================
// BTriggerCondition::tcUISquadOK
//==============================================================================
bool BTriggerCondition::tcUISquadOK()
{
   enum { cInputUISquad = 1, cOutputSquad = 2, };
   BTriggerVarUISquad *pVar = getVar(cInputUISquad)->asUISquad();
   if (pVar->readResult() == BTriggerVarUISquad::cUISquadResultOK)
   {
      getVar(cOutputSquad)->asSquad()->writeVar(pVar->readSquad());
      return (true);
   }
   else
      return (false);
}


//==============================================================================
// BTriggerCondition::tcUISquadCancel
//==============================================================================
bool BTriggerCondition::tcUISquadCancel()
{
   enum { cInputUISquad = 1, };
   BTriggerVarUISquad *pVar = getVar(cInputUISquad)->asUISquad();
   if (pVar->readResult() == BTriggerVarUISquad::cUISquadResultCancel)
      return (true);
   else
      return (false);
}

//==============================================================================
// BTriggerCondition::tcCanGetOneUnit
//==============================================================================
bool BTriggerCondition::tcCanGetOneUnit()
{
   enum 
   { 
      cInputUnitList = 1, 
      cInputListPos  = 2,
      cOutputUnit    = 3,
   };

   BEntityIDArray unitList;
   long           listPos = getVar(cInputListPos)->asListPosition()->readVar();
   BUnit*         pUnit   = NULL;

   // Get the unit
   switch(listPos)
   {
      // First
      case BTriggerVarListPosition::cListPosTypeFirst:
      {
         unitList = getVar(cInputUnitList)->asUnitList()->readVar();
         if (unitList.getNumber() > 0)
         {
            pUnit = gWorld->getUnit(unitList[0]);
         }
         break;
      }

      // Last
      case BTriggerVarListPosition::cListPosTypeLast:
      {
         unitList = getVar(cInputUnitList)->asUnitList()->readVar();

         long numUnits = unitList.getNumber();
         if (numUnits > 0)
         {
            pUnit = gWorld->getUnit(unitList[numUnits - 1]);
         }
         break;
      }

      // Random
      case BTriggerVarListPosition::cListPosTypeRandom:
      {
         unitList = getVar(cInputUnitList)->asUnitList()->readVar();
         uint numUnits = unitList.getSize();
         if (numUnits > 0)
         {
            uint randomIndex = getRandRange(cSimRand, 0, (numUnits-1));
            pUnit = gWorld->getUnit(unitList[randomIndex]);
         }
         break;
      }
   }

   // Output the unit
   if (pUnit)
   {
      getVar(cOutputUnit)->asUnit()->writeVar(pUnit->getID());
      return (true);
   }
   else
   {
      getVar(cOutputUnit)->asUnit()->writeVar(cInvalidObjectID);
      return (false);
   }
}

//==============================================================================
// BTriggerCondition::tcCanGetOneSquad
//==============================================================================
bool BTriggerCondition::tcCanGetOneSquad()
{
   enum 
   { 
      cInputSquadList = 1,
      cInputListPos   = 2,
      cOutputSquad    = 3,
   };
   
   BEntityIDArray squadList;
   long           listPos = getVar(cInputListPos)->asListPosition()->readVar();
   BSquad*        pSquad  = NULL;

   // Get the squad
   switch(listPos)
   {
      // First
      case BTriggerVarListPosition::cListPosTypeFirst:
      {
         squadList = getVar(cInputSquadList)->asSquadList()->readVar();
         if (squadList.getNumber() > 0)
         {
            pSquad = gWorld->getSquad(squadList[0]);
         }
         break;
      }

      // Last
      case BTriggerVarListPosition::cListPosTypeLast:
      {
         squadList = getVar(cInputSquadList)->asSquadList()->readVar();

         long numSquads = squadList.getNumber();
         if (numSquads > 0)
         {
            pSquad = gWorld->getSquad(squadList[numSquads - 1]);
         }
         break;
      }

      // Random
      case BTriggerVarListPosition::cListPosTypeRandom:
      {
         squadList = getVar(cInputSquadList)->asSquadList()->readVar();
         uint numSquads = squadList.getSize();
         if (numSquads > 0)
         {
            uint randomIndex = getRandRange(cSimRand, 0, (numSquads-1));
            pSquad = gWorld->getSquad(squadList[randomIndex]);
         }
         break;
      }
   }

   // Output the squad
   if (pSquad)
   {
      getVar(cOutputSquad)->asSquad()->writeVar(pSquad->getID());
      return (true);
   }
   else
   {
      getVar(cOutputSquad)->asSquad()->writeVar(cInvalidObjectID);
      return (false);
   }
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCompareLeader()
{
   enum { cFirstLeader = 1, cCompareType = 2, cSecondLeader = 3, };
   BLeaderID firstLeader = getVar(cFirstLeader)->asLeader()->readVar();
   long compareType = getVar(cCompareType)->asOperator()->readVar();
   BLeaderID secondLeader = getVar(cSecondLeader)->asLeader()->readVar();
   BTRIGGER_ASSERT(compareType == Math::cEqualTo || compareType == Math::cNotEqualTo);
   bool result = BTriggerCondition::compareValues(firstLeader, compareType, secondLeader);
   return (result);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcPlayerUsingLeader()
{
   enum { cPlayer = 1, cLeader = 2, };
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
   if (!pPlayer)
      return (false);

   BLeaderID leaderID = getVar(cLeader)->asLeader()->readVar();
   if (pPlayer->getLeaderID() == leaderID)
      return(true);
   else
      return (false);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCanGetOneProtoObject()
{
   enum { cProtoObjectList = 1, cListPosition = 2, cFoundProtoObject = 4, };
   const BProtoObjectIDArray& protoObjectList = getVar(cProtoObjectList)->asProtoObjectList()->readVar();
   uint listSize = protoObjectList.getSize();
   if (listSize == 0)
      return (false);

   long listPos = getVar(cListPosition)->asListPosition()->readVar();
   switch (listPos)
   {
   case BTriggerVarListPosition::cListPosTypeFirst:
      {
         getVar(cFoundProtoObject)->asProtoObject()->writeVar(protoObjectList[0]);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeLast:
      {
         getVar(cFoundProtoObject)->asProtoObject()->writeVar(protoObjectList[listSize - 1]);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeRandom:
      {
         uint randomIndex = getRandRange(cSimRand, 0, (listSize-1));
         getVar(cFoundProtoObject)->asProtoObject()->writeVar(protoObjectList[randomIndex]);
         return (true);
      }
   }

   return (false);  
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCanGetOneProtoSquad()
{
   enum { cProtoSquadList = 1, cListPosition = 2, cFoundProtoSquad = 3, };
   const BProtoSquadIDArray& protoSquadList = getVar(cProtoSquadList)->asProtoSquadList()->readVar();
   uint listSize = protoSquadList.getSize();
   if (listSize == 0)
      return (false);

   long listPos = getVar(cListPosition)->asListPosition()->readVar();
   switch (listPos)
   {
   case BTriggerVarListPosition::cListPosTypeFirst:
      {
         getVar(cFoundProtoSquad)->asProtoSquad()->writeVar(protoSquadList[0]);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeLast:
      {
         getVar(cFoundProtoSquad)->asProtoSquad()->writeVar(protoSquadList[listSize - 1]);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeRandom:
      {
         uint randomIndex = getRandRange(cSimRand, 0, (listSize-1));
         getVar(cFoundProtoSquad)->asProtoSquad()->writeVar(protoSquadList[randomIndex]);
         return (true);
      }
   }

   return (false);  
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCanGetOneObjectType()
{
   enum { cObjectTypeList = 1, cListPosition = 2, cFoundObjectType = 3, };
   const BObjectTypeIDArray& objectTypeList = getVar(cObjectTypeList)->asObjectTypeList()->readVar();
   uint listSize = objectTypeList.getSize();
   if (listSize == 0)
      return (false);

   long listPos = getVar(cListPosition)->asListPosition()->readVar();
   switch (listPos)
   {
   case BTriggerVarListPosition::cListPosTypeFirst:
      {
         getVar(cFoundObjectType)->asObjectType()->writeVar(objectTypeList[0]);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeLast:
      {
         getVar(cFoundObjectType)->asObjectType()->writeVar(objectTypeList[listSize - 1]);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeRandom:
      {
         uint randomIndex = getRandRange(cSimRand, 0, (listSize-1));
         getVar(cFoundObjectType)->asObjectType()->writeVar(objectTypeList[randomIndex]);
         return (true);
      }
   }

   return (false);  
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCanGetOneTech()
{
   enum { cTechList = 1, cListPosition = 2, cFoundTech = 3, };
   const BTechIDArray& techList = getVar(cTechList)->asTechList()->readVar();
   uint listSize = techList.getSize();
   if (listSize == 0)
      return (false);

   long listPos = getVar(cListPosition)->asListPosition()->readVar();
   switch (listPos)
   {
   case BTriggerVarListPosition::cListPosTypeFirst:
      {
         getVar(cFoundTech)->asTech()->writeVar(techList[0]);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeLast:
      {
         getVar(cFoundTech)->asTech()->writeVar(techList[listSize - 1]);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeRandom:
      {
         uint randomIndex = getRandRange(cSimRand, 0, (listSize-1));
         getVar(cFoundTech)->asTech()->writeVar(techList[randomIndex]);
         return (true);
      }
   }

   return (false);  
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcNextProtoObject()
{
   enum { cIterator = 1, cNextProtoObject = 2, };

   // Get our list var, and assert that it exists (or we're using an uninitialized iterator)
   BTriggerVarIterator *pIterator = getVar(cIterator)->asIterator();
   BTRIGGER_ASSERT(pIterator);
   BTriggerVarID listVarID = pIterator->getListVarID();
   BTriggerVar *pListVar = getParentTriggerScript()->getTriggerVar(listVarID);
   BTRIGGER_ASSERT(pListVar);
   const BProtoObjectIDArray& protoObjectList = pListVar->asProtoObjectList()->readVar();

   uint numProtoObjects = protoObjectList.getSize();
   for (uint i=0; i<numProtoObjects; i++)
   {
      if (!pIterator->isProtoObjectVisited(protoObjectList[i]))
      {
         pIterator->visitProtoObject(protoObjectList[i]);
         getVar(cNextProtoObject)->asProtoObject()->writeVar(protoObjectList[i]);
         return (true);
      }
   }

   return (false);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcNextProtoSquad()
{
   enum { cIterator = 1, cNextProtoSquad = 2, };

   // Get our list var, and assert that it exists (or we're using an uninitialized iterator)
   BTriggerVarIterator *pIterator = getVar(cIterator)->asIterator();
   BTRIGGER_ASSERT(pIterator);
   BTriggerVarID listVarID = pIterator->getListVarID();
   BTriggerVar *pListVar = getParentTriggerScript()->getTriggerVar(listVarID);
   BTRIGGER_ASSERT(pListVar);
   const BProtoSquadIDArray& protoSquadList = pListVar->asProtoSquadList()->readVar();

   uint numProtoSquads = protoSquadList.getSize();
   for (uint i=0; i<numProtoSquads; i++)
   {
      if (!pIterator->isProtoSquadVisited(protoSquadList[i]))
      {
         pIterator->visitProtoSquad(protoSquadList[i]);
         getVar(cNextProtoSquad)->asProtoSquad()->writeVar(protoSquadList[i]);
         return (true);
      }
   }

   return (false);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcNextObjectType()
{
   enum { cIterator = 1, cNextObjectType = 2, };

   // Get our list var, and assert that it exists (or we're using an uninitialized iterator)
   BTriggerVarIterator *pIterator = getVar(cIterator)->asIterator();
   BTRIGGER_ASSERT(pIterator);
   BTriggerVarID listVarID = pIterator->getListVarID();
   BTriggerVar *pListVar = getParentTriggerScript()->getTriggerVar(listVarID);
   BTRIGGER_ASSERT(pListVar);
   const BObjectTypeIDArray& objectTypeList = pListVar->asObjectTypeList()->readVar();

   uint numObjectTypes = objectTypeList.getSize();
   for (uint i=0; i<numObjectTypes; i++)
   {
      if (!pIterator->isObjectTypeVisited(objectTypeList[i]))
      {
         pIterator->visitObjectType(objectTypeList[i]);
         getVar(cNextObjectType)->asObjectType()->writeVar(objectTypeList[i]);
         return (true);
      }
   }

   return (false);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcNextTech()
{
   enum { cIterator = 1, cNextTech = 2, };

   // Get our list var, and assert that it exists (or we're using an uninitialized iterator)
   BTriggerVarIterator *pIterator = getVar(cIterator)->asIterator();
   BTRIGGER_ASSERT(pIterator);
   BTriggerVarID listVarID = pIterator->getListVarID();
   BTriggerVar *pListVar = getParentTriggerScript()->getTriggerVar(listVarID);
   BTRIGGER_ASSERT(pListVar);
   const BTechIDArray& techList = pListVar->asTechList()->readVar();

   uint numTechs = techList.getSize();
   for (uint i=0; i<numTechs; i++)
   {
      if (!pIterator->isTechVisited(techList[i]))
      {
         pIterator->visitTech(techList[i]);
         getVar(cNextTech)->asTech()->writeVar(techList[i]);
         return (true);
      }
   }

   return (false);
}



//==============================================================================
// BTriggerCondition::tcPlayerSelectingUnit()
//==============================================================================
bool BTriggerCondition::tcPlayerSelectingUnit()
{
   uint version = getVersion();
   if (version == 2)
   {
      return (tcPlayerSelectingUnitV2());
   }
   else
   {
      BTRIGGER_ASSERT(false);
      return (false);
   }
}


//==============================================================================
// BTriggerCondition::tcPlayerSelectingUnitV2()
//==============================================================================
bool BTriggerCondition::tcPlayerSelectingUnitV2()
{
   enum { cInputPlayer = 1, cInputUnit = 2, };
   BTRIGGER_ASSERT(mbAsyncCondition);
   BTRIGGER_ASSERT(mbLocalAsyncMachine);
   long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   BEntityID unitID = getVar(cInputUnit)->asUnit()->readVar();

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return (false);
   BUser* pUser = pPlayer->getUser();
   if (!pUser)
      return (false);
   bool isSelected = pUser->getSelectionManager()->isUnitSelected(unitID);
   if (isSelected)
      return (true);
   else
      return (false);
}

//XXXHalwes - 7/17/2007 - This has been tagged do not use, do we need to implement/obsolete this?
//==============================================================================
// BTriggerCondition::tcPlayerLookingAtUnit()
//==============================================================================
bool BTriggerCondition::tcPlayerLookingAtUnit()
{
   uint version = getVersion();
   if (version == 2)
   {
      return (tcPlayerLookingAtUnitV2());
   }
   else
   {
      BTRIGGER_ASSERT(false);
      return (false);
   }
}

//XXXHalwes - 7/17/2007 - This has been tagged do not use, do we need to implement/obsolete this?
//==============================================================================
// BTriggerCondition::tcPlayerLookingAtUnitV2()
//==============================================================================
bool BTriggerCondition::tcPlayerLookingAtUnitV2()
{
   enum { cInputPlayer = 1, cInputUnit = 2, };
   BTRIGGER_ASSERT(mbAsyncCondition);
   BTRIGGER_ASSERT(mbLocalAsyncMachine);

   /*long playerID = */getVar(cInputPlayer)->asPlayer()->readVar();
   BEntityID unitID = getVar(cInputUnit)->asUnit()->readVar();

   // todo
   BTRIGGER_ASSERT(false);
   return (false);
}

//XXXHalwes - 7/17/2007 - This has been tagged do not use, do we need to implement this?
//==============================================================================
// BTriggerCondition::tcPlayerLookingAtLocation()
//==============================================================================
bool BTriggerCondition::tcPlayerLookingAtLocation()
{
   BTRIGGER_ASSERT(mbAsyncCondition);
   BTRIGGER_ASSERT(mbLocalAsyncMachine);

   // todo
   BTRIGGER_ASSERT(false);
   return (false);
}


//==============================================================================
// BTriggerCondition::tcCompareTime()
//==============================================================================
bool BTriggerCondition::tcCompareTime()
{
   enum { cInputFirstTime = 1, cInputCompareType = 2, cInputSecondTime = 3, };
   DWORD firstTime = getVar(cInputFirstTime)->asTime()->readVar();
   long compareType = getVar(cInputCompareType)->asOperator()->readVar();
   DWORD secondTime = getVar(cInputSecondTime)->asTime()->readVar();

   bool result = BTriggerCondition::compareValues(firstTime, compareType, secondTime);
   return (result);
}

//==============================================================================
// BTriggerCondition::tcCompareString()
//==============================================================================
bool BTriggerCondition::tcCompareString()
{
   enum { cInputFirstString = 1, cInputCompareType = 2, cInputSecondString = 3, };
   BSimUString firstString = getVar(cInputFirstString)->asString()->readVar();
   long compareType = getVar(cInputCompareType)->asOperator()->readVar();
   BSimUString secondString = getVar(cInputSecondString)->asString()->readVar();

   bool result = BTriggerCondition::compareValues(firstString, compareType, secondString);
   return (result);
}

//==============================================================================
// BTriggerCondition::tcComparePercent()
//==============================================================================
bool BTriggerCondition::tcComparePercent()
{
   enum 
   { 
      cInputFirstPercent  = 1, 
      cInputCompareType   = 2, 
      cInputSecondPercent = 3, 
   };

   float firstPercent  = getVar(cInputFirstPercent)->asFloat()->readVar();
   long  compareType   = getVar(cInputCompareType)->asOperator()->readVar();
   float secondPercent = getVar(cInputSecondPercent)->asFloat()->readVar();

   bool result = BTriggerCondition::compareValues(firstPercent, compareType, secondPercent);

   return (result);
}

//==============================================================================
// BTriggerCondition::tcCompareHitpoints()
//==============================================================================
bool BTriggerCondition::tcCompareHitpoints()
{
   enum 
   { 
      cInputFirstHP     = 1, 
      cInputCompareType = 2, 
      cInputSecondHP    = 3, 
   };

   float firstHP     = getVar(cInputFirstHP)->asFloat()->readVar();
   long  compareType = getVar(cInputCompareType)->asOperator()->readVar();
   float secondHP    = getVar(cInputSecondHP)->asFloat()->readVar();

   bool result = BTriggerCondition::compareValues(firstHP, compareType, secondHP);

   return (result);
}


//==============================================================================
// BTriggerCondition::tcIsMultiplayerActive()
//==============================================================================
bool BTriggerCondition::tcIsMultiplayerActive()
{
   bool isMultiplayerActive = (gLiveSystem->isMultiplayerGameActive() == TRUE);
   return (isMultiplayerActive);
}

//==============================================================================
// Version control function
//==============================================================================
bool BTriggerCondition::tcCanRetrieveExternals()
{
   switch (getVersion())
   {
      case 2:
         return (tcCanRetrieveExternalsV2());
         break;

      case 3:
         return (tcCanRetrieveExternalsV3());
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         return (false);
         break;
   }
}

//==============================================================================
// Retrieve the external input data to the trigger script
//==============================================================================
bool BTriggerCondition::tcCanRetrieveExternalsV2()
{
   enum 
   { 
      cPlayer = 1, 
      cPower = 2, 
      cSquad = 3, 
      cUnit = 4, 
      cCost = 5,      
      cUnitList = 6,
      cSquadList = 7,
   };

//-- FIXING PREFIX BUG ID 3870
   const BTriggerScriptExternals* pExternals = getParentTriggerScript()->getExternals();
//--
   bool errorOccurred = false;
   
   if (getVar(cPlayer)->isUsed())
   {
      long playerID = -1;
      if (pExternals->retrievePlayerID(playerID))
      {
         getVar(cPlayer)->asPlayer()->writeVar(playerID);
      }
      else
      {
         errorOccurred = true;
      }
   }

   if (getVar(cPower)->isUsed())
   {
      long protoPowerID = -1;
      if (pExternals->retrieveProtoPowerID(protoPowerID))
      {
         getVar(cPower)->asPower()->writeVar(protoPowerID);
      }
      else
      {
         errorOccurred = true;
      }
   }

   if (getVar(cSquad)->isUsed())
   {
      BEntityID squadID = cInvalidObjectID;
      if (pExternals->retrieveSquadID(squadID))
      {
         getVar(cSquad)->asSquad()->writeVar(squadID);
      }
      else
      {
         errorOccurred = true;
      }
   }

   if (getVar(cSquadList)->isUsed())
   {
      BEntityIDArray squadIDs;
      squadIDs.clear();
      if (pExternals->retrieveSquadIDs(squadIDs))
      {
         getVar(cSquadList)->asSquadList()->writeVar(squadIDs);
      }
      else
      {
         errorOccurred = true;
      }
   }

   if (getVar(cUnit)->isUsed())
   {
      BEntityID unitID = cInvalidObjectID;
      if (pExternals->retrieveUnitID(unitID))
      {
         getVar(cUnit)->asUnit()->writeVar(unitID);
      }
      else
      {
         errorOccurred = true;
      }
   }

   if (getVar(cUnitList)->isUsed())
   {
      BEntityIDArray unitIDs;
      unitIDs.clear();
      if (pExternals->retrieveUnitIDs(unitIDs))
      {
         getVar(cUnitList)->asUnitList()->writeVar(unitIDs);
      }
      else
      {
         errorOccurred = true;
      }
   }

   if (getVar(cCost)->isUsed())
   {
      BCost cost;
      if (pExternals->retrieveCost(cost))
      {
         getVar(cCost)->asCost()->writeVar(cost);
      }
      else
      {
         errorOccurred = true;
      }
   }

   return (!errorOccurred);
}

//==============================================================================
// Retrieve the external input data to the trigger script
//==============================================================================
bool BTriggerCondition::tcCanRetrieveExternalsV3()
{
   enum 
   { 
      cPlayer = 1, 
      cPower = 2, 
      cSquad = 3, 
      cUnit = 4, 
      cCost = 5,      
      cUnitList = 6,
      cSquadList = 7,
      //cLocationList = 8,
   };

//-- FIXING PREFIX BUG ID 3871
   const BTriggerScriptExternals* pExternals = getParentTriggerScript()->getExternals();
//--
   bool errorOccurred = false;

   if (getVar(cPlayer)->isUsed())
   {
      long playerID = -1;
      if (pExternals->retrievePlayerID(playerID))
      {
         getVar(cPlayer)->asPlayer()->writeVar(playerID);
      }
      else
      {
         errorOccurred = true;
      }
   }

   if (getVar(cPower)->isUsed())
   {
      long protoPowerID = -1;
      if (pExternals->retrieveProtoPowerID(protoPowerID))
      {
         getVar(cPower)->asPower()->writeVar(protoPowerID);
      }
      else
      {
         errorOccurred = true;
      }
   }

   if (getVar(cSquad)->isUsed())
   {
      BEntityID squadID = cInvalidObjectID;
      if (pExternals->retrieveSquadID(squadID))
      {
         getVar(cSquad)->asSquad()->writeVar(squadID);
      }
      else
      {
         errorOccurred = true;
      }
   }

   if (getVar(cSquadList)->isUsed())
   {
      BEntityIDArray squadIDs;
      squadIDs.clear();
      if (pExternals->retrieveSquadIDs(squadIDs))
      {
         getVar(cSquadList)->asSquadList()->writeVar(squadIDs);
      }
      else
      {
         errorOccurred = true;
      }
   }

   if (getVar(cUnit)->isUsed())
   {
      BEntityID unitID = cInvalidObjectID;
      if (pExternals->retrieveUnitID(unitID))
      {
         getVar(cUnit)->asUnit()->writeVar(unitID);
      }
      else
      {
         errorOccurred = true;
      }
   }

   if (getVar(cUnitList)->isUsed())
   {
      BEntityIDArray unitIDs;
      unitIDs.clear();
      if (pExternals->retrieveUnitIDs(unitIDs))
      {
         getVar(cUnitList)->asUnitList()->writeVar(unitIDs);
      }
      else
      {
         errorOccurred = true;
      }
   }

   if (getVar(cCost)->isUsed())
   {
      BCost cost;
      if (pExternals->retrieveCost(cost))
      {
         getVar(cCost)->asCost()->writeVar(cost);
      }
      else
      {
         errorOccurred = true;
      }
   }

   //if (getVar(cLocationList)->isUsed())
   //{
   //   BVectorArray locs;
   //   if (pExternals->retrieveLocationList(locs))
   //   {
   //      getVar(cLocationList)->asVectorList()->writeVar(locs);
   //   }
   //   else
   //   {
   //      errorOccurred = true;
   //   }      
   //}

   return (!errorOccurred);
}

//==============================================================================
// Compare squad to location distance
//==============================================================================
bool BTriggerCondition::tcSquadLocationDistance()
{
   enum 
   { 
      cInputSquad = 1, 
      cInputLocation = 2, 
      cInputOperator = 3, 
      cInputDistance = 4, 
   };

   BEntityID squadID     = getVar(cInputSquad)->asSquad()->readVar();
   BVector   comparePos  = getVar(cInputLocation)->asVector()->readVar();
   long      op          = getVar(cInputOperator)->asOperator()->readVar();
   float     compareDist = getVar(cInputDistance)->asFloat()->readVar();

//-- FIXING PREFIX BUG ID 3855
   const BSquad* pSquad = gWorld->getSquad(squadID);
//--
   if (!pSquad)
   {
      return (false);
   }

   float dist = pSquad->BEntity::calculateXZDistance(comparePos);

   bool result = compareValues(dist, op, compareDist);

   return (result);
}


//==============================================================================
// BTriggerCondition::tcCompareCiv()
//==============================================================================
bool BTriggerCondition::tcCompareCiv()
{
   enum { cFirstCiv = 1, cCompareType = 2, cSecondCiv = 3, };
   long firstCivID = getVar(cFirstCiv)->asCiv()->readVar();
   long compareType = getVar(cCompareType)->asOperator()->readVar();
   long secondCivID = getVar(cSecondCiv)->asCiv()->readVar();
   BTRIGGER_ASSERT(compareType == Math::cEqualTo || compareType == Math::cNotEqualTo);
   bool result = compareValues(firstCivID, compareType, secondCivID);
   return (result);
}

//XXXHalwes - 7/17/2007 - Valid?
//==============================================================================
// BTriggerCondition::tcIsHitZoneActive()
//==============================================================================
bool BTriggerCondition::tcIsHitZoneActive()
{
   enum 
   { 
      cUnit      = 1, 
      cHZName    = 2,
   };

   bool        result = false;
   BEntityID   unitID = getVar(cUnit)->asUnit()->readVar();
   BSimUString uName  = getVar(cHZName)->asString()->readVar();

   BSimString hzName;
   hzName.format("%S", uName.getPtr());

//-- FIXING PREFIX BUG ID 3856
   const BUnit* pUnit   = gWorld->getUnit(unitID);
//--
   long   hzIndex = -1;
   if (pUnit && pUnit->getHitZoneIndex(hzName, hzIndex))
   {
      result = pUnit->getHitZoneList()->get(hzIndex).getActive();
   }

   return (result);
}

//==============================================================================
// BTriggerCondition::tcCompareAmmoPercent()
//==============================================================================
bool BTriggerCondition::tcCompareAmmoPercent()
{
   uint version = getVersion();
   if (version == 2)
   {
      return (tcCompareAmmoPercentV2());
   }
   else
   {
      BTRIGGER_ASSERT(false);
      return (false);
   }
}


//==============================================================================
// BTriggerCondition::tcCompareAmmoPercentV2()
//==============================================================================
bool BTriggerCondition::tcCompareAmmoPercentV2()
{
   enum 
   { 
      cUnit = 1, 
      cCompareType = 2, 
      cAmmoPercent = 3, 
      cOutputCurrentAmmoPercent = 4, 
   };

   BEntityID unitID = getVar(cUnit)->asUnit()->readVar();
   long compareType = getVar(cCompareType)->asOperator()->readVar();
   float ammoPercent = getVar(cAmmoPercent)->asFloat()->readVar();
   bool useOutputCurrentAmmoPercent = getVar(cOutputCurrentAmmoPercent)->isUsed();
   float currentPercent = 0.0f;
//-- FIXING PREFIX BUG ID 3859
   const BUnit* pUnit = gWorld->getUnit(unitID);
//--
   if (pUnit)
   {
      float currentAmmo = pUnit->getAmmunition();
      float maxAmmo = pUnit->getProtoObject()->getMaxAmmo();
      BTRIGGER_ASSERT((currentAmmo >= 0.0f) && (currentAmmo <= maxAmmo));
      currentPercent = (maxAmmo > 0.0f) ? (currentAmmo / maxAmmo) : 0.0f;
   }

   if (useOutputCurrentAmmoPercent)
   {
      getVar(cOutputCurrentAmmoPercent)->asFloat()->writeVar(currentPercent);
   }

   bool result = compareValues(currentPercent, compareType, ammoPercent);
   return (result);
}


//==============================================================================
// BTriggerCondition::tcIsIdle()
//==============================================================================
bool BTriggerCondition::tcIsIdle()
{
   enum { cUnit = 1, cSquad = 2, cDuration = 3, };
   bool useUnit = getVar(cUnit)->isUsed();
   bool useSquad = getVar(cSquad)->isUsed();
   bool useIdleDuration = getVar(cDuration)->isUsed();

   DWORD idleDuration = 0;

   if (useUnit)
   {
      BEntityID unitID = getVar(cUnit)->asUnit()->readVar();
      BUnit *pUnit = gWorld->getUnit(unitID);
      if (pUnit)
      {
         BEntityActionIdle *pIdleAction = (BEntityActionIdle*)pUnit->getActionByTypeConst(BAction::cActionTypeEntityIdle);
         idleDuration = pIdleAction ? pIdleAction->getIdleDuration() : 0;
         if (useIdleDuration)
            getVar(cDuration)->asTime()->writeVar(idleDuration);
         return (pIdleAction != NULL);
      }
   }

   if (useSquad)
   {
      BEntityID squadID = getVar(cSquad)->asSquad()->readVar();
      BSquad *pSquad = gWorld->getSquad(squadID);
      if (pSquad)
      {
         BEntityActionIdle *pIdleAction = (BEntityActionIdle*)pSquad->getActionByTypeConst(BAction::cActionTypeEntityIdle);
         idleDuration = pIdleAction ? pIdleAction->getIdleDuration() : 0;
         if (useIdleDuration)
            getVar(cDuration)->asTime()->writeVar(idleDuration);
         return (pIdleAction != NULL);
      }
   }
   
   if (useIdleDuration)
      getVar(cDuration)->asTime()->writeVar(idleDuration);
   return (false);
}


//==============================================================================
// BTriggerCondition::tcGameTime()
//==============================================================================
bool BTriggerCondition::tcGameTime()
{
   enum { cCompareType = 1, cCompareTime = 2, };
   DWORD currentGametime = gWorld->getGametime();
   long compareType = getVar(cCompareType)->asOperator()->readVar();
   DWORD compareTime = getVar(cCompareTime)->asTime()->readVar();
   bool result = compareValues(currentGametime, compareType, compareTime);
   return (result);
}


//==============================================================================
// BTriggerCondition::tcGameTimeReached()
//==============================================================================
bool BTriggerCondition::tcGameTimeReached()
{
   enum { cTestTime = 1, cTimeRemaining = 2, };
   DWORD currentGametime = gWorld->getGametime();
   DWORD testTime = getVar(cTestTime)->asTime()->readVar();
   bool useTimeRemaining = getVar(cTimeRemaining)->isUsed();
   bool timeReached = (currentGametime >= testTime);
   if (useTimeRemaining)
   {
      DWORD timeRemaining = timeReached ? 0 : (testTime - currentGametime);
      getVar(cTimeRemaining)->asTime()->writeVar(timeRemaining);
   }
   return (timeReached);
}


//==============================================================================
// BTriggerCondition::tcTriggerActiveTimeReached()
//==============================================================================
bool BTriggerCondition::tcTriggerActiveTimeReached()
{
   enum { cTestTime = 1, cTimeRemaining = 2, };
   DWORD activatedTime = mpParentTrigger->getActivatedTime();
   DWORD currentGametime = gWorld->getGametime();
   DWORD triggerActiveTime = currentGametime - activatedTime;
   DWORD testTime = getVar(cTestTime)->asTime()->readVar();
   bool useTimeRemaining = getVar(cTimeRemaining)->isUsed();
   bool timeReached = (triggerActiveTime >= testTime);
   if (useTimeRemaining)
   {
      DWORD timeRemaining = timeReached ? 0 : (testTime - triggerActiveTime);
      getVar(cTimeRemaining)->asTime()->writeVar(timeRemaining);
   }
   return (timeReached);
}


//==============================================================================
// BTriggerCondition::tcPlayerSelectingSquad()
//==============================================================================
bool BTriggerCondition::tcPlayerSelectingSquad()
{
   enum { cPlayer = 1, cSquad = 2, };
   BTRIGGER_ASSERT(mbAsyncCondition);
   BTRIGGER_ASSERT(mbLocalAsyncMachine);
   long playerID = getVar(cPlayer)->asPlayer()->readVar();
   BEntityID squadID = getVar(cSquad)->asSquad()->readVar();

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return (false);
   BUser* pUser = pPlayer->getUser();
   if (!pUser)
      return (false);

   BSquad *pSquad = gWorld->getSquad(squadID);
   if (!pSquad)
      return (false);

   bool isSelected = pUser->getSelectionManager()->isSquadSelected(squadID);
   if (isSelected)
      return (true);
   else
      return (false);
}


//==============================================================================
// BTriggerCondition::tcNextObject()
//==============================================================================
bool BTriggerCondition::tcNextObject()
{
   enum { cIterator = 1, cOutputNextObject = 2, };

   // Get our list var, and assert that it exists (or we're using an uninitialized iterator)
   BTriggerVarIterator *pIterator = getVar(cIterator)->asIterator();
   BTRIGGER_ASSERT(pIterator);
   BTriggerVarID listVarID = pIterator->getListVarID();
   BTriggerVar *pListVar = getParentTriggerScript()->getTriggerVar(listVarID);
   BTRIGGER_ASSERT(pListVar);
   const BEntityIDArray& objectList = pListVar->asObjectList()->readVar();

   long numObjects = objectList.getNumber();
   for (long i=0; i<numObjects; i++)
   {
      if (!pIterator->isObjectVisited(objectList[i]))
      {
         pIterator->visitObject(objectList[i]);
         getVar(cOutputNextObject)->asObject()->writeVar(objectList[i]);
         return (true);
      }
   }

   return (false);
}


//==============================================================================
// BTriggerCondition::tcNextLocation()
//==============================================================================
bool BTriggerCondition::tcNextLocation()
{
   enum { cIterator = 1, cOutputNextLocation = 2, };

   // Get our list var, and assert that it exists (or we're using an uninitialized iterator)
   BTriggerVarIterator *pIterator = getVar(cIterator)->asIterator();
   BTRIGGER_ASSERT(pIterator);
   BTriggerVarID listVarID = pIterator->getListVarID();
   BTriggerVar *pListVar = getParentTriggerScript()->getTriggerVar(listVarID);
   BTRIGGER_ASSERT(pListVar);
   const BVectorArray& vecList = pListVar->asVectorList()->readVar();

   uint numVecs = vecList.getSize();
   for (uint i=0; i<numVecs; i++)
   {
      if (!pIterator->isVectorVisited(vecList[i]))
      {
         pIterator->visitVector(vecList[i]);
         getVar(cOutputNextLocation)->asVector()->writeVar(vecList[i]);
         return (true);
      }
   }

   return (false);
}


//==============================================================================
// BTriggerCondition::tcRandomListLocation
//==============================================================================
bool BTriggerCondition::tcRandomListLocation()
{
   enum { cLocationList = 1, cRandomLocation = 2, };
   const BLocationArray &locationList = getVar(cLocationList)->asVectorList()->readVar();
   long numRandomLocations = locationList.getNumber();
   if (numRandomLocations < 1)
      return (false);

   long randomIndex = getRandRange(cSimRand, 0, (numRandomLocations-1));
   getVar(cRandomLocation)->asVector()->writeVar(locationList[randomIndex]);
   return (true);
}


//==============================================================================
// BTriggerCondition::refCountHelper
//==============================================================================
void BTriggerCondition::refCountHelper(BEntityID entityID, short refCountType, bool doCompare, long operatorType, long compareCount, long& maxCount, bool& retval, bool& anyCompared)
{
   BEntity *pEntity = gWorld->getEntity(entityID);
   if (pEntity)
   {
      BEntityRef *pEntityRef = pEntity->getFirstEntityRefByType(refCountType);
      long foundCount = pEntityRef ? pEntityRef->mData1 : 0;
      if (foundCount > maxCount)
         maxCount = foundCount;
      if (doCompare && !BTriggerCondition::compareValues(foundCount, operatorType, compareCount))
         retval = false;
      anyCompared = true;
   }
}


//==============================================================================
// BTriggerCondition::tcRefCountUnit
//==============================================================================
bool BTriggerCondition::tcRefCountUnit()
{
   enum { cUnit = 1, cUnitList = 2, cSquad = 3, cSquadList = 4, cRefCountType = 5, cCompareType = 7, cCompareCount = 8, cMaxCount = 6 };

   bool doCompare = false;
   long operatorType = -1;
   long compareCount = 0;

   if (getVar(cCompareType)->isUsed())
   {
      doCompare = true;
      operatorType = getVar(cCompareType)->asOperator()->readVar();
      if (getVar(cCompareCount)->isUsed())
         compareCount = getVar(cCompareCount)->asInteger()->readVar();
   }

   long maxCount = 0;
   bool retval = true;
   bool anyCompared = false;

   short refCountType = getVar(cRefCountType)->asRefCountType()->readVar();
   refCountType += BEntityRef::cTypeDesignStart;

   if (refCountType >= BEntityRef::cTypeDesignStart && refCountType <= BEntityRef::cTypeDesignEnd)
   {
      if (getVar(cUnit)->isUsed())
         refCountHelper(getVar(cUnit)->asUnit()->readVar(), refCountType, doCompare, operatorType, compareCount, maxCount, retval, anyCompared);

      if (getVar(cUnitList)->isUsed())
      {
         const BEntityIDArray& entityList = getVar(cUnitList)->asUnitList()->readVar();
         long numEntities = entityList.getNumber();
         for (long i=0; i<numEntities; i++)
            refCountHelper(entityList[i], refCountType, doCompare, operatorType, compareCount, maxCount, retval, anyCompared);
      }

      if (getVar(cSquad)->isUsed())
      {
//-- FIXING PREFIX BUG ID 3759
         const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
         if (pSquad)
         {
            uint numChildren = pSquad->getNumberChildren();
            for (uint i=0; i<numChildren; i++)
               refCountHelper(pSquad->getChild(i), refCountType, doCompare, operatorType, compareCount, maxCount, retval, anyCompared);
         }
      }

      if (getVar(cSquadList)->isUsed())
      {
         const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
         long numSquads = squadList.getNumber();
         for (long j=0; j<numSquads; j++)
         {
//-- FIXING PREFIX BUG ID 3760
            const BSquad* pSquad = gWorld->getSquad(squadList[j]);
//--
            if (pSquad)
            {
               uint numChildren = pSquad->getNumberChildren();
               for (uint i=0; i<numChildren; i++)
                  refCountHelper(pSquad->getChild(i), refCountType, doCompare, operatorType, compareCount, maxCount, retval, anyCompared);
            }
         }
      }
   }

   if (getVar(cMaxCount)->isUsed())
      getVar(cMaxCount)->asInteger()->writeVar(maxCount);

   if (doCompare && !anyCompared)
      retval = BTriggerCondition::compareValues(0, operatorType, compareCount);

   return retval;
}


//==============================================================================
// BTriggerCondition::tcRefCountSquad
//==============================================================================
bool BTriggerCondition::tcRefCountSquad()
{
   enum { cSquad = 1, cSquadList = 2, cRefCountType = 3, cCompareType = 4, cCompareCount = 5, cMaxCount = 6 };

   bool doCompare = false;
   long operatorType = -1;
   long compareCount = 0;

   if (getVar(cCompareType)->isUsed())
   {
      doCompare = true;
      operatorType = getVar(cCompareType)->asOperator()->readVar();
      if (getVar(cCompareCount)->isUsed())
         compareCount = getVar(cCompareCount)->asInteger()->readVar();
   }

   long maxCount = 0;
   bool retval = true;
   bool anyCompared = false;

   short refCountType = getVar(cRefCountType)->asRefCountType()->readVar();
   refCountType += BEntityRef::cTypeDesignStart;

   if (refCountType >= BEntityRef::cTypeDesignStart && refCountType <= BEntityRef::cTypeDesignEnd)
   {
      if (getVar(cSquad)->isUsed())
         refCountHelper(getVar(cSquad)->asSquad()->readVar(), refCountType, doCompare, operatorType, compareCount, maxCount, retval, anyCompared);

      if (getVar(cSquadList)->isUsed())
      {
         const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
         long numSquads = squadList.getNumber();
         for (long j=0; j<numSquads; j++)
            refCountHelper(squadList[j], refCountType, doCompare, operatorType, compareCount, maxCount, retval, anyCompared);
      }
   }

   if (getVar(cMaxCount)->isUsed())
      getVar(cMaxCount)->asInteger()->writeVar(maxCount);

   if (doCompare && !anyCompared)
      retval = BTriggerCondition::compareValues(0, operatorType, compareCount);

   return retval;
}


//==============================================================================
// BTriggerCondition::unitFlagHelper
//==============================================================================
void BTriggerCondition::unitFlagHelper(BEntityID unitID, long flagType, bool doCompare, bool compareValue, bool& retval, bool& allOn)
{   
//-- FIXING PREFIX BUG ID 3860
   const BUnit* pUnit= gWorld->getUnit(unitID);
//--
   if (pUnit)
   {
      bool flagValue = false;
      switch(flagType)
      {
         case BUnit::cFlagAttackBlocked:            
         {  
            flagValue = pUnit->getFlagAttackBlocked();
            break;
         }
      }

      if (doCompare && flagValue != compareValue)
         retval = false;
      if (!flagValue)
         allOn = false;
   }
}

//==============================================================================
// BTriggerCondition::squadFlagHelper
//==============================================================================
void BTriggerCondition::squadFlagHelper(BEntityID squadID, long flagType, bool doCompare, bool compareValue, bool& retval, bool& allOn)
{   
//-- FIXING PREFIX BUG ID 3857
   const BSquad* pSquad = gWorld->getSquad(squadID);
//--
   if (pSquad)
   {
      bool flagValue = false;
      switch(flagType)
      {
         case BSquad::cFlagAttackBlocked:            
         {  
            flagValue = pSquad->getFlagAttackBlocked();
            break;
         }
      }

      if (doCompare && flagValue != compareValue)
         retval = false;
      if (!flagValue)
         allOn = false;
   }
}


//==============================================================================
// BTriggerCondition::tcUnitFlag
//==============================================================================
bool BTriggerCondition::tcUnitFlag()
{
   enum { cUnit = 1, cUnitList = 2, cSquad = 3, cSquadList = 4, cFlagType = 5, cCompareValue = 6, cAllOn = 8 };

   bool doCompare = false;
   bool compareValue = true;

   if (getVar(cCompareValue)->isUsed())
   {
      doCompare = true;
      compareValue = getVar(cCompareValue)->asBool()->readVar();
   }

   bool retval = true;
   bool allOn = true;

   long flagType = getVar(cFlagType)->asUnitFlag()->readVar();
   if (flagType == -1)
   {
      if (doCompare)
         retval = false;
      allOn = false;
   }
   else
   {
      if (getVar(cUnit)->isUsed())
         unitFlagHelper(getVar(cUnit)->asUnit()->readVar(), flagType, doCompare, compareValue, retval, allOn);

      if (getVar(cUnitList)->isUsed())
      {
         const BEntityIDArray& entityList = getVar(cUnitList)->asUnitList()->readVar();
         long numEntities = entityList.getNumber();
         for (long i=0; i<numEntities; i++)
            unitFlagHelper(entityList[i], flagType, doCompare, compareValue, retval, allOn);
      }

      if (getVar(cSquad)->isUsed())
      {
//-- FIXING PREFIX BUG ID 3761
         const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
         if (pSquad)
         {
            uint numChildren = pSquad->getNumberChildren();
            for (uint i=0; i<numChildren; i++)
               unitFlagHelper(pSquad->getChild(i), flagType, doCompare, compareValue, retval, allOn);
         }
      }

      if (getVar(cSquadList)->isUsed())
      {
         const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
         long numSquads = squadList.getNumber();
         for (long j=0; j<numSquads; j++)
         {
//-- FIXING PREFIX BUG ID 3762
            const BSquad* pSquad = gWorld->getSquad(squadList[j]);
//--
            if (pSquad)
            {
               uint numChildren = pSquad->getNumberChildren();
               for (uint i=0; i<numChildren; i++)
                  unitFlagHelper(pSquad->getChild(i), flagType, doCompare, compareValue, retval, allOn);
            }
         }
      }
   }

   if (getVar(cAllOn)->isUsed())
      getVar(cAllOn)->asBool()->writeVar(allOn);

   return retval;
}


//==============================================================================
// BTriggerCondition::tcUILocationUILockError
//==============================================================================
bool BTriggerCondition::tcUILocationUILockError()
{
   enum { cUILocation = 1, };
   long uiLocationResult = getVar(cUILocation)->asUILocation()->readResult();
   return (uiLocationResult == BTriggerVarUILocation::cUILocationResultUILockError);
}


//==============================================================================
// BTriggerCondition::tcUIUnitUILockError
//==============================================================================
bool BTriggerCondition::tcUIUnitUILockError()
{
   enum { cUIUnit = 1, };
   long uiUnitResult = getVar(cUIUnit)->asUIUnit()->readResult();
   return (uiUnitResult == BTriggerVarUIUnit::cUIUnitResultUILockError);
}


//==============================================================================
// BTriggerCondition::tcUISquadUILockError
//==============================================================================
bool BTriggerCondition::tcUISquadUILockError()
{
   enum { cUISquad = 1, };
   long uiSquadResult = getVar(cUISquad)->asUISquad()->readResult();
   return (uiSquadResult == BTriggerVarUISquad::cUISquadResultUILockError);
}


//==============================================================================
// BTriggerCondition::tcUILocationWaiting
//==============================================================================
bool BTriggerCondition::tcUILocationWaiting()
{
   enum { cUILocation = 1, };
   long uiLocationResult = getVar(cUILocation)->asUILocation()->readResult();
   return (uiLocationResult == BTriggerVarUILocation::cUILocationResultWaiting);
}


//==============================================================================
// BTriggerCondition::tcUIUnitWaiting
//==============================================================================
bool BTriggerCondition::tcUIUnitWaiting()
{
   enum { cUIUnit = 1, };
   long uiUnitResult = getVar(cUIUnit)->asUIUnit()->readResult();
   return (uiUnitResult == BTriggerVarUIUnit::cUIUnitResultWaiting);
}

//==============================================================================
// BTriggerCondition::tcUISquadWaiting
//==============================================================================
bool BTriggerCondition::tcUISquadWaiting()
{
   enum { cUISquad = 1, };
   long uiSquadResult = getVar(cUISquad)->asUISquad()->readResult();
   return (uiSquadResult == BTriggerVarUISquad::cUISquadResultWaiting);
}

//==============================================================================
// BTriggerCondition::tcCompareCost
//==============================================================================
bool BTriggerCondition::tcCompareCost()
{
   enum 
   { 
      cInputFirstCost   = 1, 
      cInputCompareType = 2, 
      cInputOrFlag      = 3,
      cInputSecondCost  = 4, 
   };

//-- FIXING PREFIX BUG ID 3763
   const BCost& firstCost   = getVar(cInputFirstCost)->asCost()->readVar();
//--
   long   compareType = getVar(cInputCompareType)->asOperator()->readVar();
   bool   orFlag      = getVar(cInputOrFlag)->isUsed() ? getVar(cInputOrFlag)->asBool()->readVar() : false;
//-- FIXING PREFIX BUG ID 3764
   const BCost& secondCost  = getVar(cInputSecondCost)->asCost()->readVar();
//--

   long numResources = firstCost.getNumberResources();
   if (numResources != secondCost.getNumberResources())
   {
      BTRIGGER_ASSERTM(false, "Cost parameters' resource amounts do not match!");
      return (false);
   }

   bool result = false;
   for (long i = 0; i < numResources; i++)
   {
      result = BTriggerCondition::compareValues(firstCost.get(i), compareType, secondCost.get(i));
      if (orFlag && result)
      {
         return (true);
      }
      else if (!orFlag && !result)
      {
         return (false);
      }
   }

   if (orFlag)
   {
      return (false);
   }
   else
   {
      return (true);
   }
}

//==============================================================================
// BTriggerCondition::tcCheckResourceTotals()
//==============================================================================
bool BTriggerCondition::tcCheckResourceTotals()
{
   enum 
   { 
      cInputPlayer = 1, 
      cInputCost   = 2, 
   };

   long  playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   BCost cost     = getVar(cInputCost)->asCost()->readVar();

//-- FIXING PREFIX BUG ID 3765
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
//--
   if (!pPlayer)
   {
      return (false);
   }

   bool result = pPlayer->checkTotal(&cost);

   return (result);
}

//==============================================================================
// BTriggerCondition::tcComparePopulation()
//==============================================================================
bool BTriggerCondition::tcComparePopulation()
{
   enum 
   { 
      cInputPlayer    = 1,
      cInputPopBucket = 2,
      cInputCompare   = 3, 
      cInputAmount    = 4,
   };

   long playerID  = getVar(cInputPlayer)->asPlayer()->readVar();
   long popBucket = getVar(cInputPopBucket)->asPopBucket()->readVar();
   long compare   = getVar(cInputCompare)->asOperator()->readVar();
   long amount    = getVar(cInputAmount)->asInteger()->readVar();

//-- FIXING PREFIX BUG ID 3766
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
//--
   if (!pPlayer)
   {
      return (false);
   }

   float pop = pPlayer->getPopCount(popBucket);

   bool result = BTriggerCondition::compareValues(pop, compare, (float)amount);

   return (result);
}

//==============================================================================
// BTriggerCondition::tcCheckDiplomacy
//==============================================================================
bool BTriggerCondition::tcCheckDiplomacy()
{
   enum 
   { 
      cUnit          = 1,
      cUnitList      = 2,
      cSquad         = 3,
      cSquadList     = 4,
      cRelationType  = 5,
      cPlayer        = 6,
      cTeam          = 7,
   };

   bool useUnit      = getVar(cUnit)->isUsed();
   bool useUnitList  = getVar(cUnitList)->isUsed();
   bool useSquad     = getVar(cSquad)->isUsed();
   bool useSquadList = getVar(cSquadList)->isUsed();

   if (!useUnit && !useUnitList && !useSquad && !useSquadList)
   {
      BTRIGGER_ASSERTM(false, "No Unit, UnitList, Squad, and/or SquadList assigned!");
      return (false);
   }

   // Collect test team IDs
   BTeamIDArray teamIDArray;
   if (useUnit)
   {
//-- FIXING PREFIX BUG ID 3767
      const BUnit* pUnit = gWorld->getUnit(getVar(cUnit)->asUnit()->readVar());
//--
      if (pUnit)
      {
         teamIDArray.uniqueAdd(pUnit->getTeamID());
      }
   }

   if (useUnitList)
   {
      const BEntityIDArray& unitList = getVar(cUnitList)->asUnitList()->readVar();
      long                  numUnits = unitList.getNumber();
      for (long i = 0; i < numUnits; i++)
      {
//-- FIXING PREFIX BUG ID 3768
         const BUnit* pUnit = gWorld->getUnit(unitList[i]);
//--
         if (pUnit)
         {
            teamIDArray.uniqueAdd(pUnit->getTeamID());
         }
      }
   }

   if (useSquad)
   {
//-- FIXING PREFIX BUG ID 3769
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         teamIDArray.uniqueAdd(pSquad->getTeamID());
      }
   }

   if (useSquadList)
   {
      const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
      long                  numSquads = squadList.getNumber();
      for (long i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 3770
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
         if (pSquad)
         {
            teamIDArray.uniqueAdd(pSquad->getTeamID());
         }
      }
   }

   // Collect reference team ID
   BTeamID refTeamID = -1;
   if (getVar(cPlayer)->isUsed())
   {
//-- FIXING PREFIX BUG ID 3771
      const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
//--
      if (pPlayer)
      {
         refTeamID = pPlayer->getTeamID();
      }
   }
   else if (getVar(cTeam)->isUsed())
   {
      refTeamID = getVar(cTeam)->asTeam()->readVar();
   }

   if (refTeamID == -1)
   {
      BTRIGGER_ASSERTM(false, "No reference team ID found!");
      return (false);
   }

   // Test relation
   BRelationType relationType  = getVar(cRelationType)->asRelationType()->readVar();
   uint numTeamIDs = teamIDArray.getSize();
   for (uint i = 0; i < numTeamIDs; i++)
   {
      if (!compareRelationType(refTeamID, relationType, teamIDArray[i]))
      {
         return (false);
      }
   }

   return (true);
}

//=====================================================================================================
// Checks if a squad(s) is currently changing modes and will output flag stating if its in normal mode
//=====================================================================================================
bool BTriggerCondition::tcCheckModeChange()
{
   enum
   {
      cSquad = 1,      
      cOutNormal = 2,
      cOutResult = 3,
   };

//-- FIXING PREFIX BUG ID 3772
   const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
   if (!pSquad)
   {
      return (false);
   }

   bool result = pSquad->getFlagChangingMode();

   if (getVar(cOutResult)->isUsed())
   {
      getVar(cOutResult)->asBool()->writeVar(result);
   }

   if (getVar(cOutNormal)->isUsed())
   {
      getVar(cOutNormal)->asBool()->writeVar(pSquad->getSquadMode() == BSquadAI::cModeNormal);
   }

   return (result);
}

//=====================================================================================================
// Checks if a unit or squad has been built
//=====================================================================================================
bool BTriggerCondition::tcIsBuilt()
{
   enum
   {
      cUnit = 1,
      cSquad = 2,
   };

   if (getVar(cUnit)->isUsed())
   {
//-- FIXING PREFIX BUG ID 3773
      const BUnit* pUnit = gWorld->getUnit(getVar(cUnit)->asUnit()->readVar());
//--
      if (pUnit)
      {
         return (pUnit->getFlagBuilt());
      }
   }
   else if (getVar(cSquad)->isUsed())
   {      
//-- FIXING PREFIX BUG ID 3775
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         uint numChildren = pSquad->getNumberChildren();
         for (uint i = 0; i < numChildren; i++)
         {
//-- FIXING PREFIX BUG ID 3774
            const BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
//--
            if (pUnit && !pUnit->getFlagBuilt())
            {
               return (false);
            }
         }
         return (true);
      }
   }

   return (false);
}

//=====================================================================================================
// Checks if a unit or squad is moving
//=====================================================================================================
bool BTriggerCondition::tcIsMoving()
{
   enum
   {
      cUnit = 1,
      cSquad = 2,
   };

   if (getVar(cUnit)->isUsed())
   {
//-- FIXING PREFIX BUG ID 3776
      const BUnit* pUnit = gWorld->getUnit(getVar(cUnit)->asUnit()->readVar());
//--
      if (pUnit)
      {
         return (pUnit->isMoving());
      }
   }
   else if (getVar(cSquad)->isUsed())
   {      
//-- FIXING PREFIX BUG ID 3777
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         return (pSquad->isMoving());
      }
   }

   return (false);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcPlayerIsHuman()
{
   enum { cPlayer = 1, };
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
   return (pPlayer ? pPlayer->isHuman() : false);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcPlayerIsGaia()
{
   enum { cPlayer = 1, };
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());
   return (pPlayer ? pPlayer->isGaia() : false);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcComparePlayers()
{
   enum { cInputFirstPlayer = 1, cInputCompareType = 2, cInputSecondPlayer = 3, };
   BPlayerID firstPlayerID = getVar(cInputFirstPlayer)->asPlayer()->readVar();
   long compareType = getVar(cInputCompareType)->asOperator()->readVar();
   BPlayerID secondPlayerID = getVar(cInputSecondPlayer)->asPlayer()->readVar();
   BTRIGGER_ASSERT(compareType == Math::cEqualTo || compareType == Math::cNotEqualTo);
   bool result = BTriggerCondition::compareValues(firstPlayerID, compareType, secondPlayerID);
   return (result);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCompareTeams()
{
   enum { cInputFirstTeam = 1, cInputCompareType = 2, cInputSecondTeam = 3, };
   BTeamID firstTeamID = getVar(cInputFirstTeam)->asTeam()->readVar();
   long compareType = getVar(cInputCompareType)->asOperator()->readVar();
   BTeamID secondTeamID = getVar(cInputSecondTeam)->asTeam()->readVar();
   BTRIGGER_ASSERT(compareType == Math::cEqualTo || compareType == Math::cNotEqualTo);
   bool result = BTriggerCondition::compareValues(firstTeamID, compareType, secondTeamID);
   return (result);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCanGetOnePlayer()
{
   enum { cInputPlayerList = 1, cInputListPos = 2, cOutputPlayer = 3, };
   const BPlayerIDArray& playerList = getVar(cInputPlayerList)->asPlayerList()->readVar();
   long listPos = getVar(cInputListPos)->asListPosition()->readVar();
   switch (listPos)
   {
   case BTriggerVarListPosition::cListPosTypeFirst:
      {
         if (playerList.getSize() > 0)
         {
            getVar(cOutputPlayer)->asPlayer()->writeVar(playerList[0]);
            return (true);
         }            
         break;
      }
   case BTriggerVarListPosition::cListPosTypeLast:
      {
         uint numPlayers = playerList.getSize();
         if (numPlayers > 0)
         {
            getVar(cOutputPlayer)->asPlayer()->writeVar(playerList[numPlayers - 1]);
            return (true);
         }
         break;
      }
   case BTriggerVarListPosition::cListPosTypeRandom:
      {
         uint numPlayers = playerList.getSize();
         if (numPlayers > 0)
         {
            uint randomIndex = getRandRange(cSimRand, 0, (numPlayers-1));
            getVar(cOutputPlayer)->asPlayer()->writeVar(playerList[randomIndex]);
            return (true);
         }
         break;
      }
   }

   return (false);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCanGetOneTeam()
{
   enum { cInputTeamList = 1, cInputListPos = 2, cOutputTeam = 3, };
   const BTeamIDArray& teamList = getVar(cInputTeamList)->asTeamList()->readVar();
   long listPos = getVar(cInputListPos)->asListPosition()->readVar();
   switch (listPos)
   {
   case BTriggerVarListPosition::cListPosTypeFirst:
      {
         if (teamList.getSize() > 0)
         {
            getVar(cOutputTeam)->asTeam()->writeVar(teamList[0]);
            return (true);
         }            
         break;
      }
   case BTriggerVarListPosition::cListPosTypeLast:
      {
         uint numTeams = teamList.getSize();
         if (numTeams > 0)
         {
            getVar(cOutputTeam)->asTeam()->writeVar(teamList[numTeams - 1]);
            return (true);
         }
         break;
      }
   case BTriggerVarListPosition::cListPosTypeRandom:
      {
         uint numTeams = teamList.getSize();
         if (numTeams > 0)
         {
            uint randomIndex = getRandRange(cSimRand, 0, (numTeams-1));
            getVar(cOutputTeam)->asTeam()->writeVar(teamList[randomIndex]);
            return (true);
         }
         break;
      }
   }

   return (false);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcComparePlayerUnitCount()
{
   uint version = getVersion();
   if (version == 1)
   {
      return (tcComparePlayerUnitCountV1());
   }
   else if (version == 2)
   {
      return (tcComparePlayerUnitCountV2());
   }
   else
   {
      BTRIGGER_ASSERT(false);
      return (false);
   }
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcComparePlayerUnitCountV1()
{
   enum { cPlayer = 1, cObjectType = 2, cCompareType = 3, cCompareCount = 4, };
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());

   int unitCount = 0;
//-- FIXING PREFIX BUG ID 3778
   const BTriggerVarObjectType* pVarObjectType = getVar(cObjectType)->asObjectType();
//--
   if (pVarObjectType->isUsed())
      unitCount = pPlayer->getNumUnitsOfType(pVarObjectType->readVar());
   else
      unitCount = pPlayer->getTotalUnitCount();

   long compareType = getVar(cCompareType)->asOperator()->readVar();
   int compareCount = getVar(cCompareCount)->asInteger()->readVar();
   bool result = BTriggerCondition::compareValues(unitCount, compareType, compareCount);
   return (result);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcComparePlayerUnitCountV2()
{
   enum { cPlayer = 1, cObjectType = 2, cCompareType = 3, cCompareCount = 4, cIncludeTraining = 5, };
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());

   int unitCount = 0;

   bool includeTraining = false;
   if (getVar(cIncludeTraining)->isUsed())
      includeTraining = getVar(includeTraining)->asBool()->readVar();

   BTriggerVarObjectType *pVarObjectType = getVar(cObjectType)->asObjectType();
   if (pVarObjectType->isUsed())
   {
      BObjectTypeID objectTypeID = pVarObjectType->readVar();
      unitCount = pPlayer->getNumUnitsOfType(objectTypeID);
      if (includeTraining)
         unitCount += pPlayer->getFutureUnitCount(objectTypeID);
   }
   else
   {
      unitCount = pPlayer->getTotalUnitCount();
      if (includeTraining)
         unitCount += pPlayer->getTotalFutureUnitCount();
   }

   long compareType = getVar(cCompareType)->asOperator()->readVar();
   int compareCount = getVar(cCompareCount)->asInteger()->readVar();
   bool result = BTriggerCondition::compareValues(unitCount, compareType, compareCount);
   return (result);
}

//==============================================
// Test to see if squad has max number of units
//==============================================
bool BTriggerCondition::tcIsSquadAtMaxSize()
{
   enum
   {
      cSquad = 1,
   };

//-- FIXING PREFIX BUG ID 3780
   const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
   if (pSquad)
   {
      const BProtoSquad* pProtoSquad = pSquad->getProtoSquad();
      if (pProtoSquad)
      {
         return (pSquad->getNumberChildren() == (uint)pProtoSquad->getSquadSize());
      }
   }

   return (false);
}


//==============================================
//==============================================
bool BTriggerCondition::tcCanRetrieveExternalLocation()
{
   enum { cExternalLocation = 1, };
//-- FIXING PREFIX BUG ID 3781
   const BTriggerScriptExternals* pExternals = getParentTriggerScript()->getExternals();
//--
   BVector location;
   if (pExternals->retrieveLocation(location))
   {
      getVar(cExternalLocation)->asVector()->writeVar(location);
      return (true);
   }
   else
   {
      // error.
      return (false);
   }
}


//==============================================
//==============================================
bool BTriggerCondition::tcCanRetrieveExternalLocationList()
{
   enum { cExternalLocationList = 2, };
//-- FIXING PREFIX BUG ID 3782
   const BTriggerScriptExternals* pExternals = getParentTriggerScript()->getExternals();
//--
   BVectorArray locationList;
   if (pExternals->retrieveLocationList(locationList))
   {
      getVar(cExternalLocationList)->asVectorList()->writeVar(locationList);
      return (true);
   }
   else
   {
      // error.
      return (false);
   }
}


//==============================================
//==============================================
bool BTriggerCondition::tcNextKBBase()
{
   enum { cIterator = 1, cOutputNextKBBase = 2, };

   // Get our list var, and assert that it exists (or we're using an uninitialized iterator)
   BTriggerVarIterator *pIterator = getVar(cIterator)->asIterator();
   BTRIGGER_ASSERT(pIterator);
   BTriggerVarID listVarID = pIterator->getListVarID();
   BTriggerVar *pListVar = getParentTriggerScript()->getTriggerVar(listVarID);
   BTRIGGER_ASSERT(pListVar);
   const BKBBaseIDArray& kbBaseList = pListVar->asKBBaseList()->readVar();

   uint numKBBases = kbBaseList.getSize();
   for (uint i=0; i<numKBBases; i++)
   {
      if (!pIterator->isKBBaseVisited(kbBaseList[i]))
      {
         pIterator->visitKBBase(kbBaseList[i]);
         getVar(cOutputNextKBBase)->asKBBase()->writeVar(kbBaseList[i]);
         return (true);
      }
   }

   return (false);
}


//==============================================
//==============================================
bool BTriggerCondition::tcCompareFloat()
{
   enum { cFirstFloat = 1, cCompare = 2, cSecondFloat = 3, };
   float firstFloat = getVar(cFirstFloat)->asFloat()->readVar();
   long op = getVar(cCompare)->asOperator()->readVar();
   float secondFloat = getVar(cSecondFloat)->asFloat()->readVar();
   bool result = compareValues(firstFloat, op, secondFloat);
   return (result);
}

//=================================================================
// Tests if a squad has been under attack within the time interval
//=================================================================
bool BTriggerCondition::tcIsUnderAttack()
{
   bool result = false;
   switch (getVersion())
   {
      case 1:
         result = tcIsUnderAttackV1();
         break;

      case 2:
         result = tcIsUnderAttackV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }

   return (result);
}

//=================================================================
// Tests if a squad has been under attack within the time interval
//=================================================================
bool BTriggerCondition::tcIsUnderAttackV1()
{
   enum
   {
      cSquad = 1,
      cInterval = 2,
   };

//-- FIXING PREFIX BUG ID 3783
   const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
   if (pSquad)
   {
      DWORD lastDamage = pSquad->getLastDamagedTime();
      DWORD interval = getVar(cInterval)->asTime()->readVar();
      DWORD time = gWorld->getGametime();
      if ((lastDamage > 0) && ((time - lastDamage) <= interval))
      {
         return (true);
      }
   }

   return (false);
}

//=================================================================
// Tests if a squad has been under attack within the time interval
//=================================================================
bool BTriggerCondition::tcIsUnderAttackV2()
{
   enum
   {
      cSquad = 1,
      cInterval = 2,
      cSquadList = 3,
   };

   BEntityIDArray squadList;
   if (getVar(cSquadList)->isUsed())
   {
      squadList = getVar(cSquadList)->asSquadList()->readVar();
   }

   if (getVar(cSquad)->isUsed())
   {
      squadList.uniqueAdd(getVar(cSquad)->asSquad()->readVar());
   }

   uint numSquads = squadList.getSize();
   for (uint i = 0; i < numSquads; i++)
   {
//-- FIXING PREFIX BUG ID 3784
      const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
      if (pSquad)
      {
         DWORD lastDamage = pSquad->getLastDamagedTime();
         DWORD interval = getVar(cInterval)->asTime()->readVar();
         DWORD time = gWorld->getGametime();
         if ((lastDamage > 0) && ((time - lastDamage) <= interval))
         {
            return (true);
         }
      }
   }

   return (false);
}

//=================================================================
// Tests to see if the squad has units attached to it
//=================================================================
bool BTriggerCondition::tcHasAttached()
{
   enum
   {
      cSquad = 1,
   };

   bool attached = false;
//-- FIXING PREFIX BUG ID 3786
   const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
   if (pSquad)
   {      
      uint numUnits = pSquad->getNumberChildren();
      for (uint i = 0; i < numUnits; i++)
      {
//-- FIXING PREFIX BUG ID 3785
         const BUnit* pUnit = gWorld->getUnit(pSquad->getChild(i));
//--
         if (pUnit && pUnit->getFlagHasAttached())
         {
            attached = true;
            break;
         }
      }
   }

   return (attached);
}

//=================================================================
// Test to see if the unit or squad is mobile
//=================================================================
bool BTriggerCondition::tcIsMobile()
{
   enum
   {
      cUnit = 1,
      cSquad = 2,
   };

   bool mobile = false;
   if (getVar(cUnit)->isUsed())
   {
//-- FIXING PREFIX BUG ID 3862
      const BUnit* pUnit = gWorld->getUnit(getVar(cUnit)->asUnit()->readVar());
//--
      if (pUnit)
      {
         mobile = pUnit->isMobile();
      }
   }
   else if (getVar(cSquad)->isUsed())
   {
//-- FIXING PREFIX BUG ID 3869
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         mobile = pSquad->isMobile();
      }
   }
   else
   {
      BTRIGGER_ASSERTM(false, "No unit or squad assigned!");
   }

   return (mobile);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCompareProtoSquad()
{
   enum { cFirstProtoSquad = 1, cCompare = 2, cSecondProtoSquad = 3, };
   BProtoSquadID firstProtoSquadID = getVar(cFirstProtoSquad)->asProtoSquad()->readVar();
   long op = getVar(cCompare)->asOperator()->readVar();
   BProtoSquadID secondProtoSquadID = getVar(cSecondProtoSquad)->asProtoSquad()->readVar();
   bool result = compareValues(firstProtoSquadID, op, secondProtoSquadID);
   return (result);
}

//==============================================================================
//==============================================================================
bool BTriggerCondition::tcHasCinematicTagFired()
{
   
   enum { cCinematicTag = 1 };
   
   uint tag = getVar(cCinematicTag)->asCinematicTag()->readVar();
   

   bool result = gWorld->getCinematicManager()->hasTriggerTagFired(0, tag);

   //bool result = false;//compareValues(firstProtoSquadID, op, secondProtoSquadID);
   return (result);
   
}

//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCanGetBuilder()
{
   enum {cUnit = 1, cSquad = 2, cBuilderUnit = 3, cBuilderSquad = 4, };

//-- FIXING PREFIX BUG ID 3868
   const BUnit* pUnit=NULL;
//--
   if (getVar(cUnit)->isUsed())
      pUnit = gWorld->getUnit(getVar(cUnit)->asUnit()->readVar());
   else if (getVar(cSquad)->isUsed())
   {
//-- FIXING PREFIX BUG ID 3872
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad && pSquad->getNumberChildren()>0)
         pUnit = gWorld->getUnit(pSquad->getChild(0));
   }
   if (!pUnit)
      return (false);

   BEntityID builderID = pUnit->getBuiltByUnitID();
   BUnit* pBuilderUnit = gWorld->getUnit(builderID);
   if (!pBuilderUnit)
      return (false);

   if (getVar(cBuilderUnit)->isUsed())
      getVar(cBuilderUnit)->asUnit()->writeVar(builderID);

   if (getVar(cBuilderSquad)->isUsed())
   {
//-- FIXING PREFIX BUG ID 3880
      const BSquad* pBuilderSquad = pBuilderUnit->getParentSquad();
//--
      getVar(cBuilderSquad)->asUnit()->writeVar(pBuilderSquad ? pBuilderSquad->getID() : cInvalidObjectID);
   }

   return (true);
}

//==============================================================================
// BTriggerCondition::tcUIButtonPressed()
//==============================================================================
bool BTriggerCondition::tcUIButtonPressed()
{
   enum { cUIButton=1, cX=2, cY=3, cAnalog=4, cActionModifier=5, cSpeedModifier=6 };

//-- FIXING PREFIX BUG ID 3882
   const BTriggerVarUIButton* pButton=getVar(cUIButton)->asUIButton();
//--

   bool retval = (pButton->readResult() == BTriggerVarUIButton::cUIButtonResultPressed);

   if (retval && getVersion()>1)
   {
      if (getVar(cX)->asFloat()->isUsed())
         getVar(cX)->asFloat()->writeVar(pButton->readX());
      if (getVar(cY)->asFloat()->isUsed())
         getVar(cY)->asFloat()->writeVar(pButton->readY());
      if (getVar(cAnalog)->asFloat()->isUsed())
         getVar(cAnalog)->asFloat()->writeVar(pButton->readAnalog());
      if (getVar(cActionModifier)->asBool()->isUsed())
         getVar(cActionModifier)->asBool()->writeVar(pButton->readActionModifier());
      if (getVar(cSpeedModifier)->asBool()->isUsed())
         getVar(cSpeedModifier)->asBool()->writeVar(pButton->readSpeedModifier());
   }

   return (retval);
}

//==============================================================================
// BTriggerCondition::tcUIButtonWaiting
//==============================================================================
bool BTriggerCondition::tcUIButtonWaiting()
{
   enum { cUIButton = 1, };
   return (getVar(cUIButton)->asUIButton()->readResult() == BTriggerVarUIButton::cUIButtonResultWaiting);
}

//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCompareAIMissionType()
{
   enum { cAIMissionID = 1, cAIMissionType = 2, };
   BAIMissionID missionID = getVar(cAIMissionID)->asInteger()->readVar();
   BAIMissionType missionType = getVar(cAIMissionType)->asMissionType()->readVar();
   const BAIMission* pAIMission = gWorld->getAIMission(missionID);
   if (!pAIMission)
      return (false);
   return (pAIMission->isType(missionType));
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCompareAIMissionState()
{
   enum { cAIMissionID = 1, cAIMissionState = 2, };
   BAIMissionID missionID = getVar(cAIMissionID)->asInteger()->readVar();
   BAIMissionState missionState = getVar(cAIMissionState)->asMissionState()->readVar();
   const BAIMission* pAIMission = gWorld->getAIMission(missionID);
   if (!pAIMission)
      return (false);
   return (pAIMission->isState(missionState));
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCompareAIMissionTargetType()
{
   enum { cAIMissionTargetID = 1, cAIMissionTargetType = 2, };
   BAIMissionTargetID missionTargetID = getVar(cAIMissionTargetID)->asInteger()->readVar();
   BAIMissionTargetType missionTargetType = getVar(cAIMissionTargetType)->asMissionTargetType()->readVar();
   const BAIMissionTarget* pAIMissionTarget = gWorld->getAIMissionTarget(missionTargetID);
   if (!pAIMissionTarget)
      return (false);
   return (pAIMissionTarget->isTargetType(missionTargetType));
}

//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCanGetOneInteger()
{
   enum { cIntegerList = 1, cListPosition = 2, cFoundInteger = 3, };
   const BInt32Array& integerList = getVar(cIntegerList)->asIntegerList()->readVar();
   uint listSize = integerList.getSize();
   if (listSize == 0)
      return (false);

   long listPos = getVar(cListPosition)->asListPosition()->readVar();
   switch (listPos)
   {
   case BTriggerVarListPosition::cListPosTypeFirst:
      {
         getVar(cFoundInteger)->asInteger()->writeVar(integerList[0]);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeLast:
      {
         getVar(cFoundInteger)->asInteger()->writeVar(integerList[listSize-1]);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeRandom:
      {
         uint randomIndex = getRandRange(cSimRand, 0, (listSize-1));
         getVar(cFoundInteger)->asInteger()->writeVar(integerList[randomIndex]);
         return (true);
      }
   }

   return (false);  
}

//==============================================================================
// Get correct version
//==============================================================================
bool BTriggerCondition::tcCanGetUnitsAlongRay()
{
   bool result = false;
   switch (getVersion())
   {
      case 1:
         result = tcCanGetUnitsAlongRayV1();
         break;

      case 2:
         result = tcCanGetUnitsAlongRayV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }

   return (result);
}

//==============================================================================
// See if any units are along a ray
//==============================================================================
bool BTriggerCondition::tcCanGetUnitsAlongRayV1()
{
   enum
   {
      inStartLoc = 1,
      inEndLoc = 2,
      inRadius = 3,
      outUnitList = 4,
   };

   BVector start = getVar(inStartLoc)->asVector()->readVar();
   BVector end = getVar(inEndLoc)->asVector()->readVar();
   bool useRadius = getVar(inRadius)->isUsed();
   float rayRadius = 0.0f;

   if (useRadius)
   {
      rayRadius = getVar(inRadius)->asFloat()->readVar();
   }

   if (rayRadius <= 0.0f)
   {
      useRadius = false;
   }

   ATGCollision::OrientedBox obb;
   BVector min = start;
   BVector max = end;
   BVector forward = XMVectorSubtract(end, start);
   float halfLength = forward.length() * 0.5f;
   forward.normalize();      
   if (useRadius)
   {
      min -= rayRadius;
      max += rayRadius;

      // Calculate center point      
      BVector center = XMVectorAdd(start, (forward * halfLength));

      // Calculate pitch
      bool invert = (forward.y < 0.0f);
      BVector forwardTemp = invert ? -forward : forward;
      forwardTemp.normalize();
      float pitch = Math::fFastACos(forwardTemp.dot(cYAxisVector));            
      pitch = RIGHT_ANGLE_RADIANS - pitch;
      pitch = (forwardTemp.z > 0.0f) ? -pitch : pitch;      

      // Calculate yaw
      invert = (forward.z < 0.0f);
      forwardTemp = invert ? -forward : forward;
      forwardTemp.y = 0.0f;
      forwardTemp.normalize();
      float yaw = Math::fFastACos(forwardTemp.dot(cZAxisVector));
      yaw = (forwardTemp.x > 0.0f) ? yaw : -yaw;

      // Calculate matrix
      BMatrix mat;      
      mat.makeRotateYawPitchRoll(yaw, pitch, 0.0f);
      mat.multTranslate(center.x, center.y, center.z);

      // Calculate extents
      BVector ext(rayRadius, rayRadius, halfLength);      

      // Setup OBB
      obb.Center.x = center.x;
      obb.Center.y = center.y;
      obb.Center.z = center.z;
      obb.Extents.x = ext.x;
      obb.Extents.y = ext.y;
      obb.Extents.z = ext.z;
      BQuaternion rot(mat);
      obb.Orientation.x = rot.x;
      obb.Orientation.y = rot.y;
      obb.Orientation.z = rot.z;
      obb.Orientation.w = rot.w;

      //XXXHalwes - 7/2/2207 - Debug
      //gpDebugPrimitives->addDebugBox(mat, ext, cDWORDRed);      
   }

   //XXXHalwes - 7/2/2207 - Debug
   //gpDebugPrimitives->addDebugLine(start, end, cDWORDGreen, cDWORDCyan);

   // Query units in area of ray
   BEntityIDArray results(0, 100);
   BUnitQuery query(min, max);
   gWorld->getUnitsInArea(&query, &results);

   // Resulting units
   BEntityIDArray unitList;
   unitList.clear();

   // Iterate through resulting units and test against ray
   uint numUnits = (uint)results.getNumber();
   for (uint i = 0; i < numUnits; i++)
   {
//-- FIXING PREFIX BUG ID 3873
      const BUnit* pUnit = gWorld->getUnit(results[i]);
//--
      if (pUnit)
      {
         const BBoundingBox* unitBB = pUnit->getSimBoundingBox();
         if (useRadius)
         {
            ATGCollision::AxisAlignedBox aabb;
            const BVector unitBBCenter = unitBB->getCenter();
            aabb.Center.x = unitBBCenter.x;
            aabb.Center.y = unitBBCenter.y;
            aabb.Center.z = unitBBCenter.z;
            const float* unitBBExt = unitBB->getExtents();
            aabb.Extents.x = unitBBExt[0];
            aabb.Extents.y = unitBBExt[1];
            aabb.Extents.z = unitBBExt[2];
            if (ATGCollision::IntersectAxisAlignedBoxOrientedBox(&aabb, &obb))
            {
               unitList.uniqueAdd(results[i]);
            }
         }
         else
         {
            float intersectDistSqr = 0.0f;
            if (unitBB->raySegmentIntersects(start, forward, true, NULL, intersectDistSqr))
            {
               unitList.uniqueAdd(results[i]);
            }
         }
      }
   }   

   if (getVar(outUnitList)->isUsed())
   {
      getVar(outUnitList)->asUnitList()->writeVar(unitList);
   }

   if (unitList.getNumber() > 0 )
   {
      return (true);
   }
   else
   {
      return (false);
   }
}

//==============================================================================
// See if any units are along a ray
//==============================================================================
bool BTriggerCondition::tcCanGetUnitsAlongRayV2()
{
   enum
   {
      inStartLoc = 1,
      inEndLoc = 2,
      inRadius = 3,
      outUnitList = 4,
      inSortFlag = 5,
      inReverseSortFlag = 6,
      inDebugColor = 7,
   };

   BVector start = getVar(inStartLoc)->asVector()->readVar();
   BVector end = getVar(inEndLoc)->asVector()->readVar();
   bool useRadius = getVar(inRadius)->isUsed();
   float rayRadius = 0.0f;

   if (useRadius)
   {
      rayRadius = getVar(inRadius)->asFloat()->readVar();
   }

   if (rayRadius <= 0.0f)
   {
      useRadius = false;
   }

   ATGCollision::OrientedBox obb;
   BVector min = start;
   BVector max = end;
   BVector forward = XMVectorSubtract(end, start);
   float halfLength = forward.length() * 0.5f;
   forward.normalize();      
   if (useRadius)
   {
      min -= rayRadius;
      max += rayRadius;

      // Calculate center point      
      BVector center = XMVectorAdd(start, (forward * halfLength));

      // Calculate pitch
      bool invert = (forward.y < 0.0f);
      BVector forwardTemp = invert ? -forward : forward;
      forwardTemp.normalize();
      float pitch = Math::fFastACos(forwardTemp.dot(cYAxisVector));            
      pitch = RIGHT_ANGLE_RADIANS - pitch;
      pitch = (forwardTemp.z > 0.0f) ? -pitch : pitch;      

      // Calculate yaw
      invert = (forward.z < 0.0f);
      forwardTemp = invert ? -forward : forward;
      forwardTemp.y = 0.0f;
      forwardTemp.normalize();
      float yaw = Math::fFastACos(forwardTemp.dot(cZAxisVector));
      yaw = (forwardTemp.x > 0.0f) ? yaw : -yaw;

      // Calculate matrix
      BMatrix mat;      
      mat.makeRotateYawPitchRoll(yaw, pitch, 0.0f);
      mat.multTranslate(center.x, center.y, center.z);

      // Calculate extents
      BVector ext(rayRadius, rayRadius, halfLength);      

      // Setup OBB
      obb.Center.x = center.x;
      obb.Center.y = center.y;
      obb.Center.z = center.z;
      obb.Extents.x = ext.x;
      obb.Extents.y = ext.y;
      obb.Extents.z = ext.z;
      BQuaternion rot(mat);
      obb.Orientation.x = rot.x;
      obb.Orientation.y = rot.y;
      obb.Orientation.z = rot.z;
      obb.Orientation.w = rot.w;

      //XXXHalwes - 9/7/2007 - Remove this once the particle draw bug gets fixed.
      DWORD debugColor = cDWORDRed;
      if (getVar(inDebugColor)->isUsed())
      {
         BColor tempColor = getVar(inDebugColor)->asColor()->readVar();
         debugColor = tempColor.asDWORD();
         gpDebugPrimitives->addDebugBox(mat, ext, debugColor);
         //XXXHalwes - 6/25/2208 - Debug
         //gpDebugPrimitives->addDebugBox(min, max, debugColor);
      }      
      //XXXHalwes - 7/2/2207 - Debug
      //gpDebugPrimitives->addDebugBox(mat, ext, cDWORDRed);      
   }

   DWORD debugColor = cDWORDGreen;
   if (getVar(inDebugColor)->isUsed())
   {
      BColor tempColor = getVar(inDebugColor)->asColor()->readVar();
      debugColor = tempColor.asDWORD();
      gpDebugPrimitives->addDebugLine(start, end, debugColor, debugColor);
   }      
   //XXXHalwes - 7/2/2207 - Debug
   //gpDebugPrimitives->addDebugLine(start, end, cDWORDGreen, cDWORDGreen);   

   // Query units in area of ray
   const long maxUnits = gWorld->getNumberUnits();
   BEntityIDArray results(0, maxUnits);
   BUnitQuery query(min, max);
   gWorld->getUnitsInArea(&query, &results);

   // Resulting units
   BEntityIDArray unitList;
   unitList.clear();

   // Iterate through resulting units and test against ray
   uint numUnits = results.getSize();
   for (uint i = 0; i < numUnits; i++)
   {
//-- FIXING PREFIX BUG ID 3877
      const BUnit* pUnit = gWorld->getUnit(results[i]);
//--
      if (pUnit)
      {
         const BBoundingBox* unitBB = pUnit->getSimBoundingBox();
         if (useRadius)
         {
            ATGCollision::AxisAlignedBox aabb;
            const BVector unitBBCenter = unitBB->getCenter();
            aabb.Center.x = unitBBCenter.x;
            aabb.Center.y = unitBBCenter.y;
            aabb.Center.z = unitBBCenter.z;
            const float* unitBBExt = unitBB->getExtents();
            aabb.Extents.x = unitBBExt[0];
            aabb.Extents.y = unitBBExt[1];
            aabb.Extents.z = unitBBExt[2];
            if (ATGCollision::IntersectAxisAlignedBoxOrientedBox(&aabb, &obb))
            {
               unitList.uniqueAdd(results[i]);
            }
         }
         else
         {
            float intersectDistSqr = 0.0f;
            if (unitBB->raySegmentIntersects(start, forward, true, NULL, intersectDistSqr))
            {
               unitList.uniqueAdd(results[i]);
            }
         }
      }
   }      

   // Sort the found units
   if (getVar(inSortFlag)->isUsed())
   {
      BEntityIDArray sortedUnits = unitList;
      BSmallDynamicSimArray<float> distList;
      BVector testLoc = getVar(inReverseSortFlag)->isUsed() ? end : start;
      // Calculate distances      
      int numUnits = sortedUnits.getNumber();
      for (int i = (numUnits - 1); i >= 0; i--)
      {
//-- FIXING PREFIX BUG ID 3881
         const BUnit* pUnit = gWorld->getUnit(sortedUnits[i]);
//--
         if (pUnit)
         {
            float dist = pUnit->calculateXZDistanceSqr(testLoc);
            distList.insert(0, dist);
         }
         else
         {
            sortedUnits.removeIndex(i);   
         }
      }

      // Sort from lower to higher distances
      BSmallDynamicSimArray<float> indexMap = distList;
      distList.sort();

      // Organize unit list to match sorted distance list
      unitList.clear();
      uint numDists = distList.getSize();
      uint numSortUnits = sortedUnits.getSize();
      for (uint i = 0; i < numDists; i++)
      {
         for (uint j = 0; j < numSortUnits; j++)
         {
            if (indexMap[j] == distList[i])
            {
               unitList.add(sortedUnits[j]);
               break;
            }
         }
      }
   }

   if (getVar(outUnitList)->isUsed())
   {
      getVar(outUnitList)->asUnitList()->writeVar(unitList);
   }

   if (unitList.getSize() > 0 )
   {
      return (true);
   }
   else
   {
      return (false);
   }
}

//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCanRemoveOneInteger()
{
   enum { cIntegerList = 1, cListPosition = 2, cFoundInteger = 3, };
   BInt32Array& integerList = getVar(cIntegerList)->asIntegerList()->readVar();
   uint listSize = integerList.getSize();
   if (listSize == 0)
      return (false);

   long listPos = getVar(cListPosition)->asListPosition()->readVar();
   switch (listPos)
   {
   case BTriggerVarListPosition::cListPosTypeFirst:
      {
         getVar(cFoundInteger)->asInteger()->writeVar(integerList[0]);
         integerList.removeIndex(0);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeLast:
      {
         getVar(cFoundInteger)->asInteger()->writeVar(integerList[listSize-1]);
         integerList.removeIndex(listSize-1);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeRandom:
      {
         // Note random doesn't care about maintaining order.
         uint randomIndex = getRandRange(cSimRand, 0, (listSize-1));
         getVar(cFoundInteger)->asInteger()->writeVar(integerList[randomIndex]);
         integerList.removeIndex(randomIndex, false);
         return (true);
      }
   }

   return (false);  
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCanGetOneLocation()
{
   enum { cLocationList = 1, cListPosition = 2, cFoundLocation = 3, };
   const BLocationArray& locationList = getVar(cLocationList)->asVectorList()->readVar();
   uint listSize = locationList.getSize();
   if (listSize == 0)
      return (false);

   long listPos = getVar(cListPosition)->asListPosition()->readVar();
   switch (listPos)
   {
   case BTriggerVarListPosition::cListPosTypeFirst:
      {
         getVar(cFoundLocation)->asVector()->writeVar(locationList[0]);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeLast:
      {
         getVar(cFoundLocation)->asVector()->writeVar(locationList[listSize-1]);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeRandom:
      {
         uint randomIndex = getRandRange(cSimRand, 0, (listSize-1));
         getVar(cFoundLocation)->asVector()->writeVar(locationList[randomIndex]);
         return (true);
      }
   }

   return (false);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCanRemoveOneLocation()
{
   enum { cLocationList = 1, cListPosition = 2, cRemovedLocation = 3, };
   BLocationArray& locationList = getVar(cLocationList)->asVectorList()->readVar();
   uint listSize = locationList.getSize();
   if (listSize == 0)
      return (false);

   long listPos = getVar(cListPosition)->asListPosition()->readVar();
   switch (listPos)
   {
   case BTriggerVarListPosition::cListPosTypeFirst:
      {
         getVar(cRemovedLocation)->asVector()->writeVar(locationList[0]);
         locationList.removeIndex(0);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeLast:
      {
         getVar(cRemovedLocation)->asVector()->writeVar(locationList[listSize-1]);
         locationList.removeIndex(listSize-1);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeRandom:
      {
         uint randomIndex = getRandRange(cSimRand, 0, (listSize-1));
         getVar(cRemovedLocation)->asVector()->writeVar(locationList[randomIndex]);
         locationList.removeIndex(randomIndex, false);
         return (true);
      }
   }

   return (false);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCompareProtoObject()
{
   enum { cFirstProtoObject = 1, cCompareType = 2, cSecondProtoObject = 3, };
   BProtoObjectID first = getVar(cFirstProtoObject)->asProtoObject()->readVar();
   long compareType = getVar(cCompareType)->asOperator()->readVar();
   BProtoObjectID second = getVar(cSecondProtoObject)->asProtoObject()->readVar();
   bool result = BTriggerCondition::compareValues(first, compareType, second);
   return (result);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCompareTech()
{
   enum { cFirstTech = 1, cCompareType = 2, cSecondTech = 3, };
   BProtoTechID first = getVar(cFirstTech)->asTech()->readVar();
   long compareType = getVar(cCompareType)->asOperator()->readVar();
   BProtoTechID second = getVar(cSecondTech)->asTech()->readVar();
   bool result = BTriggerCondition::compareValues(first, compareType, second);
   return (result);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcProtoObjectListContains()
{
   enum { cProtoObjectList = 1, cProtoObject = 2, };
   const BProtoObjectIDArray& protoObjectList = getVar(cProtoObjectList)->asProtoObjectList()->readVar();
   BProtoObjectID protoObjectID = getVar(cProtoObject)->asProtoObject()->readVar();
   return (protoObjectList.contains(protoObjectID));
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcProtoSquadListContains()
{
   enum { cProtoSquadList = 1, cProtoSquad = 2, };
   const BProtoSquadIDArray& protoSquadList = getVar(cProtoSquadList)->asProtoSquadList()->readVar();
   BProtoSquadID protoSquadID = getVar(cProtoSquad)->asProtoSquad()->readVar();
   return (protoSquadList.contains(protoSquadID));
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcTechListContains()
{
   enum { cTechList = 1, cTech = 2, };
   const BTechIDArray& techList = getVar(cTechList)->asTechList()->readVar();
   BProtoTechID techID = getVar(cTech)->asTech()->readVar();
   return (techList.contains(techID));
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCanRemoveOneProtoObject()
{
   enum { cList = 1, cListPosition = 2, cRemoved = 3, };
   BProtoObjectIDArray& listVar = getVar(cList)->asProtoObjectList()->readVar();
   uint listSize = listVar.getSize();
   if (listSize == 0)
      return (false);

   long listPos = getVar(cListPosition)->asListPosition()->readVar();
   switch (listPos)
   {
   case BTriggerVarListPosition::cListPosTypeFirst:
      {
         getVar(cRemoved)->asProtoObject()->writeVar(listVar[0]);
         listVar.removeIndex(0);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeLast:
      {
         getVar(cRemoved)->asProtoObject()->writeVar(listVar[listSize-1]);
         listVar.removeIndex(listSize-1);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeRandom:
      {
         uint randomIndex = getRandRange(cSimRand, 0, (listSize-1));
         getVar(cRemoved)->asProtoObject()->writeVar(listVar[randomIndex]);
         listVar.removeIndex(randomIndex, false);
         return (true);
      }
   }

   return (false);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCanRemoveOneProtoSquad()
{
   enum { cList = 1, cListPosition = 2, cRemoved = 3, };
   BProtoSquadIDArray& listVar = getVar(cList)->asProtoSquadList()->readVar();
   uint listSize = listVar.getSize();
   if (listSize == 0)
      return (false);

   long listPos = getVar(cListPosition)->asListPosition()->readVar();
   switch (listPos)
   {
   case BTriggerVarListPosition::cListPosTypeFirst:
      {
         getVar(cRemoved)->asProtoSquad()->writeVar(listVar[0]);
         listVar.removeIndex(0);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeLast:
      {
         getVar(cRemoved)->asProtoSquad()->writeVar(listVar[listSize-1]);
         listVar.removeIndex(listSize-1);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeRandom:
      {
         uint randomIndex = getRandRange(cSimRand, 0, (listSize-1));
         getVar(cRemoved)->asProtoSquad()->writeVar(listVar[randomIndex]);
         listVar.removeIndex(randomIndex, false);
         return (true);
      }
   }

   return (false);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCanRemoveOneTech()
{
   enum { cList = 1, cListPosition = 2, cRemoved = 3, };
   BTechIDArray& listVar = getVar(cList)->asTechList()->readVar();
   uint listSize = listVar.getSize();
   if (listSize == 0)
      return (false);

   long listPos = getVar(cListPosition)->asListPosition()->readVar();
   switch (listPos)
   {
   case BTriggerVarListPosition::cListPosTypeFirst:
      {
         getVar(cRemoved)->asTech()->writeVar(listVar[0]);
         listVar.removeIndex(0);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeLast:
      {
         getVar(cRemoved)->asTech()->writeVar(listVar[listSize-1]);
         listVar.removeIndex(listSize-1);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeRandom:
      {
         uint randomIndex = getRandRange(cSimRand, 0, (listSize-1));
         getVar(cRemoved)->asTech()->writeVar(listVar[randomIndex]);
         listVar.removeIndex(randomIndex, false);
         return (true);
      }
   }

   return (false);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcBidState()
{
   enum { cBidID = 1, cBidState = 2, };
   BBidState bidState = getVar(cBidState)->asBidState()->readVar();
   const BBid* pBid = gWorld->getBid(getVar(cBidID)->asInteger()->readVar());
   if (pBid)
      return (bidState == pBid->getState());
   else
      return (bidState == BidState::cInvalid);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcBuildingCommandDone()
{
   enum { cBuildingCommandState=1, cSquad=2, cSquadList=3, };

//-- FIXING PREFIX BUG ID 3887
   const BTriggerVarBuildingCommandState* pState=getVar(cBuildingCommandState)->asBuildingCommandState();
//--

   bool retval = (pState->readResult() == BTriggerVarBuildingCommandState::cResultDone);

   BTriggerVarSquad* pVarSquad = getVar(cSquad)->asSquad();
   if (pVarSquad->isUsed())
   {
      pVarSquad->writeVar(cInvalidObjectID);
      const BEntityIDArray& squads = pState->readSquads();
      if (squads.getSize() > 0)
      {
         BEntityID squadID = squads[squads.getSize()-1];
         if (gWorld->getSquad(squadID))
            pVarSquad->writeVar(squadID);
      }
   }

   BTriggerVarSquadList* pVarSquadList = getVar(cSquadList)->asSquadList();
   if (pVarSquadList->isUsed())
   {
      pVarSquadList->clear();
      const BEntityIDArray& squads = pState->readSquads();
      uint numSquads = squads.getSize();
      for (uint i=0; i<numSquads; i++)
      {
         if (gWorld->getSquad(squads[i]))
            pVarSquadList->addSquad(squads[i]);
      }
   }

   return (retval);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcAITopicIsActive()
{
   enum { cTopicID = 1, };
   const BAITopic* pAITopic = gWorld->getAITopic(getVar(cTopicID)->asInteger()->readVar());
   if (!pAITopic)
      return (false);
   return (pAITopic->getFlagActive());
}

//==============================================================================
//==============================================================================
bool BTriggerCondition::tcAITopicGetTickets()
{
   enum { cTopicID = 1, cCountOut = 2, };
   const BAITopic* pAITopic = gWorld->getAITopic(getVar(cTopicID)->asInteger()->readVar());
   if (!pAITopic)
      return (false);

   if (getVar(cCountOut)->isUsed())
      getVar(cCountOut)->asInteger()->writeVar(pAITopic->getCurrentTickets());

   return (true);
}

//==============================================================================
// Call correct version
//==============================================================================
bool BTriggerCondition::tcHasGarrisoned()
{
   bool result = false;

   switch (getVersion())
   {
      case 1:
         result = tcHasGarrisonedV1();
         break;

      case 2:
         result = tcHasGarrisonedV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }

   return (result);
}

//==============================================================================
// Test if the squad has any garrisoned squads
//==============================================================================
bool BTriggerCondition::tcHasGarrisonedV1()
{
   enum
   {
      cSquad = 1,
   };

   bool result = false;
   BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
   if (pSquad)
   {
      BEntityIDArray garrisonedSquads = pSquad->getGarrisonedSquads();
      result = (garrisonedSquads.getSize() > 0);
   }

   return (result);
}

//==============================================================================
// Test if the squad has any garrisoned squads
//==============================================================================
bool BTriggerCondition::tcHasGarrisonedV2()
{
   enum
   {
      cSquad = 1,
      cOutputNumGarrisoned = 2,
   };

   bool result = false;
   int numGarrisonedSquads = 0;
   BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
   if (pSquad)
   {
      BEntityIDArray garrisonedSquads = pSquad->getGarrisonedSquads();
      numGarrisonedSquads = garrisonedSquads.getSize();
      result = (numGarrisonedSquads > 0);
   }

   if (getVar(cOutputNumGarrisoned)->isUsed())
   {
      getVar(cOutputNumGarrisoned)->asInteger()->writeVar(numGarrisonedSquads);
   }

   return (result);
}

//==============================================================================
// Test if the squad is a garrisoned squad
//==============================================================================
bool BTriggerCondition::tcIsGarrisoned()
{
   bool result = false;

   switch (getVersion())
   {
      case 1:
         result = tcIsGarrisonedV1();
         break;

      case 2:
         result = tcIsGarrisonedV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }

   return (result);
}

//==============================================================================
// Test if the squad is a garrisoned squad
//==============================================================================
bool BTriggerCondition::tcIsGarrisonedV1()
{
   enum
   {
      cSquad = 1,
   };

   bool result = false;
//-- FIXING PREFIX BUG ID 3879
   const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
   if (pSquad)
   {
      result = pSquad->getFlagGarrisoned();
   }

   return (result);
}

//==============================================================================
// Test to see if any of the squad(s) is(are) garrisoned
//==============================================================================
bool BTriggerCondition::tcIsGarrisonedV2()
{
   enum
   {
      cInSquad = 1,
      cInSquadList = 2,
   };

   bool result = false;
   BEntityIDArray squads;
   if (getVar(cInSquadList)->isUsed())
   {
      squads = getVar(cInSquadList)->asSquadList()->readVar();
   }

   if (getVar(cInSquad)->isUsed())
   {
      squads.uniqueAdd(getVar(cInSquad)->asSquad()->readVar());
   }

   uint numSquads = squads.getSize();
   for (uint i = 0; i < numSquads; i++)
   {
//-- FIXING PREFIX BUG ID 3889
      const BSquad* pSquad = gWorld->getSquad(squads[i]);
//--
      if (pSquad && pSquad->getFlagGarrisoned())
      {
         result = true;
         break;
      }
   }

   return (result);
}

//==============================================================================
// Test if the squad is an attached squad
//==============================================================================
bool BTriggerCondition::tcIsAttached()
{
   enum
   {
      cSquad = 1,
   };

   bool result = false;
//-- FIXING PREFIX BUG ID 3890
   const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
   if (pSquad)
   {
      result = pSquad->getFlagAttached();
   }

   return (result);
}

//==============================================================================
//==============================================================================
bool BTriggerCondition::tcComparePlayerSquadCount()
{
   enum { cPlayer = 1, cProtoSquad = 2, cCompareType = 3, cCompareCount = 4, cIncludeTraining = 5, };
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayer)->asPlayer()->readVar());

   int squadCount = 0;

   bool includeTraining = false;
   if (getVar(cIncludeTraining)->isUsed())
      includeTraining = getVar(cIncludeTraining)->asBool()->readVar();

   BTriggerVarProtoSquad *pVarProtoSquad = getVar(cProtoSquad)->asProtoSquad();
   if (pVarProtoSquad->isUsed())
   {
      BProtoSquadID protoSquadID = pVarProtoSquad->readVar();
      squadCount = pPlayer->getNumSquadsOfType(protoSquadID);
      if (includeTraining)
         squadCount += pPlayer->getFutureSquadCount(protoSquadID);
   }
   else
   {
      squadCount = pPlayer->getTotalSquadCount();
      if (includeTraining)
         squadCount += pPlayer->getTotalFutureSquadCount();
   }

   long compareType = getVar(cCompareType)->asOperator()->readVar();
   int compareCount = getVar(cCompareCount)->asInteger()->readVar();
   bool result = BTriggerCondition::compareValues(squadCount, compareType, compareCount);
   return (result);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcIsPassable()
{
   enum { cSquad = 1, cSquadList = 2, cTestLocation = 3, };
//-- FIXING PREFIX BUG ID 3893
   const BTriggerVarSquad* pVarSquad = getVar(cSquad)->asSquad();
//--
//-- FIXING PREFIX BUG ID 3894
   const BTriggerVarSquadList* pVarSquadList = getVar(cSquadList)->asSquadList();
//--

   BEntityIDArray testSquads;
   if (pVarSquadList->isUsed())
   {
      testSquads = pVarSquadList->readVar(); 
   }
   if (pVarSquad->isUsed())
   {
      BEntityID squadID = pVarSquad->readVar();
      if (gWorld->getSquad(squadID))
         testSquads.uniqueAdd(squadID);
   }

   uint numSquads = testSquads.getSize();
   if (numSquads == 0)
      return (false);

   BVector testLocation = getVar(cTestLocation)->asVector()->readVar();
   for (uint i=0; i<numSquads; i++)
   {
      const BSquad* pSquad = gWorld->getSquad(testSquads[i]);
      if (!pSquad)
         return (false);
      if (!pSquad->isPassable(testLocation, testSquads))
         return (false);
   }

   return (true);
}


//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCanGetCentroid()
{
   enum { cSquadList = 1, cCentroid = 2, };
   const BEntityIDArray& squadList = getVar(cSquadList)->asSquadList()->readVar();
   uint numSquads = squadList.getSize();
   BVector centroid = cOriginVector;
   float divisor = 0.0f;

   for (uint i=0; i<numSquads; i++)
   {
      const BSquad* pSquad = gWorld->getSquad(squadList[i]);
      if (pSquad)
      {
         centroid += pSquad->getPosition();
         divisor += 1.0f;
      }
   }

   if (divisor > 0.0f)
   {
      centroid /= divisor;
      getVar(cCentroid)->asVector()->writeVar(centroid);
      return (true);
   }
   else
   {
      getVar(cCentroid)->asVector()->writeVar(cOriginVector);
      return (false);
   }
}

//==============================================================================
//==============================================================================
bool BTriggerCondition::tcPlayerIsPrimaryUser()
{
   // Halwes - 4/22/2008 - Next scan remove this.
   //enum { cPlayerID = 1, };
   //const long playerID = getVar(cPlayerID)->asPlayer()->readVar();
   //const BPlayer* pPrimary = gUserManager.getUser(BUserManager::cPrimaryUser)->getPlayer();
   //BTRIGGER_ASSERT(pPrimary);
   //const BAI* pAI = pPrimary->getAI();
   //BTRIGGER_ASSERT(pAI);

   //const BPlayerID primaryID = pAI->getPlayerID();

   //// Safety check 
   //if (gConfig.isDefined("AllowPrimaryUser"))
   //   return (playerID == primaryID);
   //else
      return (false);
}

//==============================================================================
//==============================================================================
bool BTriggerCondition::tcIsConfigDefined()
{
   enum { cConfigName = 1, };
   BSimUString configNameU = getVar(cConfigName)->asString()->readVar();
   if (configNameU.isEmpty())
      return (false);

   BSimString buffer;
   const char* pConfigName = configNameU.asANSI(buffer);
   if (!pConfigName)
      return (false);

   bool configDefined = gConfig.isDefined(pConfigName);
   return (configDefined);   
}

//==============================================================================
// Determine if the current game is a co-op game or not
//==============================================================================
bool BTriggerCondition::tcIsCoop()
{
   return (gWorld->getFlagCoop());
}

//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCustomCommandCheck()
{
   enum { cCommandID = 1, cCountOut=2, };

   int commandID = getVar(cCommandID)->asInteger()->readVar();

//-- FIXING PREFIX BUG ID 3788
   const BCustomCommand* pCustomCommand=gWorld->getCustomCommand(commandID);
//--
   if (!pCustomCommand)
   {
      if (commandID >=0 && commandID < gWorld->getNextCustomCommandID())
      {
         // Assume the command is finished if it is gone but is a valid ID (usually because it's not persistent).
         if (getVar(cCountOut)->isUsed())
            getVar(cCountOut)->asInteger()->writeVar(1);
         return true;
      }
      else
      {
         if (getVar(cCountOut)->isUsed())
            getVar(cCountOut)->asInteger()->writeVar(0);
         return false;
      }
   }

   if (pCustomCommand->mFinishedCount > 0)
   {
      if (getVar(cCountOut)->isUsed())
         getVar(cCountOut)->asInteger()->writeVar(pCustomCommand->mFinishedCount);
      return true;
   }

   if (getVar(cCountOut)->isUsed())
      getVar(cCountOut)->asInteger()->writeVar(0);
   return false;
}

//==============================================================================
// Check to see if the object type matches the input
//==============================================================================
bool BTriggerCondition::tcIsObjectType()
{
   bool result = false;
   switch (getVersion())
   {
      case 1:
         result = tcIsObjectTypeV1();
         break;

      case 2:
         result = tcIsObjectTypeV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }

   return (result);
}

//==============================================================================
// Check to see if the object type matches the input
//==============================================================================
bool BTriggerCondition::tcIsObjectTypeV1()
{
   enum 
   { 
      cSquad = 1, 
      cObjectType = 2, 
   };

   BEntityID squadID = getVar(cSquad)->asSquad()->readVar();
   int objectType = getVar(cObjectType)->asObjectType()->readVar();

//-- FIXING PREFIX BUG ID 3789
   const BSquad* pSquad = gWorld->getSquad(squadID);
//--
   if (pSquad)
   {
      const BProtoObject* pProtoObject=pSquad->getProtoObject();
      if (pProtoObject)
         return (pProtoObject->isType(objectType));
   }

   return (false);
}

//==============================================================================
// Check to see if all of the the inputs match the object type
//==============================================================================
bool BTriggerCondition::tcIsObjectTypeV2()
{
   enum 
   { 
      cInSquad = 1, 
      cInObjectType = 2, 
      cInUnit = 3,
      cInObject = 4,
      cInProtoObject = 5,
   };

   bool oneUsed = false;
   bool allMatch = true;
   int objectType = getVar(cInObjectType)->asObjectType()->readVar();

   if (getVar(cInUnit)->isUsed())
   {
      oneUsed = true;
//-- FIXING PREFIX BUG ID 3790
      const BUnit* pUnit = gWorld->getUnit(getVar(cInUnit)->asUnit()->readVar());
//--
      if (pUnit)
      {
         const BProtoObject* pProtoObject = pUnit->getProtoObject();
         if (pProtoObject)
         {
            allMatch = pProtoObject->isType(objectType);
         }
      }
   }

   if (allMatch && getVar(cInSquad)->isUsed())
   {
      oneUsed = true;
//-- FIXING PREFIX BUG ID 3791
      const BSquad* pSquad = gWorld->getSquad(getVar(cInSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         const BProtoObject* pProtoObject = pSquad->getProtoObject();
         if (pProtoObject)
         {
            allMatch = pProtoObject->isType(objectType);
         }
      }
   }

   if (allMatch && getVar(cInObject)->isUsed())
   {
      oneUsed = true;
//-- FIXING PREFIX BUG ID 3792
      const BObject* pObject = gWorld->getObject(getVar(cInObject)->asObject()->readVar());
//--
      if (pObject)
      {
         const BProtoObject* pProtoObject = pObject->getProtoObject();
         if (pProtoObject)
         {
            allMatch = pProtoObject->isType(objectType);
         }
      }
   }

   if (allMatch && getVar(cInProtoObject)->isUsed())
   {
      oneUsed = true;
      // TODO - do we need to get the player specific one here?
      const BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(getVar(cInProtoObject)->asProtoObject()->readVar());
      if (pProtoObject)
      {
         allMatch = pProtoObject->isType(objectType);
      }
   }

   return (oneUsed ? allMatch : oneUsed);
}

//==============================================================================
// Check for OK result
//==============================================================================
bool BTriggerCondition::tcUISquadListOK()
{
   enum 
   { 
      cInputUISquadList = 1, 
      cOutputSquadList = 2, 
   };

//-- FIXING PREFIX BUG ID 3793
   const BTriggerVarUISquadList* pVar = getVar(cInputUISquadList)->asUISquadList();
//--
   if (pVar->readResult() == BTriggerVarUISquadList::cUISquadListResultOK)
   {
      getVar(cOutputSquadList)->asSquadList()->writeVar(pVar->readSquadList());
      return (true);
   }
   else
      return (false);
}

//==============================================================================
// Check for cancel result
//==============================================================================
bool BTriggerCondition::tcUISquadListCancel()
{
   enum 
   { 
      cInputUISquadList = 1, 
   };

//-- FIXING PREFIX BUG ID 3794
   const BTriggerVarUISquadList* pVar = getVar(cInputUISquadList)->asUISquadList();
//--
   if (pVar->readResult() == BTriggerVarUISquadList::cUISquadListResultCancel)
      return (true);
   else
      return (false);
}

//==============================================================================
// Check for lock error result
//==============================================================================
bool BTriggerCondition::tcUISquadListUILockError()
{
   enum 
   { 
      cUISquadList = 1, 
   };

   long uiSquadListResult = getVar(cUISquadList)->asUISquadList()->readResult();
   return (uiSquadListResult == BTriggerVarUISquadList::cUISquadListResultUILockError);
}

//==============================================================================
// Check if waiting on result
//==============================================================================
bool BTriggerCondition::tcUISquadListWaiting()
{
   enum 
   { 
      cUISquadList = 1, 
   };

   long uiSquadListResult = getVar(cUISquadList)->asUISquadList()->readResult();
   return (uiSquadListResult == BTriggerVarUISquadList::cUISquadListResultWaiting);
}

//==============================================================================
bool BTriggerCondition::tcIsTimerDone()
{
   enum { cTimerID=1, cCurrentTime=2 };

   bool useTimeOut = getVar(cCurrentTime)->isUsed();

   int timerID = getVar(cTimerID)->asInteger()->readVar();
   
   DWORD currentTime = 0;
   BGameTimer* pTimer = gTimerManager.getTimer(timerID);
   if (pTimer)
      currentTime = pTimer->getCurrentTime();

   // give our time back to the caller
   if (useTimeOut)
      getVar(cCurrentTime)->asTime()->writeVar(currentTime);

   // has this timer found and expired?
   if (pTimer && pTimer->isDone())
      return true;

   // timer is not done.
   return false;
}

//==============================================================================
//==============================================================================
bool BTriggerCondition::tcNextKBSquad()
{
   enum { cIterator = 1, cOutputNextKBSquad = 2, };

   // Get our list var, and assert that it exists (or we're using an uninitialized iterator)
   BTriggerVarIterator *pIterator = getVar(cIterator)->asIterator();
   BTRIGGER_ASSERT(pIterator);
   BTriggerVarID listVarID = pIterator->getListVarID();
   BTriggerVar *pListVar = getParentTriggerScript()->getTriggerVar(listVarID);
   BTRIGGER_ASSERT(pListVar);
   const BKBSquadIDArray& KBSquadList = pListVar->asKBSquadList()->readVar();

   uint numKBSquads = KBSquadList.getSize();
   for (uint i=0; i<numKBSquads; i++)
   {
      if (!pIterator->isKBSquadVisited(KBSquadList[i]))
      {
         pIterator->visitKBSquad(KBSquadList[i]);
         getVar(cOutputNextKBSquad)->asKBSquad()->writeVar(KBSquadList[i]);
         return (true);
      }
   }

   return (false);
}

//==============================================================================
// Test if and what squad is attacking
//==============================================================================
bool BTriggerCondition::tcIsAttacking()
{
   bool result = false;
   switch (getVersion())
   {
      case 2:
         result = tcIsAttackingV2();
         break;

      case 3:
         result = tcIsAttackingV3();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }

   return (result);
}

//==============================================================================
// Test if and what squad is attacking
//==============================================================================
bool BTriggerCondition::tcIsAttackingV2()
{
   enum
   {
      cInputSquad = 1,
      cOutputUnit = 2,
      cOutputSquad = 3
   };

   bool result = false;
//-- FIXING PREFIX BUG ID 3891
   const BSquad* pSquad = gWorld->getSquad(getVar(cInputSquad)->asSquad()->readVar());
//--
   if (pSquad)
   {
//-- FIXING PREFIX BUG ID 3797
      const BSquadActionAttack* pAttackAction = reinterpret_cast<const BSquadActionAttack*>(pSquad->getActionByTypeConst(BAction::cActionTypeSquadAttack));
//--
      if (pAttackAction && (pAttackAction->getState() == BAction::cStateAttacking))
      {
         if (getVar(cOutputUnit)->isUsed() && pAttackAction->getTarget())
            getVar(cOutputUnit)->asUnit()->writeVar(pAttackAction->getTarget()->getID());

         if (getVar(cOutputSquad)->isUsed() && pAttackAction->getTarget())
         {
            BEntity *pEntity = gWorld->getEntity(pAttackAction->getTarget()->getID());
            if (pEntity && pEntity->getUnit() && pEntity->getUnit()->getParentSquad())               
               getVar(cOutputSquad)->asSquad()->writeVar( pEntity->getUnit()->getParentSquad()->getID());
         }

         result = true;
      }
   }

   return (result);
}

//==============================================================================
// Test if any and what squads are being attacked
//==============================================================================
bool BTriggerCondition::tcIsAttackingV3()
{
   enum
   {
      cInputSquad = 1,
      cOutputUnit = 2,
      cOutputSquad = 3,
      cInputSquadList = 4,
      cOutputUnitList = 5,
      cOutputSquadList = 6,
   };

   bool result = false;
   BEntityIDArray attackingSquads;
   BEntityIDArray targetedSquads;
   BEntityIDArray targetedUnits;
   // Collect attacking squads
   if (getVar(cInputSquadList)->isUsed())
   {
      attackingSquads = getVar(cInputSquadList)->asSquadList()->readVar();
   }

   if (getVar(cInputSquad)->isUsed())
   {
      attackingSquads.uniqueAdd(getVar(cInputSquad)->asSquad()->readVar());
   }

   // Iterate through attacking squads
   uint numAttackSquads = attackingSquads.getSize();
   for (uint i = 0; i < numAttackSquads; i++)
   {
//-- FIXING PREFIX BUG ID 3757
      const BSquad* pSquad = gWorld->getSquad(attackingSquads[i]);
//--
      if (pSquad)
      {
         // Is the squad attacking
         const BSquadActionAttack* pAttackAction = reinterpret_cast<const BSquadActionAttack*>(pSquad->getActionByTypeConst(BAction::cActionTypeSquadAttack));
         if (pAttackAction && (pAttackAction->getState() == BAction::cStateAttacking))
         {
            // Valid target
            const BSimTarget* pTarget = pAttackAction->getTarget();
            if (pTarget && pTarget->isIDValid())
            {
               // Unit target
               BEntityID targetID = pTarget->getID();
               uint idType = targetID.getType();
               if (idType == BEntity::cClassTypeUnit)
               {
                  BUnit* pTargetUnit = gWorld->getUnit(targetID);
                  if (pTargetUnit)
                  {
                     BSquad* pTargetSquad = pTargetUnit->getParentSquad();
                     if (pTargetSquad)
                     {
                        targetedUnits.uniqueAdd(targetID);
                        targetedSquads.uniqueAdd(pTargetSquad->getID());
                     }
                  }
               }
               // Squad target
               else if (idType == BEntity::cClassTypeSquad)
               {
                  BSquad* pTargetSquad = gWorld->getSquad(targetID);
                  if (pTargetSquad)
                  {                     
                     bool validChild = false;
                     uint numChildren = pTargetSquad->getNumberChildren();
                     for (uint j = 0; j < numChildren; j++)
                     {
                        BEntityID targetUnitID = pTargetSquad->getChild(j);
                        BUnit* pTargetUnit = gWorld->getUnit(targetUnitID);
                        if (pTargetUnit)
                        {
                           targetedUnits.uniqueAdd(targetUnitID);
                           validChild = true;
                        }
                     }

                     if (validChild)
                     {
                        targetedSquads.uniqueAdd(targetID);
                     }
                  }
               }               
            }
            else if (pTarget && pTarget->isPositionValid())
            {
               result = true;
            }
         }
      }
   }

   // Set result
   uint numTargetedUnits = targetedUnits.getSize();
   uint numTargetedSquads = targetedSquads.getSize();
   if ((numTargetedUnits > 0) && (numTargetedSquads > 0))
   {
      result = true;
   }

   // Assign unit outputs   
   if (getVar(cOutputUnit)->isUsed())
   {
      BEntityID targetedUnitID = (numTargetedUnits > 0) ? targetedUnits[0] : cInvalidObjectID;
      getVar(cOutputUnit)->asUnit()->writeVar(targetedUnitID);
   }

   if (getVar(cOutputUnitList)->isUsed())
   {
      getVar(cOutputUnitList)->asUnitList()->writeVar(targetedUnits);  
   }

   // Assign squad outputs
   if (getVar(cOutputSquad)->isUsed())
   {
      BEntityID targetedSquadID = (numTargetedSquads > 0) ? targetedSquads[0] : cInvalidObjectID;
      getVar(cOutputSquad)->asSquad()->writeVar(targetedSquadID);
   }

   if (getVar(cOutputSquadList)->isUsed())
   {
      getVar(cOutputSquadList)->asSquadList()->writeVar(targetedSquads);
   }

   return (result);
}

//==============================================================================
// Test to see if you can get an attacking unit's projectile launch location
//==============================================================================
bool BTriggerCondition::tcCanGetUnitLaunchLocation()
{
   enum
   {
      cInputUnit = 1,
      cOutputLocation = 2,
   };

   bool result = false;
   BVector launchLoc = cInvalidVector;
//-- FIXING PREFIX BUG ID 3883
   const BUnit* pUnit = gWorld->getUnit(getVar(cInputUnit)->asUnit()->readVar());
//--
   if (pUnit)
   {
//-- FIXING PREFIX BUG ID 3799
      const BUnitActionRangedAttack* pAttackAction = reinterpret_cast<const BUnitActionRangedAttack*>(pUnit->getActionByTypeConst(BAction::cActionTypeUnitRangedAttack));
//--
      if (pAttackAction)
      {
         launchLoc = pAttackAction->getLaunchLocation();
         if (launchLoc != cInvalidVector)
         {
            result = true;
         }
      }
      else
      {
         const BVisual* pVisual = pUnit->getVisual();
         if (pVisual)
         {
            long launchPointHandle = pVisual->getNextPointHandle(cActionAnimationTrack, -1, cVisualPointLaunch);
            if (launchPointHandle != -1)
            {               
               BVector launchPoint;
               BMatrix launchPointMatrix;
               if (pVisual->getPointPosition(cActionAnimationTrack, launchPointHandle, launchPoint, &launchPointMatrix))
               {
                  BMatrix mat;
                  pUnit->getWorldMatrix(mat);
                  BMatrix wsLaunchPointMatrix;
                  wsLaunchPointMatrix.mult(launchPointMatrix, mat);

                  wsLaunchPointMatrix.getTranslation(launchLoc);
                  if (launchLoc != cInvalidVector)
                  {
                     result = true;
                  }
               }
            }
         }            
      }
   }

   getVar(cOutputLocation)->asVector()->writeVar(launchLoc);

   return (result);
}

//=====================================================================================================
// Checks if a unit or squad is gathering
//=====================================================================================================
bool BTriggerCondition::tcIsGathering()
{
   enum
   {
      cUnit = 1,
      cSquad = 2,
   };

   if (getVar(cUnit)->isUsed())
   {
//-- FIXING PREFIX BUG ID 3800
      const BUnit* pUnit = gWorld->getUnit(getVar(cUnit)->asUnit()->readVar());
//--
      if (pUnit)
      {
         return (pUnit->isGathering());
      }
   }
   else if (getVar(cSquad)->isUsed())
   {      
//-- FIXING PREFIX BUG ID 3802
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         //return (pSquad->isGathering());
         uint numTargetChildren = pSquad->getNumberChildren();
         for (uint i = 0; i < numTargetChildren; i++)
         {
//-- FIXING PREFIX BUG ID 3801
            const BUnit* pSubUnit = gWorld->getUnit(pSquad->getChild(i));
//--
            if (pSubUnit != NULL)
            {
               if (pSubUnit->isGathering())
               {
                  return true;
               }
            }
         }
      }
   }

   return (false);
}

//=====================================================================================================
// Checks if a unit or squad is capturing
//=====================================================================================================
bool BTriggerCondition::tcIsCapturing()
{
   enum
   {
      cUnit = 1,
      cSquad = 2,
   };

   if (getVar(cUnit)->isUsed())
   {
//-- FIXING PREFIX BUG ID 3803
      const BUnit* pUnit = gWorld->getUnit(getVar(cUnit)->asUnit()->readVar());
//--
      if (pUnit)
      {
         return (pUnit->isCapturing());
      }
   }
   else if (getVar(cSquad)->isUsed())
   {      
//-- FIXING PREFIX BUG ID 3805
      const BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         //return (pSquad->isCapturing());

         uint numTargetChildren = pSquad->getNumberChildren();
         for (uint i = 0; i < numTargetChildren; i++)
         {
//-- FIXING PREFIX BUG ID 3804
            const BUnit* pSubUnit = gWorld->getUnit(pSquad->getChild(i));
//--
            if (pSubUnit != NULL)
            {
               if (pSubUnit->isCapturing())
               {
                  return true;
               }
            }
         }

      }
   }

   return (false);
}

//=====================================================================================================
// Check to see if the player is a computer AI
//=====================================================================================================
bool BTriggerCondition::tcPlayerIsComputerAI()
{
   enum
   {
      cInputPlayer = 1,
   };

   BPlayerID playerID = getVar(cInputPlayer)->asPlayer()->readVar();
//-- FIXING PREFIX BUG ID 3806
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
//--
   if (pPlayer)
   {
      return (pPlayer->isComputerAI());
   }
   else
   {
      return (false);
   }
}

//================================================================================
// Find the greatest threat for a designated squad out of the squads attacking it
//================================================================================
bool BTriggerCondition::tcCanGetGreatestThreat()
{
   enum
   {
      cTargetSquad = 1,
      cThreatSquad = 2,
   };
   
   BEntityID resultSquad = cInvalidObjectID;
//-- FIXING PREFIX BUG ID 3811
   const BSquad* pTargetSquad = gWorld->getSquad(getVar(cTargetSquad)->asSquad()->readVar());
//--
   if (pTargetSquad)
   {
      // Collect the squads attacking the target squad
      BEntityIDArray attackingSquads;
      uint numTargetChildren = pTargetSquad->getNumberChildren();
      for (uint i = 0; i < numTargetChildren; i++)
      {
         BUnit* pTargetUnit = gWorld->getUnit(pTargetSquad->getChild(i));
         if (!pTargetUnit)
            continue;

//-- FIXING PREFIX BUG ID 3808
         const BUnitActionUnderAttack* pUnitActionUnderAttack = reinterpret_cast<BUnitActionUnderAttack*>(pTargetUnit->getActionByType(BAction::cActionTypeUnitUnderAttack));
//--
         if (!pUnitActionUnderAttack)
            continue;

         uint numAttackingUnits = pUnitActionUnderAttack->getNumberAttackingUnits();
         for (uint j = 0; j < numAttackingUnits; j++)
         {
            BUnit* pAttackingUnit = gWorld->getUnit(pUnitActionUnderAttack->getAttackingUnitByIndex(j));
            if (!pAttackingUnit)
               continue;

//-- FIXING PREFIX BUG ID 3807
            const BSquad* pAttackingSquad = pAttackingUnit->getParentSquad();
//--
            if (!pAttackingSquad)
               continue;

            attackingSquads.uniqueAdd(pAttackingSquad->getID());
         }
      }

      // Find squad with highest DPS against the target squad
      float attackRating = 0.0f;      
      uint numAttackingSquads = attackingSquads.getSize();
      for (uint i = 0; i < numAttackingSquads; i++)
      {
//-- FIXING PREFIX BUG ID 3809
         const BSquad* pAttackingSquad = gWorld->getSquad(attackingSquads[i]);
//--
         if (!pAttackingSquad)
            continue;

         float attackCompare = pAttackingSquad->getAttackRating();
         if (attackCompare > attackRating)
         {
            attackRating = attackCompare;
            resultSquad = attackingSquads[i];
         }
      }
   }

   getVar(cThreatSquad)->asSquad()->writeVar(resultSquad);

   return ((pTargetSquad && (resultSquad != cInvalidObjectID)) ? true : false);
}

//==============================================================================
// Get the targeted squad if one exists
//==============================================================================
bool BTriggerCondition::tcCanGetTargetedSquad()
{
   enum
   {
      cInputAttackingSquad = 1,
      cInputTargetedSquad = 2,
   };

   BEntityID resultSquad = cInvalidObjectID;
   BSquad* pAttackingSquad = gWorld->getSquad(getVar(cInputAttackingSquad)->asSquad()->readVar());
   if (pAttackingSquad)
   {
      const BSimTarget* pSimTarget = NULL;
//-- FIXING PREFIX BUG ID 3814
      const BSquadActionAttack* pAttackAction = reinterpret_cast<BSquadActionAttack*>(pAttackingSquad->getActionByType(BAction::cActionTypeSquadAttack));
//--
      if (pAttackAction)
      {
         pSimTarget = pAttackAction->getTarget();
      }
      
      if (pSimTarget && pSimTarget->isIDValid())
      {
         BEntityID targetID = pSimTarget->getID();
         uint targetClassType = targetID.getType();
         if (targetClassType == BEntity::cClassTypeUnit)
         {
//-- FIXING PREFIX BUG ID 3884
            const BUnit* pUnit = gWorld->getUnit(targetID);
//--
            if (pUnit)
            {
//-- FIXING PREFIX BUG ID 3812
               const BSquad* pSquad = pUnit->getParentSquad();
//--
               if (pSquad)
               {
                  resultSquad = pSquad->getID();
               }               
            }
         }
         else if (targetClassType == BEntity::cClassTypeSquad)
         {
//-- FIXING PREFIX BUG ID 3813
            const BSquad* pSquad = gWorld->getSquad(targetID);
//--
            if (pSquad)
            {
               resultSquad = targetID;
            }
         }
      }            
   }

   getVar(cInputTargetedSquad)->asSquad()->writeVar(resultSquad);

   return ((pAttackingSquad && (resultSquad != cInvalidObjectID)) ? true : false);
}

//================================================================
// Check to see if a specific objective has been set to completed
//================================================================
bool BTriggerCondition::tcIsObjectiveComplete()
{
   enum
   {
      cInputObjective = 1,
   };

   return (gWorld->getObjectiveManager()->getObjectiveCompleted(getVar(cInputObjective)->asObjective()->readVar()));
}

//================================================================
// Compare two vectors
//================================================================
bool BTriggerCondition::tcCompareVector()
{
   enum
   {
      cInputVectorA = 1,
      cInputVectorB = 2,
      cInputEpsilon = 3,

   };
   
   BVector locA = getVar(cInputVectorA)->asVector()->readVar();
   BVector locB = getVar(cInputVectorB)->asVector()->readVar();

   if(getVar(cInputEpsilon)->isUsed())
   {
      
      return locA.almostEqual(locB, getVar(cInputEpsilon)->asFloat()->readVar());
   }
   else
   {
      return locA.almostEqual(locB);
   }
}


//================================================================
// tcGetTableRow
//================================================================
bool BTriggerCondition::tcGetTableRow()
{
   enum
   {
      cTableID = 1,
      cRow = 2,
      cUserClassType = 3,
      cUserClassData = 4,
   };
  
   int tableID = getVar(cTableID)->asInteger()->readVar();
   int rowID = getVar(cRow)->asInteger()->readVar();
   int userClassType = getVar(cUserClassType)->asUserClassType()->readVar();

   if(tableID == -1)
   {
      return false;
   }

   BTriggerUserTableXML* pTable = gTriggerManager.getTableManager()->mTablesByID[tableID];
   
   
   BTriggerUserClass* pUserClass = gTriggerManager.getUserClassManager()->mTriggerUserClasses.get(userClassType);

   uint numVarsRequiringAutoFixUp = 0;
   if(pUserClass && pTable)
   {
      if(rowID >= pTable->getNumRows())
         return false;
      BXMLNode row = pTable->getRow(rowID);    
      for(uint i=0; i<pUserClass->mFields.size(); i++)
      {
         BTriggerUserField* field = pUserClass->mFields[i];         
         getParentTriggerScript()->writeVar(getVar(cUserClassData + 1 + i), field->mType, row.getChild(i), numVarsRequiringAutoFixUp);
      }
      return true;
   }
   return false;
}

//================================================================
// tcCheckPop
//================================================================
bool BTriggerCondition::tcCheckPop()
{
   enum
   {
      cInputPlayer = 5,
      cInputProtoSquad = 1,
      cInputCheckBids = 2,
      cInputBonusLimit = 3,
      cOutputNumCanTrain = 4,

   };

   bool willfit = true;
   
   long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
   {
      return false;
   }

   long pSquadID = getVar(cInputProtoSquad)->asProtoSquad()->readVar();   
//-- FIXING PREFIX BUG ID 3818
   const BProtoSquad* pSquad = pPlayer->getProtoSquad(pSquadID);
//--
   if(!pSquad)
   {
      return false;
   }

//-- FIXING PREFIX BUG ID 3819
   const BTriggerVarBool* pCheckBids = getVar(cInputCheckBids)->asBool();
//--
//-- FIXING PREFIX BUG ID 3820
   const BTriggerVarInteger* pBonusLimit = getVar(cInputBonusLimit)->asInteger();
//--
   BTriggerVarInteger* pNumCanTrain = getVar(cOutputNumCanTrain)->asInteger();

   bool checkbids = false;
   int bonusLimit = 0;

   if(pCheckBids->isUsed())
   {
      checkbids = pCheckBids->readVar();
   }
   if(pBonusLimit->isUsed())
   {
      bonusLimit = pBonusLimit->readVar();
   }

   int squadsBiddedOn = 0; 
   if(checkbids)
   {
      BAI* pAI = gWorld->getAI(playerID);
      if (pAI)
      {
         BBidManager* pBidManager = pAI->getBidManager();
         if (pBidManager)
         {
            const BBidIDArray& currentBids = pBidManager->getBidIDs();
            uint numBids = currentBids.getSize();
            for (uint i=0; i<numBids; i++)
            {
               BBidID bidID = currentBids[i];
               const BBid* pBid = gWorld->getBid(bidID);
               if (!pBid)
                  continue;
               if(pBid->isType(BidType::cSquad))
               {
                  if(pSquadID == pBid->getSquadToBuy())
                  {
                     squadsBiddedOn++;
                  }
               }
            }
         }
      }
   }

   BPopArray pops;
   pSquad->getPops(pops);  
   
   int numCanTrain = 10000; //some big number

   for(uint i=0; i < pops.getSize(); i++)
   {
      BPop pop = pops.get(i);
      
      long id = pop.mID;
      float squadCount = pop.mCount;

      if(squadCount == 0)
         continue;

      float cap = pPlayer->getPopCap(id);
      float max = pPlayer->getPopMax(id);
      float future = pPlayer->getPopFuture(id);
      float count = pPlayer->getPopCount(id);

      //bonus limit is to allow us to overbid
      cap += bonusLimit;
      max += bonusLimit;

      float usedpop = count + future + (squadCount * squadsBiddedOn);
      float newtotal = squadCount + usedpop;

      if(newtotal <= cap && newtotal <= max) 
      {
         int numWeCouldBuild = Math::FloatToIntTrunc((cap - usedpop) / squadCount);

         if(numWeCouldBuild < numCanTrain)
         {
            numCanTrain = numWeCouldBuild;
         }
      }
      else
      {
         willfit = false;
         numCanTrain = 0;
      }   
   }

   if(willfit)
   {
      pNumCanTrain->writeVar(numCanTrain);
   }

   return willfit;
}

//==============================================================================
// Test to see if a particular user is the normal mode
//==============================================================================
bool BTriggerCondition::tcIsUserModeNormal()
{
   enum
   {
      cInputPlayer = 1,
   };

   BTRIGGER_ASSERT(mbAsyncCondition);
   BTRIGGER_ASSERT(mbLocalAsyncMachine);
   bool result = false;
   BPlayerID playerID = getVar(cInputPlayer)->asPlayer()->readVar();
//-- FIXING PREFIX BUG ID 3821
   const BUser* pUser = gUserManager.getUserByPlayerID(playerID);
//--
   if (pUser)         
   {
      result = (pUser->getUserMode() == BUser::cUserModeNormal);
   }
   return (result);
}

//==============================================================================
// Test to see if the squad or unit is hitched to another squad or unit
//==============================================================================
bool BTriggerCondition::tcIsHitched()
{
   enum
   {
      cSquad = 2,
      cHitchedToSquad = 3,
   };

   bool hitched = false;
   BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
   if (pSquad)
   {
      hitched = pSquad->isHitched();

      if (hitched && getVar(cHitchedToSquad)->isUsed())
      {
         getVar(cHitchedToSquad)->asSquad()->writeVar(pSquad->getHitchedToSquad());
      }
   }

   return (hitched);
}

//==============================================================================
// See if the squad has a squad hitched to it
//==============================================================================
bool BTriggerCondition::tcHasHitched()
{
   enum
   {
      cSquad = 1,
      cHitchedSquad = 2,
   };

   bool hasHitched = false;
   BEntityID hitchedSquadID = cInvalidObjectID;
   BSquad* pSquad = gWorld->getSquad(getVar(cSquad)->asSquad()->readVar());
   if (pSquad)
   {
      hitchedSquadID = pSquad->getHitchedSquad();
//-- FIXING PREFIX BUG ID 3822
      const BSquad* pHitchedSquad = gWorld->getSquad(hitchedSquadID);
//--
      if (pHitchedSquad)
      {
         hasHitched = true;
      }      
   }

   if (getVar(cHitchedSquad)->isUsed())
   {
      getVar(cHitchedSquad)->asSquad()->writeVar(hitchedSquadID);
   }

   return (hasHitched);
}

//==============================================================================
// Test the distance between two squads
//==============================================================================
bool BTriggerCondition::tcSquadSquadDistance()
{
   enum 
   { 
      cInputSquad1 = 1, 
      cInputSquad2 = 2, 
      cInputOperator = 3, 
      cInputDistance = 4, 
   };

   BEntityID squad1ID = getVar(cInputSquad1)->asSquad()->readVar();
   BEntityID squad2ID = getVar(cInputSquad2)->asSquad()->readVar();
   long op = getVar(cInputOperator)->asOperator()->readVar();
   float compareDistSq = getVar(cInputDistance)->asFloat()->readVar();
   compareDistSq *= compareDistSq;

   BSquad* pSquad1 = gWorld->getSquad(squad1ID);
   if (!pSquad1)
      return (false);
   BSquad* pSquad2 = gWorld->getSquad(squad2ID);
   if (!pSquad2)
      return (false);

   BVector squad1AvgPos = pSquad1->getAveragePosition();
   BVector squad2AvgPos = pSquad2->getAveragePosition();
   float distSq = squad1AvgPos.xzDistanceSqr(squad2AvgPos);

   bool result = compareValues(distSq, op, compareDistSq);
   return (result);
}

//===========================================================================================
// Test to see if a ProtoSquad is in the training queue for a particular player and/or team
//===========================================================================================
bool BTriggerCondition::tcIsInQueue()
{
   enum
   {
      cProtoSquad = 1,
      cPlayer = 2,
      cTeam = 3,      
   };

   BPlayerID playerID = getVar(cPlayer)->isUsed() ? getVar(cPlayer)->asPlayer()->readVar() : cInvalidPlayerID;
   BTeamID teamID = getVar(cTeam)->isUsed() ? getVar(cTeam)->asTeam()->readVar() : cInvalidTeamID;   
   BProtoSquadID protoSquadID = getVar(cProtoSquad)->asProtoSquad()->readVar();

   if ((playerID == cInvalidPlayerID) && (teamID == cInvalidTeamID))
   {
      BTRIGGER_ASSERTM(false, "No PlayerID or TeamID designated!");
      return (false);
   }

   BEntityHandle h = cInvalidObjectID;
//-- FIXING PREFIX BUG ID 3885
   const BUnit* pUnit = gWorld->getNextUnit(h);
//--
   while (pUnit)
   {
      // Alive?
      if (!pUnit->isAlive())
      {
         pUnit = gWorld->getNextUnit(h);
         continue;
      }

      // Player match?
      if ((playerID != cInvalidPlayerID) && (pUnit->getPlayerID() != playerID))
      {
         pUnit = gWorld->getNextUnit(h);
         continue;
      }

      // Team match?
      if ((teamID != cInvalidTeamID) && (pUnit->getTeamID() != teamID))
      {
         pUnit = gWorld->getNextUnit(h);
         continue;
      }

      // Valid proto object?
      const BProtoObject* pProtoObject = pUnit->getProtoObject();      
      if (pProtoObject)
      {
         // Does this unit have commands?
         uint numCommands = pProtoObject->getNumberCommands();
         for (uint i = 0; i < numCommands; i++)
         {
            // Are any of the commands TrainSquad commands?
            BProtoObjectCommand protoObjectCommand = pProtoObject->getCommand(i);
            if (protoObjectCommand.getType() == BProtoObjectCommand::cTypeTrainSquad)
            {
               // Now see if the unit has a build action for the ProtoObject in question
//-- FIXING PREFIX BUG ID 3823
               const BUnitActionBuilding* pUnitActionBuilding = reinterpret_cast<const BUnitActionBuilding*>(pUnit->getActionByTypeConst(BAction::cActionTypeUnitBuilding));
//--
               if (pUnitActionBuilding && (pUnitActionBuilding->getTrainCount(playerID, protoSquadID) != 0))
               {
                  return (true);
               }
            }
         }
      }

      pUnit = gWorld->getNextUnit(h);
   }

   return (false);
}

//==============================================================================
//==============================================================================
bool BTriggerCondition::tcEventTriggered()
{
   enum { cEventID=1, };

   int subscriberID = getVar(cEventID)->asInteger()->readVar();

   BGeneralEventSubscriber* pSubscriber =  gGeneralEventManager.getSubscriber(subscriberID);
   if(pSubscriber == NULL)
      return false;

   return (pSubscriber->hasFired() ||
           (pSubscriber->useCount() && (pSubscriber->mFiredCount > 0)));
}

//==============================================================================
// Choose correct version
//==============================================================================
bool BTriggerCondition::tcCanGetObjects()
{
   bool result = false;
   switch (getVersion())
   {
      case 1:
         result = tcCanGetObjectsV1();
         break;

      case 2:
         result = tcCanGetObjectsV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }

   return (result);
}

//==============================================================================
// Detect objects in the world based on search criteria
//==============================================================================
bool BTriggerCondition::tcCanGetObjectsV1()
{
   enum 
   { 
      cInputLocation = 1, 
      cInputDistance = 2, 
      cInputPlayer = 3, 
      cInputProtoObject = 4, 
      cInputFilterList = 5, 
      cOutputObjectList = 6, 
      cOutputCount = 7, 
   };

   int numObjects = 0;
   bool usePlayer = getVar(cInputPlayer)->isUsed();
   bool useProtoObject = getVar(cInputProtoObject)->isUsed();
   bool useLocation = getVar(cInputLocation)->isUsed();
   bool useDistance = getVar(cInputDistance)->isUsed();
   bool useOutputObjectList = getVar(cOutputObjectList)->isUsed();
   bool useOutputCount = getVar(cOutputCount)->isUsed();
   bool useFilterList = getVar(cInputFilterList)->isUsed();
   bool onlyCountObjects = (!useFilterList && !useOutputObjectList); // if I'm not attempting to filter or use the output list, then simply count the # of objects
   BEntityIDArray workingList(0,50);
   BEntityIDArray filteredResults(0,50);
//-- FIXING PREFIX BUG ID 3824
   const BEntityIDArray* pResults = &workingList;
//--

   // Populate our list of objects to filter.
   BPlayerID playerID = usePlayer ? getVar(cInputPlayer)->asPlayer()->readVar() : cInvalidPlayerID;
   BProtoObjectID protoObjectID = useProtoObject ? getVar(cInputProtoObject)->asProtoObject()->readVar() : cInvalidProtoObjectID;
   BEntityHandle h = cInvalidObjectID;
//-- FIXING PREFIX BUG ID 3825
   const BObject* pObject = gWorld->getNextObject(h);
//--
   while (pObject)
   {
      if (useLocation && useDistance)
      {
         BVector loc = getVar(cInputLocation)->asVector()->readVar();
         float dist = getVar(cInputDistance)->asFloat()->readVar();
         if (loc.xzDistanceSqr(pObject->getPosition()) > (dist * dist))
         {
            pObject = gWorld->getNextObject(h);
            continue;
         }
      }

      if (usePlayer && (pObject->getPlayerID() != playerID))
      {
         pObject = gWorld->getNextObject(h);
         continue;
      }

      if (useProtoObject && (pObject->getProtoID() != protoObjectID))
      {
         pObject = gWorld->getNextObject(h);
         continue;
      }

      // if I'm only here to count objects but I don't care about
      // the exact number then return now
      if (onlyCountObjects && !useOutputCount)
         return (true);
      else if (!onlyCountObjects)
         workingList.add(pObject->getID());

      ++numObjects;
      pObject = gWorld->getNextObject(h);
   }

   if (useFilterList)
   {
      const BEntityIDArray& filterList = getVar(cInputFilterList)->asObjectList()->readVar();
      uint numInWorkingList = workingList.getSize();
      for (uint i = 0; i < numInWorkingList; i++)
      {
         BEntityID entityID = workingList[i];
         if (filterList.find(entityID) != cInvalidIndex)
         {
            filteredResults.add(entityID);
            continue;
         }
      }
      numObjects = filteredResults.getNumber();
      pResults = &filteredResults;
   }

   if (useOutputCount)
   {
      getVar(cOutputCount)->asInteger()->writeVar(numObjects);
   }

   if (useOutputObjectList)
   {
      getVar(cOutputObjectList)->asObjectList()->writeVar(*pResults);
   }

   return (numObjects > 0);
}

//==============================================================================
// Detect objects in the world based on search criteria
//==============================================================================
bool BTriggerCondition::tcCanGetObjectsV2()
{
   enum 
   { 
      cInputLocation = 1, 
      cInputDistance = 2, 
      cInputPlayer = 3, 
      cInputProtoObject = 4, 
      cInputFilterList = 5, 
      cOutputObjectList = 6, 
      cOutputCount = 7, 
      cInputPlayerList = 8,
   };

   int numObjects = 0;
   bool usePlayer = getVar(cInputPlayer)->isUsed();
   bool usePlayerList = getVar(cInputPlayerList)->isUsed();
   bool useProtoObject = getVar(cInputProtoObject)->isUsed();
   bool useLocation = getVar(cInputLocation)->isUsed();
   bool useDistance = getVar(cInputDistance)->isUsed();
   bool useOutputObjectList = getVar(cOutputObjectList)->isUsed();
   bool useOutputCount = getVar(cOutputCount)->isUsed();
   bool useFilterList = getVar(cInputFilterList)->isUsed();
   bool onlyCountObjects = (!useFilterList && !useOutputObjectList); // if I'm not attempting to filter or use the output list, then simply count the # of objects
   BEntityIDArray workingList(0,50);
   BEntityIDArray filteredResults(0,50);
//-- FIXING PREFIX BUG ID 3826
   const BEntityIDArray* pResults = &workingList;
//--

   // Populate our list of objects to filter.
   BPlayerIDArray playerIDs;
   if (usePlayerList)
   {
      playerIDs = getVar(cInputPlayerList)->asPlayerList()->readVar();
   }

   if (usePlayer)
   {
      playerIDs.uniqueAdd(getVar(cInputPlayer)->asPlayer()->readVar());
   }

   BProtoObjectID protoObjectID = useProtoObject ? getVar(cInputProtoObject)->asProtoObject()->readVar() : cInvalidProtoObjectID;
   BEntityHandle h = cInvalidObjectID;
//-- FIXING PREFIX BUG ID 3827
   const BObject* pObject = gWorld->getNextObject(h);
//--
   while (pObject)
   {
      if (useLocation && useDistance)
      {
         BVector loc = getVar(cInputLocation)->asVector()->readVar();
         float dist = getVar(cInputDistance)->asFloat()->readVar();
         if (loc.xzDistanceSqr(pObject->getPosition()) > (dist * dist))
         {
            pObject = gWorld->getNextObject(h);
            continue;
         }
      }

      bool playerMatch = true;
      uint numPlayerIDs = playerIDs.getSize();
      if (numPlayerIDs > 0)
      {
         playerMatch = false;
         for (uint i = 0; i < numPlayerIDs; i++)
         {
            if (pObject->getPlayerID() == playerIDs[i])
            {
               playerMatch = true;
               break;
            }
         }
      }

      if (!playerMatch)
      {
         pObject = gWorld->getNextObject(h);
         continue;
      }

      if (useProtoObject && (pObject->getProtoID() != protoObjectID))
      {
         pObject = gWorld->getNextObject(h);
         continue;
      }

      // if I'm only here to count objects but I don't care about
      // the exact number then return now
      if (onlyCountObjects && !useOutputCount)
         return (true);
      else if (!onlyCountObjects)
         workingList.add(pObject->getID());

      ++numObjects;
      pObject = gWorld->getNextObject(h);
   }

   if (useFilterList)
   {
      const BEntityIDArray& filterList = getVar(cInputFilterList)->asObjectList()->readVar();
      uint numInWorkingList = workingList.getSize();
      for (uint i = 0; i < numInWorkingList; i++)
      {
         BEntityID entityID = workingList[i];
         if (filterList.find(entityID) != cInvalidIndex)
         {
            filteredResults.add(entityID);
            continue;
         }
      }

      numObjects = filteredResults.getNumber();
      pResults = &filteredResults;
   }

   if (useOutputCount)
   {
      getVar(cOutputCount)->asInteger()->writeVar(numObjects);
   }

   if (useOutputObjectList)
   {
      getVar(cOutputObjectList)->asObjectList()->writeVar(*pResults);
   }

   return (numObjects > 0);
}

//==============================================================================
// Return an Object from an ObjectList
//==============================================================================
bool BTriggerCondition::tcCanGetOneObject()
{
   enum 
   { 
      cInputObjectList = 1, 
      cInputListPos = 2,
      cOutputObject = 3,
   };

   BEntityIDArray objectList = getVar(cInputObjectList)->asObjectList()->readVar();
   uint numObjects = objectList.getSize();
   long listPos = getVar(cInputListPos)->asListPosition()->readVar();
//-- FIXING PREFIX BUG ID 3828
   const BObject* pObject = NULL;
//--

   // Get the unit
   switch(listPos)
   {
      // First
      case BTriggerVarListPosition::cListPosTypeFirst:
         {
            if (numObjects > 0)
            {
               pObject = gWorld->getObject(objectList[0]);
            }
            break;
         }

      // Last
      case BTriggerVarListPosition::cListPosTypeLast:
         {            
            if (numObjects > 0)
            {
               pObject = gWorld->getObject(objectList[numObjects - 1]);
            }
            break;
         }

      // Random
      case BTriggerVarListPosition::cListPosTypeRandom:
         {
            if (numObjects > 0)
            {
               uint randomIndex = getRandRange(cSimRand, 0, (numObjects-1));
               pObject = gWorld->getObject(objectList[randomIndex]);
            }
            break;
         }
   }

   // Output the unit
   if (pObject)
   {
      getVar(cOutputObject)->asObject()->writeVar(pObject->getID());
      return (true);
   }
   else
   {
      getVar(cOutputObject)->asObject()->writeVar(cInvalidObjectID);
      return (false);
   }
}

//==============================================================================
// Get the units that are corpses
//==============================================================================
bool BTriggerCondition::tcCanGetCorpseUnits()
{
   enum
   {
      cLocation = 1,
      cRadius = 2,
      cUnits = 3,
      cNumUnits = 4,
   };
   
   BEntityIDArray corpses;
   uint numCorpses = gCorpseManager.getNumCorpses();
   for (uint i = 0; i < numCorpses; i++)
   {
      BEntityID unitID = gCorpseManager.getCorpseUnit(i);
//-- FIXING PREFIX BUG ID 3829
      const BUnit* pUnit = gWorld->getUnit(unitID);
//--
      if (pUnit)
      {
         if (getVar(cLocation)->isUsed() && getVar(cRadius)->isUsed())
         {
            BVector loc = getVar(cLocation)->asVector()->readVar();
            float radiusSq = getVar(cRadius)->asFloat()->readVar();
            radiusSq *= radiusSq;
                        
            if (pUnit->calculateXZDistanceSqr(loc) < radiusSq)
            {
               corpses.add(unitID);
            }
         }
         else
         {
            corpses.add(unitID);
         }
      }
   }

   if (getVar(cUnits)->isUsed())
   {
      getVar(cUnits)->asUnitList()->writeVar(corpses);
   }

   int numUnits = corpses.getNumber();
   if (getVar(cNumUnits)->isUsed())
   {
      getVar(cNumUnits)->asInteger()->writeVar(numUnits);
   }

   bool result = (numUnits > 0);

   return (result);
}


//==============================================================================
// Call correct version
//==============================================================================
bool BTriggerCondition::tcCanGetOneTime()
{
   bool result = false;
   switch (getVersion())
   {
      case 1:
         result = tcCanGetOneTimeV1();
         break;

      case 2:
         result = tcCanGetOneTimeV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }

   return (result);
}

//==============================================================================
// Check for one time in the time list
//==============================================================================
bool BTriggerCondition::tcCanGetOneTimeV1()
{
   enum 
   { 
      cTimeList = 1, 
      cListPosition = 2, 
      cFoundTime = 3, 
   };

   const BDWORDArray& timeList = getVar(cTimeList)->asTimeList()->readVar();
   uint listSize = timeList.getSize();
   if (listSize == 0)
      return (false);

   long listPos = getVar(cListPosition)->asListPosition()->readVar();
   switch (listPos)
   {
   case BTriggerVarListPosition::cListPosTypeFirst:
      {
         getVar(cFoundTime)->asTime()->writeVar(timeList[0]);
         return (true);
      }
      break;
   case BTriggerVarListPosition::cListPosTypeLast:
      {
         getVar(cFoundTime)->asTime()->writeVar(timeList[listSize - 1]);
         return (true);
      }
      break;
   case BTriggerVarListPosition::cListPosTypeRandom:
      {
         uint randomIndex = getRandRange(cSimRand, 0, (listSize - 1));
         getVar(cFoundTime)->asTime()->writeVar(timeList[randomIndex]);
         return (true);
      }
      break;
   }

   return (false);  
}

//==============================================================================
// Check for one time in the time list
//==============================================================================
bool BTriggerCondition::tcCanGetOneTimeV2()
{
   enum 
   { 
      cTimeList = 1, 
      cListPosition = 2, 
      cFoundTime = 4, 
   };

   const BDWORDArray& timeList = getVar(cTimeList)->asTimeList()->readVar();
   uint listSize = timeList.getSize();
   DWORD foundTime = 0;
   bool result = false;
   if (listSize == 0)
      return (false);

   long listPos = getVar(cListPosition)->asListPosition()->readVar();
   switch (listPos)
   {
      case BTriggerVarListPosition::cListPosTypeFirst:
         {
            foundTime = timeList[0];         
            result = true;
         }
         break;
      case BTriggerVarListPosition::cListPosTypeLast:
         {
            foundTime = timeList[listSize - 1];
            result = true;
         }
         break;
      case BTriggerVarListPosition::cListPosTypeRandom:
         {
            uint randomIndex = getRandRange(cSimRand, 0, (listSize - 1));
            foundTime = timeList[randomIndex];
            result = true;
         }
         break;
   }

   if (getVar(cFoundTime)->isUsed())
   {
      getVar(cFoundTime)->asTime()->writeVar(foundTime);
   }

   return (result);  
}

//==============================================================================
// Get a design line from a list of design lines
//==============================================================================
bool BTriggerCondition::tcCanGetOneDesignLine()
{
   enum 
   { 
      cDesignLineList = 1, 
      cListPosition = 2, 
      cFoundDesignLine = 3, 
   };

   const BDesignLineIDArray& lineList = getVar(cDesignLineList)->asDesignLineList()->readVar();
   uint listSize = lineList.getSize();
   if (listSize == 0)
      return (false);

   long listPos = getVar(cListPosition)->asListPosition()->readVar();
   switch (listPos)
   {
      case BTriggerVarListPosition::cListPosTypeFirst:
         {
            getVar(cFoundDesignLine)->asDesignLine()->writeVar(lineList[0]);
            return (true);
         }
         break;

      case BTriggerVarListPosition::cListPosTypeLast:
         {
            getVar(cFoundDesignLine)->asDesignLine()->writeVar(lineList[listSize - 1]);
            return (true);
         }
         break;

      case BTriggerVarListPosition::cListPosTypeRandom:
         {
            uint randomIndex = getRandRange(cSimRand, 0, (listSize - 1));
            getVar(cFoundDesignLine)->asDesignLine()->writeVar(lineList[randomIndex]);
            return (true);
         }
         break;
   }

   return (false);  
}

//==============================================================================
// Compare a design line to another one
//==============================================================================
bool BTriggerCondition::tcCompareDesignLine()
{
   enum 
   { 
      cInputFirstDesignLine = 1, 
      cInputCompareType = 2, 
      cInputSecondDesignLine = 3, 
   };

   BDesignLineID firstDesignLine = getVar(cInputFirstDesignLine)->asDesignLine()->readVar();
   long compareType = getVar(cInputCompareType)->asOperator()->readVar();
   BDesignLineID secondDesignLine = getVar(cInputSecondDesignLine)->asDesignLine()->readVar();

   bool result = BTriggerCondition::compareValues(firstDesignLine, compareType, secondDesignLine);
   return (result);
}

//==============================================================================
// Check for an external flag value
//==============================================================================
bool BTriggerCondition::tcCanRetrieveExternalFlag()
{
   enum 
   { 
      cInExternalFlagIndex = 1, 
      cOutExternalFlag = 2,
   };

//-- FIXING PREFIX BUG ID 3895
   const BTriggerScriptExternals* pExternals = getParentTriggerScript()->getExternals();
//--
   bool flag = false;
   if (pExternals->retrieveFlag(getVar(cInExternalFlagIndex)->asInteger()->readVar(), flag))
   {
      getVar(cOutExternalFlag)->asBool()->writeVar(flag);
      return (true);
   }
   else
   {
      // error.
      return (false);
   }
}

//==============================================================================
//==============================================================================
bool BTriggerCondition::tcAICanGetTopicFocus()
{
   enum { cTopicID = 1, cPlayerID = 2, };
   BAI* pAI = gWorld->getAI(getVar(cPlayerID)->asPlayer()->readVar());
   if (!pAI)
   {
      getVar(cTopicID)->asInteger()->writeVar(cInvalidAITopicID);
      return (false);
   }
   BAITopicID focusTopicID = pAI->AIQGetTopic();
//-- FIXING PREFIX BUG ID 3896
   const BAITopic* pTopic = gWorld->getAITopic(focusTopicID);
//--
   if (pTopic)
   {
      getVar(cTopicID)->asInteger()->writeVar(focusTopicID);
      return (true);
   }
   else
   {
      getVar(cTopicID)->asInteger()->writeVar(cInvalidAITopicID);
      return (false);
   }
}

//==============================================================================
// Verify you can get one float from list
//==============================================================================
bool BTriggerCondition::tcCanGetOneFloat()
{
   enum 
   { 
      cFloatList = 1, 
      cListPosition = 2, 
      cFoundFloat = 3, 
   };

   const BFloatArray& floatList = getVar(cFloatList)->asFloatList()->readVar();
   uint listSize = floatList.getSize();
   if (listSize == 0)
      return (false);

   long listPos = getVar(cListPosition)->asListPosition()->readVar();
   if (!getVar(cFoundFloat)->isUsed())
      return (true);

   switch (listPos)
   {
      case BTriggerVarListPosition::cListPosTypeFirst:
         {
            getVar(cFoundFloat)->asFloat()->writeVar(floatList[0]);
            return (true);
         }
      case BTriggerVarListPosition::cListPosTypeLast:
         {
            getVar(cFoundFloat)->asFloat()->writeVar(floatList[listSize - 1]);
            return (true);
         }
      case BTriggerVarListPosition::cListPosTypeRandom:
         {
            uint randomIndex = getRandRange(cSimRand, 0, (listSize - 1));
            getVar(cFoundFloat)->asFloat()->writeVar(floatList[randomIndex]);
            return (true);
         }
   }

   return (false);  
}

//==============================================================================
// CompareLocStringID
//==============================================================================
bool BTriggerCondition::tcCompareLocStringID()
{
   enum { cLocStringA = 1, cCompareType = 2, cLocStringB = 3, };
   uint32 firstlocstring = getVar(cLocStringA)->asLocStringID()->readVar();
   long compareType = getVar(cCompareType)->asOperator()->readVar();
   uint32 secondlocstring = getVar(cLocStringB)->asLocStringID()->readVar();
   BTRIGGER_ASSERT(compareType == Math::cEqualTo || compareType == Math::cNotEqualTo);
   bool result = BTriggerCondition::compareValues(firstlocstring, compareType, secondlocstring);
   return (result);
}
//==============================================================================
// BTriggerCondition::tcUILocationMinigameWaiting
//==============================================================================
bool BTriggerCondition::tcUILocationMinigameWaiting()
{
   enum { cUILocationMinigame = 1, };
   long uiLocationResult = getVar(cUILocationMinigame)->asUILocationMinigame()->readResult();
   return (uiLocationResult == BTriggerVarUILocationMinigame::cUILocationResultWaiting);
}

//==============================================================================
// BTriggerCondition::tcUILocationMinigameOK
//==============================================================================
bool BTriggerCondition::tcUILocationMinigameOK()
{
   enum { cInputUILocationMinigame = 1, cOutputLocation = 2, cOutputData = 3, };
   BTriggerVarUILocationMinigame *pVar = getVar(cInputUILocationMinigame)->asUILocationMinigame();
   if (pVar->readResult() == BTriggerVarUILocationMinigame::cUILocationResultOK)
   {
      getVar(cOutputLocation)->asVector()->writeVar(pVar->readLocation());
      getVar(cOutputData)->asFloat()->writeVar(pVar->readData());
      return (true);
   }
   else
      return (false);
}

//==============================================================================
// BTriggerCondition::tcUILocationMinigameCancel
//==============================================================================
bool BTriggerCondition::tcUILocationMinigameCancel()
{
   enum { cInputUILocationMinigame = 1, };
   BTriggerVarUILocationMinigame *pVar = getVar(cInputUILocationMinigame)->asUILocationMinigame();
   if (pVar->readResult() == BTriggerVarUILocationMinigame::cUILocationResultCancel)
      return (true);
   else
      return (false);
}

//==============================================================================
// BTriggerCondition::tcUILocationMinigameUILockError
//==============================================================================
bool BTriggerCondition::tcUILocationMinigameUILockError()
{
   enum { cUILocationMinigame = 1, };
   long uiLocationResult = getVar(cUILocationMinigame)->asUILocationMinigame()->readResult();
   return (uiLocationResult == BTriggerVarUILocationMinigame::cUILocationResultUILockError);
}

//==============================================================================
// Is the entity selectable
//==============================================================================
bool BTriggerCondition::tcIsSelectable()
{
   enum
   {
      cInUnit = 1,
      cInUnitList = 2,
      cInSquad = 3,
      cInSquadList = 4,
      cInObject = 5,
      cInObjectList = 6,      
   };

   BEntityIDArray entities;
   if (getVar(cInUnitList)->isUsed())
   {
      entities = getVar(cInUnitList)->asUnitList()->readVar();
   }

   if (getVar(cInUnit)->isUsed())
   {
      entities.uniqueAdd(getVar(cInUnit)->asUnit()->readVar());
   }

   if (getVar(cInSquadList)->isUsed())
   {
      entities.append(getVar(cInSquadList)->asSquadList()->readVar());
   }

   if (getVar(cInSquad)->isUsed())
   {
      entities.uniqueAdd(getVar(cInSquad)->asSquad()->readVar());
   }

   if (getVar(cInObjectList)->isUsed())
   {
      entities.append(getVar(cInObjectList)->asObjectList()->readVar());
   }

   if (getVar(cInObject)->isUsed())
   {
      entities.uniqueAdd(getVar(cInObject)->asObject()->readVar());
   }

   uint numEntities = entities.getSize();
   for (uint i = 0; i < numEntities; i++)
   {
//-- FIXING PREFIX BUG ID 3899
      const BEntity* pEntity = gWorld->getEntity(entities[i]);
//--
      // TODO - is there a way to know what the team id is that is trying to check for selection? 
      // pass in gaia for now
      if (pEntity && (pEntity->getSelectType(0) == cSelectTypeNone))
      {
         return (false);
      }
   }

   return (true);
}

//==============================================================================
// Detect if a chat's completed event ID has fired
//==============================================================================
bool BTriggerCondition::tcChatCompleted()
{
   bool result = false;   
   switch (getVersion())
   {
   case 1:
      result = tcChatCompletedV1();
      break;

   case 2:
      result = tcChatCompletedV2();
      break;

   default:
      BTRIGGER_ASSERTM(false, "This version is not supported!");
      break;
   }

   return (result);
}

//==============================================================================
bool BTriggerCondition::tcChatCompletedV1()
{
   bool result = false;   
   BChatManager* pChatManager = gWorld->getChatManager();
   if (pChatManager)
   {
      BGeneralEventSubscriber* pSubscription = pChatManager->getChatCompletedEventSubscriber();
      if (pSubscription)
      {
         result = pSubscription->hasFired();
      }
   }

   return (result);
}

//==============================================================================
bool BTriggerCondition::tcChatCompletedV2()
{
   enum
   {
      cDelay = 2
   };

   bool result = false;   
   BChatManager* pChatManager = gWorld->getChatManager();
   if (pChatManager)
   {
      BGeneralEventSubscriber* pSubscription = pChatManager->getChatCompletedEventSubscriber();
      if (pSubscription)
      {
         if (getVar(cDelay)->isUsed())
         {
            if (pSubscription->hasFired())
               result = ((gWorld->getGametime() - pSubscription->getFireTime()) >= getVar(cDelay)->asTime()->readVar());
         }
         else
            result = pSubscription->hasFired();
      }
   }

   return (result);
}

//==============================================================================
// Detect if a cinematic's completed event ID has fired
//==============================================================================
bool BTriggerCondition::tcCinematicCompleted()
{
   bool result = false;

   BCinematicManager* pCinematicManager = gWorld->getCinematicManager();
   if (pCinematicManager)
   {
      BGeneralEventSubscriber* pSubscription = pCinematicManager->getCinematicCompletedEventSubscriber();
      if (pSubscription)
         result = pSubscription->hasFired();
   }
         
   return (result);
}


//==============================================================================
// BTriggerCondition::tcSquadFlag
//==============================================================================
bool BTriggerCondition::tcSquadFlag()
{
   enum { cSquad = 1, cSquadList = 2, cFlagType = 3, cCompareValue = 4, cAllOn = 5, };

   bool doCompare = false;
   bool compareValue = true;

//-- FIXING PREFIX BUG ID 3902
   const BTriggerVarBool* pVarCompareValue = getVar(cCompareValue)->asBool();
//--
   if (pVarCompareValue->isUsed())
   {
      doCompare = true;
      compareValue = pVarCompareValue->readVar();
   }

   bool retval = true;
   bool allOn = true;

   long flagType = getVar(cFlagType)->asSquadFlag()->readVar();
   if (flagType == -1)
   {
      if (doCompare)
         retval = false;
      allOn = false;
   }
   else
   {
//-- FIXING PREFIX BUG ID 3900
      const BTriggerVarSquad* pVarSquad = getVar(cSquad)->asSquad();
//--
      if (pVarSquad->isUsed())
         squadFlagHelper(pVarSquad->readVar(), flagType, doCompare, compareValue, retval, allOn);

//-- FIXING PREFIX BUG ID 3901
      const BTriggerVarSquadList* pVarSquadList = getVar(cSquadList)->asSquadList();
//--
      if (pVarSquadList->isUsed())
      {
         const BEntityIDArray& squadList = pVarSquadList->readVar();
         uint numSquads = squadList.getSize();
         for (uint i=0; i<numSquads; i++)
            squadFlagHelper(squadList[i], flagType, doCompare, compareValue, retval, allOn);
      }
   }

   BTriggerVarBool* pVarAllOn = getVar(cAllOn)->asBool();
   if (pVarAllOn->isUsed())
      pVarAllOn->writeVar(allOn);

   return retval;
}

//==============================================================================
// Get associated socket units from another unit
//==============================================================================
bool BTriggerCondition::tcCanGetSocketUnits()
{
   bool result = false;

   switch (getVersion())
   {
      case 1:
         result = tcCanGetSocketUnitsV1();
         break;

      case 2:
         result = tcCanGetSocketUnitsV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }

   return (result);
}

//==============================================================================
// Get associated socket units from another unit
//==============================================================================
bool BTriggerCondition::tcCanGetSocketUnitsV1()
{
   enum
   {
      cInUnit = 1,
      cOutSocketUnits = 2,
      cOutTurretSocketUnits = 3,
   };

   BEntityIDArray socketUnits;
   BEntityIDArray turrectSocketUnits;
   bool result = false;
//-- FIXING PREFIX BUG ID 3756
   const BUnit* pUnit = gWorld->getUnit(getVar(cInUnit)->asUnit()->readVar());
//--
   if (pUnit)
   {
      uint numEntityRefs = pUnit->getNumberEntityRefs();
      for (uint i = 0; i < numEntityRefs; i++)
      {
//-- FIXING PREFIX BUG ID 3863
         const BEntityRef* pEntityRef = pUnit->getEntityRefByIndex(i);
//--
         if (pEntityRef && (pEntityRef->mType == BEntityRef::cTypeAssociatedSocket))
         {
//-- FIXING PREFIX BUG ID 3903
            const BUnit* pSocketUnit = gWorld->getUnit(pEntityRef->mID);
//--
            if (pSocketUnit)
            {
               const BProtoObject* pSocketProtoObject = pSocketUnit->getProtoObject();
               if (pSocketProtoObject && pSocketProtoObject->isType(gDatabase.getOTIDBuildingSocket()))
               {
                  socketUnits.uniqueAdd(pEntityRef->mID);
                  result = true;
               }
               else if (pSocketProtoObject && pSocketProtoObject->isType(gDatabase.getOTIDTurretSocket()))
               {
                  turrectSocketUnits.uniqueAdd(pEntityRef->mID);
                  result = true;
               }
            }
         }
      }
   }

   if (getVar(cOutSocketUnits)->isUsed())
   {
      getVar(cOutSocketUnits)->asUnitList()->writeVar(socketUnits);
   }

   if (getVar(cOutTurretSocketUnits)->isUsed())
   {
      getVar(cOutTurretSocketUnits)->asUnitList()->writeVar(turrectSocketUnits);
   }

   return (result);
}

//==============================================================================
// Get associated socket units from another unit
//==============================================================================
bool BTriggerCondition::tcCanGetSocketUnitsV2()
{
   enum
   {
      cInUnit = 1,
      cOutSocketUnits = 2,
      cOutTurretSocketUnits = 3,
      cInOnlyEmptySockets = 4,
   };

   bool onlyEmpty = getVar(cInOnlyEmptySockets)->isUsed() ? getVar(cInOnlyEmptySockets)->asBool()->readVar() : false;
   BEntityIDArray socketUnits;
   BEntityIDArray turrectSocketUnits;
   bool result = false;
   BUnit* pUnit = gWorld->getUnit(getVar(cInUnit)->asUnit()->readVar());
   if (pUnit)
   {
      uint numEntityRefs = pUnit->getNumberEntityRefs();
      for (uint i = 0; i < numEntityRefs; i++)
      {
//-- FIXING PREFIX BUG ID 3904
         const BEntityRef* pEntityRef = pUnit->getEntityRefByIndex(i);
//--
         if (pEntityRef && (pEntityRef->mType == BEntityRef::cTypeAssociatedSocket))
         {
//-- FIXING PREFIX BUG ID 3905
            const BUnit* pSocketUnit = gWorld->getUnit(pEntityRef->mID);
//--
            if (pSocketUnit)
            {
               if (onlyEmpty && (pSocketUnit->getFirstEntityRefByType(BEntityRef::cTypeSocketPlug) != NULL))
                  continue;

               const BProtoObject* pSocketProtoObject = pSocketUnit->getProtoObject();
               if (pSocketProtoObject && pSocketProtoObject->isType(gDatabase.getOTIDBuildingSocket()))
               {
                  socketUnits.uniqueAdd(pEntityRef->mID);
                  result = true;
               }
               else if (pSocketProtoObject && pSocketProtoObject->isType(gDatabase.getOTIDTurretSocket()))
               {
                  turrectSocketUnits.uniqueAdd(pEntityRef->mID);
                  result = true;
               }
            }
         }
      }
   }

   if (getVar(cOutSocketUnits)->isUsed())
   {
      getVar(cOutSocketUnits)->asUnitList()->writeVar(socketUnits);
   }

   if (getVar(cOutTurretSocketUnits)->isUsed())
   {
      getVar(cOutTurretSocketUnits)->asUnitList()->writeVar(turrectSocketUnits);
   }

   return (result);
}

//==============================================================================
// Get a specific associated socket unit from another unit
//==============================================================================
bool BTriggerCondition::tcCanGetOneSocketUnit()
{
   enum
   {
      cInUnit = 1,
      cInBuildingIndex = 2,
      cInTurretIndex = 3,
      cOutSocketUnit = 4,
      cOutTurretSocketUnit = 5,
   };
      
   bool result = false;
   BEntityID buildingUnitID = cInvalidObjectID;
   BEntityID turretUnitID = cInvalidObjectID;
   BUnit* pUnit = gWorld->getUnit(getVar(cInUnit)->asUnit()->readVar());
   if (pUnit)
   {
      int buildingIndex = getVar(cInBuildingIndex)->isUsed() ? getVar(cInBuildingIndex)->asInteger()->readVar() : -1;
      int turretIndex = getVar(cInTurretIndex)->isUsed() ? getVar(cInTurretIndex)->asInteger()->readVar() : -1;
      if ((buildingIndex != -1) || (turretIndex != -1))
      {
         uint numEntityRefs = pUnit->getNumberEntityRefs();
         for (uint i = 0; i < numEntityRefs; i++)
         {
//-- FIXING PREFIX BUG ID 3906
            const BEntityRef* pEntityRef = pUnit->getEntityRefByIndex(i);
//--
            if (pEntityRef && (pEntityRef->mType == BEntityRef::cTypeAssociatedSocket))
            {
//-- FIXING PREFIX BUG ID 3907
               const BUnit* pSocketUnit = gWorld->getUnit(pEntityRef->mID);
//--
               if (pSocketUnit)
               {
                  const BProtoObject* pSocketProtoObject = pSocketUnit->getProtoObject();
                  if (pSocketProtoObject && pSocketProtoObject->isType(gDatabase.getOTIDBuildingSocket()))
                  {
                     if (buildingIndex == 0)
                     {
                        buildingUnitID = pEntityRef->mID;
                        result = true;
                     }

                     if (buildingIndex >= 0)
                     {
                        buildingIndex--;
                     }                  
                  }
                  else if (pSocketProtoObject && pSocketProtoObject->isType(gDatabase.getOTIDTurretSocket()))
                  {
                     if (turretIndex == 0)
                     {
                        turretUnitID = pEntityRef->mID;
                        result = true;                     
                     }

                     if (turretIndex >= 0)
                     {
                        turretIndex--;
                     }
                  }
               }
            }
         }
      }
   }

   if (getVar(cOutSocketUnit)->isUsed())
   {
      getVar(cOutSocketUnit)->asUnit()->writeVar(buildingUnitID);
   }

   if (getVar(cOutTurretSocketUnit)->isUsed())
   {
      getVar(cOutTurretSocketUnit)->asUnit()->writeVar(turretUnitID);
   }

   return (result);
}

//==============================================================================
// Detect if the fade has completed
//==============================================================================
bool BTriggerCondition::tcFadeCompleted()
{
   bool result = false;

   BGeneralEventSubscriber* pSubscription = gWorld->getTransitionManager()->getCompletedEventSubscriber();;
   if (pSubscription)
      result = pSubscription->hasFired();

   return (result);
}

//====================================================================================
// See if all of the squad(s), unit(s), and/or object(s) are able to be auto attacked
//====================================================================================
bool BTriggerCondition::tcIsAutoAttackable()
{
   enum
   {
      cInUnit = 1,
      cInUnitList = 2,
      cInSquad = 3,
      cInSquadList = 4,
      cOutAutoAttackable = 6,
   };

   bool result = true;
   BEntityIDArray objectList;
   if (getVar(cInUnitList)->isUsed())
   {
      objectList.append(getVar(cInUnitList)->asUnitList()->readVar());
   }

   if (getVar(cInUnit)->isUsed())
   {
      objectList.uniqueAdd(getVar(cInUnit)->asUnit()->readVar());
   }

   if (getVar(cInSquadList)->isUsed())
   {
      const BEntityIDArray& squadList = getVar(cInSquadList)->asSquadList()->readVar();
      uint numSquads = squadList.getSize();
      for (uint i = 0; i < numSquads; i++)
      {
//-- FIXING PREFIX BUG ID 3909
         const BSquad* pSquad = gWorld->getSquad(squadList[i]);
//--
         if (pSquad)
         {
            objectList.append(pSquad->getChildList());
         }
      }
   }

   if (getVar(cInSquad)->isUsed())
   {
//-- FIXING PREFIX BUG ID 3910
      const BSquad* pSquad = gWorld->getSquad(getVar(cInSquad)->asSquad()->readVar());
//--
      if (pSquad)
      {
         objectList.append(pSquad->getChildList());
      }
   }
   
   uint numObjects = objectList.getSize();
   for (uint i = 0; i < numObjects; i++)
   {
//-- FIXING PREFIX BUG ID 3911
      const BObject* pObject = gWorld->getObject(objectList[i]);
//--
      if (pObject && pObject->getFlagDontAutoAttackMe())
      {
         result = false;
         break;
      }
   }

   return (result);
}

//====================================================================================
//====================================================================================
bool BTriggerCondition::tcIsBeingGatheredFrom()
{
   enum
   {
      cInUnit = 1,
      cInUnitList = 2,
   };
   
   BEntityIDArray objectList;
   if (getVar(cInUnit)->isUsed())
   {
      objectList.uniqueAdd(getVar(cInUnit)->asUnit()->readVar());
   }
   if (getVar(cInUnitList)->isUsed())
   {
      objectList.append(getVar(cInUnitList)->asUnitList()->readVar());
   }

   uint numObjects = objectList.getSize();
   for (uint idx = 0; idx < numObjects; idx++)
   {
//-- FIXING PREFIX BUG ID 3912
      const BUnit* pUnit = gWorld->getUnit(objectList[idx]);
//--
      if (pUnit && pUnit->getNumberGatherers() > 0)
         return true;
   }

   return false;
}


//====================================================================================
//====================================================================================
bool BTriggerCondition::tcAICanGetDifficultySetting()
{
   enum { cPlayerID = 1, cSettingName = 2, cDefaultName = 3, cValue = 4, };
   BPlayerID playerID = getVar(cPlayerID)->asPlayer()->readVar();
   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return (false);
   BAI* pAI = gWorld->getAI(playerID);
   if (!pAI)
      return (false);

   const BAIDifficultySetting* pSetting = pAI->getDifficultySettingByName(getVar(cSettingName)->asString()->readVar());
   if (!pSetting)
      return (false);

   BTriggerVarFloat* pVarValue = getVar(cValue)->asFloat();
//-- FIXING PREFIX BUG ID 3913
   const BTriggerVarString* pVarDefaultName = getVar(cDefaultName)->asString();
//--
   if (pVarDefaultName->isUsed())
   {
      float defaultValue = 0.0f;
      if (pSetting->canGetDefaultValueByName(pVarDefaultName->readVar(), defaultValue))
      {
         if (pVarValue->isUsed())
            pVarValue->writeVar(defaultValue);
         return (true);
      }
      else
         return (false);
   }
   else
   {
      float playerDifficulty = pPlayer->getDifficulty();
      float outputValue = pSetting->getFunc().evaluate(playerDifficulty);
      if (pVarValue->isUsed())
         pVarValue->writeVar(outputValue);
      return (true);
   }
}

   
//====================================================================================
//====================================================================================
bool BTriggerCondition::tcConceptGetParent()
{

   return false;
}
   
//====================================================================================
//====================================================================================
bool BTriggerCondition::tcConceptGetCommand()
{
   enum { cPlayerID = 1, cConcept = 2, cCommand = 3 };

   BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayerID)->asPlayer()->readVar());
   if (!pPlayer)
      return (false);
  
   #ifdef SYNC_Trigger
      syncTriggerCode("BTriggerCondition::tcConceptGetCommand 1");
   #endif

   BHintEngine* pHintEngine = pPlayer->getHintEngine();
   if(!pHintEngine)
      return false;
  
   #ifdef SYNC_Trigger
      syncTriggerCode("BTriggerCondition::tcConceptGetCommand 2");
   #endif

   int concept = getVar(cConcept)->asConcept()->readVar();
    
   if(pHintEngine->hasCommand(concept))
   {
      int command = pHintEngine->popCommand(concept);
      getVar(cCommand)->asInteger()->writeVar(command);

      return true;
   }


   return false;
}

//====================================================================================
//====================================================================================
bool BTriggerCondition::tcConceptGetStateChange()
{
   enum { cPlayerID = 1, cConcept = 2, cState = 3 };

   BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayerID)->asPlayer()->readVar());
   if (!pPlayer)
      return (false);
   
   BHintEngine* pHintEngine = pPlayer->getHintEngine();
   if(!pHintEngine)
      return false;
  
   int concept = getVar(cConcept)->asConcept()->readVar();
 
   if(pHintEngine->hasStateChange(concept))
   {
      int state = pHintEngine->popState(concept);
      getVar(cState)->asInteger()->writeVar(state);

      return true;
   }
   return false;
}
   
//====================================================================================
//====================================================================================
bool BTriggerCondition::tcConceptCompareState()
{
   enum { cPlayerID = 1, cConcept = 2, cOperator = 3, cState = 4 };

   BPlayer* pPlayer = gWorld->getPlayer(getVar(cPlayerID)->asPlayer()->readVar());
   if (!pPlayer)
      return (false);

   BHintEngine* pHintEngine = pPlayer->getHintEngine();
   if(!pHintEngine)
      return false;
  
   int concept = getVar(cConcept)->asConcept()->readVar();
 
   int conceptState = -1;  

   if(pHintEngine->hasCommand(concept))
   {
      conceptState = pHintEngine->peekState(concept);
      return true;
   }
   else
   {
      return false;
   }

   int state = getVar(cState)->asInteger()->readVar();;
   long op = getVar(cOperator)->asOperator()->readVar();

   return compareValues(conceptState, op, state);
}
//==============================================================================
// Check to see if this unit is an empty socket unit
//==============================================================================
bool BTriggerCondition::tcIsEmptySocketUnit()
{
   enum
   {
      cInUnit = 1,
   };

   bool result = false;
//-- FIXING PREFIX BUG ID 3915
   const BUnit* pSocketUnit = gWorld->getUnit(getVar(cInUnit)->asUnit()->readVar());
//--
   if (pSocketUnit)
   {
      const BProtoObject* pProtoObject = pSocketUnit->getProtoObject();
      if (pProtoObject && (pProtoObject->isType(gDatabase.getOTIDBuildingSocket()) || pProtoObject->isType(gDatabase.getOTIDTurretSocket()) || pProtoObject->isType(gDatabase.getOTIDSettlement())) && (pSocketUnit->getFirstEntityRefByType(BEntityRef::cTypeSocketPlug) == NULL))
      {
         // See if the base socket's rebuild timer is active
         if (pProtoObject->isType(gDatabase.getOTIDSettlement()))
         {
            const BUnitActionBuilding* pBuildingAction = (const BUnitActionBuilding*)pSocketUnit->getActionByTypeConst(BAction::cActionTypeUnitBuilding);
            if (pBuildingAction && (pBuildingAction->getRebuildTimer() == 0.0f))
            {
               result = true;
            }               
         }
         else
         {
            result = true;
         }
      }
   }

   return (result);
}

//==============================================================================
// Check to see if any of the proto data is forbidden for this player
//==============================================================================
bool BTriggerCondition::tcIsForbidden()
{
   enum
   {
      cInPlayer = 1,
      cInProtoSquad = 2,
      cInProtoObject = 3,
      cInProtoTech = 4,
   };

   bool result = false;
   BPlayerID playerID = getVar(cInPlayer)->asPlayer()->readVar();
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (pPlayer)
   {
      if (getVar(cInProtoSquad)->isUsed())
      {
//-- FIXING PREFIX BUG ID 3916
         const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(getVar(cInProtoSquad)->asProtoSquad()->readVar());
//--
         if (pProtoSquad)
         {
            result = pProtoSquad->getFlagForbid();
         }
      }

      if (!result && getVar(cInProtoObject)->isUsed())
      {
//-- FIXING PREFIX BUG ID 3917
         const BProtoObject* pProtoObject = pPlayer->getProtoObject(getVar(cInProtoObject)->asProtoObject()->readVar());
//--
         if (pProtoObject)
         {
            result = pProtoObject->getFlagForbid();
         }
      }

      if (!result && getVar(cInProtoTech)->isUsed())
      {
//-- FIXING PREFIX BUG ID 3918
         const BProtoTech* pProtoTech = pPlayer->getProtoTech(getVar(cInProtoTech)->asTech()->readVar());
//--
         if (pProtoTech)
         {
            result = pProtoTech->getFlagForbid();
         }
      }      
   }

   return (result);
}

//==============================================================================
// Return the socket unit's parent building
//==============================================================================
bool BTriggerCondition::tcCanGetSocketParentBuilding()
{
   enum
   {
      cInSocket = 1,
      cOutParentBuilding = 2,
   };

   bool result = false;
   BEntityID parentID = cInvalidObjectID;
   BUnit* pSocketUnit = gWorld->getUnit(getVar(cInSocket)->asUnit()->readVar());
   if (pSocketUnit)
   {
      const BProtoObject* pProtoObject = pSocketUnit->getProtoObject();
      if (pProtoObject && (pProtoObject->isType(gDatabase.getOTIDBuildingSocket()) || pProtoObject->isType(gDatabase.getOTIDTurretSocket())))
      {
//-- FIXING PREFIX BUG ID 3908
         const BEntityRef* pEntityRef = pSocketUnit->getFirstEntityRefByType(BEntityRef::cTypeAssociatedBase);
//--
         if (pEntityRef)
         {
//-- FIXING PREFIX BUG ID 3919
            const BUnit* pParentBuilding = gWorld->getUnit(pEntityRef->mID);
//--
            if (pParentBuilding)
            {
               parentID = pEntityRef->mID;
               result = true;
            }
         }
      }      
   }

   getVar(cOutParentBuilding)->asUnit()->writeVar(parentID);
   return (result);
}

//==============================================================================
// See if a random location can be returned based on provided parameters
//==============================================================================
bool BTriggerCondition::tcCanGetRandomLocation()
{
   enum 
   { 
      cInLocation = 1, 
      cInInnerRadius = 2,       
      cInOuterRadius = 3,
      cInTestObstructions = 4,
      cInTestPathing = 5,
      cInProtoObjectID = 6,
      cInProtoSquadID = 7,
      cOutLocation = 8,       
   };

   BVector inLoc = getVar(cInLocation)->asVector()->readVar();
   float innerRad = getVar(cInInnerRadius)->isUsed() ? getVar(cInInnerRadius)->asFloat()->readVar() : 0.0f;
   float outerRad = getVar(cInOuterRadius)->asFloat()->readVar();
   bool testObstruction = getVar(cInTestObstructions)->isUsed() ? getVar(cInTestObstructions)->asBool()->readVar() : false;
   bool testPathing = getVar(cInTestPathing)->isUsed() ? getVar(cInTestPathing)->asBool()->readVar() : false;
   BProtoObjectID protoObjectID = getVar(cInProtoObjectID)->isUsed() ? getVar(cInProtoObjectID)->asProtoObject()->readVar() : cInvalidProtoID;
   BProtoSquadID protoSquadID = getVar(cInProtoSquadID)->isUsed() ? getVar(cInProtoSquadID)->asProtoSquad()->readVar() : cInvalidProtoSquadID;

   BVector outLoc = cInvalidVector;   
   bool found = false;
   uint searchCount = 0;
   while (!found)
   {
      outLoc = BSimHelper::randomCircularDistribution(inLoc, outerRad, innerRad);
      found = true;
      if (testObstruction)
      {
         BVector suggestion = cInvalidVector;
         BVector tempForward = outLoc - inLoc;
         tempForward.normalize();
         const BVector forward = tempForward;
         long losMode = BWorld::cCPLOSDontCare;
         DWORD flags = BWorld::cCPCheckObstructions | BWorld::cCPSetPlacementSuggestion | BWorld::cCPExpandHull | BWorld::cCPIgnoreMoving | BWorld::cCPIgnoreParkingAndFlatten;
         if ((protoObjectID == cInvalidObjectID) && (protoSquadID == cInvalidProtoSquadID))
         {
            flags |= BWorld::cCPIgnorePlacementRules;            
         }
         bool useSquadObstruction = (protoSquadID == cInvalidProtoSquadID) ? false : true;
         found = gWorld->checkPlacement(protoObjectID, cInvalidPlayerID, outLoc, suggestion, forward, losMode, flags, 1, NULL, NULL, -1, protoSquadID, useSquadObstruction);
         outLoc = suggestion;
      }

      if (found && testPathing)
      {
         float pathingRadius = -1.0f;
         BProtoSquad* pProtoSquad = gDatabase.getGenericProtoSquad(protoSquadID);
         if (pProtoSquad)
         {
            float obsX = 0.0f;
            float obsZ = 0.0f;
            pProtoSquad->calcObstructionRadiiFromObjects(obsX, obsZ);
            pathingRadius = Math::Max(obsX, obsZ);
         }
         
         if (pathingRadius < 0.0f)
         {
//-- FIXING PREFIX BUG ID 3922
            const BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(protoObjectID);
//--
            pathingRadius = (pProtoObject) ? pProtoObject->getObstructionRadius() : 2.0f;
         }
         float range = 0.0f;
         const BEntityIDArray* ignoreList = NULL;
         static BPath path;
         path.reset();
         bool skipBegin = false;
         bool fullPathOnly = true;
         long targetID = -1L;
         int pathType = BPather::cLongRange;
         int pathClass = BPather::cLandPath;         
         bool canJump = false;               //Don't put random locations in a jump area
         long result = gPather.findPath(&gObsManager, outLoc, inLoc, pathingRadius, range, ignoreList, &path, skipBegin, fullPathOnly, canJump, targetID, pathType, pathClass);         
         found = (BPath::cFull == result);
      }

      if (!found)
      {
         searchCount++;
      }

      if (searchCount >= TC_RANDOM_LOCATION_SEARCH_MAX)
      {
         break;
      }
   }

   #ifndef BUILD_FINAL
      if ((testObstruction || testPathing) && !found)
      {
         BTRIGGER_ASSERTM(false, "WARNING:  Random location obstructions and/or pathing could not be verified.  Reevaluate trigger parameters.");
      }
   #endif

   gTerrainSimRep.getHeight(outLoc, true);
   getVar(cOutLocation)->asVector()->writeVar(outLoc);

   return (found);
}

//==============================================================================
// Test the player's difficulty setting
//==============================================================================
bool BTriggerCondition::tcCheckDifficulty()
{
   bool result = false;

   switch (getVersion())
   {
      case 1:
         result = tcCheckDifficultyV1();
         break;

      case 2:
         result = tcCheckDifficultyV2();
         break;

      default:
         BTRIGGER_ASSERTM(false, "This version is not supported!");
         break;
   }

   return (result);
}

//==============================================================================
// Test the player's difficulty setting
//==============================================================================
bool BTriggerCondition::tcCheckDifficultyV1()
{
   enum  
   {
      cInPlayer = 1,
      cInDifficulty = 2,
   };

   bool result = false;

//-- FIXING PREFIX BUG ID 3923
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cInPlayer)->asPlayer()->readVar());
//--
   if (pPlayer)
   {
      BDifficultyType playerDifficulty = pPlayer->getDifficultyType();
      BDifficultyType testDifficulty = getVar(cInDifficulty)->asDifficulty()->readVar();
      if (playerDifficulty == testDifficulty)
      {
         result = true;
      }
   }

   return (result);
}

//==============================================================================
// Test the player's difficulty setting
//==============================================================================
bool BTriggerCondition::tcCheckDifficultyV2()
{
   enum  
   {
      cInPlayer = 1,
      cInDifficulty = 2,
      cInOperator = 3,
   };

   bool result = false;

//-- FIXING PREFIX BUG ID 3924
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cInPlayer)->asPlayer()->readVar());
//--
   if (pPlayer)
   {
      BDifficultyType playerDifficulty = pPlayer->getDifficultyType();
      BDifficultyType testDifficulty = getVar(cInDifficulty)->asDifficulty()->readVar();
      BOperatorType testOperator = getVar(cInOperator)->asOperator()->readVar();
      result = BTriggerCondition::compareValues(playerDifficulty, testOperator, testDifficulty);
   }

   return (result);
}

//==============================================================================
// tcCanGetDesignSpheres
//==============================================================================
bool BTriggerCondition::tcCanGetDesignSpheres()
{
   enum  
   {
      cType = 1,
      cCenterPointList = 2,
      cRadiusList = 3,
   };

   bool result = false;
   
   bool checkType = getVar(cType)->isUsed();
   BSimString type;
   if(checkType)
   {
      type = getVar(cType)->asString()->readVar();
   }
   
   BVectorArray centerPointList;
   BFloatArray radiusList;

   BDesignObjectManager* pDesignObjectManager = gWorld->getDesignObjectManager();
   if (pDesignObjectManager)
   {
      uint numSpheres = pDesignObjectManager->getDesignSphereCount();
      for (uint i = 0; i < numSpheres; i++)
      {            
//-- FIXING PREFIX BUG ID 3925
         const BDesignSphere& sphere = pDesignObjectManager->getDesignSphere(i);
//--
         if(checkType && (sphere.mDesignData.mType != type))
         {
            continue;
         }
         result = true;
         centerPointList.add(sphere.mPosition);
         radiusList.add(sphere.mRadius);
      }
   }

   getVar(cCenterPointList)->asVectorList()->writeVar(centerPointList);
   getVar(cRadiusList)->asFloatList()->writeVar(radiusList);

   return (result);
}

//==============================================================================
//==============================================================================
bool BTriggerCondition::tcCanRemoveOneFloat()
{
   enum { cFloatList = 1, cListPosition = 2, cFoundFloat = 3, };
   BFloatArray& FloatList = getVar(cFloatList)->asFloatList()->readVar();
   uint listSize = FloatList.getSize();
   if (listSize == 0)
      return (false);

   long listPos = getVar(cListPosition)->asListPosition()->readVar();
   switch (listPos)
   {
   case BTriggerVarListPosition::cListPosTypeFirst:
      {
         getVar(cFoundFloat)->asFloat()->writeVar(FloatList[0]);
         FloatList.removeIndex(0);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeLast:
      {
         getVar(cFoundFloat)->asFloat()->writeVar(FloatList[listSize-1]);
         FloatList.removeIndex(listSize-1);
         return (true);
      }
   case BTriggerVarListPosition::cListPosTypeRandom:
      {
         // Note random doesn't care about maintaining order.
         uint randomIndex = getRandRange(cSimRand, 0, (listSize-1));
         getVar(cFoundFloat)->asFloat()->writeVar(FloatList[randomIndex]);
         FloatList.removeIndex(randomIndex, false);
         return (true);
      }
   }

   return (false);  
}


//==============================================
//==============================================
bool BTriggerCondition::tcCanRetrieveExternalFloat()
{
   enum { cExternalFloat = 1, };
//-- FIXING PREFIX BUG ID 3926
   const BTriggerScriptExternals* pExternals = getParentTriggerScript()->getExternals();
//--
   float value;
   if (pExternals->retrieveFloat(value))
   {
      getVar(cExternalFloat)->asFloat()->writeVar(value);
      return (true);
   }
   else
   {
      // error.
      return (false);
   }
}


//==============================================
//==============================================
bool BTriggerCondition::tcMarkerSquadsInArea()
{
   enum { cSquadList = 1, cPoint =2, cRadius = 3, cResultList = 4 };

   BEntityIDArray results(0,50);
   BVector loc = getVar(cPoint)->asVector()->readVar();
   float radius = getVar(cRadius)->asFloat()->readVar();
   const BEntityIDArray& filterList = getVar(cSquadList)->asSquadList()->readVar();
   
   bool foundUnit = false;
   bool useResults = getVar(cResultList)->isUsed();
   for(unsigned int i=0; i< filterList.size(); i++)
   {
      BEntityID id = filterList[i];
      BSquad *pSquad = gWorld->getSquad(id);
      if (!pSquad)
         continue;

      float distance = pSquad->calculateXZDistance(loc);
      if (distance <= radius)
      {
         foundUnit=true;
         if(useResults)
         {
            results.add(id);
         }
         else
         {
            return true;
         }
      }
   }
   if(useResults)
   {
      getVar(cResultList)->asSquadList()->writeVar(results);
   }
   return foundUnit;
}

//==============================================================================
// Return the unit plugged into the socket unit if it exists
//==============================================================================
bool BTriggerCondition::tcCanGetSocketPlugUnit()
{
   enum
   {
      cInSocketUnit = 1,
      cOutPlugUnit = 2,
   };

   bool result = false;
   BEntityID plugUnitID = cInvalidObjectID;
   BUnit* pSocketUnit = gWorld->getUnit(getVar(cInSocketUnit)->asUnit()->readVar());
   if (pSocketUnit)
   {
//-- FIXING PREFIX BUG ID 3928
      const BEntityRef* pPlugEntityRef = pSocketUnit->getFirstEntityRefByType(BEntityRef::cTypeSocketPlug);      
//--
      if (pPlugEntityRef)
      {         
         plugUnitID = pPlugEntityRef->mID;
         result = true;
      }
   }

   if (getVar(cOutPlugUnit)->isUsed())
   {
      getVar(cOutPlugUnit)->asUnit()->writeVar(plugUnitID);
   }

   return (result);
}

//==============================================================================
// Get the player's hover point if possible
//==============================================================================
bool BTriggerCondition::tcCanGetHoverPoint()
{
   enum { cInPlayer = 1, cOutHoverPoint = 2, };
   const BPlayer* pPlayer = gWorld->getPlayer(getVar(cInPlayer)->asPlayer()->readVar());
   if (pPlayer && pPlayer->isPlaying())
   {
      getVar(cOutHoverPoint)->asVector()->writeVar(pPlayer->getLookAtPos());
      return (true);
   }
   else
   {
      getVar(cOutHoverPoint)->asVector()->writeVar(BVector(0.0f, 0.0f, 0.0f));
      return (false);
   }
}


//==============================================================================
// Get the player's hover point if possible
//==============================================================================
bool BTriggerCondition::tcCanGetCoopPlayer()
{
   enum { cTestPlayer = 1, cCoopPlayer = 2, };

   // Not in coop.
   if (!gWorld->getFlagCoop())
   {
      getVar(cCoopPlayer)->asPlayer()->writeVar(cInvalidPlayerID);
      return (false);
   }

   // Not a valid test player.
   BPlayerID testPlayerID = getVar(cTestPlayer)->asPlayer()->readVar();
   const BPlayer* pTestPlayer = gWorld->getPlayer(testPlayerID);
   if (!pTestPlayer || !pTestPlayer->isHuman())
   {
      getVar(cCoopPlayer)->asPlayer()->writeVar(cInvalidPlayerID);
      return (false);
   }

   // Easy check this guy for the coop player.
   if (gWorld->getPlayer(pTestPlayer->getCoopID()))
   {
      getVar(cCoopPlayer)->asPlayer()->writeVar(pTestPlayer->getCoopID());
      return (true);
   }

   // Find his coop buddy.
   long numPlayers = gWorld->getNumberPlayers();
   for (long i=0; i<numPlayers; i++)
   {
      const BPlayer* pPlayer = gWorld->getPlayer(i);
      if (!pPlayer)
         continue;
      if (!pPlayer->isHuman())
         continue;
      if (pPlayer->getCoopID() == testPlayerID)
      {
         getVar(cCoopPlayer)->asPlayer()->writeVar(i);
         return (true);
      }
   }

   getVar(cCoopPlayer)->asPlayer()->writeVar(cInvalidPlayerID);
   return (false);
}

//==============================================================================
// Compare on unit ID to another
//==============================================================================
bool BTriggerCondition::tcCompareUnit()
{
   enum
   {
      cInUnit1 = 1,
      cInOperator = 2,
      cInUnit2 = 3,
   };

   long op = getVar(cInOperator)->asOperator()->readVar();   
   BEntityID unit1 = getVar(cInUnit1)->asUnit()->readVar();
   BEntityID unit2 = getVar(cInUnit2)->asUnit()->readVar();
   switch (op)
   {
      case Math::cNotEqualTo:
         return (unit1 != unit2);
      case Math::cEqualTo:
         return (unit1 == unit2);
      default:
         BFATAL_ASSERTM(false, "Using compareUnit trigger condition without = or !=");
         return (false);
   }
}

//==============================================================================
bool BTriggerCondition::tcASYNCUnitsOnScreenSelected()
{
   enum { cInputPlayer = 1, cSearchRadius = 2, cInputLocation = 3, cIgnoreObjectTypes = 4 };

   BTRIGGER_ASSERT(mbAsyncCondition);
   BTRIGGER_ASSERT(mbLocalAsyncMachine);

   float radius = 50.0f;

   long playerID = getVar(cInputPlayer)->asPlayer()->readVar();
   if (getVar(cSearchRadius)->isUsed())
      radius = getVar(cSearchRadius)->asFloat()->readVar();

   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return (false);

   BUser* pUser = pPlayer->getUser();
   if (!pUser)
      return (false);

   BSelectionManager *pSM = pUser->getSelectionManager();
   if (!pSM)
      return (false);

   BEntityIDArray workingList(0,50);
   BUnitQuery query;

   BVector loc = pPlayer->getLookAtPos();
   if (getVar(cInputLocation)->isUsed())
      loc = getVar(cInputLocation)->asVector()->readVar();

   query = BUnitQuery(loc, radius, true);

   query.addPlayerFilter(playerID);
   query.setFlagIgnoreDead(true);

   gWorld->getSquadsInArea(&query, &workingList);
   uint numInWorkingList = workingList.getSize();

   if (getVar(cIgnoreObjectTypes)->isUsed())
   {
      const BObjectTypeIDArray& ignoreTypes = getVar(cIgnoreObjectTypes)->asObjectTypeList()->readVar();

      for (uint idx = 0; idx < numInWorkingList; idx++)
      {
         BSquad *pSquad = gWorld->getSquad(workingList[idx]);
         if (!pSquad)
            continue;

         bool allChildrenQualify = true;
         uint numSquadChildren = pSquad->getNumberChildren();
         for (uint child = 0; child < numSquadChildren; child++)
         {
            BUnit* pChildUnit = gWorld->getUnit(pSquad->getChild(child));
            if (!pChildUnit)
               continue;

            uint typeCount = ignoreTypes.getNumber();
            for (uint tid=0; tid<typeCount; tid++)
            {
               if (pChildUnit->isType(ignoreTypes[tid]))
               {
                  // this unit is one of the types we should ignore
                  allChildrenQualify = false;
                  break;
               }
            }

            // if we should ignore ANY child unit in the squad, we're done checking the entire squad
            if (!allChildrenQualify)
               break;
         }

         // not all children qualify, so ignore
         if (!allChildrenQualify)
            continue;

         if (!pSM->isSquadSelected(workingList[idx]))
            return false;
      }
   }
   else
   {
      for (uint idx = 0; idx < numInWorkingList; idx++)
      {
         if (!pSM->isSquadSelected(workingList[idx]))
            return false;
      }
   }

   return true;
}

//==============================================================================
bool BTriggerCondition::tcCheckAndSetFalse()
{
   enum { cBool = 2 };

   bool value = getVar(cBool)->asBool()->readVar();
   getVar(cBool)->asBool()->writeVar(false);

   return value;
}

// NEWTRIGGERCONDITION
// Add the body of your trigger condition function here.  All trigger condition functions
// return a bool result and are void functions.  They look up their parameters internally.
// Any questions?  Ask Marc.

//==============================================================================
// BTriggerCondition::compareValues(int val1, BCompareType op, int val2)
//==============================================================================
bool BTriggerCondition::compareValues(int val1, BCompareType op, int val2)
{
   switch(op)
   {
   case Math::cNotEqualTo:
      return (val1 != val2);
   case Math::cLessThan:
      return (val1 < val2);
   case Math::cLessThanOrEqualTo:
      return (val1 <= val2);
   case Math::cEqualTo:
      return (val1 == val2);
   case Math::cGreaterThanOrEqualTo:
      return (val1 >= val2);
   case Math::cGreaterThan:
      return (val1 > val2);
   default:
      BFATAL_ASSERT(false);
      return (false);
   }
}


//==============================================================================
// BTriggerCondition::compareValues(float val1, BCompareType op, float val2)
//==============================================================================
bool BTriggerCondition::compareValues(float val1, BCompareType op, float val2)
{
   switch(op)
   {
   case Math::cNotEqualTo:
      return (val1 != val2);
   case Math::cLessThan:
      return (val1 < val2);
   case Math::cLessThanOrEqualTo:
      return (val1 <= val2);
   case Math::cEqualTo:
      return (val1 == val2);
   case Math::cGreaterThanOrEqualTo:
      return (val1 >= val2);
   case Math::cGreaterThan:
      return (val1 > val2);
   default:
      BFATAL_ASSERT(false);
      return (false);
   }
}


//==============================================================================
// BTriggerCondition::compareValues(DWORD val1, BCompareType op, DWORD val2)
//==============================================================================
bool BTriggerCondition::compareValues(DWORD val1, BCompareType op, DWORD val2)
{
   switch(op)
   {
   case Math::cNotEqualTo:
      return (val1 != val2);
   case Math::cLessThan:
      return (val1 < val2);
   case Math::cLessThanOrEqualTo:
      return (val1 <= val2);
   case Math::cEqualTo:
      return (val1 == val2);
   case Math::cGreaterThanOrEqualTo:
      return (val1 >= val2);
   case Math::cGreaterThan:
      return (val1 > val2);
   default:
      BFATAL_ASSERT(false);
      return (false);
   }
}


//==============================================================================
// BTriggerCondition::compareValues(bool val1, BCompareType op, bool val2)
//==============================================================================
bool BTriggerCondition::compareValues(bool val1, BCompareType op, bool val2)
{
   switch(op)
   {
   case Math::cNotEqualTo:
      return (val1 != val2);
   case Math::cEqualTo:
      return (val1 == val2);
   default:
      BFATAL_ASSERTM(false, "Using compareBool trigger condition without = or !=");
      return (false);
   }
}


//==============================================================================
// BTriggerCondition::compareValues(BSimUString val1, BCompareType op, BSimUString val2)
//==============================================================================
bool BTriggerCondition::compareValues(BSimUString val1, BCompareType op, BSimUString val2)
{
   switch(op)
   {
      case Math::cNotEqualTo:
         return (val1 != val2);

      case Math::cLessThan:
         return (val1 < val2);

      case Math::cLessThanOrEqualTo:
         return (val1 <= val2);

      case Math::cEqualTo:
         return (val1 == val2);

      case Math::cGreaterThanOrEqualTo:
         return (val1 >= val2);

      case Math::cGreaterThan:
         return (val1 > val2);

      default:
         BFATAL_ASSERT(false);
         return (false);
   }
}

//===================================================================================
//===================================================================================
bool BTriggerCondition::compareValues(uint val1, BCompareType op, uint val2)
{
   switch(op)
   {
   case Math::cNotEqualTo:
      return (val1 != val2);
   case Math::cLessThan:
      return (val1 < val2);
   case Math::cLessThanOrEqualTo:
      return (val1 <= val2);
   case Math::cEqualTo:
      return (val1 == val2);
   case Math::cGreaterThanOrEqualTo:
      return (val1 >= val2);
   case Math::cGreaterThan:
      return (val1 > val2);
   default:
      BFATAL_ASSERT(false);
      return (false);
   }
}

//===================================================================================
//===================================================================================
bool BTriggerCondition::compareRelationType(BTeamID teamID1, BRelationType relationType, BTeamID teamID2)
{
   return (gWorld->getTeamRelationType(teamID1, teamID2) == relationType);
}

//==============================================================================
//==============================================================================
void BTriggerCondition::onAcquire()
{
   mpParentTrigger = NULL;
   mDBID = -1;
   mVersion = cTCVersionInvalid;
   mID = -1;
   mpParentTriggerScript = NULL;
   mAsyncKeyVarID = BTriggerVar::cVarSigIDInvalid;
   mbInvert = false;
   mbAsyncCondition = false;
   mbLocalAsyncMachine = false;
   mbLocalMachineState = false;
   mbLastBroadcastState = false;
   mbConsensusState = false;
}


//==============================================================================
//==============================================================================
void BTriggerCondition::onRelease()
{
   mConditionVars.clear();
}


//==============================================================================
// BTriggerCondition::evaluate()
//==============================================================================
bool BTriggerCondition::evaluate()
{
   SCOPEDSAMPLE(BTriggerCondition_evaluate);

   bool evalResult = false;
   switch (mDBID)
   {
   case cTCCanGetUnits:                         { evalResult = tcCanGetUnits(); break; }
   case cTCCanGetSquads:                        { evalResult = tcCanGetSquads(); break; }
   case cTCTechStatus:                          { evalResult = tcTechStatus(); break; }
   case cTCTriggerActiveTime:                   { evalResult = tcTriggerActiveTime(); break; }
   case cTCUnitUnitDistance:                    { evalResult = tcUnitUnitDistance(); break; }
   case cTCUnitLocationDistance:                { evalResult = tcUnitLocationDistance(); break; }
   case cTCUILocationOK:                        { evalResult = tcUILocationOK(); break; }
   case cTCUILocationCancel:                    { evalResult = tcUILocationCancel(); break; }
   case cTCCompareCount:                        { evalResult = tcCompareCount(); break; }
   case cTCCanPayCost:                          { evalResult = tcCanPayCost(); break; }
   case cTCCheckPlacement:                      { evalResult = tcCheckPlacement(); break; }
   case cTCCanUsePower:                         { evalResult = tcCanUsePower(); break; }
   case cTCIsUnitPower:                         { evalResult = tcIsUnitPower(); break; }
   case cTCIsOwnedBy:                           { evalResult = tcIsOwnedBy(); break; }
   case cTCIsProtoObject:                       { evalResult = tcIsProtoObject(); break; }
   case cTCIsAlive:                             { evalResult = tcIsAlive(); break; }
   case cTCIsDead:                              { evalResult = tcIsDead(); break; }
   case cTCCompareBool:                         { evalResult = tcCompareBool(); break; }
   case cTCNextPlayer:                          { evalResult = tcNextPlayer(); break; }
   case cTCNextTeam:                            { evalResult = tcNextTeam(); break; }
   case cTCUnitListLocationDistance:            { evalResult = tcUnitListLocationDistance(); break; }
   case cTCPlayerInState:                       { evalResult = tcPlayerInState(); break; }
   case cTCContainsGarrisoned:                  { evalResult = tcContainsGarrisoned(); break; }
   case cTCNextUnit:                            { evalResult = tcNextUnit(); break; }
   case cTCNextSquad:                           { evalResult = tcNextSquad(); break; }
   case cTCUIUnitOK:                            { evalResult = tcUIUnitOK(); break; }
   case cTCUIUnitCancel:                        { evalResult = tcUIUnitCancel(); break; }
   case cTCUISquadOK:                           { evalResult = tcUISquadOK(); break; }
   case cTCUISquadCancel:                       { evalResult = tcUISquadCancel(); break; }
   case cTCCompareTime:                         { evalResult = tcCompareTime(); break; }
   case cTCCompareString:                       { evalResult = tcCompareString(); break; }
   case cTCComparePercent:                      { evalResult = tcComparePercent(); break; }
   case cTCCompareHitpoints:                    { evalResult = tcCompareHitpoints(); break; }
   case cTCIsMultiplayerActive:                 { evalResult = tcIsMultiplayerActive(); break; }
   case cTCCanRetrieveExternals:                { evalResult = tcCanRetrieveExternals(); break; }
   case cTCSquadLocationDistance:               { evalResult = tcSquadLocationDistance(); break; }
   case cTCCompareCiv:                          { evalResult = tcCompareCiv(); break; }
   case cTCCompareAmmoPercent:                  { evalResult = tcCompareAmmoPercent(); break; }
   case cTCIsHitZoneActive:                     { evalResult = tcIsHitZoneActive(); break; }
   case cTCIsIdle:                              { evalResult = tcIsIdle(); break; }
   case cTCGameTime:                            { evalResult = tcGameTime(); break; }
   case cTCGameTimeReached:                     { evalResult = tcGameTimeReached(); break; }
   case cTCTriggerActiveTimeReached:            { evalResult = tcTriggerActiveTimeReached(); break; }
   case cTCNextObject:                          { evalResult = tcNextObject(); break; }
   case cTCNextLocation:                        { evalResult = tcNextLocation(); break; }
   case cTCRandomListLocation:                  { evalResult = tcRandomListLocation(); break; }
   case cTCRefCountUnit:                        { evalResult = tcRefCountUnit(); break; }
   case cTCRefCountSquad:                       { evalResult = tcRefCountSquad(); break; }
   case cTCUnitFlag:                            { evalResult = tcUnitFlag(); break; }
   case cTCUILocationUILockError:               { evalResult = tcUILocationUILockError(); break; }
   case cTCUIUnitUILockError:                   { evalResult = tcUIUnitUILockError(); break; }
   case cTCUISquadUILockError:                  { evalResult = tcUISquadUILockError(); break; }
   case cTCUILocationWaiting:                   { evalResult = tcUILocationWaiting(); break; }
   case cTCUIUnitWaiting:                       { evalResult = tcUIUnitWaiting(); break; }
   case cTCUISquadWaiting:                      { evalResult = tcUISquadWaiting(); break; }
   case cTCCompareCost:                         { evalResult = tcCompareCost(); break; }
   case cTCCheckResourceTotals:                 { evalResult = tcCheckResourceTotals(); break; }
   case cTCComparePopulation:                   { evalResult = tcComparePopulation(); break; }
   case cTCCanGetOneUnit:                       { evalResult = tcCanGetOneUnit(); break; }
   case cTCCanGetOneSquad:                      { evalResult = tcCanGetOneSquad(); break; }
   case cTCCheckDiplomacy:                      { evalResult = tcCheckDiplomacy(); break; }
   case cTCCheckModeChange:                     { evalResult = tcCheckModeChange(); break; }
   case cTCIsBuilt:                             { evalResult = tcIsBuilt(); break; }
   case cTCIsMoving:                            { evalResult = tcIsMoving(); break; }
   case cTCPlayerIsHuman:                       { evalResult = tcPlayerIsHuman(); break; }
   case cTCPlayerIsGaia:                        { evalResult = tcPlayerIsGaia(); break; }
   case cTCComparePlayers:                      { evalResult = tcComparePlayers(); break; }
   case cTCCompareTeams:                        { evalResult = tcCompareTeams(); break; }
   case cTCCanGetOnePlayer:                     { evalResult = tcCanGetOnePlayer(); break; }
   case cTCCanGetOneTeam:                       { evalResult = tcCanGetOneTeam(); break; }
   case cTCComparePlayerUnitCount:              { evalResult = tcComparePlayerUnitCount(); break; }
   case cTCIsSquadAtMaxSize:                    { evalResult = tcIsSquadAtMaxSize(); break; }
   case cTCCanRetrieveExternalLocation:         { evalResult = tcCanRetrieveExternalLocation(); break; }
   case cTCCanRetrieveExternalLocationList:     { evalResult = tcCanRetrieveExternalLocationList(); break; }
   case cTCNextKBBase:                          { evalResult = tcNextKBBase(); break; }
   case cTCCompareFloat:                        { evalResult = tcCompareFloat(); break; }
   case cTCIsUnderAttack:                       { evalResult = tcIsUnderAttack(); break; }
   case cTCCompareLeader:                       { evalResult = tcCompareLeader(); break; }
   case cTCPlayerUsingLeader:                   { evalResult = tcPlayerUsingLeader(); break; }
   case cTCCanGetOneProtoObject:                { evalResult = tcCanGetOneProtoObject(); break; }
   case cTCCanGetOneProtoSquad:                 { evalResult = tcCanGetOneProtoSquad(); break; }
   case cTCCanGetOneObjectType:                 { evalResult = tcCanGetOneObjectType(); break; }
   case cTCCanGetOneTech:                       { evalResult = tcCanGetOneTech(); break; }
   case cTCNextProtoObject:                     { evalResult = tcNextProtoObject(); break; }
   case cTCNextProtoSquad:                      { evalResult = tcNextProtoSquad(); break; }
   case cTCNextObjectType:                      { evalResult = tcNextObjectType(); break; }
   case cTCNextTech:                            { evalResult = tcNextTech(); break; }
   case cTCHasAttached:                         { evalResult = tcHasAttached(); break; }
   case cTCIsMobile:                            { evalResult = tcIsMobile(); break; }
   case cTCCompareProtoSquad:                   { evalResult = tcCompareProtoSquad(); break; }
   case cTCHasCinematicTagFired:                { evalResult = tcHasCinematicTagFired(); break; }
   case cTCCanGetBuilder:                       { evalResult = tcCanGetBuilder(); break; }
   case cTCUIButtonPressed:                     { evalResult = tcUIButtonPressed(); break; }
   case cTCUIButtonWaiting:                     { evalResult = tcUIButtonWaiting(); break; }
   case cTCCompareAIMissionType:                { evalResult = tcCompareAIMissionType(); break; }
   case cTCCompareAIMissionState:               { evalResult = tcCompareAIMissionState(); break; }
   case cTCCompareAIMissionTargetType:          { evalResult = tcCompareAIMissionTargetType(); break; }
   case cTCCanGetOneInteger:                    { evalResult = tcCanGetOneInteger(); break; }
   case cTCCanGetUnitsAlongRay:                 { evalResult = tcCanGetUnitsAlongRay(); break; }
   case cTCCanRemoveOneInteger:                 { evalResult = tcCanRemoveOneInteger(); break; }
   case cTCCanGetOneLocation:                   { evalResult = tcCanGetOneLocation(); break; }
   case cTCCanRemoveOneLocation:                { evalResult = tcCanRemoveOneLocation(); break; }
   case cTCCompareProtoObject:                  { evalResult = tcCompareProtoObject(); break; }
   case cTCCompareTech:                         { evalResult = tcCompareTech(); break; }
   case cTCProtoObjectListContains:             { evalResult = tcProtoObjectListContains(); break; }
   case cTCProtoSquadListContains:              { evalResult = tcProtoSquadListContains(); break; }
   case cTCTechListContains:                    { evalResult = tcTechListContains(); break; }
   case cTCCanRemoveOneProtoObject:             { evalResult = tcCanRemoveOneProtoObject(); break; }
   case cTCCanRemoveOneProtoSquad:              { evalResult = tcCanRemoveOneProtoSquad(); break; }
   case cTCCanRemoveOneTech:                    { evalResult = tcCanRemoveOneTech(); break; }
   case cTCBidState:                            { evalResult = tcBidState(); break; }
   case cTCBuildingCommandDone:                 { evalResult = tcBuildingCommandDone(); break; }
   case cTCAITopicIsActive:                     { evalResult = tcAITopicIsActive(); break; }
   case cTCComparePlayerSquadCount:             { evalResult = tcComparePlayerSquadCount(); break; }
   case cTCHasGarrisoned:                       { evalResult = tcHasGarrisoned(); break; }
   case cTCIsGarrisoned:                        { evalResult = tcIsGarrisoned(); break; }
   case cTCIsAttached:                          { evalResult = tcIsAttached(); break; }
   case cTCIsPassable:                          { evalResult = tcIsPassable(); break; }
   case cTCCanGetCentroid:                      { evalResult = tcCanGetCentroid(); break; }
   case cTCPlayerIsPrimaryUser:                 { evalResult = tcPlayerIsPrimaryUser(); break; }
   case cTCIsConfigDefined:                     { evalResult = tcIsConfigDefined(); break; }
   case cTCIsCoop:                              { evalResult = tcIsCoop(); break; }
   case cTCCustomCommandCheck:                  { evalResult = tcCustomCommandCheck(); break; }
   case cTCIsObjectType:                        { evalResult = tcIsObjectType(); break; }
   case cTCUISquadListOK:                       { evalResult = tcUISquadListOK(); break; }
   case cTCUISquadListCancel:                   { evalResult = tcUISquadListCancel(); break; }
   case cTCUISquadListUILockError:              { evalResult = tcUISquadListUILockError(); break; }
   case cTCUISquadListWaiting:                  { evalResult = tcUISquadListWaiting(); break; }
   case cTCIsTimerDone:                         { evalResult = tcIsTimerDone(); break; }
   case cTCNextKBSquad:                         { evalResult = tcNextKBSquad(); break; }
   case cTCIsAttacking:                         { evalResult = tcIsAttacking(); break; }
   case cTCCanGetUnitLaunchLocation:            { evalResult = tcCanGetUnitLaunchLocation(); break; }
   case cTCIsGathering:                         { evalResult = tcIsGathering(); break; }
   case cTCIsCapturing:                         { evalResult = tcIsCapturing(); break; }
   case cTCPlayerIsComputerAI:                  { evalResult = tcPlayerIsComputerAI(); break; }
   case cTCCanGetGreatestThreat:                { evalResult = tcCanGetGreatestThreat(); break; }
   case cTCCanGetTargetedSquad:                 { evalResult = tcCanGetTargetedSquad(); break; }
   case cTCIsObjectiveComplete:                 { evalResult = tcIsObjectiveComplete(); break; }
   case cTCCompareVector:                       { evalResult = tcCompareVector(); break; }
   case cTCGetTableRow:                         { evalResult = tcGetTableRow(); break; }
   case cTCCheckPop:                            { evalResult = tcCheckPop(); break; }                                              
   case cTCCanGetObjects:                       { evalResult = tcCanGetObjects(); break; }
   case cTCIsHitched:                           { evalResult = tcIsHitched(); break; }
   case cTCHasHitched:                          { evalResult = tcHasHitched(); break; }
   case cTCSquadSquadDistance:                  { evalResult = tcSquadSquadDistance(); break; }
   case cTCEventTriggered:                      { evalResult = tcEventTriggered(); break; }
   case cTCIsInQueue:                           { evalResult = tcIsInQueue(); break; }
   case cTCCompareLocStringID:                  { evalResult = tcCompareLocStringID(); break; }
   case cTCCanGetOneObject:                     { evalResult = tcCanGetOneObject(); break; }
   case cTCCanGetCorpseUnits:                   { evalResult = tcCanGetCorpseUnits(); break; }
   case cTCCanGetOneTime:                       { evalResult = tcCanGetOneTime(); break; }
   case cTCCanGetOneDesignLine:                 { evalResult = tcCanGetOneDesignLine(); break; }
   case cTCCompareDesignLine:                   { evalResult = tcCompareDesignLine(); break; }
   case cTCCanRetrieveExternalFlag:             { evalResult = tcCanRetrieveExternalFlag(); break; }
   case cTCAICanGetTopicFocus:                  { evalResult = tcAICanGetTopicFocus(); break; }
   case cTCCanGetOneFloat:                      { evalResult = tcCanGetOneFloat(); break; }
   case cTCUILocationMinigameWaiting:           { evalResult = tcUILocationMinigameWaiting(); break; }
   case cTCUILocationMinigameOK:                { evalResult = tcUILocationMinigameOK(); break; }
   case cTCUILocationMinigameCancel:            { evalResult = tcUILocationMinigameCancel(); break; }
   case cTCUILocationMinigameUILockError:       { evalResult = tcUILocationMinigameUILockError(); break; }
   case cTCIsSelectable:                        { evalResult = tcIsSelectable(); break; }
   case cTCChatCompleted:                       { evalResult = tcChatCompleted(); break; }
   case cTCCinematicCompleted:                  { evalResult = tcCinematicCompleted(); break; }
   case cTCSquadFlag:                           { evalResult = tcSquadFlag(); break; }
   case cTCCanGetSocketUnits:                   { evalResult = tcCanGetSocketUnits(); break; }
   case cTCCanGetOneSocketUnit:                 { evalResult = tcCanGetOneSocketUnit(); break; }
   case cTCFadeCompleted:                       { evalResult = tcFadeCompleted(); break; }
   case cTCIsAutoAttackable:                    { evalResult = tcIsAutoAttackable(); break; }
   case cTCIsBeingGatheredFrom:                 { evalResult = tcIsBeingGatheredFrom(); break; }
   case cTCAICanGetDifficultySetting:           { evalResult = tcAICanGetDifficultySetting(); break; }
   case cTCConceptGetParent:                    { evalResult = tcConceptGetParent(); break; }
   case cTCConceptGetCommand:                   { evalResult = tcConceptGetCommand(); break; }
   case cTCConceptGetStateChange:               { evalResult = tcConceptGetStateChange(); break; }
   case cTCConceptCompareState:                 { evalResult = tcConceptCompareState(); break; }
   case cTCIsEmptySocketUnit:                   { evalResult = tcIsEmptySocketUnit(); break; }
   case cTCIsForbidden:                         { evalResult = tcIsForbidden(); break; }
   case cTCCanGetSocketParentBuilding:          { evalResult = tcCanGetSocketParentBuilding(); break; }
   case cTCCanGetRandomLocation:                { evalResult = tcCanGetRandomLocation(); break; }
   case cTCCheckDifficulty:                     { evalResult = tcCheckDifficulty(); break; }
   case cTCCanGetDesignSpheres:                 { evalResult = tcCanGetDesignSpheres(); break; }
   case cTCCanRemoveOneFloat:                   { evalResult = tcCanRemoveOneFloat(); break; }
   case cTCCanRetrieveExternalFloat:            { evalResult = tcCanRetrieveExternalFloat(); break; }
   case cTCMarkerSquadsInArea:                  { evalResult = tcMarkerSquadsInArea(); break; }
   case cTCCanGetSocketPlugUnit:                { evalResult = tcCanGetSocketPlugUnit(); break; }
   case cTCCanGetHoverPoint:                    { evalResult = tcCanGetHoverPoint(); break; }
   case cTCCanGetCoopPlayer:                    { evalResult = tcCanGetCoopPlayer(); break; }
   case cTCCompareUnit:                         { evalResult = tcCompareUnit(); break; }
   case cTCCheckAndSetFalse:                    { evalResult = tcCheckAndSetFalse(); break; }
   case cTCAITopicGetTickets:                   { evalResult = tcAITopicGetTickets(); break; }

   // NEWTRIGGERCONDITION
   // Add case statement calling your trigger condition here, assigning the bool return value to evalResult.
   // Follow the conventions as shown.
   // IMPORTANT:  If your trigger condition is an ASYNC condition (unusual), do NOT call it here.
   // ASYNC trigger conditions are handled separately.
   // Any questions?  Ask Marc.

   default:
      {
         if (mbAsyncCondition)
         {
            evalResult = evaluateAsyncConditionConsensusState();
         }
         else
         {
            BTRIGGER_ASSERT(false);
            evalResult = false;
         }
      }
   }// switch(mDBID)
   
   // Do we invert the condition result?
   evalResult = mbInvert ? !evalResult : evalResult;

   // Sync the trigger conditions.
   syncTriggerData("BTriggerCondition::evaluate() - evaluated condition DBID = ", mDBID);
   syncTriggerData("BTriggerCondition::evaluate() - evaluated condition ID = ", mID);
   syncTriggerData("BTriggerCondition::evaluate() - evaluated condition Version = ", static_cast<DWORD>(mVersion));
   syncTriggerData("BTriggerCondition::evaluate() - evaluated condition Result = ", evalResult);

   return (evalResult);
}


//==============================================================================
// BTriggerCondition::updateAsyncCondition()
//==============================================================================
void BTriggerCondition::updateAsyncCondition()
{
   if (!mbAsyncCondition || !mbLocalAsyncMachine)
      return;

   bool newEvalState = false;
   switch (mDBID)
   {
   case BTriggerCondition::cTCPlayerSelectingUnit:
      newEvalState = tcPlayerSelectingUnit();
      break;
   case BTriggerCondition::cTCPlayerLookingAtUnit:
      newEvalState = tcPlayerLookingAtUnit();
      break;
   case BTriggerCondition::cTCPlayerLookingAtLocation:
      newEvalState = tcPlayerLookingAtLocation();
      break;
   case BTriggerCondition::cTCPlayerSelectingSquad:
      newEvalState = tcPlayerSelectingSquad();
      break;
   case BTriggerCondition::cTCIsUserModeNormal:
      newEvalState = tcIsUserModeNormal();
      break;

   case BTriggerCondition::cTCASYNCUnitsOnScreenSelected:
      newEvalState = tcASYNCUnitsOnScreenSelected();
      break;

      // NEWTRIGGERCONDITION (SPECIAL CASE FOR ASYNC CONDITIONS)
      // IF, AND ONLY IF, your trigger condition is asynchronous, which should be very rarely and judiciously used, call the function here
      // An example would be tcPlayerSelectingUnit() which checks whether a player is selecting a specific unit, something that only happens
      // Asynchronously on one person's machine...
      // Any questions?  Ask Marc.

   default:
      BTRIGGER_ASSERT(false);
      return;
   }

   // don't forget to handle the invert case!
   newEvalState = mbInvert ? !newEvalState : newEvalState;

   if (mbLocalMachineState != newEvalState)
   {
      mbLocalMachineState = newEvalState;
   }
   if (mbLastBroadcastState != newEvalState)
   {
      broadcastLocalMachineState();
      mbLastBroadcastState = newEvalState;
   }  
}


//==============================================================================
// BTriggerCondition::broadcastLocalMachineState()
//==============================================================================
void BTriggerCondition::broadcastLocalMachineState()
{
   long playerID = getVar(mAsyncKeyVarID)->asPlayer()->readVar();
   BTriggerCommand *c = (BTriggerCommand*)gWorld->getCommandManager()->createCommand(playerID, cCommandTrigger);
   if (!c)
      return;

   c->setSenders(1, &playerID);
   c->setSenderType(BCommand::cPlayer);
   c->setRecipientType(BCommand::cPlayer);
   c->setType(BTriggerCommand::cTypeBroadcastAsyncCondition);
   c->setTriggerScriptID(getParentTriggerScript()->getID());
   c->setTriggerID(mpParentTrigger->getID());
   c->setConditionID(mID);
   c->setAsyncCondition(mbLocalMachineState);
   gWorld->getCommandManager()->addCommandToExecute(c);
}


//==============================================================================
// BTriggerCondition::initAsyncSettings()
//==============================================================================
void BTriggerCondition::initAsyncSettings()
{
   mbLocalAsyncMachine = false;
   mbLocalMachineState = false;
   mbLastBroadcastState = false;
   mbConsensusState = false;

   // If we determined this was an async condition, see if this is the local machine for it to be evaluated on.
   if (mbAsyncCondition)
   {
      long playerID = getVar(mAsyncKeyVarID)->asPlayer()->readVar();
      // Check for any user on this machine
      if (gUserManager.getUserByPlayerID(playerID))
         mbLocalAsyncMachine = true;
   }
}


//==============================================================================
// BTriggerCondition::loadFromXML(BXMLNode  node, BTrigger *pParentTrigger)
//==============================================================================
bool BTriggerCondition::loadFromXML(BXMLNode  node, BTrigger *pParentTrigger)
{
   BFATAL_ASSERT(node);

   // Get the trigger condition DBID.
   mDBID = -1;
   if (!node.getAttribValueAsLong("DBID", mDBID))
      return (false);
   if (mDBID == -1)
      return (false);

   // Get the trigger version.
   DWORD versionDWORD = cTCVersionInvalid;
   if (!node.getAttribValueAsDWORD("Version", versionDWORD))
      return (false);
   if (versionDWORD == cTCVersionInvalid)
      return (false);
   mVersion = versionDWORD;

   // read in the ID here please.
   mID = 0;
   if (!node.getAttribValueAsLong("ID", mID))
      return (false);
   if (mID < 0)
      return (false);

   // Read in whether to invert this or not.
   const BXMLAttribute pAttribute = node.getAttribute("Invert");
   if (pAttribute)
   {
      bool invert = false;
      pAttribute.getValueAsBool(invert);
      mbInvert = invert;
   }

   // Get our mapping for the sig ids.
   DWORD maxSigID = 0;
   long numTriggerVarNodes = node.getNumberChildren();
   for (long i=0; i<numTriggerVarNodes; i++)
   {
      BXMLNode varNode(node.getChild(i));
      DWORD varDBIDDWORD = 0;
      if (varNode.getAttribValueAsDWORD("SigID", varDBIDDWORD) && varDBIDDWORD > maxSigID)
         maxSigID = varDBIDDWORD;
   }

   // If we have some variables, add one slot in the array for the invalid sigid.  (KIND OF A HACK)
   //if (maxSigID > 0)
   //   maxSigID++;5

   mConditionVars.resize(maxSigID);
   for (uint i=0; i<maxSigID; i++)
      mConditionVars[i] = NULL;

   // Get all the inputs and outputs for this trigger condition
   for (long i=0; i<numTriggerVarNodes; i++)
   {
      BXMLNode  varNode(node.getChild(i));
      
      // Get the trigger var ID
      long triggerVarIDLong = BTriggerVar::cVarIDInvalid;
      if (!varNode.getTextAsLong(triggerVarIDLong))
         return (false);
      BTRIGGER_ASSERT(triggerVarIDLong < BTriggerVar::cVarIDMax);
      BTriggerVarID triggerVarID = (BTriggerVarID)triggerVarIDLong;
      BTriggerVar* pVar = getParentTriggerScript()->getTriggerVar(triggerVarID);
      if (!pVar)
         return (false);

      // Get the signature ID of the var (the DBID) that uniquely identifies it within the signature.
      DWORD varDBIDDWORD = BTriggerVar::cVarSigIDInvalid;
      if (!varNode.getAttribValueAsDWORD("SigID", varDBIDDWORD))
         return (false);
      if (varDBIDDWORD == BTriggerVar::cVarSigIDInvalid)
         return (false);

      BTRIGGER_ASSERT(varDBIDDWORD < BTriggerVar::cVarSigIDMax);
      BTriggerVarSigID varDBID = (BTriggerVarSigID)varDBIDDWORD;

      mConditionVars[varDBID-1] = pVar;
   }

   // Read in whether the condition is Async or not.
   bool isAsync = false;
   if (node.getAttribValueAsBool("Async", isAsync))
   {
      mbAsyncCondition = isAsync;
   }
   if (isAsync)
   {
      DWORD asyncKeyVarDWORD = BTriggerVar::cVarSigIDInvalid;
      node.getAttribValueAsDWORD("AsyncParameterKey", asyncKeyVarDWORD);
      BTRIGGER_ASSERT(asyncKeyVarDWORD != BTriggerVar::cVarSigIDInvalid && asyncKeyVarDWORD < BTriggerVar::cVarSigIDMax);
      BTriggerVarSigID asyncKeyVarID = (BTriggerVarSigID)asyncKeyVarDWORD;
      if (!getVar(asyncKeyVarID))
         return (false);
      mAsyncKeyVarID = asyncKeyVarID;
   }

   return (true);
}

//==============================================================================
//==============================================================================
bool BTriggerCondition::save(BStream* pStream, int saveType) const
{
   uint8 varCount = (uint8)mConditionVars.size();
   GFVERIFYCOUNT(varCount, 200);
   GFWRITEVAR(pStream, uint8, varCount);
   for (uint8 i=0; i<varCount; i++)
   {
//-- FIXING PREFIX BUG ID 3930
      const BTriggerVar* pVar = mConditionVars[i];
//--
      BTriggerVarID varID = (pVar ? pVar->getID() : UINT16_MAX);
      GFWRITEVAR(pStream, BTriggerVarID, varID);
   }

   GFWRITEVAR(pStream, long, mDBID);
   GFWRITEVAR(pStream, long, mID);
   GFWRITEVAR(pStream, int8, mVersion);
   GFWRITEVAR(pStream, BTriggerVarSigIDSmall, mAsyncKeyVarID);

   GFWRITEBITBOOL(pStream, mbInvert);
   GFWRITEBITBOOL(pStream, mbAsyncCondition);
   GFWRITEBITBOOL(pStream, mbLocalAsyncMachine);
   GFWRITEBITBOOL(pStream, mbLocalMachineState);
   GFWRITEBITBOOL(pStream, mbLastBroadcastState);
   GFWRITEBITBOOL(pStream, mbConsensusState);

   return true;
}

//==============================================================================
//==============================================================================
bool BTriggerCondition::load(BStream* pStream, int saveType)
{
   mConditionVars.clear();
   uint8 varCount;
   GFREADVAR(pStream, uint8, varCount);
   GFVERIFYCOUNT(varCount, 200);

   if (mGameFileVersion < 2 && varCount > 0)
      mConditionVars.reserve(varCount-1);
   else
      mConditionVars.reserve(varCount);

   for (uint8 i=0; i<varCount; i++)
   {
      BTriggerVarID varID;
      GFREADVAR(pStream, BTriggerVarID, varID);
      if (mGameFileVersion == 1 && i == 0)      // the first value in version 1 was always NULL
         continue;

      BTriggerVar* pVar = (varID != UINT16_MAX ? mpParentTriggerScript->getTriggerVar(varID) : NULL);
      mConditionVars.add(pVar);
   }

   //BTriggerScript* mpParentTriggerScript;
   //BTrigger* mpParentTrigger;

   GFREADVAR(pStream, long, mDBID);
   GFREADVAR(pStream, long, mID);
   GFREADVAR(pStream, int8, mVersion);
   GFREADVAR(pStream, BTriggerVarSigIDSmall, mAsyncKeyVarID);

   GFREADBITBOOL(pStream, mbInvert);
   GFREADBITBOOL(pStream, mbAsyncCondition);
   GFREADBITBOOL(pStream, mbLocalAsyncMachine);
   GFREADBITBOOL(pStream, mbLocalMachineState);
   GFREADBITBOOL(pStream, mbLastBroadcastState);
   GFREADBITBOOL(pStream, mbConsensusState);

   return true;
}

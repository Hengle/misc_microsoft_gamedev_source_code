//==============================================================================
// unitquery.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "unit.h"
#include "unitquery.h"
#include "world.h"
#include "database.h"
#include "protoobject.h"
#include "team.h"

//==============================================================================
// BUnitQuery::BUnitQuery()
// Constructor for query type cQueryTypeNone
//==============================================================================
BUnitQuery::BUnitQuery() :
   mQueryType(cQueryTypeNone),
   mRelationPlayerID(cInvalidPlayerID),
   mRelationType(cRelationTypeAny),
   mpVisibleUnitsArray(NULL)
{
   clearFlags();
}

//==============================================================================
// BUnitQuery::BUnitQuery()
// Constructor for query type cQueryTypeTwoVector
//==============================================================================
BUnitQuery::BUnitQuery(BVector point0, BVector point1) :
   mQueryType(cQueryTypeTwoVector),
   mRelationPlayerID(cInvalidPlayerID),
   mRelationType(cRelationTypeAny),
   mpVisibleUnitsArray(NULL),
   mPoint0(point0),
   mPoint1(point1)
{
   clearFlags();
}

//==============================================================================
// BUnitQuery::BUnitQuery()
// Constructor for query type cQueryTypePointRadius
//==============================================================================
BUnitQuery::BUnitQuery(BVector center, float radius, bool circular) :
   mQueryType(cQueryTypePointRadius),
   mRelationPlayerID(cInvalidPlayerID),
   mRelationType(cRelationTypeAny),
   mpVisibleUnitsArray(NULL),
   mCenter(center),
   mRadius(radius),
   mCircular(circular)
{
   clearFlags();
}

//==============================================================================
// BUnitQuery::BUnitQuery()
// Constructor for query type cQueryTypeConvexHull
//==============================================================================
BUnitQuery::BUnitQuery(BConvexHull *pConvexHull) :
   mQueryType(cQueryTypeConvexHull),
   mRelationPlayerID(cInvalidPlayerID),
   mRelationType(cRelationTypeAny),
   mpVisibleUnitsArray(NULL),
   mpConvexHull(pConvexHull)
{
   clearFlags();
}

//==============================================================================
// BUnitQuery::BUnitQuery()
// Constructor for query type cQueryTypeOPQuadHull
//==============================================================================
BUnitQuery::BUnitQuery(BOPQuadHull *pQuadHull) :
   mQueryType(cQueryTypeOPQuadHull),
   mRelationPlayerID(cInvalidPlayerID),
   mRelationType(cRelationTypeAny),
   mpVisibleUnitsArray(NULL),
   mpOPQuadHull(pQuadHull)
{
   clearFlags();
}

//==============================================================================
// BUnitQuery::BUnitQuery()
// Constructor for query type cQueryTypeOPQuadHull
//==============================================================================
BUnitQuery::BUnitQuery(BVector center, BVector forward, BVector right, float extentX, float extentY, float extentZ) :
   mQueryType(cQueryTypeOBBNoRoll),
   mRelationPlayerID(cInvalidPlayerID),
   mRelationType(cRelationTypeAny),
   mpVisibleUnitsArray(NULL),
   mCenter(center),
   mPoint0(forward),
   mPoint1(right)
{
   mPoint2.x = extentX;
   mPoint2.y = extentY;
   mPoint2.z = extentZ;

   clearFlags();
}

//==============================================================================
// BUnitQuery::~BUnitQuery()
//==============================================================================
BUnitQuery::~BUnitQuery()
{
}

//==============================================================================
// BUnitQuery::passesQueryFilter()
// Returns true if the BEntityID passes the query filters, false if it fails and
// needs to be filtered out of the results.
//==============================================================================
bool BUnitQuery::passesQueryFilter(BUnit& unit) const
{
   const BProtoObject *pProtoObject = unit.getProtoObject();
   if (!pProtoObject)
      return(false);
   long playerID = unit.getPlayerID();

   // See if we pass the object type filter.
   long numObjectTypeFilters = mObjectTypeFilters.getNumber();
   if (numObjectTypeFilters > 0)
   {
      bool passesType = false;
      for (long i=0; i<numObjectTypeFilters; i++)
      {
         if (pProtoObject->isType(mObjectTypeFilters[i]))
         {
            passesType = true;
            break;
         }
      }
      if (!passesType)
         return(false);
   }

   // See if we pass the player type filter.
   long numPlayerFilters = mPlayerFilters.getNumber();
   if (numPlayerFilters > 0)
   {
      if (mPlayerFilters.find(playerID) == cInvalidIndex)
         return(false);
   }

   // Relation
   if(mRelationType!=cRelationTypeAny)
   {
      switch(mRelationType)
      {
         case cRelationTypeSelf:
            if(playerID!=mRelationPlayerID)
               return(false);
            break;
         case cRelationTypeAlly:
            if(playerID!=mRelationPlayerID)
            {
               if(!gWorld->getPlayer(playerID)->isAlly(mRelationPlayerID))
                  return(false);
            }
            break;
         case cRelationTypeEnemy:
         {
            if(playerID==mRelationPlayerID)
               return(false);
            if(!gWorld->getPlayer(playerID)->isEnemy(mRelationPlayerID))
               return(false);
            if(pProtoObject->getFlagNeutral())
               return(false);
            break;
         }
         default:
            BASSERT(false);
            break;
      }
   }

   // Visible
   if(mpVisibleUnitsArray)
   {
      if(!mpVisibleUnitsArray->isBitSet(unit.getID().getIndex()))
         return(false);
   }

   // See if we pass the living status.
   bool isAlive = unit.isAlive();
   if (getFlagIgnoreLiving() && isAlive)
      return(false);
   if (getFlagIgnoreDead() && !isAlive)
      return(false);

   return(true);
}

//==============================================================================
// BUnitQuery::setRelation
//==============================================================================
void BUnitQuery::setRelation(BPlayerID playerID, BRelationType relationType)
{
   mRelationPlayerID=playerID;
   mRelationType=relationType;
}

//==============================================================================
// BUnitQuery::setUnitVisibility
//==============================================================================
void BUnitQuery::setUnitVisibility(long playerID)
{
   BPlayer* pPlayer=gWorld->getPlayer(playerID);
   if (!pPlayer)
      return;
   BTeam* pTeam = pPlayer->getTeam();
   if (!pTeam)
      return;
   mpVisibleUnitsArray = pTeam->getVisibleUnits();
}

//==============================================================================
// BUnitQuery::clearFlags
//==============================================================================
void BUnitQuery::clearFlags()
{
   // Flags
   mFlagIgnoreLiving=false;
   mFlagIgnoreDead=false;
   //mFlagAllowObjects=false;
}

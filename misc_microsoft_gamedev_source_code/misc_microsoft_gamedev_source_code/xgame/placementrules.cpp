//=============================================================================
// placementrules.cpp
//
// Copyright (c) 2006 Ensemble Studios
//=============================================================================

// xgame
#include "common.h"
#include "database.h"
#include "gamedirectories.h"
#include "placementrules.h"
#include "protoobject.h"
#include "unit.h"
#include "unitquery.h"
#include "world.h"

// xsystem
#include "xmlreader.h"

BEntityIDArray BPlacementRules::sUnitsToTrack(0,100);
float BPlacementRules::sMaxDistance = 0.0f;

//=============================================================================
// BPlacementRule::loadXML
//=============================================================================
bool BPlacementRule::loadXML(BXMLNode node)
{
   // Sanity.
   if(!node)
      return(false);

   // Look for error string attrib.
   node.getAttribValueAsLong("errorStringID", mErrorStringID);
   node.getAttribValueAsLong("successStringID", mSuccessStringID);
   return(true);
}

//=============================================================================
//=============================================================================
int BPlacementRule::getSpecialType(const char* pName) const
{
   if (stricmp(pName, "_Builder")==0)
      return cSpecialTypeBuilder;
   return -1;
}

//=============================================================================
// BPlacementRuleAnd::~BPlacementRuleAnd
//=============================================================================
BPlacementRuleAnd::~BPlacementRuleAnd(void)
{
   // Kill child rules.
   for(long i=0; i<mChildRules.getNumber(); i++)
      delete mChildRules[i];
   mChildRules.setNumber(0);
}


//=============================================================================
// BPlacementRuleAnd::evaluate
//=============================================================================
bool BPlacementRuleAnd::evaluate(const BUnit* pBuilder, const long protoID, const long playerID, const BVector &location, const BVector &forward,
                                 const BVector &right, long losMode, DWORD flags, long *errorStringID, long *successStringID, BEntityIDArray *pCachedUnitsToTrack) const
{
   bool success = true;

   // Check each rule.
   for(long i=0; i<mChildRules.getNumber(); i++)
   {
      if (mChildRules[i] == NULL)
         continue;               // if the rule is somehow gone, then we figure this rule is ok.

      // Evaluate this child.
      if(!mChildRules[i]->evaluate(pBuilder, protoID, playerID, location, forward, right, losMode, flags, errorStringID, successStringID, pCachedUnitsToTrack))
      {
         // Set error string.
         if(errorStringID && *errorStringID<0)
            *errorStringID= mChildRules[i]->getErrorStringID();

         // Since this is an AND clause, we can bail as soon as we hit a false.
         success = false;
      }
   }

   // set the success string if not already set
   if(successStringID && *successStringID<0)
      *successStringID = mSuccessStringID;

   // They all passed.
   return success;
}


//=============================================================================
// BPlacementRuleAnd::loadXML
//=============================================================================
bool BPlacementRuleAnd::loadXML(BXMLNode node)
{
   // Sanity.
   if(!node)
   {
      BFAIL("Null node.");
      return(false);
   }

   // Load parent stuff.
   BPlacementRule::loadXML(node);

   // Walk each child.
   for(long i=0; i<node.getNumberChildren(); i++)
   {
      // Get child.
      BXMLNode child(node.getChild(i));
      if(!child)
         continue;

      // Create rule.
      BPlacementRule *rule = BPlacementRules::createFromXML(child);
      if(!rule)
         continue;

      // Load it.
      bool ok = rule->loadXML(child);
      if(!ok)
      {
         // Clean up this rule since it failed to load.
         delete rule;
         continue;
      }

      // Add it to our list.
      mChildRules.add(rule);
   }

   return(true);
}

//=============================================================================
// BPlacementRuleAnd::addPlacementRuleUnits
//=============================================================================
bool BPlacementRuleAnd::addPlacementRuleUnits(BDynamicSimArray<BPlacementRuleUnit>& placementRuleUnits)
{
   bool retval=false;
   for(long i=0; i<mChildRules.getNumber(); i++)
   {
      if (mChildRules[i] != NULL)
      {
         if(mChildRules[i]->addPlacementRuleUnits(placementRuleUnits))
            retval=true;
      }
   }
   return retval;
}

//=============================================================================
// BPlacementRuleOr::evaluate
//=============================================================================
bool BPlacementRuleOr::evaluate(const BUnit* pBuilder, const long protoID, const long playerID, const BVector &location, const BVector &forward,
                                const BVector &right, long losMode, DWORD flags, long *errorStringID, long *successStringID, BEntityIDArray *pCachedUnitsToTrack) const
{
   bool success = false;

   // Check each rule.
   for(long i=0; i<mChildRules.getNumber(); i++)
   {
      // Evaluate this child.
      if(mChildRules[i]->evaluate(pBuilder, protoID, playerID, location, forward, right, losMode, flags, errorStringID, successStringID, pCachedUnitsToTrack))
      {
         // set our string if this is not already set
         if(successStringID && *successStringID<0)
            *successStringID = mSuccessStringID;

         // Since this is an OR clause, we can bail as soon as we hit a true.
         success = true;
      }
   }

   // Set error string if not set
   if(errorStringID && *errorStringID<0)
      *errorStringID = mErrorStringID;

   // None passed.
   return success;
}


//=============================================================================
// BPlacementRuleOr::~BPlacementRuleOr
//=============================================================================
BPlacementRuleOr::~BPlacementRuleOr(void)
{
   // Kill child rules.
   for(long i=0; i<mChildRules.getNumber(); i++)
      delete mChildRules[i];
   mChildRules.setNumber(0);
}


//=============================================================================
// BPlacementRuleOr::loadXML
//=============================================================================
bool BPlacementRuleOr::loadXML(BXMLNode node)
{
   // Sanity.
   if(!node)
   {
      BFAIL("Null node.");
      return(false);
   }

   // Load parent stuff.
   BPlacementRule::loadXML(node);

   // Walk each child.
   for(long i=0; i<node.getNumberChildren(); i++)
   {
      // Get child.
      BXMLNode child(node.getChild(i));
      if(!child)
         continue;

      // Create rule.
      BPlacementRule *rule = BPlacementRules::createFromXML(child);
      if(!rule)
         continue;

      // Load it.
      bool ok = rule->loadXML(child);
      if(!ok)
      {
         // Clean up this rule since it failed to load.
         delete rule;
         continue;
      }

      // Add it to our list.
      mChildRules.add(rule);
   }

   return(true);
}

//=============================================================================
// BPlacementRuleOr::addPlacementRuleUnits
//=============================================================================
bool BPlacementRuleOr::addPlacementRuleUnits(BDynamicSimArray<BPlacementRuleUnit>& placementRuleUnits)
{
   bool retval=false;
   for(long i=0; i<mChildRules.getNumber(); i++)
   {
      if (mChildRules[i] != NULL)
      {
         if(mChildRules[i]->addPlacementRuleUnits(placementRuleUnits))
            retval=true;
      }
   }
   return retval;
}




//=============================================================================
// BPlacementRuleDistanceAtMostFromType::BPlacementRuleDistanceAtMostFromType
//=============================================================================
BPlacementRuleDistanceAtMostFromType::BPlacementRuleDistanceAtMostFromType(void) :
mDistance(0.0f),
mType(-1),
mPlayerScope(cPlayerScopeAny),
mLifeScope(cLifeScopeAlive),
mFlagSpecialType(false)
{
}


// we are overloading this for the Native Town Rule, we may want to break it out like the trade route stuff.
//=============================================================================
// BPlacementRuleDistanceAtMostFromType::evaluate
//=============================================================================
bool BPlacementRuleDistanceAtMostFromType::evaluate(const BUnit* pBuilder, const long protoID, const long playerID, const BVector &location, const BVector &forward,
                                                    const BVector &right, long losMode, DWORD flags, long *errorStringID, long *successStringID, BEntityIDArray *pCachedUnitsToTrack) const
{
   //protoID; playerID; location; forward; right; losMode; flags; errorStringID, successStringID;

   // Co-op support
   BPlayerID coopPlayerID=(gWorld->getFlagCoop() ? gWorld->getPlayer(playerID)->getCoopID() : cInvalidPlayerID);

   // Find objects.
   BVector loc = location;
   bool success = false;
   float distanceSqr = mDistance * mDistance;

   const BEntityIDArray &unitsToTrack = pCachedUnitsToTrack ? *pCachedUnitsToTrack : BPlacementRules::getUnitsToTrack();
   long numResults = unitsToTrack.getNumber();
   for(long i=0; i<numResults; i++)
   {
      // Get unit.
      BUnit *pUnit = gWorld->getUnit(unitsToTrack[i]);
      if (!pUnit || !pUnit->getPlayer())
         continue;
      if ((mFlagSpecialType && pUnit!=pBuilder) || (!mFlagSpecialType && !pUnit->isType(mType)))
         continue;
      //if (!unit->isFullyBuilt())
      //   continue;

      if (!gDatabase.checkPlayerScope(pUnit, playerID, mPlayerScope))
      {
         if (coopPlayerID==-1)
            continue;
         if (!gDatabase.checkPlayerScope(pUnit, coopPlayerID, mPlayerScope))
            continue;
      }

      if (!gDatabase.checkLifeScope(pUnit, mLifeScope))
         continue;
      
      // Show allowable areas
      if (flags & cRenderFlag)
      {
         float thickness = 0.1f;
         gTerrainSimRep.addDebugThickCircleOverTerrain(pUnit->getPosition(), mDistance, thickness, cDWORDGreen, 0.1f);
      }

      // Find maximum distance from builder center to any vertex on placed building obstruction hull
      float bestDistSqr = 0.0f;
      BVector basePos = pUnit->getPosition();
      BVector corner[4];

      BProtoObject *pPlacedBuildingProto = pUnit->getPlayer()->getProtoObject(protoID);
      float obsRadiusX = pPlacedBuildingProto->getObstructionRadiusX();
      float obsRadiusZ = pPlacedBuildingProto->getObstructionRadiusZ();

      corner[0].x = loc.x - obsRadiusX;
      corner[0].z = loc.z - obsRadiusZ;
      corner[1].x = loc.x + obsRadiusX;
      corner[1].z = loc.z - obsRadiusZ;
      corner[2].x = loc.x + obsRadiusX;
      corner[2].z = loc.z + obsRadiusZ;
      corner[3].x = loc.x - obsRadiusX;
      corner[3].z = loc.z + obsRadiusZ;

      for(long cornerPoint=0; cornerPoint<4; cornerPoint++)
      {

         float thisDistSqr = basePos.distanceSqr(BVector(corner[cornerPoint].x, basePos.y, corner[cornerPoint].z));
         if(thisDistSqr>bestDistSqr)
            bestDistSqr=thisDistSqr;
      }

      if (!success && (bestDistSqr <= distanceSqr))
      {
         success = true;
         if (!(flags & cRenderFlag))
            break;
      }
   }

   return success;
}


//=============================================================================
// BPlacementRuleDistanceAtMostFromType::loadXML
//=============================================================================
bool BPlacementRuleDistanceAtMostFromType::loadXML(BXMLNode node)
{
   // Sanity.
   if(!node)
   {
      BFAIL("Null node.");
      return(false);
   }

   // Load parent stuff.
   BPlacementRule::loadXML(node);

   // Get type
   BSimString tempStr;
   node.getText(tempStr);
   mType = getSpecialType(tempStr);
   if (mType != -1)
      mFlagSpecialType=true;
   else
   {
      mType = gDatabase.getObjectType(tempStr);
      if (mType < 0)
         return(false);
   }

   if (!node.getAttribValueAsFloat("distance", mDistance))
      return(false);
   BPlacementRules::sMaxDistance = Math::Max(BPlacementRules::sMaxDistance, mDistance);

   mPlayerScope = cPlayerScopeAny;
   BSimString text;
   if (node.getAttribValueAsString("player", text))
   {
      int scope=gDatabase.getPlayerScope(text);
      if (scope!=-1)
         mPlayerScope=scope;
   }

   mLifeScope = cLifeScopeAny;
   if (node.getAttribValueAsString("life", text))
   {
      int scope=gDatabase.getLifeScope(text);
      if (scope!=-1)
         mLifeScope=scope;
   }

   // Success.
   return(true);
}

//=============================================================================
// BPlacementRuleDistanceAtLeastFromType::addPlacementRuleUnits
//=============================================================================
bool BPlacementRuleDistanceAtMostFromType::addPlacementRuleUnits(BDynamicSimArray<BPlacementRuleUnit>& placementRuleUnits)
{
   BPlacementRuleUnit item(mType, mDistance, mPlayerScope, mLifeScope, BPlacementRuleUnit::cBeNearUnit);
   placementRuleUnits.add(item);
   return true;
}

//=============================================================================
// BPlacementRuleDistanceAtLeastFromType::BPlacementRuleDistanceAtLeastFromType
//=============================================================================
BPlacementRuleDistanceAtLeastFromType::BPlacementRuleDistanceAtLeastFromType(void) :
mDistance(0.0f),
mType(-1),
mFoundationType(cRuleFoundationSolid),
mPlayerScope(cPlayerScopeAny),
mLifeScope(cLifeScopeAlive),
mFlagIncludeObstructionRadius(false),
mFlagSpecialType(false)
{
}


//=============================================================================
// BPlacementRuleDistanceAtLeastFromType::evaluate
//=============================================================================
bool BPlacementRuleDistanceAtLeastFromType::evaluate(const BUnit* pBuilder, const long protoID, const long playerID, const BVector &location, const BVector &forward,
                                                     const BVector &right, long losMode, DWORD flags, long *errorStringID, long *successStringID, BEntityIDArray *pCachedUnitsToTrack) const
{
  // protoID; playerID; location; forward; right; losMode; flags; errorStringID, successStringID;

   // Co-op support
   BPlayer* pPlayer = gWorld->getPlayer(playerID);
   if (!pPlayer)
      return false;
   BPlayerID coopPlayerID=(gWorld->getFlagCoop() ? pPlayer->getCoopID() : cInvalidPlayerID);

   bool success = true;

   float distance = mDistance;
   float radius = 0.0f;
   if (mFlagIncludeObstructionRadius)
   {
      const BProtoObject *pProto = pPlayer->getProtoObject(protoID);
      if (pProto)
      {
         //-- get more units than we are going to need 
         radius = pProto->getObstructionRadius();

         //-- add in a little buffer for edge cases
         distance +=  radius + 10.0f;
      }
   }
   float distanceSqr = distance * distance;

   const BEntityIDArray &unitsToTrack = pCachedUnitsToTrack ? *pCachedUnitsToTrack : BPlacementRules::getUnitsToTrack();
   long numResults = unitsToTrack.getNumber();
   for(long i=0; i<numResults; i++)
   {
      // Get unit.
      BUnit *pUnit = gWorld->getUnit(unitsToTrack[i]);
      if (!pUnit || !pUnit->getPlayer())
         continue;
      if ((mFlagSpecialType && pUnit!=pBuilder) || (!mFlagSpecialType && !pUnit->isType(mType)))
         continue;
      //if (!unit->isFullyBuilt())
      //   continue;

      if (!gDatabase.checkPlayerScope(pUnit, playerID, mPlayerScope))
      {
         if (coopPlayerID==-1)
            continue;
         if (!gDatabase.checkPlayerScope(pUnit, coopPlayerID, mPlayerScope))
            continue;
      }

      if (!gDatabase.checkLifeScope(pUnit, mLifeScope))
         continue;

      // Show allowable areas
      if (flags & cRenderFlag)
      {
         float thickness = 0.1f;
         gTerrainSimRep.addDebugThickCircleOverTerrain(pUnit->getPosition(), mDistance, thickness, cDWORDRed, 0.1f);
      }

      if (success)
      {
         // Find maximum distance from builder center to any vertex on placed building obstruction hull
         float closestDistSqr = 100000.0f;
         BVector basePos = pUnit->getPosition();
         BVector corner[4];

//-- FIXING PREFIX BUG ID 2640
         const BProtoObject* pPlacedBuildingProto = pPlayer->getProtoObject(protoID);
//--
         float obsRadiusX = (pPlacedBuildingProto) ? pPlacedBuildingProto->getObstructionRadiusX() : 0.0f;
         float obsRadiusZ = (pPlacedBuildingProto) ? pPlacedBuildingProto->getObstructionRadiusZ() : 0.0f;

         corner[0].x = location.x - obsRadiusX;
         corner[0].z = location.z - obsRadiusZ;
         corner[1].x = location.x + obsRadiusX;
         corner[1].z = location.z - obsRadiusZ;
         corner[2].x = location.x + obsRadiusX;
         corner[2].z = location.z + obsRadiusZ;
         corner[3].x = location.x - obsRadiusX;
         corner[3].z = location.z + obsRadiusZ;

         for(long cornerPoint=0; cornerPoint<4; cornerPoint++)
         {

            float thisDistSqr = basePos.distanceSqr(BVector(corner[cornerPoint].x, basePos.y, corner[cornerPoint].z));
            if(thisDistSqr<closestDistSqr)
               closestDistSqr=thisDistSqr;
         }

         if (closestDistSqr <= distanceSqr)
            success = false;
      }
   }

   return success;
}


//=============================================================================
// BPlacementRuleDistanceAtLeastFromType::loadXML
//=============================================================================
bool BPlacementRuleDistanceAtLeastFromType::loadXML(BXMLNode node)
{
   // Sanity.
   if(!node)
   {
      BFAIL("Null node.");
      return(false);
   }

   // Load parent stuff.
   BPlacementRule::loadXML(node);

   // Get type.
   BSimString tempStr;
   node.getText(tempStr);
   mType = getSpecialType(tempStr);
   if (mType != -1)
      mFlagSpecialType=true;
   else
   {
      mType = gDatabase.getObjectType(tempStr);
      if (mType < 0)
         return(false);
   }

   // Get distance.
   if (!node.getAttribValueAsFloat("distance", mDistance))
      return(false);
   BPlacementRules::sMaxDistance = Math::Max(BPlacementRules::sMaxDistance, mDistance);

   BSimString text;

   // get player scope (any, player, team, enemy)
   mPlayerScope = cPlayerScopeAny;
   if (node.getAttribValueAsString("player", text))
   {
      if (text == "any")
         mPlayerScope = cPlayerScopeAny;
      else if (text == "player")
         mPlayerScope = cPlayerScopePlayer;
      else if (text == "team")
         mPlayerScope = cPlayerScopeTeam;
      else if (text == "enemy")
         mPlayerScope = cPlayerScopeEnemy;
      else if (text == "gaia")
         mPlayerScope = cPlayerScopeGaia;
   }

   mLifeScope = cLifeScopeAny;
   if (node.getAttribValueAsString("life", text))
   {
      if (text == "any")
         mLifeScope = cLifeScopeAny;
      else if (text == "alive")
         mLifeScope = cLifeScopeAlive;
      else if (text == "dead")
         mLifeScope = cLifeScopeDead;
   }

   // get foundation (solid/fully built/any)
   mFoundationType = cRuleFoundationSolid;
   if (node.getAttribValueAsString("foundation", text))
   {
      if (text == "any")
         mFoundationType = cRuleFoundationAny;
      //else if (text == "solid")
      //   mFoundationType = cRuleFoundationSolid;
      //else if (text == "fullybuilt")
      //   mFoundationType = cRuleFoundationFullyBuilt;
   }

   bool includeObRadius = false;
   if (node.getAttribValueAsBool("includeObstructionRadius", includeObRadius))
      mFlagIncludeObstructionRadius = includeObRadius;

   // Success.
   return(true);
}

//=============================================================================
// BPlacementRuleDistanceAtLeastFromType::addPlacementRuleUnits
//=============================================================================
bool BPlacementRuleDistanceAtLeastFromType::addPlacementRuleUnits(BDynamicSimArray<BPlacementRuleUnit>& placementRuleUnits)
{
   BPlacementRuleUnit item(mType, mDistance, mPlayerScope, mLifeScope, BPlacementRuleUnit::cAvoidUnit);
   placementRuleUnits.add(item);
   return true;
}


//=============================================================================
// BPlacementRuleObstructionAtLeastFromType::BPlacementRuleObstructionAtLeastFromType
//=============================================================================
BPlacementRuleObstructionAtLeastFromType::BPlacementRuleObstructionAtLeastFromType(void) :
mDistance(0.0f),
mType(-1),
mFoundationType(cRuleFoundationSolid),
mPlayerScope(cPlayerScopeAny),
mLifeScope(cLifeScopeAlive),
mFlagSpecialType(false)
{
}


//=============================================================================
// BPlacementRuleObstructionAtLeastFromType::evaluate
//=============================================================================
bool BPlacementRuleObstructionAtLeastFromType::evaluate(const BUnit* pBuilder, const long protoID, const long playerID, const BVector &location, const BVector &forward,
                                                        const BVector &right, long losMode, DWORD flags, long *errorStringID, long *successStringID, BEntityIDArray *pCachedUnitsToTrack) const
{
   //protoID; playerID; location; forward; right; losMode; flags; errorStringID, successStringID;

//-- FIXING PREFIX BUG ID 2643
   const BPlayer* pPlayer = gWorld->getPlayer(playerID);
//--
   if (!pPlayer)
      return false;
   const BProtoObject *pProto = pPlayer->getProtoObject(protoID);
   if (!pProto)
      return(false);

   // Co-op support
   BPlayerID coopPlayerID=(gWorld->getFlagCoop() ? gWorld->getPlayer(playerID)->getCoopID() : cInvalidPlayerID);

   // Find objects.
   BDynamicSimLongArray unitList;

   BVector loc = location;
   BEntityIDArray results(0, 64);
   BUnitQuery query(loc, mDistance, true);
   query.addObjectTypeFilter(mType);
   long numResults = gWorld->getUnitsInArea(&query, &results);
   for(long i=0; i<numResults; i++)
   {
      // Get unit.
      BUnit *pUnit = gWorld->getUnit(results[i]);
      if(!pUnit)
         continue;

      // if we match the type, then match the rest
      if((mFlagSpecialType && pUnit==pBuilder) || (!mFlagSpecialType && pUnit->isType(mType)))
      {
         /*
         // we don't want to test walls at this point.
         if (unit->isAbstractType(cAbstractUnitTypeAbstractWall))
         continue;
         */
         if (!gDatabase.checkPlayerScope(pUnit, playerID, mPlayerScope))
         {
            if (coopPlayerID==-1)
               continue;
            if (!gDatabase.checkPlayerScope(pUnit, coopPlayerID, mPlayerScope))
               continue;
         }

         if (!gDatabase.checkLifeScope(pUnit, mLifeScope))
            continue;

         // we had the right player scope, now see if we have a match on foundation

         // foundation
         switch(mFoundationType)
         {
         case cRuleFoundationAny:
            // make sure we are not looking at ourselves
            if (location == pUnit->getPosition())
               continue;
            break;
         case cRuleFoundationSolid:
            //if (!unit->isSolid())
            //   continue;
            break;
         case cRuleFoundationFullyBuilt:
            //if (!unit->isFullyBuilt())
            //   break;
            break;
         }


         // TODO: make this work.
         //const BOPQuadHull *pCompareUnitObstructionHull = pUnit->getObstructionHull();
         //if (!pCompareUnitObstructionHull)
         //   continue;
         //bool doesOverlap = pCompareUnitObstructionHull->overlapsHull(placingUnitObstructionHull);
         //if (!doesOverlap)
         //   continue;
         //return(false);
      }
   }

   // No matches found, so placement is ok.
   return(true);
}


//=============================================================================
// BPlacementRuleObstructionAtLeastFromType::loadXML
//=============================================================================
bool BPlacementRuleObstructionAtLeastFromType::loadXML(BXMLNode node)
{
   // Sanity.
   if(!node)
   {
      BFAIL("Null node.");
      return(false);
   }

   // Load parent stuff.
   BPlacementRule::loadXML(node);

   // Get type.
   BSimString tempStr;
   node.getText(tempStr);
   mType = getSpecialType(tempStr);
   if (mType != -1)
      mFlagSpecialType=true;
   else
   {
      mType = gDatabase.getObjectType(tempStr);
      if (mType < 0)
         return(false);
   }

   // Get distance.
   if (!node.getAttribValueAsFloat("distance", mDistance))
      return(false);

   BSimString text;
   // get player scope (any, player, team, enemy)
   mPlayerScope = cPlayerScopeAny;
   if (node.getAttribValueAsString("player", text))
   {
      if (text == "any")
         mPlayerScope = cPlayerScopeAny;
      else if (text == "player")
         mPlayerScope = cPlayerScopePlayer;
      else if (text == "team")
         mPlayerScope = cPlayerScopeTeam;
      else if (text == "enemy")
         mPlayerScope = cPlayerScopeEnemy;
      else if (text == "gaia")
         mPlayerScope = cPlayerScopeGaia;
   }

   mLifeScope = cLifeScopeAny;
   if (node.getAttribValueAsString("life", text))
   {
      if (text == "any")
         mLifeScope = cLifeScopeAny;
      else if (text == "alive")
         mLifeScope = cLifeScopeAlive;
      else if (text == "dead")
         mLifeScope = cLifeScopeDead;
   }

   // get foundation (solid/fully built/any)
   mFoundationType = cRuleFoundationSolid;
   if (node.getAttribValueAsString("foundation", text))
   {
      if (text == "any")
         mFoundationType = cRuleFoundationAny;
      else if (text == "solid")
         mFoundationType = cRuleFoundationSolid;
      else if (text == "fullybuilt")
         mFoundationType = cRuleFoundationFullyBuilt;
   }

   // Success.
   return(true);
}

//=============================================================================
// BPlacementRuleObstructionAtLeastFromType::addPlacementRuleUnits
//=============================================================================
bool BPlacementRuleObstructionAtLeastFromType::addPlacementRuleUnits(BDynamicSimArray<BPlacementRuleUnit>& placementRuleUnits)
{
   // Don't add any because we don't want the red circles.
   placementRuleUnits;
   return(true);
}


/*
//=============================================================================
// BPlacementRuleDistanceAtMostFromWater::BPlacementRuleDistanceAtMostFromWater
//=============================================================================
BPlacementRuleDistanceAtMostFromWater::BPlacementRuleDistanceAtMostFromWater(void) :
mDistance(0.0f)
{
}


//=============================================================================
// BPlacementRuleDistanceAtMostFromWater::evaluate
//=============================================================================
bool BPlacementRuleDistanceAtMostFromWater::evaluate(const BUnit* pBuilder, const long protoID, const long playerID, const BVector &location, const BVector &forward,
                                                     const BVector &right, long losMode, DWORD flags, long *errorStringID, long *successStringID, BEntityIDArray *pCachedUnitsToTrack) const
{
   //protoID; playerID; location; forward; right; losMode; flags; errorStringID, successStringID;

   BPlayer *player = game->getPlayer(playerID);

   // Need terrain
   if(!game || !gWorld || !player)
      return(false);

   // Find my tile
   BTerrainBase * terrain = gWorld->getTerrain();
   if (terrain == NULL)
      BASSERT(0);

   static BVector tile;
   tile = location;
   terrain->convertWorldToTile(&tile);

   bool inEditor = game->getIsEditorMode();

   // Determine my "radius" of tiles that I need to get 
   float tileCount = mDistance*terrain->getRecipTileSize();

   // This is a simple square check - might want to make it more circular.
   long xStart=(long)(tile.x-tileCount);
   long zStart=(long)(tile.z-tileCount);
   long end = (long)tileCount*2;
   BYTE vis=0;
   for (long i=0, x=xStart; i<end; i++, x++)
   {
      for (long j=0, z=zStart; j<end; j++, z++)
      {
         // check for LOS/FOG
         vis = player->getVisibility(x,z);
         if (!inEditor)
         {
            // only check if not in editor.
            if ( (vis != BVisibleMap::cVisible) &&
               (vis != BVisibleMap::cFogged ) )
               continue;
         }

         // we want water close by.  
         if (terrain->getWaterType(x, z) != 255 )  // getWaterType does range checking.
         {
            if (successStringID)
               *successStringID = mSuccessStringID;
            return(true);
         }
      }
   }
   if (errorStringID)
      *errorStringID = mErrorStringID;

   return false;
}


//=============================================================================
// BPlacementRuleDistanceAtMostFromWater::loadXML
//=============================================================================
bool BPlacementRuleDistanceAtMostFromWater::loadXML(BXMLNode node)
{
   // Sanity.
   if(!node)
   {
      BFAIL("Null node.");
      return(false);
   }

   // Load parent stuff.
   BPlacementRule::loadXML(node);

   // Get distance.
   bool ok = node.getAttribValueAsFloat("distance", mDistance);
   if(!ok)
   {
      node.logInfo("No distance specified.");
      return(false);
   }


   // Success.
   return(true);
}*/
/*
//=============================================================================
// BPlacementRuleDistanceAtMostFromMapEdge::BPlacementRuleDistanceAtMostFromMapEdge
//=============================================================================
BPlacementRuleDistanceAtMostFromMapEdge::BPlacementRuleDistanceAtMostFromMapEdge(void) :
mDistance(0.0f)
{
}


//=============================================================================
// BPlacementRuleDistanceAtMostFromMapEdge::evaluate
//=============================================================================
bool BPlacementRuleDistanceAtMostFromMapEdge::evaluate(const BUnit* pBuilder, const long protoID, const long playerID, const BVector &location, const BVector &forward,
                                                       const BVector &right, long losMode, DWORD flags, long *errorStringID, long *successStringID, BEntityIDArray *pCachedUnitsToTrack) const
{
   //protoID; playerID; location; forward; right; losMode; flags; errorStringID, successStringID;

   BPlayer *player = game->getPlayer(playerID);

   // Need terrain
   if(!game || !gWorld || !player)
      return(false);

   // Find my tile
   BTerrainBase * terrain = gWorld->getTerrain();
   if (terrain == NULL)
      BASSERT(0);

   // if within the circle mDistance from edge, we are too far from edge.
   if (!terrain->isLocWithinMainCircle(location, mDistance))
   {
      if (successStringID)
         *successStringID = mSuccessStringID;
      return(true);
   }
   if (errorStringID)
      *errorStringID = mErrorStringID;
   return false;
}


//=============================================================================
// BPlacementRuleDistanceAtMostFromMapEdge::loadXML
//=============================================================================
bool BPlacementRuleDistanceAtMostFromMapEdge::loadXML(BXMLNode node)
{
   // Sanity.
   if(!node)
   {
      BFAIL("Null node.");
      return(false);
   }

   // Load parent stuff.
   BPlacementRule::loadXML(node);

   // Get distance.
   bool ok = node.getAttribValueAsFloat("distance", mDistance);
   if(!ok)
   {
      node.logInfo("No distance specified.");
      return(false);
   }


   // Success.
   return(true);
}



//=============================================================================
// BPlacementRuleMapType::BPlacementRuleMapType
//=============================================================================
BPlacementRuleMapType::BPlacementRuleMapType(void) : 
mMapType(-1)
{
}


//=============================================================================
// BPlacementRuleMapType::evaluate
//=============================================================================
bool BPlacementRuleMapType::evaluate(const BUnit* pBuilder, const long protoID, const long playerID, const BVector &location, const BVector &forward,
                                     const BVector &right, long losMode, DWORD flags, long *errorStringID, long *successStringID, BEntityIDArray *pCachedUnitsToTrack) const
{
   //protoID; playerID; location; forward; right; losMode; flags; errorStringID, successStringID;

   // Need terrain
   if(!game || !gWorld)
      return(false);

   bool inEditor = game->getIsEditorMode();
   if (inEditor)
      return true;

   if(gWorld->isMapType(mMapType))
   {
      if (successStringID)
         *successStringID = mSuccessStringID;
      return(true);
   }
   if (errorStringID)
      *errorStringID = mErrorStringID;
   return false;
}


//=============================================================================
// BPlacementRuleMapType::loadXML
//=============================================================================
bool BPlacementRuleMapType::loadXML(BXMLNode node)
{
   // Sanity.
   if(!node)
   {
      BFAIL("Null node.");
      return(false);
   }

   // Load parent stuff.
   BPlacementRule::loadXML(node);

   // load the map type.
   mMapType = game->getDatabase()->getMapTypeID(node.getText().asANSI());
   if(mMapType<0)
   {
      node.logInfo("'%s' is not a valid map type", BStrConv::toA(node.getText()));
      return false;
   }

   // Success.
   return(true);
}


//=============================================================================
// BPlacementRuleInsidePerimeterWall::BPlacementRuleInsidePerimeterWall
//=============================================================================
BPlacementRuleInsidePerimeterWall::BPlacementRuleInsidePerimeterWall(void) :
mDistance(0.0f),
mType(-1)
{
}


//=============================================================================
// BPlacementRuleInsidePerimeterWall::evaluate
//=============================================================================
bool BPlacementRuleInsidePerimeterWall::evaluate(const BUnit* pBuilder, const long protoID, const long playerID, const BVector &location, const BVector &forward,
                                                 const BVector &right, long losMode, DWORD flags, long *errorStringID, long *successStringID, BEntityIDArray *pCachedUnitsToTrack) const
{
   //protoID; playerID; location; forward; right; losMode; flags; errorStringID, successStringID;

   // Check for inside perimeter wall
   BPlayer* player=gWorld->getPlayer(playerID);
   if(player)
   {
      BPerimeterWallManager* mgr=player->getPerimeterWallManager();
      if(mgr)
      {
         if(mgr->isPointInside(location))
            return true;
      }
   }

   // Find objects.
   BVector loc = location;
   BUnitQuery query(loc, mDistance, true);
   gWorld->getUnitsInArea(&query);
   //BDynamicSimLongArray unitList;
   //gWorld->getUnitsInArea(location, mDistance, true, unitList);
   for(long i=0; i<unitList.getNumber(); i++)
   {
      // Get unit.
      BUnit *unit = gWorld->getUnit(unitList[i]);
      if(!unit)
         continue;

      //-- check player
      if (unit->getPlayer() != player)
         continue;

      // Skip if type does not match.
      if(!unit->isType(mType))
         continue;

      // make sure the building it is next to is fully built.
      if (!unit->isFullyBuilt())
         continue;

      // A match, so we're good.
      // TODO: better distance check?
      return(true);
   }

   return(false);
}


//=============================================================================
// BPlacementRuleInsidePerimeterWall::loadXML
//=============================================================================
bool BPlacementRuleInsidePerimeterWall::loadXML(BXMLNode node)
{
   // Sanity.
   if(!node)
   {
      BFAIL("Null node.");
      return(false);
   }

   // Load parent stuff.
   BPlacementRule::loadXML(node);

   // Get type.
   mType = game->getDatabase()->getUnitTypeID(node.getText().asANSI());
   if(mType<0)
   {
      node.logInfo("Invalid type.");
      return(false);
   }

   // Get distance.
   bool ok = node.getAttribValueAsFloat("distance", mDistance);
   if(!ok)
   {
      node.logInfo("No distance specified.");
      return(false);
   }

   // Success.
   return(true);
}

*/
//=============================================================================
// BPlacementRules::BPlacementRules
//=============================================================================
BPlacementRules::BPlacementRules(void)
{
}


//=============================================================================
//=============================================================================
BPlacementRules::~BPlacementRules(void)
{
}


//=============================================================================
// BPlacementRules::evaluate
//=============================================================================
bool BPlacementRules::evaluate(const BUnit* pBuilder, const long protoID, const long playerID, const BVector &location, const BVector &forward,
                               const BVector &right, long losMode, DWORD flags, long *errorStringID, long *successStringID, BEntityIDArray *pCachedUnitsToTrack) const
{
   // Evaluate the tree.
   bool ok = mRoot.evaluate(pBuilder, protoID, playerID, location, forward, right, losMode, flags, errorStringID, successStringID, pCachedUnitsToTrack);
   return(ok);
}

//=============================================================================
// BPlacementRules::getPlacementRuleUnits
//=============================================================================
const BDynamicSimArray<BPlacementRuleUnit> &BPlacementRules::getPlacementRuleUnits() const
{
   return mPlacementRuleUnits;
}

//=============================================================================
// BPlacementRules::loadXML
//=============================================================================
bool BPlacementRules::loadXML(const BSimString &filename)
{
   // Save filename.
   mFilename = filename;

   // Read the XML file.
   BXMLReader reader;
   bool ok = reader.load(cDirPlacementRules, filename, XML_READER_LOAD_DISCARD_ON_CLOSE);
   const BXMLNode root(reader.getRootNode());
   if(!ok || !root)
      return(false);

   // Sanity check.
   if(root.getName().compare(B("PlacementRules")))
   {
      root.logInfo("Expected tag 'PlacementRules' but found '%s' instead.", BStrConv::toA(root.getName()));
      return(false);
   }

   // Load it.
   ok = mRoot.loadXML(root);

   mRoot.addPlacementRuleUnits(mPlacementRuleUnits);

   // Hand back result.
   return(ok);
}


//=============================================================================
// BPlacementRules::createFromXML
//=============================================================================
BPlacementRule *BPlacementRules::createFromXML(BXMLNode node)
{
   // Sanity check.
   if(!node)
   {
      BFAIL("BPlacementRules::createFromXML -- null node.");
      return(NULL);
   }

   // Allocate node based on type.
   BPlacementRule *rule = NULL;
   if(node.getName().compare(B("and")) == 0)
      rule = new BPlacementRuleAnd;
   else if(node.getName().compare(B("or")) == 0)
      rule = new BPlacementRuleOr;
   else if(node.getName().compare(B("DistanceAtMostFromType")) == 0)
      rule = new BPlacementRuleDistanceAtMostFromType;
   else if(node.getName().compare(B("DistanceAtLeastFromType")) == 0)
      rule = new BPlacementRuleDistanceAtLeastFromType;
   else if(node.getName().compare(B("ObstructionAtLeastFromType")) == 0)
      rule = new BPlacementRuleObstructionAtLeastFromType;
   //else if(node.getName().compare(B("DistanceAtMostFromWater")) == 0)
   //   rule = new BPlacementRuleDistanceAtMostFromWater;
   //else if(node.getName().compare(B("DistanceAtMostFromMapEdge")) == 0)
   //   rule = new BPlacementRuleDistanceAtMostFromMapEdge;
   //else if(node.getName().compare(B("MapType")) == 0)
   //   rule = new BPlacementRuleMapType;
   //else if(node.getName().compare(B("InsidePerimeterWall")) == 0)
   //   rule = new BPlacementRuleInsidePerimeterWall;
   //else if(node.getName().compare(B("DistanceAtLeastFromCliff")) == 0)
   //   rule = new BPlacementRuleDistanceAtLeastFromCliff;

   // Oops bad type.
   if(!rule)
   {
      node.logInfo("'%s' is not a valid rule type.", BStrConv::toA(node.getName()));
      return(NULL);
   }

   // Hand it back.
   return(rule);
}

//=============================================================================
// BPlacementRules::getUnitsToTrack
//=============================================================================
BEntityIDArray &BPlacementRules::getUnitsToTrack()
{
   return sUnitsToTrack;
}

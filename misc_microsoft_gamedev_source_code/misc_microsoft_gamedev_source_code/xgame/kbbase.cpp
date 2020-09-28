//==============================================================================
// kbbase.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "ai.h"
#include "aidebug.h"
#include "configsgame.h"
#include "game.h"
#include "kb.h"
#include "kbbase.h"
#include "kbsquad.h"
#include "protoobject.h"
#include "protosquad.h"
#include "team.h"
#include "world.h"

// xgamerender
#include "debugprimitives.h"
#include "fontsystem2.h"
#include "render.h"

// xrender
#include "renderdraw.h"

GFIMPLEMENTVERSION(BKBBase, 2);
GFIMPLEMENTVERSION(BKBBaseQuery, 1);

//==============================================================================
//==============================================================================
BKBBase::BKBBase()
{
   mID = 0;
   resetNonIDData();
}


//==============================================================================
//==============================================================================
BKBBase::~BKBBase()
{

}


//==============================================================================
//==============================================================================
#ifndef BUILD_FINAL
void BKBBase::debugRender() const
{
   if (gGame.getAIDebugType() != AIDebugType::cKBBase)
      return;

   const float cKBBaseDebugThickness = 0.0f;
   const float cKBBaseDebugTerrainOffset = 1.0f;
   if (gWorld->isSphereVisible(mPosition, mRadius))
   {
      if (cKBBaseDebugThickness > 0.0f)
         gTerrainSimRep.addDebugThickCircleOverTerrain(mPosition, mRadius, cKBBaseDebugThickness, gWorld->getPlayerColor(mPlayerID, BWorld::cPlayerColorContextSelection), cKBBaseDebugTerrainOffset, BDebugPrimitives::cCategoryAI);
      else
         gTerrainSimRep.addDebugCircleOverTerrain(mPosition, mRadius, gWorld->getPlayerColor(mPlayerID, BWorld::cPlayerColorContextSelection), cKBBaseDebugTerrainOffset, BDebugPrimitives::cCategoryAI);

      // Draw the summary over the mission group
      DWORD playerColor = gWorld->getPlayerColor(mPlayerID, BWorld::cPlayerColorContextSelection);
      BHandle hFont = gFontManager.getFontCourier10();
      gFontManager.setFont(hFont);
      BFixedString128 debugText;
      float sx = 0.0f;
      float sy = 0.0f;
      gRender.getViewParams().calculateWorldToScreen(mPosition, sx, sy);

      float lineHeight = gFontManager.getLineHeight();
      debugText.format("KBBase DebugID = %d", mDebugID);
      gFontManager.drawText(hFont, sx, sy, debugText, playerColor);
      sy += lineHeight;

      debugText.format("Owner = Player %d", mPlayerID);
      gFontManager.drawText(hFont, sx, sy, debugText, playerColor);
      sy += lineHeight;

      debugText.format("Assert Value = %f", mAssetValue);
      gFontManager.drawText(hFont, sx, sy, debugText, playerColor);
      sy += lineHeight;

      // Add KBBase stuff here.
   }
}
#endif


//==============================================================================
//==============================================================================
void BKBBase::resetNonIDData()
{
   setPosition(cInvalidVector);
   setRadius(0.0f);
   setMass(0.0f);
   setTeamID(cInvalidTeamID);
   setPlayerID(cInvalidPlayerID);
   mKBSquadIDs.clear();
   mLastSeenTime = 0;
   mAssetValue = 0.0f;
   mAIMissionTargetIDs.resize(0);
   mbIsPositionAnchored = false;
   #ifndef BUILD_FINAL
      mDebugID = 0;
   #endif
}


//==============================================================================
//==============================================================================
void BKBBase::setPosition(const BVector pos)
{
   // Set the position on the base.
   mPosition = pos;

   // Refresh the position on all mission targets that refer to this base.
   uint numMissionTargets = mAIMissionTargetIDs.getSize();
   for (uint i=0; i<numMissionTargets; i++)
   {
      BAIMissionTarget* pTarget = gWorld->getAIMissionTarget(mAIMissionTargetIDs[i]);
      if (pTarget)
         pTarget->setPosition(pos);
   }
}


//==============================================================================
//==============================================================================
void BKBBase::setRadius(float r)
{
   // Don't do this.
   BASSERTM(r >= 0.0f, "BKBBase::setRadius() - Getting a negative radius passed in.");

   // Set the radius on the base.
   mRadius = r;

   // Refresh the position on all mission targets that refer to this base.
   uint numMissionTargets = mAIMissionTargetIDs.getSize();
   for (uint i=0; i<numMissionTargets; i++)
   {
      BAIMissionTarget* pTarget = gWorld->getAIMissionTarget(mAIMissionTargetIDs[i]);
      if (pTarget)
         pTarget->setRadius(r);
   }
}


//==============================================================================
//==============================================================================
void BKBBase::addKBSquad(BKBSquadID kbSquadID)
{
   BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquadID);
   BASSERT(!containsKBSquad(kbSquadID));
   BASSERT(pKBSquad);
   if (!pKBSquad)
      return;

   BASSERT(!gWorld->getKBBase(pKBSquad->getKBBaseID()));
   BASSERT(pKBSquad->getPlayerID() == mPlayerID);

   mKBSquadIDs.add(kbSquadID);
   pKBSquad->setKBBaseID(mID);

   if (!mbIsPositionAnchored && pKBSquad->containsBuilding())
   {
      const BSquad* pSquad = pKBSquad->getSquad();
      if (pSquad)
      {
         const BUnit* pSettlement = gWorld->getUnit(pSquad->getAssociatedSettlement());
         if (pSettlement)
         {
            setPosition(pSettlement->getPosition());
            setIsPositionAnchored(true);
         }
      }
   }

   BPlayer* pPlayer = gWorld->getPlayer(pKBSquad->getPlayerID());
   BASSERT(pPlayer);
   if (pPlayer)
   {
      bool containsBuildings = false;
      BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(pKBSquad->getProtoSquadID());
      BASSERT(pProtoSquad);
      if (pProtoSquad)
      {
         long numUnitNodes = pProtoSquad->getNumberUnitNodes();
         for (long i=0; i<numUnitNodes; i++)
         {
            const BProtoSquadUnitNode& node = pProtoSquad->getUnitNode(i);
            BProtoObject* pProtoObject = pPlayer->getProtoObject(node.mUnitType);
            if (!pProtoObject)
               continue;
            if (pProtoObject->isType(gDatabase.getOTIDBuilding()))
               containsBuildings = true;
            mMass += pProtoObject->getMass() * node.mUnitCount;
         }
      }
   }


   // Add to our asset value.
   mAssetValue += pKBSquad->getAssetValue();

   update();
}


//==============================================================================
//==============================================================================
void BKBBase::removeKBSquad(BKBSquadID kbSquadID)
{
   BASSERT(containsKBSquad(kbSquadID));
   mKBSquadIDs.remove(kbSquadID);

   BKBSquad* pKBSquad = gWorld->getKBSquad(kbSquadID);
   BASSERT(pKBSquad);
   if (!pKBSquad)
      return;

   pKBSquad->setKBBaseID(cInvalidKBBaseID);

   BPlayer* pPlayer = gWorld->getPlayer(pKBSquad->getPlayerID());
   BASSERT(pPlayer);
   if (pPlayer)
   {
      BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(pKBSquad->getProtoSquadID());
      if (pProtoSquad)
      {
         long numUnitNodes = pProtoSquad->getNumberUnitNodes();
         for (long i=0; i<numUnitNodes; i++)
         {
            const BProtoSquadUnitNode& node = pProtoSquad->getUnitNode(i);
            BProtoObject* pProtoObject = pPlayer->getProtoObject(node.mUnitType);
            if (!pProtoObject)
               continue;
            mMass -= pProtoObject->getMass() * node.mUnitCount;
         }
      }
   }

   // Remove from our asset value.
   mAssetValue -= pKBSquad->getAssetValue();

   update();
}


//==============================================================================
//==============================================================================
void BKBBase::removeAllKBSquads()
{
   BKBSquadIDArray kbSquadsToRemove = mKBSquadIDs;
   uint numKBSquads = kbSquadsToRemove.getSize();

   for (uint i=0; i<numKBSquads; i++)
      removeKBSquad(kbSquadsToRemove[i]);

   mKBSquadIDs.resize(0);
   mAssetValue = 0.0f;
   mMass = 0.0f;
}


//==============================================================================
//==============================================================================
void BKBBase::update()
{
   BVector centroid(0.0f);

   mMass = 0.0f;
   mAssetValue = 0.0f;

   uint numKBSquadIDs = mKBSquadIDs.getSize();
   for (uint i=0; i<numKBSquadIDs; i++)
   {
      BKBSquad* pKBSquad = gWorld->getKBSquad(mKBSquadIDs[i]);
      if (!pKBSquad)
         continue;

      BPlayer* pPlayer = gWorld->getPlayer(pKBSquad->getPlayerID());
      if (!pPlayer)
         continue;

//-- FIXING PREFIX BUG ID 4756
      const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(pKBSquad->getProtoSquadID());
//--
      if (!pProtoSquad)
         continue;

      float mass = 0.0f;
      long numUnitNodes = pProtoSquad->getNumberUnitNodes();
      for (long u=0; u<numUnitNodes; u++)
      {
         const BProtoSquadUnitNode& node = pProtoSquad->getUnitNode(u);
//-- FIXING PREFIX BUG ID 4755
         const BProtoObject* pProtoObject = pPlayer->getProtoObject(node.mUnitType);
//--
         if (!pProtoObject)
            continue;
         mass += pProtoObject->getMass() * node.mUnitCount;
      }
      
      centroid += pKBSquad->getPosition() * mass;
      mAssetValue += pKBSquad->getAssetValue();
      mMass += mass;
   }

   if (!mbIsPositionAnchored && mMass > 0.0f)
   {
      centroid /= mMass;
      setPosition(centroid);
   }

   float maxDistSqr = cDefaultBKBBaseRadiusSqr;
   for (uint i=0; i<numKBSquadIDs; i++)
   {
      BKBSquad* pKBSquad = gWorld->getKBSquad(mKBSquadIDs[i]);
      if (!pKBSquad)
         continue;
      float distSqr = mPosition.xzDistanceSqr(pKBSquad->getPosition());
      maxDistSqr = static_cast<float>(Math::fSelectMax(distSqr, maxDistSqr));
   }
   
   setRadius(Math::fSqrt(maxDistSqr));
}


//==============================================================================
//==============================================================================
uint BKBBase::getVisibleSquadCount() const
{
   uint visibleSquadCount = 0;

   uint numKBSquadIDs = mKBSquadIDs.getSize();
   for (uint i=0; i<numKBSquadIDs; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(mKBSquadIDs[i]);
      if (pKBSquad && pKBSquad->getCurrentlyVisible())
         visibleSquadCount++;
   }

   return (visibleSquadCount);
}


//==============================================================================
//==============================================================================
uint BKBBase::getBuildingCount() const
{
   uint buildingCount = 0;

   uint numKBSquadIDs = mKBSquadIDs.getSize();
   for (uint i=0; i<numKBSquadIDs; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(mKBSquadIDs[i]);
      if (pKBSquad && pKBSquad->containsBuilding())
         buildingCount++;
   }

   return (buildingCount);
}


//==============================================================================
// Note: does actual distance check (no distance squared, or XZ distance for now)
//==============================================================================
bool BKBBase::containsPosition(BVector testPosition) const
{
   return ((mRadius * mRadius) >= mPosition.xzDistanceSqr(testPosition));
}


//==============================================================================
//==============================================================================
bool BKBBase::canMergeIntoBase(BKBBase& rKBBase) const
{
   if (mID == rKBBase.mID)
      return (false);
   if (mPlayerID != rKBBase.mPlayerID)
      return (false);
   if (rKBBase.getKBSquadIDs().getSize() == 0)
      return (false);
   if (!rKBBase.containsPosition(mPosition))
      return (false);
   if (rKBBase.getBuildingCount() < getBuildingCount())
      return (false);

   uint numKBSquads = mKBSquadIDs.getSize();
   if (numKBSquads == 0)
      return (false);

   if (rKBBase.mKBSquadIDs.getSize() < numKBSquads)
      return (false);

   for (uint i=0; i<numKBSquads; i++)
   {
      const BKBSquad* pKBSquad = gWorld->getKBSquad(mKBSquadIDs[i]);
      if (!pKBSquad || !rKBBase.containsPosition(pKBSquad->getPosition()))
         return (false);
   }
  
   return (true);
}


//==============================================================================
//==============================================================================
void BKBBase::mergeIntoBase(BKBBase& rKBBase)
{
   BASSERT(mID != rKBBase.mID);
   BASSERT(mPlayerID == rKBBase.mPlayerID);
   BASSERT(rKBBase.getKBSquadIDs().getSize() > 0);
   BASSERT(rKBBase.containsPosition(mPosition));
   BASSERT(mKBSquadIDs.getSize() > 0);

   BKBSquadIDArray kbSquadIDs = mKBSquadIDs;
   uint numKBSquads = kbSquadIDs.getSize();
   for (uint i=0; i<numKBSquads; i++)
   {
      removeKBSquad(kbSquadIDs[i]);
      rKBBase.addKBSquad(kbSquadIDs[i]);
   }
}


//==============================================================================
//==============================================================================
DWORD BKBBase::getStaleness() const
{
   if (getVisibleSquadCount() > 0)
   {
      return (0);
   }
   else
   {
      DWORD currentGametime = gWorld->getGametime();
      DWORD lastSeenTime = getLastSeenTime();
      BASSERT(currentGametime >= lastSeenTime);
      DWORD staleness = currentGametime - lastSeenTime;
      return (staleness);
   }
}

//==============================================================================
//==============================================================================
bool BKBBase::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mPosition);
   GFWRITEARRAY(pStream, BAIMissionTargetID, mAIMissionTargetIDs, uint16, 500);
   GFWRITEARRAY(pStream, BKBSquadID, mKBSquadIDs, uint16, 500);
   GFWRITEVAR(pStream, float, mRadius);
   GFWRITEVAR(pStream, float, mMass);
   GFWRITEVAR(pStream, BTeamID, mTeamID);
   GFWRITEVAR(pStream, BPlayerID, mPlayerID);
   GFWRITEVAR(pStream, BKBBaseID, mID);
   GFWRITEVAR(pStream, DWORD, mLastSeenTime);
   GFWRITEVAR(pStream, float, mAssetValue);
   GFWRITEBITBOOL(pStream, mbIsPositionAnchored);

   #ifndef BUILD_FINAL  
      GFWRITEVAR(pStream, uint, mDebugID);
    #else
      GFWRITEVAL(pStream, uint, 0);
   #endif

   return true;
}

//==============================================================================
//==============================================================================
bool BKBBase::load(BStream* pStream, int saveType)
{
   GFREADVECTOR(pStream, mPosition);
   GFREADARRAY(pStream, BAIMissionTargetID, mAIMissionTargetIDs, uint16, 500);
   GFREADARRAY(pStream, BKBSquadID, mKBSquadIDs, uint16, 500);
   GFREADVAR(pStream, float, mRadius);
   GFREADVAR(pStream, float, mMass);
   GFREADVAR(pStream, BTeamID, mTeamID);
   GFREADVAR(pStream, BPlayerID, mPlayerID);
   GFREADVAR(pStream, BKBBaseID, mID);
   GFREADVAR(pStream, DWORD, mLastSeenTime);
   GFREADVAR(pStream, float, mAssetValue);
   GFREADBITBOOL(pStream, mbIsPositionAnchored);

   if (BKBBase::mGameFileVersion >= 2)
   {
      #ifndef BUILD_FINAL  
         GFREADVAR(pStream, uint, mDebugID);
      #else
         GFREADTEMPVAL(pStream, uint);
      #endif
   }

   return true;
}

//==============================================================================
//==============================================================================
BKBBaseQuery::BKBBaseQuery()
{
   resetQuery();
}


//==============================================================================
//==============================================================================
void BKBBaseQuery::resetQuery()
{
   // Simply clear the flags that tell us to use the parameter data (and we'll ignore it)
   mFlagPointRadius = false;
   mFlagPlayerRelation = false;
   mFlagMinStaleness = false;
   mFlagMaxStaleness = false;
   mFlagRequireBuildings = false;
   mbSelfAsAlly = false;
}

//==============================================================================
//==============================================================================
bool BKBBaseQuery::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mPosition);
   GFWRITEVAR(pStream, float, mRadius);
   GFWRITEVAR(pStream, BPlayerID, mPlayerID);
   GFWRITEVAR(pStream, DWORD, mMinStaleness);
   GFWRITEVAR(pStream, DWORD, mMaxStaleness);
   GFWRITEVAR(pStream, BRelationType, mPlayerRelation);
   GFWRITEBITBOOL(pStream, mFlagPointRadius);
   GFWRITEBITBOOL(pStream, mFlagPlayerRelation);
   GFWRITEBITBOOL(pStream, mFlagMinStaleness);
   GFWRITEBITBOOL(pStream, mFlagMaxStaleness);
   GFWRITEBITBOOL(pStream, mFlagRequireBuildings);
   GFWRITEBITBOOL(pStream, mbSelfAsAlly);
   return true;
}

//==============================================================================
//==============================================================================
bool BKBBaseQuery::load(BStream* pStream, int saveType)
{
   GFREADVECTOR(pStream, mPosition);
   GFREADVAR(pStream, float, mRadius);
   GFREADVAR(pStream, BPlayerID, mPlayerID);
   GFREADVAR(pStream, DWORD, mMinStaleness);
   GFREADVAR(pStream, DWORD, mMaxStaleness);
   GFREADVAR(pStream, BRelationType, mPlayerRelation);
   GFREADBITBOOL(pStream, mFlagPointRadius);
   GFREADBITBOOL(pStream, mFlagPlayerRelation);
   GFREADBITBOOL(pStream, mFlagMinStaleness);
   GFREADBITBOOL(pStream, mFlagMaxStaleness);
   GFREADBITBOOL(pStream, mFlagRequireBuildings);
   GFREADBITBOOL(pStream, mbSelfAsAlly);
   return true;
}

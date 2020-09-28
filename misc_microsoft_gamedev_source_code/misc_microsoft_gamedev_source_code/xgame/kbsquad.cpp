
// kbsquad.cpp
//
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

// xgame
#include "common.h"
#include "ai.h"
#include "aidebug.h"
#include "configsgame.h"
#include "game.h"
#include "kbsquad.h"
#include "protoobject.h"
#include "protosquad.h"
#include "savegame.h"
#include "simhelper.h"
#include "squad.h"
#include "world.h"

GFIMPLEMENTVERSION(BKBSquad, 2);
GFIMPLEMENTVERSION(BKBSquadQuery, 1);

//==============================================================================
//==============================================================================
BKBSquad::BKBSquad()
{
   mID = 0;
   resetNonIDData();
}


//==============================================================================
//==============================================================================
BKBSquad::~BKBSquad()
{
}


//==============================================================================
//==============================================================================
void BKBSquad::resetNonIDData()
{
   setSquadID(cInvalidObjectID);
   setTeamID(cInvalidTeamID);
   setPlayerID(cInvalidPlayerID);
   setProtoSquadID(cInvalidProtoSquadID);
   setKBBaseID(cInvalidKBBaseID);
   setHitpoints(0.0f);
   setShieldpoints(0.0f);
   setPercentHP(0.0f);
   setPercentSP(0.0f);
   setLastSeenTime(gWorld->getGametime());
   setCurrentlyVisible(false);
   setLastKnownPosValid(false);
   #ifndef BUILD_FINAL
      mDebugID = 0;
   #endif
}


//==============================================================================
//==============================================================================
BVector BKBSquad::getPosition() const
{
   if (mbCurrentlyVisible)
   {
      const BSquad* pSquad = getSquad();
      if (pSquad)
         return (pSquad->getPosition());
   }
   return (mPosition);
}


//==============================================================================
//==============================================================================
#ifndef BUILD_FINAL
BVector BKBSquad::getForward() const
{
   if (mbCurrentlyVisible)
   {
      const BSquad* pSquad = getSquad();
      if (pSquad)
         return (pSquad->getForward());
   }
   return (mForward);
}
#endif


//==============================================================================
//==============================================================================
#ifndef BUILD_FINAL
BVector BKBSquad::getUp() const
{
   if (mbCurrentlyVisible)
   {
      const BSquad* pSquad = getSquad();
      if (pSquad)
         return (pSquad->getUp());
   }
   return (mUp);
}
#endif


//==============================================================================
//==============================================================================
#ifndef BUILD_FINAL
BVector BKBSquad::getRight() const
{
   if (mbCurrentlyVisible)
   {
      const BSquad* pSquad = getSquad();
      if (pSquad)
         return (pSquad->getRight());
   }
   return (mRight);
}
#endif


//==============================================================================
//==============================================================================
bool BKBSquad::getValidKnownPosition(BVector& pos) const
{
   // Most accurate.
   if (mbCurrentlyVisible)
   {
      const BSquad* pSquad = getSquad();
      if (pSquad)
      {
         pos = pSquad->getPosition();
         return (true);
      }
   }
   
   // Stale / less accurate
   if (mbLastKnownPosValid)
   {
      pos = mPosition;
      return (true);
   }

   // Failsauce.
   return (false);
}


//==============================================================================
//==============================================================================
float BKBSquad::getHitpoints() const
{
   if (mbCurrentlyVisible)
   {
      const BSquad* pSquad = getSquad();
      if (pSquad)
         return (pSquad->getHPMax() * pSquad->getHPPercentage());
   }
   return (mHitpoints);
}


//==============================================================================
//==============================================================================
float BKBSquad::getHPPercentage() const
{
   float hpMax = getHPMax();
   if (hpMax > 0.0f)
      return (getHitpoints() / hpMax);
   else
      return (0.0f);
}


//==============================================================================
//==============================================================================
float BKBSquad::getShieldpoints() const
{
   if (mbCurrentlyVisible)
   {
      const BSquad* pSquad = getSquad();
      if (pSquad)
         return (pSquad->getSPMax() * pSquad->getSPPercentage());
   }
   return (mShieldpoints);
}


//==============================================================================
//==============================================================================
float BKBSquad::getSPPercentage() const
{
   float spMax = getSPMax();
   if (spMax > 0.0f)
      return (getShieldpoints() / spMax);
   else
      return (0.0f);
}


//==============================================================================
//==============================================================================
DWORD BKBSquad::getLastSeenTime() const
{
   if (mbCurrentlyVisible)
      return (gWorld->getGametime());
   else
      return (mLastSeenTime);
}


//==============================================================================
//==============================================================================
float BKBSquad::getAssetValue() const
{
   float totalAssetValue = 0.0f;
   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (totalAssetValue);

   const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(getProtoSquadID());
   if (!pProtoSquad)
      return (totalAssetValue);

   long numUnitNodes = pProtoSquad->getNumberUnitNodes();
   for (long i=0; i<numUnitNodes; i++)
   {
      const BProtoSquadUnitNode& node = pProtoSquad->getUnitNode(i);
      const BProtoObject* pProtoObject = pPlayer->getProtoObject(node.mUnitType);
      BASSERT(pProtoObject);
      //if (!pProtoObject)
      //   continue;
      // The asset value is based on hitpoints but we allow for an adjustment to correct this.
      float assetValue = pProtoObject->getHitpoints() * node.mUnitCount;
      float assetAdjust = pProtoObject->getAIAssetValueAdjust() * node.mUnitCount;
      // We also hack this for buildings.  :)
      if (pProtoObject->isType(gDatabase.getOTIDBuilding()))
         assetValue *= 0.1f;

      totalAssetValue += (assetValue + assetAdjust);
   }

   return (totalAssetValue);
}


//==============================================================================
//==============================================================================
bool BKBSquad::containsBuilding() const
{
   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (false);
   const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(getProtoSquadID());
   if (!pProtoSquad)
      return (false);
   long numUnitNodes = pProtoSquad->getNumberUnitNodes();
   for (long i=0; i<numUnitNodes; i++)
   {
      const BProtoSquadUnitNode& node = pProtoSquad->getUnitNode(i);
      const BProtoObject* pProtoObject = pPlayer->getProtoObject(node.mUnitType);
      if (pProtoObject && pProtoObject->isType(gDatabase.getOTIDBuilding()))
         return (true);
   }

   return (false);
}


//==============================================================================
//==============================================================================
bool BKBSquad::containsGatherable() const
{
   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (false);
   const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(getProtoSquadID());
   if (!pProtoSquad)
      return (false);
   long numUnitNodes = pProtoSquad->getNumberUnitNodes();
   for (long i=0; i<numUnitNodes; i++)
   {
      const BProtoSquadUnitNode& node = pProtoSquad->getUnitNode(i);
      const BProtoObject* pProtoObject = pPlayer->getProtoObject(node.mUnitType);
      if (pProtoObject && pProtoObject->isType(gDatabase.getOTIDGatherable()))
         return (true);
   }

   return (false);
}


//==============================================================================
//==============================================================================
bool BKBSquad::containsBase() const
{
   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (false);
   const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(getProtoSquadID());
   if (!pProtoSquad)
      return (false);
   long numUnitNodes = pProtoSquad->getNumberUnitNodes();
   for (long i=0; i<numUnitNodes; i++)
   {
      const BProtoSquadUnitNode& node = pProtoSquad->getUnitNode(i);
      const BProtoObject* pProtoObject = pPlayer->getProtoObject(node.mUnitType);
      if (pProtoObject && pProtoObject->isType(gDatabase.getOTIDBase()))
         return (true);
   }

   return (false);
}


//==============================================================================
//==============================================================================
bool BKBSquad::containsSettlement() const
{
   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (false);
   const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(getProtoSquadID());
   if (!pProtoSquad)
      return (false);
   long numUnitNodes = pProtoSquad->getNumberUnitNodes();
   for (long i=0; i<numUnitNodes; i++)
   {
      const BProtoSquadUnitNode& node = pProtoSquad->getUnitNode(i);
      const BProtoObject* pProtoObject = pPlayer->getProtoObject(node.mUnitType);
      if (pProtoObject && pProtoObject->isType(gDatabase.getOTIDSettlement()))
         return (true);
   }

   return (false);
}


//==============================================================================
//==============================================================================
uint BKBSquad::getAttackGrade(BDamageTypeID damageType) const
{
   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (0);
   const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(getProtoSquadID());
   if (!pProtoSquad)
      return (0);
   return pProtoSquad->getAttackGrade(damageType);
}


//==============================================================================
//==============================================================================
BSquad* BKBSquad::getSquad()
{
   return (gWorld->getSquad(mSquadID));
}


//==============================================================================
//==============================================================================
const BSquad* BKBSquad::getSquad() const
{
   return (gWorld->getSquad(mSquadID));
}


//==============================================================================
//==============================================================================
float BKBSquad::getAttackRatingDPS(BDamageTypeID damageType) const
{
   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (0.0f);
   const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(getProtoSquadID());
   if (!pProtoSquad)
      return (0.0f);
   return pProtoSquad->getAttackRatingDPS(damageType);
}


//==============================================================================
//==============================================================================
float BKBSquad::getAttackGradeRatio(BDamageTypeID damageType) const
{
   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (0.0f);
   const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(getProtoSquadID());
   if (!pProtoSquad)
      return (0.0f);
   return pProtoSquad->getAttackGradeRatio(damageType);
}


//==============================================================================
//==============================================================================
BDamageTypeID BKBSquad::getDamageType() const
{
   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (cInvalidDamageTypeID);
   const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(getProtoSquadID());
   if (!pProtoSquad)
      return (cInvalidDamageTypeID);
   return (pProtoSquad->getDamageType());
}


//==============================================================================
//==============================================================================
bool BKBSquad::isDamageType(BDamageTypeID damageType) const
{
   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (false);
   const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(getProtoSquadID());
   if (!pProtoSquad)
      return (false);
   return (pProtoSquad->isDamageType(damageType));
}


//==============================================================================
//==============================================================================
float BKBSquad::getCombatValue() const
{
   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (0.0f);
   const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(getProtoSquadID());
   if (!pProtoSquad)
      return (0.0f);
   return (pProtoSquad->getCombatValue());
}


//==============================================================================
//==============================================================================
float BKBSquad::getHPMax() const
{
   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (0.0f);
   const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(getProtoSquadID());
   if (!pProtoSquad)
      return (0.0f);
   return (pProtoSquad->getMaxHP());
}


//==============================================================================
//==============================================================================
float BKBSquad::getSPMax() const
{
   const BPlayer* pPlayer = gWorld->getPlayer(mPlayerID);
   if (!pPlayer)
      return (0.0f);
   const BProtoSquad* pProtoSquad = pPlayer->getProtoSquad(getProtoSquadID());
   if (!pProtoSquad)
      return (0.0f);
   return (pProtoSquad->getMaxSP());
}


//==============================================================================
//==============================================================================
float BKBSquad::getCombatValueHP() const
{
   float actualHPSP = getHitpoints() + getShieldpoints();
   float maxHPSP = getHPMax() + getSPMax();
   float percentTotal = 0.0f;
   if (maxHPSP > 0.0f)
      percentTotal = actualHPSP / maxHPSP;

   return (getCombatValue() * percentTotal);
}


//==============================================================================
// How long has it been since we have observed this unit?
//==============================================================================
DWORD BKBSquad::getStaleness() const
{
   if (mbCurrentlyVisible)
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
#ifndef BUILD_FINAL
void BKBSquad::debugRender() const
{
   if (gGame.getAIDebugType() != AIDebugType::cKBSquad)
      return;

   if (mbCurrentlyVisible || mbLastKnownPosValid)
   {
      static const float cKBSquadDebugBoxScale = 3.0f;
      static const float cKBSquadDebugBoxTerrainOffset = 3.0f;

      BMatrix matrix;
      DWORD playerColor = gWorld->getPlayerColor(mPlayerID, BWorld::cPlayerColorContextObjects);
      BSimHelper::calculateDebugRenderMatrix(getPosition(), getForward(), getUp(), getRight(), cKBSquadDebugBoxTerrainOffset, matrix);
      gpDebugPrimitives->addDebugBox(matrix, cKBSquadDebugBoxScale, playerColor, BDebugPrimitives::cCategoryAI);
   }
}
#endif


//==============================================================================
// void BKBSquad::update()
// Update the KB's view of the squad (the BKBSquad).
// Parameters should have already been validated by this point.
//==============================================================================
void BKBSquad::update(const BKB *pKB, BSquad *pSquad)
{
   //SCOPEDSAMPLE(BKBSquad_update);
   BASSERT(pKB);
   BASSERT(pSquad);

   setSquadID(pSquad->getID());
   setProtoSquadID(pSquad->getProtoSquadID());
   setPlayerID(pSquad->getPlayerID());
   setPosition(pSquad->getPosition());

   #ifndef BUILD_FINAL
   setForward(pSquad->getForward());
   setUp(pSquad->getUp());
   setRight(pSquad->getRight());
   #endif

   setHitpoints(pSquad->getCurrentHP());
   setShieldpoints(pSquad->getCurrentSP());
   setPercentHP(pSquad->getHPPercentage());
   setPercentSP(pSquad->getSPPercentage());
   setLastSeenTime(gWorld->getGametime());
   setLastKnownPosValid(true);
}

//==============================================================================
// BKBSquad::save
//==============================================================================
bool BKBSquad::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mPosition);

#ifndef BUILD_FINAL
   GFWRITEVECTOR(pStream, mForward);
   GFWRITEVECTOR(pStream, mUp);
   GFWRITEVECTOR(pStream, mRight);
#else
   GFWRITEVECTOR(pStream, cInvalidVector);
   GFWRITEVECTOR(pStream, cInvalidVector);
   GFWRITEVECTOR(pStream, cInvalidVector);
#endif

   GFWRITEVAR(pStream, BKBSquadID, mID);
   GFWRITEVAR(pStream, BEntityID, mSquadID);
   GFWRITEVAR(pStream, BTeamID, mTeamID);
   GFWRITEVAR(pStream, BPlayerID, mPlayerID);
   GFWRITEVAR(pStream, BProtoSquadID, mProtoSquadID);
   GFWRITEVAR(pStream, BKBBaseID, mKBBaseID);
   GFWRITEVAR(pStream, float, mHitpoints);
   GFWRITEVAR(pStream, float, mShieldpoints);
   GFWRITEVAR(pStream, float, mPercentHP);
   GFWRITEVAR(pStream, float, mPercentSP);
   GFWRITEVAR(pStream, DWORD, mLastSeenTime);

   GFWRITEBITBOOL(pStream, mbCurrentlyVisible);
   GFWRITEBITBOOL(pStream, mbLastKnownPosValid);

   #ifndef BUILD_FINAL  
      GFWRITEVAR(pStream, uint, mDebugID);
   #else
      GFWRITEVAL(pStream, uint, 0);
   #endif

   return true;
}

//==============================================================================
// BKBSquad::load
//==============================================================================
bool BKBSquad::load(BStream* pStream, int saveType)
{  
   GFREADVECTOR(pStream, mPosition);

#ifndef BUILD_FINAL
   GFREADVECTOR(pStream, mForward);
   GFREADVECTOR(pStream, mUp);
   GFREADVECTOR(pStream, mRight);
#else
   BVector tempVec;
   GFREADVECTOR(pStream, tempVec);
   GFREADVECTOR(pStream, tempVec);
   GFREADVECTOR(pStream, tempVec);
#endif

   GFREADVAR(pStream, BKBSquadID, mID);
   GFREADVAR(pStream, BEntityID, mSquadID);
   GFREADVAR(pStream, BTeamID, mTeamID);
   GFREADVAR(pStream, BPlayerID, mPlayerID);
   GFREADVAR(pStream, BProtoSquadID, mProtoSquadID);
   GFREADVAR(pStream, BKBBaseID, mKBBaseID);
   GFREADVAR(pStream, float, mHitpoints);
   GFREADVAR(pStream, float, mShieldpoints);
   GFREADVAR(pStream, float, mPercentHP);
   GFREADVAR(pStream, float, mPercentSP);
   GFREADVAR(pStream, DWORD, mLastSeenTime);

   GFREADBITBOOL(pStream, mbCurrentlyVisible);
   GFREADBITBOOL(pStream, mbLastKnownPosValid);

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
BKBSquadQuery::BKBSquadQuery()
{
   resetQuery();
}


//==============================================================================
//==============================================================================
void BKBSquadQuery::resetQuery()
{
   // Simply clear the flags that tell us to use the parameter data (and we'll ignore it)
   mFlagPointRadius        = false;
   mFlagPlayerRelation     = false;
   mFlagObjectType         = false;
   mFlagBase               = false;
   mFlagMinStaleness       = false;
   mFlagMaxStaleness       = false;
   mFlagCurrentlyVisible   = false;
   mFlagIncludeBuildings   = false;
   mbSelfAsAlly            = false;
}

//==============================================================================
//==============================================================================
bool BKBSquadQuery::save(BStream* pStream, int saveType) const
{
   GFWRITEVECTOR(pStream, mPosition);
   GFWRITEVAR(pStream, float, mRadius);
   GFWRITEVAR(pStream, BPlayerID, mPlayerID);
   GFWRITEVAR(pStream, BRelationType, mPlayerRelation);
   GFWRITEVAR(pStream, BObjectTypeID, mObjectTypeID);
   GFWRITEVAR(pStream, BKBBaseID, mBaseID);
   GFWRITEVAR(pStream, DWORD, mMinStaleness);
   GFWRITEVAR(pStream, DWORD, mMaxStaleness);
   GFWRITEBITBOOL(pStream, mbCurrentlyVisible);
   GFWRITEBITBOOL(pStream, mbIncludeBuildings);
   GFWRITEBITBOOL(pStream, mbSelfAsAlly);
   GFWRITEBITBOOL(pStream, mFlagPointRadius);
   GFWRITEBITBOOL(pStream, mFlagPlayerRelation);
   GFWRITEBITBOOL(pStream, mFlagObjectType);
   GFWRITEBITBOOL(pStream, mFlagBase);
   GFWRITEBITBOOL(pStream, mFlagMinStaleness);
   GFWRITEBITBOOL(pStream, mFlagMaxStaleness);
   GFWRITEBITBOOL(pStream, mFlagCurrentlyVisible);
   GFWRITEBITBOOL(pStream, mFlagIncludeBuildings);
   return true;
}

//==============================================================================
//==============================================================================
bool BKBSquadQuery::load(BStream* pStream, int saveType)
{
   GFREADVECTOR(pStream, mPosition);
   GFREADVAR(pStream, float, mRadius);
   GFREADVAR(pStream, BPlayerID, mPlayerID);
   GFREADVAR(pStream, BRelationType, mPlayerRelation);
   GFREADVAR(pStream, BObjectTypeID, mObjectTypeID);
   GFREADVAR(pStream, BKBBaseID, mBaseID);
   GFREADVAR(pStream, DWORD, mMinStaleness);
   GFREADVAR(pStream, DWORD, mMaxStaleness);
   GFREADBITBOOL(pStream, mbCurrentlyVisible);
   GFREADBITBOOL(pStream, mbIncludeBuildings);
   GFREADBITBOOL(pStream, mbSelfAsAlly);
   GFREADBITBOOL(pStream, mFlagPointRadius);
   GFREADBITBOOL(pStream, mFlagPlayerRelation);
   GFREADBITBOOL(pStream, mFlagObjectType);
   GFREADBITBOOL(pStream, mFlagBase);
   GFREADBITBOOL(pStream, mFlagMinStaleness);
   GFREADBITBOOL(pStream, mFlagMaxStaleness);
   GFREADBITBOOL(pStream, mFlagCurrentlyVisible);
   GFREADBITBOOL(pStream, mFlagIncludeBuildings);
   gSaveGame.remapObjectType(mObjectTypeID);
   return true;
}

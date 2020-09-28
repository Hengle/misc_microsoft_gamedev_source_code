//==============================================================================
// team.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "entity.h"
#include "kb.h"
#include "syncmacros.h"
#include "team.h"
#include "world.h"

GFIMPLEMENTVERSION(BTeam, 2);

//==============================================================================
// BTeam::BTeam
//==============================================================================
BTeam::BTeam() :
   mID(-1),
   mHasSpies(false)
{
}

//==============================================================================
// BTeam::~BTeam
//==============================================================================
BTeam::~BTeam()
{
   reset();
}

//==============================================================================
// BTeam::setup
//==============================================================================
bool BTeam::setup(long id)
{
   mID=id;
   //AJL FIXME - The max number of objects is hard-coded below
   mVisibleUnits.setNumber(8000);
   mVisibleProjectiles.setNumber(2000);
   mVisibleObjects.setNumber(16000);
   return true;
}


//==============================================================================
// BTeam::reset
//==============================================================================
void BTeam::reset()
{
   mPlayers.clear();
   mVisibleUnits.setNumber(0);
   mVisibleProjectiles.setNumber(0);
   mVisibleObjects.setNumber(0);
   mHasSpies = false;
}


//==============================================================================
// BTeam::update
//==============================================================================
void BTeam::update(float elapsedTime)
{
   SCOPEDSAMPLE(BTeam_update);
   elapsedTime;

#ifdef SYNC_Team
   syncTeamData("BTeam::update mID", mID);
#endif
}


//=============================================================================
// BTeam::addVisibleObject
//=============================================================================
void BTeam::addVisibleObject(BEntityID id)
{
   long type=id.getType();

#ifdef SYNC_Visibility
   if(type==BEntity::cClassTypeUnit || type==BEntity::cClassTypeProjectile)
   {
      syncVisibilityData("BTeam::addVisibleObject team id", mID);
      syncVisibilityData("BTeam::addVisibleObject entity id", id.asLong());
   }
#endif

   if(id==cInvalidObjectID)
   {
      BASSERT(0);
      return;
   }

   BBitArray* pBitArray;
   if(type==BEntity::cClassTypeUnit)
      pBitArray=&mVisibleUnits;
   else if(type==BEntity::cClassTypeProjectile)
      pBitArray=&mVisibleProjectiles;
   else if(type==BEntity::cClassTypeObject)
      pBitArray=&mVisibleObjects;
   else
   {
      BASSERT(0);
      return;
   }

   long index=id.getIndex();
   long num=pBitArray->getNumber();
   if(index>=num)
      pBitArray->setNumber(index+1);
   pBitArray->setBit(index);
}

//=============================================================================
// BTeam::removeVisibleObject
//=============================================================================
void BTeam::removeVisibleObject(BEntityID id)
{
   long type=id.getType();

#ifdef SYNC_Visibility
   if(type==BEntity::cClassTypeUnit || type==BEntity::cClassTypeProjectile)
   {
      syncVisibilityData("BTeam::removeVisibleObject team id", mID);
      syncVisibilityData("BTeam::removeVisibleObject entity id", id.asLong());
   }
#endif

   if(id==cInvalidObjectID)
   {
      BASSERT(0);
      return;
   }


   BBitArray* pBitArray;
   if(type==BEntity::cClassTypeUnit)
      pBitArray=&mVisibleUnits;
   else if(type==BEntity::cClassTypeProjectile)
      pBitArray=&mVisibleProjectiles;
   else if(type==BEntity::cClassTypeObject)
      pBitArray=&mVisibleObjects;
   else
   {
      BASSERT(0);
      return;
   }

   long index=id.getIndex();
   long num=pBitArray->getNumber();
   if(index>=num)
   {
      BASSERT(0);
      return;
   }
   // SLB: This asserts all the time and it's unnecessary
   if(!pBitArray->isBitSet(index))
   {
      BASSERT(0);
      return;
   }
   pBitArray->clearBit(index);
}

//=============================================================================
// BTeam::onRelease
//=============================================================================
void BTeam::onRelease(BEntityID id)
{
   if (id == cInvalidObjectID)
   {
      BASSERT(0);
      return;
   }

   long type = id.getType();
   BBitArray* pBitArray;
   if (type == BEntity::cClassTypeUnit)
      pBitArray = &mVisibleUnits;
   else if (type == BEntity::cClassTypeProjectile)
      pBitArray = &mVisibleProjectiles;
   else if (type == BEntity::cClassTypeObject)
      pBitArray = &mVisibleObjects;
   else
   {
      BASSERT(0);
      return;
   }

   long index = id.getIndex();
   long num = pBitArray->getNumber();
   if (index >= num)
   {
      BASSERT(0);
      return;
   }

   pBitArray->clearBit(index);
}

//=============================================================================
// BTeam::isObjectVisible
//=============================================================================
bool BTeam::isObjectVisible(BEntityID id) const
{
   if(id==cInvalidObjectID)
      return false;

   long type=id.getType();

   const BBitArray* pBitArray;
   if(type==BEntity::cClassTypeUnit)
      pBitArray=&mVisibleUnits;
   else if(type==BEntity::cClassTypeProjectile)
      pBitArray=&mVisibleProjectiles;
   else if(type==BEntity::cClassTypeObject)
      pBitArray=&mVisibleObjects;
   else
      return false;

   long index=id.getIndex();
   long num=pBitArray->getNumber();
   if(index>=num)
      return false;

   // SLB: Make sure this ID is actually valid. We don't want false positives because of a recycled index.
   if (!gWorld->validateEntityID(id))
      return false;

   return (pBitArray->isBitSet(index)!=0);
}


//=============================================================================
// Static functions
//=============================================================================

//=============================================================================
// BTeam::doesTeamHaveSpies
//=============================================================================
bool BTeam::doesTeamHaveSpies(BTeamID id)
{
   BTeam *team = gWorld->getTeam(id);
   if( team )
      return (team->hasSpies());

   return false;
}

//=============================================================================
// BTeam::setSpiesForTeam
//=============================================================================
void BTeam::setSpiesForTeam(BTeamID id, bool val)
{
   BTeam *team = gWorld->getTeam(id);
   if( team )
      team->setSpies(val);
}

//=============================================================================
// BTeam::canTeamASeeTeamB
//=============================================================================
bool BTeam::canTeamASeeTeamB( BTeamID teamIDA, BTeamID teamIDB )
{
   if( teamIDA == teamIDB )
      return true;

   if( (gWorld->getTeamRelationType(teamIDA, teamIDB) == cRelationTypeEnemy) && doesTeamHaveSpies(teamIDA) )
      return true;

   return false;
}

//=============================================================================
//=============================================================================
bool BTeam::save(BStream* pStream, int saveType) const
{
   GFWRITEARRAY(pStream, long, mPlayers, uint8, 16);
   GFWRITEVAR(pStream, long, mID);
   GFWRITEBITARRAY(pStream, mVisibleUnits, uint16, 16000);
   GFWRITEBITARRAY(pStream, mVisibleProjectiles, uint16, 4000); 
   GFWRITEBITARRAY(pStream, mVisibleObjects, uint16, 32000);
   GFWRITEVAR(pStream, bool, mHasSpies);
   return true;
}

//=============================================================================
//=============================================================================
bool BTeam::load(BStream* pStream, int saveType)
{
   GFREADARRAY(pStream, long, mPlayers, uint8, 16);
   GFREADVAR(pStream, long, mID);
   GFREADBITARRAY(pStream, mVisibleUnits, uint16, 16000);
   GFREADBITARRAY(pStream, mVisibleProjectiles, uint16, 4000); 
   if (mGameFileVersion >= 2)
      GFREADBITARRAY(pStream, mVisibleObjects, uint16, 32000);
   GFREADVAR(pStream, bool, mHasSpies);
   return true;
}

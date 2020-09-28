//==============================================================================
// lspCache.cpp
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================

#include "common.h"
#include "configsgame.h"

#include "lspCache.h"

//==============================================================================
// 
//==============================================================================
PBLSPServiceRecordUser BLSPServiceRecordCache::getUser(XUID xuid) const
{
   BServiceRecordUsers::iterator it(mUsers.find(xuid));
   if (it == mUsers.end())
      return NULL;
   return &(it->second);
}

//==============================================================================
// 
//==============================================================================
void BLSPServiceRecordCache::addUser(XUID xuid, uint level, uint ttl)
{
   ttl = Math::Max<uint>(ttl, cDefaultServiceRecordTTL);

   PBLSPServiceRecordUser pUser = getUser(xuid);
   if (pUser == NULL)
   {
      mUsers.insert(xuid, BLSPServiceRecordUser(xuid, level, ttl));
   }
   else
   {
      pUser->mLevel = level;
      pUser->mTTL = ttl;
      pUser->mLastUpdate = timeGetTime();
   }
}

//==============================================================================
// 
//==============================================================================
uint BLSPServiceRecordCache::getLevel(XUID xuid) const
{
   PBLSPServiceRecordUser pUser = getUser(xuid);
   if (pUser == NULL)
      return 0;

   return pUser->mLevel;
}

//==============================================================================
// 
//==============================================================================
const BLSPAuthUser& BLSPAuthCache::getMachine() const
{
   return mMachine;
}

//==============================================================================
// 
//==============================================================================
void BLSPAuthCache::updateMachine(XUID xuid, bool banMedia, bool banMatchMaking, bool banEverything, uint ttl)
{
   mMachine.mXuid = xuid;
   mMachine.mBanMedia = banMedia;
   mMachine.mBanMatchMaking = banMatchMaking;
   mMachine.mBanEverything = banEverything;
   mMachine.mTTL = ttl;
   mMachine.mLastUpdate = timeGetTime();
}

//==============================================================================
// 
//==============================================================================
void BLSPAuthCache::updateMachine()
{
   mMachine.mLastUpdate = timeGetTime();
}

//==============================================================================
// 
//==============================================================================
PBLSPAuthUser BLSPAuthCache::getUser(XUID xuid) const
{
   BAuthUser::iterator it(mUsers.find(xuid));
   if (it == mUsers.end())
      return NULL;
   return &(it->second);
}

//==============================================================================
// 
//==============================================================================
void BLSPAuthCache::addUser(XUID xuid, bool banMedia, bool banMatchMaking, bool banEverything, uint ttl)
{
   PBLSPAuthUser pUser = getUser(xuid);
   if (pUser == NULL)
   {
      mUsers.insert(xuid, BLSPAuthUser(xuid, banMedia, banMatchMaking, banEverything, ttl));
   }
   else
   {
      pUser->mBanMedia = banMedia;
      pUser->mBanMatchMaking = banMatchMaking;
      pUser->mBanEverything = banEverything;
      pUser->mTTL = ttl;
      pUser->mLastUpdate = timeGetTime();
   }
}

//==============================================================================
// 
//==============================================================================
bool BLSPAuthCache::isBanMedia(XUID xuid)
{
//-- FIXING PREFIX BUG ID 2851
   const PBLSPAuthUser pUser = getUser(xuid);
//--
   if (pUser == NULL)
      return true;
   return pUser->mBanMedia;
}

//==============================================================================
// 
//==============================================================================
bool BLSPAuthCache::isBanMatchMaking(XUID xuid)
{
//-- FIXING PREFIX BUG ID 2852
   const PBLSPAuthUser pUser = getUser(xuid);
//--
   if (pUser == NULL)
      return true;
   return pUser->mBanMatchMaking;
}

//==============================================================================
// 
//==============================================================================
bool BLSPAuthCache::isBanEverything(XUID xuid)
{
//-- FIXING PREFIX BUG ID 2853
   const PBLSPAuthUser pUser = getUser(xuid);
//--
   if (pUser == NULL)
      return true;
   return pUser->mBanEverything;
}

//==============================================================================
// 
//==============================================================================
void BLSPConfigCache::init()
{
   long ttl;
   if (gConfig.get(cConfigLSPDefaultConfigTTL, &ttl))
   {
      mStatic.mTTL = static_cast<uint>(ttl);
      mDynamic.mTTL = static_cast<uint>(ttl);
      mNoData.mTTL = static_cast<uint>(ttl);
   }
}

//==============================================================================
// 
//==============================================================================
void BLSPConfigCache::updateStatic(int16 version, uint ttl)
{
   mStatic.mVersion = version;
   mStatic.mTTL = ttl;
   mStatic.mLastUpdate = timeGetTime();
}

//==============================================================================
// 
//==============================================================================
void BLSPConfigCache::updateDynamic(int16 version, uint ttl)
{
   mDynamic.mVersion = version;
   mDynamic.mTTL = ttl;
   mDynamic.mLastUpdate = timeGetTime();
}

//==============================================================================
// 
//==============================================================================
void BLSPConfigCache::updateNoData(uint ttl)
{
   mNoData.mTTL = ttl;
   mNoData.mLastUpdate = timeGetTime();
}

//==============================================================================
// 
//==============================================================================
void BLSPConfigCache::updateCache()
{
   mStatic.mLastUpdate = timeGetTime();
   mDynamic.mLastUpdate = timeGetTime();
   mNoData.mLastUpdate = timeGetTime();
}

//==============================================================================
// 
//==============================================================================
const BLSPConfigCache::BLSPConfigVersion& BLSPConfigCache::getStatic() const
{
   return mStatic;
}

//==============================================================================
// 
//==============================================================================
const BLSPConfigCache::BLSPConfigVersion& BLSPConfigCache::getDynamic() const
{
   return mDynamic;
}

//==============================================================================
// 
//==============================================================================
const BLSPConfigCache::BLSPConfigVersion& BLSPConfigCache::getNoData() const
{
   return mNoData;
}

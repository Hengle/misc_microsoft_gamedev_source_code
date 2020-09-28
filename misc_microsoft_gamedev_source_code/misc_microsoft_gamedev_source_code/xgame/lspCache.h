//==============================================================================
// lspCache.h
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================
#pragma once

// Includes
#include "common.h"

// xcore
#include "containers/skiplist.h"

enum
{
   cDefaultAuthTTL = 60*60*1000,
   cDefaultConfigTTL = 60*60*1000,
   cDefaultMediaTTL = 60*60*1000,
   cDefaultServiceRecordTTL = 15*60*1000,
};

//==============================================================================
typedef struct _BLSPServiceRecordUser {
   XUID mXuid;
   uint mTTL;
   uint mLastUpdate;
   uint mLevel;

   _BLSPServiceRecordUser() :
      mXuid(INVALID_XUID),
      mTTL(cDefaultAuthTTL), // default to 4 hours
      mLastUpdate(timeGetTime()),
      mLevel(0)
   {}

   _BLSPServiceRecordUser(XUID xuid, uint level, uint ttl) :
      mXuid(xuid),
      mTTL(ttl),
      mLastUpdate(timeGetTime()),
      mLevel(level)
   {}
} BLSPServiceRecordUser, *PBLSPServiceRecordUser;

//==============================================================================
class BLSPServiceRecordCache
{
   public:

      PBLSPServiceRecordUser getUser(XUID xuid) const;
      void                   addUser(XUID xuid, uint level, uint ttl);

      uint                   getLevel(XUID xuid) const;

   private:
      typedef BSkipList<XUID, BLSPServiceRecordUser, 4> BServiceRecordUsers;

      BServiceRecordUsers mUsers;
};

//==============================================================================
typedef struct _BLSPAuthUser {
   XUID mXuid;
   uint mTTL;
   uint mLastUpdate;
   bool mBanMedia : 1;
   bool mBanMatchMaking : 1;
   bool mBanEverything : 1;

   _BLSPAuthUser() :
      mXuid(INVALID_XUID),
      mTTL(cDefaultAuthTTL), // default to 4 hours
      mLastUpdate(timeGetTime()),
      mBanMedia(true),
      mBanMatchMaking(true),
      mBanEverything(true)
      {}

   _BLSPAuthUser(XUID xuid, bool banMedia, bool banMatchMaking, bool banEverything, uint ttl) :
      mXuid(xuid),
      mTTL(ttl),
      mLastUpdate(timeGetTime()),
      mBanMedia(banMedia),
      mBanMatchMaking(banMatchMaking),
      mBanEverything(banEverything)
      {}
} BLSPAuthUser, *PBLSPAuthUser;

//==============================================================================
class BLSPAuthCache
{
   public:

      const BLSPAuthUser& getMachine() const;
      void                updateMachine(XUID xuid, bool banMedia, bool banMatchMaking, bool banEverything, uint ttl);
      void                updateMachine(); // updates the last update time

      PBLSPAuthUser getUser(XUID xuid) const;
      void          addUser(XUID xuid, bool banMedia, bool banMatchMaking, bool banEverything, uint ttl);

      bool          isBanMedia(XUID xuid);
      bool          isBanMatchMaking(XUID xuid);
      bool          isBanEverything(XUID xuid);

   private:
      typedef BSkipList<XUID, BLSPAuthUser, 4> BAuthUser;

      BAuthUser    mUsers;
      BLSPAuthUser mMachine;
};

//==============================================================================
class BLSPConfigCache
{
   public:
      struct BLSPConfigVersion
      {
         uint   mTTL;
         uint   mLastUpdate;
         int16  mVersion;

         BLSPConfigVersion() :
            mTTL(cDefaultConfigTTL),
            mLastUpdate(0),
            mVersion(-1)
            {}
      };

      void init();

      void updateStatic(int16 version, uint ttl);
      void updateDynamic(int16 version, uint ttl);
      void updateNoData(uint ttl);

      void updateCache();

      const BLSPConfigVersion& getStatic() const;
      const BLSPConfigVersion& getDynamic() const;
      const BLSPConfigVersion& getNoData() const;

   private:

      BLSPConfigVersion mStatic;
      BLSPConfigVersion mDynamic;
      BLSPConfigVersion mNoData;
};

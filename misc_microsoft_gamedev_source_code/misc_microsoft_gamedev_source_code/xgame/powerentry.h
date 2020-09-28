//==============================================================================
// powerentry.h
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================
#pragma once

#include "gamefilemacros.h"
#include "simtypes.h"

//==============================================================================
// BPowerEntryItem
//==============================================================================
class BPowerEntryItem
{
   public:
      BPowerEntryItem();

      bool                    operator==( const BPowerEntryItem& pe ) const;

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      BEntityID               mSquadID;
      int                     mUsesRemaining;
      int                     mTimesUsed;
      int                     mChargeCap;
      DWORD                   mNextGrantTime;
      bool                    mInfiniteUses;
      bool                    mRecharging;
};

//==============================================================================
// BPowerEntry
// Data class which contains info about the usage of a power for a player.
//==============================================================================
class BPowerEntry
{
   public:
      BPowerEntry();

      bool                    operator==( const BPowerEntry& pe ) const;

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      BSmallDynamicSimArray<BPowerEntryItem> mItems;
      BProtoPowerID           mProtoPowerID;
      long                    mTimesUsed;      
      int                     mIconLocation;
      bool                    mFlagIgnoreCost:1;
      bool                    mFlagIgnoreTechPrereqs:1;
      bool                    mFlagIgnorePop:1;      
};
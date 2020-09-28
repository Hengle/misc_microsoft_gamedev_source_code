//==============================================================================
// powerentry.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
// xgame
#include "common.h"
#include "powerentry.h" 
#include "gamefile.h"

GFIMPLEMENTVERSION(BPowerEntry, 1);
GFIMPLEMENTVERSION(BPowerEntryItem, 1);

//==============================================================================
// BPowerEntryItem::BPowerEntryItem
//==============================================================================
BPowerEntryItem::BPowerEntryItem() :
   mSquadID(-1),
   mUsesRemaining(0),
   mTimesUsed(0),
   mChargeCap(0),
   mNextGrantTime(0),
   mInfiniteUses(false),
   mRecharging(false)
{
}

//==============================================================================
// BPowerEntryItem::operator==
//==============================================================================
bool BPowerEntryItem::operator==( const BPowerEntryItem& pe ) const
{
   return (mSquadID == pe.mSquadID);
}

//==============================================================================
//==============================================================================
bool BPowerEntryItem::save(BStream* pStream, int saveType) const
{
   GFWRITEVAR(pStream, BEntityID, mSquadID);
   GFWRITEVAR(pStream, int, mUsesRemaining);
   GFWRITEVAR(pStream, int, mTimesUsed);
   GFWRITEVAR(pStream, int, mChargeCap);
   GFWRITEVAR(pStream, DWORD, mNextGrantTime);
   GFWRITEVAR(pStream, bool, mInfiniteUses);
   GFWRITEVAR(pStream, bool, mRecharging);
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerEntryItem::load(BStream* pStream, int saveType)
{
   GFREADVAR(pStream, BEntityID, mSquadID);
   GFREADVAR(pStream, int, mUsesRemaining);
   GFREADVAR(pStream, int, mTimesUsed);
   GFREADVAR(pStream, int, mChargeCap);
   GFREADVAR(pStream, DWORD, mNextGrantTime);
   GFREADVAR(pStream, bool, mInfiniteUses);
   GFREADVAR(pStream, bool, mRecharging);
   return true;
}

//==============================================================================
// BPowerEntry::BPowerEntry
//==============================================================================
BPowerEntry::BPowerEntry() :
   mProtoPowerID(cInvalidProtoPowerID), 
   mTimesUsed(0),
   mIconLocation(-1),
   mFlagIgnoreCost(false),
   mFlagIgnoreTechPrereqs(false),
   mFlagIgnorePop(false)      
{
}

//==============================================================================
// BPowerEntry::operator==
//==============================================================================
bool BPowerEntry::operator==(const BPowerEntry& pe) const
{
   return (mProtoPowerID == pe.mProtoPowerID);
}

//==============================================================================
//==============================================================================
bool BPowerEntry::save(BStream* pStream, int saveType) const
{
   GFWRITECLASSARRAY(pStream, saveType, mItems, uint16, 1000);
   GFWRITEVAR(pStream, BProtoPowerID, mProtoPowerID);
   GFWRITEVAR(pStream, long, mTimesUsed);      
   GFWRITEVAR(pStream, int, mIconLocation);
   GFWRITEBITBOOL(pStream, mFlagIgnoreCost);
   GFWRITEBITBOOL(pStream, mFlagIgnoreTechPrereqs);
   GFWRITEBITBOOL(pStream, mFlagIgnorePop);      
   return true;
}

//==============================================================================
//==============================================================================
bool BPowerEntry::load(BStream* pStream, int saveType)
{
   GFREADCLASSARRAY(pStream, saveType, mItems, uint16, 1000);
   GFREADVAR(pStream, BProtoPowerID, mProtoPowerID);
   GFREADVAR(pStream, long, mTimesUsed);      
   GFREADVAR(pStream, int, mIconLocation);
   GFREADBITBOOL(pStream, mFlagIgnoreCost);
   GFREADBITBOOL(pStream, mFlagIgnoreTechPrereqs);
   GFREADBITBOOL(pStream, mFlagIgnorePop);      
   return true;
}

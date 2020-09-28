//==============================================================================
// civ.h
//
// Copyright (c) 1999-2007 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "bitvector.h"
#include "cost.h"
#include "xmlreader.h"

// Forward declarations

//==============================================================================
// BCivUnit
//==============================================================================
/*class BCivUnit
{
   public:
      BCivUnit() : mPlayerOwned(false), mUnitID(-1), mOffset(cOriginVector), mAutoTrainID(-1), mAutoTrainCount(0) {}
      BCivUnit(const BCivUnit& source) { *this=source; }
      BCivUnit& operator=(const BCivUnit& source)
      {
         if(this==&source)
            return *this;
         mPlayerOwned=source.mPlayerOwned;
         mUnitID=source.mUnitID;
         mOffset=source.mOffset;
         mAutoTrainID=source.mAutoTrainID;
         mAutoTrainCount=source.mAutoTrainCount;
         return *this;
      }
      BVector  mOffset;
      long     mUnitID;
      long     mAutoTrainID;
      long     mAutoTrainCount;
      bool     mPlayerOwned;
};*/

//==============================================================================
// BCivPop
//==============================================================================
/*class BCivPop
{
   public:
      BCivPop() : mID(-1), mCap(0.0f), mMax(0.0f) {}
      BCivPop(const BCivPop& source) { *this=source; }
      BCivPop& operator=(const BCivPop& source)
      {
         if(this==&source)
            return *this;
         mID=source.mID;
         mCap=source.mCap;
         mMax=source.mMax;
         return *this;
      }
      long     mID;
      float    mCap;
      float    mMax;
};*/

//==============================================================================
// class BCiv
//==============================================================================
class BCiv
{
   public:

      BCiv(long id);
      ~BCiv();
      const BUString&    getDisplayName() const;
      bool               preload(BXMLNode root);
      bool               load(BXMLNode root);
      long               getID() const { return mID; }
      const BSimString&  getCivName() const { return mName; }
      long               getCivTech() const { return mCivTech; }
      long               getCommandAckObjectID( void ) const { return mCommandAckObjectID; }
      long               getRallyPointObjectID( void ) const { return mRallyPointObjectID; }
      long               getLocalRallyPointObjectID( void ) const { return mLocalRallyPointObjectID; }
      float              getHullExpansionRadius() const { return mHullExpansionRadius; }
      float              getTerrainPushOffRadius() const { return mTerrainPushOffRadius; }
      float              getBuildingMagnetRange() const { return mBuildingMagnetRange; }
      long               getTransportProtoID() const { return mTransportProtoID; }
      long               getTransportTriggerProtoID() const { return mTransportTriggerProtoID; }
      const BSimString&  getSoundBank() const {return mSoundBank; }
      long               getLeaderMenuNameIndex() const { return mLeaderMenuNameIndex; }
      int                getAlpha() const { return mAlpha; }
      bool               getPowerFromHero() const { return mPowerFromHero; }
      const BString&     getUIBackgroundImage() const { return mUIBackground; }

   protected:

      // mrh 5/17/07 - Reordered for cache
      BSimString  mName;
      BSimString  mSoundBank;
      BString     mUIBackground;
      long        mID;
      long        mDisplayNameStringIndex;
      long        mCivTech;
      long        mCommandAckObjectID;
      long        mRallyPointObjectID;
      long        mLocalRallyPointObjectID;
      float       mHullExpansionRadius;
      float       mTerrainPushOffRadius;
      float       mBuildingMagnetRange;
      long        mTransportProtoID;
      long        mTransportTriggerProtoID;
      long        mLeaderMenuNameIndex;
      int         mAlpha;
      bool        mPowerFromHero;

};
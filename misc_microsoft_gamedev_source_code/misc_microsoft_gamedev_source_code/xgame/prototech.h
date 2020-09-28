//==============================================================================
// prototech.h
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "bitvector.h"
#include "cost.h"
#include "xmlreader.h"

// Forward declarations
class BTechEffect;
class BTechTree;

//==============================================================================
// BProtoTechStatic
//==============================================================================
class BProtoTechStatic
{
   public:
                                       BProtoTechStatic(long id);
                                       ~BProtoTechStatic();

      BSimString                       mName;
      long                             mID;
      long                             mDBID;
      long                             mDisplayNameIndex;
      long                             mRolloverTextIndex;
      long                             mPrereqTextIndex;
      BSimString                       mIcon;
      long                             mAnimType;
      BDynamicSimLongArray             mTechPrereqs;
      BSimDWORDArray                   mUnitPrereqs;
      BDynamicSimArray<BTechEffect*>   mTechEffects;
      BDynamicSimLongArray             mDependentTechs;
      BSmallDynamicSimArray<int>       mVisualLogicProtoUnits;
      long                             mProtoVisualIndex;
      int                              mCircleMenuIconID;
      int                              mStatsProtoID;
      int                              mUINotificationID;
      BCueIndex                        mCompletedSoundCue;
      int                              mAlpha;
      bool                             mHiddenFromStats;
};

//==============================================================================
// BProtoTech
//==============================================================================
class BProtoTech
{
   public:

                              BProtoTech();
                              BProtoTech(const BProtoTech* pBase);
                              ~BProtoTech();

      bool                    init(long id);
      bool                    load(BXMLNode root);

      void                    applyEffects(BTechTree*  pTechTree, long unitID, bool noCost);
      void                    unapplyEffects(BTechTree*  pTechTree, long unitID);

      // Dynamic data access (changeable per player)      
      const BCost*            getCost() const { return &mCost; }
      float                   getResearchPoints() const { return mResearchPoints; }

      void                    setCost(BCost* pVal) { mCost=*pVal; }
      void                    setResearchPoints(float val) { mResearchPoints=val; }

      // Static data access
      long                    getID() const { return mpStaticData->mID; }
      long                    getDBID() const { return mpStaticData->mDBID; }
      const BSimString&       getName() const { return mpStaticData->mName; }
      void                    getDisplayName(BUString& string) const;
      void                    getRolloverText(BUString& string) const;
      void                    getPrereqText(BUString& string) const;
      const BSimString&       getIcon() const { return mpStaticData->mIcon; }
      long                    getAnimType() const { return mpStaticData->mAnimType; }
      long                    getNumberTechPrereqs() const { return mpStaticData->mTechPrereqs.getNumber(); }
      long                    getTechPrereq(long index) const { return mpStaticData->mTechPrereqs[index]; }
      long                    getNumberUnitPrereqs() const { return mpStaticData->mUnitPrereqs.getNumber(); }
      DWORD                   getUnitPrereq(long index) const { return mpStaticData->mUnitPrereqs[index]; }
      long                    getNumberDependentTechs() const { return mpStaticData->mDependentTechs.getNumber(); }
      long                    getDependentTech(long index) const { return mpStaticData->mDependentTechs[index]; }
      uint                    getNumberVisualLogicProtoUnits() const { return mpStaticData->mVisualLogicProtoUnits.getSize(); }
      int                     getVisualLogicProtoUnit(uint index) const { return mpStaticData->mVisualLogicProtoUnits[index]; }
      void                    addVisualLogicProtoUnit(int protoID) { mpStaticData->mVisualLogicProtoUnits.add(protoID); }
      long                    getProtoVisualIndex() const { return mpStaticData->mProtoVisualIndex; }
      int                     getCircleMenuIconID() const { return mpStaticData->mCircleMenuIconID; }
      void                    setCircleMenuIconID(int id) const { mpStaticData->mCircleMenuIconID=id; }
      int                     getStatsProtoID() const { return mpStaticData->mStatsProtoID; }
      int                     getUINotificationID() const { return mpStaticData->mUINotificationID; }
      void                    setUINotificationID(int id) { mpStaticData->mUINotificationID = id; }
      BCueIndex               getResearchCompleteSound() const { return mpStaticData->mCompletedSoundCue; }
      int                     getAlpha() const { return mpStaticData->mAlpha; }
      bool                    getHiddenFromStats() const { return mpStaticData->mHiddenFromStats; }

      //-- Flags
      bool                       getFlagOwnStaticData() const { return(mFlagOwnStaticData); }
      void                       setFlagOwnStaticData(bool v) { mFlagOwnStaticData=v; }
      bool                       getFlagUnobtainable() const { return(mFlagUnobtainable); }
      void                       setFlagUnobtainable(bool v) { mFlagUnobtainable=v; }
      bool                       getFlagUnique() const { return(mFlagUnique); }
      void                       setFlagUnique(bool v) { mFlagUnique=v; }
      bool                       getFlagShadow() const { return(mFlagShadow); }
      void                       setFlagShadow(bool v) { mFlagShadow=v; }
      bool                       getFlagOrPrereqs() const { return(mFlagOrPrereqs); }
      void                       setFlagOrPrereqs(bool v) { mFlagOrPrereqs=v; }
      bool                       getFlagPerpetual() const { return(mFlagPerpetual); }
      void                       setFlagPerpetual(bool v) { mFlagPerpetual=v; }
      bool                       getFlagForbid() const { return(mFlagForbid); }
      void                       setFlagForbid(bool v) { mFlagForbid=v; }
      bool                       getFlagNoSound() const { return(mFlagNoSound); }
      void                       setFlagNoSound(bool v) { mFlagNoSound=v; }
      bool                       getFlagInstant() const { return(mFlagInstant); }
      void                       setFlagInstant(bool v) { mFlagInstant=v; }


      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   protected:
      bool                       addDependentTech(long techID) { return(mpStaticData->mDependentTechs.add(techID)!=-1); }      

      BProtoTechStatic*          mpStaticData;

      BCost                      mCost;
      float                      mResearchPoints;

      //-- Flags
      bool                       mFlagOwnStaticData:1;
      bool                       mFlagUnobtainable:1;
      bool                       mFlagUnique:1;
      bool                       mFlagShadow:1;
      bool                       mFlagOrPrereqs:1;
      bool                       mFlagPerpetual:1;
      bool                       mFlagForbid:1;
      bool                       mFlagNoSound:1;
      bool                       mFlagInstant:1;

};

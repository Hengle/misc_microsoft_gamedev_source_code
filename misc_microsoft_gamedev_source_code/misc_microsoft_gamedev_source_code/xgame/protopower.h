//==============================================================================
// protopower.h
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================
#pragma once

// xgame
#include "cost.h"
#include "pop.h"
// xrender
#include "D3DTextureManager.h"
//xsystem
#include "bitvector.h"
#include "userminigame.h"

class BProtoObject;

//==============================================================================
//==============================================================================
class BPowerDynamicCost
{
public:
   int   mObjectType;
   float mMultiplier;
};
typedef BSmallDynamicSimArray<BPowerDynamicCost> BPowerDynamicCostArray;

//==============================================================================
//==============================================================================
class BPowerTargetEffectiveness
{
public:
   int   mObjectType;
   int   mEffectiveness;
};
typedef BSmallDynamicSimArray<BPowerTargetEffectiveness> BPowerTargetEffectivenessArray;

namespace ProtoPowerDataType
{
   enum
   {
      cInvalid,
      cFloat,
      cInt,
      cProtoObject,
      cProtoSquad,
      cTech,
      cBool,
      cCost,
      cObjectType,
      cSound,
      cTexture,

      cMin = cInvalid,
      cMax = cTexture,
   };
}
typedef uint BProtoPowerDataType;
typedef BUInt8<BProtoPowerDataType, ProtoPowerDataType::cMin, ProtoPowerDataType::cMax> BSmallProtoPowerDataType;


class BProtoPowerDataFloat;
class BProtoPowerDataInt;
class BProtoPowerDataProtoObject;
class BProtoPowerDataProtoSquad;
class BProtoPowerDataTech;
class BProtoPowerDataBool;
class BProtoPowerDataObjectType;
class BProtoPowerDataSound;
class BProtoPowerDataTexture;

//==============================================================================
//==============================================================================
class BProtoPowerData
{
public:
   const BProtoPowerDataFloat* asFloat() const;
   const BProtoPowerDataInt* asInt() const;
   const BProtoPowerDataProtoObject* asProtoObject() const;
   const BProtoPowerDataProtoSquad* asProtoSquad() const;
   const BProtoPowerDataTech* asTech() const;
   const BProtoPowerDataBool* asBool() const;
   const BProtoPowerDataObjectType* asObjectType() const;
   const BProtoPowerDataSound* asSound() const;
   const BProtoPowerDataTexture* asTexture() const;

   BSimString mName;
   BSmallProtoPowerDataType mType;

protected:
   // Don't new the base class.
   BProtoPowerData() { mType = ProtoPowerDataType::cInvalid; mName = ""; }
};


//==============================================================================
//==============================================================================
class BProtoPowerDataFloat : public BProtoPowerData
{
public:
   BProtoPowerDataFloat() { mType = ProtoPowerDataType::cFloat; mData = 0.0f; }
   float mData;
};


//==============================================================================
//==============================================================================
class BProtoPowerDataInt : public BProtoPowerData
{
public:
   BProtoPowerDataInt() { mType = ProtoPowerDataType::cInt; mData = 0; }
   int mData;
};


//==============================================================================
//==============================================================================
class BProtoPowerDataProtoObject : public BProtoPowerData
{
public:
   BProtoPowerDataProtoObject() { mType = ProtoPowerDataType::cProtoObject; mData = cInvalidProtoObjectID; }
   BProtoObjectID mData;
};


//==============================================================================
//==============================================================================
class BProtoPowerDataProtoSquad : public BProtoPowerData
{
public:
   BProtoPowerDataProtoSquad() { mType = ProtoPowerDataType::cProtoSquad; mData = cInvalidProtoSquadID; }
   BProtoSquadID mData;
};


//==============================================================================
//==============================================================================
class BProtoPowerDataTech : public BProtoPowerData
{
public:
   BProtoPowerDataTech() { mType = ProtoPowerDataType::cTech; mData = cInvalidTechID; }
   BProtoTechID mData;
};



//==============================================================================
//==============================================================================
class BProtoPowerDataBool : public BProtoPowerData
{
public:
   BProtoPowerDataBool() { mType = ProtoPowerDataType::cBool; mData = false; }
   bool mData;
};


//==============================================================================
//==============================================================================
class BProtoPowerDataObjectType : public BProtoPowerData
{
public:
   BProtoPowerDataObjectType() { mType = ProtoPowerDataType::cObjectType; mData = cInvalidObjectTypeID; }
   BObjectTypeID mData;
};

//==============================================================================
//==============================================================================
class BProtoPowerDataSound : public BProtoPowerData
{
public:
   BProtoPowerDataSound() { mType = ProtoPowerDataType::cSound; mData = cInvalidCueIndex; }
   BCueIndex mData;
};

//==============================================================================
//==============================================================================
class BProtoPowerDataTexture : public BProtoPowerData
{
public:
   BProtoPowerDataTexture() { mType = ProtoPowerDataType::cTexture; mData = cInvalidManagedTextureHandle; }
   BSimString              mTextureName;
   BManagedTextureHandle   mData;
};

typedef BSmallDynamicSimArray<BProtoPowerData*> BPPDataArray;
typedef BSmallDynamicSimArray<BPPDataArray> BPPLevelArray;


//==============================================================================
// class BProtoPower
// ProtoPower class is loaded at dawn of time and stored in the database.
// It contains a template version of a power and the modules the power contains.
// It is used to instantiate a power instance when a player invokes a power.
//==============================================================================
class BProtoPower
{
public:

   BProtoPower();
   ~BProtoPower() {}

   // Accessor functions
   void init();
   void setName(const BSimString &name);
   const BSimString& getName(void) const;
   BCost* getCost(const BProtoObject* pProtoObject=NULL);
   const BCost* getCost(const BProtoObject* pProtoObject=NULL) const;
   int getTargetEffectiveness(const BProtoObject* pProtoObject) const;
   const BPopArray* getPop() const { return (&mPops); }
   float getUIRadius(void) const { return(mUIRadius); }
   BPowerType getPowerType() const { return (mPowerType); }
   DWORD getAutoRechargeTime(void) const { return(mAutoRechargeTime); }
   int getUseLimit(void) const { return(mUseLimit); }
   const BSimString& getIconTextureName(void) const { return(mIconTextureName); }
   long getNumberIconLocations() const { return mIconLocations.getNumber(); }
   long getIconLocation(long index) const { return mIconLocations[index]; }
   long getNumberTechPrereqs() const { return mTechPrereqs.getNumber(); }
   long getTechPrereq(long index) const { return mTechPrereqs[index]; }

   float getCameraZoomMin() const { return (mCameraZoomMin); }
   float getCameraZoomMax() const { return (mCameraZoomMax); }
   float getCameraPitchMin() const { return (mCameraPitchMin); }
   float getCameraPitchMax() const { return (mCameraPitchMax); }
   int getCameraEffectIn() const { return mCameraEffectIn; }
   int getCameraEffectOut() const { return mCameraEffectOut; }

   float getMinDistanceToSquad() const { return mMinDistanceToSquad; }
   float getMaxDistanceToSquad() const { return mMaxDistanceToSquad; }

   BObjectTypeID getTargetObjectType() const { return mTargetObjectType; }
   BRelationType getTargetRelation() const { return mTargetRelation; }

   // Flags functions   
   bool getFlagAutoRecharges() const { return(mFlagAutoRecharges); }
   void setFlagAutoRecharges(bool v) { mFlagAutoRecharges=v; }
   bool getFlagInfiniteUses() const { return(mFlagInfiniteUses); }
   void setFlagInfiniteUses(bool v) { mFlagInfiniteUses=v; }
   bool getFlagUnitPower() const { return(mFlagUnitPower); }
   void setFlagUnitPower(bool v) { mFlagUnitPower=v; }
   bool getFlagMultiRechargePower() const { return mFlagMultiRechargePower; }
   void setFlagMultiRechargePower(bool v) { mFlagMultiRechargePower=v; }
   bool getFlagShowLimit() const { return(mFlagShowLimit); }
   void setFlagShowLimit(bool v) { mFlagShowLimit=v; }
   bool getFlagShowTargetHighlight() const { return mFlagShowTargetHighlight; }
   void setFlagShowTargetHighlight(bool v) { mFlagShowTargetHighlight=v; }
   bool getFlagLeaderPower() const { return(mFlagLeaderPower); }
   void setFlagLeaderPower(bool v) { mFlagLeaderPower=v; }
   bool getFlagSequentialRecharge() const { return(mFlagSequentialRecharge); }
   void setFlagSequentialRecharge(bool v) { mFlagSequentialRecharge=v; }
   bool getFlagAffectedByDisruption() const { return(mFlagAffectedByDisruption); }
   void setFlagAffectedByDisruption(bool v) { mFlagAffectedByDisruption=v; }

   bool getFlagCameraZoomMin() const { return (mFlagCameraZoomMin); }
   bool getFlagCameraZoomMax() const { return (mFlagCameraZoomMax); }
   bool getFlagCameraPitchMin() const { return (mFlagCameraPitchMin); }
   bool getFlagCameraPitchMax() const { return (mFlagCameraPitchMax); }
   bool getFlagCameraEnableUserScroll() const { return (mFlagCameraEnableUserScroll); }
   bool getFlagCameraEnableUserYaw() const { return (mFlagCameraEnableUserYaw); }
   bool getFlagCameraEnableUserZoom() const { return (mFlagCameraEnableUserZoom); }
   bool getFlagCameraEnableAutoZoomInstant() const { return (mFlagCameraEnableAutoZoomInstant); }
   bool getFlagCameraEnableAutoZoom() const { return (mFlagCameraEnableAutoZoom); }
   bool getFlagShowInPowerMenu() const { return (mFlagShowInPowerMenu); }
   bool getFlagShowTransportArrows() const { return mFlagShowTransportArrows; }

   // Load data
   bool load(BXMLNode node);
   bool preload(BXMLNode node);

   void loadDataLevel(BXMLNode childNode, BPPDataArray& dataArray);

   const BSimString& getTriggerScript() const { return (mTriggerScript); }
   const BSimString& getCommandTriggerScript() const { return (mCommandTriggerScript); }

   void getDisplayName(BUString& string) const;
   void getRolloverText(BUString& string) const;
   void getPrereqText(BUString& string) const;
   void getChooseText(BUString& string) const;

   int  getCircleMenuIconID() const { return mCircleMenuIconID; }
   void setCircleMenuIconID(int id) { mCircleMenuIconID = id; }  

   int  getUIPowerOverlayID() const { return mUIPowerOverlayID; }
   void setUIPowerOverlayID(int id) { mUIPowerOverlayID = id; }  

   int getActionType() const { return mActionType; }

   BMinigame::eMinigameType getMinigameType() const { return mMinigameType; }
   int16 getChildObjectID(uint index) const;

   bool isValidPowerLevel(BPowerLevel level) const;
   BPowerLevel getMaxPowerLevel() const;

   void resolveTextures();

   bool getDataFloat(BPowerLevel level, const BSimString& name, float& data) const;
   bool getDataInt(BPowerLevel level, const BSimString& name, int& data) const;
   bool getDataProtoObject(BPowerLevel level, const BSimString& name, BProtoObjectID& data) const;
   bool getDataProtoSquad(BPowerLevel level, const BSimString& name, BProtoSquadID& data) const;
   bool getDataTech(BPowerLevel level, const BSimString& name, BProtoTechID& data) const;
   bool getDataBool(BPowerLevel level, const BSimString& name, bool& data) const;
   bool getDataObjectType(BPowerLevel level, const BSimString& name, BObjectTypeID& data) const;
   bool getDataSound(BPowerLevel level, const BSimString& name, BCueIndex& data) const;
   bool getDataTexture(BPowerLevel level, const BSimString& name, BManagedTextureHandle& data) const;

protected:

   BSimString mName;                                        // The name of the proto power   
   long mDisplayNameIndex;
   long mRolloverTextIndex;
   long mPrereqTextIndex;
   long mChooseTextIndex;
   float mUIRadius;                                      // The Radius of the UI circle if any
   BPowerType mPowerType;
   BCost mCost;                                          // The cost of the proto power
   BPowerDynamicCostArray mDynamicCosts;
   BPowerTargetEffectivenessArray mTargetEffectiveness;
   BPopArray mPops;                                      // The pop requirements of the proto power
   DWORD mAutoRechargeTime;                              // Time to auto recharge
   int mUseLimit;
   BSimString mTriggerScript;
   BSimString mCommandTriggerScript;
   BSimString mIconTextureName;
   BSmallDynamicSimArray<long> mIconLocations;
   BSmallDynamicSimArray<long> mTechPrereqs;
   int mCircleMenuIconID;
   int mUIPowerOverlayID;
   int mActionType;
   BMinigame::eMinigameType mMinigameType;
   float mCameraZoomMin;
   float mCameraZoomMax;
   float mCameraPitchMin;
   float mCameraPitchMax;
   int mCameraEffectIn;
   int mCameraEffectOut;
   float mMinDistanceToSquad;
   float mMaxDistanceToSquad;
   BObjectTypeID mTargetObjectType;
   BRelationType mTargetRelation;
   BPPDataArray mBaseLevelData;
   BPPLevelArray mLevelData;

   BSmallDynamicSimArray<int16> mChildObjectIDs;

   //-- Flags
   bool                       mFlagAutoRecharges:1;
   bool                       mFlagInfiniteUses:1;
   bool                       mFlagUnitPower:1;
   bool                       mFlagMultiRechargePower:1;
   bool                       mFlagShowLimit:1;
   bool                       mFlagShowTargetHighlight:1;
   bool                       mFlagLeaderPower:1;
   bool                       mFlagSequentialRecharge:1;

   bool                       mFlagCameraZoomMin:1;
   bool                       mFlagCameraZoomMax:1;
   bool                       mFlagCameraPitchMin:1;
   bool                       mFlagCameraPitchMax:1;
   bool                       mFlagCameraEnableUserScroll:1;
   bool                       mFlagCameraEnableUserYaw:1;
   bool                       mFlagCameraEnableUserZoom:1;
   bool                       mFlagCameraEnableAutoZoomInstant:1;

   bool                       mFlagCameraEnableAutoZoom:1;
   bool                       mFlagShowInPowerMenu:1;
   bool                       mFlagShowTransportArrows:1;
   bool                       mFlagAffectedByDisruption:1;
};

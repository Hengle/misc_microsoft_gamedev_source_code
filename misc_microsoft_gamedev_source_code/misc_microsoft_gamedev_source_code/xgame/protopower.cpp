//==============================================================================
// protopower.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
// xgame
#include "common.h"
#include "database.h"
#include "gamedirectories.h"
#include "player.h"
#include "protoobject.h"
#include "protopower.h"
#include "techtree.h"
#include "triggermanager.h"
#include "world.h"
#include "visualmanager.h"
#include "cameraEffectManager.h"
#include "config.h"
#include "configsgame.h"
#include "soundmanager.h"

// xsystem
#include "xmlreader.h"

//==============================================================================
// BProtoPower::BProtoPower
//==============================================================================
BProtoPower::BProtoPower() :
   mDisplayNameIndex(-1),
   mRolloverTextIndex(-1),
   mPrereqTextIndex(-1),
   mChooseTextIndex(-1),
   mUIRadius(0.0f),
   mPowerType(PowerType::cInvalid),
   mAutoRechargeTime(0),
   mUseLimit(0),
   mCircleMenuIconID(-1),
   mUIPowerOverlayID(-1),
   mActionType(-1),
   mMinigameType(BMinigame::cNone),
   mCameraZoomMin(0.0f),
   mCameraZoomMax(0.0f),
   mCameraPitchMin(0.0f),
   mCameraPitchMax(0.0f),
   mCameraEffectIn(-1),
   mCameraEffectOut(-1),
   mMinDistanceToSquad(-1.0f),
   mMaxDistanceToSquad(-1.0f),
   mTargetObjectType(cInvalidObjectTypeID),
   mTargetRelation(cRelationTypeAny)
{
   init();
}

//==============================================================================
// BProtoPower::init
//==============================================================================
void BProtoPower::init()
{
   mDisplayNameIndex = -1;
   mRolloverTextIndex = -1;
   mPrereqTextIndex = -1;
   mChooseTextIndex = -1;
   mUIRadius = 0.0f;
   mPowerType = PowerType::cInvalid;
   mAutoRechargeTime = 0;
   mUseLimit = 0;
   mCircleMenuIconID = -1;
   mUIPowerOverlayID = -1;
   mActionType = -1;
   mMinigameType = BMinigame::cNone;
   mCameraZoomMin = 0.0f;
   mCameraZoomMax = 0.0f;
   mCameraPitchMin = 0.0f;
   mCameraPitchMax = 0.0f;
   mCameraEffectIn = -1;
   mCameraEffectOut = -1;
   mMinDistanceToSquad = -1.0f;
   mMaxDistanceToSquad = -1.0f;
   mTargetObjectType = cInvalidObjectTypeID;
   mTargetRelation = cRelationTypeAny;

   // Init Flags
   mFlagAutoRecharges=false;
   mFlagInfiniteUses=false;
   mFlagUnitPower=false;
   mFlagMultiRechargePower=false;
   mFlagShowLimit=false;
   mFlagShowTargetHighlight=false;
   mFlagLeaderPower=false;
   mFlagSequentialRecharge=false;
   mFlagCameraZoomMin = false;
   mFlagCameraZoomMax = false;
   mFlagCameraPitchMin = false;
   mFlagCameraPitchMax = false;
   mFlagCameraEnableUserScroll = true;
   mFlagCameraEnableUserYaw = true;
   mFlagCameraEnableUserZoom = true;
   mFlagCameraEnableAutoZoomInstant = gConfig.isDefined(cConfigCameraAutoZoomInstant);
   mFlagCameraEnableAutoZoom = true;
   mFlagShowInPowerMenu = true;
   mFlagShowTransportArrows = false;
   mFlagAffectedByDisruption = true;

   mTriggerScript = "";
   mCommandTriggerScript = "";

   mDynamicCosts.clear();
   mTargetEffectiveness.clear();
   mPops.clear();
   mIconLocations.clear();
   mTechPrereqs.clear();
   mBaseLevelData.clear();
   mLevelData.clear();
   mChildObjectIDs.clear();
}
//==============================================================================
// BProtoPower::setName()
//==============================================================================
void BProtoPower::setName(const BSimString &name)
{
   mName = name;
}

//==============================================================================
// BProtoPower::getName()
//==============================================================================
const BSimString& BProtoPower::getName(void) const
{
   return(mName);
}

//==============================================================================
// BProtoPower::getCost
//==============================================================================
BCost* BProtoPower::getCost(const BProtoObject* pProtoObject)
{ 
   if (!pProtoObject)
      return(&mCost);
   else
   {
      uint count=mDynamicCosts.getSize();
      if(count>0)
      {
         for (uint i=0; i<count; i++)
         {
            if (pProtoObject->isType(mDynamicCosts[i].mObjectType))
            {
               static BCost tempCost;
               tempCost=mCost;
               tempCost.multiply(mDynamicCosts[i].mMultiplier);
               return(&tempCost);
            }
         }
      }
      return(&mCost);
   }
}

//==============================================================================
// BProtoPower::getCost
//==============================================================================
const BCost* BProtoPower::getCost(const BProtoObject* pProtoObject) const
{ 
   if (!pProtoObject)
      return(&mCost);
   else
   {
      uint count=mDynamicCosts.getSize();
      if(count>0)
      {
         for (uint i=0; i<count; i++)
         {
            if (pProtoObject->isType(mDynamicCosts[i].mObjectType))
            {
               static BCost tempCost;
               tempCost=mCost;
               tempCost.multiply(mDynamicCosts[i].mMultiplier);
               return(&tempCost);
            }
         }
      }
      return(&mCost);
   }
}

//==============================================================================
// BProtoPower::getTargetEffectiveness
//==============================================================================
int BProtoPower::getTargetEffectiveness(const BProtoObject* pProtoObject) const
{ 
   if (pProtoObject)
   {
      uint count=mTargetEffectiveness.getSize();
      if(count>0)
      {
         for (uint i=0; i<count; i++)
         {
            if (pProtoObject->isType(mTargetEffectiveness[i].mObjectType))
               return mTargetEffectiveness[i].mEffectiveness;
         }
      }
   }
   return -1;
}

//==============================================================================
// BProtoPower::getChildObjectID
//==============================================================================
int16 BProtoPower::getChildObjectID(uint index) const
{ 
   if(index >= 0 && index < mChildObjectIDs.getSize())
      return mChildObjectIDs[index];
   return -1;
}


//==============================================================================
//==============================================================================
const BProtoPowerDataFloat* BProtoPowerData::asFloat() const
{
   BASSERT(mType == ProtoPowerDataType::cFloat);
   if (mType == ProtoPowerDataType::cFloat)
      return (reinterpret_cast<const BProtoPowerDataFloat*>(this));
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
const BProtoPowerDataInt* BProtoPowerData::asInt() const
{
   BASSERT(mType == ProtoPowerDataType::cInt);
   if (mType == ProtoPowerDataType::cInt)
      return (reinterpret_cast<const BProtoPowerDataInt*>(this));
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
const BProtoPowerDataProtoObject* BProtoPowerData::asProtoObject() const
{
   BASSERT(mType == ProtoPowerDataType::cProtoObject);
   if (mType == ProtoPowerDataType::cProtoObject)
      return (reinterpret_cast<const BProtoPowerDataProtoObject*>(this));
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
const BProtoPowerDataProtoSquad* BProtoPowerData::asProtoSquad() const
{
   BASSERT(mType == ProtoPowerDataType::cProtoSquad);
   if (mType == ProtoPowerDataType::cProtoSquad)
      return (reinterpret_cast<const BProtoPowerDataProtoSquad*>(this));
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
const BProtoPowerDataTech* BProtoPowerData::asTech() const
{
   BASSERT(mType == ProtoPowerDataType::cTech);
   if (mType == ProtoPowerDataType::cTech)
      return (reinterpret_cast<const BProtoPowerDataTech*>(this));
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
const BProtoPowerDataBool* BProtoPowerData::asBool() const
{
   BASSERT(mType == ProtoPowerDataType::cBool);
   if (mType == ProtoPowerDataType::cBool)
      return (reinterpret_cast<const BProtoPowerDataBool*>(this));
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
const BProtoPowerDataObjectType* BProtoPowerData::asObjectType() const
{
   BASSERT(mType == ProtoPowerDataType::cObjectType);
   if (mType == ProtoPowerDataType::cObjectType)
      return (reinterpret_cast<const BProtoPowerDataObjectType*>(this));
   else
      return (NULL);
}

//==============================================================================
//==============================================================================
const BProtoPowerDataSound* BProtoPowerData::asSound() const
{
   BASSERT(mType == ProtoPowerDataType::cSound);
   if (mType == ProtoPowerDataType::cSound)
      return (reinterpret_cast<const BProtoPowerDataSound*>(this));
   else
      return (NULL);
}

//==============================================================================
//==============================================================================
const BProtoPowerDataTexture* BProtoPowerData::asTexture() const
{
   BASSERT(mType == ProtoPowerDataType::cTexture);
   if (mType == ProtoPowerDataType::cTexture)
      return (reinterpret_cast<const BProtoPowerDataTexture*>(this));
   else
      return (NULL);
}


//==============================================================================
//==============================================================================
bool BProtoPower::isValidPowerLevel(BPowerLevel level) const
{
   return (level < mLevelData.getSize());
}

//==============================================================================
// Get the maximum power level index
//==============================================================================
BPowerLevel BProtoPower::getMaxPowerLevel() const
{
   uint maxLevel = mLevelData.getSize();
   if (maxLevel > 1)
   {
      return (maxLevel - 1);
   }
   else
   {
      return (0);
   }
}

//==============================================================================
//==============================================================================
void BProtoPower::resolveTextures()
{
   for(long level = 0; level < mLevelData.getNumber(); level++)
   {
      BPPDataArray dataArray = mLevelData[level];
      uint numDataItems = dataArray.getSize();
      for (uint i=0; i<numDataItems; i++)
      {
         if (!dataArray[i])
            continue;
         if (dataArray[i]->mType != ProtoPowerDataType::cTexture)
            continue;
         BProtoPowerDataTexture* pDataTexture = const_cast<BProtoPowerDataTexture*>(dataArray[i]->asTexture());
         if (!pDataTexture)
            continue;

         // Good enough.
         pDataTexture->mData = gD3DTextureManager.getOrCreateHandle(pDataTexture->mTextureName, BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "BProtoPower");
      }
   }

   uint numDataItems = mBaseLevelData.getSize();
   for (uint i=0; i<numDataItems; i++)
   {
      if (!mBaseLevelData[i])
         continue;
      if (mBaseLevelData[i]->mType != ProtoPowerDataType::cTexture)
         continue;
      BProtoPowerDataTexture* pDataTexture = const_cast<BProtoPowerDataTexture*>(mBaseLevelData[i]->asTexture());
      if (!pDataTexture)
         continue;

      // Good enough.
      pDataTexture->mData = gD3DTextureManager.getOrCreateHandle(pDataTexture->mTextureName, BFILE_OPEN_NORMAL, BD3DTextureManager::cUI, false, cDefaultTextureRed, true, false, "BProtoPower");
   }

}

//==============================================================================
//==============================================================================
bool BProtoPower::getDataFloat(BPowerLevel level, const BSimString& name, float& data) const
{
   if (level >= mLevelData.getSize())
      return (false);

   const BPPDataArray& dataArray = mLevelData[level];
   uint numDataItems = dataArray.getSize();
   for (uint i=0; i<numDataItems; i++)
   {
      if (!dataArray[i])
         continue;
      if (dataArray[i]->mType != ProtoPowerDataType::cFloat)
         continue;
      if (dataArray[i]->mName != name)
         continue;
      const BProtoPowerDataFloat* pDataFloat = dataArray[i]->asFloat();
      if (!pDataFloat)
         continue;

      // Good enough.
      data = pDataFloat->mData;
      return (true);
   }

   numDataItems = mBaseLevelData.getSize();
   for (i=0; i<numDataItems; i++)
   {
      if (!mBaseLevelData[i])
         continue;
      if (mBaseLevelData[i]->mType != ProtoPowerDataType::cFloat)
         continue;
      if (mBaseLevelData[i]->mName != name)
         continue;
      const BProtoPowerDataFloat* pDataFloat = mBaseLevelData[i]->asFloat();
      if (!pDataFloat)
         continue;

      // Good enough.
      data = pDataFloat->mData;
      return (true);
   }

   // Did not find the data.
   return (false);
}


//==============================================================================
//==============================================================================
bool BProtoPower::getDataInt(BPowerLevel level, const BSimString& name, int& data) const
{
   if (level >= mLevelData.getSize())
      return (false);

   const BPPDataArray& dataArray = mLevelData[level];
   uint numDataItems = dataArray.getSize();
   for (uint i=0; i<numDataItems; i++)
   {
      if (!dataArray[i])
         continue;
      if (dataArray[i]->mType != ProtoPowerDataType::cInt)
         continue;
      if (dataArray[i]->mName != name)
         continue;
      const BProtoPowerDataInt* pDataInt = dataArray[i]->asInt();
      if (!pDataInt)
         continue;

      // Good enough.
      data = pDataInt->mData;
      return (true);
   }

   numDataItems = mBaseLevelData.getSize();
   for (i=0; i<numDataItems; i++)
   {
      if (!mBaseLevelData[i])
         continue;
      if (mBaseLevelData[i]->mType != ProtoPowerDataType::cInt)
         continue;
      if (mBaseLevelData[i]->mName != name)
         continue;
      const BProtoPowerDataInt* pDataInt = mBaseLevelData[i]->asInt();
      if (!pDataInt)
         continue;

      // Good enough.
      data = pDataInt->mData;
      return (true);
   }

   // Did not find the data.
   return (false);
}


//==============================================================================
//==============================================================================
bool BProtoPower::getDataProtoObject(BPowerLevel level, const BSimString& name, BProtoObjectID& data) const
{
   if (level >= mLevelData.getSize())
      return (false);

   const BPPDataArray& dataArray = mLevelData[level];
   uint numDataItems = dataArray.getSize();
   for (uint i=0; i<numDataItems; i++)
   {
      if (!dataArray[i])
         continue;
      if (dataArray[i]->mType != ProtoPowerDataType::cProtoObject)
         continue;
      if (dataArray[i]->mName != name)
         continue;
      const BProtoPowerDataProtoObject* pDataProtoObject = dataArray[i]->asProtoObject();
      if (!pDataProtoObject)
         continue;

      // Good enough.
      data = pDataProtoObject->mData;
      return (true);
   }

   numDataItems = mBaseLevelData.getSize();
   for (i=0; i<numDataItems; i++)
   {
      if (!mBaseLevelData[i])
         continue;
      if (mBaseLevelData[i]->mType != ProtoPowerDataType::cProtoObject)
         continue;
      if (mBaseLevelData[i]->mName != name)
         continue;
      const BProtoPowerDataProtoObject* pDataProtoObject = mBaseLevelData[i]->asProtoObject();
      if (!pDataProtoObject)
         continue;

      // Good enough.
      data = pDataProtoObject->mData;
      return (true);
   }

   // Did not find the data.
   return (false);
}


//==============================================================================
//==============================================================================
bool BProtoPower::getDataProtoSquad(BPowerLevel level, const BSimString& name, BProtoSquadID& data) const
{
   if (level >= mLevelData.getSize())
      return (false);

   const BPPDataArray& dataArray = mLevelData[level];
   uint numDataItems = dataArray.getSize();
   for (uint i=0; i<numDataItems; i++)
   {
      if (!dataArray[i])
         continue;
      if (dataArray[i]->mType != ProtoPowerDataType::cProtoSquad)
         continue;
      if (dataArray[i]->mName != name)
         continue;
      const BProtoPowerDataProtoSquad* pDataProtoSquad = dataArray[i]->asProtoSquad();
      if (!pDataProtoSquad)
         continue;

      // Good enough.
      data = pDataProtoSquad->mData;
      return (true);
   }

   numDataItems = mBaseLevelData.getSize();
   for (i=0; i<numDataItems; i++)
   {
      if (!mBaseLevelData[i])
         continue;
      if (mBaseLevelData[i]->mType != ProtoPowerDataType::cProtoSquad)
         continue;
      if (mBaseLevelData[i]->mName != name)
         continue;
      const BProtoPowerDataProtoSquad* pDataProtoSquad = mBaseLevelData[i]->asProtoSquad();
      if (!pDataProtoSquad)
         continue;

      // Good enough.
      data = pDataProtoSquad->mData;
      return (true);
   }

   // Did not find the data.
   return (false);
}


//==============================================================================
//==============================================================================
bool BProtoPower::getDataTech(BPowerLevel level, const BSimString& name, BProtoTechID& data) const
{
   if (level >= mLevelData.getSize())
      return (false);

   const BPPDataArray& dataArray = mLevelData[level];
   uint numDataItems = dataArray.getSize();
   for (uint i=0; i<numDataItems; i++)
   {
      if (!dataArray[i])
         continue;
      if (dataArray[i]->mType != ProtoPowerDataType::cTech)
         continue;
      if (dataArray[i]->mName != name)
         continue;
      const BProtoPowerDataTech* pDataTech = dataArray[i]->asTech();
      if (!pDataTech)
         continue;

      // Good enough.
      data = pDataTech->mData;
      return (true);
   }

   numDataItems = mBaseLevelData.getSize();
   for (i=0; i<numDataItems; i++)
   {
      if (!mBaseLevelData[i])
         continue;
      if (mBaseLevelData[i]->mType != ProtoPowerDataType::cTech)
         continue;
      if (mBaseLevelData[i]->mName != name)
         continue;
      const BProtoPowerDataTech* pDataTech = mBaseLevelData[i]->asTech();
      if (!pDataTech)
         continue;

      // Good enough.
      data = pDataTech->mData;
      return (true);
   }

   // Did not find the data.
   return (false);
}




//==============================================================================
//==============================================================================
bool BProtoPower::getDataBool(BPowerLevel level, const BSimString& name, bool& data) const
{
   if (level >= mLevelData.getSize())
      return (false);

   const BPPDataArray& dataArray = mLevelData[level];
   uint numDataItems = dataArray.getSize();
   for (uint i=0; i<numDataItems; i++)
   {
      if (!dataArray[i])
         continue;
      if (dataArray[i]->mType != ProtoPowerDataType::cBool)
         continue;
      if (dataArray[i]->mName != name)
         continue;
      const BProtoPowerDataBool* pDataBool = dataArray[i]->asBool();
      if (!pDataBool)
         continue;

      // Good enough.
      data = pDataBool->mData;
      return (true);
   }

   numDataItems = mBaseLevelData.getSize();
   for (i=0; i<numDataItems; i++)
   {
      if (!mBaseLevelData[i])
         continue;
      if (mBaseLevelData[i]->mType != ProtoPowerDataType::cBool)
         continue;
      if (mBaseLevelData[i]->mName != name)
         continue;
      const BProtoPowerDataBool* pDataBool = mBaseLevelData[i]->asBool();
      if (!pDataBool)
         continue;

      // Good enough.
      data = pDataBool->mData;
      return (true);
   }

   // Did not find the data.
   return (false);
}


//==============================================================================
//==============================================================================
bool BProtoPower::getDataObjectType(BPowerLevel level, const BSimString& name, BObjectTypeID& data) const
{
   if (level >= mLevelData.getSize())
      return (false);

   const BPPDataArray& dataArray = mLevelData[level];
   uint numDataItems = dataArray.getSize();
   for (uint i=0; i<numDataItems; i++)
   {
      if (!dataArray[i])
         continue;
      if (dataArray[i]->mType != ProtoPowerDataType::cObjectType)
         continue;
      if (dataArray[i]->mName != name)
         continue;
      const BProtoPowerDataObjectType* pDataObjectType = dataArray[i]->asObjectType();
      if (!pDataObjectType)
         continue;

      // Good enough.
      data = pDataObjectType->mData;
      return (true);
   }

   numDataItems = mBaseLevelData.getSize();
   for (i=0; i<numDataItems; i++)
   {
      if (!mBaseLevelData[i])
         continue;
      if (mBaseLevelData[i]->mType != ProtoPowerDataType::cObjectType)
         continue;
      if (mBaseLevelData[i]->mName != name)
         continue;
      const BProtoPowerDataObjectType* pDataObjectType = mBaseLevelData[i]->asObjectType();
      if (!pDataObjectType)
         continue;

      // Good enough.
      data = pDataObjectType->mData;
      return (true);
   }

   // Did not find the data.
   return (false);
}

//==============================================================================
//==============================================================================
bool BProtoPower::getDataSound(BPowerLevel level, const BSimString& name, BCueIndex& data) const
{
   if (level >= mLevelData.getSize())
      return (false);

   const BPPDataArray& dataArray = mLevelData[level];
   uint numDataItems = dataArray.getSize();
   for (uint i=0; i<numDataItems; i++)
   {
      if (!dataArray[i])
         continue;
      if (dataArray[i]->mType != ProtoPowerDataType::cSound)
         continue;
      if (dataArray[i]->mName != name)
         continue;
      const BProtoPowerDataSound* pDataSound = dataArray[i]->asSound();
      if (!pDataSound)
         continue;

      // Good enough.
      data = pDataSound->mData;
      return (true);
   }

   numDataItems = mBaseLevelData.getSize();
   for (i=0; i<numDataItems; i++)
   {
      if (!mBaseLevelData[i])
         continue;
      if (mBaseLevelData[i]->mType != ProtoPowerDataType::cSound)
         continue;
      if (mBaseLevelData[i]->mName != name)
         continue;
      const BProtoPowerDataSound* pDataSound = mBaseLevelData[i]->asSound();
      if (!pDataSound)
         continue;

      // Good enough.
      data = pDataSound->mData;
      return (true);
   }

   // Did not find the data.
   return (false);
}

//==============================================================================
//==============================================================================
bool BProtoPower::getDataTexture(BPowerLevel level, const BSimString& name, BManagedTextureHandle& data) const
{
   if (level >= mLevelData.getSize())
      return (false);

   const BPPDataArray& dataArray = mLevelData[level];
   uint numDataItems = dataArray.getSize();
   for (uint i=0; i<numDataItems; i++)
   {
      if (!dataArray[i])
         continue;
      if (dataArray[i]->mType != ProtoPowerDataType::cTexture)
         continue;
      if (dataArray[i]->mName != name)
         continue;
      const BProtoPowerDataTexture* pDataTexture = dataArray[i]->asTexture();
      if (!pDataTexture)
         continue;

      // Good enough.
      data = pDataTexture->mData;
      return (true);
   }

   numDataItems = mBaseLevelData.getSize();
   for (i=0; i<numDataItems; i++)
   {
      if (!mBaseLevelData[i])
         continue;
      if (mBaseLevelData[i]->mType != ProtoPowerDataType::cTexture)
         continue;
      if (mBaseLevelData[i]->mName != name)
         continue;
      const BProtoPowerDataTexture* pDataTexture = mBaseLevelData[i]->asTexture();
      if (!pDataTexture)
         continue;

      // Good enough.
      data = pDataTexture->mData;
      return (true);
   }

   // Did not find the data.
   return (false);
}


//==============================================================================
// BProtoPower::load()
//==============================================================================
bool BProtoPower::load(BXMLNode node)
{
   // re-init, so reloading proto powers works, 
   // but we save the name, since that's done in preload
   BSimString name = getName();
   init();
   setName(name);

   //===========================================================================
   // Process the attributes of the power
   //===========================================================================
   BXMLNode PPCN(node.getChildNode(B("Attributes")));
   if (PPCN)
   {
      BSimString tempStr;
      long numChildren = PPCN.getNumberChildren();
      for (long i=0; i<numChildren; i++)
      {
         BXMLNode child(PPCN.getChild(i));
         const BPackedString name(child.getName());
         if (name == "Cost")
         {
            long attributeCount = child.getAttributeCount();
            for (long i=0; i<attributeCount; i++)
            {
               BXMLAttribute attribute(child.getAttribute(i));
               
               BSimString resourceName;
               attribute.getName(resourceName);
               long resourceID = gDatabase.getResource(resourceName);
               if (resourceID == -1)
                  continue;

               float resourceValue = 0.0f;
               if (!attribute.getValueAsFloat(resourceValue))
                  continue;

               mCost.set(resourceID, resourceValue);
            }
         }
         else if (name == "DynamicCost")
         {
            BSimString typeName;
            if (child.getAttribValue("ObjectType", &typeName))
            {
               BPowerDynamicCost item;
               item.mObjectType=gDatabase.getObjectType(typeName);
               if(item.mObjectType!=-1)
               {
                  item.mMultiplier=1.0f;
                  if (child.getTextAsFloat(item.mMultiplier))
                     mDynamicCosts.add(item);
               }
            }
         }
         else if (name == "TargetEffectiveness")
         {
            BSimString typeName;
            if (child.getAttribValue("ObjectType", &typeName))
            {
               BPowerTargetEffectiveness item;
               item.mObjectType=gDatabase.getObjectType(typeName);
               if(item.mObjectType!=-1)
               {
                  item.mEffectiveness=0;
                  if (child.getTextAsInt(item.mEffectiveness))
                     mTargetEffectiveness.add(item);
               }
            }
         }
         else if (name == "Pop")
         {
            BSimString typeName;
            if (child.getAttribValue("type", &typeName))
            {
               int popID = gDatabase.getPop(typeName);
               if (popID != -1)
               {
                  float popCount = 0.0f;
                  if (child.getTextAsFloat(popCount))
                  {
                     BPop pop;
                     pop.mID = (short)popID;
                     pop.mCount = popCount;
                     mPops.add(pop);
                  }
               }
            }
         }
         else if (name == "UIRadius")
         {
            float uiRadius = 0.0f;
            if(child.getTextAsFloat(uiRadius))
               mUIRadius = uiRadius;
         }
         else if (name == "PowerType")
            mPowerType = gDatabase.getPowerType(child.getText(tempStr));
         else if (name == "AutoRecharge")
         {
            DWORD rechargeTime = 0;
            if (child.getTextAsDWORD(rechargeTime))
            {
               setFlagAutoRecharges(true);
               mAutoRechargeTime = rechargeTime;
            }
         }
         else if (name == "UseLimit")
            child.getTextAsInt(mUseLimit);
         else if (name == "SequentialRecharge")
            mFlagSequentialRecharge = true;
         else if (name == "InfiniteUses")
         {
            setFlagInfiniteUses(true);
         }
         else if (name == "UnitPower")
         {
            setFlagUnitPower(true);
         }
         else if (name == "NotDisruptable")
            setFlagAffectedByDisruption(false);
         else if (name == "MultiRechargePower")
            mFlagMultiRechargePower=true;
         else if (name == "ShowLimit")
            mFlagShowLimit=true;
         else if (name == "ShowTargetHighlight")
            mFlagShowTargetHighlight=true;
         else if (name == "LeaderPower")
            mFlagLeaderPower=true;
         else if (name == "Icon")
         {
            child.getText(mIconTextureName);
         }
         else if (name == "IconLocation")
         {
            long iconLocation;
            if(child.getTextAsLong(iconLocation))
               mIconLocations.add(iconLocation);
         }
         else if (name == "TechPrereq")
         {
            long techPrereq = gDatabase.getProtoTech(child.getTextPtr(tempStr));
            if (techPrereq >= 0)
               mTechPrereqs.add(techPrereq);
         }
         else if (name == "DisplayNameID")
         {
            long id;
            if(child.getTextAsLong(id))
               mDisplayNameIndex=gDatabase.getLocStringIndex(id);
         }
         else if (name == "RolloverTextID")
         {
            long id;
            if(child.getTextAsLong(id))
               mRolloverTextIndex=gDatabase.getLocStringIndex(id);
         }
         else if (name == "PrereqTextID")
         {
            long id;
            if(child.getTextAsLong(id))
               mPrereqTextIndex=gDatabase.getLocStringIndex(id);
         }
         else if (name == "ChooseTextID")
         {
            long id;
            if(child.getTextAsLong(id))
               mChooseTextIndex=gDatabase.getLocStringIndex(id);
         }
         else if (name == "Action")
         {
            mActionType = gDatabase.getActionTypeByName(child.getTextPtr(tempStr));
         }
         else if (name == "Minigame")
         {
            child.getText(tempStr);
            if (tempStr.compare("OneButtonPress") == 0)
               mMinigameType = BMinigame::cOneButtonPress;
            else if (tempStr.compare("TwoButtonPress") == 0)
               mMinigameType = BMinigame::cTwoButtonPress;
            else if (tempStr.compare("ThreeButtonPress") == 0)
               mMinigameType = BMinigame::cThreeButtonPress;
            else
               mMinigameType = BMinigame::cNone;
         }
         else if (name == "CameraZoomMin")
         {
            if(child.getTextAsFloat(mCameraZoomMin))
               mFlagCameraZoomMin = true;
         }
         else if (name == "CameraZoomMax")
         {
            if(child.getTextAsFloat(mCameraZoomMax))
               mFlagCameraZoomMax = true;
         }
         else if (name == "CameraPitchMin")
         {
            if(child.getTextAsFloat(mCameraPitchMin))
               mFlagCameraPitchMin = true;
         }
         else if (name == "CameraPitchMax")
         {
            if(child.getTextAsFloat(mCameraPitchMax))
               mFlagCameraPitchMax = true;
         }
         else if (name == "CameraEffectIn")
         {
            child.getText(tempStr);
            mCameraEffectIn = gCameraEffectManager.getOrCreateCameraEffect(tempStr.getPtr());
         }
         else if (name == "CameraEffectOut")
         {
            child.getText(tempStr);
            mCameraEffectOut = gCameraEffectManager.getOrCreateCameraEffect(tempStr.getPtr());
         }
         else if (name == "MinDistanceToSquad")
            child.getTextAsFloat(mMinDistanceToSquad);
         else if (name == "MaxDistanceToSquad")
            child.getTextAsFloat(mMaxDistanceToSquad);
         else if (name == "CameraEnableUserScroll")
         {
            bool val=true;
            if (child.getTextAsBool(val))
               mFlagCameraEnableUserScroll=val;
         }
         else if (name == "CameraEnableUserYaw")
         {
            bool val=true;
            if (child.getTextAsBool(val))
               mFlagCameraEnableUserYaw=val;
         }
         else if (name == "CameraEnableUserZoom")
         {
            bool val=true;
            if (child.getTextAsBool(val))
               mFlagCameraEnableUserZoom=val;
         }
         else if (name == "CameraEnableAutoZoomInstant")
         {
            bool val = true;
            if (child.getTextAsBool(val))
            {
               mFlagCameraEnableAutoZoomInstant = val;
            }
         }
         else if (name == "CameraEnableAutoZoom")
         {
            bool val = true;
            if (child.getTextAsBool(val))
            {
               mFlagCameraEnableAutoZoom = val;
            }
         }
         else if (name == "ShowTargetHighlight")
         {
            mFlagShowTargetHighlight=true;
            if (child.getAttribValueAsString("ObjectType", tempStr))
               mTargetObjectType = gDatabase.getObjectType(tempStr);
            if (child.getAttribValueAsString("Relation", tempStr))
               mTargetRelation = gDatabase.getRelationType(tempStr);
         }
         else if (name == "ShowInPowerMenu")
         {
            bool val=true;
            if (child.getTextAsBool(val))
               mFlagShowInPowerMenu=val;
         }
         else if (name == "ShowTransportArrows")
         {
            bool val=true;
            if (child.getTextAsBool(val))
               mFlagShowTransportArrows=val;
         }
         else if (name == "ChildObjects")
         {
            for (int j=0; j<child.getNumberChildren(); j++)
            {
               BXMLNode child2(child.getChild(j));
               const BPackedString childName(child2.getName());
               if (childName == "Object")
               {
                  BPackedString childNodeText(child2.getTextPtr(tempStr));
                  int16 id = (int16)gDatabase.getProtoObject(childNodeText);
                  if (id != -1)
                     mChildObjectIDs.add((int16)gDatabase.getProtoObject(childNodeText));
               }
            }
         }
         else if (name == "DataLevel")
         {
            uint level = 0;
            if (!child.getAttribValueAsUInt("level", level))
            {
               BFAIL("level attribute missing.");
               continue;
            }
            if (level >= mLevelData.getSize())
               mLevelData.resize(level+1);

            loadDataLevel(child, mLevelData[level]);
         }
         else if (name == "BaseDataLevel")
         {
            loadDataLevel(child, mBaseLevelData);
         }
      }
   }

   // Load our file.
   BXMLNode triggerScriptNode(node.getChildNode("TriggerScript"));
   if (triggerScriptNode)
   {
      BSimString tempStr;
      mTriggerScript = triggerScriptNode.getTextPtr(tempStr);
   }         

   // Load unit trigger script.
   BXMLNode unitTriggerScriptNode(node.getChildNode("CommandTriggerScript"));
   if (unitTriggerScriptNode)
   {
      BSimString tempStr;
      mCommandTriggerScript = unitTriggerScriptNode.getTextPtr(tempStr);
   }         

   return(true);
}

//==============================================================================
// BProtoPower::preload()
//==============================================================================
void BProtoPower::loadDataLevel(BXMLNode child, BPPDataArray& dataArray)
{
   BSimString tempStr;

#ifndef BUILD_FINAL
   BSmallDynamicSimArray<BSimString> nameCollisionArray;
#endif

   long numDataNodes = child.getNumberChildren();
   for (long d=0; d<numDataNodes; d++)
   {
      BXMLNode dataNode(child.getChild(d));
      if (dataNode.getName() != "Data")
      {
         BFAIL("Data node improperly named or missing.");
         continue;
      }
      BSimString typeString;
      if (!dataNode.getAttribValueAsString("type", typeString))
      {
         BFAIL("type attribute missing.");
         continue;
      }
      BSimString nameString;
      if (!dataNode.getAttribValueAsString("name", nameString))
      {
         BFAIL("name attribute missing.");
         continue;
      }

#ifndef BUILD_FINAL
      if (nameCollisionArray.contains(nameString))
      {
         BSimString errorString;
         errorString.format("multiple data nodes in same level with name %s", nameString.getPtr());
         BFAIL(errorString);
         continue;
      }
      else
      {
         nameCollisionArray.add(nameString);
      }
#endif

      if (typeString == "float")
      {
         float data = 0.0f;
         if (!dataNode.getTextAsFloat(data))
         {
            BFAIL("float data missing.");
            continue;
         }
         BProtoPowerDataFloat* pNewData = new BProtoPowerDataFloat();
         pNewData->mName = nameString;
         pNewData->mData = data;
         dataArray.add(reinterpret_cast<BProtoPowerData*>(pNewData));
      }
      else if (typeString == "int")
      {
         int data = 0;
         if (!dataNode.getTextAsInt(data))
         {
            BFAIL("int data missing.");
            continue;
         }
         BProtoPowerDataInt* pNewData = new BProtoPowerDataInt();
         pNewData->mName = nameString;
         pNewData->mData = data;
         dataArray.add(reinterpret_cast<BProtoPowerData*>(pNewData));
      }
      else if (typeString == "protoobject")
      {
         BProtoObjectID data = gDatabase.getProtoObject(dataNode.getText(tempStr));
#ifndef BUILD_FINAL
         if (data == cInvalidProtoObjectID)
         {
            BSimString debugString;
            debugString.format("protoobject data %s is invalid", dataNode.getText(tempStr).getPtr());
            BFAIL(debugString);
         }
#endif
         BProtoPowerDataProtoObject* pNewData = new BProtoPowerDataProtoObject();
         pNewData->mName = nameString;
         pNewData->mData = data;
         dataArray.add(reinterpret_cast<BProtoPowerData*>(pNewData));
      }
      else if (typeString == "bool")
      {
         bool data = false;
         if (!dataNode.getTextAsBool(data))
         {
            BFAIL("bool data missing.");
            continue;
         }
         BProtoPowerDataBool* pNewData = new BProtoPowerDataBool();
         pNewData->mName = nameString;
         pNewData->mData = data;
         dataArray.add(reinterpret_cast<BProtoPowerData*>(pNewData));
      }
      else if (typeString == "objecttype")
      {
         BObjectTypeID data = gDatabase.getObjectType(dataNode.getText(tempStr));
#ifndef BUILD_FINAL
         if (data == cInvalidObjectTypeID)
         {
            BSimString debugString;
            debugString.format("objecttype data %s is invalid", dataNode.getText(tempStr).getPtr());
            BFAIL(debugString);
         }
#endif
         BProtoPowerDataObjectType* pNewData = new BProtoPowerDataObjectType();
         pNewData->mName = nameString;
         pNewData->mData = data;
         dataArray.add(reinterpret_cast<BProtoPowerData*>(pNewData));
      }
      else if (typeString == "sound")
      {
         BCueIndex data = gSoundManager.getCueIndex(dataNode.getText(tempStr));         
#ifndef BUILD_FINAL
         if(data == cInvalidCueIndex)
         {
            BSimString str;
            str.format("Powers.xml: Cannot find sound cue: %s", dataNode.getText(tempStr).getPtr());
            gConsole.output(cMsgWarning, str.getPtr());
         }
#endif
         BProtoPowerDataSound* pNewData = new BProtoPowerDataSound();
         pNewData->mName = nameString;
         pNewData->mData = data;
         dataArray.add(reinterpret_cast<BProtoPowerData*>(pNewData));
      }
      else if (typeString == "texture")
      {
         BSimString textureName;
         dataNode.getText(textureName);
         BProtoPowerDataTexture* pNewData = new BProtoPowerDataTexture();
         pNewData->mName = nameString;
         pNewData->mTextureName = textureName;
         dataArray.add(reinterpret_cast<BProtoPowerData*>(pNewData));
      }
      else
         continue;
   }
}

//==============================================================================
// BProtoPower::preload()
//==============================================================================
bool BProtoPower::preload(BXMLNode node)
{
   BSimString name;
   if (!node.getAttribValue("name", &name))
      return(false);

   // Set the ProtoPower name.
   setName(name);

   return true;
}

//==============================================================================
// BProtoPower::getDisplayName
//==============================================================================
void BProtoPower::getDisplayName(BUString& string) const
{
   if(mDisplayNameIndex==-1)
      string=mName.getPtr(); // AJL FIXME - Need to return empty string or other value to indicate missing string
   else
      string=gDatabase.getLocStringFromIndex(mDisplayNameIndex);
}

//==============================================================================
// BProtoPower::getRolloverText
//==============================================================================
void BProtoPower::getRolloverText(BUString& string) const
{
   string=gDatabase.getLocStringFromIndex(mRolloverTextIndex);
}

//==============================================================================
// BProtoPower::getPrereqText
//==============================================================================
void BProtoPower::getPrereqText(BUString& string) const
{
   string=gDatabase.getLocStringFromIndex(mPrereqTextIndex);
}

//==============================================================================
// BProtoPower::getChooseText
//==============================================================================
void BProtoPower::getChooseText(BUString& string) const
{
   string=gDatabase.getLocStringFromIndex(mChooseTextIndex);
}

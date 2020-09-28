//==============================================================================
// protosquad.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "protosquad.h"
#include "xmlreader.h"
#include "formation2.h"
#include "database.h"
#include "cost.h"
#include "visualmanager.h"
#include "uigame.h"
#include "player.h"
#include "tactic.h"
#include "config.h"
#include "configsgame.h"
#include "soundmanager.h"

GFIMPLEMENTVERSION(BProtoSquad, 3);

enum
{
   cSaveMarkerProtoSquad1=10000,
};

const char* gStanceNames[] = {"Passive", "Aggressive", "Defensive"};


//==============================================================================
// lookupStanceType
//==============================================================================
long lookupStanceType( const char *szName)
{
   for (long i = 0; i < BProtoSquad::cNumberStances; i++)
   {
      if (stricmp(gStanceNames[i], szName)==0)
         return i;
   }

   return -1;
}

//==============================================================================
// BProtoSquadStatic::BProtoSquadStatic
//==============================================================================
BProtoSquadStatic::BProtoSquadStatic() :
   mName(),
   mProtoObjectID(-1),
   mDBID(-1),
   mStatsNameIndex(-1),
   mRolloverTextIndex(-1),
   mPrereqTextIndex(-1),
   mRoleTextIndex(-1),
   mSquadSize(0),
   mIcon(),
   mAltIcon(),
   mStance(BProtoSquad::cStanceDefensive),
   mTrainAnimType(-1),
   mSelectionRadiusX(1.0f),
   mSelectionRadiusZ(1.0f),
   mSelectionYOffset(0.0f),
   mSelectionZOffset(0.0f),
   mSelectionConformToTerrain(false),
   mSelectionAllowOrientation(true),
   mSelectionDecalID(-1),
   mClumpSelectionDecalID(BFlashProperties::eInvalidFlashPropertyHandle),
   mClumpSelectionDecalMaskID(BFlashProperties::eInvalidFlashPropertyHandle),
   mStaticDecalID(BFlashProperties::eInvalidFlashPropertyHandle),

   mUnitSelectionIconID(-1),
   mBirthType(-1),
   mBirthBone(),
   mBirthTrainerAnim(-1),
   mHPBarSizeX(0.0f),
   mHPBarSizeY(0.0f),
   mHPBarOffset(cOriginVector),
   mSubSelectSort(INT_MAX),
   mTurnRadiusMin(-1.0f),
   mTurnRadiusMax(-1.0f),
   mAttackGradeDPS(0.0f),
   mDamageType(-1),
   mRecoveringEffect(-1),
   mLeashDistance(0.0f),
   mLeashDeadzone(10.0f),
   mLeashRecallDelay(2000),
   mAggroDistance(0.0f),
   mMinimapScale(0.0f),
   mUnitNodes(),
   mLevels(),
   mFormationType(BFormation2::eTypeStandard),
   mSounds(),
   mUsedSoundTypes(cSquadSoundNone),
   mVeterancyBarID(-1),
   mVeterancyCenteredBarID(-1),
   mVeterancyBarSizeX(1.0f),
   mVeterancyBarSizeY(1.0f),
   mVeterancyBarOffsetX(0.0f),
   mVeterancyBarOffsetY(0.0f),
   mAbilityRecoveryBarID(-1),
   mAbilityRecoveryCenteredBarID(-1),
   mAbilityRecoveryBarSizeX(1.0f),
   mAbilityRecoveryBarSizeY(1.0f),
   mAbilityRecoveryBarOffsetX(0.0f),
   mAbilityRecoveryBarOffsetY(0.0f),
   mBobbleHeadID(-1),
   mBuildingStrengthID(-1),
   mDazeResist(1.0f),
   mID(-1)
{
   mCryoPoints = gDatabase.getDefaultCryoPoints();

   for(int i=0; i<4; i++)
   {
      mBirthAnim[i]=-1;
   }

   // Static flags
   mFlagHasHPBar=false;
   mFlagNoTieToGround=false;
   mFlagRepairable=false;
   mFlagChatter=false;
   mFlagCreateAudioReactions=false;
   mFlagForceToGaiaPlayer=false;
   mFlagInstantTrainWithRecharge=false;
   mFlagAlwaysAttackReviveUnits=false;
   mFlagScaredByRoar=false;
   mFlagAlwaysRenderSelectionDecal=false;
   mFlagOnlyShowBobbleHeadWhenContained = false;
   mFlagDiesWhenFrozen = false;
   mFlagFlyingFlood = false;
   mFlagFlood = false;
   mFlagForerunner = false;
   mFlagRebel = false;
   mFlagCanAttackWhileMoving = true;
   mFlagNoPlatoonMerge = false;
   mFlagObjectProtoSquad = false;
   mFlagMergedProtoSquad = false;
   mFlagAlwaysShowHPBar = false;
}

//==============================================================================
//==============================================================================
BProtoSquadStatic::BProtoSquadStatic(const BProtoSquadStatic* pBase)
{
   mName = pBase->mName;
   mProtoObjectID = pBase->mProtoObjectID;
   mDBID = pBase->mDBID;
   mStatsNameIndex = pBase->mStatsNameIndex;
   mRolloverTextIndex = pBase->mRolloverTextIndex;
   mPrereqTextIndex = pBase->mPrereqTextIndex;
   mRoleTextIndex = pBase->mRoleTextIndex;
   mSquadSize = pBase->mSquadSize;
   mIcon = pBase->mIcon;
   mAltIcon = pBase->mAltIcon;
   mStance = pBase->mStance;
   mTrainAnimType = pBase->mTrainAnimType;
   mSelectionRadiusX = pBase->mSelectionRadiusX;
   mSelectionRadiusZ = pBase->mSelectionRadiusZ;
   mSelectionYOffset = pBase->mSelectionYOffset;
   mSelectionZOffset = pBase->mSelectionZOffset;
   mSelectionConformToTerrain = pBase->mSelectionConformToTerrain;
   mSelectionAllowOrientation = pBase->mSelectionAllowOrientation;
   mSelectionDecalID = pBase->mSelectionDecalID;
   mClumpSelectionDecalID = pBase->mClumpSelectionDecalID;
   mClumpSelectionDecalMaskID = pBase->mClumpSelectionDecalMaskID;
   mStaticDecalID = pBase->mStaticDecalID;
   mUnitSelectionIconID = pBase->mUnitSelectionIconID;
   mBirthType = pBase->mBirthType;
   mBirthBone = pBase->mBirthBone;
   mBirthEndBone = pBase->mBirthEndBone;
   for(int i=0; i<4; i++)
   {
      mBirthAnim[i] = pBase->mBirthAnim[i];
   }
   mBirthTrainerAnim = pBase->mBirthTrainerAnim;
   mHPBarSizeX = pBase->mHPBarSizeX;
   mHPBarSizeY = pBase->mHPBarSizeY;
   mHPBarOffset = pBase->mHPBarOffset;
   mSubSelectSort = pBase->mSubSelectSort;
   mTurnRadiusMin = pBase->mTurnRadiusMin;
   mTurnRadiusMax = pBase->mTurnRadiusMax;
   mAttackGradeDPS = pBase->mAttackGradeDPS;
   mDamageType = pBase->mDamageType;
   mRecoveringEffect = pBase->mRecoveringEffect;
   mLeashDistance = pBase->mLeashDistance;
   mLeashDeadzone = pBase->mLeashDeadzone;
   mLeashRecallDelay = pBase->mLeashRecallDelay;
   mAggroDistance = pBase->mAggroDistance;
   mMinimapScale = pBase->mMinimapScale;
   mUnitNodes = pBase->mUnitNodes;
   mLevels = pBase->mLevels;
   mFormationType = pBase->mFormationType;
   mSounds = pBase->mSounds;
   mUsedSoundTypes = pBase->mUsedSoundTypes;
   mFlagHasHPBar = pBase->mFlagHasHPBar;
   mFlagNoTieToGround = pBase->mFlagNoTieToGround;
   mFlagRepairable = pBase->mFlagRepairable;
   mFlagChatter = pBase->mFlagChatter;
   mFlagCreateAudioReactions = pBase->mFlagCreateAudioReactions;
   mFlagForceToGaiaPlayer = pBase->mFlagForceToGaiaPlayer;
   mFlagInstantTrainWithRecharge = pBase->mFlagInstantTrainWithRecharge;
   mFlagAlwaysAttackReviveUnits = pBase->mFlagAlwaysAttackReviveUnits;
   mFlagScaredByRoar = pBase->mFlagScaredByRoar;
   mFlagAlwaysRenderSelectionDecal = pBase->mFlagAlwaysRenderSelectionDecal;
   mFlagOnlyShowBobbleHeadWhenContained = pBase->mFlagOnlyShowBobbleHeadWhenContained;
   mFlagDiesWhenFrozen = pBase->mFlagDiesWhenFrozen;
   mDazeResist = pBase->mDazeResist;
   mFlagFlyingFlood = pBase->mFlagFlyingFlood;
   mFlagFlood = pBase->mFlagFlood;
   mFlagForerunner = pBase->mFlagForerunner;
   mFlagRebel = pBase->mFlagRebel;
   mFlagCanAttackWhileMoving = pBase->mFlagCanAttackWhileMoving;
   mFlagNoPlatoonMerge = pBase->mFlagNoPlatoonMerge;
   mFlagObjectProtoSquad = pBase->mFlagObjectProtoSquad;
   mFlagMergedProtoSquad = pBase->mFlagMergedProtoSquad;
   mFlagAlwaysShowHPBar = pBase->mFlagAlwaysShowHPBar;
}

//==============================================================================
// BProtoSquadStatic::~BProtoSquadStatic
//==============================================================================
BProtoSquadStatic::~BProtoSquadStatic()
{
}

//==============================================================================
// BProtoSquad::BProtoSquad
//==============================================================================
BProtoSquad::BProtoSquad(long id) :
   mID(id),
   mBaseType(id),
   mBuildPoints(0.0f),
   mMaxHP(0.0f),
   mMaxSP(0.0f),
   mMaxAmmo(0.0f),
   mLevel(0),
   mTechLevel(0),
   mDisplayNameIndex(-1),
   mCircleMenuIconID(-1),
   mAltCircleMenuIconID(-1),
   mHPBar(-1),
   mCost(),
   mpStaticData(NULL),
   mpPlayer(NULL),
   mMergeIntoProtoSquadIDs(),
   mMergeFromProtoSquadIDs(),
   mpOverrideUnitNodeArray(NULL)
{
   // Dynamic flags
   mFlagOwnStaticData=false;
   mFlagAvailable=false;
   mFlagForbid=false;
   mFlagOneTimeSpawnUsed=false;
   mFlagUniqueInstance=false;
   mFlagKBAware = false;
}

//==============================================================================
// BProtoSquad::BProtoSquad
//==============================================================================
BProtoSquad::BProtoSquad(const BProtoSquad* pBase, BPlayer* pPlayer)
{
   // Dynamic data
   mID=pBase->mID;      
   mBaseType=pBase->mBaseType;
   mBuildPoints=pBase->mBuildPoints;
   mMaxHP=pBase->mMaxHP;
   mMaxSP=pBase->mMaxSP;
   mMaxAmmo=pBase->mMaxAmmo;
   mLevel=pBase->mLevel;
   mTechLevel=pBase->mTechLevel;
   mDisplayNameIndex=pBase->mDisplayNameIndex;
   mCircleMenuIconID=pBase->mCircleMenuIconID;
   mAltCircleMenuIconID=pBase->mAltCircleMenuIconID;
   mHPBar=pBase->mHPBar;
   mCost=pBase->mCost;
   mAttackRatingDPS=pBase->mAttackRatingDPS;
   mpPlayer = pPlayer;
   mpOverrideUnitNodeArray = NULL;

   // Copy Flags
   copyFlags(pBase);

   // Static data
   mpStaticData=pBase->mpStaticData;
   mFlagOwnStaticData = false;

   // Merge data
   mMergeIntoProtoSquadIDs=pBase->mMergeIntoProtoSquadIDs;
   mMergeFromProtoSquadIDs=pBase->mMergeFromProtoSquadIDs;
}

//==============================================================================
//==============================================================================
BProtoSquad::BProtoSquad(long id, const BProtoSquad* pBase, const BProtoSquad* pMerge)
{
   // Dynamic data
   mID=id;
   mBaseType=pBase->mBaseType;
   mBuildPoints=pBase->mBuildPoints;
   mMaxHP=pBase->mMaxHP;
   mMaxSP=pBase->mMaxSP;
   mMaxAmmo=pBase->mMaxAmmo;
   mLevel=pBase->mLevel;
   mTechLevel=pBase->mTechLevel;
   mDisplayNameIndex=pBase->mDisplayNameIndex;
   mCircleMenuIconID=pBase->mCircleMenuIconID;
   mAltCircleMenuIconID=pBase->mAltCircleMenuIconID;
   mHPBar=pBase->mHPBar;
   mCost=pBase->mCost;
   mAttackRatingDPS=pBase->mAttackRatingDPS;
   mpPlayer = NULL;
   mpOverrideUnitNodeArray = NULL;

   copyFlags(pBase);

   // Copy static data
   mpStaticData=new BProtoSquadStatic(pBase->mpStaticData);
   mFlagOwnStaticData = true;
   mpStaticData->mID = mID;

   // Make the name unique
   mpStaticData->mName.format("merged_%s_%s", pBase->getName().getPtr(), pMerge->getName().getPtr());
   mpStaticData->mFlagMergedProtoSquad = true;

   // Merge data
   mMergeIntoProtoSquadIDs=pBase->mMergeIntoProtoSquadIDs;
   mMergeFromProtoSquadIDs=pBase->mMergeFromProtoSquadIDs;

   // Merge
   if (pMerge)
   {
      for (int i = 0; i < pMerge->getNumberUnitNodes(); i++)
      {
         const BProtoSquadUnitNode& srcNode = pMerge->getUnitNode(i);
         const BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(srcNode.mUnitType);
         if (!pProtoObject)
            continue;

         // Search for matching unit node type to add to
         for (int j = 0; j < getNumberUnitNodes(); j++)
         {
            BProtoSquadUnitNode& destNode = const_cast<BProtoSquadUnitNode&>(getUnitNode(i));
            if (srcNode.mUnitType == destNode.mUnitType)
            {
               // Add new units
               destNode.mUnitCount += srcNode.mUnitCount;
               break;
            }
         }

         // If matching unit node not found, add a new one
         mpStaticData->mUnitNodes.add(srcNode);

         // Update data based on unit count
         mpStaticData->mSquadSize += srcNode.mUnitCount;
         mMaxHP += (pProtoObject->getHitpoints() * srcNode.mUnitCount);
         mMaxSP += (pProtoObject->getShieldpoints() * srcNode.mUnitCount);
         mMaxAmmo += (pProtoObject->getMaxAmmo() * srcNode.mUnitCount);
         mpStaticData->mAttackGradeDPS += (pProtoObject->getAttackGradeDPS() * srcNode.mUnitCount);
      }
   }
}

//==============================================================================
// BProtoSquad::~BProtoSquad
//==============================================================================
BProtoSquad::~BProtoSquad()
{
   if(getFlagOwnStaticData() && mpStaticData)
   {
      delete mpStaticData;
      mpStaticData=NULL;
      mFlagOwnStaticData = false;
   }
   if (mpOverrideUnitNodeArray)
   {
      delete mpOverrideUnitNodeArray;
      mpOverrideUnitNodeArray = NULL;
   }
}

//==============================================================================
// BProtoSquad::copyFlags
//==============================================================================
void BProtoSquad::copyFlags(const BProtoSquad* pBase)
{
   // Dynamic flags
   mFlagAvailable=pBase->mFlagAvailable;
   mFlagForbid=pBase->mFlagForbid;
   mFlagOneTimeSpawnUsed=pBase->mFlagOneTimeSpawnUsed;
   mFlagUniqueInstance=pBase->mFlagUniqueInstance;
   mFlagKBAware = pBase->mFlagKBAware;
}

//==============================================================================
// BProtoSquad::transform
//==============================================================================
void BProtoSquad::transform(const BProtoSquad* pBase)
{
   if(mpStaticData)
   {
      if(getFlagOwnStaticData())
      {
         delete mpStaticData;
         mFlagOwnStaticData = false;
      }
      mpStaticData=NULL;
   }

   // Dynamic data

   // Don't copy mID/mBaseType... Leave it set to the original value.
   // Don't copy merge data... Leave it set to the original value.

   //--Flags
   mFlagOwnStaticData=pBase->mFlagOwnStaticData;
   
   copyFlags(pBase);

   mBuildPoints=pBase->mBuildPoints;
   mMaxHP=pBase->mMaxHP;
   mMaxSP=pBase->mMaxSP;
   mMaxAmmo=pBase->mMaxAmmo;
   mLevel=pBase->mLevel;
   mTechLevel=pBase->mTechLevel;
   mDisplayNameIndex=pBase->mDisplayNameIndex;
   mCircleMenuIconID=pBase->mCircleMenuIconID;
   mAltCircleMenuIconID=pBase->mAltCircleMenuIconID;
   mHPBar=pBase->mHPBar;
   mCost=pBase->mCost;
   mAttackRatingDPS=pBase->mAttackRatingDPS;

   // Static data
   mpStaticData=pBase->mpStaticData;
   mFlagOwnStaticData = false;
}

//==============================================================================
// BProtoSquad::load
//==============================================================================
bool BProtoSquad::load(BXMLNode root)
{
   mpStaticData=new BProtoSquadStatic();
   if(!mpStaticData)
      return false;
   mFlagOwnStaticData = true;
   mpStaticData->mID = mID;

   BSimString attribute;

   // Read name
   root.getAttribValueAsString("name", mpStaticData->mName);

   //Read formation type.
   root.getAttribValueAsString("formationType", attribute);
   if (attribute == "flock")
      mpStaticData->mFormationType=BFormation2::eTypeFlock;
   else if (attribute == "line")
      mpStaticData->mFormationType=BFormation2::eTypeLine;
   else if (attribute == "gaggle")
      mpStaticData->mFormationType=BFormation2::eTypeGaggle;
   else
      mpStaticData->mFormationType=BFormation2::eTypeStandard;

   // read the DBID
   root.getAttribValueAsLong("dbid", mpStaticData->mDBID);

   // Iterate through children
   long numNodes = root.getNumberChildren();
   for (long i = 0; i < numNodes; i++)
   {
      BXMLNode node(root.getChild(i));
      const BPackedString name(node.getName());

      BSimString tempStr;
      if(name=="DisplayNameID")
      {
         long id;
         if(node.getTextAsLong(id))
            mDisplayNameIndex=gDatabase.getLocStringIndex(id);
      }
      else if(name=="RolloverTextID")
      {
         long id;
         if(node.getTextAsLong(id))
           mpStaticData->mRolloverTextIndex=gDatabase.getLocStringIndex(id);
      }
      else if(name=="StatsNameID")
      {
         long id;
         if(node.getTextAsLong(id))
            mpStaticData->mStatsNameIndex=gDatabase.getLocStringIndex(id);
      }
      else if(name=="PrereqTextID")
      {
         long id;
         if(node.getTextAsLong(id))
            mpStaticData->mPrereqTextIndex=gDatabase.getLocStringIndex(id);
      }
      else if(name=="RoleTextID")
      {
         long id;
         if(node.getTextAsLong(id))
            mpStaticData->mRoleTextIndex=gDatabase.getLocStringIndex(id);
      }
      else if (name == "PortraitIcon")
         node.getTextAsString(mpStaticData->mIcon);
      else if (name == "AltIcon")
         node.getTextAsString(mpStaticData->mAltIcon);
      else if (name == "BuildPoints")
         node.getTextAsFloat(mBuildPoints);
      else if (name == "Cost")
         mCost.load(node);
      else if (name == "Stance")
         mpStaticData->mStance = lookupStanceType(node.getTextPtr(tempStr));
      else if(name=="TrainAnim")
         mpStaticData->mTrainAnimType = gVisualManager.getAnimType(node.getTextPtr(tempStr));
      else if(name=="Selection")
      {
         int childCount = node.getNumberChildren();
         for (int i = 0; i < childCount; ++i)
         {
            BXMLNode childNode(node.getChild(i));
            const BPackedString childName(childNode.getName());
            if (childName=="RadiusX")
               childNode.getTextAsFloat(mpStaticData->mSelectionRadiusX);
            else if (childName =="RadiusZ")
               childNode.getTextAsFloat(mpStaticData->mSelectionRadiusZ);
            else if (childName =="YOffset")
               childNode.getTextAsFloat(mpStaticData->mSelectionYOffset);
            else if (childName =="ZOffset")
               childNode.getTextAsFloat(mpStaticData->mSelectionZOffset);
            else if (childName =="ConformToTerrain")
               childNode.getTextAsBool(mpStaticData->mSelectionConformToTerrain);
            else if (childName =="AllowOrientation")
               childNode.getTextAsBool(mpStaticData->mSelectionAllowOrientation);
         }
      }      
      else if (name =="HPBar")
      {
         mHPBar = gDatabase.getProtoHPBarID(node.getTextPtr(tempStr));         
         if (mHPBar != -1)
            mpStaticData->mFlagHasHPBar = true;

         node.getAttribValueAsFloat("sizeX", mpStaticData->mHPBarSizeX);
         node.getAttribValueAsFloat("sizeY", mpStaticData->mHPBarSizeY);
         node.getAttribValueAsVector("offset", mpStaticData->mHPBarOffset);
      }
      else if (name =="VeterancyBar")
      {
         mpStaticData->mVeterancyBarID = gDatabase.getProtoVeterancyBarID(node.getTextPtr(tempStr)); 

         BSimString temp;         
         node.getAttribValueAsString("Centered", temp);
         mpStaticData->mVeterancyCenteredBarID = gDatabase.getProtoVeterancyBarID(BStrConv::toA(temp));

         node.getAttribValueAsFloat("sizeX",    mpStaticData->mVeterancyBarSizeX);
         node.getAttribValueAsFloat("sizeY",    mpStaticData->mVeterancyBarSizeY);
         node.getAttribValueAsFloat("offsetX", mpStaticData->mVeterancyBarOffsetX);
         node.getAttribValueAsFloat("offsetY", mpStaticData->mVeterancyBarOffsetY);
      }
      else if (name =="AbilityRecoveryBar")
      {
         mpStaticData->mAbilityRecoveryBarID = gDatabase.getProtoPieProgressBarID(node.getTextPtr(tempStr));

         BSimString temp;         
         node.getAttribValueAsString("Centered", temp);
         mpStaticData->mAbilityRecoveryCenteredBarID = gDatabase.getProtoPieProgressBarID(BStrConv::toA(temp));

         node.getAttribValueAsFloat("sizeX",    mpStaticData->mAbilityRecoveryBarSizeX);
         node.getAttribValueAsFloat("sizeY",    mpStaticData->mAbilityRecoveryBarSizeY);
         node.getAttribValueAsFloat("offsetX", mpStaticData->mAbilityRecoveryBarOffsetX);
         node.getAttribValueAsFloat("offsetY", mpStaticData->mAbilityRecoveryBarOffsetY);
      }
      else if (name =="BobbleHead")
      {
         mpStaticData->mBobbleHeadID = gDatabase.getProtoBobbleHeadID(node.getTextPtr(tempStr));         
      }
      else if (name =="BuildingStrengthDisplay")
      {
         mpStaticData->mBuildingStrengthID = gDatabase.getProtoBuildingStrengthID(node.getTextPtr(tempStr));         
      }
      else if (name =="CryoPoints")
      {
         node.getTextAsFloat(mpStaticData->mCryoPoints);
      }
      else if (name == "DazeResist")
      {
         node.getTextAsFloat(mpStaticData->mDazeResist);
      }
      else if(name=="Birth")
      {
         BSimString birthType;
         node.getText(birthType);
         
         if (birthType == "Trained")
            mpStaticData->mBirthType = BProtoSquadStatic::cTrainedBirthType;
         else if (birthType == "FlyIn")
            mpStaticData->mBirthType = BProtoSquadStatic::cFlyInBirthType;

         if (mpStaticData->mBirthType != -1)
         {
            node.getAttribValueAsString("spawnPoint", mpStaticData->mBirthBone);
            node.getAttribValueAsString("endPoint", mpStaticData->mBirthEndBone);
            if (node.getAttribValueAsString("anim0", attribute))
               mpStaticData->mBirthAnim[0] = gVisualManager.getAnimType(attribute);
            if (node.getAttribValueAsString("anim1", attribute))
               mpStaticData->mBirthAnim[1] = gVisualManager.getAnimType(attribute);
            if (node.getAttribValueAsString("anim2", attribute))
               mpStaticData->mBirthAnim[2] = gVisualManager.getAnimType(attribute);
            if (node.getAttribValueAsString("anim3", attribute))
               mpStaticData->mBirthAnim[3] = gVisualManager.getAnimType(attribute);
            if (node.getAttribValueAsString("trainerAnim", attribute))
               mpStaticData->mBirthTrainerAnim = gVisualManager.getAnimType(attribute);
		 }
	  }
      else if (name == "Units")
      {
         // Read units
         long numUnitNodes = node.getNumberChildren();
         mpStaticData->mUnitNodes.reserve(numUnitNodes);
         for (long j = 0; j < numUnitNodes; j++)
         {
            BXMLNode childNode(node.getChild(j));
            const BPackedString name(childNode.getName());

            if (name == "Unit")
            {
               BProtoSquadUnitNode unitNode;

               // Get unit type
               BSimString tempStr;
               unitNode.mUnitType=gDatabase.getProtoObject(childNode.getTextPtr(tempStr));
               if (gDatabase.isValidProtoObject(unitNode.mUnitType))
               {
                  const BProtoObject* pProtoObject = gDatabase.getGenericProtoObject(unitNode.mUnitType);

                  if (mpStaticData->mDamageType == -1)
                     mpStaticData->mDamageType = pProtoObject->getDamageType();

                  // Get unit count
                  childNode.getAttribValueAsLong("count", unitNode.mUnitCount);
                  mpStaticData->mSquadSize += unitNode.mUnitCount;

                  mMaxHP += (pProtoObject->getHitpoints() * unitNode.mUnitCount);
                  mMaxSP += (pProtoObject->getShieldpoints() * unitNode.mUnitCount);
                  mMaxAmmo += (pProtoObject->getMaxAmmo() * unitNode.mUnitCount);

                  // Get unit role
                  childNode.getAttribValueAsString("role", attribute);
                  if (attribute == "normal")
                     unitNode.mUnitRole = cRoleNormal;
                  else if (attribute == "leader")
                     unitNode.mUnitRole = cRoleLeader;
                  else if (attribute == "support")
                     unitNode.mUnitRole = cRoleSupport;

                  // Add unit node
                  mpStaticData->mUnitNodes.add(unitNode);

                  // Update attack grade DPS
                  mpStaticData->mAttackGradeDPS += (pProtoObject->getAttackGradeDPS() * unitNode.mUnitCount);
               }
            }
         }
      }
      else if (name == "SubSelectSort")
         node.getTextAsInt(mpStaticData->mSubSelectSort);
      else if (name == "TurnRadius")
      {
         if (!node.getAttribValueAsFloat("min", mpStaticData->mTurnRadiusMin))
            mpStaticData->mTurnRadiusMin = -1.0f;
         if (!node.getAttribValueAsFloat("max", mpStaticData->mTurnRadiusMax))
            mpStaticData->mTurnRadiusMax = -1.0f;
      }
      else if (name == "LeashDistance")
         node.getTextAsFloat(mpStaticData->mLeashDistance);
      else if (name == "LeashDeadzone")
         node.getTextAsFloat(mpStaticData->mLeashDeadzone);
      else if (name == "LeashRecallDelay")
         node.getTextAsDWORD(mpStaticData->mLeashRecallDelay);
      else if (name == "AggroDistance")
         node.getTextAsFloat(mpStaticData->mAggroDistance);
      else if (name == "MinimapScale")
         node.getTextAsFloat(mpStaticData->mMinimapScale);
      else if (name == "Flag")
      {
         BPackedString nodeText(node.getTextPtr(tempStr));
         if (nodeText == "Repairable")
            mpStaticData->mFlagRepairable = true;
         else if(nodeText == "Chatter")
            mpStaticData->mFlagChatter = true;
         else if (nodeText == "CreateAudioReactions")
            mpStaticData->mFlagCreateAudioReactions = true;
         else if (nodeText == "ForceToGaiaPlayer")
            mpStaticData->mFlagForceToGaiaPlayer = true;
         else if (nodeText == "InstantTrainWithRecharge")
            mpStaticData->mFlagInstantTrainWithRecharge = true;
         else if (nodeText == "AlwaysAttackReviveUnits")
            mpStaticData->mFlagAlwaysAttackReviveUnits = true;
         else if (nodeText == "OneTimeSpawnUsed")
            mFlagOneTimeSpawnUsed = true;
         else if (nodeText == "ScaredByRoar")
            mpStaticData->mFlagScaredByRoar = true;
         else if (nodeText == "AlwaysRenderSelectionDecal")
            mpStaticData->mFlagAlwaysRenderSelectionDecal = true;
         else if (nodeText == "OnlyShowBobbleHeadWhenContained")
            mpStaticData->mFlagOnlyShowBobbleHeadWhenContained = true;         
         else if (nodeText == "KBAware")
            mFlagKBAware = true;
         else if (nodeText == "NoPlatoonMerge")
            mpStaticData->mFlagNoPlatoonMerge = true;
         else if (nodeText == "DiesWhenFrozen")
            mpStaticData->mFlagDiesWhenFrozen = true;
         else if (nodeText == "FlyingFlood")
            mpStaticData->mFlagFlyingFlood = true;
         else if (nodeText == "Flood")
            mpStaticData->mFlagFlood = true;
         else if (nodeText == "Forerunner")
            mpStaticData->mFlagForerunner = true;
         else if (nodeText == "Rebel")
            mpStaticData->mFlagRebel = true;
         else if (nodeText == "AlwaysShowHPBar")
            mpStaticData->mFlagAlwaysShowHPBar = true;
      }
      else if (name == "Level")
         node.getTextAsInt(mLevel);
      else if (name == "TechLevel")
         node.getTextAsInt(mTechLevel);
      else if (name == "Sound")
      {
         BPackedString nodeText(node.getTextPtr(tempStr));
         BCueIndex cueIndex = gSoundManager.getCueIndex(nodeText.getPtr());         

         if(cueIndex == cInvalidCueIndex)
         {
            BSimString str;
            str.format("Squads.xml: Cannot find sound cue: %s", nodeText.getPtr());
            gConsole.output(cMsgWarning, str.getPtr());
         }

         if (cueIndex != cInvalidCueIndex)
         {
            BSimString typeName;
            if (node.getAttribValue("Type", &typeName))
            {
               BSquadSoundType soundType = getSoundType(typeName);
               if (soundType != cSquadSoundNone)
               {
                  BProtoSound sound;

                  //-- Load the cue Index
                  sound.mSoundCue = cueIndex;            
                  sound.mSquadSoundType = soundType;

                  //-- Load squad specific info
                  BSimString squadType;
                  if(node.getAttribValue("Squad", &squadType))
                  {
                     long squadID = gDatabase.getProtoSquad(squadType);                     
                     BASSERT(squadID != cInvalidProtoID);
                     sound.mSquadID = squadID;
                  }

                  BSimString worldIDStr;
                  if(node.getAttribValue("World", &worldIDStr))
                  {
                     uint8 worldID = gDatabase.getWorldID(worldIDStr);
                     BASSERT(worldID != cWorldNone);
                     sound.mWorldID = worldID;
                  }

                  bool castingUnitOnly = false;
                  if (node.getAttribValueAsBool("CastingUnitOnly", castingUnitOnly))
                  {
                     sound.mCastingUnitOnly = castingUnitOnly;
                  }

                  mpStaticData->mUsedSoundTypes = mpStaticData->mUsedSoundTypes | soundType;

                  //-- Add the sound to the list
                  mpStaticData->mSounds.add(sound);                  
               }
            }
         }
      }
      else if (name == "RecoveringEffect")
         mpStaticData->mRecoveringEffect = gDatabase.getProtoObject(node.getTextPtr(tempStr));
      else if (name == "CanAttackWhileMoving")
      {
         bool value = true;
         node.getTextAsBool(value);
         mpStaticData->mFlagCanAttackWhileMoving = value;
      }
   }

   updateAttackRatings();
   calcLevels();

   return true;
}

//==============================================================================
// BProtoSquad::initFromProtoObject
//==============================================================================
bool BProtoSquad::initFromProtoObject(const BProtoObject* pProtoObject, int protoSquadID)
{
   mID = protoSquadID;
   mBaseType=mID;

   mBuildPoints = pProtoObject->getBuildPoints();
   mMaxHP = pProtoObject->getHitpoints();
   mMaxSP = pProtoObject->getShieldpoints();
   mMaxAmmo = pProtoObject->getMaxAmmo();
   mLevel = pProtoObject->getLevel();
   mTechLevel = 0;
   mDisplayNameIndex = pProtoObject->getDisplayNameIndex();
   mCircleMenuIconID = pProtoObject->getCircleMenuIconID();

   const BCost* pCost = pProtoObject->getCost();
   if (pCost)
      mCost = *pCost;

   uint numDamageTypes = gDatabase.getNumberAttackRatingDamageTypes();
   mAttackRatingDPS.resize(numDamageTypes);
   for (uint i=0; i<numDamageTypes; i++)
      mAttackRatingDPS[i]=pProtoObject->getAttackRatingDPS(gDatabase.getAttackRatingDamageType(i));

   mpPlayer = NULL;

   mFlagAvailable = pProtoObject->getFlagAvailable();
   mFlagForbid = pProtoObject->getFlagForbid();
   mFlagUniqueInstance = pProtoObject->getFlagUniqueInstance();
   mFlagKBAware = pProtoObject->getFlagKBAware();

   mpStaticData=new BProtoSquadStatic();
   if(!mpStaticData)
      return false;
   mFlagOwnStaticData = true;

   mpStaticData->mFormationType=BFormation2::eTypeStandard;
   mpStaticData->mProtoObjectID = pProtoObject->getID();
   mpStaticData->mDBID = pProtoObject->getDBID();
   mpStaticData->mStatsNameIndex = pProtoObject->getStatsTextIndex();
   mpStaticData->mRolloverTextIndex = pProtoObject->getRolloverTextIndex();
   mpStaticData->mPrereqTextIndex = pProtoObject->getPrereqTextIndex();
   mpStaticData->mRoleTextIndex = pProtoObject->getRoleTextIndex();
   mpStaticData->mName = pProtoObject->getName();
   mpStaticData->mSquadSize = 1;
   mpStaticData->mIcon = pProtoObject->getIcon();
   mpStaticData->mTrainAnimType = pProtoObject->getTrainAnimType();
   mpStaticData->mSelectionRadiusX = pProtoObject->getSelectedRadiusX();
   mpStaticData->mSelectionRadiusZ = pProtoObject->getSelectedRadiusZ();
   mpStaticData->mSelectionYOffset = 0.0f;
   mpStaticData->mSelectionZOffset = 0.0f;
   mpStaticData->mSelectionConformToTerrain = !pProtoObject->getFlagFlying();
   mpStaticData->mSelectionDecalID = pProtoObject->getSelectionDecalID();
   mpStaticData->mClumpSelectionDecalID = pProtoObject->getClumpSelectionDecalID();
   mpStaticData->mClumpSelectionDecalMaskID = pProtoObject->getClumpSelectionDecalMaskID();
   mpStaticData->mStaticDecalID = pProtoObject->getStaticDecalID();

   mpStaticData->mUnitSelectionIconID = pProtoObject->getUnitSelectionIconID();
   mHPBar = pProtoObject->getHPBarID();
   mpStaticData->mHPBarSizeX = pProtoObject->getHPBarSizeX();
   mpStaticData->mHPBarSizeY = pProtoObject->getHPBarSizeY();
   mpStaticData->mHPBarOffset = pProtoObject->getHPBarOffset();
   if (mpStaticData->mHPBarOffset==cOriginVector)
      mpStaticData->mHPBarOffset.y = pProtoObject->getObstructionRadiusY()*2.0f;
   mpStaticData->mBuildingStrengthID = pProtoObject->getBuildingStrengthID();
   mpStaticData->mSubSelectSort = pProtoObject->getSubSelectSort();
   mpStaticData->mTurnRadiusMin = -1.0f;
   mpStaticData->mTurnRadiusMax = -1.0f;
   mpStaticData->mAttackGradeDPS = pProtoObject->getAttackGradeDPS();
   mpStaticData->mDamageType = pProtoObject->getDamageType();
   mpStaticData->mRecoveringEffect = pProtoObject->getRecoveringEffect(); 

   mpStaticData->mFlagHasHPBar  = pProtoObject->getFlagHasHPBar();
   mpStaticData->mFlagNoTieToGround = pProtoObject->getFlagNoTieToGround();
   mpStaticData->mFlagRepairable = pProtoObject->getFlagRepairable();
   mpStaticData->mFlagForceToGaiaPlayer = pProtoObject->getFlagForceToGaiaPlayer();
   mpStaticData->mFlagInstantTrainWithRecharge = pProtoObject->getFlagInstantTrainWithRecharge();
   mpStaticData->mSelectionConformToTerrain = !pProtoObject->getFlagSelectionDontConformToTerrain();
   mpStaticData->mSelectionAllowOrientation = true;
   mpStaticData->mFlagObjectProtoSquad = true;

   BProtoSquadUnitNode unitNode;
   unitNode.mUnitType = mpStaticData->mProtoObjectID;
   unitNode.mUnitCount = 1;
   unitNode.mUnitRole = cRoleNormal;
   mpStaticData->mUnitNodes.add(unitNode);

   return true;
}

//==============================================================================
// BProtoSquad::getPops
//==============================================================================
void BProtoSquad::getPops(BPopArray& pops) const
{
   long unitNodeCount=getNumberUnitNodes();
   for(long i=0; i<unitNodeCount; i++)
   {
      const BProtoSquadUnitNode& unitNode=getUnitNode(i);
      const BProtoObject* pProtoObject=(mpPlayer ? mpPlayer->getProtoObject(unitNode.mUnitType) : gDatabase.getGenericProtoObject(unitNode.mUnitType));
      long unitPopCount=pProtoObject->getNumberPops();
      for(long j=0; j<unitPopCount; j++)
      {
         BPop pop=pProtoObject->getPop(j);
         pop.mCount = (pop.mCount * unitNode.mUnitCount);
         bool added=false;
         for(long k=0; k<pops.getNumber(); k++)
         {
            if(pops[k].mID==pop.mID)
            {
               pops[k].mCount = (pops[k].mCount + pop.mCount);
               added=true;
            }
         }
         if(!added)
            pops.add(pop);
      }
   }
}

//==============================================================================
// BProtoSquad::getDisplayName
//==============================================================================
void BProtoSquad::getDisplayName(BUString& string) const
{
   if(mDisplayNameIndex==-1)
      string=mpStaticData->mName.getPtr();  // AJL FIXME - Need to return empty string or other value to indicate missing string
   else
      string=gDatabase.getLocStringFromIndex(mDisplayNameIndex);
}

//==============================================================================
// BProtoSquad::getStatsName
//==============================================================================
void BProtoSquad::getStatsName(BUString& string) const
{
   if(mpStaticData->mStatsNameIndex==-1)
      getDisplayName(string);
   else
      string=gDatabase.getLocStringFromIndex(mpStaticData->mStatsNameIndex);
}

//==============================================================================
// BProtoSquad::getRolloverText
//==============================================================================
void BProtoSquad::getRolloverText(BUString& string) const
{
   string=gDatabase.getLocStringFromIndex(mpStaticData->mRolloverTextIndex);
}

//==============================================================================
// BProtoSquad::getPrereqText
//==============================================================================
void BProtoSquad::getPrereqText(BUString& string) const
{
   string=gDatabase.getLocStringFromIndex(mpStaticData->mPrereqTextIndex);
}

//==============================================================================
// BProtoSquad::getRoleText
//==============================================================================
void BProtoSquad::getRoleText(BUString& string) const
{
   string=gDatabase.getLocStringFromIndex(mpStaticData->mRoleTextIndex);
}

//==============================================================================
//==============================================================================
float BProtoSquad::getBuildPoints() const
{
   if (!mpPlayer || mpStaticData->mProtoObjectID==-1)
      return mBuildPoints;
   else
      return mpPlayer->getProtoObject(mpStaticData->mProtoObjectID)->getBuildPoints();
}

//==============================================================================
//==============================================================================
const BCost* BProtoSquad::getCost() const
{
   if (!mpPlayer || mpStaticData->mProtoObjectID==-1)
      return &mCost;
   else
      return mpPlayer->getProtoObject(mpStaticData->mProtoObjectID)->getCost();
}

//==============================================================================
//==============================================================================
void BProtoSquad::setBuildPoints(float val)
{ 
   BASSERT(mpStaticData->mProtoObjectID==-1);
   mBuildPoints=val;
}


//==============================================================================
//==============================================================================
void BProtoSquad::setCost(BCost* pVal)
{
   BASSERT(mpStaticData->mProtoObjectID==-1);
   mCost=*pVal;
}

//==============================================================================
//==============================================================================
void BProtoSquad::updateAttackRatings()
{
   BASSERT(mpStaticData->mProtoObjectID==-1);
   bool doInit = true;
   uint numDamageTypes = gDatabase.getNumberAttackRatingDamageTypes();
   long numUnitNodes = getNumberUnitNodes();
   for (long i = 0; i < numUnitNodes; i++)
   {
      const BProtoSquadUnitNode& unitNode = getUnitNode(i);
      const BProtoObject* pProtoObject=(mpPlayer ? mpPlayer->getProtoObject(unitNode.mUnitType) : gDatabase.getGenericProtoObject(unitNode.mUnitType));
      const BTactic* pTactic = pProtoObject->getTactic();
      if (pTactic && pTactic->canAttack())
      {
         if (doInit)
         {
            mAttackRatingDPS.resize(numDamageTypes);
            for (uint j=0; j<numDamageTypes; j++)
               mAttackRatingDPS[j]=0.0f;
            doInit = false;
         }
         float unitCount = (float)unitNode.mUnitCount;
         for (uint j=0; j<numDamageTypes; j++)
         {
            float dps = pTactic->getAttackRatingDPS(gDatabase.getAttackRatingDamageType(j)) * unitCount;
            mAttackRatingDPS[j] += dps;
         }
      }
   }
}

//==============================================================================
//==============================================================================
bool BProtoSquad::getHasAttackRatings() const
{ 
   if (!mpPlayer || mpStaticData->mProtoObjectID==-1)
      return (mAttackRatingDPS.getSize() > 0);
   else
      return mpPlayer->getProtoObject(mpStaticData->mProtoObjectID)->getHasAttackRatings();
}

//==============================================================================
//==============================================================================
float BProtoSquad::getAttackRatingDPS(BDamageTypeID damageType) const
{
   if (!mpPlayer || mpStaticData->mProtoObjectID==-1)
   {
      if (mAttackRatingDPS.getSize() > 0)
      {
         uint index;
         if (gDatabase.getAttackRatingIndex(damageType, index))
            return mAttackRatingDPS[index];
      }
      return 0.0f;
   }
   else
      return mpPlayer->getProtoObject(mpStaticData->mProtoObjectID)->getAttackRatingDPS(damageType);
}

//==============================================================================
//==============================================================================
float BProtoSquad::getAttackRating(BDamageTypeID damageType) const
{
   if (!mpPlayer || mpStaticData->mProtoObjectID==-1)
   {
      if (mAttackRatingDPS.getSize() > 0)
      {
         uint index;
         if (gDatabase.getAttackRatingIndex(damageType, index))
            return gDatabase.getAttackRating(mAttackRatingDPS[index]);
      }
      return 0.0f;
   }
   else
      return mpPlayer->getProtoObject(mpStaticData->mProtoObjectID)->getAttackRating(damageType);
}

//==============================================================================
//==============================================================================
float BProtoSquad::getAttackRating() const
{
   if (!mpPlayer || mpStaticData->mProtoObjectID==-1)
   {
      uint numAttackRatingDPS = mAttackRatingDPS.getSize();
      if (numAttackRatingDPS == 0)
         return (0.0f);
      float totalAttackRatingDPS = 0.0f;
      for (uint i=0; i<numAttackRatingDPS; i++)
         totalAttackRatingDPS += mAttackRatingDPS[i];
      return (totalAttackRatingDPS / static_cast<float>(numAttackRatingDPS));
   }
   else
      return mpPlayer->getProtoObject(mpStaticData->mProtoObjectID)->getAttackRating();
}

//==============================================================================
//==============================================================================
float BProtoSquad::getDefenseRating() const
{
   if (!mpPlayer || mpStaticData->mProtoObjectID==-1)
   {
      float hp=0.0f;
      long numUnitNodes = getNumberUnitNodes();
      for (long i = 0; i < numUnitNodes; i++)
      {
         const BProtoSquadUnitNode& unitNode = getUnitNode(i);
         const BProtoObject* pProtoObject=(mpPlayer ? mpPlayer->getProtoObject(unitNode.mUnitType) : gDatabase.getGenericProtoObject(unitNode.mUnitType));
         hp += (pProtoObject->getHitpoints() + pProtoObject->getShieldpoints()) * unitNode.mUnitCount;
      }
      return gDatabase.getDefenseRating(hp);
   }
   else
      return mpPlayer->getProtoObject(mpStaticData->mProtoObjectID)->getDefenseRating();
}

//==============================================================================
//==============================================================================
float BProtoSquad::getStrength() const
{
   if (!mpPlayer || mpStaticData->mProtoObjectID==-1)
      return (getAttackRating() * getDefenseRating());
   else
      return mpPlayer->getProtoObject(mpStaticData->mProtoObjectID)->getStrength();
}

//==============================================================================
//==============================================================================
uint BProtoSquad::getAttackGrade(BDamageTypeID damageType) const
{
   return gDatabase.getAttackGrade(mpStaticData->mAttackGradeDPS, getAttackRatingDPS(damageType));
}

//==============================================================================
//==============================================================================
float BProtoSquad::getAttackGradeRatio(BDamageTypeID damageType) const
{
   return gDatabase.getAttackGradeRatio(mpStaticData->mAttackGradeDPS, getAttackRatingDPS(damageType));
}

//==============================================================================
//==============================================================================
float BProtoSquad::getCombatValue() const
{
   float totalCombatValue = 0.0f;
   long numUnitNodes = getNumberUnitNodes();
   for (long i=0; i<numUnitNodes; i++)
   {
      const BProtoSquadUnitNode& node = getUnitNode(i);
      const BProtoObject* pProtoObject = mpPlayer ? mpPlayer->getProtoObject(node.mUnitType) : gDatabase.getGenericProtoObject(node.mUnitType);
      BASSERT(pProtoObject);
      totalCombatValue += (pProtoObject->getCombatValue() * node.mUnitCount);
   }
   return (totalCombatValue);
}

//==============================================================================
//==============================================================================
BCueIndex BProtoSquad::getSound(BSquadSoundType soundType, long squadID, uint worldID, BEntityID callingSquadID, BEntityID castingSquadID) const
{
   //-- Must match soundType and SquadID
   //-- Must match worldID if specified in data, otherwise will use a sound that has cWorldNone.

   if((mpStaticData->mUsedSoundTypes & soundType) == 0x00000000)
      return cInvalidCueIndex;

   BCueIndex fallBack = cInvalidCueIndex;

   uint numSounds = mpStaticData->mSounds.getSize();
   for(uint i=0; i < numSounds; i++)
   {
      if(mpStaticData->mSounds[i].mSoundType == soundType)
      {
         // Skip any calling squad that isn't the casting squad if this is meant for casting unit only
         if (mpStaticData->mSounds[i].mCastingUnitOnly)
         {
            if (callingSquadID != castingSquadID)
               continue;
         }
         // Skip any calling squad that is the casting squad if this isn't meant for casting unit only and the casting squad is specified
         else if ((castingSquadID != cInvalidObjectID) && (castingSquadID == callingSquadID))
            continue;

         if(mpStaticData->mSounds[i].mSquadID == squadID)
         {
            //-- If same squad and world then we're good
            if(mpStaticData->mSounds[i].mWorldID == worldID)
               return mpStaticData->mSounds[i].mSoundCue;

            //-- If the world wasn't specified in the data, then we can use that as our fallback.
            if(mpStaticData->mSounds[i].mWorldID == cWorldNone)
               fallBack = mpStaticData->mSounds[i].mSoundCue;
         }
      }
   }

   return fallBack;
}

//==============================================================================
//==============================================================================
void BProtoSquad::calcLevels()
{
   int numUnitNodes = getNumberUnitNodes();
   for (int i = 0; i < numUnitNodes; i++)
   {
      const BProtoSquadUnitNode& unitNode = getUnitNode(i);
      const BProtoObject* pProtoObject=gDatabase.getGenericProtoObject(unitNode.mUnitType);
      int objectLevelCount = pProtoObject->getNumberLevels();
      for (int j = 0; j < objectLevelCount; j++)
      {
         const BProtoObjectLevel* pLevel = pProtoObject->getLevel(j);
         if (pLevel->mXP == 0.0f)
            break;
         int objectLevel = pLevel->mLevel; // smallest level is 1, so store level 1 xp at index 0
         int squadLevelCount = mpStaticData->mLevels.getNumber();
         int minCount = objectLevel;
         if (squadLevelCount < minCount)
         {
            mpStaticData->mLevels.setNumber(minCount);
            for (int i=squadLevelCount; i<minCount; i++)
               mpStaticData->mLevels[i]=0.0f;
         }
         float xp = pLevel->mXP * unitNode.mUnitCount;
         mpStaticData->mLevels[objectLevel-1] += xp;
      }
   }
}

//==============================================================================
//==============================================================================
void BProtoSquad::calcObstructionRadiiFromObjects(float& obsRadiusX, float& obsRadiusZ)
{
   // MPB [3/14/2008]
   // This function calculates the squad obstruction size in the same manner as
   // BFormation2 does.  These probably want to be rolled up into one function
   // and the data cached in the BProtoSquad.  For now, just make sure these stay
   // in sync.
   if (getSquadSize() == 1)
   {
      const BProtoSquadUnitNode& unitNode = getUnitNode(0);
      const BProtoObject* pProtoObject=gDatabase.getGenericProtoObject(unitNode.mUnitType);
      obsRadiusX = pProtoObject->getObstructionRadiusX();
      obsRadiusZ = pProtoObject->getObstructionRadiusZ();
   }
   else
   {
      float totalObsRadiusX = 0.0f;

      int numUnitNodes = getNumberUnitNodes();
      for (int i = 0; i < numUnitNodes; i++)
      {
         const BProtoSquadUnitNode& unitNode = getUnitNode(i);
         const BProtoObject* pProtoObject=gDatabase.getGenericProtoObject(unitNode.mUnitType);

         totalObsRadiusX += Math::Max(pProtoObject->getObstructionRadiusX(), pProtoObject->getObstructionRadiusZ()) * unitNode.mUnitCount;
      }

      if (getFormationType() == BFormation2::eTypeLine)
      {
         const float lineScale = 1.5f * 0.75f * 1.1f;
         obsRadiusX = totalObsRadiusX * lineScale;
      }
      else
      {
         // DLM - adjusting this.
         //const float standardScale = 0.75f * 1.1f;
         const float standardScale = 0.75f * 0.9f;
         obsRadiusX = totalObsRadiusX * standardScale;
      }

      obsRadiusZ = obsRadiusX;
   }
}

//==============================================================================
//==============================================================================
bool BProtoSquad::canMerge(BProtoSquadID baseID, BProtoSquadID& mergedID) const
{
   if (!mpStaticData)
      return false;

   for (int i = 0; i < mMergeIntoProtoSquadIDs.getNumber(); i++)
   {
      const BProtoSquadIDPair& pair = mMergeIntoProtoSquadIDs[i];
      if (pair.mBaseID == baseID)
      {
         mergedID = pair.mMergedID;
         return true;
      }
   }

   return false;
}

//==============================================================================
//==============================================================================
void BProtoSquad::getCommonMergeProtoSquadIDs(const BProtoSquad* pToProtoSquad, BProtoSquadIDPairArray& results)
{
   if (!pToProtoSquad)
      return;

   const BProtoSquadIDPairArray& fromMerges = getMergeFromProtoSquadIDs();
   const BProtoSquadIDPairArray& toMerges = pToProtoSquad->getMergeFromProtoSquadIDs();

   // Between this protoSquad and toID protoSquad, find all the common merge partners.
   // For each common partner, return the mergedID for this proto and the to proto
   for (int i = 0; i < fromMerges.getNumber(); i++)
   {
      for (int j = 0; j < toMerges.getNumber(); j++)
      {
         if (fromMerges[i].mBaseID == toMerges[j].mBaseID)
         {
            results.add(BProtoSquadIDPair(fromMerges[i].mMergedID, toMerges[j].mMergedID));
            break;
         }
      }
   }
}

//==============================================================================
//==============================================================================
void BProtoSquad::createOverrideUnitNodeArray()
{
   if (mpOverrideUnitNodeArray)
   {
      mpOverrideUnitNodeArray->setNumber(0);
      *mpOverrideUnitNodeArray = mpStaticData->mUnitNodes;
   }
   else
      mpOverrideUnitNodeArray = new BUnitNodeArray(mpStaticData->mUnitNodes);
}

//==============================================================================
//==============================================================================
BSquadSoundType BProtoSquad::getSoundType(BSimString& typeName)
{
   BSquadSoundType soundType = cSquadSoundNone;
   if (typeName == "Exist")
      soundType = cSquadSoundExist;
   else if (typeName == "StopExist")
      soundType = cSquadSoundStopExist;               
   else if (typeName == "MoveChatter")
      soundType = cSquadSoundChatterMove;               
   else if (typeName == "AttackChatter")
      soundType = cSquadSoundChatterAttack;               
   else if (typeName == "MoveAttackChatter")
      soundType = cSquadSoundChatterMoveAttack;               
   else if (typeName == "IdleChatter")
      soundType = cSquadSoundChatterIdle;               
   else if (typeName == "KilledEnemy")
      soundType = cSquadSoundChatterKilledEnemy;               
   else if (typeName == "AllyKilled")
      soundType = cSquadSoundChatterAllyKilled;               
   else if (typeName == "ReactBirth")
      soundType = cSquadSoundChatterReactBirth;
   else if (typeName == "ReactDeath")
      soundType = cSquadSoundChatterReactDeath;
   else if (typeName == "ReactJoinBattle")
      soundType = cSquadSoundChatterReactJoinBattle;
   else if (typeName == "Cheer")
      soundType = cSquadSoundChatterCheer;
   else if (typeName == "LevelUp")
      soundType = cSquadSoundChatterLevelUp;
   else if (typeName == "StartMove")
      soundType = cSquadSoundStartMove;               
   else if (typeName == "StopMove")
      soundType = cSquadSoundStopMove;               
   else if (typeName == "StartJump")
      soundType = cSquadSoundStartJump;               
   else if (typeName == "StopJump")
      soundType = cSquadSoundStopJump;               
   else if (typeName == "Kamikaze")
      soundType = cSquadSoundKamikaze;
   else if (typeName == "ReactPowCarpetBomb")
      soundType = cSquadSoundChatterReactPowCarpet;
   else if (typeName == "ReactPowOrbital")
      soundType = cSquadSoundChatterReactPowOrbital;
   else if (typeName == "ReactPowCleansing")
      soundType = cSquadSoundChatterReactPowCleansing;
   else if (typeName == "ReactPowCryo")
      soundType = cSquadSoundChatterReactPowCryo;
   else if (typeName == "ReactPowWave")
      soundType = cSquadSoundChatterReactPowWave;
   else if (typeName == "ReactPowRage")
      soundType = cSquadSoundChatterReactPowRage;
   else if (typeName == "ReactPowDisruption")
      soundType = cSquadSoundChatterReactPowDisruption;
   else if (typeName == "ReactFatalityUNSC")
      soundType = cSquadSoundChatterReactFatalityUNSC;
   else if (typeName == "ReactFatalityCOV")
      soundType = cSquadSoundChatterReactFatalityCOV;
   else if (typeName == "ReactJacking")
      soundType = cSquadSoundChatterReactJacking;
   else if (typeName == "ReactCommandeer")
      soundType = cSquadSoundChatterReactCommandeer;
   else if (typeName == "ReactHotDrop")
      soundType = cSquadSoundChatterReactHotDrop;

   return soundType;
}

//==============================================================================
//==============================================================================
bool BProtoSquad::save(BStream* pStream, int saveType) const
{
   // mpStaticData
   GFWRITEVAR(pStream, long, mpStaticData->mID);

   GFWRITEVAR(pStream, float, mBuildPoints);
   GFWRITECLASS(pStream, saveType, mCost);
   GFWRITEVAR(pStream, float, mMaxHP);
   GFWRITEVAR(pStream, float, mMaxSP);
   GFWRITEVAR(pStream, float, mMaxAmmo);
   GFWRITEVAR(pStream, int, mLevel);
   GFWRITEVAR(pStream, int, mTechLevel);
   GFWRITEVAR(pStream, long, mDisplayNameIndex);
   GFWRITEVAR(pStream, int, mCircleMenuIconID);      
   GFWRITEVAR(pStream, int, mAltCircleMenuIconID);      
   GFWRITEVAR(pStream, long, mHPBar);
   GFWRITEBITBOOL(pStream, mFlagAvailable);
   GFWRITEBITBOOL(pStream, mFlagForbid);
   GFWRITEBITBOOL(pStream, mFlagOneTimeSpawnUsed);
   GFWRITEBITBOOL(pStream, mFlagKBAware);

   bool overrideNodes = (mpOverrideUnitNodeArray != NULL);
   GFWRITEVAR(pStream, bool, overrideNodes);
   if (overrideNodes)
   {
      const BProtoSquad* pBaseProto = gDatabase.getGenericProtoSquad(mBaseType);
      int nodeCount = mpOverrideUnitNodeArray->getNumber();
      GFWRITEVAL(pStream, int8, nodeCount);
      GFVERIFYCOUNT(nodeCount, 16);
      for (int i=0; i<nodeCount; i++)
      {
         const BProtoSquadUnitNode& baseNode = pBaseProto->getUnitNode(i);
         GFWRITEVAR(pStream, long, baseNode.mUnitType);
         BProtoSquadUnitNode& node = mpOverrideUnitNodeArray->get(i);
         GFWRITEVAR(pStream, long, node.mUnitType);
      }
   }

   GFWRITEMARKER(pStream, cSaveMarkerProtoSquad1);

   return true;
}

//==============================================================================
//==============================================================================
bool BProtoSquad::load(BStream* pStream, int saveType, BPlayer* pPlayer)
{
   if (mGameFileVersion >= 3)
   {
      // mpStaticData;
      long staticProtoID;
      GFREADVAR(pStream, long, staticProtoID);
      gSaveGame.remapProtoSquadID(staticProtoID);
      if (staticProtoID != -1 && staticProtoID != mID)
      {
         const BProtoSquad* pFromProtoSquad = pPlayer->getProtoSquad(staticProtoID);
         mpStaticData = pFromProtoSquad->mpStaticData;
         mFlagOwnStaticData = false;
      }
   }

   GFREADVAR(pStream, float, mBuildPoints);
   GFREADCLASS(pStream, saveType, mCost);
   GFREADVAR(pStream, float, mMaxHP);
   GFREADVAR(pStream, float, mMaxSP);
   GFREADVAR(pStream, float, mMaxAmmo);
   GFREADVAR(pStream, int, mLevel);
   GFREADVAR(pStream, int, mTechLevel);
   GFREADVAR(pStream, long, mDisplayNameIndex);
   GFREADVAR(pStream, int, mCircleMenuIconID);      
   GFREADVAR(pStream, int, mAltCircleMenuIconID);      
   if (BProtoSquad::mGameFileVersion >= 2)
      GFREADVAR(pStream, long, mHPBar);
   GFREADBITBOOL(pStream, mFlagAvailable);
   GFREADBITBOOL(pStream, mFlagForbid);
   GFREADBITBOOL(pStream, mFlagOneTimeSpawnUsed);
   GFREADBITBOOL(pStream, mFlagKBAware);

   bool overrideNodes;
   GFREADVAR(pStream, bool, overrideNodes);
   if (overrideNodes)
   {
      createOverrideUnitNodeArray();
      if (!mpOverrideUnitNodeArray)
         return false;
      int nodeCount;
      GFREADVAL(pStream, int8, int, nodeCount);
      GFVERIFYCOUNT(nodeCount, 16);
      for (int i=0; i<nodeCount; i++)
      {
         long baseType, unitType;
         GFREADVAR(pStream, long, baseType);
         GFREADVAR(pStream, long, unitType);
         if (i < mpOverrideUnitNodeArray->getNumber())
         {
            BProtoSquadUnitNode& node = mpOverrideUnitNodeArray->get(i);
            if (gSaveGame.remapProtoObjectID(baseType) && node.mUnitType == baseType)
               node.mUnitType = gSaveGame.getProtoObjectID(unitType);
         }
      }
   }

   GFREADMARKER(pStream, cSaveMarkerProtoSquad1);

   if (mpStaticData)
   {
      if (mpStaticData->mProtoObjectID==-1)
         updateAttackRatings();
      else
      {
         BProtoObject* pProtoObject = (mpPlayer ? mpPlayer->getProtoObject(mpStaticData->mProtoObjectID) : gDatabase.getGenericProtoObject(mpStaticData->mProtoObjectID));
         if (pProtoObject)
         {
            uint numDamageTypes = gDatabase.getNumberAttackRatingDamageTypes();
            for (uint i=0; i<numDamageTypes; i++)
               mAttackRatingDPS[i]=pProtoObject->getAttackRatingDPS(gDatabase.getAttackRatingDamageType(i));
         }
      }
   }

   return true;
}

//==============================================================================
// protosquad.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

#pragma once

//==============================================================================
// Includes
//==============================================================================
#include "bitvector.h"
#include "cost.h"
#include "protoobject.h"
#include "pop.h"
#include "xmlreader.h"
#include "flashscene.h"

//==============================================================================
// Forward declarations
//==============================================================================
class BPlayer;

//==============================================================================
// BProtoSquadUnitNode
//==============================================================================
class BProtoSquadUnitNode
{
   public:

      BProtoSquadUnitNode() : mUnitType(-1), mUnitCount(0), mUnitRole(0) { }
      ~BProtoSquadUnitNode()                               { }

   public:

      long           mUnitType;
      long           mUnitCount;
      long           mUnitRole;
};
typedef BDynamicSimArray<BProtoSquadUnitNode, 4, BDynamicArrayDefaultOptions> BUnitNodeArray;

//==============================================================================
//==============================================================================
class BProtoSquadIDPair
{
   public:
      BProtoSquadIDPair() : mBaseID(cInvalidProtoSquadID), mMergedID(cInvalidProtoSquadID) { }
      BProtoSquadIDPair(BProtoSquadID baseID, BProtoSquadID mergedID) : mBaseID(baseID), mMergedID(mergedID) { }
      ~BProtoSquadIDPair() { }

      BProtoSquadID mBaseID;
      BProtoSquadID mMergedID;
};
typedef BSmallDynamicSimArray<BProtoSquadIDPair> BProtoSquadIDPairArray;

//==============================================================================
// BProtoSquadStatic
//==============================================================================
class BProtoSquadStatic
{
   public:
                                 BProtoSquadStatic();
                                 BProtoSquadStatic(const BProtoSquadStatic* pBase);
                                 ~BProtoSquadStatic();

      enum
      {
         cTrainedBirthType,
         cFlyInBirthType,

         cMaxNumBirthTypes
      };


      BSimString                 mName;
      int                        mProtoObjectID;
      long                       mDBID;
      long                       mStatsNameIndex;
      long                       mRolloverTextIndex;
      long                       mPrereqTextIndex;
      long                       mRoleTextIndex;
      long                       mSquadSize;
      BSimString                 mIcon;
      BSimString                 mAltIcon;
      long                       mStance;
      long                       mTrainAnimType;
      float                      mSelectionRadiusX;
      float                      mSelectionRadiusZ;
      float                      mSelectionYOffset;
      float                      mSelectionZOffset;
      bool                       mSelectionConformToTerrain;
      bool                       mSelectionAllowOrientation;
      int                        mSelectionDecalID;
      BFlashPropertyHandle       mClumpSelectionDecalID;
      BFlashPropertyHandle       mClumpSelectionDecalMaskID;
      BFlashPropertyHandle       mStaticDecalID;

      int                        mUnitSelectionIconID;      
      long                       mBirthType;
      BSimString                 mBirthBone;
      BSimString                 mBirthEndBone;
      long                       mBirthAnim[4];
      long                       mBirthTrainerAnim;
      float                      mHPBarSizeX;
      float                      mHPBarSizeY;
      BVector                    mHPBarOffset;
      int                        mSubSelectSort;
      float                      mTurnRadiusMin;
      float                      mTurnRadiusMax;
      float                      mAttackGradeDPS;
      BDamageTypeIDSmall         mDamageType;
      int                        mRecoveringEffect;
      float                      mLeashDistance;
      float                      mLeashDeadzone;
      DWORD                      mLeashRecallDelay;
      float                      mAggroDistance;
      float                      mMinimapScale;

      long                       mVeterancyBarID;
      long                       mVeterancyCenteredBarID;
      float                      mVeterancyBarSizeX;
      float                      mVeterancyBarSizeY;
      float                      mVeterancyBarOffsetX;
      float                      mVeterancyBarOffsetY;

      long                       mAbilityRecoveryBarID;
      long                       mAbilityRecoveryCenteredBarID;
      float                      mAbilityRecoveryBarSizeX;
      float                      mAbilityRecoveryBarSizeY;
      float                      mAbilityRecoveryBarOffsetX;
      float                      mAbilityRecoveryBarOffsetY;

      long                       mBobbleHeadID;
      int                        mBuildingStrengthID;

      float                      mCryoPoints;
      float                      mDazeResist;

      BUnitNodeArray             mUnitNodes;
      BSmallDynamicSimArray<float>  mLevels;

      BFormationType             mFormationType;

      BProtoSoundArray  mSounds;
      uint32            mUsedSoundTypes;

      long                          mID;

      // Static flags (only set at database load time and not per player)
      bool                          mFlagHasHPBar:1;
      bool                          mFlagNoTieToGround:1;
      bool                          mFlagRepairable:1;
      bool                          mFlagChatter:1;
      bool                          mFlagCreateAudioReactions:1;
      bool                          mFlagForceToGaiaPlayer:1;
      bool                          mFlagInstantTrainWithRecharge:1;
      bool                          mFlagAlwaysAttackReviveUnits:1;
      bool                          mFlagScaredByRoar:1;
      bool                          mFlagAlwaysRenderSelectionDecal:1;
      bool                          mFlagOnlyShowBobbleHeadWhenContained:1;
      bool                          mFlagDiesWhenFrozen:1;
      bool                          mFlagFlyingFlood:1;
      bool                          mFlagFlood:1;
      bool                          mFlagForerunner:1;
      bool                          mFlagRebel:1;
      bool                          mFlagCanAttackWhileMoving:1;
      bool                          mFlagNoPlatoonMerge:1;
      bool                          mFlagObjectProtoSquad:1;
      bool                          mFlagMergedProtoSquad:1;
      bool                          mFlagAlwaysShowHPBar:1;
};

//==============================================================================
// BProtoSquad
//==============================================================================
class BProtoSquad
{
   public:  
      enum
      {
         cStancePassive,
         cStanceAggressive,
         cStanceDefensive,
         cNumberStances,
      };

      // SLB: TODO move this elsewhere
      enum
      {
         cRoleNormal,
         cRoleLeader,
         cRoleSupport,

         cUnitRoleCount
      };

                                 BProtoSquad(long id);
                                 BProtoSquad(const BProtoSquad* pBase, BPlayer* pPlayer);
                                 BProtoSquad(long id, const BProtoSquad* pBase, const BProtoSquad* pMerge);
                                 ~BProtoSquad();

      void                       transform(const BProtoSquad* pBase);

      bool                       load(BXMLNode root);
      bool                       initFromProtoObject(const BProtoObject* pProtoObject, int protoSquadID);

      // Dynamic data access (changeable per player)      
      long                       getID() const                                { return mID; }
      void                       setID(long id) { mID = id; }
      long                       getBaseType() const                                { return mBaseType; }
      void                       setBaseType(long id) { mBaseType = id; }
      long                       getStaticID() { return mpStaticData->mID; }
      float                      getBuildPoints(void) const;
      float                      getMaxHP() const { return (mMaxHP); }
      float                      getMaxSP() const { return (mMaxSP); }
      float                      getMaxAmmo() const { return (mMaxAmmo); }
      const BCost*               getCost(void) const;
      int                        getLevel() const { return mLevel; }
      int                        getTechLevel() const { return mTechLevel; }

      void                       setBuildPoints(float val);
      void                       setMaxHP(float v) { mMaxHP = v; }
      void                       setMaxSP(float v) { mMaxSP = v; }
      void                       setMaxAmmo(float v) { mMaxAmmo = v; }
      void                       setCost(BCost* pVal);
      void                       setLevel(int v) { mLevel=v; }
      void                       setTechLevel(int v) { mTechLevel=v; }
      void                       setDisplayNameIndex(int v) { mDisplayNameIndex=v; }
      void                       setCircleMenuIconID(int id) { mCircleMenuIconID = id; }
      void                       setAltCircleMenuIconID(int id) { mAltCircleMenuIconID = id; }

      void                       updateAttackRatings();
      bool                       getHasAttackRatings() const;
      float                      getAttackRatingDPS(BDamageTypeID damageType) const;
      float                      getAttackRating(BDamageTypeID damageType) const;
      float                      getAttackRating() const;
      float                      getDefenseRating() const;
      float                      getStrength() const;
      uint                       getAttackGrade(BDamageTypeID damageType) const;
      float                      getAttackGradeRatio(BDamageTypeID damageType) const;
      float                      getStars(BDamageTypeID damageType) const { return getAttackGradeRatio(damageType); }
      float                      getCombatValue() const;
      void                       getDisplayName(BUString& string) const;
      void                       getStatsName(BUString& string) const;
      int                        getCircleMenuIconID() const { return mCircleMenuIconID; }
      int                        getAltCircleMenuIconID() const { return mAltCircleMenuIconID; }

      // Static data access
      const BSimString&          getName(void) const                          { return mpStaticData->mName; }
      void                       getRolloverText(BUString& string) const;
      void                       getPrereqText(BUString& string) const;
      void                       getRoleText(BUString& string) const;
      int                        getProtoObjectID() const                     { return mpStaticData->mProtoObjectID; }
      long                       getDBID(void) const                          { return mpStaticData->mDBID; }
      BFormationType             getFormationType(void) const                 { return mpStaticData->mFormationType; }
      long                       getSquadSize(void) const                     { return mpStaticData->mSquadSize; }
      long                       getNumberUnitNodes(void) const               { return mpOverrideUnitNodeArray ? mpOverrideUnitNodeArray->getNumber() : mpStaticData->mUnitNodes.getNumber(); }
      const BProtoSquadUnitNode& getUnitNode(long node) const                 { return mpOverrideUnitNodeArray ? mpOverrideUnitNodeArray->get(node) : mpStaticData->mUnitNodes.get(node); }
      const BUnitNodeArray&      getUnitNodes() const                         { return mpOverrideUnitNodeArray ? *mpOverrideUnitNodeArray : mpStaticData->mUnitNodes; }

      int                        getNumberLevels() const                      { return mpStaticData->mLevels.getNumber(); }
      float                      getLevelXP(int index)const                   { if (index < 0 || index >= mpStaticData->mLevels.getNumber()) return NULL; return mpStaticData->mLevels[index]; }
      const BSimString&          getIcon(void) const                          { return mpStaticData->mIcon; }
      const BSimString&          getAltIcon(void) const                       { return mpStaticData->mAltIcon; }
      long                       getStance(void) const                        { return mpStaticData->mStance; }
      long                       getTrainAnimType() const                     { return mpStaticData->mTrainAnimType; }
      float                      getSelectionRadiusX() const                  { return mpStaticData->mSelectionRadiusX; }
      float                      getSelectionRadiusZ() const                  { return mpStaticData->mSelectionRadiusZ; }
      float                      getSelectionYOffset() const                  { return mpStaticData->mSelectionYOffset; }
      float                      getSelectionZOffset() const                  { return mpStaticData->mSelectionZOffset; }
      bool                       getSelectionConformToTerrain() const         { return mpStaticData->mSelectionConformToTerrain; }    
      bool                       getSelectionAllowOrientation() const         { return mpStaticData->mSelectionAllowOrientation; }          
      int                        getUnitSelectionIconID() const               { return mpStaticData->mUnitSelectionIconID; }
      void                       setUnitSelectionIconID(int id)               { mpStaticData->mUnitSelectionIconID = id; }
      int                        getSelectionDecalID() const                  { return mpStaticData->mSelectionDecalID; }
      void                       setSelectionDecalID(int id)                  { mpStaticData->mSelectionDecalID = id; }
      int                        getClumpSelectionDecalID() const             { return mpStaticData->mClumpSelectionDecalID; }
      void                       setClumpSelectionDecalID(BFlashPropertyHandle id) { mpStaticData->mClumpSelectionDecalID = id; }
      BFlashPropertyHandle       getClumpSelectionDecalMaskID() const         { return mpStaticData->mClumpSelectionDecalMaskID; }
      void                       setClumpSelectionDecalMaskID(int id)         { mpStaticData->mClumpSelectionDecalMaskID = id; }
      BFlashPropertyHandle       getStaticDecalID() const                     { return mpStaticData->mStaticDecalID; }
      void                       setStaticDecalID(BFlashPropertyHandle id) const { mpStaticData->mStaticDecalID = id; }
      void                       getPops(BPopArray& pops) const;
      long                       getBirthType() const                         { return mpStaticData->mBirthType; }
      bool                       isTrainedBirthType() const                   { return (mpStaticData->mBirthType == BProtoSquadStatic::cTrainedBirthType) ? true : false; }
      bool                       isFlyInBirthType() const                     { return (mpStaticData->mBirthType == BProtoSquadStatic::cFlyInBirthType) ? true : false; }
      const BSimString&          getBirthBone() const                         { return mpStaticData->mBirthBone; }
      const BSimString&          getBirthEndBone() const                      { return mpStaticData->mBirthEndBone; }
      long                       getBirthAnim(int id) const                   { return (mpStaticData->mBirthAnim[id] ? mpStaticData->mBirthAnim[id] : mpStaticData->mBirthAnim[0]); }
      long                       getBirthTrainerAnim() const                  { return mpStaticData->mBirthTrainerAnim; }      
      long                       getHPBarID() const                           { return mHPBar; }
      void                       setHPBarID(long id )                        { mHPBar = id; }
      float                      getHPBarSizeX() const                        { return mpStaticData->mHPBarSizeX; }
      float                      getHPBarSizeY() const                        { return mpStaticData->mHPBarSizeY; }
      const BVector&             getHPBarOffset() const                       { return mpStaticData->mHPBarOffset; }
      int                        getSubSelectSort() const                     { return mpStaticData->mSubSelectSort; }
      float                      getTurnRadiusMin() const                     { return mpStaticData->mTurnRadiusMin; }
      float                      getTurnRadiusMax() const                     { return mpStaticData->mTurnRadiusMax; }
      bool                       hasTurnRadius() const                        { return (mpStaticData->mTurnRadiusMin > 0.0f && mpStaticData->mTurnRadiusMax > 0.0f); }
      float                      getAttackGradeDPS() const                    { return mpStaticData->mAttackGradeDPS; }
      BDamageTypeID              getDamageType() const                        { return mpStaticData->mDamageType; }
      bool                       isDamageType(BDamageTypeID damageType) const { return (mpStaticData->mDamageType == damageType); }
      int                        getRecoveringEffect() const                  { return mpStaticData->mRecoveringEffect; }
      float                      getLeashDistance() const                     { return (mpStaticData->mLeashDistance); }
      float                      getLeashDeadzone() const                     { return (mpStaticData->mLeashDeadzone); }
      DWORD                      getLeashRecallDelay() const                  { return (mpStaticData->mLeashRecallDelay); }
      float                      getAggroDistance() const                     { return (mpStaticData->mAggroDistance); }
      float                      getMinimapScale() const                      { return (mpStaticData->mMinimapScale); }

      long                       getVeterancyBarID() const                    { return mpStaticData->mVeterancyBarID; }
      long                       getVeterancyCenteredBarID() const            { return mpStaticData->mVeterancyCenteredBarID; }
      float                      getVeterancyBarSizeX() const                 { return mpStaticData->mVeterancyBarSizeX; };
      float                      getVeterancyBarSizeY() const                 { return mpStaticData->mVeterancyBarSizeY; };
      float                      getVeterancyBarOffsetX() const               { return mpStaticData->mVeterancyBarOffsetX; };
      float                      getVeterancyBarOffsetY() const               { return mpStaticData->mVeterancyBarOffsetY; };

      long                       getAbilityRecoveryBarID() const              { return mpStaticData->mAbilityRecoveryBarID; };
      long                       getAbilityRecoveryCenteredBarID() const      { return mpStaticData->mAbilityRecoveryCenteredBarID; };
      float                      getAbilityRecoveryBarSizeX() const           { return mpStaticData->mAbilityRecoveryBarSizeX; };
      float                      getAbilityRecoveryBarSizeY() const           { return mpStaticData->mAbilityRecoveryBarSizeY; };
      float                      getAbilityRecoveryBarOffsetX() const         { return mpStaticData->mAbilityRecoveryBarOffsetX; };
      float                      getAbilityRecoveryBarOffsetY() const         { return mpStaticData->mAbilityRecoveryBarOffsetY; };

      long                       getBobbleHeadID() const                      { return mpStaticData->mBobbleHeadID; };      
      int                        getBuildingStrengthID() const                { return mpStaticData->mBuildingStrengthID; };

      float                      getCryoPoints() const                        { return mpStaticData->mCryoPoints; };

      float                      getDazeResist() const                        { return mpStaticData->mDazeResist; };

      BCueIndex                  getSound(BSquadSoundType soundType, long squadID=-1, uint worldID=cWorldNone, BEntityID callingSquadID=cInvalidObjectID, BEntityID castingSquadID=cInvalidObjectID) const;
      static BSquadSoundType     getSoundType(BSimString& type);

      void                       calcObstructionRadiiFromObjects(float& obsRadiusX, float& obsRadiusZ);

      void                       addMergeIntoProtoSquadIDs(BProtoSquadID baseID, BProtoSquadID mergedID) { mMergeIntoProtoSquadIDs.add(BProtoSquadIDPair(baseID, mergedID));}
      void                       addMergeFromProtoSquadIDs(BProtoSquadID mergeFromID, BProtoSquadID mergedID) { mMergeFromProtoSquadIDs.add(BProtoSquadIDPair(mergeFromID, mergedID)); }
      const BProtoSquadIDPairArray& getMergeFromProtoSquadIDs() const { return mMergeFromProtoSquadIDs; }
      bool                       canMerge(BProtoSquadID baseID, BProtoSquadID& mergedID) const;
      void                       getCommonMergeProtoSquadIDs(const BProtoSquad* pToProtoSquad, BProtoSquadIDPairArray& results);

      void                       createOverrideUnitNodeArray();

      // Dynamic flags (changeable per player during a game)
      bool                       getFlagOwnStaticData() const { return(mFlagOwnStaticData); }
      bool                       getFlagAvailable() const { return(mFlagAvailable); }
      void                       setFlagAvailable(bool v) { mFlagAvailable=v; }
      bool                       getFlagForbid() const { return(mFlagForbid); }
      void                       setFlagForbid(bool v) { mFlagForbid=v; }
      bool                       getFlagOneTimeSpawnUsed() const { return mFlagOneTimeSpawnUsed; }
      void                       setFlagOneTimeSpawnUsed(bool v) { mFlagOneTimeSpawnUsed=v; }
      bool                       getFlagUniqueInstance() const { return mFlagUniqueInstance; }
      void                       setFlagUniqueInstance(bool v) { mFlagUniqueInstance=v; }
      bool                       getFlagKBAware() const { return (mFlagKBAware); }
      void                       setFlagKBAware(bool v) { mFlagKBAware = v; }

      // Static flags (only set at database load time and not per player)
      bool                       getFlagHasHPBar() const { return(mpStaticData->mFlagHasHPBar); }
      bool                       getFlagNoTieToGround() const { return(mpStaticData->mFlagNoTieToGround); }
      bool                       getFlagRepairable() const { return(mpStaticData->mFlagRepairable); }
      bool                       getFlagChatter() const { return(mpStaticData->mFlagChatter); }
      bool                       getFlagCreateAudioReactions() const { return mpStaticData->mFlagCreateAudioReactions; }
      bool                       getFlagForceToGaiaPlayer() const { return(mpStaticData->mFlagForceToGaiaPlayer); }
      bool                       getFlagInstantTrainWithRecharge() const { return mpStaticData->mFlagInstantTrainWithRecharge; }
      bool                       getFlagAlwaysAttackReviveUnits() const { return mpStaticData->mFlagAlwaysAttackReviveUnits; }
      bool                       getFlagScaredByRoar() const { return mpStaticData->mFlagScaredByRoar; }
      bool                       getFlagAlwaysRenderSelectionDecal() const { return mpStaticData->mFlagAlwaysRenderSelectionDecal; }
      bool                       getFlagOnlyShowBobbleHeadWhenContained() const { return mpStaticData->mFlagOnlyShowBobbleHeadWhenContained; }
      bool                       getFlagDiesWhenFrozen() const { return mpStaticData->mFlagDiesWhenFrozen; }
      bool                       getFlagFlyingFlood() const { return mpStaticData->mFlagFlyingFlood; }
      bool                       getFlagFlood() const { return mpStaticData->mFlagFlood; }
      bool                       getFlagForerunner() const { return mpStaticData->mFlagForerunner; }
      bool                       getFlagRebel() const { return mpStaticData->mFlagRebel; }
      bool                       getFlagCanAttackWhileMoving() const { return mpStaticData->mFlagCanAttackWhileMoving; }
      bool                       getFlagNoPlatoonMerge() const { return (mpStaticData->mFlagNoPlatoonMerge); }
      bool                       getFlagObjectProtoSquad() const { return mpStaticData->mFlagObjectProtoSquad; }
      bool                       getFlagMergedProtoSquad() const { return mpStaticData->mFlagMergedProtoSquad; }
      bool                       getFlagAlwaysShowHPBar() const { return (mpStaticData->mFlagAlwaysShowHPBar); }

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType, BPlayer* pPlayer);

   protected:
      void                       copyFlags(const BProtoSquad* pBase);
      void                       calcLevels();

      // Static data
      BProtoSquadStatic*            mpStaticData;

      // Merge data
      BProtoSquadIDPairArray        mMergeIntoProtoSquadIDs; // Array of (ID for squad to merge into, ID for resulting merged squad)
      BProtoSquadIDPairArray        mMergeFromProtoSquadIDs; // Reciprocal array of (ID for squad that has merged into this one, ID for resulting merged squad)

      BUnitNodeArray*               mpOverrideUnitNodeArray;

      // Dynamic data (changeable per player)
      long                          mID;
      long                          mBaseType;
      BPlayer*                      mpPlayer;
      float                         mBuildPoints;
      BCost                         mCost;
      BSmallDynamicSimArray<float>  mAttackRatingDPS;
      float                         mMaxHP;
      float                         mMaxSP;
      float                         mMaxAmmo;
      int                           mLevel;
      int                           mTechLevel;
      long                          mDisplayNameIndex;
      int                           mCircleMenuIconID;      
      int                           mAltCircleMenuIconID;
      long                          mHPBar;

      //-- Dynamic flags (changeable per player during the game)
      bool                          mFlagOwnStaticData:1;
      bool                          mFlagAvailable:1;
      bool                          mFlagForbid:1;
      bool                          mFlagOneTimeSpawnUsed:1;
      bool                          mFlagUniqueInstance:1;
      bool                          mFlagKBAware:1;         // This must be in per-player data because of merged squads which don't exist in database
};

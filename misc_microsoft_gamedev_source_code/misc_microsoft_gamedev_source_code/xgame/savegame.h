//==============================================================================
// savegame.h
//
// Copyright (c) 2006-2008 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "gamefile.h"
#include "simtypes.h"

// Forward declarations
struct BSaveItem;

class BGameSettings;

//==============================================================================
// Save file types
//==============================================================================
enum
{
   cSaveFileTypeFull,
   cSaveFileTypePartial,
};

//==============================================================================
// BSaveUser
//==============================================================================
class BSaveUser
{
   public:
      GFDECLAREVERSION();
      BVector        mHoverPoint;
      BVector        mCameraHoverPoint;
      BVector        mCameraPosition;
      BVector        mCameraForward;
      BVector        mCameraRight;
      BVector        mCameraUp;
      float          mCameraDefaultPitch;
      float          mCameraDefaultYaw;
      float          mCameraDefaultZoom;
      int            mCurrentPlayer;
      int            mCoopPlayer;
      float          mCameraPitch;
      float          mCameraYaw;
      float          mCameraZoom;
      float          mCameraFOV;
      float          mCameraHoverPointOffsetHeight;
      bool           mHaveHoverPoint:1;
      bool           mDefaultCamera:1;
};

//==============================================================================
// BSaveDB
//==============================================================================
class BSaveDB
{
   public:
      GFDECLAREVERSION();
      BSmallDynamicSimArray<int> mCivMap;
      BSmallDynamicSimArray<int> mLeaderMap;
      BSmallDynamicSimArray<int> mAbilityMap;
      BSmallDynamicSimArray<int> mProtoVisualMap;
      BSmallDynamicSimArray<int> mGrannyModelMap;
      BSmallDynamicSimArray<int> mGrannyAnimMap;
      BSmallDynamicSimArray<int> mTerrainEffectMap;
      BSmallDynamicSimArray<int> mImpactEffectMap;
      BSmallDynamicSimArray<BSimString> mLightEffectNames;
      BSmallDynamicSimArray<int> mLightEffectMap;
      BSmallDynamicSimArray<BSimString> mParticleEffectNames;
      BSmallDynamicSimArray<int> mParticleEffectMap;
      BSmallDynamicSimArray<int> mProtoObjectMap;
      BSmallDynamicSimArray<int> mProtoSquadMap;
      BSmallDynamicSimArray<int> mProtoTechMap;
      BSmallDynamicSimArray<int> mProtoPowerMap;
      BSmallDynamicSimArray<int> mObjectTypeMap;
      BSmallDynamicSimArray<int> mResourceMap;
      BSmallDynamicSimArray<int> mRateMap;
      BSmallDynamicSimArray<int> mPopMap;
      BSmallDynamicSimArray<int> mPlacementRuleMap;
      BSmallDynamicSimArray<int> mWeaponTypeMap;
      BSmallDynamicSimArray<int> mDamageTypeMap;
      BSmallDynamicSimArray<BSimString> mDamageTemplateNames;
      BSmallDynamicSimArray<long> mDamageTemplateModelIDs;
      BSmallDynamicSimArray<int> mDamageTemplateMap;
      BSmallDynamicSimArray<int> mAnimTypeMap;
      BSmallDynamicSimArray<int> mAttachmentTypeMap;
      BSmallDynamicSimArray<int> mActionTypeMap;
      BSmallDynamicSimArray<BSmallDynamicSimArray<int>> mProtoActionMap;
      BSmallDynamicSimArray<BSmallDynamicSimArray<int>> mWeaponMap;
      BSmallDynamicSimArray<long> mShapeMap;
      BSmallDynamicSimArray<long> mPhysicsInfoMap;
      BSmallDynamicSimArray<BSimString> mProtoIconTypes;
      BSmallDynamicSimArray<BSimString> mProtoIconNames;
      int mNumUniqueProtoObjects;
      int mNumUniqueProtoSquads;
};

//==============================================================================
// BSavePlayer
//==============================================================================
class BSavePlayer
{
   public:
      GFDECLAREVERSION();
      BSimString                          mName;
      BUString                            mDisplayName;
      long                                mMPID;
      BPlayerID                           mCoopID;
      BPlayerID                           mScenarioID;
      long                                mCivID;
      BTeamID                             mTeamID;
      long                                mLeaderID;
      BHalfFloat                          mDifficulty;
      int8                                mPlayerType;
};

//==============================================================================
// BSaveTeam
//==============================================================================
class BSaveTeam
{
   public:
      GFDECLAREVERSION();
      BSmallDynamicSimArray<long> mPlayers;
      BSmallDynamicSimArray<BRelationType>   mRelations;
};

//==============================================================================
// BSaveGame
//==============================================================================
class BSaveGame : public BGameFile
{
   public:
      // ajl 11/18/08 - Examined save file sizes on the storage device to determine the size we needed 
      // to get file size fixed at 1 MB. There appears to be an overhead of 77248 bytes so accounting for that. 
      // 1024 bytes * 1024 k = 1 MB or 1048576. 1048576 - 77248 = 971328.
      enum 
      { 
         MAX_SIZE_IN_BYTES = 1024 * 1024,
         USER_STORAGE_OVERHEAD = 142784,
         FIXED_FILE_SIZE = MAX_SIZE_IN_BYTES - USER_STORAGE_OVERHEAD,
      };

   public:
                     BSaveGame();
                     ~BSaveGame();

      virtual bool   setup();
      virtual void   reset();

      bool           createNewCampaignFile();

      void           saveGame(const char* pFileName, bool autoQuit=false);
      void           loadGame(const char* pFileName, bool doLoadCampaignGame=false);

      bool           getDoSaveGame() const { return mDoSaveGame; }
      bool           getDoLoadGame() const { return mDoLoadGame; }
      bool           getDoAutoQuit() const { return mDoAutoQuit; }
      bool           getSaveFileInProgress() const { return mSaveRequest.mInProgress; }
      bool           getDoSaveCampaign() const { return mDoSaveCampaign; }

      bool           getCampaignSaveExists(BGameSettings* pSettings=NULL, BUString* pName=NULL);
      XCONTENTDEVICEID autoSelectStorageDevice();

      void           setDoSaveGame(XCONTENTDEVICEID deviceID);
      void           clearDoSaveGame();

      void           doSaveGameBegin();
      void           doSaveGameEnd();

      void           doLoadGame();

      bool           save();
      bool           load();

      bool           loadSetup();
      bool           loadData();

      bool           isSaving() const { return mIsSaving; }
      bool           isLoading() const { return mIsLoading; }

      bool           saveTables(BStream* pStream);
      bool           loadTables(BStream* pStream);

      bool           writeTable(BStream* pStream, const BSaveItem* pSaveTable);
      bool           readTable(BStream* pStream, BSaveItem* pNewSaveTable, const char* pTableName);

      bool           writeData(BStream* pStream, const BSaveItem* pSaveTable, const void* pBasePtr);
      bool           readData(BStream* pStream, const BSaveItem* pOldSaveTable, const BSaveItem* pNewSaveTable, void* pBasePtr);

      int            getCivID(int id) const { return ((id < 0 || id >= mDB.mCivMap.getNumber()) ? -1 : mDB.mCivMap[id]); }
      int            getLeaderID(int id) const { return ((id < 0 || id >= mDB.mLeaderMap.getNumber()) ? -1 : mDB.mLeaderMap[id]); }
      int            getAbilityID(int id) const { return ((id < 0 || id >= mDB.mAbilityMap.getNumber()) ? -1 : mDB.mAbilityMap[id]); }
      int            getProtoVisualID(int id) const { return ((id < 0 || id >= mDB.mProtoVisualMap.getNumber()) ? -1 : mDB.mProtoVisualMap[id]); }
      int            getGrannyModelID(int id) const { return ((id < 0 || id >= mDB.mGrannyModelMap.getNumber()) ? -1 : mDB.mGrannyModelMap[id]); }
      int            getGrannyAnimID(int id) const { return ((id < 0 || id >= mDB.mGrannyAnimMap.getNumber()) ? -1 : mDB.mGrannyAnimMap[id]); }
      int            getTerrainEffectID(int id) const { return ((id < 0 || id >= mDB.mTerrainEffectMap.getNumber()) ? -1 : mDB.mTerrainEffectMap[id]); }
      int            getImpactEffectID(int id) const { return ((id < 0 || id >= mDB.mImpactEffectMap.getNumber()) ? -1 : mDB.mImpactEffectMap[id]); }
      int            getLightEffectID(int id) const { return ((id < 0 || id >= mDB.mLightEffectMap.getNumber()) ? -1 : mDB.mLightEffectMap[id]); }
      int            getParticleEffectID(int id) const { return ((id < 0 || id >= mDB.mParticleEffectMap.getNumber()) ? -1 : mDB.mParticleEffectMap[id]); }
      int            getProtoObjectID(int id) const;
      int            getProtoSquadID(int id) const;
      int            getProtoTechID(int id) const { return ((id < 0 || id >= mDB.mProtoTechMap.getNumber()) ? -1 : mDB.mProtoTechMap[id]); }
      int            getProtoPowerID(int id) const { return ((id < 0 || id >= mDB.mProtoPowerMap.getNumber()) ? -1 : mDB.mProtoPowerMap[id]); }
      int            getResourceID(int id) const { return ((id < 0 || id >= mDB.mResourceMap.getNumber()) ? -1 : mDB.mResourceMap[id]); }
      int            getRateID(int id) const { return ((id < 0 || id >= mDB.mRateMap.getNumber()) ? -1 : mDB.mRateMap[id]); }
      int            getPopID(int id) const { return ((id < 0 || id >= mDB.mPopMap.getNumber()) ? -1 : mDB.mPopMap[id]); }
      int            getPlacementRuleID(int id) const { return ((id < 0 || id >= mDB.mPlacementRuleMap.getNumber()) ? -1 : mDB.mPlacementRuleMap[id]); }
      int            getProtoActionID(BProtoObjectID newProtoObjectID, int saveProtoActionID) const;
      int            getWeaponID(BProtoObjectID newProtoObjectID, int saveWeaponID) const;
      int            getObjectType(int type) const { return ((type < 0 || type >= mDB.mObjectTypeMap.getNumber()) ? -1 : mDB.mObjectTypeMap[type]); }
      int            getWeaponType(int type) const { return ((type < 0 || type >= mDB.mWeaponTypeMap.getNumber()) ? -1 : mDB.mWeaponTypeMap[type]); }
      int            getDamageType(int type) const { return ((type < 0 || type >= mDB.mDamageTypeMap.getNumber()) ? -1 : mDB.mDamageTypeMap[type]); }
      int            getDamageTemplate(int id) const { return ((id < 0 || id >= mDB.mDamageTemplateMap.getNumber()) ? -1 : mDB.mDamageTemplateMap[id]); }
      int            getAnimType(int type) const { return ((type < 0 || type >= mDB.mAnimTypeMap.getNumber()) ? -1 : mDB.mAnimTypeMap[type]); }
      int            getAttachmentType(int type) const { return ((type < 0 || type >= mDB.mAttachmentTypeMap.getNumber()) ? -1 : mDB.mAttachmentTypeMap[type]); }
      int            getActionType(int type) const { return ((type < 0 || type >= mDB.mActionTypeMap.getNumber()) ? -1 : mDB.mActionTypeMap[type]); }
      int            getShapeID(int id) const { return ((id < 0 || id >= mDB.mShapeMap.getNumber()) ? -1 : mDB.mShapeMap[id]); }
      int            getPhysicsInfoID(int id) const { return ((id < 0 || id >= mDB.mPhysicsInfoMap.getNumber()) ? -1 : mDB.mPhysicsInfoMap[id]); }
      int            getUniqueProtoObjectID(int id) const;
      int            getUniqueProtoSquadID(int id) const;

      template<class T> bool  remapCivID(T& id) const { if (id<0 || id>=mDB.mCivMap.getNumber()) { id=-1; return false; } id=(T)(mDB.mCivMap[id]); return (id != -1); }
      template<class T> bool  remapLeaderID(T& id) const { if (id<0 || id>=mDB.mLeaderMap.getNumber()) { id=-1; return false; } id=(T)(mDB.mLeaderMap[id]); return (id != -1); }
      template<class T> bool  remapAbilityID(T& id) const { if (id<0 || id>=mDB.mAbilityMap.getNumber()) { id=-1; return false; } id=(T)(mDB.mAbilityMap[id]); return (id != -1); }
      template<class T> bool  remapProtoVisualID(T& id) const { if (id<0 || id>=mDB.mProtoVisualMap.getNumber()) { id=-1; return false; } id=(T)(mDB.mProtoVisualMap[id]); return (id != -1); }
      template<class T> bool  remapGrannyModelID(T& id) const { if (id<0 || id>=mDB.mGrannyModelMap.getNumber()) { id=-1; return false; } id=(T)(mDB.mGrannyModelMap[id]); return (id != -1); }
      template<class T> bool  remapGrannyAnimID(T& id) const { if (id<0 || id>=mDB.mGrannyAnimMap.getNumber()) { id=-1; return false; } id=(T)(mDB.mGrannyAnimMap[id]); return (id != -1); }
      template<class T> bool  remapTerrainEffectID(T& id) const { if (id<0 || id>=mDB.mTerrainEffectMap.getNumber()) { id=-1; return false; } id=(T)(mDB.mTerrainEffectMap[id]); return (id != -1); }
      template<class T> bool  remapImpactEffectID(T& id) const { if (id<0 || id>=mDB.mImpactEffectMap.getNumber()) { id=-1; return false; } id=(T)(mDB.mImpactEffectMap[id]); return (id != -1); }
      template<class T> bool  remapLightEffectID(T& id) const { if (id<0 || id>=mDB.mLightEffectMap.getNumber()) { id=-1; return false; } id=(T)(mDB.mLightEffectMap[id]); return (id != -1); }
      template<class T> bool  remapParticleEffectID(T& id) const { if (id<0 || id>=mDB.mParticleEffectMap.getNumber()) { id=-1; return false; } id=(T)(mDB.mParticleEffectMap[id]); return (id != -1); }
      template<class T> bool  remapProtoObjectID(T& id) const;
      template<class T> bool  remapProtoSquadID(T& id) const;
      template<class T> bool  remapProtoTechID(T& id) const { if (id<0 || id>=mDB.mProtoTechMap.getNumber()) { id=-1; return false; } id=(T)(mDB.mProtoTechMap[id]); return (id != -1); }
      template<class T> bool  remapProtoPowerID(T& id) const { if (id<0 || id>=mDB.mProtoPowerMap.getNumber()) { id=-1; return false; } id=(T)(mDB.mProtoPowerMap[id]); return (id != -1); }
      template<class T> bool  remapResourceID(T& id) const { if (id<0 || id>=mDB.mResourceMap.getNumber()) { id=-1; return false; } id=(T)(mDB.mResourceMap[id]); return (id != -1); }
      template<class T> bool  remapRateID(T& id) const { if (id<0 || id>=mDB.mRateMap.getNumber()) { id=-1; return false; } id=(T)(mDB.mRateMap[id]); return (id != -1); }
      template<class T> bool  remapPopID(T& id) const { if (id<0 || id>=mDB.mPopMap.getNumber()) { id=-1; return false; } id=(T)(mDB.mPopMap[id]); return (id != -1); }
      template<class T> bool  remapPlacementRuleID(T& id) const { if (id<0 || id>=mDB.mPlacementRuleMap.getNumber()) { id=-1; return false; } id=(T)(mDB.mPlacementRuleMap[id]); return (id != -1); }
      bool                    remapProtoActionID(BProtoObjectID newProtoObjectID, int& saveProtoActionID) const;
      bool                    remapWeaponID(BProtoObjectID newProtoObjectID, int& saveWeaponID) const;
      template<class T> bool  remapObjectType(T& type) const { if (type<0 || type>=mDB.mObjectTypeMap.getNumber()) { type=-1; return false; } type=(T)(mDB.mObjectTypeMap[type]); return (type != -1); }
      template<class T> bool  remapWeaponType(T& type) const { if (type<0 || type>=mDB.mWeaponTypeMap.getNumber()) { type=-1; return false; } type=(T)(mDB.mWeaponTypeMap[type]); return (type != -1); }
      template<class T> bool  remapDamageType(T& type) const { if (type<0 || type>=mDB.mDamageTypeMap.getNumber()) { type=-1; return false; } type=(T)(mDB.mDamageTypeMap[type]); return (type != -1); }
      template<class T> bool  remapDamageTemplate(T& id) const { if (id<0 || id>=mDB.mDamageTemplateMap.getNumber()) { id=-1; return false; } id=(T)(mDB.mDamageTemplateMap[id]); return (id != -1); }
      template<class T> bool  remapAnimType(T& type) const { if (type<0 || type>=mDB.mAnimTypeMap.getNumber()) { type=-1; return false; } type=(T)(mDB.mAnimTypeMap[type]); return (type != -1); }
      template<class T> bool  remapAttachmentType(T& type) const { if (type<0 || type>=mDB.mAttachmentTypeMap.getNumber()) { type=-1; return false; } type=(T)(mDB.mAttachmentTypeMap[type]); return (type != -1); }
      template<class T> bool  remapActionType(T& type) const { if (type<0 || type>=mDB.mActionTypeMap.getNumber()) { type=BAction::cActionTypeInvalid; return false; } type=(T)(mDB.mActionTypeMap[type]); return (type != BAction::cActionTypeInvalid); }
      template<class T> bool  remapShapeID(T& id) const { if (id<0 || id>=mDB.mShapeMap.getNumber()) { id=-1; return false; } id=(T)(mDB.mShapeMap[id]); return (id != -1); }
      template<class T> bool  remapPhysicsInfoID(T& id) const { if (id<0 || id>=mDB.mPhysicsInfoMap.getNumber()) { id=-1; return false; } id=(T)(mDB.mPhysicsInfoMap[id]); return (id != -1); }
      template<class T> bool  remapProtoObjectIDs(T& list) const;
      template<class T> bool  remapProtoSquadIDs(T& list) const;
      template<class T> bool  remapProtoTechIDs(T& list) const;
      template<class T> bool  remapProtoPowerIDs(T& list) const;
      template<class T> bool  remapObjectTypes(T& list) const;

      int            getNumberCivs() const { return mDB.mCivMap.getNumber(); }
      int            getNumberLeaders() const { return mDB.mLeaderMap.getNumber(); }
      int            getNumberAbilities() const { return mDB.mAbilityMap.getNumber(); }
      int            getNumberProtoVisuals() const { return mDB.mProtoVisualMap.getNumber(); }
      int            getNumberGrannyModels() const { return mDB.mGrannyModelMap.getNumber(); }
      int            getNumberGrannyAnims() const { return mDB.mGrannyAnimMap.getNumber(); }
      int            getNumberTerrainEffects() const { return mDB.mTerrainEffectMap.getNumber(); }
      int            getNumberImpactEffects() const { return mDB.mImpactEffectMap.getNumber(); }
      int            getNumberLightEffects() const { return mDB.mLightEffectMap.getNumber(); }
      int            getNumberParticleEffects() const { return mDB.mParticleEffectMap.getNumber(); }
      int            getNumberProtoObjects() const { return mDB.mProtoObjectMap.getNumber(); }
      int            getNumberProtoSquads() const { return mDB.mProtoSquadMap.getNumber(); }
      int            getNumberProtoTechs() const { return mDB.mProtoTechMap.getNumber(); }
      int            getNumberProtoPowers() const { return mDB.mProtoPowerMap.getNumber(); }
      int            getNumberResources() const { return mDB.mResourceMap.getNumber(); }
      int            getNumberRates() const { return mDB.mRateMap.getNumber(); }
      int            getNumberPops() const { return mDB.mPopMap.getNumber(); }
      int            getNumberPlacementRules() const { return mDB.mPlacementRuleMap.getNumber(); }
      int            getNumberObjectTypes() const { return mDB.mObjectTypeMap.getNumber(); }
      int            getNumberWeaponTypes() const { return mDB.mWeaponTypeMap.getNumber(); }
      int            getNumberDamageTypes() const { return mDB.mDamageTypeMap.getNumber(); }
      int            getNumberDamageTemplates() const { return mDB.mDamageTemplateMap.getNumber(); }
      int            getNumberAnimTypes() const { return mDB.mAnimTypeMap.getNumber(); }
      int            getNumberAttachmentTypes() const { return mDB.mAttachmentTypeMap.getNumber(); }
      int            getNumberActionTypes() const { return mDB.mActionTypeMap.getNumber(); }
      int            getNumberShapes() const { return mDB.mShapeMap.getNumber(); }
      int            getNumberPhysicsInfos() const { return mDB.mPhysicsInfoMap.getNumber(); }

      void           logStats(const char* pName);

      bool           prepCampaignLoad();

      bool           isCampaignLoad() const { return mIsCampaignLoad; }

      bool           getSavedCampaign() const { return mSavedCampaign; }
      void           setSavedCampaign(bool val) { mSavedCampaign = val; }

      void           postUISetupFixup();
                     
      bool           getSaveFileCorrupt() const { return mSaveFileCorrupt; }

      bool           getLoadDeviceRemove() const { return mLoadDeviceRemove; }
      void           setLoadDeviceRemove(bool val) { mLoadDeviceRemove = val; }

      DWORD          getOverlappedResult(XOVERLAPPED* pOverlapped, bool* pWaitDialog);

      GFDECLAREVERSION();

   protected:
      bool           saveVersions();
      bool           saveDB();
      bool           saveSetup();
      bool           saveData();
      bool           saveEntities();

      bool           loadVersions();
      bool           loadDB();
      bool           loadEntities();

      bool           setupUser();
      void           doLoadCampaignGame();
      void           padSaveFile();

      BSaveDB        mDB;
      BSaveUser      mUser;

      BSmallDynamicSimArray<BSavePlayer>  mPlayers; 
      BSmallDynamicSimArray<BSaveTeam>    mTeams;

      int64          mTimerFrequency;
      double         mTimerFrequencyFloat;
      int64          mStartTime;
      int64          mLastTime;

      XCONTENTDEVICEID mDoSaveDeviceID;

      int            mSaveFileType;

      BSimString     mSaveFileName;

      XCONTENT_DATA  mCampaignContentData;

      bool           mIsSaving:1;
      bool           mIsLoading:1;
      bool           mDoSaveGame:1;
      bool           mDoLoadGame:1;
      bool           mDoAutoQuit:1;
      bool           mDoSaveCampaign:1;
      bool           mDoSaveGameSuccess:1;
      bool           mSavedCampaign:1;
      bool           mAutoQuitAfterSave:1;
      bool           mIsCampaignLoad:1;
      bool           mDoLoadCampaignGame:1;
      bool           mSaveFileCorrupt:1;
      bool           mLoadDeviceRemove:1;
};

extern BSaveGame gSaveGame;

//==============================================================================
// In-line methods
//==============================================================================
#include "savegame.inl"

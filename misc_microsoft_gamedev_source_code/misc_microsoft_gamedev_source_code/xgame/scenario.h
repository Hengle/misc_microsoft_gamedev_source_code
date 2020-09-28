//==============================================================================
// scenario.h
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================
#pragma once

// xgame
#include "cost.h"
#include "civ.h"
#include "leaders.h"
#include "simtypes.h"
#include "set.h"

// xcore
#include "containers\hashTable.h"

// Forward declarations
class BGameSettings;
class BObject;
class BProtoObject;
class BXMLWriter;

enum { cMaxUnitStartObjects=4 };

//==============================================================================
// BScenarioPosition
//==============================================================================
class BScenarioPosition
{
   public:
      BScenarioPosition() : mNumber(-1), mPosition(cOriginVector), mForward(cZAxisVector), mDefaultCamera(true), mCameraYaw(0.0f), mCameraPitch(0.0f), mCameraZoom(0.0f), mPlayerID(-1), mRallyStartObject(-1), mUsed(false)
      {
         for (uint i=0; i<cMaxUnitStartObjects; i++)
            mUnitStartObjects[i]=-1;
      }
      BScenarioPosition(const BScenarioPosition& source) { *this=source; }
      BScenarioPosition& operator=(const BScenarioPosition& source)
      {
         if(this==&source)
            return *this;
         mNumber=source.mNumber;
         mPosition=source.mPosition;
         mForward=source.mForward;
         mDefaultCamera=source.mDefaultCamera;
         mCameraYaw=source.mCameraYaw;
         mCameraPitch=source.mCameraPitch;
         mCameraZoom=source.mCameraZoom;
         mPlayerID=source.mPlayerID;
         mRallyStartObject=source.mRallyStartObject;
         for (uint i=0; i<cMaxUnitStartObjects; i++)
            mUnitStartObjects[i]=source.mUnitStartObjects[i];
         mUsed=source.mUsed;
         return *this;
      }
      long     mNumber;
      BVector  mPosition;
      BVector  mForward;
      float    mCameraYaw;
      float    mCameraPitch;
      float    mCameraZoom;
      long     mPlayerID;
      int      mRallyStartObject;
      int      mUnitStartObjects[cMaxUnitStartObjects];
      bool     mDefaultCamera;
      bool     mUsed;
};
typedef BDynamicSimArray<BScenarioPosition> BScenarioPositionArray;

//==============================================================================
// BScenarioPlayer
//==============================================================================
class BScenarioPlayer
{
   public:
      BScenarioPlayer() : mName(), mCivID(1), mLeaderID(-1), mColor(-1), mTeam(-1), mResources(), mUseStartingUnits(false), mControllable(true), mDefaultResources(true), mUsePlayerSettings(false), mPositionIndex(-1), mPosition(cInvalidVector), mForward(cZAxisVector), mActive(false), mDefaultCamera(true), mCameraYaw(0.0f), mCameraPitch(0.0f), mCameraZoom(0.0f), mRallyStartObject(-1) {}
      BScenarioPlayer(const BScenarioPlayer& source) { *this=source; }
      BScenarioPlayer& operator=(const BScenarioPlayer& source)
      {
         if(this==&source)
            return *this;
         mName=source.mName;
         mLocalisedDisplayName=source.mLocalisedDisplayName;
         mCivID=source.mCivID;
         mLeaderID=source.mLeaderID;
         mColor=source.mColor;
         mTeam=source.mTeam;
         mResources=source.mResources;
         mUseStartingUnits=source.mUseStartingUnits;
         mControllable=source.mControllable;
         mDefaultResources=source.mDefaultResources;
         mUsePlayerSettings=source.mUsePlayerSettings;
         mPositionIndex=source.mPositionIndex;
         mPosition=source.mPosition;
         mForward=source.mForward;
         mActive=source.mActive;
         mDefaultCamera=source.mDefaultCamera;
         mCameraYaw=source.mCameraYaw;
         mCameraPitch=source.mCameraPitch;
         mCameraZoom=source.mCameraZoom;
         mRallyStartObject=source.mRallyStartObject;
         for (uint i=0; i<cMaxUnitStartObjects; i++)
            mUnitStartObjects[i]=source.mUnitStartObjects[i];
         mForbidObjects.empty();
         for(long i=0; i<source.mForbidObjects.getNumber(); i++)
            mForbidObjects.add(source.mForbidObjects[i]);
         mForbidSquads.empty();
         for(long i=0; i<source.mForbidSquads.getNumber(); i++)
            mForbidSquads.add(source.mForbidSquads[i]);
         mForbidTechs.empty();
         for(long i=0; i<source.mForbidTechs.getNumber(); i++)
            mForbidTechs.add(source.mForbidTechs[i]);
         mLeaderPops.empty();
         for(long i=0; i<source.mLeaderPops.getNumber(); i++)
            mLeaderPops.add(source.mLeaderPops[i]);
         return *this;
      }
      bool operator<(const BScenarioPlayer& b) const
      {
         return false;
      }
      bool operator==(const BScenarioPlayer& b) const
      {
         return(this==&b);
      }
      BSimString  mName;
      BUString mLocalisedDisplayName;
      long     mCivID;
      long     mLeaderID;
      long     mColor;
      long     mTeam;
      BCost    mResources;
      bool     mUseStartingUnits;
      bool     mControllable;
      bool     mDefaultResources;
      bool     mUsePlayerSettings;
      long     mPositionIndex;
      BVector  mPosition;
      BVector  mForward;
      bool     mActive;
      bool     mDefaultCamera;
      float    mCameraYaw;
      float    mCameraPitch;
      float    mCameraZoom;
      int      mRallyStartObject;
      int      mUnitStartObjects[cMaxUnitStartObjects];
      BSmallDynamicSimArray<long> mForbidObjects;
      BSmallDynamicSimArray<long> mForbidSquads;
      BSmallDynamicSimArray<long> mForbidTechs;
      BSmallDynamicSimArray<BLeaderPop> mLeaderPops;
};
typedef BDynamicSimArray<BScenarioPlayer> BScenarioPlayerArray;

//==============================================================================
// BScenarioStartUnitPos
//==============================================================================
class BScenarioStartUnitPos
{
   public:
      BScenarioStartUnitPos() : mID(-1), mPosition(cOriginVector) {}
      int mID;
      BVector mPosition;
};

typedef BDynamicSimArray<BScenarioStartUnitPos> BScenarioStartUnitPosArray;

//==============================================================================
// BScenario
//==============================================================================
class BScenario
#ifdef ENABLE_RELOAD_MANAGER
   : public BEventReceiver
#endif
{
   public:
      BScenario();
      ~BScenario();
      
      bool save(const char*  pFileName);
      bool load(bool reload, bool leaveCamera, bool leaveRecordGame=false);
      bool unload();

      BEntityID getEntityIDFromScenarioObjectID(long scenarioObjectID) const;
      BPlayerID getPlayerIDFromScenarioPlayerID(BPlayerID scenarioPlayerID) const;
      
      // This saves the current scenario's SH fill light params to a FLS file.
      bool saveSHFillLight(const char* pFilename) const;
      void reloadLightSet(void);

      uint getNumTriggerScriptIDs() const { return mTriggerScriptIDs.size(); }
      BTriggerScriptID getTriggerScriptID(uint index) const { return mTriggerScriptIDs[index]; }

      BManagedTextureHandle getMinimapTextureHandle() const { return mMinimapTextureHandle; }

      DWORD getReloadTime() const { return mReloadTime; }
      bool getFlagReload() const { return mFlagReload; }
      bool getFlagAllowVeterancy() const { return mFlagAllowVeterancy; }

      void loadSoundBanks();

   protected:
      bool loadTerrain(BXMLNode  xmlNode, const BSimString& scenarioDir, const BSimString& fileDir, const BFixedStringMaxPath& scenarioFileName);
      bool preloadPfxFiles(const char* pPfxFileListName);
      bool preloadVisFiles(const char* pVisFileListName);
      bool preloadTfxFiles(const char* pTfxFileListName);
      bool preloadShapeFiles();
      bool preloadUnits();
      bool unloadUnits();
      void loadObject(BXMLNode  xmlNode);
      void loadObjective(BXMLNode xmlNode);
      void loadMinimapData(BXMLNode xmlNode);
      void loadPosition(BXMLNode  xmlNode, BScenarioPositionArray& positionList);
      void loadRelationType(BXMLNode  xmlNode, BRelationType* pRelationType);
      void loadLight(BXMLNode  xmlNode);
      bool loadLightSet(long dirID, const BSimString& scenarioDir, const BSimString& name);
      bool loadUILightSet();
      bool loadLocalLightSet(BXMLNode  xmlNode, const BSimString& scenarioDir, const BSimString& fileDir);
      void loadPlayer(BXMLNode  xmlNode, BGameSettings* pSettings, BScenarioPlayerArray& playerList);
      void loadObjectGroup(BXMLNode  xmlNode);
      bool setupPlayers(BGameSettings*  pSettings, BScenarioPlayerArray& playerList, BScenarioPositionArray& positionList, BRelationType* pRelationType, bool leaveCamera, long playerPlacementType, long minGap, bool isCoop, long& localPlayerID, long& localPlayerID2, bool recordGame, bool playGame);
      void setupVoice();
      void setupPlayerStartingUnits(BGameSettings*  pSettings, BScenarioPlayerArray& playerList, bool isCoop, long localPlayerID, long localPlayerID2, BScenarioPositionArray& positionList, int gameType, bool leaveCamera);
      bool setupPlayerPositions(long playerPlacementType, long playerPlacementSpacing, BScenarioPlayerArray& playerList, BScenarioPositionArray& positionList);
      bool shufflePlayersTeamTogether(BScenarioPlayerArray& playerList, BSmallDynamicSimArray<long>& shuffledPlayers, long& shuffledTeamCount);
      void createObject(long protoID, long playerID, BVector position, BVector forward, BVector right, long count);
      void saveObject(BXMLWriter*  pWriter, BObject*  pObject, const BProtoObject*  pProtoObject, BVector position, BVector forward, BVector right, long count);
      bool loadCinematic(BXMLNode xmlNode, const BSimString& scenarioDir);
      bool loadTalkingHead(BXMLNode xmlNode, const BSimString& scenarioDir);
      bool loadOtherLightset(BXMLNode xmlNode, const BSimString& scenarioDir);
      long calcRandomCiv();
      long calcRandomLeader(long civ);
      void initDefaultMinimapTexture();
      long decode(BSimUString& locString);
      void initUGXOptions();

      BHashTable<BEntityID,long> mScenarioIDToEntityID;
      BPlayerID mScenarioIDToPlayerID[cMaximumSupportedPlayers];
      BScenarioStartUnitPosArray mUnitStarts;
      BScenarioStartUnitPosArray mRallyStarts;
      BEntityIDArray mGaiaSquadList;

      //-- Loaded sound banks
      BDynamicSimArray<BString> mScenarioSoundBankNames;
      BDynamicSimArray<uint32> mScenarioSoundBankIDs;
      
      BSimString mLightSetScenarioDir;
      BSimString mLightSetName;
      BManagedTextureHandle mMinimapTextureHandle;
      long mLightSetDirID;

      BSmallDynamicSimArray<BTriggerScriptID> mTriggerScriptIDs;

      BHashTable<int16,int16> mScenarioGroupIdToGroupId;

      void reloadLightSetCallback(const BString& path, uint data, uint id);
      void reloadUILightSetCallback(const BString& path, uint data, uint id);
#ifdef ENABLE_RELOAD_MANAGER
      virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);
#endif
      DWORD mReloadTime;

      bool mFlagAllowVeterancy : 1;
      bool mFlagReload : 1;
};

extern BScenario gScenario;
extern bool gScenarioLoad;

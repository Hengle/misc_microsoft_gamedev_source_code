//==============================================================================
// statsmanager.h
//
// Copyright (c) Ensemble Studios, 2007-2008
//==============================================================================
#pragma once

// Includes
#include "common.h"
#include "database.h"
#include "protoobject.h"
#include "player.h"
#include "gamefilemacros.h"

// xcore
#include "stream\dynamicStream.h"

// xnetwork
#include "xnetwork.h"

class BEntity;
class BPlayer;
class BProtoObject;
class BProtoTech;
class BStatsManager;
class BStatRecorder;

extern BStatsManager gStatsManager;

enum BStatType
{
   eUnknown = 0,
   eTotal,
   eEvent,
   eGraph,
   eResourceGraph,
   ePopGraph,
   eBaseGraph,
   eScoreGraph,
};

struct BStatLostDestroyed
{
   uint32 mLost;
   uint32 mDestroyed;

   BStatLostDestroyed(uint32 lost, uint32 destroyed) :
      mLost(lost),
      mDestroyed(destroyed)
   { }
};

class BStatGraphDefinition
{
   public:
      DWORD  mLastUpdateTime;
      int16  mIndex;
      uint16 mSize;
      uint16 mSamples;
      uint16 mInterval;
      uint16 mOriginalInterval;
      int8   mStep;
      bool   mUpload : 1;

      BStatGraphDefinition():
         mLastUpdateTime(0),
         mIndex(0),
         mSize(0),
         mSamples(0),
         mInterval(0),
         mOriginalInterval(0),
         mStep(0),
         mUpload(true)
         {}

      void update();
      void reset();
};

namespace BStats
{
   enum
   {
      cMaxPlayers = 8,
      cUploadVersion = 1,
   };

   enum BGameType
   {
      eUnknown = 0,
      eCustom,
      eMatchMaking,
   };
}

typedef BHashMap<long, BStatLostDestroyed> BStatProtoIDHashMap;
typedef BHashMap<BProtoPowerID, uint> BStatProtoPowerIDHash;
typedef BHashMap<long, uint> BStatAbilityIDHashMap;
typedef BHashMap<long, long> BStatIDHashMap; // translates one ID to another

typedef BSmallDynamicSimArray<BStatRecorder*> BStatRecorderArray;

//==============================================================================
// 
//==============================================================================
class BStatDefinition : public IPoolable
{
   friend class BStatsManager;
   friend class BStatsPlayer;
   friend class BStatRecorder;
   friend class BStatRecorderTotal;
   friend class BStatRecorderEvent;
   friend class BStatRecorderGraph;

   public:

      BStatDefinition();
      ~BStatDefinition();

      virtual void onAcquire() {}
      virtual void onRelease();
      DECLARE_FREELIST(BStatDefinition, 4);

      void init(uint statID, BStatType type) { mID = static_cast<uint8>(statID); mType = type; }

      const uint8& getID() const { return mID; }

      const BString& getName() const { return mName; }

      const bool uploadStat() const { return mUpload; }

#ifndef BUILD_FINAL
      uint32 mMemUsage;
      bool   mMemDirty : 1;
      uint32 getMemUsage();
#endif

   protected:

      BDynamicSimLongArray mTechs;
      BDynamicSimLongArray mTechsExclude;
      BString              mName;

      // only tracking types from objecttypes.xml
      BBitArray mAbstractTypesAND;
      BBitArray mAbstractTypesOR;
      BBitArray mAbstractTypesNOT;

      BStatDefinition* mpKillers;

      long mObjectClass;
      long mDBID;

      BStatType mType;

      // for graphing
      uint16 mSize;
      uint16 mSamples;
      uint16 mInterval;

      uint8 mID;

      bool mFilterObjectClass       : 1;
      bool mFilterObjectType        : 1;
      bool mFilterObjectTypeAND     : 1;
      bool mFilterObjectTypeOR      : 1;
      bool mFilterObjectTypeNOT     : 1;
      bool mFilterDBID              : 1;
      bool mFilterExcludeTechs      : 1;
      bool mTrackAllTechs           : 1; // track all researched techs unless specifically excluded
      bool mTrackSquads             : 1;
      bool mTrackKillers            : 1;
      bool mTrackIndividual         : 1;
      bool mTrackFirstLastGameTime  : 1;
      bool mTrackTimeZero           : 1;

      bool mEventResearched         : 1;
      bool mEventBuilt              : 1;
      bool mEventLost               : 1;
      bool mEventDestroyed          : 1;
      bool mEventCombat             : 1;

      bool mEventDestroyedTarget    : 1;

      bool mUpload                  : 1;
};

//==============================================================================
// 
//==============================================================================
class BStatRecorder
{
   public:

      BStatRecorder();
      virtual ~BStatRecorder();

      virtual void onRelease();

      virtual void init(BStatDefinition* pDef, BStatIDHashMap* pObjectIDMap, BStatIDHashMap* pSquadIDMap);

      uint8 getID() const { return mpDef->mID; }
      BStatType getType() const { return mpDef->mType; }

      bool trackResearched() const { return mpDef->mEventResearched; }
      bool trackBuilt() const { return mpDef->mEventBuilt; }
      bool trackLost() const { return mpDef->mEventLost; }
      bool trackDestroyed() const { return mpDef->mEventDestroyed; }
      bool trackDestroyedTarget() const { return mpDef->mEventDestroyedTarget; }
      bool trackCombat() const { return mpDef->mEventCombat; }

      virtual void update(DWORD timeNow) {}

      virtual void eventTechResearched(BProtoTech* pProtoTech, uint32 gameTime) {}
      virtual void eventBuilt(BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, uint32 gameTime) {}
      virtual void eventLost(BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, long killerPlayerID, BProtoObject* pKillerProtoObject, BProtoSquad* pKillerProtoSquad, float xp, long level, uint32 gameTime) {}
      virtual void eventDestroyed(long playerID, BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, BProtoObject* pKillerProtoObject, BProtoSquad* pKillerProtoSquad, uint32 gameTime) {}

      virtual void updateFinalStats(BPlayerID playerID) {}

      
      virtual bool write(BPlayer* pPlayer, BDynamicStream& stream) const { return true; }

      const BStatDefinition* getStatDefiniion() const { return (mpDef); }

      struct BStatCombat
      {
         // need a dynamic array because I don't know how many levels we'll end up having
         BSmallDynamicSimArray<long> mLevels;
         float mXP;

         BStatCombat() :
            mXP(0.0f)
            {
            }
         BStatCombat(float xp) :
            mXP(xp)
            {
            }

         bool save(BStream* pStream, int saveType) const;
         bool load(BStream* pStream, int saveType);
      };

      struct BStatTotal
      {
         int32 mKillerIDs[BStats::cMaxPlayers];

         union
         {
            uint32 mBuilt;
            uint32 mResearched;
         };
         uint32 mLost;
         uint32 mDestroyed;

         uint32 mMax;

         int32 mCombatID;
         //int32 mKillersID;

         uint32 mFirstTime;
         uint32 mLastTime;

         BStatTotal() :
            mBuilt(0),
            mLost(0),
            mDestroyed(0),
            mMax(0),
            mCombatID(-1),
            mFirstTime(0),
            mLastTime(0)
            {
               Utils::FastMemSet(mKillerIDs, -1, sizeof(mKillerIDs));
            }
         BStatTotal(uint32 built, uint32 lost, uint32 destroyed, uint32 gameTime) :
            mBuilt(built),
            mLost(lost),
            mDestroyed(destroyed),
            mMax(Math::Max<uint32>(0, built-lost)),
            mCombatID(-1),
            mFirstTime(gameTime),
            mLastTime(gameTime)
            {
               Utils::FastMemSet(mKillerIDs, -1, sizeof(mKillerIDs));
            }

         void reset()
         {
            mBuilt = 0;
            mLost = 0;
            mDestroyed = 0;
            mMax = 0;
            mCombatID = -1;
            mFirstTime = 0;
            mLastTime = 0;
            Utils::FastMemSet(mKillerIDs, -1, sizeof(mKillerIDs));
         }

         virtual bool save(BStream* pStream, int saveType) const;
         virtual bool load(BStream* pStream, int saveType, BStatDefinition* pDef);
      };

      struct BStatEvent : BStatTotal
      {
         uint32 mTimestamp;
         long mProtoID;

         BStatEvent() :
            mTimestamp(0),
            mProtoID(cInvalidProtoID)
            {}

         virtual bool save(BStream* pStream, int saveType) const;
         virtual bool load(BStream* pStream, int saveType, BStatDefinition* pDef);
      };

      struct BStatResources
      {
         BCost  mCost;
         uint32 mTimestamp;

         BStatResources() :
            mTimestamp(0)
            {}

         bool save(BStream* pStream, int saveType) const;
         bool load(BStream* pStream, int saveType);
      };

      struct BStatEventFloat
      {
         float  mValue;
         uint32 mTimestamp;

         BStatEventFloat() :
            mValue(0.0f),
            mTimestamp(0)
            {}
      };

      struct BStatEventUInt32
      {
         uint32 mValue;
         uint32 mTimestamp;

         BStatEventUInt32() : 
            mValue(0),
            mTimestamp(0)
            {}
      };

      struct BStatEventInt32
      {
         int32 mValue;
         uint32 mTimestamp;

         BStatEventInt32() : 
            mValue(0),
            mTimestamp(0)
         {}
      };

      typedef BHashMap<long, BStatTotal> BStatTotalHashMap;

      //virtual bool collectAIStats(BPlayer* pPlayer, BStatTotalHashMap& rolledupSquadTotals, BStatTotalHashMap& rolledupTechTotals, BStatTotalHashMap& rolledupObjectTotals) const { return true; }

#ifndef BUILD_FINAL
      uint32 mMemUsage;
      bool   mMemDirty : 1;
      virtual uint32 getMemUsage();
#endif

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      bool checkObjectType(BStatDefinition* pDef, const BProtoObject* pProtoObject) const;
      long getMappedProtoObjectID(long protoID);
      long getMappedProtoSquadID(long protoID);

      BSmallDynamicSimArray<BStatProtoIDHashMap*> mKillers;
      BSmallDynamicSimArray<BStatCombat*> mCombat;

      BStatDefinition* mpDef;
      BStatIDHashMap* mpObjectIDMap;
      BStatIDHashMap* mpSquadIDMap;
};

//==============================================================================
// 
//==============================================================================
class BStatRecorderTotal : public BStatRecorder, IPoolable
{
   public:

      BStatRecorderTotal();
      ~BStatRecorderTotal();

      virtual void onAcquire();
      virtual void onRelease();
      DECLARE_FREELIST(BStatRecorderTotal, 4);

      void init(BStatDefinition* pDef, BStatIDHashMap* pObjectIDMap, BStatIDHashMap* pSquadIDMap);

      void eventTechResearched(BProtoTech* pProtoTech, uint32 gameTime);

      void eventBuilt(BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, uint32 gameTime);
      void eventLost(BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, long killerPlayerID, BProtoObject* pKillerProtoObject, BProtoSquad* pKillerProtoSquad, float xp, long level, uint32 gameTime);
      void eventDestroyed(long playerID, BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, BProtoObject* pKillerProtoObject, BProtoSquad* pKillerProtoSquad, uint32 gameTime);

      // combat event, triggered from eventLost
      void eventCombat(BStatTotal& total, float xp, long level);

      virtual void updateFinalStats(BPlayerID playerID);

      //bool collectAIStats(BPlayer* pPlayer, BStatTotalHashMap& rolledupSquadTotals, BStatTotalHashMap& rolledupTechTotals, BStatTotalHashMap& rolledupObjectTotals) const;
      bool write(BPlayer* pPlayer, BDynamicStream& stream) const;

      const BStatTotalHashMap& getTotalHashMap() const { return mTotalsHash; }

      long getLargestTotal(BStatTotal& total);

      const BStatTotal& getStatTotal() const { return (mTotal); }
      BStatTotal* getStatTotal2(long id)
      {
         BStatTotalHashMap::iterator it = mTotalsHash.find(id);
         if (it == mTotalsHash.end())
            return NULL;

         return &(it->second);
      }

      const BStatCombat* getStatCombat() const { return mpCombat; }

#ifndef BUILD_FINAL
      uint32 getMemUsage();
#endif

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      BStatTotalHashMap mTotalsHash;

      BStatTotal mTotal;
      BStatCombat* mpCombat;
};

//==============================================================================
// 
//==============================================================================
class BStatRecorderEvent : public BStatRecorder, IPoolable
{
   public:

      BStatRecorderEvent();
      ~BStatRecorderEvent();

      virtual void onAcquire();
      virtual void onRelease();
      DECLARE_FREELIST(BStatRecorderEvent, 4);

      void init(BStatDefinition* pDef, BStatIDHashMap* pObjectIDMap, BStatIDHashMap* pSquadIDMap);

      void eventTechResearched(BProtoTech* pProtoTech, uint32 gameTime);

      void eventBuilt(BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, uint32 gameTime);
      void eventLost(BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, long killerPlayerID, BProtoObject* pKillerProtoObject, BProtoSquad* pKillerProtoSquad, float xp, long level, uint32 gameTime);

      bool write(BPlayer* pPlayer, BDynamicStream& stream) const;

      const BStatTotal& getStatTotal() const;
      const BSmallDynamicSimArray<BStatEvent>& getEvents() const;

#ifndef BUILD_FINAL
      uint32 getMemUsage();
#endif

      virtual bool save(BStream* pStream, int saveType) const;
      virtual bool load(BStream* pStream, int saveType);

   protected:

      BStatTotal mTotal;
      BSmallDynamicSimArray<BStatEvent> mEvents;
};

//==============================================================================
// 
//==============================================================================
class BStatRecorderGraph : public BStatRecorder, IPoolable
{
   public:

      BStatRecorderGraph();
      ~BStatRecorderGraph();

      virtual void onAcquire();
      virtual void onRelease();
      DECLARE_FREELIST(BStatRecorderGraph, 4);

      void init(BStatDefinition* pDef, BStatIDHashMap* pObjectIDMap, BStatIDHashMap* pSquadIDMap);
      void update(DWORD timeNow);
      void eventTechResearched(BProtoTech* pProtoTech, uint32 gameTime);
      void eventBuilt(BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, uint32 gameTime);
      void eventLost(BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, long killerPlayerID, BProtoObject* pKillerProtoObject, BProtoSquad* pKillerProtoSquad, float xp, long level, uint32 gameTime);
      void eventDestroyed(long playerID, BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, BProtoObject* pKillerProtoObject, BProtoSquad* pKillerProtoSquad, uint32 gameTime);

      void query(uint32& minX, uint32& maxX, uint32& minY, uint32& maxY, BSmallDynamicSimArray<BStatRecorder::BStatEvent>& points);

      bool write(BPlayer* pPlayer, BDynamicStream& stream) const;

#ifndef BUILD_FINAL
      uint32 getMemUsage();
#endif

   protected:

      // basically, this will track N number of events
      BSmallDynamicSimArray<BStatEvent> mGraph;

      BStatEvent  mCurrentEvent;

      DWORD       mLastUpdateTime;

      uint16      mSize;
      uint16      mInterval;
      int16       mIndex;
      int8        mStep;
};

//==============================================================================
// 
//==============================================================================
class BStatsPlayer : public IPoolable
{
   friend class BStatsManager;

   public:

                  BStatsPlayer();
                  ~BStatsPlayer();

      virtual void onAcquire();
      virtual void onRelease();
      DECLARE_FREELIST(BStatsPlayer, 2);

      void        init(BPlayer* pPlayer);
      void        initResourceGraph(BStatDefinition* pDef);
      void        initPopGraph(BStatDefinition* pDef);
      void        initBaseGraph(BStatDefinition* pDef);
      void        initScoreGraph(BStatDefinition* pDef);
      void        initAbilities(BStatDefinition* pDef);
      void        setTeamID(BTeamID id) { mTeamID = id; }
      void        setRandomCiv() { mRandomCiv = true; }
      void        setRandomLeader() { mRandomLeader = true; }

      BPlayer*    getPlayer() const { return mpPlayer; }
      BPlayerID   getPlayerID() const { return mPlayerID; }

      int32       getStrength() const { return (mpPlayer ? mpPlayer->getStrength() : 0); }
      // total game time at the top of the strength curve
      uint32      getMaxStrengthTime() const { return mStrengthTime; }

      const BCost& getTotalResources() const { return mTotalResources; }
      const BCost& getMaxResources() const { return mMaxResources; }
      const BCost& getGatheredResources() const { return mGatheredResources; }
      const BCost& getTributedResources() const { return mTributedResources; }

      void        addResource(long resourceID, float amount, uint flags);

      void        addLeaderPower(BProtoPowerID protoPowerID);
      void        usedLeaderPower(BProtoPowerID protoPowerID);

      void        stateChanged();

      bool        isResigned() const { return mResigned; }
      bool        isDefeated() const { return mDefeated; }
      bool        isDisconnected() const { return mDisconnected; }
      bool        isWon() const { return mWon; }
      DWORD       getPlayerStateTime() const { return mPlayerStateTime; }
      uint8       getPlayerType() const { return mPlayerType; }

      void        update(DWORD timeNow, int32 maxStrength);

      void        eventTechResearched(long techID, uint32 gameTime);
      void        eventTechResearched(BProtoTech* pProtoTech, uint32 gameTime);

      void        eventBuilt(BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, uint32 gameTime);
      void        eventBuilt(BEntity* pEntity, uint32 gameTime);

      void        eventLost(BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, long killerPlayerID, BProtoObject* pKillerProtoObject, BProtoSquad* pKillerProtoSquad, float xp, long level, uint32 gameTime);
      void        eventLost(BEntity* pEntity, BEntity* pKillerEntity, uint32 gameTime);

      void        eventDestroyed(long playerID, BProtoObject* pProtoObject, BProtoSquad* pProtoSquad, BProtoObject* pKillerProtoObject, BProtoSquad* pKillerProtoSquad, uint32 gameTime);
      void        eventDestroyed(BEntity* pEntity, BEntity* pKillerEntity, uint32 gameTime);

      void        eventAbility(long abilityID);

      void        queryMachineId();

      BStatRecorderGraph* getGraphByID(uint8 statID) const;

      const void  queryGraphByID(uint8 statID, uint32& minX, uint32& maxX, uint32& minY, uint32& maxY, BSmallDynamicSimArray<BStatRecorder::BStatEvent>& points) const;
      const void  queryResourceGraph(uint8& resourcesUsed, uint32& minX, uint32& maxX, float& minY, float& maxY, BSmallDynamicSimArray<BStatRecorder::BStatResources>& resourcePoints) const;
      const void  queryPopGraph(uint32& minX, uint32& maxX, float& minY, float& maxY, BSmallDynamicSimArray<BStatRecorder::BStatEventFloat>& popPoints) const;
      const void  queryBaseGraph(uint32& minX, uint32& maxX, float& minY, float& maxY, BSmallDynamicSimArray<BStatRecorder::BStatEventUInt32>& basePoints) const;
      const void  queryScoreGraph(uint32& minX, uint32& maxX, float& minY, float& maxY, BSmallDynamicSimArray<BStatRecorder::BStatEventInt32>& scorePoints) const;

      //bool        collectAIStats(BStatRecorder::BStatTotalHashMap& rolledupSquadTotals, BStatRecorder::BStatTotalHashMap& rolledupTechTotals, BStatRecorder::BStatTotalHashMap& rolledupObjectTotals) const;
      
      bool        write(BDynamicStream& stream, ULONGLONG machineId) const;


      // get a list of all the recorders
      const BStatRecorderArray&     getStatRecorders() const { return (mStatRecorders); }
      const BStatAbilityIDHashMap&  getAbilitiesUsed(void) const {return(mAbilitiesUsed);}
      const BStatProtoPowerIDHash&  getPowersUsed(void) const {return(mPowersUsed);}

      // get a recorder by name.
      BStatRecorder* getStatsRecorder(const char* pName) const;

#ifndef BUILD_FINAL
      uint32 mMemUsage;
      bool   mMemDirty : 1;
      uint32 getMemUsage();
#endif

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   private:

      BStatRecorderArray mStatRecorders;

      // special one-off resource graph per player
      // records the current total resources of the player
      BSmallDynamicSimArray<BStatRecorder::BStatResources> mResourceGraph;
      BSmallDynamicSimArray<BStatRecorder::BStatEventFloat> mPopGraph;
      BSmallDynamicSimArray<BStatRecorder::BStatEventUInt32> mBaseGraph;
      BSmallDynamicSimArray<BStatRecorder::BStatEventInt32> mScoreGraph;

      BStatProtoPowerIDHash mPowersUsed;
      BStatAbilityIDHashMap mAbilitiesUsed;

      // for graphing resources
      BStatGraphDefinition mResGraphDef;
      BStatGraphDefinition mPopGraphDef;
      BStatGraphDefinition mBaseGraphDef;
      BStatGraphDefinition mScoreGraphDef;

      BCost             mTotalResources;
      BCost             mMaxResources; // used as a resource max watermark
      BCost             mGatheredResources;
      BCost             mTributedResources;

      XUID              mXuid;

      BPlayer*          mpPlayer;
      BPlayerID         mPlayerID;
      BTeamID           mTeamID;

      DWORD             mPlayerStateTime; // time the player state changed

      long              mPopID;

      BPlayerState      mPlayerState;

      long              mDifficultyType;
      float             mDifficulty;

      uint32            mStrengthTime;
      uint32            mStrengthTimer;

      int32             mCivID;
      int32             mLeaderID;

      uint8             mResourcesUsed;

      uint8             mPlayerType;

      bool              mRandomCiv      : 1;
      bool              mRandomLeader   : 1;
      bool              mResigned       : 1;
      bool              mDefeated       : 1;
      bool              mDisconnected   : 1;
      bool              mWon            : 1;
};

//==============================================================================
// 
//==============================================================================
class BStatsManager
{
   public:
                           BStatsManager();
                           ~BStatsManager();

      bool                 setup();       // initialize the manager
      void                 shutdown(bool destruct=false);    // shutdown the manager and free all memory

      void                 init(bool liveGame); // initialize the manager for a new game
      void                 reset(bool newGame=false);             // reset all previously recorded stats

      void                 submitUserProfileStats(DWORD gameTime = 0, long winningTeamID=-1); //-- submit stats to the user profile under tight conditions.
      void                 submit(DWORD gameTime=0, long winningTeamID=-1); // submit the game log to eso

      // BPlayer will call createPlayer and store a reference
      // BStatsManager will maintain a list of all the players and handle the serialization
      BStatsPlayer*        createStatsPlayer(BPlayer* pPlayer);
      BStatsPlayer*        getStatsPlayer(BPlayerID playerID) const;
      int                  getNumberPlayers() { return mPlayers.getNumber(); }

      const void           queryGraphByPlayerIDAndStatID(BPlayerID playerID, uint8 statID, uint32& minX, uint32& maxX, uint32& minY, uint32& maxY, BSmallDynamicSimArray<BStatRecorder::BStatEvent>& points) const;
      const void           queryResourceGraphByPlayerID(BPlayerID playerID, uint8& resourcesUsed, uint32& minX, uint32& maxX, float& minY, float& maxY, BSmallDynamicSimArray<BStatRecorder::BStatResources>& resourcePoints) const;
      const void           queryPopGraphByPlayerID(BPlayerID playerID, uint32& minX, uint32& maxX, float& minY, float& maxY, BSmallDynamicSimArray<BStatRecorder::BStatEventFloat>& popPoints) const;
      const void           queryBaseGraphByPlayerID(BPlayerID playerID, uint32& minX, uint32& maxX, float& minY, float& maxY, BSmallDynamicSimArray<BStatRecorder::BStatEventUInt32>& basePoints) const;
      const void           queryScoreGraphByPlayerID(BPlayerID playerID, uint32& minX, uint32& maxX, float& minY, float& maxY, BSmallDynamicSimArray<BStatRecorder::BStatEventInt32>& scorePoints) const;

      void                 notify(DWORD eventType, long targetPlayerID, long targetProtoID, long targetProtoSquadID, long sourcePlayerID, long sourceProtoID, long sourceProtoSquadID, float xp, long level);
      void                 notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);
      void                 update();

      void                 setGameType(BStats::BGameType gameType);
      BStats::BGameType    getGameType() const { return mGameType; }

      void                 setPlayerID(uint clientID, BPlayerID playerID);
      void                 setDisconnect(uint clientID);

      void                 setOoS(uint fromClientID, uint withClientID, long checksumID);

#ifndef BUILD_FINAL
      uint32 mMemUsage;
      bool   mMemDirty : 1;
      uint32 getMemUsage();
#endif

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   private:

      bool                 load();        // load the xml configuration
      void                 loadMap(BStatIDHashMap& map, BXMLNode& root);
      void                 loadDefinition(BStatDefinition& statDef, BXMLNode& root);

      XNADDR                                     mXnAddr;

      BPlayerID                                  mClientToPlayerMap[XNetwork::cMaxClients];
      BOOL                                       mClientDisconnect[XNetwork::cMaxClients];

      struct BOoS
      {
         long mChecksumID;
         uint mFromClientID;
         uint mWithClientID;
         BOoS() :
            mChecksumID(-1),
            mFromClientID(UINT32_MAX),
            mWithClientID(UINT32_MAX)
         { }
         BOoS(uint fromClientID, uint withClientID, long checksumID) :
            mChecksumID(checksumID),
            mFromClientID(fromClientID),
            mWithClientID(withClientID)
         { }
      };

      BSmallDynamicSimArray<BStatsPlayer*>       mPlayers;
      BSmallDynamicSimArray<BStatDefinition*>    mStatDefinitions;
      BSmallDynamicSimArray<BStatRecorderGraph*> mGraphs;

      BSmallDynamicSimArray<BOoS>                mOoSReports;

      BSimString                                 mGameID;
      BSimString                                 mMapName;

      BSimString                                 mGamertag; // the gamertag that matches the xuid

      XUID                                       mXuid; // the xuid of the user that will be responsible for uploading the log
      DWORD                                      mControllerID; // the controller ID of the above user

      DWORD                                      mXNetGetTitleXnAddrFlags;

      BStatIDHashMap*                            mpObjectIDMap;
      BStatIDHashMap*                            mpSquadIDMap;

      BStats::BGameType                          mGameType;

      long                                       mGameMode;
      long                                       mSettingsGameType;

      bool                                       mDisabled : 1;
      bool                                       mLiveGame : 1;
};

//==============================================================================
// world.h
//
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "bitvector.h"
#include "maximumsupportedplayers.h"
#include "objectmanager.h"
#include "visualmanager.h"
#include "objectivemanager.h"
#include "designobjectsmanager.h"
#include "cinematicmanager.h"
#include "math\runningAverage.h"
#include "ChatManager.h"
#include "HintManager.h"
#include "containers\hashMap.h"
#include "gamefilemacros.h"
#include "binkvideo.h"
#include "customcommand.h"
#include "timer.h"

// Forward declarations
class BAction;
class BBid;
class BAIGroupTask;
class BAIMission;
class BAIMissionTarget;
class BAIMissionTargetWrapper;
class BAIManager;
class BAITeleporterZone;
class BAITopic;
class BFormation;
class BFormationManager;
class BCommandManager;
class BConvexHull;
class BEntity;
class BEntityGrouper;
class BLightVisualInstance;
class BObject;
class BObjectCreateParms;
class BObjectManager;
class BOPQuadHull;
class BPhysicsWorld;
class BPlayer;
class BTeam;
class BUnit;
class BUnitQuery;
class BPlatoon;
class BSquad;
class BDopple;
class BProjectile;
class BArmy;
class BWorld;
class BWorldSoundManager;
class BBattleManager;
class BSquadPlotter;
class BPathingLimiter;
class BProtoVisualTag;
class BVisual;
class IDamageInfo;
class BObjectiveManager;
class BDesignObjectManager;
class BCinematic;
class BTerrainImpactDecalHandle;
class BStoredAnimEventManager;
class BTactic;
class BPhysicsBreakOffObject;
class hkpShape;
class BTransitionManager;
class BPowerManager;
class BTimingTracker;
class BPhysicsGroundVehicleAction;

// Global pointer to the current world
extern BWorld* gWorld;
extern bool gWorldReset;

extern bool gEnableSubUpdating;

//#define DECOUPLED_UPDATE
extern bool gDecoupledUpdate;

//#define UNITOFFENDERS

//==============================================================================
//==============================================================================
void setWorldReset(bool val);

class BTalkingHead
{
public:
   BTalkingHead() : mID(-1), mPreloadedData(NULL), mPreload(false) {}
   
   // If we have preloaded data hanging around, that means we're in ownership of it and need to nuke it.
   ~BTalkingHead() {delete mPreloadedData;}

   BString  mFilename;
   int      mID;
   BByteArray *mPreloadedData;
   bool     mPreload:1;
};

//==============================================================================
// class BChildObjectData
//==============================================================================
class BChildObjectData
{
   public:
      int mChildType;
      int mObjectType;
      int mUserCiv;
      float mRotation;
      BVector mPosition;
      BSimString mAttachBoneName;
};

#ifndef BUILD_FINAL
//==============================================================================
// class BOffenderEntry
//==============================================================================
class BOffenderEntry
{
   public:
      BOffenderEntry() { mID.set(-1); mTime=0; }
      BOffenderEntry(BEntityID id, double time) { mID=id; mTime=time; } 
   
      BEntityID      mID;
      double         mTime;
};
typedef BSmallDynamicSimArray<BOffenderEntry> BOffenderArray;
#endif

//==============================================================================
// class BWorld
//==============================================================================
class BWorld :
   public IEventListener,
   public IVisualHandler
{
   public:

                              BWorld();
                              ~BWorld();

      bool                    setup();
      bool                    initTerrain(const char* terrainName);
      void                    initSky(const char* skyName);
      bool                    initPathing(const char* pathingName);
      void                    initCinematic(const char* cinematicName);
      void                    reset();
      
      void                    update(int64 currentUpdateTime, float currentUpdateLength, double timerFrequency, bool runningGame, BTimingTracker *tracker = NULL);

      void                    render();
      void                    renderVisualInstances(BUser* pUser);
      void                    debugRender();  
      void                    renderTerrainFlattenRegions();
      void                    renderParkingLots();

      BObject*                createObject( BObjectCreateParms & parms );
      BUnit*                  createUnit( BObjectCreateParms & parms );
      BSquad*                 createSquad(BObjectCreateParms& parms, BEntityIDArray* existingUnits=NULL);
      BPlatoon*               createPlatoon( BObjectCreateParms & parms );
      BArmy*                  createArmy( BObjectCreateParms & parms );
      BDopple*                createDopple( BObject *pObject, BTeamID teamID, bool goIdle );
      BProjectile*            createProjectile( BObjectCreateParms &parms);      
      BObject*                createTempObject(BObjectCreateParms& parms, long protoVisIndex, float lifespan, long userData, bool visibleToAll, int displayPriority);
      void                    createTerrainEffectInstance(const BProtoAction* pPA, BVector pos, BVector fwd, BPlayerID playerID, bool bVisibleToAll);
      void                    createImpactTerrainDecal(BVector pos,const BTerrainImpactDecalHandle *impactHandle,BVector forward=cZAxisVector);
      BObject*                createRevealer(BTeamID teamID, BVector position, float los, DWORD lifespan, float finalLOS);   
      BObject*                createRevealer(BTeamID teamID, BVector position, float los, DWORD lifespan); // Creates a revealer AND gives it a lifespan.
      BObject*                createRevealer(BTeamID teamID, BVector position, float los); // Creates a revealer only.
      BObject*                createBlocker(BTeamID teamID, BVector position, float los, DWORD lifespan); // Creates a blocker AND gives it a lifespan.
      BObject*                createBlocker(BTeamID teamID, BVector position, float los); // Creates a blocker only.
      BUnit*                  createVisItemPhysicsObject(BObjectCreateParms& params, BVisualItem* pVisItem, const BPhysicsInfo *pPhysicsInfo, const BPhysicsObjectBlueprintOverrides* pBPOverrides, bool bDisregardAttachments = false );
      BUnit*                  createVisItemPhysicsObjectDirect(BObjectCreateParms& params, BVisualItem* pVisItem, const BPhysicsObjectParams &physicsparams, bool bDisregardAttachments );
      BUnit*                  createVisPhysicsObjectDirect(BObjectCreateParms& params, BVisual* pSourceVisual, const BPhysicsObjectParams &physicsparams, bool bDisregardAttachments );
      bool                    validateEntityID(BEntityID id) const;
      bool                    validateEntityID(long id) const;

      BEntity*                getEntity( BEntityID id, bool anyCount = false);
      BEntity*                getEntity( long id, bool anyCount = false);
      BObject*                getObject( BEntityID id, bool anyCount = false );
      BObject*                getObject( long id, bool anyCount = false);
      BUnit*                  getUnit( BEntityID id, bool anyCount = false );
      BUnit*                  getUnit( long id, bool anyCount = false );
      BSquad*                 getSquad( BEntityID id, bool anyCount = false);
      BSquad*                 getSquad( long id, bool anyCount = false );
      BPlatoon*               getPlatoon( BEntityID id, bool anyCount = false);
      BPlatoon*               getPlatoon( long id, bool anyCount = false );
      BArmy*                  getArmy( BEntityID id, bool anyCount = false);
      BArmy*                  getArmy( long id, bool anyCount = false );
      BDopple*                getDopple( BEntityID id, bool anyCount = false);
      BDopple*                getDopple( long id, bool anyCount = false );
      BProjectile*            getProjectile( BEntityID id, bool anyCount = false);
      BProjectile*            getProjectile( long id, bool anyCount = false );      

      long                    getNumberEntities() const { return mpObjectManager->getNumberEntities(); }
      BEntity*                getNextEntity( BEntityHandle &handle ) { return mpObjectManager->getNextEntity(handle); }

      long                    getNumberObjects() const { return mpObjectManager->getNumberObjects(); }
      long                    getObjectsHighWaterMark() const { return mpObjectManager->getObjectsHighWaterMark(); }
      BObject*                getNextObject(BEntityHandle &handle) const { return mpObjectManager->getNextObject(handle); }
     
      long                    getNumberUnits() const     { return mpObjectManager->getNumberUnits(); }
      long                    getUnitsHighWaterMark() const { return mpObjectManager->getUnitsHighWaterMark(); }
      BUnit*                  getNextUnit(BEntityHandle &handle) const { return mpObjectManager->getNextUnit(handle); }

      long                    getNumberSquads() const  { return mpObjectManager->getNumberSquads(); }
      long                    getSquadsHighWaterMark() const { return mpObjectManager->getSquadsHighWaterMark(); }
      BSquad*                 getNextSquad(BEntityHandle &handle) const { return mpObjectManager->getNextSquad(handle); }

      long                    getNumberPlatoons() const  { return mpObjectManager->getNumberPlatoons(); }
      long                    getPlatoonsHighWaterMark() const { return mpObjectManager->getPlatoonsHighWaterMark(); }
      BPlatoon*               getNextPlatoon(BEntityHandle &handle) const { return mpObjectManager->getNextPlatoon(handle); }

      long                    getNumberArmies() const  { return mpObjectManager->getNumberArmies(); }
      long                    getArmiesHighWaterMark() const { return mpObjectManager->getArmiesHighWaterMark(); }
      BArmy*                  getNextArmy(BEntityHandle &handle) const { return mpObjectManager->getNextArmy(handle); }

      long                    getNumberDopples() const  { return mpObjectManager->getNumberDopples(); }
      long                    getDopplesHighWaterMark() const { return mpObjectManager->getDopplesHighWaterMark(); }
      BDopple*                getNextDopple(BEntityHandle &handle) const { return mpObjectManager->getNextDopple(handle); }

      long                    getNumberProjectiles() const { return mpObjectManager->getNumberProjectiles(); }
      long                    getProjectilesHighWaterMark() const { return mpObjectManager->getProjectilesHighWaterMark(); }
      BProjectile*            getNextProjectile(BEntityHandle &handle) const { return mpObjectManager->getNextProjectile(handle); }      
#ifndef BUILD_FINAL
      long                    getNumberProjectilesLaunchedThisFrame() const { return mLaunchedProjectilesPerFrame; }
      unsigned int            getNumberRenderedProjectilesLastFrame() const { return mRenderedProjectilesLastFrame; }
      unsigned int            getNumberRenderedUnitsLastFrame() const { return mRenderedUnitsLastFrame; }
      unsigned int            getNumberRenderedDopplesLastFrame() const { return mRenderedDopplesLastFrame; }
      unsigned int            getNumberRenderedObjectsLastFrame() const { return mRenderedObjectsLastFrame; }
#endif

      void                    releaseObject(BEntityID id);
      void                    releaseObject(BEntity* pEntity);
      void                    releaseObjects();

      //-- high-level entity creation (creates object, unit, or squad based on the proto)
      BEntityID               createEntity(long protoID, bool isSquadID, long playerID, BVector position, BVector forward, BVector right, bool startBuilt, bool bPhysicsReplacement=false, bool noTieToGround=false, BEntityID sourceVisual=cInvalidObjectID, BPlayerID createdByPlayerID=cInvalidPlayerID, BEntityID builtByUnitID=cInvalidObjectID, BEntityID socketUnitID=cInvalidObjectID, bool noCost=false, float aoTintValue=1.0f, bool appearsBelowDecals=false, int level=0, int visualVariationIndex = -1, int16 exploreGroup = -1, bool startExistSound = true);

      long                    getNumberPlayers() const { return mPlayers.getNumber(); }

      BPlayer*                getPlayer(long id);
      const BPlayer*          getPlayer(long id) const;
      BPlayer*                getPlayerByMPID(long mpid) const;
      BPlayer*                createPlayer(long mpid, BSimString& name, BUString& localisedName, long civID, long leaderID, float difficulty, BSmallDynamicSimArray<long>* pForbidObjects, BSmallDynamicSimArray<long>* pForbidSquads, BSmallDynamicSimArray<long>* pForbidTechs, BSmallDynamicSimArray<BLeaderPop>* pLeaderPops, uint8 playerType, int scenarioID, bool fromSave);

      long                    getNumberTeams() const { return mTeams.getNumber(); }
      BTeam*                  getTeam(BTeamID id);
      const BTeam*            getTeam(BTeamID id) const;
      BTeam*                  createTeam();

      bool                    addLight(BLightVisualInstance* pInstance);
      void                    releaseLights();

      BTalkingHead*           addTalkingHead(const char * filename, int talkingHeadID, bool preload = false);
      BBinkVideoHandle        playTalkingHead(int talkingHeadID, BCueIndex soundIndex, BBinkVideoStatus* pStatusCallback);
      int                     getTalkingHeadID(const char* pFileName) const;
      int                     getNextTalkingHeadID() const;
      void                    preloadTalkingHeadVideos();
      void                    releaseTalkingHeadVideo(int talkingHeadID);

      void                    addCinematic(BCinematic* pInstance, int cinematicID);
      void                    playCinematic(uint index);
      bool                    isPlayingCinematic() { return mpCinematicManager->isPlayingCinematic(); }

      BObjectManager*         getObjectManager() { return mpObjectManager; }
      BCommandManager*        getCommandManager() { return mpCommandManager; }
      BWorldSoundManager*     getWorldSoundManager() { return mpWorldSoundManager; }
      BObjectiveManager*      getObjectiveManager(){ return( mpObjectiveManager ); }
      BChatManager*           getChatManager() { return(mpChatManager);}
      BHintManager*           getHintManager() { return(mpHintManager);}
      BBattleManager*         getBattleManager(){ return mpBattleManager;}
      BPowerManager*          getPowerManager() { return (mpPowerManager); }
      BAIManager*             getAIManager() { return (mpAIManager); }
      BEntityGrouper*         getEntityGrouper() { return(mEntityGrouper);}
      BPathingLimiter*        getPathingLimiter();
      BDesignObjectManager*   getDesignObjectManager(){ return( mpDesignObjectManager ); }
      BCinematicManager*      getCinematicManager() { return mpCinematicManager; }
      BTransitionManager*     getTransitionManager() { return mpTransitionManager; }

      bool                    doCollisionCheck( BObject &object, BVector& velocity, BObject **pFirstHit = NULL) const;

      DWORD                   getUpdateNumber() const { return(mUpdateNumber); }
      int64                   getLastUpdateTime() const { return(mLastUpdateTime); }
      DWORD                   getLastUpdateLength() const { return(mLastUpdateLength); }
      float                   getLastUpdateLengthFloat() const { return(mLastUpdateLengthFloat); }
      DWORD                   getGametime() const { return(mGametime); }
      double                  getGametimeFloat() const { return(mGametimeFloat); }      
      double                  getTotalRealtime() const { return(mTotalRealtime); }
      double                  getLastUpdateRealtimeDelta() const { return(mLastUpdateRealtimeDelta); }
      int64                   getLastUpdateRealtime() const { return mLastUpdateRealtime; }
      double                  getActualRealtime() const;
      
      void                    setUpdateNumber(DWORD val) { mUpdateNumber=val; }
      void                    setLastUpdateTime(int64 val) { mLastUpdateTime=val; }
      void                    setLastUpdateLength(DWORD val) { mLastUpdateLength=val; }
      void                    setLastUpdateLengthFloat(float val) { mLastUpdateLengthFloat=val; }
      void                    setGametime(DWORD val) { mGametime=val; }
      void                    setGametimeFloat(double val) { mGametimeFloat=val; }
      void                    setTotalRealtime(double val) { mTotalRealtime=val; }
      void                    setLastUpdateRealtimeDelta(double val) { mLastUpdateRealtimeDelta=val; }
      void                    setLastUpdateRealtime(int64 val) { mLastUpdateRealtime=val; }
      float                   getLastTimerFrequency() { return (float)mLastTimerFrequency; }

      BPhysicsWorld*          getPhysicsWorld( void ) const { return mpPhysicsWorld; }

      const BSimString&       getTerrainName() const { return mTerrainName; }
      
      enum BPlayerColorContext
      {
         cPlayerColorContextObjects    = 0,  // 3D objects
         cPlayerColorContextSelection  = 1,  // 3D in-world selection circles/decals 
         cPlayerColorContextMinimap    = 2,  // 2D minimap
         cPlayerColorContextUI         = 3   // 2D UI screens (objective, post game stats)
      };

      DWORD                   getPlayerColor(long playerID, BPlayerColorContext context);
      DWORD                   getCorpseColor(long playerID, BPlayerColorContext context);
      
      void                    setPlayerColorCategory(BPlayerColorCategory category) { mPlayerColorCategory = category; }
      BPlayerColorCategory    getPlayerColorCategory() const { return mPlayerColorCategory; }
      
      void                    setPlayerColor(BPlayerColorCategory category, long playerID, long colorIndex);
                        
      BRelationType           getTeamRelationType(BTeamID team1, BTeamID team2);
      void                    setTeamRelationType(BTeamID team1, BTeamID team2, BRelationType relationType);

      // Check placement
      enum
      {
         cCPLOSDontCare,      // no LOS checks
         cCPLOSNormal,        // visible OR fog
         cCPLOSFullVisible    // area must be fully visible
      };
      enum
      {
         cCPRender                  = 1 << 1,
         cCPCheckObstructions       = 1 << 2,
         cCPIgnoreMoving            = 1 << 3,
         cCPExpandHull              = 1 << 4,
         cCPIgnorePlacementRules    = 1 << 5,
         cCPSetPlacementSuggestion  = 1 << 6,
         cCPEarlyExit               = 1 << 7,
         cCPPushOffTerrain          = 1 << 8,
         cCPLOSCenterOnly           = 1 << 9,  // Checks the LOS for the center point only
         cCPIgnoreNonSolid          = 1 << 10, // Ignores non-solid results
         cCPIgnoreParkingAndFlatten = 1 << 11, // Ignore expansion due to parking lot and flatten regions when doing placement suggestion
         cCPCheckSquadObstructions  = 1 << 12, // Includes squad obstructions in the obstruction check
         cCPIgnoreAirObstructions   = 1 << 13, // Ignore objects in the air obstruction quad trees
         //cCPSync                    = 1 << 9,                     // Not implemented yet
         //cCPIgnoreLand              = 1 << 10,                    // Not implemented yet
         //cCPIgnoreWater             = 1 << 12,                    // Not implemented yet
         //cCPIncludeNonCollideable   = 1 << 13,                    // Not implemented yet
         //cCPIgnoreSpecifiedTypes    = 1 << 14, // to allow overlaps with certain logical types         // Not implemented yet
         //cCPIgnoreCloakedUnits      = 1 << 15,                    // Not implemented yet
      };

      bool                    checkPlacement(const long protoID, const long playerID, BVector location, BVector& suggestion, const BVector forward, long losMode, DWORD flags, long searchScale = 1, const BUnit* pBuilder = NULL, BEntityID* pSocketID = NULL, long placementRuleIndex = -1, int protoSquadID = -1, bool useSquadObsRadii = false, float extraObstructionExpansion = 0.0f);
      bool                    moveSquadsFromObstruction(BEntityID obsSquad, BOPQuadHull* pObsHull, bool checkOnly, int tryCount, bool& failNow);
      bool                    movePlayerSquadsFromObstruction(BEntityID obstructionSquad, BOPQuadHull* obstructionHull, float testRadiusX, float testRadiusZ, bool& bEnemyInTheWay);      
      BVector                 getSquadPlacement(BUnit*  pContainingUnit, BUnit*  pResource, BProtoSquad*  pProtoSquad, BProtoObject*  pProto, BVector *pForward, BVector *pRight, long preferredBoneHandle, bool &gotPreferredPosition);
      bool                    getBuildSockets(BPlayerID playerID, int protoObjectID, const BUnit* pBuilder, BVector location, float range, bool onlyAvailable, bool ignoreLocation, BEntityIDArray& sockets);
      bool                    getChildObjects(BPlayerID playerID, int protoObjectID, BVector baseLocation, BVector baseDirection, BSmallDynamicSimArray<BChildObjectData>& childObjects);

      void                    notify2(DWORD eventType, long targetPlayerID, long targetProtoID, long targetProtoSquadID=cInvalidProtoID, long sourcePlayerID=-1, long sourceProtoID=-1, long sourceProtoSquadID=-1, float xp=0.0f, long level=-1);

      // IEventListener
      virtual void            notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);

      // IVisualHandler
      virtual bool            handleAnimEvent(long attachmentHandle, long animType, long eventType, int64 userData, BProtoVisualTag* pTag, BVisual* pVisual);
      virtual bool            handleCachedAnimEvent(long eventType, BProtoVisualTag* pTag, BVector EventLocation, void* userData);
      virtual void            flushCachedAnimEvents(BEntityID entity, bool forceFire = false);
      virtual bool            handleSetAnimSync(int64 userData, long animationTrack, long animType, bool applyInstantly, float timeIntoAnimation, long forceAnimID, bool reset, BVisualItem* startOnThisAttachment, const BProtoVisualAnimExitAction* pOverrideExitAction, BVisual* pVisual);
      virtual long            handleVisualLogic(BProtoVisualLogicNode* pLogicNode, long randomTag, int64 userData, BProtoVisual* pProtoVisual);
      virtual long            handleVisualPoint(long pointType, const BSimString& data);
      virtual long            getRandomValue(long randomTag, long minVal, long maxVal);

      // Returns number of units matching the query requirements that were written out to pResults
      long                    getUnitsInArea(BUnitQuery*  pUnitQuery, BEntityIDArray*  pResults, bool includeNonCollidableUnitObs = true) { return getEntitiesInArea(pUnitQuery, pResults, includeNonCollidableUnitObs, false); }
      // Returns number of squads matching the query requirements that were written out to pResults
      long                    getSquadsInArea(BUnitQuery*  pUnitQuery, BEntityIDArray*  pResults, bool includeNonCollidableUnitObs = true) { return getEntitiesInArea(pUnitQuery, pResults, includeNonCollidableUnitObs, true); }

      BVector                 getProjectileDeviation(BVector source, BVector target, BVector targetingLead, float accuracy, float maxRange, float maxDeviation, float accuracyDistanceFactor, float accuracyDeviationFactor) const;
      BEntityID               launchProjectile( BObjectCreateParms& parms, BVector targetOffset, BVector targetingLead, BVector projectileOrientation, float damage, const BProtoAction* pProtoAction, IDamageInfo* pDamageInfo, BEntityID parentID = cInvalidObjectID, BUnit* pTargetUnit = NULL, long hitZoneIndex = -1, bool doLogging = false, bool collideWithAllUnits=true, bool fromLeaderPower = false );
      void                    calculateProjectileLaunchDirection(const float maxRange, const BUnit* pAttacker, const BUnit *pTarget, const BProtoObject *pProjectileProtoObject, BVector launchLocation, BVector targetLocation, BVector &launchDirection, bool &useGravity, float &gravity) const;

      void                    checkForGameOver();

      bool                    isMultiplayerSim() const { return getFlagMultiplayerSim(); }

      bool                    getFlagQuickBuild() const { return(mFlagQuickBuild); }
      void                    setFlagQuickBuild(bool v) { mFlagQuickBuild=v; }

      bool                    getFlagGameOver() const { return(mFlagGameOver); }
      void                    setFlagGameOver(bool v) { mFlagGameOver=v; }

      bool                    adjustGameOverCountdown(float elapsed);

      bool                    getFlagSkipGameOverCheck() const { return(mFlagSkipGameOverCheck); }
      void                    setFlagSkipGameOverCheck(bool v) { mFlagSkipGameOverCheck=v; }

      bool                    getFlagMultiplayerSim() const { return(mFlagMultiplayerSim); }
      void                    setFlagMultiplayerSim(bool v) { mFlagMultiplayerSim=v; }

      bool                    getFlagCoop() const { return(mFlagCoop); }
      void                    setFlagCoop(bool v) { mFlagCoop=v; }

      bool                    getFlagNoFogSim() const { return mFlagNoFogSim; }
      bool                    getFlagNoFogVis() const { return mFlagNoFogVis; }
      bool                    getFlagShowSkirt() const { return mFlagShowSkirt; }
      bool                    getFlagAllVisible() const { return mFlagAllVisible; }

      void                    setFlagShowSkirt(bool v) { mFlagShowSkirt=v; }

      bool                    getFlagPlayableBounds() const { return mFlagPlayableBounds; }
      void                    setFlagPlayableBounds(bool v) { mFlagPlayableBounds=v; }
      bool                    isOutsidePlayableBounds(BVector pos, bool forceCheckWorldBoundaries=false) const;

      void                    initializeKBsAndAIs();
      void                    updateAllKBsForSquad(BSquad* pSquad);

      void                    setOccluded(BVector position, float radius, bool occluded);

      uint                    getWorldID() const { return mWorldID; }
      void                    setWorldID(uint val) { mWorldID = val; updateWorldSoundSwitch(); }
      void                    updateWorldSoundSwitch();

#ifndef BUILD_FINAL      
      class BStats
      {
         public:          
            BStats() {};
           ~BStats() {};

            enum 
            {
               cRunningAvgBuffer = 60,
            };

            void init()
            {
               clear();
               clearMax();
               mAvgTotalPreAsyncUpdateTime.set(cRunningAvgBuffer);
               mAvgTotalAsyncUpdateTime.set(cRunningAvgBuffer);
               mAvgTotalPostAsyncUpdateTime.set(cRunningAvgBuffer);
               mAvgSubUpdate1Time.set(cRunningAvgBuffer);
               mAvgSubUpdate2Time.set(cRunningAvgBuffer);
               mAvgSubUpdate3Time.set(cRunningAvgBuffer);
               mAvgTotalUpdateTime.set(cRunningAvgBuffer);

               mAvgPlayerPreAsyncTime.set(cRunningAvgBuffer);
               mAvgTeamPreAsyncTime.set(cRunningAvgBuffer);
               mAvgArmyPreAsyncTime.set(cRunningAvgBuffer);
               mAvgAIPreAsyncTime.set(cRunningAvgBuffer);
               mAvgKBPreAsyncTime.set(cRunningAvgBuffer);
               mAvgPlatoonPreAsyncTime.set(cRunningAvgBuffer);
               mAvgSquadPreAsyncTime.set(cRunningAvgBuffer);
               mAvgProjectilePreAsyncTime.set(cRunningAvgBuffer);
               mAvgUnitPreAsyncTime.set(cRunningAvgBuffer);
               mAvgDopplePreAsyncTime.set(cRunningAvgBuffer);
               mAvgObjectPreAsyncTime.set(cRunningAvgBuffer);

               mAvgCommandManagerUpdateTime.set(cRunningAvgBuffer);
               mAvgCommandManagerServiceTime.set(cRunningAvgBuffer);
               mAvgArmyUpdateTime.set(cRunningAvgBuffer);
               mAvgPlatoonUpdateTime.set(cRunningAvgBuffer);
               mAvgSquadUpdateTime.set(cRunningAvgBuffer);
               mAvgProjectileUpdateTime.set(cRunningAvgBuffer);
               mAvgUnitUpdateTime.set(cRunningAvgBuffer);
               mAvgDoppleUpdateTime.set(cRunningAvgBuffer);
               mAvgObjectUpdateTime.set(cRunningAvgBuffer);

               mAvgEntitySchedulerUpdateTime.set(cRunningAvgBuffer);
               mAvgPhysicsUpdateTime.set(cRunningAvgBuffer);
               mAvgTextVisualUpdateTime.set(cRunningAvgBuffer);
               mAvgSoundUpdateTime.set(cRunningAvgBuffer);
               mAvgBattleUpdateTime.set(cRunningAvgBuffer);
               mAvgPowerUpdateTime.set(cRunningAvgBuffer);
               mAvgTriggerUpdateTime.set(cRunningAvgBuffer);
               mAvgTriggerObjectsUpdateTime.set(cRunningAvgBuffer);
               mAvgSimRenderTime.set(cRunningAvgBuffer);
               mAvgSimRenderSquadsTime.set(cRunningAvgBuffer);
               mAvgSimRenderUnitsTime.set(cRunningAvgBuffer);
               mAvgSimRenderObjectsTime.set(cRunningAvgBuffer);
               mAvgSimRenderProjectilesTime.set(cRunningAvgBuffer);
               mAvgSimRenderDopplesTime.set(cRunningAvgBuffer);
               mAvgSimRenderPlayersTime.set(cRunningAvgBuffer);
            }

            void clear(void)
            {
               mTotalPreAsyncUpdateTime=0;
               mTotalAsyncUpdateTime=0;
               mTotalPostAsyncUpdateTime=0;
               mSubUpdate1Time=0;
               mSubUpdate2Time=0;
               mSubUpdate3Time=0;
               mTotalUpdateTime=0;

               mPlayerPreAsyncTime=0;
               mTeamPreAsyncTime=0;
               mArmyPreAsyncTime=0;
               mAIPreAsyncTime=0;
               mKBPreAsyncTime=0;
               mPlatoonPreAsyncTime=0;
               mSquadPreAsyncTime=0;
               mProjectilePreAsyncTime=0;
               mUnitPreAsyncTime=0;
               mDopplePreAsyncTime=0;
               mObjectPreAsyncTime=0;

               mCommandManagerUpdateTime=0;
               mCommandManagerServiceTime=0;
               mArmyUpdateTime=0;
               mPlatoonUpdateTime=0;
               mSquadUpdateTime=0;
               mProjectileUpdateTime=0;
               mUnitUpdateTime=0;
               mDoppleUpdateTime=0;
               mObjectUpdateTime=0;

               mEntitySchedulerUpdateTime=0;
               mSoundUpdateTime=0;
               mBattleUpdateTime=0;
               mPowerUpdateTime=0;
               mTriggerUpdateTime=0;
               mTriggerObjectsUpdateTime = 0;
               mPhysicsUpdateTime=0;
               mTextVisualUpdateTime=0;
               mTotalSimRenderTime=0;
               mSimRenderSquadsTime=0;
               mSimRenderUnitsTime=0;
               mSimRenderObjectsTime=0;
               mSimRenderProjectilesTime=0;
               mSimRenderDopplesTime=0;
               mSimRenderPlayersTime=0;
            };

            void clearMax()
            {
               mMaxTotalPreAsyncUpdateTime=0;
               mMaxTotalAsyncUpdateTime=0;
               mMaxTotalPostAsyncUpdateTime=0;
               mMaxSubUpdate1Time=0;
               mMaxSubUpdate2Time=0;
               mMaxSubUpdate3Time=0;
               mMaxTotalUpdateTime=0;

               mMaxPlayerPreAsyncTime=0;
               mMaxTeamPreAsyncTime=0;
               mMaxArmyPreAsyncTime=0;
               mMaxAIPreAsyncTime=0;
               mMaxKBPreAsyncTime=0;
               mMaxPlatoonPreAsyncTime=0;
               mMaxSquadPreAsyncTime=0;
               mMaxProjectilePreAsyncTime=0;
               mMaxUnitPreAsyncTime=0;
               mMaxDopplePreAsyncTime=0;
               mMaxObjectPreAsyncTime=0;

               mMaxCommandManagerUpdateTime=0;
               mMaxCommandManagerServiceTime=0;
               mMaxArmyUpdateTime=0;
               mMaxPlatoonUpdateTime=0;
               mMaxSquadUpdateTime=0;
               mMaxProjectileUpdateTime=0;
               mMaxUnitUpdateTime=0;
               mMaxDoppleUpdateTime=0;
               mMaxObjectUpdateTime=0;

               mMaxEntitySchedulerUpdateTime=0;
               mMaxSoundUpdateTime=0;
               mMaxBattleUpdateTime=0;
               mMaxPowerUpdateTime=0;
               mMaxTriggerUpdateTime=0;
               mMaxTriggerObjectsUpdateTime = 0;
               mMaxPhysicsUpdateTime=0;
               mMaxTextVisualUpdateTime=0;
               mMaxSimRenderTime=0;
               mMaxSimRenderSquadsTime=0;
               mMaxSimRenderUnitsTime=0;
               mMaxSimRenderObjectsTime=0;
               mMaxSimRenderProjectilesTime=0;
               mMaxSimRenderDopplesTime=0;
               mMaxSimRenderPlayersTime=0;
            }

            double  mTotalPreAsyncUpdateTime;
            double  mTotalAsyncUpdateTime;
            double  mTotalPostAsyncUpdateTime;
            double  mSubUpdate1Time;
            double  mSubUpdate2Time;
            double  mSubUpdate3Time;
            double  mTotalUpdateTime;

            double  mPlayerPreAsyncTime;
            double  mTeamPreAsyncTime;
            double  mArmyPreAsyncTime;
            double  mAIPreAsyncTime;
            double  mKBPreAsyncTime;
            double  mPlatoonPreAsyncTime;
            double  mSquadPreAsyncTime;
            double  mProjectilePreAsyncTime;
            double  mUnitPreAsyncTime;
            double  mDopplePreAsyncTime;
            double  mObjectPreAsyncTime;

            double  mCommandManagerUpdateTime;
            double  mCommandManagerServiceTime;
            double  mArmyUpdateTime;
            double  mPlatoonUpdateTime;
            double  mSquadUpdateTime;
            double  mProjectileUpdateTime;
            double  mUnitUpdateTime;
            double  mDoppleUpdateTime;
            double  mObjectUpdateTime;

            double  mEntitySchedulerUpdateTime;
            double  mPhysicsUpdateTime;
            double  mTextVisualUpdateTime;
            double  mSoundUpdateTime;
            double  mBattleUpdateTime;
            double  mPowerUpdateTime;
            double  mTriggerUpdateTime;
            double  mTriggerObjectsUpdateTime;
            double  mTotalSimRenderTime;
            double  mSimRenderSquadsTime;
            double  mSimRenderUnitsTime;
            double  mSimRenderObjectsTime;
            double  mSimRenderProjectilesTime;
            double  mSimRenderDopplesTime;
            double  mSimRenderPlayersTime;

            double  mMaxTotalPreAsyncUpdateTime;
            double  mMaxTotalAsyncUpdateTime;
            double  mMaxTotalPostAsyncUpdateTime;
            double  mMaxSubUpdate1Time;
            double  mMaxSubUpdate2Time;
            double  mMaxSubUpdate3Time;
            double  mMaxTotalUpdateTime;

            double  mMaxPlayerPreAsyncTime;
            double  mMaxTeamPreAsyncTime;
            double  mMaxArmyPreAsyncTime;
            double  mMaxAIPreAsyncTime;
            double  mMaxKBPreAsyncTime;
            double  mMaxPlatoonPreAsyncTime;
            double  mMaxSquadPreAsyncTime;
            double  mMaxProjectilePreAsyncTime;
            double  mMaxUnitPreAsyncTime;
            double  mMaxDopplePreAsyncTime;
            double  mMaxObjectPreAsyncTime;

            double  mMaxCommandManagerUpdateTime;
            double  mMaxCommandManagerServiceTime;
            double  mMaxArmyUpdateTime;
            double  mMaxPlatoonUpdateTime;
            double  mMaxSquadUpdateTime;
            double  mMaxProjectileUpdateTime;
            double  mMaxUnitUpdateTime;
            double  mMaxDoppleUpdateTime;
            double  mMaxObjectUpdateTime;

            double  mMaxEntitySchedulerUpdateTime;
            double  mMaxPhysicsUpdateTime;
            double  mMaxTextVisualUpdateTime;
            double  mMaxSoundUpdateTime;
            double  mMaxBattleUpdateTime;
            double  mMaxPowerUpdateTime;
            double  mMaxTriggerUpdateTime;
            double  mMaxTriggerObjectsUpdateTime;
            double  mMaxSimRenderTime;
            double  mMaxSimRenderSquadsTime;
            double  mMaxSimRenderUnitsTime;
            double  mMaxSimRenderObjectsTime;
            double  mMaxSimRenderProjectilesTime;
            double  mMaxSimRenderDopplesTime;
            double  mMaxSimRenderPlayersTime;

            BRunningAverage<double,double>  mAvgTotalPreAsyncUpdateTime;
            BRunningAverage<double,double>  mAvgTotalAsyncUpdateTime;
            BRunningAverage<double,double>  mAvgTotalPostAsyncUpdateTime;
            BRunningAverage<double,double>  mAvgSubUpdate1Time;
            BRunningAverage<double,double>  mAvgSubUpdate2Time;
            BRunningAverage<double,double>  mAvgSubUpdate3Time;
            BRunningAverage<double,double>  mAvgTotalUpdateTime;

            BRunningAverage<double,double>  mAvgPlayerPreAsyncTime;
            BRunningAverage<double,double>  mAvgTeamPreAsyncTime;
            BRunningAverage<double,double>  mAvgArmyPreAsyncTime;
            BRunningAverage<double,double>  mAvgAIPreAsyncTime;
            BRunningAverage<double,double>  mAvgKBPreAsyncTime;
            BRunningAverage<double,double>  mAvgPlatoonPreAsyncTime;
            BRunningAverage<double,double>  mAvgSquadPreAsyncTime;
            BRunningAverage<double,double>  mAvgProjectilePreAsyncTime;
            BRunningAverage<double,double>  mAvgUnitPreAsyncTime;
            BRunningAverage<double,double>  mAvgDopplePreAsyncTime;
            BRunningAverage<double,double>  mAvgObjectPreAsyncTime;

            BRunningAverage<double,double>  mAvgCommandManagerUpdateTime;
            BRunningAverage<double,double>  mAvgCommandManagerServiceTime;
            BRunningAverage<double,double>  mAvgArmyUpdateTime;
            BRunningAverage<double,double>  mAvgPlatoonUpdateTime;
            BRunningAverage<double,double>  mAvgSquadUpdateTime;
            BRunningAverage<double,double>  mAvgProjectileUpdateTime;
            BRunningAverage<double,double>  mAvgUnitUpdateTime;
            BRunningAverage<double,double>  mAvgDoppleUpdateTime;
            BRunningAverage<double,double>  mAvgObjectUpdateTime;

            BRunningAverage<double,double>  mAvgEntitySchedulerUpdateTime;
            BRunningAverage<double,double>  mAvgPhysicsUpdateTime;
            BRunningAverage<double,double>  mAvgTextVisualUpdateTime;
            BRunningAverage<double,double>  mAvgSoundUpdateTime;
            BRunningAverage<double,double>  mAvgBattleUpdateTime;
            BRunningAverage<double,double>  mAvgPowerUpdateTime;
            BRunningAverage<double,double>  mAvgTriggerUpdateTime;
            BRunningAverage<double,double>  mAvgTriggerObjectsUpdateTime;
            BRunningAverage<double,double>  mAvgSimRenderTime;
            BRunningAverage<double,double>  mAvgSimRenderSquadsTime;
            BRunningAverage<double,double>  mAvgSimRenderUnitsTime;
            BRunningAverage<double,double>  mAvgSimRenderObjectsTime;
            BRunningAverage<double,double>  mAvgSimRenderProjectilesTime;
            BRunningAverage<double,double>  mAvgSimRenderDopplesTime;
            BRunningAverage<double,double>  mAvgSimRenderPlayersTime;
      }; 

      const BStats &getTimeStats() const { return mTimeStats; }
      //void getTimeStats(BStats& stats) const { stats = mTimeStats; }
      BStats mTimeStats;
#endif

      BAI* createAI(BPlayerID playerID, BTeamID teamID);
      BAI* getAI(BPlayerID playerID);
      const BAI* getAI(BPlayerID playerID) const;
      void deleteAI(BPlayerID playerID);
      BKB* createKB(BTeamID teamID);
      BKB* getKB(BTeamID teamID);
      const BKB* getKB(BTeamID teamID) const;
      void deleteKB(BTeamID teamID);
      BAIMission* createAIMission(BPlayerID playerID, BAIMissionType missionType);
      BAIMission* getAIMission(BAIMissionID missionID);
      const BAIMission* getAIMission(BAIMissionID missionID) const;
      void deleteAIMission(BAIMissionID missionID);
      BAIMissionTargetWrapper* createWrapper(BAIMissionID missionID, BAIMissionTargetID targetID);
      BAIMissionTargetWrapper* getWrapper(BAIMissionTargetWrapperID missionTargetWrapperID);
      const BAIMissionTargetWrapper* getWrapper(BAIMissionTargetWrapperID missionTargetWrapperID) const;
      void deleteWrapper(BAIMissionTargetWrapperID missionTargetWrapperID);
      BAIGroup* createAIGroup(BAIMissionID missionID, BProtoSquadID protoSquadID);
      BAIGroup* getAIGroup(BAIGroupID groupID);
      const BAIGroup* getAIGroup(BAIGroupID groupID) const;
      void deleteAIGroup(BAIGroupID groupID);
      BAIGroupTask* createAIGroupTask(BAIGroupID groupID);
      BAIGroupTask* getAIGroupTask(BAIGroupTaskID groupTaskID);
      const BAIGroupTask* getAIGroupTask(BAIGroupTaskID groupTaskID) const;
      void deleteAIGroupTask(BAIGroupTaskID groupTaskID);
      BAIMissionTarget* createAIMissionTarget(BPlayerID playerID);
      BAIMissionTarget* getAIMissionTarget(BAIMissionTargetID missionTargetID);
      const BAIMissionTarget* getAIMissionTarget(BAIMissionTargetID missionTargetID) const;
      void deleteAIMissionTarget(BAIMissionTargetID missionTargetID);
      BAITopic* createAITopic(BPlayerID playerID);
      BAITopic* getAITopic(BAITopicID topicID);
      const BAITopic* getAITopic(BAITopicID topicID) const;
      void deleteAITopic(BAITopicID topicID);
      BKBSquad* createKBSquad(BTeamID teamID);
      BKBSquad* getKBSquad(BKBSquadID kbSquadID);
      const BKBSquad* getKBSquad(BKBSquadID kbSquadID) const;
      void deleteKBSquad(BKBSquadID kbSquadID);
      BKBBase* createKBBase(BTeamID teamID, BPlayerID playerID, BVector position, float radius);
      BKBBase* getKBBase(BKBBaseID kbBaseID);
      const BKBBase* getKBBase(BKBBaseID kbBaseID) const;
      void deleteKBBase(BKBBaseID kbBaseID);
      BBid* createBid(BPlayerID playerID);
      BBid* getBid(BBidID bidID);
      const BBid* getBid(BBidID bidID) const;
      void deleteBid(BBidID bidID);
      BAITeleporterZone* createAITeleporterZone(BEntityID unitID, BVector boxExtent1, BVector boxExtent2);
      BAITeleporterZone* createAITeleporterZone(BEntityID unitID, BVector spherePos, float sphereRad);
      BAITeleporterZone* getAITeleporterZone(BAITeleporterZoneID zoneID);
      const BAITeleporterZone* getAITeleporterZone(BAITeleporterZoneID zoneID) const;
      void deleteAITeleporterZone(BAITeleporterZoneID zoneID);


      int                     addCustomCommand(BCustomCommand& command);
      void                    removeCustomCommand(int id);
      BCustomCommand*         getCustomCommand(int id);
      BSmallDynamicSimArray<BCustomCommand>& getCustomCommands() { return mCustomCommands; }
      int                     getNextCustomCommandID() const { return mNextCustomCommandID; }

      // Alert Helper
      float                   getAreaCombatValue(BVector location, float radius, BPlayerID playerID, int relationType, bool* playerIDInArea=NULL);

      // Sim bounds
      void                    setSimBounds(float minX, float minZ, float maxX, float maxZ);
      float                   getSimBoundsMinX() const { return mSimBoundsMinX; }
      float                   getSimBoundsMinZ() const { return mSimBoundsMinZ; }
      float                   getSimBoundsMaxX() const { return mSimBoundsMaxX; }
      float                   getSimBoundsMaxZ() const { return mSimBoundsMaxZ; }
                              
      void                    setFogOfWar(bool useFog, bool useSkirt, bool onlyVisual);
      void                    clearBlackMap();

      void                    drawBuildingPlacementDecal(int decalID, BVector pos, BVector forward, float sizeX, float sizeY, DWORD color, float intensity);

      bool                    isSphereVisible(XMVECTOR center, float radius) const;

      void                    addTransformedTactic(BTactic* mpTactic);

      void                    addPhysicsBreakOffObjectAsync(const BObject* pParentObject, const hkpShape* pParentShape);
      void                    addPhysicsVehicleTerrainCollisionAsync(BPhysicsGroundVehicleAction* pPhysicsAction);

      int16                   getNewExplorationGroup();
      void                    addObjectToExplorationGroup(const BObject& object);
      void                    removeObjectFromExplorationGroup(const BObject& object);
      void                    makeExplorationGroupVisible(BTeamID teamId, int16 groupId);

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);
      bool postLoad(int saveType);

      DWORD                   getSubGametime() const;
      double                  getSubGametimeFloat() const;
      double                  getSubTotalRealtime() const;
      DWORD                   getSubUpdate() const {return(mSubUpdate);}
      DWORD                   getAmountOfSubUpdates() const;
      long                    getSubUpdateTimeInMsecs() const;
      long                    getSubUpdateTotalTimeInMsecs() const;
      void                    incrementSubUpdate() { mSubUpdate++; }
      float                   getUpdateCompletion(DWORD startingFromSubUpdate) const;
      float                   getLastSubUpdateLengthFloat() const;
      int64                   getLastSubUpdateRealtime() const;
      bool                    shouldEndSubUpdate() const;
      bool                    isOnFirstSubUpdate() const;
      bool                    isOnLastSubUpdate() const;
      long                    calcAdjustedSubUpdateTime();
      void                    setAdjustedSubUpdateTime(long val) { mAdjustedSubUpdateTime=val; }
      void                    setSubUpdating(bool on, bool alternate, bool dynamicTime, bool decoupled);
      bool                    getDynamicSubUpdating() const { return mDynamicSubUpdating; }
      bool                    getAlternateSubUpdating() const { return mAlternateSubUpdating; }
      bool                    getDynamicSubUpdateTime() const { return mDynamicSubUpdateTime; }

      void                    recordScenarioLoadObjectHighWaterMark();
      uint                    getScenarioLoadObjectHighWaterMark() const { return mScenarioLoadObjectHighWaterMark; }

      bool                    createPowerAudioReactions(BPlayerID castingPlayerID, BSquadSoundType soundType, BVector worldPos, float radius, BEntityID castingSquadID=cInvalidObjectID);

      virtual int             getEventListenerType() const { return cEventListenerTypeWorld; }
      virtual bool            savePtr(BStream* pStream) const { return true; }

      #ifdef UNITOFFENDERS
      void                    getUnitOffenders(BOffenderArray& offenders) const { offenders=mUnitOffenders; }
      #endif

      void                    incrementLowDetailAnimRefCount() { mLowDetailAnimRefCount++; }
      void                    decrementLowDetailAnimRefCount() { mLowDetailAnimRefCount--; }
      long                    getLowDetailAnimRefCount() { return mLowDetailAnimRefCount; }

      BVector                 getTerrainMax() const { return mTerrainMax; }

      void                    resetExplorationGroupTeamData(int16 index, BPlayerID exceptionID = cInvalidTeamID);

#ifdef DECOUPLED_UPDATE
      double                  getDecoupledUpdateElapsed() const { return mDecoupledUpdateElapsed; }
      double                  getDecoupledUpdateLastSubElapsed() const { return mDecoupledUpdateLastSubElapsed; }
      double                  getDecoupledUpdateAvgUpdateTime() const { return mDecoupledUpdateAvgUpdateTime; }
      double                  getDecoupledUpdateRenderTime() const { return mDecoupledUpdateRenderTime; }
      double                  getDecoupledUpdateTotalUpdateTime() const { return mDecoupledUpdateTotalUpdateTime; }
      float                   getDecoupledUpdateLength() const { return mDecoupledUpdateLength; }
      bool                    getDecoupledUpdateSkipRenders() const { return mDecoupledUpdateSkipRenders; }
      bool                    getDecoupledUpdateEnd() const { return mDecoupledUpdateEnd; }
      bool                    getDecoupledUpdateRender() const { return mDecoupledUpdateRender; }

      void                    setDecoupledUpdateRender(bool val) { mDecoupledUpdateRender=val; }
#endif

   protected:
      void                    determinePlayerColorCategory();

      void                    handleFirstUpdate();
      void                    handleDynamicSubUpdating();
      void                    beginSubUpdate();
      void                    updatePreAsync(int64 currentUpdateTime, float currentUpdateLength, double timerFrequency);
      void                    updateAsync(int64 currentUpdateTime, float currentUpdateLength, double timerFrequency);
      void                    updatePostAsync(int64 currentUpdateTime, float currentUpdateLength, double timerFrequency);
      void                       updateUnits();
      void                       updateObjects();
      void                       updateDopples();
      void                       updateProjectiles();
      void                       updateMinimap();
      void                       updateEntityScheduler();
      void                       updateTextVisualManager();
      void                       updateSoundManager();
      void                       updateBattleManager();
      void                       updatePowerManager();
      void                       updateTriggerManager();
      void                       updatePhysics();
      void                       updateLightManager();
      void                       updateMinimapBounds();
      void                       updatePathingLimiter();
      void                       updateExplorationGroups();
      void                    updateGrannySync(bool doSyncChecks);
      void                    updateFrame();

      bool                    shouldEndSubUpdate();
      bool                    canRenderInterpolatedFrame();
      
      void                    updatePlayers();
      void                    setPlayerEndGameConditions(BPlayer* pPlayer, bool gameCompleted, bool playerWon);

      void                    adjustPlayerStrength(BEntityID entityID, bool add);
      void                    adjustPlayerStrength(int playerID, int techID);


      BVector                 getUnobstructedBirthPosition( BUnit* pContainingUnit, const float obstructionRadius, BVector* pForward, long preferredBoneHandle, bool &gotPreferredPosition, bool flyingUnit );

      // These are called from getUnitsInArea and assume that the data is valid that is passed in.
      long                    getEntitiesInArea(BUnitQuery* pUnitQuery, BEntityIDArray* pResults, bool includeNonCollidableUnitObs, bool isSquadCheck);
      bool                    checkAndUpdateEntity(const BUnitQuery& unitQuery, BEntity* pEntityToTest, BEntity*& pEntityToAdd, bool isSquadCheck);

      long                    getEntitiesInAreaTypeNone(BUnitQuery&  unitQuery, BEntityIDArray&  results, bool includeNonCollidableUnitObs, bool isSquadCheck);
      long                    getEntitiesInAreaTypeTwoVector(BUnitQuery&  unitQuery, BEntityIDArray&  results, bool includeNonCollidableUnitObs, bool isSquadCheck);
      long                    getEntitiesInAreaTypePointRadius(BUnitQuery&  unitQuery, BEntityIDArray&  results, bool includeNonCollidableUnitObs, bool isSquadCheck);
      long                    getEntitiesInAreaTypeConvexHull(BUnitQuery&  unitQuery, BEntityIDArray&  results, bool includeNonCollidableUnitObs, bool isSquadCheck);
      long                    getEntitiesInAreaTypeOPQuadHull(BUnitQuery&  unitQuery, BEntityIDArray&  results, bool includeNonCollidableUnitObs, bool isSquadCheck);
      long                    getEntitiesInAreaTypeOBBNoRoll(BUnitQuery& unitQuery, BEntityIDArray& results, bool includeNonCollidableUnitObs, bool isSquadCheck);

      // this gets called from within check placement to display the building suggestion decal
      void                    drawCheckPlacement(const BProtoObject* pProtoObject, const BPlayer* pPlayer, bool canPlace, bool suggested, BVector location, BVector forward, BVector suggestion, float protoObstructionRadiusX, float protoObstructionRadiusZ, float expansionRadius, BVector p1, BVector p2, BVector p3, BVector p4);

      void                    updateSingleThreadedPhysicsData();
      void                    createAllPhysicsBreakOffObjects();
      void                    resetAllPhysicsBreakOffObjects();
      void                    resolveAllPhysicsVehicleTerrainCollisions();
      void                    resetAllPhysicsVehicleTerrainCollisions();

      void                    resetExplorationGroups();
      void                    stopActiveExplorationGroup(int16 index);

      #ifndef BUILD_FINAL
      void                    checkForDirtyPlayerColors();
      #endif      

      #ifdef UNITOFFENDERS
      void                    addUnitOffender(const BEntityID entityID, double time);
      #endif      

#ifdef SYNC_World
      void                    archiveSyncCheck();
      uint32                  generateArchiveCRC(const char* pFilename);
#endif


      //for async update
      class BAsyncWorkUnit
      {
      public:
         BEntityHandle mEntityHandle;
      };

      class BAsyncWorkGroupThreadData
      {
      public:
         BWorld* mpWorld;
         BDynamicArray<BEntityHandle> mObjectHandles;
         float mWorldlastUpdateLengthFloat;
      };
      void addAsyncWorkUnit(BEntityHandle handle);
      void clearAsyncWorkGroups();
      void flushAsyncWorkGroups();
      static void updateAsyncWorkgroupCallback(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket);

      struct BExplorationGroupEntry
      {
         BTeamIDArray     triggeredTeams;
         BEntityIDArray   objects;
      };
      typedef BHashMap<int16, BExplorationGroupEntry> BExplorationGroupHashMap;
      BExplorationGroupHashMap mExplorationGroups;
      int16  mNumExplorationGroups;

      struct BExplorationGroupTimerEntry
      {
         BTeamID team;
         float   timeLeft;
         int16   explorationGroupIndex;
      };
      BSmallDynamicSimArray<BExplorationGroupTimerEntry> mActiveExplorationGroups;

      BDynamicArray<BAsyncWorkUnit>               mWorkUnits;
      BDynamicArray<BAsyncWorkGroupThreadData, 32> mWorldUnitUpdateWorkEntries;
      BCountDownEvent                       mWorldUnitUpdateRemainingBuckets;

      BSmallDynamicSimArray<BPlayer*>     mPlayers;                           // 8 bytes
      BSmallDynamicSimArray<BTeam*>       mTeams;                             // 8 bytes
                  
      BPlayerColor                  mPlayerColors[cMaxPlayerColorCategories][cMaximumSupportedPlayers];
      uint8                         mPlayerColorIndices[cMaxPlayerColorCategories][cMaximumSupportedPlayers];
#ifndef BUILD_FINAL      
      uint                          mPlayerColorLoadCount;
#endif                    
      
      BObjectManager*                  mpObjectManager;                          // 4 bytes
      BCommandManager*                 mpCommandManager;                         // 4 bytes
      BWorldSoundManager*              mpWorldSoundManager;                      // 4 bytes
      BPhysicsWorld*                   mpPhysicsWorld;                           // 4 bytes
      BObjectiveManager*               mpObjectiveManager;                       // 4 bytes
      BChatManager*                    mpChatManager;                            // 4 bytes
      BHintManager*                    mpHintManager;                            // 4 bytes
      BDesignObjectManager*            mpDesignObjectManager;                    // 4 bytes
      BBattleManager*                  mpBattleManager;                          // 4 bytes
      BPowerManager*                   mpPowerManager;                           // 4 bytes
      BAIManager*                      mpAIManager;                              // 4 bytes
      BEntityGrouper*                  mEntityGrouper;                           // 4 bytes
      BPathingLimiter*                 mpPathingLimiter;                         // 4 bytes

      DWORD                         mUpdateNumber;                            // 4 bytes
      int64                         mLastUpdateTime;                          // 8 bytes
      DWORD                         mLastUpdateLength;                        // 4 bytes
      float                         mLastUpdateLengthFloat;                   // 4 bytes
      DWORD                         mGametime;                                // 4 bytes
      DWORD                         mSubUpdate;                               // 4 bytes
      int                           mSubUpdateSection;
      int                           mSubUpdateSectionItem;
      DWORD                         mFirstSubUpdate;                          // 4 bytes      
      int64                         mSubUpdateBeganTime;                      // 8 bytes
      int64                         mSubUpdateLastTime;

      // rg [11/21/06] - Changing realtime values to double instead of float because after a few days floats won't have enough precision. 
      // Also, on 360 doubles are as expensive as floats.

      double                        mGametimeFloat;                           // 8 bytes
      double                        mTotalRealtime;                           // 8 bytes
      double                        mSubTotalRealtime;                        // 8 bytes
      int64                         mLastUpdateRealtime;                      // 8 bytes
      int64                         mLastSubUpdateRealtime;                   // 8 bytes
      double                        mLastUpdateRealtimeDelta;                 // 8 bytes
      double                        mLastTimerFrequency;                      // 8 bytes
      float                         mAccumulatedTimeError;
      float                         mAutoSaveTime;
     
      long                          mFogStartX;                               // 4 bytes
      long                          mFogStartZ;                               // 4 bytes
      long                          mFogEndZ;                                 // 4 bytes

      BSmallDynamicSimArray<BLightVisualInstance*> mLights;                   // 8 bytes
      BSimString                    mTerrainName;                             // 8 bytes
      BCinematicManager*            mpCinematicManager;                       // 4 bytes
      BTransitionManager*           mpTransitionManager;                      // 4 bytes
      BVisual*                      mpSkyVisual;                              // 4 bytes
      int                           mBuildingPlacementDecal;                  // 4 bytes

      long                          mTempObjectID;                            // 4 bytes      

      BRelationType                 mTeamRelationType[cMaximumSupportedTeams][cMaximumSupportedTeams];   // 49 bytes

      BEntityIDArray                mCachedUnitsToTrack;                      // 8 bytes
      BObstructionNodePtrArray      mCachedObstructions;                      // 12 bytes
      BObstructionNodePtrArray      mExpandedObstructions;                    // 12 bytes
      BObstructionNodePtrArray      mCachedTerrainObstructions;               // 12 bytes
      BObstructionNodeArray         mExpandedObstructionData;

      typedef BDynamicArray<BPhysicsBreakOffObject, ALIGN_OF(BPhysicsBreakOffObject), BDynamicArrayPrimaryHeapAllocator, BDynamicArraySmallOptions, BDynamicArraySeparatePointers> BPhysicsBreakOffObjectsArray;
      BPhysicsBreakOffObjectsArray mPhysicsBreakOffObjects;
      BLightWeightMutex mPhysicsBreakOffObjectMutex;

      typedef BSmallDynamicArray<BPhysicsGroundVehicleAction*> BPhysicsVehicleTerrainCollisionActionsArray;
      BPhysicsVehicleTerrainCollisionActionsArray mPhysicsVehicleTerrainCollisionActions;
      BLightWeightMutex mPhysicsVehicleTerrainCollisionMutex;

#ifndef BUILD_FINAL
      long                          mLaunchedProjectilesPerFrame;             // 4 bytes (NON-FINAL)
      unsigned int                  mRenderedProjectilesLastFrame;            // 4 bytes (NON-FINAL)
      unsigned int                  mRenderedUnitsLastFrame;                  // 4 bytes (NON-FINAL)
      unsigned int                  mRenderedDopplesLastFrame;                // 4 bytes (NON-FINAL)
      unsigned int                  mRenderedObjectsLastFrame;                // 4 bytes (NON-FINAL)
#endif      
      #ifdef UNITOFFENDERS
      BOffenderArray                mUnitOffenders;
      #endif

      BSmallDynamicSimArray<BCustomCommand>  mCustomCommands;          // 8 bytes
      int                                    mNextCustomCommandID;     // 4 bytes

      BStoredAnimEventManager*               mpStoredEventTagManager;    // 4 bytes                
      float                      mSimBoundsMinX;
      float                      mSimBoundsMinZ;
      float                      mSimBoundsMaxX;
      float                      mSimBoundsMaxZ;
      BVector                    mOutsideOfBoundsBlockerCenter[4];
      BVector                    mOutsideOfBoundsBlockerSize[4];

      uint                       mWorldID;
      float                      mGameOverCountdown;
      BEntityHandle              mLastSubUpdateUnit;
      long                       mAdjustedSubUpdateTime;

      uint                       mScenarioLoadObjectHighWaterMark;

      BSmallDynamicSimArray<BTactic*> mTransformedTactics;
      BSmallDynamicSimArray<BTalkingHead*> mTalkingHeads;
      
      BPlayerColorCategory       mPlayerColorCategory;

      long                       mLowDetailAnimRefCount;

      BVector                    mTerrainMax;

      BTimingTracker* mpTimingTracker;

#ifdef DECOUPLED_UPDATE
      BTimer mDecoupledUpdateTimer;
      BTimer mDecoupledRenderTimer;
      BTimer mDecoupledSubUpdateTimer;
      BTimer mDecoupledWorkSinceLastRenderTimer;
      BSmallDynamicSimArray<double> mDecoupledUpdateTimes1;
      BSmallDynamicSimArray<double> mDecoupledUpdateTimes2;
      BSmallDynamicSimArray<double> *mDecoupledUpdateTimesCurrent;
      BSmallDynamicSimArray<double> *mDecoupledUpdateTimesPrevious;
      double mDecoupledUpdateElapsed;
      double mDecoupledUpdateLastSubElapsed;
      double mDecoupledUpdateAvgUpdateTime;
      double mDecoupledUpdateRenderTime;
      double mDecoupledUpdateTotalUpdateTime;
      DWORD mDecoupledUpdateStartingSubUpdateCurrent;
      DWORD mDecoupledUpdateStartingSubUpdatePrevious;
      float mDecoupledUpdateLength;
      bool mDecoupledUpdateSkipRenders;
      bool mDecoupledUpdateEnd;
      bool mDecoupledUpdateRender;
      bool mDecoupledFinishedAnUpdate;
#endif

      BTimer mSubUpdateEnabledTimer;
      DWORD mSubUpdateEnabledNumber;
      bool mDynamicSubUpdating;
      bool mAlternateSubUpdating;
      bool mDynamicSubUpdateTime;
      bool mNextEnableSubUpdating;
#ifdef DECOUPLED_UPDATE
      bool mNextDecoupledUpdate;
#endif
      bool mNextAlternateSubUpdating;
      bool mNextDynamicSubUpdateTime;
      bool mChangeSubUpdatePendnig;

      bool mFirstUpdate;

      // Flags
      bool                       mFlagUnitCache          : 1;  // 1 byte   (1/8)
      bool                       mFlagObstructionCache   : 1;  //          (2/8)
      bool                       mFlagQuickBuild         : 1;  //          (3/8)
      bool                       mFlagGameOver           : 1;  //          (4/8)
      bool                       mFlagMultiplayerSim     : 1;  //          (5/8)
      bool                       mFlagCoop               : 1;  //          (6/8)
      bool                       mFlagPlayableBounds     : 1;  //          (7/8)
      bool                       mFlagOutsideBoundsBlocked : 1; //
      bool                       mFlagNoFogSim           : 1;  //          (8/8)
      bool                       mFlagNoFogVis           : 1;  // 1 byte   (1/8)
      bool                       mFlagShowSkirt          : 1;  //          (2/8)
      bool                       mFlagAllVisible         : 1;  //          (3/8)
      bool                       mFlagSkipGameOverCheck  : 1;  //          (4/8)
      bool                       mFlagStartedUpdate      : 1; 

      #ifdef UNITOFFENDERS
      bool                       mFlagClearUnitOffenders : 1;
      #endif
};


class BPhysicsBreakOffObject
{
   public:
      BPhysicsBreakOffObject() : 
         mpParentObject(NULL),
         mpNewShape(NULL)
      {
      }

      BPhysicsBreakOffObject(const BObject *pParentObject, const hkpShape* pShape) :
         mpParentObject(pParentObject),
         mpNewShape(pShape)
      {
      }

      const BObject*       mpParentObject;
      const hkpShape*      mpNewShape;
};


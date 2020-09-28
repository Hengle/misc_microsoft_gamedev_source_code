//==============================================================================
// entity.h
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================
#pragma once

// Includes
#include "poolable.h"
#include "bitvector.h"
#include "action.h"
#include "actionlist.h"
#include "path.h"
#include "obstructionmanager.h"
#include "protoobject.h"
#include "gamefilemacros.h"

// Forward declarations
class BPlayer;
class BTeam;
class BObject;
class BUnit;
class BSimOrder;
class BSquad;
class BPlatoon;
class BDopple;
class BProjectile;
class BArmy;
class BAction;
class BOPObstructionNode;
class BPhysicsObject;
class BPhysicsInfo;
class iAgent;
class BPhysicsObjectBlueprintOverrides;
class BPhantom;
class BPhysicsObjectParams;

//==============================================================================
// BEntityRef
//==============================================================================
class BEntityRef
{
   public:
      enum
      {
         cTypeNone,
         cTypeTrainLimitBuilding, // mID: builder unit
         cTypeTrainLimitUnit,     // mID: unit built/trained, mData1: unit's proto, mData2: train limit bucket
         cTypeTrainLimitSquad,    // mID: squad trained, mData1: squad's proto, mData2: train limit bucket
         cTypeGatherBuilding,     // mID: building attached to, mData1: gatherer object type, mData2: reserved gatherer count
         cTypeGatherResource,     // mID: attached resource, mData1: gatherer object type, mData2: auto-created flag
         cTypeGatherUnit,         // mID: gatherer unit, mData1: gatherer object type, mData2: building's player
         cTypeGatherTarget,       // mID: resource unit
         cTypeGatherParent,       // mID: parent building
         cTypeGatherChild,        // mID: child unit
         cTypeBlockerChild,       // mID: child unit
         cTypeContainUnit,        // mID: contained unit
         cTypeContainingUnit,     // mID: containing unit
         cTypeAttachObject,       // mID: attached object
         cTypeAttachedToObject,   // mID: attached to object
         cTypeCapturingUnit,      // mID: capturing unit, mData1: capturing unit's player
         cTypeCapturingPlayer,    // mID: capturing player, mData1: squad ref count, mData2: cost paid
         cTypeBuiltByUnit,        // mID: builder unit
         cTypeHitchUnit,          // mID: hitched unit
         cTypeHitchedToUnit,      // mID: hitched to unit
         cTypeBuildQueueChild,    // mID: queued building unit, mData1: building's proto ID
         cTypeBuildQueueParent,   // mID: parent building

         cTypeAssociatedParkingLot, // Summary: Attached to all fire bases, sockets and buildings that are associated with a base parking lot.
                                    // mID: This is the EntityID of the associated parking lot object.
                                    // note: Not attached to the settlement.

         cTypeParkingLotRef,        // Summary: Attached to all parking lots.
                                    // mData1: Ref counts the # of entities whose AssociatedParkingLot (see above) entityRef points back at the attached parking lot.

         cTypeAssociatedSettlement, // Summary: Attached to all sockets, buildings, parking lots, fire bases and settlements.
                                    // mID: This is the EntityID of the associated settlement socket.

         cTypeAssociatedBase,       // Summary: Attached to all sockets, buildings, parking lots, fire bases and settlements.
                                    // mID: This is the EntityID of the associated base (firebase, or main base)
                                    // mData1: Number of associated child buildings. Only set when this ref is on the actual base object.

         cTypeParentSocket,         // Summary: Attached to a building which is plugged into a socket.
                                    // mID: This is the EntityID of the parent socket the building is plugged into.

         cTypeSocketPlug,           // Summary: Attached to all sockets which have a building plugged into them.
                                    // mID: This is the EntityID of the building which is plugged into the socket.

         cTypeAssociatedSocket,     // Summary: Attached to both the settlement socket and the base (firebase, main base)
                                    // mID: This is the EntityID of the associated socket.
                                    // Note: Can be multiple of this type of entity ref on both the settlement and base.

         cTypeAssociatedBuilding,   // Summary: Attached to both the settlement socket and the base (firebase, main base)
                                    // mID: This is the EntityID of the associated building.
                                    // Note: Can be multiple of this type of entity ref on both the settlement and base.

         cTypeAssociatedUnit,       // Summary: Attached to a unit which has been created as a child object generically (eggstalk)

         cTypeStickedProjectile,    // mID: Projecile that is stuck to the unit.
                                    // Note: Can be multiple of this type.

         cTypeTrainLock,            // Summary: This goes on the parking lot (or UBL) object to indicate if a player has locked training.
                                    // mData1: Player ID

         cTypeEffect,               // Summary: Currently used by squads to attach an overhead effect (such as recovering).
                                    // mData2: Effect type (such as BSquad::cEffectRecovering).

         cTypeBaseShield,           // Summary: Reference a base holds to a plasma shield covering it. Currently used to get a valid spawn point for squads.

         cTypeAssociatedWallTower,  // Link between wall towers

         cTypeAssociatedObject,     // Summary: Attached to a unit which has been created as a child object generically (eggstalk)

         cTypeBuildingShield,       // Summary: Used to reference the shield on socketd non-turret buildings in the base

         cTypeAssociatedFoundation, // Summary: The foundation below a building 

         cTypeBoardingUnit,         // Summary: The unit that is boarding

         // AJL 3/3/07 - Leave these designer values at the end of the list
         cTypeDesignStart = 156,  // mData1: ref count
         cTypeDesignEnd = 255     // mData1: ref count
      };

      BEntityRef() : mType(0), mData2(0), mData1(0), mID(cInvalidObjectID) {}
      BEntityRef(BYTE type, BEntityID id, short data1, BYTE data2) : mType(type), mData2(data2), mData1(data1), mID(id) {}

      #ifndef BUILD_FINAL
         void createDebugString(BSimString &debugString);
      #endif

      BEntityID mID; // 4 bytes      
      short mData1;  // 2 bytes      
      BYTE mType;    // 1 byte
      BYTE mData2;   // 1 byte
};
typedef BDynamicSimArray<BEntityRef,4,BDynamicArrayNoConstructOptions> BEntityRefArray;

//==============================================================================
// BEntity
//==============================================================================
class BEntity : public IPoolable, IEventListener
{
public:

   typedef enum // (MAXIMUM 7 ENTRIES)
   {
      cClassTypeObject    =      0x0000,
      cClassTypeUnit      =      0x0001,
      cClassTypeSquad     =      0x0002,
      cClassTypeDopple    =      0x0003,
      cClassTypeProjectile =     0x0004,
      cClassTypePlatoon   =      0x0005,
      cClassTypeArmy      =      0x0006
   } BClassType;

   typedef enum
   {
      //-- Entity Events
      cEventMoveEnd,
      //cEventMoveBegin,
      cEventStopped,
      cEventTargetLost,
      cEventTargetChanged,
      //cEventUnitOutOfRange,
      //cEventNeedPath,
      cEventTrainQueued,
      cEventTrainPercent,
      cEventTrainSquadQueued,
      cEventTrainSquadPercent,
      cEventTechQueued,
      cEventTechPercent,
      cEventTechResearched,
      cEventCustomCommandQueued,
      cEventCustomCommandPercent,
      cEventCustomCommandResearched,
      cEventUnloaded,
      cEventBuildPercent,
      cEventSelfDestructTime,
      cEventTribute,
      cEventCapturePercent,
      cEventPowerPercent,
      cEventKilled,
      cEventKilledUnit,
      cEventDamaged,
      cEventDetected,
      cEventBuilt,
      cEventBuiltUnit,
      cEventCaptured,
      cEventRepair,
      cEventBuildingResource,
      cEventSquadModeChanging,
      cEventSquadModeChanaged,
      cEventRecoverSet,
      cEventDeathReplacement,
      cEventFullyHealed,
      cEventTrainLock,
      cEventContainedUnit,
      cEventRebuildTimer,
      cEventRecomputeVisualStarting,
      cEventRecomputeVisualCompleted,
      cEventPickedUp,
      cEventTargetDazed,
      cEventAllyDamaged,
      cEventGarrisonStart,
      cEventGarrisonEnd,
      cEventGarrisonFail,
      cEventUngarrisonStart,
      cEventUngarrisonEnd,
      cEventUngarrisonFail,
      cEventSquadUngarrisonStart,
      cEventSquadUngarrisonEnd,
      cEventSquadUngarrisonFail,
      cEventUnitPhysicsReplacement,
      cEventUnitAttachmentThrown,

      //-- Player Events
      cEventPlayerResigned,
      cEventPlayerDefeated,
      cEventPlayerDisconnected,
      cEventPlayerWon,
      cEventSwitchToPlayerID,
      cEventSupportPowerAvailable,

      //-- Game events
      cEventPlaybackDone,
      cEventGameOver,

      //-- Squad events
      cEventSquadAISearch,
      cEventSquadAbility,
      cEventSquadUnitAdded,
      cEventSquadTimedAbilityOver,

      //-- Action Events
      cEventActionStateChange, 
      cEventActionDone,
      cEventActionFailed,
      
      //Opp Events.
      cEventOppComplete,

      //-- Anim Tag Events
      cEventAnimLoop,
      cEventAnimEnd,
      cEventAnimChain,
      cEventAnimAttackTag,
      cEventAnimSoundTag,
      cEventAnimParticlesTag,
      cEventAnimCameraShakeTag,
      cEventAnimGroundIKTag,
      cEventAnimAttachTargetTag,
      cEventAnimSweetSpotTag,
      cEventAttachmentAnimLoop,
      cEventAttachmentAnimEnd,
      cEventAttachmentAnimChain,

      //-- Corpse Manager Events
      cEventCorpseRemove,

      cEventCount,
   } BEventType;

   typedef enum
   {
      cCollisionNone,
      cCollisionTerrain,
      cCollisionEdgeOfMap,
      cCollisionUnit,
      cNumberCollisionResults,
   } BCollisionResult;

protected:
   BEntity();
   virtual                 ~BEntity();
public:

   virtual bool            updatePreAsync(float elapsedTime);
   virtual bool            updateAsync(float elapsedTime);
   virtual bool            update(float elapsedTime);
   virtual void            debugRender();
   virtual void            kill(bool bKillImmediately);
   virtual void            destroy();
   virtual void            render() {};

   void                    setID(BEntityID id) { mID=id; }
   BEntityID               getID() const { return mID; }

   void                    setPlayerID(BPlayerID playerID) { mPlayerID = playerID; }
   BPlayerID               getPlayerID() const { return(mPlayerID); }
   virtual BPlayerID       getColorPlayerID() const { return(mPlayerID); }
   virtual void            setProtoID(long protoID) { protoID; }
   virtual long            getProtoID() const { return(cInvalidProtoID); }
   XMFINLINE const BProtoObject* getProtoObject() const;

   BPlayer*                getPlayer( void );
   const BPlayer*          getPlayer( void ) const;

   BTeamID                 getTeamID() const;
   BTeam*                  getTeam();

   inline long             getClassType( void ) const             { return (long) mID.getType(); }
   inline bool             isClassType(uint type) const           { return  ( type == mID.getType() ); }

   virtual void            onAcquire();
   virtual void            onRelease();
   virtual void            init();

   void                    setAssociatedParkingLot(BEntityID parkingLotID);
   void                    setAssociatedSettlement(BEntityID settlementID);
   void                    setAssociatedBase(BEntityID baseID);
   void                    setParentSocket(BEntityID parentSocketID);
   void                    setSocketPlug(BEntityID socketPlugID);
   void                    addAssociatedSocket(BEntityID socketID);
   void                    removeAssociatedSocket(BEntityID socketID);
   void                    addAssociatedBuilding(BEntityID buildingID);
   void                    removeAssociatedBuilding(BEntityID buildingID);
   void                    addAssociatedFoundation(BEntityID buildingID);
   void                    removeAssociatedFoundation(BEntityID buildingID);
   void                    handleKillBase(bool bKillImmediately);
   void                    handleKillSettlement();

   BEntityID               getAssociatedParkingLot() const;
   BEntityID               getAssociatedSettlement() const;
   BEntityID               getAssociatedBase() const;


   //-- Position and orientation
   const BVector&          getPosition() const { return mPosition; }
   inline void             getRotation(BMatrix &rot) const { rot.makeOrient(mForward, mUp, mRight); }
   const BVector&          getForward() const { return mForward; }
   const BVector&          getUp() const { return mUp; }
   const BVector&          getRight() const { return mRight; }
   const BVector&          getVelocity() const { return mVelocity; }
   float                   getForwardVelocity() const;
   virtual bool            isOutsidePlayableBounds(bool forceCheckWorldBoundaries=false) const;

   void                    setPosition(const BVector loc, bool overridePhysics=true);
   void                    setRotation(const BMatrix rot, bool overridePhysics=true);
   void                    setForward(const BVector forward);
   void                    setUp(const BVector  up);
   void                    setRight(const BVector  right);
   void                    setVelocity( const BVector velocity );

   void                    calcForward();
   void                    calcRight();
   void                    calcUp();

   void                    setYDisplacement( float v )                  { mYDisplacement = v;       }
   float                   getYDisplacement( void ) const               { return mYDisplacement;    }

   void                    moveForward(float distance); 
   void                    moveRight(float distance); 
   void                    moveUp(float distance);

   void                    moveWorldForward(float distance);
   void                    moveWorldRight(float distance);
   void                    moveWorldUp(float distance);

   void                    moveRelative(const BVector  offset);

   virtual DWORD           getLastMoveTime() const                      { return 0; }
   virtual void            setLastMoveTime(DWORD lastMoveTime)          { }

   void                    stop( void );
   virtual void            settle( void );

   bool                    isCollidable( void ) const                   { return getFlagCollidable(); }
   bool                    isMoving( void ) const                       { return getFlagMoving(); }
   bool                    isMobile( void ) const                       { return !getFlagNonMobile(); }
   bool                    isLockedDown( void ) const                   { return getFlagLockedDown(); }
   bool                    isGarrisoned(void) const                     { return getFlagGarrisoned(); }
   bool                    isAttached() const                           { return getFlagAttached(); }
   bool                    isInCover() const                            { return (getFlagInCover()); }
   virtual bool            isEverMobile( void ) const                   { const BProtoObject* pPO = getProtoObject(); return (pPO ? !pPO->getFlagNonMobile() : !getFlagNonMobile()); }
   bool                    isHitched() const                            { return (getFlagHitched()); }
   virtual bool            isSelectable(BTeamID teamId) const           { return (getFlagSelectable()); }

   virtual bool            isAlive( void ) const                        { return false;                     }
   virtual bool            isDamaged() const { return false; }
   virtual bool            isIdle( void ) const;
   virtual DWORD           getIdleDuration() const;

   void                    setTieToGround( bool flag)                   { setFlagTiesToGround(flag); }
   void                    tieToGround( void );
   void                    orientWithGround( void );
   void                    filterPitchAndRoll( BVector &vecRight, BVector &vecForward, float blendRate );

   virtual void            createObstruction(bool playerOwnsObstruction );
   void                    deleteObstruction( void );
   void                    updateObstruction( void );                    
   virtual long            computeObstructionType( void)                { return BObstructionManager::cObsTypeUnknown; }
   bool                    isCollisionEnabledWithEntity(BEntity* pEntity, bool collideWithMoving = false);

   float                   getObstructionRadius( void ) const
                           {
                              BASSERT(mObstructionRadiusX > -cFloatCompareEpsilon);
                              BASSERT(mObstructionRadiusZ > -cFloatCompareEpsilon);
                              return(Math::Max(mObstructionRadiusX, mObstructionRadiusZ));
                           }
   float                   getObstructionRadiusX( void ) const { BASSERT(mObstructionRadiusX > -cFloatCompareEpsilon); return(mObstructionRadiusX); }
   float                   getObstructionRadiusY( void ) const { BASSERT(mObstructionRadiusY > -cFloatCompareEpsilon); return(mObstructionRadiusY); }
   float                   getObstructionRadiusZ( void ) const { BASSERT(mObstructionRadiusZ > -cFloatCompareEpsilon); return(mObstructionRadiusZ); }

   void                    setObstructionRadiusX(float v) { mObstructionRadiusX=v; }
   void                    setObstructionRadiusY(float v) { mObstructionRadiusY=v; }
   void                    setObstructionRadiusZ(float v) { mObstructionRadiusZ=v; }

   void                    pitch(float angle); // rotate around X
   void                    yaw(float angle);   // rotate around Y
   void                    roll(float angle);  // rotate around Z

   void                    pitchWorld(float angle);
   void                    pitchWorldAbout(float angle, const BVector point);
   void                    yawWorld(float angle);
   void                    yawWorldAbout(float angle, const BVector point);
   void                    yawWorldRelative(BEntity *pRefObj, float angle, const BVector point);
   void                    rollWorld(float angle);

   void                    setWorldMatrix(const BMatrix& matrix);
   virtual void            getWorldMatrix(BMatrix& matrix) const;
   void                    getInvWorldMatrix(BMatrix& matrix) const;   

   void                    getObstructionHull(BOPQuadHull &obstructionHull) const;

   bool                    isPassable(BVector position, const BEntityIDArray& ignoreList) const;

   bool                    addEventListener(IEventListener*  pListener);
   void                    removeEventListener(IEventListener*  pListener);

   inline BObject*           getObject() { return (isClassType(BEntity::cClassTypeObject) || isClassType(BEntity::cClassTypeUnit) || isClassType(BEntity::cClassTypeDopple) || isClassType(BEntity::cClassTypeProjectile)) ? reinterpret_cast<BObject*>(this) : NULL; }
   inline const BObject*     getObject() const { return (isClassType(BEntity::cClassTypeObject) || isClassType(BEntity::cClassTypeUnit) || isClassType(BEntity::cClassTypeDopple) || isClassType(BEntity::cClassTypeProjectile)) ? reinterpret_cast<const BObject*>(this) : NULL; }
   inline BUnit*             getUnit() { return (isClassType(BEntity::cClassTypeUnit)) ? reinterpret_cast<BUnit*>(this) : NULL; }
   inline const BUnit*       getUnit() const { return (isClassType(BEntity::cClassTypeUnit)) ? reinterpret_cast<const BUnit*>(this) : NULL; }
   inline BSquad*            getSquad() { return (isClassType(BEntity::cClassTypeSquad)) ? reinterpret_cast<BSquad*>(this) : NULL; }
   inline const BSquad*      getSquad() const { return (isClassType(BEntity::cClassTypeSquad)) ? reinterpret_cast<const BSquad*>(this) : NULL; }
   inline BArmy*             getArmy() { return (isClassType(BEntity::cClassTypeArmy)) ? reinterpret_cast<BArmy*>(this) : NULL; }
   inline const BArmy*       getArmy() const { return (isClassType(BEntity::cClassTypeArmy)) ? reinterpret_cast<const BArmy*>(this) : NULL; }
   inline BDopple*           getDopple() { return (isClassType(BEntity::cClassTypeDopple)) ? reinterpret_cast<BDopple*>(this) : NULL; }
   inline const BDopple*     getDopple() const { return (isClassType(BEntity::cClassTypeDopple)) ? reinterpret_cast<const BDopple*>(this) : NULL; }
   inline BPlatoon*          getPlatoon() { return (isClassType(BEntity::cClassTypePlatoon)) ? reinterpret_cast<BPlatoon*>(this) : NULL; }
   inline const BPlatoon*    getPlatoon() const { return (isClassType(BEntity::cClassTypePlatoon)) ? reinterpret_cast<const BPlatoon*>(this) : NULL; }
   inline BProjectile*       getProjectile() { return (isClassType(BEntity::cClassTypeProjectile)) ? reinterpret_cast<BProjectile*>(this) : NULL; }
   inline const BProjectile* getProjectile() const { return (isClassType(BEntity::cClassTypeProjectile)) ? reinterpret_cast<const BProjectile*>(this) : NULL; }
   
   void                    beginMove();
   void                    endMove();

   void                    startMove();
   void                    stopMove();

   const BOPObstructionNode*     getObstructionNode( void )const                 { return mpObstructionNode; }
   const BOPObstructionNode*     getChildObstructionNode(void) const;
   virtual uint            getNumberChildren() const { return (0); }

   bool                    addAction(BAction *pAction, BSimOrder* pOrder=NULL);
   bool                    removeAction( BAction *pAction );
   void                    removeActions(bool ignorePersistent=true);
   bool                    removeActionByID( long id );
   bool                    removeAllActionsOfType(BActionType type) { return (mActions.removeAllActionsOfType(type)); }
   virtual bool            removeAllActionsForOrder(BSimOrder* pOrder) { return (mActions.removeAllActionsForOrder(pOrder)); }
   bool                    changeOrderForAllActions(BSimOrder* pOldOrder, BSimOrder* pNewOrder) { return mActions.changeOrderForAllActions(pOldOrder, pNewOrder); }
   const BAction*          getActionByTypeConst(BActionType type) const { return mActions.getActionByType(type); }
   BAction*                getActionByType(BActionType type) { return mActions.getActionByType(type); }
   long                    getNumberActions() const { return(mActions.getNumberActions()); }
   const BAction*          getActionByIndexConst(long index) const { return(mActions.getAction(index)); }
   BAction*                getActionByIndex(long index) { return(mActions.getAction(index)); }
   BAction*                findActionByID(long id);
   bool                    hasPersistentMoveAction() const;

   inline BEntityID        getParentID( void ) const     { return mParentID;  }
   virtual inline void     setParentID( BEntityID id )      { mParentID = id;    }
   BEntity*                getParent() const;

   void                    sendEvent(BEntityID targetID,  BEntityID senderID, DWORD eventType, DWORD data, DWORD data2=0);

   // IEventListener
   virtual void            notify(DWORD eventType, BEntityID senderID, DWORD data, DWORD data2);
   virtual int             getEventListenerType() const { return cEventListenerTypeEntity; }
   virtual bool            savePtr(BStream* pStream) const;
   
   //Distance methods.
   float                   calculateXZDistance(BVector point) const;
   float                   calculateXZDistanceSqr(BVector point) const;
   float                   calculateXZDistance(const BEntity* pTarget) const;
   float                   calculateXZDistance(BVector myTempPos, BVector point) const;
   float                   calculateXZDistanceSqr(BVector myTempPos, BVector point) const;
   float                   calculateXZDistance(BVector myTempPos, const BEntity* pTarget) const;
   float                   calculateXZDistance(BSimTarget &target);

   virtual float           getPathingRange() const       { return 0.0f; }

   void                    startPhysics( void )  { setFlagPhysicsControl(true);  setTieToGround(false);  }
   void                    stopPhysics( void )   { setFlagPhysicsControl(false); setTieToGround(true);   }
   bool                    createPhysicsObject(const BPhysicsInfo* pInfo, const BPhysicsObjectBlueprintOverrides* pBPOverrides, bool completeOnInactivePhysics, bool bIsPhysicsReplacement);
   bool                    createPhysicsObject(long physicsInfoID, const BPhysicsObjectBlueprintOverrides* pBPOverrides, bool completeOnInactivePhysics, bool bIsPhysicsReplacement);
   bool                    createPhysicsObjectDirect(const BPhysicsObjectParams &params, bool completeOnInactivePhysics);
   void                    releasePhysicsObject( void );
   void                    setupVehiclePhysics();

   BPhysicsObject*         getPhysicsObject() const { return mpPhysicsObject; }
   BVector                 getPhysicsVelocity() const;
   BVector                 getPhysicsForward() const;
   BVector                 getPhysicsPosition() const;
   void                    setPhysicsKeyFramed(bool keyFramed, bool resetPhysics = false);
   bool                    getPhysicsKeyFramed();
   
   virtual float           getMaxVelocity() const { return (0.0f); }
   virtual float           getDesiredVelocity() const { return (0.0f); }
   virtual float           getAcceleration() const { return (0.0f); }
   virtual float           getMovementTolerance() const { return (0.0f); }
   virtual bool            canJump() const;
   virtual bool            canMove(bool allowAutoUnlock=false) const;
   virtual bool            canAutoUnlock() const;
   virtual bool            canAutoLock() const;

   virtual bool            isVisible(BTeamID teamID) const {BASSERT(0); return true;}
   virtual bool            isVisibleOrDoppled(BTeamID teamID) const { return (isVisible(teamID)); }

   //-- entity references
   uint                    getNumberEntityRefs() const { return (mpEntityRefs ? (uint)mpEntityRefs->getNumber() : 0); }
   BEntityRef*             getEntityRefByIndex(uint index) const;
   BEntityRef*             getFirstEntityRefByType(long type) const;
   BEntityRef*             getFirstEntityRefByID(long type, BEntityID id) const;
   BEntityRef*             getFirstEntityRefByData1(long type, short data1) const;
   BEntityRef*             getFirstEntityRefByData2(long type, BYTE data2) const;
   BEntityRef*             addEntityRef(short type, BEntityID id, short data1, BYTE data2);
   void                    removeEntityRef(long type, BEntityID id);
   void                    removeEntityRefByData1(long type, short data1);
   void                    removeEntityRefByData2(long type, BYTE data2);
   void                    removeEntityRefByType(long type);
   void                    removeEntityRef(long index);
   void                    removeAllMatchingEntityRefs(long type, BEntityID id, short data1, BYTE data2);

   long                    getNumGatherUnits(void) const;

   virtual float           getResourceAmount() const { return (0.0f); }
   bool                    validateFacing(BVector targetPosition, float maxAngle);
   virtual void            changeOwner(BPlayerID newPlayerID, BProtoObjectID newPOID = cInvalidProtoObjectID){ }

   bool                    isSameUnitOrSquad(const BEntity* pEntity);

   virtual int             getSelectType(BTeamID teamId) const { return cSelectTypeNone; }
   virtual int             getGotoType() const { return cGotoTypeNone; }

   void                    queryEntityID(BEntityID killedByID, BPlayerID& killerPlayerID, long& killerProtoID, long& killerProtoSquadID);

   #ifndef BUILD_FINAL
   void                    debug(const char* pMsg, ...);
   bool                    getFlagIsTriggered() const { return (mFlagIsTriggered); }
   void                    setFlagIsTriggered(bool v) { mFlagIsTriggered = v; }   
   #endif

   void                    clearFlags();
   //-- Flags
   bool                    getFlagCollidable() const { return(mFlagCollidable); }
   void                    setFlagCollidable(bool v) { mFlagCollidable=v; }
   bool                    getFlagMoving() const { return(mFlagMoving); }
   virtual void            setFlagMoving(bool v) { mFlagMoving=v; }
   bool                    getFlagDestroy() const { return(mFlagDestroy); }
   void                    setFlagDestroy(bool v) { mFlagDestroy=v; }
   bool                    getFlagFirstUpdate() const { return(mFlagFirstUpdate); }
   void                    setFlagFirstUpdate(bool v) { mFlagFirstUpdate=v; }
   bool                    getFlagTiesToGround() const { return(mFlagTiesToGround); }
   void                    setFlagTiesToGround(bool v) { mFlagTiesToGround=v; }
   bool                    getFlagUseMaxHeight() const { return(mFlagUseMaxHeight); }
   void                    setFlagUseMaxHeight(bool v) { mFlagUseMaxHeight=v; }
   bool                    getFlagPhysicsControl() const { return(mFlagPhysicsControl); }
   void                    setFlagPhysicsControl(bool v) { mFlagPhysicsControl=v; }
   bool                    getFlagRotateObstruction() const { return(mFlagRotateObstruction); }
   void                    setFlagRotateObstruction(bool v) { mFlagRotateObstruction=v; }
   bool                    getFlagFlying() const { return(mFlagFlying); }
   void                    setFlagFlying(bool v) { mFlagFlying=v; }
   bool                    getFlagValid() const { return(mFlagValid); }
   void                    setFlagValid(bool v) { mFlagValid=v; }
   bool                    getFlagNonMobile() const { return(mFlagNonMobile); }
   void                    setFlagNonMobile(bool v) { mFlagNonMobile=v; }
   bool                    getFlagLockedDown() const { return(mFlagLockedDown); }
   virtual void            setFlagLockedDown(bool v) { mFlagLockedDown=v; }
   bool                    getFlagEntityRefsLocked() const { return(mFlagEntityRefsLocked); }
   void                    setFlagEntityRefsLocked(bool v) { mFlagEntityRefsLocked=v; }
   bool                    getFlagFlyingHeightFixup() const { return(mFlagFlyingHeightFixup); }
   void                    setFlagFlyingHeightFixup(bool v) { mFlagFlyingHeightFixup=v; }
   bool                    getFlagGarrisoned() const { return(mFlagGarrisoned); }
   void                    setFlagGarrisoned(bool v) { mFlagGarrisoned=v; }
   bool                    getFlagPassiveGarrisoned() const { return(mFlagPassiveGarrisoned); }
   
   void                    setFlagPassiveGarrisoned(bool v) { mFlagPassiveGarrisoned=v; }
   bool                    getFlagAttached() const { return(mFlagAttached); }
   void                    setFlagAttached(bool v) { mFlagAttached=v; }
   bool                    getFlagMoved() const { return(mFlagMoved); }
   void                    setFlagMoved(bool v) { mFlagMoved=v; }
   bool                    getFlagTeleported() const { return(mFlagTeleported); }
   void                    setFlagTeleported(bool v) { mFlagTeleported=v; }
   bool                    getFlagInSniper() const { return (mFlagInSniper); }
   virtual void            setFlagInSniper(bool v) { mFlagInSniper = v; }   
   bool                    getFlagIsBuilt() const { return (mFlagIsBuilt); }
   void                    setFlagIsBuilt(bool v) { mFlagIsBuilt = v; }
   bool                    getFlagHasSounds() const { return(mFlagHasSounds); }
   void                    setFlagHasSounds(bool v) { mFlagHasSounds=v; }
   bool                    getFlagHitched() const { return (mFlagHitched); }
   void                    setFlagHitched(bool v) { mFlagHitched = v; }
   bool                    getFlagSprinting() const { return (mFlagSprinting); }
   virtual void            setFlagSprinting(bool v) { mFlagSprinting = v; }   
   bool                    getFlagRecovering() const { return (mFlagRecovering); }
   virtual void            setFlagRecovering(bool v) { mFlagRecovering = v; }   
   bool                    getFlagInCover() const { return (mFlagInCover); }
   void                    setFlagInCover(bool v) { mFlagInCover = v; }
   bool                    getFlagSelectable() const { return (mFlagSelectable); }
   void                    setFlagSelectable(bool v) { mFlagSelectable = v; }
   bool                    getFlagUngarrisonValid() const { return (mFlagUngarrisonValid); }
   void                    setFlagUngarrisonValid(bool v) { mFlagUngarrisonValid = v; }
   bool                    getFlagGarrisonValid() const { return (mFlagGarrisonValid); }
   void                    setFlagGarrisonValid(bool v) { mFlagGarrisonValid = v; }
   bool                    getFlagIsPhysicsReplacement() const { return (mFlagIsPhysicsReplacement); }
   void                    setFlagIsPhysicsReplacement(bool v) { mFlagIsPhysicsReplacement = v; }
   bool                    getFlagIsDoneBuilding() const { return (mFlagIsDoneBuilding); }
   void                    setFlagIsDoneBuilding(bool v) { mFlagIsDoneBuilding = v; }

   //Halwes - 5/30/2007: Adding this to aid debugging.
   #ifndef BUILD_FINAL
      BSimString*          getName() const {return (mEntityName);}
   #endif

   GFDECLAREVERSION();
   virtual bool save(BStream* pStream, int saveType) const;
   virtual bool load(BStream* pStream, int saveType);
   virtual bool postLoad(int saveType);

protected:
   const BProtoObject*     getProtoObjectForSquad() const;
   void                    setProtoObject(BProtoObject* pProtoObject) { mpProtoObject = pProtoObject; }
   #ifdef BUILD_DEBUG
      void                 verifyProtoObject() const;
   #endif

   float                   calcDistRadiusToPoint(BVector myTempPos, BVector vPoint) const;
   float                   calcDistRadiusToRadius(BVector myTempPos, const BEntity* pTarget) const;
   float                   calcDistObstructionToPoint(BVector myTempPos, BVector vPoint) const;   
   float                   calcDistRadiusToObstruction(BVector myTempPos, const BEntity* pTarget) const;
   float                   calcDistObstructionToRadius(BVector myTempPos, const BEntity* pTarget) const;
   float                   calcDistObstructionToObstruction(BVector myTempPos, const BEntity* pTarget) const;
   float                   calcDistRadiusToPointSqr(BVector myTempPos, BVector vPoint) const;
   float                   calcDistObstructionToPointSqr(BVector myTempPos, BVector vPoint) const;

   BVector                 mPosition;                 // 16 bytes
   BVector                 mRight;                    // 16 bytes
   BVector                 mUp;                       // 16 bytes
   BVector                 mForward;                  // 16 bytes
   BVector                 mVelocity;                 // 16 bytes
   BActionList             mActions;                  // 16 bytes
   BEntityRefArray*        mpEntityRefs;              // 4 bytes
   BEntityID               mID;                       // 4 bytes
   BPlayerID               mPlayerID;                 // 4 bytes
   BProtoObject*           mpProtoObject;             // 4 bytes
   BOPObstructionNode*     mpObstructionNode;         // 4 bytes
   BEntityID               mParentID;                 // 4 bytes
   BPhysicsObject*         mpPhysicsObject;           // 4 bytes
   //BPhantom*               mpPhantom;                 // 4 bytes
   float                   mYDisplacement;            // 4 bytes
   float                   mObstructionRadiusX;       // 4 bytes
   float                   mObstructionRadiusY;       // 4 bytes
   float                   mObstructionRadiusZ;       // 4 bytes

   //DCP 04/23/07: Adding this to aid debugging.
   #ifndef BUILD_FINAL
   BSimString*             mEntityName;               // 4 bytes
   #endif
   
   //Flags
   bool                   mFlagCollidable:1;          // 1 byte   (1/8)
   bool                   mFlagMoving:1;              //          (2/8)
   bool                   mFlagDestroy:1;             //          (3/8)
   bool                   mFlagFirstUpdate:1;         //          (4/8)
   bool                   mFlagTiesToGround:1;        //          (5/8)
   bool                   mFlagUseMaxHeight:1;        //          (6/8)
   bool                   mFlagPhysicsControl:1;      //          (7/8)
   bool                   mFlagRotateObstruction:1;   //          (8/8)

   bool                   mFlagFlying:1;              // 1 byte   (1/8)
   bool                   mFlagValid:1;               //          (2/8)
   bool                   mFlagNonMobile:1;           //          (3/8)
   bool                   mFlagLockedDown:1;          //          (4/8)
   bool                   mFlagEntityRefsLocked:1;    //          (5/8)
   bool                   mFlagFlyingHeightFixup:1;   //          (6/8)
   bool                   mFlagGarrisoned:1;          //          (7/8)
   bool                   mFlagPassiveGarrisoned:1;   //          (8/8)

   bool                   mFlagAttached:1;            // 1 byte   (1/8)
   bool                   mFlagMoved:1;               //          (2/8)
   bool                   mFlagTeleported:1;          //          (3/8)
   bool                   mFlagInSniper:1;            //          (4/8)
   bool                   mFlagIsBuilt:1;             //          (5/8)
   bool                   mFlagHasSounds:1;           //          (6/8)
   bool                   mFlagHitched:1;             //          (7/8)
   bool                   mFlagSprinting:1;           //          (8/8)

   bool                   mFlagRecovering:1;          // 1 byte   (1/8)
   bool                   mFlagInCover:1;             //          (2/8)
   bool                   mFlagSelectable:1;          //          (3/8)
   bool                   mFlagUngarrisonValid:1;     //          (4/8)
   bool                   mFlagGarrisonValid:1;       //          (5/8)
   bool                   mFlagIsPhysicsReplacement:1;//          (6/8)
   bool                   mFlagIsDoneBuilding:1;      //          (7/8) semi-hack flag so the squad knows when a child has finished building

#ifndef BUILD_FINAL
   bool mFlagIsTriggered:1;
#endif
};


XMFINLINE const BProtoObject* BEntity::getProtoObject() const
{
   // Perf:  entity now caches its proto object pointer for non-squads.
   if (!isClassType(cClassTypeSquad))
   {
      #ifdef BUILD_DEBUG
         verifyProtoObject();
      #endif
      return mpProtoObject;
   }

   // Squads are screwy and can either return their leader's proto object or a proto object
   // only they know about.  So for less confusion they do a lookup similar to before.
   return (getProtoObjectForSquad());
}

//==============================================================================
// squadplotter.h
//
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================
#pragma once

//==============================================================================
//Includes.
#include "obstructionmanager.h"
#include "syncmacros.h"
#include "gamefilemacros.h"

//==============================================================================
//Forward declarations.
class BCommand;
class BSquad;
class BObject;
class BProtoAction;

//==============================================================================
// BSquadPlotterResult
//==============================================================================
class BSquadPlotterResult
{
   public:
      //Constructors and Destructor.
      BSquadPlotterResult( void );
      ~BSquadPlotterResult( void );

      //Squad.
      BEntityID                  getSquadID( void ) const { return(mSquadID); }
      void                       setSquadID( BEntityID v ) { mSquadID=v; }
      //Width.
      float                      getWidth( void ) const { return(mWidth); }
      void                       setWidth( float v ) { mWidth=v; }
      //Depth.
      float                      getDepth( void ) const { return(mDepth); }
      void                       setDepth( float v ) { mDepth=v; }
      //Range.
      float                      getRange( void ) const { return(mRange); }
      void                       setRange( float v ) { mRange=v; }
      //TrueActionRange.  This is the range retrieved from the proto action, not the reduced range value used to make
      //sure the squad gets placed well within range.
      float                      getTrueActionRange( void ) const { return(mTrueActionRange); }
      void                       setTrueActionRange( float v ) { mTrueActionRange=v; }
      //ProtoAction.
      const BProtoAction*        getProtoAction( void ) const { return(mpProtoAction); }
      void                       setProtoAction( BProtoAction* v ) { mpProtoAction=v; }
      //UsingOverrideRange
      bool                       getUsingOverrideRange() const { return mUsingOverrideRange; }
      void                       setUsingOverrideRange(bool v) { mUsingOverrideRange = v; }
      //InRange.
      bool                       getInRange( void ) const { return(mInRange); }
      void                       setInRange( bool v ) { mInRange=v; }
      //Waypoints.
      const BDynamicSimVectorArray&  getWaypoints( void ) const { return(mWaypoints); }
      bool                       setWaypoints( const BDynamicSimVectorArray & v);
      void                       zeroWaypoints( void ) { mWaypoints.setNumber(0); }
      //Desired Position.
      const BVector&             getDesiredPosition( void ) const { return(mDesiredPosition); }
      void                       setDesiredPosition( const BVector &v ) { mDesiredPosition=v; }
      // Obstructed
      bool                       isObstructed() const { return mObstructed; }
      void                       setObstructed(bool obstructed) { mObstructed = obstructed; }
      //Movement type.
      long                       getMovementType() const { return mMovementType; }
      void                       setMovementType(long v) { mMovementType=v; }
      // Is default desired position.
      bool                       getDefaultDesiredPos() const { return mDefaultDesiredPos; }
      void                       setDefaultDesiredPos(bool v) { mDefaultDesiredPos = v; }

      //Render methods.
      void                       render( void ) const;
      //Reset.
      void                       reset( void );

      bool                       getSquadCorners(BVector *pts) const; //Returns 4 points

      GFDECLAREVERSION();
      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   protected:
      //CleanUp cleans and deallocates everything.
      void                       cleanUp( void );

      BEntityID                  mSquadID;
      float                      mWidth;
      float                      mDepth;
      float                      mRange;
      float                      mTrueActionRange;
      BProtoAction*              mpProtoAction;
      BDynamicSimVectorArray     mWaypoints;
      BVector                    mDesiredPosition;
      long                       mMovementType;
      bool                       mObstructed:1;
      bool                       mInRange:1;
      bool                       mUsingOverrideRange:1;
      bool                       mDefaultDesiredPos:1;
};


//==============================================================================
// BSquadPlotterProjectileGroup
//==============================================================================
class BSquadPlotterProjectileGroup
{
   public:
      //Constructors and Destructor.
      BSquadPlotterProjectileGroup( void );   
      BSquadPlotterProjectileGroup( const BProtoAction* pProtoAction );
      ~BSquadPlotterProjectileGroup( void );   

      //ProtoAction
      const BProtoAction*        getProtoAction( void ) const { return(mpProtoAction); }
      void                       setProtoAction( const BProtoAction* v ) { mpProtoAction=v; }

      //Desired Position.
      const BDynamicSimUIntArray& getSquadIndices( void ) const { return(mSquadsIndices); }

      void                       addSquadIndex( uint v ) { mSquadsIndices.add(v); }

      //Reset.
      void                       reset( void );

      //CleanUp cleans and deallocates everything.
      void                       cleanUp( void );



   protected:

      const BProtoAction*        mpProtoAction;
      BDynamicSimUIntArray       mSquadsIndices;            // squad indices
};


//==============================================================================
// BSurroundPosition
//==============================================================================
class BSurroundPosition
{
   public:
      //Constructors and Destructor.
      BSurroundPosition() {}
      BSurroundPosition( BVector pos )
         {
            mPosition = pos;
            mbIsOccupied = false;
            mbIsObstructed = false;
         }

      BVector     mPosition;
      bool        mbIsOccupied;        // Indicates is a squad is already set to go to this space
      bool        mbIsObstructed;      // Indicates if the space is ObstructionManager obstructed
};


//==============================================================================
// BSquadPlotter
//==============================================================================
class BSquadPlotter
{
   public:

      enum
      {
         cPushOffThreshold = 2
      };

      enum
      {
         cSPFlagIgnorePlayerRestriction = (1 << 0),
         cSPFlagIgnoreWeaponLOS         = (1 << 1),
         cSPFlagSkipReset               = (1 << 2),
         cSPFlagIgnoreForwardOnly       = (1 << 3),
         cSPFlagForceMove               = (1 << 4),
         cSPFlagForceLandMovementType   = (1 << 5),         
         cSPFlagNoSync                  = (1 << 6),
      };

      //Constructors and Destructor.
      BSquadPlotter( void );
      ~BSquadPlotter( void );

      //PlotSquads.
      bool                       plotSquads( const BEntityIDArray& squads, const BCommand* command, DWORD flags = 0);     
      bool                       plotSquads( const BEntityIDArray& squads, long playerID, BEntityID targetID, const BDynamicSimVectorArray& waypoints, 
                                             BVector targetPosition, long abilityID, DWORD flags, float overrideRange = -1.0f, 
                                             BVector overrideTargetPos = cInvalidVector, float overrideRadius = 0.0f);
      bool                       debugPlotSquads( BVector &point );

      //Results.
      long                       getNumberResults( void ) const { return(mResults.getNumber()); }
      const BDynamicSimArray<BSquadPlotterResult>& getResults( void ) const { return(mResults); }

      //Render methods.
      void                       render( void ) const;
      //Reset.
      void                       reset( void );

   protected:
      // Movement positioning
      void                       makeCircularConfiguration(float totalWidth, float orientDistance,
                                                           BVector &orientForward, BVector &orientRight,
                                                           BVector &orientPosition, BDynamicSimVectorArray &resultWaypoints, DWORD flags);
      void                       makeRowConfiguration(float totalWidth, float orientDistance,
                                                      BVector &orientForward, BVector &orientRight,
                                                      BVector &orientPosition, BDynamicSimVectorArray &resultWaypoints, DWORD flags);
      void                       make2ColumnConfiguration(float orientDistance, BVector &orientForward, BVector &orientRight,
                                                          BVector &orientPosition, BDynamicSimVectorArray &resultWaypoints, DWORD flags);
      void                       makeColumnConfiguration(float totalDepth, float orientDistance, BVector &orientForward,
                                                         BVector &orientRight, BVector &orientPosition,
                                                         BDynamicSimVectorArray &resultWaypoints, DWORD flags);
      
      // Targeted positioning
      void                       makeSurroundPositionConfiguration(BVector& orientForward, BVector& orientRight, BVector& orientPosition, 
                                                                   BDynamicSimVectorArray& resultWaypoints, float minRange, BEntity* pTargetEntity, DWORD flags,
                                                                   BVector overrideTargetPos = cInvalidVector, float overrideRadius = 0.0f);

      void                       makeRangedArcConfiguration(float orientDistance, BVector &orientForward, BVector &orientRight,
                                                           BVector &orientPosition, BDynamicSimVectorArray &resultWaypoints, float minRange, BEntity *pTargetEntity, DWORD flags);

      void                       make2ColumnConfigurationTarget(float orientDistance, BVector &orientForward, BVector &orientRight,
                                                          BVector &orientPosition, BDynamicSimVectorArray &resultWaypoints, float minRange, DWORD flags);

      void                       makeColumnConfigurationTarget(float totalDepth, float orientDistance, BVector &orientForward,
                                                         BVector &orientRight, BVector &orientPosition,
                                                         BDynamicSimVectorArray &resultWaypoints, float minRange, DWORD flags);

      // Old stuff
      void                       makeOldRowConfiguration(float totalWidth, float orientDistance,
                                                      BVector &orientForward, BVector &orientRight,
                                                      BVector &orientPosition, BDynamicSimVectorArray &resultWaypoints,
                                                      BCommand *pCommand, DWORD flags);

      enum SQUAD_OBS_CHECK
      {
         cForwardOnly,
         cBackwardOnly,
         cForwardAndBackward,
         cNumObsCheckTypes
      };

      bool                       checkAgainstObstructions(SQUAD_OBS_CHECK type);
      bool                       checkAgainstObstructions(BDynamicSimVectorArray &hullPoints, BObstructionNodePtrArray &obstructions, long movementType);

      bool                       moveOffObstructions(long squadIndex, SQUAD_OBS_CHECK type);
      bool                       findClosestUnobstructedPosition(long squadIndex, BVector &newPos, SQUAD_OBS_CHECK type);
      bool                       findClosestUnobstructedPosition(BVector &pos, BVector &forward, float width, BVector &newPos, SQUAD_OBS_CHECK type, long movementType);      
      void                       calculateSurroundPositions(BEntity* pTargetEntity, float range, long largestSquadIndex, BDynamicArray<BSurroundPosition>& positions, uint numSquads, BVector orientPosition, BVector overrideTargetPos = cInvalidVector, float overrideRadius = 0.0f);
      void                       calculateUnobstructedSurroundPositions(BEntity* pTargetEntity, long largestSquadIndex, const BEntityIDArray& ignoreList, BDynamicArray<BSurroundPosition>& surroundPositions, const BDynamicSimLongArray& positionSortOrderAll, const BSquadPlotterProjectileGroup* pProjectileGroup, BDynamicSimLongArray& positionSortOrder, DWORD flags);
      float                      calculateDistanceAroundRoundedBox(BEntity* pTargetEntity, BVector flCorner, BVector frCorner, BVector brCorner, BVector blCorner, float range, BVector orientPosition);
      void                       makeQuadPoints(BVector &pos, BVector& forward, float width, float depth, BDynamicSimVectorArray &points);

      void                       filterObstructions(BObstructionNodePtrArray &obstructions);

      void                       addTempObstruction(BVector &pos, float width, float depth, BVector &forward);
      void                       clearTempObstructions();

      //CleanUp cleans and deallocates everything.
      void                       cleanUp( void );

      BVector                    getEdgeOfMapOffset(BVector point);
      void                       pushSquadsOffEdgeOfMap();

      //==============================================================================================================================
      // SLB: TEMP HACK
      //==============================================================================================================================
      void                       positionSquadsForWeaponLOSToTarget(const BEntity *pTargetEntity, long playerID, BEntityID targetID, const BDynamicSimVectorArray& waypoints, BVector targetPosition, long abilityID, DWORD flags);
      float                      percentageSquadsHaveWeaponLOSToTarget(const BEntity *pTargetEntity, BVector platoonPosition, BVector &leftIntersectionPoint, BVector &rightIntersectionPoint) const;
      bool                       squadHasWeaponLOSToTarget(int32 squadIndex, const BEntity *pTargetEntity, bool &leftIntersection, BVector &leftIntersectionPoint, bool &rightIntersection, BVector &rightIntersectionPoint) const;
      bool                       positionHasWeaponLOSToTarget(BVector sourcePosition, BVector targetPosition, BVector &intersectionPoint) const;
      //==============================================================================================================================

      // Syncing stuff
      #ifdef SYNC_Squad
         void                    syncResults();
      #endif

      //Working vars.
      BEntityIDArray             mSquads;
      long                       mBaseMovementType;
      BEntityIDArray             mIgnoreList;
      
      //Results.
      BDynamicSimArray<BSquadPlotterResult> mResults;

      // Temp calculation stuff
      static BDynamicSimUIntArray                     mTempSquadOrder;
      static BDynamicSimArray<BOPObstructionNode*>    mTempObstructions;
};

extern BSquadPlotter gSquadPlotter;
//==============================================================================
// camera.h
//
// Copyright (c) 1997-2001 Ensemble Studios
//
// 7/28/03 DLM
// Introducing the notion of "overrides" for the camera.  The camera currntly has
// modes - Strategy (game mode), and Track (cinematic) being the only two supported.
// those modes determin how the camera is updated during BGameUIState::updateCamera.
// The "overrides" are (currently) activated by triggers, and will serve to further
// refine how the camera behaves.  We still want to allow the modes to operate,
// and to do their thing, but the overrides get last crack.  The reason we don't
// just supplant the existing modes with a new mode is we *still* need the camera
// to be able to go into cinimatic mode, say for example ("track" mode), but while
// moving along the track we want the camera's orientation to be overriden to look
// at a specific unit. 
//==============================================================================
//==============================================================================

#pragma once

//==============================================================================
// Includes

#include "math\vector.h"
#include "timer.h"
#include "interptable.h"

class BChunkReader;
class BChunkWriter;
class BXMLNode;


//==============================================================================
// Constants
const float cDefaultShakeTrailOffDuration = 0.4f;
const float cDefaultShakeConservationFactor = 0.5f;
const DWORD cDefaultShakeTrailOffDurationDWORD = (DWORD)(1000.0f * cDefaultShakeTrailOffDuration);

//==============================================================================
//==============================================================================
class BCameraEffectData
{
   public:

      BCameraEffectData() : mRadialBlur(false), mUse3DPosition(false), mModeCameraEffect(false), mUserHoverPointAs3DPosition(false) {}

      void                 reset();
      void                 makeClearData(DWORD duration, uint viewportIndex);
      void                 setInitialValuesToCurrent(float currentZoom, float currentYaw, float currentPitch, uint viewportIndex);
      float                getMaxTime() const;

      void                 load(BXMLNode& node);

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

      BSimString           mName;
      BVectorLerpTable     mColorTransformRTable;
      BVectorLerpTable     mColorTransformGTable;
      BVectorLerpTable     mColorTransformBTable;
      BFloatLerpTable      mColorTransformFactorTable;
      BFloatLerpTable      mBlurFactorTable;
      BVector2LerpTable    mBlurXYTable;
      BVector2LerpTable    mRadialBlurFactorTable;
      BFloatLerpTable      mFOVTable;
      BFloatLerpTable      mZoomTable;
      BFloatLerpTable      mYawTable;
      BFloatLerpTable      mPitchTable;
      bool                 mRadialBlur : 1;
      bool                 mUse3DPosition : 1;
      bool                 mModeCameraEffect : 1;
      bool                 mUserHoverPointAs3DPosition : 1;
};


//==============================================================================
// BCamera Class
//==============================================================================
class BCamera
{
   public:
      enum
      {
         cCameraZoomNear=0,
         cCameraZoomNormal,
         cCameraZoomFar,
         cCameraZoomVeryFar,
         cCameraNumZoomTypes
      };
      enum
      {
         cStrategyMovement,
         cTrackMovement,                           // Follow Camera Track
         cRecordMovement,
         cNoMovement,                              
         cFollowUnitMovement,
         cNumberBaseMovementTypes
      };

      BCamera(void);
      BCamera(BVector loc, BVector dir, BVector up);

      const BVector           &getCameraLoc() const {return mCameraLoc;}
      const BVector           &getCameraDir() const {return mCameraDir;}
      const BVector           &getCameraUp()   const {return mCameraUp;}
      const BVector           &getCameraRight() const {return mCameraRight;}      
      void                    getViewMatrix(BMatrix &result) const;
      void                    getInverseViewMatrix(BMatrix &result) const;
      bool                    getBoundingBoxOn() const {return mBoundingBoxOn;}
      const BVector           &getBoundingBoxMin() const {return mBoundingBoxMin;}
      const BVector           &getBoundingBoxMax() const {return mBoundingBoxMax;}     
      
      void                    setCameraLoc(const BVector &loc);
                              
      void                    setCameraDir(const BVector &dir) {mCameraDir = dir; mMoved=true; mLastMovedTime = mTimer.getElapsedMilliseconds(); }
      void                    setCameraUp(const BVector &up) {mCameraUp = up; mMoved=true; mLastMovedTime = mTimer.getElapsedMilliseconds(); }
      void                    calcCameraRight();
      void                    setCameraRight(const BVector &right) {mCameraRight = right; mMoved=true; mLastMovedTime = mTimer.getElapsedMilliseconds(); }
      void                    setBoundingBoxOn(bool boxOn) {mBoundingBoxOn = boxOn;}
      void                    setBoundingBox(const BVector &boxMin, const BVector &boxMax);
      void                    setPitchLimitOn(bool pitchLimitOn) {mPitchLimitOn = pitchLimitOn;}
      bool                    getPitchLimitOn() const {return mPitchLimitOn;}
      void                    setPitchMin(float angle) {mPitchMin = angle;}
      float                   getPitchMin() const {return mPitchMin;}
      void                    setPitchMax(float angle) {mPitchMax = angle;}
      float                   getPitchMax() const {return mPitchMax;}

      //FOV.
      float                   getFOV( void ) const { return(mFOV); }
      void                    setFOV( float v ) { mFOV=v; }

      void                    moveForward(float d); 
      void                    moveRight(float d); 
      void                    moveUp(float d);

      void                    moveWorldForward(float d);
      void                    moveWorldRight(float d);
      void                    moveWorldUp(float d);

      void                    yaw(float a);
      void                    pitch(float a);
      void                    roll(float a);

      void                    yawWorld(float a);
      void                    yawWorldAbout(float a, const BVector &point);
      void                    pitchWorld(float a);
      void                    pitchWorldAbout(float a, const BVector &point);
      void                    rollWorld(float a);

      void                    setDesiredPosition( const BVector& v ) { mDesiredPosition=v; }
      const BVector&          getDesiredPosition( void ) const { return(mDesiredPosition); }
      void                    setDesiredPerspective( const BVector& v ) { mDesiredPerspective=v; }
      const BVector&          getDesiredPerspective( void ) const { return(mDesiredPerspective); }
      void                    setDesiredRelativePosition( float, float, float );
      const BVector&          getDesiredRelativePosition( void ) const { return(mDesiredRelativePosition); }
      void                    setDesiredRelativePerspective( float, float, float );
      const BVector&          getDesiredRelativePerspective( void ) const { return(mDesiredRelativePerspective); }

      BYTE                    getPositioningType( void ) const { return(mPositioningType); }
      void                    setPositioningType( BYTE );

      float                   getMaximumPositionDifference( void ) const { return(mMaximumPositionDifference); }
      void                    setMaximumPositionDifference( float v ) { mMaximumPositionDifference=v; }
      float                   getMaximumPositionChange( void ) const { return(mMaximumPositionChange); }
      void                    setMaximumPositionChange( float v ) { mMaximumPositionChange=v; }
      float                   getPositionChangeSpeed( void ) const { return(mPositionChangeSpeed); }
      void                    setPositionChangeSpeed( float v ) { mPositionChangeSpeed=v; }
      float                   getPositionInertia( void ) const { return(mPositionInertia); }
      void                    setPositionInertia( float v ) { mPositionInertia=v; }
      bool                    getMoving( void ) const { return(mMoving); }
      void                    setMoving( bool v ) { mMoving=v; }

      bool                    positionForStrategy(DWORD elapsed, bool heightAdjust, const BVector *pLookAtPos = NULL);      

      float                   getSpeed( void ) { return(mSpeed); }
      void                    setSpeed( float v ) { mSpeed=v; }
      float                   getTurnspeed( void ) { return(mTurnspeed); }
      void                    setTurnspeed( float v ) { mTurnspeed=v; }

      BYTE                    getMovementType( void ) { return(mMovementType); }
      void                    setMovementType( BYTE v ); 
      void                    setLastMovementType( BYTE v ) { mLastMovementType = v; } 
      BYTE                    getLastMovementType( void ) { return mLastMovementType; } 

      virtual char*           movementTypeName( void );
      void                    incrementMovementType( void );
      void                    decrementMovementType( void );

      void                    clearMoved(void) {mMoved = false;}
      void                    setMoved(void) {mMoved = true; mLastMovedTime = mTimer.getElapsedMilliseconds(); }
      bool                    getMoved(void) const {return mMoved;}
      DWORD                   getLastMovedTime() { return mLastMovedTime; }

      void                    setBob(bool bob);
      bool                    getBob(void) {return(mBob);}

      void                    debugList( void );

      // Old save and load.
      bool                    save(BChunkWriter *chunkWriter, bool scenario);
      bool                    load(BChunkReader *chunkReader, bool scenario);

      // These make a self-contained camera file.
      bool                    saveFull(long dirID, const char *filename);
      bool                    loadFull(long dirID, const char *filename);

      // These aren't really "zoom" in the FOV sense.  They really refer to "zooming" by translating
      // the camera forward/backward along its direction vector.  The distances are from the plane y=0;
      void                    zoomTo(float d);
      bool                    getZoomLimitOn(void) const {return(mZoomLimitOn);}
      void                    setZoomLimitOn(bool on) {mZoomLimitOn=on;}
      float                   getZoomDistMin(void) const {return(mZoomDistMin);}
      void                    setZoomDistMin(float d) {mZoomDistMin=d;}
      float                   getZoomDistMax(void) const {return(mZoomDistMax);}
      void                    setZoomDistMax(float d) {mZoomDistMax=d;}

      // Look at an entity (or unit)
      void                    lookAtEntity(const BVector &entityDir, BVector entityPos, float cameraDistance, float cameraHeight, float lookAheadDistance, float lookAtHeight, bool followUnitHeight, bool keepCameraAboveGround, bool smoothMove, float time);
      void                    setLookAtCameraSpeed(float val) { mLookAtCameraSpeed=val; }
      float                   getLookAtCameraSpeed() { return mLookAtCameraSpeed; }
      void                    resetLookAtEntity() { mHaveLastLookAtPos=false; }
      void                    adjustLastLookAtPos(float lookAheadDistanceChange);

      bool                    lookAtPosFixedY(const BVector &pos);

      void                    checkCameraShake(void);
      void                    beginShake(float duration, float strength, float trailOffDuration = cDefaultShakeTrailOffDuration, float conservationFactor = cDefaultShakeConservationFactor);
      void                    beginShake(DWORD duration, float strenth, DWORD trailOffDuration = cDefaultShakeTrailOffDurationDWORD, float conservationFactor = cDefaultShakeConservationFactor);
      void                    clearShake() { beginShake(0.0f, 0.0f); } // zero out any current shake

      void                    beginCameraEffect(DWORD curTime, BCameraEffectData* pCamEffectData, const BVector* loc, uint viewportIndex);
      bool                    updateCameraEffect(DWORD curTime, BVector hoverPoint = cOriginVector);
      void                    clearCameraEffect(DWORD curTime, DWORD duration, uint viewportIndex);

      bool                    getXZZoom(float &dist);
      bool                    getXZPitch(float &angle);
      float                   getXZYaw();

      void                    setOverride(long lOverride)
                              { mOverride = lOverride; }
      long                    getOverride(void)
                              { return mOverride; }

      void                    restrictToBox();

   protected:

      bool                    detectObstructed(BVector point1, BVector point2, float upDownOffset);
      BVector                 safeMoveCamera(BVector oldPos, BVector newPos);

      BVector                 mCameraLoc;
      BVector                 mCameraDir, mCameraUp, mCameraRight;
      BVector                 mBoundingBoxMin, mBoundingBoxMax;
      float                   mPitchMin, mPitchMax;

      float                   mZoomDistMin, mZoomDistMax;

      float                   mFOV;

      BVector                 mDesiredPosition;
      BVector                 mDesiredPerspective;
      BVector                 mDesiredRelativePosition;
      BVector                 mDesiredRelativePerspective;
      float                   mMaximumPositionDifference;
      float                   mMaximumPositionChange;
      float                   mPositionChangeSpeed;
      float                   mPositionInertia;
      float                   lastPositionDifferenceLength;

      float                   mSpeed;
      float                   mTurnspeed;

      float                   mBobOffsetVertical, mBobOffsetHorizontal;

      // Look at entity variables
      BVector                 mLastLookAtPos;
      BVector                 mLastEntityPos;
      BVector                 mLastEntityDir;
      float                   mLookAtCameraSpeed;


      // Camera shake stuff
      // TODO: make the camera shake along view plane instead of along XZ plane (for better-looking shake)
      DWORD                   mShakeTime;
      float                   mShakeStrength;
      BVector                 mAccumulatedShake;
      // this specifies the "cool-down" period, after mShakeTime,
      // where the camera is slowing down its shake
      DWORD                   mShakeTrailOffDuration;
      // how strongly we want to keep the shaking centered around
      // the "real" current camera position
      float                   mShakeConservationFactor;

      // Camera effect stuff
      DWORD                   mEffectStartTime;
      BCameraEffectData*      mpCurrentCamEffectData;
      BCameraEffectData       mClearCamEffectData;
      BVector                 mCurrentCamEffectLocation;
      uint                    mCamEffectViewportIndex;

      // Save/load.
      static const DWORD      msSaveVersion;

      BTimer                  mTimer;
      DWORD                   mLastMovedTime;

      long                    mOverride;

      bool                    mBoundingBoxOn;
      bool                    mPitchLimitOn;
      bool                    mZoomLimitOn;
      bool                    mMoving;
      bool                    mLimitedLastTime;
      bool                    mMoved;

      bool                    mBob;
      bool                    mHaveLastLookAtPos;

      char                    mMovementType;
      BYTE                    mLastMovementType;      

      BYTE                    mPositioningType;
};

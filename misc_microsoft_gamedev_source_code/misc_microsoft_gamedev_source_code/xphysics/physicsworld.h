//==============================================================================
// physicsworld.h
//
// Copyright (c) 2003-2004, Ensemble Studios
//==============================================================================

#pragma once

//==============================================================================
// Includes
#pragma warning( disable : 4244 )

//Must undefine new before including any havok headers. mwc
#pragma push_macro("new")
#undef new
   #include "havokMemoryOverride.h"
#pragma pop_macro("new")

#include "threading\win32Semaphore.h"


// This is based on the scale of a marine.  3.875 game units = 1.75 meters (average height of a human)
//
#define     GAMEUNITS_PER_METER_FACTOR       2.2142857
#define     METER_PER_GAMEUNITS_FACTOR       1 / GAMEUNITS_PER_METER_FACTOR 


//==============================================================================
// Forward declarations
class hkVisualDebugger;
class hkpPhysicsContext;
class BPhysicsObject;
class BPhysicsObjectBlueprintOverrides;
class hkpConvexVerticesShape;

//==============================================================================
//==============================================================================
class BPhysicsWorld;
struct BPhysicsWorkerCallbackData : public BEventPayload
{
   BPhysicsWorld* mpPhysicsWorld;
   uint           mNumUpdates;
   uint           mNumWorkerThreads;
   float          mUpdateTime;

   virtual void deleteThis(bool delivered) {}
};

//==============================================================================
// BPhysicaWorld
//==============================================================================
class BPhysicsWorld
{
   public:

      enum
      {
         cPropertyEntityReference,
         cPropertyCharacterControllerImpactObject,
         cPropertyRagdollBoneIndex,
         cPropertyTerrainNode,
         cPropertyPhysicsVehiclePtr,
         cNumProperties,
      };

      BPhysicsWorld( void );
      virtual ~BPhysicsWorld( void );

      //-- Initialization\Shutdown
      bool                             setup(bool multithreaded);
      bool                             shutdown(void);

      //-- update
      bool                             update(float elapsed);
      bool                             updateSingleThreaded(uint numUpdateSteps, float updateTime);
      bool                             updateMainThreadAsync(uint numUpdateSteps, uint numWorkerThreads, float updateTime);
      bool                             updateWorkerThreadAsync(uint numUpdateSteps);
      void                             releaseUpdateCompletionSemaphore() { mUpdateCompleteSemaphore.release(); }
      void                             waitForUpdateCompletion();

      // Moved to physicsobject (SAT)
      /*
      //-- helper functions for building objects
      bool                       setupObjectRepresentation(const BPhysicsObjectBlueprint &blueprint, const BVector &position, const BPhysicsMatrix &rotation, 
                                    DWORD userdata, BPhysicsObject* pObject, bool fixed, const BPhysicsObjectBlueprintOverrides *pOverrides = NULL);
      */

      // Collision layers and filters
      enum
      {
         cLayerZero = 0,
         cLayerTerrain,
         cLayerVehicles,
         cLayerPhantoms,
         cLayerObjects,
         cLayerBuildings,
         cLayerUncollidables,
         cLayerNoTerrainVehicles
      };
      const hkpCollisionFilter*        getCollisionFilter(void) { return mpHavokWorld->getCollisionFilter(); }
      long                             createCollisionFilterInfo(uint layer = cLayerObjects);
      long                             findContactSurface(hkpRigidBody *hkGroundRB, const hkpShapeKey &key) const;
      long                             getVehicleCollisionFilterInfo() const { return mVehicleCollisionFilter; }
      long                             getNoTerrainVehicleCollisionFilterInfo() const { return mNoTerrainVehicleCollisionFilter; }
      long                             getPhantomCollisionFilterInfo() const { return mPhantomCollisionFilter; }
      long                             getBuildingCollisionFilterInfo() const { return mBuildingCollisionFilter; }
      long                             getUncollidableCollisionFilterInfo() const { return mUncollidableCollisionFilter; }
      const hkpConvexVerticesShape*    getVehicleShape() const { return mpVehicleShape; }

      //-- read/write access
      void                             markForWrite(bool flag);
      void                             markForRead(bool flag);

      // Accessors
      hkpWorld*                        getHavokWorld( void )  { return mpHavokWorld; }
      bool                             isSettling() const { return mFlagSettling; }

      #ifndef BUILD_FINAL
         double                        getLastWaitTime() const { return mLastWaitTime; }
         //-- debugging
         void                          startHavokDebugger();
         void                          stepHavokDebugger(void);
         void                          stopHavokDebugger();
         void                          updateHavokDebuggerCamera(BVector from, BVector to, BVector up, float nearPlane, float farPlane, float fov);
      #endif


   protected:

      #ifndef BUILD_FINAL
         double                        mLastWaitTime;
      #endif
      hkpWorld*                        mpHavokWorld;
      hkpConvexVerticesShape*          mpVehicleShape;

      #ifndef BUILD_FINAL
         hkVisualDebugger*             mpVisualDebugger;
         hkpPhysicsContext*            mpVDBContext;
      #endif

      BPhysicsWorkerCallbackData       mPhysicsWorkerCallbackData;
      BWin32Semaphore                  mUpdateCompleteSemaphore;
      BWin32Semaphore                  mWorkerThreadFinishedSemaphore;
      BWin32Semaphore                  mStepInitSemaphore;
      BWin32Semaphore                  mStepEndSemaphore;
      long                             mVehicleCollisionFilter;
      long                             mNoTerrainVehicleCollisionFilter;
      long                             mPhantomCollisionFilter;
      long                             mBuildingCollisionFilter;
      long                             mUncollidableCollisionFilter;
      bool                             mInitialized : 1;
      bool                             mFlagSettling : 1;
      bool                             mWaitForUpdateCompletion : 1;
};

//==============================================================================
//==============================================================================
class BScopedPhysicsWrite
{
   public:
      BScopedPhysicsWrite( void )                        { mpWorld = NULL; }
      BScopedPhysicsWrite(BPhysicsWorld *pWorld)         { mpWorld = pWorld; mpWorld->markForWrite(true); }
      ~BScopedPhysicsWrite(  void )                      { if (mpWorld) mpWorld->markForWrite(false); }

      void init( BPhysicsWorld *pWorld )                 {BASSERT(mpWorld == NULL);  mpWorld = pWorld; if (mpWorld) mpWorld->markForWrite(true); }

   protected:
      BPhysicsWorld* mpWorld;
};

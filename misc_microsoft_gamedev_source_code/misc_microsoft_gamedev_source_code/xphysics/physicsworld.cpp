//==============================================================================
// physicsworld.cpp
//
// Copyright (c) 2003-2004, Ensemble Studios
//==============================================================================
 
// Includes
#include "common.h"
#include "physicsworld.h"
#include "physicsobjectblueprint.h"
#include "shape.h"
#include "configsphysics.h"
#include "xphysics.h"
#include "Physics/Internal/PreProcess/ConvexHull/hkpGeometryUtility.h"

#ifndef BUILD_FINAL
   #include "Timer.h"
   #include "Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"
   #include "Common/Visualize/hkVisualDebugger.h"
#endif


//==============================================================================
// Constants
const float cFixedUpdate = 0.0125f;
const uint cMaxPhysicsThreadIndex = 5;
const uint cPhysicsThreadOrder[cMaxPhysicsThreads] = { 1, 3, 4, 5, 0, 2 };



//==============================================================================
// BPhysicsWorld::BPhysicsWorld
//==============================================================================
BPhysicsWorld::BPhysicsWorld(void) : 
   mpHavokWorld(NULL),
   mpVehicleShape(NULL),
   mVehicleCollisionFilter(-1),
   mNoTerrainVehicleCollisionFilter(-1),
   mPhantomCollisionFilter(-1),
   mBuildingCollisionFilter(-1),
   mUncollidableCollisionFilter(-1),
   mInitialized(false),
   mFlagSettling(false),
   mWaitForUpdateCompletion(false),
   mUpdateCompleteSemaphore(0, 1),
   mWorkerThreadFinishedSemaphore(0, cMaxPhysicsThreads - 1),
   mStepInitSemaphore(0, cMaxPhysicsThreads - 1),
   mStepEndSemaphore(0, cMaxPhysicsThreads - 1)
   #ifndef BUILD_FINAL
      ,mLastWaitTime(0.0)
      ,mpVisualDebugger(NULL)
      ,mpVDBContext(NULL)
   #endif
{
}

//==============================================================================
// BPhysicsWorld::~BPhysicsWorld
//==============================================================================
BPhysicsWorld::~BPhysicsWorld(void)
{
   shutdown();
}


//==============================================================================
// BPhysicsWorld::setup
//==============================================================================
bool BPhysicsWorld::setup(bool multithreaded)
{
   mInitialized = true;
   if (mpHavokWorld)
   {
      shutdown();
   }

   // Create the physics world
   hkpWorldCinfo worldInfo;
   if (multithreaded)
   {
      worldInfo.m_simulationType = hkpWorldCinfo::SIMULATION_TYPE_MULTITHREADED;
      worldInfo.m_processActionsInSingleThread = false;
   }
   else
   {
      //worldInfo.m_simulationType = hkpWorldCinfo::SIMULATION_TYPE_CONTINUOUS;
      worldInfo.m_simulationType = hkpWorldCinfo::SIMULATION_TYPE_DISCRETE;
   }
   worldInfo.setupSolverInfo(hkpWorldCinfo::SOLVER_TYPE_4ITERS_MEDIUM);
   //worldInfo.m_gravity = hkVector4(0.0f, -gConfigSystem.getFloat(cConfigGravity, 9.8f), 0.0f);	
   worldInfo.m_gravity = hkVector4(0.0f, -9.8f * GAMEUNITS_PER_METER_FACTOR, 0.0f);	
   //worldInfo.m_gravity = hkVector4(0.0f, 0.0f, 0.0f);	
   worldInfo.setBroadPhaseWorldSize(2850.0f);

#pragma push_macro("new")
#undef new
   mpHavokWorld = new hkpWorld(worldInfo);
#pragma pop_macro("new")
   
   markForWrite(true);

   hkpAgentRegisterUtil::registerAllAgents(mpHavokWorld->getCollisionDispatcher());

   // Turn off deactivation so we can see continuous contact point processing
   mpHavokWorld->m_wantDeactivation = true;

   // Create the visual debugger server 
   #ifndef BUILD_FINAL
      if (gConfig.isDefined(cConfigStartHavokDebugger))
         startHavokDebugger();
   #endif

   //-- setup the group filter
   hkpGroupFilter *pFilter = new hkpGroupFilter();
   if (!pFilter)
      return (false);

   //pFilter->disableCollisionsBetween( cLayerTerrain, cLayerVehicles );
   pFilter->disableCollisionsBetween(cLayerPhantoms, cLayerPhantoms);
   pFilter->disableCollisionsBetween(cLayerObjects, cLayerObjects);

   //pFilter->disableCollisionsBetween(cLayerObjects, cLayerBuildings);
   pFilter->disableCollisionsBetween(cLayerVehicles, cLayerBuildings);
   pFilter->disableCollisionsBetween(cLayerVehicles, cLayerObjects);

   pFilter->disableCollisionsBetween(cLayerNoTerrainVehicles, cLayerBuildings);
   pFilter->disableCollisionsBetween(cLayerNoTerrainVehicles, cLayerObjects);
   pFilter->disableCollisionsBetween(cLayerNoTerrainVehicles, cLayerTerrain);
   pFilter->disableCollisionsBetween(cLayerNoTerrainVehicles, cLayerVehicles);

   pFilter->disableCollisionsBetween(cLayerUncollidables, cLayerObjects);
   pFilter->disableCollisionsBetween(cLayerUncollidables, cLayerVehicles);
   pFilter->disableCollisionsBetween(cLayerUncollidables, cLayerNoTerrainVehicles);
   pFilter->disableCollisionsBetween(cLayerUncollidables, cLayerBuildings);


   mpHavokWorld->setCollisionFilter((hkpCollisionFilter*)pFilter);
   pFilter->removeReference();

   // Create special collision filters
   mVehicleCollisionFilter = createCollisionFilterInfo(cLayerVehicles);
   mNoTerrainVehicleCollisionFilter = createCollisionFilterInfo(cLayerNoTerrainVehicles);
   mPhantomCollisionFilter = createCollisionFilterInfo(cLayerPhantoms);
   mBuildingCollisionFilter = createCollisionFilterInfo(cLayerBuildings);
   mUncollidableCollisionFilter = createCollisionFilterInfo(cLayerUncollidables);

   // MPB [3/24/2008] - Create special vehicle shape.  This is temporary.
   if (!mpVehicleShape)
   {
      hkReal xSize = 1.1f;
      hkReal ySize = 0.25f;
      hkReal zSize = 1.75f;

      hkReal xBumper = 1.0f;
      hkReal yBumper = 0.15f;
      hkReal zBumper = 1.9f;

      hkReal zRoofFront = 0.4f;
      hkReal zRoofBack = -1.0f;
      hkReal yRoof = ySize + 0.45f;
      hkReal xRoof = 0.7f;

      hkReal zDoorFront = zRoofFront;
      hkReal zDoorBack = zRoofBack;
      hkReal yDoor = ySize;
      hkReal xDoor = xSize + 0.1f;

      int numVertices = 22;

      // 16 = 4 (size of "each float group", 3 for x,y,z, 1 for padding) * 4 (size of float).
      int stride = sizeof(float) * 4;

      float vertices[] = { 
         xSize, ySize, zSize, 0.0f,		// v0
         -xSize, ySize, zSize, 0.0f,		// v1
         xSize, -ySize, zSize, 0.0f,		// v2
         -xSize, -ySize, zSize, 0.0f,	// v3
         xSize, -ySize, -zSize, 0.0f,	// v4
         -xSize, -ySize, -zSize, 0.0f,	// v5

         xBumper, yBumper, zBumper, 0.0f,	// v6
         -xBumper, yBumper, zBumper, 0.0f,	// v7
         xBumper, yBumper, -zBumper, 0.0f,	// v8
         -xBumper, yBumper, -zBumper, 0.0f,	// v9

         xRoof, yRoof, zRoofFront, 0.0f,		// v10
         -xRoof, yRoof, zRoofFront, 0.0f,	// v11
         xRoof, yRoof, zRoofBack, 0.0f,		// v12
         -xRoof, yRoof, zRoofBack, 0.0f,		// v13

         xDoor, yDoor, zDoorFront, 0.0f,		// v14
         -xDoor, yDoor, zDoorFront, 0.0f,	// v15
         xDoor, -yDoor, zDoorFront, 0.0f,	// v16
         -xDoor, -yDoor, zDoorFront, 0.0f,	// v17

         xDoor, yDoor, zDoorBack, 0.0f,		// v18
         -xDoor, yDoor, zDoorBack, 0.0f,		// v19
         xDoor, -yDoor, zDoorBack, 0.0f,		// v20
         -xDoor, -yDoor, zDoorBack, 0.0f,	// v21
      };
   	
      //
      // SHAPE CONSTRUCTION.
      //
   	
      hkArray<hkVector4> planeEquations;
      hkGeometry geom;
      {
         hkStridedVertices stridedVerts;
         {
	         stridedVerts.m_numVertices = numVertices;
	         stridedVerts.m_striding = stride;
	         stridedVerts.m_vertices = vertices;
         }

         hkpGeometryUtility::createConvexGeometry( stridedVerts, geom, planeEquations );

         {
	         stridedVerts.m_numVertices = geom.m_vertices.getSize();
	         stridedVerts.m_striding = sizeof(hkVector4);
	         stridedVerts.m_vertices = &(geom.m_vertices[0](0));
         }

         mpVehicleShape = new hkpConvexVerticesShape(stridedVerts, planeEquations);

         /*
         trace("<hkparam name=\"userData\" type=\"hkTypeInt32\">%d</hkparam>", chassisShape->getUserData());
         trace("<hkparam name=\"radius\" type=\"hkTypeReal\">%f</hkparam>", chassisShape->getRadius());

         // Verts
         trace("<hkparam name=\"stridedVertices\" numelements=\"%d\" stride=\"%d\">", stridedVerts.m_numVertices, stridedVerts.m_striding);
         for (int i = 0; i < stridedVerts.m_numVertices; i++)
         {
            hkVector4* pVert = (hkVector4*) const_cast<float*>(&(stridedVerts.m_vertices[i * 4]));
            trace("   (%f %f %f %f)", (*pVert)(0), (*pVert)(1), (*pVert)(2), (*pVert)(3));
         }
         trace("</hkparam>");

         // Plane eq
      	trace("<hkparam name=\"planeEquations\" numelements=\"%d\">", chassisShape->getPlaneEquations().getSize());
         const hkArray<hkVector4>& planes = chassisShape->getPlaneEquations();
         for (int i = 0; i < planes.getSize(); i++)
         {
            trace("   (%f %f %f %f)", planes[i](0), planes[i](1), planes[i](2), planes[i](3));
         }
         trace("</hkparam>");
         */
      }
   }

   markForWrite(false);
   
   return (true);
}

//==============================================================================
// BPhysicsWorld::shutdown
//==============================================================================
bool BPhysicsWorld::shutdown(void)
{
   markForWrite(true);

   if (!mInitialized)
      return false;

   mInitialized = false;
 
   // Delete visual debugger
   #ifndef BUILD_FINAL
      stopHavokDebugger();
   #endif
   
   // Delete world
   if (mpHavokWorld)
   {
      mpHavokWorld->removeReference();
      mpHavokWorld = NULL;
   }

   // Delete vehicle shape
   if (mpVehicleShape)
   {
      mpVehicleShape->removeReference();
      mpVehicleShape = NULL;
   }

   //- no longer required because the world is gone
   //markForWrite(false);

   return true;
}

//==============================================================================
//==============================================================================
bool physicsSingleThreadedCallback(uint privateData, BEventPayload* pPayload)
{
   // Init thread memory if necessary
   if (!isPhysicsThreadInited())
      XPhysicsThreadInit();

   BPhysicsWorkerCallbackData* pData = reinterpret_cast<BPhysicsWorkerCallbackData*>(pPayload);

   // Single threaded update
   pData->mpPhysicsWorld->updateSingleThreaded(pData->mNumUpdates, pData->mUpdateTime);

   // Update is complete, so release the update semaphore
   pData->mpPhysicsWorld->releaseUpdateCompletionSemaphore();

   return false;
}

//==============================================================================
//==============================================================================
bool physicsMainAsyncThreadCallback(uint privateData, BEventPayload* pPayload)
{
   // Init thread memory if necessary
   if (!isPhysicsThreadInited())
      XPhysicsThreadInit();

   BPhysicsWorkerCallbackData* pData = reinterpret_cast<BPhysicsWorkerCallbackData*>(pPayload);

   // Main thread update
   pData->mpPhysicsWorld->updateMainThreadAsync(pData->mNumUpdates, pData->mNumWorkerThreads, pData->mUpdateTime);
   
   // Update is complete, so release the update semaphore
   pData->mpPhysicsWorld->releaseUpdateCompletionSemaphore();

   return false;
}

//==============================================================================
//==============================================================================
bool physicsWorkerAsyncThreadCallback(uint privateData, BEventPayload* pPayload)
{
   // Init thread memory if necessary
   if (!isPhysicsThreadInited())
      XPhysicsThreadInit();

   BPhysicsWorkerCallbackData* pData = reinterpret_cast<BPhysicsWorkerCallbackData*>(pPayload);

   // Worker thread update
   pData->mpPhysicsWorld->updateWorkerThreadAsync(pData->mNumUpdates);
   
   return false;
}

//==============================================================================
//==============================================================================
bool BPhysicsWorld::update(float elapsed)
{
   if (!mInitialized)
      return false;

   // Calculate the number of update steps and remaining time to accumulate
   // for the next frame

   float totalTime = elapsed;
   // jce [9/9/2004] -- Sanity check.
   if (totalTime > 10.0f || _isnan(totalTime))
   {
      BFAIL("Really long or bad time sent into physics, chopping it down to prevent hang.");
      totalTime = cFixedUpdate;
   }
   uint numUpdateSteps = static_cast<uint>(totalTime / cFixedUpdate);

   float remainderTime = totalTime - (numUpdateSteps * cFixedUpdate);

   if(remainderTime > 0.0f)
   {
      numUpdateSteps += 1; 
   }

   // Don't update if nothing to do
   if(numUpdateSteps == 0)
      return false;
      
   float updateTime = totalTime / numUpdateSteps;

   // Multithreaded update
   if (mpHavokWorld->m_simulationType == hkpWorldCinfo::SIMULATION_TYPE_MULTITHREADED)
   {
      // Get number of threads to use
      long tempNumPhysicsThreads = 1;
      gConfig.get(cConfigNumPhysicsThreads, &tempNumPhysicsThreads);
      uint numPhysicsThreads = hkMath::clamp(static_cast<uint>(tempNumPhysicsThreads), static_cast<uint>(1), cMaxPhysicsThreads);

      // Get main physics thread index
      long tempMainPhysicsThreadIndex = cThreadIndexSim;
      gConfig.get(cConfigPhysicsMainThread, &tempMainPhysicsThreadIndex);
      uint mainPhysicsThreadIndex = hkMath::clamp(static_cast<uint>(tempMainPhysicsThreadIndex), static_cast<uint>(0), cMaxPhysicsThreadIndex);

      // Setup callback data
      mPhysicsWorkerCallbackData.mpPhysicsWorld = this;
      mPhysicsWorkerCallbackData.mNumUpdates = numUpdateSteps;
      mPhysicsWorkerCallbackData.mUpdateTime = updateTime;
      mPhysicsWorkerCallbackData.mNumWorkerThreads = numPhysicsThreads - 1;

      // If main thread is not sim thread, get that started first
      if (mainPhysicsThreadIndex != cThreadIndexSim)
      {
         gEventDispatcher.submitCallback(mainPhysicsThreadIndex, physicsMainAsyncThreadCallback, 0, &mPhysicsWorkerCallbackData);
         mWaitForUpdateCompletion = true; // we have to wait the main update to complete before the frame is done if this is not on the sim thread
      }

      // Kick off all non-sim worker threads
      bool simThreadWorker = false;
      uint numWorkerThreadsStarted = 0;
      uint i = 0;
      while ((numWorkerThreadsStarted < (numPhysicsThreads - 1)) && (i < cMaxPhysicsThreads))
      {
         if (cPhysicsThreadOrder[i] != mainPhysicsThreadIndex)
         {
            // For the sim thread, just set a bool to start it up at the end
            if (cPhysicsThreadOrder[i] == cThreadIndexSim)
               simThreadWorker = true;
            // Otherwise, go ahead and submit
            else
               gEventDispatcher.submitCallback(cPhysicsThreadOrder[i], physicsWorkerAsyncThreadCallback, 0, &mPhysicsWorkerCallbackData);

            numWorkerThreadsStarted++;
         }

         i++;
      }
      BASSERT(numWorkerThreadsStarted == (numPhysicsThreads - 1));  // all worker threads should be started

      // Run sim thread if needed
      if (mainPhysicsThreadIndex == cThreadIndexSim)
      {
         updateMainThreadAsync(numUpdateSteps, numWorkerThreadsStarted, updateTime);
         mWaitForUpdateCompletion = false; // don't have to wait since we're done as soon as we get here
      }
      else if (simThreadWorker)
      {
         updateWorkerThreadAsync(numUpdateSteps);
      }
   }
   // Single threaded
   else
   {
      // Get main thread index
      long mainPhysicsThreadIndex = cThreadIndexSim;
      gConfig.get(cConfigPhysicsMainThread, &mainPhysicsThreadIndex);

      // Run on sim thread
      if (mainPhysicsThreadIndex == cThreadIndexSim)
      {
         updateSingleThreaded(numUpdateSteps, updateTime);
         mWaitForUpdateCompletion = false; // don't have to wait since we're done as soon as we get here
      }
      // Kick off update on another thread
      else
      {
         mPhysicsWorkerCallbackData.mpPhysicsWorld = this;
         mPhysicsWorkerCallbackData.mNumUpdates = numUpdateSteps;
         mPhysicsWorkerCallbackData.mUpdateTime = updateTime;
         mPhysicsWorkerCallbackData.mNumWorkerThreads = 0;
         gEventDispatcher.submitCallback(mainPhysicsThreadIndex, physicsSingleThreadedCallback, 0, &mPhysicsWorkerCallbackData);
         mWaitForUpdateCompletion = true; // we have to wait for the main update to complete before the frame is done if this is not on the sim thread
      }
   }

   return true;
}

//==============================================================================
//==============================================================================
bool BPhysicsWorld::updateSingleThreaded(uint numUpdateSteps, float updateTime)
{
   for (uint i = 0; i < numUpdateSteps; i++)
   {
      mpHavokWorld->stepDeltaTime(updateTime);
   }

   // Update visual debugger
   #ifndef BUILD_FINAL
      stepHavokDebugger();
   #endif

   return (true);
}     

//==============================================================================
//==============================================================================
bool BPhysicsWorld::updateMainThreadAsync(uint numUpdateSteps, uint numWorkerThreads, float updateTime)
{
   SCOPEDSAMPLE(UpdatePhysicsMain);

   for (uint i = 0; i < numUpdateSteps; i++)
   {
      // Release step begin semaphore to workers - this is used
      // to prevent a worker thread from grabbing the step init
      // semaphore more than once per update
      if (numWorkerThreads > 0)
         mStepEndSemaphore.release(numWorkerThreads);

      // Begin
      {
         SCOPEDSAMPLE(UpdatePhysicsMainBegin);
         mpHavokWorld->stepBeginSt(updateTime);

         // Release init semaphore so worker threads can go
         if (numWorkerThreads > 0)
            mStepInitSemaphore.release(numWorkerThreads);
      }

      // Process
      {
         SCOPEDSAMPLE(UpdatePhysicsMainProcess);
         mpHavokWorld->stepProcessMt(HK_THREAD_TOKEN_FIRST);
      }


      // End
      {
         SCOPEDSAMPLE(UpdatePhysicsMainEnd);
         mpHavokWorld->stepEndSt();
      }

      // Wait for worker threads to finish
      for (uint i = 0; i < numWorkerThreads; i++)
      {
         mWorkerThreadFinishedSemaphore.wait();
      }
   }

   // Update visual debugger
   #ifndef BUILD_FINAL
      stepHavokDebugger();
   #endif

   return true;
}

//==============================================================================
//==============================================================================
bool BPhysicsWorld::updateWorkerThreadAsync(uint numUpdateSteps)
{
   SCOPEDSAMPLE(UpdatePhysicsWorker);

   for (uint i = 0; i < numUpdateSteps; i++)
   {
      // Wait for the the step to be ready (all worker threads have finished).  This
      // is so the same thread won't grab the mStepInitSemaphore multiple times
      // before all the worker threads have had a chance
      mStepEndSemaphore.wait();

      // Wait for step to be initialized
      mStepInitSemaphore.wait();

      // Process
      {
         SCOPEDSAMPLE(UpdatePhysicsWorkerProcess);
         mpHavokWorld->stepProcessMt(HK_THREAD_TOKEN_SECOND);
      }

      // Signal that this worker is done with this step
      mWorkerThreadFinishedSemaphore.release(1);
   }

   return true;
}

//==============================================================================
//==============================================================================
void BPhysicsWorld::waitForUpdateCompletion()
{
   ASSERT_THREAD(cThreadIndexSim);

   // Record the waiting time
   #ifndef BUILD_FINAL
      BTimer localTimer;
      localTimer.start();
   #endif

   // If the main processing wasn't on the main thread, we wait on the main
   // thread until the update is done (so that physics is complete before the
   // frame ends)
   if (mWaitForUpdateCompletion)
      mUpdateCompleteSemaphore.wait();

   #ifndef BUILD_FINAL
      localTimer.stop();
      mLastWaitTime = localTimer.getElapsedSeconds();
   #endif

   // Clear the wait flag so that wait is only signaled if we actually call update
   mWaitForUpdateCompletion = false;
}

//==============================================================================
// BPhysicsWorld::settle
//==============================================================================
/*
bool BPhysicsWorld::settle(void)
{
   if (!mpHavokWorld)
      return (false);

   setFlag(cFlagSettling, true);
   for (long i = 0; i < 240; i++)
   {
      mpHavokWorld->stepDeltaTime(cFixedUpdate);
   }
   setFlag(cFlagSettling, false);
   return (true);
}
*/

// Moved to physicsobject (SAT)
/*
//==============================================================================
// BPhysicsWorld::setupObjectRepresentation
//==============================================================================
bool BPhysicsWorld::setupObjectRepresentation(const BPhysicsObjectBlueprint &blueprint, const BVector &position, const BPhysicsMatrix &rotation, 
   DWORD userdata, BPhysicsObject* pObject, bool fixed, const BPhysicsObjectBlueprintOverrides *pOverrides)
{
   if (!pObject)
      return false;

   long shapeID = -1;
   // Get shape.
   if (pOverrides && pOverrides->getFlag(BPhysicsObjectBlueprintOverrides::cFlagValidShape))
   {
      shapeID = pOverrides->getShapeID();
   }
   else
   {
      shapeID = blueprint.getShape();
   }

   hkpRigidBodyCinfo info;
   
   BShape *shape = gPhysics->getShapeManager().get(shapeID);
   //--
   //-- if we don't have a valid shape, then we can try to create a simple box around the shape
   //--
   if(!shape || !shape->getHavokShape())
   {
      if (  pOverrides && 
            pOverrides->getFlag(BPhysicsObjectBlueprintOverrides::cFlagValidHalfExtents) && 
            (pOverrides->getHalfExtents().length() > cFloatCompareEpsilon))
      {
         //-- is this specified in the override
         BShape shape;
         shape.allocateBox(pOverrides->getHalfExtents());
         shape.addReference();
         info.m_shape = shape.getHavokShape();
      }
      else if (blueprint.getHalfExtents().length() > cFloatCompareEpsilon)
      {
         //-- perhaps it is specified in the blueprint itself
         BShape shape;
         shape.allocateBox(blueprint.getHalfExtents());
         shape.addReference();
         info.m_shape = shape.getHavokShape();
         
      }
      else
      {
         //-- ok, we are going to give up
         return (false);
      }
   }
   else
   {
      //-- we had a valid shape ID passed in, so this can just be used
      //-- to get our Havok shape.
      info.m_shape = shape->getHavokShape();
   }
   //--
   //--

   //-- note: valid shape beyond this point
  
   //-- get the orientation and set it up
   //-- convert to Havok space along the way
   //-- convert to quaternion and normalize it
   //CLM[12.06.07] hkRot can become QNAN here..
   hkRotation hkRot;
   BPhysics::convertRotation(rotation, hkRot);
   if(!hkRot.isOk())
   {
      hkRot.setIdentity();
   }
   info.m_rotation.set(hkRot);
   info.m_rotation.normalize();
   
   

   //-- basic motion properties
   info.m_restitution = blueprint.getRestitution();
   info.m_friction = blueprint.getFriction();
   info.m_angularDamping = blueprint.getAngularDamping();
   info.m_linearDamping = blueprint.getLinearDamping();



   if (fixed)
      info.m_motionType = hkpMotion::MOTION_FIXED;
   else
      info.m_motionType = hkpMotion::MOTION_BOX_INERTIA;

   hkReal tempMass = blueprint.getMass();
   if (tempMass == -1.0f)
      tempMass = 1.0f;

   hkpMassProperties massProperties;
   hkpInertiaTensorComputer::computeShapeVolumeMassProperties(info.m_shape, tempMass, massProperties);

   info.m_inertiaTensor = massProperties.m_inertiaTensor;
   info.m_centerOfMass = massProperties.m_centerOfMass;
   
   //-- adjust for our center of mass offset
   const BVector &massOffset = blueprint.getCenterOfMassOffset();
   info.m_centerOfMass(0) += massOffset.x;
   info.m_centerOfMass(1) += massOffset.y;
   info.m_centerOfMass(2) += massOffset.z;
   info.m_mass = massProperties.m_mass;	

   //-- create the bounding volume shape
#pragma push_macro("new")
#undef new
   hkpRigidBody *phkRigidBody = new hkpRigidBody(info);
#pragma pop_macro("new")

   //-- now we setup the physics object
   pObject->setRigidBody(phkRigidBody);
   pObject->setDeleteRigidBodyOnDestruction(true);
   pObject->setPosition(position);
   pObject->setRotation(rotation);

   //-- now we do some common code to set properties and user data
   pObject->setProperty(BPhysicsWorld::cPropertyEntityReference, userdata);
   pObject->setUserData((void*)userdata);

   //-- finally we set the collision filter layer
   if (pOverrides && pOverrides->getFlag(BPhysicsObjectBlueprintOverrides::cFlagValidCollisionFilter))
   {
      pObject->setCollisionFilterInfo(pOverrides->getCollisionFilterInfo());
   }
   else
   {
     pObject->setCollisionFilterInfo(createCollisionFilterInfo());
   }

   //-- just initialize these special ragdoll properties
   pObject->setProperty(BPhysicsWorld::cPropertyRagdollBoneIndex, DWORD(-1));
   pObject->setProperty(BPhysicsWorld::cPropertyTerrainNode, DWORD(-1));

   return (true);
}
*/

//==============================================================================
// BPhysicsWorld::createCollisionFilterInfo
//==============================================================================
long BPhysicsWorld::createCollisionFilterInfo(uint layer) 
{
   //-- get group filter information and store it
   hkpGroupFilter *pFilter = (hkpGroupFilter*)getCollisionFilter();
   if (pFilter)
   {
      hkUint32 sysgroup = pFilter->getNewSystemGroup();
      long filterinfo = hkpGroupFilter::calcFilterInfo(layer, sysgroup);
      return filterinfo;
   }

   return (-1);
}

//==============================================================================
// BPhysicsWorld::findContactSurface
//==============================================================================
long BPhysicsWorld::findContactSurface(hkpRigidBody *hkGroundRB, const hkpShapeKey &key) const
{

   return (-1);
 /*
   if (!hkGroundRB)
      return (-1);

   hkpCollidable *hkGround = hkGroundRB->getCollidable();
   if ((hkGround != NULL) && (hkGround->getShape() != NULL))
   {
      long shapeType = hkGround->getShape()->getType();
      if (shapeType == HK_SHAPE_MOPP)
      {
         hkpMoppBvTreeShape *treeShape = (hkpMoppBvTreeShape *)hkGround->getShape();
         const hkpShapeCollection *shapeCollect = treeShape->getShapeCollection();
         
         hkpShapeCollection::AllocBuffer buffer;
         const hkpShape *subShape = shapeCollect->getChildShape(key, buffer);
         if (subShape->getType() == HK_SHAPE_TRIANGLE)
         {
            //hkpTriangleShape *triangleShape = (hkpTriangleShape *)subShape;
            const BTerrainNode *pTerrainNode = NULL;
            hkpPropertyValue &terrainProperty = hkGroundRB->getProperty(BPhysicsWorld::cPropertyTerrainNode);
            long terrainNode = terrainProperty.getInt();
            pTerrainNode = getTerrainNodeConst(terrainNode);
            long subA, subB;
            subA = subB = -1;
            //subA = triangleShape->getUserIdA();
            subB = (long) key;
            subB = (subB >> 20);

            // Xemu [10/21/2003] -- Ok, now that we have the subpart, look that up in the terrain node
            if (pTerrainNode != NULL)
            {
               long surface = pTerrainNode->getSurface(subB);
               return(surface);
            }
         }
      }
   }

   return(-1);
   */
}

//==============================================================================
// BPhysicsWorld::markForWrite
//==============================================================================
void BPhysicsWorld::markForWrite(bool flag)
{
   if (!mpHavokWorld)
      return;

   if (flag)
   {
      mpHavokWorld->markForWrite();
      //trace("mark for write");
   }
   else
   {
      mpHavokWorld->unmarkForWrite();
      //trace("unmark for write");
   }
}

//==============================================================================
// BPhysicsWorld::markForRead
//==============================================================================
void BPhysicsWorld::markForRead(bool flag)
{
   if (!mpHavokWorld)
      return;

   if (flag)
      mpHavokWorld->markForRead();
   else
      mpHavokWorld->unmarkForRead();
}


//==============================================================================
// Debug functions
//==============================================================================
#ifndef BUILD_FINAL

   //==============================================================================
   //==============================================================================
   void BPhysicsWorld::startHavokDebugger()
   {
      if (!mpVisualDebugger && mpHavokWorld)
      {
         mpVDBContext = new hkpPhysicsContext;
         hkpPhysicsContext::registerAllPhysicsProcesses(); // all the physics viewers
         mpVDBContext->addWorld(mpHavokWorld); // add the physics world so the viewers can see it

         hkArray<hkProcessContext*> contexts;
         contexts.pushBack(mpVDBContext);

         mpVisualDebugger = new hkVisualDebugger(contexts);
         mpVisualDebugger->serve();

         // Allocate memory for internal profiling information
         // You can discard this if you do not want Havok profiling information
         //hkMonitorStream::getInstance().resize( 500 * 1024 );	// 500K for timer info
         //hkMonitorStream::getInstance().reset();
      }
   }

   //==============================================================================
   //==============================================================================
   void BPhysicsWorld::stepHavokDebugger(void)
   {
      // step visual debugger
      if (mpVisualDebugger)
      {
         /*
         static hkVector4 from;
         static hkVector4 to;
         static hkVector4 up;
         static BVector tempVec;
         tempVec = mCamera.getPosition();
         from = *(hkVector4*)&tempVec;
         tempVec += mCamera.getForward();
         to = *(hkVector4*)&tempVec;
         tempVec = mCamera.getUp();
         up = *(hkVector4*)&tempVec;
         HK_UPDATE_CAMERA(from, to, up, mCamera.getNearZ(), mCamera.getFarZ(), mCamera.getFOV() * 180.0f / cPi, "mofo");
         stepVisualDebugger(mHavokVDB);
         */

         mpVisualDebugger->step();

         // Reset internal profiling info for next frame
         //hkMonitorStream::getInstance().reset();
      }
   }

   //==============================================================================
   //==============================================================================
   void BPhysicsWorld::stopHavokDebugger()
   {
      if (mpVisualDebugger)
      {
         delete mpVisualDebugger;
         mpVisualDebugger = NULL;
      }

      if (mpVDBContext)
         mpVDBContext->removeReference();
   }

   //==============================================================================
   //==============================================================================
   void BPhysicsWorld::updateHavokDebuggerCamera(BVector from, BVector to, BVector up, float nearPlane, float farPlane, float fov)
   {
      #if !defined(CODE_ANALYSIS_ENABLED)
            HK_UPDATE_CAMERA(from, to, up, nearPlane, farPlane, fov, "phxCam")
      #endif
   }

#endif

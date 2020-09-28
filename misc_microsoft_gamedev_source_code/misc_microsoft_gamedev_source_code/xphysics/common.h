//==============================================================================
// common.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================



#ifndef _COMMON_H_
#define _COMMON_H_



#pragma warning( disable : 4244 )
#pragma warning( disable : 4311 )
#pragma warning( disable : 4267 )

//Must undefine new before including any havok headers. mwc
#pragma push_macro("new")
#undef new

#include "havokMemoryOverride.h"

// Dynamics includes
   #include <Physics/Collide/hkpCollide.h>

   #include <Physics/Collide/Agent/ConvexAgent/SphereBox/hkpSphereBoxAgent.h>
   #include <Physics/Collide/Shape/Compound/Collection/Mesh/hkpMeshShape.h>


   #include <Common/Base/Memory/hkThreadMemory.h>
   //#include <collide2/shape/hkShapeCinfo.h>

   //#include <collide/shape/convexvertices/hkConvexVerticesShapeCinfo.h>
   #include <Physics/Collide/Shape/HeightField/hkpSphereRepShape.h>

   #include <Physics/Dynamics/World/hkpWorld.h>
   #include <Physics/Dynamics/Entity/hkpRigidBody.h>
   //#include <utilities2/hkHavok2Common.h>
   #include <Common/Base/System/Stopwatch/hkstopwatch.h>

   #include <Common/Base/System/Error/hkerror.h>
   #include <Physics/Dynamics/Collide/hkpCollisionListener.h>
   #include <Physics/Dynamics/Entity/hkpEntityActivationListener.h>

   #include <Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h>
   #include <Physics/Utilities/Deprecated/hkpCollapseTransformsDeprecated.h>

   #include <Physics/Collide/Shape/Misc/Transform/hkpTransformShape.h>
   #include <Physics/Collide/Shape/Compound/Collection/List/hkpListShape.h>

   #include <Physics/Dynamics/Motion/hkpMotion.h>
   #include <Common/Base/Memory/Memory/Pool/hkPoolMemory.h>

   #include <Common/Base/System/Io/Reader/hkStreamReader.h>
   #include <Common/Base/System/Io/Writer/hkStreamWriter.h>

   #include <Common/Serialize/Packfile/Xml/hkXmlPackfileWriter.h>
   #include <Common/Base/System/Io/StreambufFactory/hkStreambufFactory.h>
   #include <Physics/Dynamics/World/Simulation/hkpSimulation.h>


/*
#include <Physics/Vehicle/hkpVehicle.h>
#include <Physics/Vehicle/hkpVehicleData.h>
#include <Physics/Vehicle/hkpVehicleInstance.h>
#include <Physics/Vehicle/DriverInput/Default/hkpVehicleDefaultAnalogDriverInput.h>
*/


   //-- characters
   /*#include <utilities2/charactercontrol/characterproxy/hkpCharacterProxy.h>
   #include <Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxyCinfo.h>
   #include <Physics/Utilities/CharacterControl/CharacterProxy/hkpCharacterProxyListener.h>
   #include <Physics/Utilities/CharacterControl/StateMachine/hkpDefaultCharacterStates.h>

   #include <Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h>
   #include <Physics/Dynamics/Phantom/hkpSimpleShapePhantom.h>
   #include <dynamics/phantom/hkSimpleShapePhantomCinfo.h>
   #include <Physics/Collide/Query/Collector/PointCollector/hkpClosestCdPointCollector.h>
   #include <Physics/Collide/Query/Collector/PointCollector/hkpAllCdPointCollector.h>
   #include <Physics/ConstraintSolver/Simplex/hkpSimplexSolver.h>
   #include <Physics/Collide/Query/CastUtil/hkpLinearCastInput.h>*/


   //-- Exporting/Importing
   /*#include <Preview/hkexport2/exportworld.h>
   #include <export/exportconstraint.h>
   #include <export/exportrigidbody.h>*/

   //-- Error Handling
   #pragma warning(disable: 4995)
   #include <Common/Base/System/Error/hkDefaultError.h>
   #pragma warning(default: 4995)

   //-- Mopp Tree
   #include <Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppUtility.h>

   // Output includes
   #pragma warning(disable: 4995)
   //#include <base/stream/hkStringStream.h>
   //#include <base/stream/hkIstream.h>
   //#include <base/stream/hkDefaultStreambufFactory.h>
   #pragma warning(default: 4995)
   #include <Common/Base/System/Io/IArchive/hkIArchive.h>
   #include <Physics/Utilities/Collide/ShapeUtils/MoppCodeStreamer/hkpMoppCodeStreamer.h>

   // Phantoms
   #include <Physics/Dynamics/Phantom/hkpPhantom.h>
   //#include <dynamics/phantom/hkPhantomCinfo.h>
   #include <Physics/Dynamics/Phantom/hkpAabbPhantom.h>
   //#include <dynamics/phantom/hkAabbPhantomCinfo.h>

   // Visual Debugger include
   #ifdef _DEBUG
   #include <Common/Visualize/hkVisualDebugger.h>
   #include <Common/Base/Types/Geometry/hkGeometry.h>
   #include <Common/Visualize/Shape/hkDisplayAABB.h>
   #endif

   //#include <geometryutil/hkGeometryFilterer.h>
   //#include <geometryutil/hkGeometryUtil.h>
   //-- raycast api
   #include <Physics/Internal/Collide/BroadPhase/hkpBroadPhaseCastCollector.h>
   #include <Physics/Collide/Query/CastUtil/hkpWorldRayCastInput.h>
   #include <Physics/Collide/Shape/Query/hkpShapeRayCastInput.h>
   #include <Physics/Collide/Query/CastUtil/hkpWorldRayCastOutput.h>
   #include <Physics/Internal/Collide/BroadPhase/hkpBroadPhase.h>
   #include <Physics/Collide/Query/Collector/RayCollector/hkpAllRayHitCollector.h>
   
   #include <Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h>
   #include <Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h>
   #include <Physics/Collide/Shape/Convex/Box/hkpBoxShape.h>
   #include <Physics/Collide/Shape/Convex/Cylinder/hkpCylinderShape.h>
   #include <Physics/Collide/Shape/Convex/ConvexTransform/hkpConvexTransformShape.h>
   #include <Physics/Collide/Shape/Convex/ConvexTranslate/hkpConvexTranslateShape.h>
   #include <Physics/Collide/Shape/Convex/ConvexVertices/hkpConvexVerticesShape.h>

   #include <Common/Visualize/hkDebugDisplay.h>
   #include <Physics/Dynamics/Constraint/hkpConstraintData.h>
   #include <Physics/Collide/Shape/Compound/Tree/Mopp/hkpMoppBvTreeShape.h>
   #include <Physics/Collide/Dispatch/hkpAgentRegisterUtil.h>
   #include <Physics/Collide/Filter/Group/hkpGroupFilter.h>




   //-- RagDolls
   //#include <complete\hkRagdoll.h>
   //#include <Skeleton.h>
  // #include <utilities/ragdoll/hkaLoToHiBoneMapper.h>
  // #include <utilities/ragdoll/hkHierarchy.h>

   // Serialize stuff
   #include <Common/Serialize/hkSerialize.h>
//   #include <serialize/serializer/hkXmlSerializer.h>
 //  #include <serialize/serializer/hkBinarySerializer.h>
  // #include <serialize/registry/hkDefaultPointerRegistry.h>

   // Serialize stuff - Havok2 specific
   //#include <utilities/serialize/hkHavokSession.h>
   //#include <utilities/serialize/hkRegisterHavokClasses.h>
  // #include <utilities/serialize/display/hkpSerializedDisplayMarker.h>



#pragma pop_macro("new")
#include "xcore.h"
#include "xsystem.h"
#include "color.h"
#include "physicsmatrix.h"
#include "physicsevent.h"
#include "physicsquat.h"
#include "triangle.h"
#include "terrainnode.h"



#include "xmlwriter.h"

#endif



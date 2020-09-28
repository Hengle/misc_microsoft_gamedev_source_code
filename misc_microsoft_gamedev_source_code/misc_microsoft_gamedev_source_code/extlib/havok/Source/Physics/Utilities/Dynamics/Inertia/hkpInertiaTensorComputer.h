/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_DYNAMICS2_INERTIA_TENSOR_COMPUTER_H
#define HK_DYNAMICS2_INERTIA_TENSOR_COMPUTER_H

#include <Common/Base/hkBase.h>

struct hkGeometry;
class hkpShape;
class hkpRigidBodyCinfo;
class hkpRigidBody;
class hkpConstraintInstance;

/// This structure holds the "mass and volume" properties of an object, and is filled in by each of the
/// hkpInertiaTensorComputer methods.
/// Volume is not strictly a mass property, but may be useful to help determine the mass required
/// to achieve a given density for an object.
struct hkpMassProperties
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR(HK_MEMORY_CLASS_INERTIA, hkpMassProperties);

		/// The volume of an object (calculated as a side-effect of most of the hkpInertiaTensorComputer
		/// calculations, but not used by the engine).
	hkReal	m_volume;
		/// The mass of an object.
	hkReal	m_mass;
		/// The center of mass.
	hkVector4 m_centerOfMass;
		/// The inertia tensor.
	hkMatrix3 m_inertiaTensor;

		/// Initialize (to zero data).
	hkpMassProperties() :
		m_volume(0.0f),
		m_mass(0.0f)
	{
		m_centerOfMass.setZero4();
		m_inertiaTensor.setZero();
	}

};

/// This structure is used by the hkpInertiaTensorComputer to help construct mass properties of "compound" objects.
struct hkpMassElement
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR(HK_MEMORY_CLASS_INERTIA, hkpMassElement);

		/// All mass properties for this element.
	hkpMassProperties m_properties;

		/// A transform for this element (usually with respect to body Local space)
		/// ie an ElementToLocal transform.
	hkTransform m_transform;

		/// Initialise (to zero data).
	hkpMassElement()
	{
		m_transform.setIdentity();
	}

		/// Initialise given properties and transform.
	hkpMassElement(const hkpMassProperties& properties,const hkTransform& transform) :
		m_properties(properties),
		m_transform(transform)
	{
	}

};


/// A class to compute the inertia tensor, center of mass and volume of various classes of objects.
class hkpInertiaTensorComputer
{
	public:

		///////////////////////////////////////
		// Explicit surface types calculations
		///////////////////////////////////////

			/// Creates mass properties given a sphere, considered as a volume of uniform density.
			/// Returns HK_FAILURE on failure (radius, mass invalid), otherwise returns HK_SUCCESS.
		static hkResult HK_CALL computeSphereVolumeMassProperties( hkReal radius, hkReal mass, hkpMassProperties& result);

			/// Creates mass properties given box halfextents, considered as a volume of uniform density.
			/// Returns HK_FAILURE on failure (halfExtents, mass invalid), otherwise returns HK_SUCCESS.
		static hkResult HK_CALL computeBoxVolumeMassProperties(const hkVector4& halfExtents, hkReal mass, hkpMassProperties& result);

			/// Creates mass properties given a capsule considered as a volume of uniform density.
			/// The capsule is specified by the start and end points of its axis (excluding radius), and a radius.
			/// The radius MUST be greater than zero. The height (length of axis) may be zero. 
			/// Returns HK_FAILURE on failure (radius, mass invalid), otherwise returns HK_SUCCESS.
		static hkResult HK_CALL computeCapsuleVolumeMassProperties( const hkVector4& startAxis, const hkVector4& endAxis, hkReal radius, hkReal mass, hkpMassProperties& result);

			/// Creates mass properties given a sphere hull, assumed to have a given thickness (measured "inwards") of uniform density.
			/// The thickness MUST be greater than zero and less than radius.
			/// Returns HK_FAILURE on failure (radius, surfaceThickness, mass invalid), otherwise returns HK_SUCCESS.
		static hkResult HK_CALL computeSphereSurfaceMassProperties( hkReal radius, hkReal mass, hkReal surfaceThickness, hkpMassProperties& result);

			/// Creates mass properties given box hull halfextents, assumed to have a given thickness (measured "inwards") of uniform density
			/// The thickness MUST be greater than zero and less than min of half-extents.
			/// Returns HK_FAILURE on failure (halfExtents, surfaceThickness, mass invalid), otherwise returns HK_SUCCESS.
		static hkResult HK_CALL computeBoxSurfaceMassProperties(const hkVector4& halfExtents, hkReal mass, hkReal surfaceThickness, hkpMassProperties& result);

			/// Creates mass properties given triangle vertices, assumed to have a given thickness, of uniform density
			/// The thickness MUST be greater than or EQUAL to zero. It can be zero, in which case the mass properties calculated are those
			/// of a triangular lamina. Otherwise it generates a triangular prism with center equal to the triangle center.
			/// Returns HK_FAILURE on failure (surfaceThickness, mass invalid), otherwise returns HK_SUCCESS.
		static hkResult HK_CALL computeTriangleSurfaceMassProperties(const hkVector4& v0, const hkVector4& v1, const hkVector4& v2, hkReal mass, hkReal surfaceThickness, hkpMassProperties& result);

			/// Creates mass properties given a cylinder, considered as a volume of uniform density.
			/// Returns HK_FAILURE on failure (startAxis, endAxis, radius, mass invalid), otherwise returns HK_SUCCESS.
		static hkResult HK_CALL computeCylinderVolumeMassProperties(const hkVector4& startAxis, const hkVector4& endAxis, hkReal radius, hkReal mass, hkpMassProperties& result);


		///////////////////////////////////////
		// Geometric/vertex-based calculations
		///////////////////////////////////////

			/// Creates mass properties given a point cloud, using the convex hull of the cloud, considered as a volume of uniform density.
			/// Returns HK_FAILURE on failure (vertices coplanar, mass invalid), otherwise returns HK_SUCCESS.
		static hkResult HK_CALL computeVertexHullVolumeMassProperties(const hkReal* vertexIn, int striding, int numVertices, hkReal mass, hkpMassProperties &result);

			/// Creates mass properties given a point cloud, using vertices as point masses, with uniform mass distribution.
			/// Returns HK_FAILURE on failure (vertices, mass invalid), otherwise returns HK_SUCCESS.
			/// No volume is assumed/calculated.
		static hkResult HK_CALL computeVertexCloudMassProperties(const hkReal* vertexIn, int striding, int numVertices, hkReal mass, hkpMassProperties &result);

			/// Creates mass properties for the given (possibly open/disconnected) geometry using triangles assumed to have a given thickness to provide volume.
			/// If distributeUniformly == true, the mass is distributed proportional to the area of each triangle. 
			/// If distributeUniformly == false, the same mass is given to each triangle, independent of its area. This means that "highly tessellated"
			/// areas will have more mass, usually undesirable, but perhaps useful in some instances.
			/// The thickness MUST be greater than zero.
			/// Returns HK_FAILURE on failure (geometry, surfaceThickness, mass invalid), otherwise returns HK_SUCCESS.
		static hkResult HK_CALL computeGeometrySurfaceMassProperties(const hkGeometry* geom, hkReal surfaceThickness, hkBool distributeUniformly, hkReal mass, hkpMassProperties &result);

		/// Computes the inertia tensor and the center of mass given a total mass for the provided geometry.
		/// Asserts on failure (geometry, surfaceThickness, mass invalid), otherwise returns.
		/// WARNING: This function assumes the geometry is properly closed.
		static void HK_CALL computeGeometryVolumeMassProperties(const hkGeometry* geom, hkReal mass, hkpMassProperties &result);

		/// Computes the inertia tensor and the center of mass given a total mass for the provided geometry.
		/// returns HK_FAILURE on failure (geometry, surfaceThickness, mass invalid), otherwise returns.
		/// WARNING: This function assumes the geometry is properly closed.
		static hkResult HK_CALL computeGeometryVolumeMassPropertiesChecked(const hkGeometry* geom, hkReal mass, hkpMassProperties &result);



		///////////////////////////////////////
		// Compound calculation
		///////////////////////////////////////

			/// Creates mass properties given a group of mass properties/transforms in a common space.
			/// For example, to compute mass properties for an hkpListShape-based compound body with children of different
			/// masses, use computeShapeVolumeMassProperties() to compute each child's mass element.
			/// (with m_transform left as identity), and then combine them using this method. 
			/// Returns HK_FAILURE on failure (mass invalid), otherwise returns HK_SUCCESS.
		static hkResult HK_CALL combineMassProperties(const hkArray<hkpMassElement>& elements, hkpMassProperties& result );


		///////////////////////////////////////
		// "Shape" calculation 
		///////////////////////////////////////

			/// Computes the inertia tensor and the center of mass given a total mass for a hkpShape (using "volume" methods).
			/// N.B. Every child shape of the shape passed in MUST have a volume. Triangles are automatically given a thickness of 0.01f
		static void HK_CALL computeShapeVolumeMassProperties(const hkpShape* shape, hkReal mass, hkpMassProperties &result);

			/// "Shape" calculation and assignment to a hkpRigidBodyCinfo (using "volume" methods).
			/// Useful for 1-line RigidBody mass property calculation.
			/// N.B. Every child shape of the shape passed in MUST have a volume. Triangles are automatically given a thickness of 0.01f
		static void HK_CALL setShapeVolumeMassProperties(const hkpShape* shape, hkReal mass, hkpRigidBodyCinfo& bodyInfo);

			/// Increase the inertia of all axes to be at least (max(inertia of all axes)/maxInertiaRatio)
		static void HK_CALL clipInertia(hkReal maxInertiaRatio, hkpRigidBodyCinfo& bodyInfo);

		///////////////////////////////////////
		// Inertia Tensor Simplification  
		///////////////////////////////////////

			/// Computes a "best" approximation of a full inertia tensor as an oriented particle inertia tensor, which
			/// is constant along the diagonal, and zero on the off-diagonals, ie. = I * k for some k. The matrix passed in
			/// is set to this approximation.
		static void HK_CALL simplifyInertiaTensorToOrientedParticle(hkMatrix3 &inertia);

			/// Converts a full inertia tensor to a space, where the inertia is just a diagonal
			/// matrix. principleAxisOut is a matrix converting from the new principle axis space
			/// to the old inertia tensor space.
		static void HK_CALL convertInertiaTensorToPrincipleAxis( hkMatrix3& inertia, hkRotation& principleAxisOut );

			/// Recompute the inertia tensors of a constraint tree.
			/// The idea is that bodies up in the hierarchy should not get an inertia tensor smaller than the inertia
			/// of its children.
			/// The overall inertia of the constraint tree should not change significantly, however the individual inertias
			/// can change a lot.
			/// The purpose of the function is to make motors and constraint limits of a ragdoll
			/// much stiffer, without changing the "normal" behavior of the ragdoll visibly.
			/// Set inertiaFactorHint to 1 if you want the minimal amount of inertia increase,
			/// set inertiaFactorHint to > 1 (e.g. 5) if you allow for higher inertia increase
		static void HK_CALL optimizeInertiasOfConstraintTree( hkpConstraintInstance*const* constraints, int numConstraints, hkpRigidBody* rootBody, hkReal inertiaFactorHint = 1.5f );


		///////////////////////////////////////
		// Helper functions
		///////////////////////////////////////


			/// Given an inertia tensor calculated in a space, and the center of mass in this space
			/// this function calculates the inertia around the center of mass. The physics simulation
			/// expects all inertia tensors to be expressed around the center of mass.
		static void HK_CALL shiftInertiaToCom(hkVector4& shift, hkReal mass, hkMatrix3& inertia);

			/// Given an inertia tensor calculated around the center of mass, and the center of mass itself
			/// this function calculates the inertia around a point shifted away from the
			/// center of mass. This allows you to move inertia tensors to a common space so they can be combined.
		static void HK_CALL shiftInertiaFromCom(hkVector4& shift, hkReal mass, hkMatrix3& inertia);


		//////////////////////////////////////////////////////////////////////////
		// Internal functions to separate volume calculations from inertiaTensor calculations
		//////////////////////////////////////////////////////////////////////////
		
			/// WARNING: This function assumes the geometry is properly closed.
		//static void HK_CALL internalComputeVolume(const hkGeometry* geometry, hkReal& volumeOut, hkVector4& shiftOut);

		//static void HK_CALL internalContinueComputeMassProperties(hkReal volume, const hkVector4& shift, hkReal mass, hkpMassProperties &result);
};



#endif // HK_DYNAMICS2_INERTIA_TENSOR_COMPUTER_H

/*
* Havok SDK - PUBLIC RELEASE, BUILD(#20070919)
*
* Confidential Information of Havok.  (C) Copyright 1999-2007 
* Telekinesys Research Limited t/a Havok. All Rights Reserved. The Havok
* Logo, and the Havok buzzsaw logo are trademarks of Havok.  Title, ownership
* rights, and intellectual property rights in the Havok software remain in
* Havok and/or its suppliers.
*
* Use of this software for evaluation purposes is subject to and indicates 
* acceptance of the End User licence Agreement for this product. A copy of 
* the license is included with this software and is also available from salesteam@havok.com.
*
*/

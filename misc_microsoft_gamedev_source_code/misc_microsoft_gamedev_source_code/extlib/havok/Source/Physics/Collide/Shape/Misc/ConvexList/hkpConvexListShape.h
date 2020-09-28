/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_COLLIDE2_CONVEX_LIST_SHAPE_H
#define HK_COLLIDE2_CONVEX_LIST_SHAPE_H

#include <Physics/Collide/Shape/hkpShapeContainer.h>
#include <Physics/Collide/Shape/Convex/hkpConvexShape.h>

extern const hkClass hkpConvexListShapeClass;

/// A simple static list of hkShapes. You can use this shape class to create compound bodies.
/// A list shape can hold a mix of different shape types, however the radius of all shapes must be 
/// identical (see below) which may limit its use in some cases. 
/// The convex list shape is very good for smaller objects, which are made of a few convex pieces.
/// You will get a very good CPU for this convex list, as long as the hkpConvexListShape is 
/// only colliding with its convex hull. A typical example would be a car made of several parts.
/// If your object looks more like a spaceship, where other bodies may frequently be 'inside' the hull
/// you should consider using hkpListShape wrapped in a hkMoppShape.
/// N.B. In either case, there is still a potential for a performance impact hit with complex compound bodies, so
/// it is advisable to contact Havok support to discuss optimum representation.
/// Note: This implementation has some limitations:
///     - the number of child objects is restricted to 255
///     - the number of vertices of each child object is restricted to 255 (see user guide)
///		- per-childshape collision filtering is not possible (for either collisions or raycasts)
///     - The radius of all child objects must be equal.  For this reason you cannot usually combine 
///		hkSphereShapes, hkCapsuleShapes and hkConvexVerticesShapes in hkConvexListShapes.
/// Please see the user guide for more details on the hkpConvexListShape
class hkpConvexListShape : public hkpConvexShape, public hkpShapeContainer
{
	public:

		HK_DECLARE_REFLECTION();

			/// Constructs a list shape with an array of pointers to shapes.
		hkpConvexListShape( const hkpConvexShape*const* shapeArray, int numShapes );

			/// The destructor removes references to child shapes.
		~hkpConvexListShape();
	
			/// Set whether you want to use a cached version of the aabb for getAabb().
			/// The default is true.
			/// If this is set to true a cached aabb of the children's aabbs will be used,
			/// otherwise getAabb will query all children and combine their aabbs.
			/// If true, the aabb returned is bigger, but is much faster to evaluate.
		void setUseCachedAabb( bool useCachedAabb );

			/// Get whether you want to use a cached version of the aabb for getAabb().
			/// The default is true.
			/// If this is set to true a cached aabb of the children's aabbs will be used,
			/// otherwise getAabb will query all children and combine their aabbs.
			/// If true, the aabb returned is bigger, but is much faster to evaluate.
		bool getUseCachedAabb();


			/// Returns the ith child shape.
		inline const hkpShape* getChildShapeInl(int i) const { return m_childShapes[i]; }

		//
		// hkpConvexShape interface
		//

			// Implemented method of hkpConvexShape
		void getSupportingVertexImpl( hkVector4Parameter dir, hkpCdVertex& supportingVertexOut ) const;

			// Implemented method of hkpConvexShape
		void convertVertexIdsToVerticesImpl( const hkpVertexId* ids, int numIds, hkpCdVertex* verticesOut) const;

			// Implemented method of hkpConvexShape
		virtual void getFirstVertex(hkVector4& v) const;

		//
		// hkpSphereRepShape interface
		//

			// Implemented method of hkpSphereRepShape
		int getNumCollisionSpheresImpl( )  const;

			// Implemented method of hkpSphereRepShape
		const hkSphere* getCollisionSpheresImpl( hkSphere* sphereBuffer ) const;



		//
		// hkpShapeContainer interface
		//

			// hkpShapeContainer interface implementation.
		virtual int getNumChildShapes() const;

			/// Get the first child shape key.
		virtual hkpShapeKey getFirstKey() const;

			/// Get the next child shape key.
		virtual hkpShapeKey getNextKey( hkpShapeKey oldKey ) const;

			/// hkpShapeCollection interface implementation. Always returns 0, and warns, because you cannot filter hkConvexListShapes.
		virtual hkUint32 getCollisionFilterInfo( hkpShapeKey key ) const;

			/// Note that a hkpListShape does not use the char* buffer for its returned shape.
		HK_SPU_VIRTUAL_DECLSPEC(const hkpShape*)	HK_GET_CHILD_SHAPE_FUNCTION;
			
		
		//
		// hkpShape interface
		//


			// Implemented method of hkpShape
		virtual void castRayWithCollector( const hkpShapeRayCastInput& input, const hkpCdBody& cdBody, hkpRayHitCollector& collector ) const;

			// Implemented method of hkpShape
		virtual hkBool castRayImpl( const hkpShapeRayCastInput& input, hkpShapeRayCastOutput& results ) const;

			// Implemented method of hkpShape
		void getAabbImpl( const hkTransform& localToWorld, hkReal tolerance, hkAabb& out ) const;

			// Implemented method of hkpShape
#if !defined(HK_PLATFORM_SPU)
		virtual const hkpShapeContainer* getContainer() const;
#endif
			// Implemented method of hkReferencedObject
		virtual void calcStatistics( hkStatisticsCollector* collector) const;



	protected:

		void setShapesAndRadius( const hkpConvexShape*const* shapeArray, int numShapes );

	public:

		hkpConvexListShape( class hkFinishLoadedObjectFlag flag ) : hkpConvexShape(flag), m_childShapes(flag) { m_type = HK_SHAPE_CONVEX_LIST; }

	public:


			/// A distance which is used for the getClosestPoint() call. If the distance between
			/// your query object and the convex hull is bigger than
			/// the value of this member, the function only returns this distance.
			/// Otherwise it recursively checks its children
		hkReal	m_minDistanceToUseConvexHullForGetClosestPoints;

	protected:

		hkVector4	m_aabbHalfExtents;
		hkVector4	m_aabbCenter;
		bool m_useCachedAabb; //+default(false)

	public:

		hkArray<const hkpConvexShape*> m_childShapes;
		

};


#endif // HK_COLLIDE2_CONVEX_LIST_SHAPE_H

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

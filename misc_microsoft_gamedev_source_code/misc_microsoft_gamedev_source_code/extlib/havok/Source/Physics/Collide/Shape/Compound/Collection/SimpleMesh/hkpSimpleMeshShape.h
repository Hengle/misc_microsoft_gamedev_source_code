/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_SIMPLE_MESH_SHAPE_H
#define HK_SIMPLE_MESH_SHAPE_H

#include <Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h>
#include <Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h>

extern const hkClass hkpSimpleMeshShapeClass;

class hkpMoppBvTreeShape;

/// This shape is a very simple container for triangle soups and can't handle triangle strips.
/// It does not allow sharing of triangle data with the renderer through referencing.
/// Use hkpMeshShape or your own implementation of a hkpShapeCollection to share triangle data with the renderer.
class hkpSimpleMeshShape : public hkpShapeCollection
{
	public:

		HK_DECLARE_REFLECTION();

			/// Default constructor.
			/// The data for this shape is public, so simply fill in the
			/// member data after construction.
		hkpSimpleMeshShape( hkReal radius = hkConvexShapeDefaultRadius );


			/// Compute welding info. In order to weld collisions between triangles in this mesh, welding info must be created.
			/// You must call this after all subparts have been added to the mesh.
			/// The hkpMoppBvTreeShape you pass in must be built referencing this hkpMeshShape.
			/// This adds an additional 2 bytes per triangle storage overhead.
			/// This is an expensive call, and should be done off line, and the resultant hkpMeshShape
			/// serialized, to save the runtime overhead of computing the welding info.
		void computeWeldingInfo( const hkpMoppBvTreeShape* mopp, hkpWeldingUtility::WeldingType weldingType );

		//
		// hkpShapeCollection interface
		//

			/// Get the first child shape key.
		virtual hkpShapeKey getFirstKey() const;

			/// Get the next child shape key.
		virtual hkpShapeKey getNextKey( hkpShapeKey oldKey ) const;

			// hkpShapeCollection interface implementation.
		const hkpShape* getChildShape( hkpShapeKey key, ShapeBuffer& buffer ) const;


			/// Gets the extra radius for every triangle.
		inline hkReal getRadius() const;

			/// Sets the extra radius for every triangle.
		inline void setRadius(hkReal r );


		//
		// hkpShape interface
		//


			// hkpShape interface implementation.
 		virtual void getAabbImpl( const hkTransform& localToWorld, hkReal tolerance, hkAabb& out ) const;


		virtual void calcStatistics( hkStatisticsCollector* collector) const;

	public:

		struct Triangle
		{
			HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_COLLIDE, hkpSimpleMeshShape::Triangle );
			HK_DECLARE_REFLECTION();

			int m_a;
			int m_b;
			int m_c;

			hkUint16 m_weldingInfo; // +default(0)
		};

			/// Array of vertices that the triangles can index into.
		hkArray<hkVector4> m_vertices;

			/// Array of triangles.  The triangles are triples of ints that are indices into the m_vertices array.
		hkArray<struct Triangle> m_triangles;

			/// Material indices. If you are not using material information, leave this array as 0 size.
		hkArray<hkUint8> m_materialIndices;

			/// The radius of the storage mesh shape. It is initialized to .05
		hkReal m_radius;

			/// A a welding type per triangle
		hkEnum<hkpWeldingUtility::WeldingType, hkUint8> m_weldingType; // +default(hkpWeldingUtility::WELDING_TYPE_NONE)



	public:

		hkpSimpleMeshShape( hkFinishLoadedObjectFlag flag ) : hkpShapeCollection(flag), m_vertices(flag), m_triangles(flag), m_materialIndices(flag) { m_type = HK_SHAPE_TRIANGLE_COLLECTION; }

};


#include <Physics/Collide/Shape/Compound/Collection/SimpleMesh/hkpSimpleMeshShape.inl>

#endif //HK_SIMPLE_MESH_SHAPE_H

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

/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_COLLIDE2_SHAPE_TYPES_H
#define HK_COLLIDE2_SHAPE_TYPES_H

/// All shape types. The dispatcher has only to implement at least the types that can be used as secondary types
enum hkpShapeType
{
	//
	// first we have register our types which go onto the spu
	//
	HK_SHAPE_INVALID = 0,
		
		// 
		//	The abstract base shapes<br>
		//

		/// All shapes which inherit from hkpConvexShape have this as an alternate type.
	HK_SHAPE_CONVEX,

		/// The first real shape.
	HK_FIRST_SHAPE_TYPE = HK_SHAPE_CONVEX,


		/// All shapes which inherit from hkpShapeCollection have this as an alternate type.
	HK_SHAPE_COLLECTION,

		//
		//	Special convex shapes, which get their private agents for better performance.
		//
		/// hkpSphereShape type.
	HK_SHAPE_SPHERE,

		/// hkpCylinderShape type.
	HK_SHAPE_CYLINDER, 
		/// hkpTriangleShape type.
	HK_SHAPE_TRIANGLE,
		/// hkpBoxShape type.
	HK_SHAPE_BOX,
		/// hkpCapsuleShape type.
	HK_SHAPE_CAPSULE,
		/// hkpConvexVerticesShape type.
	HK_SHAPE_CONVEX_VERTICES,
		/// hkpPackedConvexVerticesShape type.
	HK_SHAPE_PACKED_CONVEX_VERTICES,

		/// hkpListShape type.
	HK_SHAPE_LIST,

		/// hkpMoppBvTreeShape type.
	HK_SHAPE_MOPP,

		/// A shape collection which has the child shape data embedded into the mopp
	HK_SHAPE_MOPP_EMBEDDED,

		/// hkpConvexTranslateShape, allows to translate other convex shapes (especially boxes).
	HK_SHAPE_CONVEX_TRANSLATE,

		/// hkpConvexTransformShape, allows to transform other convex shapes (especially boxes), usually faster
		/// than hkpTransformShape.
	HK_SHAPE_CONVEX_TRANSFORM,

	HK_SHAPE_EXTENDED_MESH,

	//
	//	Shapes not on the spu
	//
	HK_SHAPE_MAX_ID_SPU = HK_SHAPE_EXTENDED_MESH+1, 

	/// hkpConvexPieceShape type.
	HK_SHAPE_CONVEX_PIECE,

		//
		//	Special HK_SHAPE_COLLECTION implementations
		//
		/// hkpMultiSphereShape type.
	HK_SHAPE_MULTI_SPHERE,

		/// hkpConvexListShape, a List of convex pieces which are treated as a single convex object if possible.
	HK_SHAPE_CONVEX_LIST,

		/// A shape collection which only returns triangles as child shapes, e.g. hkpMeshShape.
	HK_SHAPE_TRIANGLE_COLLECTION,

		/// All shapes which inherit from hkpBvTreeShape have this as an alternate type.
	HK_SHAPE_BV_TREE,

		// 
		// Special shapes
		// 
		/// hkpMultiRayShape type.
	HK_SHAPE_MULTI_RAY,
		/// hkpHeightFieldShape type.
	HK_SHAPE_HEIGHT_FIELD,
		/// hkpSampledHeightFieldShape type.
	HK_SHAPE_SAMPLED_HEIGHT_FIELD,
		/// hkpSphereRepShape type.
	HK_SHAPE_SPHERE_REP, 
		/// hkpBvShape type.
	HK_SHAPE_BV,
		/// hkpPlaneShape type.
	HK_SHAPE_PLANE,

		//
		//	Single shapes which are processed by unary agents.
		//
		
		/// hkpTransformShape type.
	HK_SHAPE_TRANSFORM,
		/// hkpPhantomCallbackShape type.
	HK_SHAPE_PHANTOM_CALLBACK,


		//
		//	user shapes
		//

	HK_SHAPE_USER0,
	HK_SHAPE_USER1,
	HK_SHAPE_USER2,

		///	The end of the shape type list.
	HK_SHAPE_MAX_ID,

		/// All shape flag, used by the hkpCollisionDispatcher.
	HK_SHAPE_ALL = -1

};

	/// A utility function to return a useful name for a given shape type
const char* HK_CALL hkGetShapeTypeName( hkpShapeType type );


class hkpCollisionDispatcher;

	/// Register all havok specific shapeTypes
	/// This needs to be called at setup time to tell the dispatcher which
	/// shapes inherit from other shapes
void HK_CALL hkRegisterAlternateShapeTypes( hkpCollisionDispatcher* dis );


#endif // HK_COLLIDE2_SHAPE_TYPES_H

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

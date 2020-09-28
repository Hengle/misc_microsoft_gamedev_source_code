/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_UTILS_SHAPE_SHRINKER__H
#define HK_UTILS_SHAPE_SHRINKER__H

class hkpShape;

	/// This utility class shrinks convex objects that use convex radius (box, cyl, convexvertices)
	/// by the convex radius (the padding radius). Triangles and collections of triangles that 
    /// have radius are not altered.  It will return a new shape if one was created. 
	/// It will return NULL if no new shape was required as the shape was able to be altered in 
	/// place or did not have a radius. This function is recursive. Note that this is not meant to be 
	/// done at runtime, but in the tool chain and preprocess stages.
class hkpShapeShrinker
{
	public:

			/// Struct used by hkpShapeShrinker::shrinkByConvexRadius().
		struct ShapeCache
		{
			HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_UTILITIES, hkpShapeShrinker::ShapeCache );

				/// Old shape.
			hkpShape* originalShape; 
				/// New shape. Null if not new.
			hkpShape* newShape;
		};

			/// This will try to shrink the given shape. The returned shape is not NULL only when 
			/// a new shape is created, since this isn't always the case.  For instance, if the shape 
			/// is just a box, it will just have its extents changed and the shape pointer remains 
			/// the same. On the other hand, a convex vertices shape needs to be recreated to be 
			/// altered, so it will change, as will lists of shapes that have changed etc.
			/// If you are doing multiple shrink calls and you have shared shapes that are 
			/// shrinkable, you might want to provide an array to persist between calls
			/// to stop the shapes from being shrunk more than once. If you don't provide a
			/// cache then one will be used per call internally, so multiple calls will
			/// result in possible over-shrinking on shared shapes.
		static hkpShape* HK_CALL shrinkByConvexRadius( hkpShape* s, hkArray<ShapeCache>* doneShapes);
};


#endif // HK_UTILS_SHAPE_SHRINKER__H

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

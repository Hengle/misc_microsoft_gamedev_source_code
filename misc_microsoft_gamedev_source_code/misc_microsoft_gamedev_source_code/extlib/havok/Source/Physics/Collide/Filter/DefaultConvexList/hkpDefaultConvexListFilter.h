/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_COLLIDE2_DEFAULT_CONVEX_LIST_FILTER_H
#define HK_COLLIDE2_DEFAULT_CONVEX_LIST_FILTER_H

#include <Physics/Collide/Filter/hkpConvexListFilter.h>

	/// This is the default filter that is used for a convex list shape.  You should change this if you wish to 
	/// improve performance of convex list shapes by treating them as convex for some types of collisions.
	/// For example you could treat a convex list shape as a convex object for collisions with a smooth landscape, or
	/// for collisions with fast moving debris.
class hkpDefaultConvexListFilter : public hkpConvexListFilter
{
	//+vtable(1)
	public:
		HK_DECLARE_REFLECTION();

		hkpDefaultConvexListFilter(){}

		hkpDefaultConvexListFilter( hkFinishLoadedObjectFlag ){}

			// If the convex list shape is colliding with a landscape, we dispatch the convex list shape as a list, to ensure
			// correct welding happens. Otherwise we dispatch convex list shape as normal.
		ConvexListCollisionType getConvexListCollisionType( const hkpCdBody& convexListBody, const hkpCdBody& otherBody, const hkpCollisionInput& input ) const;
};

#endif // HK_COLLIDE2_DEFAULT_CONVEX_LIST_FILTER_H

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

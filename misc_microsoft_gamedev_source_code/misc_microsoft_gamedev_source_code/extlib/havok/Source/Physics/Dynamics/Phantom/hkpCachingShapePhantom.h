/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_DYNAMICS2_CACHING_SHAPE_PHANTOM_H
#define HK_DYNAMICS2_CACHING_SHAPE_PHANTOM_H

#include <Physics/Dynamics/Phantom/hkpShapePhantom.h>

class hkCollisionEnvironment;
class hkpCollisionDispatcher;
class hkpCollidable;
class hkpCollisionAgent;

class hkCachingShapePhantomCinfo;
struct hkpLinearCastCollisionInput;
struct hkpCollisionInput;
class hkpCdPointCollector;
class hkpCdBodyPairCollector;

extern const hkClass hkpCachingShapePhantomClass;

/// This class represents a phantom with an arbitrary shape for query purposes.
/// Please read hkpShapePhantom and the hkpGskBaseAgent documentation first.<br>
/// This implementation of hkpShapePhantom creates collision agents for all potential colliding pairs.
/// It therefore uses more memory than the uncached version (hkpSimpleShapePhantom), but can perform
/// collision detection functions up to 40% faster when lots of GJK/GSK agents are involved.<br>
/// Note: In landscapes, caching the linear cast means converting the cast into an extended aabb
/// (the original aabb of the shape plus the path).
/// Therefore long linear casts can result in a huge cached aabb and many many potential triangles as
/// collision partners. The hkpSimpleShapePhantom can actually use an optimized direct linear cast for mopp
/// objects.
class hkpCachingShapePhantom : public hkpShapePhantom 
{
	public:

		HK_DECLARE_REFLECTION();

			/// Constructor takes a shape, a transform, and an optional collision filter info
		hkpCachingShapePhantom( const hkpShape* shape, const hkTransform& transform, hkUint32 m_collisionFilterInfo = 0 );

		~hkpCachingShapePhantom();

			/// Gets the hkpPhantom type. For this class the type is HK_PHANTOM_CACHING_SHAPE
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_RO] [this,HK_ACCESS_RO] );
		virtual hkpPhantomType getType() const;

			/// Implementation of hkpShapePhantom::setPositionAndLinearCast
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_RW] [this,HK_ACCESS_RW] );
		virtual void setPositionAndLinearCast( const hkVector4& position, const hkpLinearCastInput& input, hkpCdPointCollector& castCollector, hkpCdPointCollector* startCollector );
			
			/// hkpPhantom clone functionality
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_RO] [this,HK_ACCESS_RO] );
		virtual hkpPhantom* clone() const;

			/// Implementation of hkpShapePhantom::getClosestPoints
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_RO] [this,HK_ACCESS_RW] );
		void getClosestPoints( hkpCdPointCollector& collector, const hkpCollisionInput* input = HK_NULL );

			/// Implementation of hkpShapePhantom::getPenetrations
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_RO] [this,HK_ACCESS_RW] );
		void getPenetrations( hkpCdBodyPairCollector& collector, const hkpCollisionInput* input = HK_NULL );

			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [this,HK_ACCESS_RO] );
		void calcStatistics( hkStatisticsCollector* collector ) const;

	public:

		struct hkpCollisionDetail
		{
			HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_DYNAMICS, hkpCachingShapePhantom::hkpCollisionDetail );

			hkpCollisionAgent* m_agent;
			hkpCollidable*     m_collidable;
		};

		inline hkArray<struct hkpCollisionDetail>& getCollisionDetails();

		//
		// hkpPhantom interface
		//

			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_RO] [this,HK_ACCESS_RW] );
		virtual void addOverlappingCollidable( hkpCollidable* collidable );

			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_RO] [this,HK_ACCESS_RW] );
		virtual void removeOverlappingCollidable( hkpCollidable* collidable );

			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [this,HK_ACCESS_RO] );
		virtual hkBool isOverlappingCollidableAdded( hkpCollidable* collidable );

			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_RO] [this,HK_ACCESS_RW] );
		virtual void updateShapeCollectionFilter();

	protected:

		hkArray<struct hkpCollisionDetail> m_collisionDetails; //+nosave

	public:
		hkpCachingShapePhantom( class hkFinishLoadedObjectFlag flag ) : hkpShapePhantom( flag ) {}

		//
		// INTERNAL USE ONLY
		//

			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_RO] [this,HK_ACCESS_RW] );
		virtual void deallocateInternalArrays();
};

#include <Physics/Dynamics/Phantom/hkpCachingShapePhantom.inl>


#endif //HK_DYNAMICS2_CACHING_SHAPE_PHANTOM_H


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

/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_COLLIDE2_PROCESS_COLLISION_DATA_H
#define HK_COLLIDE2_PROCESS_COLLISION_DATA_H

#include <Physics/Collide/Agent/hkpProcessCdPoint.h>
#include <Common/Base/Types/Physics/ContactPoint/hkContactPointMaterial.h>
#include <Physics/Internal/Collide/Gjk/hkpGskCache.h>
#include <Physics/ConstraintSolver/Constraint/Contact/hkpContactPointProperties.h>


	/// An array of collision contact points. This class is used internally by hkdynamics to pass collision detection
	/// information from the collision detector to the constraint solver.
#if defined(HK_PLATFORM_HAS_SPU)
	enum {HK_MAX_CONTACT_POINT = 64 };
#else
	enum {HK_MAX_CONTACT_POINT = 256 };
#endif

struct hkpAgentEntry;

#ifndef hkCollisionConstraintOwner
class hkpConstraintOwner;
#	define hkCollisionConstraintOwner hkpConstraintOwner
#endif

	/// This is the output data of the collision detector (plus the contact point added/removed events).
	/// This class holds all contact points as well as continuous information (TOI = Time Of Impact)
	/// As the hkContactPointInfo or hkpContactPointProperties are part of hkDynamics they can't be part of this structure.
struct hkpProcessCollisionData
{
	public:
		HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR(HK_MEMORY_CLASS_AGENT, hkpProcessCollisionData);

			/// Get the number of contact points
		inline int getNumContactPoints() const;

			/// Get the contact point at index i
		inline hkpProcessCdPoint& getContactPoint( int i );

			/// Get a pointer to the first contact point of the contact point array
		inline hkpProcessCdPoint* getFirstContactPoint();

			/// Get a pointer to the end of the used contact point array
		inline hkpProcessCdPoint* getEnd();

			/// returns true if there are no contact points
		inline hkBool isEmpty() const;

			/// returns true if a toi was created between this pair of objects
		inline hkBool hasToi() const;

			/// get access to the toi contact point
		inline hkContactPoint& getToiContactPoint();

			/// returns the toi
		inline hkTime getToi() const;

			/// returns access to the toi material
		inline hkpContactPointProperties& getToiProperties();

	public:
			/// A pointer to the first point in the m_contactPoints which is not used yet
		hkPadSpu<hkpProcessCdPoint*> m_firstFreeContactPoint;

		hkPadSpu<hkCollisionConstraintOwner*> m_constraintOwner;

			/// A buffer for all contact points, used up to m_firstFreeContactPoint
		hkpProcessCdPoint   m_contactPoints[HK_MAX_CONTACT_POINT];

		//
		//	toi info
		//
		struct ToiInfo
		{
			public:
				HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_COLLIDE, hkpProcessCollisionData::ToiInfo );

				inline ToiInfo();

					// exchanges A-B
				inline void flip();

			public:
				/// An optional toi contact point, valid if m_toi < HK_REAL_MAX
				hkContactPoint     m_contactPoint;

				/// The toi
				hkPadSpu<hkReal>   m_time;

				/// The separating velocity of the objects at the toi
				hkPadSpu<hkReal>      m_seperatingVelocity;

				hkGskCache16			m_gskCache;

				/// The material which is going to be used by the toi handler
				hkContactPointPropertiesWithExtendedUserData16 m_properties;
		};
		struct ToiInfo m_toi;

		inline hkpProcessCollisionData( hkCollisionConstraintOwner* owner );
};

#include <Physics/Collide/Agent/hkpProcessCollisionData.inl>

#endif // HK_COLLIDE2_PROCESS_COLLISION_DATA_H

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

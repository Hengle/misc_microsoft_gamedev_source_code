/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

// WARNING: THIS FILE IS GENERATED. EDITS WILL BE LOST.
// Generated from 'Physics/Utilities/Collide/Filter/Pairwise/hkpPairwiseCollisionFilter.h'
#include <Common/Base/hkBase.h>
#include <Common/Base/Reflection/hkClass.h>
#include <Common/Base/Reflection/hkInternalClassMember.h>
#include <Common/Base/Reflection/hkTypeInfo.h>
#include <Physics/Utilities/Collide/Filter/Pairwise/hkpPairwiseCollisionFilter.h>



// External pointer and enum types
extern const hkClass hkpEntityClass;
extern const hkClass hkpPairwiseCollisionFilterCollisionPairClass;

//
// Class hkpPairwiseCollisionFilter::CollisionPair
//
HK_REFLECTION_DEFINE_SCOPED_SIMPLE(hkpPairwiseCollisionFilter,CollisionPair);
static const hkInternalClassMember hkpPairwiseCollisionFilter_CollisionPairClass_Members[] =
{
	{ "a", &hkpEntityClass, HK_NULL, hkClassMember::TYPE_POINTER, hkClassMember::TYPE_STRUCT, 0, 0, HK_OFFSET_OF(hkpPairwiseCollisionFilter::CollisionPair,m_a), HK_NULL },
	{ "b", &hkpEntityClass, HK_NULL, hkClassMember::TYPE_POINTER, hkClassMember::TYPE_STRUCT, 0, 0, HK_OFFSET_OF(hkpPairwiseCollisionFilter::CollisionPair,m_b), HK_NULL }
};
const hkClass hkpPairwiseCollisionFilterCollisionPairClass(
	"hkpPairwiseCollisionFilterCollisionPair",
	HK_NULL, // parent
	sizeof(hkpPairwiseCollisionFilter::CollisionPair),
	HK_NULL,
	0, // interfaces
	HK_NULL,
	0, // enums
	reinterpret_cast<const hkClassMember*>(hkpPairwiseCollisionFilter_CollisionPairClass_Members),
	HK_COUNT_OF(hkpPairwiseCollisionFilter_CollisionPairClass_Members),
	HK_NULL, // defaults
	HK_NULL, // attributes
	0
	);

//
// Class hkpPairwiseCollisionFilter
//
HK_REFLECTION_DEFINE_VIRTUAL(hkpPairwiseCollisionFilter);
static const hkInternalClassMember hkpPairwiseCollisionFilterClass_Members[] =
{
	{ "disabledPairs", &hkpPairwiseCollisionFilterCollisionPairClass, HK_NULL, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_STRUCT, 0, 0, HK_OFFSET_OF(hkpPairwiseCollisionFilter,m_disabledPairs), HK_NULL }
};
extern const hkClass hkpCollisionFilterClass;

extern const hkClass hkpPairwiseCollisionFilterClass;
const hkClass hkpPairwiseCollisionFilterClass(
	"hkpPairwiseCollisionFilter",
	&hkpCollisionFilterClass, // parent
	sizeof(hkpPairwiseCollisionFilter),
	HK_NULL,
	1, // interfaces
	HK_NULL,
	0, // enums
	reinterpret_cast<const hkClassMember*>(hkpPairwiseCollisionFilterClass_Members),
	HK_COUNT_OF(hkpPairwiseCollisionFilterClass_Members),
	HK_NULL, // defaults
	HK_NULL, // attributes
	0
	);

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

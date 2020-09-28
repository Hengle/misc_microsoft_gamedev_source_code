/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

// WARNING: THIS FILE IS GENERATED. EDITS WILL BE LOST.
// Generated from 'Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h'
#include <Physics/Collide/hkpCollide.h>
#include <Common/Base/Reflection/hkClass.h>
#include <Common/Base/Reflection/hkInternalClassMember.h>
#include <Common/Base/Reflection/hkTypeInfo.h>
#include <Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h>



//
// Enum hkpCapsuleShape::RayHitType
//
static const hkInternalClassEnumItem hkpCapsuleShapeRayHitTypeEnumItems[] =
{
	{0, "HIT_CAP0"},
	{1, "HIT_CAP1"},
	{2, "HIT_BODY"},
};
static const hkInternalClassEnum hkpCapsuleShapeEnums[] = {
	{"RayHitType", hkpCapsuleShapeRayHitTypeEnumItems, 3, HK_NULL, 0 }
};
extern const hkClassEnum* hkpCapsuleShapeRayHitTypeEnum = reinterpret_cast<const hkClassEnum*>(&hkpCapsuleShapeEnums[0]);

//
// Class hkpCapsuleShape
//
HK_REFLECTION_DEFINE_VIRTUAL(hkpCapsuleShape);
const hkInternalClassMember hkpCapsuleShape::Members[] =
{
	{ "vertexA", HK_NULL, HK_NULL, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, HK_OFFSET_OF(hkpCapsuleShape,m_vertexA), HK_NULL },
	{ "vertexB", HK_NULL, HK_NULL, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, HK_OFFSET_OF(hkpCapsuleShape,m_vertexB), HK_NULL }
};
extern const hkClass hkpConvexShapeClass;

extern const hkClass hkpCapsuleShapeClass;
const hkClass hkpCapsuleShapeClass(
	"hkpCapsuleShape",
	&hkpConvexShapeClass, // parent
	sizeof(hkpCapsuleShape),
	HK_NULL,
	0, // interfaces
	reinterpret_cast<const hkClassEnum*>(hkpCapsuleShapeEnums),
	1, // enums
	reinterpret_cast<const hkClassMember*>(hkpCapsuleShape::Members),
	HK_COUNT_OF(hkpCapsuleShape::Members),
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

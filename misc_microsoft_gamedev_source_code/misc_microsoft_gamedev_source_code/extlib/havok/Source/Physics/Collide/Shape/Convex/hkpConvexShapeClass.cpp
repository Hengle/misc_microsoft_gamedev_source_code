/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

// WARNING: THIS FILE IS GENERATED. EDITS WILL BE LOST.
// Generated from 'Physics/Collide/Shape/Convex/hkpConvexShape.h'
#include <Physics/Collide/hkpCollide.h>
#include <Common/Base/Reflection/hkClass.h>
#include <Common/Base/Reflection/hkInternalClassMember.h>
#include <Common/Base/Reflection/hkTypeInfo.h>
#include <Physics/Collide/Shape/Convex/hkpConvexShape.h>



//
// Enum hkpConvexShape::WeldResult
//
static const hkInternalClassEnumItem hkpConvexShapeWeldResultEnumItems[] =
{
	{0, "WELD_RESULT_REJECT_CONTACT_POINT"},
	{1, "WELD_RESULT_ACCEPT_CONTACT_POINT_MODIFIED"},
	{2, "WELD_RESULT_ACCEPT_CONTACT_POINT_UNMODIFIED"},
};
static const hkInternalClassEnum hkpConvexShapeEnums[] = {
	{"WeldResult", hkpConvexShapeWeldResultEnumItems, 3, HK_NULL, 0 }
};
extern const hkClassEnum* hkpConvexShapeWeldResultEnum = reinterpret_cast<const hkClassEnum*>(&hkpConvexShapeEnums[0]);

//
// Class hkpConvexShape
//

const hkInternalClassMember hkpConvexShape::Members[] =
{
	{ "radius", HK_NULL, HK_NULL, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, HK_OFFSET_OF(hkpConvexShape,m_radius), HK_NULL }
};
extern const hkClass hkpSphereRepShapeClass;

extern const hkClass hkpConvexShapeClass;
const hkClass hkpConvexShapeClass(
	"hkpConvexShape",
	&hkpSphereRepShapeClass, // parent
	sizeof(hkpConvexShape),
	HK_NULL,
	0, // interfaces
	reinterpret_cast<const hkClassEnum*>(hkpConvexShapeEnums),
	1, // enums
	reinterpret_cast<const hkClassMember*>(hkpConvexShape::Members),
	HK_COUNT_OF(hkpConvexShape::Members),
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

/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

// WARNING: THIS FILE IS GENERATED. EDITS WILL BE LOST.
// Generated from 'Physics/Collide/Shape/HeightField/TriSampledHeightField/hkpTriSampledHeightFieldCollection.h'
#include <Physics/Collide/hkpCollide.h>
#include <Common/Base/Reflection/hkClass.h>
#include <Common/Base/Reflection/hkInternalClassMember.h>
#include <Common/Base/Reflection/hkTypeInfo.h>
#include <Physics/Collide/Shape/HeightField/TriSampledHeightField/hkpTriSampledHeightFieldCollection.h>



// External pointer and enum types
extern const hkClass hkpSampledHeightFieldShapeClass;

//
// Class hkpTriSampledHeightFieldCollection
//
HK_REFLECTION_DEFINE_VIRTUAL(hkpTriSampledHeightFieldCollection);
const hkInternalClassMember hkpTriSampledHeightFieldCollection::Members[] =
{
	{ "heightfield", &hkpSampledHeightFieldShapeClass, HK_NULL, hkClassMember::TYPE_POINTER, hkClassMember::TYPE_STRUCT, 0, 0, HK_OFFSET_OF(hkpTriSampledHeightFieldCollection,m_heightfield), HK_NULL },
	{ "radius", HK_NULL, HK_NULL, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, HK_OFFSET_OF(hkpTriSampledHeightFieldCollection,m_radius), HK_NULL }
};
extern const hkClass hkpShapeCollectionClass;

extern const hkClass hkpTriSampledHeightFieldCollectionClass;
const hkClass hkpTriSampledHeightFieldCollectionClass(
	"hkpTriSampledHeightFieldCollection",
	&hkpShapeCollectionClass, // parent
	sizeof(hkpTriSampledHeightFieldCollection),
	HK_NULL,
	0, // interfaces
	HK_NULL,
	0, // enums
	reinterpret_cast<const hkClassMember*>(hkpTriSampledHeightFieldCollection::Members),
	HK_COUNT_OF(hkpTriSampledHeightFieldCollection::Members),
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

/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

// WARNING: THIS FILE IS GENERATED. EDITS WILL BE LOST.
// Generated from 'Physics/Internal/Collide/ConvexPieceMesh/hkpConvexPieceStreamData.h'
#include <Physics/Internal/hkpInternal.h>
#include <Common/Base/Reflection/hkClass.h>
#include <Common/Base/Reflection/hkInternalClassMember.h>
#include <Common/Base/Reflection/hkTypeInfo.h>
#include <Physics/Internal/Collide/ConvexPieceMesh/hkpConvexPieceStreamData.h>



// External pointer and enum types
extern const hkClass hkpShapeKeyClass;

//
// Class hkpConvexPieceStreamData
//
HK_REFLECTION_DEFINE_VIRTUAL(hkpConvexPieceStreamData);
static const hkInternalClassMember hkpConvexPieceStreamDataClass_Members[] =
{
	{ "convexPieceStream", HK_NULL, HK_NULL, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT32, 0, 0, HK_OFFSET_OF(hkpConvexPieceStreamData,m_convexPieceStream), HK_NULL },
	{ "convexPieceOffsets", HK_NULL, HK_NULL, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT32, 0, 0, HK_OFFSET_OF(hkpConvexPieceStreamData,m_convexPieceOffsets), HK_NULL },
	{ "convexPieceSingleTriangles", HK_NULL, HK_NULL, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT32, 0, 0, HK_OFFSET_OF(hkpConvexPieceStreamData,m_convexPieceSingleTriangles), HK_NULL }
};
extern const hkClass hkReferencedObjectClass;

extern const hkClass hkpConvexPieceStreamDataClass;
const hkClass hkpConvexPieceStreamDataClass(
	"hkpConvexPieceStreamData",
	&hkReferencedObjectClass, // parent
	sizeof(hkpConvexPieceStreamData),
	HK_NULL,
	0, // interfaces
	HK_NULL,
	0, // enums
	reinterpret_cast<const hkClassMember*>(hkpConvexPieceStreamDataClass_Members),
	HK_COUNT_OF(hkpConvexPieceStreamDataClass_Members),
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

/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

// WARNING: THIS FILE IS GENERATED. EDITS WILL BE LOST.
// Generated from 'Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h'
#include <Physics/Internal/hkpInternal.h>
#include <Common/Base/Reflection/hkClass.h>
#include <Common/Base/Reflection/hkInternalClassMember.h>
#include <Common/Base/Reflection/hkTypeInfo.h>
#include <Physics/Internal/Collide/Mopp/Code/hkpMoppCode.h>



// External pointer and enum types
extern const hkClass hkpMoppCodeCodeInfoClass;

//
// Class hkpMoppCodeReindexedTerminal
//
HK_REFLECTION_DEFINE_SIMPLE(hkpMoppCodeReindexedTerminal);
static const hkInternalClassMember hkpMoppCodeReindexedTerminalClass_Members[] =
{
	{ "origShapeKey", HK_NULL, HK_NULL, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, HK_OFFSET_OF(hkpMoppCodeReindexedTerminal,m_origShapeKey), HK_NULL },
	{ "reindexedShapeKey", HK_NULL, HK_NULL, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, HK_OFFSET_OF(hkpMoppCodeReindexedTerminal,m_reindexedShapeKey), HK_NULL }
};
extern const hkClass hkpMoppCodeReindexedTerminalClass;
const hkClass hkpMoppCodeReindexedTerminalClass(
	"hkpMoppCodeReindexedTerminal",
	HK_NULL, // parent
	sizeof(hkpMoppCodeReindexedTerminal),
	HK_NULL,
	0, // interfaces
	HK_NULL,
	0, // enums
	reinterpret_cast<const hkClassMember*>(hkpMoppCodeReindexedTerminalClass_Members),
	HK_COUNT_OF(hkpMoppCodeReindexedTerminalClass_Members),
	HK_NULL, // defaults
	HK_NULL, // attributes
	0
	);

//
// Class hkpMoppCode::CodeInfo
//
HK_REFLECTION_DEFINE_SCOPED_SIMPLE(hkpMoppCode,CodeInfo);
static const hkInternalClassMember hkpMoppCode_CodeInfoClass_Members[] =
{
	{ "offset", HK_NULL, HK_NULL, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, HK_OFFSET_OF(hkpMoppCode::CodeInfo,m_offset), HK_NULL }
};
const hkClass hkpMoppCodeCodeInfoClass(
	"hkpMoppCodeCodeInfo",
	HK_NULL, // parent
	sizeof(hkpMoppCode::CodeInfo),
	HK_NULL,
	0, // interfaces
	HK_NULL,
	0, // enums
	reinterpret_cast<const hkClassMember*>(hkpMoppCode_CodeInfoClass_Members),
	HK_COUNT_OF(hkpMoppCode_CodeInfoClass_Members),
	HK_NULL, // defaults
	HK_NULL, // attributes
	0
	);

//
// Class hkpMoppCode
//
HK_REFLECTION_DEFINE_VIRTUAL(hkpMoppCode);
static const hkInternalClassMember hkpMoppCodeClass_Members[] =
{
	{ "info", &hkpMoppCodeCodeInfoClass, HK_NULL, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0, 0, HK_OFFSET_OF(hkpMoppCode,m_info), HK_NULL },
	{ "data", HK_NULL, HK_NULL, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT8, 0, 0, HK_OFFSET_OF(hkpMoppCode,m_data), HK_NULL }
};
extern const hkClass hkReferencedObjectClass;

extern const hkClass hkpMoppCodeClass;
const hkClass hkpMoppCodeClass(
	"hkpMoppCode",
	&hkReferencedObjectClass, // parent
	sizeof(hkpMoppCode),
	HK_NULL,
	0, // interfaces
	HK_NULL,
	0, // enums
	reinterpret_cast<const hkClassMember*>(hkpMoppCodeClass_Members),
	HK_COUNT_OF(hkpMoppCodeClass_Members),
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

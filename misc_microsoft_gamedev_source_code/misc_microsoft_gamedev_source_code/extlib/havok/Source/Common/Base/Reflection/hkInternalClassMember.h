/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */
#ifndef HK_BASE_INTERNALCLASSMEMBER_H
#define HK_BASE_INTERNALCLASSMEMBER_H

class hkClassEnum;
class hkClass;

struct hkInternalCustomAttributes
{
	struct Attribute
	{
		const char* m_name;
		const void* m_data;
		const hkClass* m_klass;

		HK_DECLARE_SYSTEM_ALLOCATOR();
	};

	const Attribute* m_attributes;
	int m_numAttributes;

	HK_DECLARE_SYSTEM_ALLOCATOR();
};

// This is a struct with the same data layout as an hkMember object.
// Compilers can efficiently place hkInternalMembers directly into the data
// section, whereas the use of hkMember objects force huge number of
// calls to hkMember::hkMember not to mention code bloat from the global 
// constructor chain.
struct hkInternalClassMember
{
	// None of these classes have havok allocators since they are
	// only used to define static const data and should never be new-ed

	const char* m_name;
	const hkClass* m_class;
	const hkClassEnum* m_enum;
	hkUint8 m_type;
	hkUint8 m_subtype;
	hkUint16 m_cArraySize;
	hkUint16 m_flags;
	hkUint16 m_offset;
	const hkInternalCustomAttributes* m_attributes;

	HK_DECLARE_SYSTEM_ALLOCATOR();
};

struct hkInternalClassEnumItem
{
	int m_value;
	const char* m_name;

	HK_DECLARE_SYSTEM_ALLOCATOR();
};

struct hkInternalClassEnum
{
	const char* m_name;
	const hkInternalClassEnumItem* m_items;
	int m_numItems;
	const hkInternalCustomAttributes* m_attributes;
	hkUint32 m_flags;

	HK_DECLARE_SYSTEM_ALLOCATOR();
};

#endif // HK_BASE_INTERNALCLASSMEMBER_H

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

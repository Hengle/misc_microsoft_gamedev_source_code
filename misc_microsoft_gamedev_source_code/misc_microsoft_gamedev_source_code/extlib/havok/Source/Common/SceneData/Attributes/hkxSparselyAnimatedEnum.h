/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_SCENEDATA_HKX_ANIMATED_ENUM_H
#define HK_SCENEDATA_HKX_ANIMATED_ENUM_H

#include <Common/SceneData/Attributes/hkxSparselyAnimatedInt.h>

extern const hkClass hkxSparselyAnimatedEnumClass;

	/// An hkxSparselyAnimatedEnum stores the values associated with an hkxAttribute of type enum. 
	/// This is the same as an hkxSparselyAnimatedInt with extra information about the conversion from int->string (enum), using
	/// an hkClassEnum object for that.
struct hkxSparselyAnimatedEnum : public hkxSparselyAnimatedInt
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_SCENE_DATA, hkxSparselyAnimatedEnum );
	HK_DECLARE_REFLECTION();

		/// The enum type of the int animated array. hkClassEnums are
		/// easy to create at runtime so can in theory be any set of 
		/// string-value pairs.
	const class hkClassEnum* m_type;
};


#endif //HK_SCENEDATA_HKX_ANIMATED_ENUM_H

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

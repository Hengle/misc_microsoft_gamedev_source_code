/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */
#ifndef HKSCENEDATA_MATERIAL_HKXMATERIALEFFECT_HKCLASS_H
#define HKSCENEDATA_MATERIAL_HKXMATERIALEFFECT_HKCLASS_H

extern const class hkClass hkxMaterialEffectClass;

class hkxMaterialEffect
{
	public:

		HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_SCENE_DATA, hkxMaterialEffect );
		HK_DECLARE_REFLECTION();
	
			/// Used by textures to hint at their usage.
		enum EffectType
		{
			EFFECT_TYPE_INVALID,
			EFFECT_TYPE_UNKNOWN,
			EFFECT_TYPE_HLSL_FX, // As a source text file
			EFFECT_TYPE_CG_FX, // As a source text file
			EFFECT_TYPE_MAX_ID
		};
				
		//
		// Members
		//
	public:
		
			/// The name of the material effect 
		char* m_name;

			/// What type of effect is this?
		hkEnum<EffectType, hkUint8> m_type;

			/// The data for the effect (source or pre compiled)
		hkUint8* m_data;
		hkInt32 m_numData;
};

#endif // HKSCENEDATA_MATERIAL_HKXMATERIALEFFECT_HKCLASS_H

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

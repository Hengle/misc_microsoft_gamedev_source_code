/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */
#ifndef HK_DYNAMICS2_MATERIAL_H
#define HK_DYNAMICS2_MATERIAL_H

extern const hkClass hkpMaterialClass;

	/// hkMaterials allow you to add extra information to collision detection results, such as friction and restitution values.
class hkpMaterial
{
	public:
		HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_DYNAMICS, hkpMaterial );
		HK_DECLARE_REFLECTION();

			/// Constructor initialises friction to .5 and restitution to .4.
		inline hkpMaterial();

		//
		// Friction and Restitution
		//

			/// Returns the friction coefficient (dynamic and static).
		inline hkReal getFriction() const;

			/// Returns the default restitution.
			//  restitution = bounciness (1 should give object all its energy back, 0 means it just sits there, etc.).
		inline hkReal getRestitution() const;
			
			/// Sets the friction coefficient. Note: Setting this will not update existing contact information.
		inline void setFriction( hkReal newFriction );

			/// Sets the restitution coefficient. Note: Setting this will not update existing contact information.
		inline void setRestitution( hkReal newRestitution );

			/// This returns the default way to combine two friction values.
			/// We take the geometric mean ( sqrt (frictionA * frictionB) )
		static inline hkReal HK_CALL getCombinedFriction( hkReal frictionA, hkReal frictionB);

			/// This returns the default way to combine two restitution values.
			/// We take the geometric mean ( sqrt (restitutionA * restitutionB) )
		static inline hkReal HK_CALL getCombinedRestitution( hkReal restitutionA, hkReal restitutionB);


		//
		// Response type
		//

			/// A list of some response types as initially set up by the hkpWorld constructor,
			/// which can be overridden. The default behavior is that a higher ResponseType
			/// overrides a lower ResponseType. For instance, RESPONSE_NONE will have a higher
			/// priority than RESPONSE_SIMPLE_CONTACT. 	
		enum ResponseType
		{
				/// 
			RESPONSE_INVALID,
				/// Do normal collision resolution.
			RESPONSE_SIMPLE_CONTACT,
				/// Just do some reporting. No collision resolution is performed, but the collision
				/// listener functions are called.
			RESPONSE_REPORTING,
				/// Do nothing, ignore all the results.
			RESPONSE_NONE,
				/// 
			RESPONSE_MAX_ID
		};


			/// Get the response type. See the description of hkRepsonseType for details.
		inline void setResponseType( enum hkpMaterial::ResponseType t );

			/// Set the response type. See the description of hkRepsonseType for details.
		inline enum hkpMaterial::ResponseType getResponseType() const;



	private:

		hkEnum<ResponseType,hkInt8> m_responseType;
		hkReal m_friction;
		hkReal m_restitution;

	public:

		hkpMaterial( class hkFinishLoadedObjectFlag flag ) { }
};

#include <Physics/Dynamics/Common/hkpMaterial.inl>


#endif // HK_DYNAMICS2_MATERIAL_H

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

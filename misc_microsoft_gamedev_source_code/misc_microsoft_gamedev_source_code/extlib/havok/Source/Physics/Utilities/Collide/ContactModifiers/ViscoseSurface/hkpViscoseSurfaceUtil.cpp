/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#include <Physics/Utilities/Collide/ContactModifiers/ViscoseSurface/hkpViscoseSurfaceUtil.h>
#include <Physics/Dynamics/Entity/hkpRigidBody.h>
#include <Physics/Dynamics/Collide/hkpResponseModifier.h>

//
// Please do not change this file
//

hkpViscoseSurfaceUtil::hkpViscoseSurfaceUtil( hkpRigidBody* entity )
{
	HK_ASSERT2(0x6740aa92,  entity->getWorld() == HK_NULL, "You can only create a hkpViscoseSurfaceUtil BEFORE you add an entity to the world");

	m_entity = entity;

	HK_ASSERT2(0x42166b07, entity->getMaterial().getResponseType() == hkpMaterial::RESPONSE_SIMPLE_CONTACT, "The response type of the entity must be hkpMaterial::RESPONSE_SIMPLE_CONTACT" );

	entity->setProcessContactCallbackDelay(0);
	entity->addCollisionListener( this );
	entity->addEntityListener( this );


	entity->getCollidableRw()->m_forceCollideOntoPpu |= hkpCollidable::FORCE_PPU_MODIFIER_REQUEST;
}



			// The hkpCollisionListener interface implementation
void hkpViscoseSurfaceUtil::contactPointAddedCallback(	  hkpContactPointAddedEvent& event )
{
	hkpConstraintInstance* instance = event.m_internalContactMgr->getConstraintInstance();
	if ( !instance )
	{
		return;
	}

	hkpResponseModifier::setLowSurfaceViscosity( event.m_internalContactMgr, *event.m_collisionOutput->m_constraintOwner.val() );
}

void hkpViscoseSurfaceUtil::entityDeletedCallback( hkpEntity* entity )
{		
	entity->removeCollisionListener( this );
	entity->removeEntityListener( this );
	this->removeReference();
}


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

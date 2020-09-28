/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_DYNAMICS2_CONSTRAINT_LISTENER_H
#define HK_DYNAMICS2_CONSTRAINT_LISTENER_H

class hkpConstraintInstance;

/// Any class that is interested in events from a constraint inherits from this class
class hkpConstraintListener
{
	public:

		virtual ~hkpConstraintListener() {}

			///Called when a constraint is added to the world.
		virtual void constraintAddedCallback( hkpConstraintInstance* constraint ) {}

			///Called when a constraint is removed from the world.
		virtual void constraintRemovedCallback( hkpConstraintInstance* constraint ) {}
};


#endif // HK_DYNAMICS2_CONSTRAINT_LISTENER_H

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

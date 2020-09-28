/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_CONSTRAINTDRAWER_H
#define HK_CONSTRAINTDRAWER_H

#include <Physics/Utilities/VisualDebugger/Viewer/Dynamics/Constraint/Drawer/hkpPrimitiveDrawer.h>

/// Base class for constraint drawers
class hkpConstraintInstance;

class hkpConstraintDrawer 
{
	public:
		HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_VDB, hkpConstraintDrawer );

		hkpConstraintDrawer();

			/// Sets the size of lines drawn.
		void setLineLength(hkReal lineLength);

			/// Sets the radius of arcs drawn.
		void setArcRadius(hkReal arcRadius);

			/// Allows the user to up the number of segments used when displaying curved geometry.
		void setNumSegments(int numSegments);

			/// Set the size of the pivots drawn.
		void setPivotSize(hkReal size);


	protected:

		void drawPivots(int tag);
		
		void drawBodyFrames(int tag);

		void updateCommonParameters(hkpConstraintInstance* constraint,hkTransform& refLocalToWorld,hkTransform& attLocalToWorld);

		hkReal m_lineLength;
		hkReal m_arcRadius;
		hkReal m_pivotSize;
		hkReal m_arrowSize;
		hkReal m_coneSize;
		hkReal m_planeSize;
		int	   m_numSegments;

		// world positions
		hkVector4 m_bodyAWPos;
		hkVector4 m_bodyBWPos;

		// world positions of pivots
		hkVector4 m_bodyAWPivot;
		hkVector4 m_bodyBWPivot;

		hkRotation m_RA;
		hkRotation m_RB;

		hkpPrimitiveDrawer m_primitiveDrawer;
};

#endif 

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

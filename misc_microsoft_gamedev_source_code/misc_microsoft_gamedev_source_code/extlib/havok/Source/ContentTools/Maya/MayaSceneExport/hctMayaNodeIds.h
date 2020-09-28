/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_MAYA_NODE_IDS__H
#define HK_MAYA_NODE_IDS__H

// This file contains the name and ID for all Havok nodes in Maya,
// from a set of 255 assigned by Alias. The name and ID of new nodes
// should not conflict with those which have been used in any
// previously released plugin.

// ----------------------------------------------------------------
// LEGACY PLUGIN NODES (named 'hkXxx')
// ----------------------------------------------------------------

// Physics
#define hkWorldID								0x001052C0 + 0x00
#define hkRigidBodyCollectionID					0x001052C0 + 0x10
#define hkRigidBodyEntityID						0x001052C0 + 0x11
#define hkRigidBodyPrimitiveID					0x001052C0 + 0x12
#define hkSpringID								0x001052C0 + 0x20
#define hkDisabledPairID						0x001052C0 + 0x30
#define hkConstraintSolverID					0x001052C0 + 0x40
#define hkPointToPointConstraintID				0x001052C0 + 0x41
#define hkStiffSpringConstraintID				0x001052C0 + 0x43
#define hkHingeConstraintID						0x001052C0 + 0x45
#define hkRagdollConstraintID					0x001052C0 + 0x47
#define hkPrismaticConstraintID					0x001052C0 + 0x49
#define hkPolyCapsuleID							0x001052C0 + 0x50

// Animation
#define hkFilterSetOptionsID					0x001052C0 + 0x61

// ----------------------------------------------------------------
// 3.2+ PLUGIN NODES (named 'hkNodeXxx')
// ----------------------------------------------------------------

#define hkNodeOptionsID							0x001052C0 + 0xa0
#define hkNodeCapsuleID							0x001052C0 + 0xa2
#define hkNodeDecompositionID					0x001052C0 + 0xa4

#define hkNodeRigidBodyID						0x001052C0 + 0xb2
#define hkNodeRigidBodyManipID					0x001052C0 + 0xb3
#define hkNodeShapeID							0x001052C0 + 0xb4
#define hkNodeShapeManipID						0x001052C0 + 0xb5
#define hkNodeFxRigidBodyID                     0x001052C0 + 0xb6
#define hkNodeFxSphereID                        0x001052C0 + 0xb7
#define hkNodeFxInstanceID						0x001052C0 + 0xb8
#define hkNodeFxClothID							0x001052C0 + 0xb9

#define hkNodeBallAndSocketConstraintID			0x001052C0 + 0xc0
#define hkNodeBallAndSocketConstraintManipID	0x001052C0 + 0xc1
#define hkNodeHingeConstraintID					0x001052C0 + 0xc2
#define hkNodeHingeConstraintManipID			0x001052C0 + 0xc3
#define hkNodeRagDollConstraintID				0x001052C0 + 0xc4
#define hkNodeRagDollConstraintManipID			0x001052C0 + 0xc5
#define hkNodeStiffSpringConstraintID			0x001052C0 + 0xc6
#define hkNodeStiffSpringConstraintManipID		0x001052C0 + 0xc7
#define hkNodePrismaticConstraintID				0x001052C0 + 0xc8
#define hkNodePrismaticConstraintManipID		0x001052C0 + 0xc9
#define hkNodeWheelConstraintID 				0x001052C0 + 0xc10
#define hkNodeWheelConstraintManipID    		0x001052C0 + 0xc11

#endif // !HK_MAYA_NODE_IDS__H

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

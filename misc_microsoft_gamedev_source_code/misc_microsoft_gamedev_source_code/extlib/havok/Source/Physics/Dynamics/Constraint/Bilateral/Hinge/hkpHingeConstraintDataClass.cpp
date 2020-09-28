/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

// WARNING: THIS FILE IS GENERATED. EDITS WILL BE LOST.
// Generated from 'Physics/Dynamics/Constraint/Bilateral/Hinge/hkpHingeConstraintData.h'
#include <Physics/Dynamics/hkpDynamics.h>
#include <Common/Base/Reflection/hkClass.h>
#include <Common/Base/Reflection/hkInternalClassMember.h>
#include <Common/Base/Reflection/hkTypeInfo.h>
#include <Physics/Dynamics/Constraint/Bilateral/Hinge/hkpHingeConstraintData.h>



// External pointer and enum types
extern const hkClass hkp2dAngConstraintAtomClass;
extern const hkClass hkpBallSocketConstraintAtomClass;
extern const hkClass hkpHingeConstraintDataAtomsClass;
extern const hkClass hkpSetLocalTransformsConstraintAtomClass;
extern const hkClass hkpSolverResultsClass;

//
// Enum hkpHingeConstraintData::Atoms::Axis
//
static const hkInternalClassEnumItem hkpHingeConstraintDataAtomsAxisEnumItems[] =
{
	{0, "AXIS_AXLE"},
};
static const hkInternalClassEnum hkpHingeConstraintDataAtomsEnums[] = {
	{"Axis", hkpHingeConstraintDataAtomsAxisEnumItems, 1, HK_NULL, 0 }
};
extern const hkClassEnum* hkpHingeConstraintDataAtomsAxisEnum = reinterpret_cast<const hkClassEnum*>(&hkpHingeConstraintDataAtomsEnums[0]);

//
// Class hkpHingeConstraintData::Atoms
//
HK_REFLECTION_DEFINE_SCOPED_NONVIRTUAL(hkpHingeConstraintData,Atoms);
static const hkInternalClassMember hkpHingeConstraintData_AtomsClass_Members[] =
{
	{ "transforms", &hkpSetLocalTransformsConstraintAtomClass, HK_NULL, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0, 0, HK_OFFSET_OF(hkpHingeConstraintData::Atoms,m_transforms), HK_NULL },
	{ "2dAng", &hkp2dAngConstraintAtomClass, HK_NULL, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0, 0, HK_OFFSET_OF(hkpHingeConstraintData::Atoms,m_2dAng), HK_NULL },
	{ "ballSocket", &hkpBallSocketConstraintAtomClass, HK_NULL, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0, 0, HK_OFFSET_OF(hkpHingeConstraintData::Atoms,m_ballSocket), HK_NULL }
};
const hkClass hkpHingeConstraintDataAtomsClass(
	"hkpHingeConstraintDataAtoms",
	HK_NULL, // parent
	sizeof(hkpHingeConstraintData::Atoms),
	HK_NULL,
	0, // interfaces
	reinterpret_cast<const hkClassEnum*>(hkpHingeConstraintDataAtomsEnums),
	1, // enums
	reinterpret_cast<const hkClassMember*>(hkpHingeConstraintData_AtomsClass_Members),
	HK_COUNT_OF(hkpHingeConstraintData_AtomsClass_Members),
	HK_NULL, // defaults
	HK_NULL, // attributes
	0
	);

//
// Class hkpHingeConstraintData
//
HK_REFLECTION_DEFINE_VIRTUAL(hkpHingeConstraintData);
static const hkInternalClassMember hkpHingeConstraintDataClass_Members[] =
{
	{ "atoms", &hkpHingeConstraintDataAtomsClass, HK_NULL, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0, 0|hkClassMember::ALIGN_16, HK_OFFSET_OF(hkpHingeConstraintData,m_atoms), HK_NULL }
};
extern const hkClass hkpConstraintDataClass;

extern const hkClass hkpHingeConstraintDataClass;
const hkClass hkpHingeConstraintDataClass(
	"hkpHingeConstraintData",
	&hkpConstraintDataClass, // parent
	sizeof(hkpHingeConstraintData),
	HK_NULL,
	0, // interfaces
	HK_NULL,
	0, // enums
	reinterpret_cast<const hkClassMember*>(hkpHingeConstraintDataClass_Members),
	HK_COUNT_OF(hkpHingeConstraintDataClass_Members),
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

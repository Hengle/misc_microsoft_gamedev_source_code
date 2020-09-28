/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_DYNAMICS2_CONSTRAINT_ATOM_H
#define HK_DYNAMICS2_CONSTRAINT_ATOM_H


#include <Common/Base/hkBase.h>
#include <Physics/ConstraintSolver/Constraint/Contact/hkpSimpleContactConstraintInfo.h>
#include <Common/Base/Types/Physics/ContactPoint/hkContactPoint.h>
#include <Physics/Dynamics/Constraint/hkpConstraintData.h>

HK_REFLECTION_CLASSFILE_DESTINATION("../../../Dynamics/Constraint/Atom");

class hkContactPoint;
class hkpConstraintMotor;
class hkpContactPointPropertiesStream;

#define HK_CONSTRAINT_FILL_PADDING_WITH_ZERO(fromAfter, to) { for (hkUint8* ptr = reinterpret_cast<hkUint8*>(fromAfter.next()); ptr < reinterpret_cast<hkUint8*>(&to); *(ptr++) = 0) { } }


  /// Constraint atoms are building blocks that specify hkConstraintDatas.
  ///
  /// hkConstraintDatas either use hkpBridgeAtoms, which allow them to implement their custom logic, or use a set of generic hkConstraintAtoms
  /// to describe the geometry of the constraint.
  ///
  /// Each hkpConstraintAtom is used to specify a constraint's orientation in space or to create one or more solver-constraints of a given type.
  /// During simulation, hkConstraintAtoms are processed in the order in which they're organized in a hkpConstraintData.
  ///
  /// Generally the first constraint in a list is one that specifies the local bases of the constraint in each of the constrained bodies' spaces.
  /// Those bases are persistent throughout processing of a list of atoms. The following atoms apply a kind of a constraint (linear, angular,
  /// limit, motor, etc.) in relation to one or more of the axes of the specified local bases. See individual descriptions of atoms for more info.
  ///
  ///
struct hkpConstraintAtom
{
		HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpConstraintAtom );
		HK_DECLARE_REFLECTION();

	public:
		enum AtomType
		{
			TYPE_INVALID = 0,

			TYPE_BRIDGE,

			TYPE_SET_LOCAL_TRANSFORMS,
			TYPE_SET_LOCAL_TRANSLATIONS,
			TYPE_SET_LOCAL_ROTATIONS,

			TYPE_BALL_SOCKET,
			TYPE_STIFF_SPRING,

			TYPE_LIN,
			TYPE_LIN_SOFT,
			TYPE_LIN_LIMIT,
			TYPE_LIN_FRICTION,
			TYPE_LIN_MOTOR,

			TYPE_2D_ANG,

			TYPE_ANG,
			TYPE_ANG_LIMIT,
			TYPE_TWIST_LIMIT,
			TYPE_CONE_LIMIT,
			TYPE_ANG_FRICTION,
			TYPE_ANG_MOTOR,

			TYPE_RAGDOLL_MOTOR,

			TYPE_PULLEY,
			TYPE_OVERWRITE_PIVOT,

			TYPE_CONTACT,

				//
				// modifiers, must be the end of the list
				//
			TYPE_MODIFIER_SOFT_CONTACT,		// not supported by the spu
			TYPE_MODIFIER_MASS_CHANGER,		// spu supported
			TYPE_MODIFIER_VISCOUS_SURFACE,	// not supported by the spu
			TYPE_MODIFIER_MOVING_SURFACE,	// spu supported

			TYPE_MAX
		};

		// flag indicating whether this constraint needs some special callback treatment
		// those flags can be combined
		enum CallbackRequest
		{
			CALLBACK_REQUEST_NONE = 0,
			CALLBACK_REQUEST_NEW_CONTACT_POINT = 1,
			CALLBACK_REQUEST_SETUP_PPU_ONLY = 2,
			CALLBACK_REQUEST_SETUP_CALLBACK = 4
		};

	public:
		HK_FORCE_INLINE enum AtomType getType() const { return m_type; }

		HK_FORCE_INLINE int isModifierType() const { return m_type >= TYPE_MODIFIER_SOFT_CONTACT; }

	protected:
		hkpConstraintAtom(enum AtomType type) : m_type(type) {}

	private:
			// Illegal constructor
		hkpConstraintAtom();

	public:
		hkEnum<AtomType,hkUint16> m_type;

		hkpConstraintAtom(hkFinishLoadedObjectFlag f) {}
};


typedef void (HK_CALL *hkConstraintAtomBuildJacobianFunc) ( class hkpConstraintData* m_constraintData, const hkpConstraintQueryIn &in, hkpConstraintQueryOut &out );

	/// This atom is used to allow to call the old hkpConstraintData classes
struct hkpBridgeConstraintAtom: public hkpConstraintAtom
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpBridgeConstraintAtom );
	HK_DECLARE_REFLECTION();

	hkpBridgeConstraintAtom(  ): hkpConstraintAtom( TYPE_BRIDGE ){ }

		// call this to do stuff not done in the constructor yet
	void init (class hkpConstraintData* m_constraintData);

	HK_FORCE_INLINE hkpConstraintAtom* next()		{ return (this+1); }

		// bridge atoms are always the last atom, so no need to increment solver result, just make sure the
		// program crashes if the result of this function is used
	HK_FORCE_INLINE int numSolverResults() const    { return 100000; }

	// addToConstraintInfo not needed

	hkConstraintAtomBuildJacobianFunc       m_buildJacobianFunc;	//+nosave +overridetype(void*)

	class hkpConstraintData* m_constraintData;

	hkpBridgeConstraintAtom(hkFinishLoadedObjectFlag f) : hkpConstraintAtom(f) { init(m_constraintData); }
};

struct hkpBridgeAtoms
{
	HK_DECLARE_REFLECTION();
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpBridgeAtoms );

	struct hkpBridgeConstraintAtom m_bridgeAtom;

	hkpBridgeAtoms(){}

	// get a pointer to the first atom
	const hkpConstraintAtom* getAtoms() const { return &m_bridgeAtom; }

	// get the size of all atoms (we can't use sizeof(*this) because of align16 padding)
	int getSizeOfAllAtoms() const               { return hkGetByteOffsetInt(this, &m_bridgeAtom+1); }

	hkpBridgeAtoms(hkFinishLoadedObjectFlag f) : m_bridgeAtom(f) {}
};


	/// hkpSimpleContactConstraintAtom holds contact information for a single hkpSimpleContactConstraintData.
	///
	/// It is for internal use only and is unique in the following ways:
	///  - it is not a member of the owning hkpConstraintData, it is allocated externally
	///  - its size is dynamic and varies depending on the number of contact points in the constraint
	///  - it is a stand-alone constraint, therefore it derives from hkpConstraintAtom and cannot be followed by any other atom
struct hkpSimpleContactConstraintAtom : public hkpConstraintAtom
{
		HK_DECLARE_REFLECTION();

		HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpSimpleContactConstraintAtom );

	public:
		// Size of hkpSimpleContactConstraintAtom is dynamically changed by the engine. It holds up to 256 contact points.
		// We initialize the size of the atom to what it is when no contact points are present.
		hkpSimpleContactConstraintAtom() : hkpConstraintAtom(TYPE_CONTACT) {}

		HK_FORCE_INLINE hkContactPoint* getContactPoints() const { return const_cast<hkContactPoint*>( reinterpret_cast<const hkContactPoint*>( this+1 ) ); }
		HK_FORCE_INLINE int getContactPointPropertiesStriding() const;
		HK_FORCE_INLINE hkpContactPointPropertiesStream* getContactPointPropertiesStream() const { return const_cast<hkpContactPointPropertiesStream*>( reinterpret_cast<const hkpContactPointPropertiesStream*>( hkAddByteOffsetConst( getContactPoints(), sizeof(hkContactPoint) * m_numReservedContactPoints ) ) ); }
		HK_FORCE_INLINE hkpContactPointPropertiesStream* getContactPointPropertiesStream(int i) const;

	public:
		HK_FORCE_INLINE hkpConstraintAtom* next() const { HK_ASSERT2(0x5b5a6955, false, "Not implemented. Need to compute the entire size of contact points & properties."); return HK_NULL; }
		HK_FORCE_INLINE int numSolverResults() const    { return m_numContactPoints+3; }

		HK_FORCE_INLINE void addToConstraintInfo(hkpConstraintInfo& infoOut) const
		{
			int size = m_numContactPoints;

			infoOut.m_sizeOfSchemas    += HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA 
										+ (size >> 1) * HK_SIZE_OF_JACOBIAN_PAIR_CONTACT_SCHEMA 
				                        + (size & 1) * HK_SIZE_OF_JACOBIAN_SINGLE_CONTACT_SCHEMA 
										+ HK_SIZE_OF_JACOBIAN_2D_FRICTION_SCHEMA;
			infoOut.m_maxSizeOfSchema = infoOut.m_sizeOfSchemas + (HK_SIZE_OF_JACOBIAN_3D_FRICTION_SCHEMA - HK_SIZE_OF_JACOBIAN_2D_FRICTION_SCHEMA);
			infoOut.m_numSolverResults   += size + 2;
			infoOut.m_numSolverElemTemps += size + (2 + 1); // extra one for friction
			if ( size >= 2 )
			{
				infoOut.m_sizeOfSchemas   += HK_SIZE_OF_JACOBIAN_3D_FRICTION_SCHEMA - HK_SIZE_OF_JACOBIAN_2D_FRICTION_SCHEMA;
				infoOut.m_numSolverResults   += 1;
				infoOut.m_numSolverElemTemps += 1 + 0; // just one elem for both 2d & 3d friction anyways.
			}
		}

	public:
		hkUint16 m_sizeOfAllAtoms;
		hkUint16 m_numContactPoints;
		hkUint16 m_numReservedContactPoints;

			// 8 bytes left from here

			// Extra storage for bodyA. Holds hkpShapeKey hierarchy, by default.
		hkUint8 m_numUserDatasForBodyA;

			// Extra storage for bodyA. Holds hkpShapeKey hierarchy, by default.
		hkUint8 m_numUserDatasForBodyB;

			// store striding in here to avoid recomputing in getContactPointPropsStriding()..
		hkUint8 m_contactPointPropertiesStriding;

			// Maximum number for contact points. The limitation is driven by buffer size on SPU, and by contactPointProperties size.
		hkUint16 m_maxNumContactPoints; // -- this will not be cross-platform, and will use problems with contact points saved on PC, and to be loaded on PS3 <ag.todo.b>


		HK_ALIGN16(class hkpSimpleContactConstraintDataInfo m_info);

	public:
		hkpSimpleContactConstraintAtom(hkFinishLoadedObjectFlag f);
};


// Only include it after the hkpSimpleContactConstraintAtom is defined
#include <Physics/ConstraintSolver/Constraint/Contact/hkpContactPointProperties.h>

HK_FORCE_INLINE int hkpSimpleContactConstraintAtom::getContactPointPropertiesStriding() const { return m_contactPointPropertiesStriding; }
HK_FORCE_INLINE hkpContactPointPropertiesStream* hkpSimpleContactConstraintAtom::getContactPointPropertiesStream(int i) const 
{
	const hkContactPoint* endContactPoint = hkAddByteOffsetConst( getContactPoints(), sizeof(hkContactPoint) * m_numReservedContactPoints);
	const hkpContactPointPropertiesStream* beginningOfProperties = reinterpret_cast<const hkpContactPointPropertiesStream*>( endContactPoint );
	return const_cast<hkpContactPointPropertiesStream*>( hkAddByteOffsetConst(  beginningOfProperties, HK_HINT_SIZE16(i) * HK_HINT_SIZE16( getContactPointPropertiesStriding())  ) );
}



#if defined(HK_PLATFORM_SPU)
#	if defined(HK_SIMULATE_SPU_DMA_ON_CPU)
		extern hkpSimpleContactConstraintAtom* hkSimSpuAgentSectorJobGetSpuSimpleContactConstraintAtom(hkpSimpleContactConstraintAtom* atomOnPpu);
		extern hkpSimpleContactConstraintAtom* g_spuAgentSectorJobAtomOnPpu;
#		define HK_GET_LOCAL_CONTACT_ATOM(ATOM) hkSimSpuAgentSectorJobGetSpuSimpleContactConstraintAtom(ATOM)
#		define HK_CONTACT_ATOM_SET_PPU(ATOM) g_spuAgentSectorJobAtomOnPpu = ATOM;
#	else
		extern hkpSimpleContactConstraintAtom* g_spuAgentSectorJobAtomOnSpu;
#		define HK_GET_LOCAL_CONTACT_ATOM(ATOM) g_spuAgentSectorJobAtomOnSpu
#		define HK_CONTACT_ATOM_SET_PPU(ATOM)
#	endif
#else
#	define HK_GET_LOCAL_CONTACT_ATOM(ATOM) ATOM
#	define HK_CONTACT_ATOM_SET_PPU(ATOM)
#endif


	/// Fully eliminates relative linear movement of bodies' pivots.
	///
	/// This is the most common atom. It is advised to place it at the end of the list of atoms to minimize results error.
	/// This atom eliminates 3 degrees of freedom and returns 3 solver results. It has no parameters.
struct hkpBallSocketConstraintAtom : public hkpConstraintAtom
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpBallSocketConstraintAtom );
	HK_DECLARE_REFLECTION();

public:
	hkpBallSocketConstraintAtom() : hkpConstraintAtom(TYPE_BALL_SOCKET) {}
		/// Return the next atom after this.
	HK_FORCE_INLINE hkpConstraintAtom* next() const { return const_cast<hkpConstraintAtom*>( static_cast<const hkpConstraintAtom*>(this+1) ); }
		/// This tells how many solver-constraints this atom generates and how may solver-results slots it requires.
	HK_FORCE_INLINE int numSolverResults() const    { return 3; }
		/// This tells how much memory the system will need to store solver schemas and jacobians for this atom.
	HK_FORCE_INLINE void addToConstraintInfo(hkpConstraintInfo& infoOut) const { infoOut.add( 3 * HK_SIZE_OF_JACOBIAN_1D_BILATERAL_SCHEMA, 3, 3 ); }

	hkpBallSocketConstraintAtom(hkFinishLoadedObjectFlag f) : hkpConstraintAtom(f) {}
};

	/// Enforces a constant distance between the pivot points of linked bodies.
struct hkpStiffSpringConstraintAtom : public hkpConstraintAtom
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpStiffSpringConstraintAtom );
	HK_DECLARE_REFLECTION();

public:
	hkpStiffSpringConstraintAtom() : hkpConstraintAtom(TYPE_STIFF_SPRING) {}
	HK_FORCE_INLINE hkpConstraintAtom* next() const { return const_cast<hkpConstraintAtom*>( static_cast<const hkpConstraintAtom*>(this+1) ); }
	HK_FORCE_INLINE int numSolverResults() const    { return 1; }
	HK_FORCE_INLINE void addToConstraintInfo(hkpConstraintInfo& infoOut) const { infoOut.add( HK_SIZE_OF_JACOBIAN_1D_BILATERAL_SCHEMA, 1, 1 ); }

		/// The rest length / distance between pivot points.
	hkReal m_length;

	hkpStiffSpringConstraintAtom(hkFinishLoadedObjectFlag f) : hkpConstraintAtom(f) {}
};

	/// This specifies constraint spaces and pivot points in the local spaces of each body.
	///
	/// Pivot points are stored in the translation part of the transforms.
struct hkpSetLocalTransformsConstraintAtom : public hkpConstraintAtom
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpSetLocalTransformsConstraintAtom );
	HK_DECLARE_REFLECTION();

	hkpSetLocalTransformsConstraintAtom() : hkpConstraintAtom(TYPE_SET_LOCAL_TRANSFORMS) {}
	HK_FORCE_INLINE hkpConstraintAtom* next() const { return const_cast<hkpConstraintAtom*>( static_cast<const hkpConstraintAtom*>(this+1) ); }
	HK_FORCE_INLINE int numSolverResults() const    { return 0; }
	// addToConstraintInfo not needed

		/// Constraint orientation and origin/pivot point in bodyA's local space.
	hkTransform m_transformA;
		/// Constraint orientation and origin/pivot point in bodyB's local space.
	hkTransform m_transformB;

	hkpSetLocalTransformsConstraintAtom(hkFinishLoadedObjectFlag f) : hkpConstraintAtom(f) {}
};

	/// This specifies pivot points in the local spaces of each body.
	///
	/// Note that this does not overwrite the constraint space's orientation.
	/// This is used when constraint orientation is irrelevant, e.g. in hkpBallAndSocketConstraintData.
struct hkpSetLocalTranslationsConstraintAtom : public hkpConstraintAtom
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpSetLocalTranslationsConstraintAtom );
	HK_DECLARE_REFLECTION();

	hkpSetLocalTranslationsConstraintAtom() : hkpConstraintAtom(TYPE_SET_LOCAL_TRANSLATIONS) {}
	HK_FORCE_INLINE hkpConstraintAtom* next() const { return const_cast<hkpConstraintAtom*>( static_cast<const hkpConstraintAtom*>(this+1) ); }
	HK_FORCE_INLINE int numSolverResults() const    { return 0; }
	// addToConstraintInfo not needed

		/// Pivot point in bodyA's local space.
	hkVector4 m_translationA;
		/// Pivot point in bodyB's local space.
	hkVector4 m_translationB;

	hkpSetLocalTranslationsConstraintAtom(hkFinishLoadedObjectFlag f) : hkpConstraintAtom(f) {}
};

	/// This specifies constraint spaces in the local spaces of each body.
	///
	/// Note that this does not overwrite the pivot points.
	/// This is used when the constraint space must be reoriented for some atoms in more complex hkConstraintDatas, e.g. in the hkpWheelConstraintData.
struct hkpSetLocalRotationsConstraintAtom : public hkpConstraintAtom
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpSetLocalRotationsConstraintAtom );
	HK_DECLARE_REFLECTION();

	hkpSetLocalRotationsConstraintAtom() : hkpConstraintAtom(TYPE_SET_LOCAL_ROTATIONS) {}
	HK_FORCE_INLINE hkpConstraintAtom* next() const { return const_cast<hkpConstraintAtom*>( static_cast<const hkpConstraintAtom*>(this+1) ); }
	HK_FORCE_INLINE int numSolverResults() const    { return 0; }
	// addToConstraintInfo not needed

		/// Constraint orientation in bodyA's local space.
	hkRotation m_rotationA;
		/// Constraint orientation in bodyB's local space.
	hkRotation m_rotationB;

	hkpSetLocalRotationsConstraintAtom(hkFinishLoadedObjectFlag f) : hkpConstraintAtom(f) {}
};

struct hkpOverwritePivotConstraintAtom : public hkpConstraintAtom
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpOverwritePivotConstraintAtom );
	HK_DECLARE_REFLECTION();

	hkpOverwritePivotConstraintAtom() : hkpConstraintAtom(TYPE_OVERWRITE_PIVOT), m_copyToPivotBFromPivotA(true) { }
	HK_FORCE_INLINE hkpConstraintAtom* next() const { return const_cast<hkpConstraintAtom*>( static_cast<const hkpConstraintAtom*>(this+1) ); }
	HK_FORCE_INLINE int numSolverResults() const    { return 0; }

	hkUint8 m_copyToPivotBFromPivotA;

	hkpOverwritePivotConstraintAtom(hkFinishLoadedObjectFlag f) : hkpConstraintAtom(f) {}
};

	/// Eliminates relative linear velocity of bodies' pivot points along one specified axis.
	///
	/// This is used when relative linear movement is only partly constrained as it is in e.g. prismatic or point-to-plane constraints.
struct hkpLinConstraintAtom : public hkpConstraintAtom
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpLinConstraintAtom );
	HK_DECLARE_REFLECTION();

	hkpLinConstraintAtom() : hkpConstraintAtom(TYPE_LIN) {}
	HK_FORCE_INLINE hkpConstraintAtom* next() const { return const_cast<hkpConstraintAtom*>( static_cast<const hkpConstraintAtom*>(this+1) ); }
	HK_FORCE_INLINE int numSolverResults() const    { return 1; }
	HK_FORCE_INLINE void addToConstraintInfo(hkpConstraintInfo& infoOut) const { infoOut.add( HK_SIZE_OF_JACOBIAN_1D_BILATERAL_SCHEMA, 1, 1 ); }

		/// Specifies the index of the axis of the bodyB's constraint base, that will be constrained.
	hkUint8 m_axisIndex;

	hkpLinConstraintAtom(hkFinishLoadedObjectFlag f) : hkpConstraintAtom(f) {}
};

	/// Softens/controls relative linear velocity of bodies' pivot points along one specified axis.
	///
	/// This results in a spring-like reaction, it's used in the hkpWheelConstraintData.
struct hkpLinSoftConstraintAtom : public hkpConstraintAtom
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpLinSoftConstraintAtom );
	HK_DECLARE_REFLECTION();

	hkpLinSoftConstraintAtom() : hkpConstraintAtom(TYPE_LIN_SOFT) {}
	HK_FORCE_INLINE hkpConstraintAtom* next() const { return const_cast<hkpConstraintAtom*>( static_cast<const hkpConstraintAtom*>(this+1) ); }
	HK_FORCE_INLINE int numSolverResults() const    { return 1; }
	HK_FORCE_INLINE void addToConstraintInfo(hkpConstraintInfo& infoOut) const { infoOut.add( HK_SIZE_OF_JACOBIAN_1D_BILATERAL_USER_TAU_SCHEMA, 1, 1 ); }

		/// Specifies the index of the axis of the bodyB's constraint base, that will be constrained.
	hkUint8 m_axisIndex;
		/// Specifies a custom value for the tau parameter used by the solver.
	hkReal m_tau;
		/// Specifies a custom value for the damping parameter used by the solver.
	hkReal m_damping;

	hkpLinSoftConstraintAtom(hkFinishLoadedObjectFlag f) : hkpConstraintAtom(f) {}
};

	/// Limits allowed relative distance between bodies' pivot points along one specified axis.
	///
	/// This allows unconstrained movement within the specified range, and applies hard limits at its ends.
struct hkpLinLimitConstraintAtom : public hkpConstraintAtom
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpLinLimitConstraintAtom );
	HK_DECLARE_REFLECTION();

	hkpLinLimitConstraintAtom() : hkpConstraintAtom(TYPE_LIN_LIMIT) {}
	HK_FORCE_INLINE hkpConstraintAtom* next() const { return const_cast<hkpConstraintAtom*>( static_cast<const hkpConstraintAtom*>(this+1) ); }
	HK_FORCE_INLINE int numSolverResults() const    { return 1; }
	HK_FORCE_INLINE void addToConstraintInfo(hkpConstraintInfo& infoOut) const { infoOut.add( HK_SIZE_OF_JACOBIAN_1D_LINEAR_LIMIT_SCHEMA, 1, 1 ); }

		/// The index of the axis of the bodyB's constraint base, that will be limited.
	hkUint8 m_axisIndex;
		/// Minimum distance along the axis (may be negative).
	hkReal m_min;
		/// Maximum distance along the axis (may be negative).
	hkReal m_max;

	hkpLinLimitConstraintAtom(hkFinishLoadedObjectFlag f) : hkpConstraintAtom(f) {}
};

	/// Eliminates two degrees of freedom of angular movement and allows relative rotation along a specified axis only.
	///
	/// Angular-constraint atoms are often combined with linear-constraint atoms, e.g. this atoms combined with the ball-and-socket
	/// atom forms a hkpHingeConstraintData.
struct hkp2dAngConstraintAtom : public hkpConstraintAtom
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkp2dAngConstraintAtom );
	HK_DECLARE_REFLECTION();

	hkp2dAngConstraintAtom() : hkpConstraintAtom(TYPE_2D_ANG) {}
	HK_FORCE_INLINE hkpConstraintAtom* next() const { return const_cast<hkpConstraintAtom*>( static_cast<const hkpConstraintAtom*>(this+1) ); }
	HK_FORCE_INLINE int numSolverResults() const    { return 2; }
	HK_FORCE_INLINE void addToConstraintInfo(hkpConstraintInfo& infoOut) const { infoOut.add( 2 * HK_SIZE_OF_JACOBIAN_1D_BILATERAL_SCHEMA, 2, 2 ); }

		/// Specifies the index of the unconstrained axis of relative rotation in bodyB's constraint base.
	hkUint8 m_freeRotationAxis;

	hkp2dAngConstraintAtom(hkFinishLoadedObjectFlag f) : hkpConstraintAtom(f) {}
};


	/// Eliminates one, two, or three degrees of freedom of angular movement.
	///
	/// Note: this is only tested for eliminating three degrees of freedom.
struct hkpAngConstraintAtom : public hkpConstraintAtom
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpAngConstraintAtom );
	HK_DECLARE_REFLECTION();

	hkpAngConstraintAtom() : hkpConstraintAtom(TYPE_ANG) {}
	HK_FORCE_INLINE hkpConstraintAtom* next() const { return const_cast<hkpConstraintAtom*>( static_cast<const hkpConstraintAtom*>(this+1) ); }
	HK_FORCE_INLINE int numSolverResults() const    { return m_numConstrainedAxes; }
	HK_FORCE_INLINE void addToConstraintInfo(hkpConstraintInfo& infoOut) const { infoOut.add( m_numConstrainedAxes * HK_SIZE_OF_JACOBIAN_1D_BILATERAL_SCHEMA, m_numConstrainedAxes, m_numConstrainedAxes ); }

		/// Index of the first axis to constrain, in bodyA's constraint base.
	hkUint8 m_firstConstrainedAxis;

		/// Number of subsequent base axes to constrain.
	hkUint8 m_numConstrainedAxes;

	hkpAngConstraintAtom(hkFinishLoadedObjectFlag f) : hkpConstraintAtom(f) {}
};


	/// Limits allowed relative angle between bodies' rotations along one specified rotation axis.
	///
	/// This allows unconstrained movement within the specified range, and applies hard limits at its ends.
struct hkpAngLimitConstraintAtom : public hkpConstraintAtom
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpAngLimitConstraintAtom );
	HK_DECLARE_REFLECTION();

	hkpAngLimitConstraintAtom() : hkpConstraintAtom(TYPE_ANG_LIMIT), m_isEnabled(true) {}
	HK_FORCE_INLINE hkpConstraintAtom* next() const { return const_cast<hkpConstraintAtom*>( static_cast<const hkpConstraintAtom*>(this+1) ); }
	HK_FORCE_INLINE int numSolverResults() const    { return 1; }
	HK_FORCE_INLINE void addToConstraintInfo(hkpConstraintInfo& infoOut) const { infoOut.add( HK_SIZE_OF_JACOBIAN_1D_ANGULAR_LIMITS_SCHEMA, 1, 1 ); }

		/// Tells whether the atom should be handled by the solver.
		///
		/// Note that if it is not, the atom's corresponding hkpSolverResults are not updated.
	hkUint8 m_isEnabled;

		/// The index of the axis in the bodyA's constraint base, that will be limited.
	hkUint8 m_limitAxis;

		/// Mininum angle value in radians (may be negative).
	hkReal m_minAngle;

		/// Maximum angle value in radians (may be negative).
	hkReal m_maxAngle;

		/// A stiffness factor [0..1] used by the solver; defaults to 1.0.
	hkReal m_angularLimitsTauFactor; //+default(1.0) +absmin(0) +absmax(1)

	hkpAngLimitConstraintAtom(hkFinishLoadedObjectFlag f) : hkpConstraintAtom(f) {}
};


	/// Limits allowed relative angle between bodies' rotations along one specified rotation axis.
	///
	/// This constraint allows unconstrained movement within the specified range, and applies hard limits at its ends.
struct hkpTwistLimitConstraintAtom : public hkpConstraintAtom
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpTwistLimitConstraintAtom );
	HK_DECLARE_REFLECTION();

	hkpTwistLimitConstraintAtom() : hkpConstraintAtom(TYPE_TWIST_LIMIT), m_isEnabled(true) {}
	HK_FORCE_INLINE hkpConstraintAtom* next() const { return const_cast<hkpConstraintAtom*>( static_cast<const hkpConstraintAtom*>(this+1) ); }
	HK_FORCE_INLINE int numSolverResults() const    { return 1; }
	HK_FORCE_INLINE void addToConstraintInfo(hkpConstraintInfo& infoOut) const { infoOut.add( HK_SIZE_OF_JACOBIAN_1D_ANGULAR_LIMITS_SCHEMA, 1, 1 ); }

		/// Tells whether the atom should be handled by the solver.
		///
		/// Note that if it is not, the atom's corresponding hkpSolverResults are not updated.
	hkUint8 m_isEnabled;

		/// The index of the axis in the bodyA's constraint base, that will be limited.
	hkUint8 m_twistAxis;

		/// The index of a perpendicular axis used as a reference to measure the angle.
	hkUint8 m_refAxis;

		/// Mininum angle value in radians (may be negative).
	hkReal m_minAngle;

		/// Maximum angle value in radians (may be negative).
	hkReal m_maxAngle;

		/// A stiffness factor [0..1] used by the solver; defaults to 1.0.
	hkReal m_angularLimitsTauFactor; //+default(1.0) +absmin(0) +absmax(1)

	hkpTwistLimitConstraintAtom(hkFinishLoadedObjectFlag f) : hkpConstraintAtom(f) {}
};

	/// Limits allowed relative angle between bodies' rotations as measured between two chosen axes.
	///
	/// This allows unconstrained movement within the specified range, and applies hard limits at its ends.
struct hkpConeLimitConstraintAtom : public hkpConstraintAtom
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpConeLimitConstraintAtom );
	HK_DECLARE_REFLECTION();

	hkpConeLimitConstraintAtom() : hkpConstraintAtom(TYPE_CONE_LIMIT), m_isEnabled(true), m_memOffsetToAngleOffset(0) {}
	HK_FORCE_INLINE hkpConstraintAtom* next() const { return const_cast<hkpConstraintAtom*>( static_cast<const hkpConstraintAtom*>(this+1) ); }
	HK_FORCE_INLINE int numSolverResults() const    { return 1; }
	HK_FORCE_INLINE void addToConstraintInfo(hkpConstraintInfo& infoOut) const { infoOut.add( HK_SIZE_OF_JACOBIAN_1D_ANGULAR_LIMITS_SCHEMA, 1, 1 ); }

		/// Tells whether the atom should be handled by the solver.
		///
		/// Note that if it is not, the atom's corresponding hkpSolverResults are not updated.
	hkUint8 m_isEnabled;

		/// The index of the axis in the bodyA's constraint base, that will be used as a reference vector and constrained to lie within the limit cone.
	hkUint8 m_twistAxisInA;

		/// The index of the axis in the bodyB's constraint base, that will be used as a reference and limit-cone axis.
	hkUint8 m_refAxisInB;

		/// Specifies how the angle between the two reference vectors is measured.
	enum MeasurementMode
	{
		// Do not change enumeration values! They're used in calculations.

			/// Zero-angle corresponds to situation where the two vectors are aligned.
		ZERO_WHEN_VECTORS_ALIGNED = 0,
			/// Zero-angle corresponds to situation where the two vectors are perpendicular, and (+)90-degree corresponds to vectors being aligned.
		ZERO_WHEN_VECTORS_PERPENDICULAR = 1
	};

		/// Specifies how the angle between the two reference vectors is measured.
	hkEnum<MeasurementMode, hkUint8> m_angleMeasurementMode;

		/// Memory offset to location in runtime where m_coneAngleOffset is stored.
		/// That is used to stablize the constraint.
		/// Zero offset means, that no extra stabilization is done.
	hkUint8 m_memOffsetToAngleOffset; //+default(1)

		/// Mininum angle value in radians (may be negative).
	hkReal m_minAngle;

		/// Maximum angle value in radians (may be negative).
	hkReal m_maxAngle;

		/// A stiffness factor [0..1] used by the solver; defaults to 1.0.
	hkReal m_angularLimitsTauFactor; //+default(1.0) +absmin(0) +absmax(1)

	hkpConeLimitConstraintAtom(hkFinishLoadedObjectFlag f) : hkpConstraintAtom(f) {}
};

	/// Applies friction torque along one, two, or three specified rotation axes.
struct hkpAngFrictionConstraintAtom : public hkpConstraintAtom
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpAngFrictionConstraintAtom );
	HK_DECLARE_REFLECTION();

	hkpAngFrictionConstraintAtom() : hkpConstraintAtom(TYPE_ANG_FRICTION), m_isEnabled(true), m_numFrictionAxes(1) {}
	HK_FORCE_INLINE hkpConstraintAtom* next() const { return const_cast<hkpConstraintAtom*>( static_cast<const hkpConstraintAtom*>(this+1) ); }
	HK_FORCE_INLINE int numSolverResults() const    { return m_numFrictionAxes; }
	HK_FORCE_INLINE void addToConstraintInfo(hkpConstraintInfo& infoOut) const { infoOut.add( m_numFrictionAxes * HK_SIZE_OF_JACOBIAN_1D_ANGULAR_FRICTION_SCHEMA, m_numFrictionAxes, (1+1) * m_numFrictionAxes ); }

		/// Tells whether the atom should be handled by the solver.
		///
		/// Note that if it is not, the atom's corresponding hkpSolverResults are not updated.
	hkUint8 m_isEnabled;

		/// Index of the first axis to apply friction along, in bodyA's constraint base.
	hkUint8 m_firstFrictionAxis;

		/// Number of subsequent base axes to constrain.
	hkUint8 m_numFrictionAxes;

		/// Maximum allowed torque to be applied due to friction.
	hkReal m_maxFrictionTorque;

	hkpAngFrictionConstraintAtom(hkFinishLoadedObjectFlag f) : hkpConstraintAtom(f) {}
};

	/// Controls relative rotation angle between bodies around a specified rotation axes.
	///
	/// Note that motor atoms require access to external variables stored in hkpConstraintInstance's runtime.
	/// The atom accesses those variables using memory offsets (stored in the atom's members).
	/// Also when the motor is to operate in a range exceeding the [-Pi, Pi] range it must have a reference
	/// onto solver results of a corresponding hkpAngLimitConstraintAtom to retrieve the proper angle value.
struct hkpAngMotorConstraintAtom : public hkpConstraintAtom
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpAngMotorConstraintAtom );
	HK_DECLARE_REFLECTION();

	hkpAngMotorConstraintAtom() : hkpConstraintAtom(TYPE_ANG_MOTOR) { m_isEnabled = true; m_initializedOffset = 0; m_previousTargetAngleOffset = 0; }
	HK_FORCE_INLINE hkpConstraintAtom* next() const { return const_cast<hkpConstraintAtom*>( static_cast<const hkpConstraintAtom*>(this+1) ); }
	HK_FORCE_INLINE int numSolverResults() const    { return 1; }
	HK_FORCE_INLINE void addToConstraintInfo(hkpConstraintInfo& infoOut) const { infoOut.add( HK_SIZE_OF_JACOBIAN_1D_ANGULAR_MOTOR_SCHEMA, 1, (1+2) ); }

		/// Tells whether the atom should be handled by the solver.
		///
		/// Note that if it is not, the atom's corresponding hkpSolverResults are not updated.
	hkBool m_isEnabled;

		/// The index of the axis in the bodyA's constraint base, that will be controlled.
	hkUint8 m_motorAxis;

		/// Memory offset from atom's solver results to runtime's m_initialized member.
	hkInt16 m_initializedOffset;

		/// Memory offset from atom's solver results to runtime's m_previousTargetAngle member
	hkInt16 m_previousTargetAngleOffset;

		/// This is an optional offset to solver results of an angular limit atom.
		/// The results store the actual angle from the last frame, and are needed if the motor
		/// is to allow 'screw' functionality (ie. orientation is not represented by a cyclic
		/// [-180deg, 180deg] range, but as an unlimited number of degrees/rotations).
	hkInt16 m_correspondingAngLimitSolverResultOffset;

		/// The target angle for the motor.
	hkReal m_targetAngle;

		/// Motor; note that it is reference counted and should be handled by the owning constraint's get/set methods.
	HK_CPU_PTR(class hkpConstraintMotor*) m_motor;

	hkpAngMotorConstraintAtom(hkFinishLoadedObjectFlag f) : hkpConstraintAtom(f) {}
};

	/// Controls relative rotation angle between bodies in three dimensiond; used by the hkpRagdollConstraintData.
	///
	/// Note that motor atoms require access to external variables stored in hkpConstraintInstance's runtime.
	/// The atom accesses those variables using memory offsets (stored in the atom's members).
struct hkpRagdollMotorConstraintAtom : public hkpConstraintAtom
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpRagdollMotorConstraintAtom );
	HK_DECLARE_REFLECTION();

	hkpRagdollMotorConstraintAtom() : hkpConstraintAtom(TYPE_RAGDOLL_MOTOR) { m_isEnabled = true; m_initializedOffset = 0; m_previousTargetAnglesOffset = 0; }
	HK_FORCE_INLINE hkpConstraintAtom* next() const { return const_cast<hkpConstraintAtom*>( static_cast<const hkpConstraintAtom*>(this+1) ); }
	HK_FORCE_INLINE int numSolverResults() const    { return 3; }
	HK_FORCE_INLINE void addToConstraintInfo(hkpConstraintInfo& infoOut) const { infoOut.add( 3 * HK_SIZE_OF_JACOBIAN_1D_ANGULAR_MOTOR_SCHEMA, 3, (1+2) * 3 ); }

		/// Tells whether the atom should be handled by the solver.
		///
		/// Note that if it is not, the atom's corresponding hkpSolverResults are not updated.
	hkBool m_isEnabled;

		/// Memory offset from atom's solver results to runtime's m_initialized member.
	hkInt16 m_initializedOffset;

		/// Memory offset from atom's solver results to runtime's m_previousTargetAngle member.
	hkInt16 m_previousTargetAnglesOffset;

		/// The target frame the motors will try to match.
	hkMatrix3 m_targetFrameAinB;

		/// Three motors; note that they are reference counted and should be handled by the owning constraint's get/set methods.
	HK_CPU_PTR(class hkpConstraintMotor*) m_motors[3];

	hkpRagdollMotorConstraintAtom(hkFinishLoadedObjectFlag f) : hkpConstraintAtom(f) {}
};

	/// Applies friction force along a specified axes.
struct hkpLinFrictionConstraintAtom : public hkpConstraintAtom
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpLinFrictionConstraintAtom );
	HK_DECLARE_REFLECTION();

	hkpLinFrictionConstraintAtom() : hkpConstraintAtom(TYPE_LIN_FRICTION), m_isEnabled(true) {}
	HK_FORCE_INLINE hkpConstraintAtom* next() const { return const_cast<hkpConstraintAtom*>( static_cast<const hkpConstraintAtom*>(this+1) ); }
	HK_FORCE_INLINE int numSolverResults() const    { return 1; }
	HK_FORCE_INLINE void addToConstraintInfo(hkpConstraintInfo& infoOut) const { infoOut.add( HK_SIZE_OF_JACOBIAN_1D_FRICTION_SCHEMA, 1, (1+1) ); }

		/// Tells whether the atom should be handled by the solver.
		///
		/// Note that if it is not, the atom's corresponding hkpSolverResults are not updated.
	hkUint8 m_isEnabled;

		/// Index of the axis to apply friction along, in bodyB's constraint base.
	hkUint8 m_frictionAxis;

		/// Maximum allowed force to be applied due to friction.
	hkReal m_maxFrictionForce;

	hkpLinFrictionConstraintAtom(hkFinishLoadedObjectFlag f) : hkpConstraintAtom(f) {}
};

	/// Controls relative velocity of bodies along a specified axis.
	///
	/// Note that motor atoms require access to external variables stored in hkpConstraintInstance's runtime.
	/// The atom accesses those variables using memory offsets (stored in the atom's members).
struct hkpLinMotorConstraintAtom : public hkpConstraintAtom
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpLinMotorConstraintAtom );
	HK_DECLARE_REFLECTION();

	hkpLinMotorConstraintAtom() : hkpConstraintAtom(TYPE_LIN_MOTOR) { m_isEnabled = true; m_initializedOffset = 0; m_previousTargetPositionOffset = 0; }
	HK_FORCE_INLINE hkpConstraintAtom* next() const { return const_cast<hkpConstraintAtom*>( static_cast<const hkpConstraintAtom*>(this+1) ); }
	HK_FORCE_INLINE int numSolverResults() const    { return 1; }
	HK_FORCE_INLINE void addToConstraintInfo(hkpConstraintInfo& infoOut) const { infoOut.add( HK_SIZE_OF_JACOBIAN_1D_LINEAR_MOTOR_SCHEMA, 1, (1+2) ); }

		/// A flag saying whether the motor is active
	hkBool  m_isEnabled;

		/// The index of the axis in the bodyB's constraint base, that will be controlled.
	hkUint8 m_motorAxis;

		/// Memory offset from atom's solver results to runtime's m_initialized member.
	hkInt16 m_initializedOffset;

		/// Memory offset from atom's solver results to runtime's m_previousTargetPosition member.
	hkInt16 m_previousTargetPositionOffset;

		/// The target position for the motor.
	hkReal m_targetPosition;

		/// Motor; note that it is reference counted and should be handled by the owning constraint's get/set methods.
	HK_CPU_PTR(class hkpConstraintMotor*) m_motor;

	hkpLinMotorConstraintAtom(hkFinishLoadedObjectFlag f) : hkpConstraintAtom(f) {}
};

	/// This implements a functionality of a pulley, where bodies are attached to a rope, and the rope is lead through two pulley wheels at fixed world positions.
struct hkpPulleyConstraintAtom : public hkpConstraintAtom
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT, hkpPulleyConstraintAtom );
	HK_DECLARE_REFLECTION();

	hkpPulleyConstraintAtom() : hkpConstraintAtom(TYPE_PULLEY) { }
	HK_FORCE_INLINE hkpConstraintAtom* next() const { return const_cast<hkpConstraintAtom*>( static_cast<const hkpConstraintAtom*>(this+1) ); }
	HK_FORCE_INLINE int numSolverResults() const    { return 1; }
	HK_FORCE_INLINE void addToConstraintInfo(hkpConstraintInfo& infoOut) const { infoOut.add( HK_SIZE_OF_JACOBIAN_1D_PULLEY_SCHEMA, 1, 1 ); }


		/// Pulley's first fixed pivot point.
	hkVector4 m_fixedPivotAinWorld;
		/// Pulley's second fixed pivot point.
	hkVector4 m_fixedPivotBinWorld;

		/// The rest length (equal to ((BodyA's rope) + leverageOnBodyB * (BodyB's rope length)) )
	hkReal m_ropeLength;
		/// Leverage ratio: e.g. value of 2 means that bodyA's rope length changes by twice as much as bodyB's,
		/// and the constraint exerts twice as big forces upon bodyB.
	hkReal m_leverageOnBodyB;

	hkpPulleyConstraintAtom(hkFinishLoadedObjectFlag f) : hkpConstraintAtom(f) {}
};




	//
	//	Atom Modifiers
	//
struct hkpModifierConstraintAtom : public hkpConstraintAtom
{
		HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR(HK_MEMORY_CLASS_CONSTRAINT, hkpModifierConstraintAtom );
		HK_DECLARE_REFLECTION();

	public:
			// adds the constraintInfo of one modifier to cinfo and returns the hkpConstraintAtom::CallbackRequest
		int addModifierDataToConstraintInfo( hkpConstraintInfo& cinfo ) const;

			// adds its constraintInfo of all linked modifiers to cinfo and returns the hkpConstraintAtom::CallbackRequest
		static int HK_CALL addAllModifierDataToConstraintInfo( hkpModifierConstraintAtom* firstModifier, hkpConstraintInfo& cinfo );

	protected:

		hkpModifierConstraintAtom(enum AtomType type, int size) : hkpConstraintAtom(type), m_modifierAtomSize( hkUint16(size)) {}

	public:

		HK_ALIGN16( hkUint16		  m_modifierAtomSize );
		hkUint16                      m_childSize;
		HK_CPU_PTR(hkpConstraintAtom*) m_child;
		hkUint32 m_pad[2];

		hkpModifierConstraintAtom(hkFinishLoadedObjectFlag f) : hkpConstraintAtom(f) {}
};

	//	************************ Soft Contact **************************
	//	************************ Soft Contact **************************
	//	************************ Soft Contact **************************

struct hkpSoftContactModifierConstraintAtom : public hkpModifierConstraintAtom
{
		HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR(HK_MEMORY_CLASS_CONSTRAINT, hkpSoftContactModifierConstraintAtom );
		HK_DECLARE_REFLECTION();

	public:

		hkpSoftContactModifierConstraintAtom() : hkpModifierConstraintAtom(TYPE_MODIFIER_SOFT_CONTACT, sizeof(hkpSoftContactModifierConstraintAtom)),  m_tau(0.1f), m_maxAcceleration( 20.0f) { }

		void collisionResponseBeginCallback( const hkContactPoint& cp, struct hkpSimpleConstraintInfoInitInput& inA, struct hkpBodyVelocity& velA, hkpSimpleConstraintInfoInitInput& inB, hkpBodyVelocity& velB);
		void collisionResponseEndCallback(   const hkContactPoint& cp, hkReal impulseApplied, struct hkpSimpleConstraintInfoInitInput& inA, struct hkpBodyVelocity& velA, hkpSimpleConstraintInfoInitInput& inB, hkpBodyVelocity& velB);
		HK_FORCE_INLINE int numSolverResults() const { return 0; }

		int getConstraintInfo( hkpConstraintInfo& info ) const	{		return hkpConstraintAtom::CALLBACK_REQUEST_SETUP_PPU_ONLY;		}

	public:

		hkReal m_tau;

			/// The maximum acceleration, the solver will apply
		hkReal m_maxAcceleration;

		hkpSoftContactModifierConstraintAtom(hkFinishLoadedObjectFlag f) : hkpModifierConstraintAtom(f) {}
};




	//	************************ Mass Changer **************************
	//	************************ Mass Changer **************************
	//	************************ Mass Changer **************************

struct hkpMassChangerModifierConstraintAtom : public hkpModifierConstraintAtom
{
		HK_DECLARE_REFLECTION();
		HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR(HK_MEMORY_CLASS_CONSTRAINT, hkpMassChangerModifierConstraintAtom );

	public:

		hkpMassChangerModifierConstraintAtom() : hkpModifierConstraintAtom(TYPE_MODIFIER_MASS_CHANGER, sizeof(hkpMassChangerModifierConstraintAtom)) { }

		HK_FORCE_INLINE int numSolverResults() const { return 0; }

		void collisionResponseBeginCallback( const hkContactPoint& cp, struct hkpSimpleConstraintInfoInitInput& inA, struct hkpBodyVelocity& velA, hkpSimpleConstraintInfoInitInput& inB, hkpBodyVelocity& velB);
		void collisionResponseEndCallback(   const hkContactPoint& cp, hkReal impulseApplied, struct hkpSimpleConstraintInfoInitInput& inA, struct hkpBodyVelocity& velA, hkpSimpleConstraintInfoInitInput& inB, hkpBodyVelocity& velB);

		int getConstraintInfo( hkpConstraintInfo& info ) const
		{
			info.m_sizeOfSchemas   += 2 * HK_SIZE_OF_JACOBIAN_SET_MASS_SCHEMA  + HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA;
			return hkpConstraintAtom::CALLBACK_REQUEST_NONE;
		}

	public:

		hkReal m_factorA;
		hkReal m_factorB;
		hkReal m_pad16[2]; //+nosave

		hkpMassChangerModifierConstraintAtom(hkFinishLoadedObjectFlag f) : hkpModifierConstraintAtom(f) {}
};




	//	************************ Viscous Surface **************************
	//	************************ Viscous Surface **************************
	//	************************ Viscous Surface **************************

struct hkpViscousSurfaceModifierConstraintAtom : public hkpModifierConstraintAtom
{
		HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR(HK_MEMORY_CLASS_CONSTRAINT, hkpViscousSurfaceModifierConstraintAtom );
		HK_DECLARE_REFLECTION();

	public:

		hkpViscousSurfaceModifierConstraintAtom() : hkpModifierConstraintAtom(TYPE_MODIFIER_VISCOUS_SURFACE, sizeof(hkpViscousSurfaceModifierConstraintAtom)) { }

		int getConstraintInfo( hkpConstraintInfo& info ) const	{		return hkpConstraintAtom::CALLBACK_REQUEST_SETUP_PPU_ONLY;		}

		HK_FORCE_INLINE int numSolverResults() const { return 0; }

	public:

		hkpViscousSurfaceModifierConstraintAtom(hkFinishLoadedObjectFlag f) : hkpModifierConstraintAtom(f) {}
};




	//	************************ Moving Surface **************************
	//	************************ Moving Surface **************************
	//	************************ Moving Surface **************************

struct hkpMovingSurfaceModifierConstraintAtom : public hkpModifierConstraintAtom
{
		HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR(HK_MEMORY_CLASS_CONSTRAINT, hkpMovingSurfaceModifierConstraintAtom );
		HK_DECLARE_REFLECTION();

	public:

		hkpMovingSurfaceModifierConstraintAtom() : hkpModifierConstraintAtom(TYPE_MODIFIER_MOVING_SURFACE, sizeof(hkpMovingSurfaceModifierConstraintAtom)) { }

		HK_FORCE_INLINE int numSolverResults() const { return 0; }

		int getConstraintInfo( hkpConstraintInfo& info ) const
		{
			info.m_sizeOfSchemas   += 2 * HK_SIZE_OF_JACOBIAN_ADD_VELOCITY_SCHEMA + HK_SIZE_OF_JACOBIAN_HEADER_SCHEMA;
			return hkpConstraintAtom::CALLBACK_REQUEST_NONE;
		}

		void collisionResponseBeginCallback( const hkContactPoint& cp, struct hkpSimpleConstraintInfoInitInput& inA, struct hkpBodyVelocity& velA, hkpSimpleConstraintInfoInitInput& inB, hkpBodyVelocity& velB);
		void collisionResponseEndCallback(   const hkContactPoint& cp, hkReal impulseApplied, struct hkpSimpleConstraintInfoInitInput& inA, struct hkpBodyVelocity& velA, hkpSimpleConstraintInfoInitInput& inB, hkpBodyVelocity& velB);

		hkVector4& getVelocity() { return m_velocity; }
		const hkVector4& getVelocity() const { return m_velocity; }

	public:

 		hkVector4 m_velocity;

		hkpMovingSurfaceModifierConstraintAtom(hkFinishLoadedObjectFlag f) : hkpModifierConstraintAtom(f) {}
};





#endif // HK_DYNAMICS2_CONSTRAINT_ATOM_H

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

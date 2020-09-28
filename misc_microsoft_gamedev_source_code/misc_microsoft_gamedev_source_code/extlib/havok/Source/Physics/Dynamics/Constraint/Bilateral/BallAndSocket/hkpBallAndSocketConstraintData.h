/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_DYNAMICS2_BALL_AND_SOCKET_CONSTRAINT_H
#define HK_DYNAMICS2_BALL_AND_SOCKET_CONSTRAINT_H

#include <Physics/Dynamics/Constraint/hkpConstraintData.h>
#include <Physics/ConstraintSolver/Constraint/Atom/hkpConstraintAtom.h>

extern const hkClass hkpBallAndSocketConstraintDataClass;



	/// The ball-and-socket or point-to-point constraint.
class hkpBallAndSocketConstraintData : public hkpConstraintData
{
	public:

		HK_DECLARE_REFLECTION();

		hkpBallAndSocketConstraintData();

			/// Sets the construction information with body space information.
			/// \param pivotA The constraint pivot point, specified in bodyA's space.
			/// \param pivotB The constraint pivot point, specified in bodyB's space.
		void setInBodySpace(const hkVector4& pivotA, const hkVector4& pivotB);

			/// Sets the construction information with world space information. Will use the 
			/// given transforms to work out the two local pivots.
			/// \param bodyA The first rigid body transform
			/// \param bodyB The second rigid body transform
			/// \param pivot The constraint pivot point, specified in world space.
		void setInWorldSpace(const hkTransform& bodyATransform, const hkTransform& bodyBTransform, 
							const hkVector4& pivot);

			/// Check consistency of constraint members.
		virtual hkBool isValid() const;

			/// Get type from this constraint.
		virtual int getType() const;

	public:
			//
			//	Solver interface
			//
		enum 
		{
			SOLVER_RESULT_LIN_0 = 0,		// linear constraint 
			SOLVER_RESULT_LIN_1 = 1,		// linear constraint 
			SOLVER_RESULT_LIN_2 = 2,		// linear constraint 
			SOLVER_RESULT_MAX = 3
		};

		struct Runtime
		{
			HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_DYNAMICS, hkpBallAndSocketConstraintData::Runtime );

			class hkpSolverResults m_solverResults[3/*VC6 doesn't like the scoping for SOLVER_RESULT_MAX*/];
		};

		inline const Runtime* getRuntime( hkpConstraintRuntime* runtime ){ return reinterpret_cast<Runtime*>(runtime); }

	public:

		struct Atoms
		{
			HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_DYNAMICS, hkpBallAndSocketConstraintData::Atoms );
			HK_DECLARE_REFLECTION();

			struct hkpSetLocalTranslationsConstraintAtom m_pivots;
			struct hkpBallSocketConstraintAtom m_ballSocket;

			Atoms()	{	}

			// get a pointer to the first atom
			const hkpConstraintAtom* getAtoms() const { return &m_pivots; }

			int getSizeOfAllAtoms() const { return hkGetByteOffsetInt(this, &m_ballSocket+1); }

			Atoms(hkFinishLoadedObjectFlag f) : m_pivots(f), m_ballSocket(f) {}
		};

		HK_ALIGN16( struct Atoms m_atoms );

	public:
			//
			// Internal functions.
			//
		// hkpConstraintData interface implementations
		virtual void getConstraintInfo( hkpConstraintData::ConstraintInfo& infoOut ) const;

		// hkpConstraintData interface implementations
		virtual void getRuntimeInfo( hkBool wantRuntime, hkpConstraintData::RuntimeInfo& infoOut ) const;

	public:

		hkpBallAndSocketConstraintData(hkFinishLoadedObjectFlag f) : hkpConstraintData(f), m_atoms(f) {}
};



#endif // HK_DYNAMICS2_BALL_AND_SOCKET_H

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

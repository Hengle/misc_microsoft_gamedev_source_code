/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_DYNAMICS2_CONSTRAINT_DATA_H
#define HK_DYNAMICS2_CONSTRAINT_DATA_H

#include <Physics/ConstraintSolver/Solve/hkpSolverResults.h>
#include <Physics/ConstraintSolver/Solve/hkpSolve.h>
#include <Physics/Dynamics/Constraint/hkpConstraintInfo.h>

class hkpConstraintQueryIn;
class hkpConstraintQueryOut;
class hkpConstraintOwner;
class hkpConstraintInstance;

extern const hkClass hkpConstraintDataClass;
extern const hkClass hkpConstraintInfoClass;

	// All runtime information of the constraints must start with an array of solverresults
typedef void hkpConstraintRuntime;

class hkpConstraintData;

/// The base class for constraints.
/// This class holds all information about the data of a constraint. 
/// As this data is constant over the lifetime of a constraint, 
class hkpConstraintData : public hkReferencedObject
{
	public:

		HK_DECLARE_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT);
		HK_DECLARE_REFLECTION();

			// Serialization ctor
#if !defined(HK_PLATFORM_SPU)
		hkpConstraintData(hkFinishLoadedObjectFlag f) {}
#endif
            /// Types for canned contraints
        enum ConstraintType
        {
            CONSTRAINT_TYPE_BALLANDSOCKET = 0,
            CONSTRAINT_TYPE_HINGE = 1,
            CONSTRAINT_TYPE_LIMITEDHINGE = 2,
            CONSTRAINT_TYPE_POINTTOPATH = 3,
            CONSTRAINT_TYPE_PRISMATIC = 6,
            CONSTRAINT_TYPE_RAGDOLL = 7,
            CONSTRAINT_TYPE_STIFFSPRING = 8,
            CONSTRAINT_TYPE_WHEEL = 9,
            CONSTRAINT_TYPE_GENERIC = 10,
            CONSTRAINT_TYPE_CONTACT = 11,
            CONSTRAINT_TYPE_BREAKABLE = 12,
            CONSTRAINT_TYPE_MALLEABLE = 13,
            CONSTRAINT_TYPE_POINTTOPLANE = 14,

	        CONSTRAINT_TYPE_PULLEY = 15,

			CONSTRAINT_TYPE_ROTATIONAL = 16,

			CONSTRAINT_TYPE_HINGE_LIMITS = 18,
			CONSTRAINT_TYPE_RAGDOLL_LIMITS = 19,
        
	        // Constraint Chains
	        BEGIN_CONSTRAINT_CHAIN_TYPES = 100,        
	        CONSTRAINT_TYPE_STIFF_SPRING_CHAIN = 100,
	        CONSTRAINT_TYPE_BALL_SOCKET_CHAIN = 101,
	        CONSTRAINT_TYPE_POWERED_CHAIN = 102
        };

			/// Default constructor. You cannot instantiate this class directly - see derived classes.
			/// This constructor just does some debug checks for alignment.
		inline hkpConstraintData();

			/// Destructor.
		inline virtual ~hkpConstraintData();

			/// Get the user data for the constraint (initialized to 0).
		inline hkUlong getUserData() const;

			/// Set the user data of the constraint.
		inline void setUserData( hkUlong data );

			/// Checks member variables of constraint for consistency
			/// this method will be empty for some constraints
			/// This method is automatically called by the constraint constructors
		virtual hkBool isValid() const = 0;

			/// Get the type of constraint
		virtual int getType() const = 0;

		//
		//	Interface for solver runtime data 
		//

			/// Return runtime information
		struct RuntimeInfo
		{
			HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_DYNAMICS, hkpConstraintData::RuntimeInfo );

			int m_sizeOfExternalRuntime;
			int m_numSolverResults;
		};

			/// Get information about the size of the runtime data needed
			/// Notes:
			/// - if wantRuntime is set, you ask for a runtime information
			/// - Some constraint have their private runtime, so no need to externally allocate runtime info
			/// - Some constraint might not want to use any runtime information
			/// - All Runtimes of the constraints should start with an array of hkpSolverResults
		virtual void getRuntimeInfo( hkBool wantRuntime, RuntimeInfo& infoOut ) const = 0;

			/// Gives you access to the solver results in a generic way, use getRuntimeInfo to get the number of solver results
		virtual hkpSolverResults* getSolverResults( hkpConstraintRuntime* runtime );

			/// Initialize the runtime data. The default implementation simply zeros all values
		virtual void addInstance( hkpConstraintInstance* constraint, hkpConstraintRuntime* runtime, int sizeOfRuntime ) const;

			/// Drop an instance reference to this data
		inline void removeInstance( hkpConstraintInstance* constraint, hkpConstraintRuntime* runtime, int sizeOfRuntime ) const;



	protected:	// exported data for the solver setup

		friend class hkpConstraintSolverSetup;

	public:

		//
		// Internal functions
		//

			//Sets up the constraint for the solver.
		virtual void buildJacobian( const hkpConstraintQueryIn &in, hkpConstraintQueryOut &out ) { HK_ASSERT2(0xad567bbd, false, "Function deprecated"); }

			// Does the constraint need a single-threaded callback before jacobians are built from atoms?
		virtual hkBool isBuildJacobianCallbackRequired() { return false; }

			// The callback. Note that all callbacks are performed in single threaded mode.
		virtual void buildJacobianCallback( const hkpConstraintQueryIn &in ) { HK_ASSERT2(0xad567bbd, false, "This function must be overwritten, if the constraint requires a callback."); }

		struct ConstraintInfo : public hkpConstraintInfo
		{
			HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_DYNAMICS, hkpConstraintData::ConstraintInfo );

			struct hkpConstraintAtom* m_atoms;
			hkUint32		  m_sizeOfAllAtoms;
		};

		virtual void getConstraintInfo( ConstraintInfo& infoOut ) const = 0;

		static void HK_CALL getConstraintInfoUtil( const hkpConstraintAtom* atoms, int sizeOfAllAtoms, ConstraintInfo& infoOut );

	public:
		
			/// For any use you want. Not used by the Havok system.
		hkUlong m_userData; //+default(0)
};

#include <Physics/Dynamics/Constraint/hkpConstraintData.inl>

#endif // HK_DYNAMICS2_CONSTRAINT_DATA_H

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

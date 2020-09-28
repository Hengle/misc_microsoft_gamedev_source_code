/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_DYNAMICS2_CONSTRAINT_H
#define HK_DYNAMICS2_CONSTRAINT_H

#include <Physics/Dynamics/Constraint/hkpConstraintData.h>

class hkpConstraintData;
class hkpEntity;
class hkpRigidBody;
class hkpConstraintOwner;
class hkpSimulationIsland;
extern const hkClass hkpConstraintInstanceClass;

	/// An opaque piece of memory, where constraints can store instance related data
	/// Note: the constraint runtime is not 16 byte aligned
typedef void hkpConstraintRuntime;


	/// The base class for constraints.
class hkpConstraintInstance : public hkReferencedObject
{

	public:
		HK_DECLARE_REFLECTION();

		HK_DECLARE_CLASS_ALLOCATOR( HK_MEMORY_CLASS_CONSTRAINT );


			/// Specifies the priority for each constraint.
			/// Values must be corresponding to hkpCollisionDispatcher::CollisionQualityLevel
		enum ConstraintPriority
		{
				/// Invalid priority.
			PRIORITY_INVALID,
				/// Constraint is only solved at regular physics time steps (==PSIs)
			PRIORITY_PSI, 
				/// Constraint is also touched at time of impact events.(TOI).
			PRIORITY_TOI,

				/// For internal use only! Higher quality. Warning: Use this quality only for contact constraints between moving and fixed objects.
				/// Info: Higher priority constraints have higher priority in the solver in PSI steps. They are actually processed as normal PRIORITY_TOI
				///       constraints in TOI events.
			PRIORITY_TOI_HIGHER,
				/// For internal use only! Enforced quality. Warning: Use this quality only for contact constraints between critical and fixed objects.
				/// Info: Forced priority constraints have higher priority in the solver in PSI steps. They have also higher priority in the solver in TOI steps.
				///       Additionally extra CPU time is used at the end of TOI event in attempt to enforce those constraints (with the cost of ignoring the state
				///       of other -- non-forced -- constraints).
			PRIORITY_TOI_FORCED,
		};

			/// Constraint instance's type is queried by the engine to check whether the constraint is a 
			/// 'normal' constraint connecting two entities, or whether it's of a different kind requiring 
			/// special handling. (This is used e.g. when adding/removing constraints to the world.)
		enum InstanceType
		{
				// Standard constraint linking two bodies
			TYPE_NORMAL,
				// Chain of constraints. See hkpConstraintChainInstance
			TYPE_CHAIN
		};

		enum AddReferences
		{
			DO_NOT_ADD_REFERENCES,
			DO_ADD_REFERENCES
		};

		enum CloningMode
		{
			CLONE_INSTANCES_ONLY,
			CLONE_DATAS_WITH_MOTORS
		};

			//
			// Construction, destruction and cloning.
			//

			/// Construct a constraint instance between A and B, with the data
			/// for the constraint provided. The meaning of the Priority is given in
			/// the enum definition.EntityA can not be NULL, EntityB is allowed to be
			/// NULL and it will be replaced by the world fixed body upon addition to the 
			/// world. 
		hkpConstraintInstance(hkpEntity* entityA, hkpEntity* entityB, hkpConstraintData* data, ConstraintPriority priority = PRIORITY_PSI);

			/// Destructor removes references from entities A and B if set.
		~hkpConstraintInstance();

			/// Clone the constraint, sharing as much as possible (ie: the constraint data
			/// if it can). In the default CLONE_INSTANCES_ONLY mode, this will NOT allow you to use different 
			/// motor targets for the original and cloned instances.
			/// 
			/// To allow for different motor targets for the original and cloned instances use the CLONE_DATAS_WITH_MOTORS mode.
		hkpConstraintInstance* clone(hkpEntity* newEntityA, hkpEntity* newEntityB, CloningMode mode = CLONE_INSTANCES_ONLY) const;

	protected:
			/// Internal constructor used by hkpSimpleConstraintContactMgr only 
		hkpConstraintInstance(ConstraintPriority priority);
		
	public:

		//
		// Accessors
		//

			/// Gets the first constrained entity.
		inline hkpEntity* getEntityA() const;

		inline hkpEntity* getEntity( int index ) const;

			/// Gets the second constrained entity.
		inline hkpEntity* getEntityB() const;

			/// Gets the first constrained body.
		inline hkpRigidBody* getRigidBodyA() const;

			/// Gets the second constrained body.
		inline hkpRigidBody* getRigidBodyB() const;


			/// Gets either entity A or B which is not entity
		inline hkpEntity* getOtherEntity( const hkpEntity* entity );

			/// Gets the non fixed island of either entityA or entityB
		hkpSimulationIsland* getSimulationIsland();

			/// Get the hkpConstraintData object
		inline hkpConstraintData* getData() const;

		//
		// Runtime cache data for the constraint.
		//

			/// Get the runtime data.
			/// This can only be called after the constraint has been added to the world.
			/// Note: The runtime data is not 16 byte aligned.
			///
			/// If setWantRuntime has been set to false, this may return HK_NULL.
			/// You can use this data to:
			/// - access the solver results directly
			/// - or to go the specific constraint runtime implementations and ask for the Runtime struct, e.g. hkBallSocketConstraint::getRuntime()
			///
			/// You can get more information about this runtime if you go to the hkpConstraintData
			/// and call hkpConstraintData::getRuntimeInfo(...)
		inline hkpConstraintRuntime* getRuntime() const;

			/// Request that a hkpConstraintRuntime is allocated when you add the constraint to the hkpWorld.
			/// This must be called before the constraint is added to the world.
			///
			/// If you call setWantRuntime( false ), a runtime might not be allocated for this constraint.
			/// Note:
			///  - Not all constraints have a (external) runtime information.
			///  - Some constraints always allocate a runtime, even if the wantRuntime flag is set to false.
		inline void setWantRuntime( hkBool b );

			/// Get the wantRuntime Flag, see setWantRuntime for details
		inline hkBool getWantRuntime() const ;

		//
		// User data.
		//

			/// Get the user data for the constraint (initialized to 0).
		inline hkUlong getUserData() const;

			/// Set the user data of the constraint.
		inline void setUserData( hkUlong data );

		//
		// Priority
		//

			/// Get the priority that this constraint was created with.
		inline ConstraintPriority getPriority() const;

			/// Set the priority for this constraint.
		void setPriority( ConstraintPriority priority );


			/// Get the name of this constraint.
		inline const char* getName() const;

			/// Set the name of this constraint.
			/// IMPORTANT: This data will not be cleaned up by the hkpConstraintInstance destructor. You are required to track it yourself.
		inline void setName( const char* name );



	public:

		//
		// Internal functions
		//

		inline hkpConstraintOwner* getOwner() const;

			// Gets the master entity, can only be called if the constraint is added to the world
		inline hkpEntity* getMasterEntity() const;

			// Gets the slave entity, can only be called if the constraint is added to the world
		inline hkpEntity* getSlaveEntity() const;

			// hkpEntityListener interface implementation
		virtual void entityAddedCallback(hkpEntity* entity);

			// hkpEntityListener interface implementation
		virtual void entityRemovedCallback(hkpEntity* entity);

			// The constraint should never receive this callback, as the constraint keeps a reference to any entities it operates on.
			// The implementation asserts if called.
		virtual void entityDeletedCallback( hkpEntity* entity );

		inline void setOwner( hkpConstraintOwner* island );

		inline struct hkConstraintInternal* getInternal();

		void pointNullsToFixedRigidBody();

		virtual InstanceType getType() const { return TYPE_NORMAL; }

		//
		//	Members
		//
	protected:
		friend class hkpSimpleContactConstraintData;
		friend class hkpSimulationIsland;

		class hkpConstraintOwner* m_owner; //+nosave

		class hkpConstraintData*  m_data;

		struct hkpModifierConstraintAtom* m_constraintModifiers;

		class hkpEntity* m_entities[2];

	public:
		hkEnum<ConstraintPriority,hkUint8> m_priority;

	protected:
			/// Set this to true, if you want to get access to RuntimeData later
		hkBool				m_wantRuntime;

		const char* m_name;

	public:

		hkUlong m_userData; // +default(0)

		struct hkConstraintInternal* m_internal; //+nosave

	public:

			/// The fastconstruction is used by the serialization to
			/// init the vtables and do any extra init (but ONLY of non-serialized members).
		hkpConstraintInstance(hkFinishLoadedObjectFlag f) { }

	private:

		friend class hkpWorldConstraintUtil;
		friend class hkpSimpleConstraintContactMgr;
		friend class hkpSaveContactPointsUtil;

		/// Default constructor, you cannot derive from this class, you should derive from hkpConstraintData
		hkpConstraintInstance(){}
};


HK_CLASSALIGN16(struct) hkConstraintInternal
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_DYNAMICS, hkConstraintInternal );

	//
	//	Public functions
	//
		// Gets the other entity. Not the one specified as the parameter, but the other of the two.
	inline hkpEntity* getOtherEntity( const hkpEntity* entity ) const;
	inline hkpEntity* getMasterEntity() const;
	inline hkpEntity* getSlaveEntity() const;

	inline struct hkpConstraintAtom* getAtoms() const { return m_atoms; }
	inline hkUint16 getAtomsSize() const { return m_atomsSize; }


	//
	//	Internal functions
	//

	inline void clearConstraintInfo( );

	//
	//	Members
	//
	public:

		class hkpConstraintInstance* m_constraint;

			/// The two entities. Attention: the master entity might be entity 0 or 1
		class hkpEntity*	  m_entities[2];	// constant; owned by constraintUpdate

	protected:
		friend class hkpWorldConstraintUtil;

			/// pointer to the low level constraint information	
		struct hkpConstraintAtom* m_atoms;	// changed by the collision detection

			/// Total size of all constraint information
		hkUint16 m_atomsSize;				// changed by the collision detection

	public:
		hkUint8	m_callbackRequest;

		hkEnum<hkpConstraintInstance::ConstraintPriority,hkUint8> m_priority;

		//
		//	internal data
		//

			//
			// data controlled by the constraint owner
			//
		hkUint16 m_sizeOfSchemas;
		hkUint16 m_numSolverResults;
		hkUint16 m_numSolverElemTemps;		

		inline void getConstraintInfo( hkpConstraintInfo& info ) const;
		inline void addConstraintInfo( const hkpConstraintInfo& delta );
		inline void subConstraintInfo( const hkpConstraintInfo& delta );
			//
			// data controlled by hkpWorldConstraintUtil
			//

			// Index of the master entity in m_entities.
		hkUint8	m_whoIsMaster;

		hkBool m_isNormalType;

		hkpConstraintRuntime*	m_runtime;
		hkUint16			m_runtimeSize;

			// Index of the pointer to this hkConsraintInstance stored on the m_constraintsSlave list of the slave entity.
		hkObjectIndex		m_slaveIndex;	// this must not be changed in a multithreaded environment, e.g. by the spu collision detector

		friend class hkpSaveContactPointsUtil;
};

#include <Physics/Dynamics/Constraint/hkpConstraintInstance.inl>

#endif // HK_DYNAMICS2_CONSTRAINT_H




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

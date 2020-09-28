/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */


#ifndef HK_DYNAMICS2_ENTITY_H
#define HK_DYNAMICS2_ENTITY_H

#include <Common/Base/hkBase.h>
//#include <hkbase/htl/hkSmallArray.h>
#include <Physics/Dynamics/World/hkpWorldObject.h>
#include <Physics/Dynamics/Common/hkpMaterial.h>
#include <Physics/Dynamics/Constraint/hkpConstraintInstance.h>
#include <Physics/Dynamics/Motion/Rigid/hkpKeyframedRigidMotion.h>

class hkpEntityListener;
class hkpEntityActivationListener;
class hkpCollisionListener;
class hkpMotion;
class hkpSimulationIsland;
class hkpWorld;
class hkpConstraintInstance;
class hkpAction;
class hkpDynamicsContactMgr;
class hkSpuCollisionCallbackUtil;

extern const hkClass hkpEntityClass;

/// This class represents the core "physical object" elements in the dynamics system, such
/// as rigid bodies. 
class hkpEntity : public hkpWorldObject
{
	public:

		enum SpuCollisionCallbackEventFilter
		{
			SPU_SEND_NONE					= 0x00,
			SPU_SEND_CONTACT_POINT_ADDED	= 0x01,
			SPU_SEND_CONTACT_POINT_PROCESS	= 0x02,
			SPU_SEND_CONTACT_POINT_REMOVED	= 0x04,
			SPU_SEND_CONTACT_POINT_ADDED_OR_PROCESS = SPU_SEND_CONTACT_POINT_ADDED|SPU_SEND_CONTACT_POINT_PROCESS
		};

	public:

		HK_DECLARE_REFLECTION();

		HK_DECLARE_CLASS_ALLOCATOR(HK_MEMORY_CLASS_ENTITY);

			// Destructor.
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [this,HK_ACCESS_RW] );
		virtual ~hkpEntity();


			//
			// Event Handling
			//

			/// Adds an entity listener to the entity.
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [this,HK_ACCESS_RW] );
		void addEntityListener( hkpEntityListener* el);

			/// Removes an entity listener from the entity.
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [this,HK_ACCESS_RW] );
		void removeEntityListener( hkpEntityListener* el);

			/// Adds an entity activation listener to the entity.
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [this,HK_ACCESS_RW] );
		void addEntityActivationListener( hkpEntityActivationListener* el);

			/// Removes an entity activation listener from the entity.
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [this,HK_ACCESS_RW] );
		void removeEntityActivationListener( hkpEntityActivationListener* el);

			/// Adds a collision listener to the entity.
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [this,HK_ACCESS_RW] );
		void addCollisionListener( hkpCollisionListener* cl );

			/// Removes a collision listener from the entity.
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [this,HK_ACCESS_RW] );
		void removeCollisionListener( hkpCollisionListener* cl);

			/// Get const access to the array of entity listeners.
		inline const hkSmallArray<hkpEntityListener*>& getEntityListeners() const;

			/// Get const access to the array of entity activation listeners.
		inline const hkSmallArray<hkpEntityActivationListener*>& getEntityActivationListeners() const;
		
			/// Get const access to the array of entity listeners.
		inline const hkSmallArray<hkpCollisionListener*>& getCollisionListeners() const;

			/// Gets the process contact callback delay.
		inline hkUint16 getProcessContactCallbackDelay() const;

			/// Sets the process contact callback delay.
			/// This value is used to determine how often a callback is raised for the
			/// "process contact" collision event.
			/// A value of 0 means the callback is called every step, whereas a value of 4 means 
			/// that a callback is raised every 5th step.
			/// Process contact callbacks can be used to change properties of contact points, such
			/// as the friction of a sliding contact.
		inline void setProcessContactCallbackDelay( hkUint16 delay );


			//
			// Utility functions
			//

			/// Gets the material used by this entity.
			/// If the entity has no collision detection representation, 
			/// the material is not used.
		inline hkpMaterial& getMaterial();

			/// Gets the material used by this entity.
			/// If the entity has no collision detection representation, 
			/// the material is not used.
		inline const hkpMaterial& getMaterial() const;

			/// A utility function to determine if the entity is fixed.
		inline hkBool isFixed() const;

			/// Checks whether the body's velocity cannot be influenced by physics directly.
			/// Uses a cached variable to avoid referencing hkpMotion object.
		inline hkBool isFixedOrKeyframed() const;

			/// Get's the entity's unique id. The uid is assigned in the entity's constructor and 
			/// is also updated when your deserialize objects.
		inline hkUint32 getUid() const;

			/// Find the contact manager between 'this' and the supplied entity.
			///
			/// Returns HK_NULL if no contact manager exists between 'this' and the supplied entity.
		hkpDynamicsContactMgr* findContactMgrTo(const hkpEntity* entity) const;


			//
			// Deactivation
			//

			/// Activates the specified entity and its island.
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [this,HK_ACCESS_RW] );
		void activate();

			/// Deactivates the specified entity and its island.
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_RW] [this,HK_ACCESS_RW] );
		void deactivate();

			/// Activates the specified entity and its island. Uses postponed operations queue if the world is locked for critical operations.
		void activateAsCriticalOperation();

			/// Deactivates the specified entity and its island. Uses postponed operations queue if the world is locked for critical operations.
		void deactivateAsCriticalOperation();

			/// Returns whether the entity is active. This method returns false if the entity
			/// has not yet been added to a hkpWorld object.
		hkBool isActive() const;


			//
			// Attached action and constraint accessors
			//

			/// Get the number of actions added to the world which reference this entity
		inline int getNumActions() const;

			/// Get the ith action added to the world which references this entity
		inline hkpAction* getAction(int i);

			///	Returns the number of constraints attached to this entity
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [m_simulationIsland,HK_ACCESS_RO] );
		int getNumConstraints() const;

			/// Returns the ith constraint attached to this entity
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [m_simulationIsland,HK_ACCESS_RW] );
		hkpConstraintInstance* getConstraint( int i );

			/// Returns the ith constraint attached to this entity (const version)
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [m_simulationIsland,HK_ACCESS_RO] );
		const hkpConstraintInstance* getConstraint( int i ) const;

		/// Returns read only access to the internal constraint master list
		inline const hkSmallArray<struct hkConstraintInternal>&  getConstraintMasters() const;

		/// Returns read write access to the internal constraint master list
		inline hkSmallArray<struct hkConstraintInternal>&  getConstraintMastersRw();

		/// Returns read only access to the internal constraint master list
		inline const hkArray<class hkpConstraintInstance*>&  getConstraintSlaves() const;

			// Calculate the memory usage of this entity
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [this,HK_ACCESS_RO] );
		void calcStatistics( hkStatisticsCollector* collector) const;

			/// Initialize cached AABB memory and SPU data (if available).
		void setCachedShapeData(const hkpWorld* world, const hkpShape* shape);

			/// Recalculate the cached AABB.
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [this,HK_ACCESS_RW] );
		void updateCachedAabb();

#if defined (HK_PLATFORM_HAS_SPU)

		void setShapeSizeForSpu(const hkpWorld* world, const hkpShape* shape);

			/// If you want to receive spu collision callback events on the ppu, you can use this utility to forward them from spu to ppu.
			///
			/// You need to set this utility in at least one of the two colliding
			/// entities. Each utility will receive each event only once, i.e. if two colliding entities share the same utility,
			/// you will only get the event once, whereas two different utilities will both receive this event individually.
			/// Use 'eventFilter' to filter the events sent from spu to ppu.
			/// Events will only be sent for entities whose 'userFilter' share at least one matching bit.
		void setSpuCollisionCallbackUtil(hkSpuCollisionCallbackUtil* util, SpuCollisionCallbackEventFilter eventFilter = SPU_SEND_CONTACT_POINT_ADDED_OR_PROCESS, hkUint8 userFilter = 0x01);

#endif

	protected:

			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [m_simulationIsland,HK_ACCESS_RO] );
		const	hkSmallArray<struct hkConstraintInternal>&	getConstraintMastersImpl() const;
					/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [m_simulationIsland,HK_ACCESS_RW] );
				hkSmallArray<struct hkConstraintInternal>&	getConstraintMastersRwImpl();

			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [m_simulationIsland,HK_ACCESS_RO] );
		const	hkArray<class hkpConstraintInstance*>&	getConstraintSlavesImpl() const;

		hkpEntity( const hkpShape* shape );

	public:

			//
			// INTERNAL FUNCTIONS
			//

		hkpEntity( class hkFinishLoadedObjectFlag flag );

			// Simulation units use this interface.
		inline hkpMotion* getMotion();

			// Get the simulation island, is HK_NULL for entities not in simulation.
		inline hkpSimulationIsland* getSimulationIsland() const;
			
			// Deallocates internal arrays if size 0.
			// Called internal by hkpWorld::removeEntity. Over
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [this,HK_ACCESS_RW] );
		virtual void deallocateInternalArrays();

		virtual hkMotionState* getMotionState(){ return HK_NULL; }



		//
		// MEMBERS
		//

	protected:

			// The entity's material, only used if the collision detection is enabled.
		class hkpMaterial m_material;

	protected:

		friend class hkpWorldConstraintUtil;

	public:
		// this is just a quick workaround helper class for serialization

	public:
		class SmallArraySerializeOverrideType
		{
			public:
				HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_DYNAMICS, hkpEntity::SmallArraySerializeOverrideType );
				HK_DECLARE_REFLECTION();

				void* m_data;
				hkUint16 m_size;
				hkUint16 m_capacityAndFlags;
		};

	public:		void* m_breakOffPartsUtil; //+nosave


		// the next three elements store constraint information (note: they are owned by the simulation island
		// offset into the accumulators
	public:		hkUint32									m_solverData;					//+nosave
	public: 	hkObjectIndex								m_storageIndex;					//+overridetype(hkUint16)
	protected:	hkUint16									m_processContactCallbackDelay;
	protected:	hkSmallArray<struct hkConstraintInternal>	m_constraintsMaster;			//+overridetype(class SmallArraySerializeOverrideType) +nosave
	protected:	hkArray<hkpConstraintInstance*>				m_constraintsSlave;				//+nosave

			// ------------------ 2nd CacheLine128 (rarely accessed data ) -------------------------

	protected:

		hkArray<hkUint8> m_constraintRuntime; //+nosave

		// The entity's simulation island.
		hkpSimulationIsland* m_simulationIsland; //+nosave

	public:

			/// See: hkpRigidBodyCinfo::m_autoRemoveLevel
		hkInt8  m_autoRemoveLevel;

			/// See: hkpRigidBodyCinfo::m_numUserDatasInContactPointProperties
		hkUint8 m_numUserDatasInContactPointProperties;

			// hkpWorld-unique Id
		hkUint32 m_uid; //+default(0xffffffff)

	public:

		struct SpuCollisionCallback
		{
		public:
			HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_DYNAMICS, hkpEntity::SpuCollisionCallback );
			HK_DECLARE_REFLECTION();
			SpuCollisionCallback():
				m_util(HK_NULL),
				m_capacity(0),
				m_eventFilter(SPU_SEND_CONTACT_POINT_ADDED_OR_PROCESS),
				m_userFilter(0x01)
			{
			}

			// only entities with a callback util will send events from spu to ppu; each event will only be fired once for each util
			hkSpuCollisionCallbackUtil* m_util; //+nosave

			// the maximum buffer size (counted in 16byte blocks) for events to be sent from spu to ppu; this value is set by setSpuCollisionCallbackUtil()
			hkUint16 m_capacity; //+nosave

			// used to filter what events to send from spu to ppu
			hkUint8 m_eventFilter;

			// free to be set by the user; note that events will only be sent from spu to ppu for entities whose userFilter both have at least one matching bit set
			hkUint8	m_userFilter;

		};
		
			// this class was created to keep the entity size <= 512
		struct ExtendedListeners
		{
			HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_DYNAMICS, hkpEntity::ExtendedListeners );
			HK_DECLARE_REFLECTION();

			hkSmallArray<hkpEntityActivationListener*>	m_activationListeners;	//+overridetype(class SmallArraySerializeOverrideType) +nosave
			hkSmallArray<hkpEntityListener*>				m_entityListeners;		//+overridetype(class SmallArraySerializeOverrideType) +nosave
		};
	public:

		struct SpuCollisionCallback m_spuCollisionCallback;

	protected:	mutable ExtendedListeners* m_extendedListeners;	//+nosave

			// The motion of the object
	public:		class hkpMaxSizeMotion m_motion;

	protected:

			//
			//	Rarely used members
			//
		friend class hkpEntityCallbackUtil;
		friend class hkpWorldCallbackUtil;
		friend class hkpWorld;
		friend class hkpSimulationIsland;
		friend class hkpWorldOperationUtil;

		hkSmallArray<hkpCollisionListener*>			m_collisionListeners;	//+overridetype(class SmallArraySerializeOverrideType) +nosave
		hkSmallArray<hkpAction*>						m_actions;				//+overridetype(class SmallArraySerializeOverrideType) +nosave
};

#include <Physics/Dynamics/Entity/hkpEntity.inl>

#endif // HK_DYNAMICS2_ENTITY_H

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

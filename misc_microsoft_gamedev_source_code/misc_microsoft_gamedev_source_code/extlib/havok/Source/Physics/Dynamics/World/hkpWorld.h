/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_DYNAMICS2_WORLD_H
#define HK_DYNAMICS2_WORLD_H

#include <Common/Base/hkBase.h>

#include <Common/Base/Config/hkConfigVersion.h>
#include <Common/Base/DebugUtil/MultiThreadCheck/hkMultiThreadCheck.h>
#include <Common/Base/Types/Physics/hkStepInfo.h>
#include <Physics/ConstraintSolver/Solve/hkpSolverInfo.h>
#include <Physics/Dynamics/World/hkpWorldCinfo.h>
#include <Physics/Dynamics/World/Simulation/hkpSimulation.h>
#include <Physics/Internal/Collide/BroadPhase/hkpBroadPhase.h>

class hkpCollisionFilter;
class hkpConvexListFilter;
class hkpCollisionDispatcher;
class hkpCollidable;
struct hkpCollisionInput;
struct hkpProcessCollisionInput;
class hkpCollisionListener;
class hkpTypedBroadPhaseDispatcher;

struct hkpWorldRayCastInput;
struct hkpWorldRayCastOutput;
struct hkpLinearCastInput;
class hkpRayHitCollector;
class hkpCdPointCollector;
class hkpCdBodyPairCollector;

class hkpBroadPhase;
class hkAabb;

class hkpEntity;
class hkpEntityListener;

class hkpRigidBody;

class hkpPhantom;
class hkpPhantomListener;
class hkpPhantomBroadPhaseListener;
class hkpEntityEntityBroadPhaseListener;
class hkpBroadPhaseBorderListener;

class hkpAction;
class hkpActionListener;

class hkpConstraintInstance;
class hkpConstraintData;
class hkpConstraintListener;

class hkpPhysicsSystem;

class hkpSimulationIsland;
class hkpContactMgrFactory;

struct hkpMultithreadedSimulationJobData;

class hkpWorldPostSimulationListener;
class hkpWorldPostIntegrateListener;
class hkpWorldPostCollideListener;
class hkpWorldDeletionListener;
class hkpIslandActivationListener;
class hkpContactImpulseLimitBreachedListener;
class hkpIslandPostIntegrateListener;
class hkpIslandPostCollideListener;

class hkStepInfo;
class hkWorldPairwiseBackstepFilter;

class hkpSimulation;
class hkpWorldOperationQueue;
struct hkpDebugInfoOnPendingOperationQueues;
class hkpWorldMaintenanceMgr;

class hkWorldMemoryAvailableWatchDog;



namespace hkWorldOperation { struct BaseOperation; class UserCallback; }

struct hkOperationParameter;

#if defined HK_DEBUG
//# define HK_ENABLE_EXTENSIVE_WORLD_CHECKING
#endif


	///	Allows you optimize the  hkpWorld::updateCollisionFilterOnWorld call
enum hkpUpdateCollisionFilterOnWorldMode
{
		/// Full filter filter
	HK_UPDATE_FILTER_ON_WORLD_FULL_CHECK,

		/// Recheck filter but only check for disabled entity-entity collisions
		/// which have been enabled before
	HK_UPDATE_FILTER_ON_WORLD_DISABLE_ENTITY_ENTITY_COLLISIONS_ONLY,
};

	/// Allows you to optimize you hkpWorld::updateCollisionFilterOnEntity call
enum hkpUpdateCollisionFilterOnEntityMode
{
		/// Do a full check
	HK_UPDATE_FILTER_ON_ENTITY_FULL_CHECK,

		/// Recheck filter but only check for disabled entity-entity collisions
		/// which have been enabled before
	HK_UPDATE_FILTER_ON_ENTITY_DISABLE_ENTITY_ENTITY_COLLISIONS_ONLY
};

	/// Activation hints when calling hkpWorld::addEntity
enum hkpEntityActivation
{
		/// Tries to add the body in an inactive state. However if the body overlaps with an
		/// active body, it gets activated
	HK_ENTITY_ACTIVATION_DO_NOT_ACTIVATE,

		/// Always activate this body and overlapping bodies
	HK_ENTITY_ACTIVATION_DO_ACTIVATE
};

	/// The hkUpdateCollectionFilter specifies whether you want to reevaluate
	/// all the sub-shape collisions with the world's shape collection filter. Typical example of this is
	/// destructible terrain. So if you just disabled the collision of a bullet and a car, you do not
	/// need to recheck existing landscape collisions and can set
	/// updateShapeCollectionFilter to HK_UPDATE_COLLECTION_FILTER_IGNORE_SHAPE_COLLECTIONS
enum hkpUpdateCollectionFilterMode
{
		/// Assume that no single shapes in a shape collections changed their filter status
	HK_UPDATE_COLLECTION_FILTER_IGNORE_SHAPE_COLLECTIONS,

		/// Recheck all subshapes in a shape collection
	HK_UPDATE_COLLECTION_FILTER_PROCESS_SHAPE_COLLECTIONS
};



	/// A structure used by the hkpWorld that groups together solver and step time information.
struct hkpWorldDynamicsStepInfo
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_DYNAMICS, hkpWorldDynamicsStepInfo );

	hkStepInfo   m_stepInfo;
	hkpSolverInfo m_solverInfo;
};

	/// This structure is currently only used for multi threaded simulations. It controls how the multi threading is run.
struct hkpMultithreadConfig
{
	HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_DYNAMICS, hkpMultithreadConfig );

	hkpMultithreadConfig();

		/// PLAYSTATION(R)3 only. A flag used to determine if the PPU can take SPU tasks.
	enum CanCpuTakeSpuTasks
	{
			/// If there are only SPU jobs on the queue and no SPU is free to take one, the
			/// PPU is allowed to take the job.
		CPU_CAN_TAKE_SPU_TASKS,

			/// The PPU will never take SPU jobs, even if there are only SPU jobs on the queue.
			/// This is the default setting.
		CPU_CAN_NOT_TAKE_SPU_TASKS
	};

#if defined(HK_PLATFORM_HAS_SPU)
		/// This variable is used to set whether the cpu can take spu tasks. It defaults to
		/// true. WARNING: If you set this to false, and do not have the correct SPU program
		/// running on at least one SPU during the step() call, the step function will stall indefinitely.
	CanCpuTakeSpuTasks m_canCpuTakeSpuTasks;
#endif
		/// This variable is used to control when the constraints of a simulation island are solved in
		/// single threaded or multithreaded mode. On PC and XBOX360 this value is 70, which means that
		/// an island must have more than 70 constraints to be solved in multithreaded mode. If it
		/// is set to 0 then the constraints of all simulation islands will be solved in multithreaded mode.
		/// The reason it is not set to 0 by default is because there is a small overhead in setting up
		/// multithreading constraints for a simulation island, which can be significant for small islands.
		/// On the PS3 this variable defaults to 4.  SPU constraint setup only happens if the island
		/// has more constraints than are specified by this variable.
	hkUint32 m_maxNumConstraintsSolvedSingleThreaded;
};

	/// A token used to indicate which thread number
enum hkpThreadToken
{
		/// Primary thread
	HK_THREAD_TOKEN_FIRST,
	/// Secondary thread
	HK_THREAD_TOKEN_SECOND,

	HK_THREAD_TOKEN_THIRD,
	HK_THREAD_TOKEN_FORTH,
	HK_THREAD_TOKEN_FIFTH,
	HK_THREAD_TOKEN_SIXTH,

};


	/// The hkpWorld is a container for the simulation's physical objects.  It also steps the simulation forward in time.
	/// You add elements (including rigid bodies, actions, constraints, and listeners) to the simulation by adding them to the hkpWorld.
class hkpWorld : public hkReferencedObject
{
	public:

		//+vtable(true)
		//+serializable(false)
		HK_DECLARE_CLASS_ALLOCATOR(HK_MEMORY_CLASS_WORLD);
        HK_DECLARE_REFLECTION();

			/// Creates a world with the given construction information
		hkpWorld(const hkpWorldCinfo& info, unsigned int sdkversion = HAVOK_SDK_VERSION_NUMBER );

			/// Get construction info for this instance.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		void getCinfo(hkpWorldCinfo& info) const;


			//
			//	hkpWorld Operations:
			//
			//  Adding/Removing Instances/BatchesOfInstances of Entities/Constraints/Actions/Phantoms
			//

			/// Adds the specified entity to the world and returns its pointer
			/// or returns HK_NULL if adding the body is delayed.
			/// Lets you specify the desired activation state.
			/// Usage info: Note that even if you add a body inactive, it may be immediately automatically activated
			///             by the engine due to overlaps with other bodies.
			/// Note: When using HK_ENTITY_ACTIVATION_DO_ACTIVATE parameter, the entity is added in the active state,
			///       however no activationCallbacks are called.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		hkpEntity* addEntity( hkpEntity* entity, hkpEntityActivation initialActivationState = HK_ENTITY_ACTIVATION_DO_ACTIVATE);

			/// Removes the specified entity from the world
			/// During a simulation step the removal of the entity is delayed until it is safe to do so.
			/// returns true if removed immediately false if delayed.
		hkBool removeEntity( hkpEntity* entity);


			/// Efficiently add a group of entities at the same time.  Will not be as efficient when
			/// adding a small number of entities, in that case use addEntity.
			/// Lets you specify the desired activation state.
			/// Warning: When adding entities inactive: all entities are added in the same island,
			///          therefore any overlap with an active entity (already inserted into the world)
			///          causes activation of the whole island.
			/// Note: When using HK_ENTITY_ACTIVATION_DO_ACTIVATE parameter, the entity is added in the active state,
			///       however no activationCallbacks are called.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void addEntityBatch( hkpEntity*const* entityBatch, int numEntities, hkpEntityActivation initialActivationState = HK_ENTITY_ACTIVATION_DO_ACTIVATE);

			/// Efficiently remove a group of entities at the same time.  Will not be as efficient when
			/// removing a small number of entities, in that case use removeEntity.
		void removeEntityBatch( hkpEntity*const* entityBatch, int numEntities );


			//
			//	Constraint addition and removal
			//

			/// Adds the specified constraint to the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		hkpConstraintInstance* addConstraint( hkpConstraintInstance* constraint);

			/// Removes the specified constraint from the world
			/// returns true if removed immediately false if delayed
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		hkBool removeConstraint( hkpConstraintInstance* constraint);

			/// Utility function for creating and adding a specified constraint instance to the world
		hkpConstraintInstance* createAndAddConstraintInstance( hkpRigidBody* bodyA, hkpRigidBody* bodyB, hkpConstraintData* constraintData );



			//
			// Actions
			//

			/// Adds the specified action to the world. A reference to the action is added to every entity on the action's list
			/// of entities. All those entities must already by placed in an hkpWorld.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		hkpAction* addAction( hkpAction* action );

			/// Remove the specified action from the world and removes its reference from each entity that it was connected to.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void removeAction( hkpAction* action );

			/// Remove the specified action from the world and removes its reference from each entity that it was connected to.
			/// It bypasses the operation delay framework, there fore you _MUST_BE_SURE_ that _IT_IS_SAFE_ to use it.
			/// Info: use this only in hkpAction::entityRemovedCallback
			/// Non-symmetric function. Only needed to be used explicitly when removing an entity causes removal of an action in a callback.
			/// addActionImmediately not needed.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void removeActionImmediately( hkpAction* action );


			//
			// Phantoms
			//

			/// Add the phantom to the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		hkpPhantom* addPhantom( hkpPhantom* phantom );

			/// Add multiple phantoms to the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void addPhantomBatch( hkpPhantom*const* phantomBatch, int numPhantoms );


			/// Remove the phantom from the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void removePhantom( hkpPhantom* phantom );

			/// Remove multiple phantoms from the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void removePhantomBatch( hkpPhantom*const* phantomBatch, int numPhantoms );

			/// Activates all entities whose aabb overlaps with the specified region
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void activateRegion( const hkAabb& aabb );


			//
			// Systems (bodies, actions and phantoms all together). For instance
			// a vehicle or a ragdoll.

			/// Add a hkpPhysicsSystem to the world. The world does not
			/// store or reference the system struct itself, just
			/// uses the data held to batch add to the world.
		void addPhysicsSystem( const hkpPhysicsSystem* sys );

			/// Remove a hkpPhysicsSystem from the world.
			/// The world will just look through the lists in the given system
			/// and remove all found. It does not alter the system struct itself.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void removePhysicsSystem( const hkpPhysicsSystem* sys );

			//
			// Fixed rigid body
			//


			/// Gets the world's fixed rigid body. This is a fixed body that can be used to constrain an object to
			/// a position in world space without needing to create an additional fixed object. The world's fixed
			/// body differs from 'normal' rigid bodies in a number of ways, including but not limited to the
			/// following:<br>
			/// It doesn't have a shape/collision representation and so it never collides with anything.<br>
			/// It doesn't have a broadphase entry and so it cannot go outside the broadphase, resulting in a
			/// potential performance hit as 'normal' rigid bodies can.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		inline hkpRigidBody* getFixedRigidBody();

		/// Gets read access to the world's fixed rigid body.
		/// See the non-const version of this function for more information.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		inline const hkpRigidBody* getFixedRigidBody() const;

			//
			//  Collision filter update methods
			//

			/// This method should be called if you have altered the collision filtering information for this entity.
			/// Note: this does not activate the entity.
			/// The collisionFilterUpdateMode allows you to do an optimization: Only previously enabled entity entity collisions are
			/// checked. However entity-phantom or new entity-entity collision are ignored.
			/// The updateShapeCollectionFilter specifies whether you want to reevaluate
			/// all the sub-shape collisions with the world's shape collection filter. Typical example of this is
			/// destructible terrain. So if you just disabled the collision of a bullet and a car, you do not
			/// need to recheck existing landscape collisions and can set
			/// updateShapeCollectionFilter to HK_UPDATE_COLLECTION_FILTER_IGNORE_SHAPE_COLLECTIONS
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] [entity,HK_ACCESS_RW] );
		void updateCollisionFilterOnEntity( hkpEntity* entity,
			                                hkpUpdateCollisionFilterOnEntityMode updateMode,
											hkpUpdateCollectionFilterMode updateShapeCollectionFilter);

			/// This method should be called if you have altered the collision filtering information for this phantom. Read updateCollisionFilterOnEntity() for details
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] [phantom,HK_ACCESS_RW] );
		void updateCollisionFilterOnPhantom( hkpPhantom* phantom,
											 hkpUpdateCollectionFilterMode updateShapeCollectionFilter );

			/// This method should be called if you have changed the collision filter for the world.
			///
			/// WARNING: This method is very slow. It involves re-evaluating all the broad phase aabbs in the system.
			/// You should only ever call this at startup time.
			/// updateMode allows you to use a shortcut if you only want to disable previously enabled entity entity
			/// collisions.
			/// The updateShapeCollectionFilter specifies whether you want to reevaluate
			/// all the sub-shape collisions with the world's shape collection filter. Typical example of this is
			/// destructible terrain. So if you just disabled the collision of a bullet and a car, you do not
			/// need to recheck existing landscape collisions and can set
			/// updateShapeCollectionFilter to HK_UPDATE_COLLECTION_FILTER_IGNORE_SHAPE_COLLECTIONS
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void updateCollisionFilterOnWorld( hkpUpdateCollisionFilterOnWorldMode updateMode,
										   hkpUpdateCollectionFilterMode updateShapeCollectionFilter );

		enum ReintegrationRecollideMode
		{
			RR_MODE_REINTEGRATE = 1,			// only reintegrates the motion in the middle of a physics step
			RR_MODE_RECOLLIDE_BROADPHASE = 2,	// only reapplies the broadphase
			RR_MODE_RECOLLIDE_NARROWPHASE = 4,	// only reapplies the narrowphase
			RR_MODE_ALL = 7
		};

			/// Reintegrates bodies immediately.
			/// This function allows for calling parts of the physics pipeline selectively
			/// in single rigid bodies. This is especially useful when you
			/// think the normal pipeline isn't good enough.
			/// Examples:
			///		- When physics and frame rate run at different rates (especially when the physics
			///       is run at a much lower rate), setting velocity of a rigid body will have no effect
			///       till the next physics step. This can lead to very small
			///       delays between a player action and the expected reaction. With this function
			///       you can force the reintegration of a set of entities at the current frame time (mode = RR_MODE_ALL)
			///     - When you add an object to the world, it will pick up contact information only at the
			///       end of the next physics step, so there are no contacts for the next frame. Calling this
			///       function immediately after adding the object with mode = RR_MODE_RECOLLIDE_NARROWPHASE finds the initial set of contacts
		void reintegrateAndRecollideEntities( hkpEntity** entityBatch, int numEntities, ReintegrationRecollideMode mode = RR_MODE_ALL );


			//
			// Phantoms
			//


			/// Get a list of all the phantoms which have been added to the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		inline const hkArray<hkpPhantom*>& getPhantoms() const;


			//
			// Gravity
			//

			/// Sets the gravity value for the world. This does not wake up objects
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void setGravity( const hkVector4& gravity );

			/// Gets the world's gravity value.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		inline const hkVector4& getGravity() const;


			//
			// Simulation Control
			//


			/// This utility function may be called to initialize the contact points for a set of entities added to the world.
			/// By default contact points are evaluated conservatively, for simulation efficiency.
			/// However this can lead to a slight initial jitter when objects are added.  The same applies to objects for
			/// objects which have had their collision agents deleted upon deactivation when they reactivate. (See
			/// the helper class hkpSuspendInactiveAgentsUtil for details. ) This function helps to remove this jitter by
			/// trying to find all the initial relevant contact points for a set of entities.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void findInitialContactPoints( hkpEntity** entities, int numEntities );

			/// This simply calls initializeContactPoints for all objects in the world. It is a utility function
			/// which can be called after the world has been initialized and all objects added to it.
		void findInitialContactPointsOfAllEntities( );



			/// Gets the memory usage required to complete the next integration step
			/// This function is called internally to prevent the memory usage breaching the critical memory limit in
			/// hkMemory.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		int getMemUsageForIntegration();

			/// Calculate the memory statistics for the world, and all objects owned by the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		void calcStatistics( hkStatisticsCollector* collector ) const;

			/// checks the current state of the engine using the hkCheckDeterminismUtil
			/// ###ACCESS_CHECKS###( [island->m_world,HK_ACCESS_RO] [island,HK_ACCESS_RW] );
		void checkDeterminism();


			/// Get the memory "watch dog" used by the world. By default this is HK_NULL, i.e. the memory
			/// for the world is not checked. If this is set, hkpWorldMemoryWatchDog::watchMemory will be
			/// called every step, which gives the hkpWorldMemoryWatchDog a chance to prevent the memory
			/// exceeding a desired threshold.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		hkWorldMemoryAvailableWatchDog* getMemoryWatchDog( ) const;

			/// Set the memory "watch dog" used by the world. By default this is HK_NULL, i.e. the memory
			/// for the world is not checked. If this is set, hkpWorldMemoryWatchDog::watchMemory will be
			/// called every step, which gives the hkpWorldMemoryWatchDog a chance to prevent the memory
			/// exceeding a desired threshold.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void setMemoryWatchDog( hkWorldMemoryAvailableWatchDog* watchDog );

			//
			// Multithreaded Access
			//

			/// Locks the world in a multithreaded environment.
			/// This can only be called outside any step function and should
			/// only be used for asynchronous queries like raycast, etc.
			/// lock() and unlock() acquire and release critical sections, so they are quite slow.
			/// For this reason it is best to lock the world and do many operations on the world when you have
			/// acquired the lock before unlocking it.
			/// If you use your own synchronization mechanisms you don't need to call our implementation of lock / unlock,
			/// but you must ensure multiple threads don't access the world simultaneously.
			/// When using your own synchronization mechanisms you should also call markForRead or markForWrite
			/// to let our debugging utilities know that the world has been locked appropriately.
			/// Read the multithreading user guide about details
		void lock();

			/// Locks the world as read-only in a multithreaded environment.
			/// Needed when executing collision query jobs.
			/// See also lock().
		void lockReadOnly();

			/// Unlocks the world. Opposite of lock()
		void unlock();

			/// Unlocks the world. Opposite of lockReadOnly()
		void unlockReadOnly();

			/// Returns true if the world was locked by any thread. Returns false in single threaded mode.
		bool isLocked();


		// Internal function: Locks all entities in the simulation island for constraint updates
		void lockIslandForConstraintUpdate( hkpSimulationIsland* island );

// BEGIN MOD - HVK-4616
		// Internal function: Locks all entities in this simluation island and in the fixed island for constraint updates
		void lockForIslandSplit( hkpSimulationIsland* island );
// END MOD - HVK-4616

		// Internal function: opposite of lockIslandForConstraintUpdate
		void unlockIslandForConstraintUpdate( hkpSimulationIsland* island );

// BEGIN MOD - HVK-4616
		// Internal function: opposite of lockForIslandSplit
		void unlockForIslandSplit( hkpSimulationIsland* island );
// END MOD - HVK-4616

			//
			// Multithreaded Access checks
			//

		enum MtAccessChecking
		{
			MT_ACCESS_CHECKING_ENABLED = 0,
			MT_ACCESS_CHECKING_DISABLED
		};

			/// Enable or disable multithreaded access checking
			/// By default this is enabled if the world simulation type is SIMULATION_TYPE_MULTITHREADED
			/// and false is it is anything else.
		inline void setMultithreadedAccessChecking( MtAccessChecking accessCheckState );

			/// Get whether multithreaded access checking is enabled
			/// By default this is enabled if the world simulation type is SIMULATION_TYPE_MULTITHREADED
			/// and false is it is anything else.
		inline MtAccessChecking getMultithreadedAccessChecking() const;

			/// Mark this class and all child classes for read only access for this thread
			/// Note: This is only for debugging and does not wait to get exclusive access,
			/// but simply assert if another thread marked the hkpWorld. You must read the
			/// user guide about multithreading to use this.
		inline void markForRead( ) const;

			/// Mark this class and all child classes for read write access for this thread
			/// Note: This is only for debugging and does not wait to get exclusive access,
			/// but simply assert if another thread marked the hkpWorld. You must read the
			/// user guide about multithreading to use this.
		inline void markForWrite( );

			/// Undo lockForRead
			/// Note: This is only for debugging and does not wait to get exclusive access,
			/// but simply assert if another thread locked the hkpWorld. You must read the
			/// user guide about multithreading to use this.
		inline void unmarkForRead( ) const;

			/// Unlock For write
			/// Note: This is only for debugging and does not wait to get exclusive access,
			/// but simply assert if another thread locked the hkpWorld. You must read the
			/// user guide about multithreading to use this.
		inline void unmarkForWrite();


			//
			// Event Handling
			//

			/// Adds the specified listener to the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void addActionListener( hkpActionListener* worldListener );

			/// Removes the specified listener from the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void removeActionListener( hkpActionListener* worldListener );

			/// Adds the specified listener to the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void addConstraintListener( hkpConstraintListener* worldListener );

			/// Removes the specified listener from the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void removeConstraintListener( hkpConstraintListener* worldListener );

			/// Adds the specified listener to the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void addEntityListener( hkpEntityListener* worldListener );

			/// Removes the specified listener from the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void removeEntityListener( hkpEntityListener* worldListener );


			/// Adds the specified listener to the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void addPhantomListener( hkpPhantomListener* worldListener );

			/// Removes the specified listener from the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void removePhantomListener( hkpPhantomListener* worldListener );


			/// Adds a listener which listens to ALL entity activations.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void addIslandActivationListener( hkpIslandActivationListener* worldListener );

			/// Removes the specified listener from the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void removeIslandActivationListener( hkpIslandActivationListener* worldListener );

			/// Adds the specified listener to the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void addWorldPostCollideListener( hkpWorldPostCollideListener* worldListener );

			/// Removes the specified listener from the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void removeWorldPostCollideListener( hkpWorldPostCollideListener* worldListener );

			/// Adds the specified listener to the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void addWorldPostSimulationListener( hkpWorldPostSimulationListener* worldListener );

			/// Removes the specified listener from the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void removeWorldPostSimulationListener( hkpWorldPostSimulationListener* worldListener );

			/// Adds the specified listener to the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void addWorldPostIntegrateListener( hkpWorldPostIntegrateListener* worldListener );

			/// Removes the specified listener from the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void removeWorldPostIntegrateListener( hkpWorldPostIntegrateListener* worldListener );

			/// Adds the specified listener to the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void addIslandPostCollideListener( hkpIslandPostCollideListener* islandListener );

			/// Removes the specified listener from the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void removeIslandPostCollideListener( hkpIslandPostCollideListener* islandListener );

			/// Adds the specified listener to the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void addIslandPostIntegrateListener( hkpIslandPostIntegrateListener* islandListener );

			/// Removes the specified listener from the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void removeIslandPostIntegrateListener( hkpIslandPostIntegrateListener* islandListener );

			/// Adds a listener which listens to ALL collisions.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void addCollisionListener( hkpCollisionListener* worldListener );

			/// Removes a world collision listener.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void removeCollisionListener( hkpCollisionListener* worldListener);

			/// Add a listener which receives events when the world is deleted
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void addWorldDeletionListener( hkpWorldDeletionListener* worldListener );

			/// Remove a listener which receives events when the world is deleted
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void removeWorldDeletionListener( hkpWorldDeletionListener* worldListener );

			/// Add a listener which receives events when contact impulses breach their limit
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void addContactImpulseLimitBreachedListener( hkpContactImpulseLimitBreachedListener* listener );

			/// Remove a listener which receives events when contact impulses breach their limit
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void removeContactImpulseLimitBreachedListener( hkpContactImpulseLimitBreachedListener* listener );


			//
			// Collision detection
			//

			/// Gets write access the broad phase owned by the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		inline hkpBroadPhase* getBroadPhase();

			/// Gets read only access to the broad phase owned by the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		inline const hkpBroadPhase* getBroadPhase() const;

			/// Get read access to the collision input. This is needed to manually query collision agents.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		inline const hkpProcessCollisionInput* getCollisionInput() const;

			/// Get read/write access to the collision input. This is needed to manually query collision agents.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		inline hkpProcessCollisionInput* getCollisionInput();

			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		inline hkpSolverInfo* getSolverInfo();

			/// Gets the collision dispatcher owned by the world.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		inline hkpCollisionDispatcher* getCollisionDispatcher() const;

			/// Gets the collision filter.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		inline const hkpCollisionFilter* getCollisionFilter() const;

			/// Set the collision filter. This is used by the world by default when it calls the collision detector.
			/// WARNING: This should only be done before any objects are added to this world.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void setCollisionFilter( hkpCollisionFilter* filter,
								 hkBool             runUpdateCollisionFilterOnWorld = true,
								 hkpUpdateCollisionFilterOnWorldMode checkBroadPhaseMode = HK_UPDATE_FILTER_ON_WORLD_FULL_CHECK,
								 hkpUpdateCollectionFilterMode       updateShapeCollectionFilter = HK_UPDATE_COLLECTION_FILTER_PROCESS_SHAPE_COLLECTIONS );

			/// Cast a ray into the world and get the closest hit.
			/// Note: it uses the hkpWorld::getCollisionFilter() for filtering
			/// This function uses a ray-cast function for traversing the broad phase, so for
			/// long raycasts it is more applicable than hkpAabbPhantom::castRay
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		void castRay(const hkpWorldRayCastInput& input, hkpWorldRayCastOutput& output ) const;

			/// Cast a ray into the world and do a callback for every hit.
			/// Note: it uses the hkpWorld::getCollisionFilter() for filtering
			/// This function uses a ray-cast function for traversing the broad phase, so for
			/// long raycasts it is more applicable than hkpAabbPhantom::castRay
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		void castRay(const hkpWorldRayCastInput& input, hkpRayHitCollector& collector ) const;


			/// Cast a shape within the world.
			/// The castCollector collects all potential hits.<br>
			/// Note that the distance stored within the castCollector is a hitFraction (between 0 and 1.0) and
			/// not a distance.<br>
			/// This function uses an aabb-cast function for traversing the broad phase, so for
			/// long linear casts it will perform better than hkpShapePhantom::linearCast().
			/// The [optional] startPointCollector returns all the closest points
			/// at the start point of the linear cast. If you do not want this functionality, pass HK_NULL as the
			/// "startCollector".
			/// If you want to use this function using an hkpBroadPhaseAabbCache simply use the underlying hkpWorldLinearCaster
			/// (see implementation of hkpWorld::linearCast for details)
			/// Note that shape radius is considered - you may want to set the radius of the cast shape to zero.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		void linearCast( const hkpCollidable* collA, const hkpLinearCastInput& input, hkpCdPointCollector& castCollector, hkpCdPointCollector* startCollector = HK_NULL ) const;

			/// Get the closest points to a hkpCollidable (= [shape,transform,filterInfo] ).
			/// Note: If you have to call this function every step for a given object, use the hkpShapePhantom::getClosestPoints()
			/// By default, you should pass a pointer to world->getCollisionInput() via hkpCollisionInput* input. However you
			/// can use this parameter to pass in your own collision input
			/// structure, and change the shape collection collision filter, and the collision tolerance used.
			/// (Note: If using your own collision input, make sure you start by copying the world collision input, so that
			/// you use the default hkpCollisionDispatcher).
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		void getClosestPoints( const hkpCollidable* collA, const hkpCollisionInput& input, hkpCdPointCollector& collector) const;

			/// Get all shapes which are penetrating the collidable.
			/// Note: If you have to call this function every step for a given object, use the hkpShapePhantom version
			/// By default, you should pass a pointer to world->getCollisionInput() via hkpCollisionInput* input. However you
			/// can use this parameter to pass in your own collision input
			/// structure, and change the shape collection collision filter, and the collision tolerance used.
			/// (Note: If using your own collision input, make sure you start by copying the world collision input, so that
			/// you use the default hkpCollisionDispatcher).
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		void getPenetrations( const hkpCollidable* collA, const hkpCollisionInput& input, hkpCdBodyPairCollector& collector) const;


			/// Gets the current broadphase border, could be null.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		class hkpBroadPhaseBorder* getBroadPhaseBorder() const;


			/// Sets a custom broadphase border.
			/// By default the hkpWorld constructor checks the
			/// hkpWorldCinfo.m_broadPhaseBorderBehaviour field and will create a default
			///	broadphase border if needed. Real hkpWorldCinfo.m_broadPhaseBorderBehaviour for details
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void setBroadPhaseBorder( hkpBroadPhaseBorder* bpb );

			/// See hkpBroadPhase::shiftBroadPhase() for details.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void shiftBroadPhase( const hkVector4& shiftDistance, hkVector4& effectiveShiftDistanceOut, hkArray<hkpBroadPhaseHandle*>& objectsEnteringBroadphaseBorder );

			//
			// Simulation Island access
			//

			/// Get the active simulation islands. You can use this function to access all the active entities, constraints and actions.
			/// NOTE: This should not be used during simulation, as it will generally result in sub-optimal performance.
		inline const hkArray<hkpSimulationIsland*>& getActiveSimulationIslands() const;

			/// Get the inactive simulation islands. You can use this function to access all the inactive entities, constraints and actions.
			/// NOTE: This should not be used during simulation, as it will generally result in sub-optimal performance.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		inline const hkArray<hkpSimulationIsland*>& getInactiveSimulationIslands() const;

			/// Get the fixed simulation island. You can use this function to access all the fixed entities.
			/// There is only one fixed island, and it contains no actions or constraints.
			/// NOTE: This should not be used during simulation, as it will generally result in sub-optimal performance.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		inline const hkpSimulationIsland* getFixedIsland() const;


			//
			// Stepping the world (Default)
			//


			/// Advances the state of the simulation by the amount of time.
			/// This function may also be called for a multithreaded simulation.  If you are stepping
			/// the world multithreaded you must call resetThreadTokens() in a single thread, then for
			/// each child thread you call this function. You must implement a barrier so that no
			/// child thread calls stepDeltaTime twice before resetThreadTokens is called again.
			/// Normally this returns HK_STEP_RESULT_SUCCES, but may return a failure code on memory failure.
			/// In this case memory should be freed (by removing objects from the world) and the function should be called again.
			/// There is also finer grained interface to multithreading simulation below.
		hkpStepResult stepDeltaTime( hkReal physicsDeltaTime );


			//
			// "Half" stepping (Single threaded only)
			//

			/// Integrate and solve constraints between all bodies.
			/// Using this call instead of stepDeltaTime() allows you to distribute your
			/// physics computation for a single physics step between multiple game steps.
			/// Normally this returns HK_STEP_RESULT_SUCCES, but may return a failure code on memory failure.
			/// In this case memory should be freed (by removing objects from the world) and the function should be called again.
			///	Please consult the user guide for more details.
		hkpStepResult integrate( hkReal physicsDeltaTime );

			/// Perform collision detection.
			/// Using this call instead of stepDeltaTime() allows you to distribute your
			/// physics computation for a single physics step between multiple game steps.
			/// Normally this returns HK_STEP_RESULT_SUCCES, but may return a failure code on memory failure.
			/// In this case memory should be freed (by removing objects from the world) and the function should be called again.
			///	Please consult the user guide for more details.
		hkpStepResult collide();

			/// Advance the current time of the world.
			/// For discrete simulation, this simply advances the current time value,
			/// For continuous simulations, this will also perform the continuous physics calculations.
			/// Using this call instead of stepDeltaTime() allows you to distribute your
			/// physics computation for a single physics step between multiple game steps.
			/// This call is also used for asynchronous simulation:
			/// It will advance the time to the next PSI step, unless
			/// an earlier frame timer marker has been set (see setFrameTimeMarker).
			/// Normally this returns HK_STEP_RESULT_SUCCES, but may return a failure code on memory failure.
			/// In this case memory should be freed (by removing objects from the world) and the function should be called again.
			///	Please consult the user guide for more details.
		hkpStepResult advanceTime();


			//
			// Asynchronous Stepping
			//

			/// This call can be used to step the world asynchronously.  If you set a marker, then
			/// when advanceTime is called, it will advance either to this marker, or the current
			/// psi time, which ever is earlier.  This is illustrated by
			/// hkAsynchronousTimestepper::stepAsynchronously. It can also be used in conjunction with
			/// multithreading simulation.
			/// Once you start using setFrameTimeMarker, you must continue to use it for all subsequent
			/// stepping.
			///	Please consult the user guide for more details.
		void setFrameTimeMarker( hkReal frameDeltaTime );

			/// This call can be used to step the world asynchronously. It returns true if the simulation
			/// time is equal to the frame marker time. If the frameTimeMarker has not been set this returns
			/// false.
			///	Please consult the user guide for more details.
		bool isSimulationAtMarker() const;

			/// This call returns true if the simulation current time is equal to the simulation currentPsiTime.
			/// It can be used as a debug check when stepping the world asynchronously.
			///	Please consult the user guide for more details.
		bool isSimulationAtPsi() const;

			//
			// Multithreaded stepping
			//

			/// Single threaded setup for multithreaded simulation.  Only one thread must call this function. When it
			/// returns all threads should call stepProcessMt.  No thread should call stepProcessMt until this function
			/// completes. This function leaves the world in a markForRead() state, for stepProcesMt().  This state is
			/// reset in stepEndSt().
		void stepBeginSt( hkReal physicsDeltaTime );

			/// Multithreaded simulation.  This processes the integration and collision detection for all simulation islands.
			/// It is should be callled by all threads, after one thread has called stepBeginSt.
		void stepProcessMt( const hkpThreadToken& token );

			/// Single threaded function to end a multithreaded step.  Only one thread must call this function. It must be
			/// called when all threads have returned from calling stepProcessMt.
		void stepEndSt();


			/// This call must be called single threaded prior to calling getThreadToken or stepDeltaTime(). If you
			/// are using the fine grained multithreaded simulation functions (stepBeginSt, stepProcessMt and stepEndSt)
			/// and have identified your own primary thread, and are not calling getThreadToken, you do not need to call
			/// this function.
		void resetThreadTokens();

			/// This returns HK_THREAD_TOKEN_PRIMARY for the first thread to call it, then HK_THREAD_TOKEN_SECONDARY
			/// for all subsequent threads.  When resetThreadTokens is called, the next thread token this returns
			/// will be HK_THREAD_TOKEN_PRIMARY.  This function is a utility function which does not need to be called
			/// if you have identified your own primary thread.
		hkpThreadToken getThreadToken();

			//
			// Time accessors
			//


			/// This returns current time of the simulation. Use it to calculate current transforms of rigid bodies
			/// in between PSI steps (with hkpRigidBody::approxTransformAt). Note that this is not absolute time; its value is reset every 30 seconds or so.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		inline hkTime getCurrentTime() const;

			/// This returns the current "PSI" time of the simulation.  The PSI time is always the same or greater than
			/// the current time.  See the user guide for details.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		inline hkTime getCurrentPsiTime() const;


			//
			// Multithreaded simulation details accessors
			//


			/// This function is only valid when using multithreaded simulation. If using other
			/// simulations, it returns HK_NULL.
		class hkJobQueue* getJobQueue();


			/// This function is only valid for multithreading simulation.  It allows you to access
			/// some settings used for PLAYSTATION(R)3 multithreading simulation.
		void getMultithreadConfig( hkpMultithreadConfig& config );

			/// This function is only valid for multithreading simulation. It allows you to control
			/// some settings used for PLAYSTATION(R)3 multithreading simulation.
		void setMultithreadConfig( const hkpMultithreadConfig& config );

		hkMultiThreadCheck& getMultiThreadCheck() const { return m_multiThreadCheck; }

			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		void checkAccessGetActiveSimulationIslands() const;

		//
		// Serialization Utilities
		//

			/// Get a hkpPhysicsSystem that represents the whole system
			/// contained in this world. This can be used as a serialization utility
			/// for taking a snapshot of the world.
			/// Release the physics system when you are finished using it.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		hkpPhysicsSystem* getWorldAsOneSystem() const;

			/// Get an array of hkpPhysicsSystem(s) that represents the whole system
			/// contained in this world. This can be used as a serialization utility
			/// for taking a snapshot of the world.
			/// Release the physics systems when you are finished using it.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		void getWorldAsSystems(hkArray<hkpPhysicsSystem*>& systemsOut) const;

		/*
		**	Protected Functions
		*/

			// you should use removeReference() instead, however we allow you to force destroy the world
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		~hkpWorld();

		/*
		**	protected members: They are public so that people can get full control over the engine
		**  if they know what they are doing!!!
		*/
	public:

			// A class controlling the simulation
		hkpSimulation* m_simulation;

		hkVector4			m_gravity;

        hkpSimulationIsland* m_fixedIsland;          // +nosave
			hkpRigidBody*	m_fixedRigidBody;

        hkArray<hkpSimulationIsland*> m_activeSimulationIslands; // +nosave
        hkArray<hkpSimulationIsland*> m_inactiveSimulationIslands; // +nosave

			// Dirty islands are scheduled for cleanup, performed  by hkpWorldOperationUtil::cleanupDirtyIslands( )
        hkArray<hkpSimulationIsland*> m_dirtySimulationIslands;  // +nosave

        hkpWorldMaintenanceMgr* m_maintenanceMgr;    // +nosave
        hkWorldMemoryAvailableWatchDog* m_memoryWatchDog; // +nosave

        hkpBroadPhase* m_broadPhase; // +nosave

			// Broad phase handler - this is a class which links the broad phase to the simulation
        hkpTypedBroadPhaseDispatcher*    m_broadPhaseDispatcher; // +nosave
        hkpPhantomBroadPhaseListener*    m_phantomBroadPhaseListener; // +nosave
        hkpEntityEntityBroadPhaseListener* m_entityEntityBroadPhaseListener; // +nosave
        hkpBroadPhaseBorderListener* m_broadPhaseBorderListener; // +nosave


        hkpMultithreadedSimulationJobData* m_multithreadedSimulationJobData; // +nosave
        hkpProcessCollisionInput* m_collisionInput; // +nosave
            hkpCollisionFilter*       m_collisionFilter;  // +nosave
            hkpCollisionDispatcher*   m_collisionDispatcher; // +nosave
            hkpConvexListFilter*     m_convexListFilter; // +nosave

			/// For delayed operations. Operations get delayed if they cannot be performed as the world is hkpWorld::locked()
        hkpWorldOperationQueue* m_pendingOperations; // +nosave
			int m_pendingOperationsCount;
			int m_criticalOperationsLockCount;
				// It just holds a value offset which should be added to m_criticalOperationsLockCount when querying areCriticalOperationsLockedForPhantoms().
			int m_criticalOperationsLockCountForPhantoms;
			hkBool m_blockExecutingPendingOperations;
			hkBool m_criticalOperationsAllowed;
				// This points to structures used to debug operation postponing. Not used in release build.
            hkpDebugInfoOnPendingOperationQueues* m_pendingOperationQueues; // +nosave
				// Holds the number of pending queues (equal to the number of recursive calls to executeAllPendingOperations).
			int m_pendingOperationQueueCount;

			/// The thread index of the locking thread
        mutable class hkMultiThreadCheck m_multiThreadCheck; // +nosave

			/// Internal. Forces actions to be processed in a single thread.
		hkBool m_processActionsInSingleThread;

			// This allows to keep the islands size reasonable large. See hkpWorldCinfo::m_minDesiredIslandSize for details
		hkUint32 m_minDesiredIslandSize;

			// set if running a mt simulation
			// whenever a constraint is added, this critical section is used
        class hkCriticalSection* m_modifyConstraintCriticalSection; // +nosave

        class hkCriticalSection* m_worldLock;   // +nosave

			// a critical section which may guard the island dirty list
        class hkCriticalSection* m_islandDirtyListCriticalSection;      // +nosave

		struct PropertyLock
		{
			HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_DYNAMICS, hkpWorld::PropertyLock );

			hkUint32 m_key;
			mutable hkMultiThreadCheck m_multiThreadCheck;
		};

        class hkCriticalSection* m_propertyMasterLock; // +nosave

		hkBool m_wantSimulationIslands;


		// Welding parameters (just stored here for getCinfo())
		hkReal m_snapCollisionToConvexEdgeThreshold;
		hkReal m_snapCollisionToConcaveEdgeThreshold;
		hkBool m_enableToiWeldRejection;

			// Deactivation parameters
		hkBool m_wantDeactivation;

		hkBool m_shouldActivateOnRigidBodyTransformChange;


			// deprecated
		hkReal m_highFrequencyDeactivationPeriod;
			// deprecated
		hkReal m_lowFrequencyDeactivationPeriod;

			// see hkpWorldCinfo::m_deactivationReferenceDistance
		hkReal m_deactivationReferenceDistance;

		hkReal m_toiCollisionResponseRotateNormal;
        hkEnum<hkpWorldCinfo::SimulationType,hkInt32>  m_simulationType; //+nosave

			// debug only and used by the havok demo framework
		static hkBool m_forceMultithreadedSimulation;

			// This helper class can be used by a demo's constructor to ignore m_forceMultithreadedSimulation. This is useful e.g. for demos
			// that won't work properly in forced-multithreaded mode. Creating a local instance at the constructor's beginning will backup
			// the original value, override it with 'false' and restore the original value when leaving the demo's constructor.
			// Make sure to 'delete' this instance before leaving the demo's constructor if you explicitly 'new' it!
		class IgnoreForceMultithreadedSimulation
		{
			public:
				HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_DYNAMICS, hkpWorld::IgnoreForceMultithreadedSimulation );

				 IgnoreForceMultithreadedSimulation() { m_originalForceMultithreadedSimulationValue = hkpWorld::m_forceMultithreadedSimulation; hkpWorld::m_forceMultithreadedSimulation = false; }
				~IgnoreForceMultithreadedSimulation() { hkpWorld::m_forceMultithreadedSimulation = m_originalForceMultithreadedSimulationValue; }

			public:
				hkBool m_originalForceMultithreadedSimulationValue;
		};

	protected:
			// World-unique ids are assigned to entities. They're used to sort constraints deterministically when using hkMultithreadedSimulation.
		hkUint32 m_lastEntityUid;


	public:
		friend class hkpWorldOperationUtil;
		friend class hkpWorldCallbackUtil;
		friend class hkpSimulation;
		friend class hkHalfstepSimulation;
		friend class hkpContinuousSimulation;
		friend class hkpMultiThreadedSimulation;
		friend class hkpSimpleConstraintContactMgr;

		hkArray<hkpPhantom*> m_phantoms;

        hkArray<hkpActionListener*>              m_actionListeners;  // +nosave
        hkArray<hkpEntityListener*>              m_entityListeners;  // +nosave
        hkArray<hkpPhantomListener*>             m_phantomListeners; // +nosave
        hkArray<hkpConstraintListener*>          m_constraintListeners;  // +nosave
        hkArray<hkpWorldDeletionListener*>       m_worldDeletionListeners; // +nosave
        hkArray<hkpIslandActivationListener*>    m_islandActivationListeners; // +nosave
        hkArray<hkpWorldPostSimulationListener*> m_worldPostSimulationListeners; // +nosave
        hkArray<hkpWorldPostIntegrateListener*>  m_worldPostIntegrateListeners; // +nosave
        hkArray<hkpWorldPostCollideListener*>    m_worldPostCollideListeners; // +nosave
        hkArray<hkpIslandPostIntegrateListener*> m_islandPostIntegrateListeners; // +nosave
        hkArray<hkpIslandPostCollideListener*>   m_islandPostCollideListeners; // +nosave
        hkArray<hkpCollisionListener*>           m_collisionListeners; // +nosave
        hkArray<hkpContactImpulseLimitBreachedListener*> m_contactImpulseLimitBreachedListeners; //+nosave


        class hkpBroadPhaseBorder*               m_broadPhaseBorder; // +nosave


	public:
			//
			// These members are for internal use only
			//
		struct hkpWorldDynamicsStepInfo m_dynamicsStepInfo; //+reflected(false)


		// Min and Max extents of broadphase
		// Needed for when construction info is retrieved
		hkVector4 m_broadPhaseExtents[2];
		int			m_broadPhaseNumMarkers;
		int			m_sizeOfToiEventQueue;
		hkInt32		m_broadPhaseQuerySize;
		hkInt32		m_broadPhaseUpdateSize;
        hkEnum<hkpWorldCinfo::ContactPointGeneration, hkInt8> m_contactPointGeneration; // +nosave

		//
		// Deactivation
		//

			// Deprecated: Gets the high frequency deactivation period.
		inline hkReal getHighFrequencyDeactivationPeriod() const;

			// Deprecated: Sets the high frequency deactivation period.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void setHighFrequencyDeactivationPeriod( hkReal period );

			// Deprecated: Gets the low frequency deactivation period.
		inline hkReal getLowFrequencyDeactivationPeriod() const;

			// Deprecated: Sets the low frequency deactivation period.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void setLowFrequencyDeactivationPeriod( hkReal period );

		//
		// Extra action management
		//

			// Attaches an action to an entity -- this should be called for every entity being
			// attached to the action, when the action is already added to the world
			// Note: this does not manage the action's entity list.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void attachActionToEntity(hkpAction* action, hkpEntity* entity);

			// Detaches an action from an entity -- this should be called for every entity being
			// detached from the action, when the action is already added to the world.
			// Note: this does not manage the action's entity list.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		void detachActionFromEntity(hkpAction* action, hkpEntity* entity);


		//
		// Locking and delaying of operations
		//
	public:

			// Locks the world, i.e. prevents all critical operations (like adding/removing entities or constraints) from being
			// executed immediately. The operations are put on an hkWorldOperationsQueue and should be executed at the earliest
			// safe time. See hkpWorldOperationQueue for the list of 'critical' operations which may be postponed by the system.
			// Subsequent lockCriticalOperations() calls accumulate.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		HK_FORCE_INLINE void	lockCriticalOperations();

			// Allows critical operations to be executed immediately after unlockCriticalOperations() has been called or every corresponding lockCriticalOperations().
			// It is advised to call attemptToExecutePendingOperations right after unlockCriticalOperations() to keep the order of operations
			// the same as the order in which they were called.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		HK_FORCE_INLINE void	unlockCriticalOperations();

			// Returns number of locks put on the hkpWorld. Critical operation will be postponed if areCriticalOperationsLocked() > 0
			// We assume that this function is called at a beginning of a critical operation.
			// So this function asserts if critical operations are not allowed at all
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		HK_FORCE_INLINE int		areCriticalOperationsLocked() const;

			// like areCriticalOperationsLocked without the extra check
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		HK_FORCE_INLINE int		areCriticalOperationsLockedUnchecked() const;

			// Concept: Locking of Phantoms is an add-on work-around.
			//          It lets you execute phantom-related critical functions even when hkpWorld::areCriticalOperationsLocked() is called.
			//          Note that recursive phantom operations will not be executed, and will be postponed just like other operations are.

			// Decreases lock count for phantom operations. The operations must query hkpWorld::areCriticalOperationsLockedForPhantoms()
			// instead of hkpWorld::areCriticalOperationsLocked().
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		HK_FORCE_INLINE void	unlockCriticalOperationsForPhantoms();

			// Increases lock count for phantom operations..
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		HK_FORCE_INLINE void	lockCriticalOperationsForPhantoms();

			// Queries lock count for phantom operations. hkpWorld::m_criticalOperationsLockCountForPhantoms is an offset value added to the primary hkpWorld::m_criticalOperationsLockCount.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RO] );
		HK_FORCE_INLINE int		areCriticalOperationsLockedForPhantoms() const;



			// Concept: explained in hkpWorld.cpp. Search for: "concept: blocking of execution of pending operations"
			// Blocks or unblocks attempts of execution of pending operations.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		HK_FORCE_INLINE void	blockExecutingPendingOperations(hkBool block);

			// See: hkpWorld::unlock + hkpWorld::attemptToExecutePendingOperations
		HK_FORCE_INLINE void	unlockAndAttemptToExecutePendingOperations();

			// If the world is locked or execution of pending operations is blocked, then it does nothing.
			// Otherwise it processes the list of pending operations.
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		HK_FORCE_INLINE void	attemptToExecutePendingOperations();

			// Appends information about an operation request onto the world's hkpWorldOperationQueue.
						void	queueOperation(const hkWorldOperation::BaseOperation& operation);

			// Queues an user callback onto operation queue.
			// If the world is not locked, the callback is executed immediately.
						hkWorldOperation::UserCallback* queueCallback(hkWorldOperation::UserCallback* callback, hkUlong userData = 0);
	private:
			// Passes execution call to hkpWorldOperationQueue::attemptToExecuteAllPending()
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		                void	internal_executePendingOperations();

	#if defined HK_DEBUG
			// Returns true if both dynamic bodies attached to the constraint can collide with one another, based on
			// the logic in the hkpWorld's hkpCollisionFilter.
		hkBool constrainedDynamicBodiesCanCollide( const hkpConstraintInstance* constraint ) const;

			// Issues warnings that vary depending on whether the two dynamic, constrained bodies will definitely
			// collide or whether the user simply needs to manually call hkpWorld::updateCollisionFilter...()
		void warnIfConstrainedDynamicBodiesCanCollide( const hkpConstraintInstance* constraint,
															hkBool collidingBeforeConstraintAdded,
															hkBool collidingAfterConstraintAdded ) const;
	#endif	// #if defined HK_DEBUG


	public:
			// Debugging utility: see hkpWorld.cpp. Search for: "concept: allowing critical operations"
			/// ###ACCESS_CHECKS###( [this,HK_ACCESS_RW] );
		HK_FORCE_INLINE void	allowCriticalOperations(hkBool allow);

		/// Finish constructor
		explicit hkpWorld( hkFinishLoadedObjectFlag flag );
};




#include <Physics/Dynamics/World/hkpWorld.inl>

#endif // HK_DYNAMICS2_WORLD_H


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

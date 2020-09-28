/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_DYNAMICS2_SIMULATION_ISLAND_H
#define HK_DYNAMICS2_SIMULATION_ISLAND_H

#include <Common/Base/DebugUtil/DeterminismUtil/hkCheckDeterminismUtil.h>

#include <Physics/ConstraintSolver/Accumulator/hkpVelocityAccumulator.h>
#include <Physics/Dynamics/Entity/hkpEntity.h>

#include <Physics/Dynamics/Constraint/hkpConstraintOwner.h>

#include <Physics/Internal/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h>
#include <Physics/Collide/Dispatch/BroadPhase/hkpBroadPhaseListener.h>

class hkUnionFind;

class hkpAction;
class hkpEntity;
class hkpConstraintInstance;
class hkStepInfo;
class hkpBroadPhase;
class hkpWorld;
class hkpCollisionDispatcher;
struct hkpWorldDynamicsStepInfo;
class hkpBroadPhaseHandlePair;
struct hkpCollisionInput;
struct hkpProcessCollisionOutput;

///	These objects are internally managed by hkpWorld. They are created and destroyed continuously during simulation.
/// At any one time, you can view the contents of the simulation islands, by calling hkpWorld::getActiveSimulationIslands
/// and hkpWorld::getInactiveSimulationIslands. However you must NOT keep any handles to the simulation islands, and you
/// must not alter anything in the simulation islands.
class hkpSimulationIsland : public hkpConstraintOwner
{
	public:
			
		HK_DECLARE_CLASS_ALLOCATOR(HK_MEMORY_CLASS_SIMISLAND);

			/// Get the list of entities in the simulation island
		inline const hkArray<hkpEntity*>& getEntities() const;

			/// Get the list of actions in the simulation island
		inline const hkArray<hkpAction*>& getActions() const;

			/// Returns true if the island is active. 
			/// Tech info: returns true if the island is active in the next integration step. 
			///            To see whether the island actually was integrated in the last PSI or TOI step,
			///            check hkpSimulationIsland::m_isInActiveIslandsArray.
		inline hkBool isActive() const;


			/// Returns true is the island is fixed
		inline hkBool isFixed() const;

			/// Get the world
		inline hkpWorld* getWorld();

			/// Gets the memory usage required to complete the next integration step on this island.
			/// This function is called internally to prevent the memory usage breaching the critical memory limit in
			/// hkMemory.
		inline int getMemUsageForIntegration();


			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_RO] [this,HK_ACCESS_RO] );
		void calcStatistics( hkStatisticsCollector* collector) const;


		//
		// The remainder of the file is for Internal use only
		//

	protected:

		friend class hkpWorld;
		friend class hkpWorldOperationUtil;
		friend class hkpWorldCallbackUtil;
		friend class hkpSimulation;
		friend class hkpContinuousSimulation;

		
		hkpSimulationIsland( hkpWorld* world );
		~hkpSimulationIsland();
			
			// Properly sets storage indices and pointers on the entity. Updates island's data with entity's motion info.
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_RO] [this,HK_ACCESS_RW] );
		void internalAddEntity(hkpEntity* entity);

			// Properly sets storage indices and pointers on the entity. Updates island's data with entity's motion info.
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_RO] [this,HK_ACCESS_RW] );
		void internalRemoveEntity(hkpEntity* entity);

			// hkpConstraintOwner interface implementation
		virtual void addConstraintToCriticalLockedIsland(      hkpConstraintInstance* constraint );
		virtual void removeConstraintFromCriticalLockedIsland( hkpConstraintInstance* constraint );
			/// ###ACCESS_CHECKS###( [constraint->getSimulationIsland()->getWorld(),HK_ACCESS_RW] );
		virtual void addCallbackRequest( hkpConstraintInstance* constraint, int request );

#ifdef HK_DEBUG_MULTI_THREADING
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_IGNORE] [this,HK_ACCESS_RW] );
		virtual void checkAccessRw();
#endif

			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_RO] [this,HK_ACCESS_RW] );
		void addAction( hkpAction* act );
			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_RO] [this,HK_ACCESS_RW] );
		void removeAction( hkpAction* act );

	protected:

			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_RO] [this,HK_ACCESS_RO] );
		hkBool isFullyConnected( hkUnionFind& checkConnectivityOut );


	public:

		inline int getStorageIndex();

	public:

		void markForWrite( );

		inline void unmarkForWrite();

	public:

			/// ###ACCESS_CHECKS###( [m_world,HK_ACCESS_RO] [this,HK_ACCESS_RO] );
		void isValid();

		void mergeConstraintInfo( hkpSimulationIsland& other );

			// helper functions for debugging multithreading
		inline void markAllEntitiesReadOnly() const;

			// helper functions for debugging multithreading
		inline void unmarkAllEntitiesReadOnly() const;

		hkMultiThreadCheck& getMultiThreadCheck() const { return m_multiThreadCheck; }

	public:
		hkpWorld*	m_world;

			// The number of constraints in this island. This number could be more than 65k!
		int m_numConstraints;	

			// Storage index of the island: either in hkpWorld::m_activeSimulationIslands or hkpWorld::m_inactiveSimulationIslands
			// depending on m_isInActiveIslandsArray
		hkObjectIndex m_storageIndex;

			// If the island is dirty, than the island is also stored in the hkpWorld::m_dirtySimulationIslands
			// dirty islands are cleaned up in hkpWorldOperationUtil::cleanupDirtyIslands
		hkObjectIndex m_dirtyListIndex;

			// A counter which is incremented each frame
			// only on every 7th frame we check for split or subisland deactivation
		hkUchar m_splitCheckFrameCounter;

			// deprecated
		hkUchar m_highFrequencyDeactivationCounter;
		hkUchar m_lowFrequencyDeactivationCounter;

			// set if a split is requested
		bool m_splitCheckRequested:2;

			// this is set if the island can be split into subislands, but it
			// is not done to avoid tiny islands.
			// See hkpWorldCinfo::m_minDesiredIslandSize for details
		bool m_sparseEnabled:2;

			// set if the action array need cleanup
		bool m_actionListCleanupNeeded:2;

			// for debugging multithreading
		bool m_allowIslandLocking:2;

			// see m_storageIndex
		bool m_isInActiveIslandsArray:2;

			// desired Activation State. Can only be deactivated if m_sparseEnabled is set to false
		bool m_active:2;

			// if this is set to true, the island is integrated and cannot be modified, this is used for debugging multithreading only
		bool m_inIntegrateJob:2;

	protected:
		mutable hkMultiThreadCheck m_multiThreadCheck;

	public:
		hkReal m_timeSinceLastHighFrequencyCheck;
		hkReal m_timeSinceLastLowFrequencyCheck;

		hkArray<hkpAction*> m_actions;

		hkTime m_timeOfDeactivation;

			//
			// the next two 'objects' are both 16byte-aligned so we place them at the end of the class
			//

			// the entities (note: optimized for single entities)
			// for spus this must be 16 byte aligned (the constructor sets the initial capacity to 4
#if defined(HK_PLATFORM_HAS_SPU)
			// we need to upload this array to SPU (note: the inplace array does not guarantee a 16 byte alignment)
			hkArray<hkpEntity*> m_entities;
#else
			// the maximum size of this inplace array must be one. Or the split simulation island code could generate an invalid dealloce, which
			// will be very difficult to track down.
			hkInplaceArray<hkpEntity*,1> m_entities;
#endif

			// for narrow phase collision agents
		hkpAgentNnTrack m_agentTrack;

			// This is a variable tag. At all times these are unique tags.
		HK_ON_DETERMINISM_CHECKS_ENABLED( hkUint32 m_uTag );

};

#include <Physics/Dynamics/World/hkpSimulationIsland.inl>

#endif // HK_DYNAMICS2_SIMULATION_ISLAND_H

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

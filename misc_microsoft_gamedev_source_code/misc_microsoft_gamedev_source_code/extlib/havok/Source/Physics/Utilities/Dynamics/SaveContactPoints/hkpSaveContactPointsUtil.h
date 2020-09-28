/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */

#ifndef HK_UTILITIES2_SAVE_CONTACT_POINTS_UTIL_H
#define HK_UTILITIES2_SAVE_CONTACT_POINTS_UTIL_H

#include <Common/Base/hkBase.h>

#include <Physics/Internal/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h>
#include <Physics/Dynamics/Collide/hkpSimpleConstraintContactMgr.h>

#include <Physics/ConstraintSolver/Constraint/Contact/hkpContactPointProperties.h>

#include <Physics/Utilities/Dynamics/SaveContactPoints/hkpSerializedAgentNnEntry.h>

class hkpPhysicsSystemWithContacts;

	/// This utility saves and loads contact points an entire scene or for individual collision agents.
	///
	/// The utility only supports hkCollisionAgents using the streaming agents technology. Most commonly used agents have
	/// their 'streaming' version now. However older agents e.g. capsule-capsule are not supported. Don't register such 
	/// agents, and use more general streaming agents like Gsk instead.
	///
	/// Note that the utility saves 'raw' data from agent streams and as the result, the hkpCollisionDispatcher must 
	/// be initialized with the same collision agents when points are both saved and loaded. Also that raw agent data 
	/// cannot be versioned.
	///
	/// To allow cross-platform usability, hkpSaveContactPointsEndianUtil fixes endiness of serialized data. 
class hkpSaveContactPointsUtil
{
	public:
			//
			// Top level functions
			//

		typedef hkUlong (*GetIdForEntityFunc)( const hkpEntity* entity );

		typedef hkpEntity* (*GetEntityFromIdFunc)( hkUlong id );

		struct SavePointsInput
		{
			HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_UTILITIES, hkpSaveContactPointsUtil::SavePointsInput );

			//
			// Identifying enities
			//

				/// Uses entity id's returned by user's callback, rather than hkpEntity pointers 
				/// to bind contact data to a colliding pair.
			bool m_useEntityIds;

				/// Callback function used to get entity id to be stored with contact points. 
				/// Must be assigned valid if m_useEntityIds is true;
			GetIdForEntityFunc	m_getIdForEntity;

			SavePointsInput() : m_useEntityIds(false), m_getIdForEntity(HK_NULL) {}

		};


			/// Saves contact points of the entire world
			/// ###ACCESS_CHECKS###( [world,HK_ACCESS_RO] );
		static void HK_CALL saveContactPoints( const hkpSaveContactPointsUtil::SavePointsInput& input, const hkpWorld* world, hkpPhysicsSystemWithContacts* sys );

			/// Saves contact points of selected entities.
			/// ###ACCESS_CHECKS###( [world,HK_ACCESS_RO] );
		static void HK_CALL saveContactPoints( const hkpSaveContactPointsUtil::SavePointsInput& input, const hkpEntity** entities, int numEntities, hkpPhysicsSystemWithContacts* sys );
		
			/// Saves contact points from selected agents.
			/// ###ACCESS_CHECKS###( [world,HK_ACCESS_RO] );
		static void HK_CALL saveContactPoints( const hkpSaveContactPointsUtil::SavePointsInput& input, const hkpAgentNnEntry** agentEntries, int numEntries, hkpPhysicsSystemWithContacts* sys );


		struct LoadPointsInput
		{
			HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_UTILITIES, hkpSaveContactPointsUtil::LoadPointsInput );

				/// Remove a serialized entry from the physics system, after it is deserialized.
				/// When saved contact points are deserialized, a copy of the data is created, and the saved contact points
				/// can be destroyed, unless you're planning to reused them. Set this flag, to remove all saved agents,
				/// right after they're successfully deserialized.
			bool m_removeSerializedAgentsWhenLoaded;

				/// This zeros userData in hkpContactPointProperties.
				/// May be useful if you use it to store pointers to custom data structures.
			bool m_zeroUserDataInContactPointProperties;

			//
			// Identifying enities
			//

				/// Callback function used to get hkpEntity pointers when deserializing contact points. Must be valid 
				/// if stored contact points were saved with entity id's rather than hkpEntity pointers
			GetEntityFromIdFunc	m_getEntityFromId;

			//
			// Collision callback settings
			//

				/// Fire or suppress contact point added callbacks.
			bool m_fireContactPointAddedCallbacks;

				/// Fire or suppress contact point confirmed callbacks.
				/// Note that, regardless of this setting, this callback will not be called
				/// for contact points that had existed for several frames when they were saved.
				/// This callback is only triggered once for a contact point -- when it's
				/// passed to the constraint solver for the first time.
			bool m_fireContactPointConfirmedCallbacks;

				/// Pass hkpCollidable pointers in callback events, or pass HK_NULL otherwise.
				/// When loading contact points, the callbacks don't have access to the original hkCdBodies (child shapes)
				/// that they relate to. You may decide to pass HK_NULL, or the top level hkpCollidable instead. 
				///
				/// Note that passing top level hkCollidables may cause problems, if you have collision listeners, 
				/// that assume that they track bodies of certain type. 
				/// E.g. you may have a listener connected to Mopp shapes, and have it assume that when it
				/// gets a callback, it can query the shapeKey of the passed hkCdBodies.
			bool m_passCollidablePointersInCollisionCallbacks;

			LoadPointsInput() : m_removeSerializedAgentsWhenLoaded(false),
				                m_zeroUserDataInContactPointProperties(false), 
								m_getEntityFromId(HK_NULL),
								m_fireContactPointAddedCallbacks(true),
								m_fireContactPointConfirmedCallbacks(true),
								m_passCollidablePointersInCollisionCallbacks(false)
								{
								}

		};

			/// Loads contact points for the entire world.
		static void HK_CALL loadContactPoints( const hkpSaveContactPointsUtil::LoadPointsInput& input, hkpPhysicsSystemWithContacts* sys, hkpWorld* world );

			/// Loads contact points for selected entities.
		static void HK_CALL loadContactPoints( const hkpSaveContactPointsUtil::LoadPointsInput& input, hkpPhysicsSystemWithContacts* sys, hkpEntity** entities, int numEntities );


			//
			// New internal functions
			//

		class EntitySelector
		{
			public:
				virtual ~EntitySelector() { }
				virtual hkBool32 isEntityOk(const hkpEntity* entity) = 0;
		};

	private:

			/// ###ACCESS_CHECKS###( [world,HK_ACCESS_RW] );
		static inline void HK_CALL loadContactPointsInternal( const hkpSaveContactPointsUtil::LoadPointsInput& input, hkpPhysicsSystemWithContacts* sys, hkpWorld* world, hkpSaveContactPointsUtil::EntitySelector& selector );


		static hkResult HK_CALL serializeCollisionEntry( const SavePointsInput& input, const hkpAgentNnEntry* entry, const hkpProcessCollisionInput* collisionInput, hkpSerializedAgentNnEntry& serializedEntryOut );

		static hkResult HK_CALL deserializeCollisionEntry( const LoadPointsInput& input, const hkpSerializedAgentNnEntry& serializedEntryIn, hkpEntity* entityA, hkpEntity* entityB, hkpProcessCollisionInput* collisionInput, hkpAgentNnEntry* entry );


		static hkResult HK_CALL serializeEntry( const hkpAgentEntry* entry, hkBool isNnEntry, const hkpProcessCollisionInput* input, int& sizeOfThisEntryOut, enum hkpSerializedAgentNnEntry::SerializedAgentType& agentTypeOut, const hkpAgent1nTrack*& agent1nTrackOut, hkpSerializedTrack1nInfo& trackInfoOut );

		static hkResult HK_CALL destroyOldEntry( const hkpSerializedAgentNnEntry::SerializedAgentType agentType, hkpProcessCollisionInput* input, hkpDynamicsContactMgr* mgr, hkpConstraintOwner* constraintOwner, hkpAgentNnEntry* entryInOut );
			//this is only used for nn entries
		static hkResult HK_CALL deserializeEntry( const hkpSerializedAgentNnEntry& serializedEntryIn, const hkpSerializedAgentNnEntry::SerializedAgentType agentType, const hkpSerializedTrack1nInfo& serializedTrack, hkpProcessCollisionInput* input, hkpAgentNnEntry* entryInOut );


		static hkResult HK_CALL serialize1nTrack( const hkArray<hkpAgent1nSector*>& sectorsIn, const hkpProcessCollisionInput* input, hkpSerializedTrack1nInfo& trackInfoOut);

		static hkResult HK_CALL deserialize1nTrack( const hkpSerializedTrack1nInfo& serializedTrack, hkpProcessCollisionInput* input, hkArray<hkpAgent1nSector*>& sectorsOut);

			//
			// New helper functions
			//

		static hkpSerializedAgentNnEntry::SerializedAgentType HK_CALL getSerializedAgentType(hkAgent3::ProcessFunc func);

		friend class hkpSaveContactPointsEndianUtil;

};







#endif // HK_UTILITIES2_SAVE_CONTACT_POINTS_UTIL_H

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

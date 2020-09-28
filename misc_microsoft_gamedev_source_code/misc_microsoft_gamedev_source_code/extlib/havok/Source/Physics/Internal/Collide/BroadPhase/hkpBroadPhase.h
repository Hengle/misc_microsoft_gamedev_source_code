/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */
#ifndef HK_COLLIDE2_BROAD_PHASE_H
#define HK_COLLIDE2_BROAD_PHASE_H

#include <Common/Base/hkBase.h>
#include <Common/Base/DebugUtil/MultiThreadCheck/hkMultiThreadCheck.h>
#include <Common/Base/Types/Geometry/Aabb/hkAabb.h>

class hkpBroadPhaseHandle;
class hkpBroadPhaseHandlePair;
class hkAabb;
class hkPrimitiveCastCallback;
class hkpBroadPhaseCastCollector;

//
typedef char hkpBroadPhaseAabbCache;

/// The job of the broadphase is to quickly find pairs of AABBs that are intersecting, and thus to identify
/// pairs of objects that require narrowphase collision detection. Objects that can be
/// processed by the broadphase must have an hkpBroadPhaseHandle, which is used as an id for each object by the broadphase. The broadphase actually keeps
/// an internal pointer to each hkpBroadPhaseHandle, so do not forget this - for instance, do not copy handles.
///
/// The 16bit broadphase:
///   - Supports up to 2^14 objects (including markers)
///   - Has an internal resolution of 15 bits.
///
/// The 32bit broadphase:
///   - Supports up to 2^31 objects (including markers)
///   - Has an internal resolution of 31 bits.
///   - Is twice as slow, and uses twice as much memory as the 16bit version
///
/// Use the following macro to enable the 32bit broadphase. Put it *before* world construction. You will need to #include the hkpBroadPhase.h header:
/// \code
/// #include <hkinternal/collide/broadphase/hkpBroadPhase.h>
/// ...
/// HK_ENABLE_32BIT_BROADPHASE;
/// hkpWorld* myWorld = new hkpWorld(worldCinfo);
/// \endcode
class hkpBroadPhase: public hkReferencedObject
{
	public:

		hkpBroadPhase();
		~hkpBroadPhase();

		HK_DECLARE_CLASS_ALLOCATOR(HK_MEMORY_CLASS_BROAD_PHASE);

		enum BroadPhaseType
		{
			BROADPHASE_16BIT,
			BROADPHASE_32BIT
		};

			/// Returns the type of the broadphase. This can be either 16bit or 32bit.
		virtual BroadPhaseType getType() const = 0;

			/// Adds an object to the broadphase. The new overlapping pairs are reported in pairsOut.
		virtual void addObject( hkpBroadPhaseHandle* object, const hkAabbUint32& aabb, hkArray<hkpBroadPhaseHandlePair>& pairsOut  ) = 0;

			/// Adds an object to the broadphase. The new overlapping pairs are reported in pairsOut.
		virtual void addObject( hkpBroadPhaseHandle* object, const hkAabb& aabb, hkArray<hkpBroadPhaseHandlePair>& pairsOut  ) = 0;

			/// Adds a list of objects to the broadphase. The new overlapping pairs are reported in pairsOut.
			/// This is faster than repeatedly calling addObject, if you have a list of objects to add
		virtual void addObjectBatch( hkArray<hkpBroadPhaseHandle*>& addObjectList, hkArray< hkAabb >& addAabbList, hkArray<hkpBroadPhaseHandlePair>& newPairs ) = 0;

			/// Removes an object from the broadphase. The removed overlapping pairs are reported in pairsOut.
		virtual void removeObject( hkpBroadPhaseHandle* object, hkArray<hkpBroadPhaseHandlePair>& pairsOut ) = 0;

			/// Removes a list of objects from the broadphase. The removed overlapping pairs are reported in pairsOut.
			/// This is faster than repeatedly calling removeObject, if you have a list of objects to remove
		virtual void removeObjectBatch( hkArray<hkpBroadPhaseHandle*>& removeObjectList, hkArray<hkpBroadPhaseHandlePair>& delPairsOut ) = 0;

			/// Queries the total number of objects added to the broad phase.
		virtual int getNumObjects() = 0;

			/// Changes the position of an object and reports the changes. New overlaps are reported in addedPairs, removed pairs are reported in removedPairs.
			/// Note that a pair can be in both lists.
		virtual void updateAabbs( hkpBroadPhaseHandle* objects[], const hkAabb* aabbs, int numObjects, hkArray<hkpBroadPhaseHandlePair>& addedPairs, hkArray<hkpBroadPhaseHandlePair>& removedPairs ) = 0;

			// The same as updateAabbs(), only working on hkAabbUint32s.
		virtual void updateAabbsUint32( hkpBroadPhaseHandle* objects[], const hkAabbUint32* aabbs, int numObjects, hkArray<hkpBroadPhaseHandlePair>& addedPairs, hkArray<hkpBroadPhaseHandlePair>& removedPairs ) = 0;

			/// Optimizes internal memory layout. This can speed up the broadphase by up to 20%.
			/// This should be done after all static objects and before all dynamics objects
			/// are added to the scene.
		virtual void defragment() = 0;

			/// Gets all AABBs (for debug purposes).
		virtual void getAllAabbs( hkArray<hkAabb>& allAabbs ) = 0;

			/// Gets AABB from hkpBroadPhaseHandle
		virtual void getAabb(const hkpBroadPhaseHandle* object, hkAabb& aabb) const = 0;

			/// Gets the current extents of the entire broadphase (passed in construction, but may have later been updated via shiftBroadPhase)
		virtual void getExtents(hkVector4& worldMinOut, hkVector4& worldMaxOut) const = 0;

			/// Finds all intersections between the input object and the rest.
		virtual void querySingleAabb( const hkAabb& aabb, hkArray<hkpBroadPhaseHandlePair>& pairs_out) const = 0;

			/// Finds all intersection of an existing object and the rest
		virtual void reQuerySingleObject( const hkpBroadPhaseHandle* object, hkArray<hkpBroadPhaseHandlePair>& pairs_out) const = 0;

			/// If you want to shift all objects in the entire world but without altering the broadphase extents, you need to call this
			/// function. It will simply correct all internally stored offset information by the shiftDistance.
			/// Motivation for this method:
			///  If your world is very large and both our graphics and physics coordinate spaces are 
			/// single-precision, and if you stray far enough from the origin you'll get floating point errors.
			/// To prevent this you have to periodically shift your coordinate spaces when you detect we have moved too far.
			///  To shift the coordinate space, you have to reposition all objects in the world, which you can do by calling
			/// hkpRigidBody::setPosition(). However this is pretty CPU intensive and can have same bad side effects (disabling
		    /// tims for fixed objects).
			///  A better way is to silently reposition all objects:
			///   - shift all world space information of all objects silently(= avoid calling callbacks) by accessing their hkpMotion directly
			///   - update the broadphase to reflect these new positions by calling this method, hkpBroadPhase::shiftAllObjects()
			///
			/// Important Usage Notes:
			///     - The broadphase cannot shift by an arbitrary floating point value, it has to round the shiftDistance up or down.
			///       The actual shift distance is reported using effectiveShiftDistanceOut
			///     - This function reports all objects which now touch the world extents (but did not touch it before) in the array 
			///        objectsEnteringBroadphaseBorder. These are objects which 'left' the broadphase during the shift.
			///     - Please use the hkutilities/collide/hkpBroadPhaseBorder utility to prevent objects touching the world
			///       extents before the shift.
			///     - Objects which hit several borders are reported several times
			///     - Objects which hit a border must be removed *immediately* to avoid internal broadphase inconsistency
			///     - This function does not work if markers are enabled
		virtual void shiftAllObjects( const hkVector4& shiftDistance, hkVector4& effectiveShiftDistanceOut, hkArray<hkpBroadPhaseHandle*>& objectsEnteringBroadphaseBorder ) = 0;

			/// If you want to shift the broadphase extents without changing the world position of any objects, you need to call this function.
			/// It will modify the extents and then update the offset information.
			/// Note that this changes the current extents of the broadphase (in 'world' space) returned by hkpBroadPhase::getExtents().
			/// Same notes as shiftAllObjects apply, since the shift may cause objects to 'leave' the broadphase.
			/// Motivation for this method:
			///  If your world is large (but not large enough to run into floating point errors, when straying far from the
			/// origin), you may wish to keep a smaller broadphase active which only encompasses a subset of the full game world,
			/// probably those objects that are 'near' the current player or area of interaction.
			/// Thus your broadphase can fit more tightly than one which is large enough to encompass the whole game area.
		virtual void shiftBroadPhase( const hkVector4& shiftDistance, hkVector4& effectiveShiftDistanceOut, hkArray<hkpBroadPhaseHandle*>& objectsEnteringBroadphaseBorder ) = 0;

			// Returns the 32bit low & high offset values.
		virtual void getOffsetLowHigh32bit(hkVector4& offsetLow32bit, hkVector4& offsetHigh32bit) const = 0;

			/// Input structure
		struct hkpCastRayInput
		{
			HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_BROAD_PHASE, hkpBroadPhase::hkpCastRayInput );

				/// The 'from' position of multiple casts
			hkVector4		m_from;

				/// The number of casts you would like to make from the same m_from start position
			int       m_numCasts;

				/// Pointer to the first 'to' position
			const hkVector4* m_toBase;

				/// the byte difference between two consecutive 'to' positions, typically sizeof(hkVector4) = 16
			int		  m_toStriding;

				/// An optional cache. Note: all rays should be within the aabb which was used to build the cache
			const hkpBroadPhaseAabbCache* m_aabbCacheInfo;

			hkpCastRayInput(): m_numCasts(1), m_toStriding( hkSizeOf(hkVector4)),m_aabbCacheInfo(HK_NULL){}
		};

			/// Queries the broadphase for overlapping AABBs along the ray 'from-to'.
			/// For each aabb the ray cast hits the collector passed in receives a callback.
		    /// Basically this cast allows for doing multiple casts starting from the same position.
		    /// For each input ray( from, to[x] ) the collector:
			///      at memory address: (collectorBase+x*collectorStriding) is called.<br>
			/// E.g.
			///  - by setting collectorStriding to 0, always the collector collectorBase is called. You can identify
		    ///    the input ray by the castIndex in the hkpBroadPhaseCastCollector::addBroadPhaseHandle.
		    ///  - Or if you have an array of MyCollector collectors[10] you set the collectorBase to &collectors[0] and
		    ///    the collectorStriding = hkSizeOf(MyCollector)
		virtual	void castRay( const hkpCastRayInput& input, hkpBroadPhaseCastCollector* collectorBase, int collectorStriding) const = 0;

			/// Get the size for the axis cache
		virtual int getAabbCacheSize() const = 0;

			/// Build the aabb cache. This is an optimization for broadphase raycasts.
			/// The idea is that the aabb cache is actually a reduced broadphase, just storing
			/// objects inside the aabb. Therefore using this cache can speed up rayCasts significantly<br>
			/// To work properly you have to:
			///  - Calc an aabb which encapsulates a set of raycasts. The smaller the aabb is the better.
		    ///    Sometimes it might make sense to sort the rays into groups and than build one cache
			///    for each group.
			///  - Allocate enough space to hold your cache (use getAabbCacheSize()). The cache size can be very
			///    big, it's actually 12 bytes per object in the broadphase. This is a worst case situation,
			///    however the cache size cannot be calculated easily in advance, so getAabbCacheSize always returns
			///    this worst case number. The best is to use hkAllocateStack to get the memory (check for overflows
			///    in your allocate stack memory, which could dramatically slow down the operations).
			///  - call calcAabbCache with your aabb and your cache memory.
			///  - Than you can call as many calls to castRay using these cache. As long as the rays are within the
			///    aabb everything works well.
			/// Note:
			///    The cache only represents a snapshot of the engine. As soon as something changes the broadphase
			///    (object moving, getting added, etc) the cache is invalid. If an invalid cache is used the engine
			///    can crash !!!!.
		virtual void calcAabbCache( const hkAabb& aabb, hkpBroadPhaseAabbCache* aabbCacheOut) const = 0;

			/// Build the AABB cache from a list of collidables. This is useful for creating a cache when
		    /// you already have an AABB Phantom or a list of object that you know you want to raycast against.
		    /// This runs much faster that computing the cache from an AABB, because it doesn't have to search
		    /// through the broadphase.
		virtual void calcAabbCache( const hkArray<class hkpCollidable*>& overlappingCollidables, hkpBroadPhaseAabbCache* aabbCacheOut) const = 0;

			/// This structure is used as the input to hkpBroadPhase::castAabb
		struct hkpCastAabbInput
		{
			HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_BROAD_PHASE, hkpBroadPhase::hkpCastAabbInput );

				/// The start of the aabb cast (in world space)
			hkVector4 m_from;
				/// To end of the aabb cast (in world space)
			hkVector4 m_to;
				/// The half extents of the aabb
			hkVector4 m_halfExtents;

				/// An optional cache. Note: all casts should completely be within the aabb which was used to build the cache
			const hkpBroadPhaseAabbCache* m_aabbCacheInfo;

			hkpCastAabbInput(): m_aabbCacheInfo(HK_NULL){}
		};


			/// Queries the broadphase for overlapping AABBs along the aabb cast as specified in the input
			/// For each aabb the aabbcast hits the collector passed in receives a callback
		virtual void castAabb( const hkpCastAabbInput& input, hkpBroadPhaseCastCollector& collector ) const = 0;

			/// Set the 32bit offset and scale values for the broadphase. The broadphase will calculate some more internal values from that.
		virtual void set32BitOffsetAndScale(const hkVector4& offsetLow, const hkVector4& offsetHigh, const hkVector4& scale) = 0;

			/// Turn on locking and multithreading checks
		void enableMultiThreading( int spinCountForCriticalSection);

		inline hkMultiThreadCheck& getMultiThreadCheck();
		inline const hkMultiThreadCheck& getMultiThreadCheck() const;

			/// Mark this class and all child classes for read only access for this thread
		HK_FORCE_INLINE void markForRead() const;

			/// Mark this class and all child classes for read write access for this thread
		HK_FORCE_INLINE void markForWrite();

			/// Undo markForRead
		HK_FORCE_INLINE void unmarkForRead() const;

			/// Undo markForWrite
		HK_FORCE_INLINE void unmarkForWrite();

			/// lock the broadphase
		HK_FORCE_INLINE void lock();

			/// unlock the broadphase
		HK_FORCE_INLINE void unlock();

		typedef hkpBroadPhase* (HK_CALL *createBroadPhaseFunc)( const hkVector4& worldMin, const hkVector4& worldMax, int numMarkers );

		static createBroadPhaseFunc m_defaultCreationFunction;

	protected:

		void lockImplementation();
		void unlockImplementation();

	protected:

		mutable hkMultiThreadCheck m_multiThreadCheck;

		class hkCriticalSection *m_criticalSection;
};

hkpBroadPhase* HK_CALL hk3AxisSweep16CreateBroadPhase( const hkVector4& worldMin, const hkVector4& worldMax, int numMarkers );
hkpBroadPhase* HK_CALL hk3AxisSweep32CreateBroadPhase( const hkVector4& worldMin, const hkVector4& worldMax, int numMarkers );

// Use this macro to enable 32 bit broadphase. Put it *before* world construction. You will need to #include this header
// Example:
// #include <hkinternal/collide/broadphase/hkpBroadPhase.h>
// ...
// HK_ENABLE_32BIT_BROADPHASE;
// hkpWorld* myWorld = new hkpWorld(worldCinfo);
#define HK_ENABLE_32BIT_BROADPHASE hkpBroadPhase::m_defaultCreationFunction = hk3AxisSweep32CreateBroadPhase

#include <Physics/Internal/Collide/BroadPhase/hkpBroadPhase.inl>

#endif // HK_COLLIDE2_BROAD_PHASE_H


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

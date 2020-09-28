/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */
#ifndef HK_MATCH_SNAPSHOT_STATISTICS_COLLECTOR_H
#define HK_MATCH_SNAPSHOT_STATISTICS_COLLECTOR_H

#include <Common/Base/DebugUtil/StatisticsCollector/hkStatisticsCollector.h>
#include <Common/Base/Monitor/MonitorStreamAnalyzer/hkMonitorStreamAnalyzer.h>
#include <Common/Base/Container/PointerMap/hkPointerMap.h>
#include <Common/Base/Memory/StackTracer/hkStackTracer.h>

#include <Common/Base/Memory/Memory/Debug/hkDebugMemory.h>

class hkMatchSnapshotStatisticsCollector;
class hkAllocationSet
{
    public:
    HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR(HK_MEMORY_CLASS_BASE, hkAllocationSet);

    enum { MAX_ALLOCS = 8 };

        /// Ctor
    hkAllocationSet(const void* ptr,const hkDebugMemory::PointerInfo* info);

        /// Add an allocation to this
    void addAlloc(const void* ptr,const hkDebugMemory::PointerInfo* info);

        /// Returns true if the allocations 'match' - in particular the stack trace matches
    hkBool matches(int traceDepth,const void* ptr,const hkDebugMemory::PointerInfo* info);

        /// Dump out the contained information
    void dump(hkMatchSnapshotStatisticsCollector& collector);

        /// Returns true if any part of the call stack contains the string
    hkBool callStackContainsString(hkMatchSnapshotStatisticsCollector& collector,const hkString& in);

        ///
    hkString getCallStackString(hkMatchSnapshotStatisticsCollector& collector);

    static void HK_CALL _printFunc(const char*, void* context);

    static void HK_CALL _concatFunc(const char* text, void* context);

    //
    // Members
    //

    int m_size;
    int m_totalAllocs;

        /// Number of pointers we have stored
    hkUint8 m_numAllocs;
    /// Allocations of this type
    const void* m_allocs[MAX_ALLOCS];
    /// Info
    const hkDebugMemory::PointerInfo* m_info;
    /// Linked to the next allocation
    hkAllocationSet* m_next;
};


class hkMatchSnapshotStatisticsCollector: public hkReferencedObject, public hkStatisticsCollector
{
	friend class hkAllocationSet;
	public:
        enum { MAX_TRACE = 16 };

            /// Ctor
        hkMatchSnapshotStatisticsCollector(const hkDebugMemorySnapshot& snapshot,hkOstream& stream);

        // hkStatisticsCollector interface
        virtual void beginObject( const char* name, StatisticClass statisticClass, const hkReferencedObject* object );

		virtual void addChunk( const char* name, StatisticClass statisticClass, const void* chunkAddress, int usedSize, int allocatedSize);

		virtual void addChildObject( const char* name, StatisticClass statisticClass, const hkReferencedObject* object );

        virtual void endObject( ) {}

        virtual void pushDir( const char* dirName ) {}

        virtual void popDir() {}

            /// Dump how many are remaining
        void dumpRemaining();

            /// Marks all these allocations as already reached
		void addReached( const hkDebugMemorySnapshot& snapshot);

            /// Get the strings that are ignored
        hkObjectArray<hkString>& getCallStackIgnoreStrings() { return m_callStackIgnoreStrings; }

            /// Add an ignore string
        void addCallStackIgnoreString(const hkString& in);

	protected:

        static void HK_CALL _printFunc(const char*, void* context);

        static HK_FORCE_INLINE hkBool _compareAllocs( const hkAllocationSet* a, const hkAllocationSet* b )
		{
            return (a->m_info->numBytes > b->m_info->numBytes);
        }


        hkBool _isKnown(const hkReferencedObject* obj);
        void _addKnown(const hkReferencedObject* obj);
		hkBool _notReachable(const void* ptr);
		void _dumpStackTraceForUnknown(const void* ptr);

        hkOstream& m_stream;
        const hkDebugMemorySnapshot& m_snapshot;
            /// Map of pointers
        hkPointerMap<const void*,const hkDebugMemory::PointerInfo*> m_chunks;
            /// As we traverse we can remove pointers that are reached
        hkPointerMap<const void*,int> m_notReached;

            /// Known objects
		hkPointerMap<const hkReferencedObject*,int> m_knownObjects;

			/// Stack tracer - for debugging where allocations are made
        hkStackTracer m_tracer;

            /// List of strings, which if found in the call stack will cause the
            /// list to be ignored
        hkObjectArray<hkString> m_callStackIgnoreStrings;
};


#endif // HK_MATCH_SNAPSHOT_STATISTICS_COLLECTOR_H

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

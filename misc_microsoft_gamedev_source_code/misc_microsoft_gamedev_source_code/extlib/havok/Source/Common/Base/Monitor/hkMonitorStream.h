/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */
#ifndef HKBASE_HKMONITOR_STREAM_H
#define HKBASE_HKMONITOR_STREAM_H

class hkBool;
#include <Common/Base/Config/hkConfigMonitors.h>
#include <Common/Base/Config/hkConfigThread.h>
#include <Common/Base/Thread/Thread/hkThreadLocalData.h>

/// This class is used to capture all monitor information for a single frame. 
/// You can use monitors with the following macros
///
/// HK_TIMER_BEGIN( NAME, OBJECT )
/// HK_TIMER_END()
///
/// HK_TIMER_BEGIN_LIST( NAME, OBJECT )
/// HK_TIMER_SPLIT_LIST( NAME )
/// HK_TIMER_END_LIST(  )
///
/// HK_MONITOR_PUSH_DIR( PATH )
/// HK_MONITOR_POP_DIR()
/// HK_MONITOR_ADD_VALUE( NAME, VALUE, MONITOR_TYPE ) 
///
/// You can use the hkMonitorStreamAnalyzer to store multiple frames
/// and to pretty print the information.<br>
/// Notes:
///  - You must initialize this class on a per thread basis after calling hkBaseSystem::init or hkBaseSystem::initThread
///  - The hkMonitorStreamAnalyzer is singled threaded
///  - The hkMonitorStreamAnalyzer has utilities for analysing multiple monitor streams for use in a multithreaded environment
class hkMonitorStream 
{
	public:
		
		HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR(HK_MEMORY_CLASS_MONITOR, hkMonitorStream);

		static hkMonitorStream& getInstance()
		{
			extern HK_THREAD_LOCAL( hkMonitorStream* ) hkMonitorStream__m_instance;
			return *HK_THREAD_LOCAL_GET(hkMonitorStream__m_instance);
		}

			/// Clear all memory. This must be called per thread, before each frame.
		void HK_CALL reset();

			/// Sets the size of the stream used. By default the stream size is 0. If this is the
			/// case, or if the stream is full, no monitors are captured.
		void HK_CALL resize( int newSize );


	public:
		
		//
		//	These methods are for internal use only, and are called by the macros listed above.
		//

		// get the pointer to the beginning of the memory used for timers
		HK_FORCE_INLINE char* HK_CALL getStart() { return m_start.val(); } 

		// get the pointer to the current write pointer
		HK_FORCE_INLINE char* HK_CALL getEnd() { return m_end.val(); } 

		// get the pointer to the end of the timer memory minus 16
		HK_FORCE_INLINE char* HK_CALL getCapacityMinus16() { return m_capacityMinus16.val(); } 

		// get the pointer to the capacity of timer memory
		HK_FORCE_INLINE char* HK_CALL getCapacity() { return m_capacity.val(); } 

			// Check whether at least 16 bytes are free
		HK_FORCE_INLINE hkBool HK_CALL memoryAvailable( ) 
		{ 
			return getEnd() < getCapacityMinus16(); 
		}

		HK_FORCE_INLINE int HK_CALL getCurrentMonitorDataSize()
		{
			return (int)(m_end.val() - m_start.val());
		}

			// get a piece of memory from the timer memory 
		HK_FORCE_INLINE void* HK_CALL expandby( int size ) { char* h = m_end.val(); char* newEnd = h + size; m_end = newEnd; return h; }

				// set the pointer to the current write pointer
		HK_FORCE_INLINE void HK_CALL setEnd(char* ptr) {  m_end = ptr; } 

			// is the buffer allocated on the heap or not 
		HK_FORCE_INLINE hkBool HK_CALL isBufferAllocatedOnTheHeap() { return m_isBufferAllocatedOnTheHeap; }

			// set the buffer to be a static, preallocated, buffer
		void HK_CALL setStaticBuffer( char* buffer, int bufferSize );

	public:

			// Called by hkBaseSystem
		static void HK_CALL init();

			// Called by hkBaseSystem
		void HK_CALL quit();

	public:
		
		hkPadSpu<char*> m_start;
		hkPadSpu<char*> m_end;
		hkPadSpu<char*> m_capacity;
		hkPadSpu<char*> m_capacityMinus16;
		hkBool		m_isBufferAllocatedOnTheHeap;

	public:

		struct Command
		{
			HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_MONITOR, hkMonitorStream::Command );
			const char* m_commandAndMonitor;
		};

		struct AddValueCommand : public hkMonitorStream::Command
		{
			HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_MONITOR, hkMonitorStream::AddValueCommand );
			float m_value;
		};

		struct TimerCommand : public hkMonitorStream::Command
		{
			HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_MONITOR, hkMonitorStream::TimerCommand );
			hkUint32	m_time0;
			hkUint32	m_time1;
			HK_FORCE_INLINE void setTime();
		};

		struct TimerBeginListCommand : public hkMonitorStream::TimerCommand
		{
			HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR( HK_MEMORY_CLASS_MONITOR, hkMonitorStream::TimerBeginListCommand );
			const char* m_nameOfFirstSplit;
		};
};

#include <Common/Base/Monitor/hkMonitorStream.inl>

#endif // HKBASE_HKMONITOR_STREAM_H

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

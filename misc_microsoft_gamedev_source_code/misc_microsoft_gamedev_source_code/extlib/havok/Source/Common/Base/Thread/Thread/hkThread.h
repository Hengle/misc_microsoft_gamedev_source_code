/* 
 * 
 * Confidential Information of Telekinesys Research Limited (t/a Havok).  Not for disclosure or distribution without Havok's
 * prior written consent.This software contains code, techniques and know-how which is confidential and proprietary to Havok.
 * Level 2 and Level 3 source code contains trade secrets of Havok. Havok Software (C) Copyright 1999-2007 Telekinesys Research Limited t/a Havok. All Rights Reserved. Use of this software is subject to the terms of an end user license agreement.
 * 
 */
#ifndef HKBASE_HK_THREAD_H
#define HKBASE_HK_THREAD_H

/// A platform independent wrapper for a thread
class hkThread
{
	public:

		HK_DECLARE_NONVIRTUAL_CLASS_ALLOCATOR(HK_MEMORY_CLASS_BASE, hkThread);

		typedef void* (HK_CALL *StartFunction)(void*);

		hkThread();

		~hkThread();

		hkResult startThread( StartFunction f, void* );
		void stopThread();

		enum Status
		{
			THREAD_TERMINATED,
			THREAD_RUNNING,
			THREAD_NOT_STARTED
		};

		Status getStatus();

		hkUint64 getChildThreadId();

		void* getHandle();

			/// returns the id of the calling thread 
		static hkUint64 HK_CALL getMyThreadId();

	protected:
		void* m_thread;
		hkUint64 m_threadId;
};



#if defined (HK_PLATFORM_PS3SPU)
#	include <Common/Base/Thread/Thread/Empty/hkEmptyThread.inl>
#endif
#endif // HKBASE_HK_THREAD_H


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

//////////////////////////////////////////////////////////////////////
//
// AkDeviceBase.cpp
//
// Device implementation that is common across all IO devices.
// Win32 specific.
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkDeviceBase.h"
#include <AK/Tools/Common/AkAutoLock.h>
#include <AK/Tools/Common/AkMonitorError.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <tchar.h>

using namespace AK;

//-------------------------------------------------------------------
// Defines.
//-------------------------------------------------------------------

#define AK_INFINITE_DEADLINE            (100000)    // 100 seconds.

const AkUInt8  AK_USER_POSITION_SEQUENTIAL  = -1;   // Applies to CAkAutoStmBase::m_uPositionChgIdx: user position is sequential.
const AkUInt16 AK_NO_LOOPING_DATA           = -1;   // Applies to CAkAutoStmBase::m_uLoopedBufferIdx: no data streamed from look-ahead looping region.

#ifdef XBOX360
#define AK_PHYSICALALLOC_IOMEM_BLOCKALIGN   (2048)  // Block align for physical alloc mem pool (IO pool)
#endif


//-------------------------------------------------------------------
// CAkDeviceBase implementation.
//-------------------------------------------------------------------

// Device construction/destruction.
CAkDeviceBase::CAkDeviceBase( )
{
    m_uGranularity          = 0;

    m_hIOThreadStopEvent    = NULL;
    m_hIOThread             = NULL;
    
    // IO thread semaphore.
    m_hStdSem               = NULL;
    m_hAutoSem              = NULL;
    m_cPendingStdStms       = 0;
    m_cRunningAutoStms      = 0;
    m_bDoWaitMemoryChange   = false;
#ifdef _DEBUG
    m_bIsAutoSemSignaled    = false;
#endif

    m_uWaitTime             = INFINITE;
    m_fTargetAutoStmBufferLength  = 0;
    m_dwIdleWaitTime        = 0;

    m_streamIOPoolId        = AK_INVALID_POOL_ID;
#ifndef AK_OPTIMIZED
    m_streamIOPoolSize      = 0;
#endif

#ifdef AK_INSTRUMENT_STM_MGR
	m_pDump					= NULL;
#endif
}

CAkDeviceBase::~CAkDeviceBase( )
{
}

// Init.
AKRESULT CAkDeviceBase::Init( 
    const AkDeviceSettings & in_settings,
    AkDeviceID in_deviceID 
    )
{
    // Verify object pool.
    if( CAkStreamMgr::GetObjPoolID( ) == AK_INVALID_POOL_ID )
	{
        AKASSERT( !"Stream manager object pool was not properly initialized" );
		return AK_Fail;
	}
    if ( in_settings.uGranularity < 1 )
    {
        AKASSERT( !"Invalid I/O granularity" );
        return AK_InvalidParameter;
    }
    if ( in_settings.uIOMemorySize && 
         in_settings.fTargetAutoStmBufferLength < 0 )
    {
        AKASSERT( !"Invalid automatic stream average buffering value" );
        return AK_InvalidParameter;
    }
    m_uGranularity          = in_settings.uGranularity;
    m_fTargetAutoStmBufferLength  = in_settings.fTargetAutoStmBufferLength;
    m_dwIdleWaitTime        = in_settings.dwIdleWaitTime;
    m_deviceID              = in_deviceID;
    
    // Create stream memory pool.
    if ( in_settings.uIOMemorySize > 0 )
    {		
#ifdef WIN32

		m_streamIOPoolId = AK::MemoryMgr::CreatePool( 
            NULL,
			in_settings.uIOMemorySize,
			in_settings.uGranularity,
			AkVirtualAlloc | AkFixedSizeBlocksMode,
            0 );
        									        
#elif defined (XBOX360)

		m_streamIOPoolId = AK::MemoryMgr::CreatePool( 
            NULL,
			in_settings.uIOMemorySize,
			in_settings.uGranularity,
			AkPhysicalAlloc | AkFixedSizeBlocksMode,
			AK_PHYSICALALLOC_IOMEM_BLOCKALIGN );
													
#else
#error Platform not supported
#endif
        
    }

    if( m_streamIOPoolId != AK_INVALID_POOL_ID )
	{
        // This pool must not send error notifications: not having memory is normal and notifications are costly.
        AK::MemoryMgr::SetMonitoring(
            m_streamIOPoolId,
            false );
        AK_SETPOOLNAME( m_streamIOPoolId, L"Stream I/O" );
	}
    else if ( in_settings.uIOMemorySize > 0 )
    {
        AKASSERT( !"Cannot create stream pool" );
		return AK_Fail;
    }
    // otherwise, device does not support automatic streams.

#ifndef AK_OPTIMIZED
    m_streamIOPoolSize  = in_settings.uIOMemorySize;

    m_bIsMonitoring     = false;
    m_uNextStreamID     = 0;
    m_bIsNew            = true;
#endif

#ifdef AK_INSTRUMENT_STM_MGR
	m_pDump = _tfopen( L"C:\\AK_StmDump.log", L"w" );
	if ( !m_pDump )
	{
		AKASSERT( !"Failed creating instrumentation file" );
		return AK_Fail;
	}
#endif

    return CreateScheduler( in_settings.pThreadProperties );
}

// Destroy.
void CAkDeviceBase::Destroy()
{
#ifdef AK_INSTRUMENT_STM_MGR
	if ( m_pDump )
		fclose( m_pDump );
#endif

#ifndef AK_OPTIMIZED
    m_arStreamProfiles.Term( );
#endif
    DestroyScheduler( );

    // Destroy IO pool.
    if ( m_streamIOPoolId != AK_INVALID_POOL_ID )
        AKVERIFY( AK::MemoryMgr::DestroyPool( m_streamIOPoolId ) == AK_Success );
    m_streamIOPoolId = AK_INVALID_POOL_ID;

    AkDelete( CAkStreamMgr::GetObjPoolID( ), this );
}

// Device ID. Virtual method defined in IAkDevice.
AkDeviceID CAkDeviceBase::GetDeviceID( )
{
    return m_deviceID;
}


// Init/term scheduler objects.
AKRESULT CAkDeviceBase::CreateScheduler( 
    AkThreadProperties * in_pThreadProperties // Platform-specific thread properties. Can be NULL (uses default).
    )
{
    // Create scheduler semaphore.
    m_hStdSem = ::CreateEvent( NULL,
                               TRUE,    // Manual reset.
                               FALSE,   // Initially not signaled.
                               NULL );
    m_cPendingStdStms = 0;
    
    m_hAutoSem = ::CreateEvent( NULL,
                                TRUE,    // Manual reset.
                                FALSE,   // Initially not signaled.
                                NULL );
    m_cRunningAutoStms = 0;
    
    // Create stop event.
    m_hIOThreadStopEvent = ::CreateEvent( NULL,
                                          FALSE,  // Automatic reset
                                          FALSE,  // Initially non signaled
                                          NULL    // No name
                                          );

    // Launch the scheduler/IO thread.
    // Create and start the worker IO thread with default stack size.
	AKPLATFORM::AkCreateThread( CAkDeviceBase::IOSchedThread, 
								 this, 
								 in_pThreadProperties,
								 &m_hIOThread,
								 "AK::IOThread" );
    
    if ( !AKPLATFORM::AkIsValidThread(&m_hIOThread) ||
		 !m_hIOThreadStopEvent || 
         !m_hStdSem || 
         !m_hAutoSem )
    {
        return AK_Fail;
    }

    return AK_Success;
}

void CAkDeviceBase::DestroyScheduler( )
{
    // If it exists, signal stop event.
    if ( m_hIOThreadStopEvent )
    {
        AKVERIFY( ::SetEvent( m_hIOThreadStopEvent ) );

        // Wait until thread stops.
        if ( m_hIOThread )
        {
            DWORD dwWait = ::WaitForSingleObject( m_hIOThread, 3000 );  // 3 secs.
            if ( dwWait == WAIT_TIMEOUT )
            {
#ifdef WIN32
				// Thread would not stop. Kill it.
		        AKVERIFY( ::TerminateThread( m_hIOThread, AK_RETURN_THREAD_ERROR ) );
#endif
                AKASSERT( !"I/O thread did not properly terminate" );                
            }
            // Close events and thread handles.
            AKVERIFY( ::CloseHandle( m_hIOThread ) );
            m_hIOThread = NULL;
        }
        AKASSERT( m_hIOThreadStopEvent );
        AKVERIFY( ::CloseHandle( m_hIOThreadStopEvent ) );
        m_hIOThreadStopEvent = NULL;
    }
    // Destroy thread handle if it exists.
    if ( m_hIOThread )
    {
        ::CloseHandle( m_hIOThread );
        m_hIOThread = NULL;
    }
    // Destroy scheduler semaphore.
    if ( m_hStdSem )
    {
        ::CloseHandle( m_hStdSem );
        m_hStdSem = NULL;
    }
    m_cPendingStdStms = 0;
    if ( m_hAutoSem )
    {
        ::CloseHandle( m_hAutoSem );
        m_hAutoSem = NULL;
    }
    m_cRunningAutoStms = 0;
}


// Stream creation interface.
// --------------------------------------------------------

// Standard stream.
AKRESULT CAkDeviceBase::CreateStd(
    AkFileDesc &		in_fileDesc,        // Application defined ID.
    AkOpenMode          in_eOpenMode,       // Open mode (read, write, ...).
    IAkStdStream *&     out_pStream         // Returned interface to a standard stream.    
    )
{
    out_pStream = NULL;
    
    AKRESULT eResult = AK_Fail;

    CAkStdStmBase * pNewStm = NULL;
    pNewStm = AkNew( CAkStreamMgr::GetObjPoolID( ), CAkStdStmBase( ) );
    if ( pNewStm != NULL )
    {
        eResult = pNewStm->Init( this, 
                                 in_fileDesc, 
                                 in_eOpenMode );
    }
    else
        eResult = AK_InsufficientMemory;

    if ( eResult == AK_Success )
        eResult = AddTask( pNewStm );
    // --------------------------------------------------------
    // Failed. Clean up.
    // --------------------------------------------------------
    if ( eResult != AK_Success )
    {
    	if ( pNewStm != NULL )
       	{
       		pNewStm->Term( );
        	AkDelete( CAkStreamMgr::GetObjPoolID( ), pNewStm );
        }
        else 
            CAkStreamMgr::GetLowLevel( )->Close( in_fileDesc );
    }
    else
    	out_pStream = pNewStm;

    return eResult;
}

// Automatic stream
AKRESULT CAkDeviceBase::CreateAuto(
    AkFileDesc &				in_fileDesc,        // Application defined ID.
    const AkAutoStmHeuristics & in_heuristics,      // Streaming heuristics.
    AkAutoStmBufSettings *      in_pBufferSettings, // Stream buffer settings. Pass NULL to use defaults (recommended).
    IAkAutoStream *&            out_pStream         // Returned interface to an automatic stream.
    )
{
    AKASSERT( in_heuristics.fThroughput >= 0 &&
              in_heuristics.priority >= AK_MIN_PRIORITY &&
              in_heuristics.priority <= AK_MAX_PRIORITY );

    out_pStream = NULL;
    
	if ( m_streamIOPoolId == AK_INVALID_POOL_ID )
    {
        AKASSERT( !"Streaming pool does not exist: cannot create automatic stream" );
		AK_MONITOR_ERROR( AK::Monitor::ErrorCode_CannotStartStreamNoMemory );
        return AK_Fail;
    }

    CAkAutoStmBase * pNewStm = NULL;

    // Instantiate new stream object.
    AKRESULT eResult;
	pNewStm = AkNew( CAkStreamMgr::GetObjPoolID( ), CAkAutoStmBase( ) );
	if ( pNewStm != NULL )
	{
		eResult = pNewStm->Init( this,
                                 in_fileDesc,
                                 in_heuristics,
                                 in_pBufferSettings,
                                 m_uGranularity
                                 );
		if ( eResult == AK_Success)
            eResult = AddTask( pNewStm );
	}
	else
	{
		eResult = AK_InsufficientMemory;
	}
	// --------------------------------------------------------
    // Failed. Clean up.
    // --------------------------------------------------------
    if ( eResult != AK_Success )
    {
        if ( pNewStm != NULL )
        {
            pNewStm->Term( );
			AkDelete( CAkStreamMgr::GetObjPoolID( ), pNewStm );
        }
        else 
            CAkStreamMgr::GetLowLevel( )->Close( in_fileDesc );
        out_pStream = NULL;
    }
    else
        out_pStream = pNewStm;

    return eResult;
}

// Destroys all streams remaining to be destroyed.
void CAkDeviceBase::ClearStreams( )
{
    TaskArray::Iterator it = m_arTasks.Begin( );
    while ( it != m_arTasks.End( ) )
    {
        (*it)->InstantDestroy( );
        it = m_arTasks.Erase( it );
    }
    m_arTasks.Term( );
}

// Helper: adds a new task to the list.
// Sync: task list lock.
AKRESULT CAkDeviceBase::AddTask( 
    CAkStmTask * in_pStmTask
    )
{
    AkAutoLock<CAkLock> gate( m_lockTasksList );
    
    if ( m_arTasks.AddLast( in_pStmTask ) == NULL )
    { 
        // Could not add task (not enough memory in objects pool).
        return AK_Fail;
    }

#ifndef AK_OPTIMIZED
    // Compute and assign a new unique stream ID.
    in_pStmTask->SetStreamID(
        ++m_uNextStreamID // Gen stream ID.
        );
#endif

    // Set thread wait time.
    // The scheduler sets it to INFINITE when task list becomes empty.
    m_uWaitTime = m_dwIdleWaitTime;

    return AK_Success;
}


// Scheduler thread.
// --------------------------------------------------------

// The implementation of the I/O scheduler thread.
DWORD WINAPI CAkDeviceBase::IOSchedThread( 
    LPVOID lpParameter 
    )
{
    const AkUInt32 NUM_OBJ_WAIT = 3;    // stop, std semaphore, auto semaphore.

    CAkDeviceBase * pDevice = reinterpret_cast<CAkDeviceBase*>(lpParameter);
    bool bRun = true;
    DWORD dwWaitRes;
    HANDLE pEvents[NUM_OBJ_WAIT];
    pEvents[0] = pDevice->m_hIOThreadStopEvent;
    pEvents[1] = pDevice->m_hStdSem; // Signaled when at least one standard stream task is ready.
    pEvents[2] = pDevice->m_hAutoSem;

    while ( bRun )
    {
        dwWaitRes = ::WaitForMultipleObjects( NUM_OBJ_WAIT,
                                              pEvents,
                                              FALSE,    // !bWaitAll
                                              pDevice->GetIOThreadWaitTime() );
        switch ( dwWaitRes )
        {
        case WAIT_OBJECT_0:
            // Stop
			pDevice->ClearStreams( );
            bRun = false;
            break;
        case WAIT_TIMEOUT:      // Idle throughput
        case WAIT_OBJECT_0+1:   // Standard streams semaphore
        case WAIT_OBJECT_0+2:   // Automatic streams semaphore
            // Schedule.
            pDevice->PerformIO( );
            break;
        default:
            AKASSERT( !"Fatal error in I/O device thread" );
            return AK_RETURN_THREAD_ERROR;

        }
    }
    return AK_RETURN_THREAD_OK;
}

// Scheduler algorithm.
// Finds the next task for which an I/O request should be issued.
// Return: If a task is found, a valid pointer to a task is returned, as well
// as the address in/from which to perform a data transfer.
// Otherwise, returns NULL.
// Sync: 
// 1. Locks task list ("scheduler lock").
// 2. If it chooses a standard stream task, the stream becomes "I/O locked" before the scheduler lock is released.
CAkStmTask * CAkDeviceBase::SchedulerFindNextTask( 
    void *& out_pBuffer     // Returned I/O buffer.
    )
{
    // Start scheduling.
    // ------------------------------------

    // Lock tasks list.
    AkAutoLock<CAkLock> scheduling( m_lockTasksList );

    // Stamp time.
    AKPLATFORM::PerformanceCounter( &m_time );

    // If m_bDoWaitMemoryChange, no automatic stream operation can be scheduled because memory is full
    // and will not be reassigned until someone calls NotifyMemChange().
    // Therefore, we only look for a pending standard stream (too bad if memory is freed in the meantime).
    if ( m_bDoWaitMemoryChange )
        return ScheduleStdStmOnly( out_pBuffer );

    TaskArray::Iterator it = m_arTasks.Begin( );
    CAkStmTask * pTask, * pMostBufferedTask;
    pTask = NULL;
    
    // Get first valid task's values for comparison.
    while ( it != m_arTasks.End( ) )
    {
		// Verify that we can perform I/O on this one.
        if ( (*it)->IsToBeDestroyed( ) )
        {
            (*it)->InstantDestroy( );   // Clean up.
            it = m_arTasks.EraseSwap( it );
        }
        else if ( !(*it)->IsWaitingForIO( ) )
            ++it;   // Skip this one.
        else
        {
            // Current iterator refers to a task that is not scheduled to be destroyed, and is waiting for I/O. Proceed.
            pTask = pMostBufferedTask = (*it);
            break;  
        }
    }    

    if ( !pTask )
    {
        // No task was ready for I/O. Leave.
        
        // If task list is empty, reset thread wait time to INFINITE. 
        if ( m_arTasks.IsEmpty( ) )
            m_uWaitTime = INFINITE;
        return NULL;
    }


    // pTask and fSmallestDeadline are the current task and associated effective deadline chosen for I/O.
    // pTaskSlowest and fGreatestDeadline are used to determine which task is going to give a buffer away
    // if we lack memory.
    AkReal32 fSmallestDeadline, fGreatestDeadline;
    fSmallestDeadline = fGreatestDeadline = pTask->EffectiveDeadline( );

    // Find task with smallest effective deadline.
    // If a task has a deadline equal to 0, this means we are starving; user throughtput is greater than
    // low-level bandwidth. In that situation, starving streams are chosen according to their priority.
    // If more than one starving stream has the same priority, the scheduler chooses the one that has been 
    // waiting for I/O for the longest time.
    // Note 1: This scheduler does not have any idea of the actual low-level throughput, nor does it try to
    // know it. It just reacts to the status of its streams at a given moment.
    // Note 2: By choosing the highest priority stream only when we encounter starvation, we take the bet
    // that the transfer will complete before the user has time consuming its data. Therefore it remains 
    // possible that high priority streams starve.
    // Note 3: Automatic streams that just started are considered starving. They are chosen according to
    // their priority first, in a round robin fashion (starving mechanism).
    // Note 4: If starving mode lasts for a long time, low-priority streams will stop being chosen for I/O.

    // Start with the following task.
    AKASSERT( pTask && pMostBufferedTask );
    ++it;

    while ( it != m_arTasks.End( ) )
    {
        // Verify that we can perform I/O on this one.
        if ( (*it)->IsToBeDestroyed( ) )
        {
            (*it)->InstantDestroy( );   // Clean up.
            it = m_arTasks.EraseSwap( it );
        }
        else if ( (*it)->IsWaitingForIO( ) )
        {
            AkReal32 fDeadline = (*it)->EffectiveDeadline();

            if ( fDeadline == 0 )
            {
                // Deadline is zero: starvation mode.
                // Choose task with highest priority among those that are starving.
                if ( (*it)->Priority( ) > pTask->Priority( ) || fSmallestDeadline > 0 )
                {
                    pTask = (*it);
                    fSmallestDeadline = fDeadline;
                }
                else if ( (*it)->Priority( ) == pTask->Priority( ) )
                {
                    // Same priority: choose the one that has waited the most.
                    if ( (*it)->TimeSinceLastTransfer( GetTime( ) ) > pTask->TimeSinceLastTransfer( GetTime( ) ) )
                    {
                        pTask = (*it);
                        fSmallestDeadline = fDeadline;
                    }
                }
            }
            else if ( fDeadline < fSmallestDeadline )
            {
                // Deadline is not zero: low-level has enough bandwidth. Just take the task with smallest deadline.
                // We take the bet that this transfer will have time to occur fast enough to properly service
                // the others on next pass.
                pTask = (*it);
                fSmallestDeadline = fDeadline;
            }

            // Keep track of automatic stream tasks that have a great deadline, for eventual buffer reassignment.
            if ( (*it)->StmType( ) == AK_StmTypeAutomatic &&
                 fDeadline > fGreatestDeadline )
            {
                fGreatestDeadline = fDeadline;
                pMostBufferedTask = (*it);
            }

            ++it;
        }
        else
            ++it;   // Skip this task: it is not waiting for I/O.
    }
    
    // Standard streams:
    // ------------------------------
    if ( pTask->StmType( ) == AK_StmTypeStandard )
    {
        // Standard streams may not return a buffer if the operation was cancelled while we were scheduling.
        // IMPORTANT: If this method succeeds (returns a buffer), the task will lock itself for I/O (AkStdStmBase::m_lockIO).
        // All operations that need to wait for I/O to complete block on that lock.
        out_pBuffer = pTask->TryGetIOBuffer( );
        if ( !out_pBuffer )
            return NULL;    // Task cancelled or destroyed by user. Return NULL to cancel I/O.
        return pTask;
    }
    
    // Automatic streams:
    // ------------------------------

    // Automatic streams' TryGetIOBuffer() must be m_lockAutoSems protected, because it tries to allocate a 
    // buffer from the memory manager.
    // If it fails, and we decide not to reassign a buffer from another automatic stream, the automatic 
    // streams semaphore should be notified as "memory idle" (inhibates the semaphore). 
    // Memory allocation and semaphore inhibition must be atomic, in case someone freed memory in the meantime.
    // Since most of the time this situation does not happen, we first try getting memory without locking.

    // Try allocate a buffer.
	m_lockAutoSems.Lock( );
    out_pBuffer = pTask->TryGetIOBuffer( ); 
	m_lockAutoSems.Unlock( );
    if ( !out_pBuffer )
    {
        // No buffer is available. Remove a buffer from the most buffered task unless one of the following 
        // conditions is met:
        // - The most buffered task is also the one that was chosen for I/O.
        // - Its deadline is greater than the target buffering length.

        // If we decide not to remove a buffer, inhibate automatic stream semaphore (NotifyMemIdle).

        if ( pTask != pMostBufferedTask &&
             fGreatestDeadline > m_fTargetAutoStmBufferLength )
        {
            // Remove a buffer from the most buffered task.
            // Note 1. PopIOBuffer() does not free memory, it just removes a buffer from its table and passes it back. 
            // Note 2. Bad citizens could be harmful to this algorithm. A stream could decide
            // not to give a buffer just because its user owns too much at a time. Technically, we could seek the next
            // "most buffered" stream, but we prefer not.
            // Note 3. If PopIOBuffer() fails returning a buffer, we must call NotifyMemIdle(). However, since PopIOBuffer()
            // needs to lock its status to manipulate its array of buffers, NotifyMemIdle() is called from within to
            // avoid potential deadlocks.
            
            out_pBuffer = pMostBufferedTask->PopIOBuffer( );
            if ( !out_pBuffer )
            {
                // The stream would not let go one of its buffers. Sleep (NotifyMemIdle() was called from PopIOBuffer()).
                return NULL;
            }
        }
        else
        {
            // We decided not to reassign a buffer. Sleep until a buffer is freed.
            // IMPORTANT: However, allocation and notification must be atomic, otherwise the scheduler could erroneously
            // think that there is no memory in the case a buffer was freed after our failed attempt to allocate one.
            // Therefore, we lock, try to allocate again, perform I/O if it succeeds, notify that memory is idle otherwise.
            m_lockAutoSems.Lock( );
            out_pBuffer = pTask->TryGetIOBuffer( ); 
            if ( !out_pBuffer )
                NotifyMemIdle( );

            m_lockAutoSems.Unlock( );

            // Starting now, automatic streams will not trigger I/O thread awakening until a change occurs with memory.
            // Pending standard streams will continue notifying, so the thread might come back and execute a
            // standard stream. 
            if ( !out_pBuffer )
                return NULL;
            return pTask;
        }
    }

    // Task remains locked for I/O.
    return pTask;
}

// Scheduler algorithm: standard stream-only version.
// Finds next task among standard streams only (typically when there is no more memory).
CAkStmTask * CAkDeviceBase::ScheduleStdStmOnly(
    void *& out_pBuffer     // Returned I/O buffer.
    )
{
    TaskArray::Iterator it = m_arTasks.Begin( );
    CAkStmTask * pTask = NULL;
    
    // Get first valid task's values for comparison.
    while ( it != m_arTasks.End( ) )
    {
        // Verify that we can perform I/O on this one.
        if ( (*it)->IsToBeDestroyed( ) )
        {
            (*it)->InstantDestroy( );   // Clean up.
            it = m_arTasks.EraseSwap( it );
        }
        else if ( (*it)->StmType( ) == AK_StmTypeStandard &&
                    (*it)->IsWaitingForIO( ) )
        {
            // Current iterator refers to a standard stream task that is not scheduled to be destroyed, 
            // and that is pending. Proceed.
            pTask = (*it);
            break;
        }
        else
            ++it;
    }    

    if ( !pTask )
    {
        // No task was ready for I/O. Leave.
        
        // If task list is empty, reset thread wait time to INFINITE. 
        if ( m_arTasks.IsEmpty( ) )
            m_uWaitTime = INFINITE;

        return NULL;
    }

    // fSmallestDeadline is the smallest effective deadline found to date. Used to find the next task for I/O.
    AkReal32 fSmallestDeadline = pTask->EffectiveDeadline( );
    
    // Find task with smallest effective deadline.
    // See note in SchedulerFindNextTask(). It is the same algorithm, except that automatic streams are excluded.
    
    // Start with the following task.
    AKASSERT( pTask );
    ++it;

    while ( it != m_arTasks.End( ) )
    {
        // Verify that we can perform I/O on this one.
        if ( (*it)->IsToBeDestroyed( ) )
        {
            (*it)->InstantDestroy( );   // Clean up.
            it = m_arTasks.EraseSwap( it );
        }
        else if ( (*it)->StmType( ) == AK_StmTypeStandard &&
                    (*it)->IsWaitingForIO( ) )
        {
            AkReal32 fDeadline = (*it)->EffectiveDeadline(); 

            if ( fDeadline == 0 )
            {
                // Deadline is zero. Starvation mode: user throughput is greater than low-level bandwidth.
                // Choose task with highest priority among those that are starving.
                if ( (*it)->Priority( ) > pTask->Priority( ) || fSmallestDeadline > 0 )
                {
                    pTask = (*it);
                    fSmallestDeadline = fDeadline;
                }
                else if ( (*it)->Priority( ) == pTask->Priority( ) )
                {
                    // Same priority: choose the one that has waited the most.
                    if ( (*it)->TimeSinceLastTransfer( GetTime() ) > pTask->TimeSinceLastTransfer( GetTime() ) )
                    {
                        pTask = (*it);
                        fSmallestDeadline = fDeadline;
                    }
                }
            }
            else if ( fDeadline < fSmallestDeadline )
            {
                // Deadline is not zero: low-level has enough bandwidth. Just take the task with smallest deadline.
                // We take the bet that this transfer will have time to occur fast enough to properly service
                // the others.
                pTask = (*it);
                fSmallestDeadline = fDeadline;
            }
            ++it;
        }
        else
            ++it;   // Skip this task; not waiting for I/O.
    }

    // Lock task.
    AKASSERT( pTask );
    
    // IMPORTANT: If this method succeeds (returns a buffer), the task will lock itself for I/O (AkStdStmBase::m_lockIO).
    // All operations that need to wait for I/O to complete block on that lock.
    out_pBuffer = pTask->TryGetIOBuffer( ); 
    if ( !out_pBuffer )
        return NULL;    // Task cancelled or destroyed by user. Return NULL to cancel I/O.
    return pTask;

}


//
// Methods used by stream objects: 
// Scheduler thread control.
// -----------------------------------

// Increment pending standard streams count.
// Sync: Standard streams semaphore interlock.
void CAkDeviceBase::StdSemIncr()
{
    if ( AKPLATFORM::AkInterlockedIncrement( &m_cPendingStdStms ) == 1 )
	{
		// We just incremented it from 0 to 1. Signal event.
        ::SetEvent( m_hStdSem );
	}
}

// Decrement pending standard streams count.
// Sync: Standard streams semaphore interlock.
void CAkDeviceBase::StdSemDecr()
{
    if ( AKPLATFORM::AkInterlockedDecrement( &m_cPendingStdStms ) == 0 )
	{
        // We just decremented it from 1 to 0. Reset event.
        ::ResetEvent( m_hStdSem );
	}

}

// Increment pending automatic streams count.
// Note: Event remains unsignaled if there is not memory ("Mem Idle" state).
// Sync: Automatic streams semaphore lock.
void CAkDeviceBase::AutoSemIncr()
{
    AkAutoLock<CAkLock> gate(m_lockAutoSems);
    m_cRunningAutoStms++;
    if ( m_cRunningAutoStms == 1 &&
         !m_bDoWaitMemoryChange )
    {
        // We just incremented it from 0 to 1 and memory is available. Signal event.
#ifdef _DEBUG
        m_bIsAutoSemSignaled = true;
#endif
        ::SetEvent( m_hAutoSem );
    }
}

// Decrement pending automatic streams count.
// Sync: Automatic streams semaphore lock.
void CAkDeviceBase::AutoSemDecr()
{
    AkAutoLock<CAkLock> gate(m_lockAutoSems);
    AKASSERT( m_cRunningAutoStms > 0 );
    m_cRunningAutoStms--;    
    if ( m_cRunningAutoStms == 0 )
    {
        // We just decremented it from 1 to 0. Reset event.
#ifdef _DEBUG
        m_bIsAutoSemSignaled = false;
#endif
        ::ResetEvent( m_hAutoSem );
    }
}

// Notify that memory was freed, or memory usage must be reviewed.
// Un-inhibates automatic streams semaphore. Event is signaled if pending automatic streams count is greater than 0.
// IMPORTANT Sync: None. Locking must be handled on the caller side, to enclose calls to Memory Manager
// and protect automatic streams semaphore. Tasks use LockMem().
void CAkDeviceBase::NotifyMemChange()
{
    if ( m_bDoWaitMemoryChange )
    {
        m_bDoWaitMemoryChange = false;
        if ( m_cRunningAutoStms > 0 )
        {
            // Auto streams are running and we just notified that some memory is available. Signal event.
#ifdef _DEBUG
            m_bIsAutoSemSignaled = true;
#endif
            ::SetEvent( m_hAutoSem );
        }
    }
}

// Notify that memory is idle. I/O thread should not wake up to service automatic streams until memory usage
// changes (until someone calls NotifyMemChange).
// Inhibates automatic streams semaphore.
// IMPORTANT Sync: None. Locking must be handled on the caller side, to enclose calls to Memory Manager
// and protect automatic streams semaphore. Tasks use LockMem().
void CAkDeviceBase::NotifyMemIdle()
{
    m_bDoWaitMemoryChange = true;
    if ( m_cRunningAutoStms > 0 )
    {
        // Auto streams are running but we just notified that memory is not available. Reset event.
#ifdef _DEBUG
        m_bIsAutoSemSignaled = false;
#endif
        ::ResetEvent( m_hAutoSem );
    }
}

// Lock/unlock memory. Must enclose atomic calls to the Memory Manager and memory notifications.
void CAkDeviceBase::LockMem()
{
    m_lockAutoSems.Lock( );
}
void CAkDeviceBase::UnlockMem()
{
    m_lockAutoSems.Unlock( );
}

// Device Profile Ex interface.
// --------------------------------------------------------
#ifndef AK_OPTIMIZED

// Caps/desc.
void CAkDeviceBase::GetDesc( 
    AkDeviceDesc & out_deviceDesc 
    )
{
    AKVERIFY( CAkStreamMgr::GetLowLevel( )->GetDeviceDesc( m_deviceID, out_deviceDesc ) == AK_Success );
}

bool CAkDeviceBase::IsNew( )
{
    return m_bIsNew;
}

void CAkDeviceBase::ClearNew( )
{
    m_bIsNew = false;
}


AKRESULT CAkDeviceBase::StartMonitoring( )
{
    m_bIsMonitoring = true;
    return AK_Success;
}

void CAkDeviceBase::StopMonitoring( )
{
    m_bIsMonitoring = false;
}

// Stream profiling: GetNumStreams.
// Clears profiling array.
// Inspects all streams. 
// Grants permission to destroy if scheduled for destruction and AND not new.
// Copies all stream profile interfaces into its array, that will be accessed
// by IAkStreamProfile methods.
AkUInt32 CAkDeviceBase::GetNumStreams( )
{
    m_arStreamProfiles.RemoveAll( );

    AkAutoLock<CAkLock> gate( m_lockTasksList );

    TaskArray::Iterator it = m_arTasks.Begin( );
    while ( it != m_arTasks.End( ) )
    {
        // If it is scheduled for destruction and it is not new, allow destruction.
        if ( (*it)->ProfileIsToBeDestroyed( ) && 
             !(*it)->IsProfileNew( ) )
        {
            (*it)->ProfileAllowDestruction( );
        }
        else
        {
            // Copy into profiler list.
            m_arStreamProfiles.AddLast( (*it)->GetStreamProfile( ) );
        }

        ++it;
    }
    return m_arStreamProfiles.Length( );
}

// Note. The following functions refer to streams by index, which must honor the call to GetNumStreams().
AK::IAkStreamProfile * CAkDeviceBase::GetStreamProfile( 
    AkUInt32    in_uStreamIndex             // [0,numStreams[
    )
{
    // Get stream profile and return.
    return m_arStreamProfiles[in_uStreamIndex];
}
#endif


//-----------------------------------------------------------------------------
//
// Stream objects implementation.
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Name: class CAkStmTask
// Desc: Base implementation common to streams. Defines the interface used by
//       the device scheduler.
//-----------------------------------------------------------------------------

CAkStmTask::CAkStmTask()
{
    // Profiling.
#ifndef AK_OPTIMIZED
    //Not set m_ulStreamID;
    m_bIsNew            = true;
    m_bIsProfileDestructionAllowed  = false;
    m_uBytesTransfered  = 0;
#endif

    // Other members are initialized in derived classes' Init().
}

CAkStmTask::~CAkStmTask()
{
}

#ifndef AK_OPTIMIZED
// Profiling: Get stream information. This information should be queried only once, since it is unlikely to change.
void CAkStmTask::GetStreamRecord( 
    AkStreamRecord & out_streamRecord
    )
{
    out_streamRecord.bIsAutoStream = true;
    out_streamRecord.deviceID = m_pDevice->GetDeviceID( );
    if ( m_pszStreamName != NULL )
    {
    	AkUInt32 uStringSize = (AkUInt32)_tcslen( m_pszStreamName )+1;
        out_streamRecord.uStringSize = AkMin( uStringSize, AK_MONITOR_STREAMNAME_MAXLENGTH );
        AKPLATFORM::AkMemCpy( out_streamRecord.szStreamName, m_pszStreamName, sizeof(AkTChar)*out_streamRecord.uStringSize );    
        out_streamRecord.szStreamName[AK_MONITOR_STREAMNAME_MAXLENGTH-1] = 0;
    }
    else
    {
        out_streamRecord.uStringSize = 0;    
        out_streamRecord.szStreamName[0] = NULL;
    }
    out_streamRecord.uFileSize = m_fileDesc.iFileSize;
    out_streamRecord.uStreamID = m_uStreamID;
}
#endif

//-----------------------------------------------------------------------------
// Name: class CAkStdStmBase
// Desc: Standard stream base implementation.
//-----------------------------------------------------------------------------

CAkStdStmBase::CAkStdStmBase( )
{
    m_pszStreamName = NULL;
    m_eStmType      = AK_StmTypeStandard;
}

CAkStdStmBase::~CAkStdStmBase( )
{
}

// Init.
// Sync: None.
AKRESULT CAkStdStmBase::Init(
    CAkDeviceBase *     in_pDevice,         // Owner device.
    const AkFileDesc &  in_fileDesc,        // File descriptor.
    AkOpenMode          in_eOpenMode        // Open mode.
    )
{
    AKASSERT( in_pDevice != NULL );

    m_pDevice           = in_pDevice;

    // Copy file descriptor.
    m_fileDesc          = in_fileDesc;
    if ( m_fileDesc.iFileSize < 0 )
    {
        AKASSERT( !"Invalid file size" );
        return AK_InvalidParameter;
    }
    
    m_iCurPosition = 0;
    // Notify low-level IO if file descriptor is sector based and offset is not null.
    if ( m_fileDesc.uSector > 0 )
        m_bIsPositionDirty       = true;
    else
        m_bIsPositionDirty       = false;
    
    m_bHasReachedEof    = false;
    
    m_eOpenMode         = in_eOpenMode;
    AKASSERT( m_pszStreamName == NULL );

    m_eStmStatus        = AK_StmStatusIdle;
    m_bIsWriteOp        = false;
    m_pBuffer           = NULL;
    m_uBufferSize       = 0;
    m_uActualSize       = 0;

    m_uLLBlockSize      = CAkStreamMgr::GetLowLevel( )->GetBlockSize( in_fileDesc );
    if ( !m_uLLBlockSize )
    {
        AKASSERT( !"Invalid Low-Level I/O block size. Must be >= 1" );
        return AK_Fail;
    }

    m_hBlockEvent       = NULL;

    m_bIsToBeDestroyed  = false;

    m_overlapped.ClearPosition();

    return AK_Success;
}

// Term (called at stream's destruction).
// Sync: None.
void CAkStdStmBase::Term()
{
    // If the status is pending, it is actually "Pending to be cleaned up"
    // (see Destroy()), and the semaphore must be decremented now.
    if ( m_eStmStatus == AK_StmStatusPending )
        m_pDevice->StdSemDecr( );

    // Low-Level IO clean-up.
    AKVERIFY( CAkStreamMgr::GetLowLevel( )->Close( m_fileDesc ) == AK_Success );

    if ( m_pszStreamName )
    {
        AkFree( CAkStreamMgr::GetObjPoolID( ), m_pszStreamName );
        m_pszStreamName = NULL;
    }
    AKASSERT( m_hBlockEvent == NULL );
}

//-----------------------------------------------------------------------------
// IAkStdStream interface.
//-----------------------------------------------------------------------------

// Destruction. The object is destroyed and the interface becomes invalid.
// Sync: 
// 1. Lock scheduler. Streams lock the scheduler to change their flag m_bIsToBeDestroyed safely.
//      This is better than having to lock each stream's status at every scheduler pass.
// 2. If I/O is pending, lock IO before changing status (Cancel()).
void CAkStdStmBase::Destroy()
{
    m_pDevice->LockScheduler( );

    // If an operation is pending, the scheduler might be executing it. This method must not return until it 
    // is complete: lock I/O for this task.

    // Allow destruction.
    m_bIsToBeDestroyed = true;   

    // If not pending, will not be later. The opposite is not true.
    if ( m_eStmStatus == AK_StmStatusPending )
    {
        // Block if a data transfer is being executed.
        AkAutoLock<CAkLock> cancelIO( m_lockIO );
    }

    m_pDevice->UnlockScheduler( );
}

// Stream info access.
// Sync: None.
void CAkStdStmBase::GetInfo(
    AkStreamInfo & out_info       // Returned stream info.
    )
{
    AKASSERT( m_pDevice != NULL );
    out_info.deviceID     = m_pDevice->GetDeviceID( );
    out_info.pszName      = m_pszStreamName;
    out_info.uSize		= m_fileDesc.iFileSize;
}

// Name the stream (appears in Wwise profiler).
// Sync: None
AKRESULT CAkStdStmBase::SetStreamName(
    AkLpCtstr       in_pszStreamName    // Stream name.
    )
{
    if ( m_pszStreamName != NULL )
        AkFree( CAkStreamMgr::GetObjPoolID( ), m_pszStreamName );

    if ( in_pszStreamName != NULL )
    {
        // Allocate string buffer for user defined stream name.
        m_pszStreamName = (AkTChar*)AkAlloc( CAkStreamMgr::GetObjPoolID( ), (AkUInt32)sizeof(AkTChar)*( wcslen( in_pszStreamName ) + 1 ) );
        if ( m_pszStreamName == NULL )
            return AK_InsufficientMemory;

        // Copy.
        wcscpy( m_pszStreamName, in_pszStreamName );
    }
    return AK_Success;
}

// Get low-level block size for this stream.
// Returns block size for optimal/unbuffered IO.
AkUInt32 CAkStdStmBase::GetBlockSize( )
{
    return m_uLLBlockSize;
}

// Get stream position.
// Sync: None. 
// Users should not call this when pending.
AkUInt64 CAkStdStmBase::GetPosition( 
    bool * out_pbEndOfStream   // Input streams only. Can be NULL.
    )   
{
    AKASSERT( m_eStmStatus != AK_StmStatusPending ||
              !"Inaccurate stream position when operation is pending" );
    if ( out_pbEndOfStream != NULL )
    {
        *out_pbEndOfStream = m_bHasReachedEof && !m_bIsPositionDirty;
    }
    AkUInt64 uCurPosition;
    uCurPosition = m_iCurPosition;
    return uCurPosition;
}

// Operations.
// ------------------------------------------

// Set stream position. Modifies position of next read/write.
// Sync: 
// Fails if an operation is pending.
AKRESULT CAkStdStmBase::SetPosition(
    AkInt64         in_iMoveOffset,     // Seek offset.
    AkMoveMethod    in_eMoveMethod,     // Seek method, from beginning, end or current file position.
    AkInt64 *       out_piRealOffset    // Actual seek offset may differ from expected value when unbuffered IO. 
                                        // In that case, floors to sector boundary. Pass NULL if don't care.
    )
{
    if ( out_piRealOffset != NULL )
    {
        *out_piRealOffset = 0;
    }

    // Safe status.
    if ( m_eStmStatus == AK_StmStatusPending )
    {
        AKASSERT( !"Trying to change stream position while standard IO is pending" );
        return AK_Fail;
    }

    // Compute absolute position.
    AkInt64 iPosition;
    if ( in_eMoveMethod == AK_MoveBegin )
    {
        iPosition = in_iMoveOffset;
    }
    else if ( in_eMoveMethod == AK_MoveCurrent )
    {
        iPosition = m_iCurPosition + in_iMoveOffset;
    }
    else if ( in_eMoveMethod == AK_MoveEnd )
    {
        iPosition = m_fileDesc.iFileSize + in_iMoveOffset;
    }
    else
    {
        AKASSERT( !"Invalid move method" );
        return AK_InvalidParameter;
    }

    if ( iPosition < 0 )
    {
        AKASSERT( !"Trying to move the file pointer before the beginning of the file" );
        return AK_InvalidParameter;
    }

    // Round offset to block size.
    if ( iPosition % m_uLLBlockSize != 0 )
    {
        // Snap to lower boundary.
        iPosition -= ( iPosition % m_uLLBlockSize );
        AKASSERT( iPosition >= 0 );
    }

    // Set real offset if argument specified.
    if ( out_piRealOffset != NULL )
    {
        switch ( in_eMoveMethod )
        {
        case AK_MoveBegin:
            *out_piRealOffset = iPosition;
            break;
        case AK_MoveCurrent:
            *out_piRealOffset = iPosition - m_iCurPosition;
            break;
        case AK_MoveEnd:
            *out_piRealOffset = iPosition - m_fileDesc.iFileSize;
            break;
        default:
            AKASSERT( !"Invalid move method" );
            return AK_Fail;
        }
    }

    // Update position if it changed.
    // Set new file position.
    m_iCurPosition = iPosition;
    // Set position dirty flag.
    m_bIsPositionDirty = true;
    
    return AK_Success;
}

// Read.
// Sync: Returns if task pending. Status change.
AKRESULT CAkStdStmBase::Read(
    void *          in_pBuffer,         // User buffer address. 
    AkUInt32        in_uReqSize,        // Requested read size.
    bool            in_bWait,           // Block until operation is complete.
    AkPriority      in_priority,        // Heuristic: operation priority.
    AkReal32        in_fDeadline,       // Heuristic: operation deadline (s).
    AkUInt32 &      out_uSize           // Size actually read.
    )
{
    out_uSize = 0;

    // Check requested size.
    if ( in_uReqSize == 0 ||
         in_pBuffer == NULL )
    {
        AKASSERT( !"Invalid buffer or requested size" );
        return AK_InvalidParameter;
    }

    // Check heuristics.
    if ( in_priority < AK_MIN_PRIORITY ||
         in_priority > AK_MAX_PRIORITY ||
         in_fDeadline < 0 )
    {
        AKASSERT( !"Invalid heuristics" );
        return AK_InvalidParameter;
    }

    // Check status.
    if ( m_eStmStatus == AK_StmStatusPending ||
         m_eStmStatus == AK_StmStatusError )
    {
        AKASSERT( !"Operation already pending or stream is in error mode" );
        return AK_Fail;
    }

    // Leave right away if reached end of file.
    if ( m_bHasReachedEof && !m_bIsPositionDirty )
    {
        return AK_Success;
    }

    // Verify with block size.
    if ( in_uReqSize % m_uLLBlockSize != 0 )
    {
        AKASSERT( !"Requested size incompatible with Low-Level block size" );
        return AK_Fail;
    }

    // Prepare IO.
    m_uActualSize      = 0;
    m_uBufferSize      = in_uReqSize;
    m_bIsWriteOp       = false;
    m_pBuffer          = in_pBuffer;
    m_priority         = in_priority;
    // Compute the per-transfer deadline value. 
    // Note. Requested size is rounded up to the next integer value of granularity.
    in_uReqSize = ( in_uReqSize + m_pDevice->GetGranularity( ) - 1 ) / m_pDevice->GetGranularity() * m_pDevice->GetGranularity();
    m_fDeadline = in_fDeadline * ( m_pDevice->GetGranularity( ) / (AkReal32)in_uReqSize );

    // Reset time.
    m_iIOStartTime = m_pDevice->GetTime( );
    
    // If blocking, create the blocking event.
    // Note: This implementation is not optimized for blocking I/O.
    if ( in_bWait )
    {
        AKASSERT( m_hBlockEvent == NULL );
        // Note. Event is manual reset in case IO thread had time to execute I/O before we block on it.
        m_hBlockEvent = ::CreateEvent( NULL,    // no security
                                       TRUE,    // manual reset
                                       FALSE,   // unsignaled
                                       NULL );  // no name
        if ( m_hBlockEvent == NULL )
        {
            AKASSERT( !"Failed creating event for blocking IO" );
            return AK_Fail;
        }
    }

    // Set Status. Notify device sheduler.
    SetStatus( AK_StmStatusPending );

    // If blocking, wait for the blocking event.
    if ( in_bWait && m_hBlockEvent )
    {
        AKVERIFY( ::WaitForSingleObject( m_hBlockEvent, INFINITE ) == WAIT_OBJECT_0 );
        ::CloseHandle( m_hBlockEvent );
        m_hBlockEvent = NULL;
    }

    out_uSize = m_uActualSize;
    
    return AK_Success;
}

// Write.
// Sync: Returns if task pending. Changes status.
AKRESULT CAkStdStmBase::Write(
    void *          in_pBuffer,         // User buffer address. 
    AkUInt32        in_uReqSize,        // Requested write size. 
    bool            in_bWait,           // Block until operation is complete.
    AkPriority      in_priority,        // Heuristic: operation priority.
    AkReal32        in_fDeadline,       // Heuristic: operation deadline (s).
    AkUInt32 &      out_uSize           // Size actually written.
    )
{
    out_uSize = 0;

    // Check requested size.
    if ( in_uReqSize == 0 ||
         in_pBuffer == NULL )
    {
        AKASSERT( !"Invalid buffer or requested size" );
        return AK_InvalidParameter;
    }

    // Check heuristics.
    if ( in_priority < AK_MIN_PRIORITY ||
         in_priority > AK_MAX_PRIORITY ||
         in_fDeadline < 0 )
    {
        AKASSERT( !"Invalid heuristics" );
        return AK_InvalidParameter;
    }

    // Check status.
    if ( m_eStmStatus == AK_StmStatusPending ||
         m_eStmStatus == AK_StmStatusError )
    {
        AKASSERT( !"Operation already pending or stream is in error mode" );
        return AK_Fail;
    }

    // Verify with block size.
    if ( in_uReqSize % m_uLLBlockSize != 0 )
    {
        AKASSERT( !"Requested size incompatible with Low-Level block size" );
        return AK_Fail;
    }

    // Prepare IO.
    m_uActualSize      = 0;
    m_uBufferSize      = in_uReqSize;
    m_bIsWriteOp       = true;
    m_pBuffer          = in_pBuffer;
    m_priority         = in_priority;
    // Compute the per-transfer deadline value. 
    // Note. Requested size is rounded up to the next integer value of granularity.
    in_uReqSize = ( in_uReqSize + m_pDevice->GetGranularity( ) - 1 ) / m_pDevice->GetGranularity() * m_pDevice->GetGranularity();
    m_fDeadline = in_fDeadline * ( m_pDevice->GetGranularity( ) / (AkReal32)in_uReqSize );

    // Reset time.
    m_iIOStartTime = m_pDevice->GetTime( );
    
    // If blocking, create the blocking event.
    // Note: This implementation is not optimized for blocking I/O.
    if ( in_bWait )
    {
        AKASSERT( m_hBlockEvent == NULL );
        // Note. Event is manual reset in case IO thread had time to execute IO before we block on it.
        m_hBlockEvent = ::CreateEvent( NULL,    // no security
                                       TRUE,    // manual reset
                                       FALSE,   // unsignaled
                                       NULL );  // no name
        if ( m_hBlockEvent == NULL )
        {
            AKASSERT( !"Failed creating event for blocking IO" );
            return AK_Fail;
        }
    }

    // Set Status. Notify device sheduler.
    SetStatus( AK_StmStatusPending );

    // If blocking, wait for the blocking event.
    if ( in_bWait && m_hBlockEvent )
    {
        AKASSERT( m_hBlockEvent != NULL );
        AKVERIFY( ::WaitForSingleObject( m_hBlockEvent, INFINITE ) == WAIT_OBJECT_0 );
        ::CloseHandle( m_hBlockEvent );
        m_hBlockEvent = NULL;
    }

    out_uSize = m_uActualSize;

    return AK_Success;
}

// Cancel. If Pending, sets its status to Cancelled. Otherwise it returns right away.
// Sync: 
// 1. I/O execution critical section. 
// 2. status lock (inside SetStatus())
void CAkStdStmBase::Cancel( )
{
    // If not pending, will not be later. The opposite is not true.
    if ( m_eStmStatus == AK_StmStatusPending )
    {
        // Block if a data transfer is being executed.
        AkAutoLock<CAkLock> cancelIO( m_lockIO );

        // Set status. Semaphore will be released.
        SetStatus( AK_StmStatusCancelled );
    }
}

// Get data and size.
// Returns address of data. No check for pending I/O.
// Sync: None. Always accurate when I/O not pending.
void * CAkStdStmBase::GetData( 
    AkUInt32 & out_uActualSize   // Size actually read or written.
    )
{
    out_uActualSize = m_uActualSize;
    return m_pBuffer;
}
// Info access.
// Sync: None. Status query.
AkStmStatus CAkStdStmBase::GetStatus( )           // Get operation status.
{
    return m_eStmStatus;
}

//-----------------------------------------------------------------------------
// CAkStmTask virtual methods implementation.
//-----------------------------------------------------------------------------

// Task management.
// Actual, non-deferred Destroy.
// Sync: None. At this point, only the scheduler thread owns the stream object.
void CAkStdStmBase::InstantDestroy( )
{
#ifndef AK_OPTIMIZED
    AKASSERT( m_bIsToBeDestroyed && 
              ( !m_pDevice->IsMonitoring( ) || m_bIsProfileDestructionAllowed ) );
#else
    AKASSERT( m_bIsToBeDestroyed );
#endif

    // Term.
    Term( );

    // Destroys itself.
    AkDelete( CAkStreamMgr::GetObjPoolID( ), this );
}

// Task is waiting for I/O if its status is Pending.
// Sync: None. Status query.
bool CAkStdStmBase::IsWaitingForIO()
{
#ifndef AK_OPTIMIZED
    return (m_eStmStatus == AK_StmStatusPending) && !m_bIsToBeDestroyed;
#else
    return m_eStmStatus == AK_StmStatusPending;
#endif
    
}

// Try get I/O buffer.
// Returns the address supplied by the user, into which the data transfer should be performed.
// Returns NULL if the stream was cancelled.
// Sync: Locks the stream for I/O. 
// IMPORTANT: If it returns a non-null address, the I/O lock remains locked and the scheduler needs to unlock
// it when the data transfer is complete (by calling Update()).
void * CAkStdStmBase::TryGetIOBuffer()
{
    // Lock for I/O now.
    m_lockIO.Lock( );
    if ( !IsWaitingForIO( ) ||
    	 IsToBeDestroyed( ) )
    {
        m_lockIO.Unlock( );
        return NULL;
    }
    AKASSERT( m_pBuffer );
    return (AkUInt8*)m_pBuffer + m_uActualSize;
}

// Transfer info. Returns everything that is needed by the scheduler to perform a data transfer with the
// Low-Level IO. Must be called if and only if this task was chosen.
// Sync: Locks stream's status.
void CAkStdStmBase::TransferInfo( 
    AkFileDesc *& out_pFileDesc,    // Stream's associated file descriptor.
    OVERLAPPED *& out_pOverlapped,  // Asynchronous data and file position.
    bool & out_bIsSequential,       // Returned !m_bNotifyLLNotSequential flag.
    AkUInt32 *  out_puSize          // Required transfer size.
    )
{
    out_pFileDesc = &m_fileDesc;

    // Lock status.
    AkAutoLock<CAkLock> atomicPosition( m_lockStatus );

	// Required transfer size is the buffer size for this stream.
    // Slice request to granularity.
	if ( m_uBufferSize > m_pDevice->GetGranularity( ) )
        *out_puSize = m_pDevice->GetGranularity( );
    else
        *out_puSize = m_uBufferSize;
    
    out_bIsSequential = !m_bIsPositionDirty;
    
    // If position is dirty, set file position to client position.
    if ( m_bIsPositionDirty )
    {
        m_overlapped.Position( m_fileDesc.uSector * m_uLLBlockSize + m_iCurPosition );
        // Reset position dirty flag.
    	m_bIsPositionDirty = false;
    }
    out_pOverlapped = &m_overlapped;

    // Reset timer. Time count since last transfer starts now.
    m_iIOStartTime = m_pDevice->GetTime( );
}

// Update task after data transfer.
// Sync: Locks status. 
// Called from inside I/O critical section. I/O is unlocked here.
void CAkStdStmBase::Update( 
    void *      in_pBuffer,             // Address of data. Ignored (member of this object: must be the same).
    AkUInt32    in_uIOSize,             // Size actually read/written.
    bool        in_bWasIOSuccessful     // IO was successful (no error).
    )
{
    // Buffer returned must be the same than buffer provided by TryGetIOBuffer.
    AKASSERT( m_eStmStatus == AK_StmStatusPending &&
              in_pBuffer == (AkUInt8*)m_pBuffer + m_uActualSize );
    
    // Lock status.
    m_lockStatus.Lock( );
    
	// EOF.
	// Truncate data size if the stream is going to pass the end of file.
    if ( ( m_iCurPosition + in_uIOSize ) >= m_fileDesc.iFileSize
		&& !m_bIsWriteOp )
	{
         in_uIOSize = (AkUInt32)( m_fileDesc.iFileSize - m_iCurPosition );
		 m_bHasReachedEof = true;
	}

	// Update position.
    AKASSERT( in_uIOSize <= m_uBufferSize );
    m_uActualSize += in_uIOSize;
    m_uBufferSize -= in_uIOSize;
    AKASSERT( !m_bIsPositionDirty ); // else, the client called SetPosition while IO was in progress (illegal with std streams).
	m_iCurPosition += in_uIOSize;
    m_overlapped.IncrementPosition( in_uIOSize );

    // Profiling.
#ifndef AK_OPTIMIZED
    m_uBytesTransfered += in_uIOSize;
#endif
    
    // Compute status.
    AkStmStatus eStatus;
    if ( !in_bWasIOSuccessful )
    {
        // Unsignals semaphore.
        eStatus = AK_StmStatusError;
    }
    else
    {
        if ( m_bHasReachedEof ||
			 !m_uBufferSize )
        {
            // Completed.
            // Unsignal semaphore.
            eStatus = AK_StmStatusCompleted;
        }
        else
        {
            // Still pending.
            eStatus = AK_StmStatusPending;            
        }
    }

    // Change Status and update semaphore count.
    if ( eStatus != AK_StmStatusPending )
    {
        // WAS pending, not anymore.
        m_eStmStatus = eStatus;
        m_pDevice->StdSemDecr( );
    }

    // Unlock status. 
    m_lockStatus.Unlock( );

    // Release the client thread if blocking I/O.
    if ( m_eStmStatus != AK_StmStatusPending &&
         m_hBlockEvent != NULL )
    {
        ::SetEvent( m_hBlockEvent );
    }

    // Data transfer is complete. Unlock I/O.
    m_lockIO.Unlock( );
}

// PopIOBuffer.
// Does not apply: automatic stream specific.
void * CAkStdStmBase::PopIOBuffer()
{
    AKASSERT( !"Cannot pop buffer from standard stream" );
    return NULL;
}

// Compute task's deadline for next operation.
// Sync: None.
AkReal32 CAkStdStmBase::EffectiveDeadline()
{
    AkReal32 fDeadline = m_fDeadline - AKPLATFORM::Elapsed( m_pDevice->GetTime( ), m_iIOStartTime );
    return ( fDeadline > 0 ? fDeadline : 0 );
}

//-----------------------------------------------------------------------------
// Profiling.
//-----------------------------------------------------------------------------
#ifndef AK_OPTIMIZED
// Get stream data.
void CAkStdStmBase::GetStreamData(
    AkStreamData & out_streamData
    )
{
    out_streamData.uStreamID = m_uStreamID;
    // Note. Priority appearing in profiler will be that which was used in last operation. 
    out_streamData.uPriority = m_priority;
    out_streamData.uBufferSize = 0;
    out_streamData.uAvailableData = 0;
    out_streamData.uFilePosition = m_iCurPosition;
    out_streamData.uNumBytesTransfered = m_uBytesTransfered;
    m_uBytesTransfered = 0;    // Reset.
}
#endif

//-----------------------------------------------------------------------------
// Helpers.
//-----------------------------------------------------------------------------
// Set task's status.
// Sync: status lock.
AKRESULT CAkStdStmBase::SetStatus( 
    AkStmStatus in_eStatus 
    )
{
    AKRESULT eResult = AK_Success;

    // Lock status.
    m_lockStatus.Lock( );

    // Check again in case status was changed.
    if ( m_eStmStatus != in_eStatus &&
         m_eStmStatus != AK_StmStatusError )
    {
        // WAS pending, not anymore.
        if ( m_eStmStatus == AK_StmStatusPending )
        {
            m_eStmStatus = in_eStatus;
            m_pDevice->StdSemDecr( );
        }
        else if ( in_eStatus == AK_StmStatusPending )
        {
            // Is NOW pending.
            m_eStmStatus = in_eStatus;
            m_pDevice->StdSemIncr( );
        }
        else
        {
            m_eStmStatus = in_eStatus;
        }
    }

    // Unlock status.
    m_lockStatus.Unlock( );
        
    return eResult;
}


//-----------------------------------------------------------------------------
// Name: class CAkAutoStmBase
// Desc: Automatic stream base implementation.
//-----------------------------------------------------------------------------

CAkAutoStmBase::CAkAutoStmBase( )
{
    m_pszStreamName = NULL;
    m_eStmType      = AK_StmTypeAutomatic;
    m_bIsWriteOp    = false;
}

CAkAutoStmBase::~CAkAutoStmBase( )
{
}

// Init.
AKRESULT CAkAutoStmBase::Init( 
    CAkDeviceBase *             in_pDevice,         // Owner device.
    const AkFileDesc &          in_fileDesc,        // File descriptor.
    const AkAutoStmHeuristics & in_heuristics,      // Streaming heuristics.
    AkAutoStmBufSettings *      in_pBufferSettings, // Stream buffer settings.
    AkUInt32                    in_uGranularity     // Device's I/O granularity.
    )
{
    AKASSERT( in_pDevice != NULL );
    AKASSERT( in_heuristics.fThroughput >= 0 &&
              in_heuristics.priority >= AK_MIN_PRIORITY &&
              in_heuristics.priority <= AK_MAX_PRIORITY );

    m_pDevice           = in_pDevice;

    if ( in_fileDesc.iFileSize < 0 )
    {
        AKASSERT( !"Invalid file size" );
        return AK_InvalidParameter;
    }

    m_fileDesc          = in_fileDesc;

    m_iCurPosition = 0;
    // Make position dirty if file descriptor is sector based and offset is not null.
    // First data transfer will force Low-Level to read at client position.
    if ( m_fileDesc.uSector > 0 )
    {
        m_bIsPositionDirty       = true;
        m_bFilePosNotSequential  = true;
    }
    else
    {
        m_bIsPositionDirty       = false;
        m_bFilePosNotSequential  = false;
    }
    m_uLoopedBufferIdx = AK_NO_LOOPING_DATA;
    m_uPositionChgIdx = AK_USER_POSITION_SEQUENTIAL;
    m_iTmpSeekPosition = 0;

    m_overlapped.ClearPosition();

    m_fThroughput   = in_heuristics.fThroughput;
    m_uLoopStart    = in_heuristics.uLoopStart;
	if ( in_heuristics.uLoopEnd <= m_fileDesc.iFileSize )
		m_uLoopEnd      = in_heuristics.uLoopEnd;
	else
		m_uLoopEnd		= (AkUInt32)m_fileDesc.iFileSize;
    m_uMinNumBuffers= in_heuristics.uMinNumBuffers;
    m_priority      = in_heuristics.priority;
    
    AKASSERT( m_pszStreamName == NULL );
    
    m_hBlockEvent       = NULL;

    m_uAvailDataSize    = 0;
    m_uLLBlockSize = CAkStreamMgr::GetLowLevel( )->GetBlockSize( in_fileDesc );
    if ( !m_uLLBlockSize )
    {
        AKASSERT( !"Invalid Low-Level I/O block size. Must be >= 1" );
        return AK_Fail;
    }

    // Set up buffer size according to streaming memory constraints.
    if ( !in_pBufferSettings )
        m_uBufferSize = in_uGranularity;
    else
    {
        // Buffer size constraints.
        if ( in_pBufferSettings->uBufferSize != 0 )
        {
            // User constrained buffer size. Ensure that it is valid with device granularity and low-level IO block size.
            if ( ( in_pBufferSettings->uBufferSize - in_uGranularity ) % m_uLLBlockSize != 0 )
            {
                AKASSERT( !"Specified buffer size is invalid with device's granularity and low-level block size" );
                return AK_InvalidParameter;
            }
            m_uBufferSize = in_pBufferSettings->uBufferSize;
        }
        else
        {
            m_uBufferSize = AkMax( in_pBufferSettings->uMinBufferSize, in_uGranularity );     // User constrained MIN buffer size.

            if ( in_pBufferSettings->uBlockSize > 0 )
            {
                // Block size specified. Buffer size must be a multiple of this size.
                AkUInt32 uRemaining = m_uBufferSize % in_pBufferSettings->uBlockSize;
                if ( uRemaining != 0 )
                {
                    // Snap to ceil.
                    m_uBufferSize += ( in_pBufferSettings->uBlockSize - uRemaining );
                }
            }
        }
    }

    // Correct buffer size to block size.
    if ( ( m_uBufferSize % m_uLLBlockSize ) != 0 )
    {
        // Warn with an assert.
        AKASSERT( !"Warning, unbuffered IO invalidated due to specified stream buffer block size" );
        // Snap to ceil.
        m_uBufferSize += ( m_uLLBlockSize - ( m_uBufferSize % m_uLLBlockSize ) );
    }

    m_uNextToGrant = 0;

    // Initial status.
    m_bRequiresScheduling = false;
    m_bIsRunning = false;
    m_bIOError = false;
    m_bHasReachedEof = false;
    AKRESULT eResult = AK_Success;
    if ( m_fileDesc.iFileSize == 0 )
    {
        m_bHasReachedEof = true;
    }

    m_bIsToBeDestroyed  = false;

    return eResult;
}

// Automatic stream termination.
// Sync: None. At this point, the user is not using the object anymore.
void CAkAutoStmBase::Term( )
{
#ifdef AK_INSTRUMENT_STM_MGR
	fwprintf_s( m_pDevice->DumpFile(), L"Destroyed: %u\n", reinterpret_cast<AkUInt32>(m_fileDesc.hFile) );
#endif
    // Cleanup in Low-Level IO.
    AKASSERT( m_pDevice != NULL );
    AKVERIFY( CAkStreamMgr::GetLowLevel( )->Close( m_fileDesc ) == AK_Success );

    if ( m_pszStreamName )
    {
        AkFree( CAkStreamMgr::GetObjPoolID( ), m_pszStreamName );
        m_pszStreamName = NULL;
    }
    if ( m_overlapped.hEvent )
    {
        AKVERIFY( ::CloseHandle( m_overlapped.hEvent ) );
        m_overlapped.hEvent = NULL;
    }
    AKASSERT( m_hBlockEvent == NULL );
}

//-----------------------------------------------------------------------------
// IAkAutoStream interface.
//-----------------------------------------------------------------------------

// Destruction.
// Close stream. The object is destroyed and the interface becomes invalid.
// Buffers are flushed here.
// Sync: 
// 1. Locks scheduler to set its m_bIsToBeDestroued flag. Better lock the
// scheduler once in a while than having the scheduler lock all the streams
// every time it schedules a task. Also, it spends most of its time executing I/O,
// so risks of interlock are minimized.
// 2. Lock status.
void CAkAutoStmBase::Destroy( )
{
    m_pDevice->LockScheduler( );

    m_lockStatus.Lock( );

    m_bIsToBeDestroyed = true;

#ifdef AK_INSTRUMENT_STM_MGR
	fwprintf_s( m_pDevice->DumpFile(), L"Destroy %u\n", reinterpret_cast<AkUInt32>(m_fileDesc.hFile) );
#endif

    // Free all buffers currently owned by the stream.
    // Note: IO pool access must be enclosed in m_lockAutoSems.
    if ( m_arBuffers.Length( ) > 0 )
    {
        m_pDevice->LockMem( );
        AkBuffersArray::Iterator it = m_arBuffers.End( );
        while ( it != m_arBuffers.Begin( ) )
        {
            --it;
			AK::MemoryMgr::ReleaseBlock( m_pDevice->GetIOPoolID(), (*it).pBuffer );
            m_arBuffers.Erase( it );
        }
        m_pDevice->NotifyMemChange( );
        m_pDevice->UnlockMem( );
    }
    m_uLoopedBufferIdx = AK_NO_LOOPING_DATA;
    m_arBuffers.Term( );

    if ( m_bIsRunning )
    {
    	m_bIsRunning = false;
		// "Force signal" feature disabled: the scheduler can keep some handles until other streams wake it up.
        UpdateSchedulingStatus();
	}    
    m_lockStatus.Unlock( );

    m_pDevice->UnlockScheduler( );
}

// Stream info access.
// Sync: None.
void CAkAutoStmBase::GetInfo(
    AkStreamInfo & out_info       // Returned stream info.
    )
{
    AKASSERT( m_pDevice != NULL );
    out_info.deviceID     = m_pDevice->GetDeviceID( );
    out_info.pszName      = m_pszStreamName;
    out_info.uSize = m_fileDesc.iFileSize;
}

// Stream heuristics access.
// Sync: None.
void CAkAutoStmBase::GetHeuristics(
    AkAutoStmHeuristics & out_heuristics    // Returned stream heuristics.
    )
{
    out_heuristics.fThroughput  = m_fThroughput;
    out_heuristics.uLoopStart   = m_uLoopStart;
    out_heuristics.uLoopEnd     = m_uLoopEnd;
    out_heuristics.uMinNumBuffers   = m_uMinNumBuffers;
    out_heuristics.priority     = m_priority;
}

// Stream heuristics run-time change.
// Sync: None.
AKRESULT CAkAutoStmBase::SetHeuristics(
    const AkAutoStmHeuristics & in_heuristics   // New stream heuristics.
    )
{
    if ( in_heuristics.fThroughput < 0 ||
         in_heuristics.priority < AK_MIN_PRIORITY ||
         in_heuristics.priority > AK_MAX_PRIORITY )
    {
        AKASSERT( !"Invalid stream heuristics" );
        return AK_InvalidParameter;
    }

    m_fThroughput   = in_heuristics.fThroughput;
    
    m_uMinNumBuffers= in_heuristics.uMinNumBuffers;
    m_priority      = in_heuristics.priority;

    // If looping heuristic changed.
	AkUInt32 uLoopEnd;
	if ( in_heuristics.uLoopEnd <= m_fileDesc.iFileSize )
		uLoopEnd = in_heuristics.uLoopEnd;
	else
		uLoopEnd = (AkUInt32)m_fileDesc.iFileSize;
    if ( m_uLoopEnd != uLoopEnd ||
         m_uLoopStart != in_heuristics.uLoopStart )
    {
        // Lock status.
        AkAutoLock<CAkLock> stmBufferGate( m_lockStatus );
        
        // If data was read in the looping region, flush it.
        if ( m_uLoopedBufferIdx != AK_NO_LOOPING_DATA )
        {        
	        int iBuffer = (int)m_arBuffers.Length( ) - 1;
	        AKASSERT( m_uNextToGrant <= m_uLoopedBufferIdx );
	        m_pDevice->LockMem( );
	        while ( iBuffer >= m_uLoopedBufferIdx )
	        {
	            AKASSERT( m_uAvailDataSize >= m_arBuffers[iBuffer].uDataSize );
	            m_uAvailDataSize -= m_arBuffers[iBuffer].uDataSize;
	            AK::MemoryMgr::ReleaseBlock( m_pDevice->GetIOPoolID(), m_arBuffers[iBuffer].pBuffer );
	            m_arBuffers.Erase(iBuffer);
	            --iBuffer;
	            
	        }
	        m_pDevice->NotifyMemChange( );
	        m_pDevice->UnlockMem( );
	        m_uLoopedBufferIdx = AK_NO_LOOPING_DATA;
	        m_bFilePosNotSequential = true;
	
	        // Set position to where it was before look-ahead.
	        m_overlapped.Position( m_uLoopEndPosition );
	    }
	
	    m_uLoopStart    = in_heuristics.uLoopStart;
	    m_uLoopEnd      = uLoopEnd;
		
		UpdateSchedulingStatus();
    }
    return AK_Success;
}

// Name the stream (appears in Wwise profiler).
// Sync: None.
AKRESULT CAkAutoStmBase::SetStreamName(
    AkLpCtstr       in_pszStreamName    // Stream name.
    )
{
    if ( m_pszStreamName != NULL )
        AkFree( CAkStreamMgr::GetObjPoolID( ), m_pszStreamName );

    if ( in_pszStreamName != NULL )
    {
        // Allocate string buffer for user defined stream name.
        m_pszStreamName = (AkTChar*)AkAlloc( CAkStreamMgr::GetObjPoolID( ), (AkUInt32)sizeof(AkTChar)*( wcslen( in_pszStreamName ) + 1 ) );
        if ( m_pszStreamName == NULL )
            return AK_InsufficientMemory;

        // Copy.
        wcscpy( m_pszStreamName, in_pszStreamName );
    }
    return AK_Success;
}

// Returns low-level IO block size for this stream's file descriptor.
AkUInt32 CAkAutoStmBase::GetBlockSize()
{
    return m_uLLBlockSize;
}

// Operations.
// ---------------------------------------

// Starts automatic scheduling.
// Sync: Status update if not already running. 
// Notifies memory change.
AKRESULT CAkAutoStmBase::Start( )
{
    if ( !m_bIsRunning )
    {
        m_bIsRunning = true; 
        // UpdateSchedulingStatus() will notify scheduler if required.
        m_lockStatus.Lock( );
        UpdateSchedulingStatus( );
        m_lockStatus.Unlock( );

        // The scheduler should reevaluate memory usage. Notify it.
        m_pDevice->LockMem( );
        m_pDevice->NotifyMemChange( );
        m_pDevice->UnlockMem( );

        // Reset time. Time count since last transfer starts now.
        m_iIOStartTime = m_pDevice->GetTime( );
    }
    return m_bIOError ? AK_Fail : AK_Success;
}

// Stops automatic scheduling.
// Sync: Status update.
AKRESULT CAkAutoStmBase::Stop( )
{
    m_bIsRunning = false;
    // UpdateSchedulingStatus() will notify scheduler if required.
    m_lockStatus.Lock( );
    UpdateSchedulingStatus( );
    m_lockStatus.Unlock( );
    return AK_Success;
}

// Get stream position; position as seen by the user.
AkUInt64 CAkAutoStmBase::GetPosition( 
    bool * out_pbEndOfStream    // Can be NULL.
    )
{
    if ( out_pbEndOfStream != NULL )
        *out_pbEndOfStream = ( m_iCurPosition >= m_fileDesc.iFileSize );

    return m_iCurPosition;
}

// Set stream position. Modifies position of next read/write.
// Sync: Updates status. 
AKRESULT CAkAutoStmBase::SetPosition(
    AkInt64         in_iMoveOffset,     // Seek offset.
    AkMoveMethod    in_eMoveMethod,     // Seek method, from beginning, end or current file position.
    AkInt64 *       out_piRealOffset    // Actual seek offset may differ from expected value when unbuffered IO.
                                        // In that case, floors to sector boundary. Pass NULL if don't care.
    )
{
    if ( out_piRealOffset != NULL )
    {
        *out_piRealOffset = 0;
    }

    // Compute absolute position.
    AkInt64 iPosition;
    if ( in_eMoveMethod == AK_MoveBegin )
    {
        iPosition = in_iMoveOffset;
    }
    else if ( in_eMoveMethod == AK_MoveCurrent )
    {
        iPosition = m_iCurPosition + in_iMoveOffset;
    }
    else if ( in_eMoveMethod == AK_MoveEnd )
    {
        iPosition = m_fileDesc.iFileSize + in_iMoveOffset;
    }
    else
    {
        AKASSERT( !"Invalid move method" );
        return AK_InvalidParameter;
    }

    if ( iPosition < 0 )
    {
        AKASSERT( !"Trying to move the file pointer before the beginning of the file" );
        return AK_InvalidParameter;
    }

    // Change offset if Low-Level block size is greater than 1.
    if ( iPosition % m_uLLBlockSize != 0 )
    {
        // Round down to block size.
        iPosition -= ( iPosition % m_uLLBlockSize );
        AKASSERT( iPosition >= 0 );
    }

    // Set real offset if argument specified.
    if ( out_piRealOffset != NULL )
    {
        switch ( in_eMoveMethod )
        {
        case AK_MoveBegin:
            *out_piRealOffset = iPosition;
            break;
        case AK_MoveCurrent:
            *out_piRealOffset = iPosition - m_iCurPosition;
            break;
        case AK_MoveEnd:
            *out_piRealOffset = iPosition - m_fileDesc.iFileSize;
            break;
        default:
            AKASSERT( !"Invalid move method" );
        }
    }

    // Set new position and update status. 
    ForceFilePosition( iPosition );

    return AK_Success;
}

// Data/status access. 
// -----------------------------------------

// GetBuffer.
// Return values : 
// AK_DataReady     : if buffer is granted.
// AK_NoDataReady   : if buffer is not granted yet.
// AK_NoMoreData    : if buffer is granted but reached end of file (next will return with size 0).
// AK_Fail          : there was an IO error. 

// Sync: Updates status.
AKRESULT CAkAutoStmBase::GetBuffer(
    void *&         out_pBuffer,        // Address of granted data space.
    AkUInt32 &      out_uSize,          // Size of granted data space.
    bool            in_bWait            // Block until data is ready.
    )
{
    out_pBuffer    = NULL;
    out_uSize       = 0;

    // Data ready?
    out_pBuffer = GetReadBuffer( out_uSize );
    
    // Handle blocking GetBuffer. No data is ready, but there is more data to come
    // (otherwise out_pBuffer would not be NULL).
    // If blocking, create the blocking event.
    if ( out_pBuffer == NULL && 
         in_bWait )
    {
        if ( !m_bIsRunning )
        {
            AKASSERT( !"Blocking GetBuffer() on a stopped stream: would never return" );
            return AK_Fail;
        }
        // Note. Event could be created after the scheduler called Update()>SetEvent(). 
        // Get status lock. Try get buffer again. If it returns nothing, then wait on event.
        m_lockStatus.Lock( );

        // Try again.
        out_pBuffer = GetReadBuffer( out_uSize );

        if ( out_pBuffer == NULL &&
             !m_bHasReachedEof )
        {
            // Still not ready. Safely create the event.
            AKASSERT( m_hBlockEvent == NULL );
            // Note. Event is manual reset in case IO thread had time to execute IO before we block on it.
            // Note. This implementation is not optimized for blocking I/O.
            m_hBlockEvent = ::CreateEvent( NULL,    // no security
                                           TRUE,    // manual reset
                                           FALSE,   // unsignaled
                                           NULL );  // no name

            // Release lock and let the scheduler perform IO.
            m_lockStatus.Unlock( );

            // Verify blocking event.
            if ( m_hBlockEvent == NULL )
            {
                AKASSERT( !"Failed creating event for blocking IO" );
                return AK_Fail;
            }

            // Wait for the event.
            AKVERIFY( ::WaitForSingleObject( m_hBlockEvent, INFINITE ) == WAIT_OBJECT_0 );
            // Clean up.
            ::CloseHandle( m_hBlockEvent );
            m_hBlockEvent = NULL;

            // Retry getting buffer.
            out_pBuffer = GetReadBuffer( out_uSize );
            
            // If there is no data, this means there was an I/O error.
            if ( out_pBuffer == NULL )
            {
                AKASSERT( m_bIOError ||
                          !"IO error on blocking IO" );
                m_lockStatus.Unlock( );
                return AK_Fail;
            }
            // else, handled below.
        }
        else
        {
            // Scheduler filled a buffer by the time we got the status lock.
            // Release and proceed.
            m_lockStatus.Unlock( );
        }
    }
    
    AKRESULT eRetCode;
    if ( m_bIOError )
    {
        eRetCode = AK_Fail;
    }
    else if ( out_pBuffer == NULL )
    {
        // Buffer is empty, either because no data is ready, or because scheduling has completed 
        // and there is no more data.
        if ( m_iCurPosition >= m_fileDesc.iFileSize )
            eRetCode = AK_NoMoreData;
        else
            eRetCode = AK_NoDataReady;
    }
    else
    {
        // Keep file position granted to client.
        if ( m_iCurPosition >= ( m_fileDesc.iFileSize - m_uBufferSize ) )
            eRetCode = AK_NoMoreData;
        else
            eRetCode = AK_DataReady;
    }
    return eRetCode;
}

// Release buffer granted to user.
// Sync: Status lock.
AKRESULT CAkAutoStmBase::ReleaseBuffer()
{
    // Lock status.
    AkAutoLock<CAkLock> stmBufferGate( m_lockStatus );

    // Release buffer granted to client. 
    AkUInt32 uReleasedSize;
    AKRESULT eResult = ReleaseReadBuffer( uReleasedSize );
    if ( eResult == AK_Success )
    {
        // Update position seen by user.
        UpdateUserPosition( uReleasedSize );
    }

    return eResult;
}

//-----------------------------------------------------------------------------
// CAkStmTask implementation.
//-----------------------------------------------------------------------------

// Task management.
// Sync: None. IsToBeDestroyed must have returned true, so the stream is only handled 
// by the scheduler thread.
void CAkAutoStmBase::InstantDestroy( )
{
#ifndef AK_OPTIMIZED
    AKASSERT( m_bIsToBeDestroyed && 
              ( !m_pDevice->IsMonitoring( ) || m_bIsProfileDestructionAllowed ) );
#else
    AKASSERT( m_bIsToBeDestroyed );
#endif

    // If the stream kept asking to be scheduled, now it is time to stop.
    if ( m_bRequiresScheduling )
        m_pDevice->AutoSemDecr( );

    // Term.
    Term( );

    // Destroys itself.
    AkDelete( CAkStreamMgr::GetObjPoolID( ), this );
}

// Returns true if it is waiting for I/O.
// An automatic stream is waiting for I/O if it is runnning and it did not reach the end of file.
// Sync: None. Status query.
bool CAkAutoStmBase::IsWaitingForIO()
{
    return m_bIsRunning && !m_bHasReachedEof;
}

// Data buffer access.
// Allocate a buffer from the Memory Mgr.
// Sync: None. 
// IMPORTANT: However, if device's MemIdle() needs to be signaled, the call to this method must be atomic
// with CAkDeviceBase::NotifyMemIdle(). Then they both need to be enclosed in CAkDeviceBase::LockMem().
void * CAkAutoStmBase::TryGetIOBuffer()
{
    // Allocate a buffer.
    AKASSERT( m_uBufferSize > 0 );
	return AK::MemoryMgr::GetBlock( m_pDevice->GetIOPoolID() );
}

void CAkAutoStmBase::TransferInfo( 
    AkFileDesc *& out_pFileDesc,    // Stream's associated file descriptor.
    OVERLAPPED *& out_pOverlapped,  // Asynchronous data and file position.
    bool &      out_bIsSequential,  // Returned !bIsPositionDirty flag.
    AkUInt32 *  out_puSize          // Required transfer size.
    )
{
    out_pFileDesc = &m_fileDesc;

    // Lock status.
    AkAutoLock<CAkLock> atomicPosition( m_lockStatus );

	// Required transfer size is the buffer size for this stream.
    // Slice request to granularity.
	if ( m_uBufferSize > m_pDevice->GetGranularity( ) )
        *out_puSize = m_pDevice->GetGranularity( );
    else
        *out_puSize = m_uBufferSize;
    
    // m_bIsPositionDirty cannot be set if m_bNotifyLLNotSequential is not. The opposite is not true though, since
    // stream objects can change the file position of the next data transfer, without requiring it to be set to
    // the client position m_iCurPosition.
    // With automatic streams, m_bIsPositionDirty is used to force the next data transfer to occur at the m_iTmpSeekPosition.
    AKASSERT( !m_bIsPositionDirty || 
              m_bIsPositionDirty && m_bFilePosNotSequential );

#ifdef AK_INSTRUMENT_STM_MGR
	fwprintf_s( m_pDevice->DumpFile(), L"Read (t=%lld) file=%u, avail=%u, pos=%u\n", m_pDevice->GetTime(), reinterpret_cast<AkUInt32>(m_fileDesc.hFile), m_uAvailDataSize, m_overlapped.Position() );
#endif

    if ( !m_bIsPositionDirty )
    {
        // Set file position for next I/O to loop start point if the stream is looping,
        // the updated position is further than the end of the loop, and we did not already try to stream
        // in any loop data.
        if ( m_uLoopEnd &&
             m_uLoopedBufferIdx == AK_NO_LOOPING_DATA &&
             m_overlapped.Position( ) - ( m_fileDesc.uSector * m_uLLBlockSize ) >= m_uLoopEnd )
        {
            // Remember file position before looping, in case all buffers with loop data get reassigned.
            m_uLoopEndPosition = (AkUInt32)m_overlapped.Position( );
            // Snap to Low-Level block size.
            m_overlapped.Position( m_uLoopStart - ( m_uLoopStart % m_uLLBlockSize ) + ( m_fileDesc.uSector * m_uLLBlockSize ) );
            m_uLoopedBufferIdx = (AkUInt16)m_arBuffers.Length( );
            m_bFilePosNotSequential = true;

#ifdef AK_INSTRUMENT_STM_MGR
			fwprintf_s( m_pDevice->DumpFile(), L"Loop pos=%u\n", m_overlapped.Position() );
#endif
        }
    }
    else
    {
        // If position is "dirty", set file position to client position.
        AKASSERT( m_uLoopedBufferIdx == AK_NO_LOOPING_DATA ); // (no data should be loaded in looping region).
        m_overlapped.Position( ( m_fileDesc.uSector * m_uLLBlockSize ) + m_iTmpSeekPosition );
#ifdef AK_INSTRUMENT_STM_MGR
		fwprintf_s( m_pDevice->DumpFile(), L"Dirty pos=%u\n", m_overlapped.Position() );
#endif
    }

    out_bIsSequential = !m_bFilePosNotSequential;
    m_bFilePosNotSequential = false;

    out_pOverlapped = &m_overlapped;

    // Reset timer. Time count since last transfer starts now.
    m_iIOStartTime = m_pDevice->GetTime( );
}

// Update task after data transfer.
// Sync: Update status.
// Note: Actual write size should be consistent with file size. It is the responsiblity of
// the low-level IO to return the proper value.
void CAkAutoStmBase::Update( 
    void *      in_pBuffer,             // Address of data.
    AkUInt32    in_uActualIOSize,       // Size available for writing/reading.
    bool        in_bWasIOSuccessful     // IO was successful (no error).
    )
{
    // Lock status.
    m_lockStatus.Lock( );

    // Note: size transferred should be greater than zero. Zero sized transfers can only occured if the 
    // user set the position to the end of the file.
    AKASSERT( in_uActualIOSize ||
              m_overlapped.Position( ) >= m_fileDesc.iFileSize + m_fileDesc.uSector*m_uLLBlockSize );

    if ( !m_bIsToBeDestroyed &&
         !m_bFilePosNotSequential &&
		 in_uActualIOSize > 0 )
    {
        // Add buffer to list.
        AkStmBuffer * pNewBuffer = m_arBuffers.AddLast();
        if ( !pNewBuffer )
        {
#ifdef AK_INSTRUMENT_STM_MGR
			fwprintf_s( m_pDevice->DumpFile(), L"Update ENQUEUE FAILED\n" );
#endif

            // Object pool size is too small! Flush this buffer.
            // Note: I/O pool access must be enclosed in m_lockAutoSems.
            m_pDevice->LockMem( );
            AK::MemoryMgr::ReleaseBlock( m_pDevice->GetIOPoolID(), in_pBuffer );
            m_pDevice->NotifyMemChange( );
            m_pDevice->UnlockMem( );

            m_bFilePosNotSequential = true;    // Low-level IO will have to seek back to this position.
            m_lockStatus.Unlock( );
            return;
        }
        pNewBuffer->pBuffer = in_pBuffer;

#ifdef AK_INSTRUMENT_STM_MGR
		AkUInt32 uTransferredSize = in_uActualIOSize;
#endif
        
        // Truncate data size if the stream is going to pass the end of file.
        if ( ( m_overlapped.Position( ) + in_uActualIOSize ) > m_fileDesc.iFileSize + m_fileDesc.uSector*m_uLLBlockSize )
             in_uActualIOSize = (AkUInt32)( m_fileDesc.iFileSize + m_fileDesc.uSector*m_uLLBlockSize - m_overlapped.Position( ) );
        
        pNewBuffer->uDataSize = in_uActualIOSize;

        m_overlapped.IncrementPosition( in_uActualIOSize );

        // Update total available data size (for scheduler query, to avoid locking).
        m_uAvailDataSize += in_uActualIOSize;

        // Resolve position dirty flag now.
        m_bIsPositionDirty = false;

#ifdef AK_INSTRUMENT_STM_MGR
		fwprintf_s( m_pDevice->DumpFile(), L"Update transferred=%u, avail=%u, pos=%u\n", uTransferredSize, m_uAvailDataSize, m_overlapped.Position() );
#endif
    }
    else
    {
        // Stream was either scheduled to be destroyed while I/O was occurring, stopped, or 
        // its position was set dirty while I/O was occuring. Flush that data.
        // Note: I/O pool access must be enclosed in m_lockAutoSems.
        m_pDevice->LockMem( );
        AK::MemoryMgr::ReleaseBlock( m_pDevice->GetIOPoolID(), in_pBuffer );
        m_pDevice->NotifyMemChange( );
        m_pDevice->UnlockMem( );

        // Might need to read again at a position that is not sequential.
        m_bFilePosNotSequential = true;

#ifdef AK_INSTRUMENT_STM_MGR
		fwprintf_s( m_pDevice->DumpFile(), L"NO Update transferred=%u, not_sequential=%u, destroy=%u\n", in_uActualIOSize, m_bFilePosNotSequential, m_bIsToBeDestroyed );
#endif
    }
    
    // Profiling. 
#ifndef AK_OPTIMIZED
    m_uBytesTransfered += in_uActualIOSize;
#endif
    
    // Status.
    if ( !in_bWasIOSuccessful )
    {
        // Set to Error.
        m_bIOError = true;
        // Stop auto stream.
        Stop();
    }
    else
    {
        // Update scheduling status.
        UpdateSchedulingStatus( );
    }

    m_lockStatus.Unlock( );

    // Release the client thread if an operation was pending and call was blocking on it.
    if ( m_hBlockEvent != NULL )
    {
        ::SetEvent( m_hBlockEvent );
    }

}

// Try to remove a buffer from this stream's list of buffers. Fails if user owns them all.
// Sync: Status.
void * CAkAutoStmBase::PopIOBuffer( )
{
    // Lock status.
    AkAutoLock<CAkLock> stmBufferGate( m_lockStatus );

    // Lock memory.
    m_pDevice->LockMem( );

    // Now that memory is locked, try to get a buffer again.
    void * pBuffer = AK::MemoryMgr::GetBlock( m_pDevice->GetIOPoolID() );

    if ( pBuffer )
    {
        // A buffer was freed by the time we acquired the memory lock. Leave.
        m_pDevice->UnlockMem( );
        return pBuffer;
    }

    // Memory is still full. Try to remove a buffer from this task.

	// But do not remove if
	// 1) All buffers are already granted to client.
	// 2) Removing a buffer would take this stream under target buffering.

    if ( m_arBuffers.Length( ) > m_uNextToGrant &&  
		(AkInt32)( m_uAvailDataSize ) - (AkInt32)( ( m_uNextToGrant + 1 ) * m_uBufferSize ) > m_pDevice->GetTargetAutoStmBufferLength() * m_fThroughput )
    {
        AkUInt32 uSizeRemoved = m_arBuffers.Last( ).uDataSize;
        AKASSERT( m_uAvailDataSize >= uSizeRemoved );
        m_uAvailDataSize -= uSizeRemoved;
        pBuffer = m_arBuffers.Last( ).pBuffer;
        m_arBuffers.RemoveLast( );

        // Update status: Correct position. Reset EOF flag and restart if necessary.

        m_bFilePosNotSequential = true;
        m_bHasReachedEof = false;
        AKASSERT( pBuffer );

        // Reset looping buffer index if applicable.
        if ( m_uLoopedBufferIdx != AK_NO_LOOPING_DATA )
        {
            // There was looping data.

            AKASSERT( m_uLoopedBufferIdx <= m_arBuffers.Length( ) + 1 );
            AKASSERT( !m_bIsPositionDirty &&
                      m_uLoopEnd );

            // If we removed all buffers of the loop look-ahead region, 
            // clear looping, set position to loop end, decrement.
            if ( m_uLoopedBufferIdx > m_arBuffers.Length( ) )
        	{
            	m_uLoopedBufferIdx = AK_NO_LOOPING_DATA;
                m_overlapped.Position( m_uLoopEndPosition );
            }

            // Note. If ( m_uLoopedBufferIdx == m_arBuffers.Length( ) ), leave the m_uLoopedBufferIdx
            // as is, and decrement the position normally (it will match the beginning of the loop).
            // File position will be ready for next transfer.
    	}
            
        AKASSERT( m_overlapped.Position( ) - m_fileDesc.uSector * m_uLLBlockSize >= uSizeRemoved );
        m_overlapped.DecrementPosition( uSizeRemoved );

#ifdef AK_INSTRUMENT_STM_MGR
		fwprintf_s( m_pDevice->DumpFile(), L"Pop GRANTED file=%u, avail=%u, pos=%u\n", reinterpret_cast<AkUInt32>(m_fileDesc.hFile), m_uAvailDataSize, (AkUInt32)m_overlapped.Position() );
#endif
    }
    else
    {
        // This task cannot let a buffer go. Notify scheduler that memory is blocked for a while.
        AKASSERT( !pBuffer );
        m_pDevice->NotifyMemIdle( );

#ifdef AK_INSTRUMENT_STM_MGR
		fwprintf_s( m_pDevice->DumpFile(), L"Pop REFUSED, file=%u, avail=%u, pos=%u\n", reinterpret_cast<AkUInt32>(m_fileDesc.hFile), m_uAvailDataSize, (AkUInt32)m_overlapped.Position() );
#endif
    }

    m_pDevice->UnlockMem( );
    return pBuffer;
}

// Compute task's deadline for next operation.
// Sync: None, but copy throughput heuristic on the stack.
AkReal32 CAkAutoStmBase::EffectiveDeadline()
{
    // Copy throughput value on the stack, because it can be modified (to zero) by user.
    AkReal32 fThroughput = m_fThroughput;
    if ( fThroughput == 0 )
        return AK_INFINITE_DEADLINE;
    else
    {
        // Note: Sync. These values might be changed by another thread. 
        // In the worst case, the scheduler will take a sub-optimal decision.
		AkInt32 iEffAvailDataSize = (AkInt32)( m_uAvailDataSize - m_uNextToGrant * m_uBufferSize );
		if ( m_uLoopEnd )
		{
			// Looping data. Discard bytes that are located after loop end.
			AkInt32 iDataAfterLoop = (AkUInt32)m_overlapped.Position() - m_uLoopEnd;
			if ( iDataAfterLoop > 0
				&& (AkUInt32)iDataAfterLoop < m_uBufferSize )
				iEffAvailDataSize -= iDataAfterLoop;
		}
        return ( iEffAvailDataSize > 0 ) ? iEffAvailDataSize / fThroughput : 0;
    }
}

//-----------------------------------------------------------------------------
// Profiling.
//-----------------------------------------------------------------------------
#ifndef AK_OPTIMIZED
// Profiling: get data.
void CAkAutoStmBase::GetStreamData(
    AkStreamData & out_streamData
    )
{
    out_streamData.uStreamID = m_uStreamID;
    out_streamData.uPriority = m_priority;
    out_streamData.uFilePosition = m_iCurPosition;
    
    if ( m_fThroughput > 0 )
		out_streamData.uBufferSize = static_cast<AkUInt32>( m_pDevice->GetTargetAutoStmBufferLength() * m_fThroughput );
	else
		out_streamData.uBufferSize = 1;
	
    out_streamData.uAvailableData = m_uAvailDataSize;
	if ( out_streamData.uAvailableData > out_streamData.uBufferSize )
		out_streamData.uAvailableData = out_streamData.uBufferSize;	// Clamp amount of available data to target buffer length.
    
	out_streamData.uNumBytesTransfered = m_uBytesTransfered;
    m_uBytesTransfered = 0;    // Reset.
}
#endif

//-----------------------------------------------------------------------------
// Helpers.
//-----------------------------------------------------------------------------
// Update task status.
// Sync: Status lock.
void CAkAutoStmBase::ForceFilePosition(
    const AkInt64 & in_iNewPosition     // New stream position (absolute).
    )
{
    // Lock status.
    AkAutoLock<CAkLock> statusGate( m_lockStatus );

    // Update position.

	if ( m_uPositionChgIdx != AK_USER_POSITION_SEQUENTIAL )
	{
		// The position-change buffer index is already set to another buffer.
		// Force user position to this position now (intermediate position information will be lost - see note in UpdateUserPosition()).
		m_iCurPosition = m_iTmpSeekPosition;
		m_uPositionChgIdx = AK_USER_POSITION_SEQUENTIAL;
	}

#ifdef AK_INSTRUMENT_STM_MGR
	fwprintf_s( m_pDevice->DumpFile(), L"SetPos file=%u to %u\n", reinterpret_cast<AkUInt32>(m_fileDesc.hFile), in_iNewPosition );
#endif

    // Set m_iTmpSeekPosition to new position.
    m_iTmpSeekPosition = in_iNewPosition;
    // User position (m_iCurPosition) is set now if no buffer is currently granted to user, or later
    // otherwise (we use m_uApplyNewPosition to keep track of this deferred position assignation).
    if ( m_uNextToGrant > 0 )
    {
        // Deferred user position change.
        m_uPositionChgIdx = m_uNextToGrant;
#ifdef AK_INSTRUMENT_STM_MGR
		fwprintf_s( m_pDevice->DumpFile(), L"deferred...\n" );
#endif
    }
    else
    {
        // Change user position now.
        m_iCurPosition = in_iNewPosition;
    }

    // If we already read data in the looping section of the file, and the position specified corresponds
    // to the loop start position, clear the LoopedBuffer index.
    // Otherwise, flush all data that is not currently granted to user (position change not predicted),
    // and set position dirty so that TransferInfo() uses m_iCurPosition.
    AkUInt32 uRealLoopStart = m_uLoopStart - m_uLoopStart % m_uLLBlockSize;
    if ( m_uLoopedBufferIdx != AK_NO_LOOPING_DATA &&
         m_iTmpSeekPosition == uRealLoopStart )
    {
        // It is however possible that the scheduler added buffers passed the loop end position.
        // Flush all buffers passed that position.
        int iBuffer = m_uLoopedBufferIdx-1;
        m_pDevice->LockMem( );
        while ( iBuffer >= m_uNextToGrant )
        {
            AKASSERT( m_uAvailDataSize >= m_arBuffers[iBuffer].uDataSize );
            m_uAvailDataSize -= m_arBuffers[iBuffer].uDataSize;
            AK::MemoryMgr::ReleaseBlock( m_pDevice->GetIOPoolID(), m_arBuffers[iBuffer].pBuffer );
            m_arBuffers.Erase(iBuffer);
            --iBuffer;
        }
        m_pDevice->NotifyMemChange( );
        m_pDevice->UnlockMem( );
    }
    else 
    {
        // Position change was either not predicted, or the stream did not have time to fill the loop region.
        // Set m_bIsPositionDirty so that next I/O occurs at m_iTmpSeekPosition. Flush all non granted buffers.
        m_bIsPositionDirty = true;
        Flush( );
    }
    // In all cases, position was changed, so we need to notify the low-level IO,
    // and reset the looping buffers flag.
    m_uLoopedBufferIdx = AK_NO_LOOPING_DATA;
    m_bFilePosNotSequential = true;
    
    UpdateSchedulingStatus( );
}

// Update user position. Generally sequentially, unless user called SetPosition() while holding at least
// one buffer. In such a case it is set to the seeked position when the last of these buffers is released
// (the count is maintained by using m_uPositionChgIdx). 
// Sync: Status MUST be locked from outside.
void CAkAutoStmBase::UpdateUserPosition(
    AkUInt32 in_uOffset     // Relative offset.
    )
{
    // Increment user position if sequential. 
    // Otherwise, set to seeked position if the user just released the last buffer he owned when he called SetPosition.
    AKASSERT( m_uPositionChgIdx != 0 );
    if ( m_uPositionChgIdx != AK_USER_POSITION_SEQUENTIAL )
        m_uPositionChgIdx--;
    if ( m_uPositionChgIdx != 0 )
    {
        // Normal sequential position update.
        m_iCurPosition += in_uOffset;
    }
    else
    {
        // Set position to deferred user-forced position.
        m_iCurPosition = m_iTmpSeekPosition;
        m_uPositionChgIdx = AK_USER_POSITION_SEQUENTIAL;
    }
    /* Note. Normally the client position should never be smaller than the file size, except if the client called SetPosition()
	more than once while holding more than one buffer. In such a case the intermediary positions are invalid.
	AKASSERT( m_iCurPosition <= m_fileDesc.iFileSize );
	*/
    
    UpdateSchedulingStatus( );
}

// Update task scheduling status; whether or not it is waiting for I/O and counts in scheduler semaphore.
// Sync: Status MUST be locked from outside.
void CAkAutoStmBase::UpdateSchedulingStatus( 
    bool in_bForceSignalSem /*=false*/ )
{
	if ( !m_bIsPositionDirty &&
         ( !m_uLoopEnd || ( m_uLoopedBufferIdx != AK_NO_LOOPING_DATA && m_uLoopEnd ) ) &&
         m_overlapped.Position( ) >= m_fileDesc.iFileSize + m_fileDesc.uSector * m_uLLBlockSize )
    {
        m_bHasReachedEof = true;
    }
    else
        m_bHasReachedEof = false;

    // Update scheduler control.
    if ( in_bForceSignalSem || 
    	( IsWaitingForIO() && 
         ( EffectiveDeadline() <= m_pDevice->GetTargetAutoStmBufferLength() ||
			m_arBuffers.Length() < m_uMinNumBuffers ) ) )
    {
        if ( !m_bRequiresScheduling )
        {
            m_pDevice->AutoSemIncr( );
            m_bRequiresScheduling = true;
        }
    }
    else
    {
        if ( m_bRequiresScheduling &&
			 !IsToBeDestroyed() )
        {
            m_pDevice->AutoSemDecr( );
            m_bRequiresScheduling = false;
        }
    }
}

// Returns a buffer filled with data. NULL if no data is ready.
// Sync: Lock status (buffer array) while iterating: the scheduler could be adding a buffer, and values (e.g. pBuffer) 
// are invalid for a time being.
void * CAkAutoStmBase::GetReadBuffer(     
    AkUInt32 & out_uSize                // Buffer size.
    )
{
    // Lock status.
    AkAutoLock<CAkLock> stmBufferGate( m_lockStatus );
    
    if ( m_uNextToGrant < m_arBuffers.Length( ) )
    {
        if ( m_uLoopedBufferIdx == m_uNextToGrant )
        {
            // User attempts to read a buffer passed the end loop point heuristic, but did not set the
            // stream position accordingly!
            // Flush data, set m_bIsPositionDirty flag so that next I/O will occur at the right position,
            // reset looping heuristics for user, to avoid repeating this mistake, return AK_NoDataReady.
            m_uLoopedBufferIdx = AK_NO_LOOPING_DATA;
            Flush( );
            m_bIsPositionDirty = true;
            m_bFilePosNotSequential = true;
            m_uLoopEnd = 0;
            out_uSize = 0;
            return NULL;
        }
        // Get first buffer not granted.
        AkStmBuffer & stmBuffer = m_arBuffers[m_uNextToGrant];
        
        // Update "next to grant" index.
        m_uNextToGrant++;

        UpdateSchedulingStatus( );

        out_uSize = stmBuffer.uDataSize;
        return stmBuffer.pBuffer;
    }
    
    // No data ready.
    out_uSize = 0;
    return NULL;
}

// Releases a buffer granted to user. Returns AK_Fail if no granted buffer matches the address supplied.
// Sync: Status MUST be locked from outside.
AKRESULT CAkAutoStmBase::ReleaseReadBuffer( 
    AkUInt32 &  out_uReleasedSize       // Size of data released.
    )
{
    if ( m_uNextToGrant > 0 )
    {
    	AkBuffersArray::Iterator it = m_arBuffers.Begin( );
        out_uReleasedSize = (*it).uDataSize;
        AKASSERT( m_uAvailDataSize >= out_uReleasedSize );
        m_uAvailDataSize -= out_uReleasedSize;  // Update available data for scheduler query.
        
        // Note: IO pool access must be enclosed in m_lockAutoSems.
        m_pDevice->LockMem( );
        AK::MemoryMgr::ReleaseBlock( m_pDevice->GetIOPoolID(), (*it).pBuffer );
        // Memory was released. Signal it.
        m_pDevice->NotifyMemChange( );
        m_pDevice->UnlockMem( );

        m_arBuffers.Erase( it );

        // Update "next to grant" index.
        m_uNextToGrant--;

        // Update looping buffer index if applicable.
        if ( m_uLoopedBufferIdx != AK_NO_LOOPING_DATA )
        {
            AKASSERT( m_uLoopedBufferIdx > m_uNextToGrant );
            m_uLoopedBufferIdx--;
        }
    }
    else
    {
        // Failure: Buffer was not found or not granted.
        // No need to set out_uReleasedSize and out_bAllReleased.
        return AK_Fail;
    }

    return AK_Success;
}

// Flushes all stream buffers that are not currently granted.
// Sync: None. Always called from within status-protected code.
void CAkAutoStmBase::Flush( )
{
    if ( m_arBuffers.Length( ) > m_uNextToGrant )
    {
#ifdef AK_INSTRUMENT_STM_MGR
		fwprintf_s( m_pDevice->DumpFile(), L"Flush file=%u %u buffers\n", reinterpret_cast<AkUInt32>(m_fileDesc.hFile), m_arBuffers.Length( ) - m_uNextToGrant );
#endif
        int iBuffer = (int)m_arBuffers.Length( ) - 1;
        m_pDevice->LockMem( );
        while ( iBuffer >= m_uNextToGrant )
        {
            AKASSERT( m_uAvailDataSize >= m_arBuffers[iBuffer].uDataSize );
            m_uAvailDataSize -= m_arBuffers[iBuffer].uDataSize;
            AK::MemoryMgr::ReleaseBlock( m_pDevice->GetIOPoolID(), m_arBuffers[iBuffer].pBuffer );
            m_arBuffers.Erase(iBuffer);
            --iBuffer;
        }
        m_pDevice->NotifyMemChange( );
        m_pDevice->UnlockMem( );
    }
}

//////////////////////////////////////////////////////////////////////
//
// AkDeviceDeferredLinedUp.h
//
// Win32 Deferred Scheduler Device implementation.
// Requests to low-level are sent in a lined-up fashion.
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkDeviceDeferredLinedUp.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/Tools/Common/AkMonitorError.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>

#if !defined(WIN32) && !defined(XBOX360)
#error Platform not supported
#endif

using namespace AK;

//--------------------------------------------------------------------
// Defines.
//--------------------------------------------------------------------

CAkDeviceDeferredLinedUp::CAkDeviceDeferredLinedUp( )
{
}

CAkDeviceDeferredLinedUp::~CAkDeviceDeferredLinedUp( )
{
}

// Stream creation interface,
// because we need to initialize specialized stream objects.
// ---------------------------------------------------------------
// Standard stream.
AKRESULT CAkDeviceDeferredLinedUp::CreateStd(
    AkFileDesc &		in_fileDesc,        // Application defined ID.
    AkOpenMode          in_eOpenMode,       // Open mode (read, write, ...).
    IAkStdStream *&     out_pStream         // Returned interface to a standard stream.    
    )
{
    out_pStream = NULL;
    
    AKRESULT eResult = AK_Fail;

    CAkStdStmDeferredLinedUp * pNewStm = NULL;
    pNewStm = AkNew( CAkStreamMgr::GetObjPoolID( ), CAkStdStmDeferredLinedUp( ) );
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
AKRESULT CAkDeviceDeferredLinedUp::CreateAuto(
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
	    AK_MONITOR_ERROR( AK::Monitor::ErrorCode_CannotStartStreamNoMemory );
        return AK_Fail;
    }
    
	CAkAutoStmDeferredLinedUp * pNewStm = NULL;

    // Instantiate new stream object.
    AKRESULT eResult;
	pNewStm = AkNew( CAkStreamMgr::GetObjPoolID( ), CAkAutoStmDeferredLinedUp( ) );
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
		eResult = AK_InsufficientMemory;

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


// This device's implementation of PerformIO(), called by the I/O thread.
void CAkDeviceDeferredLinedUp::PerformIO( )
{
    void * pBuffer;
    CAkStmTask * pTask = SchedulerFindNextTask( pBuffer );

    if ( pTask )
    {
        AKASSERT( pBuffer );    // If scheduler chose a task, it must have provided a valid buffer.
        
        // Post task to Low-Level IO.
        ExecuteTask( pTask, 
                     pBuffer );
    }
}

// Execute task chosen by scheduler.
void CAkDeviceDeferredLinedUp::ExecuteTask( 
    CAkStmTask * in_pTask,
    void * in_pBuffer
    )
{
    AkIOTransferInfo info;

    // Check params.
    AKASSERT( in_pTask != NULL );

    // Get info for IO.
    AkFileDesc * pFileDesc;
    in_pTask->TransferInfo( 
        pFileDesc, 
        info.pOverlapped,
        info.bIsSequential,
        &info.uTransferSize );
    AKASSERT( info.uTransferSize > 0 &&
    		  info.uTransferSize <= m_uGranularity );
    AKASSERT( info.pOverlapped->hEvent != NULL );
    
    // Read or write?
    AKRESULT eResult;
    if ( in_pTask->IsWriteOp( ) )
    {
        // Write.
        eResult = CAkStreamMgr::GetLowLevel( )->Write( 
            *pFileDesc,
            in_pBuffer,
            info );
    }
    else
    {
        // Read.
        eResult = CAkStreamMgr::GetLowLevel( )->Read( 
            *pFileDesc,
            in_pBuffer,
            info );
    }

    if ( eResult == AK_NoDataReady )
    {
        // Operation did not complete. Wait.
        AKASSERT( info.pOverlapped->hEvent );
        DWORD dwRes = ::WaitForSingleObject( info.pOverlapped->hEvent, INFINITE );
        
        if ( dwRes != WAIT_OBJECT_0 )
        {
            AKASSERT( !"Low-level IO destroyed the event" );
            eResult = AK_Fail;
        }
        else
        {
            // Query IO completion.
            eResult = CAkStreamMgr::GetLowLevel( )->GetAsyncResult(
                *pFileDesc,
                info );
        }
    }

    bool bSuccess = true;
    if ( eResult != AK_DataReady &&
         eResult != AK_NoMoreData )
    {
        // Monitor error.
		AK_MONITOR_ERROR( AK::Monitor::ErrorCode_IODevice );
        info.uSizeTransferred = 0;
        bSuccess = false;
    }

    // Update: Locks object, increments size and sets status, unlocks, calls back if needed.
    // According to actual size, success flag and EOF, status could be set to completed, pending or error.
    // Release IO buffer.
    in_pTask->Update( 
        in_pBuffer,
        info.uSizeTransferred,
        bSuccess );
}

//-----------------------------------------------------------------------------
// Stream objects specificity.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Name: class CAkStdStmDeferredLinedUp
// Desc: Overrides methods for deferred lined-up device specificities.
//-----------------------------------------------------------------------------
CAkStdStmDeferredLinedUp::CAkStdStmDeferredLinedUp()
{
}

CAkStdStmDeferredLinedUp::~CAkStdStmDeferredLinedUp()
{
}

// Initializes stream: creates an OVERLAPPED event.
AKRESULT CAkStdStmDeferredLinedUp::Init(
    CAkDeviceBase *     in_pDevice,         // Owner device.
    const AkFileDesc &  in_fileDesc,        // File descriptor.
    AkOpenMode          in_eOpenMode        // Open mode.
    )
{
    AKRESULT eResult = CAkStdStmBase::Init( in_pDevice,
                                            in_fileDesc,
                                            in_eOpenMode );
    if ( eResult == AK_Success )
    {
        AKASSERT( !m_overlapped.hEvent );
        m_overlapped.hEvent = ::CreateEvent( NULL, TRUE, FALSE, NULL ); // Manual reset, init not signaled.
        eResult = ( m_overlapped.hEvent ? AK_Success : AK_Fail );
    }
    return eResult;
}

// Closes stream. Destroys the OVERLAPPED event.
void CAkStdStmDeferredLinedUp::InstantDestroy()
{
    if ( m_overlapped.hEvent )
    {
        AKVERIFY( ::CloseHandle( m_overlapped.hEvent ) );
        m_overlapped.hEvent = NULL;
    }
    CAkStdStmBase::InstantDestroy( );
}

//-----------------------------------------------------------------------------
// Name: class CAkAutoStmDeferredLinedUp
// Desc: Base automatic stream implementation.
//-----------------------------------------------------------------------------
CAkAutoStmDeferredLinedUp::CAkAutoStmDeferredLinedUp()
{
}

CAkAutoStmDeferredLinedUp::~CAkAutoStmDeferredLinedUp()
{
}

// Initializes stream: creates an OVERLAPPED event.
AKRESULT CAkAutoStmDeferredLinedUp::Init( 
    CAkDeviceBase *             in_pDevice,         // Owner device.
    const AkFileDesc &          in_pFileDesc,       // File descriptor.
    const AkAutoStmHeuristics & in_heuristics,      // Streaming heuristics.
    AkAutoStmBufSettings *      in_pBufferSettings, // Stream buffer settings. 
    AkUInt32                    in_uGranularity     // Device's I/O granularity.
    )
{
    AKRESULT eResult = CAkAutoStmBase::Init( in_pDevice,
                                             in_pFileDesc,
                                             in_heuristics,
                                             in_pBufferSettings,
                                             in_uGranularity );
    if ( eResult == AK_Success )
    {
        AKASSERT( !m_overlapped.hEvent );
        m_overlapped.hEvent = ::CreateEvent( NULL, TRUE, FALSE, NULL ); // Manual reset, init not signaled.
        eResult = ( m_overlapped.hEvent ? AK_Success : AK_Fail );
    }
    return eResult;
}

// Closes stream. Destroys the OVERLAPPED event.
void CAkAutoStmDeferredLinedUp::InstantDestroy()
{
    if ( m_overlapped.hEvent )
    {
        AKVERIFY( ::CloseHandle( m_overlapped.hEvent ) );
        m_overlapped.hEvent = NULL;
    }
    CAkAutoStmBase::InstantDestroy( );
}
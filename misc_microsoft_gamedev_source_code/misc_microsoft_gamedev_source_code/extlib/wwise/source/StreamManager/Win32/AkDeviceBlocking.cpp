//////////////////////////////////////////////////////////////////////
//
// AkDeviceBlocking.h
//
// Win32 Blocking Scheduler Device implementation.
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkDeviceBlocking.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/Tools/Common/AkMonitorError.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>

#if !defined(WIN32) && !defined(XBOX360)
#error Platform not supported
#endif

using namespace AK;

CAkDeviceBlocking::CAkDeviceBlocking( )
{
}

CAkDeviceBlocking::~CAkDeviceBlocking( )
{
}

// Scheduler.

// Finds the next task to be executed,
// posts the request to Low-Level IO and blocks until it is completed,
// updates the task.
void CAkDeviceBlocking::PerformIO( )
{
    void * pBuffer;

    CAkStmTask * pTask = SchedulerFindNextTask( pBuffer );

    if ( pTask )
    {
        AKASSERT( pBuffer );    // If scheduler chose a task, it must have provided a valid buffer.

        // Execute.
        ExecuteTask( pTask,
                     pBuffer );
    }
}

// Execute task that was chosen by scheduler.
void CAkDeviceBlocking::ExecuteTask( 
    CAkStmTask * in_pTask,
    void * in_pBuffer
    )
{
    bool bWasSuccessful;
    
    // Check params.
    AKASSERT( in_pTask != NULL );
    // Get info for IO.
    AkFileDesc * pFileDesc;
    AkIOTransferInfo info;
    in_pTask->TransferInfo( 
        pFileDesc,        // Stream's associated file descriptor.
        info.pOverlapped,
        info.bIsSequential,
        &info.uTransferSize );
    AKASSERT( info.uTransferSize > 0 &&
    		  info.uTransferSize <= m_uGranularity );
    
    // Read or write?
    if ( in_pTask->IsWriteOp( ) )
    {
        // Write.
        bWasSuccessful = CAkStreamMgr::GetLowLevel( )->Write( 
            *pFileDesc,
            in_pBuffer,
            info ) == AK_DataReady;
    }
    else
    {
        // Read.
        AKRESULT eResult = CAkStreamMgr::GetLowLevel( )->Read( 
            *pFileDesc,
            in_pBuffer,
            info );
        bWasSuccessful = ( eResult == AK_DataReady || eResult == AK_NoMoreData );
    }

    // Monitor errors.
#ifndef AK_OPTIMIZED
    if ( !bWasSuccessful )
		AK_MONITOR_ERROR( AK::Monitor::ErrorCode_IODevice );
#endif

    // Update: Locks object, increments size and sets status, unlocks, calls back if needed.
    // According to actual size, success flag and EOF, status could be set to completed, pending or error.
    // Release IO buffer.
    in_pTask->Update( 
        in_pBuffer,  
        info.uSizeTransferred,
        bWasSuccessful );

}

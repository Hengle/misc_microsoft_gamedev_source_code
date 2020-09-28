//////////////////////////////////////////////////////////////////////
//
// AkStreamMgr.cpp
//
// Stream manager Windows-specific implementation:
// Device factory.
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkStreamMgr.h"
#include <AK/Tools/Common/AkMonitorError.h>

// Factory products.
#include "AkDeviceBlocking.h"
#include "AkDeviceDeferredLinedUp.h"

using namespace AK;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Declaration of the one and only global pointer to the stream manager.
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
AKSTREAMMGR_API IAkStreamMgr * IAkStreamMgr::m_pStreamMgr = NULL;

//-----------------------------------------------------------------------
// Global variables.
//-----------------------------------------------------------------------
IAkLowLevelIO * CAkStreamMgr::m_pLowLevelIO = NULL;
CAkStreamMgr::AkDeviceArray CAkStreamMgr::m_arDevices;
AkMemPoolId CAkStreamMgr::m_streamMgrPoolId = AK_INVALID_POOL_ID;

//-----------------------------------------------------------------------------
// Factory.
//-----------------------------------------------------------------------------
AK::IAkStreamMgr * AK::CreateStreamMgr( 
    AkStreamMgrSettings * in_pSettings      // Stream manager settings.
    )
{
    // Check memory manager.
    if ( !AK::MemoryMgr::IsInitialized() )
    {
        AKASSERT( !"Memory manager does not exist" );
        return NULL;
    }

    // Factory.
    AKASSERT( AK::IAkStreamMgr::m_pStreamMgr == NULL || !"CreateStreamMgr( ) should be called only once" );
    if ( AK::IAkStreamMgr::m_pStreamMgr == NULL )
    {
        // Check init parameters.
        if ( in_pSettings == NULL ||
             in_pSettings->pLowLevelIO == NULL )
        {
            AKASSERT( !"Invalid stream settings" );
            AKASSERT( in_pSettings->pLowLevelIO || 
                      !"Invalid low-level IO interface" );
            return NULL;
        }
        
        // Create stream manager.
        if ( CAkStreamMgr::m_streamMgrPoolId == AK_INVALID_POOL_ID )
        {
            // Create stream manager objects pool.
            CAkStreamMgr::m_streamMgrPoolId = AK::MemoryMgr::CreatePool( NULL,
                                                    in_pSettings->uMemorySize,
                                                    AK_STM_OBJ_POOL_BLOCK_SIZE,
                                                    AkMalloc );
        }
	    if ( CAkStreamMgr::m_streamMgrPoolId == AK_INVALID_POOL_ID )
		{
            AKASSERT( !"Stream manager pool creation failed" );
			return NULL;
		}
		AK_SETPOOLNAME(CAkStreamMgr::m_streamMgrPoolId,L"Stream Manager");
        
        // Instantiate stream manager.
        CAkStreamMgr * pStreamMgr = AkNew( CAkStreamMgr::m_streamMgrPoolId, CAkStreamMgr( ) );
        
        // Initialize.
        if ( pStreamMgr != NULL )
        {
            if ( pStreamMgr->Init( in_pSettings ) != AK_Success )
            {
                // Failed. Clean up.
                AKASSERT( !"Failed intializing stream manager" );
                pStreamMgr->Destroy( );
                pStreamMgr = NULL;
            }
        }

        // If instantiation failed, need to destroy stm mgr pool. 
        if ( pStreamMgr == NULL )
        {
            AKVERIFY( AK::MemoryMgr::DestroyPool( CAkStreamMgr::m_streamMgrPoolId ) == AK_Success );
        }
    }
        
	AKASSERT( AK::IAkStreamMgr::m_pStreamMgr != NULL );
    return AK::IAkStreamMgr::m_pStreamMgr;
}

// Device creation.
AkDeviceID AK::CreateDevice(
    const AkDeviceSettings *  in_pDeviceSettings  // Device settings.
    )
{
    return static_cast<CAkStreamMgr*>(AK::IAkStreamMgr::Get( ))->CreateDevice( in_pDeviceSettings );
}
AKRESULT AK::DestroyDevice(
    AkDeviceID                  in_deviceID         // Device ID.
    )
{
    return static_cast<CAkStreamMgr*>(AK::IAkStreamMgr::Get( ))->DestroyDevice( in_deviceID );
}

void CAkStreamMgr::Destroy()
{
    Term( );

    // Destroy singleton.
    AKASSERT( AK::MemoryMgr::IsInitialized() &&
              m_streamMgrPoolId != AK_INVALID_POOL_ID );
    if ( AK::MemoryMgr::IsInitialized() &&
         m_streamMgrPoolId != AK_INVALID_POOL_ID )
    {
        AkDelete( m_streamMgrPoolId, this );
    }

    // Destroy stream manager pool.
    AKVERIFY( AK::MemoryMgr::DestroyPool( m_streamMgrPoolId ) == AK_Success );
    m_streamMgrPoolId = AK_INVALID_POOL_ID;
}

CAkStreamMgr::CAkStreamMgr()
{
    // Assign global pointer.
    m_pStreamMgr = this;
}

CAkStreamMgr::~CAkStreamMgr()
{
    // Reset global pointer.
    m_pStreamMgr = NULL;
}


// Initialise/Terminate.
//-------------------------------------
AKRESULT CAkStreamMgr::Init(
    AkStreamMgrSettings * in_pSettings
    )
{
    AKASSERT( in_pSettings &&
              in_pSettings->pLowLevelIO );

    m_pLowLevelIO = in_pSettings->pLowLevelIO;

	return AK_Success;
}

void CAkStreamMgr::Term()
{
    // Term devices.
    AkDeviceArray::Iterator it = m_arDevices.Begin( );
    while ( it != m_arDevices.End( ) )
    {
        AKASSERT( (*it) );
        (*it)->Destroy( );
        ++it;
    }
    m_arDevices.Term( );
}

//-----------------------------------------------------------------------------
// Device management.
// Warning: These functions are not thread safe.
//-----------------------------------------------------------------------------
// Device creation.
AkDeviceID CAkStreamMgr::CreateDevice(
    const AkDeviceSettings *  in_pDeviceSettings  // Device settings.
    )
{
    // Note. Buffer size could be anything. Scheduler type is verified below.

    AkDeviceID newDeviceID = (AkDeviceID)m_arDevices.Length( );

    IAkDevice * pNewDevice = NULL;
    AKRESULT eResult = AK_Fail;

    // Device factory.
    if ( in_pDeviceSettings->uSchedulerTypeFlags & AK_SCHEDULER_BLOCKING )
    {
        // AK_SCHEDULER_BLOCKING
        pNewDevice = AkNew( m_streamMgrPoolId, CAkDeviceBlocking( ) );
        if ( pNewDevice != NULL )
        {
            eResult = pNewDevice->Init( *in_pDeviceSettings,
                                        newDeviceID );
        }
        AKASSERT( eResult == AK_Success || !"Cannot initialize IO device" );
    }
    else if ( in_pDeviceSettings->uSchedulerTypeFlags & AK_SCHEDULER_DEFERRED_LINED_UP )
    {
        // AK_SCHEDULER_DEFERRED_LINED_UP.
        pNewDevice = AkNew( m_streamMgrPoolId, CAkDeviceDeferredLinedUp( ) );
        if ( pNewDevice != NULL )
        {
            eResult = pNewDevice->Init( *in_pDeviceSettings,
                                        newDeviceID );
        }
        AKASSERT( eResult == AK_Success || !"Cannot initialize IO device" );
    }
    else
    {
        AKASSERT( !"Invalid device type" );
        return AK_INVALID_DEVICE_ID;
    }

    if ( eResult != AK_Success ||
         !pNewDevice )
    {
        if ( pNewDevice )
            pNewDevice->Destroy( );
        return AK_INVALID_DEVICE_ID;
    }

    // Add device to list.
    if ( !m_arDevices.AddLast( pNewDevice ) )
    {
        AKASSERT( !"Could not add new device to list" );
        
        // Cleanup.
        pNewDevice->Destroy( );

        return AK_INVALID_DEVICE_ID;
    }

    return newDeviceID;
}

// Warning: This function is not thread safe. No stream should exist for that device when it is destroyed.
AKRESULT   CAkStreamMgr::DestroyDevice(
    AkDeviceID          in_deviceID         // Device ID.
    )
{
    if ( (AkUInt32)in_deviceID >= m_arDevices.Length( ) )
    {
        AKASSERT( !"Invalid device ID" );
        return AK_InvalidParameter;
    }

    m_arDevices[in_deviceID]->Destroy( );

    return m_arDevices.Remove( m_arDevices[in_deviceID] );
}


// Stream creation interface.
// ------------------------------------------------------


// Standard stream open methods.
// -----------------------------

// String overload.
AKRESULT CAkStreamMgr::CreateStd(
    AkLpCtstr           in_pszFileName,     // Application defined string (title only, or full path, or code...).
    AkFileSystemFlags * in_pFSFlags,        // Special file system flags. Can pass NULL.
    AkOpenMode          in_eOpenMode,       // Open mode (read, write, ...).
    IAkStdStream *&     out_pStream         // Returned interface to a standard stream.
    )
{
    // Check parameters.
    if ( in_pszFileName == NULL )
    {
        AKASSERT( !"Invalid file name" );
        return AK_InvalidParameter;
    }

    AkFileDesc fileDesc;
    AKRESULT eRes = m_pLowLevelIO->Open( in_pszFileName,
                                         in_eOpenMode,
                                         in_pFSFlags,
                                         fileDesc );
    if ( eRes != AK_Success )
    {
#ifndef AK_OPTIMIZED
        // HACK: Hide monitoring errors for banks that are not found in bIsLanguageSpecific directory.
        if ( in_pFSFlags &&
             in_pFSFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC &&
             in_pFSFlags->uCodecID == AKCODECID_BANK && 
             in_pFSFlags->bIsLanguageSpecific )
             return eRes;

        // Monitor error.
        if ( eRes == AK_FileNotFound )
			AK_MONITOR_ERROR( AK::Monitor::ErrorCode_FileNotFound );
        else // ( eRes == AK_Fail ) - any other reason.
            AK_MONITOR_ERROR( AK::Monitor::ErrorCode_CannotOpenFile );
#endif
        return eRes;
    }

    IAkDevice * pDevice = GetDevice( fileDesc.deviceID );
    if ( !pDevice )
        return AK_Fail;

    return pDevice->CreateStd( fileDesc,
                               in_eOpenMode,
                               out_pStream );
}

// ID overload.
AKRESULT CAkStreamMgr::CreateStd(
    AkFileID            in_fileID,          // Application defined ID.
    AkFileSystemFlags * in_pFSFlags,        // Special file system flags. Can pass NULL.
    AkOpenMode          in_eOpenMode,       // Open mode (read, write, ...).
    IAkStdStream *&     out_pStream         // Returned interface to a standard stream.
    )
{
    AkFileDesc fileDesc;
    AKRESULT eRes = m_pLowLevelIO->Open( in_fileID,
                                         in_eOpenMode,
                                         in_pFSFlags,
                                         fileDesc );
    if ( eRes != AK_Success )
    {
#ifndef AK_OPTIMIZED
        // HACK: Hide monitoring errors for banks that are not found in bIsLanguageSpecific directory.
        if ( in_pFSFlags &&
             in_pFSFlags->uCompanyID == AKCOMPANYID_AUDIOKINETIC &&
             in_pFSFlags->uCodecID == AKCODECID_BANK && 
             in_pFSFlags->bIsLanguageSpecific )
             return eRes;

        // Monitor error.
        if ( eRes == AK_FileNotFound )
			AK_MONITOR_ERROR( AK::Monitor::ErrorCode_FileNotFound );
        else // ( eRes == AK_Fail ) - any other reason.
			AK_MONITOR_ERROR( AK::Monitor::ErrorCode_CannotOpenFile );
#endif
        return eRes;
    }

    IAkDevice * pDevice = GetDevice( fileDesc.deviceID );
    if ( !pDevice )
        return AK_Fail;

    return pDevice->CreateStd( fileDesc,
                               in_eOpenMode,
                               out_pStream );
}


// Automatic stream open methods.
// ------------------------------

// String overload.
AKRESULT CAkStreamMgr::CreateAuto(
    AkLpCtstr                   in_pszFileName,     // Application defined string (title only, or full path, or code...).
    AkFileSystemFlags *         in_pFSFlags,        // Special file system flags. Can pass NULL.
    const AkAutoStmHeuristics & in_heuristics,      // Streaming heuristics.
    AkAutoStmBufSettings *      in_pBufferSettings, // Stream buffer settings. Pass NULL to use defaults (recommended).
    IAkAutoStream *&            out_pStream         // Returned interface to an automatic stream.
    )
{
    // Check parameters.
    if ( in_pszFileName == NULL )
    {
        AKASSERT( !"Invalid file name" );
        return AK_InvalidParameter;
    }
    if ( in_heuristics.fThroughput < 0 ||
         in_heuristics.priority < AK_MIN_PRIORITY ||
         in_heuristics.priority > AK_MAX_PRIORITY )
    {
        AKASSERT( !"Invalid automatic stream heuristic" );
        return AK_InvalidParameter;
    }

    AkFileDesc fileDesc;
    AKRESULT eRes = m_pLowLevelIO->Open( in_pszFileName,
                                         AK_OpenModeRead,  // Always read from an autostream.
                                         in_pFSFlags,
                                         fileDesc );
    if ( eRes != AK_Success )
    {
#ifndef AK_OPTIMIZED
        // Monitor error.
        if ( eRes == AK_FileNotFound )
            AK_MONITOR_ERROR( AK::Monitor::ErrorCode_FileNotFound );
        else // ( eRes == AK_Fail ) - any other reason.
            AK_MONITOR_ERROR( AK::Monitor::ErrorCode_CannotOpenFile );
#endif
        return eRes;
    }

    IAkDevice * pDevice = GetDevice( fileDesc.deviceID );
    if ( !pDevice )
        return AK_Fail;

#ifdef AK_INSTRUMENT_STM_MGR
	CAkDeviceBase * pDeviceBase = (CAkDeviceBase*)(pDevice);
	fwprintf_s( pDeviceBase->DumpFile(), L"Open %s in %u (size=%lld)\n", in_pszFileName, reinterpret_cast<AkUInt32>(fileDesc.hFile), fileDesc.iFileSize );
#endif


    return pDevice->CreateAuto( fileDesc,
                                in_heuristics,
                                in_pBufferSettings,
                                out_pStream );
}

// ID overload.
AKRESULT CAkStreamMgr::CreateAuto(
    AkFileID                    in_fileID,          // Application defined ID.
    AkFileSystemFlags *         in_pFSFlags,        // Special file system flags. Can pass NULL.
    const AkAutoStmHeuristics & in_heuristics,      // Streaming heuristics.
    AkAutoStmBufSettings *      in_pBufferSettings, // Stream buffer settings. Pass NULL to use defaults (recommended).
    IAkAutoStream *&            out_pStream         // Returned interface to an automatic stream.
    )
{
    // Check parameters.
    if ( in_heuristics.fThroughput < 0 ||
         in_heuristics.priority < AK_MIN_PRIORITY ||
         in_heuristics.priority > AK_MAX_PRIORITY )
    {
        AKASSERT( !"Invalid automatic stream heuristic" );
        return AK_InvalidParameter;
    }

    AkFileDesc fileDesc;
    AKRESULT eRes = m_pLowLevelIO->Open( in_fileID,
                                         AK_OpenModeRead,  // Always read from an autostream.
                                         in_pFSFlags,
                                         fileDesc );
    if ( eRes != AK_Success )
    {
#ifndef AK_OPTIMIZED
        // Monitor error.
        if ( eRes == AK_FileNotFound )
            AK_MONITOR_ERROR( AK::Monitor::ErrorCode_FileNotFound );
        else // ( eRes == AK_Fail ) - any other reason.
            AK_MONITOR_ERROR( AK::Monitor::ErrorCode_CannotOpenFile );
#endif
        return eRes;
    }

    IAkDevice * pDevice = GetDevice( fileDesc.deviceID );
    if ( !pDevice )
        return AK_Fail;

#ifdef AK_INSTRUMENT_STM_MGR
	CAkDeviceBase * pDeviceBase = (CAkDeviceBase*)(pDevice);
	fwprintf_s( pDeviceBase->DumpFile(), L"Open ID=%u in %u (size=%lld)\n", in_fileID, reinterpret_cast<AkUInt32>(fileDesc.hFile), fileDesc.iFileSize );
#endif

    return pDevice->CreateAuto( fileDesc,
                                in_heuristics,
                                in_pBufferSettings,
                                out_pStream );
}


// Profiling interface.
// --------------------------------------------------------------------
#ifndef AK_OPTIMIZED
IAkStreamMgrProfile * CAkStreamMgr::GetStreamMgrProfile()
{
    return this;
}

// Device enumeration.
AkUInt32 CAkStreamMgr::GetNumDevices()
{
    return m_arDevices.Length( );
}

IAkDeviceProfile * CAkStreamMgr::GetDeviceProfile( 
    AkUInt32 in_uDeviceIndex    // [0,numStreams[
    )
{
    if ( in_uDeviceIndex >= m_arDevices.Length( ) )
    {
        AKASSERT( !"Invalid device index" );
        return NULL;
    }
    AKASSERT( m_arDevices[in_uDeviceIndex] != NULL ||
              !"Device was not instantiated" );
    
    return m_arDevices[in_uDeviceIndex];
}

AKRESULT CAkStreamMgr::StartMonitoring()
{
    for ( unsigned int u=0; u<m_arDevices.Length( ); u++ )
    {
        AKVERIFY( m_arDevices[u]->StartMonitoring( ) == AK_Success );
    }
    return AK_Success;
}

void CAkStreamMgr::StopMonitoring()
{
    for ( unsigned int u=0; u<m_arDevices.Length( ); u++ )
    {
        m_arDevices[u]->StopMonitoring( );
    }
}

#endif

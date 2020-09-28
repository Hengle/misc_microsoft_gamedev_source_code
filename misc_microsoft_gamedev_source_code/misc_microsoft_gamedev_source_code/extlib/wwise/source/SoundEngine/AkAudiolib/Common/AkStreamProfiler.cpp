/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2008.2.1  Build: 2821
  Copyright (c) 2006-2008 Audiokinetic Inc.
 ***********************************************************************/

//////////////////////////////////////////////////////////////////////
//
// AkStreamProfiler.cpp
//
// Stream profiler implementation.
// Holds and manages the list of StreamProfile interfaces.
// Builds only in non-optimized configurations.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AkStreamProfiler.h"
#include <AK/Tools/Common/AkAutoLock.h>

#ifndef AK_OPTIMIZED

namespace AK
{
    namespace STREAMMGR
    {
        CAkStreamProfiler * g_pStreamProfiler = NULL;
    }
}


CAkStreamProfiler * CAkStreamProfiler::Create( 
    unsigned int in_uiNumStreamsMax,
    AkMemPoolId in_poolId 
    )
{
    AKASSERT( AK::STREAMMGR::g_pStreamProfiler == NULL );
    if ( AK::STREAMMGR::g_pStreamProfiler == NULL &&
         in_poolId != AK_INVALID_POOL_ID )
    {
        AK::STREAMMGR::g_pStreamProfiler = AkNew( in_poolId, CAkStreamProfiler );
        if ( AK::STREAMMGR::g_pStreamProfiler->Init( in_uiNumStreamsMax, 
                                                     in_poolId ) != AK_Success )
        {
            AKASSERT( !"Failed initializing stream profiler" );
            AK::STREAMMGR::g_pStreamProfiler->Destroy( );
        }
    }
    AKASSERT( AK::STREAMMGR::g_pStreamProfiler );
    return AK::STREAMMGR::g_pStreamProfiler;
}

void CAkStreamProfiler::Destroy( void )
{
    Term( );
    AKASSERT( m_poolId != AK_INVALID_POOL_ID );
    AkDelete( m_poolId, this );
}

CAkStreamProfiler::CAkStreamProfiler( )
{
    m_poolId = AK_INVALID_POOL_ID;
    AK::STREAMMGR::g_pStreamProfiler = this;
}

CAkStreamProfiler::~CAkStreamProfiler( )
{
    AK::STREAMMGR::g_pStreamProfiler = NULL;
}

AKRESULT CAkStreamProfiler::Init( 
        unsigned int in_uiNumStreamsMax,
        AkMemPoolId in_poolId )
{
/*
    m_poolId = in_poolId;
    m_bIsMonitoring = false;
    m_ulNextStreamID = 0;

    return m_arStreamProfiles.Init( in_uiNumStreamsMax, 
                                    in_poolId );
*/
    return AK_Success;
}

void CAkStreamProfiler::Term( void )
{
/*
    // Destroy all streams that were waiting for destruction,
    // in the case the profiler did not get the chance to actualize the list.
    StopMonitoring( );
    AKVERIFY( m_arStreamProfiles.Term( ) == AK_Success );
*/
}

/*
AKRESULT CAkStreamProfiler::StartMonitoring( void )
{
    m_bIsMonitoring = true;
    return AK_Success;
}

void CAkStreamProfiler::StopMonitoring( void )
{
    AkAutoLock<CAkLock> profileLock( m_lockProfilingData );
    
    m_bIsMonitoring = false;
    // Remove all stream profiles from list. Destroy those that are scheduled to be destroyed.
    ArrayStreamProfiles::Iterator it = m_arStreamProfiles.Begin( );
    while ( it != m_arStreamProfiles.End( ) )
    {
        // If this module is terminating, all streams should have already been scheduled to be destroyed.
        if ( (*it)->IsToBeDestroyed( ) )
        {
            (*it)->InstantDestroy( );
        }
        m_arStreamProfiles.Erase( it );
    }
}

unsigned int CAkStreamProfiler::GetNumStreams( void )
{
    AkAutoLock<CAkLock> profileLock( m_lockProfilingData );

    // Go through all the streams. Destroy those that are not new and scheduled to be destroyed.
    // Return number of streams, return number of New streams if out_pulNumNewStreams is not NULL.
    ArrayStreamProfiles::Iterator it = m_arStreamProfiles.Begin( );
    while ( it != m_arStreamProfiles.End( ) )
    {
        // If it is scheduled for destruction and it is not new, destroy and remove from list.
        if ( (*it)->IsToBeDestroyed( ) &&
             !(*it)->IsNew( ) )
        {
            (*it)->InstantDestroy( );
            m_arStreamProfiles.EraseSwap( it );
        }
        else
        {
            // Otherwise, increment number of streams.
            // Also increment number of new streams if new and argument specified.
            ++it;
        }
    }
    return m_arStreamProfiles.Length( );
}

AK::IAkStreamProfileEx * CAkStreamProfiler::GetStreamProfile( 
                unsigned int in_uiStreamIndex             // [0,numStreams[
                )
{
    AkAutoLock<CAkLock> profileLock( m_lockProfilingData );

    // Get stream profile and return.
    return m_arStreamProfiles[in_uiStreamIndex];
}

void CAkStreamProfiler::RegisterStream( 
                AK::IAkStreamProfileEx * in_pNewStream
                )
{
    AkAutoLock<CAkLock> profileLock( m_lockProfilingData );

    // Compute and assign a new unique stream ID.
    in_pNewStream->SetStreamID(
                        ++m_ulNextStreamID // Gen stream ID.
                        );
    // Add at the end of the array.
    AKVERIFY( m_arStreamProfiles.AddLast( in_pNewStream ) );
}

void CAkStreamProfiler::UnregisterStream( 
        AK::IAkStreamProfileEx * in_pNewStream
        )
{
    bool bFound = false;
    AkAutoLock<CAkLock> profileLock( m_lockProfilingData );

    // Find the stream and remove.
    ArrayStreamProfiles::Iterator it = m_arStreamProfiles.Begin( );
    while ( it != m_arStreamProfiles.End( ) )
    {
        if ( (*it) == in_pNewStream )
        {
            m_arStreamProfiles.Erase( it );
            bFound = true;
            break;
        }
        else
            ++it;            
    }
}
*/
#endif
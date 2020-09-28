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
// AkStreamProfiler.h
//
// Stream profiler implementation.
// Holds and manages the list of StreamProfile interfaces.
// Builds only in non-optimized configurations.
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_STREAM_PROFILER_H_
#define _AK_STREAM_PROFILER_H_

#ifndef AK_OPTIMIZED

#include <AK/SoundEngine/Common/IAkStreamMgr.h>
#include "IAkStreamProfilerEx.h"
#include <AK/Tools/Common/AkArray.h>
#include <AK/Tools/Common/AkLock.h>
#include <AK/Tools/Common/AkObject.h>

// Profiling.
class CAkStreamProfiler : public CAkObject
{
public:
    // Factory/Destroy.
    static CAkStreamProfiler * Create( 
        unsigned int in_uiNumStreamsMax,            // Max number of streams.
        AkMemPoolId in_poolId                       // Pool ID for stream profile interfaces.
        );
    void Destroy( void );
/*
    // Monitoring status.
    AKRESULT StartMonitoring( void );
	void     StopMonitoring( void );
    inline bool IsMonitoring( void ) { return m_bIsMonitoring; }
    
    // Stream profiling.
    unsigned int GetNumStreams( void );
    // Note. The following function refer to streams by index, which must honor the call to GetNumStreams().
    virtual AK::IAkStreamProfileEx * GetStreamProfile( 
        unsigned int in_uiStreamIndex             // [0,numStreams[
        );
    
    // Register a stream profile interface.
    void RegisterStream( 
        AK::IAkStreamProfileEx * in_pNewStream
        );
    void UnregisterStream( 
        AK::IAkStreamProfileEx * in_pNewStream
        );
  */      

private:
    CAkStreamProfiler( );
    CAkStreamProfiler( const CAkStreamProfiler& );
    virtual ~CAkStreamProfiler( );
    AKRESULT Init( 
        unsigned int in_uiNumStreamsMax,
        AkMemPoolId in_poolId 
        );
    void Term( void );
    
    AkMemPoolId m_poolId;
    /*
    bool        m_bIsMonitoring;
    AkUInt32    m_ulNextStreamID;
    typedef AkArray<AK::IAkStreamProfileEx*,AK::IAkStreamProfileEx*,0> ArrayStreamProfiles;
    ArrayStreamProfiles m_arStreamProfiles;
    CAkLock             m_lockProfilingData;    // Protect all profile globals.
    */
};

namespace AK
{
    namespace STREAMMGR
    {
        extern CAkStreamProfiler * g_pStreamProfiler;
    }
}

#endif

#endif //_AK_STREAM_PROFILER_H_
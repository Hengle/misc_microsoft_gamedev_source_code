//////////////////////////////////////////////////////////////////////
//
// IAkDevice.h
//
// I/O Device interface definition.
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////
#ifndef _IAK_DEVICE_H_
#define _IAK_DEVICE_H_

#include <AK/SoundEngine/Common/IAkStreamMgr.h>

// Platform-specific initialization settings.
struct AkDeviceSettings;

namespace AK
{

    //-----------------------------------------------------------------------------
    // Name: class IAkDevice
    // Desc: High-level interface of an I/O device. 
    //-----------------------------------------------------------------------------
    class IAkDevice 
#ifndef AK_OPTIMIZED
    : public AK::IAkDeviceProfile
#endif
    {
    public:
        virtual AKRESULT        Init( 
            const AkDeviceSettings & in_pSettings,
            AkDeviceID               in_deviceID 
            ) = 0;
        virtual void            Destroy() = 0;

        // Returns device ID, defined by the stream manager.
        virtual AkDeviceID      GetDeviceID() = 0;
        
        // Stream open interface.
        // --------------------------------------------------------

        // Standard stream.
        virtual AKRESULT        CreateStd(
            AkFileDesc &				in_fileDesc,        // Application defined ID.
            AkOpenMode                  in_eOpenMode,       // Open mode (read, write, ...).
            IAkStdStream *&             out_pStream         // Returned interface to a standard stream.
            ) = 0;

        
        // Automatic stream
        virtual AKRESULT        CreateAuto(
            AkFileDesc &				in_fileDesc,        // Application defined ID.
            const AkAutoStmHeuristics & in_heuristics,      // Streaming heuristics.
            AkAutoStmBufSettings *      in_pBufferSettings, // Stream buffer settings. Pass NULL to use defaults (recommended).
            IAkAutoStream *&            out_pStream         // Returned interface to an automatic stream.
            ) = 0;

        
        // Profiling interface.
        // --------------------------------------------------------

    #ifndef AK_OPTIMIZED
        
        virtual AKRESULT        StartMonitoring() = 0;
	    virtual void            StopMonitoring() = 0;

    #endif

    };
}

#endif //_IAK_DEVICE_H_

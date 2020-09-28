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
#ifndef _AK_DEVICE_DEFERRED_LINEDUP_H_
#define _AK_DEVICE_DEFERRED_LINEDUP_H_

#include "AkDeviceBase.h"

namespace AK
{

    //-----------------------------------------------------------------------------
    // Name: CAkDeviceDeferredLinedUp
    // Desc: Implementation of the deferred lined-up scheduler.
    //-----------------------------------------------------------------------------
    class CAkDeviceDeferredLinedUp : public CAkDeviceBase
    {
    public:

        CAkDeviceDeferredLinedUp();
        virtual ~CAkDeviceDeferredLinedUp();
        
        // Stream creation interface override
        // (because we need to initialize specialized stream objects).
        // ---------------------------------------------------------------
        // Standard stream.
        virtual AKRESULT CreateStd(
            AkFileDesc &		in_fileDesc,        // Application defined ID.
            AkOpenMode          in_eOpenMode,       // Open mode (read, write, ...).
            IAkStdStream *&     out_pStream         // Returned interface to a standard stream.
            );
        // Automatic stream.
        virtual AKRESULT CreateAuto(
            AkFileDesc &				in_fileDesc,        // Application defined ID.
            const AkAutoStmHeuristics & in_heuristics,      // Streaming heuristics.
            AkAutoStmBufSettings *      in_pBufferSettings, // Stream buffer settings. Pass NULL to use defaults (recommended).
            IAkAutoStream *&            out_pStream         // Returned interface to an automatic stream.
            );

    protected:
        
        // This device's implementation of PerformIO().
        virtual void PerformIO( );

        // Execute task chosen by scheduler.
        void ExecuteTask( 
            CAkStmTask * in_pTask,
            void * in_pBuffer
            );
    };

    //-----------------------------------------------------------------------------
    // Stream objects specificity.
    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    // Name: class CAkStdStmDeferredLinedUp
    // Desc: Overrides methods for deferred lined-up device specificities.
    //-----------------------------------------------------------------------------
    class CAkStdStmDeferredLinedUp : public CAkStdStmBase
    {
    public:

        CAkStdStmDeferredLinedUp();
        virtual ~CAkStdStmDeferredLinedUp();

        // Initializes stream: creates an OVERLAPPED event.
        virtual AKRESULT Init(
            CAkDeviceBase *     in_pDevice,         // Owner device.
            const AkFileDesc &  in_fileDesc,        // File descriptor.
            AkOpenMode          in_eOpenMode        // Open mode.
            );

        // Closes stream. Destroys the OVERLAPPED event.
        virtual void      InstantDestroy();
    };

    //-----------------------------------------------------------------------------
    // Name: class CAkAutoStmDeferredLinedUp
    // Desc: Base automatic stream implementation.
    //-----------------------------------------------------------------------------
    class CAkAutoStmDeferredLinedUp : public CAkAutoStmBase
    {
    public:

        CAkAutoStmDeferredLinedUp();
        virtual ~CAkAutoStmDeferredLinedUp();

        // Initializes stream: creates an OVERLAPPED event.
        virtual AKRESULT Init( 
            CAkDeviceBase *             in_pDevice,         // Owner device.
            const AkFileDesc &          in_pFileDesc,       // File descriptor.
            const AkAutoStmHeuristics & in_heuristics,      // Streaming heuristics.
            AkAutoStmBufSettings *      in_pBufferSettings, // Stream buffer settings.
            AkUInt32                    in_uGranularity     // Device's I/O granularity.
            );

        // Closes stream. Destroys the OVERLAPPED event.
        virtual void      InstantDestroy();
    };
}

#endif //_AK_DEVICE_DEFERRED_LINEDUP_H_

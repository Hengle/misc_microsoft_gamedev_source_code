//////////////////////////////////////////////////////////////////////
//
// AkStreamMgr.h
//
// Stream manager Windows-specific implementation:
// Device factory.
// Platform-specific scheduling strategy.
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_STREAM_MGR_H_
#define _AK_STREAM_MGR_H_

#include <AK/SoundEngine/Common/IAkStreamMgr.h>
#include "IAkDevice.h"
#include <AK/Tools/Common/AkObject.h>
#include <AK/Tools/Common/AkArray.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>

#if !defined(WIN32) && !defined(XBOX360)
#error Platform not supported
#endif

#define AK_STM_OBJ_POOL_BLOCK_SIZE      (32)

namespace AK
{

    //-----------------------------------------------------------------------------
    // Name: class CAkStreamMgr
    // Desc: Implementation of the stream manager.
    //-----------------------------------------------------------------------------
    class CAkStreamMgr : public CAkObject
						,public IAkStreamMgr
#ifndef AK_OPTIMIZED
                        ,public IAkStreamMgrProfile
#endif
    {
        // Public factory.
        friend IAkStreamMgr * CreateStreamMgr( 
            AkStreamMgrSettings * in_pSettings      // Stream manager settings.
            );

        // Device management.
        // Warning: This function is not thread safe.
        friend AkDeviceID CreateDevice(
            const AkDeviceSettings *    in_pDeviceSettings  // Device settings.
            );
        // Warning: This function is not thread safe. No stream should exist for that device when it is destroyed.
        friend AKRESULT   DestroyDevice(
            AkDeviceID                  in_deviceID         // Device ID.
            );

    public:

        // Stream manager destruction.
        virtual void     Destroy();

        // Globals access.
        inline static IAkLowLevelIO * GetLowLevel()
        {
            return m_pLowLevelIO;		// Low-level IO interface.
        }
        inline static AkMemPoolId GetObjPoolID()
        {
            return m_streamMgrPoolId;   // Stream manager instance, devices, objects.
        }

        // Stream creation interface.
        // ------------------------------------------------------
        
        // Standard stream create methods.
        // -----------------------------

        // String overload.
        virtual AKRESULT CreateStd(
            AkLpCtstr           in_pszFileName,     // Application defined string (title only, or full path, or code...).
            AkFileSystemFlags * in_pFSFlags,        // Special file system flags. Can pass NULL.
            AkOpenMode          in_eOpenMode,       // Open mode (read, write, ...).
            IAkStdStream *&     out_pStream         // Returned interface to a standard stream.
            );
        // ID overload.
        virtual AKRESULT CreateStd(
            AkFileID            in_fileID,          // Application defined ID.
            AkFileSystemFlags * in_pFSFlags,        // Special file system flags. Can pass NULL.
            AkOpenMode          in_eOpenMode,       // Open mode (read, write, ...).
            IAkStdStream *&     out_pStream         // Returned interface to a standard stream.
            );

        
        // Automatic stream create methods.
        // ------------------------------

        // Note: Open does not start automatic streams. 
        // They need to be started explicitly with IAkAutoStream::Start().

        // String overload.
        virtual AKRESULT CreateAuto(
            AkLpCtstr                   in_pszFileName,     // Application defined string (title only, or full path, or code...).
            AkFileSystemFlags *         in_pFSFlags,        // Special file system flags. Can pass NULL.
            const AkAutoStmHeuristics & in_heuristics,      // Streaming heuristics.
            AkAutoStmBufSettings *      in_pBufferSettings, // Stream buffer settings. Pass NULL to use defaults (recommended).
            IAkAutoStream *&            out_pStream         // Returned interface to an automatic stream.
            );
        // ID overload.
        virtual AKRESULT CreateAuto(
            AkFileID                    in_fileID,          // Application defined ID.
            AkFileSystemFlags *         in_pFSFlags,        // Special file system flags. Can pass NULL.
            const AkAutoStmHeuristics & in_heuristics,      // Streaming heuristics.
            AkAutoStmBufSettings *      in_pBufferSettings, // Stream buffer settings. Pass NULL to use defaults (recommended).
            IAkAutoStream *&            out_pStream         // Returned interface to an automatic stream.
            );

        // -----------------------------------------------

        // Profiling interface.
        // -----------------------------------------------

    #ifndef AK_OPTIMIZED
        // Profiling access.
        virtual IAkStreamMgrProfile * GetStreamMgrProfile();

        // Public profiling interface.
        // ---------------------------
        virtual AKRESULT StartMonitoring();
	    virtual void     StopMonitoring();

        // Devices enumeration.
        virtual AkUInt32 GetNumDevices();         // Returns number of devices.
        virtual IAkDeviceProfile * GetDeviceProfile( 
            AkUInt32 in_uDeviceIndex              // [0,numDevices[
            );
    #endif

    private:

        // Device management.
        // -----------------------------------------------

        // Warning: This function is not thread safe.
        AkDeviceID CreateDevice(
            const AkDeviceSettings *    in_pDeviceSettings  // Device settings.
            );
        // Warning: This function is not thread safe. No stream should exist for that device when it is destroyed.
        AKRESULT   DestroyDevice(
            AkDeviceID                  in_deviceID         // Device ID.
            );

        // Get device by ID.
        inline IAkDevice * GetDevice( 
            AkDeviceID  in_deviceID 
            )
        {
	        if ( (AkUInt32)in_deviceID >= m_arDevices.Length( ) )
	        {
	            AKASSERT( !"Invalid device ID" );
	            return NULL;
	        }
	        return m_arDevices[in_deviceID];
	    }

        // Singleton.
        CAkStreamMgr();
        CAkStreamMgr( CAkStreamMgr& );
        CAkStreamMgr & operator=( CAkStreamMgr& );
        virtual ~CAkStreamMgr();

        // Initialise/Terminate.
        AKRESULT Init( 
            AkStreamMgrSettings *   in_pSettings        // Stream manager settings.
            );
        void     Term();

        // Globals: pools and low-level IO interface.
	    static AkMemPoolId      m_streamMgrPoolId;      // Stream manager instance, devices, objects.
        static IAkLowLevelIO *  m_pLowLevelIO;		    // Low-level IO interface.

        // Array of devices.
        AK_DEFINE_ARRAY_POOL( ArrayPoolLocal, CAkStreamMgr::m_streamMgrPoolId );
        typedef AkArray<IAkDevice*,IAkDevice*, ArrayPoolLocal, 1> AkDeviceArray;
        static AkDeviceArray m_arDevices;
    };
}
#endif //_AK_STREAM_MGR_H_

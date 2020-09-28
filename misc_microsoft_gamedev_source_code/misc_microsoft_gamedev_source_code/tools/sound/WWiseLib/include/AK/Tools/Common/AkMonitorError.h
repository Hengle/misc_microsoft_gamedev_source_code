//////////////////////////////////////////////////////////////////////
//
// Copyright 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

namespace AK
{
    // Error monitoring.
    // Note: Use macros, because they are compiled out in AK_OPTIMIZED configuration.

    /// Error codes.
    enum AkMonitorError
    {
        AK_ErrorFileNotFound                = 1008, ///< Error: File not found.
        AK_ErrorCannotOpenFile              = 1009, ///< Error: File could not be opened.
        AK_ErrorCannotStartStreamNoMemory   = 1010, ///< Error: Not enough memory to start stream.
        AK_ErrorIODeviceError               = 1011	///< Error: Error in Low-Level IO data transfer.
    };
    /// Send an error code through the monitoring system.
    void MonitorError( 
        AkMonitorError in_eErrorCode        // Error code.
        );
}

// Macros.
#ifndef AK_OPTIMIZED
    #define AK_MONITOR_ERROR( in_eErrorCode )\
        AK::MonitorError( in_eErrorCode )
#else
    #define AK_MONITOR_ERROR( in_eErrorCode )
#endif

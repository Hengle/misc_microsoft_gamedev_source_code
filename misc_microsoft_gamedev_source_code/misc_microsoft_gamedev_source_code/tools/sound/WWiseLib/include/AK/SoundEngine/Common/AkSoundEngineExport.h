//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// AkSoundEngineExport.h

/// \file 
/// Exportation definitions.

#ifndef _AK_SOUNDENGINE_EXPORT_H_
#define _AK_SOUNDENGINE_EXPORT_H_

#ifdef AKSOUNDENGINE_STATIC

    // Static libs.
    #define AKSOUNDENGINE_API	///< Sound Engine API exportation definition
    #define AKSTREAMMGR_API		///< Stream Manager API exportation definition
    #define AKMEMORYMGR_API		///< Memory Manager API exportation definition

#else

    // DLL exports.

    // Sound Engine
    #ifdef AKSOUNDENGINE_EXPORTS
        #define AKSOUNDENGINE_API __declspec(dllexport)///< Sound Engine API exportation definition
    #else
        #define AKSOUNDENGINE_API __declspec(dllimport)///< Sound Engine API exportation definition
    #endif // Export

    // Stream Manager
    #ifdef AKSTREAMMGR_EXPORTS
        #define AKSTREAMMGR_API __declspec(dllexport)///< Stream Manager API exportation definition
    #else
        #define AKSTREAMMGR_API __declspec(dllimport)///< Stream Manager API exportation definition
    #endif // Export

    // Memory Manager
    #ifdef AKMEMORYMGR_EXPORTS
        #define AKMEMORYMGR_API __declspec(dllexport)///< Memory Manager API exportation definition
    #else
        #define AKMEMORYMGR_API __declspec(dllimport)///< Memory Manager API exportation definition
    #endif // Export

#endif

#endif  //_AK_SOUNDENGINE_EXPORT_H_

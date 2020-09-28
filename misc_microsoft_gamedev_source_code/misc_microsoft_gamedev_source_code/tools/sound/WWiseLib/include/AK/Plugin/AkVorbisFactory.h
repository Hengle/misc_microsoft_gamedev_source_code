//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// AkVorbisFactory.h

/// \file
/// Codec plug-in unique ID and creation functions (hooks) necessary to register the Vorbis codec in the sound engine.

#ifndef _AK_VORBISFACTORY_H_
#define _AK_VORBISFACTORY_H_

#ifdef AKSOUNDENGINE_STATIC

#define AKVORBISDECODER_API

#else

// VorbisDecoder
#ifdef AKVORBISDECODER_EXPORTS
	#define AKVORBISDECODER_API __declspec(dllexport) ///< Vorbis decoder API exportation definition
#else
	#define AKVORBISDECODER_API __declspec(dllimport) ///< Vorbis decoder API exportation definition
#endif // Export

#endif // AKSOUNDENGINE_STATIC

const unsigned long AKCODECID_VORBIS = 4;	///< Unique ID of the Vorbis codec plug-in

/// Prototype of the Vorbis codec bank source creation function.
AKVORBISDECODER_API void* CreateVorbisBankPlugin( 
	void* in_pCtx			///< Bank source decoder context
	);

/// Prototype of the Vorbis codec file source creation function.
AKVORBISDECODER_API void* CreateVorbisFilePlugin( 
	void* in_pCtx 			///< File source decoder context
	);

#endif // _AK_VORBISFACTORY_H_

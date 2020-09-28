// Copyright (C) 2008 Audiokinetic Inc.
// AkAudioInputSourceFactory.h

/// \file 
///! Plug-in unique ID and creation functions (hooks) necessary to register the audio input plug-in to the sound engine.
/// <br><b>Wwise source name:</b>  AudioInput
/// <br><b>Library file:</b> AkAudioInputSource.lib

#ifndef _AK_AUDIOINPUTSOURCEFACTORY_H_
#define _AK_AUDIOINPUTSOURCEFACTORY_H_

#include <AK/SoundEngine/Common/IAkPlugin.h>

///
/// - This is the plug-in's unique ID (combined with the AKCOMPANYID_AUDIOKINETIC company ID)
/// - This ID must be the same as the plug-in ID in the plug-in's XML definition file, and is persisted in project files. 
/// \akwarning
/// Changing this ID will cause existing projects not to recognize the plug-in anymore.
/// \endakwarning
const AkUInt32 AKSOURCEID_AUDIOINPUT = 200;

/// Static creation function that returns an instance of the sound engine plug-in parameter node to be hooked by the sound engine plug-in manager.
AK::IAkPluginParam * CreateAudioInputSourceParams(
	AK::IAkPluginMemAlloc * in_pAllocator			///< Memory allocator interface
	);

/// Plugin mechanism. Source create function and register its address to the plug-in manager.
AK::IAkPlugin* CreateAudioInputSource(
	AK::IAkPluginMemAlloc * in_pAllocator			///< Memory allocator interface
	);

//////////////////////////////////////////////////////////////////////////
///
/// NOTE: The following functions interface the plug-ins global data to 
/// the application. Use these to set buffer data that will be read by
/// active source voices
///
//////////////////////////////////////////////////////////////////////////

/// Set the buffer data corresponding to an input channel of the Audio Input plug-in. This function should be called 
/// before any voices using the Audio Input plug-in are started. The format of the data is a mono (one-channel) floating 
/// point buffer. The sample rate to use is the native sample-rate of the Sound Engine.
AKRESULT SetAudioInputData(
	AkReal32 *const data,							///< Pointer to data
	AkUInt32 const dataSize,						///< Size of data
	int const input = 0								///< Input to set the data to 
	);

/// Get the number of available input channels. The current set size for the maximum number of inputs is 16. 
/// Refer to INPUT_MAX_INPUTS for more information. To change this value, set INPUT_MAX_INPUTS to the desired size. 
/// Also, you must change the field 'AudioInputInput' MIN and MAX size in the AudioInput.xml file to reflect this change
AkUInt32 GetMaxAudioInputs();

/*
Use the following code to register your plug-in

AK::SoundEngine::RegisterPlugin( AkPluginTypeSource, 
								 AKCOMPANYID_AUDIOKINETIC, 
								 AKSOURCEID_AUDIOINPUT,
								 CreateAudioInputSource,
								 CreateAudioInputSourceParam );
*/

#endif // _AK_AUDIOINPUTSOURCEFACTORY_H_

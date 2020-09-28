// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
// AkToneSourceFactory.h

/// \file
/// Plug-in unique ID and creation functions (hooks) necessary to register the Tone Generator plug-in to the sound engine.
/// <br><b>Wwise source name:</b>  Tone Generator
/// <br><b>Library file:</b> AkToneGen.lib

#ifndef _AK_PITCHROLLHEAVEFACTORY_H_
#define _AK_PITCHROLLHEAVEFACTORY_H_

#include <AK/SoundEngine/Common/IAkPlugin.h>

/// - This is the Plug-in unique ID (when combined with Company ID AKCOMPANYID_AUDIOKINETIC)
/// - This ID must be the same as the PluginID in the Plug-in's XML definition file, and is persisted in project files. 
/// \aknote Don't change the ID or existing projects will not recognize this plug-in anymore.
const AkUInt32 AKSOURCEID_MOTIONDBOXPITCHROLLHEAVE = 407;

/// Static creation function that returns an instance of the sound engine plug-in parameter node to be hooked by the sound engine plug-in manager.
AK::IAkPluginParam * CreateDBoxPitchRollHeaveParams(
	AK::IAkPluginMemAlloc * in_pAllocator			///< Memory allocator interface
	);

/// Static creation function that returns an instance of the sound engine plug-in to be hooked by the sound engine plug-in manager.
AK::IAkPlugin* CreateDBoxPitchRollHeave(
	AK::IAkPluginMemAlloc * in_pAllocator			///< Memory allocator interface
	);

#endif // _AK_PITCHROLLHEAVEFACTORY_H_



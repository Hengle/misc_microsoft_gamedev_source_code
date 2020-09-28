//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2008 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

/// \file
/// Plug-in unique ID and creation functions necessary to register the DBox plug-in in the sound engine.
/// <br><b>Wwise effect name:</b>  Wwise D-Box Plugin
/// <br><b>Library file:</b> AkDBox.lib

#pragma once

#include <AK/SoundEngine/Common/IAkPlugin.h>

/// - Plugin ID for the DBOX motion Bus (when combined with Company ID AKCOMPANYID_AUDIOKINETIC)
/// - This ID must be the same as the PluginID in the Plug-in's XML definition file, and is persisted in project files. 
/// \aknote Don't change the ID or existing projects will not recognize this plug-in anymore.
const unsigned long AKMOTIONDEVICEID_DBOX = 401;

/// - Plugin ID for the sources of DBOX motion FX (when combined with Company ID AKCOMPANYID_AUDIOKINETIC)
/// - This ID must be the same as the PluginID in the Plug-in's XML definition file, and is persisted in project files. 
/// \aknote Don't change the ID or existing projects will not recognize this plug-in anymore.
const unsigned long AKMOTIONSOURCEID_IMPORTFILE = 402;
extern AK::IAkPlugin* AkCreateDBox( AK::IAkPluginMemAlloc * in_pAllocator );

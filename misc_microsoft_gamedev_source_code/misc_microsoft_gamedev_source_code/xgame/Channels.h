//==============================================================================
// Channels.h
//
// Copyright (c) 2000-2008, Ensemble Studios
//==============================================================================

#pragma once

//==============================================================================
// Includes

#include <Channel.h>

//==============================================================================
namespace BChannelType
{
   enum eBangTypes 
   {  
      cCommandChannel = cNumberOfBuiltinTypes,
      cSyncChannel,
      cVoteChannel,
      cMessageChannel,
      cSimChannel,
      cSettingsChannel,
      cMaxChannels
   };
};

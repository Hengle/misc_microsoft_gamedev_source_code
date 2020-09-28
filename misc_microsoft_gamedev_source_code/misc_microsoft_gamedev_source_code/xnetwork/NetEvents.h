//==============================================================================
// NetEvents.h
//
// Copyright (c) Ensemble Studios, 2008
//==============================================================================

#pragma once

// shared network messages between the BSession, BXConnection and BXUDPSocket layers
enum
{
   cNetEventUDPSend = cEventClassFirstUser,
   cNetEventUDPRecv,
   cNetEventConnRecv,
   cNetEventVoiceRecv,

   //cNetEventVoiceInitSession,                // initialize a voice session, i.e. party or game
   cNetEventVoiceDeinitSession,

   cNetEventVoiceInitClient,                 // add a new client to the session & XHV
   cNetEventVoiceDeinitClient,

   //cNetEventVoiceSetSession,
   //cNetEventVoiceSetChannel,

   cNetEventVoiceBroadcast,
   cNetEventVoiceMuteClient,
   cNetEventVoiceTalkerList,
   cNetEventVoiceHeadsetPresent,

   cNetEventPingUpdate,
   cNetEventPingNotResponding,

   cNetEventSocketError,

   cNetEventFirstUser,

   cNetEventTotal
};

//==============================================================================
// MaxSendSize.h
//
// Copyright (c) 2001-2008, Ensemble Studios
//==============================================================================

#pragma once

enum
{
   //cMaxSendSize   = 4096,
   cMaxSendSize   = 1152,
   cMaxBufferSize = 1264,
   cMaxProxyBufferSize = cMaxBufferSize / 2,
};

enum
{
   cSendUnreliable            = 0x00000001,
   cSendUnencrypted           = 0x00000002,
   cSendImmediate             = 0x00000004,
   cSendProxy                 = 0x00000008,
};

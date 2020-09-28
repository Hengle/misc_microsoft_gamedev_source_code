//==============================================================================
// xmultiplayer.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

#ifndef _XMULTIPLAYER_H_
#define _XMULTIPLAYER_H_

// Local files
#include "multiplayer.h"

// XMultiplayerInfo
class XMultiplayerInfo
{
   public:
      XMultiplayerInfo()
      {
      }
};

// Functions
bool XMultiplayerCreate(XMultiplayerInfo* info);
void XMultiplayerRelease();

#endif

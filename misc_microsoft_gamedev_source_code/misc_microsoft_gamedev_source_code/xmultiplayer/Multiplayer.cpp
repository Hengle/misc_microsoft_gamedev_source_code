//==============================================================================
// BMultiplayer.cpp
//
// Copyright (c) 2003-2004, Ensemble Studios
//==============================================================================

// Includes
#include "multiplayercommon.h"
#include "Multiplayer.h"
#include "MultiplayerImpl.h"

//==============================================================================
// Defines 

//==============================================================================
bool BMultiplayer::createInstance(void)
{
   return MultiplayerImpl::createInstance();
}

//==============================================================================
BMultiplayer *BMultiplayer::getInstance(void)
{
   return MultiplayerImpl::getInstance();
} // BMultiplayer::~BMultiplayer


//==============================================================================
void BMultiplayer::destroyInstance(void)
{
   MultiplayerImpl::destroyInstance();
} // BMultiplayer::~BMultiplayer

//==============================================================================
// eof: BMultiplayer.cpp
//==============================================================================

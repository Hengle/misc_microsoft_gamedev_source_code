//==============================================================================
// soundInfoProvider.h
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#pragma once 
#include "soundmanager.h"
#include "configsgame.h"
#include "gamedirectories.h"
//==============================================================================
//-- This class simply provides information to BSoundManager, so that
//-- BSoundmanager doesn't need to know anything about xgame directly.
//==============================================================================

class BSoundInfoProvider : ISoundInfoProvider
{
public:
   BSoundInfoProvider () ;
   ~BSoundInfoProvider() {}

   virtual BCueIndex getSoundCueIndex(const char* pName) const;
   virtual long getSoundDirID() const {return cDirSound; }
   virtual long getDataDirID() const { return cDirData; }
};

extern BSoundInfoProvider gSoundInfoProvider;
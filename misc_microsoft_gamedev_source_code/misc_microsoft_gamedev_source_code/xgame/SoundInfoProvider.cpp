//==============================================================================
// soundInfoProvider.cpp
// Copyright (c) 2008 Ensemble Studios
//==============================================================================
#include "common.h"
#include "soundinfoprovider.h"
#include "database.h"

BSoundInfoProvider gSoundInfoProvider;
//==============================================================================
//==============================================================================
BSoundInfoProvider::BSoundInfoProvider () 
{ 
}

//==============================================================================
//==============================================================================
BCueIndex BSoundInfoProvider::getSoundCueIndex(const char* pName) const
{
#ifndef BUILD_FINAL
   return gDatabase.getSoundCueIndex(pName);
#else
   return cInvalidCueIndex;
#endif 
}
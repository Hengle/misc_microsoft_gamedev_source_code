//============================================================================
// eventsystem.h
//
// Copyright (c) 2003, Ensemble Studios
//============================================================================

// Includes
#include "common.h"
#include "eventsystem.h"

//==============================================================================
// BEventSystem::BEventSystem
//==============================================================================
BEventSystem::BEventSystem() :
   mEventTypes()
{
}

//==============================================================================
// BEventSystem::~BEventSystem
//==============================================================================
BEventSystem::~BEventSystem()
{
}

//==============================================================================
// BEventSystem::registerEventType
//==============================================================================
long BEventSystem::registerEventType(const char* name)
{
   long index=mEventTypes.add(name);
   return index;
}

//==============================================================================
// BEventSystem::lookupEventType
//==============================================================================
long BEventSystem::lookupEventType(const char* name)
{
   return mEventTypes.find(name);
}

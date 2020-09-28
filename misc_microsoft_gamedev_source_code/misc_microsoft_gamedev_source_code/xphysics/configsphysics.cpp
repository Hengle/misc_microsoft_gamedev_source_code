//==============================================================================
// configsgamerender.cpp
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "configsphysics.h"

//==============================================================================
// Defines
//==============================================================================
DEFINE_CONFIG(cConfigStartHavokDebugger);
DEFINE_CONFIG(cConfigNumPhysicsThreads);
DEFINE_CONFIG(cConfigPhysicsMainThread);

//==============================================================================
// registerPhysicsConfigs
//==============================================================================
static bool registerPhysicsConfigs(bool)
{
   DECLARE_CONFIG(cConfigStartHavokDebugger, "StartHavokDebugger", "Starts the havok visual debugger at physics startup time", 0, NULL);
   DECLARE_CONFIG(cConfigNumPhysicsThreads, "NumPhysicsThreads", "Sets the number of threads for physics to use", 0, NULL);
   DECLARE_CONFIG(cConfigPhysicsMainThread, "PhysicsMainThread", "Sets the thread index of the main physics update thread", 0, NULL);
   return true;
}

// This causes xcore to call registerGameRenderConfigs() after it initializes.
#pragma data_seg(".ENS$XIU") 
BXCoreInitFuncPtr gpRegisterPhysicsConfigs[] = { registerPhysicsConfigs };
#pragma data_seg() 

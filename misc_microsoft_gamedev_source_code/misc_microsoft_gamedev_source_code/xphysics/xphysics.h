//==============================================================================
// xphysics.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#pragma once

// Dependant files
//#include "xcore.h"

// xphysics files
#include "physics.h"

// XPhysicsInfo
class XPhysicsInfo
{
   public:
      XPhysicsInfo() :
         mDirPhysics(-1),
         mpRenderInterface(NULL)
      {
      }

      long mDirPhysics;
      BPhysicsRenderInterface* mpRenderInterface;
};

// XPhysics create and release functions
bool XPhysicsCreate(XPhysicsInfo* info);
void XPhysicsRelease();

// XPhysics memory manager init/deinit
void XPhysicsMemoryInit();
void XPhysicsMemoryDeinit();

// Thread memory and initialization
const uint cMaxPhysicsThreads = 6;
extern char* gStackBuffers[cMaxPhysicsThreads];
inline bool isPhysicsThreadInited() { return (gStackBuffers[gEventDispatcher.getThreadIndex()] != NULL); }
bool XPhysicsThreadInit();

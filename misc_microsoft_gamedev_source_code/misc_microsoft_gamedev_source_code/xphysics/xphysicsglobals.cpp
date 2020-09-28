//==============================================================================
// xphysicsglobals.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

// Includes
#include "common.h"
#include "physics.h"
//#include "physicsinternal.h"

// Define all the global system variables in the opposite order we want them 
// destroyed since there are dependecies between them. The last one created 
// will be destroyed first.

BPhysics*                        gPhysics;



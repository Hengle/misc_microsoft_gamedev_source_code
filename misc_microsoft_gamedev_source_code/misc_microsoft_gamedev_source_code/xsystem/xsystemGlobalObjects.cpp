//==============================================================================
// xsystemGlobalObjects.cpp
//
// Copyright (c) 2005-2007 Ensemble Studios
//==============================================================================
#include "xsystem.h"
#include "config.h"
#include "systeminfo.h"
#include "perf.h"
#ifndef XBOX
   #include "debughelp.h"
#endif   

//-- This pragma junk makes sure these globals get loaded before anything else.
#pragma warning(disable: 4073)
#pragma init_seg(user)
#pragma warning(default: 4073)

//============================================================================
//  GLOBALS
//============================================================================
// These are initialized in the order they appear and de-initialized in the reverse order.
// This ensures that, for example, the memory manager is created first and shut down last ensuring 
// that allocations done by the other globals here are accounted for properly.

BSystemInfo gSystemInfo;
BConfig gConfig;
BFileManager gFileManager;
BLogManager gLogManager;
BRandomManager gRandomManager;
BPerf gPerf;



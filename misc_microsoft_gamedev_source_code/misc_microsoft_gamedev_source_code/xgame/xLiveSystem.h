//==============================================================================
// xLiveSystem.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

//Class that lets the game have access to the singleton LiveSystem

#ifndef _XLIVESYSTEM_H_
#define _XLIVESYSTEM_H_

// Dependant files
#include "xsystem.h"

// Local files
#include "liveSystem.h"

// XInputSystemInfo
class XLiveSystemInfo
{
public:
	XLiveSystemInfo() 
	  {
	  }
};

// Functions
bool XLiveSystemCreate(XLiveSystemInfo* info);
void XLiveSystemRelease();

#endif

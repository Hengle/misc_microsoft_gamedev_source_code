//==============================================================================
// xLiveSystem.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "Common.h"
#include "xLiveSystem.h"



// Globals
static long             gXLiveRefCount=0;
static XLiveSystemInfo  gXLiveSystemInfo;

//==============================================================================
// XLiveSystemCreate
//==============================================================================
bool XLiveSystemCreate(XLiveSystemInfo* info)
{
	if(gXLiveRefCount==0)
	{
		if(info)
			gXLiveSystemInfo=*info;

		if(!BLiveSystem::createInstance())
			return false;

		BLiveSystem::getInstance()->startup();
	}

	gXLiveRefCount++;

	return true;
}

//==============================================================================
// XLiveSystemRelease
//==============================================================================
void XLiveSystemRelease()
{
	if(gXLiveRefCount==0)
	{
		BASSERT(0);
		return;
	}

	gXLiveRefCount--;

	if(gXLiveRefCount==0)
	{
		if(BLiveSystem::getInstance())
			BLiveSystem::getInstance()->shutdown();
		BLiveSystem::destroyInstance();
		SleepEx(1, true); 
	}
}

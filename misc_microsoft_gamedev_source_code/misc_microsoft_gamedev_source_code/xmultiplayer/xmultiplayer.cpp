//==============================================================================
// xmultiplayer.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "multiplayercommon.h"
#include "xmultiplayer.h"
#include "connectivity.h"

// Globals
static long             gXMultiplayerRefCount=0;
static XMultiplayerInfo gXMultiplayerInfo;

//==============================================================================
// XMultiplayerCreate
//==============================================================================
bool XMultiplayerCreate(XMultiplayerInfo* info)
{
   if(gXMultiplayerRefCount==0)
   {
      if(info)
         gXMultiplayerInfo=*info;

      if(!BMultiplayer::createInstance())
         return false;
   }

   gXMultiplayerRefCount++;

   return true;
}

//==============================================================================
// XMultiplayerRelease
//==============================================================================
void XMultiplayerRelease()
{
   if(gXMultiplayerRefCount==0)
   {
      BASSERT(0);
      return;
   }

   gXMultiplayerRefCount--;

   if(gXMultiplayerRefCount==0)
   {
      if(BMultiplayer::getInstance())
         BMultiplayer::getInstance()->shutdown();
      BMultiplayer::destroyInstance();
      BConnectivity::destroyInstance();
      SleepEx(1, true); // If you don't know why, I'm not gonna tell you.
   }
}

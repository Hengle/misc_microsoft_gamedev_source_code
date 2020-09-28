//==============================================================================
// xsound.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

// Includes
#include "xsound.h"
#include "soundmanager.h"

// Globals
static long       gXSoundRefCount=0;

//==============================================================================
// XSoundCreate
//==============================================================================
bool XSoundCreate()
{
   if(gXSoundRefCount==0)
   {  
      if (!gSoundManager.setup())
         return false;

      if (!gSoundManager.initSoundEngine())
         return false;
   }

   gXSoundRefCount++;

   return true;
}

//==============================================================================
// XSoundRelease
//==============================================================================
void XSoundRelease()
{
   if(gXSoundRefCount==0)
   {
      BASSERT(0);
      return;
   }

   gXSoundRefCount--;

   if(gXSoundRefCount==0)
   {
      gSoundManager.shutdown();
   }
}

//==============================================================================
// XSoundSetInterface
//==============================================================================
void XSoundSetInterface(ISoundInfoProvider* pProvider)
{
   gSoundManager.setSoundInfoProvider(pProvider);
}

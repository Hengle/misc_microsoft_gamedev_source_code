//============================================================================
// xgranny.cpp
//
// Copyright (c) 2006 Ensemble Studios
//============================================================================

// Includes
#include "common.h"
#include "xgranny.h"

// Globals
static long gGrannyRefCount=0;
static XGrannyInfo gXGrannyInfo;

//==============================================================================
// XGrannyCreate
//==============================================================================
bool XGrannyCreate(XGrannyInfo* info)
{
   if(gGrannyRefCount==0)
   {
      if(info)
      {
         gXGrannyInfo.mDirName=info->mDirName;
      }

      if(!gGrannyManager.init(gXGrannyInfo.mDirName))
         return false;
   }

   gGrannyRefCount++;
   return true;
}

//==============================================================================
// XGrannyRelease
//==============================================================================
void XGrannyRelease()
{
   if(gGrannyRefCount==0)
   {
      BASSERT(0);
      return;
   }

   gGrannyRefCount--;

   if(gGrannyRefCount==0)
   {
      gGrannyManager.deinit();
   }
}

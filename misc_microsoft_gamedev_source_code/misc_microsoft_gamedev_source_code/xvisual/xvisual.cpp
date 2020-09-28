//============================================================================
// xvisual.cpp
//
// Copyright (c) 2006 Ensemble Studios
//============================================================================

// Includes
#include "common.h"
#include "xvisual.h"
#include "lightEffectManager.h"

// xparticles
#include "particleworkdirsetup.h"
#include "particlesystemmanager.h"
#include "particlegateway.h"

// xgamerender
#include "configsgamerender.h"
#include "flashmanager.h"
#include "flashgateway.h"
#include "graphmanager.h"

// Globals
static long gVisualRefCount=0;
static XVisualInfo gXVisualInfo;

//==============================================================================
// XVisualCreate
//==============================================================================
bool XVisualCreate(XVisualInfo* info)
{
   if(gVisualRefCount==0)
   {
      if(info)
      {
         gXVisualInfo.mDirID=info->mDirID;
         gXVisualInfo.mDirName=info->mDirName;
      }

      if(!gVisualManager.init(gXVisualInfo.mDirID, gXVisualInfo.mDirName))
         return false;
         
      gLightEffectManager.init(gXVisualInfo.mDirID);
      
      {
         initializeParticleWorkingDirectoryIDs();

         int dataBlockSize = 1024;
         int instanceBlockSize = 8192;
         int particleBlockSize = 1000000;
         gPSManager.init(dataBlockSize, instanceBlockSize, particleBlockSize);   
         gParticleGateway.init();
         TRACEMEM
      }      

      if (!gFlashManager.init())
         return false;

      gFlashGateway.init();

      #ifndef BUILD_FINAL
      gFlashGateway.setEnableForceSWFLoading(gConfig.isDefined(cConfigFlashForceSWFLoading));
      #endif

      if (!gGraphManager.init())
         return false;
   }

   gVisualRefCount++;
   return true;
}

//==============================================================================
// XVisualRelease
//==============================================================================
void XVisualRelease()
{
   if(gVisualRefCount==0)
   {
      BASSERT(0);
      return;
   }

   gVisualRefCount--;

   if(gVisualRefCount==0)
   {
      gParticleGateway.deinit();

      gPSManager.kill();
      
      gLightEffectManager.deinit();
      
      gVisualManager.deinit();

      gFlashManager.deinit();

      gFlashGateway.deinit();

      gGraphManager.deinit();
   }
}

//==============================================================================
// xgamerender.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

#pragma once

#include "xsystem.h"

#include "xrender.h"

// XGameRenderInfo
class XGameRenderInfo
{
   public:
      XGameRenderInfo() :
         mDirArt(cDirBase),
         mDirEffects(cDirBase),
         mDirFonts(cDirBase),
         mDirStartup(cDirBase),
         mDirData(cDirBase)
      {
      }
      
      long mDirArt;
      long mDirEffects;
      long mDirFonts;
      long mDirStartup;
      long mDirData;
};

// Functions
bool XGameRenderCreate(XGameRenderInfo* info);
void XGameRenderRelease();

// Call this when there's a read error loading archives
void DiskReadFailAlert(void);

void ShowDirtyDiskError(void); //called to directly bring up the dirty disk blade (Ie, you shouldn't call this. DiskReadFailAlert calls this)
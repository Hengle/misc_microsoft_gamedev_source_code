//==============================================================================
// xvisual.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

// xvisual files
#include "visual.h"
#include "visualinstance.h"
#include "visualmanager.h"

// xgranny files
#include "xgranny.h"

// XVisualInfo
class XVisualInfo
{
   public:
      XVisualInfo() :
         mDirID(-1),
         mDirName()
      {
      }

      long     mDirID;
      BString  mDirName;
};

// XVisual create and release functions
bool XVisualCreate(XVisualInfo* info);
void XVisualRelease();

//==============================================================================
// xgranny.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================

#ifndef _XGRANNY_H_
#define _XGRANNY_H_

// xgranny files
#include "grannymanager.h"
#include "grannyanimation.h"
#include "grannyinstance.h"
#include "grannyinstancerenderer.h"
#include "grannymodel.h"

// XGrannyInfo
class XGrannyInfo
{
   public:
      XGrannyInfo() :
         mDirName()
      {
      }

      BSimString  mDirName;
};

// XGranny create and release functions
bool XGrannyCreate(XGrannyInfo* info);
void XGrannyRelease();

#endif

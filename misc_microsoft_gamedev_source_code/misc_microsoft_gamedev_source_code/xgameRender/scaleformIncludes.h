// File: scaleformIncludes.h
// All scaleform includes should go here!
#pragma once

#ifndef GFC_FORCE_RELEASE_LIBS
   #ifndef BUILD_DEBUG
      #define GFC_FORCE_RELEASE_LIBS
   #endif
#endif

#include "GTypes.h"
#include "GRefCount.h"
#include "GImage.h"
#include "GTLTypes.h"
#include "GFxLog.h"
#include "GFxPlayer.h"
#include "GFile.h"
#include "GFxLoader.h"
#include "GMemory.h"
#include "GAllocator.h"
#include "GStandardAllocator.h"
#include "GFxImageResource.h"
#include "GRenderer.h"
#include "GRendererCommonImpl.h"
#include "GRendererD3D9Common.h"
#include "GRendererXbox360.h"
#include "GFxFontLib.h"
#include "flashassetcategory.h"
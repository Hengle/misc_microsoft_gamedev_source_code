//==============================================================================
// configsgamerender.cpp
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================

// Includes
#include "xgamerender.h"
#include "configsgamerender.h"
#include "renderthread.h"


//==============================================================================
//==============================================================================
#ifndef BUILD_FINAL
void minRenderTimeCallback(long configEnum, bool beingDefined)
{
   configEnum;

   long time = 0;
   if(beingDefined)
      gConfig.get(cConfigMinRenderTime, &time);

   gRenderThread.setMinFrameTime((DWORD)time);
   gConsoleOutput.output(cMsgConsole, "Min render time set to: %d ms", time);
}
#endif


//==============================================================================
// Defines
//==============================================================================
DEFINE_CONFIG(cConfigForce720p);
DEFINE_CONFIG(cConfigVSync);
DEFINE_CONFIG(cConfigDisplayAspectRatio);
DEFINE_CONFIG(cConfigConsoleRenderEnable);
DEFINE_CONFIG(cConfigConsoleRenderMode);
DEFINE_CONFIG(cConfigNumAATiles);
DEFINE_CONFIG(cConfigPresentImmediateThreshold);
DEFINE_CONFIG(cConfigPresentImmediateThresholdSD);

#ifndef BUILD_FINAL
DEFINE_CONFIG(cConfigDisableShadowRendering);
DEFINE_CONFIG(cConfigJPEGScreenshots);
DEFINE_CONFIG(cConfigHigherQualityShadows);
DEFINE_CONFIG(cConfigMinRenderTime);
#endif

DEFINE_CONFIG(cConfigFlashEnableDynamicTesselation);
DEFINE_CONFIG(cConfigFlashEnableBatching);
DEFINE_CONFIG(cConfigFlashForceSWFLoading);
DEFINE_CONFIG(cConfigFlashFontsFile);
DEFINE_CONFIG(cConfigFlashCustomWordWrappingMode);

//==============================================================================
// registerGameRenderConfigs
//==============================================================================
static bool registerGameRenderConfigs(bool)
{
   DECLARE_CONFIG(cConfigForce720p, "Force720p", "Force 1280x720 resolution", 0, NULL);
   DECLARE_CONFIG(cConfigVSync, "VSync", "VSync <numFrames> where numFrames can be 1, 2, or 3.", 0, NULL);
   DECLARE_CONFIG(cConfigDisplayAspectRatio, "displayAspectRatio", "Specifies the aspect ratio of the physica display (such as 4:3, 16:9, 16:10)", 0, NULL);
   DECLARE_CONFIG(cConfigConsoleRenderEnable, "consoleRenderEnable", "Debug console on/off", 0, NULL);
   DECLARE_CONFIG(cConfigConsoleRenderMode, "consoleRenderMode", "RenderMode of debug console", 0, NULL);
   DECLARE_CONFIG(cConfigNumAATiles, "NumAATiles", "", 0, NULL);
   DECLARE_CONFIG(cConfigPresentImmediateThreshold, "PresentImmediateThreshold", "", 0, NULL);
   DECLARE_CONFIG(cConfigPresentImmediateThresholdSD, "PresentImmediateThresholdSD", "", 0, NULL);
   
#ifndef BUILD_FINAL
   DECLARE_CONFIG(cConfigDisableShadowRendering, "disableShadowRendering", "Disable shadow model rendering", 0, NULL);
   DECLARE_CONFIG(cConfigJPEGScreenshots, "JPEGScreenshots", "Save JPEG screenshots by default, instead of TGA", 0, NULL);
   DECLARE_CONFIG(cConfigHigherQualityShadows, "HigherQualityShadows", "Enable higher quality shadows", 0, NULL);
   DECLARE_CONFIG(cConfigMinRenderTime, "MinRenderTime", "Sets a minimum time for a render frame in ms", 0, minRenderTimeCallback);
#endif

   DECLARE_CONFIG(cConfigFlashEnableDynamicTesselation, "FlashEnableDynamicTesselation", "Toggles Dynamic Tesselation of Shapes in Flash", 0, NULL);
   DECLARE_CONFIG(cConfigFlashEnableBatching, "FlashEnableBatching", "Toggles Draw Call Batching in Flash", 0, NULL);
   DECLARE_CONFIG(cConfigFlashForceSWFLoading, "FlashForceSWFLoading", "Forces Flash to load .SWF files instead of .GFX files", 0, NULL);
   DECLARE_CONFIG(cConfigFlashFontsFile, "FlashFontsFile", "Changes the default flash fonts file", 0, NULL);
   DECLARE_CONFIG(cConfigFlashCustomWordWrappingMode, "WordWrappingMode", "Changes Wordwrap mode, Valid modes are default, mgs, scaleform", 0, NULL);
      
   return true;
}

// This causes xcore to call registerGameRenderConfigs() after it initializes.
#pragma data_seg(".ENS$XIU") 
BXCoreInitFuncPtr gpRegisterGameRenderConfigs[] = { registerGameRenderConfigs };
#pragma data_seg() 

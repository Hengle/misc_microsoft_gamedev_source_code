//==============================================================================
// configsgamerender.h
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================
#pragma once

#include "config.h"

//==============================================================================
// Config constants
//==============================================================================
extern const long& cConfigForce720p;
extern const long& cConfigVSync;
extern const long& cConfigDisplayAspectRatio;
extern const long& cConfigConsoleRenderEnable;
extern const long& cConfigConsoleRenderMode;
extern const long& cConfigNumAATiles;
extern const long& cConfigPresentImmediateThreshold;
extern const long& cConfigPresentImmediateThresholdSD;

#ifndef BUILD_FINAL
extern const long& cConfigDisableShadowRendering;
extern const long& cConfigJPEGScreenshots;
extern const long& cConfigHigherQualityShadows;
extern const long& cConfigMinRenderTime;
#endif

extern const long& cConfigFlashEnableDynamicTesselation;
extern const long& cConfigFlashEnableBatching;
extern const long& cConfigFlashForceSWFLoading;
extern const long& cConfigFlashFontsFile;
extern const long& cConfigFlashCustomWordWrappingMode;


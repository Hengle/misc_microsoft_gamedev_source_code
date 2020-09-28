//==============================================================================
//
// File: meshEffectTextures.h
//
// Copyright (c) 2008 Ensemble Studios
//
//==============================================================================
#pragma once
#include "utils\heapSingleton.h"
#include "d3dTextureManager.h"

class BMeshEffectTextures : public BHeapSingleton<BMeshEffectTextures>
{
public:
   BMeshEffectTextures();
   ~BMeshEffectTextures();
   
   void init();
   void deinit();
   
   // If you modify this enum, be sure to also update gpTextureTypeFilenames!
   enum BTextureType
   {
      cTTSelection,
      cTTIce,

      cTTMax
   };
   
   BManagedTextureHandle get(BTextureType type);

private:
   BManagedTextureHandle mTextures[cTTMax];
};


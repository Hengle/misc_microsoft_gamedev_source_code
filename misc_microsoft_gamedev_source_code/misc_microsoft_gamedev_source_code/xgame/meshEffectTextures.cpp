//==============================================================================
//
// File: meshEffectTextures.cpp
//
// Copyright (c) 2008 Ensemble Studios
//
//==============================================================================
#include "common.h"
#include "meshEffectTextures.h"

// Note: All *.ddx files located under system\meshEffect will be located in the root archive.
static const char* gpTextureTypeFilenames[] = 
{
   "system\\meshEffect\\selection.ddx",
   "system\\meshEffect\\ice.ddx"
};
const uint cNumTextureTypeFilenames = sizeof(gpTextureTypeFilenames)/sizeof(gpTextureTypeFilenames[0]);

BMeshEffectTextures::BMeshEffectTextures()
{
   BCOMPILETIMEASSERT(cNumTextureTypeFilenames == cTTMax);
   
   for (uint i = 0; i < cTTMax; i++)
      mTextures[i] = cInvalidManagedTextureHandle;
}

BMeshEffectTextures::~BMeshEffectTextures()
{
}

void BMeshEffectTextures::init()
{
   for (uint i = 0; i < cTTMax; i++)
   {
      if (mTextures[i] == cInvalidManagedTextureHandle)
      {
         mTextures[i] = gD3DTextureManager.getOrCreateHandle(
            gpTextureTypeFilenames[i], static_cast<BFileOpenFlags>(BFILE_OPEN_NORMAL | BFILE_OPEN_DISCARD_ON_CLOSE), 
            BD3DTextureManager::cSystem, true, cDefaultTextureWhite, true, false, "BMeshEffectTextures");
      }
   }
}

void BMeshEffectTextures::deinit()
{
   for (uint i = 0; i < cTTMax; i++)
   {
      if (mTextures[i] != cInvalidManagedTextureHandle)
      {
         gD3DTextureManager.releaseManagedTextureByHandle(mTextures[i]);
         mTextures[i] = cInvalidManagedTextureHandle;
      }
   }
}

BManagedTextureHandle BMeshEffectTextures::get(BMeshEffectTextures::BTextureType type)
{
   BDEBUG_ASSERT(type < cTTMax);
   return mTextures[type];
}

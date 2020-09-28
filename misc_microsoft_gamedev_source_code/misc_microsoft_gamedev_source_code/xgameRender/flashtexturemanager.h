//============================================================================
// flashtexturemanager.h
// Copyright 2007 (c) Ensemble Studios
//============================================================================

#pragma once 
#include "scaleformIncludes.h"
#include "D3DTextureLoader.h"
#include "D3DTextureManager.h"

class BD3DTexture;

typedef int BFlashTextureHandle;
const BFlashTextureHandle cInvalidFlashTextureHandle = -1;

//============================================================================
//============================================================================
class BFlashTextureManager
{
   public: 
      BFlashTextureManager();
     ~BFlashTextureManager();

      bool init();
      void deInit();
      void clear();

      bool isInitialized() const { return mbInitialized; }
      
      BD3DTextureManager::BManagedTexture* getOrCreate(const BCHAR_T* pFilename, DWORD textureCategoryMask = BD3DTextureManager::cScaleformCommon);
      GImageInfoBase* getOrCreateGamerPic(const BCHAR_T* gamerKey, GRendererXbox360* pRenderer);

      void               unloadPreGameUITextures();
      void               loadPreGameUITextures();
              
   private:

      bool preloadUITextures();
      
      typedef BHashMap<BString, BFlashTextureHandle> BFileStringHashTable;
      bool mbInitialized:1;   
};

extern BFlashTextureManager gFlashTextureManager;
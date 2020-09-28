//============================================================================
// flashtexturemanager.h
// Copyright 2007 (c) Ensemble Studios
//============================================================================

#include "xgameRender.h"
#include "renderThread.h"
#include "flashtexturemanager.h"
#include "D3DTexture.h"
#include "D3DTextureLoader.h"
#include "renderDraw.h"
#include "GamerPicManager.h"
#include "flashtexture.h"
#include "../xgame/archiveManager.h"

BFlashTextureManager gFlashTextureManager;

//============================================================================
//============================================================================
BFlashTextureManager::BFlashTextureManager():
mbInitialized(false)
{

}

//============================================================================
//============================================================================
BFlashTextureManager::~BFlashTextureManager()
{
}

//============================================================================
//============================================================================
bool BFlashTextureManager::init()
{
   ASSERT_RENDER_THREAD

   if (mbInitialized)
      return true;

   mbInitialized = true;

   if (gArchiveManager.getArchivesEnabled())
   {      
      gArchiveManager.beginInGameUILoad();
      if (!preloadUITextures())
         return false;
   }      
   return true;
}

//============================================================================
//============================================================================
void BFlashTextureManager::deInit()
{
   ASSERT_RENDER_THREAD

   if (!mbInitialized)
      return;

   clear();

   mbInitialized = false;
}

//============================================================================
//============================================================================
bool BFlashTextureManager::preloadUITextures()
{
   SCOPEDSAMPLE(BFlashTextureManager_preloadUITextures)

   trace("FlashTextureManager::Texture preload begin");

   BFile texFileList;
   if (!texFileList.open(cDirProduction, "FlashTexturePreload.txt", BFILE_OPEN_ENABLE_BUFFERING | BFILE_OPEN_DISCARD_ON_CLOSE))
   {
      gConsoleOutput.error("BFlashTextureManager::preloadUITextures: Unable to open FlashTexturePreload.txt\n");
      return false;
   }

   BStream* pStream = texFileList.getStream();

   for ( ; ; )
   {
      BString str;
      if (!pStream->readLine(str))
         break;

      str.standardizePath();
      
      int i = str.findLeft("\\art\\");
      if (i != -1)
         str.crop(i+1, str.length() - 1);

      if (str.isEmpty())
         continue;

      gConsoleOutput.resource("Preloading Flash UI Texture file: %s\n", str.getPtr());
      if (!getOrCreate(str, BD3DTextureManager::cScaleformInGame))
      {
         gConsoleOutput.resource("ERROR Flash Texture Preload failed: %s\n", str.getPtr());
      }
   }      

   trace("Flash Texture Preload end");

   return true;
}

//============================================================================
//============================================================================
void BFlashTextureManager::clear()
{
   ASSERT_RENDER_THREAD      
}

//============================================================================
// BTextureManager::getOrCreate
//============================================================================
BD3DTextureManager::BManagedTexture* BFlashTextureManager::getOrCreate(const BCHAR_T* pFilename, DWORD textureCategoryMask)
{
   SCOPEDSAMPLE(BFlashTextureManager_getOrCreate)
   ASSERT_RENDER_THREAD
   BDEBUG_ASSERT(isInitialized());

   if (pFilename == NULL)
      return NULL;

   //-- we don't have it so we have to load and create it
   BString filename;
   filename.set(pFilename);
   filename.removeExtension();
   filename.append(".ddx");

   int i = filename.findLeft("art\\");
   if (i != -1)
      filename.crop(i+4, filename.length() - 1);
   
   filename.standardizePath();     

   // fix up relatives paths if needed.
   if ( filename.findLeft("..\\") != -1)      // scope this so we don't have to do this for every file.
   {
      BDynamicArray<BString> pathParts;

      BString token;
      long strLen = filename.length();
      long loc = token.copyTok(filename, strLen, -1, B("\\"));
      while (loc != -1)
      {
         if (token == "..")
         {
            // remove the previous path.
            if (pathParts.getNumber() > 0)
               pathParts.removeIndex(pathParts.getNumber()-1);
            else
               pathParts.add(token);   // if we can't go up from here, then leave the .. and let the filemanager handle it.
         }
         else if (token != ".")        // just omit this part
         {
            pathParts.add(token);
         }

         loc = token.copyTok(filename, strLen, loc+1, B("\\"));
      }

      filename.set("");
      // now we reconstitute the path
      for (int i=0; i<pathParts.getNumber(); i++)
      {
         if (i!=0)
            filename.append("\\");

         filename.append(pathParts[i]);
      }
   }

   BD3DTextureManager::BManagedTexture* pManagedTexture = NULL;
   pManagedTexture = gD3DTextureManager.find(filename);
   if (pManagedTexture)
   {
      if (pManagedTexture->getStatus() != BD3DTextureManager::BManagedTexture::cStatusLoaded)
      {
         if (!pManagedTexture->load())
         {
            trace("WARNING: FlashTextureManager::getOrCreate(): Found uninitialized Texture in Cache but Load() failed: %s", filename.getPtr());
            return NULL;
         }
      }

      // add ref since we are reusing it
      pManagedTexture->addRef();

      return pManagedTexture;
   }
   
   pManagedTexture = gD3DTextureManager.getOrCreate(filename, BFILE_OPEN_NORMAL, textureCategoryMask, false, cDefaultTextureWhite, true, false, "Scaleform");
   if (!pManagedTexture)
   {
      trace("Texture Load Error: %s", filename.getPtr());
      return NULL;
   }

   if (!pManagedTexture->load())
   {
      trace("WARNING: FlashTextureManager::getOrCreate(): Initial Load Failed: %s", filename.getPtr());
      return NULL;
   }

   return pManagedTexture;   
}

//============================================================================
// BTextureManager::getOrCreateGamerPic
//============================================================================
GImageInfoBase* BFlashTextureManager::getOrCreateGamerPic(const BCHAR_T* gamerKey, GRendererXbox360* pRenderer)
{
   SCOPEDSAMPLE(BFlashTextureManager_getOrCreateGamerPic)
   ASSERT_RENDER_THREAD
   BASSERT(isInitialized());

   if (!pRenderer)
      return NULL;
     
   IDirect3DTexture9* pD3DTexture = NULL;
   bool bIsAReleaseableTexture = true;

   pD3DTexture = gGamerPicManager.createTexture(gamerKey);
   if (pD3DTexture == NULL)
   {
      gConsoleOutput.error("BFlashTextureManager::getOrCreateGamerPic: Cannot find Gamer Pic for : %s\n", gamerKey);
      trace("BFlashTextureManager::getOrCreateGamerPic: Cannot find Gamer Pic for : %s\n", gamerKey);   

      BD3DTextureManager::BManagedTexture* pManagedTexture = getOrCreate("ui\\flash\\shared\\textures\\misc\\default_gamerpic", BD3DTextureManager::cScaleformInGame);
      if (!pManagedTexture)
      {
         BFAIL("Default Gamerpic Texture could not be loaded");
      }

      pD3DTexture = pManagedTexture->getD3DTexture().getTexture();
      if (!pD3DTexture)
      {
         BFAIL("Default Gamerpic Texture: Invalid D3D Pointer");
      }

      bIsAReleaseableTexture = false;
   }

   // let's get this managed.
   BD3DTexture newTexture;
   newTexture.setTexture(pD3DTexture, BD3DTexture::cD3D);   
   
   // rg [1/26/08] - WARNING - THIS TEXTURE POINTER MAY NOT HAVE BEEN CREATED BY D3D. DO NOT Release() IT! Instead, use BD3DTexture::release() or BD3DTexture::releaseWildTexture().
   IDirect3DTexture9* pTex = newTexture.getTexture();
   BASSERT(pTex->GetIdentifier());

   GPtr<BFlashTexture> pNewTexture = *(reinterpret_cast<BFlashTexture*>(pRenderer->CreateTexture()));
   if (!pNewTexture)
   {
      if (bIsAReleaseableTexture)
         BD3DTexture::releaseWildTexture(pD3DTexture);

      newTexture.clear();

      return NULL;
   }

   UInt twidth = 0;
   UINT theight = 0;

   D3DSURFACE_DESC d3dDesc;
   pTex->GetLevelDesc(0, &d3dDesc);
   twidth=d3dDesc.Width;
   theight=d3dDesc.Height;
         
   if (!pNewTexture->InitTexture(pTex, twidth, theight))
   {
      if (bIsAReleaseableTexture)
         BD3DTexture::releaseWildTexture(pD3DTexture);

      newTexture.clear();

      pNewTexture = NULL;
      return NULL;
   }   

   //-- we override the release state flag because the gamerpics are a special case where 
   //-- sometimes the D3D texture pointer points to a managed texture.  If it is a 
   //-- managed texture we don't want to actually free up the texture as we do not own
   //-- it.
   pNewTexture->SetReleaseable(bIsAReleaseableTexture);

   return new GImageInfo(pNewTexture, twidth, theight);   
}

//============================================================================
// BTextureManager::unloadPreGameUITextures
//============================================================================
void BFlashTextureManager::unloadPreGameUITextures()
{
   ASSERT_RENDER_THREAD

   DWORD memberShip = BD3DTextureManager::cScaleformPreGame;      
   gD3DTextureManager.unloadAll(memberShip);
}

//============================================================================
// BTextureManager::unloadPreGameUITextures
//============================================================================
void BFlashTextureManager::loadPreGameUITextures()
{
   ASSERT_RENDER_THREAD

   DWORD memberShip = BD3DTextureManager::cScaleformPreGame;      
   gD3DTextureManager.loadAll(memberShip);   
}
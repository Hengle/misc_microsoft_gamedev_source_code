//============================================================================
// flashtexture.h
// Copyright 2007 (c) Ensemble Studios
//============================================================================

#pragma once 
#include "scaleformIncludes.h"
#include "flashrender.h"
#include "D3DTextureManager.h"

class GFxImageCreator;
class GFxImageCreateInfo;
class GFxImageLoader;


//============================================================================
//============================================================================
class BFlashTextureLoader : public GFxImageLoader
{
public: 
   BFlashTextureLoader(GRendererXbox360* renderer, uint category){ mpRenderer=renderer; mCategory = category;};

   virtual ~BFlashTextureLoader() {};
   virtual GImageInfoBase* LoadImage(const char *purl);

protected:
   GRendererXbox360* mpRenderer;
   uint              mCategory;
};



//============================================================================
//============================================================================
class BFlashTextureCreator : public GFxImageCreator
{
   public: 
      BFlashTextureCreator(uint category, bool keepImageBindData = 0){mCategory = category;};      

      virtual ~BFlashTextureCreator() {};
      virtual GImageInfoBase* CreateImage(const GFxImageCreateInfo &info);      
   protected:
      uint mCategory;
};


//============================================================================
//============================================================================
class BFlashTexture : public GTextureD3D9
{
   public:

      BFlashTexture(BFlashRender* pRenderer);
     ~BFlashTexture();

      // Obtains the renderer that create TextureInfo 
      virtual GRenderer*  GetRenderer() const;        
      virtual bool        IsDataValid() const;

      // Texture loading logic.
      virtual bool        InitTexture(BD3DTextureManager::BManagedTexture* pTexture, SInt width, SInt height);
      virtual bool        InitTexture(IDirect3DTexture9* pTexture, SInt width, SInt height);
      virtual bool        InitTexture(GImageBase* pImage, int targetWidth, int targetHeight);
      virtual bool        InitTextureFromFile(const char* pFilename, int targetWidth, int targetHeight); 

      virtual bool        InitTexture(int width, int height, GImage::ImageFormat format, int mipmaps, int targetWidth, int targetHeight);
      virtual void        Update(int level, int n, const UpdateRect *rects, const GImageBase *pim);

      // Remove texture from renderer, notifies of renderer destruction
      void                RemoveFromRenderer();
      
      void                SetReleaseable(bool releasable) { mReleasableTexture = releasable; }

      IDirect3DTexture9*  GetD3DTexture() const;      

      int getWidth() const { return mWidth; }
      int getHeight() const { return mHeight; }
      float getOneOverWidth() const { return mOneOverWidth; }
      float getOneOverHeight() const { return mOneOverHeight; }

   private:
      // Renderer 
      BFlashRender*   mpRenderer;
      int             mWidth;
      int             mHeight;
      float           mOneOverWidth;
      float           mOneOverHeight;      
      
      BD3DTextureManager::BManagedTexture* mpManagedTexture;
      IDirect3DTexture9*  mpD3DTexture;

      D3DFORMAT           mTextureFormat;
      
      bool           mReleasableTexture;
};
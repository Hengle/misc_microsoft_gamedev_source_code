//============================================================================
// flashtexture.cpp
// Ensemble Studios (C) 2007
//============================================================================

#include "xgameRender.h"
#include "flashtexture.h"
#include "flashtexturemanager.h"

#include "scaleformIncludes.h"


//============================================================================6
//============================================================================
GImageInfoBase* BFlashTextureLoader::LoadImage(const char *purl)
{
   GPtr<GImage> pimage;
   UInt         twidth = 0, theight = 0;

   IDirect3DTexture9* pDDXTexture = NULL;
   BD3DTextureManager::BManagedTexture* pManagedTexture = NULL;

   BRenderString path(purl);

   // trim off the img:// or imgps://
   if (path.compare("img://", false, 6) == 0)
      path.substring(6, path.length());
   else if (path.compare("imgps://", false, 8) == 0)
      path.substring(8, path.length());

   // check for gamerPic key
   if (path.compare("gamerPic:", false, 9) == 0)
   {
      BRenderString gamerKey(path);
      gamerKey.substring(9, gamerKey.length());

      return gFlashTextureManager.getOrCreateGamerPic(gamerKey, mpRenderer);
   }
   else
   {
      pManagedTexture = gFlashTextureManager.getOrCreate(path, mCategory);
   }

   // Renderer ok, create a texture-based GFxImageInfo instead. This means that
   // the source image memory will be released by our caller.  
   BDEBUG_ASSERT(mpRenderer);
   if (!pManagedTexture && !pDDXTexture)
      return NULL;

   GPtr<BFlashTexture> pNewTexture = *(reinterpret_cast<BFlashTexture*>(mpRenderer->CreateTexture()));
   if (!pNewTexture)
      return NULL;

   if (pManagedTexture)
   {
      twidth = pManagedTexture->getWidth();
      theight = pManagedTexture->getHeight();
      
      if (!pNewTexture->InitTexture(pManagedTexture, twidth, theight))
         return NULL;

      return new GImageInfo(pNewTexture, twidth, theight);
   }   

   return NULL;
}

//============================================================================
//============================================================================
GImageInfoBase* BFlashTextureCreator::CreateImage(const GFxImageCreateInfo &info)
{
   GPtr<GImage> pimage;
   UInt         twidth = 0, theight = 0;

   static bool bAllowDDX = true;
   BD3DTextureManager::BManagedTexture* pDDXTexture = NULL;
   switch(info.Type)
   {
      case GFxImageCreateInfo::Input_Image:
         pimage  = info.pImage;
         // We have to pass correct size; it is required at least
         // when we are initializing image info with a texture.
         twidth  = pimage->Width;
         theight = pimage->Height;
         break;

      case GFxImageCreateInfo::Input_File:
         {
            if (bAllowDDX)
            {
               BRenderString path(info.pFileInfo->FileName.ToCStr());

               int i = path.findLeft("file://");
               if (i != -1)
                  path.crop(i + 7, path.length() - 1);

               BRenderString directory;
               BRenderString filename;
               strPathGetDirectory(path, directory, true);
               strPathGetFilename(path, filename);


               BRenderString fullpath;
               fullpath.format("%stextures\\%s", directory.getPtr(), filename.getPtr());

               pDDXTexture = gFlashTextureManager.getOrCreate(fullpath, mCategory);
            }

            if (!pDDXTexture)
            {
               // If we got here, we are responsible for loading an image file.
               GPtr<GFile> pfile  = *info.pFileOpener->OpenFile(info.pFileInfo->FileName.ToCStr());
               if (!pfile)
                  return 0; // Log ??
               
               // Load an image into GImage object.
               pimage = *LoadBuiltinImage(pfile, info.pFileInfo->Format, info.Use);
               if (!pimage)
                  return 0;               
            }

            twidth  = info.pFileInfo->TargetWidth;
            theight = info.pFileInfo->TargetHeight;
         }        
         break;

         // No input - empty image info.
      case GFxImageCreateInfo::Input_None:
      default:
         return new GImageInfo();
   }


   // Make a distinction whether to keep the data or not based on
   // the KeepBindingData flag in GFxImageInfo.
   if (IsKeepingImageData())    
      return new GImageInfo(pimage, twidth, theight);

   // Else, create a texture.
   if (!info.pRenderConfig || !info.pRenderConfig->GetRenderer())
   {
      // We need to either provide a renderer, or use KeepImageBindData flag.
      GFC_DEBUG_ERROR(1, "GFxImageCreator failed to create texture; no renderer installed");
      return 0;
   }

   // If renderer can generate Event_DataLost for textures (D3D9), store GImage in GFxImageInfo
   // anyway. This is helpful because otherwise images can be lost and wiped out. Users can
   // override this behavior with custom creator if desired, handling loss in alternative manner.
   if (info.pRenderConfig->GetRendererCapBits() & GRenderer::Cap_CanLoseData)    
      return new GImageInfo(pimage, twidth, theight);    


   BFlashRender* pRenderer = reinterpret_cast<BFlashRender*>(info.pRenderConfig->GetRenderer());
   BDEBUG_ASSERT(pRenderer);
   // Renderer ok, create a texture-based GFxImageInfo instead. This means that
   // the source image memory will be released by our caller.      

   GPtr<BFlashTexture> pTexture = *(reinterpret_cast<BFlashTexture*>(pRenderer->CreateTexture()));
   if (pDDXTexture)
   {            
      if (!pTexture || !pTexture->InitTexture(pDDXTexture, twidth, theight))
         return 0;

      return new GImageInfo(pTexture, twidth, theight);
   }   
         
   if (!pTexture || !pTexture->InitTexture(pimage, twidth, theight))
      return 0;

   return new GImageInfo(pTexture, twidth, theight);
}


//============================================================================
//============================================================================
BFlashTexture::BFlashTexture(BFlashRender* pRenderer)
: GTextureD3D9(&pRenderer->mTextures)
{
   mpRenderer = pRenderer;
   mWidth = 0; 
   mHeight = 0;
   mOneOverWidth = 0.0f;
   mOneOverHeight = 0.0f;   
   mpD3DTexture = NULL;   
   mpManagedTexture = NULL;
   mReleasableTexture = true;
}

//============================================================================
//============================================================================
BFlashTexture::~BFlashTexture()
{
   if (mpManagedTexture)
   {
      //trace("BFlashTexture::~BFlashTexture() - releasing managed texture %x", mpManagedTexture);
      mpManagedTexture->release();
      mpManagedTexture = NULL;
   }

   if (mpD3DTexture)
   {   
      if (mpRenderer && mpRenderer->mpDevice && mpD3DTexture->IsSet(mpRenderer->mpDevice))
      {
         mpRenderer->mpDevice->SetTexture(0,0);
         mpRenderer->mpDevice->SetTexture(1,0);
      }
      if (mReleasableTexture)
         BD3DTexture::releaseWildTexture(mpD3DTexture);
      mpD3DTexture = NULL;
      mReleasableTexture = true;
   }
}

//============================================================================
// // Obtains the renderer that create TextureInfo 
//============================================================================
GRenderer*  BFlashTexture::GetRenderer() const
{ 
   return mpRenderer; 
}

//============================================================================
//============================================================================
bool BFlashTexture::IsDataValid() const
{ 
   return ((mpManagedTexture != NULL) || (mpD3DTexture != NULL));
}

//============================================================================
// Remove texture from renderer, notifies renderer destruction
//============================================================================
void BFlashTexture::RemoveFromRenderer()
{
   if (mpManagedTexture)
   {
      //trace("BFlashTexture::~BFlashTexture() - releasing managed texture %x", mpManagedTexture);
      mpManagedTexture->release();
      mpManagedTexture = NULL;
   }

   if (mpD3DTexture)
   {
      if (mReleasableTexture)
         BD3DTexture::releaseWildTexture(mpD3DTexture);
      mpD3DTexture = 0;
      mReleasableTexture = true;
      mWidth = 0; 
      mHeight = 0;
      mOneOverWidth = 0.0f;
      mOneOverHeight = 0.0f;
   }

   mpRenderer = 0;
   AddRef();
   CallHandlers(ChangeHandler::Event_RendererReleased);
   if (pNext) // We may have been released by user
      RemoveNode();
   Release();
}

//============================================================================
// Creates a D3D texture of the specified dest dimensions, from a
// resampled version of the given src image.  Does a bilinear
// resampling to create the dest image.
// Source can be 4,3, or 1 bytes/pixel. Destination is either 1 or 4 bytes/pixel.
//============================================================================
void FlashSoftwareResample(
                         UByte* pDst, int dstWidth, int dstHeight, int dstPitch,
                         UByte* pSrc, int srcWidth, int srcHeight, int srcPitch,
                         int bytesPerPixel )
{
   switch(bytesPerPixel)
   {
   case 4: 
      GRenderer::ResizeImage(pDst, dstWidth, dstHeight, dstPitch,
         pSrc, srcWidth, srcHeight, srcPitch,
         GRenderer::ResizeRgbaToRgba);
      break;

   case 3:
      GRenderer::ResizeImage(pDst, dstWidth, dstHeight, dstPitch,
         pSrc, srcWidth, srcHeight, srcPitch,
         GRenderer::ResizeRgbToRgba);
      break;

   case 1:
      GRenderer::ResizeImage(pDst, dstWidth, dstHeight, dstPitch,
         pSrc, srcWidth, srcHeight, srcPitch,
         GRenderer::ResizeGray);
      break;
   }
}

//============================================================================
//============================================================================
bool BFlashTexture::InitTexture(BD3DTextureManager::BManagedTexture* pTexture, SInt width, SInt height)
{
   if (!mpRenderer || !mpRenderer->mpDevice)
      return 0;

   if (mpManagedTexture)
   {
      //trace("BFlashTexture::~BFlashTexture() - releasing managed texture %x", mpManagedTexture);
      mpManagedTexture->release();
      mpManagedTexture = NULL;
   }

   if (mpD3DTexture)
   {
      if (mReleasableTexture)
         BD3DTexture::releaseWildTexture(mpD3DTexture);      
      mpD3DTexture = NULL;
   }
   
   mpManagedTexture = pTexture;
   if (mpManagedTexture)
   {
      mWidth = mpManagedTexture->getWidth();
      mHeight = mpManagedTexture->getHeight();      
      mOneOverWidth  = 1.0f / mWidth;
      mOneOverHeight = 1.0f / mHeight;            
   }

   CallHandlers(ChangeHandler::Event_DataChange);
   return 1;
}

//============================================================================
//============================================================================
bool BFlashTexture::InitTexture(IDirect3DTexture9* pTex, SInt width, SInt height)
{
   if (!mpRenderer || !mpRenderer->mpDevice)
      return 0;

   if (mpManagedTexture)
   {
      //trace("BFlashTexture::~BFlashTexture() - releasing managed texture %x", mpManagedTexture);
      mpManagedTexture->release();
      mpManagedTexture = NULL;
   }

   if (mpD3DTexture)
   {
      if (mReleasableTexture)
         BD3DTexture::releaseWildTexture(mpD3DTexture);
      mReleasableTexture = true;
      mpD3DTexture = 0;
   }
   
   if (pTex)
   {
      mWidth = width;
      mHeight = height;
      mOneOverWidth  = 1.0f / mWidth;
      mOneOverHeight = 1.0f / mHeight;      
      mpD3DTexture = pTex;
      
      mReleasableTexture = true;
      // rg [1/26/08] - Try to determine if the texture came from the BFlashTextureManager.
      if (mpD3DTexture->GetIdentifier())
         mReleasableTexture = false;
      
      if (mReleasableTexture)
         pTex->AddRef();
   }

   CallHandlers(ChangeHandler::Event_DataChange);
   return 1;
}

//============================================================================
// NOTE: This function destroys pim's data in the process of making mipmaps.
//============================================================================
bool BFlashTexture::InitTexture(GImageBase* pImage, int targetWidth, int targetHeight)
{   
   if (!mpRenderer || !mpRenderer->mpDevice)
      return 0;

   // Delete old data
   if (mpManagedTexture)
   {
      //trace("BFlashTexture::~BFlashTexture() - releasing managed texture %x", mpManagedTexture);
      mpManagedTexture->release();
      mpManagedTexture = NULL;
   }

   if (mpD3DTexture)
   {
      if (mReleasableTexture)
         BD3DTexture::releaseWildTexture(mpD3DTexture);
      mpD3DTexture = 0;
      mReleasableTexture = false;
      mWidth = 0;
      mHeight = 0;
      mOneOverWidth = 0.0f;
      mOneOverHeight = 0.0f;
   }
   if (!pImage || !pImage->pData)
   {
      // Kill texture     
      CallHandlers(ChangeHandler::Event_DataChange);
      return 1;
   }

   // Determine format
   UInt    bytesPerPixel = 0;  

   switch(pImage->Format)
   {
   case GImage::Image_ARGB_8888:
      bytesPerPixel = 4;        
      mTextureFormat = D3DFMT_A8R8G8B8;
      break;   
   case GImage::Image_RGB_888:
      bytesPerPixel   = 3;
      mTextureFormat   = D3DFMT_DXT1;
      //TextureFormat = D3DFMT_A8R8G8B8;
      break;
   case GImage::Image_A_8:
      bytesPerPixel   = 1;
      mTextureFormat   = D3DFMT_A8;
      break;
   case GImage::Image_DXT1:
      mTextureFormat = D3DFMT_LIN_DXT1;
      bytesPerPixel = 1;
      break;
   case GImage::Image_DXT3:
      mTextureFormat = D3DFMT_LIN_DXT3;
      bytesPerPixel = 1;
      break;
   case GImage::Image_DXT5:
      mTextureFormat = D3DFMT_LIN_DXT5;
      bytesPerPixel = 1;
      break;

   default:
      // Unsupported format
      GASSERT(0);
      return 0;
   }


   mWidth = (targetWidth == 0)  ? pImage->Width : targetWidth;
   mHeight = (targetHeight == 0) ? pImage->Height: targetHeight;
   mOneOverWidth = 1.0f / mWidth;
   mOneOverHeight = 1.0f / mHeight;
   

   // Don't use DXT1 for thin textures (it must be a multiple of 4 for DirectX)
   /*
   if (((TextureFormat== D3DFMT_DXT1) || (TextureFormat== D3DFMT_DXT3)) && 
   ((Width<4) || (Height<4)))
   TextureFormat = D3DFMT_A8R8G8B8;
   */

   UInt    wlevels = 1, hlevels = 1;
   UInt    w = 1; while (w < pImage->Width) { w <<= 1; wlevels++; }
   UInt    h = 1; while (h < pImage->Height) { h <<= 1; hlevels++; }
   UInt    levelsNeeded;

   if (pImage->IsDataCompressed() || pImage->MipMapCount > 1)
      levelsNeeded = GTL::gmax<UInt>(1, pImage->MipMapCount);
   else
      levelsNeeded = GTL::gmax<UInt>(wlevels, hlevels);


   GPtr<GImage> presampleImage;

   // Set the actual data.
   if (!pImage->IsDataCompressed() && (w != pImage->Width || h != pImage->Height ||
      (pImage->Format == GImage::Image_RGB_888) ||
      (pImage->Format == GImage::Image_ARGB_8888)) )  
   {
      // Resample the image to new size
      if (presampleImage = *new GImage(
         ((pImage->Format == GImage::Image_RGB_888) ? GImage::Image_ARGB_8888 : pImage->Format), w, h))
      {
         GASSERT_ON_RENDERER_RESAMPLING;

         if (w != pImage->Width || h != pImage->Height)
         {
            // Resample will store an extra Alpha byte for RGB_888 -> RGBA_8888
            FlashSoftwareResample(presampleImage->pData, w, h, presampleImage->Pitch,
               pImage->pData, pImage->Width, pImage->Height, pImage->Pitch, bytesPerPixel);
         }
         else if (pImage->Format == GImage::Image_RGB_888)
         {
            // Need to insert a dummy alpha byte in the image data, for D3DXLoadSurfaceFromMemory.      
            for (UInt y = 0; y < h; y++)
            {
               UByte*  scanline = pImage->GetScanline(y);
               UByte*  pdest    = presampleImage->GetScanline(y);
               for (UInt x = 0; x < w; x++)
               {
                  pdest[x * 4 + 0] = scanline[x * 3 + 0]; // red
                  pdest[x * 4 + 1] = scanline[x * 3 + 1]; // green
                  pdest[x * 4 + 2] = scanline[x * 3 + 2]; // blue
                  pdest[x * 4 + 3] = 255; // alpha
               }
            }
         }
         else if (pImage->Format == GImage::Image_ARGB_8888)
         {
            XMemCpy(presampleImage->GetScanline(0), pImage->GetScanline(0),
               pImage->GetPitch() * pImage->Height);
         }
         // HACK: disable mipmaps, need to do something with them! (AB)
         if (pImage->MipMapCount > 1)
         {
            GFC_DEBUG_WARNING(1, "GRendererD3D9 - Existing mipmap levels have been skipped due to resampling");
            levelsNeeded = 1;
         }

         // Swap byte order for Big-Endian XBox 360
         if  (presampleImage->Format == GImage::Image_ARGB_8888)
         {
            for (UInt y = 0; y < h; y++)
            {                   
               UInt32* pdest = (UInt32*)presampleImage->GetScanline(y);
               for (UInt x = 0; x < w; x++)
                  pdest[x] = _byteswap_ulong(pdest[x]);
            }

            // The new image has 4 bytes/pixel.
            bytesPerPixel = 4;          
         }
      }
   }

   // Create the texture.
   // For now, only generate mipmaps for alpha textures.
   // MA: For some reason we need to specify levelsNeeded-1, otherwise
   // surface accesses may crash (when running with Gamebryo).
   // So, 256x256 texture seems to have levelCount of 8 (not 9).
   if (pImage->MipMapCount <= 1 && bytesPerPixel != 1)
      levelsNeeded = 1;
   else
      levelsNeeded = GTL::gmax(1u, levelsNeeded - 1);
   //levelsNeeded = GTL::gmax(1u, (bytesPerPixel == 1) ? UInt(levelsNeeded - 1) : 1u);

   HRESULT result = mpRenderer->mpDevice->CreateTexture(
      w, h,
      levelsNeeded, 
      0, // XBox: D3DUSAGE_BORDERSOURCE_TEXTURE                       
      mTextureFormat,
      D3DPOOL_DEFAULT,                        
      &mpD3DTexture, NULL);
   
   if (result != S_OK)
   {    
      GFC_DEBUG_ERROR(1, "GTextureD3D9 - Can't create texture");
      mWidth = 0;
      mHeight = 0;
      mOneOverWidth = 0.0f;
      mOneOverHeight = 0.0f;
      return 0;
   }

   mReleasableTexture = true;

   if (pImage->IsDataCompressed() || pImage->MipMapCount > 1)
   {
      IDirect3DSurface9*  psurface    = NULL; 
      UInt                level;  
      GImageBase*         psourceImage= presampleImage ? presampleImage.GetPtr() : pImage;
      RECT                sourceRect  = { 0,0, w,h};
      D3DFORMAT           sourceSurfaceFormat = mTextureFormat;

      // Determine source data format correctly (it may be different from texture).         
      if (mTextureFormat == D3DFMT_A8R8G8B8)
         sourceSurfaceFormat = D3DFMT_LIN_A8B8G8R8;
      else if (mTextureFormat == D3DFMT_A8)
         sourceSurfaceFormat = D3DFMT_LIN_A8;

      for(level = 0; level < levelsNeeded; level++)
      {           
         // Load all levels...
         if (mpD3DTexture->GetSurfaceLevel(level, &psurface) != S_OK)
         {
            mpD3DTexture->Release();
            mpD3DTexture = 0;
            mReleasableTexture = false;
            mWidth = 0;
            mHeight = 0;
            mOneOverWidth = 0.0f;
            mOneOverHeight = 0.0f;
            return 0;
         }       
         GASSERT(psurface);

         UInt mipW, mipH;
         const UByte* pmipData = psourceImage->GetMipMapLevelData(level, &mipW, &mipH);

         sourceRect.right = mipW;
         sourceRect.bottom = mipH;

         result = D3DXLoadSurfaceFromMemory(
            psurface, NULL, NULL,
            pmipData,
            sourceSurfaceFormat,
            (pImage->IsDataCompressed()) ? 
            GImageBase::GetMipMapLevelSize(psourceImage->Format, mipW, 1) : 
         GImageBase::GetPitch(psourceImage->Format, mipW),
            NULL,
            &sourceRect,
            FALSE, 0, 0, // no packed mipmap for now
            D3DX_DEFAULT, 0);
         
         mpD3DTexture->Release();
         psurface = 0;
         
         if (result != S_OK)
         {
            GFC_DEBUG_ERROR1(1, "GTextureD3D9 - Can't load surface from memory, result = %d", result);
            mpD3DTexture->Release();
            mpD3DTexture = 0;
            mReleasableTexture = true;
            mWidth = 0;
            mHeight = 0;
            mOneOverWidth = 0.0f;
            mOneOverHeight = 0.0f;
            return 0;
         }
      }

   }
   else
   {
      // Will need a buffer for destructive mipmap generation         
      if ((bytesPerPixel == 1) && (levelsNeeded >1) && !presampleImage)
      {
         GASSERT_ON_RENDERER_MIPMAP_GEN;
         // A bit hacky, needs to be more general
         presampleImage = *GImage::CreateImage(pImage->Format, pImage->Width, pImage->Height);
         GASSERT(pImage->Pitch == presampleImage->Pitch);
         XMemCpy(presampleImage->pData, pImage->pData, pImage->Height * pImage->Pitch);       
      }

      IDirect3DSurface9*  psurface    = NULL; 
      UInt                level;  
      GImageBase*         psourceImage= presampleImage ? presampleImage.GetPtr() : pImage;
      RECT                sourceRect  = { 0,0, w,h};

      for(level = 0; level<levelsNeeded; level++)
      {           
         // Load all levels...
         if (mpD3DTexture->GetSurfaceLevel(level, &psurface) != S_OK)
         {
            mpD3DTexture->Release();
            mpD3DTexture = 0;
            mReleasableTexture = true;
            mWidth = 0;
            mHeight = 0;
            mOneOverWidth = 0.0f;
            mOneOverHeight = 0.0f;
            return 0;
         }       
         GASSERT(psurface);

         result = D3DXLoadSurfaceFromMemory(
            psurface, NULL, NULL,
            psourceImage->pData,
            (bytesPerPixel == 1) ? D3DFMT_LIN_A8 : D3DFMT_LIN_A8B8G8R8, // NOTE: format order conversion from GL
            (bytesPerPixel == 1) ? sourceRect.right : psourceImage->Pitch,
            NULL,
            &sourceRect,
            FALSE, 0, 0, // no packed mipmap for now
            D3DX_DEFAULT, 0);           
         
         mpD3DTexture->Release();
         psurface = 0;

         if (result != S_OK)
         {
            GFC_DEBUG_ERROR1(1, "GTextureD3D9 - Can't load surface from memory, result = %d", result);
            mpD3DTexture->Release();
            mpD3DTexture = 0;
            mReleasableTexture = true;
            mWidth = 0;
            mHeight = 0;
            mOneOverWidth = 0.0f;
            mOneOverHeight = 0.0f;
            return 0;
         }

         // For Alpha-only images, we might need to generate the next mipmap level.
         if (level< (levelsNeeded-1))
         {
            GCOMPILER_ASSERT(sizeof(sourceRect.right) == sizeof(int));
            BFlashRender::MakeNextMiplevel((int*) &sourceRect.right, (int*) &sourceRect.bottom,
               psourceImage->pData);
         }
      }
   }

   CallHandlers(ChangeHandler::Event_DataChange);
   return 1;
}

//============================================================================
//============================================================================
bool BFlashTexture::InitTextureFromFile(const char* pfilename, int targetWidth, int targetHeight)
{   
   if (!mpRenderer || !mpRenderer->mpDevice)
      return 0;

   // Delete old data
   if (mpManagedTexture)
   {
      //trace("BFlashTexture::~BFlashTexture() - releasing managed texture %x", mpManagedTexture);
      mpManagedTexture->release();
      mpManagedTexture = NULL;
   }

   if (mpD3DTexture)
   {
      if (mReleasableTexture)
         BD3DTexture::releaseWildTexture(mpD3DTexture);
      mpD3DTexture = 0;
      mReleasableTexture = true;
      mWidth = 0;
      mHeight = 0;
      mOneOverWidth = 0.0f;
      mOneOverHeight = 0.0f;
   }
   if (!pfilename)
   {
      // Kill texture     
      CallHandlers(ChangeHandler::Event_DataChange);
      return 1;
   }

   // Create the texture.
   HRESULT result;
   result = D3DXCreateTextureFromFileEx(
      mpRenderer->mpDevice, 
      pfilename, 
      D3DX_DEFAULT, 
      D3DX_DEFAULT, 
      D3DX_DEFAULT, 0, 
      D3DFMT_UNKNOWN, 
      D3DPOOL_DEFAULT, 
      D3DX_DEFAULT, 
      D3DX_DEFAULT, 0, NULL, NULL, 
      &mpD3DTexture);

   if (result != S_OK)
   {
      GFC_DEBUG_ERROR1(1, "GTextureXbox360 - Can't create texture from file %s", pfilename);
      mWidth = 0;
      mHeight = 0;
      mOneOverWidth = 0.0f;
      mOneOverHeight = 0.0f;
      return 0;
   }
   
   mReleasableTexture = true;

   mWidth   = targetWidth;
   mHeight  = targetHeight;
   
   if ((mWidth == 0) || (mHeight==0))
   {
      D3DSURFACE_DESC desc;
      mpD3DTexture->GetLevelDesc(0, &desc);
      mWidth   = desc.Width;
      mHeight  = desc.Height;      
   }

   mOneOverWidth = 1.0f / mWidth;
   mOneOverHeight = 1.0f / mHeight;

   CallHandlers(ChangeHandler::Event_DataChange);
   return 1;
}

//============================================================================
//============================================================================
bool BFlashTexture::InitTexture(int width, int height, GImage::ImageFormat format, int mipmaps,
                                      int targetWidth , int targetHeight)
{
   if (!mpRenderer || !mpRenderer->mpDevice)
      return 0;

   if (mpManagedTexture)
   {
      //trace("BFlashTexture::~BFlashTexture() - releasing managed texture %x", mpManagedTexture);
      mpManagedTexture->release();
      mpManagedTexture = NULL;
   }

   if (mpD3DTexture)
   {
      if (mReleasableTexture)
         BD3DTexture::releaseWildTexture(mpD3DTexture);
      mpD3DTexture = NULL;
      mReleasableTexture = true;
   }

   if (format == GImage::Image_ARGB_8888)
      mTextureFormat   = D3DFMT_LIN_A8R8G8B8;
   else if (format == GImage::Image_RGB_888)
      mTextureFormat   = D3DFMT_LIN_A8R8G8B8;
   else if (format == GImage::Image_A_8)
      mTextureFormat   = D3DFMT_LIN_A8;
   else 
   { // Unsupported format
      GASSERT(0);
      return 0;
   }

   mWidth   = (targetWidth == 0)  ? width : targetWidth;
   mHeight  = (targetHeight == 0) ? height: targetHeight;

   mOneOverWidth = 1.0f / mWidth;
   mOneOverHeight = 1.0f / mHeight;

   HRESULT result = mpRenderer->mpDevice->CreateTexture(width, height, mipmaps+1, 0, mTextureFormat, D3DPOOL_DEFAULT,
      &mpD3DTexture, NULL);
   if (result != S_OK)
      return 0;
   mReleasableTexture = true;

   return 1;
}

//============================================================================
//============================================================================
void BFlashTexture::Update(int level, int n, const UpdateRect *rects, const GImageBase *pim)
{
   UInt     bytesPerPixel, convert = 0;

   if (pim->Format == GImage::Image_ARGB_8888)
      bytesPerPixel   = 4;
   else if (pim->Format == GImage::Image_RGB_888)
   {
      convert = 1;
      bytesPerPixel   = 4;
   }
   else if (pim->Format == GImage::Image_A_8)
      bytesPerPixel   = 1;
   else
   {
      GASSERT(0);
      return;
   }

   if (!mpD3DTexture && !CallRecreate())
   {
      GFC_DEBUG_WARNING(1, "GTextureXBox360::Update failed, could not recreate texture");
      return;     
   }

   // Ensure that our texture isn't set on the device during Update.
   mpRenderer->mpDevice->SetTexture(0, 0);
   mpRenderer->mpDevice->SetTexture(1, 0);

   for (int i = 0; i < n; i++)
   {
      GRect<int> rect = rects[i].src;
      GPoint<int> dest = rects[i].dest;

      D3DLOCKED_RECT      lr;
      RECT                destr;

      destr.left	 = dest.x;
      destr.bottom = dest.y + rect.Height() - 1;
      destr.right  = dest.x + rect.Width() - 1;
      destr.top    = dest.y;

      mpD3DTexture->LockRect(level, &lr, &destr, 0);
      //if (FAILED( pD3DTexture->LockRect(level, &lr, &destr, 0) ))
      //    return;

      if (convert && pim->Format == GImage::Image_RGB_888)
      {
         UByte *pdest = (UByte*)lr.pBits;
         //pdest += dest.y * lr.Pitch + dest.x * bytesPerPixel;

         for (int j = 0; j < rect.Height(); j++)
            for (int k = 0; k < rect.Width(); k++)
            {
               pdest[j * lr.Pitch + k * bytesPerPixel +0] = pim->pData[(j + rect.Top) * pim->Pitch + (k + rect.Left) * 3 +0];
               pdest[j * lr.Pitch + k * bytesPerPixel +1] = pim->pData[(j + rect.Top) * pim->Pitch + (k + rect.Left) * 3 +1];
               pdest[j * lr.Pitch + k * bytesPerPixel +2] = pim->pData[(j + rect.Top) * pim->Pitch + (k + rect.Left) * 3 +2];
               pdest[j * lr.Pitch + k * bytesPerPixel +3] = 255;
            }
      }
      else
      {
         UByte *pdest = (UByte*)lr.pBits;

         for (int j = 0; j < rect.Height(); j++)
            memcpy(pdest + j * lr.Pitch,
            pim->GetScanline(j + rect.Top) + bytesPerPixel * rect.Left,
            rect.Width() * bytesPerPixel);
      }

      mpD3DTexture->UnlockRect(level);
   }
}

//============================================================================
//============================================================================
IDirect3DTexture9* BFlashTexture::GetD3DTexture() const
{
   if (mpManagedTexture)
      return mpManagedTexture->getD3DTexture().getTexture();

   if (mpD3DTexture)
   {
      //trace("FLASH WARNING: Using non managed texture for Flash!!! Are you using a SWF file instead of GFX?");
      return mpD3DTexture;
   }

   return NULL;
}
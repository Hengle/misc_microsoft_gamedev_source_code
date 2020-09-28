//==============================================================================
// File: cubemapGen.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#include "xgameRender.h"
#include "cubemapGen.h"
#include "imageUtils.h"
#include "colorUtils.h"
#include "bfileStream.h"
#include "math\cubemap.h"

//==============================================================================
// gVerticalCrossXYOffsets
//==============================================================================
static const uchar gVerticalCrossXYOffsets[] = 
{
   2, 1, // +X
   0, 1, // -X
   1, 0, // +Y
   1, 2, // -Y
   1, 1, // +Z
   1, 3, // -Z
};

//==============================================================================
// BCubemapGen::BCubemapGen
//==============================================================================
BCubemapGen::BCubemapGen() :
   mSrcWidth(0),
   mSrcHeight(0),
   mCubeDim(0)
{
   Utils::ClearObj(mpResamplers);
}

//==============================================================================
// BCubemapGen::~BCubemapGen
//==============================================================================
BCubemapGen::~BCubemapGen()
{
   deinit();
}

//==============================================================================
// BCubemapGen::init
//==============================================================================
void BCubemapGen::init(uint srcWidth, uint srcHeight, uint dim)
{
   BDEBUG_ASSERT((srcWidth > 0) && (srcHeight > 0) && (dim > 0));
   
   deinit();
   
   mSrcWidth = srcWidth;
   mSrcHeight = srcHeight;
   mCubeDim = dim;
   
   const float filterXScale = 1.0f;
   const float filterYScale = 1.0f;
   const char* pFilter = "box";
   if ((srcWidth > dim) || (srcHeight > dim))
      pFilter = "lanczos3";
   
   for (uint resamplerIndex = 0; resamplerIndex < cNumResamplers; resamplerIndex++)
   {
      if (!resamplerIndex)
         mpResamplers[resamplerIndex] = new Resampler(mSrcWidth, mSrcHeight, mCubeDim, mCubeDim, Resampler::BOUNDARY_CLAMP, 0.0f, 64.0f, pFilter, NULL, NULL, filterXScale, filterYScale, 0.0f, 0.0f);
      else
         mpResamplers[resamplerIndex] = new Resampler(mSrcWidth, mSrcHeight, mCubeDim, mCubeDim, Resampler::BOUNDARY_CLAMP, 0.0f, 64.0f, pFilter, mpResamplers[0]->get_clist_x(), mpResamplers[0]->get_clist_y(), filterXScale, filterYScale, 0.0f, 0.0f);
   }
   
   mCubemap.setSize(mCubeDim * 3, mCubeDim * 4);
   mCubemap.clear(BRGBAColor16(0, 0, 0, 0));
}

//==============================================================================
// BCubemapGen::deinit
//==============================================================================
void BCubemapGen::deinit(void)
{
   for (uint resamplerIndex = 0; resamplerIndex < cNumResamplers; resamplerIndex++)
   {
      delete mpResamplers[resamplerIndex];
      mpResamplers[resamplerIndex] = NULL;
   }
   
   mCubemap.clear();
}

//==============================================================================
// BCubemapGen::captureFace
//==============================================================================
void BCubemapGen::captureFace(IDirect3DTexture9* pCaptureTex, uint faceIndex)
{
   BDEBUG_ASSERT(mpResamplers[0]);
   
   D3DSURFACE_DESC texDesc;
   pCaptureTex->GetLevelDesc(0, &texDesc);

   BDEBUG_ASSERT((mSrcWidth <= texDesc.Width) && (mSrcHeight <= texDesc.Height));
         
   if (faceIndex > 0)
   {
      for (uint resamplerIndex = 0; resamplerIndex < cNumResamplers; resamplerIndex++)
         mpResamplers[resamplerIndex]->restart();
   }
   
   D3DLOCKED_RECT rect;
   pCaptureTex->LockRect(0, &rect, NULL, 0);

   const uint64* pSurf = static_cast<const uint64*>(rect.pBits);

   pCaptureTex->UnlockRect(0);
   
   BDynamicArray<float> scanlineBufs[cNumResamplers];
   for (uint i = 0; i < cNumResamplers; i++)
      scanlineBufs[i].resize(mSrcWidth);
   
   const uint dstX = gVerticalCrossXYOffsets[faceIndex * 2 + 0] * mCubeDim;
   uint dstY = gVerticalCrossXYOffsets[faceIndex * 2 + 1] * mCubeDim;
   const uint startDstY = dstY;
   startDstY;

   for (uint srcY = 0; srcY < mSrcHeight; srcY++)
   {
      for (uint srcX = 0; srcX < mSrcWidth; srcX++)
      {
         const uint ofs = XGAddress2DTiledOffset(srcX, srcY, texDesc.Width, sizeof(uint64));

         // pSurf points to uncached memory, so load it into a temp before converting it.
         const uint64 p = pSurf[ofs];

         XMFLOAT4A c;
         XMStoreFloat4A(&c, XMLoadHalf4(reinterpret_cast<const XMHALF4*>(&p)));
         
         scanlineBufs[0][srcX] = c.x;
         scanlineBufs[1][srcX] = c.y;
         scanlineBufs[2][srcX] = c.z;
      }
      
      for (uint resamplerIndex = 0; resamplerIndex < cNumResamplers; resamplerIndex++)
         mpResamplers[resamplerIndex]->put_line(scanlineBufs[resamplerIndex].getPtr());
   
      for ( ; ; )
      {
         const Resampler::Sample* pSamples[3];
         for (uint resamplerIndex = 0; resamplerIndex < cNumResamplers; resamplerIndex++)
            pSamples[resamplerIndex] = mpResamplers[resamplerIndex]->get_line();
          
         if (!pSamples[0])
            break;
         
         XMFLOAT4A c;
         c.w = 1.0f;
         for (uint x = 0; x < mCubeDim; x++)
         {
            c.x = pSamples[0][x];
            c.y = pSamples[1][x];
            c.z = pSamples[2][x];            
            
            XMHALF4 h;
            XMStoreHalf4(&h, XMLoadFloat4A(&c));
            
            mCubemap.setPixel(x + dstX, dstY, BRGBAColor16(h.x, h.y, h.z, h.w));
         }  
         
         dstY++;
      }
   }
   
   BDEBUG_ASSERT((dstY - startDstY) == mCubeDim);
}

//==============================================================================
// BCubemapGen::saveToHDRFile
//==============================================================================
bool BCubemapGen::saveToHDRFile(long dirID, const char* pFilename)
{
   BDEBUG_ASSERT(mCubemap.getWidth() > 0);
   
   BFileSystemStream dstStream;
   if (!dstStream.open(dirID, pFilename, cSFWritable | cSFSeekable | cSFEnableBuffering))
      return false;

   if (!BImageUtils::writeHDR(dstStream, mCubemap))
      return false;
      
   if (!dstStream.close())
      return false;

   return true;      
}

//==============================================================================
// BCubemapGen::genSHCoefficients
//==============================================================================
void BCubemapGen::calculateSHCoefficients(SphericalHarmonic::Vec9Vector& coeffs)
{
   BCubemap<BVec3> floatCubemap(mCubeDim, mCubeDim);
   
   for (uint faceIndex = 0; faceIndex < 6; faceIndex++)
   {
      const bool flip = (faceIndex == 5);

      const uint faceXOffset = gVerticalCrossXYOffsets[faceIndex * 2 + 0];
      const uint faceYOffset = gVerticalCrossXYOffsets[faceIndex * 2 + 1];

      for (uint y = 0; y < mCubeDim; y++)
      {
         for (uint x = 0; x < mCubeDim; x++)
         {
//-- FIXING PREFIX BUG ID 6512
            const BRGBAColor16& srcPixel = mCubemap(faceXOffset * mCubeDim + x, faceYOffset * mCubeDim + y);
//--
            
            XMHALF4 h((HALF)srcPixel.r, (HALF)srcPixel.g, (HALF)srcPixel.b, (HALF)srcPixel.a);
            
            XMVECTOR v = XMLoadHalf4(&h);
            
            BVec3 f(v.x, v.y, v.z);

            if (flip)
               floatCubemap.getPixel(faceIndex, mCubeDim - 1 - x, mCubeDim - 1 - y) = f;
            else
               floatCubemap.getPixel(faceIndex, x, y) = f;
         }
      }         
   }
   
   coeffs = SphericalHarmonic::project(floatCubemap);
}

//==============================================================================
// BCubemapGen::unprojectSHCoefficients
//==============================================================================
void BCubemapGen::unprojectSHCoefficients(const SphericalHarmonic::Vec9Vector& coeffs, uint dim)
{
   mCubeDim = dim;
   mCubemap.setSize(mCubeDim * 3, mCubeDim * 4);
   mCubemap.clear(BRGBAColor16(0, 0, 0, 0));   
         
   for (uint faceIndex = 0; faceIndex < 6; faceIndex++)
   {
      const bool flip = (faceIndex == 5);

      const uint faceXOffset = gVerticalCrossXYOffsets[faceIndex * 2 + 0];
      const uint faceYOffset = gVerticalCrossXYOffsets[faceIndex * 2 + 1];
      
      for (uint y = 0; y < dim; y++)
      {
         XMFLOAT4A c;
         c.w = 1.0f;
         
         for (uint x = 0; x < dim; x++)
         {
            float fx = (x + .5f) / dim;
            float fy = (y + .5f) / dim;

            fx = fx * 2.0f - 1.0f;
            fy = fy * 2.0f - 1.0f;

            BVec3 dir;
            switch (faceIndex)
            {
               case 0: dir[0] = 1.0f;  dir[1] = -fy;   dir[2] = -fx;   break;
               case 1: dir[0] = -1.0f; dir[1] = -fy;   dir[2] = fx;    break;
               case 2: dir[0] = fx;    dir[1] = 1.0f;  dir[2] = fy;    break;
               case 3: dir[0] = fx;    dir[1] = -1.0f; dir[2] = -fy;   break;
               case 4: dir[0] = fx;    dir[1] = -fy;   dir[2] = 1.0f;  break;
               case 5: dir[0] = -fx;   dir[1] = -fy;   dir[2] = -1.0f; break;
            }

            dir.normalize();
            
            const BVecN<9> e(SphericalHarmonic::evaluate(dir));
            
            c.x = coeffs[0] * e;
            c.y = coeffs[1] * e;
            c.z = coeffs[2] * e;
            
            XMHALF4 h;
            XMStoreHalf4(&h, XMLoadFloat4A(&c));

            const uint dx = flip ? (dim - 1 - x) : x;
            const uint dy = flip ? (dim - 1 - y) : y;
            mCubemap.setPixel(dx + faceXOffset * dim, dy + faceYOffset * dim, BRGBAColor16(h.x, h.y, h.z, h.w));            
         }         
      }
   }
   
}









//============================================================================
//
//  File: xboxDataComposers.h
//
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

#include <xgraphics.h>

//==============================================================================
// class BXboxMemoryPool
//==============================================================================
class BXboxMemoryPool
{
public:
   BXboxMemoryPool(uint maxSize) : 
      mSize(0),
      mMaxSize(maxSize)
   {
   }
   
   // Be sure the array is cleared before calling this method!
   void reserve(uint maxSize)
   {
      mMaxSize = maxSize;
      mMemory.getAllocator().virtualReserve(mMaxSize);
   }

   uint alloc(uint size, uint alignment)
   {
      const uint curOfs = mSize;
      const uint bytesToAlignUp = Utils::BytesToAlignUpValue(curOfs, alignment);

      const uint totalBytesToAllocate = size + bytesToAlignUp;

      const uint allocOfs = curOfs + bytesToAlignUp;

      BVERIFY((curOfs + totalBytesToAllocate) <= mMaxSize);

      mSize = curOfs + totalBytesToAllocate;
      mMemory.resize(mSize);

      return allocOfs;
   }

   uint getOfs(const void* p)
   {
      if ((!p) || (!mSize))
         return 0;
      
      const int ofs = static_cast<const BYTE*>(p) - mMemory.getPtr();
      BDEBUG_ASSERT((ofs >= 0) && (ofs < (int)mSize));
      return ofs;
   }

   BYTE* getPtr(uint ofs = 0)
   {
      if (!mSize)
         return NULL;
      
      BDEBUG_ASSERT(ofs < mSize);
      return &mMemory[ofs];
   }

   BYTE* allocPtr(uint size, uint alignment)
   {
      return getPtr(alloc(size, alignment));
   }

   uint getSize(void) const 
   { 
      return mSize;
   }

   void clear(void)
   {
      mMemory.clear();
      mSize = 0;
   }

   BDynamicArray<BYTE, 16, BDynamicArrayVirtualAllocator> mMemory;
   uint mSize;
   uint mMaxSize;
};

//==============================================================================
// class BXboxVBComposer
//==============================================================================
class BXboxVBComposer
{
   uint mNumVerts;
   uint mBytesPerVert;

   IDirect3DVertexBuffer9* mpVB;
   BYTE* mpVBData;

   BXboxMemoryPool& mCachedPool;
   BXboxMemoryPool& mPhysicalPool;

public:
   BXboxVBComposer(uint numVerts, uint bytesPerVert, BXboxMemoryPool& cachedPool, BXboxMemoryPool& physicalPool) :
      mNumVerts(numVerts), 
      mBytesPerVert(bytesPerVert), 
      mCachedPool(cachedPool), 
      mPhysicalPool(physicalPool)
   {
      mpVB = (IDirect3DVertexBuffer9*)mCachedPool.allocPtr(sizeof(IDirect3DVertexBuffer9), 16);

      mpVBData = mPhysicalPool.allocPtr(numVerts * bytesPerVert, 32);

      XGSetVertexBufferHeader(numVerts * bytesPerVert, 0, 0, mPhysicalPool.getOfs(mpVBData), mpVB);
      
      if (cLittleEndianNative)
         XGEndianSwapMemory(mpVB, mpVB, XGENDIAN_8IN32, sizeof(DWORD), sizeof(IDirect3DVertexBuffer9) / sizeof(DWORD));
   }

   uint getNumVerts(void) const { return mNumVerts; }
   uint getBytesPerVert(void) const { return mBytesPerVert; }

   IDirect3DVertexBuffer9* getVB(void) const { return mpVB; }
   BYTE* getVBData(void) const { return mpVBData; }
   
   uint getOfs(void) const { return mCachedPool.getOfs(getVB()); }

   BYTE* getVertPtr(uint i) { BDEBUG_ASSERT(i < mNumVerts); return getVBData() + i * mBytesPerVert; }

   void setVert(uint i, const void* pSrc) { memcpy(getVertPtr(i), pSrc, mBytesPerVert); }
};

//==============================================================================
// class BXboxTextureComposer
//==============================================================================
class BXboxTextureComposer
{
   uint mWidth;
   uint mHeight;

   D3DFORMAT mFmt;

   XGTEXTURE_DESC mTexDesc;

   IDirect3DTexture9* mpTex;
   BYTE* mpTexData;

   uint mAllocSize;

   BXboxMemoryPool& mCachedPool;
   BXboxMemoryPool& mPhysicalPool;

public:
   BXboxTextureComposer(uint width, uint height, D3DFORMAT fmt, BXboxMemoryPool& cachedPool, BXboxMemoryPool& physicalPool) :
      mWidth(width),
      mHeight(height),
      mFmt(fmt),
      mCachedPool(cachedPool),
      mPhysicalPool(physicalPool)
   {
      mpTex = (IDirect3DTexture9*)mCachedPool.allocPtr(sizeof(IDirect3DTexture9), 16);

      Utils::ClearObj(*mpTex);

      UINT D3DTexBaseSize;
      UINT D3DTexMipSize;               
      DWORD d3dTexTotalSize = XGSetTextureHeader( 
         mWidth, 
         mHeight, 
         1, 0, 
         mFmt, D3DPOOL_DEFAULT,
         0, 
         XGHEADER_CONTIGUOUS_MIP_OFFSET,
         0, 
         mpTex, 
         &D3DTexBaseSize, 
         &D3DTexMipSize);

      mAllocSize = d3dTexTotalSize;
      mpTexData = mPhysicalPool.allocPtr(d3dTexTotalSize, 4096);

      mpTex->Format.BaseAddress = mPhysicalPool.getOfs(mpTexData) >> 12;
      mpTex->Format.MipAddress = 0;

      XGGetTextureDesc(mpTex, 0, &mTexDesc);
      
      if (cLittleEndianNative)
         XGEndianSwapMemory(mpTex, mpTex, XGENDIAN_8IN32, sizeof(DWORD), sizeof(IDirect3DTexture9) / sizeof(DWORD));
   }
   
   void setD3DFormatSign(GPUSIGN sign)
   {
      if (cLittleEndianNative)
         XGEndianSwapMemory(mpTex, mpTex, XGENDIAN_8IN32, sizeof(DWORD), sizeof(IDirect3DTexture9) / sizeof(DWORD));
      
      mpTex->Format.SignX = sign;
      mpTex->Format.SignY = sign;
      mpTex->Format.SignZ = sign;
      
      if (cLittleEndianNative)
         XGEndianSwapMemory(mpTex, mpTex, XGENDIAN_8IN32, sizeof(DWORD), sizeof(IDirect3DTexture9) / sizeof(DWORD));
   }
   
   void setD3DFormatNumFormat(GPUNUMFORMAT numFmt)      
   {
      if (cLittleEndianNative)
         XGEndianSwapMemory(mpTex, mpTex, XGENDIAN_8IN32, sizeof(DWORD), sizeof(IDirect3DTexture9) / sizeof(DWORD));
         
      mpTex->Format.NumFormat = numFmt;

      if (cLittleEndianNative)
         XGEndianSwapMemory(mpTex, mpTex, XGENDIAN_8IN32, sizeof(DWORD), sizeof(IDirect3DTexture9) / sizeof(DWORD));      
   }      

   bool setTexel(uint x, uint y, const BRGBAColor32& c)
   {
      BDEBUG_ASSERT(x < mWidth && y < mHeight);

      BYTE* pBYTE;

      if (mTexDesc.Flags & XGTDESC_TILED)
         pBYTE = mpTexData + XGAddress2DTiledOffset(x, y, mTexDesc.RowPitch / mTexDesc.BytesPerBlock, mTexDesc.BytesPerBlock) * mTexDesc.BytesPerBlock;
      else
         pBYTE = mpTexData + x * mTexDesc.BytesPerBlock + y * mTexDesc.RowPitch;

      DWORD* pDWORD = (DWORD*)pBYTE;
      WORD* pWORD = (WORD*)pBYTE;

      switch (mFmt)
      {
         case D3DFMT_A2B10G10R10:
         case D3DFMT_LIN_A2B10G10R10:
         {
            *pDWORD = (c.a << 30) | (c.b << 20) | (c.g << 10) | c.r;
            if (cLittleEndianNative)
               EndianSwitchDWords(pDWORD, 1);
            break;
         }      
         case D3DFMT_G16R16:
         case D3DFMT_LIN_G16R16:
         {
            pWORD[0] = (WORD)c.r;
            pWORD[1] = (WORD)c.g;
            if (cLittleEndianNative)
               EndianSwitchDWords(pDWORD, 1);
            break;
         }
         case D3DFMT_A8B8G8R8:
         case D3DFMT_LIN_A8B8G8R8:
         {
            pBYTE[0] = (BYTE)c.r;
            pBYTE[1] = (BYTE)c.g;
            pBYTE[2] = (BYTE)c.b;
            pBYTE[3] = (BYTE)c.a;
            break;
         }      
         case D3DFMT_A8R8G8B8:   
         case D3DFMT_LIN_A8R8G8B8:   
         {
            pBYTE[0] = (BYTE)c.b;
            pBYTE[1] = (BYTE)c.g;
            pBYTE[2] = (BYTE)c.r;
            pBYTE[3] = (BYTE)c.a;
            break;
         }      
         default:
         {
            return false;
         }
      }

      return true;
   }

   bool getTexel(uint x, uint y, BRGBAColor32& c)
   {
      BDEBUG_ASSERT(x < mWidth && y < mHeight);

      const BYTE* pBYTE;

      if (mTexDesc.Flags & XGTDESC_TILED)
         pBYTE = mpTexData + XGAddress2DTiledOffset(x, y, mTexDesc.RowPitch / mTexDesc.BytesPerBlock, mTexDesc.BytesPerBlock) * mTexDesc.BytesPerBlock;
      else
         pBYTE = mpTexData + y * mTexDesc.RowPitch;

      const DWORD* pDWORD = (const DWORD*)pBYTE;
      const WORD* pWORD = (const WORD*)pBYTE;

      switch (mFmt)
      {
         case D3DFMT_A2B10G10R10:
         {
            DWORD d = *pDWORD;
            if (cLittleEndianNative)
               EndianSwitchDWords(&d, 1);

            c.a = (d >> 30) & 3;
            c.b = (d >> 20) & 1023;
            c.g = (d >> 10) & 1023;
            c.r = d & 1023;

            break;
         }      
         case D3DFMT_G16R16:
         {
            WORD w[2] = { pWORD[0], pWORD[1] };
            if (cLittleEndianNative)
               EndianSwitchDWords((DWORD*)w, 1);

            c.r = w[0];
            c.g = w[1];
            c.b = 0;
            c.a = 0;

            break;
         }
         case D3DFMT_A8B8G8R8:
         {
            c.set(pBYTE[0], pBYTE[1], pBYTE[2], pBYTE[3]);
            break;
         }      
         case D3DFMT_A8R8G8B8:
         {
            c.set(pBYTE[2], pBYTE[1], pBYTE[0], pBYTE[3]);
            break;
         }      
         default:
         {
            return false;
         }
      }

      return true;
   }

   uint getWidth(void) const { return mWidth; }
   uint getHeight(void) const { return mHeight; }
   D3DFORMAT getFormat(void) const { return mFmt; }

   uint getAllocSize(void) const { return mAllocSize; }

   const XGTEXTURE_DESC& getTexDesc(void) const { return mTexDesc; }

   const IDirect3DTexture9* getTex(void) const { return mpTex; }
   IDirect3DTexture9* getTex(void) { return mpTex; }
   
   uint getTexOfs(void) const { return mCachedPool.getOfs(mpTex); }
   
   uint getOfs(void) const { return mCachedPool.getOfs(getTex()); }

   const BYTE* getTexData(void) const { return mpTexData; }
   uint getTexDataOfs(void) const { return mPhysicalPool.getOfs(mpTexData); }
};

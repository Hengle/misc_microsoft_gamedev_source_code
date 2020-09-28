//============================================================================
//
//  xboxTextureHeap.cpp
//
//  Copyright (c) 2008, Ensemble Studios
//
//============================================================================
#include "xrender.h"
#include "xboxTextureHeap.h"
#include "memory\regionAllocator.h"
#include "renderDraw.h"

//#define XBOX_TEXTURE_HEAP_TRACES

BXboxTextureHeap* gpXboxTextureHeap;

static const uint cMaxRegionHeapAllocSize = 65536;

static void* const pPhysHeapStart      = (void*)0xA0000000;
static void* const pPhysHeapEnd        = (void*)0xC0000000;
static const uint cPhysHeapSize        = 512U * 1024U * 1024U;
static const uint cPhysHeapSizeInPages = cPhysHeapSize >> 12;

#define PHYS_MEMORY_CLEAR_BYTE 0x00
#define PHYS_MEMORY_CLEAR_DWORD 0x00000000

#define HEAP_TO_INTERCEPT gRenderHeap

class BScopedMemoryHeapLock
{
public:
   BScopedMemoryHeapLock() { HEAP_TO_INTERCEPT.claimLock(); }
   ~BScopedMemoryHeapLock() { HEAP_TO_INTERCEPT.releaseLock(); }
};

void* BXboxTextureHeapMemoryHeapInterceptor::New(int size, int* pActualSize, bool zero)
{
   if (!mpHeap)
   {
      if (pActualSize) *pActualSize = 0;
      return NULL;
   }

   uint allocSize;

   void* p = mpHeap->alloc(size, &allocSize);
   if (!p)
   {
      if (pActualSize) *pActualSize = 0;
      return NULL;
   }

   if (zero)
      XMemSet(p, 0, allocSize);

   if (pActualSize) *pActualSize = allocSize;

   return p;
}

bool BXboxTextureHeapMemoryHeapInterceptor::Delete(void* pData, int size)
{
   if (!mpHeap)
      return false;

   size;
   return mpHeap->free(pData);
}

bool BXboxTextureHeapMemoryHeapInterceptor::Details(void* pData, int* pSize)
{
   if (!mpHeap)
   {
      if (pSize) *pSize = 0;  
      return false;
   }

   uint size = mpHeap->getSize(pData);
   if (!size)
   {
      if (pSize) *pSize = 0;  
      return false;
   }

   if (pSize) *pSize = size;
   return true;
}

bool BXboxTextureHeapMemoryHeapInterceptor::verify(void) 
{ 
   if (!mpHeap)
      return true;

   return mpHeap->check();
}

BXboxTextureHeap::BXboxTextureHeap() :
   mRegionHeapMaxUnusedSize(0),
   mTotalPhysAllocations(0),
   mTotalPhysBytes(0),
   mUnusedRegionBufLargestUnused(0),
   mRegionPages(cPhysHeapSizeInPages),
   mInitialized(false)
{
   mMemoryHeapInterceptor.setXboxTextureHeap(this);
}

BXboxTextureHeap::~BXboxTextureHeap()
{  
   // purposely does nothing
}

void BXboxTextureHeap::init(bool enableHeapInterception, bool willBeAllocatingMipsOnly)
{
   BScopedCriticalSection lock(mValleyMutex);
 
   if (mInitialized)
      return;
         
   mInitialized = true;

   // rg FIXME: This data should be in an XML file!
   // Use the "xboxtextureheapstats" console command to find the ideal values.
#if defined(BUILD_DEBUG) && !defined(BUILD_CHECKED)
   reserveValleys(20, 64, 64, 0, 7, D3DFMT_DXT1, D3DRTYPE_TEXTURE);
   reserveValleys(50, 128, 128, 0, 8, D3DFMT_DXT1, D3DRTYPE_TEXTURE);
   reserveValleys(76, 256, 256, 0, 9, D3DFMT_DXT1, D3DRTYPE_TEXTURE);
   reserveValleys(32, 256, 256, 0, 9, D3DFMT_DXN, D3DRTYPE_TEXTURE);
#else
#if 0
   reserveValleys(70,   64,  64, 0,  7, D3DFMT_DXT1, D3DRTYPE_TEXTURE, willBeAllocatingMipsOnly);
   reserveValleys(9,    64,  64, 0,  7, D3DFMT_DXN, D3DRTYPE_TEXTURE, willBeAllocatingMipsOnly);
   reserveValleys(18,   64,  64, 0,  7, D3DFMT_DXT5, D3DRTYPE_TEXTURE, willBeAllocatingMipsOnly);

   reserveValleys(143, 128, 128, 0,  8, D3DFMT_DXT1, D3DRTYPE_TEXTURE, willBeAllocatingMipsOnly);
   reserveValleys(37,  128, 128, 0,  8, D3DFMT_DXN, D3DRTYPE_TEXTURE, willBeAllocatingMipsOnly);
   reserveValleys(35,  128, 128, 0,  8, D3DFMT_DXT5, D3DRTYPE_TEXTURE, willBeAllocatingMipsOnly);

   reserveValleys(103, 256, 256, 0,  9, D3DFMT_DXT1, D3DRTYPE_TEXTURE, willBeAllocatingMipsOnly);
   reserveValleys(16,  256, 256, 0,  9, D3DFMT_DXT5, D3DRTYPE_TEXTURE, willBeAllocatingMipsOnly);
   reserveValleys(46,  256, 256, 0,  9, D3DFMT_DXN, D3DRTYPE_TEXTURE, willBeAllocatingMipsOnly);   

   reserveValleys(24,  512, 512, 0,  9, D3DFMT_DXT1, D3DRTYPE_TEXTURE, willBeAllocatingMipsOnly);   
   reserveValleys(15,  512, 512, 0, 10, D3DFMT_DXN, D3DRTYPE_TEXTURE, willBeAllocatingMipsOnly);   
#endif

#if 0   
   reserveValleys(4, 32, 32, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(16, 512, 256, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(19, 256, 256, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(19, 512, 64, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(90, 512, 2, 0, 1, (D3DFORMAT)0x1A200186, D3DRTYPE_TEXTURE, false); // A8B8G8R8
   reserveValleys(93, 64, 64, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(221, 128, 128, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3

   reserveValleys(1, 256, 256, 0, 9, (D3DFORMAT)0x1A200154, D3DRTYPE_CUBETEXTURE, false); // DXT5

   reserveValleys(1, 64, 64, 8, 7, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 128, 128, 8, 8, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 256, 64, 8, 9, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 256, 128, 8, 9, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(6, 256, 256, 8, 9, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 512, 512, 8, 10, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 64, 32, 8, 7, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 64, 64, 8, 7, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 128, 32, 8, 8, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(3, 128, 128, 8, 8, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 128, 128, 9, 8, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 256, 128, 8, 9, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(5, 256, 256, 8, 9, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   
   reserveValleys(2, 32, 32, 0, 6, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(9, 64, 64, 0, 7, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(4, 64, 128, 0, 8, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(2, 128, 64, 0, 8, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(35, 128, 128, 0, 8, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(68, 256, 256, 0, 9, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(1, 1024, 256, 0, 11, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(1, 128, 32, 0, 8, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(1, 128, 256, 0, 9, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(1, 256, 128, 0, 9, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(32, 512, 512, 0, 10, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(3, 32, 32, 0, 6, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(36, 64, 64, 0, 7, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(6, 64, 128, 0, 8, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(3, 128, 32, 0, 8, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(1, 128, 64, 0, 8, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(86, 128, 128, 0, 8, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(1, 128, 256, 0, 9, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(2, 256, 128, 0, 9, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(107, 256, 256, 0, 9, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(1, 512, 256, 0, 10, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(50, 512, 512, 0, 10, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(3, 256, 256, 0, 9, (D3DFORMAT)0x1A200154, D3DRTYPE_CUBETEXTURE, true); // DXT5
#endif

#if 0
   reserveValleys(2, 16, 16, 0, 5, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(2, 128, 128, 0, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(13, 128, 128, 0, 8, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(3, 256, 128, 0, 9, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(3, 256, 256, 0, 9, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(4, 1024, 512, 0, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(2, 16, 16, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(66, 32, 32, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(62, 64, 64, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(520, 128, 128, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(35, 256, 256, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(6, 512, 64, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(9, 512, 256, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(5, 2048, 256, 0, 1, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, false); // DXT5
   reserveValleys(1, 16, 16, 0, 5, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, false); // DXN
   reserveValleys(3, 128, 128, 0, 8, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, false); // DXN
   reserveValleys(36, 256, 1, 0, 1, (D3DFORMAT)0x1A200186, D3DRTYPE_TEXTURE, false); // A8B8G8R8
   reserveValleys(7, 1, 1, 0, 1, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(1, 128, 64, 0, 1, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(3, 1, 1, 0, 1, (D3DFORMAT)0x1A207F54, D3DRTYPE_TEXTURE, false); // DXT5
   reserveValleys(1, 2, 2, 0, 2, (D3DFORMAT)0x1A207F54, D3DRTYPE_TEXTURE, false); // DXT5
   reserveValleys(3, 256, 256, 0, 9, (D3DFORMAT)0x1A207F54, D3DRTYPE_TEXTURE, false); // DXT5
   reserveValleys(2, 256, 256, 0, 9, (D3DFORMAT)0x1A200154, D3DRTYPE_CUBETEXTURE, false); // DXT5
   reserveValleys(1, 64, 64, 8, 7, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 64, 128, 8, 8, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 128, 32, 8, 8, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(2, 128, 128, 8, 8, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 256, 16, 8, 9, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 256, 64, 8, 9, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(2, 256, 128, 8, 9, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(7, 256, 256, 8, 9, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 512, 256, 8, 10, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(2, 512, 512, 8, 10, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 64, 32, 8, 7, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 64, 64, 8, 7, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 128, 32, 8, 8, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(5, 128, 128, 8, 8, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(0, 128, 128, 9, 8, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 256, 64, 8, 9, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 256, 128, 8, 9, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(6, 256, 256, 8, 9, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 512, 512, 8, 10, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(2, 256, 256, 10, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 512, 512, 10, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(2, 256, 64, 17, 1, (D3DFORMAT)0x1A200154, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 128, 128, 30, 1, (D3DFORMAT)0x1A200171, D3DRTYPE_ARRAYTEXTURE, false); // DXN
   reserveValleys(1, 128, 128, 60, 1, (D3DFORMAT)0x1A200171, D3DRTYPE_ARRAYTEXTURE, false); // DXN
   reserveValleys(1, 512, 512, 10, 1, (D3DFORMAT)0x1A200171, D3DRTYPE_ARRAYTEXTURE, false); // DXN
   reserveValleys(4, 32, 32, 0, 6, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(24, 64, 64, 0, 7, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(2, 64, 128, 0, 8, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(1, 128, 64, 0, 8, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(90, 128, 128, 0, 8, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(88, 256, 256, 0, 9, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(1, 1024, 256, 0, 11, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(1, 128, 32, 0, 8, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(1, 128, 256, 0, 9, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(1, 256, 64, 0, 9, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(1, 256, 128, 0, 9, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(19, 512, 512, 0, 10, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(4, 32, 32, 0, 6, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(73, 64, 64, 0, 7, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(1, 64, 128, 0, 8, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(3, 128, 32, 0, 8, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(0, 128, 64, 0, 8, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(198, 128, 128, 0, 8, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(2, 128, 256, 0, 9, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(2, 256, 64, 0, 9, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(2, 256, 128, 0, 9, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(143, 256, 256, 0, 9, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(1, 256, 512, 0, 10, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(0, 512, 256, 0, 10, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(70, 512, 512, 0, 10, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(1, 128, 128, 0, 8, (D3DFORMAT)0x1A200154, D3DRTYPE_CUBETEXTURE, true); // DXT5
   reserveValleys(2, 256, 256, 0, 9, (D3DFORMAT)0x1A200154, D3DRTYPE_CUBETEXTURE, true); // DXT5  
#endif

#if 0
   reserveValleys(2, 16, 16, 0, 5, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(1, 32, 32, 0, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(3, 32, 32, 0, 6, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(15, 64, 64, 0, 7, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(3, 128, 128, 0, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(18, 128, 128, 0, 8, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(0, 256, 128, 0, 9, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(6, 256, 256, 0, 9, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(5, 1024, 512, 0, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(9, 16, 16, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(105, 32, 32, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(51, 64, 64, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(547, 128, 128, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(20, 256, 256, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(0, 512, 64, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(27, 512, 256, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(0, 32, 8, 0, 6, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, false); // DXT5
   reserveValleys(0, 2048, 256, 0, 1, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, false); // DXT5
   reserveValleys(5, 16, 16, 0, 5, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, false); // DXN
   reserveValleys(2, 32, 32, 0, 6, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, false); // DXN
   reserveValleys(7, 64, 64, 0, 7, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, false); // DXN
   reserveValleys(5, 128, 128, 0, 8, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, false); // DXN
   reserveValleys(66, 256, 1, 0, 1, (D3DFORMAT)0x1A200186, D3DRTYPE_TEXTURE, false); // A8B8G8R8
   reserveValleys(6, 1, 1, 0, 1, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(0, 8, 8, 0, 4, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(0, 32, 8, 0, 6, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(1, 128, 64, 0, 1, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(3, 1, 1, 0, 1, (D3DFORMAT)0x1A207F54, D3DRTYPE_TEXTURE, false); // DXT5
   reserveValleys(1, 2, 2, 0, 2, (D3DFORMAT)0x1A207F54, D3DRTYPE_TEXTURE, false); // DXT5
   reserveValleys(7, 256, 256, 0, 9, (D3DFORMAT)0x1A207F54, D3DRTYPE_TEXTURE, false); // DXT5
   reserveValleys(0, 128, 128, 0, 8, (D3DFORMAT)0x1A200154, D3DRTYPE_CUBETEXTURE, false); // DXT5
   reserveValleys(2, 256, 256, 0, 1, (D3DFORMAT)0x1A200154, D3DRTYPE_CUBETEXTURE, false); // DXT5
   reserveValleys(0, 256, 256, 0, 9, (D3DFORMAT)0x1A200154, D3DRTYPE_CUBETEXTURE, false); // DXT5
   reserveValleys(1, 32, 32, 8, 6, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 32, 64, 8, 7, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 64, 64, 8, 7, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 64, 128, 8, 8, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 128, 32, 8, 8, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 128, 64, 8, 8, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(4, 128, 128, 8, 8, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 128, 256, 8, 9, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 256, 16, 8, 9, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 256, 32, 8, 9, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 256, 64, 8, 9, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(2, 256, 128, 8, 9, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(0, 256, 128, 15, 9, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(10, 256, 256, 8, 9, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 512, 16, 4, 10, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 512, 256, 4, 10, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(0, 512, 256, 8, 10, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(3, 512, 512, 4, 10, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(0, 512, 512, 8, 10, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(0, 512, 512, 15, 10, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 64, 32, 8, 7, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(2, 64, 64, 8, 7, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 128, 32, 8, 8, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 128, 64, 8, 8, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(6, 128, 128, 8, 8, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 128, 128, 9, 8, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 256, 32, 8, 9, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 256, 64, 8, 9, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 256, 128, 8, 9, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(8, 256, 256, 8, 9, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(0, 512, 512, 4, 10, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(0, 512, 512, 8, 10, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(0, 256, 256, 10, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(2, 256, 256, 11, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(0, 512, 512, 8, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(0, 512, 512, 10, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 512, 512, 11, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(0, 256, 64, 17, 1, (D3DFORMAT)0x1A200154, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(2, 256, 64, 18, 1, (D3DFORMAT)0x1A200154, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 256, 256, 11, 1, (D3DFORMAT)0x1A200154, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(0, 64, 64, 30, 1, (D3DFORMAT)0x1A200171, D3DRTYPE_ARRAYTEXTURE, false); // DXN
   reserveValleys(0, 128, 128, 30, 1, (D3DFORMAT)0x1A200171, D3DRTYPE_ARRAYTEXTURE, false); // DXN
   reserveValleys(0, 128, 128, 60, 1, (D3DFORMAT)0x1A200171, D3DRTYPE_ARRAYTEXTURE, false); // DXN
   reserveValleys(0, 512, 512, 8, 1, (D3DFORMAT)0x1A200171, D3DRTYPE_ARRAYTEXTURE, false); // DXN
   reserveValleys(0, 512, 512, 10, 1, (D3DFORMAT)0x1A200171, D3DRTYPE_ARRAYTEXTURE, false); // DXN
   reserveValleys(1, 512, 512, 11, 1, (D3DFORMAT)0x1A200171, D3DRTYPE_ARRAYTEXTURE, false); // DXN
   reserveValleys(6, 32, 32, 0, 6, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(47, 64, 64, 0, 7, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(4, 64, 128, 0, 8, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(0, 128, 64, 0, 8, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(83, 128, 128, 0, 8, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(0, 128, 512, 0, 10, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(59, 256, 256, 0, 9, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(0, 512, 128, 0, 10, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(1, 1024, 256, 0, 11, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(0, 32, 128, 0, 8, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(1, 64, 32, 0, 7, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(1, 128, 32, 0, 8, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(1, 128, 256, 0, 9, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(1, 256, 64, 0, 9, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(1, 256, 128, 0, 9, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(33, 512, 512, 0, 10, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(5, 32, 32, 0, 6, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(0, 32, 128, 0, 8, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(98, 64, 64, 0, 7, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(7, 64, 128, 0, 8, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(3, 128, 32, 0, 8, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(0, 128, 64, 0, 8, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(215, 128, 128, 0, 8, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(3, 128, 256, 0, 9, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(0, 128, 512, 0, 10, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(2, 256, 64, 0, 9, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(3, 256, 128, 0, 9, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(123, 256, 256, 0, 9, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(0, 256, 512, 0, 10, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(0, 512, 256, 0, 10, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(59, 512, 512, 0, 10, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(1, 64, 64, 0, 7, (D3DFORMAT)0x1A200154, D3DRTYPE_CUBETEXTURE, true); // DXT5
   reserveValleys(2, 128, 128, 0, 8, (D3DFORMAT)0x1A200154, D3DRTYPE_CUBETEXTURE, true); // DXT5
   reserveValleys(0, 256, 256, 0, 9, (D3DFORMAT)0x1A200154, D3DRTYPE_CUBETEXTURE, true); // DXT5
#endif 
   reserveValleys(2, 16, 16, 0, 5, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(0, 32, 32, 0, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(1, 32, 32, 0, 6, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(22, 64, 64, 0, 7, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(3, 128, 128, 0, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(26, 128, 128, 0, 8, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(0, 256, 128, 0, 9, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(8, 256, 256, 0, 9, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(0, 512, 128, 0, 10, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(0, 1024, 256, 0, 11, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(3, 1024, 512, 0, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(6, 16, 16, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(170, 32, 32, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(4, 64, 32, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(89, 64, 64, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(467, 128, 128, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(61, 256, 256, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(0, 512, 64, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(4, 512, 256, 0, 1, (D3DFORMAT)0x1A200153, D3DRTYPE_TEXTURE, false); // DXT3
   reserveValleys(1, 16, 64, 0, 7, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, false); // DXT5
   reserveValleys(0, 32, 8, 0, 6, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, false); // DXT5
   reserveValleys(0, 2048, 256, 0, 1, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, false); // DXT5
   reserveValleys(4, 16, 16, 0, 5, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, false); // DXN
   reserveValleys(1, 32, 32, 0, 6, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, false); // DXN
   reserveValleys(10, 64, 64, 0, 7, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, false); // DXN
   reserveValleys(9, 128, 128, 0, 8, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, false); // DXN
   reserveValleys(77, 256, 1, 0, 1, (D3DFORMAT)0x1A200186, D3DRTYPE_TEXTURE, false); // A8B8G8R8
   reserveValleys(4, 1, 1, 0, 1, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(0, 8, 8, 0, 4, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(0, 32, 8, 0, 6, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(2, 128, 64, 0, 1, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, false); // DXT1
   reserveValleys(3, 1, 1, 0, 1, (D3DFORMAT)0x1A207F54, D3DRTYPE_TEXTURE, false); // DXT5
   reserveValleys(1, 2, 2, 0, 2, (D3DFORMAT)0x1A207F54, D3DRTYPE_TEXTURE, false); // DXT5
   reserveValleys(7, 256, 256, 0, 9, (D3DFORMAT)0x1A207F54, D3DRTYPE_TEXTURE, false); // DXT5
   reserveValleys(1, 512, 512, 0, 10, (D3DFORMAT)0x1A207F54, D3DRTYPE_TEXTURE, false); // DXT5
   reserveValleys(1, 128, 128, 0, 8, (D3DFORMAT)0x1A200154, D3DRTYPE_CUBETEXTURE, false); // DXT5
   reserveValleys(0, 256, 256, 0, 1, (D3DFORMAT)0x1A200154, D3DRTYPE_CUBETEXTURE, false); // DXT5
   reserveValleys(2, 256, 256, 0, 9, (D3DFORMAT)0x1A200154, D3DRTYPE_CUBETEXTURE, false); // DXT5
   reserveValleys(1, 32, 32, 8, 6, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 32, 64, 8, 7, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 64, 16, 8, 7, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 64, 32, 8, 7, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(2, 64, 64, 8, 7, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 64, 128, 8, 8, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 128, 32, 8, 8, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 128, 64, 8, 8, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(5, 128, 128, 8, 8, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 128, 256, 8, 9, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 256, 16, 8, 9, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 256, 32, 8, 9, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(2, 256, 64, 8, 9, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(2, 256, 128, 8, 9, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(0, 256, 128, 15, 9, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(10, 256, 256, 8, 9, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 512, 16, 4, 10, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 512, 64, 4, 10, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 512, 128, 4, 10, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 512, 256, 4, 10, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(0, 512, 256, 8, 10, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(4, 512, 512, 4, 10, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(0, 512, 512, 8, 10, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(0, 512, 512, 15, 10, (D3DFORMAT)0x1A200112, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 64, 32, 8, 7, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(2, 64, 64, 8, 7, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 128, 32, 8, 8, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 128, 64, 8, 8, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(5, 128, 128, 8, 8, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 128, 128, 9, 8, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 256, 32, 8, 9, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 256, 64, 8, 9, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(1, 256, 128, 8, 9, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(7, 256, 256, 8, 9, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(0, 512, 512, 4, 10, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(0, 512, 512, 8, 10, (D3DFORMAT)0x1A200114, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(0, 256, 256, 9, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(0, 256, 256, 10, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(2, 256, 256, 11, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(0, 256, 256, 14, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(0, 512, 512, 8, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(0, 512, 512, 9, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(0, 512, 512, 10, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(1, 512, 512, 11, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(0, 512, 512, 14, 1, (D3DFORMAT)0x1A200152, D3DRTYPE_ARRAYTEXTURE, false); // DXT1
   reserveValleys(0, 256, 64, 17, 1, (D3DFORMAT)0x1A200154, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(0, 256, 64, 18, 1, (D3DFORMAT)0x1A200154, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(2, 256, 64, 20, 1, (D3DFORMAT)0x1A200154, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(0, 256, 256, 9, 1, (D3DFORMAT)0x1A200154, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(0, 256, 256, 11, 1, (D3DFORMAT)0x1A200154, D3DRTYPE_ARRAYTEXTURE, false); // DXT5
   reserveValleys(0, 64, 64, 30, 1, (D3DFORMAT)0x1A200171, D3DRTYPE_ARRAYTEXTURE, false); // DXN
   reserveValleys(0, 128, 128, 30, 1, (D3DFORMAT)0x1A200171, D3DRTYPE_ARRAYTEXTURE, false); // DXN
   reserveValleys(0, 128, 128, 60, 1, (D3DFORMAT)0x1A200171, D3DRTYPE_ARRAYTEXTURE, false); // DXN
   reserveValleys(0, 512, 512, 8, 1, (D3DFORMAT)0x1A200171, D3DRTYPE_ARRAYTEXTURE, false); // DXN
   reserveValleys(0, 512, 512, 9, 1, (D3DFORMAT)0x1A200171, D3DRTYPE_ARRAYTEXTURE, false); // DXN
   reserveValleys(0, 512, 512, 10, 1, (D3DFORMAT)0x1A200171, D3DRTYPE_ARRAYTEXTURE, false); // DXN
   reserveValleys(1, 512, 512, 11, 1, (D3DFORMAT)0x1A200171, D3DRTYPE_ARRAYTEXTURE, false); // DXN
   reserveValleys(0, 512, 512, 14, 1, (D3DFORMAT)0x1A200171, D3DRTYPE_ARRAYTEXTURE, false); // DXN
   reserveValleys(11, 32, 32, 0, 6, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(35, 64, 64, 0, 7, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(3, 64, 128, 0, 8, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(0, 128, 64, 0, 8, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(66, 128, 128, 0, 8, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(0, 128, 512, 0, 10, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(59, 256, 256, 0, 9, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(0, 512, 128, 0, 10, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(0, 1024, 256, 0, 11, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(0, 2048, 512, 0, 12, (D3DFORMAT)0x1A200154, D3DRTYPE_TEXTURE, true); // DXT5
   reserveValleys(0, 32, 128, 0, 8, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(0, 64, 32, 0, 7, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(1, 128, 32, 0, 8, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(0, 128, 256, 0, 9, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(1, 256, 64, 0, 9, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(0, 256, 128, 0, 9, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(0, 256, 512, 0, 10, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(24, 512, 512, 0, 10, (D3DFORMAT)0x1A200171, D3DRTYPE_TEXTURE, true); // DXN
   reserveValleys(27, 32, 32, 0, 6, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(0, 32, 128, 0, 8, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(1, 64, 32, 0, 7, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(120, 64, 64, 0, 7, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(5, 64, 128, 0, 8, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(4, 128, 32, 0, 8, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(0, 128, 64, 0, 8, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(198, 128, 128, 0, 8, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(1, 128, 256, 0, 9, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(0, 128, 512, 0, 10, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(2, 256, 64, 0, 9, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(1, 256, 128, 0, 9, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(121, 256, 256, 0, 9, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(0, 256, 512, 0, 10, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(0, 512, 256, 0, 10, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(43, 512, 512, 0, 10, (D3DFORMAT)0x1A207F52, D3DRTYPE_TEXTURE, true); // DXT1
   reserveValleys(0, 32, 32, 0, 6, (D3DFORMAT)0x1A200154, D3DRTYPE_CUBETEXTURE, true); // DXT5
   reserveValleys(1, 64, 64, 0, 7, (D3DFORMAT)0x1A200154, D3DRTYPE_CUBETEXTURE, true); // DXT5
   reserveValleys(2, 128, 128, 0, 8, (D3DFORMAT)0x1A200154, D3DRTYPE_CUBETEXTURE, true); // DXT5
   reserveValleys(1, 256, 256, 0, 9, (D3DFORMAT)0x1A200154, D3DRTYPE_CUBETEXTURE, true); // DXT5

#endif   
   
   if (!HEAP_TO_INTERCEPT.getInterceptor())
   {
      if (enableHeapInterception)
      {
         HEAP_TO_INTERCEPT.setInterceptor(&mMemoryHeapInterceptor);
         
         trace("BXboxTextureHeap::init: Heap interception ENABLED");
      }
      else
      {
         trace("BXboxTextureHeap::init: Heap interception DISABLED");
      }
   }         
}

void* BXboxTextureHeap::allocatePhysicalMemory(uint size)
{
   void* p = gPhysCachedHeap.AlignedNew(size, 4096);
   if (!p)
      return p;
   
   mTotalPhysAllocations++;
   mTotalPhysBytes += size;
   
   BDEBUG_ASSERT(p >= pPhysHeapStart);
   BDEBUG_ASSERT((static_cast<uchar*>(p) + size) <= pPhysHeapEnd);
   
   XMemSet(p, PHYS_MEMORY_CLEAR_BYTE, size);
   
   return p;
}

void BXboxTextureHeap::deletePhysicalMemory(void* p, uint size)
{
   if (!p)     
      return;

   BDEBUG_ASSERT((mTotalPhysAllocations > 0) && (mTotalPhysBytes >= size));
   
   mTotalPhysAllocations--;
   mTotalPhysBytes -= size;
   
   if (!mTotalPhysAllocations)
   {
      BDEBUG_ASSERT(!mTotalPhysBytes);
   }
   
   bool success = gPhysCachedHeap.Delete(p);
   BASSERT(success);
}

void BXboxTextureHeap::deinit()
{
   BScopedCriticalSection lock(mValleyMutex); 
   
   if (!mInitialized)
      return;
   
   for (BValleyHashMap::const_iterator it = mValleyHashMap.begin(); it != mValleyHashMap.end(); ++it)
   {
      const BValley& valley = it->second;
      
      deletePhysicalMemory(valley.mpData, valley.mSize);
   }
   
   BDEBUG_ASSERT(!mTotalPhysAllocations && !mTotalPhysBytes);
   
   mValleyHashMap.clear();

   mValleyClasses.clear();

   mRegionHeap.clear();

   mUnusedRegionBuf.clear();

   mRegionPages.clear();

   mUnusedRegionBufLargestUnused = 0;
   mTotalPhysAllocations = 0;
   mTotalPhysBytes = 0;
   mRegionHeapMaxUnusedSize = 0;

   mInitialized = false;
}

void BXboxTextureHeap::freeUnusedValleys(bool forcablyFreeReservedAllocations)
{
   BScopedCriticalSection lock(mValleyMutex);   
      
   if (!mInitialized)
      return;
   
   // EVIL, but necessary.
   HEAP_TO_INTERCEPT.claimLock();
   
   flushUnusedRegions();
   
   const uint initialPhysAllocations = mTotalPhysAllocations;
   const uint initialPhysBytes = mTotalPhysBytes;
      
   BDynamicCHeapArray<void*> valleysToErase;
   for (BValleyHashMap::const_iterator it = mValleyHashMap.begin(); it != mValleyHashMap.end(); ++it)
   {
      const BValley& valley = it->second;
      
      if (valley.mState != cVSAvailable) 
         continue;
      
      if (!forcablyFreeReservedAllocations)
      {
         if (valley.mIsReservedAllocation)
            continue;
      }
               
      BValleyClass& valleyClass = mValleyClasses[valley.mClassIndex];
      
      if (valley.mUnusedRegionsInUse)
      {
         const BDynamicCHeapArray<uint>& regionSizes = valleyClass.mRegions;
         
         BDynamicCHeapArray<BBFHeapAllocator::BRegion> regionsToFree;

         uint curOfs = 0;
         for (uint i = 0; i < regionSizes.getSize(); i++)
         {
            const uint regionSize = regionSizes[i] & BRegionAllocator::cSizeMask;
            if (regionSizes[i] & BRegionAllocator::cFreeFlag)
               regionsToFree.pushBack(BBFHeapAllocator::BRegion(static_cast<uchar*>(valley.mpData) + curOfs, regionSize));         

            curOfs += regionSize;
         }
                           
         bool success = mRegionHeap.removeFreeRegions(regionsToFree.getPtr(), regionsToFree.getSize());
                           
         if (!success)
            continue;
      }
      
      bool removeResult = valleyClass.mFreePtrs.removeValue(valley.mpData, false);
      removeResult;
      BDEBUG_ASSERT(removeResult);
                                             
      deletePhysicalMemory(valley.mpData, valley.mSize);
      
      const uint startPageIndex = ((uint)valley.mpData - (uint)pPhysHeapStart) >> 12;
      const uint endPageIndex = startPageIndex + (valley.mSize >> 12) - 1;
      for (uint pageIndex = startPageIndex; pageIndex <= endPageIndex; pageIndex++)
         mRegionPages.clearBit(pageIndex);
      
      valleysToErase.pushBack(valley.mpData);
   }
   
   HEAP_TO_INTERCEPT.releaseLock();
      
   for (uint i = 0; i < valleysToErase.getSize(); i++)
   {
      bool eraseResult = mValleyHashMap.erase(valleysToErase[i]);
      eraseResult;
      BDEBUG_ASSERT(eraseResult);
   }
   
   trace("BXboxTextureHeap::freeUnusedValleys: Freed %u allocations out of %u, %u bytes out of %u",
      initialPhysAllocations - mTotalPhysAllocations, initialPhysAllocations,
      initialPhysBytes - mTotalPhysBytes, initialPhysBytes);
}

bool BXboxTextureHeap::getTextureRegions(const IDirect3DBaseTexture9* pHeader, bool mipsOnly, BDynamicCHeapArray<uint>& regions, uint& allocSize, uint& unusedSize)
{
   enum { cMaxBaseRegions = 256, cMaxMipRegions = 256 };

   XGLAYOUT_REGION baseRegions[cMaxBaseRegions];
   UINT baseRegionCount = cMaxBaseRegions - 1;
   
   XGLAYOUT_REGION mipRegions[cMaxMipRegions];
   UINT mipRegionCount = cMaxMipRegions - 1;
      
   UINT baseData = 0;
   UINT baseSize = 0;
   UINT mipData = 0;
   UINT mipSize = 0;
   
   BRenderDraw::getTextureLayout((D3DBaseTexture*)pHeader, &baseData, &baseSize, baseRegions, &baseRegionCount, 16, &mipData, &mipSize, mipRegions, &mipRegionCount, 16);
   
   if ((mipsOnly) && (!mipRegionCount))
      return false;
   
   if (mipSize)
   {
      BDEBUG_ASSERT(Utils::IsValueAligned(baseSize, 4096));
   }
   
   baseSize = Utils::AlignUpValue(baseSize, 4096);
   mipSize = Utils::AlignUpValue(mipSize, 4096);
      
   allocSize = mipSize;
   
   if (!mipsOnly)
      allocSize += baseSize;
   else
      baseSize = 0;
      
   BDEBUG_ASSERT((allocSize & 4095) == 0);
      
   unusedSize = allocSize;
      
   BRegionAllocator::BReservedRegion reservedRegions[cMaxBaseRegions + cMaxMipRegions];
   uint numReservedRegions = 0;
   
   {
      XGLAYOUT_REGION* pFirstRegion = mipsOnly ? mipRegions : baseRegions;
      uint& regionCount = mipsOnly ? mipRegionCount : baseRegionCount;
      if (regionCount)
      {
         if (pFirstRegion[0].StartOffset)
         {
            if (pFirstRegion[0].StartOffset <= BBFHeapAllocator::cMinRegionSize)
               pFirstRegion[0].StartOffset = 0;
            else
            {
               memmove(&pFirstRegion[1], &pFirstRegion[0], regionCount * sizeof(XGLAYOUT_REGION));
               pFirstRegion[0].StartOffset = 0;
               pFirstRegion[0].EndOffset = BBFHeapAllocator::cMinRegionSize;      
               regionCount++;
            }         
         }
      }         
   }      
   
   if (!mipsOnly)
   {
      for (uint i = 0; i < baseRegionCount; i++)
      {
         if (i > 1)
         {
            BDEBUG_ASSERT(baseRegions[i - 1].EndOffset < baseRegions[i].StartOffset);
         }
      
         const uint size = baseRegions[i].EndOffset - baseRegions[i].StartOffset;
         unusedSize -= size;
         reservedRegions[numReservedRegions++].set(baseRegions[i].StartOffset, baseRegions[i].EndOffset);
      }
   }      
   
   for (uint i = 0; i < mipRegionCount; i++)
   {
      if (i > 1)
      {
         BDEBUG_ASSERT(mipRegions[i - 1].EndOffset < mipRegions[i].StartOffset);
      }
      
      const uint size = mipRegions[i].EndOffset - mipRegions[i].StartOffset;
      unusedSize -= size;
      reservedRegions[numReservedRegions++].set(mipRegions[i].StartOffset + baseSize, mipRegions[i].EndOffset + baseSize);
   }
   
   // This uses the primary heap!         
   BRegionAllocator regionAlloc;
   bool success = regionAlloc.init((void*)0x10000, allocSize, reservedRegions, numReservedRegions);
   BASSERT(success);
   
   regions.pushBack(regionAlloc.getRegions().getPtr(), regionAlloc.getRegions().getSize());
   
   return true;
}

uint BXboxTextureHeap::getOrCreateValleyClass(
   const IDirect3DBaseTexture9* pTex, 
   bool mipsOnly, 
   BDynamicCHeapArray<uint>& regions, 
   uint allocSize, 
   uint unusedSize)
{
   uint classIndex;

   for (classIndex = 0; classIndex < mValleyClasses.getSize(); classIndex++)
   {
      const BValleyClass& valleyClass = mValleyClasses[classIndex];

      if ((valleyClass.mAllocSize == allocSize) && 
          (valleyClass.mUnusedSize == unusedSize) && 
          (valleyClass.mRegions == valleyClass.mRegions) && 
          (valleyClass.mMipsOnly == mipsOnly))
         break;
   }

   if (classIndex == mValleyClasses.getSize())
   {
      BValleyClass& newClass = *mValleyClasses.enlarge(1);
      newClass.mAllocSize = allocSize;
      newClass.mUnusedSize = unusedSize;
      newClass.mMipsOnly = mipsOnly;
      newClass.mRegions.swap(regions);
      
      XGTEXTURE_DESC desc;
      XGGetTextureDesc((D3DBaseTexture*)pTex, 0, &desc);
      newClass.mWidth         = static_cast<uint16>(desc.Width);
      newClass.mHeight        = static_cast<uint16>(desc.Height);
      newClass.mLevels        = static_cast<uchar>(((D3DBaseTexture*)pTex)->GetLevelCount());
      newClass.mDepth         = static_cast<uint16>(desc.Depth);
      newClass.mResourceType  = static_cast<uchar>(desc.ResourceType);
      newClass.mFormat        = desc.Format;
      
      if (desc.ResourceType == D3DRTYPE_ARRAYTEXTURE)
         newClass.mDepth = static_cast<uint16>(((IDirect3DArrayTexture9*)pTex)->GetArraySize());
   }
   
   return classIndex;
}

bool BXboxTextureHeap::reserveValleys(uint num, uint width, uint height, uint depth, uint levels, D3DFORMAT fmt, D3DRESOURCETYPE textureType, bool mipsOnly)
{
   if ((mipsOnly) && (!levels))
      return false;
      
   BScopedCriticalSection lock(mValleyMutex);
   
   if (!mInitialized)
      return false;
      
   IDirect3DBaseTexture9 header;
   Utils::ClearObj(header);
   uint baseSize = 0;
   uint mipSize = 0;
   
   switch (textureType)
   {
      case D3DRTYPE_TEXTURE:
      {
         XGSetTextureHeader(width, height, levels, D3DUSAGE_CPU_CACHED_MEMORY, fmt, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, (D3DTexture*)&header, &baseSize, &mipSize);
         break;
      }
      case D3DRTYPE_VOLUMETEXTURE:
      {
         XGSetVolumeTextureHeader(width, height, depth, levels, D3DUSAGE_CPU_CACHED_MEMORY, fmt, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, (D3DVolumeTexture*)&header, &baseSize, &mipSize);
         break;
      }
      case D3DRTYPE_CUBETEXTURE:
      {
         BDEBUG_ASSERT(width == height);
         XGSetCubeTextureHeader(width, levels, D3DUSAGE_CPU_CACHED_MEMORY, fmt, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, (D3DCubeTexture*)&header, &baseSize, &mipSize);
         break;
      }
      case D3DRTYPE_ARRAYTEXTURE:
      {
         XGSetArrayTextureHeader(width, height, depth, levels, D3DUSAGE_CPU_CACHED_MEMORY, fmt, 0, 0, XGHEADER_CONTIGUOUS_MIP_OFFSET, 0, (D3DArrayTexture*)&header, &baseSize, &mipSize);
         break;
      }
      default:
      {
         BDEBUG_ASSERT(0);
         return false;
      }
   }
   
   BDynamicCHeapArray<uint> regions;
   uint allocSize, unusedSize;
   if (!getTextureRegions(&header, mipsOnly, regions, allocSize, unusedSize))
      return false;
         
   const uint classIndex = getOrCreateValleyClass(&header, mipsOnly, regions, allocSize, unusedSize);
               
   for (uint i = 0; i < num; i++)
      if (!createNewValley(classIndex, cVSAvailable, true, true, true))
         return false;
   
   return true;
}

void* BXboxTextureHeap::createNewValley(
   uint valleyClassIndex, 
   eValleyState initialState, 
   bool isReservedAllocation, 
   bool addUnusedRegionsToHeap,
   bool isLongTermAllocation)
{
   BValleyClass& valleyClass = mValleyClasses[valleyClassIndex];
   
   void* p = allocatePhysicalMemory(valleyClass.mAllocSize);
   if (!p)
      return NULL;

   BValleyHashMap::InsertResult insertRes(mValleyHashMap.insert(p, BValley()));
   BDEBUG_ASSERT(insertRes.second);

   BValley& valley = insertRes.first->second;

   BDEBUG_ASSERT(valleyClassIndex <= UINT16_MAX);
   valley.mClassIndex = static_cast<uint16>(valleyClassIndex);
   valley.mState = initialState;
   valley.mpData = p;
   valley.mSize = valleyClass.mAllocSize;
   valley.mUnusedRegionsInUse = false;
   valley.mAddUnusedRegionsToHeap = addUnusedRegionsToHeap;
   valley.mIsReservedAllocation = isReservedAllocation;
   valley.mIsLongTermAllocation = isLongTermAllocation;
   
   if (initialState == cVSAvailable)
      valleyClass.mFreePtrs.pushBack(p);
   else
      valleyClass.mUsedPtrs.pushBack(p);
   
   queueUnusedRegions(valley);   
      
   return p;      
}   

void BXboxTextureHeap::queueUnusedRegions(BValley& valley)
{
   if ((valley.mUnusedRegionsInUse) || (!valley.mAddUnusedRegionsToHeap))
      return;
      
   if ((!valley.mIsLongTermAllocation) && (!valley.mIsReservedAllocation))
      return;
   
   const BValleyClass& valleyClass = mValleyClasses[valley.mClassIndex];
   
   if (!valleyClass.mUnusedSize)
      return;
   
   uint largestUnusedSize = 0;
   
   const BDynamicCHeapArray<uint>& regionSizes = valleyClass.mRegions;
   
   uint curOfs = 0;
   for (uint i = 0; i < regionSizes.getSize(); i++)
   {
      const uint regionSize = regionSizes[i] & BRegionAllocator::cSizeMask;
      
      if (regionSizes[i] & BRegionAllocator::cFreeFlag)
      {
         largestUnusedSize = Math::Max(largestUnusedSize, regionSize);
                           
         BUnusedRegion unusedRegion(static_cast<uchar*>(valley.mpData) + curOfs, regionSize);

#ifdef XBOX_TEXTURE_HEAP_TRACES        
         trace("Enqueing region 0x%08X size %u", unusedRegion.mpStart, unusedRegion.mSize);
#endif
         
         mUnusedRegionBuf.pushBack(unusedRegion);
      }
      
      curOfs += regionSize;
   }
   
   mUnusedRegionBufLargestUnused = Math::Max(mRegionHeapMaxUnusedSize, largestUnusedSize);
   
   valley.mUnusedRegionsInUse = true;
}

void* BXboxTextureHeap::getValley(
   const IDirect3DBaseTexture9* pTex, 
   uint* pAllocSize, 
   bool addUnusedRegionsToHeap, 
   bool isLongTermAllocation,
   bool mipsOnly, 
   uint* pActualUsedSize)
{
   if (pAllocSize) *pAllocSize = 0;
   if (pActualUsedSize) *pActualUsedSize = 0;
   
   if (!pTex)
      return NULL;
   
   if (pTex->Common & D3DCOMMON_D3DCREATED)  
      return false;
      
   if ((pTex->Common & D3DCOMMON_CPU_CACHED_MEMORY) == 0)
      return false;
      
   if (mipsOnly)
   {
      if (!const_cast<IDirect3DBaseTexture9*>(pTex)->GetLevelCount())  
         return NULL;
   }
      
   BScopedCriticalSection lock(mValleyMutex);
   
   if (!mInitialized)
      return false;
   
   BDynamicCHeapArray<uint> regions;
   uint allocSize, unusedSize;
   if (!getTextureRegions(pTex, mipsOnly, regions, allocSize, unusedSize))
      return false;
      
   if (pAllocSize) *pAllocSize = allocSize;
   
   const uint classIndex = getOrCreateValleyClass(pTex, mipsOnly, regions, allocSize, unusedSize);

   BValleyClass& valleyClass = mValleyClasses[classIndex];      
   
   void* p = NULL;
   if (valleyClass.mFreePtrs.getSize())
   {
      for (int i = (int)valleyClass.mFreePtrs.getSize() - 1; i >= 0; i--)
      {
         void* q = valleyClass.mFreePtrs[i];
         
         BValleyHashMap::iterator it = mValleyHashMap.find(q);
         BDEBUG_ASSERT(it != mValleyHashMap.end());
         BDEBUG_ASSERT(it->first == q);

         BValley& valley = it->second;
         
         if (valley.mAddUnusedRegionsToHeap == addUnusedRegionsToHeap)
         {
            p = q;
                        
            valleyClass.mFreePtrs.removeIndex(i, false);
            
            break;
         }
      }
      
      if (p)
         valleyClass.mUsedPtrs.pushBack(p);
   }
   
   if (!p)
   {
      p = createNewValley(classIndex, cVSOccupied, false, addUnusedRegionsToHeap, isLongTermAllocation);
      
      if (!p)
         return NULL;
   }
         
   BValleyHashMap::iterator it = mValleyHashMap.find(p);
   BDEBUG_ASSERT(it != mValleyHashMap.end());
   BDEBUG_ASSERT(it->first == p);
   
   BValley& newValley = it->second;
   BDEBUG_ASSERT(newValley.mSize == allocSize);

   if (newValley.mState == cVSAvailable)
      clearValleyTextureData(newValley);
      
#ifdef XBOX_TEXTURE_HEAP_TRACES        
   if (newValley.mState == cVSAvailable)
      trace("Reusing physical memory 0x%08X size %u", newValley.mpData, newValley.mSize);
   else
      trace("Creating physical memory 0x%08X size %u", newValley.mpData, newValley.mSize);
#endif
      
   newValley.mState = cVSOccupied;
   
   queueUnusedRegions(newValley);  
   
   if (pActualUsedSize)
      *pActualUsedSize = allocSize - unusedSize;
         
   return p;
}

void BXboxTextureHeap::clearValleyTextureData(BValley& valley)
{
   const BValleyClass& valleyClass = mValleyClasses[valley.mClassIndex];
   
   uint curOfs = 0;
   for (uint i = 0; i < valleyClass.mRegions.getSize(); i++)
   {
      const uint regionBits = valleyClass.mRegions[i];
      const uint regionSize = regionBits & BRegionAllocator::cSizeMask;
      
      if ((regionBits & BRegionAllocator::cFreeFlag) == 0)
         XMemSet(static_cast<uchar*>(valley.mpData) + curOfs, 0, regionSize);
      
      curOfs += regionSize;
   }
}

#ifndef BUILD_FINAL   
bool BXboxTextureHeap::setValleyTextureDetails(void* pPhysicalAlloc, const char* pManager, const char* pName, uint arrayIndex)
{
   BDEBUG_ASSERT(pPhysicalAlloc && pManager && pName);
   BDEBUG_ASSERT(strlen(pManager));
   BDEBUG_ASSERT(strlen(pName));
   
   BValleyClass::BTextureDetails details;
   details.mManager.set(pManager);
   details.mName.set(pName);
   details.mPtr = pPhysicalAlloc;
   details.mArrayIndex = arrayIndex;
   
   BScopedCriticalSection lock(mValleyMutex);

   if (!mInitialized)
      return false;

   BValleyHashMap::iterator it = mValleyHashMap.find(pPhysicalAlloc);
   if (it == mValleyHashMap.end())
      return false;

   BDEBUG_ASSERT(it->first == pPhysicalAlloc);

   BValley& valley = it->second;

   BDEBUG_ASSERT(valley.mpData == pPhysicalAlloc);

   if (valley.mState == cVSAvailable)
      return false;

   BValleyClass& valleyClass = mValleyClasses[valley.mClassIndex];
   
   valleyClass.addTextureDetails(details);
   
   return true;
}
#endif   

bool BXboxTextureHeap::releaseValley(const IDirect3DBaseTexture9* pTex, bool mipsOnly)
{
   if (!pTex)
      return false;

   if (pTex->Common & D3DCOMMON_D3DCREATED)  
      return false;

   if ((pTex->Common & D3DCOMMON_CPU_CACHED_MEMORY) == 0)
      return false;

   void* pPhysicalAlloc;
   
   if (mipsOnly)
      pPhysicalAlloc = reinterpret_cast<void*>(reinterpret_cast<const D3DBaseTexture*>(pTex)->Format.MipAddress << GPU_TEXTURE_ADDRESS_SHIFT);
   else
      pPhysicalAlloc = reinterpret_cast<void*>(reinterpret_cast<const D3DBaseTexture*>(pTex)->Format.BaseAddress << GPU_TEXTURE_ADDRESS_SHIFT);
      
   if (pPhysicalAlloc < pPhysHeapStart)
      return false;
      
   return releaseValley(pPhysicalAlloc);
}   

bool BXboxTextureHeap::releaseValley(void* pPhysicalAlloc)
{
   BScopedCriticalSection lock(mValleyMutex);
   
   if (!mInitialized)
      return false;

   BValleyHashMap::iterator it = mValleyHashMap.find(pPhysicalAlloc);
   if (it == mValleyHashMap.end())
      return false;

   BDEBUG_ASSERT(it->first == pPhysicalAlloc);

   BValley& valley = it->second;
   
   BDEBUG_ASSERT(valley.mpData == pPhysicalAlloc);
      
   if (valley.mState == cVSAvailable)
      return false;
   
   BValleyClass& valleyClass = mValleyClasses[valley.mClassIndex];

#ifndef BUILD_FINAL   
   valleyClass.removeTextureDetails(pPhysicalAlloc);
#endif   
   
   bool foundPtr = valleyClass.mUsedPtrs.removeValue(pPhysicalAlloc, false);
   foundPtr;
   BDEBUG_ASSERT(foundPtr);
   
   if ((!valley.mUnusedRegionsInUse) && (!valley.mIsReservedAllocation))
   {
#ifdef XBOX_TEXTURE_HEAP_TRACES        
      trace("Deleting physical memory 0x%08X size %u", valley.mpData, valley.mSize);
#endif
      
      deletePhysicalMemory(valley.mpData, valley.mSize);
      
      mValleyHashMap.erase(pPhysicalAlloc);
   }
   else
   {
#ifdef XBOX_TEXTURE_HEAP_TRACES           
      trace("Make available physical memory 0x%08X size %u", valley.mpData, valley.mSize);
#endif
   
      valley.mState = cVSAvailable;
      valleyClass.mFreePtrs.pushBack(pPhysicalAlloc);
   }
      
   return true;
}

void BXboxTextureHeap::flushUnusedRegions()
{
   BScopedCriticalSection lock(mValleyMutex);
   
   if (!mInitialized)
      return;
   
   for (uint i = 0; i < mUnusedRegionBuf.getSize(); i++)
   {
      const BUnusedRegion& region = mUnusedRegionBuf[i];
      
#ifdef BUILD_DEBUG
      {
         const uint regionSize = region.mSize >> 2;
         DWORD* pRegionData = static_cast<DWORD*>(region.mpStart);
         for (uint i = 0; i < regionSize; i++)
         {
            BDEBUG_ASSERT(pRegionData[i] == PHYS_MEMORY_CLEAR_DWORD);
            pRegionData[i] = 0xF0F0F0F0;
         }
      }
#endif     

#ifdef XBOX_TEXTURE_HEAP_TRACES        
      trace("Adding region 0x%08X size %u", region.mpStart, region.mSize);
#endif
      
      bool success = mRegionHeap.addRegion(region.mpStart, region.mSize);
      BASSERT(success);
      
      mRegionHeapMaxUnusedSize = Math::Max(mRegionHeapMaxUnusedSize, region.mSize);
   }
   
   mUnusedRegionBuf.resize(0);
   
   mUnusedRegionBufLargestUnused = 0;
}

void* BXboxTextureHeap::alloc(uint size, uint* pActualSize)
{
   if (!mInitialized)
   {
      if (pActualSize) *pActualSize = 0;
      return NULL;
   }
      
   if (size < sizeof(uint))
      size = sizeof(uint);

   size = Utils::AlignUpValue(size, sizeof(uint));
      
   if (size > cMaxRegionHeapAllocSize)
   {
      if (pActualSize) *pActualSize = 0;
      return NULL;
   }
            
   uint actualSize;         
   void* p = mRegionHeap.alignedAlloc(size, &actualSize);
   if (!p)
   {
      if (static_cast<uint>(mUnusedRegionBufLargestUnused) >= size)
      {
         flushUnusedRegions();
         
         p = mRegionHeap.alignedAlloc(size, &actualSize);
      }
   }
   
   if (!p)
   {
      if (pActualSize) *pActualSize = 0;
      return NULL;
   }
         
   const uint pageIndex = ((uint)p - (uint)pPhysHeapStart) >> 12;
   mRegionPages.setBit(pageIndex);
   
   if (pActualSize) *pActualSize = actualSize;
   
   return p;
}

bool BXboxTextureHeap::free(void* p)
{
   if (!mInitialized)
      return false;
      
   if (p < pPhysHeapStart)
      return false;
   
   const uint pageIndex = ((uint)p - (uint)pPhysHeapStart) >> 12;
   if (pageIndex >= cPhysHeapSizeInPages)
      return false;
      
   if (!mRegionPages.isBitSet(pageIndex))
      return false;
   
   bool success = mRegionHeap.alignedFree(p);
   BASSERT(success);
   
   return success;
}

uint BXboxTextureHeap::getSize(void* p)
{
   if (!mInitialized)
      return 0;
      
   if (p < pPhysHeapStart)
      return 0;

   const uint pageIndex = ((uint)p - (uint)pPhysHeapStart) >> 12;
   if (pageIndex >= cPhysHeapSizeInPages)
      return 0;

   if (!mRegionPages.isBitSet(pageIndex))
      return 0;

   return mRegionHeap.alignedGetSize(p);
}

#ifndef BUILD_FINAL
   #define CHECK(x) do { if (!(x)) { BASSERTM(0, #x); return false; } } while(0)
#else
   #define CHECK(x) do { if (!(x)) return false; } while(0)
#endif

bool BXboxTextureHeap::check() const
{
   {
      BScopedMemoryHeapLock heapLock;

      if (!mRegionHeap.check())
         return false;
   }
   
   BScopedCriticalSection lock(mValleyMutex);
   
   if (!mInitialized)
      return false;
         
   CHECK(mTotalPhysAllocations == (uint)mValleyHashMap.getSize());
   uint totalPhysAllocated = 0;
   
   for (BValleyHashMap::const_iterator it = mValleyHashMap.begin(); it != mValleyHashMap.end(); ++it)
   {
      const BValley& valley = it->second;
      CHECK(valley.mpData == it->first);
      
      CHECK(valley.mClassIndex < mValleyClasses.getSize());
      
      const BValleyClass& valleyClass = mValleyClasses[valley.mClassIndex];

      CHECK((valley.mState >= cVSAvailable) && (valley.mState <= cVSOccupied));
      
      if (valley.mState == cVSAvailable)
         CHECK(valleyClass.mFreePtrs.find(valley.mpData) >= 0);
      else
         CHECK(valleyClass.mUsedPtrs.find(valley.mpData) >= 0);
      
      CHECK(valley.mSize);
      CHECK((valley.mSize & 4095) == 0);   
      CHECK(Utils::IsAligned(valley.mpData, 4096));
      CHECK(valley.mpData >= pPhysHeapStart);
      CHECK(((uchar*)valley.mpData + valley.mSize) <= pPhysHeapEnd);
      
      int actualSize = 0;
      bool detailSuccess = gPhysCachedHeap.Details(valley.mpData, &actualSize);
      CHECK(detailSuccess);
    
      CHECK((uint)actualSize >= valley.mSize);
      
      totalPhysAllocated += valley.mSize;
   }  
   
   CHECK(totalPhysAllocated == mTotalPhysBytes);    
   
   return true;
}

#undef CHECK

void BXboxTextureHeap::getHeapStats(BHeapStats& stats) const
{
   stats.clear();
   if (!mInitialized)
      return;
      
   BScopedMemoryHeapLock heapLock;
   
   mRegionHeap.getAllocInfo(stats.mTotalRegionBytes, stats.mTotalNumAllocations, stats.mTotalBytesAllocated, stats.mTotalBytesFree, stats.mTotalOperations);
   mRegionHeap.getMemoryInfo(stats.mTotalFreeChunks, stats.mLargestFreeChunk, stats.mTotalBins, stats.mTotalOverhead, stats.mDVSize);
}

void BXboxTextureHeap::getValleyStats(BValleyStats& stats) const
{
   stats.clear();
   
   BScopedCriticalSection lock(mValleyMutex);
   
   if (!mInitialized)
      return;

   stats.mNumValleys = mValleyHashMap.getSize();
   stats.mNumValleyClasses = mValleyClasses.getSize();
   stats.mTotalAllocatedBytes = mTotalPhysBytes;
   stats.mLargestRegionSize = mRegionHeapMaxUnusedSize;
   
   for (BValleyHashMap::const_iterator it = mValleyHashMap.begin(); it != mValleyHashMap.end(); ++it)
   {
      const BValley& valley = it->second;
      const BValleyClass& valleyClass = mValleyClasses[valley.mClassIndex];
      
      switch (valley.mState)
      {
         case cVSAvailable:   stats.mNumAvailable++; stats.mTotalAvailableBytes += valley.mSize; break;
         case cVSOccupied:    stats.mNumOccupied++; stats.mTotalOccupiedBytes += valley.mSize; break;
         default: BASSERT(0); return;
      }
                  
      stats.mTotalRegionBytes += valleyClass.mUnusedSize;
      
      if (valley.mIsReservedAllocation)
         stats.mTotalReserved++;
      if (valley.mIsLongTermAllocation)
         stats.mTotalLongTerm++;
      if (valley.mUnusedRegionsInUse)
         stats.mTotalUsingUnusedRegions++;
      if (valley.mAddUnusedRegionsToHeap)
         stats.mTotalOKToUseUnusedRegions++;
   }
}

#ifndef BUILD_FINAL
//==============================================================================
// getTexFormatString
//==============================================================================
static const char* getTexFormatString(D3DFORMAT fmt)
{
   switch (MAKENOSRGBFMT(MAKELEFMT(fmt)))
   {
      case MAKELEFMT(D3DFMT_A8):                return "A8";
      case MAKELEFMT(D3DFMT_A8R8G8B8):          return "A8R8G8B8";
      case MAKELEFMT(D3DFMT_A8B8G8R8):          return "A8B8G8R8";
      case MAKELEFMT(D3DFMT_R5G6B5):            return "R5G6B5";
      case MAKELEFMT(D3DFMT_A16B16G16R16F):     return "A16B16G16R16F";
      case MAKELEFMT(D3DFMT_DXT1):              return "DXT1";
      case MAKELEFMT(D3DFMT_DXT3):              return "DXT3";
      case MAKELEFMT(D3DFMT_DXT5):              return "DXT5";
      case MAKELEFMT(D3DFMT_DXN):               return "DXN";
      case MAKELEFMT(D3DFMT_LIN_A8):            return "LIN_A8";
      case MAKELEFMT(D3DFMT_LIN_A8R8G8B8):      return "LIN_A8R8G8B8";
      case MAKELEFMT(D3DFMT_LIN_R5G6B5):        return "LIN_R5G6B5";
      case MAKELEFMT(D3DFMT_LIN_A16B16G16R16F): return "LIN_A16B16G16R16F";
      case MAKELEFMT(D3DFMT_LIN_DXT1):          return "LIN_DXT1";
      case MAKELEFMT(D3DFMT_LIN_DXT3):          return "LIN_DXT3";
      case MAKELEFMT(D3DFMT_LIN_DXT5):          return "LIN_DXT5";
      case MAKELEFMT(D3DFMT_LIN_DXN):           return "LIN_DXN";
      default:                                  return "?";
   }
}

struct BManagerStats
{
   BRenderString mName;
   BDynamicArray<void*> mPtrs;
   uint mTotalTextures;
   uint mTotalBytes;
};

void BXboxTextureHeap::dumpValleyInfo(BStream* pStream)
{
   if (!mInitialized)
      return;
      
   BDynamicCHeapArray<BValleyClass>  sortedValleyClasses;
   
   {
      BScopedCriticalSection lock(mValleyMutex);
      
      sortedValleyClasses = mValleyClasses;
      
      sortedValleyClasses.sort();
   }   
   
   pStream->printf("         NumValleys: %u\n", mValleyHashMap.getSize());
   pStream->printf("   NumValleyClasses: %u\n", sortedValleyClasses.getSize());
   pStream->printf("TotalAllocatedBytes: %u\n", mTotalPhysBytes);
   pStream->printf("  LargestRegionSize: %u\n", mRegionHeapMaxUnusedSize);

   BDynamicArray<BManagerStats> managerStats;
   
   for (uint classIndex = 0; classIndex < sortedValleyClasses.getSize(); classIndex++)
   {
      const BValleyClass& valleyClass = sortedValleyClasses[classIndex];
      
      pStream->printf("------------- ValleyClass Index: %u\n", classIndex);
      const char* p = "?";
      switch (valleyClass.mResourceType)
      {
         case D3DRTYPE_TEXTURE:        p = "Texture"; break;
         case D3DRTYPE_VOLUME:         p = "Volume"; break;
         case D3DRTYPE_VOLUMETEXTURE:  p = "VolumeTexture"; break;
         case D3DRTYPE_CUBETEXTURE:    p = "CubeTexture"; break;
         case D3DRTYPE_ARRAYTEXTURE:   p = "ArrayTexture"; break;     
         case D3DRTYPE_LINETEXTURE:    p = "LineTexture"; break;
      }
      pStream->printf("ResourceType: %s\n", p);
      pStream->printf("   Allocated: %u bytes\n", (valleyClass.mUsedPtrs.getSize() + valleyClass.mFreePtrs.getSize()) * valleyClass.mAllocSize);
      pStream->printf("   AllocSize: %u\n", valleyClass.mAllocSize);
      pStream->printf("  UnusedSize: %u\n", valleyClass.mUnusedSize);
      pStream->printf("  NumRegions: %u\n", valleyClass.mRegions.getSize());
      pStream->printf("    MipsOnly: %u\n", valleyClass.mMipsOnly);
      pStream->printf("    NumAvail: %u (%u bytes)\n", valleyClass.mFreePtrs.getSize(), valleyClass.mFreePtrs.getSize() * valleyClass.mAllocSize);
      pStream->printf("     NumUsed: %u (%u bytes)\n", valleyClass.mUsedPtrs.getSize(), valleyClass.mUsedPtrs.getSize() * valleyClass.mAllocSize);
      pStream->printf("  Dimensions: %ux%ux%u\n", valleyClass.mWidth, valleyClass.mHeight, valleyClass.mDepth);
      pStream->printf("   MipLevels: %u\n", valleyClass.mLevels);
      pStream->printf("      Format: %s Tiled: %u SRGB: %u Endian: %u\n", 
         getTexFormatString(valleyClass.mFormat),
         (valleyClass.mFormat & D3DFORMAT_TILED_MASK) != 0,
         (valleyClass.mFormat & (D3DFORMAT_SIGNX_MASK | D3DFORMAT_SIGNY_MASK | D3DFORMAT_SIGNZ_MASK)) != 0,
         (valleyClass.mFormat & D3DFORMAT_ENDIAN_MASK) >> D3DFORMAT_ENDIAN_SHIFT);

#ifndef BUILD_FINAL
      pStream->printf("NumDetails: %i\n", valleyClass.mTextureDetails.getSize());
      for (uint i = 0; i < valleyClass.mTextureDetails.getSize(); i++)
      {
         pStream->printf("  Manager: %s, Name: %s, PhysPtr: 0x%08X, ArrayIndex: %u\n",
            valleyClass.mTextureDetails[i].mManager.getPtr(),
            valleyClass.mTextureDetails[i].mName.getPtr(),
            valleyClass.mTextureDetails[i].mPtr,
            valleyClass.mTextureDetails[i].mArrayIndex);
            
         uint j;
         for (j = 0; j < managerStats.getSize(); j++)
         {
            if (managerStats[j].mName == valleyClass.mTextureDetails[i].mManager)
            {
               if (managerStats[j].mPtrs.find(valleyClass.mTextureDetails[i].mPtr) == cInvalidIndex)
               {
                  managerStats[j].mPtrs.pushBack(valleyClass.mTextureDetails[i].mPtr);
                  managerStats[j].mTotalTextures++;
                  managerStats[j].mTotalBytes += valleyClass.mAllocSize; 
               }
               break;
            }
         }
         
         if (j == managerStats.getSize())
         {
            BManagerStats* p = managerStats.enlarge(1);
            p->mName = valleyClass.mTextureDetails[i].mManager;
            p->mTotalTextures = 1;
            p->mTotalBytes = valleyClass.mAllocSize;
            p->mPtrs.pushBack(valleyClass.mTextureDetails[i].mPtr);
         }
      }
#endif
   }
   
#ifndef BUILD_FINAL   
   pStream->printf("Overall manager stats:\n");
   for (uint i = 0; i < managerStats.getSize(); i++)
   {
      pStream->printf("  Name: %s, TotalTextures: %u, TotalBytes: %u\n", managerStats[i].mName.getPtr(), managerStats[i].mTotalTextures, managerStats[i].mTotalBytes);
   }   
#endif   

   for (uint classIndex = 0; classIndex < sortedValleyClasses.getSize(); classIndex++)   
   {
      const BValleyClass& valleyClass = sortedValleyClasses[classIndex];      

      const char* p = "?";
      switch (valleyClass.mResourceType)
      {
         case D3DRTYPE_TEXTURE:        p = "D3DRTYPE_TEXTURE"; break;
         case D3DRTYPE_VOLUME:         p = "D3DRTYPE_VOLUME"; break;
         case D3DRTYPE_VOLUMETEXTURE:  p = "D3DRTYPE_VOLUMETEXTURE"; break;
         case D3DRTYPE_CUBETEXTURE:    p = "D3DRTYPE_CUBETEXTURE"; break;
         case D3DRTYPE_ARRAYTEXTURE:   p = "D3DRTYPE_ARRAYTEXTURE"; break;     
         case D3DRTYPE_LINETEXTURE:    p = "D3DRTYPE_LINETEXTURE"; break;
      }
      
      pStream->printf("reserveValleys(%u, %u, %u, %u, %u, (D3DFORMAT)0x%08X, %s, %s); // %s\n",
         valleyClass.mUsedPtrs.getSize(),
         valleyClass.mWidth,
         valleyClass.mHeight,
         ((valleyClass.mResourceType == D3DRTYPE_ARRAYTEXTURE) || (valleyClass.mResourceType == D3DRTYPE_VOLUME) || (valleyClass.mResourceType == D3DRTYPE_VOLUMETEXTURE)) ? valleyClass.mDepth : 0,
         valleyClass.mLevels,
         valleyClass.mFormat,
         p,
         valleyClass.mMipsOnly ? "true" : "false",
         getTexFormatString(valleyClass.mFormat));
   }
}
#endif

#ifndef BUILD_FINAL
void BXboxTextureHeap::BValleyClass::addTextureDetails(const BTextureDetails& details)
{
   mTextureDetails.pushBack(details);
}

bool BXboxTextureHeap::BValleyClass::removeTextureDetails(void* pPhysicalAlloc)
{
   bool found = false;
   
   for (uint i = 0; i < mTextureDetails.getSize(); i++)
   {
      if (mTextureDetails[i].mPtr == pPhysicalAlloc)
      {
         mTextureDetails.removeIndex(i, false);
         found = true;
      }
   }
   
   return false;
}
#endif

bool BXboxTextureHeap::BValleyClass::operator< (const BValleyClass& rhs) const
{
   if (mMipsOnly < rhs.mMipsOnly)
      return true;
   else if (mMipsOnly == rhs.mMipsOnly)
   {
      if (mResourceType < rhs.mResourceType)
         return true;
      else if (mResourceType == rhs.mResourceType)
      {
         if (mFormat < rhs.mFormat)
            return true;
         else if (mFormat == rhs.mFormat)
         {
            if (mWidth < rhs.mWidth)
               return true;
            else if (mWidth == rhs.mWidth)
            {
               if (mHeight < rhs.mHeight)
                  return true;
               else if (mHeight == rhs.mHeight)
               {
                  if (mDepth < rhs.mDepth)
                     return true;
                  else if (mDepth == rhs.mDepth)
                  {
                     if (mLevels < rhs.mLevels)
                        return true;
                  }
               }
            }
         }
      }
   }
   return false;
}

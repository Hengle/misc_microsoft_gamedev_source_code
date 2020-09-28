// File: mallocWrappers.cpp
#include "xcore.h"
#include "mallocWrappers.h"

#ifdef XBOX
//============================================================================
// alignedMalloc
//============================================================================
void* alignedMalloc(uint size, uint alignment)
{
   if (size < sizeof(DWORD)) 
      size = sizeof(DWORD);

   if (!alignment)
   {
      alignment = sizeof(DWORD);
      if ((size & 15) == 0)
         alignment = 16;
      else if ((size & 7) == 0)
         alignment = 8;
   }         
   else if (alignment < sizeof(DWORD))
      alignment = sizeof(DWORD);

   BDEBUG_ASSERT(alignment <= 16);            

   size = Utils::AlignUpValue(size, alignment);

   void* p = malloc(size);

   BDEBUG_ASSERT(Utils::IsAligned(p, alignment));

   return p;        
}

//============================================================================
// alignedFree
//============================================================================
void alignedFree(void* p)
{
   free(p);
}

//============================================================================
// alignedRealloc
//============================================================================
void* alignedRealloc(void* p, uint newSize, uint alignment, bool move)
{
   if (!p)
      return alignedMalloc(newSize, alignment);

   if (!newSize)
   {
      alignedFree(p);
      return NULL;  
   }      

   if (newSize < sizeof(DWORD)) 
      newSize = sizeof(DWORD);

   if (!alignment)
   {
      alignment = sizeof(DWORD);
      if ((newSize & 15) == 0)
         alignment = 16;
      else if ((newSize & 7) == 0)
         alignment = 8;
   }         
   else if (alignment < sizeof(DWORD))
      alignment = sizeof(DWORD);

   BDEBUG_ASSERT(alignment <= 16);      

   newSize = Utils::AlignUpValue(newSize, alignment);

   void* q;
   if (move)
      q = realloc(p, newSize);
   else
      q = _expand(p, newSize);   

   BDEBUG_ASSERT(Utils::IsAligned(q, alignment));

   return q;
}

//============================================================================
// alignedExpand
//============================================================================
void* alignedExpand(void* p, uint newSize, uint alignment)
{
   return alignedRealloc(p, newSize, alignment, false);
}

//============================================================================
// alignedMSize
//============================================================================
uint alignedMSize(void* p)
{
   return p ? _msize(p) : 0;
}

#else // !XBOX

//============================================================================
// getRawPtr
//============================================================================
static void* getRawPtr(void* p)
{
   if (!p)
      return NULL;

   BDEBUG_ASSERT(Utils::IsAligned(p, 3));      

   void* pRawPtr = static_cast<void**>(p)[-1];
   BDEBUG_ASSERT((pRawPtr < p) && Utils::IsAligned(pRawPtr, 3));

   BDEBUG_ASSERT(((uchar*)p - (uchar*)pRawPtr) <= 16);

   return pRawPtr;   
}

//============================================================================
// alignedMalloc
//============================================================================
void* alignedMalloc(uint size, uint alignment)
{
   if (size < sizeof(void*)) 
      size = sizeof(void*);

   if (!alignment)
   {
      alignment = sizeof(void*);
      if ((size & 15) == 0)
         alignment = 16;
      else if ((size & 7) == 0)
         alignment = 8;
   }         
   else 
   {  
      if (alignment < sizeof(void*))
         alignment = sizeof(void*);
   }

   BDEBUG_ASSERT(alignment <= 16);

   const uint rawSize = size + alignment;

   void* pRaw = malloc(rawSize);
   if (!pRaw)
      return NULL;

   void* pAligned = Utils::AlignDown(static_cast<uchar*>(pRaw) + alignment, alignment);

   BDEBUG_ASSERT( (((uchar*)pAligned + size) - (uchar*)pRaw) <= (int)rawSize );
   BDEBUG_ASSERT( ((uchar*)pAligned - (uchar*)pRaw) >= sizeof(void*) );

   static_cast<void**>(pAligned)[-1] = pRaw;

   BDEBUG_ASSERT(getRawPtr(pAligned) == pRaw);

   return pAligned;
}

//============================================================================
// alignedFree
//============================================================================
void alignedFree(void* p)
{
   if (!p)
      return;

   void* pRawPtr = getRawPtr(p);
   free(pRawPtr);
}

//============================================================================
// alignedRealloc
//============================================================================
void* alignedRealloc(void* p, uint newSize, uint alignment, bool move)
{
   if (!p)
      return alignedMalloc(newSize, alignment);

   BDEBUG_ASSERT(Utils::IsAligned(p, 3));      

   if (!newSize)
   {
      alignedFree(p);
      return NULL;  
   }      

   if (newSize < sizeof(void*)) 
      newSize = sizeof(void*);

   if (!alignment)
   {
      alignment = sizeof(void*);
      if ((newSize & 15) == 0)
         alignment = 16;
      else if ((newSize & 7) == 0)
         alignment = 8;
   }         
   else if (alignment < sizeof(void*))
      alignment = sizeof(void*);

   BDEBUG_ASSERT(alignment <= 16);

   const uint expandSize = newSize + alignment;

   void* pRawPtr = getRawPtr(p);

   void* pNewRawPtr = _expand(pRawPtr, expandSize);
   if (pNewRawPtr)
   {
      if ((pNewRawPtr == pRawPtr) && (_msize(pRawPtr) >= expandSize))
      {
         if (Utils::IsAligned(p, alignment))
         {
            return p;
         }
      }
   }      

   if (!move)
      return NULL;

   void* pNewBlock = alignedMalloc(newSize, alignment);            
   if (!pNewBlock)
      return NULL;

   Utils::FastMemCpy(pNewBlock, p, Math::Min(alignedMSize(p), newSize));

   alignedFree(p);

   return pNewBlock;
}

//============================================================================
// alignedExpand
//============================================================================
void* alignedExpand(void* p, uint newSize, uint alignment)
{
   return alignedRealloc(p, newSize, alignment, false);
}

//============================================================================
// alignedMSize
//============================================================================
uint alignedMSize(void* p)
{
   if (!p)
      return NULL;

   void* pRawPtr = getRawPtr(p);

   const uint rawBlockSize = _msize(pRawPtr);
   const uint alignmentBytes = static_cast<uchar*>(p) - static_cast<uchar*>(pRawPtr);

   BDEBUG_ASSERT(rawBlockSize > alignmentBytes);

   if (rawBlockSize <= alignmentBytes)
      return 0;

   return rawBlockSize - alignmentBytes;
}
#endif
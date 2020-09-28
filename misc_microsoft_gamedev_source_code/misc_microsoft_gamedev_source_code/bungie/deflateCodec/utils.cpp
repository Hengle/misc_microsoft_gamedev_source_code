//============================================================================
// utils.cpp
// Copyright (c) 1996-2006, Ensemble Studios
//
// rgeldreich@ensemblestudios.com
//============================================================================
#include "ens.h"

namespace ens
{
   namespace Utils
   {
      void StoreCacheLines(void* p, uint len)
      {
         assert(len);
         p;
         len;

   #ifdef _XBOX
         uchar* pStart = (uchar*)AlignDown(p, 128);
         uchar* pEnd = (uchar*)AlignUp((uchar*)p + len, 128);
         
         const uint totalBytesToStore = pEnd - pStart;

         for (uint i = 0; i < totalBytesToStore; i += 128)
            __dcbst(i, pStart);
   #endif           
      }
      
      void FlushCacheLines(void* p, uint len)
      {
         assert(len);
         p;
         len;

   #ifdef _XBOX
         uchar* pStart = (uchar*)AlignDown(p, 128);
         uchar* pEnd = (uchar*)AlignUp((uchar*)p + len, 128);

         const uint totalBytesToFlush = pEnd - pStart;

         for (uint i = 0; i < totalBytesToFlush; i += 128)
            __dcbf(i, pStart);
   #endif           
      }
      
   } // namespace utils
   
} // namespace ens

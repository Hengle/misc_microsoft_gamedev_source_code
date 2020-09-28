//============================================================================
// utils.h
// Copyright (c) 1996-2006, Ensemble Studios
//
// rgeldreich@ensemblestudios.com
//============================================================================
#pragma once

namespace ens
{
   namespace Utils
   {
      template<class T> T AlignUp(T p, size_t alignment);
      template<class T> T AlignDown(T p, size_t alignment);
         
      // Calls XMemCpy() on 360, memcpy() on Win32.
      PVOID FastMemCpy(PVOID dst, const VOID *src, SIZE_T count);
      
      PVOID FastMemSet(PVOID dest, int c, SIZE_T count);

      // Clears every 128-byte cache line that lies completely within the specified block.
      // pStart does not need to be aligned. The cleared area will never lie outside the specified block.
      // Not every byte will be cleared to 0 if pStart or len is not 128 byte aligned.
      void ClearCacheLines(void* p, uint len);
      
      void StoreCacheLines(void* p, uint len);
      void FlushCacheLines(void* p, uint len);
      void TouchCacheLine(int offset, const void* base);
      
      typedef const void* BPrefetchState;
      
      BPrefetchState BeginPrefetch(const void* p, uint cacheLinesToReadAhead = 3);
      BPrefetchState BeginPrefetch(const void* p, const void* pEnd, uint cacheLinesToReadAhead = 3);
      BPrefetchState UpdatePrefetch(BPrefetchState state, const void* p, uint cacheLinesToReadAhread = 3);
      BPrefetchState UpdatePrefetch(BPrefetchState state, const void* p, const void* pEnd, uint cacheLinesToReadAhread = 3);
      
      BPrefetchState BeginPrefetchLargeStruct(const void* p, const void* pEnd, uint cacheLinesToReadAhead = 6);
      BPrefetchState BeginPrefetchLargeStruct(const void* p, uint cacheLinesToReadAhead = 6);
      BPrefetchState UpdatePrefetchLargeStruct(BPrefetchState state, const void* p, const void* pEnd, uint cacheLinesToReadAhead = 6);
      BPrefetchState UpdatePrefetchLargeStruct(BPrefetchState state, const void* p, uint cacheLinesToReadAhead = 6);
      
   } // namespace Utils

} // namespace ens

#include "utils.inl"

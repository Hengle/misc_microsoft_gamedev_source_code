//============================================================================
//
// File: utils.cpp
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "file\win32File.h"
#include "hash\bsha1.h"
#include "math\random.h"

namespace Utils
{
   uint64 BBitMasks::mBitMasks[64];      

   BBitMasks mBitMasks;
   
   void GetHRESULTDesc(HRESULT hres, char* pBuf, uint bufLen)
   {
      BDEBUG_ASSERT(pBuf);
      
#ifdef XBOX      
      sprintf_s(pBuf, bufLen, "0x%X", hres);
#else      
      if (0 == FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hres, 0, pBuf, bufLen, NULL))
         *pBuf = '\0';
#endif         
   }
         
   bool WriteWin32File(const char* pFileName, const uchar* pData, uint len)
   {
      BWin32File file;
      
      if (!file.open(pFileName, BWin32File::cCreateAlways | BWin32File::cWriteAccess))
         return false;
      
      if ((pData) && (len))      
      {
         if (!file.write(pData, len))
            return false;
      }
      
      return true;         
   }
   
   uchar* ReadWin32File(const char* pFileName, uint& len)
   {
      len = 0;
      
      BWin32File file;
      if (!file.open(pFileName))
         return NULL;
      
      if (!file.getSize(len))
         return false;
           
      uchar* p = new uchar[max(1, len)];
      
      DWORD numRead = 0;
      if (len)
         numRead = file.read(p, len);
         
      if (numRead != len)
      {
         len = 0;
         delete[] p;
         return NULL;
      }
      
      return p;
   }
   
   void MountUtilityDrive(BOOL fFormatClean, DWORD dwBytesPerCluster, SIZE_T dwFileCacheSize)
   {
      fFormatClean;
      dwBytesPerCluster;
      dwFileCacheSize;
      
#ifdef XBOX
      XMountUtilityDrive(fFormatClean, dwBytesPerCluster, dwFileCacheSize);   
#endif      
   }
   
   const BCHAR_T* GetUtilityDrivePath(void)
   {
#ifdef XBOX
      return B("cache:\\");
#else
      return B("c:\\temp\\");
#endif   
   }
   
   void ClearCacheLines(void* p, uint len)
   {
      BDEBUG_ASSERT(len);   
      p;
      len;
      
#ifdef XBOX
      uchar* pStart = (uchar*)p;
      uchar* pEnd = (uchar*)pStart + len;
      
      uchar* pFirstCacheLineToClear = AlignUp(pStart, 128);
      if (pFirstCacheLineToClear < pEnd)
      {
         const uint totalBytesToClear = (pEnd - pFirstCacheLineToClear) & ~127;

         BDEBUG_ASSERT( (pFirstCacheLineToClear >= pStart) && (pFirstCacheLineToClear + totalBytesToClear - 1) < pEnd );

         for (uint i = 0; i < totalBytesToClear; i += 128)
            __dcbz128(i, pFirstCacheLineToClear);
      }    
#endif           
   }
   
   void StoreCacheLines(void* p, uint len)
   {
      BDEBUG_ASSERT(len);
      p;
      len;

#ifdef XBOX
      uchar* pStart = (uchar*)AlignDown(p, 128);
      uchar* pEnd = (uchar*)AlignUp((uchar*)p + len, 128);
      
      const uint totalBytesToStore = pEnd - pStart;

      for (uint i = 0; i < totalBytesToStore; i += 128)
         __dcbst(i, pStart);
#endif           
   }
   
   void FlushCacheLines(void* p, uint len)
   {
      BDEBUG_ASSERT(len);
      p;
      len;

#ifdef XBOX
      uchar* pStart = (uchar*)AlignDown(p, 128);
      uchar* pEnd = (uchar*)AlignUp((uchar*)p + len, 128);

      const uint totalBytesToFlush = pEnd - pStart;

      for (uint i = 0; i < totalBytesToFlush; i += 128)
      {
         __dcbf(i, pStart);
      }
#endif           
   }
   
   void TouchCacheLines(void* p, uint len)
   {
      BDEBUG_ASSERT(len);
      p;
      len;

#ifdef XBOX
      uchar* pStart = (uchar*)AlignDown(p, 128);
      uchar* pEnd = (uchar*)AlignUp((uchar*)p + len, 128);

      const uint totalBytesToTouch = pEnd - pStart;

      for (uint i = 0; i < totalBytesToTouch; i += 128)
         __dcbt(i, pStart);
#endif           
   }
   
   void BBitMasks::initBitMasks(void)
   {
      uint64 mask = 1U;
      uint64* pDst = mBitMasks;

      do
      {
         *pDst++ = mask;
         mask <<= 1U;
      } while (mask);

      BDEBUG_ASSERT( (mBitMasks[63] >> 63U) == 1U );
   }

   void EndianSwitch(void* p, uint size)
   {
      uchar* pBytes = static_cast<uchar*>(p);
      
      const uint halfSize = size >> 1;
      for (uint i = 0; i < halfSize; i++)
      {
         const uchar c = pBytes[i];
         pBytes[i] = pBytes[size - 1 - i];
         pBytes[size - 1 - i] = c;
      }
   }
   

#ifdef XBOX
#pragma optimize("", off)
   void ClearCaches(void)
   {
      const size_t MemSize = 2*1024*1024;
      char* pMemory = new char[MemSize];

      // Zero the newly allocated memory - this gets as much of it as will
      // fit into the L2 cache. This step isn't strictly necessary, but it avoids
      // any potential warnings about reading from uninitialized memory.
      XMemSet( pMemory, 0, MemSize );

      // Now loop through, reading from every byte. This pulls the data into the
      // L1 cache - as much of it as will fit.
      DWORD gSum = 0;
      for( int i = 0; i < MemSize; ++i )
      {
         gSum += pMemory[i];
      }

      // Now flush the data out of the caches. This should leave the L1 and L2
      // caches virtually empty.
      for( int i = 0; i < MemSize; i += 128 )
         __dcbf( i, pMemory );

      delete [] pMemory;

#if 0   
      MEMORYSTATUS status;
      ZeroMemory(&status, sizeof(status));
      GlobalMemoryStatus(&status);

      const uint MB = 1024U * 1024U;

      trace("PhysAlloc: %uMB, PhysFree=%uMB, VirtFree=%uMB", 
         (status.dwTotalPhys - status.dwAvailPhys + (MB / 2)) / MB, 
         status.dwAvailPhys / MB, 
         status.dwAvailVirtual/MB );
#endif      
   }
   // Restore optimizations.
#pragma optimize("", on)
#else
   void ClearCaches(void)
   {
   }
#endif
   
}


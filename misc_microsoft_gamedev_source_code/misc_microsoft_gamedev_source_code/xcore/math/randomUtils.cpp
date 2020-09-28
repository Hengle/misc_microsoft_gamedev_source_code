//============================================================================
//
// File: randomUtils.cpp
// Copyright (c) 2005-2007, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "randomUtils.h"
#include "hash\teaCrypt.h"

#define RND_SHR3  (jsr^=(jsr<<17), jsr^=(jsr>>13), jsr^=(jsr<<5))

namespace RandomUtils
{
   DWORD RandomFill32(void* p, uint size, DWORD seed)
   {
      BDEBUG_ASSERT(p);
      
      // This should be pretty fast on 360.
   
      DWORD jsr = seed ? seed : 1;

      DWORD* pMemD = static_cast<DWORD*>(p);
      const uint numDWORDs = size >> 2;
      for (uint i = numDWORDs; i > 0; i--)
         *pMemD++ = RND_SHR3;

      BYTE* pMemB = reinterpret_cast<BYTE*>(pMemD);
      const uint numBYTEs = size & 3;
      for (uint i = numBYTEs; i > 0; i--)
         *pMemB++ = (BYTE)RND_SHR3;

      return jsr;
   }  
      
   uint64 RandomFill64(void* p, uint size, uint64 seed)
   {
      BDEBUG_ASSERT(p);
      
      Random rand;
      rand.setSeed64(seed);

      DWORD* pMemD = static_cast<DWORD*>(p);
      const uint numDWORDs = size >> 2;
      for (uint i = numDWORDs; i > 0; i--)
         *pMemD++ = rand.uRandFast();

      BYTE* pMemB = reinterpret_cast<BYTE*>(pMemD);
      const uint numBYTEs = size & 3;
      for (uint i = numBYTEs; i > 0; i--)
         *pMemB++ = (BYTE)rand.uRandFast();

      uint64 newSeed = rand.uRandFast();
      newSeed |= (((uint64)rand.uRandFast()) << 32U);
      
      return newSeed;
   }  

   #pragma warning(disable:4700)         
   void GenerateRandomSeed(BSHA1& sha1)
   {
      // Could also call XNetRandom() on 360, but this function is really only intended for build system and debugging purposes so why bother.

      const uint cMaxDWords = 256;
      DWORD dwords[cMaxDWords];
      uint totalDWords = 0;
      #define addDWord(x) do { BDEBUG_ASSERT(totalDWords < cMaxDWords); dwords[totalDWords++] = (DWORD)(x); } while(0)

      const uint64 startCounter = Utils::GetCounter();

      // Combine a few sources of "random" dwords.
      MEMORYSTATUS memStatus;
      GlobalMemoryStatus(&memStatus);

      // This purposely reads from an uninitialized array!
      const uint cStackStateSize = 64;
      DWORD stackState[cStackStateSize];

      // This purposely does NOT initialize seed!
      
      for (uint i = 1; i < cStackStateSize; i++) 
         addDWord(stackState[i]);

      void* p = HeapAlloc(GetProcessHeap(), 0, 64);
      
      if (p)
      {
         // Purposely read whatever garbage is in the heap at p
         for (uint i = 0; i < 16; i++)
            addDWord(static_cast<const DWORD*>(p)[i]);
         HeapFree(GetProcessHeap(), 0, p);
      }
      
      SYSTEMTIME systemTime;
      GetSystemTime(&systemTime);
      for (uint i = 0; i < sizeof(SYSTEMTIME) / sizeof(DWORD); i++)
         addDWord(reinterpret_cast<const DWORD*>(&systemTime)[i]);
      
      addDWord((DWORD)p);
      addDWord((DWORD)&GenerateGUID);
      addDWord((DWORD)&GetModuleHandleA);
      addDWord((DWORD)GetModuleHandleA(NULL));
      addDWord((DWORD)Utils::GetCounterFrequency());
      addDWord((DWORD)Utils::GetCounter());
      addDWord((DWORD)&memStatus);
      addDWord((DWORD)GetProcessHeap());
      addDWord((DWORD)GetCurrentThreadId());
      addDWord((DWORD)Utils::GetCounter());
      addDWord((DWORD)memStatus.dwAvailVirtual);
      addDWord((DWORD)memStatus.dwAvailPhys);
      addDWord((DWORD)(Utils::GetCounter() >> 32U));

      FILETIME CreationTime, ExitTime, KernelTime, UserTime;      
      // Won't don't care if GetThreadTimes() fails.
      const BOOL result = GetThreadTimes(GetCurrentThread(), &CreationTime, &ExitTime, &KernelTime, &UserTime);
      result;
      
      addDWord(CreationTime.dwHighDateTime);
      addDWord(CreationTime.dwLowDateTime);
      addDWord(ExitTime.dwHighDateTime);
      addDWord(ExitTime.dwLowDateTime);
      addDWord(KernelTime.dwHighDateTime);  
      addDWord(KernelTime.dwLowDateTime);
      addDWord(UserTime.dwHighDateTime);     
      addDWord(UserTime.dwLowDateTime);
      addDWord((DWORD)GetTickCount());

#ifndef XBOX   
      static DWORD volumeSerial = 0;
      if (!volumeSerial)
         GetVolumeInformation(NULL, NULL, 0, &volumeSerial, NULL, NULL, NULL, 0);
         
      addDWord(volumeSerial);
            
      static uint64 a = 0, b = 0, c = 0;
      if ((a | b | c) == 0)
         GetDiskFreeSpaceEx(NULL, (PULARGE_INTEGER)&a, (PULARGE_INTEGER)&b, (PULARGE_INTEGER)&c);
         
      addDWord(a);
      addDWord(a>>32);
      addDWord(b);
      addDWord(b>>32);
      addDWord(c);
      addDWord(c>>32);
      
      char buf[64];
      DWORD bufSize = sizeof(buf);
      
      if (GetComputerNameA(buf, &bufSize))
      {
         const uint totalDWORDs = (strlen(buf) + 3) >> 2;
         // We don't care if this reads up to 3 bytes beyond the string.
         for (uint i = 0; i < totalDWORDs; i++)
            addDWord( reinterpret_cast<const DWORD*>(buf)[i] );
      }
      
      bufSize = sizeof(buf);
      if (GetUserNameA(buf, &bufSize))
      {
         const uint totalDWORDs = (strlen(buf) + 3) >> 2;
         // We don't care if this reads up to 3 bytes beyond the string.
         for (uint i = 0; i < totalDWORDs; i++)
            addDWord( reinterpret_cast<const DWORD*>(buf)[i] );
      }
#endif    

      const uint64 totalCount = Utils::GetCounter() - startCounter;              
      addDWord((DWORD)totalCount);
      
      BSHA1Gen sha1Gen;
      sha1Gen.update(dwords, totalDWords * sizeof(DWORD));
      sha1 = sha1Gen.finalize();
   }
   #pragma warning(default:4700)                        
   
   uint64 GenerateRandomSeed(void)
   {
      BSHA1 sha1;
      GenerateRandomSeed(sha1);
      
      uint64 result  =  (uint64)sha1.getDWORD(0) | (((uint64)sha1.getDWORD(1)) << 32U);
             result ^= ((uint64)sha1.getDWORD(2) | (((uint64)sha1.getDWORD(3)) << 32U));
             result ^=  (uint64)sha1.getDWORD(4);
      
      return result;
   }

   void GenerateGUID(DWORD pGUID[4])
   {
      BDEBUG_ASSERT(pGUID);

      // There are plenty of standard ways of generating GUID's under Win32. 
      // This method should be good enough for content build purposes, doesn't require ole32.dll, and the same code will work on 360.

      BSHA1Gen sha1Gen;

      for (uint i = 0; i < 4; i++)
      {
         BSHA1 sha1;
         GenerateRandomSeed(sha1);
         sha1Gen.update(&sha1, sizeof(sha1));
         
         Sleep(0);
      }

      const BSHA1 guid(sha1Gen.finalize());

      pGUID[0] = guid.getDWORD(0);
      pGUID[1] = guid.getDWORD(1);
      pGUID[2] = guid.getDWORD(2);
      pGUID[3] = guid.getDWORD(3);
      pGUID[0] ^= guid.getDWORD(4);
   }
   
   static __declspec(thread) uint64 mThreadNonceKey[2];
   static __declspec(thread) uint64 mPrevNonce;
   
   static const uint64 cNonceIV = 0x453ff9fdb9563fd8ULL;
   
   static uint64 GenerateNonceInternal(void)
   {
      if (!mThreadNonceKey[0])
      {
         const uint currentThreadID = GetCurrentThreadId();
         
         RandomFill64(mThreadNonceKey, sizeof(mThreadNonceKey), currentThreadID ^ cNonceIV);
         
         RandomFill64(&mPrevNonce, sizeof(mPrevNonce), mThreadNonceKey[0] ^ cNonceIV);
         
         if (!mThreadNonceKey[0])
            mThreadNonceKey[0]++;
      }
      
      // Block cipher in OFB mode.
      mPrevNonce = teaEncipher(mPrevNonce, mThreadNonceKey[0], mThreadNonceKey[1]);
                  
      return mPrevNonce;
   }
   
   uint64 GenerateNonce64(void)
   {
      const uint64 n = GenerateNonceInternal();
      if (!n)
         return 1U;
      return n;
   }
   
   uint GenerateNonce32(void)
   {
      const uint64 n = GenerateNonceInternal();
      
      const uint n32 = ((uint)n) ^ (uint)(n >> 32U);
      if (!n32)
         return 1U;
         
      return n32;
   }
}
   
//============================================================================
//
// File: randomUtils.h
// Copyright (c) 2005-2007, Ensemble Studios
//
//============================================================================
#pragma once
#include "hash\bsha1.h"
#include "math\random.h"

namespace RandomUtils
{
   // Uses a simple 32-bit LFSR to fill an arbitrary block of memory with "random" bits. 
   // Should be pretty fast, even on 360. Returns new seed.
   DWORD RandomFill32(void* p, uint size, DWORD seed = 123456789);
   
   // Uses the random class to fill an arbitrary block of memory with "random" bits. 
   // Slower than the 32-bit RandomFill function. Returns new seed.
   uint64 RandomFill64(void* p, uint size, uint64 seed = 0xABCDEF0123456789);
 
   // Combines a few sources of "random" bits to generate a seed.
   // Intended as a debug helper only.
   // This calls GlobalMemoryStatus() and allocates a temporary block using HeapAlloc().    
   void GenerateRandomSeed(BSHA1& sha1);
   uint64 GenerateRandomSeed(void);

   // This is pretty slow (at least 4ms). 
   // Note that this is not a valid Windows GUID, just a random number that is highly likely to be spatially and temporally unique.
   void GenerateGUID(DWORD pGUID[4]);
   
   template<typename PtrType>
   inline void RandomizeSequence(Random& rand, PtrType pBegin, PtrType pEnd)
   {
      if (pEnd <= pBegin)
         return;
      const uint num = pEnd - pBegin;
      for (uint i = 0; i < num; i++)
      {
         const uint j = rand.iRand(i, num);
         if (i != j)
            std::swap(pBegin[i], pBegin[j]);
      }
   }
   
   // These nonce generators are not secure, but very fast and good enough for handle managers that need to link handles with memory blocks.
   // The return values are completely deterministic, and are only randomized based off the thread's ID!
   
   uint64 GenerateNonce64(void);
   
   uint GenerateNonce32(void);
}
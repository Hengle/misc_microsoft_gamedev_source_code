//==============================================================================
//
// File: tigerCrypt.cpp
//
// Copyright (c) 2007, Ensemble Studios
//
// This borrows the basic operations of the SHAZAM cypher:
// http://theory.lcs.mit.edu/~zulfikar/papers/shazam_ahag.ps
//
// This algorithm is definitely less secure than SHAZAM.
// This isn't used anymore - see teaCrypt.cpp/.h instead.
//
//==============================================================================
#include "xcore.h"
#include "tigerCrypt.h"
#include "tiger.h"

void tigerCryptInitKeys(const BString& keyPhrase, uint64& k1, uint64& k2, uint64& k3)
{
   BTigerHashGen hashGen;
   hashGen.update32(0xa4800c14);
   hashGen.update(keyPhrase.getPtr(), keyPhrase.length());
   hashGen.update32(0x5AF4A9F1);
   hashGen.update32(0xCA6884EC);

   BTigerHash hash(hashGen.finalize());
   
   hashGen.clear();
   hashGen.update32(0xcb92eaeb);
   hashGen.update(hash);
   hashGen.update32(0x1d919bf8);
   
   BTigerHash hash2(hashGen.finalize());

   k1 = hash2.getQWORD(0);
   k2 = hash2.getQWORD(1);
   k3 = hash2.getQWORD(2);
}

void tigerEncryptBlock48(uint64 iv, uint64 k1, uint64 k2, uint64 k3, const uchar* pSrcData, uchar* pDstData, uint counter)
{
   BCOMPILETIMEASSERT((cTigerHashSize * 2) == cTigerCryptBlockSize);
   
   const uchar* pL = pSrcData;
   const uchar* pR = pSrcData + cTigerHashSize;
   
   BTigerHashGen initialGen;
   initialGen.update64(iv);
   if (counter)
      initialGen.update32(counter);
      
   BTigerHash s(pL), t(pR);
   
   BTigerHashGen gen(initialGen);
   gen.update(pR, cTigerHashSize);
   gen.update64(k1);
   s ^= gen.finalize();
      
   gen = initialGen;
   gen.update(s);
   gen.update64(k2);
   t ^= gen.finalize();
   
   BTigerHash v(s);
      
   gen = initialGen;
   gen.update(t);
   gen.update64(k2);
   v ^= gen.finalize();
   
   BTigerHash w(t);
   
   gen = initialGen;
   gen.update(v);
   gen.update64(k3);
   w ^= gen.finalize();
   
   memcpy(pDstData, v.getBuf(), cTigerHashSize);
   memcpy(pDstData + cTigerHashSize, w.getBuf(), cTigerHashSize);
}

void tigerDecryptBlock48(uint64 iv, uint64 k1, uint64 k2, uint64 k3, const uchar* pSrcData, uchar* pDstData, uint counter)
{
   const uchar* pV = pSrcData;
   const uchar* pW = pSrcData + cTigerHashSize;

   BTigerHashGen initialGen;
   initialGen.update64(iv);
   if (counter)
      initialGen.update32(counter);

   BTigerHash t(pW), s(pV);
   
   BTigerHashGen gen(initialGen);
   gen.update(pV);
   gen.update64(k3);
   t ^= gen.finalize();
   
   gen = initialGen;
   gen.update(t);
   gen.update64(k2);
   s ^= gen.finalize();
   
   BTigerHash r(t);
   
   gen = initialGen;   
   gen.update(s);
   gen.update64(k2);
   r ^= gen.finalize();
   
   BTigerHash l(s);
   
   gen = initialGen;
   gen.update(r);
   gen.update64(k1);
   l ^= gen.finalize();
         
   memcpy(pDstData, l.getBuf(), cTigerHashSize);
   memcpy(pDstData + cTigerHashSize, r.getBuf(), cTigerHashSize);
}


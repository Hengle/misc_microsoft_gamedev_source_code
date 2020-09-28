//==============================================================================
//
// File: teaCrypt.cpp
//
// Copyright (c) 2007, Ensemble Studios
//
// See: http://www.simonshepherd.supanet.com/tea.htm
//==============================================================================
#include "xcore.h"
#include "hash\teaCrypt.h"
#include "hash\bsha1.h"

void teaEncipher(const uint* v, uint* w, const uint* k, uint n)
{
   uint y = v[0], z = v[1], sum = 0, delta = 0x9E3779B9;
   uint a = k[0], b = k[1], c = k[2], d = k[3];

   while (n-- > 0)
   {
      sum += delta;
      y += (z << 4) + (a ^ z) + (sum ^ (z >> 5)) + b;
      z += (y << 4) + (c ^ y) + (sum ^ (y >> 5)) + d;
   }

   w[0] = y; 
   w[1] = z;
}

void teaDecipher(const uint* v, uint* w, const uint* k, uint n)
{
   uint y = v[0], z = v[1];
   uint delta = 0x9E3779B9, a = k[0], b = k[1];
   uint c = k[2], d = k[3];
   uint sum = (delta == 32) ? 0xC6EF3720 : (delta * n);

   /* sum = delta<<5, in general sum = delta * n */

   while (n-- > 0)
   {
      z -= (y << 4) + (c ^ y) + (sum ^ (y >> 5)) + d;
      y -= (z << 4) + (a ^ z) + (sum ^ (z >> 5)) + b;
      sum -= delta;
   }

   w[0] = y; 
   w[1] = z;
}

uint64 teaEncipher(uint64 v0, uint64 k0, uint64 k1)
{
   const uint a = (uint)k0;
   const uint b = (uint)(k0 >> 32U);
   const uint c = (uint)k1;
   const uint d = (uint)(k1 >> 32U);
   
   uint y0 = (uint)v0; uint z0 = (uint)(v0 >> 32U);
   
#define TEA_ROUND(x) \
   y0 += (z0 << 4) + (a ^ z0) + (x ^ (z0 >> 5)) + b; \
   z0 += (y0 << 4) + (c ^ y0) + (x ^ (y0 >> 5)) + d;
   
   TEA_ROUND(0x9E3779B9); TEA_ROUND(0x3C6EF372); TEA_ROUND(0xDAA66D2B); TEA_ROUND(0x78DDE6E4);
   TEA_ROUND(0x1715609D); TEA_ROUND(0xB54CDA56); TEA_ROUND(0x5384540F); TEA_ROUND(0xF1BBCDC8);
   
//   TEA_ROUND(0x8FF34781); TEA_ROUND(0x2E2AC13A); TEA_ROUND(0xCC623AF3); TEA_ROUND(0x6A99B4AC);
//   TEA_ROUND(0x08D12E65); TEA_ROUND(0xA708A81E); TEA_ROUND(0x454021D7); TEA_ROUND(0xE3779B90);
#undef TEA_ROUND   
   
   return ((uint64)y0) | (((uint64)z0) << 32U);
}

uint64 teaDecipher(uint64 v0, uint64 k0, uint64 k1)
{
   const uint a = (uint)k0;
   const uint b = (uint)(k0 >> 32U);
   const uint c = (uint)k1;
   const uint d = (uint)(k1 >> 32U);

   uint y0 = (uint)v0; uint z0 = (uint)(v0 >> 32U);
   
   // This is dying to be optimized with VMX!
   
#define TEA_ROUND(x) \
   z0 -= (y0 << 4) + (c ^ y0) + (x ^ (y0 >> 5)) + d; \
   y0 -= (z0 << 4) + (a ^ z0) + (x ^ (z0 >> 5)) + b;

//   TEA_ROUND(0xE3779B90); TEA_ROUND(0x454021D7); TEA_ROUND(0xA708A81E); TEA_ROUND(0x08D12E65); 
//   TEA_ROUND(0x6A99B4AC); TEA_ROUND(0xCC623AF3); TEA_ROUND(0x2E2AC13A); TEA_ROUND(0x8FF34781); 

   TEA_ROUND(0xF1BBCDC8); TEA_ROUND(0x5384540F); TEA_ROUND(0xB54CDA56); TEA_ROUND(0x1715609D); 
   TEA_ROUND(0x78DDE6E4); TEA_ROUND(0xDAA66D2B); TEA_ROUND(0x3C6EF372); TEA_ROUND(0x9E3779B9); 
#undef TEA_ROUND   

   return ((uint64)y0) | (((uint64)z0) << 32U);
}

void teaEncipher(
   uint64 v0, uint64 v1, uint64 v2, uint64 v3,
   uint64& w0, uint64& w1, uint64& w2, uint64& w3, 
   uint64 k0, uint64 k1)
{
   const uint a = (uint)k0;
   const uint b = (uint)(k0 >> 32U);
   const uint c = (uint)k1;
   const uint d = (uint)(k1 >> 32U);
   
   uint y0 = (uint)v0; uint z0 = (uint)(v0 >> 32U);
   uint y1 = (uint)v1; uint z1 = (uint)(v1 >> 32U);
   uint y2 = (uint)v2; uint z2 = (uint)(v2 >> 32U);
   uint y3 = (uint)v3; uint z3 = (uint)(v3 >> 32U);

   // 16 hardcoded rounds on 4 64-bit values in parallel         
#define TEA_ROUND(x) \
   y0 += (z0 << 4) + (a ^ z0) + (x ^ (z0 >> 5)) + b; \
   y1 += (z1 << 4) + (a ^ z1) + (x ^ (z1 >> 5)) + b; \
   y2 += (z2 << 4) + (a ^ z2) + (x ^ (z2 >> 5)) + b; \
   y3 += (z3 << 4) + (a ^ z3) + (x ^ (z3 >> 5)) + b; \
   z0 += (y0 << 4) + (c ^ y0) + (x ^ (y0 >> 5)) + d; \
   z1 += (y1 << 4) + (c ^ y1) + (x ^ (y1 >> 5)) + d; \
   z2 += (y2 << 4) + (c ^ y2) + (x ^ (y2 >> 5)) + d; \
   z3 += (y3 << 4) + (c ^ y3) + (x ^ (y3 >> 5)) + d;
   
   TEA_ROUND(0x9E3779B9); TEA_ROUND(0x3C6EF372); TEA_ROUND(0xDAA66D2B); TEA_ROUND(0x78DDE6E4);
   TEA_ROUND(0x1715609D); TEA_ROUND(0xB54CDA56); TEA_ROUND(0x5384540F); TEA_ROUND(0xF1BBCDC8);
   
   TEA_ROUND(0x8FF34781); TEA_ROUND(0x2E2AC13A); TEA_ROUND(0xCC623AF3); TEA_ROUND(0x6A99B4AC);
   TEA_ROUND(0x08D12E65); TEA_ROUND(0xA708A81E); TEA_ROUND(0x454021D7); TEA_ROUND(0xE3779B90);
#undef TEA_ROUND   
   
   w0 = ((uint64)y0) | (((uint64)z0) << 32U);
   w1 = ((uint64)y1) | (((uint64)z1) << 32U);
   w2 = ((uint64)y2) | (((uint64)z2) << 32U);
   w3 = ((uint64)y3) | (((uint64)z3) << 32U);
}

void teaDecipher(
   uint64 v0, uint64 v1, uint64 v2, uint64 v3,
   uint64& w0, uint64& w1, uint64& w2, uint64& w3,
   uint64 k0, uint64 k1)
{
   const uint a = (uint)k0;
   const uint b = (uint)(k0 >> 32U);
   const uint c = (uint)k1;
   const uint d = (uint)(k1 >> 32U);

   uint y0 = (uint)v0; uint z0 = (uint)(v0 >> 32U);
   uint y1 = (uint)v1; uint z1 = (uint)(v1 >> 32U);
   uint y2 = (uint)v2; uint z2 = (uint)(v2 >> 32U);
   uint y3 = (uint)v3; uint z3 = (uint)(v3 >> 32U);
   
   // This is dying to be optimized with VMX!
   
   // 16 hardcoded rounds on 4 64-bit values in parallel
#define TEA_ROUND(x) \
   z0 -= (y0 << 4) + (c ^ y0) + (x ^ (y0 >> 5)) + d; \
   z1 -= (y1 << 4) + (c ^ y1) + (x ^ (y1 >> 5)) + d; \
   z2 -= (y2 << 4) + (c ^ y2) + (x ^ (y2 >> 5)) + d; \
   z3 -= (y3 << 4) + (c ^ y3) + (x ^ (y3 >> 5)) + d; \
   y0 -= (z0 << 4) + (a ^ z0) + (x ^ (z0 >> 5)) + b; \
   y1 -= (z1 << 4) + (a ^ z1) + (x ^ (z1 >> 5)) + b; \
   y2 -= (z2 << 4) + (a ^ z2) + (x ^ (z2 >> 5)) + b; \
   y3 -= (z3 << 4) + (a ^ z3) + (x ^ (z3 >> 5)) + b;

   TEA_ROUND(0xE3779B90); TEA_ROUND(0x454021D7); TEA_ROUND(0xA708A81E); TEA_ROUND(0x08D12E65); 
   TEA_ROUND(0x6A99B4AC); TEA_ROUND(0xCC623AF3); TEA_ROUND(0x2E2AC13A); TEA_ROUND(0x8FF34781); 

   TEA_ROUND(0xF1BBCDC8); TEA_ROUND(0x5384540F); TEA_ROUND(0xB54CDA56); TEA_ROUND(0x1715609D); 
   TEA_ROUND(0x78DDE6E4); TEA_ROUND(0xDAA66D2B); TEA_ROUND(0x3C6EF372); TEA_ROUND(0x9E3779B9); 
#undef TEA_ROUND   

   w0 = ((uint64)y0) | (((uint64)z0) << 32U);
   w1 = ((uint64)y1) | (((uint64)z1) << 32U);
   w2 = ((uint64)y2) | (((uint64)z2) << 32U);
   w3 = ((uint64)y3) | (((uint64)z3) << 32U);
}

void teaCryptInitKeys(const char* pKeyPhrase, uint64& k1, uint64& k2, uint64& k3)
{
   BDEBUG_ASSERT(pKeyPhrase && strlen(pKeyPhrase));
   
   BSHA1Gen hashGen;
   hashGen.update32(0xa4800c14);
   hashGen.update(pKeyPhrase, strlen(pKeyPhrase));
   hashGen.update32(0x5AF4A9F1);
   hashGen.update32(0xCA6884EC);

   BSHA1 hash(hashGen.finalize());

   hashGen.clear();
   hashGen.update32(0xcb92eaeb);
   hashGen.update(hash);
   hashGen.update32(0x1d919bf8);

   BSHA1 hash2(hashGen.finalize());

   k1 = ((uint64)hash2.getDWORD(0)) | (((uint64)hash2.getDWORD(1)) << 32U);
   k2 = ((uint64)hash2.getDWORD(2)) | (((uint64)hash2.getDWORD(3)) << 32U);
   k3 = ((uint64)hash2.getDWORD(4)) | (((uint64)hash.getDWORD(0)) << 32U);
}

#define LFSR3(x)  (x^=(x<<17), x^=(x>>13), x^=(x<<5))
#define BLOCK_DISPERSE(x, y, z, w) x ^= y; y ^= z; z ^= w; w ^= x;
#define BLOCK_CONTRACT(x, y, z, w) w ^= x; z ^= w; y ^= z; x ^= y;

void teaEncryptBlock64(uint64 iv, uint64 k1, uint64 k2, uint64 k3, const uchar* pSrcData, uchar* pDstData, uint counter)
{
   uint64 v0, v1, v2, v3, v4, v5, v6, v7;
   
   if (cBigEndianNative)   
   {
      v0 = ((uint64*)pSrcData)[0];
      v1 = ((uint64*)pSrcData)[1];
      v2 = ((uint64*)pSrcData)[2];
      v3 = ((uint64*)pSrcData)[3];
      v4 = ((uint64*)pSrcData)[4];
      v5 = ((uint64*)pSrcData)[5];
      v6 = ((uint64*)pSrcData)[6];
      v7 = ((uint64*)pSrcData)[7];
   }
   else
   {
      Utils::ReadValueByteSwapped<uint64>(&((uint64*)pSrcData)[0], v0);
      Utils::ReadValueByteSwapped<uint64>(&((uint64*)pSrcData)[1], v1);
      Utils::ReadValueByteSwapped<uint64>(&((uint64*)pSrcData)[2], v2);
      Utils::ReadValueByteSwapped<uint64>(&((uint64*)pSrcData)[3], v3);
      Utils::ReadValueByteSwapped<uint64>(&((uint64*)pSrcData)[4], v4);
      Utils::ReadValueByteSwapped<uint64>(&((uint64*)pSrcData)[5], v5);
      Utils::ReadValueByteSwapped<uint64>(&((uint64*)pSrcData)[6], v6);
      Utils::ReadValueByteSwapped<uint64>(&((uint64*)pSrcData)[7], v7);
   }
   
   counter += (uint)(iv >> 10);
   if (!counter) 
      counter++;
      
   counter = LFSR3(counter); v0 ^= (counter + iv);
   counter = LFSR3(counter); v1 ^= (counter - iv);
   counter = LFSR3(counter); v2 ^= (counter + iv);
   counter = LFSR3(counter); v3 ^= (counter - iv);
   counter = LFSR3(counter); v4 ^= (counter + iv);
   counter = LFSR3(counter); v5 ^= (counter - iv);
   counter = LFSR3(counter); v6 ^= (counter + iv);
   counter = LFSR3(counter); v7 ^= (counter - iv);
               
   uint64 w0, w1, w2, w3;
   teaEncipher(v0, v1, v2, v3, w0, w1, w2, w3, k2, k1);
   BLOCK_DISPERSE(w0, w1, w2, w3)
   BLOCK_DISPERSE(w0, w1, w2, w3)
   BLOCK_DISPERSE(w0, w1, w2, w3)
   
   v4 ^= w3;
   v5 -= w2;
   v6 ^= w1;
   v7 += w0;
   
   uint64 w4, w5, w6, w7;
   teaEncipher(v4, v5, v6, v7, w4, w5, w6, w7, k2, k3);
   BLOCK_DISPERSE(w4, w5, w6, w7)
   BLOCK_DISPERSE(w4, w5, w6, w7)
   BLOCK_DISPERSE(w4, w5, w6, w7)
   
   w0 += w7;
   w1 ^= w6;
   w2 -= w5;
   w3 ^= w4;
   
   teaEncipher(w0, w1, w2, w3, v0, v1, v2, v3, k1, k2);
   BLOCK_DISPERSE(w4, w5, w6, w7)
   BLOCK_DISPERSE(w4, w5, w6, w7)
   BLOCK_DISPERSE(w4, w5, w6, w7)
         
   if (cBigEndianNative)   
   {
      ((uint64*)pDstData)[0] = v0;
      ((uint64*)pDstData)[1] = v1;
      ((uint64*)pDstData)[2] = v2;
      ((uint64*)pDstData)[3] = v3;
      ((uint64*)pDstData)[4] = w4;
      ((uint64*)pDstData)[5] = w5;
      ((uint64*)pDstData)[6] = w6;
      ((uint64*)pDstData)[7] = w7;
   }
   else
   {
      Utils::WriteValueByteSwapped<uint64>(&((uint64*)pDstData)[0], v0);
      Utils::WriteValueByteSwapped<uint64>(&((uint64*)pDstData)[1], v1);
      Utils::WriteValueByteSwapped<uint64>(&((uint64*)pDstData)[2], v2);
      Utils::WriteValueByteSwapped<uint64>(&((uint64*)pDstData)[3], v3);
      Utils::WriteValueByteSwapped<uint64>(&((uint64*)pDstData)[4], w4);
      Utils::WriteValueByteSwapped<uint64>(&((uint64*)pDstData)[5], w5);
      Utils::WriteValueByteSwapped<uint64>(&((uint64*)pDstData)[6], w6);
      Utils::WriteValueByteSwapped<uint64>(&((uint64*)pDstData)[7], w7);
   }
}

void teaDecryptBlock64(uint64 iv, uint64 k1, uint64 k2, uint64 k3, const uchar* pSrcData, uchar* pDstData, uint counter)
{
   uint64 v0, v1, v2, v3, v4, v5, v6, v7;   

   if (cBigEndianNative)   
   {
      v0 = ((uint64*)pSrcData)[0];
      v1 = ((uint64*)pSrcData)[1];
      v2 = ((uint64*)pSrcData)[2];
      v3 = ((uint64*)pSrcData)[3];
      v4 = ((uint64*)pSrcData)[4];
      v5 = ((uint64*)pSrcData)[5];
      v6 = ((uint64*)pSrcData)[6];
      v7 = ((uint64*)pSrcData)[7];
   }
   else
   {
      Utils::ReadValueByteSwapped<uint64>(&((uint64*)pSrcData)[0], v0);
      Utils::ReadValueByteSwapped<uint64>(&((uint64*)pSrcData)[1], v1);
      Utils::ReadValueByteSwapped<uint64>(&((uint64*)pSrcData)[2], v2);
      Utils::ReadValueByteSwapped<uint64>(&((uint64*)pSrcData)[3], v3);
      Utils::ReadValueByteSwapped<uint64>(&((uint64*)pSrcData)[4], v4);
      Utils::ReadValueByteSwapped<uint64>(&((uint64*)pSrcData)[5], v5);
      Utils::ReadValueByteSwapped<uint64>(&((uint64*)pSrcData)[6], v6);
      Utils::ReadValueByteSwapped<uint64>(&((uint64*)pSrcData)[7], v7);
   }
   
   BLOCK_CONTRACT(v4, v5, v6, v7)
   BLOCK_CONTRACT(v4, v5, v6, v7)
   BLOCK_CONTRACT(v4, v5, v6, v7)
   uint64 w0, w1, w2, w3;
   teaDecipher(v0, v1, v2, v3, w0, w1, w2, w3, k1, k2);
   
   w0 -= v7;
   w1 ^= v6;
   w2 += v5;
   w3 ^= v4;
   
   BLOCK_CONTRACT(v4, v5, v6, v7)
   BLOCK_CONTRACT(v4, v5, v6, v7)
   BLOCK_CONTRACT(v4, v5, v6, v7)
   uint64 w4, w5, w6, w7;
   teaDecipher(v4, v5, v6, v7, w4, w5, w6, w7, k2, k3);

   w4 ^= w3;
   w5 += w2;
   w6 ^= w1;
   w7 -= w0;

   BLOCK_CONTRACT(w0, w1, w2, w3)
   BLOCK_CONTRACT(w0, w1, w2, w3)
   BLOCK_CONTRACT(w0, w1, w2, w3)
   teaDecipher(w0, w1, w2, w3, v0, v1, v2, v3, k2, k1);
   
   counter += (uint)(iv >> 10);
   if (!counter) 
      counter++;

   counter = LFSR3(counter); v0 ^= (counter + iv);
   counter = LFSR3(counter); v1 ^= (counter - iv);
   counter = LFSR3(counter); v2 ^= (counter + iv);
   counter = LFSR3(counter); v3 ^= (counter - iv);
   counter = LFSR3(counter); w4 ^= (counter + iv);
   counter = LFSR3(counter); w5 ^= (counter - iv);
   counter = LFSR3(counter); w6 ^= (counter + iv);
   counter = LFSR3(counter); w7 ^= (counter - iv);
  
   if (cBigEndianNative)   
   {
      ((uint64*)pDstData)[0] = v0;
      ((uint64*)pDstData)[1] = v1;
      ((uint64*)pDstData)[2] = v2;
      ((uint64*)pDstData)[3] = v3;
      ((uint64*)pDstData)[4] = w4;
      ((uint64*)pDstData)[5] = w5;
      ((uint64*)pDstData)[6] = w6;
      ((uint64*)pDstData)[7] = w7;
   }
   else
   {
      Utils::WriteValueByteSwapped<uint64>(&((uint64*)pDstData)[0], v0);
      Utils::WriteValueByteSwapped<uint64>(&((uint64*)pDstData)[1], v1);
      Utils::WriteValueByteSwapped<uint64>(&((uint64*)pDstData)[2], v2);
      Utils::WriteValueByteSwapped<uint64>(&((uint64*)pDstData)[3], v3);
      Utils::WriteValueByteSwapped<uint64>(&((uint64*)pDstData)[4], w4);
      Utils::WriteValueByteSwapped<uint64>(&((uint64*)pDstData)[5], w5);
      Utils::WriteValueByteSwapped<uint64>(&((uint64*)pDstData)[6], w6);
      Utils::WriteValueByteSwapped<uint64>(&((uint64*)pDstData)[7], w7);
   }
}






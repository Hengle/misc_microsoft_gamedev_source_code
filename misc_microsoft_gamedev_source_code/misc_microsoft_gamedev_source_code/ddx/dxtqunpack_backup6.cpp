// File: DXTQUnpack.cpp
#include "xcore.h"
#include "DXTQUnpack.h"
#include "file\ecfUtils.h"
#include "threading\win32Event.h"
#include "Timer.h"
#include "DDXUtils.h"

#ifdef XBOX
#include "threading\workDistributor.h"
#endif

#include <xgraphics.h>

#define THREADED_DECOMPRESSION 0

#ifdef XBOX
#define TRACE_RECORDING 1
#define PMC_LIB 1
#endif

#define TIMING_STATS 1
#define PRINT_DECOMP_CRC 1

#if PRINT_DECOMP_CRC
#include "hash\crc.h"
#endif

#if TRACE_RECORDING
#include "tracerecording.h"
#pragma comment( lib, "tracerecording.lib" )
#pragma comment(lib, "xbdm.lib")
#endif

#if PMC_LIB
#include <pmcpb.h>
#include <pmcpbsetup.h>
#endif

#pragma warning(disable:4244) // warning C4244: 'argument' : conversion from 'uint64' to 'uint', possible loss of data

#pragma optimize( "t", on )

//------------------------------------------------------------------------------------------------------
// BeginWrite
//------------------------------------------------------------------------------------------------------
typedef void* BWriteState;

inline BWriteState BeginWrite(void* p, void* pEnd, uint cacheLinesToWriteAhead)
{
#ifdef XBOX
   BWriteState state = Utils::AlignUp(p, 128);  
   const uint ofs = (cacheLinesToWriteAhead << 7);
   if (((uchar*)state + ofs + 127) < (uchar*)pEnd)
   {
      __dcbz128(ofs, state);
   }
#endif   
   return p;
}

//------------------------------------------------------------------------------------------------------
// UpdateWrite
//------------------------------------------------------------------------------------------------------
inline BWriteState UpdateWrite(BWriteState prevP, void* p, void* pEnd, uint cacheLinesToWriteAhead)
{
#ifdef XBOX
   if (((uint)prevP ^ (uint)p) & 0xFFFFFF80)
   {
      __dcbf(0, prevP);
      
      //BWriteState state = Utils::AlignUp(prevP, 128);
      BWriteState newState = Utils::AlignUp(p, 128);
      
      const uint ofs = (cacheLinesToWriteAhead << 7);
      if (((uchar*)newState + ofs + 127) < (uchar*)pEnd)
      {
         __dcbz128(ofs, newState);
      }
   }
#endif   
   return p;
}

//------------------------------------------------------------------------------------------------------
// UNPACK_DXT1Q_PACKET
//------------------------------------------------------------------------------------------------------
#define UNPACK_DXT1Q_PACKET(packetBits) \
if (cLittleEndianNative) EndianSwitchQWords(&packetBits, 1); \
if ((int64)packetBits >= 0) \
{ \
   pDst[0] = pColorCodebook[packetBits & 1023]; \
   pDst[1] = pColorSelectorCodebook[(packetBits >> 10) & 2047]; \
   pDst[2] = pColorCodebook[(packetBits >> 21) & 1023]; \
   pDst[3] = pColorSelectorCodebook[(packetBits >> 31) & 2047]; \
   pDst[4] = pColorCodebook[(packetBits >> 42) & 1023]; \
   pDst[5] = pColorSelectorCodebook[(packetBits >> 52) & 2047]; \
   pDst += 6; \
} \
else \
{ \
   if ((packetBits & 511) == 511) \
   { \
      uint runLen = (uint)((packetBits >> 9) & 511) + 4; \
      const uint bc0 = pColorCodebook[(packetBits >> 18) & 1023]; \
      const uint bs0 = pColorSelectorCodebook[(packetBits >> 28) & 2047]; \
      while (runLen >= 4) \
      { \
         pDst[0] = bc0; pDst[1] = bs0; \
         pDst[2] = bc0; pDst[3] = bs0; \
         pDst[4] = bc0; pDst[5] = bs0; \
         pDst[6] = bc0; pDst[7] = bs0; \
         pDst += 8; \
         runLen -= 4; \
      } \
      while (runLen) \
      { \
         pDst[0] = bc0; pDst[1] = bs0; \
         pDst += 2; \
         runLen--; \
      } \
      pDst[0] = pColorCodebook[(packetBits >> 39) & 1023]; \
      pDst[1] = pColorSelectorCodebook[(packetBits >> 49) & 2047]; \
      pDst += 2; \
   } \
   else \
   { \
      uint runLen = (uint)((packetBits >> 60) & 7) + 1; \
      const uint bc0 = pColorCodebook[(packetBits      ) & 15]; \
      const uint bs0 = pColorSelectorCodebook[(packetBits >>  4) & 31]; \
      while (runLen >= 4) \
      { \
         pDst[0] = bc0; pDst[1] = bs0; \
         pDst[2] = bc0; pDst[3] = bs0; \
         pDst[4] = bc0; pDst[5] = bs0; \
         pDst[6] = bc0; pDst[7] = bs0; \
         pDst += 8; \
         runLen -= 4; \
      } \
      while (runLen) \
      { \
         pDst[0] = bc0; pDst[1] = bs0; \
         pDst += 2; \
         runLen--; \
      } \
      pDst[0] = pColorCodebook[(packetBits >>  9) & 255]; \
      pDst[1] = pColorSelectorCodebook[(packetBits >> 17) & 511]; \
      pDst[2] = pColorCodebook[(packetBits >> 26) & 255]; \
      pDst[3] = pColorSelectorCodebook[(packetBits >> 34) & 511]; \
      pDst[4] = pColorCodebook[(packetBits >> 43) & 255]; \
      pDst[5] = pColorSelectorCodebook[(packetBits >> 51) & 511]; \
      pDst += 6; \
   }  \
}

//------------------------------------------------------------------------------------------------------
// BDXTQUnpack::decodeSegmentDXT1
//------------------------------------------------------------------------------------------------------
bool BDXTQUnpack::decodeSegmentDXT1(
   const uchar* __restrict pSrc, 
   const uint srcDataLen,
   DWORD* __restrict pDst, 
   DWORD* __restrict pDstEnd,
   const DWORD* __restrict pColorCodebook,
   const DWORD* __restrict pColorSelectorCodebook,
   bool writeCombinedDst)
{
#ifdef XBOX   
   BDEBUG_ASSERT(((uint)pSrc & 7) == 0);
#endif   
   
   const uchar* __restrict pSrcEndMinus4 = pSrc + srcDataLen - 8 * 4;
   const uchar* __restrict pSrcEnd = pSrc + srcDataLen;
   Utils::BPrefetchState srcPrefetch = Utils::BeginPrefetch(pSrc, pSrcEnd, 2);
   
   BWriteState dstWrite = BeginWrite(pDst, pDstEnd, 3);

   // This loop CANNOT process the last packet because this packet may contain up to 3 extra padding blocks.
   if (!writeCombinedDst)
   {
      while (pSrc < pSrcEndMinus4)
      {
         dstWrite = UpdateWrite(dstWrite, pDst, pDstEnd, 3);
         
         uint64 packet = *(const uint64*)pSrc;
                  
         srcPrefetch = Utils::UpdatePrefetch(srcPrefetch, pSrc, pSrcEnd, 2);

         UNPACK_DXT1Q_PACKET(packet)
         
         packet = *(const uint64*)(pSrc + 8*1);
         UNPACK_DXT1Q_PACKET(packet)                

         packet = *(const uint64*)(pSrc + 8*2);
         UNPACK_DXT1Q_PACKET(packet)
         
         packet = *(const uint64*)(pSrc + 8*3);
         UNPACK_DXT1Q_PACKET(packet)

         pSrc += 8*4;
      };
            
   }
   else
   {
      while (pSrc < pSrcEndMinus4)
      {
         uint64 packet = *(const uint64*)pSrc;
         srcPrefetch = Utils::UpdatePrefetch(srcPrefetch, pSrc, pSrcEnd, 2);

         UNPACK_DXT1Q_PACKET(packet)
         
         packet = *(const uint64*)(pSrc + 8*1);
         UNPACK_DXT1Q_PACKET(packet)                

         packet = *(const uint64*)(pSrc + 8*2);
         UNPACK_DXT1Q_PACKET(packet)
         
         packet = *(const uint64*)(pSrc + 8*3);
         UNPACK_DXT1Q_PACKET(packet)

         pSrc += 8*4;
      };
   }      

   while (pSrc < pSrcEnd)
   {
      if (!writeCombinedDst)
         dstWrite = UpdateWrite(dstWrite, pDst, pDstEnd, 3);
         
      uint64 packet = *(const uint64*)pSrc;
      if (cLittleEndianNative) EndianSwitchQWords(&packet, 1);
      
      pSrc += 8;

      srcPrefetch = Utils::UpdatePrefetch(srcPrefetch, pSrc, pSrcEnd, 2);
                  
      if ((int64)packet < 0)
      {
         if ((packet & 511) == 511)
         {
            uint runLen = (uint)((packet >> 9) & 511) + 4;

            const uint c0 = (uint)((packet >> 18) & 1023);
            const uint s0 = (uint)((packet >> 28) & 2047);

            const uint c1 = (uint)((packet >> 39) & 1023);
            const uint s1 = (uint)((packet >> 49) & 2047);

            const uint bc0 = pColorCodebook[c0];
            const uint bs0 = pColorSelectorCodebook[s0];

            const uint bc1 = pColorCodebook[c1];
            const uint bs1 = pColorSelectorCodebook[s1];

            do 
            {
               pDst[0] = bc0;
               pDst[1] = bs0;
               pDst += 2;
               runLen--;
            } while(runLen);

            if (pDst < pDstEnd)
            {
               pDst[0] = bc1;
               pDst[1] = bs1;
               pDst += 2;
            }               
         }
         else
         {
            uint runLen = (uint)(((packet >> 60) & 7) + 1);

            const uint c0 = (uint)((packet      ) & 15);
            const uint s0 = (uint)((packet >>  4) & 31);

            const uint c1 = (uint)((packet >>  9) & 255);
            const uint s1 = (uint)((packet >> 17) & 511);

            const uint c2 = (uint)((packet >> 26) & 255);
            const uint s2 = (uint)((packet >> 34) & 511);

            const uint c3 = (uint)((packet >> 43) & 255);
            const uint s3 = (uint)((packet >> 51) & 511);

            const uint bc0 = pColorCodebook[c0];
            const uint bs0 = pColorSelectorCodebook[s0];

            const uint bc1 = pColorCodebook[c1];
            const uint bs1 = pColorSelectorCodebook[s1];

            const uint bc2 = pColorCodebook[c2];
            const uint bs2 = pColorSelectorCodebook[s2];

            const uint bc3 = pColorCodebook[c3];
            const uint bs3 = pColorSelectorCodebook[s3];

            do 
            {
               pDst[0] = bc0;
               pDst[1] = bs0;
               pDst += 2;
               runLen--;
            } while(runLen);

            if (pDst < pDstEnd)
            {
               pDst[0] = bc1;
               pDst[1] = bs1;
               pDst += 2;
            
               if (pDst < pDstEnd)
               {
                  pDst[0] = bc2;
                  pDst[1] = bs2;
                  pDst += 2;
               
                  if (pDst < pDstEnd)
                  {
                     pDst[0] = bc3;
                     pDst[1] = bs3;
                     pDst += 2;
                  }
               }
            }
         }            
      }
      else
      {
         const uint c0 = (uint)((packet      ) & 1023);
         const uint s0 = (uint)((packet >> 10) & 2047);

         const uint c1 = (uint)((packet >> 21) & 1023);
         const uint s1 = (uint)((packet >> 31) & 2047);

         const uint c2 = (uint)((packet >> 42) & 1023);
         const uint s2 = (uint)((packet >> 52) & 2047);

         const uint bc0 = pColorCodebook[c0];
         const uint bs0 = pColorSelectorCodebook[s0];

         const uint bc1 = pColorCodebook[c1];
         const uint bs1 = pColorSelectorCodebook[s1];

         const uint bc2 = pColorCodebook[c2];
         const uint bs2 = pColorSelectorCodebook[s2];

         pDst[0] = bc0;
         pDst[1] = bs0;
         pDst += 2;

         if (pDst < pDstEnd)
         {
            pDst[0] = bc1;
            pDst[1] = bs1;
            pDst += 2;
            
            if (pDst < pDstEnd)
            {
               pDst[0] = bc2;
               pDst[1] = bs2;
               pDst += 2;   
            }
         }
      }
   };
   
   return (pDst == pDstEnd);
}

//------------------------------------------------------------------------------------------------------
// writeAlphaBlock
//------------------------------------------------------------------------------------------------------
__forceinline void writeAlphaBlock(
   DWORD* __restrict pDst, uint alphaCode, uint selectorCode, 
   const WORD* __restrict pAlphaCodebook, const DWORD* __restrict pAlphaSelectorCodebookD, const WORD* __restrict pAlphaSelectorCodebookW)
{
#ifdef XBOX   
   pDst[0] = (pAlphaCodebook[alphaCode] << 16) | pAlphaSelectorCodebookW[selectorCode];
#else
   pDst[0] = pAlphaCodebook[alphaCode] | (pAlphaSelectorCodebookW[selectorCode] << 16);
#endif   
   pDst[1] = pAlphaSelectorCodebookD[selectorCode];
}

//------------------------------------------------------------------------------------------------------
// createAlphaBlock
//------------------------------------------------------------------------------------------------------
__forceinline void createAlphaBlock(
   uint& a, uint& b,
   uint alphaCode, uint selectorCode, 
   const WORD* __restrict pAlphaCodebook, const DWORD* __restrict pAlphaSelectorCodebookD, const WORD* __restrict pAlphaSelectorCodebookW)
{
#ifdef XBOX   
   a = (pAlphaCodebook[alphaCode] << 16) | pAlphaSelectorCodebookW[selectorCode];
#else
   a = pAlphaCodebook[alphaCode] | (pAlphaSelectorCodebookW[selectorCode] << 16);
#endif   
   b = pAlphaSelectorCodebookD[selectorCode];
}

//------------------------------------------------------------------------------------------------------
// UNPACK_DXT5Q_PACKET
//------------------------------------------------------------------------------------------------------
#define UNPACK_DXT5Q_PACKET(packet0Bits, packet1Bits) \
if (cLittleEndianNative) { EndianSwitchQWords(&packet0Bits, 1); EndianSwitchQWords(&packet1Bits, 1); } \
if ((int64)packet0Bits >= 0) \
{ \
   writeAlphaBlock(pDst, packet0Bits & 1023, (packet0Bits >> 10) & 2047, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
   pDst[2] = pColorCodebook[packet1Bits & 1023]; \
   pDst[3] = pColorSelectorCodebook[(packet1Bits >> 10) & 2047]; \
   writeAlphaBlock(pDst + 4, (packet0Bits >> 21) & 1023, (packet0Bits >> 31) & 2047, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
   pDst[6] = pColorCodebook[(packet1Bits >> 21) & 1023]; \
   pDst[7] = pColorSelectorCodebook[(packet1Bits >> 31) & 2047]; \
   writeAlphaBlock(pDst + 8, (packet0Bits >> 42) & 1023, (packet0Bits >> 52) & 2047, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
   pDst[10] = pColorCodebook[(packet1Bits >> 42) & 1023]; \
   pDst[11] = pColorSelectorCodebook[(packet1Bits >> 52) & 2047]; \
   pDst += 12; \
} \
else \
{ \
   if ((packet0Bits & 511) == 511) \
   { \
      uint runLen = (uint)((packet0Bits >> 9) & 511) + 4; \
      uint ac0, as0; \
      createAlphaBlock(ac0, as0, (packet0Bits >> 18) & 1023, (packet0Bits >> 28) & 2047, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
      const uint bc0 = pColorCodebook[(packet1Bits >> 18) & 1023]; \
      const uint bs0 = pColorSelectorCodebook[(packet1Bits >> 28) & 2047]; \
      while (runLen >= 4) \
      { \
         pDst[0] = ac0; pDst[1] = as0; pDst[2] = bc0; pDst[3] = bs0; \
         pDst[4] = ac0; pDst[5] = as0; pDst[6] = bc0; pDst[7] = bs0; \
         pDst[8] = ac0; pDst[9] = as0; pDst[10] = bc0; pDst[11] = bs0; \
         pDst[12] = ac0; pDst[13] = as0; pDst[14] = bc0; pDst[15] = bs0; \
         pDst += 16; \
         runLen -= 4; \
      } \
      while (runLen) \
      { \
         pDst[0] = ac0; pDst[1] = as0; pDst[2] = bc0; pDst[3] = bs0; \
         pDst += 4; \
         runLen--; \
      } \
      writeAlphaBlock(pDst, (packet0Bits >> 39) & 1023, (packet0Bits >> 49) & 2047, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
      pDst[2] = pColorCodebook[(packet1Bits >> 39) & 1023]; \
      pDst[3] = pColorSelectorCodebook[(packet1Bits >> 49) & 2047]; \
      pDst += 4; \
   } \
   else \
   { \
      uint runLen = (uint)((packet0Bits >> 60) & 7) + 1; \
      uint ac0, as0; \
      createAlphaBlock(ac0, as0, (packet0Bits      ) & 15, (packet0Bits >>  4) & 31, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
      const uint bc0 = pColorCodebook[(packet1Bits      ) & 15]; \
      const uint bs0 = pColorSelectorCodebook[(packet1Bits >>  4) & 31]; \
      while (runLen >= 4) \
      { \
         pDst[0] = ac0; pDst[1] = as0; pDst[2] = bc0; pDst[3] = bs0; \
         pDst[4] = ac0; pDst[5] = as0; pDst[6] = bc0; pDst[7] = bs0; \
         pDst[8] = ac0; pDst[9] = as0; pDst[10] = bc0; pDst[11] = bs0; \
         pDst[12] = ac0; pDst[13] = as0; pDst[14] = bc0; pDst[15] = bs0; \
         pDst += 16; \
         runLen -= 4; \
      } \
      while (runLen) \
      { \
         pDst[0] = ac0; pDst[1] = as0; pDst[2] = bc0; pDst[3] = bs0; \
         pDst += 4; \
         runLen--; \
      } \
      writeAlphaBlock(pDst, (packet0Bits >>  9) & 255, (packet0Bits >> 17) & 511, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
      pDst[2] = pColorCodebook[(packet1Bits >>  9) & 255]; \
      pDst[3] = pColorSelectorCodebook[(packet1Bits >> 17) & 511]; \
      writeAlphaBlock(pDst + 4, (packet0Bits >> 26) & 255, (packet0Bits >> 34) & 511, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
      pDst[6] = pColorCodebook[(packet1Bits >> 26) & 255]; \
      pDst[7] = pColorSelectorCodebook[(packet1Bits >> 34) & 511]; \
      writeAlphaBlock(pDst + 8, (packet0Bits >> 43) & 255, (packet0Bits >> 51) & 511, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
      pDst[10] = pColorCodebook[(packet1Bits >> 43) & 255]; \
      pDst[11] = pColorSelectorCodebook[(packet1Bits >> 51) & 511]; \
      pDst += 12; \
   }  \
}

//------------------------------------------------------------------------------------------------------
// BDXTQUnpack::decodeSegmentDXT5
//------------------------------------------------------------------------------------------------------
bool BDXTQUnpack::decodeSegmentDXT5(
   const uchar* __restrict pSrc, 
   const uint srcDataLen,
   DWORD* __restrict pDst, 
   DWORD* __restrict pDstEnd,
   const DWORD* __restrict pColorCodebook,
   const DWORD* __restrict pColorSelectorCodebook,
   const WORD* __restrict pAlphaCodebook,
   const DWORD* __restrict pAlphaSelectorCodebookD,
   const WORD* __restrict pAlphaSelectorCodebookW,
   bool writeCombinedDst)
{
#ifdef XBOX   
   BDEBUG_ASSERT(((uint)pSrc & 7) == 0);
#endif   
   
   const uchar* __restrict pSrcEndMinus4 = pSrc + srcDataLen - 16 * 4;
   const uchar* __restrict pSrcEnd = pSrc + srcDataLen;
   Utils::BPrefetchState srcPrefetch = Utils::BeginPrefetch(pSrc, pSrcEnd, 2);
   
   BWriteState dstWrite = BeginWrite(pDst, pDstEnd, 3);

   // This loop CANNOT process the last packet because this packet may contain up to 3 extra padding blocks.
   if (!writeCombinedDst)
   {
      while (pSrc < pSrcEndMinus4)
      {
         dstWrite = UpdateWrite(dstWrite, pDst, pDstEnd, 3);
         
         uint64 packet0 = *(const uint64*)pSrc;
         uint64 packet1 = *(const uint64*)(pSrc + 16*0 + 8);
         
         srcPrefetch = Utils::UpdatePrefetch(srcPrefetch, pSrc, pSrcEnd, 2);

         UNPACK_DXT5Q_PACKET(packet0, packet1)
         
         packet0 = *(const uint64*)(pSrc + 16*1);
         packet1 = *(const uint64*)(pSrc + 16*1 + 8);
         UNPACK_DXT5Q_PACKET(packet0, packet1)

         packet0 = *(const uint64*)(pSrc + 16*2);
         packet1 = *(const uint64*)(pSrc + 16*2 + 8);
         UNPACK_DXT5Q_PACKET(packet0, packet1)
         
         packet0 = *(const uint64*)(pSrc + 16*3);
         packet1 = *(const uint64*)(pSrc + 16*3 + 8);
         UNPACK_DXT5Q_PACKET(packet0, packet1)

         pSrc += 16 * 4;
      };
      
   }
   else
   {
      while (pSrc < pSrcEndMinus4)
      {
         uint64 packet0 = *(const uint64*)pSrc;
         uint64 packet1 = *(const uint64*)(pSrc + 16*0 + 8);
         
         srcPrefetch = Utils::UpdatePrefetch(srcPrefetch, pSrc, pSrcEnd, 2);

         UNPACK_DXT5Q_PACKET(packet0, packet1)
         
         packet0 = *(const uint64*)(pSrc + 16*1);
         packet1 = *(const uint64*)(pSrc + 16*1 + 8);
         UNPACK_DXT5Q_PACKET(packet0, packet1)

         packet0 = *(const uint64*)(pSrc + 16*2);
         packet1 = *(const uint64*)(pSrc + 16*2 + 8);
         UNPACK_DXT5Q_PACKET(packet0, packet1)
         
         packet0 = *(const uint64*)(pSrc + 16*3);
         packet1 = *(const uint64*)(pSrc + 16*3 + 8);
         UNPACK_DXT5Q_PACKET(packet0, packet1)

         pSrc += 16 * 4;
      };
   }      

   while (pSrc < pSrcEnd)
   {
      if (!writeCombinedDst)
         dstWrite = UpdateWrite(dstWrite, pDst, pDstEnd, 3);
         
      uint64 packet0Bits = *(const uint64*)pSrc;
      uint64 packet1Bits = *(const uint64*)(pSrc + 16*0 + 8);
      
      pSrc += 16;

      srcPrefetch = Utils::UpdatePrefetch(srcPrefetch, pSrc, pSrcEnd, 2);
                                         
      if (cLittleEndianNative) { EndianSwitchQWords(&packet0Bits, 1); EndianSwitchQWords(&packet1Bits, 1); } 
                  
      if ((int64)packet0Bits >= 0) 
      { 
         writeAlphaBlock(pDst, packet0Bits & 1023, (packet0Bits >> 10) & 2047, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW);
         pDst[2] = pColorCodebook[packet1Bits & 1023]; 
         pDst[3] = pColorSelectorCodebook[(packet1Bits >> 10) & 2047]; 
         pDst += 4;
         if (pDst < pDstEnd)
         {
            writeAlphaBlock(pDst, (packet0Bits >> 21) & 1023, (packet0Bits >> 31) & 2047, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW);
            pDst[2] = pColorCodebook[(packet1Bits >> 21) & 1023]; 
            pDst[3] = pColorSelectorCodebook[(packet1Bits >> 31) & 2047]; 
            pDst += 4;
            if (pDst < pDstEnd)
            {
               writeAlphaBlock(pDst, (packet0Bits >> 42) & 1023, (packet0Bits >> 52) & 2047, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW);
               pDst[2] = pColorCodebook[(packet1Bits >> 42) & 1023]; 
               pDst[3] = pColorSelectorCodebook[(packet1Bits >> 52) & 2047]; 
               pDst += 4; 
            }               
         }            
      } 
      else 
      { 
         if ((packet0Bits & 511) == 511) 
         { 
            uint runLen = (uint)((packet0Bits >> 9) & 511) + 4; 
            uint ac0, as0;
            createAlphaBlock(ac0, as0, (packet0Bits >> 18) & 1023, (packet0Bits >> 28) & 2047, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW);
            const uint bc0 = pColorCodebook[(packet1Bits >> 18) & 1023]; 
            const uint bs0 = pColorSelectorCodebook[(packet1Bits >> 28) & 2047]; 
            while (runLen >= 4) 
            { 
               pDst[0] = ac0; pDst[1] = as0; pDst[2] = bc0; pDst[3] = bs0; 
               pDst[4] = ac0; pDst[5] = as0; pDst[6] = bc0; pDst[7] = bs0; 
               pDst[8] = ac0; pDst[9] = as0; pDst[10] = bc0; pDst[11] = bs0; 
               pDst[12] = ac0; pDst[13] = as0; pDst[14] = bc0; pDst[15] = bs0; 
               pDst += 16; 
               runLen -= 4; 
            } 
            while (runLen) 
            { 
               pDst[0] = ac0; pDst[1] = as0; pDst[2] = bc0; pDst[3] = bs0; 
               pDst += 4; 
               runLen--; 
            } 
            if (pDst < pDstEnd)
            {
               writeAlphaBlock(pDst, (packet0Bits >> 39) & 1023, (packet0Bits >> 49) & 2047, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW);
               pDst[2] = pColorCodebook[(packet1Bits >> 39) & 1023]; 
               pDst[3] = pColorSelectorCodebook[(packet1Bits >> 49) & 2047]; 
               pDst += 4; 
            }               
         } 
         else 
         { 
            uint runLen = (uint)((packet0Bits >> 60) & 7) + 1; 
            uint ac0, as0;
            createAlphaBlock(ac0, as0, (packet0Bits      ) & 15, (packet0Bits >>  4) & 31, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW);
            const uint bc0 = pColorCodebook[(packet1Bits      ) & 15]; 
            const uint bs0 = pColorSelectorCodebook[(packet1Bits >>  4) & 31]; 
            while (runLen >= 4) 
            { 
               pDst[0] = ac0; pDst[1] = as0; pDst[2] = bc0; pDst[3] = bs0; 
               pDst[4] = ac0; pDst[5] = as0; pDst[6] = bc0; pDst[7] = bs0; 
               pDst[8] = ac0; pDst[9] = as0; pDst[10] = bc0; pDst[11] = bs0; 
               pDst[12] = ac0; pDst[13] = as0; pDst[14] = bc0; pDst[15] = bs0; 
               pDst += 16; 
               runLen -= 4; 
            } 
            while (runLen) 
            { 
               pDst[0] = ac0; pDst[1] = as0; pDst[2] = bc0; pDst[3] = bs0; 
               pDst += 4; 
               runLen--; 
            } 
            if (pDst < pDstEnd)
            {
               writeAlphaBlock(pDst, (packet0Bits >>  9) & 255, (packet0Bits >> 17) & 511, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW);
               pDst[2] = pColorCodebook[(packet1Bits >>  9) & 255]; 
               pDst[3] = pColorSelectorCodebook[(packet1Bits >> 17) & 511]; 
               pDst += 4; 
               if (pDst < pDstEnd)
               {
                  writeAlphaBlock(pDst, (packet0Bits >> 26) & 255, (packet0Bits >> 34) & 511, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW);
                  pDst[2] = pColorCodebook[(packet1Bits >> 26) & 255]; 
                  pDst[3] = pColorSelectorCodebook[(packet1Bits >> 34) & 511]; 
                  pDst += 4; 
                  if (pDst < pDstEnd)
                  {
                     writeAlphaBlock(pDst, (packet0Bits >> 43) & 255, (packet0Bits >> 51) & 511, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW);
                     pDst[2] = pColorCodebook[(packet1Bits >> 43) & 255]; 
                     pDst[3] = pColorSelectorCodebook[(packet1Bits >> 51) & 511]; 
                     pDst += 4; 
                  }                     
               }                  
            }               
         }  
      }
   };
   
   return (pDst == pDstEnd);  
}   

//------------------------------------------------------------------------------------------------------
// writeAlphaBlock
//------------------------------------------------------------------------------------------------------
__forceinline void writeAlphaBlockN(
   DWORD* __restrict pDst, 
   uint alphaCode, uint selectorCode, 
   const WORD* __restrict pAlphaCodebook, const DWORD* __restrict pAlphaSelectorCodebookD, const WORD* __restrict pAlphaSelectorCodebookW)
{
#ifdef XBOX   
   pDst[0] = (pAlphaCodebook[alphaCode] << 16) | pAlphaSelectorCodebookW[selectorCode];
#else
   pDst[0] = pAlphaCodebook[alphaCode] | (pAlphaSelectorCodebookW[selectorCode] << 16);
#endif   
   pDst[1] = pAlphaSelectorCodebookD[selectorCode];
}

//------------------------------------------------------------------------------------------------------
// createAlphaBlock
//------------------------------------------------------------------------------------------------------
__forceinline void createAlphaBlockN(
   uint& a, uint& b,
   uint alphaCode, uint selectorCode,
   const WORD* __restrict pAlphaCodebook, const DWORD* __restrict pAlphaSelectorCodebookD, const WORD* __restrict pAlphaSelectorCodebookW)
{
#ifdef XBOX   
   a = (pAlphaCodebook[alphaCode] << 16) | pAlphaSelectorCodebookW[selectorCode];
#else
   a = pAlphaCodebook[alphaCode] | (pAlphaSelectorCodebookW[selectorCode] << 16);
#endif   
   b = pAlphaSelectorCodebookD[selectorCode];
}

#define UNPACK_DXNQ_PACKET(packet0Bits, packet1Bits, packet2Bits) \
if (cLittleEndianNative) { EndianSwitchQWords(&packet0Bits, 1); EndianSwitchQWords(&packet1Bits, 1); EndianSwitchQWords(&packet2Bits, 1); } \
int encodedRunLen = (uint)(packet2Bits >> 57); \
if (!encodedRunLen) \
{ \
   writeAlphaBlockN(pDst     , (packet0Bits        & 2047), ((packet0Bits >> 11) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
   writeAlphaBlockN(pDst + 2 , (packet0Bits >> 23) & 2047,  ((packet0Bits >> 34) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
   writeAlphaBlockN(pDst + 4 , (packet1Bits        & 2047), ((packet1Bits >> 11) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
   writeAlphaBlockN(pDst + 6 , (packet1Bits >> 23) & 2047,  ((packet1Bits >> 34) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
   writeAlphaBlockN(pDst + 8 , (packet2Bits        & 2047), ((packet2Bits >> 11) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
   writeAlphaBlockN(pDst + 10, (packet2Bits >> 23) & 2047,  ((packet2Bits >> 34) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
   writeAlphaBlockN(pDst + 12, (packet0Bits >> 46) & 2047,  ((packet1Bits >> 46) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
   writeAlphaBlockN(pDst + 14, (packet2Bits >> 46) & 2047,  ((packet0Bits >> 57) | ((packet1Bits >> 58) << 6)), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
   pDst += 16; \
} else { \
   uint a0, a1, b0, b1; \
   createAlphaBlockN(a0, a1, (packet0Bits & 2047), ((packet0Bits >> 11) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
   createAlphaBlockN(b0, b1, (packet0Bits >> 23) & 2047, ((packet0Bits >> 34) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
   while (encodedRunLen >= 0) { \
      pDst[0] = a0; \
      pDst[1] = a1; \
      pDst[2] = b0; \
      pDst[3] = b1; \
      pDst += 4; \
      encodedRunLen--; \
   }; \
   writeAlphaBlockN(pDst, (packet1Bits & 2047), ((packet1Bits >> 11) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
   writeAlphaBlockN(pDst + 2, (packet1Bits >> 23) & 2047, ((packet1Bits >> 34) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
   writeAlphaBlockN(pDst + 4, (packet2Bits & 2047), ((packet2Bits >> 11) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
   writeAlphaBlockN(pDst + 6, (packet2Bits >> 23) & 2047, ((packet2Bits >> 34) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
   writeAlphaBlockN(pDst + 8, (packet0Bits >> 46) & 2047, ((packet1Bits >> 46) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
   writeAlphaBlockN(pDst + 10, (packet2Bits >> 46) & 2047, ((packet0Bits >> 57) | ((packet1Bits >> 58) << 6)), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
   pDst += 12; \
}   

static uint __declspec(naked) decodeSegmentDXNAsm(
   uint ptrTopSrc,                  // r3
   uint pSrcEndMinus1,              // r4
   uint ptrTopDst,                  // r5
   uint pDstEnd,                    // r6
   uint pAlphaCodebook,             // r7
   uint pAlphaSelectorCodebookD,    // r8
   uint pAlphaSelectorCodebookW)    // r9
{
#define PTRTOPSRC          r3
#define PSRCENDMINUS1      r4
#define PTRTOPDST          r5
#define PDSTEND            r6
#define PALPHACODEBOOK     r7
#define PSELECTORCODEBOOKD r8
#define PSELECTORCODEBOOKW r9

   __asm
   {
      mflr         r12
      bl           __savegprlr
         
      //add r3, r3, r4
      //rlwinm      r3, r3, 1, 30, 31
      
      // 3-10 params    volatile
      // 11-12 scratch  volatile
      // 14-31          nonvolatile
      // 27 regs
      
      // avail: 10, 11, 16-27
                  
      // r14 pSrc
      // r15 pDst
      // r27 prevpDst
      // r29-r31 packetBits
      // r28 runLen

#define PACKET0BITS        r29
#define PACKET1BITS        r30
#define PACKET2BITS        r31

#define PSRC r14
#define PDST r15
#define PDSTPREV r27
      
      lwz         PSRC, 0(PTRTOPSRC)
      lwz         PDST, 0(PTRTOPDST)
      
      cmpw        cr6, PSRC, PSRCENDMINUS1
      bge         cr6, DXNAsmRet
      
      mr          PDSTPREV, PDST
                        
      nopalign    8

DXNAsmPacketLoop:  

      xor         r25, PDST, PDSTPREV
      ld          PACKET2BITS, 16(PSRC)
      
      li          r24, 256
      dcbt        r24, PSRC
      
      ld          PACKET1BITS, 8(PSRC)                     
      clrrwi      r25, r25, 7
      
      ld          PACKET0BITS, 0(PSRC)         
      rldicl      r28, PACKET2BITS, 7, 57      //srdi.       r28, PACKET2BITS, 57
      
      cmplwi      cr6, r25, 0
      beq         cr6, DXNAsmNoDstPrefetch
      
      dcbf        r0, PDSTPREV
      
      mr          PDSTPREV, PDST
      
      addi        r25, PDST, 127
      clrrwi      r25, r25, 7
      addi        r25, r25, 384
      
      addi        r26, r25, 127
      cmpw        cr6, r26, PDSTEND
      bge         DXNAsmNoDstPrefetch
      
      dcbz128     r0, r25
      
      nopalign    8
      
DXNAsmNoDstPrefetch:
           
                        
      //                      11111111111    
      //                     111111             
      // 00000000*0000000*0000000*0000000
      // 32      40      48      56    63      
      // 0       8       16      24    31
      //
      //
      //                                                     11111111111
      //                                                   111111111111                      
      // rrrrrrrfffffffffffeeeeeeeeeeeecccccccccccbbbbbbbbbbbbaaaaaaaaaaa
      // 00000000*0000000*0000000*0000000*0000000*0000000*0000000*0000000
      //                                 32      40      48      56    63      
      // 0       8       16      24    31
      
      // PACKET0BITS - packet0Bits
      // PACKET1BITS - packet1Bits
      // PACKET2BITS - packet2Bits
      
      // block0: r18, r19, r20, r21, r22, r23
      // block1: r10, r11, r16, r24, r25, r26
      
      // writeAlphaBlockN(pDst     , (packet0Bits        & 2047), ((packet0Bits >> 11) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
      // writeAlphaBlockN(pDst + 2 , (packet0Bits >> 23) & 2047,  ((packet0Bits >> 34) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
      // writeAlphaBlockN(pDst + 4 , (packet1Bits        & 2047), ((packet1Bits >> 11) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
      // writeAlphaBlockN(pDst + 6 , (packet1Bits >> 23) & 2047,  ((packet1Bits >> 34) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \

      //------------------------------------------------------- Blocks 0-1
      
      cmplwi       cr6, r28, 0
      
      rlwinm       r18, PACKET0BITS, 1, 20, 30    // B0AlphaCode0*2             
      
      rlwinm       r19, PACKET0BITS, 22, 19, 30   // B0SelectorCode0*2                      
      
      lhzx         r18, r18, PALPHACODEBOOK  // pAlphaCodebook[B0AlphaCode0*2]      
      rlwinm       r20, PACKET0BITS, 23, 18, 29   // B0SelectorCode0*4
      
      lhzx         r19, r19, PSELECTORCODEBOOKW  // pAlphaSelectorCodebokW[B0SelectorCode0*2]
      rldicl       r21, PACKET0BITS, 41, 23         // SHR 23 B0AlphaCode1
      
      lwzx         r20, r20, PSELECTORCODEBOOKD // pAlphaSelectorCodebokD[B0SelectorCode0*4]
      rldicl       r22, PACKET0BITS, 30, 34       // SHR 34 B0SelectorCode1          

      rlwinm       r10, PACKET1BITS, 1, 20, 30    // B1AlphaCode0*2             
      
      rlwinm       r11, PACKET1BITS, 22, 19, 30   // B1SelectorCode0*2                      
      
      lhzx         r10, r10, PALPHACODEBOOK  // pAlphaCodebook[B1AlphaCode0*2]      
      rlwinm       r16, PACKET1BITS, 23, 18, 29   // B1SelectorCode0*4
      
      lhzx         r11, r11, PSELECTORCODEBOOKW  // pAlphaSelectorCodebokW[B1SelectorCode0*2]
      rldicl       r24, PACKET1BITS, 41, 23         // SHR 23 B1AlphaCode1
      
      lwzx         r16, r16, PSELECTORCODEBOOKD // pAlphaSelectorCodebokD[B1SelectorCode0*4]
      rldicl       r25, PACKET1BITS, 30, 34      // SHR 34 B1SelectorCode1          

      rlwinm       r21, r21, 1, 20, 30    // B0AlphaCode1*2
      
      rlwinm       r23, r22, 1, 19, 30 // B0SelectorCode1*2
      
      lhzx         r21, r21, PALPHACODEBOOK // pAlphaCodebook[B0AlphaCode1*2]
      rlwinm       r22, r22, 2, 18, 29 // B0SelectorCode1*4

      lhzx         r23, r23, PSELECTORCODEBOOKW // pAlphaSelectorCodebookW[B0SelectorCode1*2]
      rlwinm       r24, r24, 1, 20, 30    // B1AlphaCode1*2
      
      lwzx         r22, r22, PSELECTORCODEBOOKD // pAlphaSelectorCodebookD[B0SelectorCode1*4]                        
      rlwinm       r26, r25, 1, 19, 30 // B1SelectorCode1*2
      
      lhzx         r24, r24, PALPHACODEBOOK // pAlphaCodebook[B1AlphaCode1*2]
      rlwinm       r25, r25, 2, 18, 29 // B1SelectorCode1*4
            
      lwzx         r25, r25, PSELECTORCODEBOOKD // pAlphaSelectorCodebookD[B1SelectorCode1*4]                        
      rotlwi       r18, r18, 16  // pAlphaCodebook[B0AlphaCode0*2] << 16    
      
      lhzx         r26, r26, PSELECTORCODEBOOKW // pAlphaSelectorCodebookW[B1SelectorCode1*2]
      rotlwi       r21, r21, 16  // pAlphaCodebook[B0AlphaCode1*2] << 16  

      rotlwi       r10, r10, 16  // pAlphaCodebook[B1AlphaCode0*2] << 16    
      or           r19, r19, r18  //B0    
      
      rotlwi       r24, r24, 16  // pAlphaCodebook[B1AlphaCode1*2] << 16  
      
      stw          r19, 0(PDST)   //B0
      or           r23, r23, r21  //B0
      
      stw          r20, 4(PDST)   //B0
      or           r11, r11, r10  //B1
      
      stw          r23, 8(PDST)   //B0
      or           r26, r26, r24  //B1
      
      stw          r22, 12(PDST)  //B0
      bne          cr6, DXNAsmProcessRun

DXNAsmDoneRun:      
      stw          r11, 16(PDST)  //B1
      stw          r16, 20(PDST)  //B1
      stw          r26, 24(PDST)  //B1
      stw          r25, 28(PDST)  //B1            
      
      //------------------------------------------------------- Blocks 2-3
      // block2: r18, r19, r20, r21, r22, r23
      
      // writeAlphaBlockN(pDst + 8 , (packet2Bits        & 2047), ((packet2Bits >> 11) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
      // writeAlphaBlockN(pDst + 10, (packet2Bits >> 23) & 2047,  ((packet2Bits >> 34) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
      
      rlwinm       r18, PACKET2BITS, 1, 20, 30    // B2AlphaCode0*2             
      rlwinm       r19, PACKET2BITS, 22, 19, 30   // B2SelectorCode0*2                      
      rlwinm       r20, PACKET2BITS, 23, 18, 29   // B2SelectorCode0*4
      
      rldicl       r21, PACKET2BITS, 41, 23       // SHR 23 B2AlphaCode1
      
      rldicl       r22, PACKET2BITS, 30, 34       // 34 B2SelectorCode1          
      
      rlwinm       r21, r21, 1, 20, 30    // B2AlphaCode1*2
      rlwinm       r23, r22, 1, 19, 30    // B2SelectorCode1*2
      rlwinm       r22, r22, 2, 18, 29    // B2SelectorCode1*4
      
      lhzx         r18, r18, PALPHACODEBOOK        // pAlphaCodebook[B2AlphaCode0*2]      
      rotlwi       r18, r18, 16                    // pAlphaCodebook[B2AlphaCode0*2] << 16  
      lhzx         r19, r19, PSELECTORCODEBOOKW    // pSelectorCodebookW[B2SelectorCode0*2]
      or           r19, r19, r18                   // B2_0 first DWORD
      lwzx         r20, r20, PSELECTORCODEBOOKD    // B2_0 pSelectorCodebookD[B2SelectorCode0*4]
      
      lhzx         r21, r21, PALPHACODEBOOK        // pAlphaCodebook[B2AlphaCode1*2]      
      rotlwi       r21, r21, 16                    // pAlphaCodebook[B2AlphaCode1*2] << 16  
      lhzx         r23, r23, PSELECTORCODEBOOKW    // pSelectorCodebookW[B2SelectorCode1*2]
      or           r23, r23, r21                   // B2_1 first DWORD
      lwzx         r22, r22, PSELECTORCODEBOOKD    // B2_1 pSelectorCodebookD[B2SelectorCode1*4]
      
      // block3: r10, r11, r16, r24, r25, r26
      
      //writeAlphaBlockN(pDst + 12, (packet0Bits >> 46) & 2047,  ((packet1Bits >> 46) & 4095), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \
      
      rldicl       r10, PACKET0BITS, 18, 46       // SHR B3AlphaCode0
      
      rldicl       r11, PACKET1BITS, 18, 46         // SHR B3SelectorCode0
      
      rlwinm       r10, r10, 1, 20, 30    // B3AlphaCode0*2
      rlwinm       r16, r11, 1, 19, 30    // B3SelectorCode0*2
      rlwinm       r11, r11, 2, 18, 29    // B3SelectorCode0*4

      // writeAlphaBlockN(pDst + 14, (packet2Bits >> 46) & 2047,  ((packet0Bits >> 57) | ((packet1Bits >> 58) << 6)), pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW); \      
      
      rldicl       r24, PACKET2BITS, 18, 46         // SHR B3AlphaCode1
            
      rldicl       r25, PACKET0BITS, 7, 57         // SHR 57
      
      rldicl       r26, PACKET1BITS, 6, 58         // SHR 58
      
      rlwinm       r26, r26, 6, 20, 25
      or           r26, r26, r25                 // B3SelectorCode1
      
      rlwinm       r24, r24, 1, 20, 30    // B3AlphaCode1*2
      rlwinm       r25, r26, 1, 19, 30    // B3SelectorCode1*2
      rlwinm       r26, r26, 2, 18, 29    // B3SelectorCode1*4
      
      lhzx         r10, r10, PALPHACODEBOOK        // pAlphaCodebook[B3AlphaCode0*2]      
      rotlwi       r10, r10, 16                    // pAlphaCodebook[B3AlphaCode0*2] << 16  
      lhzx         r16, r16, PSELECTORCODEBOOKW    // pSelectorCodebookW[B3SelectorCode0*2]
      or           r10, r10, r16                   // B3_0 first DWORD
      lwzx         r11, r11, PSELECTORCODEBOOKD    // B3_0 pSelectorCodebookD[B3SelectorCode0*4]

      lhzx         r24, r24, PALPHACODEBOOK        // pAlphaCodebook[B3AlphaCode1*2]      
      rotlwi       r24, r24, 16                    // pAlphaCodebook[B3AlphaCode1*2] << 16  
      lhzx         r25, r25, PSELECTORCODEBOOKW    // pSelectorCodebookW[B3SelectorCode1*2]
      or           r24, r24, r25                   // B3_1 first DWORD
      lwzx         r26, r26, PSELECTORCODEBOOKD    // B3_1 pSelectorCodebookD[B3SelectorCode1*4]
      
      stw          r19, 32(PDST)   //B2_0
      stw          r20, 36(PDST)   //B2_0
      stw          r23, 40(PDST)   //B2_1
      stw          r22, 44(PDST)   //B2_1
      
      stw          r10, 48(PDST)   //B3_0
      stw          r11, 52(PDST)   //B3_0
      stw          r24, 56(PDST)   //B3_1
      stw          r26, 60(PDST)   //B3_2
      
      addi         PSRC, PSRC, 24
      addi         PDST, PDST, 64
      
      cmpw         cr6, PSRC, PSRCENDMINUS1
      blt          cr6, DXNAsmPacketLoop

DXNAsmRet:      
      stw         PSRC, 0(PTRTOPSRC)
      stw         PDST, 0(PTRTOPDST)

      bl          __restgprlr
      
DXNAsmProcessRun:
      // r28 runLen

      addi         PDST, PDST, 16

      subi         r28, r28, 1
      cmplwi       cr6, r28, 0

      stw          r19, 0(PDST)   //B0
      stw          r20, 4(PDST)   //B0
      stw          r23, 8(PDST)   //B0
      stw          r22, 12(PDST)  //B0

      bne          cr6, DXNAsmProcessRun      
      
      b            DXNAsmDoneRun
   }
}

//------------------------------------------------------------------------------------------------------
// BDXTQUnpack::decodeSegmentDXN
//------------------------------------------------------------------------------------------------------
//__declspec(noalias) 
bool BDXTQUnpack::decodeSegmentDXN(
   const uchar* __restrict pSrc, 
   const uint srcDataLen,
   DWORD* __restrict pDst, 
   DWORD* __restrict pDstEnd,
   const WORD* __restrict pAlphaCodebook,
   const DWORD* __restrict pAlphaSelectorCodebookD,
   const WORD* __restrict pAlphaSelectorCodebookW,
   bool writeCombinedDst)
{
#ifdef XBOX   
   BDEBUG_ASSERT(((uint)pSrc & 7) == 0);
#endif   
   
   const uchar* __restrict pSrcEndMinus1 = pSrc + srcDataLen - 24;
   const uchar* __restrict pSrcEnd = pSrc + srcDataLen;
   Utils::BPrefetchState srcPrefetch = Utils::BeginPrefetch(pSrc, pSrcEndMinus1, 2);
   
   BWriteState dstWrite = BeginWrite(pDst, pDstEnd, 3);

   // This loop CANNOT process the last packet because this packet may contain extra padding blocks.
   if (!writeCombinedDst)
   {
#if 1
      decodeSegmentDXNAsm(
         (uint)&pSrc,                
         (uint)pSrcEndMinus1,                 
         (uint)&pDst,                
         (uint)pDstEnd,              
         (uint)pAlphaCodebook,       
         (uint)pAlphaSelectorCodebookD,
         (uint)pAlphaSelectorCodebookW);
#else
      while (pSrc < pSrcEndMinus1)
      {
         dstWrite = UpdateWrite(dstWrite, pDst, pDstEnd, 3);
         
         uint64 packet2Bits = *(const uint64*)(pSrc + 16);
         uint64 packet1Bits = *(const uint64*)(pSrc + 8);
         uint64 packet0Bits = *(const uint64*)pSrc;
         
         srcPrefetch = Utils::UpdatePrefetch(srcPrefetch, pSrc, pSrcEnd, 2);
         
         UNPACK_DXNQ_PACKET(packet0Bits, packet1Bits, packet2Bits)
         
         pSrc += 24;
      }
#endif      
   }
   else
   {
      while (pSrc < pSrcEndMinus1)
      {
         uint64 packet2Bits = *(const uint64*)(pSrc + 16);
         uint64 packet1Bits = *(const uint64*)(pSrc + 8);
         uint64 packet0Bits = *(const uint64*)pSrc;

         srcPrefetch = Utils::UpdatePrefetch(srcPrefetch, pSrc, pSrcEnd, 2);

         UNPACK_DXNQ_PACKET(packet0Bits, packet1Bits, packet2Bits)

         pSrc += 24;
      }
   }

   while (pSrc < pSrcEnd)
   {
      if (!writeCombinedDst)
         dstWrite = UpdateWrite(dstWrite, pDst, pDstEnd, 3);
         
      uint64 packet2Bits = *(const uint64*)(pSrc + 16);
      uint64 packet1Bits = *(const uint64*)(pSrc + 8);
      uint64 packet0Bits = *(const uint64*)pSrc;
      
      if (cLittleEndianNative)
      {
         EndianSwitchQWords(&packet0Bits, 1);
         EndianSwitchQWords(&packet1Bits, 1);
         EndianSwitchQWords(&packet2Bits, 1);
      }
      
      srcPrefetch = Utils::UpdatePrefetch(srcPrefetch, pSrc, pSrcEnd, 2);
      
      pSrc += 24;
      
      uint encodedRunLen = (uint)(packet2Bits >> 57) + 1;

      const uint alpha0Index0 = (uint)(packet0Bits & 2047);
      const uint alpha1Index0 = (uint)(packet0Bits >> 23) & 2047;
      const uint selector0Index0 = (uint)(packet0Bits >> 11) & 4095;
      const uint selector1Index0 = (uint)(packet0Bits >> 34) & 4095;

      uint a0, a1, b0, b1;
      createAlphaBlock(a0, a1, alpha0Index0, selector0Index0, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW);
      createAlphaBlock(b0, b1, alpha1Index0, selector1Index0, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW);

      do 
      {
         pDst[0] = a0;
         pDst[1] = a1;
         pDst[2] = b0;
         pDst[3] = b1;
         pDst += 4;
         encodedRunLen--;
      } while (encodedRunLen);

      if (pDst < pDstEnd)
      {
         const uint alpha0Index1 = (uint)(packet1Bits & 2047);
         const uint alpha1Index1 = (uint)(packet1Bits >> 23) & 2047;
         const uint selector0Index1 = (uint)(packet1Bits >> 11) & 4095;
         const uint selector1Index1 = (uint)(packet1Bits >> 34) & 4095;
         writeAlphaBlock(pDst, alpha0Index1, selector0Index1, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW);
         writeAlphaBlock(pDst + 2, alpha1Index1, selector1Index1, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW);
         pDst += 4;

         if (pDst < pDstEnd)
         {
            const uint alpha0Index2 = (uint)(packet2Bits & 2047);
            const uint alpha1Index2 = (uint)(packet2Bits >> 23) & 2047;
            const uint selector0Index2 = (uint)(packet2Bits >> 11) & 4095;
            const uint selector1Index2 = (uint)(packet2Bits >> 34) & 4095;
            writeAlphaBlock(pDst, alpha0Index2, selector0Index2, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW);
            writeAlphaBlock(pDst + 2, alpha1Index2, selector1Index2, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW);
            pDst += 4;
            
            if (pDst < pDstEnd)
            {
               const uint alpha0Index3 = (uint)(packet0Bits >> 46) & 2047;
               const uint alpha1Index3 = (uint)(packet2Bits >> 46) & 2047;
               const uint selector0Index3 = (uint)(packet1Bits >> 46) & 4095;
               const uint selector1Index3 = (uint)((packet0Bits >> 57) | ((packet1Bits >> 58) << 6));
               writeAlphaBlock(pDst, alpha0Index3, selector0Index3, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW);
               writeAlphaBlock(pDst + 2, alpha1Index3, selector1Index3, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW);
               pDst += 4;
            }
         }                           
      }
   };
   
   return (pDst == pDstEnd);  
}   

//------------------------------------------------------------------------------------------------------
// BDXTQUnpack::decodeSegment
//------------------------------------------------------------------------------------------------------
bool BDXTQUnpack::decodeSegment(
   BYTE* __restrict pDst,
   const BDXTQHeader& header,
   const BYTE* pCachedMem,
   const BDXTQHeader::BSegment& segment,
   eDDXDataFormat dataFormat,
   bool endianSwap)      
{
   const uint bytesPerBlock = (dataFormat == cDDXDataFormatDXT1Q) ? 8 : 16;
   
   DWORD* __restrict pColorCodebook                  = (DWORD*)(pCachedMem + header.mColorCodebookOfs);
   DWORD* __restrict pColorSelectorCodebook          = (DWORD*)(pCachedMem + header.mColorSelectorCodebookOfs);

   WORD* __restrict pAlphaCodebook                  = (WORD*)(pCachedMem + header.mAlphaCodebookOfs);
   DWORD* __restrict pAlphaSelectorCodebookD        = (DWORD*)(pCachedMem + header.mAlphaSelectorCodebookOfs);
   WORD* __restrict pAlphaSelectorCodebookW         = (WORD*)(pCachedMem + header.mAlphaSelectorCodebookOfs + header.mAlphaSelectorCodebookSize * sizeof(DWORD));
   
   const uchar* __restrict pSegSrc = pCachedMem + segment.mDataOfs;
      
   DWORD* __restrict pSegDst = (DWORD*)(pDst + segment.mFirstBlock * bytesPerBlock);
   DWORD* __restrict pSegDstEnd = (DWORD*)((uchar*)pSegDst + segment.mNumBlocks * bytesPerBlock);
   
   const bool writeCombinedDst = false;
   
   bool result = false;
   if (dataFormat == cDDXDataFormatDXT1Q)
   {
      result = decodeSegmentDXT1(pSegSrc, segment.mDataLen, pSegDst, pSegDstEnd, pColorCodebook, pColorSelectorCodebook, writeCombinedDst);
   }
   else if (dataFormat == cDDXDataFormatDXNQ)
   {
      result = decodeSegmentDXN(pSegSrc, segment.mDataLen, pSegDst, pSegDstEnd, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW, writeCombinedDst);
   }
   else
   {
      result = decodeSegmentDXT5(pSegSrc, segment.mDataLen, pSegDst, pSegDstEnd, pColorCodebook, pColorSelectorCodebook, pAlphaCodebook, pAlphaSelectorCodebookD, pAlphaSelectorCodebookW, writeCombinedDst);
   }
         
   if (!result)
      return false;
           
   if (endianSwap)
      EndianSwitchWords((WORD*)(pDst + segment.mFirstBlock * bytesPerBlock), (segment.mNumBlocks * bytesPerBlock) >> 1);
      
   return true;      
}   

#ifdef XBOX
//------------------------------------------------------------------------------------------------------
// struct BDecodeWorkItem
//------------------------------------------------------------------------------------------------------
struct BDecodeWorkItem
{
   BDecodeWorkItem() { }
   
   BDecodeWorkItem(BYTE* __restrict pDst, const BDXTQHeader* pHeader, const BYTE* pCachedMem, const BDXTQHeader::BSegment* pSegment, eDDXDataFormat dataFormat, bool endianSwap) : 
      mpDst(pDst), mpHeader(pHeader), mpCachedMem(pCachedMem), mpSegment(pSegment), mDataFormat(dataFormat), mEndianSwap(endianSwap)
   {
   }         
   
   BYTE* __restrict              mpDst;
   const BDXTQHeader*            mpHeader;
   const BYTE*                   mpCachedMem;
   const BDXTQHeader::BSegment*  mpSegment;
   eDDXDataFormat                mDataFormat;
   bool                          mEndianSwap : 1;
};

//------------------------------------------------------------------------------------------------------
// decodeSegmentWorkFunc
//------------------------------------------------------------------------------------------------------
static void decodeSegmentWorkFunc(void* privateData0, uint64 privateData1, uint workBucketIndex, bool lastWorkEntryInBucket)
{
   BScopedPIXNamedEvent pixEvent("DXTQPack::decodeSegmentWorkFunc");
   
   const BDecodeWorkItem* pWorkItem = static_cast<const BDecodeWorkItem*>(privateData0);
   BCountDownEvent* pCountDownEvent = reinterpret_cast<BCountDownEvent*>(privateData1);

   BDXTQUnpack::decodeSegment(
      pWorkItem->mpDst,
      *pWorkItem->mpHeader,
      pWorkItem->mpCachedMem,
      *pWorkItem->mpSegment,
      pWorkItem->mDataFormat,
      pWorkItem->mEndianSwap);

   pCountDownEvent->decrement();
}
#endif

//------------------------------------------------------------------------------------------------------
// BDXTQUnpack::unpackDXTQToDXT
//------------------------------------------------------------------------------------------------------
bool BDXTQUnpack::unpackDXTQToDXT(
   const BDDXTextureInfo& srcTextureInfo,
   const BYTE* pSrcData, uint srcDataSize,
   BDDXTextureInfo& dstTextureInfo, BByteArray& dstData,
   bool platformSpecificData)
{
   const uint width = srcTextureInfo.mWidth;
   const uint height = srcTextureInfo.mHeight;
   const uint numMipChainLevels = srcTextureInfo.mNumMipChainLevels;
   const eDDXDataFormat format = srcTextureInfo.mDataFormat;

   const BECFFileReader ecfReader(BConstDataBuffer(pSrcData, srcDataSize));
   
   if (!ecfReader.checkHeader(true))
      return false;
   
   if (ecfReader.getHeader()->getID() != cDXTQID)
      return false;
   
   if (ecfReader.getHeader()->getNumChunks() < 2)
      return false;
   
   if (ecfReader.getChunkDataLenByIndex(0) != sizeof(BDXTQHeader))
      return false;
   const BDXTQHeader& header = *(const BDXTQHeader*)ecfReader.getChunkDataByIndex(0);
   if (header.mVersion != BDXTQHeader::cVersion)
      return false;
   
   const BYTE* pCachedMem =  ecfReader.getChunkDataByIndex(1);  
   const uint cachedMemSize = ecfReader.getChunkDataLenByIndex(1);
   
   if (!cachedMemSize)
      return false;

   if ((cachedMemSize - header.mD3DTexOfs) < sizeof(IDirect3DTexture9))
      return false;
      
   IDirect3DTexture9 d3dTex = *(const IDirect3DTexture9*)(pCachedMem + header.mD3DTexOfs);
   if (cLittleEndianNative)
      XGEndianSwapMemory(&d3dTex, &d3dTex, XGENDIAN_8IN32, sizeof(DWORD), sizeof(IDirect3DTexture9) / sizeof(DWORD));
      
   XGTEXTURE_DESC d3dTexDesc;
   XGGetTextureDesc(&d3dTex, 0, &d3dTexDesc);
   const DWORD dwNumLevels = d3dTex.Format.MaxMipLevel + 1;

   if ((d3dTexDesc.Width != width) || (d3dTexDesc.Height != height) || (dwNumLevels != (numMipChainLevels + 1U)))
      return false;
      
   const D3DFORMAT d3dFormat = getD3DDataFormatFromDDX(format);
   if (d3dFormat == D3DFMT_UNKNOWN)
      return false;
   if (d3dTexDesc.Format != d3dFormat)    
      return false;
   
   const uint bytesPerBlock = getDDXDataFormatDXTBlockSize(format);
   const uint d3dTexBaseSize = header.mBaseBlocks * bytesPerBlock;
   const uint d3dTexMipSize = header.mMipBlocks * bytesPerBlock;
   d3dTexMipSize;
   const uint totalBlocks = header.mBaseBlocks + header.mMipBlocks;
   
   dstTextureInfo = srcTextureInfo;
      
   switch (srcTextureInfo.mDataFormat)
   {
      case cDDXDataFormatDXT1Q:  dstTextureInfo.mDataFormat = cDDXDataFormatDXT1;  break;
      case cDDXDataFormatDXT5Q:  dstTextureInfo.mDataFormat = cDDXDataFormatDXT5;  dstTextureInfo.mHasAlpha = true; break;
      case cDDXDataFormatDXT5HQ: dstTextureInfo.mDataFormat = cDDXDataFormatDXT5H; dstTextureInfo.mHasAlpha = true; break;
      case cDDXDataFormatDXT5YQ: dstTextureInfo.mDataFormat = cDDXDataFormatDXT5Y; dstTextureInfo.mHasAlpha = true; break;
      case cDDXDataFormatDXNQ:   dstTextureInfo.mDataFormat = cDDXDataFormatDXN;   break;
      default:
         return false;
   }
   
   dstTextureInfo.mOrigDataFormat = format;
   dstTextureInfo.mPackType = cDDXTDPTMipsRaw;
      
   BByteArray tempData;
   
   BByteArray* pDstArray = &tempData;
   uint dstArrayOfs = 0;
   bool endianSwap = false;
   
   if (platformSpecificData)
   {
      dstTextureInfo.mPlatform = cDDXPlatformXbox;
                  
      dstArrayOfs = dstData.getSize();
      
      dstData.resize(dstData.size() + sizeof(IDirect3DTexture9) + Utils::AlignUpValue(totalBlocks * bytesPerBlock, 1024));
      
      memcpy(dstData.getPtr() + dstArrayOfs, &d3dTex, sizeof(IDirect3DTexture9));
      
      dstArrayOfs += sizeof(IDirect3DTexture9);
      
      pDstArray = &dstData;
   }
   else
   {
      tempData.resize(totalBlocks * bytesPerBlock);
      endianSwap = true;
   }

#if TIMING_STATS
   BTimer timer;
   timer.start();
#endif  

#if TRACE_RECORDING
   static bool traceRecording;
   if (!endianSwap)
   {
      if (traceRecording) 
      {
         XTraceStartRecording("e:\\decompress.bin");
      }
   }      
#endif   

#if PMC_LIB
   PMCStart();
#endif

   bool threadedDecomp = false;

#if defined(XBOX) && THREADED_DECOMPRESSION
   threadedDecomp = gWorkDistibutor.getPermittingNewWork() && ((header.mNumBaseSegments + header.mNumMipSegments) > 1);
#endif   

   if (!threadedDecomp)
   {
      for (uint segmentIndex = 0; segmentIndex < header.mNumBaseSegments; segmentIndex++)
      {
         if (!decodeSegment(
            pDstArray->getPtr() + dstArrayOfs,
            header, pCachedMem,
            header.mBaseIndices[segmentIndex],
            format,
            endianSwap))
         {
            return false;
         }
      }
      
      for (uint segmentIndex = 0; segmentIndex < header.mNumMipSegments; segmentIndex++)
      {
         if (!decodeSegment(
            pDstArray->getPtr() + dstArrayOfs,
            header, pCachedMem,
            header.mMipIndices[segmentIndex],
            format,
            endianSwap))
         {         
            return false;
         }         
      }
   }
   else
   {
#ifdef XBOX   
      gWorkDistibutor.flush();
      
      const uint cMaxWorkItems = BDXTQHeader::cNumSegments * 2;
      BDecodeWorkItem workItems[cMaxWorkItems];
      BDecodeWorkItem* pWorkItems = workItems;
      
      if (((uint)header.mNumBaseSegments + (uint)header.mNumMipSegments) > cMaxWorkItems)
         return false;
            
      for (uint segmentIndex = 0; segmentIndex < header.mNumBaseSegments; segmentIndex++)
      {
         pWorkItems->mpDst = pDstArray->getPtr() + dstArrayOfs;
         pWorkItems->mpHeader = &header;
         pWorkItems->mpCachedMem = pCachedMem;
         pWorkItems->mpSegment = &header.mBaseIndices[segmentIndex];
         pWorkItems->mDataFormat = format;
         pWorkItems->mEndianSwap = endianSwap;
         pWorkItems++;
      }

      for (uint segmentIndex = 0; segmentIndex < header.mNumMipSegments; segmentIndex++)
      {
         pWorkItems->mpDst = pDstArray->getPtr() + dstArrayOfs;
         pWorkItems->mpHeader = &header;
         pWorkItems->mpCachedMem = pCachedMem;
         pWorkItems->mpSegment = &header.mMipIndices[segmentIndex];
         pWorkItems->mDataFormat = format;
         pWorkItems->mEndianSwap = endianSwap;
         pWorkItems++;
      }         
      const uint numWorkItems = pWorkItems - workItems;
      
      BCountDownEvent countDownEvent;
      countDownEvent.set(numWorkItems);
      
      for (uint i = 1; i < numWorkItems; i++)
         gWorkDistibutor.queue(decodeSegmentWorkFunc, &workItems[i], (uint64)&countDownEvent, 1);
            
      // Not needed because we're using 1 work entry per bucket.
      //gWorkDistibutor.flush();
                  
      decodeSegmentWorkFunc((void*)&workItems[0], (uint64)&countDownEvent, 0, 0);
            
      gWorkDistibutor.flushAndWaitSingle(countDownEvent, INFINITE, 8, false, false);
#endif      
   }      
   
#if PMC_LIB
   PMCStop();
#endif   
   
#if TRACE_RECORDING
   if (traceRecording)
   {
      XTraceStopRecording();      
      
      DebugBreak();
      
      traceRecording = false;
   }
#endif      

#if TIMING_STATS
double t = timer.getElapsedSeconds();
double cCPUClockRate = 3201239722;
double cyclesPerBlock = (t * cCPUClockRate) / totalBlocks;
static double bestCyclesPerBlock = Math::fNearlyInfinite;
if (cyclesPerBlock < bestCyclesPerBlock)
   bestCyclesPerBlock = cyclesPerBlock;
if (!endianSwap)
   trace("endian %u, %u blocks, %2.6f, %2.6f cycles per block, %2.6f best cycles per block", endianSwap, totalBlocks, t, cyclesPerBlock, bestCyclesPerBlock);
#endif   

#if PRINT_DECOMP_CRC
   trace("CRC: 0x%08X", calcCRC32(pDstArray->getPtr(), pDstArray->getSizeInBytes(), cInitCRC32));
#endif
   
   if (platformSpecificData)
      return true;
               
   for (uint mipLevel = 0; mipLevel < (numMipChainLevels + 1U); mipLevel++)
   {
      uint mipWidth, mipHeight;
      BDDXUtils::calcMipDimension(mipWidth, mipHeight, width, height, mipLevel);

      const int widthInBlocks = Math::Max<int>(1, mipWidth >> 2);
      const int heightInBlocks = Math::Max<int>(1, mipHeight >> 2);
      
      const uint dstPitch = widthInBlocks * bytesPerBlock; 
      
      const uint dstSize = dstPitch * heightInBlocks;

      const uint outStreamSize = dstData.size();
      dstData.resize(outStreamSize + dstSize);

      BYTE* pDst = dstData.getPtr() + outStreamSize;

      BYTE* pSrc = tempData.getPtr();

      if ((mipLevel > 0) && (d3dTexMipSize))
         pSrc += d3dTexBaseSize;
      
      const uint mipLevelOffset = XGGetMipLevelOffset( &d3dTex, 0, mipLevel );
      pSrc += mipLevelOffset;

      const DWORD flags = d3dTex.Format.PackedMips ? 0 : XGTILE_NONPACKED;

      XGUntileTextureLevel(
         width,
         height,
         mipLevel,
         XGGetGpuFormat(d3dTexDesc.Format),
         flags,
         pDst,
         dstPitch,
         NULL,
         pSrc,
         NULL);
   }            
             
   return true;
}

inline D3DFORMAT BDXTQUnpack::getD3DDataFormatFromDDX(eDDXDataFormat dataFormat)
{
   switch (dataFormat)
   {
      case cDDXDataFormatDXT1Q:  return D3DFMT_DXT1; 
      case cDDXDataFormatDXT5Q:  return D3DFMT_DXT5; 
      case cDDXDataFormatDXT5HQ: return D3DFMT_DXT5; 
      case cDDXDataFormatDXT5YQ: return D3DFMT_DXT5; 
      case cDDXDataFormatDXNQ:   return D3DFMT_DXN;  
   }    
   return D3DFMT_UNKNOWN;
}

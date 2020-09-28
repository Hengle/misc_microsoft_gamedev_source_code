//==============================================================================
// File: bytePacker.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

namespace BBytePacker
{
   // Big endian get/puts.
   
   inline void put32(BYTE* pDst, uint i)
   {
      pDst[0] = static_cast<BYTE>(i >> 24);
      pDst[1] = static_cast<BYTE>(i >> 16);
      pDst[2] = static_cast<BYTE>(i >> 8);
      pDst[3] = static_cast<BYTE>(i);
   }

   inline uint get32(const BYTE* pSrc)
   {
#ifdef XBOX
      // pSrc may be misaligned!
      return *reinterpret_cast<const uint*>(pSrc);
#else   
      return (pSrc[0] << 24) | (pSrc[1] << 16) | (pSrc[2] << 8) | pSrc[3];
#endif      
   }

   inline void put24(BYTE* pDst, uint i)
   {
      assert(i <= 0xFFFFFF);
      pDst[0] = static_cast<BYTE>(i >> 16);
      pDst[1] = static_cast<BYTE>(i >> 8);
      pDst[2] = static_cast<BYTE>(i);
   }

   inline uint get24(const BYTE* pSrc)
   {
      return (pSrc[0] << 16) | (pSrc[1] << 8) | (pSrc[2]);
   }

   inline void put16(BYTE* pDst, uint i)
   {
      assert(i <= 0xFFFF);
      pDst[0] = static_cast<BYTE>(i >> 8);
      pDst[1] = static_cast<BYTE>(i);
   }

   inline uint get16(const BYTE* pSrc)
   {
#ifdef XBOX   
      // pSrc may be misaligned!
      return *reinterpret_cast<const WORD*>(pSrc);
#else      
      return (pSrc[0] << 8) | pSrc[1];
#endif      
   }
   
} // namespace BBytePacker

#pragma pack(push)
#pragma pack(1)

class BPacked32
{
public:
   BPacked32() { }
   BPacked32(uint val) { BBytePacker::put32(mVal, val); }
   operator uint() const { return BBytePacker::get32(mVal); }
   BPacked32& operator= (uint i) { BBytePacker::put32(mVal, i); return *this; }

private:   
   BYTE mVal[4];
};

class BPacked24
{
public:
   BPacked24() { }
   BPacked24(uint val) { BBytePacker::put24(mVal, val); }
   operator uint() const { return BBytePacker::get24(mVal); }
   BPacked24& operator= (uint i) { BBytePacker::put24(mVal, i); return *this; }
   
private:   
   BYTE mVal[3];
};

class BPacked16
{
public:
   BPacked16() { }
   BPacked16(uint val) { BBytePacker::put16(mVal, val); }
   operator uint() const { return BBytePacker::get16(mVal); }
   BPacked16& operator= (uint i) { BBytePacker::put16(mVal, i); return *this; }

private:   
   BYTE mVal[2];
};

#pragma pack(pop)

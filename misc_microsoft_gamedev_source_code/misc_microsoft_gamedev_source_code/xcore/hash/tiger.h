// File: tiger.h
#pragma once

const uint cTigerHashSize = 24;

typedef unsigned __int64 word64;
void tiger(const void* p, word64 length, word64 res[3]);

class BTigerHash
{
   // Three big endian QWORD's
   enum { cBufSize = 24 };
   union
   {
      uchar mBuf[cBufSize];
      uint64 mQWORDs[3];
   };
   
public:
   BTigerHash()
   {
      clear();
   }

   BTigerHash(uint64 a, uint64 b, uint64 c)
   {
      Utils::WriteValueBigEndian(mBuf, a);
      Utils::WriteValueBigEndian(mBuf + 8, b);
      Utils::WriteValueBigEndian(mBuf + 16, c);
   }

   BTigerHash(const uchar* pBuf)
   {
      set(pBuf);
   }

   BTigerHash(const BTigerHash& b)
   {
      memcpy(mBuf, b.mBuf, cBufSize);
   }

   BTigerHash& operator= (const BTigerHash& rhs)
   {
      if (this != &rhs)
         memcpy(mBuf, rhs.mBuf, cBufSize);
      return *this;
   }

   void clear(void)
   {
      memset(mBuf, 0, cBufSize);
   }

   uint size(void) const { return cBufSize; }
   
   void set(const uchar* pBuf)
   {
      BDEBUG_ASSERT(pBuf);
      memcpy(mBuf, pBuf, cBufSize);
   }   

         uchar  operator[] (int i) const  { return mBuf[debugRangeCheck(i, cBufSize)]; }
   const uchar& operator[] (int i)        { return mBuf[debugRangeCheck(i, cBufSize)]; }

   uint64 getQWORD(uint i) const { BDEBUG_ASSERT(i < 3); return Utils::GetValueBigEndian<uint64>(reinterpret_cast<const uint64*>(mBuf) + i); }

         uchar* getBuf(void)        { return mBuf; }
   const uchar* getBuf(void) const  { return mBuf; }

   friend bool operator== (const BTigerHash& lhs, const BTigerHash& rhs) { return memcmp(lhs.mBuf, rhs.mBuf, cBufSize) == 0; }
   friend bool operator!= (const BTigerHash& lhs, const BTigerHash& rhs) { return !(lhs == rhs); }
   friend bool operator< (const BTigerHash& lhs, const BTigerHash& rhs) { return memcmp(lhs.mBuf, rhs.mBuf, cBufSize) < 0; }
   
   BTigerHash& operator^= (const BTigerHash& other)
   {
      mQWORDs[0] ^= other.mQWORDs[0];
      mQWORDs[1] ^= other.mQWORDs[1];
      mQWORDs[2] ^= other.mQWORDs[2];
      
      return *this;
   }

   friend BStream& operator<< (BStream& dst, const BTigerHash& src)
   {
      dst.writeBytes(src.mBuf, cBufSize);
      return dst;
   }

   friend BStream& operator>> (BStream& src, BTigerHash& dst)
   {
      src.readBytes(dst.mBuf, cBufSize);
      return src;
   }
};

class BTigerHashGen
{
public:
   BTigerHashGen() { clear(); }
   BTigerHashGen(const BTigerHash& tigerHash) { clear(); update(tigerHash.getBuf(), tigerHash.size()); }
   BTigerHashGen(const void* p, uint len) { clear(); update(p, len); }

   void clear(void);

   BTigerHashGen& update(const void* p, uint len);
   
   BTigerHashGen& update(const BTigerHash& tigerHash) { return update(tigerHash.getBuf(), tigerHash.size()); }
   
   BTigerHashGen& update(uchar c) { return update(&c, 1); }
   
   // c will always be updated as a stream of big endian bytes.
   BTigerHashGen& update16(uint c)
   {
      update((uchar)((c >> 8) & 0xFF));
      return update((uchar)(c & 0xFF));
   }

   // c will always be updated as a stream of big endian bytes.
   BTigerHashGen& update32(uint c)
   {
      update((uchar)((c >> 24) & 0xFF));
      update((uchar)((c >> 16) & 0xFF));
      update((uchar)((c >> 8) & 0xFF));
      return update((uchar)(c & 0xFF));
   }

   // c will always be updated as a stream of big endian bytes.
   BTigerHashGen& update64(uint64 c)
   {
      for (uint i = 0; i < 8; i++)
      {
         update((uchar)((c >> 56) & 0xFF));
         c <<= 8;
      }
      return *this;
   }
   
   BTigerHash finalize(void);

private:
   uint64   mRes[3];
   uint64   mLength;
   uint     mBufSize;
   uchar    mBuf[64];

   inline void compressBuf(const word64* pStr);
};

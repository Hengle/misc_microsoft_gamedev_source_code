//============================================================================
//
// File: bsha1.h
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================
// FIXME: create .cpp
// SHA1 message digest/one way hash classes.
// This is a straightforward implementation of the algorithm and psuedocode described here:
// http://en.wikipedia.org/wiki/SHA-1
// Also see the NSA FIPS PUB 180-1 publication.
#pragma once

//============================================================================
// class BSHA1
// This class is bitcopied, do not add virtual methods to this class!
//============================================================================
class BSHA1
{
   // Five big endian DWORD's
   enum { cBufSize = 20 };
   uchar mBuf[cBufSize];
   
   static inline void writeDWORD(void* p, DWORD d)
   {
      uchar* pDst = reinterpret_cast<uchar*>(p);
      pDst[0] = static_cast<uchar>((d >> 24) & 0xFF);
      pDst[1] = static_cast<uchar>((d >> 16) & 0xFF);
      pDst[2] = static_cast<uchar>((d >>  8) & 0xFF);
      pDst[3] = static_cast<uchar>((d      ) & 0xFF);
   }
   
   static inline DWORD readDWORD(const void* p)
   {
      const uchar* pSrc = reinterpret_cast<const uchar*>(p);
      return (pSrc[0] << 24) | (pSrc[1] << 16) | (pSrc[2] << 8) | pSrc[3];
   }

public:
   BSHA1()
   {
      clear();
   }

   BSHA1(DWORD a, DWORD b, DWORD c, DWORD d, DWORD e)
   {
      DWORD* pWordBuf = reinterpret_cast<DWORD*>(mBuf);
      writeDWORD(pWordBuf + 0, a);
      writeDWORD(pWordBuf + 1, b);
      writeDWORD(pWordBuf + 2, c);
      writeDWORD(pWordBuf + 3, d);
      writeDWORD(pWordBuf + 4, e);
   }

   BSHA1(const uchar* pBuf)
   {
      BDEBUG_ASSERT(pBuf);
      memcpy(mBuf, pBuf, cBufSize);
   }

   BSHA1(const BSHA1& b)
   {
      memcpy(mBuf, b.mBuf, cBufSize);
   }

   BSHA1& operator= (const BSHA1& rhs)
   {
      if (this != &rhs)
         memcpy(mBuf, rhs.mBuf, cBufSize);
      return *this;
   }

   void clear(void)
   {
      memset(mBuf, 0, cBufSize);
   }

   uint size(void) const { return cBufSize ;}

   uchar  operator[] (int i) const  { return mBuf[debugRangeCheck(i, cBufSize)]; }
   uchar& operator[] (int i)        { return mBuf[debugRangeCheck(i, cBufSize)]; }
   
   DWORD getDWORD(uint i) const { BDEBUG_ASSERT(i < 5); return readDWORD(mBuf + i * 4); }

         uchar* getBuf(void)        { return mBuf; }
   const uchar* getBuf(void) const  { return mBuf; }
   
   void set(const uchar* pBuf)
   {
      BDEBUG_ASSERT(pBuf);
      memcpy(mBuf, pBuf, cBufSize);    
   }
         
   friend bool operator== (const BSHA1& lhs, const BSHA1& rhs) { return memcmp(lhs.mBuf, rhs.mBuf, cBufSize) == 0; }
   friend bool operator!= (const BSHA1& lhs, const BSHA1& rhs) { return !(lhs == rhs); }
   friend bool operator< (const BSHA1& lhs, const BSHA1& rhs) { return memcmp(lhs.mBuf, rhs.mBuf, cBufSize) < 0; }

   friend BStream& operator<< (BStream& dst, const BSHA1& src)
   {
      dst.writeBytes(src.mBuf, cBufSize);
      return dst;
   }

   friend BStream& operator>> (BStream& src, BSHA1& dst)
   {
      src.readBytes(dst.mBuf, cBufSize);
      return src;
   }
};

//============================================================================
// class BSHA1Gen
// This class is bitcopied, do not add virtual methods to this class!
//============================================================================
class BSHA1Gen
{
   uint64 mTotalBytes;

   enum { cDigestBytes = 20 };
   enum { cDigestWords = 5 };
   enum { cChunkBytes = 64 };
   enum { cChunkWords = 16 };

   uchar mChunk[cChunkBytes];
   DWORD mDigest[cDigestWords];

public:
   BSHA1Gen()
   {
      clear();
   }

   BSHA1Gen(BStream& stream)
   {
      clear();
      update(stream);
   }

   BSHA1Gen(const BSHA1Gen& b)
   {
      clear(b);
   }

   BSHA1Gen& operator= (const BSHA1Gen& rhs)
   {
      if (this == &rhs)
         return *this;

      memcpy(this, &rhs, sizeof(*this));
      return *this;
   }

   BSHA1Gen(const void* pData, int dataLen)
   {
      clear();
      update(pData, dataLen);
   }

   BSHA1Gen(const void* pData, int dataLen, const BSHA1Gen& prev)
   {
      clear(prev);
      update(pData, dataLen);
   }

   void clear(void)
   {
      mTotalBytes = 0;

      memset(mChunk, 0, sizeof(mChunk));

      mDigest[0] = 0x67452301;
      mDigest[1] = 0xEFCDAB89;
      mDigest[2] = 0x98BADCFE;
      mDigest[3] = 0x10325476;
      mDigest[4] = 0xC3D2E1F0;
   }

   void clear(const BSHA1Gen& prev)
   {
      memcpy(this, &prev, sizeof(BSHA1Gen));
   }

   BSHA1 finalize(void)
   {
      static uchar messagePadding[64] =
      {
         128, 
         0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
         0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0
      };

      const uint64 totalBits = mTotalBytes << 3;

      const uint lastChunkBytes = static_cast<uint>(mTotalBytes & 63);
      update(messagePadding, debugRangeCheckIncl((lastChunkBytes < 56) ? (56 - lastChunkBytes) : (120 - lastChunkBytes), 64));

      uchar messageSize[8];
      putWord(messageSize, static_cast<uint>(totalBits >> 32));
      putWord(messageSize + 4, static_cast<uint>(totalBits & 0xFFFFFFFF));
      update(messageSize, 8);

      BDEBUG_ASSERT(0 == (mTotalBytes & 63));

      mTotalBytes = 0;
      memset(mChunk, 0, sizeof(mChunk));

      return BSHA1(mDigest[0], mDigest[1], mDigest[2], mDigest[3], mDigest[4]);
   }
      
   BSHA1Gen& update(const void* pData, uint dataLen)
   {
      BDEBUG_ASSERT(pData);
      if (!dataLen)
         return *this;

      const uchar* pBytes = reinterpret_cast<const uchar*>(pData);

      const uint curChunkBytes = static_cast<uint>(mTotalBytes & 63);
      mTotalBytes += dataLen;

      if (curChunkBytes)
      {
         const uint bytesNeeded = cChunkBytes - curChunkBytes;
         const uint bytesToCopy = Math::Min(bytesNeeded, dataLen);

         memcpy(mChunk + curChunkBytes, pBytes, bytesToCopy);
         pBytes      += bytesToCopy;
         dataLen     -= bytesToCopy;

         if (bytesToCopy == bytesNeeded)
            processChunk(mChunk);
         else
         {
            BDEBUG_ASSERT(0 == dataLen);
            return *this;
         }
      }

      while (dataLen >= cChunkBytes)
      {
         processChunk(pBytes);
         pBytes      += cChunkBytes;
         dataLen     -= cChunkBytes;
      }

      if (dataLen)
         memcpy(mChunk, pBytes, dataLen);

      return *this;
   }
   
   BSHA1Gen& update(const BSHA1& hash)
   {
      return update(hash.getBuf(), hash.size());
   }
   
   BSHA1Gen& update(uchar c)
   {
      return update(&c, 1);
   }
   
   // c will always be updated as a stream of big endian bytes.
   BSHA1Gen& update16(uint c)
   {
      update((uchar)((c >> 8) & 0xFF));
      return update((uchar)(c & 0xFF));
   }
   
   // c will always be updated as a stream of big endian bytes.
   BSHA1Gen& update32(uint c)
   {
      update((uchar)((c >> 24) & 0xFF));
      update((uchar)((c >> 16) & 0xFF));
      update((uchar)((c >> 8) & 0xFF));
      return update((uchar)(c & 0xFF));
   }
   
   // c will always be updated as a stream of big endian bytes.
   BSHA1Gen& update64(uint64 c)
   {
      for (uint i = 0; i < 8; i++)
      {
         update((uchar)((c >> 56) & 0xFF));
         c <<= 8;
      }
      return *this;
   }

   BSHA1Gen& update(BStream& stream)
   {
      const int cBufSize = 8192;
      uchar buf[cBufSize];

      for ( ; ; )
      {
         const int n = stream.readBytes(buf, cBufSize);
         if (!n)
            break;

         update(buf, n);
      }

      return *this;
   }

private:

   // Pass 1, 0-19 
   static inline DWORD F1(DWORD b, DWORD c, DWORD d)   
   {
      return d ^ (b & (c ^ d));
   }

   // Pass 2, 20-39 
   static inline DWORD F2(DWORD b, DWORD c, DWORD d)   
   {
      return b ^ c ^ d;
   }

   // Pass 3, 40-59 
   static inline DWORD F3(DWORD b, DWORD c, DWORD d)
   {
      return (b & c) | (d & (b | c));
   }

   // Pass 4, 60-79
   static inline DWORD F4(DWORD b, DWORD c, DWORD d)   
   {
      return b ^ c ^ d;
   }

   // Rotate bits left
   static inline DWORD rotateLeft(DWORD i, UCHAR l)
   {
      return (i << l) | (i >> (32 - l));
   }

   // SHA1 extend function
   static inline DWORD extend(const DWORD* w, DWORD i) 
   {
      BDEBUG_ASSERT(i >= 16);
      return rotateLeft(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
   }  

   void processChunk(const uchar* pChunkData)
   {
      DWORD w[80];    

      getWords(w, pChunkData, cChunkWords);

      // This could be done with a circular array
      for (int i = 16; i <= 79; i++)
         w[i] = extend(w, i);      

      DWORD a = mDigest[0];
      DWORD b = mDigest[1];
      DWORD c = mDigest[2];
      DWORD d = mDigest[3];
      DWORD e = mDigest[4];

      const DWORD K1 = 0x5A827999;
      const DWORD K2 = 0x6ED9EBA1;
      const DWORD K3 = 0x8F1BBCDC;
      const DWORD K4 = 0xCA62C1D6;

      // This could be unrolled
      for (int i = 0; i <= 19; i++)
      {
         const DWORD temp = rotateLeft(a, 5) + F1(b, c, d) + e + K1 + w[i];
         e = d;       
         d = c;
         c = rotateLeft(b, 30);
         b = a;
         a = temp;
      }

      for (int i = 20; i <= 39; i++)
      {
         const DWORD temp = rotateLeft(a, 5) + F2(b, c, d) + e + K2 + w[i];
         e = d;       
         d = c;
         c = rotateLeft(b, 30);
         b = a;
         a = temp;
      }

      for (int i = 40; i <= 59; i++)
      {
         const DWORD temp = rotateLeft(a, 5) + F3(b, c, d) + e + K3 + w[i];
         e = d;       
         d = c;
         c = rotateLeft(b, 30);
         b = a;
         a = temp;
      }

      for (int i = 60; i <= 79; i++)
      {
         const DWORD temp = rotateLeft(a, 5) + F4(b, c, d) + e + K4 + w[i];
         e = d;       
         d = c;
         c = rotateLeft(b, 30);
         b = a;
         a = temp;
      }

      mDigest[0] += a;
      mDigest[1] += b;
      mDigest[2] += c;
      mDigest[3] += d;
      mDigest[4] += e;
   }

   static inline DWORD getWord(const void* p)
   {
      const uchar* pSrc = reinterpret_cast<const uchar*>(p);
      return (pSrc[0] << 24) | (pSrc[1] << 16) | (pSrc[2] << 8) | pSrc[3];
   }

   static inline void putWord(void* p, DWORD d)
   {
      uchar* pDst = reinterpret_cast<uchar*>(p);
      pDst[0] = static_cast<uchar>((d >> 24) & 0xFF);
      pDst[1] = static_cast<uchar>((d >> 16) & 0xFF);
      pDst[2] = static_cast<uchar>((d >>  8) & 0xFF);
      pDst[3] = static_cast<uchar>((d      ) & 0xFF);
   }

   static void getWords(DWORD* pDst, const uchar *pSrc, uint numWords)
   {
      while (numWords)
      {
         *pDst++ = getWord(pSrc);
         pSrc += sizeof(DWORD);
         numWords--;
      }
   }

}; // BSHA1Gen

#ifdef SHA1_TEST

//============================================================================
// SHA1Test
//============================================================================
void SHA1Test(void)
{
   // Also see http://www.itl.nist.gov/fipspubs/fip180-1.htm
   const char* pTest = "The quick brown fox jumps over the lazy dog";
   const DWORD testSHA1[5] = { 0x2fd4e1c6, 0x7a2d28fc, 0xed849ee1, 0xbb76e739, 0x1b93eb12 };
   
   BSHA1Gen sha1Gen;
   
   sha1Gen.update(pTest, strlen(pTest));
   
   BSHA1 sha1(sha1Gen.finalize());
   
   for (uint i = 0; i < 5; i++)
   {
      BVERIFY(sha1.getDWORD(i) == testSHA1[i]);
   }
}

#endif

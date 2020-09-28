//-----------------------------------------------------------------------------
// File: string.h
// Basic C-style string class
//-----------------------------------------------------------------------------
#pragma once
#include "hash\hash.h"

// Yet another string class. BFixedString is just a plain array of char's.
// If you stick to using its methods to operate on the string, you can't overrun
// its internal buffer.  

template<uint cSize>
struct BFixedString
{
   typedef char charType;
   enum { cBufSize = cSize };
   enum { cMaxLen = cSize - 1 };
      
   BFixedString()
   {
      BCOMPILETIMEASSERT(cBufSize >= 2);
      setTerminator(0);
   }

   BFixedString(const char* pStr)
   {
      BCOMPILETIMEASSERT(cBufSize >= 2);
      BDEBUG_ASSERT(pStr);
      const uint l = Math::Min(strlen(pStr), getMaxLen());
      memcpy(mBuf, pStr, l);
      setTerminator(l);
   }

   BFixedString(eVarArg e, const char* pFmt, ...)
   {
      e;
      va_list args;
      va_start(args, pFmt);
      StringCchVPrintf(mBuf, sizeof(mBuf), pFmt, args);
      va_end(args);
      BDEBUG_ASSERT(getLen() <= getMaxLen());
   }
   
   // The size of the string buffer.      
   uint getBufSize(void) const { return cBufSize; }

   // The maximum possible size of a string that can be safely zero terminated. Always getBufSize() - 1.
   uint getMaxLen(void) const { return cMaxLen; }
         
   uint getLen(void) const
   {
      return debugRangeCheckIncl(strlen(mBuf), getMaxLen());
   }
   
   uint length(void) const { return getLen(); }

   bool getEmpty(void) const { return (mBuf[0] == '\0'); }
   
   bool isEmpty(void) const { return getEmpty(); }

   BFixedString& clear(void) 
   {
      memset(mBuf, 0, sizeof(mBuf));
      return *this;
   }
   
   operator const char*() const { return mBuf; }   
   operator       char*()       {  return mBuf; } 
         
   const char* getPtr(void) const   { return mBuf; }
         char* getPtr(void)         { return mBuf; }
   
   const char* c_str() const    { return mBuf; }
   
   char  get(uint i) const      { return mBuf[debugRangeCheck(i, cSize)]; }
   char& get(uint i)            { return mBuf[debugRangeCheck(i, cSize)]; }
   
   char  getChar(uint i) const   { return mBuf[debugRangeCheck(i, cSize)]; }
   char& getChar(uint i)         { return mBuf[debugRangeCheck(i, cSize)]; }
      
   bool operator== (const BFixedString& b) const   { return strcmp(mBuf, b.mBuf) == 0; }
   bool operator== (const char* pStr) const  { return strcmp(mBuf, pStr) == 0;   }
   friend bool operator== (const char* pStr, const BFixedString& b) { return strcmp(pStr, b.mBuf) == 0; }
   
   bool operator!= (const BFixedString& b) const    { return strcmp(mBuf, b.mBuf) != 0; }
   bool operator!= (const char* pStr) const   { return strcmp(mBuf, pStr) != 0; }
   friend bool operator!= (const char* pStr, const BFixedString& b) { return strcmp(pStr, b.mBuf) != 0; }
               
   bool operator<  (const BFixedString& b) const    { return strcmp(mBuf, b.mBuf)  < 0; }
   bool operator<= (const BFixedString& b) const    { return strcmp(mBuf, b.mBuf) <= 0; }
   bool operator>  (const BFixedString& b) const    { return strcmp(mBuf, b.mBuf)  > 0; }
   bool operator>= (const BFixedString& b) const    { return strcmp(mBuf, b.mBuf) >= 0; }
   
   int compare(const char* pStr) const        { return strcmp(mBuf, pStr); }
   int comparei(const char* pStr) const       { return _stricmp(mBuf, pStr); }
         
   BFixedString& set(const char* pStr) 
   {
      BDEBUG_ASSERT(pStr);
      const uint l = Math::Min(strlen(pStr), getMaxLen());
      memcpy(mBuf, pStr, l);
      setTerminator(l);
      return *this;
   }
   
   // Assumes pStr is zero terminated.
   BFixedString& set(const char* pStr, uint n) 
   {
      BDEBUG_ASSERT(pStr);
      const uint l = Math::Min(n, Math::Min(strlen(pStr), getMaxLen()));
      memcpy(mBuf, pStr, l);
      setTerminator(l);
      return *this;
   }
   
   // pStr does not need to be zero terminated.
   BFixedString& setBuf(const char* pBuf, uint n) 
   {
      BDEBUG_ASSERT(pBuf);
      const uint l = Math::Min(n, getMaxLen());
      memcpy(mBuf, pBuf, l);
      setTerminator(l);
      return *this;
   }
   
   BFixedString& set(const WCHAR* pStr)
   {
      BDEBUG_ASSERT(pStr);
      
      const uint strLen = wcslen(pStr);
      if (!strLen)
      {
         empty();
         return *this;
      }
      
      const long numCharsWritten = WideCharToMultiByte(CP_ACP, 0, pStr, strLen, mBuf, getMaxLen(), NULL, NULL);     
      BDEBUG_ASSERT((numCharsWritten != -1) && (numCharsWritten <= (long)getMaxLen()));
      
      setTerminator(numCharsWritten);
      
      return *this;
   }
               
   BFixedString& operator= (const char* pStr) 
   {
      return set(pStr);
   }

   BFixedString& tolower(void) 
   {
      _strlwr_s(mBuf, sizeof(mBuf));
      return *this;
   }
   
   BFixedString& toupper(void) 
   {
      _strupr_s(mBuf, sizeof(mBuf));
      return *this;
   }
   
   BFixedString& truncate(uint ofs = 0)
   {
      if (ofs >= getLen())
         return *this;
      return setTerminator(ofs);
   }
   
   BFixedString& empty(void)
   {
      mBuf[0] = '\0';
      return *this;
   }
   
   BFixedString& appendChar(char c, int ofs = cInvalidIndex)
   {
      const uint d = (ofs < 0) ? getLen() : static_cast<uint>(ofs);
      
      BDEBUG_ASSERT(d < getMaxLen());
      
      if (d < getMaxLen())
      {
         (*this)[d] = c;
         setTerminator(d + 1);
      }

      return *this;
   }
   
   BFixedString& appendChars(char c, uint num, int ofs = cInvalidIndex)
   {
      const uint d = (ofs < 0) ? getLen() : static_cast<uint>(ofs);

      BDEBUG_ASSERT(d < getMaxLen());

      if ((d + num) <= getMaxLen())
      {
         memset(mBuf + d, c, num);
         setTerminator(d + num);
      }

      return *this;
   }
      
   BFixedString& append(const char* pSrc, int srcLen = cInvalidIndex, uint maxChars = UINT_MAX)
   {
      BDEBUG_ASSERT(pSrc);
      
      const uint curLen = getLen();
      if (srcLen < 0)
         srcLen = static_cast<int>(strlen(pSrc));
      
      const uint numChars = Math::Min3<uint>(srcLen, maxChars, getMaxLen() - curLen);

      if (numChars)
      {
         memcpy(mBuf + curLen, pSrc, numChars);
         setTerminator(curLen + numChars);
      }

      return *this;
   }

   BFixedString& format(const char* pFmt, ...)
   {
      va_list args;
      va_start(args, pFmt);
      StringCchVPrintf(mBuf, sizeof(mBuf), pFmt, args);
      va_end(args);
      BDEBUG_ASSERT(getLen() <= getMaxLen());
      return *this;
   }
   
   BFixedString& formatAppend(const char* pFmt, ...)
   {
      const uint curLen = getLen();
      
      if ((sizeof(mBuf) - curLen) > 1)
      {
         va_list args;
         va_start(args, pFmt);
         StringCchVPrintf(mBuf + curLen, sizeof(mBuf) - curLen, pFmt, args);
         va_end(args);
         BDEBUG_ASSERT(getLen() <= getMaxLen());
      }
      return *this;
   }
   
   BFixedString& formatAppendArgs(const char* pFmt, va_list args)
   {
      const uint curLen = getLen();

      if ((sizeof(mBuf) - curLen) > 1)
      {
         StringCchVPrintf(mBuf + curLen, sizeof(mBuf) - curLen, pFmt, args);
         BDEBUG_ASSERT(getLen() <= getMaxLen());
      }
      return *this;
   }
   
   BFixedString& formatArgs(const char* pFmt, va_list args)
   {
      StringCchVPrintf(mBuf, sizeof(mBuf), pFmt, args);
      BDEBUG_ASSERT(getLen() <= getMaxLen());
      return *this;
   }

   BFixedString& trim(void) 
   {
      int i, j;

      for (i = getLen() - 1; i >= 0; i--)
         if (' ' != mBuf[i])
            break;

      mBuf[i + 1] = '\0';
      
      for (j = 0; ' ' == mBuf[j]; j++) 
         ;
      
      if (j)
         memmove(mBuf, mBuf + j, (i + 1) - j + 1);

      return *this;
   }
   
   BFixedString& copy(const char* pText, int srcCount = cInvalidIndex, int srcPos = cInvalidIndex)
   {
      if (srcCount == cInvalidIndex)
      {
         if (srcPos == cInvalidIndex)
            set(pText);
         else
            set(pText + srcPos);
      }
      else
      {
         if (srcPos == cInvalidIndex)
            set(pText, srcCount);
         else
            set(pText + srcPos, srcCount);
      }
      
      return *this;
   }         
   
   // first, last define a closed interval: [start, end]
   // first = index of first character
   // last = index of last character
   // If f is zero, the string is truncated, otherwise it is modified 
   // in-place with memmove then zero terminated.
   // This function will never fail with invalid inputs.
   BFixedString& crop(uint first, uint last)
   {
      uint firstChr = Math::Min(first, last);
      uint lastChr = Math::Max(first, last);
      const uint curLen = getLen();
      if (!curLen)
         return *this;
         
      firstChr = Math::Min(firstChr, curLen - 1);
      lastChr = Math::Min(lastChr, curLen - 1);
      
      if (firstChr == 0)
      {
         mBuf[lastChr + 1] = '\0';
      }
      else
      {
         const uint count = lastChr - firstChr + 1;
         memmove(mBuf, mBuf + firstChr, count);
         mBuf[count] = '\0';
      }
      
      return *this;
   }
   
   // Like crop(), except start, end define a half open interval: [start, end)
   // end should be the index of the character AFTER the end of the desired substring.
   BFixedString& substring(uint start, uint end)
   {
      const uint curLen = getLen();
      
      if ((start >= curLen) || (end <= start))
      {
         if (curLen)
            empty();
         return *this;
      }
      
      end = Math::Min(end, curLen);
      
      const uint newLen = end - start;
      
      if (start)
         memmove(mBuf, mBuf + start, newLen);
      
      mBuf[newLen] = '\0';
      
      BDEBUG_ASSERT(length() <= getMaxLen());

      return *this;
   }

   BFixedString& left(uint numChars)
   {
      return substring(0, numChars);
   }
   
   BFixedString& mid(uint start, uint len)
   {
      return substring(start, start + len);
   }
   
   BFixedString& right(uint start)
   {
      return substring(start, length());
   }
   
   int find(const char* pStr) const
   {
      if ((getEmpty()) || (!pStr))
         return cInvalidIndex;

      const char* p = strstr(mBuf, pStr);
      if (!p)
         return cInvalidIndex;

      return p - mBuf;
   }
   
   int findLeft(const char* pStr, int startPos = cInvalidIndex) const
   {
      if (getEmpty() || (!pStr) || (*pStr == '\0'))
         return cInvalidIndex;

      return strFindLeft<char>(getPtr(), getLen(), pStr, strlen(pStr), startPos);
   }
   
   int findRight(const char* pStr, int startPos = cInvalidIndex) const
   {
      if (getEmpty() || (!pStr) || (*pStr == '\0'))
         return cInvalidIndex;

      return strFindRight<char>(getPtr(), getLen(), pStr, strlen(pStr), startPos);
   }
   
   int findLeft(char srcChr, int startPos = cInvalidIndex) const
   {
      if (getEmpty())
         return cInvalidIndex;

      return strFindLeftCh<char>(getPtr(), getLen(), srcChr, startPos);
   }
   
   int findRight(char srcChr, int startPos = cInvalidIndex) const
   {
      if (getEmpty())
         return cInvalidIndex;

      return strFindRightCh<char>(getPtr(), getLen(), srcChr, startPos);
   }
      
   BFixedString operator+ (const char* pStr) const
   {
      return BFixedString(*this).append(BFixedString(pStr));
   }

   template<uint RHSize> BFixedString operator+ (const BFixedString<RHSize>& b) const
   {
      return BFixedString(*this).append(b);
   }

   BFixedString& operator+= (const char* pStr)
   {
      return append(BFixedString(pStr));
   }

   template<uint RHSize> BFixedString& operator+= (const BFixedString<RHSize>& b)
   {
      return append(b);
   }
   
   friend BStream& operator<< (BStream& dst, const BFixedString& src)
   {
      dst << src.getLen();
      dst.writeBytes(src.mBuf, src.getLen());
      return dst;
   }

   friend BStream& operator>> (BStream& src, BFixedString& dst)
   {
      uint l;
      src >> l;
      
      if (l > dst.getMaxLen())
      {
         BDEBUG_ASSERT(0);
         dst.setTerminator(0);
         return src;
      }
         
      dst.setTerminator(l);
      src.readBytes(dst.mBuf, l);
      return src;
   }
   
   BStream& writeLine(BStream& dst)
   {
      dst.writeBytes(getPtr(), getLen());
      dst.writeBytes("\r\n", 2);
      return dst;   
   }

   BStream& readLine(BStream& src)
   {
      uint ofs = 0;
      while (ofs < getMaxLen())
      {
         const int c = src.getch();

         // 0D 0A \r\n
         if (c < 0) 
            break;
         else if (c == '\r')
         {
         }
         else if (c == '\n')
            break;
         else
            (*this)[ofs++] = static_cast<char>(c);
      }

      setTerminator(ofs);

      return src;
   }

   BFixedString& removeFilename(void)
   {
      char *p = mBuf + getLen() - 1;
      while (p >= mBuf)
      {
         if ((*p == ':') || (*p == '\\') || (*p == '/'))
            break;
         p--;
      }
      p[1] = '\0';
      BDEBUG_ASSERT(strlen(mBuf) <= getMaxLen());
      return *this;
   }
   
   BFixedString& removeExtension(void)
   {
      char *p = mBuf + getLen() - 1;
      while (p >= mBuf)
      {
         if ((*p == ':') || (*p == '\\') || (*p == '/'))
            return *this;
         if (*p == '.')
         {
            *p = '\0';
            return *this;
         }
         p--;
      }
      return *this;
   }
   
   void standardizePath(void)
   {  
      DWORD newLen = strPathStandardizePath(getPtr(), getPtr(), getMaxLen());
      newLen;
      
      BDEBUG_ASSERT(newLen <= getMaxLen());
   }
   
   void standardizePathFrom(const char* pSrc)
   {  
      DWORD newLen = strPathStandardizePath(pSrc, getPtr(), getMaxLen());

      BDEBUG_ASSERT(newLen <= getMaxLen());
   }
      
   // Operator size_t hasher, for hash containers.
   operator size_t() const
   {
      return hashFast(mBuf, getLen());
   }

private:
   char mBuf[cBufSize];
         
   BFixedString& setTerminator(uint ofs)
   {
      BDEBUG_ASSERT(ofs < getBufSize());
      mBuf[ofs] = '\0';
      return *this;
   }
};

typedef BFixedString<16> BFixedString16;
typedef BFixedString<32> BFixedString32;
typedef BFixedString<64> BFixedString64;
typedef BFixedString<128> BFixedString128;
typedef BFixedString<256> BFixedString256;
typedef BFixedString<MAX_PATH> BFixedStringMaxPath;

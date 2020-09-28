//============================================================================
//
//  stream.h
//
//  Copyright (c) 2003-2007, Ensemble Studios
//
//============================================================================
#pragma once

#define BSTREAM_UNKNOWN_SIZE (0x7fffffffffffffff)

enum eStreamFlags
{
   cSFReadable                      = 1,
   cSFWritable                      = 2,
   cSFSeekable                      = 4,
   cSFNoAutoClose                   = 8,
   cSFErrorOnFailure                = 16,       // cSFErrorOnFailure is deprecated!
   cSFOpenExisting                  = 32,
   cSFOptimizeForRandomAccess       = 64,
   cSFLittleEndian                  = 128,
   cSFOptimizeForSequentialAccess   = 256,
   cSFEnableBuffering               = 512,
   cSFDiscardOnClose                = 1024,
   cSFForceLoose                    = 2048,
   
   cSFDefaultFlags     = cSFReadable | cSFSeekable | cSFOpenExisting
};

//============================================================================
// class BStream
//============================================================================
class BStream
{
public:
   BStream(const BString& name = B(""), uint flags = cSFDefaultFlags) : 
      mName(name),
      mFlags(flags),
      mLastCRFileOfs(-1)
   { 
   }
   
   BStream(const BStream& b)
   {
      mName = b.mName;
      mFlags = b.mFlags;
      mLastCRFileOfs = b.mLastCRFileOfs;
   }

   BStream& operator= (const BStream& rhs)
   {
      mName = rhs.mName;
      mFlags = rhs.mFlags;
      mLastCRFileOfs = rhs.mLastCRFileOfs;
      return *this;
   }

   virtual ~BStream() 
   { 
   }

   const BString& getName(void) const 
   { 
      return mName; 
   }

   void setName(const BString& name)
   {
      mName = name;
   }
   
   uint getFlags(void) const
   {
      return mFlags;
   }
   
   void setFlags(uint flags) 
   {
      mFlags = flags;
   }
   
   bool getLittleEndian(void) const
   {
      return (mFlags & cSFLittleEndian) != 0;
   }
   
   void setLittleEndian(bool littleEndian)
   {
      mFlags &= ~cSFLittleEndian;
      if (littleEndian)
         mFlags |= cSFLittleEndian;
   }
                           
   // true if the actual stream size is known
   // if false, size() returns BSTREAM_UNKNOWN_SIZE until the size of the stream is known.
   virtual bool sizeKnown(void) const { return true; }
         
   // If the stream size is currently unknown, size() returns BSTREAM_UNKNOWN_SIZE
   virtual uint64 size(void) const = 0;

   // Return value is number of bytes actually read.
   // If the return value is less than n, this _may_ indicate an error condition (call errorStatus()).
   virtual uint readBytes(void* p, uint n) = 0;
   
   // Return value is number of bytes actually written.
   // Values less than n indicate an error condition.
   virtual uint writeBytes(const void* p, uint n) = 0;
   
   virtual uint64 curOfs(void) const = 0;
   virtual uint64 bytesLeft(void) const = 0;
   
   virtual bool flush(void) { return true; }
   
   // Default implementation is a no-op.
   // Most stream classes assume the user will manage thread ownership.
   virtual void setOwnerThread(DWORD threadID) { threadID; }
   
   uint64 writeDuplicateBytes(uchar c, uint64 n)
   {
      uchar buf[512];
      memset(buf, c, sizeof(buf));
      
      uint64 k = 0;
      
      while (n)
      {
         const uint l = static_cast<uint>(Math::Min<uint64>(sizeof(buf), n));
         
         if (writeBytes(buf, l) < l)
            return k;
         
         n -= l;
         k += l;
      }
      
      return k;
   }
   
   uint64 skipBytes(uint64 n)
   {  
      uchar buf[1024];

      uint64 k = 0;

      while (n)
      {
         const uint l = static_cast<uint>(Math::Min<uint64>(sizeof(buf), n));
         
         if (readBytes(buf, l) < l)
            return k;
            
         n -= l;
         k += l;
      }

      return k;
   }
         
   // Returns new file offset, or -1 on error.
   // If getSeekable() returns false, this method is not guaranteed to work, even for forward seeks.
   virtual int64 seek(int64 ofs, bool absolute = true) = 0;
   
   // seekOrSkip() is a more flexible seek():
   // For forward seeks, this method either skips ahead or seeks depending on whether or not the stream is seekable and the value of forwardSeekThreshold.
   // For backward seeks, this method will fail if the stream doesn't support seeking.
   virtual bool seekOrSkip(uint64 newOfs, uint64 forwardSeekThreshold = 512)
   {
      uint64 curOfs = this->curOfs();
      if (curOfs == newOfs)
         return true;

      const bool seekable = getSeekable();         
      
      if (newOfs < curOfs)
      {
         if (!seekable)   
            return false;

         if (seek(newOfs, true) != (int64)newOfs)
            return false;
      }
      else 
      {
         const uint64 bytesToSkip = newOfs - curOfs;
         
         if ((!seekable) || (bytesToSkip < forwardSeekThreshold))
         {
            if (skipBytes(bytesToSkip) != bytesToSkip)
               return false;
         }
         else
         {
            if (seek(newOfs, true) != (int64)newOfs)
               return false;
         }
      }

      return true;
   }
         
   // Returned pointer is relative to the start of the stream, not the current offset!
   // len is the intended number of bytes you'll be reading.
   virtual const void* ptr(uint len = 0) { len; return NULL; } 

   // rg [3/18/07] - Deprecated - use the get variants instead, I want to remove these old methods.
   virtual bool readable(void) const { return Utils::GetBitFlag(mFlags, cSFReadable); }
   virtual bool writable(void) const { return Utils::GetBitFlag(mFlags, cSFWritable); }
   virtual bool seekable(void) const { return Utils::GetBitFlag(mFlags, cSFSeekable); }
   
   virtual bool getReadable(void) const { return Utils::GetBitFlag(mFlags, cSFReadable); }
   virtual bool getWritable(void) const { return Utils::GetBitFlag(mFlags, cSFWritable); }
   virtual bool getSeekable(void) const { return Utils::GetBitFlag(mFlags, cSFSeekable); }
   
   // Each set() returns true if the flag was successfully changed
   virtual bool setReadable(bool f) { mFlags = Utils::SetBitFlag(mFlags, cSFReadable, f); return true; }
   virtual bool setWritable(bool f) { mFlags = Utils::SetBitFlag(mFlags, cSFWritable, f); return true; }
   virtual bool setSeekable(bool f) { mFlags = Utils::SetBitFlag(mFlags, cSFSeekable, f); return true; }
         
   // true on error
   virtual bool errorStatus(void) const { return false; }
   
   // true if successfully closed
   virtual bool close(void) { return true; }
   
   // true if stream is opened
   virtual bool opened(void) const { return true; }
   
   virtual bool getTime(uint64& time) const { time = 0; return false; }
   
   // true if object is in a "signaled" state:
   // Either the object has a non-zero error status, or a readable object is out of data.
   virtual operator bool() const 
   { 
      if (errorStatus())
         return true;

      if ((readable()) && (!bytesLeft()))
         return true;

      return false;
   }
         
   virtual void printf(const char* pMsg, ...)
   {
      va_list args;
      va_start(args, pMsg);
      const int BufSize = 1024;
      char buf[BufSize];
      
      _vsnprintf_s(buf, sizeof(buf), pMsg, args);
      va_end(args);

      writeBytes(buf, static_cast<int>(strlen(buf)));
   }

   // -1 (the CRT macro EOF)
   virtual int getch(void) 
   {
      if ((errorStatus()) || (bytesLeft() == 0))
         return -1;
      
      uint8 c;
      if (readBytes(&c, 1) != 1)
         return -1;
         
      return c;
   }

   // true on success
   virtual bool putch(uint8 c)
   {
      return writeObj(c);
   }

   // writing
   
   // true on success
   template<class Type> 
   bool writeObjRaw(const Type& obj) 
   { 
      return sizeof(Type) == writeBytes(&obj, sizeof(Type)); 
   }
   
   template<class Type> 
   bool writeObjByteSwapped(const Type& obj) 
   { 
      Type tempObj;
      Utils::ReadValueByteSwapped(&obj, tempObj);
      return writeObjRaw(tempObj);
   }
   
   template<class Type> bool writeObjBigEndian(Type& obj) { if (cLittleEndianNative) return writeObjByteSwapped(obj); else return writeObjRaw(obj); }
   template<class Type> bool writeObjLittleEndian(Type& obj) { if (cBigEndianNative) return writeObjByteSwapped(obj); else return writeObjRaw(obj); }
   template<class Type> bool writeObjSelectEndian(Type& obj) { return getLittleEndian() ? writeObjLittleEndian(obj) : writeObjBigEndian(obj); }
   
   template<class Type> bool writeObj(const Type& obj)      { return writeObjRaw(obj); }
   template<> bool writeObj<short>(const short& obj)        { return writeObjSelectEndian(obj); }
   template<> bool writeObj<ushort>(const ushort& obj)      { return writeObjSelectEndian(obj); }
   template<> bool writeObj<wchar_t>(const wchar_t& obj)    { return writeObjSelectEndian(obj); }
   template<> bool writeObj<int>(const int& obj)            { return writeObjSelectEndian(obj); }
   template<> bool writeObj<uint>(const uint& obj)          { return writeObjSelectEndian(obj); }
   template<> bool writeObj<long>(const long& obj)          { return writeObjSelectEndian(obj); }
   template<> bool writeObj<unsigned long>(const unsigned long& obj) { return writeObjSelectEndian(obj); }
   template<> bool writeObj<int64>(const int64& obj)        { return writeObjSelectEndian(obj); }
   template<> bool writeObj<uint64>(const uint64& obj)      { return writeObjSelectEndian(obj); }
   template<> bool writeObj<float>(const float& obj)        { return writeObjSelectEndian(*reinterpret_cast<const uint*>(&obj)); }
   template<> bool writeObj<double>(const double& obj)      { return writeObjSelectEndian(*reinterpret_cast<const uint64*>(&obj)); }
   
   template<class Type> bool writeObjects(const Type* pSrc, uint num) 
   { 
      for (uint i = 0; i < num; i++)
         if (!writeObj(pSrc[i]))
            return false;
      return true;
   }
     
   // true on success
   template<class Type>
   bool writeValue(Type v)
   { 
      return writeObj(v);
   }
   
   // reading

   // true on success
   template<class Type> 
   bool readObjRaw(Type& obj) 
   { 
      if (readBytes(&obj, sizeof(Type)) != sizeof(Type))
      {
         Utils::ClearObj(obj);
         return false;
      }
      return true; 
   }
   
   template<class Type> 
   bool readObjByteSwapped(Type& obj) 
   { 
      if (!readObjRaw(obj))
         return false;
      
      uchar* p = reinterpret_cast<uchar*>(&obj);
      for (uint i = 0; i < (sizeof(obj) >> 1); ++i)
      {
         const uchar c = p[i];
         p[i] = p[sizeof(obj) - 1 - i];
         p[sizeof(obj) - 1 - i] = c;
      }
      
      return true;
   }
   
   template<class Type> bool readObjBigEndian(Type& obj) { if (cLittleEndianNative) return readObjByteSwapped(obj); else return readObjRaw(obj); }
   template<class Type> bool readObjLittleEndian(Type& obj) { if (cBigEndianNative) return readObjByteSwapped(obj); else return readObjRaw(obj); }
   template<class Type> bool readObjSelectEndian(Type& obj) { return getLittleEndian() ? readObjLittleEndian(obj) : readObjBigEndian(obj); }
   
   template<class Type> bool readObj(Type& obj)    { return readObjRaw(obj); }
   template<> bool readObj<short>(short& obj)      { return readObjSelectEndian(obj); }
   template<> bool readObj<ushort>(ushort& obj)    { return readObjSelectEndian(obj); }
   template<> bool readObj<wchar_t>(wchar_t& obj)  { return readObjSelectEndian(obj); }
   template<> bool readObj<int>(int& obj)          { return readObjSelectEndian(obj); }
   template<> bool readObj<uint>(uint& obj)        { return readObjSelectEndian(obj); }
   template<> bool readObj<long>(long& obj)        { return readObjSelectEndian(obj); }
   template<> bool readObj<unsigned long>(unsigned long& obj) { return readObjSelectEndian(obj); }
   template<> bool readObj<int64>(int64& obj)      { return readObjSelectEndian(obj); }
   template<> bool readObj<uint64>(uint64& obj)    { return readObjSelectEndian(obj); }
   template<> bool readObj<float>(float& obj)      { return readObjSelectEndian(*reinterpret_cast<uint*>(&obj)); }
   template<> bool readObj<double>(double& obj)    { return readObjSelectEndian(*reinterpret_cast<uint64*>(&obj)); }
   
   template<class Type> bool readObjects(Type* pDst, uint num) 
   { 
      for (uint i = 0; i < num; i++)
         if (!readObj(pDst[i]))
            return false;
      return true;
   }
   
   // read
   template<class Type>
   Type readValue(void) 
   { 
      Type ret = Type(); 
      readObj(ret); 
      return ret; 
   }
               
   // always reads a uint
   template<class Type>
   Type readEnum(void)
   {
      uint ret = Type();
      readObj(ret);
      return static_cast<Type>(ret);
   }
   
   // true on success
   template<class Type>
   bool writeEnum(Type t)
   {
      const uint it = static_cast<uint>(t);
      return writeObj(it);
   }

   // Reads vector's raw data!
   template<class Type>
   bool readVecRaw(Type& v, int numExpected = -1)
   {
      v.clear();
      if (errorStatus())
         return false;

      const int numElements = readValue<int>();
      if ((errorStatus()) || ((-1 != numExpected) && (numElements != numExpected)))
      {
         tracenocrlf("BStream::readVec: Unexpected number of elements in stream!\n");
         return false;
      }

      v.resize(numElements);

      const int n = numElements * sizeof(Type::valueType);
      const int bytesRead = readBytes(&v[0], n);
      
      if ((bytesRead != n) || (errorStatus()))
      {
         v.clear();
         return false;
      }
               
      return true;
   }

   // true on success
   template<class Type>
   bool readVec(Type& v, int numExpected = -1)
   {
      v.clear();
      
      if (errorStatus())
         return false;

      const int numElements = readValue<int>();
      if ((errorStatus()) || ((-1 != numExpected) && (numElements != numExpected)))
      {
         tracenocrlf("BStream::readVec: Unexpected number of elements in stream!\n");
         return false;
      }

      v.resize(numElements);
      for (int i = 0; i < numElements; i++)
      {
         *this >> v[i];
         
         if (errorStatus())
         {
            v.clear();
            return false;
         }
      }

      return true;
   }

   // Writes vector's raw data!
   // true on success
   template<class Type>
   bool writeVecRaw(const Type& v)
   {
      if (errorStatus())
         return false;
         
      writeObj(static_cast<int>(v.size()));
      
      if (errorStatus())
         return false;
      
      const uint n = static_cast<int>(v.size()) * sizeof(Type::valueType);
      return n == writeBytes(&v[0], n);
   }

   // true on success
   template<class Type>
   bool writeVec(const Type& v)
   {
      if (errorStatus())
         return false;
         
      writeObj(static_cast<int>(v.size()));
      
      if (errorStatus())
         return false;
         
      for (uint i = 0; i < v.size(); i++)
      {
         *this << v[i];
         
         if (errorStatus())
            return false;
      }

      return true;
   }
   
   uint writeLine(const char* pStr)
   {
      BDEBUG_ASSERT(pStr);
      const uint len = static_cast<uint>(strlen(pStr));
      return writeBytes(pStr, len);
   }
   
   // Returns false on error or EOF. 
   // Call errorStatus() to determine which.
   // Always strips CR/LF's from the returned string.
   bool readLine(BString& str)
   {
      // See http://en.wikipedia.org/wiki/Newline
      // CR 0x0D
      // LF 0x0A
      // Standard is: CR LF 0x0D 0x0A (\r\n)
      // But we should also handle just CR, or just LF, without peeking ahead.
      
      int64 curFileOfs = (int64)curOfs();
      
      // Seeking invalidates the last place we've seen a CR
      if ((0 == curFileOfs) || (curFileOfs <= mLastCRFileOfs) || (curFileOfs > (mLastCRFileOfs + 1)))
         mLastCRFileOfs = -1;
            
      str.empty();
      
      for ( ; ; )
      {
         const int c = getch();
         
         if (c < 0)
         {
            if ((errorStatus()) || (str.isEmpty()))
               return false;
            
            mLastCRFileOfs = -1;
            break;
         }
            
         const uchar ch = static_cast<uchar>(c);
         
         if ((ch == 0) || (ch == 0x1A))
         {
            // Just ignore 0's and Ctrl+Z, getch() will give us an EOF at the true end of file. (Yes this is a hack.)
            continue;
         } 
         else if (ch == 0x0D)
         {
            mLastCRFileOfs = (int64)curOfs() - 1;
            break;
         }
         else if (ch == 0x0A)
         {
            // Ignore LF's that immediately follow a CR, otherwise treat it as end of line.
            if ((mLastCRFileOfs + 1) == ((int)curOfs() - 1))
            {
               mLastCRFileOfs = -1;
               continue;
            }
            
            mLastCRFileOfs = -1;
            break;
         }
         
         str.append((const char*)&ch, 1);
      }
      
      return true;
   }
   
   static bool compareStreams(BStream& src0, BStream& src1)
   {
      if ((src0.errorStatus()) || (src1.errorStatus()))
         return false;

      if ((!src0.readable()) || (!src1.readable()))
         return false;

      const int BufSize = 8192;
      uchar buf0[BufSize];
      uchar buf1[BufSize];

      int src0Read;
      do
      {
         src0Read = src0.readBytes(buf0, BufSize);
         const int src1Read = src1.readBytes(buf1, BufSize);
         
         if ((src0.errorStatus()) || (src1.errorStatus()))
            return false;
         
         if (src0Read != src1Read)
            return false;
         
         if (memcmp(buf0, buf1, src0Read) != 0)
            return false;
            
      } while (src0Read);
      
      if ((src0.errorStatus()) || (src1.errorStatus()))
         return false;

      return true;
   }
   
   static bool copyStream(BStream& src, BStream& dst, uint64* pBytesCopied)
   {  
      if (pBytesCopied)
         *pBytesCopied = 0;
                        
      if ((src.errorStatus()) || (dst.errorStatus()))
         return false;
      
      if ((!src.readable()) || (!dst.writable()))
         return false;
         
      const int cBufSize = 65536*2;
      BByteArray buf(cBufSize);
         
      for ( ; ; )
      {
         const int n = src.readBytes(buf.getPtr(), cBufSize);
         if (n < cBufSize)
         {
            if (src.errorStatus())
               return false;
            
            if (!n)
               break;
         }
         
         if (pBytesCopied)
            *pBytesCopied += n;
         
         const int w = dst.writeBytes(buf.getPtr(), n);
         if (w != n)
            return false;
      }
      
      if ((src.errorStatus()) || (dst.errorStatus()))
         return false;
      
      return true;
   }
                                          
protected:
   BString mName;
   uint mFlags;
   int64 mLastCRFileOfs;
};

//============================================================================
// BStream write helpers
//============================================================================
inline BStream& operator<< (BStream& dest, const bool& i)            { dest.writeObj(i); return dest; }
inline BStream& operator<< (BStream& dest, const int8& i)            { dest.writeObj(i); return dest; }
inline BStream& operator<< (BStream& dest, const uint8& i)           { dest.writeObj(i); return dest; }
inline BStream& operator<< (BStream& dest, const int16& i)           { dest.writeObj(i); return dest; }
inline BStream& operator<< (BStream& dest, const uint16& i)          { dest.writeObj(i); return dest; }
inline BStream& operator<< (BStream& dest, const int32& i)           { dest.writeObj(i); return dest; }
inline BStream& operator<< (BStream& dest, const uint32& i)          { dest.writeObj(i); return dest; }
inline BStream& operator<< (BStream& dest, const long& i)            { dest.writeObj(i); return dest; }
inline BStream& operator<< (BStream& dest, const unsigned long& i)   { dest.writeObj(i); return dest; }
inline BStream& operator<< (BStream& dest, const int64& i)           { dest.writeObj(i); return dest; }
inline BStream& operator<< (BStream& dest, const uint64& i)          { dest.writeObj(i); return dest; }
inline BStream& operator<< (BStream& dest, const float& i)           { dest.writeObj(i); return dest; }
inline BStream& operator<< (BStream& dest, const double& i)          { dest.writeObj(i); return dest; }
inline BStream& operator<< (BStream& dest, const char* pMsg)         { dest.writeBytes(pMsg, static_cast<int>(strlen(pMsg))); return dest; }
inline BStream& operator<< (BStream& dest, const FILETIME& time)     { dest.writeObj(time.dwLowDateTime); dest.writeObj(time.dwHighDateTime); return dest; }

template<typename T> inline BStream& operator<< (BStream& dest, const T* ptr) 
{ 
   dest.writeObj(ptr); 
   return dest; 
}

inline BStream& operator<< (BStream& dest, const BString& str)  
{ 
   dest.writeObj(static_cast<uint>(str.length()));
   if (!str.isEmpty())
      dest.writeBytes(str.getPtr(), static_cast<uint>(str.length())); 
   return dest; 
}

// If you change BDynamicArray's template params, be sure to change this! Otherwise VS.2003's compiler will crash.
template< class V, uint A, template <class, uint> class All, template <class, uint> class Opt, template <class, uint> class Ptr >
inline BStream& operator<< (BStream& dest, const BDynamicArray<V, A, All, Opt, Ptr>& vec)
{
   dest.writeVec(vec);
   return dest; 
}

template<typename CharType, typename Allocator>
inline BStream& operator<< (BStream& dst, const BStringTemplate<CharType, Allocator>& src)
{
   const ushort len = src.length();
   if (!dst.writeObj(len))
      return dst;
      
   // rg [5/29/06] - FIXME: Slow!
   for (uint i = 0; i < len; i++)
   {
      if (!dst.writeObj(src.getChar(i)))
         return dst;
   }
   
   return dst;
}

//============================================================================
// BStream read helpers
//============================================================================
inline BStream& operator>> (BStream& src, bool& i)          { src.readObj(i); return src; }
inline BStream& operator>> (BStream& src, int8& i)          { src.readObj(i); return src; }
inline BStream& operator>> (BStream& src, uint8& i)         { src.readObj(i); return src; }
inline BStream& operator>> (BStream& src, int16& i)         { src.readObj(i); return src; }
inline BStream& operator>> (BStream& src, uint16& i)        { src.readObj(i); return src; }
inline BStream& operator>> (BStream& src, int32& i)         { src.readObj(i); return src; }
inline BStream& operator>> (BStream& src, uint32& i)        { src.readObj(i); return src; }
inline BStream& operator>> (BStream& src, long& i)          { src.readObj(i); return src; }
inline BStream& operator>> (BStream& src, unsigned long& i) { src.readObj(i); return src; }
inline BStream& operator>> (BStream& src, int64& i)         { src.readObj(i); return src; }
inline BStream& operator>> (BStream& src, uint64& i)        { src.readObj(i); return src; }
inline BStream& operator>> (BStream& src, float& i)         { src.readObj(i); return src; }
inline BStream& operator>> (BStream& src, double& i)        { src.readObj(i); return src; }
inline BStream& operator>> (BStream& src, FILETIME& time)   { src.readObj(time.dwLowDateTime); src.readObj(time.dwHighDateTime); return src; }

template<typename T> inline BStream& operator>> (BStream& src, const T*& i)    
{ 
   src.readObj(i);
   return src;
}
      
inline BStream& operator>> (BStream& src, BString& dst)  
{ 
   const int len = src.readValue<uint>();
   BASSERT(len >= 0);
   const int BufSize = 512;
   
   if (len < BufSize)
   {
      BCHAR_T buf[BufSize];
      src.readBytes(buf, len);
      buf[len] = B('\0');
      dst.set(buf);
   }
   else
   {
      BCHAR_T* pBuf = new BCHAR_T [len + 1];
      src.readBytes(pBuf, len);
      pBuf[len] = B('\0');
      dst.set(pBuf);
      delete [] pBuf;
   }
   
   return src; 
}  

// If you change BDynamicArray's template params, be sure to change this! Otherwise VS.2003's compiler will crash.
template< class V, uint A, template <class, uint> class All, template <class, uint> class Opt, template <class, uint> class Ptr >         
inline BStream& operator>> (BStream& src, BDynamicArray<V, A, All, Opt, Ptr>& vec)
{
   src.readVec(vec);
   return src;
}

template<typename CharType, typename Allocator>
inline BStream& operator>> (BStream& src, BStringTemplate<CharType, Allocator>& dst)
{
   ushort len;
   if (!src.readObj<ushort>(len))
      return src;

   if (!len)
   {
      dst.empty();
      return src;
   }

   const uint cBufSize = 256;
   CharType temp[cBufSize];
   CharType* pBuf = temp;

   if (len >= cBufSize)
      pBuf = new CharType[len + 1];

   const uint numBytes = len * sizeof(CharType);
   if (src.readBytes(pBuf, numBytes) < numBytes)
   {
      delete[] pBuf;
      return src;
   }

   pBuf[len] = 0;

   if (sizeof(CharType) == sizeof(WORD))
   {
      const bool swap = src.getLittleEndian() ? cBigEndianNative : cLittleEndianNative;
      if (swap)
         EndianSwitchWords(pBuf, len);
   }

   dst.set(temp);

   if (len >= cBufSize)
      delete[] pBuf;

   return src;
}

//============================================================================
// class IStreamFactory
//============================================================================
class IStreamFactory
{
public:
   virtual ~IStreamFactory() { }
   // Use operator delete to delete the stream object.
   virtual BStream* create(long dirID, const BString& filename, eStreamFlags flags) = 0;
   
   virtual bool getFileTime(long dirID, const BString& filename, uint64& fileTime) = 0;
};

//============================================================================
//
// File: utils.h
// Copyright (c) 2003-2006, Ensemble Studios
//
// This consists of a junk drawer namespace from three codebases! 
// We'll clean this up -- eventually.
//
//============================================================================
#pragma once

//============================================================================
// Macros
//============================================================================
#define       CAST(type, object) (*reinterpret_cast<type*>(&(object)))
#define CONST_CAST(type, object) (*reinterpret_cast<const type*>(&(object)))

#define ALIGN_OF(v) (__alignof(v) ? __alignof(v) : sizeof(DWORD)) 

//============================================================================
// struct BEmptyStruct
//============================================================================
struct BEmptyStruct { };
DEFINE_BUILT_IN_TYPE(BEmptyStruct);

//============================================================================
// class BEqualTo
//============================================================================
template <class T> 
struct BEqualTo { inline bool operator()(const T& a, const T& b) const { return a == b; } };

//============================================================================
// class BLessThan
//============================================================================
template <class T> 
struct BLessThan { inline bool operator()(const T& a, const T& b) const { return a < b; } };

//============================================================================
// namespace Utils
//============================================================================
namespace Utils
{
   template<class T> void ClearObj(T& obj, int val = 0);
   
   template<class T> bool IsAligned(T p, size_t alignment);
   bool IsValueAligned(uint p, uint alignment);
   template<class T> T AlignUp(T p, size_t alignment);
   template<class T> T AlignDown(T p, size_t alignment);
   template<class T> T AlignUpValue(T p, size_t alignment);
   template<class T> T BytesToAlignUpValue(T p, size_t alignment);
   template<class T> T RoundUp(T p, size_t alignment);
   template<class T> int BytesToAlignUp(T p, size_t alignment);
      
   // Writes bytes of value to pDst.
   template<class T> void* WriteValue(void* pDst, const T& v);
   template<class T> void* WriteValue(void* pDst, const T& v, bool bigEndian);
   template<class T> void* WriteValueByteSwapped(void* pDst, const T& v);
   template<class T> void* WriteValueLittleEndian(void* pDst, const T& v);
   template<class T> void* WriteValueBigEndian(void* pDst, const T& v);
   
   template<class T> const void* ReadValue(const void* pSrc, T& v);  
   template<class T> const void* ReadValue(const void* pSrc, T& v, bool bigEndian);
   template<class T> const void* ReadValueByteSwapped(const void* pSrc, T& v);
   template<class T> const void* ReadValueLittleEndian(const void* pSrc, T& v);
   template<class T> const void* ReadValueBigEndian(const void* pSrc, T& v);
   
   template<class T> const T GetValue(const void* pSrc);  
   template<class T> const T GetValue(const void* pSrc, bool bigEndian);
   template<class T> const T GetValueByteSwapped(const void* pSrc);
   template<class T> const T GetValueLittleEndian(const void* pSrc);
   template<class T> const T GetValueBigEndian(const void* pSrc);
   
   void EndianSwitch(void* pBytes, uint size);
   
   WORD CreateWORD(BYTE lo, BYTE hi);
   DWORD CreateDWORD(BYTE lo, BYTE b, BYTE c, BYTE hi);
   
   uint ExtractLowByteFromWORD(WORD w);
   uint ExtractHighByteFromWORD(WORD w);
   
   // writeObj/readObj FastMemCpy's object to destination, returns updated pointer.
   template<class T> void* writeObj(void* pDst, const T& obj);
   template<class T> const void* readObj(const void* pSrc, T& obj);
   
   // Returns true if b is 0 or 1.
   bool IsValidBool(const bool& b);
                              
   template<class T>
   struct RelativeOperators
   {   
      friend bool operator >  (const T& a, const T& b) 
      { 
            return b < a;      
      }
      friend bool operator <= (const T& a, const T& b) 
      { 
         return !(b < a);  
      }
      friend bool operator >= (const T& a, const T& b) 
      { 
         return !(a < b);  
      }
      friend bool operator != (const T& a, const T& b) 
      { 
         return !(a == b); 
      }
   };
   
   template<class Type>
   void DeletePointerVector(Type& vec)
   {
      for (int i = 0; i < vec.size(); i++)
         delete vec[i];
   }
   
   template<class Type>
   void DeleteArrayPointerVector(Type& vec)
   {
      for (int i = 0; i < vec.size(); i++)
         delete [] vec[i];
   }
   
   inline int ConvertHexChar(int c);
   
   // Converts a pointer to an offset.
   // Assumes sizeof(T*)==sizeof(uint32)
   template <typename T>
      T* Offsetize(T*& ptr, void* pBase)
   {
      BDEBUG_ASSERT(sizeof(T*) == sizeof(uint32));
      
      if (NULL == ptr)
         return ptr = reinterpret_cast<T*>(static_cast<size_t>(0xFFFFFFFF));
      
      BDEBUG_ASSERT(ptr >= reinterpret_cast<T*>(pBase));
      
      return ptr = reinterpret_cast<T*>(
         (reinterpret_cast<char*>(ptr) - reinterpret_cast<char*>(pBase))
         );
   }
   
   // Converts an offset into a pointer.
   // Assumes sizeof(T*)==sizeof(uint32)
   template <typename T>
      T* Pointerize(T*& ptr, void* pBase)
   {
      BDEBUG_ASSERT(sizeof(T*) == sizeof(uint32));
      
      if (ptr == reinterpret_cast<T*>(static_cast<size_t>(0xFFFFFFFF)))
         return ptr = NULL;
         
      return ptr = reinterpret_cast<T*>(
         (reinterpret_cast<char*>(pBase) + *reinterpret_cast<uint32*>(&ptr))
         );
   }
   
   template <typename T>
      const T* Pointerize(const T*& ptr, const void* pBase)
   {
      BDEBUG_ASSERT(sizeof(const T*) == sizeof(uint32));
      
      if (ptr == reinterpret_cast<const T*>(static_cast<size_t>(0xFFFFFFFF)))
         return ptr = NULL;
         
      return ptr = reinterpret_cast<const T*>(
         (reinterpret_cast<const char*>(pBase) + *reinterpret_cast<uint32*>(&ptr))
         );
   }

#ifdef new
   #pragma push_macro("new")
   #undef new
   #define NEW_PUSHED
#endif

   // This WILL NOT initialize basic types such as int to 0!
   template <class X> inline X*           ConstructInPlace(X* p) { return new (static_cast<void*>(p)) X; }
   
   // This WILL NOT initialize basic types such as int to 0!
   template <class X, class Y> inline X*  ConstructInPlace(X* p, const Y& src) { return new (static_cast<void*>(p)) X(src); }
      
   template <class X> void                ConstructArrayInPlace(X* p, uint n) { for (uint i = n; i > 0; i--, p++) new (static_cast<void*>(p)) X; }
   template <class X, class Y> void       ConstructArrayInPlace(X* p, uint n, const Y& src) { for (uint i = n; i > 0; i--, p++) new (static_cast<void*>(p)) X(src); }
   
#ifdef NEW_PUSHED
   #undef NEW_PUSHED
   #pragma pop_macro("new")
#endif

   template <class X> inline void DestructInPlace(X* p) {	if (p) p->~X(); } 
   template <class X> inline void DestructArrayInPlace(X* p, uint n) {	if (p) { for (uint i = 0; i < n; i++) p[i].~X(); } } 
            
   inline uint64 CreateUInt64(DWORD low, DWORD high) { return (static_cast<uint64>(high) << 32) | static_cast<uint64>(low); }
   inline int64 CreateInt64(DWORD low, DWORD high) { return (int64)CreateUInt64(low, high); }
   
   inline void SeparateInt64(int64 li, DWORD& low, LONG& high) { low = static_cast<DWORD>(li & 0xFFFFFFFF); high = static_cast<LONG>(li >> 32); }
   inline void SeparateUInt64(uint64 li, DWORD& low, DWORD& high) { low = static_cast<DWORD>(li & 0xFFFFFFFF); high = static_cast<DWORD>(li >> 32); }
   
   inline int64 LargeIntegerToInt64(const LARGE_INTEGER& li) { return CreateInt64(li.LowPart, li.HighPart); }
   inline uint64 LargeIntegerToUInt64(const LARGE_INTEGER& li) { return CreateUInt64(li.LowPart, li.HighPart); }
               
   inline LARGE_INTEGER Int64ToLargeInteger(const uint64& li) { LARGE_INTEGER ret; SeparateInt64(li, ret.LowPart, ret.HighPart); return ret; }
   inline LARGE_INTEGER UInt64ToLargeInteger(const uint64& li) { LARGE_INTEGER ret; SeparateUInt64(li, ret.LowPart, reinterpret_cast<DWORD&>(ret.HighPart)); return ret; }
   
   inline uint64 FileTimeToUInt64(const FILETIME& fileTime) { return CreateUInt64(fileTime.dwLowDateTime, fileTime.dwHighDateTime); }
   inline FILETIME UInt64ToFileTime(uint64 time) { FILETIME ret; ret.dwLowDateTime = static_cast<DWORD>(time & 0xFFFFFFFF); ret.dwHighDateTime = static_cast<DWORD>(time >> 32); return ret; }
   
   inline bool GetBitFlag(uint flags, uint msk) { return (flags & msk) != 0; }
   
   inline uint SetBitFlag(uint flags, uint msk, bool val) { if (val) flags |= msk; else flags &= (~msk); return flags; }
   
   void GetHRESULTDesc(HRESULT hres, char* pBuf, uint bufLen);
      
   // Bubblesort - Useful for sorting tiny arrays.
   template<typename T> void BubbleSort(T pStart, T pEnd);
   template<typename T, typename F> void BubbleSortElemFunc(T pStart, T pEnd, F elemFunc);
   template<typename T, typename F> void BubbleSortPredFunc(T pStart, T pEnd, F predFunc);
               
   // Shellsort - O(n^1.25) to O(n^1.5) - Useful for medium size arrays that fit in the L1/L2 cache.
   template<typename T> void ShellSort(T pStart, T pEnd);
   template<typename T, typename F> void ShellSortElemFunc(T pStart, T pEnd, F elemFunc);
   template<typename T, typename F> void ShellSortPredFunc(T pStart, T pEnd, F predFunc);
   
   // Returns true if the region defined by [p, len] completely refers to virtual memory (and is therefore cachable).
   // Always returns true on the PC.
   bool IsVirtualMemory(void* p, uint len);
   
   // Calls XMemCpy() on 360. DO NOT use this on noncached memory.
   // According to the docs, it is always better to call XMemCpy() than memcpy() on cached memory.
   PVOID FastMemCpy(PVOID dst, const VOID *src, SIZE_T count);
   
   PVOID FastMemSet(PVOID dest, int c, SIZE_T count);

   // Clears every 128-byte cache line that lies completely within the specified block.
   // pStart does not need to be aligned. The cleared area will never lie outside the specified block.
   // Not every byte will be cleared to 0 if pStart or len is not 128 byte aligned.
   void ClearCacheLines(void* p, uint len);
   
   void StoreCacheLines(void* p, uint len);
   void FlushCacheLines(void* p, uint len);
   void TouchCacheLine(int offset, const void* base);
   void TouchCacheLines(void* p, uint len);
   
   typedef const void* BPrefetchState;
   
   BPrefetchState BeginPrefetch(const void* p, uint cacheLinesToReadAhead);
   BPrefetchState BeginPrefetch(const void* p, const void* pEnd, uint cacheLinesToReadAhead);
   BPrefetchState UpdatePrefetch(BPrefetchState state, const void* p, uint cacheLinesToReadAhread);
   BPrefetchState UpdatePrefetch(BPrefetchState state, const void* p, const void* pEnd, uint cacheLinesToReadAhread);
   
   BPrefetchState BeginPrefetchLargeStruct(const void* p, const void* pEnd, uint cacheLinesToReadAhead);
   BPrefetchState BeginPrefetchLargeStruct(const void* p, uint cacheLinesToReadAhead);
   BPrefetchState UpdatePrefetchLargeStruct(BPrefetchState state, const void* p, const void* pEnd, uint cacheLinesToReadAhead);
   BPrefetchState UpdatePrefetchLargeStruct(BPrefetchState state, const void* p, uint cacheLinesToReadAhead);
   
   // Writes data to a Win32 disk file. Intended for debugging/development.
   bool WriteWin32File(const char* pFileName, const uchar* pData, uint len);
   
   // Reads data from a Win32 disk file. Intended for debugging/development.
   // NULL on failure, caller must delete[] the pointer when done.
   // Returns a buffer of 1 char for zero length files.
   uchar* ReadWin32File(const char* pFileName, uint& len);
   
   // rg [1/14/05] - As of the Nov SDK, you can only view the cache partition from the command 
   // prompt using xbcp while the game is running! Argh. 
   void MountUtilityDrive(BOOL fFormatClean = TRUE, DWORD dwBytesPerCluster = 8192, SIZE_T dwFileCacheSize = 65536);
   const BCHAR_T* GetUtilityDrivePath(void);     
            
   uint64 GetCounter(void);
   uint64 GetCounterFrequency(void);
   double GetCounterFrequencyAsDouble(void);
   double GetCounterOneOverFrequency(void);
   
   template<class InIt, class OutIt> inline OutIt Copy(InIt First, InIt Last, OutIt Dest) { for (; First != Last; ++Dest, ++First) *Dest = *First; return Dest; }
   template<class InIt, class Val> inline InIt Set(InIt First, InIt Last, Val V) { for (; First != Last; ++First) *First = V; return First; }
   
   template<typename T> struct BIsInteger { enum { Flag = false }; };
   template<> struct BIsInteger<char> { enum { Flag = true}; };
   template<> struct BIsInteger<uchar> { enum { Flag = true}; };
   template<> struct BIsInteger<short> { enum { Flag = true}; };
   template<> struct BIsInteger<ushort> { enum { Flag = true}; };
   template<> struct BIsInteger<int> { enum { Flag = true}; };
   template<> struct BIsInteger<uint> { enum { Flag = true}; };
   template<> struct BIsInteger<int64> { enum { Flag = true}; };
   template<> struct BIsInteger<uint64> { enum { Flag = true}; };
   
   template<typename T> struct BIsFloatingPoint { enum { Flag = false }; };
   template<> struct BIsFloatingPoint<float> { enum { Flag = true}; };
   template<> struct BIsFloatingPoint<double> { enum { Flag = true}; };
   
   // DO NOT use this class in global constructors!
   class BBitMasks
   {
      static uint64 mBitMasks[64];

      static void initBitMasks(void);

   public:
      BBitMasks() { initBitMasks(); }

      static inline uint64 get64(uint bit)          { BDEBUG_ASSERT((bit < 64U) && mBitMasks[0]); return mBitMasks[bit]; }
      static inline uint64 getInverted64(uint bit)  { BDEBUG_ASSERT((bit < 64U) && mBitMasks[0]); return ~mBitMasks[bit]; }

      static inline uint get(uint bit)              { BDEBUG_ASSERT((bit < 32U) && mBitMasks[0]); return (uint)mBitMasks[bit]; }
      static inline uint getInverted(uint bit)      { BDEBUG_ASSERT((bit < 32U) && mBitMasks[0]); return ~((uint)mBitMasks[bit]); }
   };
   
   uint CountLeadingZeros64(uint64 value);
   uint CountLeadingZeros32(uint value);
   
   uint CountBits(uint value);
   
   void ClearCaches(void);
                                                                     
} // namespace Utils

#include "utils.inl"

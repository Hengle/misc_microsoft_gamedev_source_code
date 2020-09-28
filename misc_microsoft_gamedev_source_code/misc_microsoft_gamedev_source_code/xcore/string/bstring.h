//============================================================================
//
//  BString.h
//
//  Copyright (c) 2005, Ensemble Studios
//
//============================================================================
#pragma once

#if defined(UNICODE) || defined(_UNICODE)
   #error UNICODE or _UNICODE cannot be defined here right now.
#endif

#if !defined(XBOX) && defined(BUILD_DEBUG)
   // Puts a header/footer of 0xFFFFFFFF around the BString class to catch printf("%s", xxx) type problems.
   #define BSTRING_PADDING
#endif   

// Maximum length of string in characters (leaves enough for the terminating NULL).
const uint cMaxStringLen = USHRT_MAX - 1;

//----------------------------------------------------------------------------
// Dynamic string allocator. Each string has a pointer to its heap.
//----------------------------------------------------------------------------
extern BMemoryHeap gStringHeap;

class BStringHeapAllocator : public BHeapAllocator
{
public:
   typedef BHeapAllocator Base;
   BStringHeapAllocator(BMemoryHeap* pHeap = &gStringHeap) : Base(pHeap) { }
   BStringHeapAllocator(const BStringHeapAllocator& other) : Base(other) { }
};

extern BStringHeapAllocator gStringHeapStringAllocator;
extern BStringHeapAllocator gPrimaryHeapStringAllocator;
extern BStringHeapAllocator gRenderHeapStringAllocator;
extern BStringHeapAllocator gSimHeapStringAllocator;
//----------------------------------------------------------------------------
// Fixed allocator. Permits the user to create custom zero-overhead string types.
// However, strings customized in this manner can't be used with our existing
// utility routines that accept BString's.
//----------------------------------------------------------------------------
struct BGetStringHeapPolicy { BMemoryHeap& getHeap(void) const { return gStringHeap; } };
struct BStringFixedHeapAllocator  : BFixedHeapAllocator<BGetStringHeapPolicy>  { };
extern BStringFixedHeapAllocator gStringFixedHeapAllocator;

//----------------------------------------------------------------------------
// Public Structs
//----------------------------------------------------------------------------
// String tracking is currently inoperative!
#define BSTRING_ENABLE_TRACKING 0

struct BStringHeader { };
//----------------------------------------------------------------------------
//  Externs
//----------------------------------------------------------------------------
extern const char* cStringDelimeter;

//----------------------------------------------------------------------------
//  Class BStringTemplate
//  If you change these template params, be sure to update stream.h
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator = BStringFixedHeapAllocator>
class BStringTemplate : private Allocator
{
public:
   friend class BFile;
   
   friend BStringTemplate<char, Allocator>;
   friend BStringTemplate<WCHAR, Allocator>;
   
   typedef CharType charType;
   typedef typename BOppositeCharType<CharType>::t oppositeCharType;
   typedef BStringTemplate<oppositeCharType, Allocator> oppositeStringType;
   typedef Allocator allocator;
   
   enum 
   {
      cBytesPerChar = sizeof(CharType),
      cBytesPerCharLog2 = ((cBytesPerChar == 1) ? 0 : 1)
   };
   
   //-- Construction/Destruction
   BStringTemplate(Allocator& alloc = Allocator());
   
   BStringTemplate(const BStringTemplate& srcString);
   
   BStringTemplate(const oppositeStringType& srcString);
      
   BStringTemplate(const BStringTemplate& srcString, Allocator& alloc);
   BStringTemplate(const oppositeStringType& srcString, Allocator& alloc);
   
   BStringTemplate(const CharType* pSrcString, Allocator& alloc = Allocator());
   BStringTemplate(const oppositeCharType* pSrcString, Allocator& alloc = Allocator());

   template<typename OtherAllocator>
   BStringTemplate(const BStringTemplate<CharType, OtherAllocator>& srcString, Allocator& alloc = Allocator());
          
   ~BStringTemplate();
   
   // -- Allocator Control
   const Allocator& getAllocator(void) const { return *this; }
         Allocator& getAllocator(void)       { return *this; }
   
   // setAllocator() will copy the current string to the new heap if necessary.
   void setAllocator(Allocator& alloc);
   
   //-- Setting
   void         set       (const char* pSrcString, long srcCount = -1, long srcPos = -1);
   void         set       (const WCHAR* pSrcString, long srcCount = -1, long srcPos = -1);
   
   void         set       (const BStringTemplate& srcString, long srcCount = -1, long srcPos = -1) { set(srcString.getPtr(), srcCount, srcPos); }
   void         set       (const oppositeStringType& srcString, long srcCount = -1, long srcPos = -1) { set(srcString.getPtr(), srcCount, srcPos); }
            
   //-- Modification
   void         empty            (void);
   void         setNULL          (void); // explictly free the memory buffer
   void         copy             (const CharType* srcString, long srcCount = -1, long srcPos = -1);
   long         copyTok          (const BStringTemplate& srcString, long srcCount = -1, long srcPos = -1, const CharType *pTokens = BStringDefaults<CharType>::getSpaceString());
   void         append           (const BStringTemplate& srcString, long srcCount = -1, long srcPos = -1);
   void         append           (const CharType* srcString, long srcCount = -1, long srcPos = -1);
   void         append           (CharType c);
   void         prepend          (const BStringTemplate& srcString, long srcCount = -1, long srcPos = -1);
   void         insert           (long dstPos, const BStringTemplate& srcString, long srcCount = -1, long srcPos = -1);
   void         insert           (long dstPos, CharType srcChr);
   void         remove           (long dstPos, const BStringTemplate& srcString, long srcCount = -1, long srcPos = -1);
   void         remove           (long dstPos, long count);
   void         remove           (CharType srcChr);
   
   // [firstChr, lastChr] define a closed interval, i.e. lastChr is the index of the last char to be cropped.
   // crop() may call BFAIL if the indices are invalid! See strCrop().
   void         crop             (long firstChr, long lastChr);
         
   // substring() is like crop(), except start, end define a half open interval: [start, end).
   // end should be the index of the character AFTER the end of the desired substring.
   // substring() will never call BFAIL, unlike crop(). If the parameters are invalid, the string will be set to empty.
   // Returns false if the string was set to empty because start was too high, or end was <= start. 
   bool         substring        (uint start, uint end);
   
   // left(), mid(), and right() utilize substring() to extract different sections of the string.
   void         left(uint numChars)         { substring(0, numChars); }
   void         mid(uint start, uint len)   { substring(start, start + len); }
   void         right(uint start)           { substring(start, UINT_MAX); }
   
   // localization format (dependent on format string lib from MGS
   void         locFormat        (const CharType* pFormat, ...);
   void         locFormatArgs    (const CharType* pFormat, va_list args);

   void         format           (const CharType* pFormat, ...);
   void         formatArgs       (const CharType* pFormat, va_list args);
   void         trimLeft         (const CharType* pTokens = BStringDefaults<CharType>::getSpaceString());
   void         trimRight        (const CharType* pTokens = BStringDefaults<CharType>::getSpaceString());
   void         toLower          (void);
   void         toUpper          (void);
   void         removePath       (void);
   void         removeExtension  (void);
   void         removeTrailingPathSeperator(void);
   void         endianSwap       (void);
   void         check            (void) const;
   
   charType     getChar          (uint pos) const { BDEBUG_ASSERT(pos < (uint)length()); return getPtr()[pos]; }
   void         setChar          (uint pos, uint c) { BDEBUG_ASSERT(pos < (uint)length() && c); ((CharType*)getPtr())[pos] = static_cast<CharType>(c); }

   void         standardizePath  (void);
   void         standardizePathFrom (const BStringTemplate &src);
   void         standardizePathFrom (const CharType *src, long len = -1);

   //-- Searching
   long         findLeft      (const BStringTemplate& srcString, long startPos = -1) const;
   long         findRight     (const BStringTemplate& srcString, long startPos = -1) const;
   long         findLeft      (CharType srcChr, long startPos = -1) const;
   long         findRight     (CharType srcChr, long startPos = -1) const;
   bool         contains      (const BStringTemplate& srcString) const { return findLeft(srcString) >= 0; }

   //-- Search and Replace
   void         findAndReplace(const BStringTemplate& srcString, const BStringTemplate& newString);
   void         findAndReplace(CharType srcChr, CharType dstChr);

   //-- Status
   bool         isEmpty    (void) const;
   long         length     (void) const;
   long         compare    (const BStringTemplate& srcString, bool caseSensitive = false, long srcCount = -1) const;
   long         compare    (const CharType * pSrcString, bool caseSensitive = false, long srcCount = -1) const;

   //-- Conversions
   
   const WCHAR* asUnicode     (BStringTemplate<WCHAR, Allocator>& buffer) const;
   const char*  asANSI        (BStringTemplate<char, Allocator>& buffer) const;
   const BCHAR_T*  asBCHAR    (BStringTemplate<BCHAR_T, Allocator>& buffer) const { return asANSI(buffer); }
   
   // -- The "native" data type of our platform (360) is ANSI.
   const char*  asNative      (void) const { return getPtr(); }      
   
   // getPtr() returns a pointer to an empty string if string is empty.
   const CharType* getPtr   (void) const;

   long         asLong     (void) const;
   int64        asInt64    (void) const;
   uint64       asUInt64   (void) const;

   float        asFloat    (void) const;
   double       asDouble   (void) const;

   bool         convertToVector3(float* pVector3) const;

   void         setToLong  (long value);
   void         setToFloat (float value);
   void         setToDouble(double value);
   
   // Quickly swaps the contents of two strings.
   void         swap(BStringTemplate& other);

   //-- Operators
   
   // operator= does not change the string's allocator!
   BStringTemplate& operator =  (const BStringTemplate& srcString);
   BStringTemplate& operator =  (const oppositeStringType& srcString);
   template<typename OtherAllocator> BStringTemplate& operator =  (const BStringTemplate<CharType, OtherAllocator>& srcString);
   BStringTemplate& operator =  (const char* pSrcString);
   BStringTemplate& operator =  (const WCHAR* pSrcString);
   
   BStringTemplate& operator += (const BStringTemplate& srcString);
   template<typename OtherAllocator>  BStringTemplate& operator += (const BStringTemplate<CharType, OtherAllocator>& srcString);
   BStringTemplate& operator += (CharType srcChr);
   BStringTemplate& operator += (const CharType *src);
   BStringTemplate& operator += (const oppositeCharType *src);

   bool  operator == (const BStringTemplate& srcString) const;
   template<typename OtherAllocator> bool  operator == (const BStringTemplate<CharType, OtherAllocator>& srcString) const;
   bool  operator == (const CharType *srcString) const;
   
   bool  operator != (const BStringTemplate& srcString) const;
   template<typename OtherAllocator> bool  operator != (const BStringTemplate<CharType, OtherAllocator>& srcString) const;
   bool  operator != (const CharType *srcString) const;
   
   bool  operator >  (const BStringTemplate& srcString) const;
   template<typename OtherAllocator> bool  operator >  (const BStringTemplate<CharType, OtherAllocator>& srcString) const;
   
   bool  operator >=  (const BStringTemplate& srcString) const;
   template<typename OtherAllocator> bool  operator >= (const BStringTemplate<CharType, OtherAllocator>& srcString) const;
   
   bool  operator <  (const BStringTemplate& srcString) const;
   template<typename OtherAllocator> bool  operator <  (const BStringTemplate<CharType, OtherAllocator>& srcString) const;
   
   bool  operator <=  (const BStringTemplate& srcString) const;
   template<typename OtherAllocator> bool  operator <=  (const BStringTemplate<CharType, OtherAllocator>& srcString) const;
   
   friend BStringTemplate operator+ (const BStringTemplate& lhs, const BStringTemplate& rhs) { BStringTemplate result(lhs); result += rhs; return result; }

   operator const CharType* () const { return getPtr(); }
   
   uint hash(uint prevHash = 0) const;
                  
   static const BStringTemplate& getEmptyString(void);
   static int arrayAscendingSortFunc(const void *a, const void *b);
   static int arrayDescendingSortFunc(const void *a, const void *b);

   DWORD        maxChars   (void) const;      // maximum capacity of the current memory allocation in characters
   DWORD        maxBytes   (void) const;      // maximum capacity of the current memory allocation in bytes

   // jce [7/20/2005] -- added these to help make BFile::read faster by letting it manipulate the string directly.  Use with care.
   // for example, makeRawString does not do ANY initialization on the string data (no null terminator, etc)
   // size is in chars. Internal allocation will be (size + 1) * sizeof(charType) bytes.
   void         makeRawString(long size);
   
   // getString() will return NULL if string is empty!
   CharType*   getString(void) { return mpString; }

private:
   
   // Padding is used to help detect if someone tries to pass a BString to a vararg function like printf(), which is NOT supported!   
#ifdef BSTRING_PADDING   
   DWORD mPad0;
#endif   
   
   //-- Private data
   union
   {
      CharType* mpString;
      char*    mpStringA;
      WCHAR*   mpStringW;
   };
      
   ushort mDataLength;              // Data length of the string in characters (not including the zero terminator)
   ushort mBufLength;               // Size of the allocated buffer in characters
                     
#ifdef BSTRING_PADDING   
   DWORD mPad1;
#endif   

   void setDataLength(DWORD dataLength)
   {
      BASSERT(dataLength <= USHRT_MAX);
      mDataLength = static_cast<ushort>(dataLength);
   }
      
   void acquireBuffer(long size);
   void releaseBuffer(void);
   void trace(void);
   
   BStringHeader* newBuffer(uint numChars, uint bytesPerCharLog2, ushort& bufferChars);
   void deleteBuffer(BStringHeader* pData, uint bytesPerCharLog2, uint dataSize);
   void copyHeader(BStringHeader *pNewHeader, BStringHeader *pOldHeader);
};

// BString uses a fixed heap allocator that always allocates from the string heap (sizeof(BSimString)==8). 
typedef BStringTemplate<BCHAR_T, BStringFixedHeapAllocator> BString;
typedef BStringTemplate<WCHAR, BStringFixedHeapAllocator>   BUString;

// BSimString uses a fixed heap allocator that always allocates from the sim heap (sizeof(BSimString)==8). 
// Because it's a different type than BString, it's not compatible with our existing classes 
// and functions that accept BString's.
// Can only be used from the sim thread!
typedef BStringTemplate<BCHAR_T, BSimFixedHeapAllocator>    BSimString;
typedef BStringTemplate<WCHAR, BSimFixedHeapAllocator>      BSimUString;

typedef BStringTemplate<BCHAR_T, BRenderFixedHeapAllocator> BRenderString;
typedef BStringTemplate<WCHAR, BRenderFixedHeapAllocator>   BRenderUString;

typedef BStringTemplate<BCHAR_T, BCRunTimeFixedHeapAllocator> BCRunTimeString;
typedef BStringTemplate<WCHAR, BCRunTimeFixedHeapAllocator>   BCRunTimeUString;

class BStringEqualsCaseSensitive
{
public:
   bool operator()(const BString& a, const BString& b) const
   {
      return strcmp(a.getPtr(), b.getPtr()) == 0;
   }
};

class BUStringEqualsCaseSensitive
{
public:
   bool operator()(const BUString& a, const BUString& b) const
   {
      return wcscmp(a.getPtr(), b.getPtr()) == 0;
   }
};

#ifndef BUILD_FINAL
void BStringTest(void);
#endif

const long  FORMAT_WORKSPACE_SIZE   = 2048;
const long  NUMBER_WORKSPACE_SIZE   = 64;

extern BString sEmptyString;
extern BUString sEmptyUString;
extern BSimString sEmptySimString;

#include "bstring.inl"


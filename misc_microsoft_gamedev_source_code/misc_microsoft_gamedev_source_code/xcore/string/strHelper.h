//============================================================================
//
//  strHelper.h
//
//  Copyright (c) 2002-2007, Ensemble Studios
//
//============================================================================
#pragma once

//----------------------------------------------------------------------------
// struct BStrConv
//----------------------------------------------------------------------------
struct BStrConv
{
   static const char*         toA(const char* pString) { return pString; }
   static const BCHAR_T*      toB(const char* pString) { return pString; }             
   
   static const char*         toA(const BString& string) { return string.getPtr(); }                     
   static const char*         toA(const BString* pString) { return pString->getPtr(); }
   static const BCHAR_T*      toB(const BString& string) { return string.getPtr(); }    
   static const BCHAR_T*      toB(const BString* pString) { return pString->getPtr(); }
   
   static const char*         toA(const BSimString& string) { return string.getPtr(); }                     
   static const char*         toA(const BSimString* pString) { return pString->getPtr(); }
   static const BCHAR_T*      toB(const BSimString& string) { return string.getPtr(); }    
   static const BCHAR_T*      toB(const BSimString* pString) { return pString->getPtr(); }
};

//----------------------------------------------------------------------------
//  Helper Functions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Escapes an ANSI string for printing to an XML document.
//----------------------------------------------------------------------------
template<class T>
inline void escapeXMLString(T& str);

//----------------------------------------------------------------------------
// BStringArrayHelpers
//----------------------------------------------------------------------------
class BStringArrayHelpers
{
public:
   static const char* getString(const char* p, uint sizeInBytes, uint index);
   static const wchar_t* getString(const wchar_t* p, uint sizeInBytes, uint index);
   static bool getSize(const char* p, uint sizeInBytes, uint& size);
   static bool getSize(const wchar_t* p, uint sizeInBytes, uint& size);
};   

//----------------------------------------------------------------------------
// Fuzzy string matching function - From Game Programming Gems 6: "Closest String Matching"
//----------------------------------------------------------------------------
float stringMatch(char const *left, char const *right);

//----------------------------------------------------------------------------
// strCopy
//    Parameters:
//       pDst        - The destination buffer to insert into. Non-NULL
//       dstLen      - The length of the destination buffer. Non-Zero (chars)
//       pSrc        - The source buffer to insert.
//       srcLen      - The length of the source buffer. Must be Non-Neg if srcCount/srcPos are specified.
//       srcCount    - The amount of pSrc to copy. default everything
//       srcPos      - The starting position in pSrc. default start at pos 0
//    Return:
//       -1 on Error, otherwise new pDst length in characters.
//----------------------------------------------------------------------------
template<typename CharType>
long strCopy(CharType* pDst, DWORD dstLen, const CharType* pSrc, long srcLen=-1, long srcCount = -1, long srcPos = -1);

//----------------------------------------------------------------------------
// strInsert  - inserts a substring
//    Parameters:
//       pDst        - The destination buffer to insert into. Non-NULL
//       dstLen      - The length of the destination buffer. Non-Zero (chars)
//       dstPos      - The starting position in pDst. Non-Negative
//       pSrc        - The source buffer to insert.
//       srcLen      - The length of the source buffer.Must be Non-Neg if srcCount/srcPos are specified.
//       srcCount    - The amount of pSrc to insert. default everything
//       srcPos      - The starting position in pSrc. default start at pos 0
//    Return:
//       -1 on Error, otherwise new pDst length in characters.
//----------------------------------------------------------------------------
template<typename CharType>
long strInsert(CharType* pDst, DWORD dstLen, DWORD dstPos, const CharType* pSrc, long srcLen=-1, long srcCount = -1, long srcPos = -1);

template<typename CharType>
long strInsertCh  (CharType* pDst, DWORD dstLen, DWORD dstPos, CharType srcChr);

template<typename CharType>
long strRemove    (CharType* pDst, DWORD dstLen, DWORD dstPos, const CharType* pSrc, long srcLen=-1, long srcCount = -1, long srcPos = -1);

template<typename CharType>
long strRemove    (CharType* pDst, DWORD dstLen, DWORD dstPos, long count = -1);

template<typename CharType>
long strRemoveCh  (CharType* pDst, DWORD dstLen, DWORD dstPos, CharType srcChr);

template<typename CharType>
long strCrop      (CharType* pDst, DWORD dstLen, long firstPos, long lastPos);

template<typename CharType>
bool strFormat    (CharType* pDst, DWORD dstLen, const CharType* pFormat, ...);

template<typename CharType>
bool strFormat    (CharType* pDst, DWORD dstLen, const CharType* pFormat, va_list args);

template<typename CharType>
long strTrimLeft  (CharType* pDst, DWORD dstLen, const CharType* pTokens = BStringDefaults<CharType>::getSpaceString());

template<typename CharType>
long strTrimRight (CharType* pDst, DWORD dstLen, const CharType* pTokens = BStringDefaults<CharType>::getSpaceString());

template<typename CharType>
void strToLower   (CharType* pDst, DWORD dstLen);

template<typename CharType>
void strToUpper   (CharType* pDst, DWORD dstLen);

char strToLowerCh (char chr);
WCHAR strToLowerCh (WCHAR chr);

char strToUpperCh (char chr);
WCHAR strToUpperCh (WCHAR chr);

//-- Searching
template<typename CharType>
long strFindLeft      (const CharType* pDst, DWORD dstLen, const CharType* pSrc, DWORD srcLen, long startPos = -1);

template<typename CharType>
long strFindRight     (const CharType* pDst, DWORD dstLen, const CharType* pSrc, DWORD srcLen, long startPos = -1);

template<typename CharType>
long strFindLeftCh    (const CharType* pDst, DWORD dstLen, CharType srcChr, long startPos = -1);

template<typename CharType>
long strFindRightCh   (const CharType* pDst, DWORD dstLen, CharType srcChr, long startPos = -1);

//-- Search and Replace
template<typename CharType>
long strFindAndReplace(CharType* pDst, DWORD dstLen, const CharType* pSrc, const CharType* pNew);

template<typename CharType>
long strFindAndReplaceCh(CharType* pDst, DWORD dstLen, CharType srcChr, CharType dstChr);

//-- Status
template<typename CharType>
bool   strIsEmpty    (const CharType* pSrc);

template<typename CharType>
DWORD  strLength     (const CharType* pSrc);

template<typename CharType>
long   strCompare    (const CharType* pDst, DWORD dstLen, const CharType* pSrc, DWORD srcLen, bool caseSensitive = false, long srcCount = -1);

//-- Conversions
template<typename CharType>
long   strGetAsLong     (const CharType* pSrc, DWORD srcLen);

template<typename CharType>
float  strGetAsFloat    (const CharType* pSrc, DWORD srcLen);

template<typename CharType>
double strGetAsDouble   (const CharType* pSrc, DWORD srcLen);

template<typename CharType>
long   strSetToLong     (CharType* pDst, DWORD dstLen, long value);

template<typename CharType>
long   strSetToFloat    (CharType* pDst, DWORD dstLen, float value);

template<typename CharType>
long   strSetToDouble   (CharType* pDst, DWORD dstLen, double value);

// Converts wacky chars to XML escape codes so you can put the string safely in an XML file.  This expands the string, so if dest buffer isn't big enough, it will return false.
bool strwEscapeForXML(const WCHAR *src, WCHAR *dst, long dstSize);

inline int bcsicmp(const BCHAR_T *string1, const BCHAR_T *string2)
{
#ifdef UNICODE
   return _wcsicmp(string1, string2);
#else
   return _stricmp(string1, string2);
#endif   
}

inline int bcscmp(const BCHAR_T* string1, const BCHAR_T* string2)
{           
#ifdef UNICODE
   return wcscmp(string1, string2);
#else
   return strcmp(string1, string2);
#endif
}

inline BCHAR_T *bcscpy(
                BCHAR_T *strDestination,
                uint destSizeInChars,
                const BCHAR_T *strSource 
                )
{
#ifdef UNICODE
   wcscpy_s(strDestination, destSizeInChars, strSource);
#else
   strcpy_s(strDestination, destSizeInChars, strSource);
#endif
   return strDestination;
}                

inline BCHAR_T *bcscat(
                BCHAR_T *strDestination,
                uint destSizeInChars,
                const BCHAR_T *strSource 
                )
{
#ifdef UNICODE
   wcscat_s(strDestination, destSizeInChars, strSource);
#else
   strcat_s(strDestination, destSizeInChars, strSource);
#endif
   return strDestination;
}                

inline BCHAR_T *bcslwr(BCHAR_T* string, uint stringSizeInChars)
{
#ifdef UNICODE
   _wcslwr_s(string, stringSizeInChars);
#else
   _strlwr_s(string, stringSizeInChars);
#endif
   return string;
}

inline BCHAR_T *bcsupr(BCHAR_T* string, uint stringSizeInChars)
{
#ifdef UNICODE
   _wcsupr_s(string, stringSizeInChars);
#else
   _strupr_s(string, stringSizeInChars);
#endif
   return string;
}

inline BCHAR_T *bcsncpy(BCHAR_T* strDest,
                 uint destSizeInChars,
                 const BCHAR_T* strSource,
                 size_t count)
{
#ifdef UNICODE
   wcsncpy_s(strDest, destSizeInChars, strSource, count);
#else                 
   strncpy_s(strDest, destSizeInChars, strSource, count);
#endif   
   return strDest;
}   

inline BCHAR_T toblower( BCHAR_T c )
{
#ifdef UNICODE
   return static_cast<BCHAR_T>(towlower(c));
#else
   return static_cast<BCHAR_T>(tolower(c));
#endif
}

inline BCHAR_T tobupper( BCHAR_T c )
{
#ifdef UNICODE
   return static_cast<BCHAR_T>(towupper(c));
#else
   return static_cast<BCHAR_T>(toupper(c));
#endif
}

inline size_t bcslen(const BCHAR_T* string)
{
#ifdef UNICODE
   return wcslen(string);
#else
   return strlen(string);
#endif   
}

inline BCHAR_T* bcschr(
             const BCHAR_T* string,
             int c 
             )
{
#ifdef UNICODE
   return const_cast<WCHAR*>(wcschr(string, c));
#else
   return const_cast<BCHAR_T*>(strchr(string, c));
#endif
}             

inline int bcsncmp(
            const BCHAR_T *string1,
            const BCHAR_T *string2,
            size_t count 
            )
{            
#ifdef UNICODE
   return wcsncmp(string1, string2, count);
#else
   return strncmp(string1, string2, count);
#endif
}

inline int bcsnicmp(
              const BCHAR_T *string1,
              const BCHAR_T *string2,
              size_t count 
              )
{
#ifdef UNICODE
   return wcsnicmp(string1, string2, count);
#else
   return _strnicmp(string1, string2, count);
#endif
}              

inline double bcstod(
              const BCHAR_T *nptr,
              BCHAR_T**endptr 
              )      
              
{
#ifdef UNICODE
   return bcstod(nptr, endptr);
#else
   return strtod(nptr, endptr);
#endif   
}          

inline BCHAR_T *bcsrchr(
                 const BCHAR_T*string,
                 BCHAR_T c 
                 )
{                 
#ifdef UNICODE
   return const_cast<WCHAR*>(wcsrchr(string, c));
#else
   return const_cast<char*>(strrchr(string, c));
#endif
}

inline long bcstol(
            const BCHAR_T *nptr,
            BCHAR_T **endptr,
            int base 
            )
{
#ifdef UNICODE
   return wcstol(nptr, endptr, base);
#else
   return strtol(nptr, endptr, base);
#endif
}            

inline size_t bcsspn(
              const BCHAR_T *string,
              const BCHAR_T *strCharSet 
              )
{              
#ifdef UNICODE
   return wcsspn(string, strCharSet);
#else
   return strspn(string, strCharSet);
#endif   
}

inline BCHAR_T *bcsstr(
               const BCHAR_T *string,
               const BCHAR_T *strSearch 
                )
{
#ifdef UNICODE
   return const_cast<WCHAR*>(wcsstr(string, strSearch));
#else
   return const_cast<char*>(strstr(string, strSearch));
#endif
}                

#ifdef UNICODE
   #define _itob _itow
#else
   #define _itob _itoa
#endif

#ifdef UNICODE
   #define sbscanf swscanf
#else
   #define sbscanf sscanf
#endif

#ifdef XBOX
   #ifdef UNICODE
      #define StringCchVPrintfEx  StringCchVPrintfExW
      #define StringCchVPrintf StringCchVPrintfW
      #define StringCchCat StringCchCatW
      #define StringCchCatN StringCchCatNW
      #define StringCchCopy StrHelperStringCchCopyW
      #define StringCchCopyNEx StringCchCopyNExW
      #define StringCchLength StrHelperStringCchLengthW
      #define StringCchPrintf StringCchPrintfW
   #else
      #define StringCchVPrintfEx  StrHelperStringCchVPrintfExA
      #define StringCchVPrintf StringCchVPrintfA
      #define StringCchCat StringCchCatA
      #define StringCchCatN StringCchCatNA
      #define StringCchCopy StringCchCopyA
      #define StringCchCopyNEx StringCchCopyNExA
      #define StringCchLength StringCchLengthA
      #define StringCchPrintf StringCchPrintfA
   #endif // !UNICODE

   #define STRSAFE_FILL_BEHIND_NULL    0x00000200  // fill in extra space behind the null terminator
   #define STRSAFE_MAX_CCH INT_MAX

   HRESULT StringCchCopyA(          
      char* pszDest,
      size_t cchDest,
      const char* pszSrc
      );
   
   HRESULT StrHelperStringCchCopyW(          
      wchar_t* pszDest,
      size_t cchDest,
      const wchar_t* pszSrc
      );
  
   HRESULT StringCchVPrintfW(          
      wchar_t* pszDest,
      size_t cchDest,
      const wchar_t* pszFormat,
      va_list argList
      );
   
   HRESULT StringCchCatW(          
      wchar_t* pszDest,
      size_t cchDest,
      const wchar_t* pszSrc
      );

   HRESULT StringCchCatNA(char* pszDest, size_t cchDest, const char* pszSrc, size_t cchMaxAppend);
   HRESULT StringCchCatNW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc, size_t cchMaxAppend);

   HRESULT StrHelperStringCchVPrintfExA(
      char* pszDest, 
      size_t cchDest, 
      char** ppszDestEnd, 
      size_t* pcchRemaining, 
      unsigned long dwFlags, 
      const char* pszFormat, 
      va_list argList
      );

   HRESULT StringCchVPrintfExW(
      wchar_t* pszDest, 
      size_t cchDest, 
      wchar_t** ppszDestEnd, 
      size_t* pcchRemaining, 
      unsigned long dwFlags, 
      const wchar_t* pszFormat, 
      va_list argList);

   HRESULT StringCchVPrintfA(
      char* pszDest,
      size_t cchDest,
      const char* pszFormat,
      va_list argList
      );

   HRESULT StringCchPrintfA(
      char* pszDest,
      size_t cchDest,
      const char* pszFormat,
      ...);
           
   HRESULT StringCchPrintfW(
      wchar_t* pszDest,
      size_t cchDest,
      const wchar_t* pszFormat,
      ...);

   HRESULT StringCchCatA(
      char* pszDest,
      size_t cchDest,
      const char* pszSrc);
        
   HRESULT StringCchCopyNExA(char* pszDest, size_t cchDest, const char* pszSrc, size_t cchSrc, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags);
   HRESULT StringCchCopyNExW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc, size_t cchSrc, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags);

   HRESULT StringCchLengthA(          
      const char* psz,
      size_t cchMax,
      size_t *pcch
      );

   HRESULT StrHelperStringCchLengthW(          
      const wchar_t* psz,
      size_t cchMax,
      size_t *pcch
      );
#endif   

//==============================================================================
// locFormatFloat
// helper function for formatting a float using a localized decimal separator
//==============================================================================
void locFormatFloat(wchar_t * dest, size_t destSizeInWords, const float value, const unsigned int numDecimalPlaces, const wchar_t * decimalSeparator);

#include "strHelper.inl"


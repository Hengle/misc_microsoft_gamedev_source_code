//============================================================================
//
//  strHelper.cpp
//
//  Copyright (c) 2002-2007, Ensemble Studios
//
//============================================================================


//============================================================================
//  INCLUDES
//============================================================================
#include "xcore.h"
#include "bsnprintf.h"
#include "strHelper.h"

#pragma warning( push )
#pragma warning( disable : 4018 )

const char* BStringArrayHelpers::getString(const char* p, uint sizeInBytes, uint index)
{
   if (!p)
      return NULL;
      
   uint curStringIndex = 0;
   for (uint i = 0; i < sizeInBytes; i++)
   {
      if (curStringIndex == index)
         return p + i;

      if (!p[i])
         curStringIndex++;
   }

   return NULL;
}

const wchar_t* BStringArrayHelpers::getString(const wchar_t* p, uint sizeInBytes, uint index)
{
   if (!p)
      return NULL;
      
   uint curStringIndex = 0;
   const uint sizeInChars = sizeInBytes >> 1;
   for (uint i = 0; i < sizeInChars; i++)
   {
      if (curStringIndex == index)
         return p + i;

      if (!p[i])
         curStringIndex++;
   }

   return NULL;
}

bool BStringArrayHelpers::getSize(const char* p, uint sizeInBytes, uint& size)
{
   size = 0;
   if ((!p) || (!sizeInBytes))
      return true;

   if (p[sizeInBytes - 1] != 0)
      return false;

   for (uint i = 0; i < sizeInBytes; i++)
      if (!p[i])
         size++;

   return true;
}

bool BStringArrayHelpers::getSize(const wchar_t* p, uint sizeInBytes, uint& size)
{
   size = 0;

   if (!p)
      return false;
   
   if (sizeInBytes & 1) 
      return false;
   else if (!sizeInBytes)
      return true;

   const uint sizeInChars = sizeInBytes >> 1;

   if (p[sizeInChars - 1] != 0)
      return false;

   for (uint i = 0; i < sizeInChars; i++)
      if (!p[i])
         size++;

   return true;
}


// From Game Programming Gems 6: "Closest String Matching"
// This is the actual string matching algorithm.  It
// returns a value from zero to one indicating an approximate 
// percentage of how closely two strings match.
const float CAP_MISMATCH_VAL = 0.9f;
float stringMatch(char const *left, char const *right)
{
   // get the length of the left, right, and longest string 
   // (to use as a basis for comparison in value calculateions)
   size_t leftSize = strlen(left);
   size_t rightSize = strlen(right);
   size_t largerSize = (leftSize > rightSize) ? 
leftSize : rightSize;
   char const *leftPtr = left;
   char const *rightPtr = right;
   float matchVal = 0.0f;
   // Iterate through the left string until we run out of room
   while(leftPtr != (left + leftSize) && 
      rightPtr != (right + rightSize))
   {
      // First, check for a simple left/right match
      if(*leftPtr == *rightPtr)
      {
         // If it matches, add a proportional character's value
         // to the match total.
         matchVal += 1.0f / largerSize;
         // Advance both pointers
         ++leftPtr; 
         ++rightPtr;
      }
      // If the simple match fails, 
      // try a match ignoring capitalization
      else if(::tolower(*leftPtr) == ::tolower(*rightPtr))
      {
         // We'll consider a capitalization mismatch worth 90% 
         // of a normal match
         matchVal += CAP_MISMATCH_VAL / largerSize;
         // Advance both pointers
         ++leftPtr; 
         ++rightPtr;
      }
      else
      {
         char const *lpbest = left + leftSize;
         char const *rpbest = right + rightSize;
         int totalCount = 0;
         int bestCount = INT_MAX;
         int leftCount = 0;
         int rightCount = 0;
         // Here we loop through the left word in an outer loop,
         // but we also check for an early out by ensuring we
         // don't exceed our best current count
         for(char const *lp = leftPtr; (lp != (left + leftSize) 
            && ((leftCount + rightCount) < bestCount)); ++lp)
         {
            // Inner loop counting
            for(char const *rp = rightPtr; (rp != (right + 
               rightSize) && ((leftCount + rightCount) < 
               bestCount)); ++rp)
            {
               // At this point, we don't care about case
               if(::tolower(*lp) == ::tolower(*rp))
               {
                  // This is the fitness measurement
                  totalCount = leftCount + rightCount;
                  if(totalCount < bestCount)
                  {
                     bestCount = totalCount;
                     lpbest = lp;
                     rpbest = rp;
                  }
               }
               ++rightCount;
            }
            ++leftCount;
            rightCount = 0;
         }
         leftPtr = lpbest;
         rightPtr = rpbest;
      }
   }
   // clamp the value in case of floating-point error
   if(matchVal > 0.99f)
      matchVal = 1.0f;
   else if(matchVal < 0.01f)
      matchVal = 0.0f;
   return matchVal;
}

//----------------------------------------------------------------------------
//  Wide Character Helper Functions
//----------------------------------------------------------------------------

//============================================================================
// strEscapeForXML
//============================================================================
bool strwEscapeForXML(const WCHAR *src, WCHAR *dst, long dstSize)
{
   // Sanity.
   if(!src || !dst || (dstSize < 1))
   {
      BFAIL("strEscapeForXML -- bad params");
      return(false);
   }

   // Get max dst pos.
   WCHAR *maxDst = dst+(dstSize-1);

   // Step through string converting as needed.
   while(*src)
   {
      // If a normal char, just copy.
      if(*src == L'&')
      {
         // Format a wacky escape sequence.
         int chars = bsnwprintf(dst, maxDst-dst, L"&amp;");

         // If the whole thing couldn't fit, chop the string before this point instead of putting leaving the busted
         // escape sequence.
         if(chars<0)
         {
            *dst = 0;
            dst = maxDst;
         }
         else
            dst += chars;
      }
      else if((*src == 0) || (*src == L' ') || (*src == L'\\') || (*src == L'.') || (*src == L'_') || (*src == L'-') || (*src>=L'0' && *src<=L'9') || (*src>=L'a' && *src<=L'z') || (*src>=L'A' && *src<=L'Z'))
      {
         *dst = *src;
         dst++;
      }
      else
      {
         // Format a wacky escape sequence.
         int chars = bsnwprintf(dst, maxDst-dst, L"&#%d;", *src);

         // If the whole thing couldn't fit, chop the string before this point instead of putting leaving the busted
         // escape sequence.
         if(chars<0)
         {
            *dst = 0;
            dst = maxDst;
         }
         else
            dst += chars;
      }

      // Next.
      src++;

      if(dst == maxDst)
         break;
   }

   // Force terminate for safety.
   *dst = 0;

   // If more src, then fail.
   if(*src)
      return(false);

   // Success.
   return(true);
}

#if 0
//============================================================================
// StringCchCopyA
//============================================================================
HRESULT StringCchCopyA(char* pszDest, size_t cchDest, const char* pszSrc)
{
   BASSERT(pszDest);
   BASSERT(pszSrc);
   BASSERT(cchDest >= 1);

   if (0 != strcpy_s(pszDest, cchDest, pszSrc))
   {
      *pszDest = '\0';
      return E_FAIL;
   }

   return S_OK;
}    
#endif  

#ifdef XBOX
//============================================================================
// StrHelperStringCchCopyW
//============================================================================
HRESULT StrHelperStringCchCopyW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc)
{
   BASSERT(pszDest);
   BASSERT(pszSrc);
   BASSERT(cchDest >= 1);

   if (0 != wcscpy_s(pszDest, cchDest, pszSrc))
   {
      wcscpy_s(pszDest, cchDest, L"");
      return E_FAIL;
   }

   return S_OK;
}      

//============================================================================
// StringCchVPrintfW
//============================================================================
HRESULT StringCchVPrintfW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszFormat, va_list argList)
{
   BASSERT(pszDest); 
   BASSERT(pszFormat);
   BASSERT(cchDest >= 1);

//   BCOMPILETIMEASSERT(sizeof(TCHAR) == sizeof(wchar_t));

   HRESULT hr = S_OK;
   int result = _vsnwprintf_s(reinterpret_cast<wchar_t*>(pszDest), cchDest, _TRUNCATE, pszFormat, argList);
   if (result < 0)
   {
      hr = E_FAIL;
      wcscpy_s(reinterpret_cast<wchar_t*>(pszDest), cchDest, L"");
   }
   return hr;
}

//============================================================================
// StringCchCatW
//============================================================================
HRESULT StringCchCatW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc)
{
   BASSERT(pszDest);
   BASSERT(cchDest >= 1);

   HRESULT hres = S_OK;

//   BCOMPILETIMEASSERT(sizeof(TCHAR) == sizeof(wchar_t));

   if (0 != wcscat_s(reinterpret_cast<wchar_t*>(pszDest), cchDest, pszSrc))
   {
      hres = E_FAIL;
      wcscpy_s(pszDest, cchDest, L"");
   }

   return hres;
}

//============================================================================
// StringCchCatNA
//============================================================================
HRESULT StringCchCatNA(char* pszDest, size_t cchDest, const char* pszSrc, size_t cchMaxAppend)
{
   BASSERT(pszDest);
   BASSERT(pszSrc);
   BASSERT(cchDest >= 1);

   if (0 != strncat_s(pszDest, cchDest, pszSrc, cchMaxAppend))
   {
      *pszDest = '\0';
      return E_FAIL;
   }

   return S_OK;
}

//============================================================================
// StringCchCatNW
//============================================================================
HRESULT StringCchCatNW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc, size_t cchMaxAppend)
{
   BASSERT(pszDest);
   BASSERT(cchDest >= 1);

   HRESULT hres = S_OK;

//   BCOMPILETIMEASSERT(sizeof(TCHAR) == sizeof(wchar_t));

   if (0 != wcsncat_s(reinterpret_cast<wchar_t*>(pszDest), cchDest, pszSrc, cchMaxAppend))
   {
      hres = E_FAIL;
      wcscpy_s(pszDest, cchDest, L"");
   }

   return hres;
}

//============================================================================
// StrHelperStringCchVPrintfExA
//============================================================================
HRESULT StrHelperStringCchVPrintfExA(char* pszDest, size_t cchDest, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags, const char* pszFormat, va_list argList)
{
   BASSERT(pszDest);
   BASSERT(pszFormat);
   BASSERT(0 == dwFlags);
   BASSERT(cchDest >= 1);

   HRESULT hr = S_OK;

   if (ppszDestEnd)
      *ppszDestEnd = NULL;
   if (pcchRemaining)
      *pcchRemaining = 0;

   int result = _vsnprintf_s(pszDest, cchDest, _TRUNCATE, pszFormat, argList);
   if (result < 0)
   {
      hr = E_FAIL;
      strcpy_s(pszDest, cchDest, "");
   }
   else 
   {  
      if (ppszDestEnd)
         *ppszDestEnd = pszDest + result; 
      if (pcchRemaining)
         *pcchRemaining = cchDest - result;
   }

   return hr;
}

//============================================================================
// StringCchVPrintfExW
//============================================================================
HRESULT StringCchVPrintfExW(wchar_t* pszDest, size_t cchDest, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags, const wchar_t* pszFormat, va_list argList)
{
   BASSERT(pszDest);
   BASSERT(pszFormat);
   BASSERT(0 == dwFlags);
   BASSERT(cchDest >= 1);

   HRESULT hr = S_OK;

   if (ppszDestEnd)
      *ppszDestEnd = NULL;
   if (pcchRemaining)
      *pcchRemaining = 0;

   int result = _vsnwprintf_s(pszDest, cchDest, _TRUNCATE, pszFormat, argList);
   if (result < 0)
   {
      hr = E_FAIL;
      wcscpy_s(pszDest, cchDest, L"");
   }
   else 
   {
      if (ppszDestEnd)
         *ppszDestEnd = pszDest + result;
      if (pcchRemaining)
         *pcchRemaining = cchDest - result;
   }

   return hr;
}

#if 0
//============================================================================
// StringCchVPrintfA
//============================================================================
HRESULT StringCchVPrintfA(char* pszDest, size_t cchDest, const char* pszFormat, va_list argList)
{
   BASSERT(pszDest);
   BASSERT(cchDest >= 1);
   BASSERT(pszFormat);

   HRESULT hr = S_OK;

   int result = _vsnprintf_s(pszDest, cchDest, _TRUNCATE, pszFormat, argList);
   if (result < 0)
   {
      hr = E_FAIL;
      *pszDest = '\0';
   }

   return hr;
}
#endif

#if 0
//============================================================================
// StringCchPrintfA
//============================================================================
HRESULT StringCchPrintfA(char* pszDest, size_t cchDest, const char* pszFormat, ...)
{
   BASSERT(pszDest);
   BASSERT(pszFormat);
   BASSERT(cchDest >= 1);

   va_list argList;

   va_start(argList, pszFormat);     /* Initialize variable arguments. */

   HRESULT hres = StringCchVPrintfA(pszDest, cchDest, pszFormat, argList);

   va_end(argList);

   return hres;
}  
#endif

//============================================================================
// StringCchPrintfW
//============================================================================
HRESULT StringCchPrintfW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszFormat, ...)
{
   BASSERT(pszDest);
   BASSERT(pszFormat);
   BASSERT(cchDest >= 1);

   va_list argList;

   va_start(argList, pszFormat);     /* Initialize variable arguments. */

   HRESULT hres = StringCchVPrintfW(pszDest, cchDest, pszFormat, argList);

   va_end(argList);

   return hres;
}  

#if 0
//============================================================================
// StringCchCatA
//============================================================================
HRESULT StringCchCatA(char* pszDest, size_t cchDest, const char* pszSrc)
{
   BASSERT(pszDest);
   BASSERT(cchDest >= 1);
   BASSERT(pszSrc);

   HRESULT hr = S_OK;

   int result = strcat_s(pszDest, cchDest, pszSrc);
   if (0 != result)
   {
      *pszDest = '\0';
      hr = E_FAIL;
   }

   return hr;
}      
#endif

//============================================================================
// StringCchCopyNExA
//============================================================================
HRESULT StringCchCopyNExA(char* pszDest, size_t cchDest, const char* pszSrc, size_t cchSrc, char** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags)
{
   BASSERT(pszDest);
   BASSERT(pszSrc);
   BASSERT(cchDest >= 1);

   if (0 != strncpy_s(pszDest, cchDest, pszSrc, cchSrc))
   {
      *pszDest = '\0';
      return E_FAIL;
   }

   return S_OK;
}

//============================================================================
// StringCchCopyNExW
//============================================================================
HRESULT StringCchCopyNExW(wchar_t* pszDest, size_t cchDest, const wchar_t* pszSrc, size_t cchSrc, wchar_t** ppszDestEnd, size_t* pcchRemaining, unsigned long dwFlags)
{
   BASSERT(pszDest);
   BASSERT(pszSrc);
   BASSERT(cchDest >= 1);

   if (0 != wcsncpy_s(pszDest, cchDest, pszSrc, cchSrc))
   {
      wcscpy_s(pszDest, cchDest, L"");
      return E_FAIL;
   }

   return S_OK;
}

HRESULT StringCchLengthA(          
                         const char* psz,
                         size_t cchMax,
                         size_t *pcch
                         )
{
   BASSERT(psz);
   BASSERT(pcch);
   // RG [6/22/05] - FIXME
   *pcch = strlen(psz);
   BASSERT(*pcch < cchMax);
   return S_OK;
}                         

HRESULT StrHelperStringCchLengthW(          
                         const wchar_t* psz,
                         size_t cchMax,
                         size_t *pcch
                         )
{
   BASSERT(psz);
   BASSERT(pcch);
   // RG [6/22/05] - FIXME
   *pcch = wcslen(psz);
   BASSERT(*pcch < cchMax);
   return S_OK;
}

#endif // XBOX

//==============================================================================
// locFormatFloat
// helper function for formatting a float using a localized decimal separator
// Don't use more then seven decimal places or you could overflow 'after_decimal_portion'
// which will give you incorrect output
//==============================================================================
void locFormatFloat(wchar_t * dest, size_t destSizeInWords, const float value, const unsigned int numDecimalPlaces, const wchar_t * decimalSeparator)
{
   if(dest == NULL || decimalSeparator == NULL)
      return;

   const long cFmtStringLength = 8;
   wchar_t fmtString[cFmtStringLength];
   swprintf_s(fmtString, cFmtStringLength, L"%%.%if", numDecimalPlaces);
   swprintf_s(dest, destSizeInWords, fmtString, value);
   for(unsigned short i = 0; i < destSizeInWords; i++)
   {
      if(dest[i] == '.')
         dest[i] = *decimalSeparator;
   }
}

#pragma warning( pop )

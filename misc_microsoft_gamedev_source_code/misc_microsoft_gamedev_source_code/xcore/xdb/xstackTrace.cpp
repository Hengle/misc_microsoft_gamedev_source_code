//==============================================================================
// xstackTrace.cpp
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#include "xcore.h"

#ifdef XBOX
#include "xstacktrace.h"

#ifndef BUILD_FINAL
   #include <xbdm.h>
#endif

#define MM_VIRTUAL_4KB_BASE   ((ULONG)0x00000000)
#define MM_VIRTUAL_4KB_END    ((ULONG)0x3FFFFFFF)
#define MM_VIRTUAL_64KB_BASE  ((ULONG)0x40000000)
#define MM_VIRTUAL_64KB_END   ((ULONG)0x7FFFFFFF)
#define MM_IMAGE_64KB_BASE    ((ULONG)0x80000000)
#define MM_IMAGE_64KB_END     ((ULONG)0x8FFFFFFF)
#define MM_IMAGE_4KB_BASE     ((ULONG)0x90000000)
#define MM_IMAGE_4KB_END      ((ULONG)0x9FFFFFFF)
#define MM_PHYSICAL_64KB_BASE ((ULONG)0xA0000000)
#define MM_PHYSICAL_64KB_END  ((ULONG)0xBFFFFFFF)
#define MM_PHYSICAL_16MB_BASE ((ULONG)0xC0000000)
#define MM_PHYSICAL_16MB_END  ((ULONG)0xDFFFFFFF)
#define MM_PHYSICAL_4KB_BASE  ((ULONG)0xE0000000)
#define MM_PHYSICAL_4KB_END   ((ULONG)0xFFFFFFFF)

//==============================================================================
// BXStackTrace::BXStackTrace
//==============================================================================
BXStackTrace::BXStackTrace() : 
   mNumLevels(0)
{
}

//==============================================================================
// BXStackTrace::~BXStackTrace
//==============================================================================
BXStackTrace::~BXStackTrace()
{
}

//==============================================================================
// BXStackTrace::capture
//==============================================================================
bool BXStackTrace::capture(int skipLevels)
{
   mNumLevels = 0;
   
   // We always skip THIS function in addition to any other levels the caller has asked us to skip.
   skipLevels++;

#ifdef BUILD_FINAL
   return false;
#else
   enum { cMaxTempLevels = 64 };
   BCOMPILETIMEASSERT(cMaxTempLevels >= cMaxLevels);
   PVOID backTrace[cMaxTempLevels];
   HRESULT hres = DmCaptureStackBackTrace(cMaxTempLevels, backTrace);
   if (FAILED(hres))
      return false;

   int actualNumLevels;
   for (actualNumLevels = 0; actualNumLevels < cMaxLevels; actualNumLevels++)
      if (!backTrace[actualNumLevels])
         break;

   // If there are no levels left after we skip the requested levels, we're done.
   if (actualNumLevels <= skipLevels)
      return false;
      
   // Ignore the first entry because it's the capture() method.
   mNumLevels = Math::Min<int>(actualNumLevels - skipLevels, cMaxLevels);

   memcpy(mBackTrace, backTrace + skipLevels, sizeof(DWORD) * mNumLevels);

   return true;
#endif   
}

//==============================================================================
// BXStackTrace::capture
//==============================================================================
bool BXStackTrace::capture(_EXCEPTION_POINTERS* pExcept)
{
   return capture((DWORD*)pExcept->ContextRecord->Iar, (DWORD*)pExcept->ContextRecord->Gpr1);
}

//==============================================================================
// BXStackTrace::capture
//==============================================================================
bool BXStackTrace::capture(DWORD* pInstPtr, DWORD* pStackPtr)
{
   PVOID* pBacktrace = (PVOID*)mBackTrace;
   
   uint backTraceNum = 1;
   pBacktrace[0] = (PVOID)pInstPtr;

   __try
   {
      if (((ULONG)pStackPtr >= MM_VIRTUAL_4KB_BASE) && ((ULONG)pStackPtr <= MM_VIRTUAL_64KB_END))
      {
         DWORD* pStackTop = (DWORD*)*pStackPtr;
         if ((pStackTop) && (pStackTop > pStackPtr))
         {
            for ( ; ; )
            {
               DWORD* pPrevLR = (DWORD*)pStackTop[0];
               if (pPrevLR <= pStackTop)
                  break;
               DWORD* pBackChain = (DWORD*)pStackTop[-2];
               if (((ULONG)pBackChain < MM_IMAGE_64KB_BASE) || ((ULONG)pBackChain > MM_IMAGE_4KB_END))
                  break;

               if (backTraceNum == cMaxLevels)
                  break;

               pBacktrace[backTraceNum++] = pBackChain;

               pStackTop = pPrevLR;
               if (!pStackTop)
                  break;
               
               if (((ULONG)pStackTop < MM_VIRTUAL_4KB_BASE) && ((ULONG)pStackTop > MM_VIRTUAL_64KB_END))
                  break;                  
            }
         }
      }         
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
   }

   if (backTraceNum < cMaxLevels)
      pBacktrace[backTraceNum] = NULL;

   mNumLevels = backTraceNum;
   
   return true;      
}

//==============================================================================
// BXStackTrace::operator==
//==============================================================================
bool BXStackTrace::operator== (const BXStackTrace& rhs) const
{
   if (mNumLevels != rhs.mNumLevels)
      return false;
   
   return std::equal(mBackTrace, mBackTrace + mNumLevels, rhs.mBackTrace);
}

//==============================================================================
// BXStackTrace::operator<
//==============================================================================
bool BXStackTrace::operator< (const BXStackTrace& rhs) const
{
   return std::lexicographical_compare(mBackTrace, mBackTrace + mNumLevels, rhs.mBackTrace, rhs.mBackTrace + rhs.mNumLevels);
}
#else
uint xStackTraceDummy;
#endif // XBOX

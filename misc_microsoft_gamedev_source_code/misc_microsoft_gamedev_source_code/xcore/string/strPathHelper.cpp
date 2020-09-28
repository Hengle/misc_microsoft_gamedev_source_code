//============================================================================
//
//  StrPathHelper.cpp
//
//  Copyright (c) 2002, Ensemble Studios
//
//============================================================================
#include "xcore.h"

#ifndef XBOX
   #include <shlwapi.h>
#endif   

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
bool strPathIsAbsolute(const BCHAR_T *pFilename)
{
   const uint len = bcslen(pFilename);
   
   // A valid absolute math must be at least 3 characters.
   if (len > 2)
   {
      if (strFindLeft(pFilename, len, ":\\", 2) != -1)
         return true;
         
      if (strFindLeft(pFilename, len, ":/", 2) != -1)
         return true;
         
      if (!bcsncmp(pFilename, B("\\\\"), 2))
         return true;
   }         

   return false;
}

//----------------------------------------------------------------------------
// Adapted from: http://www.codeproject.com/string/wildcmp.asp
//---------------------------------------------------------------------------
bool wildcmp(const BCHAR_T* wild, const BCHAR_T* string) 
{
   const BCHAR_T *cp = NULL, *mp = NULL;

   while ((*string) && (*wild != B('*'))) 
   {
      if ((*wild != *string) && (*wild != B('?')))
      {
         return(false);
      }
      wild++;
      string++;
   }

   while (*string) 
   {
      if (*wild == B('*')) 
      {
         if (!*++wild) 
         {
            return(true);
         }
         mp = wild;
         cp = string+1;
      } 
      else if ((*wild == *string) || (*wild == B('?'))) 
      {
         wild++;
         string++;
      } 
      else
      {
         wild = mp;
         string = cp++;
      }
   }

   while (*wild == '*')
   {
      wild++;
   }
   return !*wild;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD strPathStandardizePath(const BCHAR_T *src, BCHAR_T *dst, DWORD dstLenWithoutNull)
{
   bool lastWasSlash = false;
   DWORD count = 0;
#ifdef DEBUG   
   BCHAR_T* origDst = dst;
#endif   
   while(*src && count<dstLenWithoutNull)
   {
      // Fixup forward vs. backslash
      if(*src == '/' || *src == B('\\'))
      {
         // Unless preceding character was a slash too, write a backslash
         // rg [3/20/07] - Changing this to work with UNC paths.
         if ((!lastWasSlash) || (1U == count))
         {
            *dst = B('\\');
            dst++;
            count++;
         }
         lastWasSlash = true;
      }
      else
      {
         lastWasSlash = false;

         // lower case.         
         *dst = toblower(*src);
         dst++;
         count++;
      }

      // next src character
      src++;
   }

   // Terminate
   *dst = B('\0');

#ifdef DEBUG
   BASSERT(bcslen(origDst) == count);
#endif   

   // Give back length.
   return(count);
}

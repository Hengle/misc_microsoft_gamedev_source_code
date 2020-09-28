//============================================================================
//
// File: math.cpp
// Copyright (c) 2005-2006, Ensemble Studios
//
//============================================================================

#include "xcore.h"

namespace Math
{
   bool isNumeric(const char* pStr, bool allowExponential, uint startOfs, int* pFailOfs, uint* pIntegralDigits, uint* pFractionalDigits, uint* pSignificantDigits)
   {
//     [whitespace] [sign] [digits] [.digits] [ {d | D | e | E}[sign]digits]
//     0            1      2         34          5              6  
      
      if (pFailOfs) *pFailOfs = 0;
      if (pIntegralDigits) *pIntegralDigits = 0;
      if (pFractionalDigits) *pFractionalDigits = 0;
      if (pSignificantDigits) *pSignificantDigits = 0;
               
      uint curPos = startOfs;
      uint curPhase = 0;
      bool foundDigits = false;
      bool foundFirstNonZeroDigit = false;
      
      for ( ; ; )
      {
         char c = pStr[curPos];
         if (!c)
            break;
            
         if (isdigit(c))
            foundDigits = true;
            
         for ( ; ; )
         {
            bool nextChar = false;
            
            switch (curPhase)
            {
               case 0:
               {
                  if ((c == ' ') || (c == '\t'))
                     nextChar = true;
                  else
                     curPhase = 1;
                     
                  break;
               }
               case 1:
               {
                  curPhase = 2;
                  
                  if ((c == '-') || (c == '+'))
                     nextChar = true;
                     
                  break;
               }
               case 2:
               {
                  if (isdigit(c))
                  {
                     if (pIntegralDigits)
                        *pIntegralDigits = *pIntegralDigits + 1;
                     if (c != '0')
                        foundFirstNonZeroDigit = true;
                     if ((pSignificantDigits) && (foundFirstNonZeroDigit))
                        *pSignificantDigits = *pSignificantDigits + 1;
                        
                     nextChar = true;
                  }
                  else  
                     curPhase = 3;
                     
                  break;
               }
               case 3:
               {
                  if (c == '.')
                  {
                     nextChar = true;
                     curPhase = 4;
                  }
                  else if ((tolower(c) == 'e') || (tolower(c) == 'd'))
                  {
                     if (!allowExponential)
                     {
                        if (pFailOfs)
                           *pFailOfs = curPos;
                        return false;
                     }
                     nextChar = true;
                     curPhase = 5;  
                  }
                  else
                  {
                     if (pFailOfs)
                        *pFailOfs = curPos;
                     return false;
                  }
                                            
                  break;
               }
               case 4:
               {
                  if (isdigit(c))
                  {
                     nextChar = true;
                     if (pFractionalDigits)
                        *pFractionalDigits = *pFractionalDigits + 1;
                     if (c != '0')
                        foundFirstNonZeroDigit = true;
                     if ((pSignificantDigits) && (foundFirstNonZeroDigit))
                        *pSignificantDigits = *pSignificantDigits + 1;                        
                  }
                  else if ((tolower(c) == 'e') || (tolower(c) == 'd'))
                  {
                     if (!allowExponential)
                     {
                        if (pFailOfs)
                           *pFailOfs = curPos;
                     }
                     nextChar = true;
                     curPhase = 5;  
                  }
                  else if ((c == ' ') || (c == '\t'))
                  {
                     nextChar = true;
                     curPhase = 7;
                  }
                  else
                  {
                     if (pFailOfs)
                        *pFailOfs = curPos;
                     return false;
                  }
                     
                  break;
               }
               case 5:
               {
                  curPhase = 6;
                  if ((c == '-') || (c == '+'))
                     nextChar = true;
                     
                  break;
               }
               case 6:
               {
                  if (isdigit(c))
                     nextChar = true;
                  else if ((c == ' ') || (c == '\t'))
                  {
                     nextChar = true;
                     curPhase = 7;
                  }
                  else
                  {
                     if (pFailOfs)
                        *pFailOfs = curPos;
                     return false;
                  }
                     
                  break;
               }
               case 7:
               {
                  if ((c == ' ') || (c == '\t'))
                     nextChar = true;
                  else
                  {
                     if (pFailOfs)
                        *pFailOfs = curPos;
                     return false;
                  }
                     
                  break;
               }
            }
            
            if (nextChar)
               break;
         }            
                  
         curPos++;
      };
      
      return foundDigits;
   }
   
   bool isZero(const char* pStr, bool allowExponential, bool* pIsNegative)
   {
      if (pIsNegative)
         *pIsNegative = false;
         
      uint curPos = 0;
      uint curPhase = 0;
      bool foundDigits = false;
      
      for ( ; ; )
      {
         char c = pStr[curPos];
         if (!c)
            break;
            
         if (isdigit(c))
         {
            if (c != '0')
               return false;
               
            foundDigits = true;
         }
            
         for ( ; ; )
         {
            bool nextChar = false;
            
            switch (curPhase)
            {
               case 0:
               {
                  if ((c == ' ') || (c == '\t'))
                     nextChar = true;
                  else
                     curPhase = 1;
                     
                  break;
               }
               case 1:
               {
                  curPhase = 2;
                  
                  if ((c == '-') && (pIsNegative))
                     *pIsNegative = true;
                                    
                  if ((c == '-') || (c == '+'))
                     nextChar = true;
                     
                  break;
               }
               case 2:
               {
                  if (isdigit(c))
                  {
                     if (c != '0')
                        return false;
                        
                     nextChar = true;
                  }
                  else  
                     curPhase = 3;
                     
                  break;
               }
               case 3:
               {
                  if (c == '.')
                  {
                     nextChar = true;
                     curPhase = 4;
                  }
                  else if ((tolower(c) == 'e') || (tolower(c) == 'd'))
                  {
                     if (!allowExponential)
                     {
                        return false;
                     }
                     nextChar = true;
                     curPhase = 5;  
                  }
                  else
                  {
                     return false;
                  }
                                            
                  break;
               }
               case 4:
               {
                  if (isdigit(c))
                  {
                     if (c != '0')
                        return false;
                        
                     nextChar = true;
                  }
                  else if ((tolower(c) == 'e') || (tolower(c) == 'd'))
                  {
                     if (!allowExponential)
                     {
                        return false;
                     }
                     nextChar = true;
                     curPhase = 5;  
                  }
                  else if ((c == ' ') || (c == '\t'))
                  {
                     nextChar = true;
                     curPhase = 7;
                  }
                  else
                  {
                     return false;
                  }
                     
                  break;
               }
               case 5:
               {
                  curPhase = 6;
                  if ((c == '-') || (c == '+'))
                     nextChar = true;
                     
                  break;
               }
               case 6:
               {
                  if (isdigit(c))
                     nextChar = true;
                  else if ((c == ' ') || (c == '\t'))
                  {
                     nextChar = true;
                     curPhase = 7;
                  }
                  else
                  {
                     return false;
                  }
                     
                  break;
               }
               case 7:
               {
                  if ((c == ' ') || (c == '\t'))
                     nextChar = true;
                  else
                  {
                     return false;
                  }
                     
                  break;
               }
            }
            
            if (nextChar)
               break;
         }            
                  
         curPos++;
      };
      
      return foundDigits;
   }
   
   uint64 FloatToUInt64TruncIntALU(const float* pFloat)
   {
      const uint32 floatBits = *reinterpret_cast<const uint32*>(pFloat);
      if (!floatBits)
         return 0;
      
      uint64 mantissa = (floatBits & 0x7FFFFF) | 0x800000;
      int exponent = ((floatBits >> 23) & 0xFF) - 127;
      
      int shift = exponent - 23;
      if (shift < 0)
         mantissa >>= (-shift);
      else
         mantissa <<= shift;
         
      return mantissa;
   }
}
 
 
 
 
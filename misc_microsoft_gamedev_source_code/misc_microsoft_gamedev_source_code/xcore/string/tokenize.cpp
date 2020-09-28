//============================================================================
//
//  tokenize.cpp
//
//  Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "tokenize.h"

bool tokenizeString(const char* pStr, BDynamicArray<BString>& tokens)
{
   bool inToken = false;
   bool wasQuoted = false;

   BString curToken;
   
   const uint strLen = strlen(pStr);

   for (uint i = 0; i < strLen; i++)
   {
      const char c = pStr[i];

      if (!inToken)
      {
         if ((c != ' ') && (c != '\t'))
         {
            if (c == '"')
               wasQuoted = true;
            else
            {
               wasQuoted = false;
               curToken.append(c);
            }

            inToken = true;
         }            
      }
      else
      {
         if ( 
            ((!wasQuoted) && ((c == ' ') || (c == '\t'))) || 
            ((wasQuoted) && (c == '"'))
            )
         {
            tokens.grow().swap(curToken);
            inToken = false;
            wasQuoted = false;
         }
         else
            curToken.append(c);
      }
   }

   if (inToken)
   {
      if (wasQuoted)  
         return false;
      tokens.grow().swap(curToken);
   }

   return true;
}   
//============================================================================
//
//  StrHelper.inl
//
//  Copyright (c) 2005, Ensemble Studios
//
//============================================================================

template<class T>
inline void escapeXMLString(T& str)
{
   const uint strLen = str.length();

   BDynamicArray<char> buf;
   buf.reserve(strLen + 128);

   uint ofs = 0;
   for ( ; ; )
   {
      const char c = (ofs < strLen) ? str.getChar(ofs) : 0;
      ofs++;

      switch (c)
      {
      case '<':
         {
            buf.pushBack("&lt;", 4);
            break;
         }
      case '>':
         {
            buf.pushBack("&gt;", 4);
            break;
         }
      case '\'':
         {  
            buf.pushBack("&apos;", 6);
            break;
         }
      case '"':
         {
            buf.pushBack("&quot;", 6);
            break;
         }
      case '&':
         {
            buf.pushBack("&amp;", 5);
            break;
         }
      case '\r':
         {
            buf.pushBack("&#x0D;", 6);
            break;
         }
      case '\n':
         {
            buf.pushBack("&#x0A;", 6);
            break;
         }
      default:
         {
            buf.pushBack(c);
            break;
         }  
      }

      if (!c)
         break;
   }

   str.set(buf.getPtr());
}

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
long strCopy(CharType* pDst, DWORD dstLen, const CharType* pSrc, long srcLen, long srcCount, long srcPos)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pDst || !pSrc || !dstLen || !srcLen)
   {
      BFAIL("ERROR: strCopy - Invalid string parameters.");
      return(-1);
   }

   // srcLen has to be specified if srcCount or srcPos are
   if ((srcCount > 0 || srcPos > 0) && srcLen < 0)
   {
      BFAIL("ERROR: strCopy - Invalid string parameters.");
      return(-1);
   }

   const CharType *pBuffer = pSrc;
   long srcLength = -1;

   // if srcCount wasn't specified, length is the entire string
   if (srcCount < 0)
   {
      srcLength = strLength(pSrc);
      if (srcLen >= 0 && srcLength > srcLen)
      {
         BFAIL("ERROR: strCopy - Invalid string parameters.");
         return(-1);
      }
      srcCount = min(srcLength, (srcLength-srcPos));
   }

   // if it is a substring, do some verification
   if (srcPos > 0)
   {
      // make sure length is set at least once
      if (srcLength < 0)
      {
         srcLength = strLength(pSrc);
         if (srcLen >= 0 && srcLength > srcLen)
         {
            BFAIL("ERROR: strCopy - Invalid string parameters.");
            return(-1);
         }
      }

      if (srcPos >= srcLength || (srcPos+srcCount) > srcLength)
      {
         BFAIL("ERROR: strCopy - Invalid string operation.");
         return(-1);
      }

      pBuffer = &pSrc[srcPos];
   }

   long count = min((long)dstLen - 1, srcCount);
   CopyMemory(pDst, pBuffer, count * sizeof(CharType));

   pDst[count] = 0;

   return(count);
}

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
long strInsert(CharType* pDst, DWORD dstLen, DWORD dstPos, const CharType* pSrc, long srcLen, long srcCount, long srcPos)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   // jce [1/10/2003] -- disabled assert when the src size is 0... appending an empty string should work silently I think.
   if (!pDst || !pSrc || !dstLen /*|| !srcLen*/)
   {
      BFAIL("ERROR: strInsert - Invalid string parameters.");
      return(-1);
   }

   // srcLen has to be specified if srcCount or srcPos are
   if ((srcCount > 0 || srcPos > 0) && srcLen < 0)
   {
      BFAIL("ERROR: strCopy - Invalid string parameters.");
      return(-1);
   }

   long srcLength = strLength(pSrc);
   DWORD dstLength = strLength(pDst);
   if (dstLength > dstLen || (srcLen >= 0 && srcLength > srcLen))
   {
      BFAIL("strInsert - Invalid string parameters.");
      return(-1);
   }

   long availSpace = dstLen - dstLength - 1;

   if (dstPos > dstLength)
   {
      BFAIL("ERROR: strInsert - Invalid string parameters.");
      return(-1);
   }

   const CharType* pSrcBuffer = pSrc;
   CharType* pDstBuffer = &pDst[dstPos];

   // if no count specified, copy the max possible
   if (srcCount < 0)
      srcCount = min(srcLength, (srcLength-srcPos));

   // if there was a starting position specified
   if (srcPos > 0)
   {
      // validate the parameters
      if (srcPos >= srcLength || (srcPos+srcCount) > srcLength)
      {
         BFAIL("ERROR: strInsert - Invalid string operation.");
         return(-1);
      }
      // adjust required space to skip characters at the front
      srcLength -= srcPos;
      pSrcBuffer = &pSrc[srcPos];
   }

   // srcCount should have been set above if it wasn't specified
   srcLength = min(srcCount, srcLength);

   // if we don't have enough space, we're screwed
   if (srcLength > availSpace)
   {
      BFAIL("ERROR: strInsert - Not enough free space for string operation.");
      return(-1);
   }

   if ((dstLength-dstPos) > 0)
      MoveMemory(pDstBuffer + srcLength, pDstBuffer, (dstLength-dstPos) * sizeof(CharType));

   CopyMemory(pDstBuffer, pSrcBuffer, srcLength * sizeof(CharType));

   pDst[srcLength+dstLength] = 0;

   return(srcLength+dstLength);
}

//----------------------------------------------------------------------------
// strInsertCh
//    Parameters:
//       pDst        - The destination buffer to insert into. Non-NULL
//       dstLen      - The length of the destination buffer. Non-Zero (chars)
//       dstPos      - The starting position in pDst. Non-Negative
//       srcChr      - The source character to insert.
//    Return:
//       -1 on Error, otherwise new pDst length in characters.
//----------------------------------------------------------------------------
template<typename CharType>
long strInsertCh(CharType* pDst, DWORD dstLen, DWORD dstPos, CharType srcChr)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pDst || !dstLen)
   {
      BFAIL("ERROR: strInsertCh - Invalid string parameters.");
      return(-1);
   }

   DWORD dstLength = strLength(pDst);
   if (dstLength > dstLen)
   {
      BFAIL("ERROR: strInsertCh - Invalid string parameters.");
      return(-1);
   }

   DWORD availSpace = dstLen - dstLength - 1;

   // inserting into an invalid location
   if (dstPos > dstLength)
   {
      BFAIL("ERROR: strInsertCh - Invalid string parameters.");
      return(-1);
   }

   CharType* pDstBuffer = &pDst[dstPos];

   // if we don't have enough space, we're screwed
   if (availSpace == 0)
   {
      BFAIL("ERROR: strInsertCh - Not enough free space for string operation.");
      return(-1);
   }

   if ((dstLength-dstPos) > 0)
      MoveMemory(pDstBuffer + 1, pDstBuffer, (dstLength-dstPos) * sizeof(CharType));

   pDst[dstPos] = srcChr;
   pDst[dstLength + 1] = 0;

   return(dstLength + 1);
}

//----------------------------------------------------------------------------
// strRemove - removes a substring
//    Parameters:
//       pDst        - The destination buffer to remove from. Non-NULL
//       dstLen      - The length of the destination buffer. Non-Zero (chars)
//       dstPos      - The starting position in pDst. Non-Negative
//       pSrc        - The source buffer to remove.
//       srcLen      - The length of the source buffer.Must be Non-Neg if srcCount/srcPos are specified.
//       srcCount    - The amount of pSrc to remove. default everything
//       srcPos      - The starting position in pSrc. default start at pos 0
//    Return:
//       -1 on Error, otherwise new pDst length in characters.
//----------------------------------------------------------------------------
template<typename CharType>
long strRemove(CharType* pDst, DWORD dstLen, DWORD dstPos, const CharType* pSrc, long srcLen, long srcCount, long srcPos)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pDst || !dstLen || !pSrc || !srcLen)
   {
      BFAIL("ERROR: strRemove - Invalid string parameters.");
      return(-1);
   }

   // srcLen has to be specified if srcCount or srcPos are
   if ((srcCount > 0 || srcPos > 0) && srcLen < 0)
   {
      BFAIL("ERROR: strRemove - Invalid string parameters.");
      return(-1);
   }

   long srcLength = strLength(pSrc);
   DWORD dstLength = strLength(pDst);
   if (dstLength > dstLen || (srcLen >= 0 && srcLength > srcLen))
   {
      BFAIL("ERROR: strRemove - Invalid string parameters.");
      return(-1);
   }

   if (dstPos > dstLength)
   {
      BFAIL("ERROR: strRemove - Invalid string parameters.");
      return(-1);
   }

   CharType* pDstBuffer = &pDst[dstPos];
   const CharType* pSrcBuffer = pSrc;

   // if no count specified, copy the max possible
   if (srcCount < 0)
      srcCount = min(srcLength, (srcLength-srcPos));

   // if there was a starting position specified
   if (srcPos > 0)
   {
      // validate the parameters
      if (srcPos >= srcLength || (srcPos+srcCount) > srcLength)
      {
         BFAIL("ERROR: strRemove - Invalid string operation.");
         return(-1);
      }
      // adjust required space to skip characters at the front
      srcLength -= srcPos;
      pSrcBuffer = &pSrc[srcPos];
   }

   // srcCount should have been set above if it wasn't specified
   srcLength = min(srcCount, srcLength);

   // cannot remove more data than exist in the destination
   if ((DWORD)srcLength > dstLength)
   {
      return(dstLength);
   }

   CharType* pPos = pDstBuffer;
   long index, count = (&pDst[dstLength] - pDstBuffer);

   // while we haven't hit the end of the dst buffer, and there is still room for a src string match
   while (*pPos != 0 && count >= srcLength)
   {
      index = 1;

      // check for potential match if first characters match
      if (pPos[0] == pSrcBuffer[0])
      {
         // test rest of buffer (dstBuffer was checked above to be larger than srcLength)
         while (pSrcBuffer[index] == pPos[index] && index < srcLength)
            index++;

         // complete match was found
         if (index == srcLength)
         {
            // remove the src string from the buffer
            MoveMemory(pPos, pPos + srcLength, sizeof(CharType)*(count - srcLength + 1));

            // adjust the remaning dstBuffer to check
            count -= srcLength;

            // keep track of our new dstBuffer length
            dstLength -= srcLength;

            continue;
         }
         // the remaining string cannot possibly match
         else if ((count-index) < srcLength)
            break;
      }

      // jump ahead to the next index
      pPos+=index;
      count-=index;
   }

   return(dstLength);
}

//----------------------------------------------------------------------------
// strRemove - removes a substring
//    Parameters:
//       pDst        - The destination buffer to remove from. Non-NULL
//       dstLen      - The length of the destination buffer. Non-Zero (chars)
//       dstPos      - The starting position in pDst. Non-Negative
//       count       - The number of characters to remove. default everything
//    Return:
//       -1 on Error, otherwise new pDst length in characters.
//----------------------------------------------------------------------------
template<typename CharType>
long strRemove(CharType* pDst, DWORD dstLen, DWORD dstPos, long count)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pDst || !dstLen || !count)
   {
      BFAIL("ERROR: strRemove - Invalid string parameters.");
      return(-1);
   }

   DWORD dstLength = strLength(pDst);
   if (dstLength > dstLen)
   {
      BFAIL("ERROR: strRemove - Invalid string parameters.");
      return(-1);
   }

   if (dstPos > dstLength)
   {
      BFAIL("ERROR: strRemove - Invalid string parameters.");
      return(-1);
   }

   // if we are trimming the end it is easy
   if (count < 0 || (dstPos+count >= dstLength))
   {
      pDst[dstPos] = 0;
      return(dstPos);
   }

   CharType* pDstBuffer = &pDst[dstPos];
   long diff = dstLength - (dstPos + count);
   MoveMemory(pDstBuffer, &pDst[dstPos + count], sizeof(CharType)*diff);
   pDst[dstPos + diff] = 0;

   return(dstPos + diff);
}

//----------------------------------------------------------------------------
// strRemoveCh - remove all instances of a character from starting pos to end
//    Parameters:
//       pDst        - The destination buffer to insert into. Non-NULL
//       dstLen      - The length of the destination buffer. Non-Zero (chars)
//       dstPos      - The starting position in pDst. Non-Negative
//       srcChr      - The number of characters to remove. Non-Zero (# chars)
//    Return:
//       -1 on Error, otherwise new pDst length in characters.
//----------------------------------------------------------------------------
template<typename CharType>
long strRemoveCh(CharType* pDst, DWORD dstLen, DWORD dstPos, CharType srcChr)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pDst || !dstLen || dstPos<0)
   {
      BFAIL("ERROR: strRemoveCh - Invalid string parameters.");
      return(-1);
   }

   DWORD dstLength = strLength(pDst);
   if (dstLength > dstLen || dstPos > dstLength)
   {
      BFAIL("ERROR: strRemoveCh - Invalid string parameters.");
      return(-1);
   }

   CharType* pDstBuffer = &pDst[dstPos];
   CharType* pPos = pDstBuffer, *pEnd = &pDst[dstLength];

   while (*pPos != 0)
   {
      if (*pPos == srcChr)
      {
         MoveMemory(pPos, pPos+1, sizeof(CharType)*(pEnd - pPos));
         dstLength--;
      }
      pPos++;
   }

   return(dstLength);
}

//----------------------------------------------------------------------------
// strCrop - crop the string to a substring of itself
//    Parameters:
//       pDst        - The destination buffer to insert into. Non-NULL
//       dstLen      - The length of the destination buffer. Non-Zero (chars)
//       firstPos    - The starting position in pDst. Non-Negative (<= lastPos)
//       lastPos     - The ending position in pDst. Non-Negative (>= firstPos)
//    Return:
//       -1 on Error, otherwise new pDst length in characters.
//----------------------------------------------------------------------------
template<typename CharType>
long strCrop(CharType* pDst, DWORD dstLen, long firstPos, long lastPos)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pDst || !dstLen || firstPos<0 || lastPos<0 || lastPos < firstPos)
   {
      BFAIL("ERROR: strCrop - Invalid string parameters.");
      return(-1);
   }

   long dstLength = strLength(pDst);
   if (lastPos >= dstLength)
      lastPos = dstLength-1;

   if ((DWORD)dstLength > dstLen || firstPos > dstLength)
   {
      BFAIL("ERROR: strCopr - Invalid string parameters.");
      return(-1);
   }

   CharType* pDstBuffer = &pDst[firstPos];
   CharType* pEnd = &pDst[lastPos];
   long length = pEnd - pDstBuffer + 1;

   MoveMemory(pDst, pDstBuffer, sizeof(CharType)*length);
   pDst[length] = 0;

   return(length);
}

//----------------------------------------------------------------------------
// strFormat
//    Parameters:
//       pDst        - The destination buffer to insert into. Non-NULL
//       dstLen      - The length of the destination buffer. Non-Zero (chars)
//       pFormat     - The source string with formatting
//       ...         - The formatting parameters
//    Return:
//       false on Error, otherwise true.
//----------------------------------------------------------------------------
template<typename CharType>
bool strFormat(CharType* pDst, DWORD dstLen, const CharType* pFormat, ...)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pDst || !dstLen || !pFormat)
   {
      BFAIL("ERROR: strFormat - Invalid string parameters.");
      return false;
   }

   va_list args;
   va_start(args, pFormat);     
   bool failed = false;

   HRESULT hr;
   
   if (sizeof(CharType) == sizeof(char))
      hr = StringCchVPrintfA((char*)pDst, dstLen, (const char*)pFormat, args);
   else
      hr = StringCchVPrintfW((WCHAR*)pDst, dstLen, (const WCHAR*)pFormat, args);

   va_end(args);

   if (!SUCCEEDED(hr))
      failed = true;
   
   if (failed)
   {
      BFAIL("ERROR: strFormat - insufficient buffer space or invalid argument.");
      return false;
   }

   return true;
}

//----------------------------------------------------------------------------
// strFormat
//    Parameters:
//       pDst        - The destination buffer to insert into. Non-NULL
//       dstLen      - The length of the destination buffer. Non-Zero (chars)
//       pFormat     - The source string with formatting
//       args        - The formatting parameters
//    Return:
//       false on Error, otherwise true.
//----------------------------------------------------------------------------
template<typename CharType>
bool strFormat(CharType* pDst, DWORD dstLen, const CharType* pFormat, va_list args)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pDst || !dstLen || !pFormat || !args)
   {
      BFAIL("ERROR: strFormat - Invalid string parameters.");
      return false;
   }

   bool failed = false;

   HRESULT hr; 
   if (sizeof(CharType) == sizeof(char))
      hr = StringCchVPrintfA((char*)pDst, dstLen, (const char*)pFormat, args);
   else
      hr = StringCchVPrintfW((WCHAR*)pDst, dstLen, (const WCHAR*)pFormat, args);
      
   if (!SUCCEEDED(hr))
      failed = true;

   if (failed)
   {
      BFAIL("ERROR: strFormat - insufficient buffer space or invalid argument.");
      return false;
   }
   return true;
}

//----------------------------------------------------------------------------
// strTrimLeft - remove all characters in token list from left of string
//    Parameters:
//       pDst        - The destination buffer to insert into. Non-NULL
//       dstLen      - The length of the destination buffer. Non-Zero (chars)
//       pTokens     - The token list
//    Return:
//       -1 on Error, otherwise new pDst length in characters.
//----------------------------------------------------------------------------
template<typename CharType>
long strTrimLeft(CharType* pDst, DWORD dstLen, const CharType* pTokens)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pDst || !dstLen || !pTokens)
   {
      BFAIL("ERROR: strTrimLeft - Invalid string parameters.");
      return(-1);
   }

   DWORD dstLength = strLength(pDst);
   if (dstLength > dstLen)
   {
      BFAIL("ERROR: strTrimLeft - Invalid string parameters.");
      return(-1);
   }

   DWORD tokenLength = strLength(pTokens);
   if (tokenLength == 0)
   {
      BFAIL("ERROR: strTrimLeft - Invalid string parameters.");
      return(-1);
   }

   CharType *pPos = pDst;
   //CharType *pEnd = &pDst[dstLength];

   DWORD idx;
   for (idx=0; idx<dstLength; idx++)
   {
      CharType* p;
      if (sizeof(CharType) == sizeof(char))
         p = (CharType*)strchr((char*)pTokens, (char)pPos[idx]);
      else
         p = (CharType*)wcschr((WCHAR*)pTokens, (WCHAR)pPos[idx]);
      
      if (p == NULL)
      {
         MoveMemory(pDst, pPos+idx, sizeof(CharType)*(dstLength-idx));
         dstLength -= idx;
         pDst[dstLength] = 0;
         return(dstLength);
      }
   }

   if (idx >= dstLength)
   {
      pDst[0] = 0;
      dstLength = 0;
   }
   return(dstLength);
}

//----------------------------------------------------------------------------
// strTrimRight - remove all characters in token list from right of string
//    Parameters:
//       pDst        - The destination buffer to insert into. Non-NULL
//       dstLen      - The length of the destination buffer. Non-Zero (chars)
//       pTokens     - The token list
//    Return:
//       -1 on Error, otherwise new pDst length in characters.
//----------------------------------------------------------------------------
template<typename CharType>
long strTrimRight(CharType* pDst, DWORD dstLen, const CharType* pTokens)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pDst || !dstLen || !pTokens)
   {
      BFAIL("ERROR: strTrimRight - Invalid string parameters.");
      return(-1);
   }

   DWORD dstLength = strLength(pDst);
   if (dstLength > dstLen)
   {
      BFAIL("ERROR: strTrimRight - Invalid string parameters.");
      return(-1);
   }

   DWORD tokenLength = strLength(pTokens);
   if (tokenLength == 0)
   {
      BFAIL("ERROR: strTrimRight - Invalid string parameters.");
      return(-1);
   }

   CharType *pPos = pDst;
   //CharType *pEnd = &pDst[dstLength];

   long idx;
   for (idx=dstLength-1; idx>=0; idx--)
   {
      CharType* p;
      if (sizeof(CharType) == sizeof(char))
         p = (CharType*)strchr((char*)pTokens, (char)pPos[idx]);
      else
         p = (CharType*)wcschr((WCHAR*)pTokens, (WCHAR)pPos[idx]);
         
      if (p == NULL)
      {
         dstLength = idx+1;
         pDst[dstLength] = 0;
         return(dstLength);
      }
   }

   if (idx < 0)
   {
      pDst[0] = 0;
      dstLength = 0;
   }
   return(dstLength);
}

//----------------------------------------------------------------------------
// strToLower
//    Parameters:
//       pDst        - The destination buffer to insert into. Non-NULL
//       dstLen      - The length of the destination buffer. Non-Zero (chars)
//    Return:
//----------------------------------------------------------------------------
template<typename CharType>
void strToLower(CharType* pDst, DWORD dstLen)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pDst || !dstLen)
   {
      BFAIL("ERROR: strToLower - Invalid string parameters.");
      return;
   }

   DWORD dstLength = strLength(pDst);
   if (dstLength > dstLen)
   {
      BFAIL("ERROR: strToLower - Invalid string parameters.");
      return;
   }

   for (DWORD idx=0; idx<dstLength; idx++)
      pDst[idx] = strToLowerCh(pDst[idx]);
}

//----------------------------------------------------------------------------
// strToUpper
//    Parameters:
//       pDst        - The destination buffer to insert into. Non-NULL
//       dstLen     - The size of the destination buffer. Non-Zero (bytes)
//    Return:
//----------------------------------------------------------------------------
template<typename CharType>
void strToUpper(CharType* pDst, DWORD dstLen)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pDst || !dstLen)
   {
      BFAIL("ERROR: strToUpper - Invalid string parameters.");
      return;
   }

   DWORD dstLength = strLength(pDst);
   if (dstLength > dstLen)
   {
      BFAIL("ERROR: strToUpper - Invalid string parameters.");
      return;
   }
   
   for (DWORD idx=0; idx<dstLength; idx++)
      pDst[idx] = strToUpperCh(pDst[idx]);
}

//----------------------------------------------------------------------------
// strToLowerCh
//    Parameters:
//       chr        - The original character
//    Return:
//       The lower case version of the character.
//----------------------------------------------------------------------------
inline char strToLowerCh(char chr)
{
   return static_cast<char>(tolower(chr));
}

inline WCHAR strToLowerCh(WCHAR chr)
{
   return static_cast<WCHAR>(towlower(chr));
}

//----------------------------------------------------------------------------
// strToUpperCh
//    Parameters:
//       chr        - The original character
//    Return:
//       The upper case version of the character.
//----------------------------------------------------------------------------
inline char strToUpperCh(char chr)
{
   return static_cast<char>(toupper(chr));
}

inline WCHAR strToUpperCh(WCHAR chr)
{
   return static_cast<WCHAR>(toupper(chr));
}

//----------------------------------------------------------------------------
// strFindLeft
//    Parameters:
//       pDst        - The destination buffer to insert into. Non-NULL
//       dstLen      - The length of the destination buffer. Non-Zero (chars)
//       pSrc        - The source buffer to remove.
//       srcLen      - The length of the source buffer.Must be Non-Neg if srcCount/srcPos are specified.
//       startPos    - The starting position in pDst. default start at pos 0
//    Return:
//       The position that the source string was found, or -1 if not found.
//----------------------------------------------------------------------------
template<typename CharType>
long strFindLeft(const CharType* pDst, DWORD dstLen, const CharType* pSrc, DWORD srcLen, long startPos)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR))); 
   
   if (!pDst || !dstLen || !pSrc || !srcLen)
   {
      BFAIL("ERROR: strFindLeft - Invalid string parameters.");
      return(-1);
   }

   DWORD srcLength = strLength(pSrc);
   DWORD dstLength = strLength(pDst);
   if (dstLength > dstLen || srcLength > srcLen)
   {
      BFAIL("ERROR: strFindLeft - Invalid string parameters.");
      return(-1);
   }

   // simple stupid checks
   if (dstLength == 0 && srcLength == 0)
      return(0);
   else if (dstLength == 0 || srcLength == 0)
      return(-1);

   if (startPos > (long)dstLength)
   {
      BFAIL("ERROR: strFindLeft - Invalid string parameters.");
      return(-1);
   }

   if (startPos < 0)
      startPos = 0;

   // source cannot have more data than exist in the destination
   if (srcLength > (dstLength - (DWORD)startPos))
      return(-1);

   const CharType* pDstBuffer = &pDst[startPos];

   const CharType* pPos = pDstBuffer;
   DWORD index, count = (&pDst[dstLength] - pDstBuffer);

   // while we haven't hit the end of the dst buffer, and there is still room for a src string match
   while (*pPos != 0 && count >= srcLength)
   {
      index = 1;

      // check for potential match if first characters match
      if (pPos[0] == pSrc[0])
      {
         // test rest of buffer (dstBuffer was checked above to be larger than srcLength)
         while (pSrc[index] == pPos[index] && index < srcLength)
            index++;

         // complete match was found
         if (index == srcLength)
            return(pPos - pDst);
         // the remaining string cannot possibly match
         else if ((count-index) < srcLength)
            break;
      }

      // jump ahead to the next index
      pPos+=index;
      count-=index;
   }
   return(-1);
}

//----------------------------------------------------------------------------
// strFindRight
//    Parameters:
//       pDst        - The destination buffer to insert into. Non-NULL
//       dstLen      - The length of the destination buffer. Non-Zero (chars)
//       pSrc        - The source buffer to remove.
//       srcLen      - The length of the source buffer.
//       startPos    - The starting position in pDst. default start at pos 0
//    Return:
//       The position that the source string was found, or -1 if not found.
//----------------------------------------------------------------------------
template<typename CharType>
long strFindRight(const CharType* pDst, DWORD dstLen, const CharType* pSrc, DWORD srcLen, long startPos)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pDst || !dstLen || !pSrc || !srcLen)
   {
      BFAIL("ERROR: strFindLeft - Invalid string parameters.");
      return(-1);
   }

   DWORD srcLength = strLength(pSrc);
   DWORD dstLength = strLength(pDst);
   if (dstLength > dstLen || srcLength > srcLen)
   {
      BFAIL("ERROR: strFindLeft - Invalid string parameters.");
      return(-1);
   }

   // simple stupid checks
   if (dstLength == 0 && srcLength == 0)
      return(0);
   else if (dstLength == 0 || srcLength == 0)
      return(-1);

   // source cannot have more data than exist in the destination
   if (srcLength > dstLength)
      return(-1);

   if (startPos < 0)
      startPos = dstLength - 1;

   // 0------P----End   (search from P to 0)
   // impossible for src to exist if the search area is smaller than src length
   if ((srcLength - 1) > (DWORD)startPos)
      return(-1);

   DWORD srcStartPos = srcLength - 1;

   const CharType* pDstBuffer = &pDst[startPos];
   const CharType* pSrcBuffer = &pSrc[srcStartPos];
   const CharType* pPos = pDstBuffer;

   long index;
   DWORD count = (pDstBuffer - pDst + 1);

   // while we haven't hit the end of the dst buffer, and there is still room for a src string match
   while (pPos >= pDst && count >= srcLength)
   {
      index = 1;

      // check for potential match if last characters match
      if (pPos[0] == pSrcBuffer[0])
      {
         // test rest of buffer
         while (pSrcBuffer[-index] == pPos[-index] && (DWORD)index < srcLength)
            index++;

         // complete match was found
         if ((DWORD)index == srcLength)
            return(pPos - pDst - index + 1);
         // the remaining string cannot possibly match
         else if ((count-index) < srcLength)
            break;
      }

      // jump ahead to the next index
      pPos-=index;
      count-=index;
   }
   return(-1);
}

//----------------------------------------------------------------------------
// strFindLeftCh
//    Parameters:
//       pDst        - The destination buffer to insert into. Non-NULL
//       dstLen      - The length of the destination buffer. Non-Zero (chars)
//       srcChr      - The character to search for.
//       startPos    - The starting position in pDst. default start at pos 0
//    Return:
//       The position that the source character was found, or -1 if not found.
//----------------------------------------------------------------------------
template<typename CharType>
long strFindLeftCh(const CharType* pDst, DWORD dstLen, CharType srcChr, long startPos)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pDst || !dstLen)
   {
      BFAIL("ERROR: strFindLeftCh - Invalid string parameters.");
      return(-1);
   }

   DWORD dstLength = strLength(pDst);
   if (dstLength > dstLen)
   {
      BFAIL("ERROR: strFindLeftCh - Invalid string parameters.");
      return(-1);
   }

   if (dstLength == 0)
      return(-1);

   if (startPos > (long)dstLength)
   {
      BFAIL("ERROR: strFindLeftCh - Invalid string parameters.");
      return(-1);
   }

   if (startPos < 0)
      startPos = 0;

   const CharType* pPos;
   if (sizeof(CharType) == sizeof(char))
      pPos = (const CharType*)strchr((char*)&pDst[startPos], srcChr);
   else
      pPos = (const CharType*)wcschr((WCHAR*)&pDst[startPos], srcChr);
   return(pPos == 0?-1:pPos - pDst);
}

//----------------------------------------------------------------------------
// strFindRightCh
//    Parameters:
//       pDst        - The destination buffer to insert into. Non-NULL
//       dstLen      - The length of the destination buffer. Non-Zero (chars)
//       srcChr      - The character to search for.
//       startPos    - The starting position in pDst. default start at pos 0
//    Return:
//       The position that the source character was found, or -1 if not found.
//----------------------------------------------------------------------------
template<typename CharType>
long strFindRightCh(const CharType* pDst, DWORD dstLen, CharType srcChr, long startPos)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pDst || !dstLen)
   {
      BFAIL("ERROR: strFindRightCh - Invalid string parameters.");
      return(-1);
   }

   DWORD dstLength = strLength(pDst);
   if (dstLength> dstLen)
   {
      BFAIL("ERROR: strFindRightCh - Invalid string parameters.");
      return(-1);
   }

   if (dstLength == 0)
      return(-1);

   if (startPos > (long)dstLength)
   {
      BFAIL("ERROR: strFindRightCh - Invalid string parameters.");
      return(-1);
   }

   if (startPos < 0)
      startPos = 0;

   const CharType* pPos;
   if (sizeof(CharType) == sizeof(char))
      pPos = (const CharType*)strrchr((char*)&pDst[startPos], srcChr);
   else
      pPos = (const CharType*)wcsrchr((WCHAR*)&pDst[startPos], srcChr);
   return(pPos == 0?-1:pPos - pDst);
}

//----------------------------------------------------------------------------
// strFindAndReplace
//    Parameters:
//       pDst        - The destination buffer to insert into. Non-NULL
//       dstLen      - The length of the destination buffer. Non-Zero (chars)
//       pSrc        - The source buffer to find and replace.
//       pNew        - The new buffer to replace pSrc with.
//    Return:
//       -1 on Error, otherwise new pDst length in characters.
//----------------------------------------------------------------------------
template<typename CharType>
long strFindAndReplace(CharType* pDst, DWORD dstLen, const CharType* pSrc, const CharType* pNew)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pDst || !dstLen || !pSrc || !pNew)
   {
      BFAIL("ERROR: strFindAndReplace - Invalid string parameters.");
      return(-1);
   }

   //-- this seems somewhat insane, but I don't want to depend on utsimplearray yet
   long locations[256];
   long count = 0;

   DWORD dstLength = strLength(pDst);
   if (dstLength > dstLen)
   {
      BFAIL("ERROR: strFindAndReplace -- Invalid string parameter.");
      return(-1);
   }

   DWORD srcLength = strLength(pSrc);
   DWORD srcLen = srcLength + 1;
   DWORD newLength = strLength(pNew);

   // determine the replacement positions
   long pos = strFindLeft(pDst, dstLen, pSrc, srcLen, 0);
   while (pos >= 0 && count < 256)
   {
      locations[count++] = pos;
      pos = strFindLeft(pDst, dstLen, pSrc, srcLen, pos+1);
   }

   if (count == 256)
      BFAIL("ERROR: strFindAndReplace -- Internal buffer limitation reached.");

   if ( (dstLength+(count*(newLength-srcLength))) >= dstLen)
   {
      BFAIL("ERROR: strFindAndReplace -- Insufficient string space for operation.");
      return(-1);
   }

   if (srcLength != newLength)
   {
      for (long idx=count-1; idx>=0; idx--)
      {
         // This is basically what we are doing given:
         // 0_____________S_______NE____SE________NULL
         // where: 
         //    0  - start of string
         //    S  - starting location (locations[idx])
         //    NE - new string end location (locations[idx] + newLength)
         //    SE - source string end location (locations[idx] + srcLength)
         //    NULL - End of the string (dstLength)

         pos = locations[idx];

         // Move SE to NE
         // 0_____________S_______NE________NULL
         MoveMemory(pDst + pos + newLength, pDst + pos + srcLength, sizeof(CharType)*(dstLength - (pos + srcLength) + 1));

         // Decrement string length by difference of SE and NE
         dstLength -= (srcLength - newLength);

         // Copy the new source string into position (S_____NE)
         CopyMemory(pDst + pos, pNew, sizeof(CharType)*newLength);         
      }
   }
   else // same size
   {
      for (long idx=count-1; idx>=0; idx--)
      {
         // Copy the new source string into position
         CopyMemory(pDst + locations[idx], pNew, sizeof(CharType)*newLength);
      }
   }

   return(dstLength);
}

//----------------------------------------------------------------------------
// strFindAndReplaceCh
//    Parameters:
//       pDst        - The destination buffer to insert into. Non-NULL
//       dstLen      - The length of the destination buffer. Non-Zero (chars)
//       srcChr      - The source character to find and replace.
//       dstChr      - The new character to replace srcChr with.
//    Return:
//       -1 on Error, otherwise new pDst length in characters.
//----------------------------------------------------------------------------
template<typename CharType>
long strFindAndReplaceCh(CharType* pDst, DWORD dstLen, CharType srcChr, CharType dstChr)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pDst || !dstLen)
   {
      BFAIL("ERROR: strFindAndReplaceCh - Invalid string parameters.");
      return(-1);
   }

   DWORD dstLength = strLength(pDst);
   if (dstLength > dstLen)
   {
      BFAIL("ERROR: strFindAndReplaceCh -- Invalid string parameter.");
      return(-1);
   }

   for (DWORD idx=0; idx<dstLength; idx++)
   {
      if (pDst[idx] == srcChr)
         pDst[idx] = dstChr;
   }
   return(dstLength);
}

//----------------------------------------------------------------------------
// strIsEmpty
//    Parameters:
//       pSrc       - The source string to test
//    Return:
//       true if empty or error, otherwise false
//----------------------------------------------------------------------------
template<typename CharType>
bool strIsEmpty(const CharType* pSrc)
{
   if (!pSrc)
   {
      BFAIL("ERROR: strIsEmpty - Invalid string parameters.");
      return(true);
   }
   return(pSrc[0] == 0);
}

//----------------------------------------------------------------------------
// strToUpperCh
//    Parameters:
//       chr        - The original character
//    Return:
//       The upper case version of the character.
//----------------------------------------------------------------------------
template<typename CharType>
DWORD strLength(const CharType* pSrc)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pSrc)
   {
      BFAIL("ERROR: strLength - Invalid string parameters.");
      return(0);
   }

   if (sizeof(CharType) == sizeof(char))
      return(strlen((const char*)pSrc));
   else
      return(wcslen((const WCHAR*)pSrc));
}

//----------------------------------------------------------------------------
// strCompare
//    Parameters:
//       pDst        - The destination buffer to insert into. Non-NULL
//       dstLen      - The length of the destination buffer. Non-Zero (chars)
//       pSrc        - The source buffer to find and replace.
//       srcLen      - The length of the source buffer.
//       caseSens    - True to use case in comparison, False to ignore case.
//       srcCount    - The number of characters to compare from pSrc.
//    Return:
//       -1 if pDst < pSrc, 0 if pDst == pSrc, 1 if pDst > pSrc.
//----------------------------------------------------------------------------
template<typename CharType>
long strCompare(const CharType* pDst, DWORD dstLen, const CharType* pSrc, DWORD srcLen, bool caseSensitive, long srcCount)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   //-- Check the trivial case.
   if (srcCount == 0)
      return(0);

   //-- CompareStringW, wcscmp, and _wcsicmp don't handle NULLs like we want
   //-- them to.  So we have handle NULL strings manually.
   if (pDst == NULL)
      return ((pSrc == NULL) ? 0 : -1);
   else if (pSrc == NULL)
      return(1);

   DWORD dstLength = strLength(pDst);
   DWORD srcLength = strLength(pSrc);
   if ((dstLength > dstLen) || (srcLength > srcLen))
   {
      BFAIL("ERROR: strCompare - Invalid string parameters.");
      return(0);
   }

   if (srcCount < 0)
   {
      srcCount = max(srcLength, dstLength);
   }
   else
   {
      dstLength = srcCount;
      srcLength = srcCount;
   }

   //-- Compare the strings, taking into account the locale specific rules.
   int result;

#ifndef XBOX   
   DWORD flags = (caseSensitive ? 0 : NORM_IGNORECASE);
   if (sizeof(CharType) == sizeof(WCHAR))
      result = CompareStringW(LOCALE_USER_DEFAULT, flags, (WCHAR*)pDst, dstLength, (WCHAR*)pSrc, srcLength);
   else
      result = CompareStringA(LOCALE_USER_DEFAULT, flags, (char*)pDst, dstLength, (const char*)pSrc, srcLength);
   if (result == CSTR_LESS_THAN)
      return(-1);
   if (result == CSTR_GREATER_THAN)
      return(1);
#endif      

   //-- Either the strings are equal in the "locale aware" sense, or there was an error.
   //-- In either case, return a strict comparison of strings.
   if (caseSensitive)
   {
      if (sizeof(CharType) == sizeof(char))
         result = strncmp((char*)pDst, (const char*)pSrc, srcCount);
      else
         result = wcsncmp((WCHAR*)pDst, (const WCHAR*)pSrc, srcCount);
   }
   else
   {
      if (sizeof(CharType) == sizeof(char))
         result = _strnicmp((char*)pDst, (const char*)pSrc, srcCount);      
      else
         result = _wcsnicmp((WCHAR*)pDst, (const WCHAR*)pSrc, srcCount);
   }

   //-- Translate the result.
   if (result < 0)
      return(-1);
   if (result > 0)
      return(1);
   return(0);
}

//----------------------------------------------------------------------------
// strGetAsLong
//    Parameters:
//       pSrc        - The source buffer to convert.
//       srcLen      - The length of the source buffer. Non-Zero (chars)
//    Return:
//       The value of the contents as a long.
//----------------------------------------------------------------------------
template<typename CharType>
long strGetAsLong(const CharType* pSrc, DWORD srcLen)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pSrc || !srcLen)
   {
      BFAIL("ERROR: strGetAsLong - Invalid string parameters.");
      return(0);
   }

   DWORD srcLength = strLength(pSrc);
   if (srcLength > srcLen)
   {
      BFAIL("ERROR: strGetAsLong - Invalid string parameters.");
      return(0);
   }

   const CharType* pEnd = &pSrc[srcLength - 1];
   if (sizeof(CharType) == sizeof(char))
      return(strtol((const char*)pSrc, (char**)&pEnd, 10));
   else
      return(wcstol((const WCHAR*)pSrc, (WCHAR**)&pEnd, 10));
}

//----------------------------------------------------------------------------
// strGetAsFloat
//    Parameters:
//       pSrc        - The source buffer to convert.
//       srcLen      - The length of the source buffer. Non-Zero (chars)
//    Return:
//       The value of the contents as a float.
//----------------------------------------------------------------------------
template<typename CharType>
float strGetAsFloat(const CharType* pSrc, DWORD srcLen)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pSrc || !srcLen)
   {
      BFAIL("ERROR: strGetAsFloat - Invalid string parameters.");
      return(0);
   }

   DWORD srcLength = strLength(pSrc);
   if (srcLength > srcLen)
   {
      BFAIL("ERROR: strGetAsFloat - Invalid string parameters.");
      return(0);
   }

   const CharType* pEnd = &pSrc[srcLength - 1];
   if (sizeof(CharType) == sizeof(char))
      return((float)strtod((const char*)pSrc, (char**)&pEnd));
   else
      return((float)wcstod((const WCHAR*)pSrc, (WCHAR**)&pEnd));
}

//----------------------------------------------------------------------------
// strGetAsDouble
//    Parameters:
//       pSrc        - The source buffer to convert.
//       srcLen      - The length of the source buffer. Non-Zero (bytes)
//    Return:
//       The value of the contents as a double.
//----------------------------------------------------------------------------
template<typename CharType>
double strGetAsDouble(const CharType* pSrc, DWORD srcLen)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pSrc || !srcLen)
   {
      BFAIL("ERROR: strGetAsDouble - Invalid string parameters.");
      return(0);
   }

   DWORD srcLength = strLength(pSrc);
   if (srcLength > srcLen)
   {
      BFAIL("ERROR: strGetAsDouble - Invalid string parameters.");
      return(0);
   }

   const CharType* pEnd = &pSrc[srcLength - 1];
   if (sizeof(CharType) == sizeof(char))
      return(strtod((const char*)pSrc, (char**)&pEnd));
   else
      return(wcstod((const WCHAR*)pSrc, (WCHAR**)&pEnd));
}

//----------------------------------------------------------------------------
// strSetToLong
//    Parameters:
//       pDst        - The destination buffer to receive the converted value.
//       dstLen      - The length of the destination buffer. Non-Zero (chars)
//       value       - The value to put into pDst.
//    Return:
//       -1 on Error, otherwise the length of pDst in characters.
//----------------------------------------------------------------------------
template<typename CharType>
long strSetToLong(CharType* pDst, DWORD dstLen, long value)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pDst || !dstLen)
   {
      BFAIL("ERROR: strSetToLong - Invalid string parameters.");
      return(-1);
   }

   bool formatResult;
   
   if (sizeof(CharType) == sizeof(char))
      formatResult = strFormat((char*)pDst, dstLen, "%d", value);
   else   
      formatResult = strFormat((WCHAR*)pDst, dstLen, L"%d", value);
      
   if (!formatResult)
      return(-1);
   else
      return(strLength(pDst));
}

//----------------------------------------------------------------------------
// strSetToFloat
//    Parameters:
//       pDst        - The destination buffer to receive the converted value.
//       dstLen     - The size of the source buffer. Non-Zero (bytes)
//       value       - The value to put into pDst.
//    Return:
//       -1 on Error, otherwise the length of pDst in characters.
//----------------------------------------------------------------------------
template<typename CharType>
long strSetToFloat(CharType* pDst, DWORD dstLen, float value)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pDst || !dstLen)
   {
      BFAIL("ERROR: strSetToFloat - Invalid string parameters.");
      return(-1);
   }
   bool formatResult;
   if (sizeof(CharType) == sizeof(char))
      formatResult = strFormat((char*)pDst, dstLen, "%0.3f", value);
   else
      formatResult = strFormat((WCHAR*)pDst, dstLen, L"%0.3f", value);
      
   if (!formatResult)
      return(-1);
   else
      return(strLength(pDst));
}

//----------------------------------------------------------------------------
// strSetToDouble
//    Parameters:
//       pDst        - The destination buffer to receive the converted value.
//       dstLen     - The size of the source buffer. Non-Zero (bytes)
//       value       - The value to put into pDst.
//    Return:
//       -1 on Error, otherwise the length of pDst in characters.
//----------------------------------------------------------------------------
template<typename CharType>
long strSetToDouble(CharType* pDst, DWORD dstLen, double value)
{
   BCOMPILETIMEASSERT((sizeof(CharType) == sizeof(char)) || (sizeof(CharType) == sizeof(WCHAR)));
   
   if (!pDst || !dstLen)
   {
      BFAIL("ERROR: strSetToDouble - Invalid string parameters.");
      return(-1);
   }
   
   bool formatResult;
   if (sizeof(WCHAR) == 1)
      formatResult = strFormat((char*)pDst, dstLen, "%0.6f", value);
   else
      formatResult = strFormat((WCHAR*)pDst, dstLen, L"%0.6f", value);
      
   if (!formatResult)
      return(-1);
   else
      return(strLength(pDst));
}

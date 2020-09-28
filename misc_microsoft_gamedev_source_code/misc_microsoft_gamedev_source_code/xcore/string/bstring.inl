//============================================================================
//
//  BString.inl
//
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================

#include "strHelper.h"
#include "bstringmanager.h"
#include "strPathHelper.h"

#include "..\..\code\extlib\mgs\FormatString\FormatString.h"


//============================================================================
//  PRIVATE MACROS
//============================================================================
#define STRING_HEADER(pSrcString) ((sizeof(BStringHeader)>1)? (((BStringHeader*)pSrcString) - 1) : (BStringHeader*)pSrcString);
#define STRING_BUFFER(pHeader, CharType) ((sizeof(BStringHeader)>1) ? ((CharType*)(pHeader + 1)) : (CharType*)pHeader);

//----------------------------------------------------------------------------
// BStringTemplate::getEmptyString
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
const BStringTemplate<CharType, Allocator>& BStringTemplate<CharType, Allocator>::getEmptyString()
{
   static const BStringTemplate<CharType, Allocator> emptyString("");
   return emptyString;
}

//============================================================================
//  CONSTRUCTION/DESTRUCTION
//============================================================================

//----------------------------------------------------------------------------
// BStringTemplate::BStringTemplate
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
BStringTemplate<CharType, Allocator>::BStringTemplate(Allocator& alloc) :
   Allocator(alloc),
   mpString(NULL),
   mDataLength(0),
   mBufLength(0)
{
#ifdef BSTRING_PADDING   
   mPad0 = 0xFFFFFFFF;
   mPad1 = 0xFFFFFFFF;
#endif

#if BSTRING_ENABLE_TRACKING
   set("");
   trace();
#endif
}

//----------------------------------------------------------------------------
// BStringTemplate::BStringTemplate
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
BStringTemplate<CharType, Allocator>::BStringTemplate(const BStringTemplate<CharType, Allocator>& srcString) : 
   Allocator(srcString.getAllocator()),
   mpString(NULL),
   mDataLength(0),
   mBufLength(0)
{
#ifdef BSTRING_PADDING   
   mPad0 = 0xFFFFFFFF;
   mPad1 = 0xFFFFFFFF;
#endif

   if (srcString.length())
      set(srcString.getPtr());
   trace();
}

//----------------------------------------------------------------------------
// BStringTemplate::BStringTemplate
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
BStringTemplate<CharType, Allocator>::BStringTemplate(const BStringTemplate<CharType, Allocator>& srcString, Allocator& alloc) : 
   Allocator(alloc),
   mpString(NULL),
   mDataLength(0),
   mBufLength(0)
{
#ifdef BSTRING_PADDING   
   mPad0 = 0xFFFFFFFF;
   mPad1 = 0xFFFFFFFF;
#endif

   if (srcString.length())
      set(srcString.getPtr());
   trace();
}

//----------------------------------------------------------------------------
// BStringTemplate::BStringTemplate
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
BStringTemplate<CharType, Allocator>::BStringTemplate(const oppositeStringType& srcString) : 
   Allocator(srcString.getAllocator()),
   mpString(NULL),
   mDataLength(0),
   mBufLength(0)
{
#ifdef BSTRING_PADDING   
   mPad0 = 0xFFFFFFFF;
   mPad1 = 0xFFFFFFFF;
#endif

   if (srcString.length())
      set(srcString.getPtr());
   trace();
}

//----------------------------------------------------------------------------
// BStringTemplate::BStringTemplate
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
BStringTemplate<CharType, Allocator>::BStringTemplate(const oppositeStringType& srcString, Allocator& alloc) : 
   Allocator(alloc),
   mpString(NULL),
   mDataLength(0),
   mBufLength(0)
{
#ifdef BSTRING_PADDING   
   mPad0 = 0xFFFFFFFF;
   mPad1 = 0xFFFFFFFF;
#endif

   if (srcString.length())
      set(srcString.getPtr());
   trace();
}

//----------------------------------------------------------------------------
// BStringTemplate::BStringTemplate
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
BStringTemplate<CharType, Allocator>::BStringTemplate(const CharType* pSrcString, Allocator& alloc) : 
   Allocator(alloc),
   mpString(NULL),
   mDataLength(0),
   mBufLength(0)
{
#ifdef BSTRING_PADDING   
   mPad0 = 0xFFFFFFFF;
   mPad1 = 0xFFFFFFFF;
#endif

   set(pSrcString);
   trace();
}

//----------------------------------------------------------------------------
// BStringTemplate::BStringTemplate
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
BStringTemplate<CharType, Allocator>::BStringTemplate(const oppositeCharType* pSrcString, Allocator& alloc) : 
   Allocator(alloc),
   mpString(NULL),
   mDataLength(0),
   mBufLength(0)
{
#ifdef BSTRING_PADDING   
   mPad0 = 0xFFFFFFFF;
   mPad1 = 0xFFFFFFFF;
#endif

   set(pSrcString);
   trace();
}

//----------------------------------------------------------------------------
// BStringTemplate::BStringTemplate
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
template<typename OtherAllocator>
BStringTemplate<CharType, Allocator>::BStringTemplate(const BStringTemplate<CharType, OtherAllocator>& srcString, Allocator& alloc) :
   Allocator(alloc),
   mpString(NULL),
   mDataLength(0),
   mBufLength(0)
{
#ifdef BSTRING_PADDING   
   mPad0 = 0xFFFFFFFF;
   mPad1 = 0xFFFFFFFF;
#endif

   if (srcString.length())
      set(srcString.getPtr());
   trace();   
};

//----------------------------------------------------------------------------
// BStringTemplate::~BStringTemplate
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
BStringTemplate<CharType, Allocator>::~BStringTemplate()
{
   if (mpString)
   {
      BStringHeader* pHeader = STRING_HEADER(mpString);
      deleteBuffer(pHeader, cBytesPerCharLog2, mBufLength);

#ifdef BUILD_DEBUG      
      mpString = (CharType*)0xFEEEFEEE;
#endif      
   }
}

//----------------------------------------------------------------------------
// BStringTemplate::setAllocator
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::setAllocator(Allocator& alloc) 
{ 
   if (*this == alloc)
      return;
   
   if (mpString)
   {
      const uint size = mBufLength * sizeof(CharType);
      void* p = alloc.alloc(size);
      
      if (!p)
      {
         BFATAL_FAIL("Out of memory");
      }
      
      memcpy(p, mpString, size);
      
      BStringHeader* pHeader = STRING_HEADER(mpString);
      deleteBuffer(pHeader, cBytesPerCharLog2, mBufLength);
      
      mpString = static_cast<CharType*>(p);         
   }
   
   *this = alloc; 
};

//----------------------------------------------------------------------------
// BStringTemplate::trace
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::trace()
{
#if 0
#if BSTRING_ENABLE_TRACKING
   if (mpString)
   {
      BStringHeader* pHeader = STRING_HEADER(mpString);

      BDebugCallStackEntry entry;
      if (gDebugHelp.stackWalkCaller(entry, 2))
      {
         snprintf(pHeader->mStringCreator, countof(pHeader->mStringCreator), "%s + 0x%x  -- %s(%d) -- %s\r\n", 
         BStrConv::toANSI(entry.mFunctionName), 
         entry.mFunctionOffset, 
         BStrConv::toANSI(entry.mFile), 
         entry.mLine, 
         BStrConv::toANSI(entry.mModule));
      }
      else
         pHeader->mStringCreator[0] = 0;
   }
#endif
#endif
   return;
}


//============================================================================
//  SETTING
//============================================================================

//----------------------------------------------------------------------------
// BStringTemplate::set
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::set(const char* pSrcString, long srcCount, long srcPos)
{
   //BRANCH

   //-- Handle an empty string.
   if ((pSrcString == NULL) || (*pSrcString == 0) || srcCount == 0)
   {
      empty();
      return;
   }

   if (sizeof(CharType) == sizeof(char))
   {
      //-- Trying to set our pointer within our own buffer would be bad.
      if (mpString)
      {
         //-- See if we are setting ourself to ourself.
         if (mpStringA == pSrcString && srcCount == length() && srcPos<=0)
         {
            BFAIL("BStringTemplate::set - invalid string operation.");
            return;
         }

         //-- See if we are setting ourself to somewhere within our own buffer.
         char*          pFirstChar = mpStringA;
         char*          pLastChar  = mpStringA + mBufLength - 1;
         if ((pSrcString >= pFirstChar) && (pSrcString <= pLastChar))
         {
            //-- We are trying to set it within ourselves.  We can shortcut this
            //-- whole operation by just moving pSrcString to mpString.

            //-- pSrcString had damn well better fit inside our buffer.  Otherwise,
            //-- it means that it starts inside our buffer and overwrites the end.
            long maxLength = mBufLength - (pSrcString - pFirstChar);

            // jce [6/11/2004] -- this seems wrong... making new check below 
            //if (srcCount > 0)
            // maxLength = srcCount;

            long srcLength = strLength(pSrcString);
            if (srcPos > 0)
            {
               pSrcString = pSrcString + srcPos;
               srcLength -= srcPos;
            }
                                          
            if((srcCount>=0) && (srcLength>srcCount))
               srcLength = srcCount;

            BASSERT(srcLength < maxLength);
            if (srcLength >= maxLength)
            {            
               BFATAL_FAIL("BStringTemplate::set - possible memory overwrite.");
               //releaseBuffer();
               //return;
            }

            //-- Move the memory.
            MoveMemory(mpString, pSrcString, srcLength * sizeof(char));

            //-- Terminate it.
            mpString[srcLength] = 0;

            //-- Set the length.
            setDataLength(srcLength);
            check();
            return;
         }
      }

      if (srcPos > 0)
         pSrcString = pSrcString + srcPos;
      //-- See what size buffer we need.
      if (srcCount < 0)
         srcCount = strLength(pSrcString);
      else
         srcCount = min(srcCount, static_cast<long>(strLength(pSrcString)));

      //-- Make sure our buffer is a suitable size.
      acquireBuffer(srcCount + 1);

      //-- Copy it in.
      CopyMemory(mpString, pSrcString, srcCount * sizeof(char));

      //-- Terminate it.
      mpString[srcCount] = 0;

      //-- Set the length.
      setDataLength(srcCount);
      check();
   }
   else
   {
      if (srcPos > 0)
         pSrcString = pSrcString + srcPos;
      //-- See what size buffer we need.
      if (srcCount < 0)
         srcCount = strLength(pSrcString);
      else
         srcCount = min(srcCount, static_cast<long>(strLength(pSrcString)));

      const long numCharsNeeded = MultiByteToWideChar(CP_ACP, 0, pSrcString, srcCount, NULL, 0);
      if (numCharsNeeded <= 0)
      {
         acquireBuffer(1);
         mpString[0] = L'\0';
         
         setDataLength(0);
         check();
      }
      else
      {
         acquireBuffer(numCharsNeeded + 1);

         const long numCharsWritten = MultiByteToWideChar(CP_ACP, 0, pSrcString, srcCount, mpStringW, numCharsNeeded);
         BASSERT(numCharsWritten == numCharsNeeded);

         mpString[numCharsWritten] = L'\0';
         
         setDataLength(numCharsWritten);
         check();
      }
   }      
}

//----------------------------------------------------------------------------
// BStringTemplate::set
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::set(const WCHAR* pSrcString, long srcCount, long srcPos)
{
   if ((NULL == pSrcString) || (pSrcString[0] == L'\0') || (0 == srcCount))
   {
      empty();
      return;
   }

   if (srcPos > 0)
      pSrcString += srcPos;
   
   if (srcCount < 0)
   {
      srcCount = wcslen(pSrcString);
      if (0 == srcCount)
      {
         empty();
         return;
      }
   }
   else
   {
      srcCount = min(srcCount, static_cast<long>(wcslen(pSrcString)));
   }
   
   if (sizeof(CharType) == sizeof(char))
   {  
      const long numCharsNeeded = WideCharToMultiByte(CP_ACP, 0, (const WCHAR*)pSrcString, srcCount, NULL, 0, NULL, NULL);
      if (numCharsNeeded <= 0)
      {
         empty();
         return;
      }

      //-- Make sure our buffer is a suitable size.
      acquireBuffer(numCharsNeeded + 1);

      const long numCharsWritten = WideCharToMultiByte(CP_ACP, 0, (const WCHAR*)pSrcString, srcCount, mpStringA, numCharsNeeded, NULL, NULL);     
      BASSERT(numCharsWritten == numCharsNeeded);

      //-- Terminate it.
      mpString[numCharsWritten] = 0;

      setDataLength(numCharsWritten);
      check();
   }
   else
   {
      acquireBuffer(srcCount + 1);
      MoveMemory(mpString, pSrcString, srcCount * sizeof(WCHAR));
      mpString[srcCount] = L'\0';
      
      setDataLength(srcCount);
      check();
   }      
}

//============================================================================
//  MODIFICATION
//============================================================================

//----------------------------------------------------------------------------
//  BStringTemplate::empty
//  Sets this string to an empty string, "".  If a buffer was already
//  allocated, this function does not release the buffer back to the memory
//  manager.
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::empty()
{
   if (mpString)
   {
      //-- Terminate the string.
      *mpString = 0;

      //-- Set the length to 0.
      setDataLength(0);
      check();
      return;
   }

   //-- Get a buffer.
   BStringHeader* pHeader = newBuffer(1, cBytesPerCharLog2, mBufLength);

   //-- Set up our string pointer.
   mpString = STRING_BUFFER(pHeader, CharType);

   //-- Terminate the string.
   *mpString = 0;

   //-- Set the length to 0.
   setDataLength(0);
   check();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::setNULL()
{
   releaseBuffer();
}

//----------------------------------------------------------------------------
// BStringTemplate::copy
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::copy(const CharType *srcString, long srcCount, long srcPos)
{
   set(srcString, srcCount, srcPos);
}


//----------------------------------------------------------------------------
// BStringTemplate::copyTok
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
long BStringTemplate<CharType, Allocator>::copyTok(const BStringTemplate<CharType, Allocator>& srcString, long srcCount, long srcPos, const CharType *pTokens)
{
   //-- Handle an empty string.
   if (srcString.isEmpty())
      return -1;
   if (srcPos >= srcString.length())
      return -1;

   // Fixup srcPos.
   if(srcPos<0)
      srcPos=0;

   // Scan past any tokens.
   const CharType *src = srcString.mpString;
   long count=0;
   while((srcCount<0 || count<srcCount) && src[srcPos] != 0)
   {
      // Is it a token character.
      bool isToken=false;
      const CharType *token=pTokens;
      while(*token)
      {
         if(src[srcPos] == *token)
         {
            isToken=true;
            break;
         }
         token++;
      }

      // If not a token, then break out of the loop.
      if(!isToken)
         break;

      count++;
      srcPos++;
   }

   // Copy data until we hit another token.
   long copyLength=0;
   while((srcCount<0 || count<srcCount) && src[srcPos+copyLength] != 0)
   {
      // Is it a token character.
      bool isToken=false;
      const CharType *token=pTokens;
      while(*token)
      {
         if(src[srcPos+copyLength] == *token)
         {
            isToken=true;
            break;
         }
         token++;
      }

      // If a token, then break out of the loop.
      if(isToken)
         break;

      count++;
      copyLength++;
   }

   // Nothing to copy means we're done.
   if(copyLength == 0)
   {
      empty();
      return(-1);
   }

   // Copy data.
   copy(srcString.getPtr(), copyLength, srcPos);

   // Return the next starting position.
   return(srcPos+copyLength);   
}

//----------------------------------------------------------------------------
// BStringTemplate::append
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::append(const BStringTemplate<CharType, Allocator>& srcString, long srcCount, long srcPos)
{
   if(!srcString.mpString)
      return;

   if(srcCount<0)
      append(srcString.mpString, srcString.length(), srcPos);
   else
      append(srcString.mpString, srcCount, srcPos);
}


//----------------------------------------------------------------------------
// BStringTemplate::append
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::append(const CharType *srcString, long srcCount, long srcPos)
{
   if (!mpString)
   {
      set(srcString, srcCount, srcPos);
      return;
   }
   
   if (srcPos > 0)
      srcString += srcPos;

   if (srcCount < 0)
      srcCount = strLength(srcString);
   else
      srcCount = min(srcCount, static_cast<long>(strLength(srcString)));
      
   BStringHeader* pHeader = STRING_HEADER(mpString);
   
   DWORD newLen = mDataLength + srcCount;
   if (newLen >= maxChars())
   {
      //-- Save our old string.
      CharType*       pOldString = mpString;
      BStringHeader* pOldHeader = pHeader;
      const uint oldDataSize = mBufLength;
      
      pOldHeader;

      const CharType* pSrcBuffer = srcString;
      
      long length = mDataLength;

      //-- Get a new buffer.
      pHeader  = newBuffer(newLen + 1, cBytesPerCharLog2, mBufLength);
      mpString = STRING_BUFFER(pHeader, CharType);

      //-- Set contents
      CopyMemory(mpString, pOldString, length * sizeof(CharType));
      CopyMemory(&mpString[length], pSrcBuffer, srcCount * sizeof(CharType));
      length += srcCount;

      //-- Terminate it.
      mpString[length] = 0;
      setDataLength(length);

#if BSTRING_ENABLE_TRACKING
      copyHeader(pHeader, pOldHeader);
#endif

      //-- Release our old string.
      deleteBuffer(pOldHeader, cBytesPerCharLog2, oldDataSize);         
   }
   else
   {
      CopyMemory(mpString + mDataLength, srcString, srcCount * sizeof(CharType));
      mpString[newLen] = 0;
      setDataLength(newLen);
   }
      
   check();
}

//----------------------------------------------------------------------------
// BStringTemplate::append
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::append(CharType c)
{
   if (!mpString)
   {
      acquireBuffer(2);

      //-- Copy it in.
      mpString[0] = c;
      mpString[1] = 0;

      //-- Set the length.
      setDataLength(1);
                  
      return;
   }

   BStringHeader* pHeader = STRING_HEADER(mpString);

   DWORD newLen = mDataLength + 1;
   if (newLen >= maxChars())
   {
      //-- Save our old string.
      CharType* pOldString = mpString;
      BStringHeader* pOldHeader = pHeader;
      const uint oldDataSize = mBufLength;

      pOldHeader;
      
      long length = mDataLength;

      //-- Get a new buffer.
      pHeader  = newBuffer(newLen + 1, cBytesPerCharLog2, mBufLength);
      mpString = STRING_BUFFER(pHeader, CharType);

      //-- Set contents
      CopyMemory(mpString, pOldString, length * sizeof(CharType));
      mpString[length] = c;
      length++;

      //-- Terminate it.
      mpString[length] = 0;
      setDataLength(length);

#if BSTRING_ENABLE_TRACKING
      copyHeader(pHeader, pOldHeader);
#endif

      //-- Release our old string.
      deleteBuffer(pOldHeader, cBytesPerCharLog2, oldDataSize);         
   }
   else
   {
      mpString[mDataLength] = c;
      mpString[newLen] = 0;
      setDataLength(newLen);
   }
}

//----------------------------------------------------------------------------
// BStringTemplate::prepend
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::prepend(const BStringTemplate<CharType, Allocator>& srcString, long srcCount, long srcPos)
{
   //BRANCH

   //-- if there is already some content
   if (mpString)
   {      
      BStringHeader* pHeader = STRING_HEADER(mpString);

      if (srcCount < 0)
         srcCount = srcString.length();

      if ((DWORD)(mDataLength + srcCount) >= maxChars())
      {
         //-- Save our old string.
         CharType*          pOldString = mpString;
         BStringHeader* pOldHeader = pHeader;
         const uint oldDataSize = mBufLength;

         const CharType* pSrcBuffer = srcString.getPtr();
         if (srcPos > 0)
         {
            if (srcPos > srcString.length() || (srcPos + srcCount) > srcString.length())
            {
               BFAIL("ERRRO: BStringTemplate::prepend -- invalid string parameters.");
               return;
            }

            pSrcBuffer = pSrcBuffer + srcPos;
         }

         long length = mDataLength;

         //-- Get a new buffer.
         pHeader  = newBuffer(length + srcCount + 1, cBytesPerCharLog2, mBufLength);
         mpString = STRING_BUFFER(pHeader, CharType);

         //-- Set contents
         CopyMemory(mpString, pSrcBuffer, srcCount * sizeof(CharType));
         CopyMemory(&mpString[srcCount],  pOldString, length* sizeof(CharType));
         length += srcCount;

         //-- Terminate it.
         mpString[length] = 0;
         setDataLength(length);
         check();

#if BSTRING_ENABLE_TRACKING
         copyHeader(pHeader, pOldHeader);
#endif

         //-- Release our old string.
         deleteBuffer(pOldHeader, cBytesPerCharLog2, oldDataSize);         
      }
      else
      {
         //-- Insert at the beginning
         long length = strInsert(mpString, maxChars(), 0, srcString.getPtr(), srcString.maxChars(), srcCount, srcPos);
         if (length < 0)
            return;

         setDataLength(length);
         check();
      }
   }
   //-- this is really a initial set operation
   else
   {
      set(srcString, srcCount, srcPos);
   }
}

//----------------------------------------------------------------------------
//  BStringTemplate::insert
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::insert(long dstPos, const BStringTemplate<CharType, Allocator>& srcString, long srcCount, long srcPos)
{
   //BRANCH

   //-- If they pass us an empty string, we don't do anything.
   if (srcString.isEmpty() || srcCount == 0)
      return;

   //-- Set default pos, if not specified.
   if (srcPos < 0)
      srcPos = 0;

   //-- If our string is empty, we just set it.
   if (isEmpty())
   {
      set(srcString, srcCount, srcPos);
      return;
   }

   //-- See how much space we need.
   BStringHeader* pHeader = STRING_HEADER(mpString);
   long length = srcString.length();

   // Override with count passed in, if valid
   if (srcCount>0)
      length = srcCount - srcPos;
   else
      length = length - srcPos;

   DWORD totalLength = length + mDataLength;

   //-- If our current buffer is too small
   //const CharType* pSrcString = srcString.getPtr();
   if (maxChars() <= totalLength)
   {
      CharType*          pOldString = mpString;
      BStringHeader* pOldHeader = pHeader;
      const uint oldDataSize = mBufLength;

      //-- Get a new buffer.
      pHeader  = newBuffer(totalLength + 1, cBytesPerCharLog2, mBufLength);
      mpString = STRING_BUFFER(pHeader, CharType);

      setDataLength(strCopy(mpString, maxChars(), pOldString, oldDataSize));
      check();

#if BSTRING_ENABLE_TRACKING
      copyHeader(pHeader, pOldHeader);
#endif

      //-- Release our old string.
      deleteBuffer(pOldHeader, cBytesPerCharLog2, oldDataSize);
   }

   length = strInsert(mpString, maxChars(), dstPos, srcString.getPtr(), srcString.maxChars(), srcCount, srcPos);
   if (length > 0)
   {
      setDataLength(length);      
      check();
   }
}

//----------------------------------------------------------------------------
// BStringTemplate::insert
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::insert(long dstPos, CharType srcChr)
{
   //BRANCH

   //-- Handle an empty string.
   if (isEmpty())
   {
      CharType str[2] = { srcChr, 0 };
      set(str);
      return;
   }

   // MS 6/14/2005: 8799
   if(dstPos < 0 || dstPos > length())
   {
      BFAIL("invalid dstPos");
      return;
   }
   
   if (srcChr == 0)
   {
      BFAIL("invalid srcChr");
      return;
   }

   //-- Get our string header.
   BStringHeader* pHeader = STRING_HEADER(mpString);

   if ((DWORD)(mDataLength + 1) >= maxChars())
   {
      //-- Save our old string.
      CharType*          pOldString = mpString;
      BStringHeader* pOldHeader = pHeader;
      const uint oldDataSize = mBufLength;

      //-- Get a new buffer.
      long length = mDataLength + 1;
      pHeader  = newBuffer(length + 1, cBytesPerCharLog2, mBufLength);
      mpString = STRING_BUFFER(pHeader, CharType);

      //-- Set contents
      CopyMemory(mpString, pOldString, dstPos * sizeof(CharType));
      mpString[dstPos] = srcChr;
      CopyMemory(&mpString[dstPos+1], pOldString + dstPos, (length - dstPos - 1) * sizeof(CharType));

      //-- Terminate it.
      mpString[length] = 0;
      setDataLength(length);
      check();

#if BSTRING_ENABLE_TRACKING
      copyHeader(pHeader, pOldHeader);
#endif

      //-- Release our old string.
      deleteBuffer(pOldHeader, cBytesPerCharLog2, oldDataSize);
   }
   else
   {
      long length = strInsertCh(mpString, maxChars(), dstPos, srcChr);
      if (length < 0)
         return;
      setDataLength(length);
      check();
   }
}

//----------------------------------------------------------------------------
// BStringTemplate::remove
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::remove(long dstPos, long count)
{
   //BRANCH

   //-- Handle an empty string.
   //-- Also handle someone sending in 0 for the count.  JER
   if (isEmpty() || count <= 0)
      return;

   //-- Get our string header.
   long length = strRemove(mpString, maxChars(), dstPos, count);
   if (length < 0)
      return;

   setDataLength(length);
   check();
}

//----------------------------------------------------------------------------
// BStringTemplate::remove
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::remove(long dstPos, const BStringTemplate<CharType, Allocator>& srcString, long srcCount, long srcPos)
{
   //BRANCH

   //-- Handle an empty string.
   if (isEmpty())
      return;

   //-- Get our string header.
   long length = strRemove(mpString, maxChars(), dstPos, srcString.getPtr(), srcString.maxChars(), srcCount, srcPos);
   if (length < 0)
      return;

   setDataLength(length);
   check();
}

//----------------------------------------------------------------------------
// BStringTemplate::remove
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::remove(CharType srcChr)
{
   //BRANCH

   //-- Handle an empty string.
   if (isEmpty())
      return;

   //-- Get our string header.
   long length = strRemoveCh(mpString, maxChars(), 0, srcChr);
   if (length < 0)
      return;

   setDataLength(length);
   check();
}

//----------------------------------------------------------------------------
// BStringTemplate::crop
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::crop(long firstChr, long lastChr)
{
   //BRANCH

   //-- Handle an empty string.
   if (isEmpty())
      return;

   //-- Get our string header.
   long length = strCrop(mpString, maxChars(), firstChr, lastChr);
   if (length < 0)
      return;

   setDataLength(length);
   check();
}

//----------------------------------------------------------------------------
// BStringTemplate::substring
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
bool BStringTemplate<CharType, Allocator>::substring(uint start, uint end)
{
   const uint curLen = length();

   if ((start >= curLen) || (end <= start))
   {
      if (curLen)
         empty();
      return false;
   }

   end = Math::Min(end, curLen);

   const uint newLen = end - start;

   if (start)
      memmove(mpString, mpString + start, newLen * cBytesPerChar);

   mpString[newLen] = 0;

   setDataLength(newLen);
   check();
   
   return true;
}

//----------------------------------------------------------------------------
// BStringTemplate::locFormat
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::locFormat(const CharType* pFormat, ...)
{
   //BRANCH

   //-- Let the other format function do all the work.
   va_list args;
   va_start(args, pFormat);     

   if (sizeof(CharType) == sizeof(char))
      format(pFormat, args);
   else
      locFormatArgs(pFormat, args);

   va_end(args);
}

//----------------------------------------------------------------------------
// BStringTemplate::locFormatArgs
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::locFormatArgs(const CharType* pFormat, va_list args)
{
   if (sizeof(CharType) == sizeof(char))
   {
      formatArgs(pFormat, args);
   }
   else
   {      
      //-- Handle an empty string.
      if ((pFormat == NULL) || (*pFormat == 0))
      {
         empty();
         return;
      }

      //-- Get a workspace.  It would be swell if we knew the right size buffer
      //-- to allocate, but that would be a lot of work.  So for now we just use
      //-- a fixed buffer size as a temporary holder.
      WCHAR workspace [FORMAT_WORKSPACE_SIZE] = L"";
      WCHAR workspace2[FORMAT_WORKSPACE_SIZE] = L"";
      
      //-- Print it to the workspace.      
      MGS::_FormatStringW(0, (LPWSTR) workspace, FORMAT_WORKSPACE_SIZE, (LPWSTR) workspace2, FORMAT_WORKSPACE_SIZE, (LPCWSTR) pFormat, &args);
      
      //-- Let set() do the rest.
      set(workspace);
   }
}

//----------------------------------------------------------------------------
// BStringTemplate::format
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::format(const CharType* pFormat, ...)
{
   //BRANCH

   //-- Let the other format function do all the work.
   va_list args;
   va_start(args, pFormat);     
   formatArgs(pFormat, args);
   va_end(args);
}

//----------------------------------------------------------------------------
// BStringTemplate::format
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::formatArgs(const CharType* pFormat, va_list args)
{
   //BRANCH

   //-- Handle an empty string.
   if ((pFormat == NULL) || (*pFormat == 0))
   {
      empty();
      return;
   }

   //-- Get a workspace.  It would be swell if we knew the right size buffer
   //-- to allocate, but that would be a lot of work.  So for now we just use
   //-- a fixed buffer size as a temporary holder.
   CharType workspace[FORMAT_WORKSPACE_SIZE] = { 0 };

   //-- Print it to the workspace.
   strFormat(workspace, FORMAT_WORKSPACE_SIZE, pFormat, args);

   //-- Let set() do the rest.
   set(workspace);
}

//----------------------------------------------------------------------------
// BStringTemplate::trimLeft
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::trimLeft(const CharType* pTokens)
{
   //BRANCH

   //-- Handle an empty string.
   if (isEmpty())
      return;

   //-- Get our string header.
   long length = strTrimLeft(mpString, maxChars(), pTokens);
   if (length < 0)
      return;

   setDataLength(length);
   check();
}

//----------------------------------------------------------------------------
// BStringTemplate::trimRight
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::trimRight(const CharType* pTokens)
{
   //BRANCH

   //-- Handle an empty string.
   if (isEmpty())
      return;

   //-- Get our string header.
   long length = strTrimRight(mpString, maxChars(), pTokens);
   if (length < 0)
      return;

   setDataLength(length);
   check();   
}

//----------------------------------------------------------------------------
// BStringTemplate::toLower
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::toLower()
{
   //BRANCH

   //-- Handle an empty string.
   if (isEmpty())
      return;

   strToLower(mpString, maxChars());
}

//----------------------------------------------------------------------------
// BStringTemplate::toUpper
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::toUpper()
{
   //BRANCH

   //-- Handle an empty string.
   if (isEmpty())
      return;

   strToUpper(mpString, maxChars());
}

//----------------------------------------------------------------------------
// BStringTemplate::removePath
//    Note: It is assumed that the string will be in path form: for example:
//       <Drive><Directories><Filename>.<Ext> -- "C:\Windows\System\system32.dll". 
//       This function removes <Drive><Directories>.
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::removePath()
{
   if (isEmpty())
      return;

   long sepPos = findRight('\\', -1);
   if (sepPos < 0)
      sepPos = findRight('/', sepPos);
   if (sepPos < 0)
      return;

   long pos = findRight('.');
   if (pos > sepPos)
      right(sepPos + 1);
}

//----------------------------------------------------------------------------
// BStringTemplate::removeExtension
//    Note: It is assumed that the string will be in path form: for example:
//       <Drive><Directories><Filename>.<Ext> -- "C:\Windows\System\system32.dll". 
//       Not all elements have to be present.
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::removeExtension()
{
   if (isEmpty())
      return;

   long sepPos = findRight('\\');
   if (sepPos < 0)
      sepPos = findRight('/');
         
   long pos = findRight('.');
   if (pos > sepPos)
      left(pos);
}

//----------------------------------------------------------------------------
// BStringTemplate::removeTrailingPathSeperator
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::removeTrailingPathSeperator()
{
   if (isEmpty())
      return;

   uint l = length();
   if (l)
   {
      charType c = getChar(l - 1);
      
      if ((c == '\\') || (c == '/'))
         left(l - 1);
   }
}

//----------------------------------------------------------------------------
// endianSwap
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::endianSwap()
{
   if (sizeof(CharType) == sizeof(WCHAR))
   {
      long len=length();
      if(len)
         EndianSwitchWords(reinterpret_cast<WORD*>(mpStringW), len);
   }      
}

//----------------------------------------------------------------------------
// check
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::check(void) const
{
#ifdef BUILD_DEBUG
   if (!mpString)
   {
      BDEBUG_ASSERT(0 == mBufLength);
      BDEBUG_ASSERT(0 == mDataLength);
      return;
   }
   
   if (sizeof(CharType) == sizeof(WCHAR))
   {
      BDEBUG_ASSERT(wcslen(mpStringW) == mDataLength);
   }      
   else
   {
      BDEBUG_ASSERT(strlen(mpStringA) == mDataLength);
   }
   
   BDEBUG_ASSERT(mBufLength >= mDataLength);
#endif   
}

//============================================================================
//  SEARCHING
//============================================================================
//----------------------------------------------------------------------------
//  BStringTemplate::findLeft
//  Returns the index of the first occurence of srcString in this string.  If
//  srcString is not found, -1 is returned.  If startPos is -1, the search is
//  started from the beginning of the string.  Otherwise, the search is
//  started from startPos.
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
long BStringTemplate<CharType, Allocator>::findLeft(const BStringTemplate<CharType, Allocator>& srcString, long startPos) const
{
   //-- Handle an empty string.
   if (isEmpty() || srcString.isEmpty())
      return -1;

   return(strFindLeft(mpString, maxChars(), srcString.getPtr(), srcString.maxChars(), startPos));
}

//----------------------------------------------------------------------------
//  BStringTemplate::findRight
//  Returns the index of the last occurence of srcString in this string.  If
//  srcString is not found, -1 is returned.  If startPos is -1, the search is
//  started from the end of the string.  Otherwise, the search is started
//  from startPos.
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
long BStringTemplate<CharType, Allocator>::findRight(const BStringTemplate<CharType, Allocator>& srcString, long startPos) const
{
   //-- Handle an empty string.
   if (isEmpty() || srcString.isEmpty())
      return -1;

   return(strFindRight(mpString, maxChars(), srcString.getPtr(), srcString.maxChars(), startPos));
}

//----------------------------------------------------------------------------
// BStringTemplate::findLeft
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
long BStringTemplate<CharType, Allocator>::findLeft(CharType srcChr, long startPos) const
{
   //-- Handle an empty string.
   if (isEmpty())
      return -1;
   return(strFindLeftCh(mpString, maxChars(), srcChr, startPos));
}

//----------------------------------------------------------------------------
// BStringTemplate::findRight
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
long BStringTemplate<CharType, Allocator>::findRight(CharType srcChr, long startPos) const
{
   //-- Handle an empty string.
   if (isEmpty())
      return -1;

   return(strFindRightCh(mpString, maxChars(), srcChr, startPos));
}

//----------------------------------------------------------------------------
//  BStringTemplate::findAndReplace
//  Replaces all occurences of srcString with newString.
//  newString can be empty.
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::findAndReplace(const BStringTemplate<CharType, Allocator>& srcString, const BStringTemplate<CharType, Allocator>& newString)
{
   //BRANCH

   //-- Handle an empty string.
   if (isEmpty())
      return;

   if (srcString.isEmpty())
   {
      BFAIL("ERROR: BStringTemplate::findAndReplace -- Invalid source string.");
      return;
   }

   //-- Get our string header.
   BStringHeader* pHeader = STRING_HEADER(mpString);

   long count = 0;
   long pos = strFindLeft(mpString, maxChars(), srcString.getPtr(), srcString.maxChars(), 0);
   while (pos >= 0)
   {
      count++;
      pos = strFindLeft(mpString, maxChars(), srcString.getPtr(), srcString.maxChars(), pos+1);
   }

   long sizeDiff = (newString.length() - srcString.length());

   // possibly need new memory allocation
   if (sizeDiff > 0)
   {
      DWORD sizeNeeded = (mDataLength + (sizeDiff * count)) + 1;

      if (sizeNeeded > (DWORD)mBufLength)
      {
         CharType*          pOldString = mpString;
         BStringHeader* pOldHeader = pHeader;
         const uint oldDataSize = mBufLength;

         //-- Get a new buffer.
         pHeader  = newBuffer(sizeNeeded, cBytesPerCharLog2, mBufLength);
         mpString = STRING_BUFFER(pHeader, CharType);

         setDataLength(strCopy(mpString, maxChars(), pOldString, oldDataSize));
         check();
         
#if BSTRING_ENABLE_TRACKING
         copyHeader(pHeader, pOldHeader);
#endif

         //-- Release our old string.
         deleteBuffer(pOldHeader, cBytesPerCharLog2, oldDataSize);
      }
   }

   long length = strFindAndReplace(mpString, maxChars(), srcString.getPtr(), newString.getPtr());
   if (length < 0)
      return;

   setDataLength(length);
   check();
}

//----------------------------------------------------------------------------
//  BStringTemplate::findAndReplace
//  Replaces all occurences of srcChr with dstChr.
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::findAndReplace(CharType srcChr, CharType dstChr)
{
   //-- Handle an empty string.
   if (isEmpty())
      return;

   //-- Get our string header.

   long length = strFindAndReplaceCh(mpString, maxChars(), srcChr, dstChr);
   if (length < 0)
      return;

   setDataLength(length);
   check();
}


//============================================================================
//  STATUS
//============================================================================

//----------------------------------------------------------------------------
// BStringTemplate::isEmpty
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
bool BStringTemplate<CharType, Allocator>::isEmpty() const
{
   return 0 == length();
}

//----------------------------------------------------------------------------
// BStringTemplate::length
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
long BStringTemplate<CharType, Allocator>::length() const
{
   if (!mpString)
   {
#ifdef BUILD_DEBUG
      BASSERT(0 == mDataLength);
      BASSERT(0 == mBufLength);
#endif      
      return(0);
   }
   
#ifdef BUILD_DEBUG
   if (sizeof(CharType) == sizeof(char))
   {
      BASSERT(strlen(mpStringA) == mDataLength);
   }
   else
   {
      BASSERT(wcslen(mpStringW) == mDataLength);
   }
#endif
   
   return(mDataLength);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
long BStringTemplate<CharType, Allocator>::compare(const CharType * pSrcString, bool caseSensitive, long srcCount) const
{
   long srcLength = strLength(pSrcString);

   if (isEmpty())
      return((srcLength==0)?0:-1);

   return(strCompare(mpString, maxChars(), pSrcString, srcLength, caseSensitive, srcCount));
}

//----------------------------------------------------------------------------
// BStringTemplate::compare
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
long BStringTemplate<CharType, Allocator>::compare(const BStringTemplate<CharType, Allocator>& srcString, bool caseSensitive, long srcCount) const
{

   if (isEmpty())
      return(srcString.isEmpty()?0:-1);

   return(strCompare(mpString, maxChars(), srcString.getPtr(), srcString.maxChars(), caseSensitive, srcCount));
}

//----------------------------------------------------------------------------
//BStringTemplate::asUnicode
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
const WCHAR* BStringTemplate<CharType, Allocator>::asUnicode(BStringTemplate<WCHAR, Allocator>& buffer) const
{
   if (isEmpty())
   {
      buffer.empty();
   }
   else if (sizeof(CharType) == sizeof(WCHAR))
   {
      if ((void*)this != (void*)&buffer)
         buffer.set(mpStringW);
   }
   else
   {
      const long numCharsNeeded = MultiByteToWideChar(CP_ACP, 0, mpStringA, length(), NULL, 0);
      if (numCharsNeeded <= 0)
      {
         buffer.acquireBuffer(1);
         buffer.mpString[0] = L'\0';
         buffer.setDataLength(0);
         buffer.check();
      }
      else
      {
         buffer.acquireBuffer(numCharsNeeded + 1);

         const long numCharsWritten = MultiByteToWideChar(CP_ACP, 0, mpStringA, length(), buffer.mpStringW, numCharsNeeded);
         BASSERT(numCharsWritten == numCharsNeeded);

         buffer.mpString[numCharsWritten] = L'\0';
         buffer.setDataLength(numCharsWritten);
         buffer.check();
      }
   }  
   
   return buffer.getPtr();    
}

//----------------------------------------------------------------------------
// BStringTemplate::asANSI
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
const char* BStringTemplate<CharType, Allocator>::asANSI(BStringTemplate<char, Allocator>& buffer) const
{
   if (isEmpty())
      buffer.empty();
   else if (sizeof(CharType) == sizeof(char))
   {
      if ((void*)this != (void*)&buffer)
         buffer.set(mpStringA);
   }
   else
   {
      const long numCharsNeeded = WideCharToMultiByte(CP_ACP, 0, mpStringW, length(), NULL, 0, NULL, NULL);
      if (numCharsNeeded <= 0)
      {
         buffer.empty();
      }
      else
      {
         buffer.acquireBuffer(numCharsNeeded + 1);

         const long numCharsWritten = WideCharToMultiByte(CP_ACP, 0, mpStringW, length(), buffer.mpStringA, numCharsNeeded, NULL, NULL);
         BASSERT(numCharsWritten == numCharsNeeded);

         buffer.mpString[numCharsWritten] = '\0';
         buffer.setDataLength(numCharsWritten);
      }         
   }
   
   buffer.check();
   
   return buffer.getPtr();
}

//----------------------------------------------------------------------------
// BStringTemplate::asLong
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
long BStringTemplate<CharType, Allocator>::asLong() const
{
   if (isEmpty())
      return 0;

   return (strGetAsLong(mpString, maxChars()));
}

//----------------------------------------------------------------------------
// BStringTemplate::asInt64
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
int64 BStringTemplate<CharType, Allocator>::asInt64() const
{
   if (isEmpty())
      return 0;

   int64 value;
   if (sizeof(CharType) == sizeof(char))
      value = _atoi64(mpStringA);
   else
      value = _wtoi64(mpStringW);
   
   return value;
}

//----------------------------------------------------------------------------
// BStringTemplate::asUInt64
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
uint64 BStringTemplate<CharType, Allocator>::asUInt64() const
{
   if (isEmpty())
      return 0;

   uint64 value;
   if (sizeof(CharType) == sizeof(char))
      value = _strtoui64(mpStringA, NULL, 10);
   else
      value = _wcstoui64(mpStringW, NULL, 10);

   return value;
}

//----------------------------------------------------------------------------
// BStringTemplate::asFloat
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
float BStringTemplate<CharType, Allocator>::asFloat() const
{
   if (isEmpty())
      return 0.0f;

   return (strGetAsFloat(mpString, maxChars()));
}

//----------------------------------------------------------------------------
// BStringTemplate::asDouble
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
double BStringTemplate<CharType, Allocator>::asDouble() const
{
   if (isEmpty())
      return 0.0f;

   return (strGetAsDouble(mpString, maxChars()));
}

//----------------------------------------------------------------------------
// BStringTemplate::convertToVector
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
bool BStringTemplate<CharType, Allocator>::convertToVector3(float* pVector3) const
{
   if (!pVector3)
   {
      pVector3[0] = 0.0f;
      pVector3[1] = 0.0f;
      pVector3[2] = 0.0f;
      return false;
   }
   float x,y,z;
   BStringTemplate<char, Allocator> ansiBuf;
   asANSI(ansiBuf);
   if(sscanf_s(ansiBuf.getPtr(), "%f,%f,%f", &x, &y, &z) < 3)
   {
      pVector3[0] = 0.0f;
      pVector3[1] = 0.0f;
      pVector3[2] = 0.0f;
      return false;
   }
   pVector3[0] = x;
   pVector3[1] = y;
   pVector3[2] = z;
   return true;
}


//----------------------------------------------------------------------------
// BStringTemplate::setToLong
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::setToLong( long value )
{
   BStringHeader* pHeader;
   if (!mpString)
   {
      pHeader  = newBuffer(NUMBER_WORKSPACE_SIZE + 1, cBytesPerCharLog2, mBufLength);
      mpString = STRING_BUFFER(pHeader, CharType);
      mpString[0] = 0;
      setDataLength(0);
      check();
   }
   else
      pHeader = STRING_HEADER(mpString);

   long length = strSetToLong(mpString, maxChars(), value);
   if (length < 0)
      return;

   setDataLength(length);
   check();
}

//----------------------------------------------------------------------------
// BStringTemplate::setToFloat
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::setToFloat( float value )
{
   BStringHeader* pHeader;
   if (!mpString)
   {
      pHeader  = newBuffer(NUMBER_WORKSPACE_SIZE + 1, cBytesPerCharLog2, mBufLength);
      mpString = STRING_BUFFER(pHeader, CharType);
      mpString[0] = 0;
      setDataLength(0);
      check();
   }
   else
      pHeader = STRING_HEADER(mpString);

   long length = strSetToFloat(mpString, maxChars(), value);
   if (length < 0)
      return;

   setDataLength(length);
   check();
}

//----------------------------------------------------------------------------
// BStringTemplate::setToDouble
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::setToDouble( double value )
{
   BStringHeader* pHeader;
   if (!mpString)
   {
      pHeader  = newBuffer(NUMBER_WORKSPACE_SIZE + 1, cBytesPerCharLog2, mBufLength);
      mpString = STRING_BUFFER(pHeader, CharType);
      mpString[0] = 0;
      setDataLength(0);
      check();
   }
   else
      pHeader = STRING_HEADER(mpString);

   long length = strSetToDouble(mpString, maxChars(), value);
   if (length < 0)
      return;

   setDataLength(length);
   check();
}

//============================================================================
//  SETTING OPERATORS
//============================================================================

//----------------------------------------------------------------------------
// BStringTemplate::operator =
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
BStringTemplate<CharType, Allocator>& BStringTemplate<CharType, Allocator>::operator= (const BStringTemplate<CharType, Allocator>& srcString)
{
   set(srcString.getPtr());
   return(*this);
}

//----------------------------------------------------------------------------
// BStringTemplate::operator =
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
BStringTemplate<CharType, Allocator>& BStringTemplate<CharType, Allocator>::operator= (const oppositeStringType& srcString)
{
   set(srcString.getPtr());
   return(*this);
}

//----------------------------------------------------------------------------
// BStringTemplate::operator=
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
BStringTemplate<CharType, Allocator>& BStringTemplate<CharType, Allocator>::operator= (const char* pSrcString)
{
   set(pSrcString);
   return(*this);
}

//----------------------------------------------------------------------------
// BStringTemplate::operator=
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
BStringTemplate<CharType, Allocator>& BStringTemplate<CharType, Allocator>::operator= (const WCHAR* pSrcString)
{
   set(pSrcString);
   return(*this);
}

//----------------------------------------------------------------------------
// BStringTemplate::operator =
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
template<typename OtherAllocator>
BStringTemplate<CharType, Allocator>& BStringTemplate<CharType, Allocator>::operator= (const BStringTemplate<CharType, OtherAllocator>& srcString)
{
   set(srcString.getPtr());
   return(*this);
}

//============================================================================
//  MODIFICATION OPERATORS
//============================================================================

//----------------------------------------------------------------------------
//BStringTemplate::operator +=
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
BStringTemplate<CharType, Allocator>& BStringTemplate<CharType, Allocator>::operator += (const BStringTemplate<CharType, Allocator>& srcString)
{
   append(srcString);
   return(*this);
}


//----------------------------------------------------------------------------
// BStringTemplate::operator +=
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
BStringTemplate<CharType, Allocator>& BStringTemplate<CharType, Allocator>::operator += (CharType srcChr)
{
   if (mpString)
   {
      insert(mDataLength, srcChr);
   }
   else
   {
      CharType src[2] = { srcChr, 0 };
      append(src);
   }
   return(*this);
}

//----------------------------------------------------------------------------
// BStringTemplate::operator +=
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
BStringTemplate<CharType, Allocator>& BStringTemplate<CharType, Allocator>::operator += (const CharType *src)
{
   append(src);
   return(*this);
}

//----------------------------------------------------------------------------
// BStringTemplate::operator +=
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
BStringTemplate<CharType, Allocator>& BStringTemplate<CharType, Allocator>::operator += (const oppositeCharType *src)
{
   BStringTemplate temp(src);
   append(temp);
   return(*this);
}

//----------------------------------------------------------------------------
//BStringTemplate::operator +=
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
template<typename OtherAllocator>  
BStringTemplate<CharType, Allocator>& BStringTemplate<CharType, Allocator>::operator += (const BStringTemplate<CharType, OtherAllocator>& srcString)
{
   append(srcString.getPtr());
   return(*this);
}

//============================================================================
//  BStringTemplate::operator ==
//============================================================================
template<typename CharType, typename Allocator>
bool BStringTemplate<CharType, Allocator>::operator == (const BStringTemplate<CharType, Allocator>& srcString) const
{
   // most common
   if (length() != srcString.length())
      return false;
     
   // less common
   return (compare(srcString.getPtr()) == 0);
}

//============================================================================
//  BStringTemplate::operator ==
//============================================================================
template<typename CharType, typename Allocator>
template<typename OtherAllocator>
bool BStringTemplate<CharType, Allocator>::operator == (const BStringTemplate<CharType, OtherAllocator>& srcString) const
{
   // most common
   if (length() != srcString.length())
      return false;

   // less common
   return (compare(srcString.getPtr()) == 0);
}


//----------------------------------------------------------------------------
// BStringTemplate::operator ==
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
bool BStringTemplate<CharType, Allocator>::operator == (const CharType* srcString) const
{
   return (compare(srcString) == 0);
}

//----------------------------------------------------------------------------
// BStringTemplate::operator !=
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
bool BStringTemplate<CharType, Allocator>::operator != (const BStringTemplate<CharType, Allocator>& srcString) const
{
   // most common
   if (length() != srcString.length())
      return true;
   
   // less common
   return (compare(srcString.getPtr()) != 0);
}

//----------------------------------------------------------------------------
// BStringTemplate::operator !=
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
template<typename OtherAllocator>
bool BStringTemplate<CharType, Allocator>::operator != (const BStringTemplate<CharType, OtherAllocator>& srcString) const
{
   // most common
   if (length() != srcString.length())
      return true;

   // less common
   return (compare(srcString.getPtr()) != 0);
}

//----------------------------------------------------------------------------
// BStringTemplate::operator !=
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
bool BStringTemplate<CharType, Allocator>::operator != (const CharType *srcString) const
{
   return (compare(srcString) != 0);
}

//----------------------------------------------------------------------------
// BStringTemplate::operator >
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
bool BStringTemplate<CharType, Allocator>::operator > (const BStringTemplate<CharType, Allocator>& srcString) const
{
   return (compare(srcString.getPtr()) == 1);
}

//----------------------------------------------------------------------------
// BStringTemplate::operator >
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
template<typename OtherAllocator>
bool BStringTemplate<CharType, Allocator>::operator > (const BStringTemplate<CharType, OtherAllocator>& srcString) const
{
   return (compare(srcString.getPtr()) == 1);
}

//----------------------------------------------------------------------------
// BStringTemplate::operator >=
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
bool BStringTemplate<CharType, Allocator>::operator >= (const BStringTemplate<CharType, Allocator>& srcString) const
{
   return (compare(srcString.getPtr()) >= 0);
}

//----------------------------------------------------------------------------
// BStringTemplate::operator >
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
template<typename OtherAllocator>
bool BStringTemplate<CharType, Allocator>::operator >= (const BStringTemplate<CharType, OtherAllocator>& srcString) const
{
   return (compare(srcString.getPtr()) >= 0);
}

//----------------------------------------------------------------------------
// BStringTemplate::operator <
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
bool BStringTemplate<CharType, Allocator>::operator < (const BStringTemplate<CharType, Allocator>& srcString) const
{
   return (compare(srcString.getPtr()) == -1);
}

//----------------------------------------------------------------------------
// BStringTemplate::operator <
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
template<typename OtherAllocator>
bool BStringTemplate<CharType, Allocator>::operator < (const BStringTemplate<CharType, OtherAllocator>& srcString) const
{
   return (compare(srcString.getPtr()) == -1);
}

//----------------------------------------------------------------------------
// BStringTemplate::operator <=
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
bool BStringTemplate<CharType, Allocator>::operator <= (const BStringTemplate<CharType, Allocator>& srcString) const
{
   return (compare(srcString.getPtr()) <= 0);
}

//----------------------------------------------------------------------------
// BStringTemplate::operator <
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
template<typename OtherAllocator>
bool BStringTemplate<CharType, Allocator>::operator <= (const BStringTemplate<CharType, OtherAllocator>& srcString) const
{
   return (compare(srcString.getPtr()) <= 0);
}

//============================================================================
//  PROTECTED FUNCTIONS
//============================================================================
template<typename CharType, typename Allocator>
DWORD BStringTemplate<CharType, Allocator>::maxChars() const
{
   if (!mpString)
      return(0);

   return(mBufLength);
}

//----------------------------------------------------------------------------
// BStringTemplate::maxBytes
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
DWORD BStringTemplate<CharType, Allocator>::maxBytes() const
{
   if (!mpString)
      return(0);

   return(mBufLength*sizeof(CharType));
}

//============================================================================
//  PRIVATE FUNCTIONS
//============================================================================
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::releaseBuffer()
{
   if (!mpString)
      return;

   BStringHeader* pHeader = STRING_HEADER(mpString);

   deleteBuffer(pHeader, cBytesPerCharLog2, mBufLength);

   mpString = NULL;
   mBufLength = 0;
   mDataLength = 0;
}

//----------------------------------------------------------------------------
//  acquireBuffer()
//  This function is guaranteed to work.  You do not have to check mpString
//  for success.
//  size is in characters!
//----------------------------------------------------------------------------
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::acquireBuffer(long size)
{
#ifdef BSTRING_PADDING
   BDEBUG_ASSERT(mPad0 == 0xFFFFFFFF);
   BDEBUG_ASSERT(mPad1 == 0xFFFFFFFF);
#endif
   
   BASSERT((uint)size < cMaxStringLen);

   BStringHeader *pOldHeader = NULL; 
   BStringHeader *pHeader = NULL;
   if (mpString)
   {
      //-- If the current buffer is big enough, reuse it.
      pHeader = STRING_HEADER(mpString);
      if (mBufLength >= size)
         return;

      // reference the existing header
      pOldHeader = pHeader;
   }
   
   const uint oldDataSize = mBufLength;

   //-- Get a buffer.
   pHeader = newBuffer(size, cBytesPerCharLog2, mBufLength);

   //-- Set up our string pointer.
   mpString = STRING_BUFFER(pHeader, CharType);

   if (pOldHeader)
   {
#if BSTRING_ENABLE_TRACKING
      copyHeader(pHeader, pOldHeader);
#endif

      //-- Release the old buffer
      deleteBuffer(pOldHeader, cBytesPerCharLog2, oldDataSize);
   }

}

//==============================================================================
// BStringTemplate::makeRawString
//==============================================================================
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::makeRawString(long size)
{
   //-- Make sure our buffer is a suitable size.
   acquireBuffer(size+1);
  
   //-- Set the length.
   setDataLength(size);
}

//==============================================================================
// BStringTemplate::standardizePath
//==============================================================================
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::standardizePath(void)
{
   // Sanity for null/empty
   if(isEmpty())
      return;

   // Standardize.
   setDataLength((long)strPathStandardizePath(mpString, mpString, (DWORD)mDataLength));
   check();
}


//==============================================================================
// BStringTemplate::standardizePathFrom
//==============================================================================
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::standardizePathFrom(const CharType *src, long len)
{
   // Get src length.
   if(len<0)
      len = strLength(src);
   makeRawString(len+1);

   // Standardize.
   setDataLength((long)strPathStandardizePath(src, mpString, len));
   check();
}


//==============================================================================
// BStringTemplate::standardizePathFrom
//==============================================================================
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::standardizePathFrom(const BStringTemplate &src)
{
   standardizePathFrom(src, src.length());
}

//==============================================================================
// BStringTemplate::getPtr
//==============================================================================
template<typename CharType, typename Allocator>
const CharType* BStringTemplate<CharType, Allocator>::getPtr(void) const
{
   if (isEmpty())
      return BStringDefaults<CharType>::getEmptyString();
   
   return mpString;
}


//==============================================================================
// ascending sort function for the bstrings(used for the quick sort on BSimpleArray)
//==============================================================================
template<typename CharType, typename Allocator>
int BStringTemplate<CharType, Allocator>::arrayAscendingSortFunc(const void *a, const void *b)
{
   const BStringTemplate *pA = reinterpret_cast<const BStringTemplate *>(a);
   const BStringTemplate *pB = reinterpret_cast<const BStringTemplate *>(b);
   return pA->compare(*pB);
}

//==============================================================================
// descending sort function for the bstrings(used for the quick sort on BSimpleArray)
//==============================================================================
template<typename CharType, typename Allocator>
int BStringTemplate<CharType, Allocator>::arrayDescendingSortFunc(const void *a, const void *b)
{
   const BStringTemplate *pA = reinterpret_cast<const BStringTemplate *>(a);
   const BStringTemplate *pB = reinterpret_cast<const BStringTemplate *>(b);
   return pB->compare(*pA);
}

//==============================================================================
// BStringTemplate::newBuffer
//==============================================================================
template<typename CharType, typename Allocator>
BStringHeader* BStringTemplate<CharType, Allocator>::newBuffer(uint numChars, uint bytesPerCharLog2, ushort& bufferChars)
{
   bytesPerCharLog2;
   
   const uint size = numChars * sizeof(CharType);
   
   uint actualSize;
   BStringHeader* p = static_cast<BStringHeader*>(alloc(size, &actualSize));
   if (!p)
   {
      BFATAL_FAIL("Out of memory");
   }
   
   if (sizeof(CharType) == 2)
      actualSize >>= 1;
   
   BDEBUG_ASSERT(actualSize <= USHRT_MAX);
   bufferChars = static_cast<ushort>(actualSize);
   
   return p;
}

//==============================================================================
// BStringTemplate::deleteBuffer
//==============================================================================
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::deleteBuffer(BStringHeader* pData, uint bytesPerCharLog2, uint dataSize)
{
   dataSize;
   bytesPerCharLog2;
   dealloc(pData);
}

//==============================================================================
// BStringTemplate::copyHeader
//==============================================================================
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::copyHeader(BStringHeader *pNewHeader, BStringHeader *pOldHeader)
{
#if BSTRING_ENABLE_TRACKING
   memcpy(pNewHeader, pOldHeader, sizeof(BStringHeader));
#endif
}

//==============================================================================
// BStringTemplate::hash
//==============================================================================
template<typename CharType, typename Allocator>
uint BStringTemplate<CharType, Allocator>::hash(uint prevHash) const
{
   uint hash = hashFast(&mDataLength, sizeof(mDataLength), prevHash);
   if (mDataLength)
      hash = hashFast(getPtr(), mDataLength * sizeof(CharType), hash);
   return hash;
}

//==============================================================================
// BStringTemplate::swap
//==============================================================================
template<typename CharType, typename Allocator>
void BStringTemplate<CharType, Allocator>::swap(BStringTemplate& other)
{
   std::swap(mpString,     other.mpString);
   std::swap(mDataLength,  other.mDataLength);
   std::swap(mBufLength,   other.mBufLength);
}

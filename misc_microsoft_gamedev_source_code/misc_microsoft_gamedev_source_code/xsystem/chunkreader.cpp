//==============================================================================
// chunkreader.cpp
//
// Copyright (c) 1999-2007, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xsystem.h"
#include "chunkreader.h"
#include "config.h"
#include "econfigenum.h"
#include "logmanager.h"

#include "bfileStream.h"

// This is how many calls we want to make to the UI.
static const long cCallbackMeter=20;

//==============================================================================
// 
//==============================================================================
BChunkReader* getChunkReader(BStream* pStream, bool strict )
{
   return new BChunkReader(pStream, strict);
}

//==============================================================================
// BChunkReader::BChunkReader
//==============================================================================
BChunkReader::BChunkReader() :
   mpStream(NULL),
   mFileSize(0),
   mManageFile(false),
   mStrict(false),
   mUIUpdateCallback(NULL),
   //mChunks doesn't need any ctor args.
   mLog(0),
   mCurrentCallbackPos(0),
   mpStringBuffer(0),
   mdwStringBuffer(0),
   mdwPeekOffset((DWORD) -1),
   mdwArrayLength((DWORD) -1),
   mPeekTag(0),
   mPeekTagSize(-1),
   mUIMaxPercent(1.0f),
   mNoSkipTagMode(false)
{
#ifdef _BANG
#ifndef BUILD_FINAL
   gConfig.get(cConfigLogChunker, &mLog);
#endif
#endif
}

//==============================================================================
// BChunkReader::BChunkReader
//==============================================================================
BChunkReader::BChunkReader(BStream* pStream, bool strict) :
   mpStream(pStream),
   mFileSize(0),
   mManageFile(false),
   mStrict(strict),
   mUIUpdateCallback(NULL),
   //mChunks doesn't need any ctor args.
   mLog(0),
   mCurrentCallbackPos(0),
   mpStringBuffer(0),
   mdwStringBuffer(0),
   mdwArrayLength(0),
   mdwPeekOffset(0),
   mPeekTagSize(0),
   mPeekTag(0),
   mNoSkipTagMode(false)
{
   if (mpStream)
   {
      mFileSize = static_cast<long>(mpStream->size());
      mSizePerCallback = mFileSize / cCallbackMeter;
   }

#ifdef _BANG
#ifndef BUILD_FINAL
   gConfig.get(cConfigLogChunker, &mLog);
#endif
#endif
}

//==============================================================================
// BChunkReader::BChunkReader
//==============================================================================
BChunkReader::BChunkReader(long dirID, const char* pFilename, bool strict) :
   mpStream(NULL),
   mFileSize(0),
   mManageFile(false),
   mStrict(strict),
   mUIUpdateCallback(NULL),
   //mChunks doesn't need any ctor args.
   mLog(0),
   mCurrentCallbackPos(0),
   mpStringBuffer(0),
   mdwStringBuffer(0),
   mNoSkipTagMode(false)
{
   if (openFile(dirID, pFilename))
      mManageFile=true;
#ifdef _BANG
#ifndef BUILD_FINAL
   gConfig.get(cConfigLogChunker, &mLog);
#endif
#endif
}

//==============================================================================
// BChunkReader::~BChunkReader
//==============================================================================
BChunkReader::~BChunkReader()
{
   delete [] mpStringBuffer;

   if (mManageFile == true)
   {
      if (mpStream != NULL)
         closeFile();
   }
}

//==============================================================================
// BChunkReader::getFilePercentage
//==============================================================================
float BChunkReader::getFilePercentage(void) const
{
   if (mFileSize <= 0)
      return(0.0f);

   uint64 filePos = mpStream->curOfs();

   if (mFileSize == 0)
      return 0.0f;

   return (static_cast<float>(filePos) / static_cast<float>(mFileSize));
}

//==============================================================================
// BChunkReader::readExpectedTag
//==============================================================================
long BChunkReader::readExpectedTag(BChunkTag tag, long atPosition, long *size)
{
   BChunkTag tempTag=BCHUNKTAG("\0\0");
   long s=0;
   if (!size)
      size = &s;

   long startPosition=0;
   long previousPosition=-1;
   // if we're skipping ahead, save the current position
   if (atPosition != -1)
   {
      uint64 currentFilePosition = mpStream->curOfs();

      previousPosition = static_cast<long>(currentFilePosition);
      seek(atPosition);
   }
   long result = readTag(&tempTag, size, startPosition);
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   //Debug.
#ifdef _BANG
   if (mLog)
   {
      char eT1=(char)(tag&0x00FF);
      char eT2=(char)(tag>>8);
      char rT1=(char)(tempTag&0x00FF);
      char rT2=(char)(tempTag>>8);

      uint64 currentFilePosition = mpStream->curOfs();

      blog("RET : Expected Tag='%c%c', ReadTag='%c%c', Size=%d, Start=%d, FilePos=%d.", eT1, eT2, rT1, rT2, *size, startPosition, currentFilePosition);
   }
#endif

   //Verify the tag was correct.
   if (tempTag != tag)
   {
      strictASSERT();
      return(0);
   }

   //Add the entry for this chunk to the chunk entry stack.
   long stackSize=mChunks.getNumber();
   if (mChunks.setNumber(stackSize+1) == false)
   {
      strictASSERT();
      return(0);
   }
   mChunks[stackSize].mTag=tag;
   mChunks[stackSize].mSize=*size;
   mChunks[stackSize].mStartPosition=startPosition;
   mChunks[stackSize].mPreviousPosition=previousPosition;

   return(1);
}

//==============================================================================
// BChunkReader::readExpectedTagAndLevel
//==============================================================================
long BChunkReader::readExpectedTagAndLevel(BChunkTag tag, long& stackLevel, long atPosition)
{
   BChunkTag tempTag=BCHUNKTAG("\0\0");
   long size=0;
   long startPosition=0;
   long previousPosition=-1;
   // if we're skipping ahead, save the current position
   if (atPosition != -1)
   {
      uint64 currentFilePosition = mpStream->curOfs();

      previousPosition = static_cast<long>(currentFilePosition);
      seek(atPosition);
   }
   long result=readTag(&tempTag, &size, startPosition);
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   //Debug.
#ifdef _BANG
   if (mLog)
   {
      char eT1=(char)(tag&0x00FF);
      char eT2=(char)(tag>>8);
      char rT1=(char)(tempTag&0x00FF);
      char rT2=(char)(tempTag>>8);
      uint64 currentFilePosition = mpStream->curOfs();

      blog("RET2: Expected Tag='%c%c', ReadTag='%c%c', Size=%d, Start=%d, FilePos=%d.", eT1, eT2, rT1, rT2, size, startPosition, currentFilePosition);
   }
#endif

   //Verify the tag was correct.
   if (tempTag != tag)
   {
      strictASSERT();
      return(0);
   }

   //Add the entry for this chunk to the chunk entry stack.
   long stackSize=mChunks.getNumber();
   if (mChunks.setNumber(stackSize+1) == false)
   {
      strictASSERT();
      return(0);
   }
   mChunks[stackSize].mTag=tag;
   mChunks[stackSize].mSize=size;
   mChunks[stackSize].mStartPosition=startPosition;
   mChunks[stackSize].mPreviousPosition=previousPosition;
   //Return the stack level.
   stackLevel=stackSize;

   return(1);
}

//==============================================================================
// BChunkReader::validateChunkRead
//==============================================================================
long BChunkReader::validateChunkRead(BChunkTag tag)
{
   //Make sure our file pointer is valid.
   if (mpStream == NULL)
   {
      strictASSERT();
      return(0);
   }

   long chunkSize=0;
   if(mNoSkipTagMode)
   {
      //Read the size.
      if (mpStream->readBytes(&chunkSize, sizeof(long)) == 0)
      {
         strictASSERT();
         return(0);
      }
#ifdef XBOX
      EndianSwitchDWords((DWORD*)&chunkSize, 1);
#endif
   }

   long stackSize=mChunks.getNumber();

   //Debug.
#ifdef _BANG
   if (mLog)
   {
      char eT1=(char)(tag&0x00FF);
      char eT2=(char)(tag>>8);
      char rT1=(char)((mChunks[stackSize-1].mTag)&0x00FF);
      char rT2=(char)((mChunks[stackSize-1].mTag)>>8);
      uint64 currentFilePosition = mpStream->curOfs();

      blog("VCR: Expected Tag='%c%c', Expected Size=%d, ActualTag='%c%c', ActualSizeRead=%d, FilePos=%d.",
         eT1, eT2, mChunks[stackSize-1].mSize, rT1, rT2, currentFilePosition-mChunks[stackSize-1].mStartPosition,
         currentFilePosition);
   }
#endif

   //Verify that the tags are right first.
   if (mChunks[stackSize-1].mTag != tag)
   {
      strictASSERT();
      return(0);
   }

   //Now do a simple subtraction of where we started the chunk data (we don't count
   //the "header" (2 BYTEs of Tag and 4 BYTEs of data size) in the size of the chunk
   //data) from where we currently are in the file.  If that result is
   //the same as the chunkSize, we're good.
   uint64 currentFilePosition = mpStream->curOfs();

   long diff = static_cast<long>(currentFilePosition) - mChunks[stackSize-1].mStartPosition;
   if(!mNoSkipTagMode)
      chunkSize=mChunks[stackSize-1].mSize;
   if (diff != chunkSize)
   {
      strictASSERT();
      return(0);
   }

   // if we had a previous position, seek back
   if (mChunks[stackSize-1].mPreviousPosition != -1)
   {
      seek(mChunks[stackSize-1].mPreviousPosition);      
   }

   //"Pop" the stack down one.
   mChunks.setNumber(stackSize-1);

   // last chunk?
   if (mChunks.getNumber() == 0)
   {
      if (mUIUpdateCallback)
      {
         // do a final update to the UI
         mUIUpdateCallback(mUIMaxPercent*getFilePercentage());
      }
   }

   return(1);
}



//==============================================================================
// readBYTE
//==============================================================================
long BChunkReader::readBYTE(BYTE *data)
{
   return read(data, sizeof(*data));
} //readBYTE

//==============================================================================
// readSignedChar
//==============================================================================
long BChunkReader::readSignedChar(signed char *data)
{
   return read(data, sizeof(*data));
} //readSignedChar

//==============================================================================
// readWORD
//==============================================================================
long BChunkReader::readWORD(WORD *data)
{
#ifdef XBOX
   if(read(data, sizeof(*data))==0)
      return 0;
   EndianSwitchWords(data, 1);
   return 1;
#else
   return read(data, sizeof(*data));
#endif
} //readWORD

//==============================================================================
// readDWORD
//==============================================================================
long BChunkReader::readDWORD(DWORD *data)
{
#ifdef XBOX
   if(read(data, sizeof(*data))==0)
      return 0;
   EndianSwitchDWords((DWORD*)data, 1);
   return 1;
#else
   return read(data, sizeof(*data));
#endif
} //readDWORD

//==============================================================================
// readInt64
//==============================================================================
long BChunkReader::readInt64(__int64 *data)
{
#ifdef XBOX
   if(read(data, sizeof(*data))==0)
      return 0;
   EndianSwitchQWords((unsigned __int64*)data, 1);
   return 1;
#else
   return read(data, sizeof(*data));
#endif
} //readInt64

//==============================================================================
// readFloat
//==============================================================================
long BChunkReader::readFloat(float *data)
{
#ifdef XBOX
   if(read(data, sizeof(*data))==0)
      return 0;
   EndianSwitchDWords((DWORD*)data, 1);
   return 1;
#else
   return read(data, sizeof(*data));
#endif
} //readFloat

//==============================================================================
// readDouble
//==============================================================================
long BChunkReader::readDouble(double *data)
{
#ifdef XBOX
   if(read(data, sizeof(*data))==0)
      return 0;
   EndianSwitchQWords((unsigned __int64*)data, 1);
   return 1;
#else
   return read(data, sizeof(*data));
#endif
} //readDouble

//==============================================================================
// readChar
//==============================================================================
long BChunkReader::readChar(char *data)
{
   return read(data, sizeof(*data));
} //readChar

//==============================================================================
// readLong
//==============================================================================
long BChunkReader::readLong(long *data)
{
#ifdef XBOX
   if(read(data, sizeof(*data))==0)
      return 0;
   EndianSwitchDWords((DWORD*)data, 1);
   return 1;
#else
   return read(data, sizeof(*data));
#endif
} //readLong

//==============================================================================
// readShort
//==============================================================================
long BChunkReader::readShort(short *data)
{
#ifdef XBOX
   if(read(data, sizeof(*data))==0)
      return 0;
   EndianSwitchWords((WORD*)data, 1);
   return 1;
#else
   return read(data, sizeof(*data));
#endif
} //readShort

//==============================================================================
// readBool
//==============================================================================
long BChunkReader::readBool(bool *data)
{
   return read(data, sizeof(*data));
} //readBool

//==============================================================================
// readVector
//==============================================================================
long BChunkReader::readVector(BVector *data)
{
#ifdef XBOX
   // For compatibility between xbox and pc scenarios, always read in 3 floats,
   // even though the xbox vector contains 4. 
   if(read(data, sizeof(float)*3)==0)
      return 0;
   EndianSwitchDWords((DWORD*)data, 3);
   data->w=0.0f;
   return 1;
#else
   return read(data, sizeof(*data));
#endif
}

//==============================================================================
// readMatrix
//==============================================================================
long BChunkReader::readMatrix(BMatrix *data)
{
#ifdef XBOX
   if(read(data, sizeof(*data))==0)
      return 0;
   EndianSwitchDWords((DWORD*)data, 12);
   return 1;
#else
   return read(data, sizeof(*data));
#endif
}

//==============================================================================
//  BChunkReader::readBSimString
//==============================================================================
bool BChunkReader::readBSimString(BSimString& string)
{
   long length=0;
   if (mpStream->readBytes(&length, sizeof(length)) != sizeof(length))
      return(false);

#ifdef XBOX
   EndianSwitchDWords((DWORD*)&length, 1);
   if(length>1024)
   {
      BASSERT(0); // possible endian switch error
      return(false);
   }
#endif

   const bool unicodeString = (length & 0x80000000) == 0;
   length &= 0x7FFFFFFF;

   //-- Handle an empty string.
   if (length <= 0)
   {
      string.empty();
      return(true);
   }

   if (unicodeString)
   {
      BUString temp;
      temp.makeRawString(length);

      // Read.
      if (mpStream->readBytes(temp.getString(), sizeof(WCHAR) * length) != sizeof(WCHAR) * length)
         return(false);

      // Terminate the string since we don't save out the null.
      temp.getString()[length] = L'\0';

#ifdef XBOX
      temp.endianSwap();
#endif

      temp.check();

      string.set(temp.getPtr());
   }
   else
   {
      // Manually acquire space in the string.
      string.makeRawString(length);

      // Read.
      if (mpStream->readBytes(string.getString(), sizeof(BCHAR_T) * length) != sizeof(BCHAR_T) * length)
         return(false);

      // Terminate the string since we don't save out the null.
      string.getString()[length] = B('\0');
   }

   string.check();

   return(true);
}

//==============================================================================
//  BChunkReader::readBSimString
//==============================================================================
bool BChunkReader::readBSimString(BSimString* pString)
{
   if (!pString)
      return false;

   return(readBSimString(*pString));
}

//==============================================================================
//  BChunkReader::readBString
//==============================================================================
bool BChunkReader::readBString(BString& string)
{
   long length=0;
   if (mpStream->readBytes(&length, sizeof(length)) != sizeof(length))
      return(false);

#ifdef XBOX
   EndianSwitchDWords((DWORD*)&length, 1);
   if(length>1024)
   {
      BASSERT(0); // possible endian switch error
      return(false);
   }
#endif

   const bool unicodeString = (length & 0x80000000) == 0;
   length &= 0x7FFFFFFF;

   //-- Handle an empty string.
   if (length <= 0)
   {
      string.empty();
      return(true);
   }

   if (unicodeString)
   {
      BUString temp;
      temp.makeRawString(length);

      // Read.
      if (mpStream->readBytes(temp.getString(), sizeof(WCHAR) * length) != sizeof(WCHAR) * length)
         return(false);

      // Terminate the string since we don't save out the null.
      temp.getString()[length] = L'\0';

#ifdef XBOX
      temp.endianSwap();
#endif

      temp.check();

      string.set(temp);
   }
   else
   {
      // Manually acquire space in the string.
      string.makeRawString(length);

      // Read.
      if (mpStream->readBytes(string.getString(), sizeof(BCHAR_T) * length) != sizeof(BCHAR_T) * length)
         return(false);

      // Terminate the string since we don't save out the null.
      string.getString()[length] = B('\0');
   }      

   string.check();

   return(true);
}

//==============================================================================
//  BChunkReader::readBString
//==============================================================================
bool BChunkReader::readBString(BString* pString)
{
   if (!pString)
      return false;

   return(readBString(*pString));
}

//==============================================================================
// BChunkReader::readTagged
//==============================================================================
long BChunkReader::readTagged(BChunkTag tag, void *pdata, DWORD dwBytes)
{
   long size=0;
   
   if (!readExpectedTag(tag, &size))
   {
      strictASSERT();
      return(0);
   }

   if (!read(pdata, dwBytes))
   {
      strictASSERT();
      return(0);
   }

   return(1);

} // BChunkReader::readTagged

//==============================================================================
// readTaggedBYTE
//==============================================================================
long BChunkReader::readTaggedBYTE(BChunkTag tag, BYTE* data)
{
   return readTagged(tag, data, sizeof(BYTE));
}

//==============================================================================
// readTaggedWORD
//==============================================================================
long BChunkReader::readTaggedWORD(BChunkTag tag, WORD* data)
{
#ifdef XBOX
   if(readTagged(tag, data, sizeof(WORD))==0)
      return 0;
   EndianSwitchWords(data, 1);
   return 1;
#else
   return readTagged(tag, data, sizeof(WORD));
#endif
}

//==============================================================================
// readTaggedDWORD
//==============================================================================
long BChunkReader::readTaggedDWORD(BChunkTag tag, DWORD* data)
{
#ifdef XBOX
   if(readTagged(tag, data, sizeof(DWORD))==0)
      return 0;
   EndianSwitchDWords(data, 1);
   return 1;
#else
   return readTagged(tag, data, sizeof(DWORD));
#endif
}

//==============================================================================
// readTaggedInt64
//==============================================================================
long BChunkReader::readTaggedInt64(BChunkTag tag, __int64* data)
{
#ifdef XBOX
   if(readTagged(tag, data, sizeof(__int64))==0)
      return 0;
   EndianSwitchQWords((unsigned __int64*)data, 1);
   return 1;
#else
   return readTagged(tag, data, sizeof(__int64));
#endif
}

//==============================================================================
// readTaggedFloat
//==============================================================================
long BChunkReader::readTaggedFloat(BChunkTag tag, float* data)
{
#ifdef XBOX
   if(readTagged(tag, data, sizeof(float))==0)
      return 0;
   EndianSwitchDWords((DWORD*)data, 1);
   return 1;
#else
   return readTagged(tag, data, sizeof(float));
#endif
}

//==============================================================================
// readTaggedDouble
//==============================================================================
long BChunkReader::readTaggedDouble(BChunkTag tag, double* data)
{
#ifdef XBOX
   if(readTagged(tag, data, sizeof(double))==0)
      return 0;
   EndianSwitchQWords((unsigned __int64*)data, 1);
   return 1;
#else
   return readTagged(tag, data, sizeof(double));
#endif
}

//==============================================================================
// readTaggedChar
//==============================================================================
long BChunkReader::readTaggedChar(BChunkTag tag, char* data)
{
   return readTagged(tag, data, sizeof(char));
}

//==============================================================================
// readTaggedLong
//==============================================================================
long BChunkReader::readTaggedLong(BChunkTag tag, long* data)
{
#ifdef XBOX
   if(readTagged(tag, data, sizeof(long))==0)
      return 0;
   EndianSwitchDWords((DWORD*)data, 1);
   return 1;
#else
   return readTagged(tag, data, sizeof(long));
#endif
}

//==============================================================================
// readTaggedShort
//==============================================================================
long BChunkReader::readTaggedShort(BChunkTag tag, short* data)
{
#ifdef XBOX
   if(readTagged(tag, data, sizeof(short))==0)
      return 0;
   EndianSwitchWords((WORD*)data, 1);
   return 1;
#else
   return readTagged(tag, data, sizeof(short));
#endif
}

//==============================================================================
// readTaggedBool
//==============================================================================
long BChunkReader::readTaggedBool(BChunkTag tag, bool* data)
{
   return readTagged(tag, data, sizeof(bool));
}

//==============================================================================
// readTaggedVector
//==============================================================================
long BChunkReader::readTaggedVector(BChunkTag tag, BVector* data)
{
#ifdef XBOX
   // For compatibility between xbox and pc scenarios, always read in 3 floats,
   // even though the xbox vector contains 4. 
   if(readTagged(tag, data, sizeof(float)*3)==0)
      return 0;
   EndianSwitchDWords((DWORD*)data, 3);
   data->w=0.0f;
   return 1;
#else
   return readTagged(tag, data, sizeof(BVector));
#endif
}

//==============================================================================
// readBYTEArray
//==============================================================================
long BChunkReader::readBYTEArray(long *count, BYTE* data, long maxCount)
{
   return readArray(data, sizeof(BYTE), count, maxCount);
}

//==============================================================================
// readSignedCharArray
//==============================================================================
long BChunkReader::readSignedCharArray(long *count, signed char* data, long maxCount)
{
   return readArray(data, sizeof(signed char), count, maxCount);
}

//==============================================================================
// readWORDArray
//==============================================================================
long BChunkReader::readWORDArray(long *count, WORD* data, long maxCount)
{
#ifdef XBOX
   if(readArray(data, sizeof(WORD), count, maxCount)==0)
      return 0;
   if(*count>0)
      EndianSwitchWords(data, *count);
   return 1;
#else
   return readArray(data, sizeof(WORD), count, maxCount);
#endif
}

//==============================================================================
// readDWORDArray
//==============================================================================
long BChunkReader::readDWORDArray(long *count, DWORD* data, long maxCount)
{
#ifdef XBOX
   if(readArray(data, sizeof(DWORD), count, maxCount)==0)
      return 0;
   if(*count>0)
      EndianSwitchDWords(data, *count);
   return 1;
#else
   return readArray(data, sizeof(DWORD), count, maxCount);
#endif
}

//==============================================================================
// readInt64Array
//==============================================================================
long BChunkReader::readInt64Array(long *count, __int64 *data, long maxCount)
{
#ifdef XBOX
   if(readArray(data, sizeof(__int64), count, maxCount)==0)
      return 0;
   if(*count>0)
      EndianSwitchQWords((unsigned __int64*)data, *count);
   return 1;
#else
   return readArray(data, sizeof(__int64), count, maxCount);
#endif
}

//==============================================================================
// readFloatArray
//==============================================================================
long BChunkReader::readFloatArray(long *count, float* data, long maxCount)
{
#ifdef XBOX
   if(readArray(data, sizeof(float), count, maxCount)==0)
      return 0;
   if(*count>0)
      EndianSwitchDWords((DWORD*)data, *count);
   return 1;
#else
   return readArray(data, sizeof(float), count, maxCount);
#endif
}

//==============================================================================
// readDoubleArray
//==============================================================================
long BChunkReader::readDoubleArray(long *count, double* data, long maxCount)
{
#ifdef XBOX
   if(readArray(data, sizeof(double), count, maxCount)==0)
      return 0;
   if(*count>0)
      EndianSwitchQWords((unsigned __int64*)data, *count);
   return 1;
#else
   return readArray(data, sizeof(double), count, maxCount);
#endif
}

//==============================================================================
// readCharArray
//==============================================================================
long BChunkReader::readCharArray(long *count, char* data, long maxCount)
{
   return readArray(data, sizeof(char), count, maxCount);
}

//==============================================================================
// readLongArray
//==============================================================================
long BChunkReader::readLongArray(long *count, long* data, long maxCount)
{
#ifdef XBOX
   if(readArray(data, sizeof(long), count, maxCount)==0)
      return 0;
   if(*count>0)
      EndianSwitchDWords((DWORD*)data, *count);
   return 1;
#else
   return readArray(data, sizeof(long), count, maxCount);
#endif
}

//==============================================================================
// readShortArray
//==============================================================================
long BChunkReader::readShortArray(long *count, short* data, long maxCount)
{
#ifdef XBOX
   if(readArray(data, sizeof(short), count, maxCount)==0)
      return 0;
   if(*count>0)
      EndianSwitchWords((WORD*)data, *count);
   return 1;
#else
   return readArray(data, sizeof(short), count, maxCount);
#endif
}

//==============================================================================
// readBoolArray
//==============================================================================
long BChunkReader::readBoolArray(long *count, bool* data, long maxCount)
{
   return readArray(data, sizeof(bool), count, maxCount);
}

//==============================================================================
// readVectorArray
//==============================================================================
long BChunkReader::readVectorArray(long *count, BVector* data, long maxCount)
{
#ifdef XBOX
   // For compatibility between xbox and pc scenarios, always read in 3 floats,
   // even though xbox vector contains 4. 
   if (readArrayHeader(data, count, maxCount) == 0)
      return(0);

   // read data
   if (!*count)
      return 1;

   BVector* ptr=data;
   for(long i=0; i<*count; i++)
   {
      if (mpStream->readBytes(ptr, sizeof(float)*3) == 0)
      {
         strictASSERT();
         return 0;
      }
      ptr->w=0.0f;
      ptr++;
   }

   if(*count>0)
      EndianSwitchDWords((DWORD*)data, (*count)*4);

   //Handle the UI callback
   handleCallback((sizeof(float)*3) * (*count));

   return 1;
#else
   return readArray(data, sizeof(BVector), count, maxCount);
#endif
}

//==============================================================================
// BChunkReader::peekArrayLength
//==============================================================================
long BChunkReader::peekArrayLength(long *count)
{
   //The first sizeof(long) of any array is the array count.
   long result=read(count, sizeof(long));
   
   if (!result)
   {
      strictASSERT();
      mdwArrayLength = (DWORD) -1;
      mdwPeekOffset = 0;
      return(0);
   }

#ifdef XBOX
   EndianSwitchDWords((DWORD*)count, 1);
#endif

   mdwArrayLength = *count;

   uint64 currentFilePosition = mpStream->curOfs();

   mdwPeekOffset = static_cast<DWORD>(currentFilePosition);

   return(1);
}

//==============================================================================
// BChunkReader::readTaggedArray
//==============================================================================
long BChunkReader::readTaggedArray(BChunkTag tag, long *count, void *data, long dwDataLen, long maxCount)
{
   long size=0;

   if (tag == mPeekTag)
      size = mPeekTagSize;
   else
      if (!readExpectedTag(tag, &size)) // this will clear mPeekTag to an invalid value
      {
         strictASSERT();
         return(0);
      }

   if (!readArray(data, dwDataLen, count, maxCount))
   {
      strictASSERT();
      return(0);
   }

   //Make sure chunk size matches total array size + space for the array count.
   if ((unsigned long)size != (sizeof(long)+*count*dwDataLen) )
   {
      strictASSERT();
      return(0);
   }

   mPeekTagSize = 0;

   return(1);

}

//==============================================================================
// readTaggedBYTEArray
//==============================================================================
long BChunkReader::readTaggedBYTEArray(BChunkTag tag, long *count, BYTE* data, long maxCount)
{
   return readTaggedArray(tag, count, data, sizeof(BYTE), maxCount);
}

//==============================================================================
// readTaggedWORDArray
//==============================================================================
long BChunkReader::readTaggedWORDArray(BChunkTag tag, long *count, WORD* data, long maxCount)
{
#ifdef XBOX
   if(readTaggedArray(tag, count, data, sizeof(WORD), maxCount)==0)
      return 0;
   if(*count>0)
      EndianSwitchWords(data, *count);
   return 1;
#else
   return readTaggedArray(tag, count, data, sizeof(WORD), maxCount);
#endif
}

//==============================================================================
// readTaggedDWORDArray
//==============================================================================
long BChunkReader::readTaggedDWORDArray(BChunkTag tag, long *count, DWORD* data, long maxCount)
{
#ifdef XBOX
   if(readTaggedArray(tag, count, data, sizeof(DWORD), maxCount)==0)
      return 0;
   if(*count>0)
      EndianSwitchDWords(data, *count);
   return 1;
#else
   return readTaggedArray(tag, count, data, sizeof(DWORD), maxCount);
#endif
}

//==============================================================================
// readTaggedInt64Array
//==============================================================================
long BChunkReader::readTaggedInt64Array(BChunkTag tag, long *count, __int64 *data, long maxCount)
{
#ifdef XBOX
   if(readTaggedArray(tag, count, data, sizeof(__int64), maxCount)==0)
      return 0;
   if(*count>0)
      EndianSwitchQWords((unsigned __int64*)data, *count);
   return 1;
#else
   return readTaggedArray(tag, count, data, sizeof(__int64), maxCount);
#endif
}

//==============================================================================
// readTaggedFloatArray
//==============================================================================
long BChunkReader::readTaggedFloatArray(BChunkTag tag, long *count, float* data, long maxCount)
{
#ifdef XBOX
   if(readTaggedArray(tag, count, data, sizeof(float), maxCount)==0)
      return 0;
   if(*count>0)
      EndianSwitchDWords((DWORD*)data, *count);
   return 1;
#else
   return readTaggedArray(tag, count, data, sizeof(float), maxCount);
#endif
}

//==============================================================================
// readTaggedDoubleArray
//==============================================================================
long BChunkReader::readTaggedDoubleArray(BChunkTag tag, long *count, double* data, long maxCount)
{
#ifdef XBOX
   if(readTaggedArray(tag, count, data, sizeof(double), maxCount)==0)
      return 0;
   if(*count>0)
      EndianSwitchQWords((unsigned __int64*)data, *count);
   return 1;
#else
   return readTaggedArray(tag, count, data, sizeof(double), maxCount);
#endif
}

//==============================================================================
// readTaggedCharArray
//==============================================================================
long BChunkReader::readTaggedCharArray(BChunkTag tag, long *count, char* data, long maxCount)
{
   return readTaggedArray(tag, count, data, sizeof(char), maxCount);
}

//==============================================================================
// readTaggedLongArray
//==============================================================================
long BChunkReader::readTaggedLongArray(BChunkTag tag, long *count, long* data, long maxCount)
{
#ifdef XBOX
   if(readTaggedArray(tag, count, data, sizeof(long), maxCount)==0)
      return 0;
   if(*count>0)
      EndianSwitchDWords((DWORD*)data, *count);
   return 1;
#else
   return readTaggedArray(tag, count, data, sizeof(long), maxCount);
#endif
}

//==============================================================================
// readTaggedShortArray
//==============================================================================
long BChunkReader::readTaggedShortArray(BChunkTag tag, long *count, short* data, long maxCount)
{
#ifdef XBOX
   if(readTaggedArray(tag, count, data, sizeof(short), maxCount)==0)
      return 0;
   if(*count>0)
      EndianSwitchWords((WORD*)data, *count);
   return 1;
#else
   return readTaggedArray(tag, count, data, sizeof(short), maxCount);
#endif
}

//==============================================================================
// readTaggedBoolArray
//==============================================================================
long BChunkReader::readTaggedBoolArray(BChunkTag tag, long *count, bool* data, long maxCount)
{
   return readTaggedArray(tag, count, data, sizeof(bool), maxCount);
}

//==============================================================================
// readTaggedVectorArray
//==============================================================================
long BChunkReader::readTaggedVectorArray(BChunkTag tag, long *count, BVector* data, long maxCount)
{
#ifdef XBOX
   long size=0;

   if (tag == mPeekTag)
      size = mPeekTagSize;
   else
   {
      if (!readExpectedTag(tag, &size)) // this will clear mPeekTag to an invalid value
      {
         strictASSERT();
         return(0);
      }
   }

   if (!readVectorArray(count, data, maxCount))
   {
      strictASSERT();
      return(0);
   }

   //Make sure chunk size matches total array size + space for the array count.
   if ((unsigned long)size != (sizeof(long)+*count*sizeof(float)*3))
   {
      strictASSERT();
      return(0);
   }

   mPeekTagSize = 0;

   return 1;
#else
   return readTaggedArray(tag, count, data, sizeof(BVector), maxCount);
#endif
}

//==============================================================================
// BChunkReader::peekTaggedArrayLength
//==============================================================================
long BChunkReader::peekTaggedArrayLength(BChunkTag tag, long *count)
{
   long size=0;
   long result=readExpectedTag(tag, &size);
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   //The first sizeof(long) of any array is the array count.
   result=read(count, sizeof(long));
   if (!result)
   {
      strictASSERT();
      return(0);
   }

#ifdef XBOX
   EndianSwitchDWords((DWORD*)count, 1);
#endif

   mdwArrayLength = *count;

   mPeekTag = tag;
   mPeekTagSize = size;
   
   uint64 currentFilePosition = mpStream->curOfs();
   
   mdwPeekOffset = static_cast<DWORD>(currentFilePosition);

   return(1);
}

//==============================================================================
// BChunkReader::readTag
//==============================================================================
long BChunkReader::readTag(BChunkTag *tag, long *chunkSize)
{
   //Make sure our file pointer is valid.
   if (mpStream == NULL)
   {
      strictASSERT();
      return(0);
   }

   //Read the tag.
   if (mpStream->readBytes(tag, sizeof(BChunkTag)) == 0)
   {
      strictASSERT();
      return(0);
   }

#ifdef XBOX
   EndianSwitchWords((WORD*)tag, 1);
#endif

   //Read the size.
   if (mpStream->readBytes(chunkSize, sizeof(long)) == 0)
   {
      strictASSERT();
      return(0);
   }

#ifdef XBOX
   EndianSwitchDWords((DWORD*)chunkSize, 1);
#endif

   //Handle the UI callback
   // jce 8/9/2001 -- skip this since it's only two bytes
   //handleCallback(2);

   return(1);
} //readTag

//==============================================================================
// BChunkReader::readTag
//==============================================================================
long BChunkReader::readTag(BChunkTag *tag, long *size, long &chunkStartPosition)
{
   //Make sure our file pointer is valid (doesn't really have to be done since
   //the first readTag will return failure, but what the hell).
   if (mpStream == NULL)
   {
      strictASSERT();
      return(0);
   }

   if (!readTag(tag, size))
   {
      strictASSERT();
      return(0);
   }

   uint64 currentFilePosition = mpStream->curOfs();

   chunkStartPosition = static_cast<long>(currentFilePosition);

   return(1);
}

//==============================================================================
// BChunkReader::getPosition
//==============================================================================
long BChunkReader::getPosition() const
{
   if (mpStream == NULL)
   {
      strictASSERT();
      return(0);
   }

   return static_cast<long>(mpStream->curOfs());
}

//==============================================================================
// BChunkReader::readExpectedTag
//==============================================================================
long BChunkReader::readExpectedTag(BChunkTag tag, long *size)
{
   BChunkTag tempTag=BCHUNKTAG("\0\0");
   long result=readTag(&tempTag, size);
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   if (tempTag != tag)
   {
      strictASSERT();
      return(0);
   }

   mPeekTag = (WORD) -1;

   return(1);
}

//==============================================================================
// BChunkReader::skip
//==============================================================================
long BChunkReader::skip(const long positionToSkipFrom, const long skipAmount)
{
   //NOTE: You should know what you're doing when you call this.

   //Make sure our file pointer is valid.
   if (mpStream == NULL)
   {
      strictASSERT();
      return(0);
   }

   //Not really any checks to do here...
   int64 finalPosition = static_cast<int64>(positionToSkipFrom + skipAmount);
   if (mpStream->seek(finalPosition) != finalPosition)
   {
      strictASSERT();
      return(0);
   }

   return(1);
}

//==============================================================================
// BChunkReader::seek
//==============================================================================
long BChunkReader::seek(const long seekPosition)
{
   //NOTE: You should know what you're doing when you call this.

   //Make sure our file pointer is valid.
   if (mpStream == NULL)
   {
      strictASSERT();
      return(0);
   }

   //Not really any checks to do here...
   if (mpStream->seek(static_cast<int64>(seekPosition)) != static_cast<int64>(seekPosition))
   {
      strictASSERT();
      return(0);
   }

   return(1);
}

//==============================================================================
// openFile
//==============================================================================
long BChunkReader::openFile(long dirID, const char* pFilename)
{
   //Make sure we don't already have a file open.
   if (mpStream != NULL)
   {
      BFAIL("mpStream is NOT NULL.");
      return(0);
   }

   BFileSystemStream* pStream = new BFileSystemStream();

   if (!pStream->open(dirID, pFilename, cSFReadable))
   {
      delete pStream;
      mpStream = NULL;
      return 0;
   }

   mFileSize = static_cast<long>(pStream->size());

   // setup our callback size for this file
   mSizePerCallback = mFileSize / cCallbackMeter;

   mpStream = pStream;

   return(1);
}

//==============================================================================
// closeFile
//==============================================================================
long BChunkReader::closeFile()
{
   //Make sure we have a file open.
   if (mpStream == NULL)
   {
      BASSERT(0);
      return(0);
   }

   delete mpStream;
   mpStream = NULL;
   mFileSize=0;
   return(1);
}

//==============================================================================
// read
//==============================================================================
long BChunkReader::read(void *data, const long datalen)
{
   //Make sure the file we have is valid.
   if (mpStream == NULL)
   {
      strictASSERT();
      return(0);
   }
   bool success = (mpStream->readBytes(data, datalen) == static_cast<uint>(datalen));

   //Handle the UI callback
   handleCallback(datalen);

   if (success == false)
   {
      strictASSERT();
      return(0);
   }
   return(1);
}

//==============================================================================
// readArray
//==============================================================================
long BChunkReader::readArray(void *data, long datalen, long *count, long maxCount)
{
   if (readArrayHeader(data, count, maxCount) == 0)
      return(0);

   // read data

   if (!*count)
      return 1;
   if (mpStream->readBytes(data, datalen * *count) == 0)
   {
      strictASSERT();
      return 0;
   }

   //Handle the UI callback
   handleCallback(datalen * (*count));

   return 1;
}

//==============================================================================
// readArrayHeader
//==============================================================================
long BChunkReader::readArrayHeader(const void *data, long *count, long maxCount)
{
   //Make sure the file we have is valid.
   if (mpStream == NULL)
   {
      strictASSERT();
      return(0);
   }

   if (!data && maxCount)
   {
      strictASSERT();
      return(0);
   }

   uint64 currentFilePosition = mpStream->curOfs();

   DWORD dwFileOffset = static_cast<DWORD>(currentFilePosition);

   // if these offsets don't match, then we haven't read the count, yet

   if (dwFileOffset == mdwPeekOffset)
   {
      *count = mdwArrayLength;
      mdwPeekOffset = (DWORD) -1;
   }
   else
   {
      //The first sizeof(long) of any array is the array count.
      long result=read(count, sizeof(long));
#ifdef XBOX
      if(result)
         EndianSwitchDWords((DWORD*)count, 1);
#endif
      if (!result || (*count > maxCount))
      {
         strictASSERT();
         return(0);
      }
   }

   if (*count > maxCount)
   {
      strictASSERT();
      return(0);
   }

   return 1;
}

//==============================================================================
// BChunkReader::handleCallback
//==============================================================================
void BChunkReader::handleCallback(long count)
{
   // Bail if no callback set.
   if(!mUIUpdateCallback)
      return;

   // Inc by count.
   mCurrentCallbackPos+=count;

   // Call back?
   if(mCurrentCallbackPos>=mSizePerCallback)
   {
      mUIUpdateCallback(mUIMaxPercent*getFilePercentage());
      mCurrentCallbackPos -= mSizePerCallback;
   }
}

//==============================================================================
// eof: ChunkReader.cpp
//==============================================================================

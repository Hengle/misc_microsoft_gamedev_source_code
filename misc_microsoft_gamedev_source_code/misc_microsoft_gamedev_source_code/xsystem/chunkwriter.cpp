//==============================================================================
// chunkwriter.cpp
//
// Copyright (c) 1999-2007, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xsystem.h"
#include "chunkwriter.h"
#include "config.h"
#include "econfigenum.h"
#include "logmanager.h"

#include "bfileStream.h"

//==============================================================================
// 
//==============================================================================
BChunkWriter* getChunkWriter(BStream* pStream, bool strict)
{
   return new BChunkWriter(pStream, strict);
}

//==============================================================================
// BChunkWriter::BChunkWriter
//==============================================================================
BChunkWriter::BChunkWriter() : 
   mpStream(NULL),
   mManageFile(false),
   mStrict(false),
   //mPositionStack doesn't need any ctor args.
   //mTagStack doesn't need any ctor args.
   mLog(0),
   mNoSkipTagMode(false)
{
#ifdef _BANG
#ifndef BUILD_FINAL
   gConfig.get(cConfigLogChunker, &mLog);
#endif
#endif
}

//==============================================================================
// BChunkWriter::BChunkWriter
//==============================================================================
BChunkWriter::BChunkWriter(BStream* pStream, bool strict) : 
   mpStream(pStream),
   mManageFile(false),
   mStrict(strict),
   //mPositionStack doesn't need any ctor args.
   //mTagStack doesn't need any ctor args.
   mLog(0),
   mNoSkipTagMode(false)
{
#ifdef _BANG
#ifndef BUILD_FINAL
   gConfig.get(cConfigLogChunker, &mLog);
#endif
#endif
}

//==============================================================================
// BChunkWriter::BChunkWriter
//==============================================================================
BChunkWriter::BChunkWriter(long dirID, const char* pFilename, bool strict) : 
   mpStream(NULL),
   mManageFile(false),
   mStrict(strict),
   //mPositionStack doesn't need any ctor args.
   //mTagStack doesn't need any ctor args.
   mLog(0),
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
// BChunkWriter::~BChunkWriter
//==============================================================================
BChunkWriter::~BChunkWriter()
{
   if (mManageFile)
      if (mpStream)
         closeFile();

} //~BChunkWriter

//==============================================================================
// 
//==============================================================================
DWORD BChunkWriter::getFilePosition() const
{   
   if (!mpStream)
   {
      strictASSERT();
      return(0);
   }
   //Get the current file position.   
   uint64 filePos = mpStream->curOfs();
   return static_cast<DWORD>(filePos);
}

//==============================================================================
// BChunkWriter::skipAhead
//==============================================================================
long BChunkWriter::skipAhead(long& position, long dataSize)
{
   //Get the current file position.
   uint64 currentFilePosition = mpStream->curOfs();

   //Return the position.
   position = static_cast<long>(currentFilePosition);

   int64 pos = static_cast<int64>(position + dataSize);

   //Move forward the size of a chunk to leave space for the size writeback.
   if (mpStream->seek(pos) != pos)
   {
      strictASSERT();
      return(0);
   }

   return(1);
}

//==============================================================================
// BChunkWriter::skipAhead
//==============================================================================
long BChunkWriter::skipAheadArray(long &position, long dataSize)
{
   return skipAhead(position, dataSize+sizeof(long)); // add in the count position   
}

//==============================================================================
// BChunkWriter::writeLongData
//==============================================================================
long BChunkWriter::writeLongData(long position, long data)
{
   //Get the current file position.
   int64 currentFilePosition = static_cast<int64>(mpStream->curOfs());

   int64 pos = static_cast<int64>(position);

   //Seek to the given position
   if (mpStream->seek(pos) != pos)
   {
      strictASSERT();
      return(0);
   }

   //Write the data.
#ifdef XBOX
   EndianSwitchDWords((DWORD*)&data, 1);
#endif
   DWORD num = mpStream->writeBytes((void*)&data, sizeof(long));
   if (!num)
   {
      strictASSERT();
      return(0);
   }

   //Seek back to the original position.
   if (mpStream->seek(currentFilePosition) != currentFilePosition)
   {
      strictASSERT();
      return(0);
   }

   return(1);
}

//==============================================================================
// BChunkWriter::writeDWORDData
//==============================================================================
long BChunkWriter::writeDWORDData(long position, DWORD data)
{
   //Get the current file position.
   int64 currentFilePosition = static_cast<int64>(mpStream->curOfs());

   int64 pos = static_cast<int64>(position);

   //Seek to the given position
   if (mpStream->seek(pos) != pos)
   {
      strictASSERT();
      return(0);
   }

   //Write the data.
#ifdef XBOX
   EndianSwitchDWords((DWORD*)&data, 1);
#endif
   DWORD num = mpStream->writeBytes((void*)&data, sizeof(DWORD));
   if (!num)
   {
      strictASSERT();
      return(0);
   }

   //Seek back to the original position.
   if (mpStream->seek(currentFilePosition) != currentFilePosition)
   {
      strictASSERT();
      return(0);
   }

   return(1);
}

//==============================================================================
// BChunkWriter::writeBYTEArrayData
//==============================================================================
long BChunkWriter::writeBYTEArrayData(long position, long count, const BYTE *data)
{
   //Get the current file position.
   int64 currentFilePosition = static_cast<int64>(mpStream->curOfs());

   int64 pos = static_cast<int64>(position);

   //Seek to the given position
   if (mpStream->seek(pos) != pos)
   {
      strictASSERT();
      return(0);
   }

   //Write the size
#ifdef XBOX
   long switched=count;
   EndianSwitchDWords((DWORD*)&switched, 1);
   DWORD num = mpStream->writeBytes((void*)&switched, sizeof(long));
#else
   DWORD num = mpStream->writeBytes((void*)&count, sizeof(long));
#endif
   if (!num)
   {
      strictASSERT();
      return(0);
   }
   //Write the data.
   num = mpStream->writeBytes((void*)data, sizeof(BYTE) * count);
   if (!num)
   {
      strictASSERT();
      return(0);
   }

   //Seek back to the original position.
   if (mpStream->seek(currentFilePosition) != currentFilePosition)
   {
      strictASSERT();
      return(0);
   }

   return(1);
}

//==============================================================================
// BChunkWriter::writeTag
//==============================================================================
long BChunkWriter::writeTag(BChunkTag tag, long chunkSize)
{
   //Make sure the file we have is valid.
   if (mpStream == NULL)
   {
      strictASSERT();
      return(0);
   }

   //Write the tag
#ifdef XBOX
   BChunkTag switchedTag=tag;
   EndianSwitchWords((WORD*)&switchedTag, 1);
   DWORD result = mpStream->writeBytes((void*)&switchedTag, sizeof(switchedTag));
#else
   DWORD result = mpStream->writeBytes((void*)&tag, sizeof(tag));
#endif
   if (!result)
   {
      strictASSERT();
      return(0);
   }
   //Write the size
#ifdef XBOX
   long switchedSize = chunkSize;
   EndianSwitchDWords((DWORD*)&switchedSize, 1);
   result = mpStream->writeBytes((void*)&switchedSize, sizeof(switchedSize));
#else
   result = mpStream->writeBytes((void*)&chunkSize, sizeof(chunkSize));
#endif
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   //Debug.
#ifdef _BANG
   if (mLog)
   {
      char t1=(char)(tag&0x00FF);
      char t2=(char)(tag>>8);
      blog("WT: Tag '%c%c' written with a size of %d.", t1, t2, chunkSize);
   }
#endif

   return(1);
}

//==============================================================================
// BChunkWriter::writeTagPostSized
//==============================================================================
long BChunkWriter::writeTagPostSized(BChunkTag tag, long& position)
{
   //Make sure the file we have is valid.
   if (mpStream == NULL)
   {
      strictASSERT();
      return(0);
   }

   //Write the tag.
#ifdef XBOX
   BChunkTag switchedTag=tag;
   EndianSwitchWords((WORD*)&switchedTag, 1);
   DWORD result = mpStream->writeBytes((void*)&switchedTag, sizeof(switchedTag));
#else
   DWORD result = mpStream->writeBytes((void*)&tag, sizeof(tag));
#endif
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   //Get the file position.
   uint64 filePosition = mpStream->curOfs();

   //Put the file position on the stack.  When we pop this off, we will be
   //able to do the size writeback at the proper spot.
   long stackSize = mPositionStack.getNumber();
   if ((mPositionStack.setNumber(stackSize+1) == false) || (mTagStack.setNumber(stackSize+1) == false))
   {
      strictASSERT();
      return(0);
   }
   mPositionStack[stackSize] = static_cast<long>(filePosition);
   mTagStack[stackSize] = tag;

   //Debug.
#ifdef _BANG
   if (mLog)
   {
      char t1=(char)(tag&0x00FF);
      char t2=(char)(tag>>8);
      blog("WTPS: Tag '%c%c' started at position %d.", t1, t2, filePosition);
   }
#endif

   //Return the position.
   position = static_cast<long>(filePosition);

   long space=0;
   result = mpStream->writeBytes((void*)&space, sizeof(space));

   if (!result)
   {
      strictASSERT();
      return 0;
   }

   return(1);
}

//==============================================================================
// BChunkWriter::writeSize
//==============================================================================
long BChunkWriter::writeSize(long position)
{
   //Make sure the file we have is valid.
   if (mpStream == NULL)
   {
      strictASSERT();
      return(0);
   }

   //Take the position off of the top of the stack.
   long stackSize = mPositionStack.getNumber();
   long startTagPosition = mPositionStack[stackSize-1];

   //Make sure this position matches the position passed in (which was passed out of the
   //chunker when we wrote the tag).
   if (startTagPosition != position)
   {
      //Debug.
#ifdef _BANG
      if (mLog)
      {
         long tag=mTagStack[stackSize-1];
         char t1=(char)(tag&0x00FF);
         char t2=(char)(tag>>8);
         {setBlogError(4020); blogerror("  WriteSize:: FAILURE, Tag of '%c%c' was started at %d, but handle is %d.", t1, t2, startTagPosition, position);}
      }
#endif
      strictASSERT();
      return(0);
   }

   //Get the current file position.

   int64 endTagPosition = static_cast<int64>(mpStream->curOfs());

   if (mNoSkipTagMode)
   {
      //Calc and write the size of the chunk.
      long chunkSize = static_cast<long>(endTagPosition) - startTagPosition;
      long result = writeDWORD(chunkSize);
      if (!result)
      {
         strictASSERT();
         return(0);
      }
   }
   else
   {
      //Seek to the start position for this tag.
      bool rc = (mpStream->seek(static_cast<int64>(position)) == static_cast<int64>(position));
      if (!rc)
      {
         strictASSERT();
         return(0);
      }

      //Calc and write the size of the chunk.  We make sure to account for the size of the
      //size of the chunk in this calc.
      long chunkSize = static_cast<long>(endTagPosition) - startTagPosition - sizeof(long);
      long result = writeDWORD(chunkSize);
      if (!result)
      {
         strictASSERT();
         return(0);
      }

      //Seek back to the end of the tag.
      rc = (mpStream->seek(endTagPosition) == endTagPosition);
      if (!rc)
      {
         strictASSERT();
         return(0);
      }
   }

   //Debug.
#ifdef _BANG
   if (mLog)
   {
      long tag=mTagStack[stackSize-1];
      char t1=(char)(tag&0x00FF);
      char t2=(char)(tag>>8);
      blog("  WriteSize:: Tag '%c%c' written with a size of %d.", t1, t2, chunkSize);
   }
#endif

   //If it is equal, reduce the size of the stack.
   mPositionStack.setNumber(stackSize-1);
   mTagStack.setNumber(stackSize-1);

   return(1);
}

//==============================================================================
// writeBYTE
//==============================================================================
long BChunkWriter::writeBYTE(const BYTE data)
{
   return write((void*)&data, sizeof(data));
} //writeBYTE

//==============================================================================
// writeSignedChar
//==============================================================================
long BChunkWriter::writeSignedChar(const signed char data)
{
   return write((void*)&data, sizeof(data));
} //writeBYTE

//==============================================================================
// writeWORD
//==============================================================================
long BChunkWriter::writeWORD(const WORD data)
{
#ifdef XBOX
   EndianSwitchWords((WORD*)&data, 1);
#endif
   return write((void*)&data, sizeof(data));

} //writeWORD

//==============================================================================
// writeDWORD
//==============================================================================
long BChunkWriter::writeDWORD(const DWORD data)
{
#ifdef XBOX
   EndianSwitchDWords((DWORD*)&data, 1);
#endif
   return write((void*)&data, sizeof(data));
} //writeDWORD

//==============================================================================
// writeInt64
//==============================================================================
long BChunkWriter::writeInt64(const __int64 data)
{
#ifdef XBOX
   EndianSwitchQWords((unsigned __int64*)&data, 1);
#endif
   return write((void*)&data, sizeof(data));
} //writeInt64

//==============================================================================
// writeFloat
//==============================================================================
long BChunkWriter::writeFloat(const float data)
{
#ifdef XBOX
   EndianSwitchDWords((DWORD*)&data, 1);
#endif
   return write((void*)&data, sizeof(data));
} //writeFloat

//==============================================================================
// writeDouble
//==============================================================================
long BChunkWriter::writeDouble(const double data)
{
#ifdef XBOX
   EndianSwitchQWords((unsigned __int64*)&data, 1);
#endif
   return write((void*)&data, sizeof(data));
} //writeDouble

//==============================================================================
// writeChar
//==============================================================================
long BChunkWriter::writeChar(const char data)
{
   return write((void*)&data, sizeof(data));
} //writeChar

//==============================================================================
// writeLong
//==============================================================================
long BChunkWriter::writeLong(long data)
{
#ifdef XBOX
   EndianSwitchDWords((DWORD*)&data, 1);
#endif
   return write((void*)&data, sizeof(data));
} //writeLong

//==============================================================================
// writeShort
//==============================================================================
long BChunkWriter::writeShort(const short data)
{
#ifdef XBOX
   EndianSwitchWords((WORD*)&data, 1);
#endif
   return write((void*)&data, sizeof(data));
} //writeShort

//==============================================================================
// writeBool
//==============================================================================
long BChunkWriter::writeBool(const bool data)
{
   return write((void*)&data, sizeof(data));
} //writeBool

//==============================================================================
// writeVector
//==============================================================================
long BChunkWriter::writeVector(const BVector &data)
{
#ifdef XBOX
   BVector switched(data);
   EndianSwitchDWords((DWORD*)&switched, 3);
   // Only write out 3 floats to keep compatibility of loading files between xbox and PC
   return write((void*)&switched, sizeof(float)*3);
#else
   return write((void*)&data, sizeof(data));
#endif
}

//==============================================================================
// writeMatrix
//==============================================================================
long BChunkWriter::writeMatrix(const BMatrix &data)
{
#ifdef XBOX
   BMatrix switched(data);
   EndianSwitchDWords((DWORD*)&switched, 12);
   return write((void*)&switched, sizeof(switched));
#else
   return write((void*)&data, sizeof(data));
#endif
}


//==============================================================================
// BChunkWriter::writeBString
//==============================================================================
bool BChunkWriter::writeBString(const BString& string)
{
   DWORD length = string.length();

   DWORD outLength = length;

#ifndef UNICODE
   outLength |= 0x80000000;
#endif

#ifdef XBOX
   EndianSwitchDWords((DWORD*)&outLength, 1);
#endif

   if (mpStream->writeBytes(&outLength, sizeof(outLength)) != sizeof(outLength))
      return(false);

   if (length == 0)
      return(true);

#if defined(XBOX) && defined(UNICODE)
   BString outString(string.getPtr());
   outString.endianSwap();
   if (mpStream->writeBytes(outString.getPtr(), length*sizeof(BCHAR_T)) != length*sizeof(BCHAR_T))
      return(false);
#else
   if (mpStream->writeBytes(string.getPtr(), length*sizeof(BCHAR_T)) != length*sizeof(BCHAR_T))
      return(false);
#endif

   return true;
}

//==============================================================================
// BChunkWriter::writeBSimString
//==============================================================================
bool BChunkWriter::writeBSimString(const BSimString& string)
{
   DWORD length = string.length();

   DWORD outLength = length;

#ifndef UNICODE
   outLength |= 0x80000000;
#endif

#ifdef XBOX
   EndianSwitchDWords((DWORD*)&outLength, 1);
#endif

   if (mpStream->writeBytes(&outLength, sizeof(outLength)) != sizeof(outLength))
      return(false);

   if (length == 0)
      return(true);

#if defined(XBOX) && defined(UNICODE)
   BString outString(string.getPtr());
   outString.endianSwap();
   if (mpStream->writeBytes(outString.getPtr(), length*sizeof(BCHAR_T)) != length*sizeof(BCHAR_T))
      return(false);
#else
   if (mpStream->writeBytes(string.getPtr(), length*sizeof(BCHAR_T)) != length*sizeof(BCHAR_T))
      return(false);
#endif

   return true;
}

//==============================================================================
// writeTaggedBYTE
//==============================================================================
long BChunkWriter::writeTaggedBYTE(BChunkTag tag, const BYTE data)
{
   long result=writeTag(tag, (long)sizeof(data));
   if (!result)
   {
      strictASSERT();
      return(0);
   }
   result=writeBYTE(data);
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   return(1);
}

//==============================================================================
// writeTaggedWORD
//==============================================================================
long BChunkWriter::writeTaggedWORD(BChunkTag tag, const WORD data)
{
   long result=writeTag(tag, (long)sizeof(data));
   if (!result)
   {
      strictASSERT();
      return(0);
   }
   result=writeWORD(data);
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   return(1);
}

//==============================================================================
// writeTaggedDWORD
//==============================================================================
long BChunkWriter::writeTaggedDWORD(BChunkTag tag, const DWORD data)
{
   long result=writeTag(tag, (long)sizeof(data));
   if (!result)
   {
      strictASSERT();
      return(0);
   }
   result=writeDWORD(data);
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   return(1);
}


//==============================================================================
// writeTaggedInt64
//==============================================================================
long BChunkWriter::writeTaggedInt64(BChunkTag tag, const __int64 data)
{
   long result=writeTag(tag, (long)sizeof(data));
   if (!result)
   {
      strictASSERT();
      return(0);
   }
   result=writeInt64(data);
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   return(1);
}

//==============================================================================
// writeTaggedFloat
//==============================================================================
long BChunkWriter::writeTaggedFloat(BChunkTag tag, const float data)
{
   long result=writeTag(tag, (long)sizeof(data));
   if (!result)
   {
      strictASSERT();
      return(0);
   }
   result=writeFloat(data);
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   return(1);
}

//==============================================================================
// writeTaggedDouble
//==============================================================================
long BChunkWriter::writeTaggedDouble(BChunkTag tag, const double data)
{
   long result=writeTag(tag, (long)sizeof(data));
   if (!result)
   {
      strictASSERT();
      return(0);
   }
   result=writeDouble(data);
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   return(1);
}

//==============================================================================
// writeTaggedChar
//==============================================================================
long BChunkWriter::writeTaggedChar(BChunkTag tag, const char data)
{
   long result=writeTag(tag, (long)sizeof(data));
   if (!result)
   {
      strictASSERT();
      return(0);
   }
   result=writeChar(data);
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   return(1);
}

//==============================================================================
// writeTaggedLong
//==============================================================================
long BChunkWriter::writeTaggedLong(BChunkTag tag, long data)
{
   long result=writeTag(tag, (long)sizeof(data));
   if (!result)
   {
      strictASSERT();
      return(0);
   }
   result=writeLong(data);
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   return(1);
}

//==============================================================================
// writeTaggedShort
//==============================================================================
long BChunkWriter::writeTaggedShort(BChunkTag tag, const short data)
{
   long result=writeTag(tag, (long)sizeof(data));
   if (!result)
   {
      strictASSERT();
      return(0);
   }
   result=writeShort(data);
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   return(1);
}

//==============================================================================
// writeTaggedBool
//==============================================================================
long BChunkWriter::writeTaggedBool(BChunkTag tag, const bool data)
{
   long result=writeTag(tag, (long)sizeof(data));
   if (!result)
   {
      strictASSERT();
      return(0);
   }
   result=writeBool(data);
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   return(1);
}

//==============================================================================
// writeTaggedVector
//==============================================================================
long BChunkWriter::writeTaggedVector(BChunkTag tag, const BVector &data)
{
   long result=writeTag(tag, (long)sizeof(data));
   if (!result)
   {
      strictASSERT();
      return(0);
   }
   result=writeVector(data);
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   return(1);
}

//==============================================================================
// writeBYTEArray
//==============================================================================
long BChunkWriter::writeBYTEArray(long count, const BYTE* data)
{
   return writeArray((void*)data, sizeof(BYTE), count);
} //writeBYTEArray

//==============================================================================
// writeSignedCharArray
//==============================================================================
long BChunkWriter::writeSignedCharArray(long count, const signed char *data)
{
   return writeArray((void*)data, sizeof(signed char), count);
} //writeBYTEArray

//==============================================================================
// writeWORDArray
//==============================================================================
long BChunkWriter::writeWORDArray(long count, const WORD* data)
{
#ifdef XBOX
   if(writeLong(count)==0)
      return(0);
   if (count < 1)
      return(1);
   for(long i=0; i<count; i++)
   {
      if(writeWORD(data[i])==0)
         return(0);
   }
   return(1);
#else
   return writeArray((void*)data, sizeof(WORD), count);
#endif
} //writeWORDArray

//==============================================================================
// writeDWORDArray
//==============================================================================
long BChunkWriter::writeDWORDArray(long count, const DWORD* data)
{
#ifdef XBOX
   if(writeLong(count)==0)
      return(0);
   if (count < 1)
      return(1);
   for(long i=0; i<count; i++)
   {
      if(writeDWORD(data[i])==0)
         return(0);
   }
   return(1);
#else
   return writeArray((void*)data, sizeof(DWORD), count);
#endif
} //writeDWORDArray

//==============================================================================
// writeInt64Array
//==============================================================================
long BChunkWriter::writeInt64Array(long count, const __int64* data)
{
#ifdef XBOX
   if(writeLong(count)==0)
      return(0);
   if (count < 1)
      return(1);
   for(long i=0; i<count; i++)
   {
      if(writeInt64(data[i])==0)
         return(0);
   }
   return(1);
#else
   return writeArray((void*)data, sizeof(__int64), count);
#endif
} //writeInt64Array

//==============================================================================
// writeFloatArray
//==============================================================================
long BChunkWriter::writeFloatArray(long count, const float* data)
{
#ifdef XBOX
   if(writeLong(count)==0)
      return(0);
   if (count < 1)
      return(1);
   for(long i=0; i<count; i++)
   {
      if(writeFloat(data[i])==0)
         return(0);
   }
   return(1);
#else
   return writeArray((void*)data, sizeof(float), count);
#endif
} //writeFloatArray

//==============================================================================
// writeDoubleArray
//==============================================================================
long BChunkWriter::writeDoubleArray(long count, const double* data)
{
#ifdef XBOX
   if(writeLong(count)==0)
      return(0);
   if (count < 1)
      return(1);
   for(long i=0; i<count; i++)
   {
      if(writeDouble(data[i])==0)
         return(0);
   }
   return(1);
#else
   return writeArray((void*)data, sizeof(double), count);
#endif
} //writeDoubleArray

//==============================================================================
// writeCharArray
//==============================================================================
long BChunkWriter::writeCharArray(long count, const char* data)
{
   return writeArray((void*)data, sizeof(*data), count);
} //writeCharArray

//==============================================================================
// writeLongArray
//==============================================================================
long BChunkWriter::writeLongArray(long count, const long* data)
{
#ifdef XBOX
   if(writeLong(count)==0)
      return(0);
   if (count < 1)
      return(1);
   for(long i=0; i<count; i++)
   {
      if(writeLong(data[i])==0)
         return(0);
   }
   return(1);
#else
   return writeArray((void*)data, sizeof(*data), count);
#endif
} //writeLongArray

//==============================================================================
// writeShortArray
//==============================================================================
long BChunkWriter::writeShortArray(long count, const short* data)
{
#ifdef XBOX
   if(writeLong(count)==0)
      return(0);
   if (count < 1)
      return(1);
   for(long i=0; i<count; i++)
   {
      if(writeShort(data[i])==0)
         return(0);
   }
   return(1);
#else
   return writeArray((void*)data, sizeof(*data), count);
#endif
} //writeShortArray

//==============================================================================
// writeBoolArray
//==============================================================================
long BChunkWriter::writeBoolArray(long count, const bool* data)
{
   return writeArray((void*)data, sizeof(*data), count);
} //writeBoolArray

//==============================================================================
// writeVectorArray
//==============================================================================
long BChunkWriter::writeVectorArray(long count, const BVector* data)
{
#ifdef XBOX
   if(writeLong(count)==0)
      return(0);
   if (count < 1)
      return(1);
   for(long i=0; i<count; i++)
   {
      if(writeVector(data[i])==0)
         return(0);
   }
   return(1);
#else
   return writeArray((void*)data, sizeof(*data), count);
#endif
} //writeBoolArray

//==============================================================================
// writeTaggedBYTEArray
//==============================================================================
long BChunkWriter::writeTaggedBYTEArray(BChunkTag tag,
   long count, const BYTE* data)
{
   //total size=size of array count + size of array
   long result=writeTag(tag, (long)(sizeof(count) + sizeof(BYTE) * count));
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   return writeBYTEArray(count, data);

} //writeTaggedBYTEArray

//==============================================================================
// writeTaggedWORDArray
//==============================================================================
long BChunkWriter::writeTaggedWORDArray(BChunkTag tag,
   long count, const WORD* data)
{
   long result=writeTag(tag, (long)(sizeof(count) + sizeof(WORD) * count));
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   return writeWORDArray(count, data);

} //writeTaggedWORDArray

//==============================================================================
// writeTaggedDWORDArray
//==============================================================================
long BChunkWriter::writeTaggedDWORDArray(BChunkTag tag,
   long count, const DWORD* data)
{
   long result=writeTag(tag, (long)(sizeof(count) + sizeof(DWORD) * count));
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   return writeDWORDArray(count, data);

}

//==============================================================================
// writeTaggedInt64Array
//==============================================================================
long BChunkWriter::writeTaggedInt64Array(BChunkTag tag,
   long count, const __int64* data)
{
   long result=writeTag(tag, (long)(sizeof(count) + sizeof(__int64) * count));
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   return writeInt64Array(count, data);

}

//==============================================================================
// writeTaggedFloatArray
//==============================================================================
long BChunkWriter::writeTaggedFloatArray(BChunkTag tag,
   long count, const float* data)
{
   long result=writeTag(tag, (long)(sizeof(count) + sizeof(float) * count));
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   return writeFloatArray(count, data);

}

//==============================================================================
// writeTaggedDoubleArray
//==============================================================================
long BChunkWriter::writeTaggedDoubleArray(BChunkTag tag,
   long count, const double* data)
{
   long result=writeTag(tag, (long)(sizeof(count) + sizeof(double) * count));
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   return writeDoubleArray(count, data);

}

//==============================================================================
// writeTaggedCharArray
//==============================================================================
long BChunkWriter::writeTaggedCharArray(BChunkTag tag,
   long count, const char* data)
{
   long result=writeTag(tag, (long)(sizeof(count) + sizeof(*data) * count));
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   return writeCharArray(count, data);
}

//==============================================================================
// writeTaggedLongArray
//==============================================================================
long BChunkWriter::writeTaggedLongArray(BChunkTag tag, long count, const long* data)
{
   long result=writeTag(tag, (long)(sizeof(count) + sizeof(*data) * count));
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   return writeLongArray(count, data);

}

//==============================================================================
// writeTaggedShortArray
//==============================================================================
long BChunkWriter::writeTaggedShortArray(BChunkTag tag,
   long count, const short* data)
{
   long result=writeTag(tag, (long)(sizeof(count) + sizeof(*data) * count));
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   return writeShortArray(count, data);

}

//==============================================================================
// writeTaggedBoolArray
//==============================================================================
long BChunkWriter::writeTaggedBoolArray(BChunkTag tag,
   long count, const bool* data)
{
   long result=writeTag(tag, (long)(sizeof(count) + sizeof(*data) * count));
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   return writeBoolArray(count, data);

}

//==============================================================================
// writeTaggedVectorArray
//==============================================================================
long BChunkWriter::writeTaggedVectorArray(BChunkTag tag,
   long count, const BVector* data)
{
   long result=writeTag(tag, (long)(sizeof(count) + sizeof(*data) * count));
   if (!result)
   {
      strictASSERT();
      return(0);
   }

   return writeVectorArray(count, data);

}

//==============================================================================
// BChunkWriter::openFile
//==============================================================================
long BChunkWriter::openFile(long dirID, const char* pFilename)
{
   //Make sure we don't already have a file open.
   if (mpStream != NULL)
   {
      BASSERT(0);
      return(0);
   }

   BFileSystemStream* pStream = new BFileSystemStream();

   if (!pStream->open(dirID, pFilename, cSFWritable | cSFEnableBuffering))
   {
      delete pStream;
      mpStream = NULL;
      return 0;
   }

   mpStream = pStream;

   return(1);
}

//==============================================================================
// BChunkWriter::closeFile
//==============================================================================
long BChunkWriter::closeFile()
{
   //Make sure we have a file open.
   if (mpStream == NULL)
   {
      BASSERT(0);
      return(0);
   }

   delete mpStream;
   mpStream = NULL;
   return(1);
}

//==============================================================================
// BChunkWriter::write
//==============================================================================
long BChunkWriter::write(const void *data, long datalen)
{
   //Make sure the file we have is valid.
   if (mpStream == NULL)
   {
      strictASSERT();
      return(0);
   }

   //Write the data.
   DWORD num = mpStream->writeBytes(data, datalen);
   if (!num)
   {
      strictASSERT();
      return(0);
   }

   //Debug.
   //if (mLog)
   //   blog("Write:: %d BYTEs of data.", datalen);

   return(1);
}

//==============================================================================
// BChunkWriter::writeArray
//==============================================================================
long BChunkWriter::writeArray(const void *data, long datalen, long count)
{
   //Make sure the file we have is valid.
   if (mpStream == NULL)
   {
      strictASSERT();
      return(0);
   }

   //The first sizeof(long) of any array is the array count.
   //bool num=mpStream->writeBytes((void*)&count, sizeof(count));
   long retval=writeLong(count);
   if (retval==0)
   {
      strictASSERT();
      return(0);
   }

   //Check for an invalid count.  We return success, though, so that callers
   //don't have to check before calling.
   if (count < 1)
      return(1);

   DWORD num = mpStream->writeBytes(data, datalen * count);
   if (!num)
   {
      strictASSERT();
      return(0);
   }

   //Debug.
   //if (mLog)
   //   blog("WriteArray:: %d BYTEs of data (%d length of %d count).", datalen*count, datalen, count);

   return(1);
}

//==============================================================================
// eof: ChunkWriter.cpp
//==============================================================================

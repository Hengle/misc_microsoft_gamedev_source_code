//==============================================================================
// chunkreader.h
//
// Copyright (c) 1999-2007, Ensemble Studios
//==============================================================================

#pragma once

//==============================================================================
#include "chunker.h"

//==============================================================================

//==============================================================================
// BChunkReaderEntry
//==============================================================================
class BChunkReaderEntry
{
   public:
      BChunkReaderEntry( void ) :
         mTag(BCHUNKTAG("\0\0")),
         mSize(0),
         mStartPosition(0),
         mPreviousPosition(-1) { }
      
      BChunkTag                           mTag;
      long                                mSize;
      long                                mStartPosition;
      long                                mPreviousPosition;
};
typedef BDynamicArray<BChunkReaderEntry> BSimpleChunkReaderEntryArray;


//==============================================================================
// BChunkReader
//==============================================================================
class BChunkReader // : public BChunkReader
{
   public:
      //Constructors/Destructors.
      BChunkReader();
      BChunkReader(BStream* pStream, bool strict);
      BChunkReader(long dirID, const char* pFilename, bool strict);
      ~BChunkReader();

      //File percentage.
      float          getFilePercentage( void ) const;
      long           getFileSize( void ) const { return(mFileSize); }
      long           getPosition( void ) const;

      //No skip tag mode.
      void           setNoSkipTagMode(bool val) { mNoSkipTagMode=val; }

      //UI update callback.
      void           setUpdateCallback( UIUPDATECALLBACK callback, float maxPercent ) { mUIUpdateCallback=callback; mUIMaxPercent=maxPercent;}

      //Chunk/Tag Reading/Verification.
	   long           readExpectedTag( BChunkTag tag, long atPosition = -1, long *size=0 );
	   long           readExpectedTagAndLevel( BChunkTag tag, long &stackLevel, long atPosition = -1 );
	   long           validateChunkRead( BChunkTag tag );
   

      //Simple reads.
      long           readChar(char *data);
	   long           readBYTE(BYTE *data);
	   long           readSignedChar(signed char *data);
      long           readShort(short *data);
	   long           readWORD(WORD *data);
      long           readLong(long *data);
	   long           readDWORD(DWORD *data);
      long           readInt64(__int64 *data);
	   long           readFloat(float *data);
	   long           readDouble(double *data);
      long           readBool(bool *data);
      long           readVector(BVector *data);
      long           readMatrix(BMatrix *data);

      bool           readBString(BString &string);
      bool           readBString(BString* pString);
      
      bool           readBSimString(BSimString &string);
      bool           readBSimString(BSimString* pString);
            
      //Simple, Tagged reads.
      long           readTaggedChar(BChunkTag tag, char *data);
	   long           readTaggedBYTE(BChunkTag tag, BYTE *data);

      long           readTaggedShort(BChunkTag tag, short *data);
      long           readTaggedWORD(BChunkTag tag, WORD *data);
	   
      long           readTaggedLong(BChunkTag tag, long *data);
      long           readTaggedDWORD(BChunkTag tag, DWORD *data);
      long           readTaggedInt64(BChunkTag tag, __int64 *data);
	   
      long           readTaggedFloat(BChunkTag tag, float *data);
	   long           readTaggedDouble(BChunkTag tag, double *data);

      long           readTaggedBool(BChunkTag tag, bool *data);
      long           readTaggedVector(BChunkTag tag, BVector *data);

      //Array reads.
      long           readCharArray(long *count, char *data, const long maxCount);
	   long           readBYTEArray(long *count, BYTE *data, const long maxCount);
	   long           readSignedCharArray(long *count, signed char *data, const long maxCount);

      long           readShortArray(long *count, short *data, const long maxCount);
	   long           readWORDArray(long *count, WORD *data, const long maxCount);

      long           readLongArray(long *count, long *data, const long maxCount);
      long           readDWORDArray(long *count, DWORD *data, const long maxCount);
      long           readInt64Array(long *count, __int64 *data, const long maxCount);

	   long           readFloatArray(long *count, float *data, const long maxCount);
	   long           readDoubleArray(long *count, double *data, const long maxCount);

      long           readBoolArray(long *count, bool *data, const long maxCount);
      long           readVectorArray(long *count, BVector *data, const long maxCount);

      long           peekArrayLength(long *count);

      //Tagged array reads.
      long           readTaggedCharArray(BChunkTag tag, long *count, char *data, const long maxCount);
	   long           readTaggedBYTEArray(BChunkTag tag, long *count, BYTE *data, const long maxCount);

      long           readTaggedShortArray(BChunkTag tag, long *count, short *data, const long maxCount);
	   long           readTaggedWORDArray(BChunkTag tag, long *count, WORD *data, const long maxCount);

      long           readTaggedLongArray(BChunkTag tag, long *count, long *data, const long maxCount);
	   long           readTaggedDWORDArray(BChunkTag tag, long *count, DWORD *data, const long maxCount);
      long           readTaggedInt64Array(BChunkTag tag, long *count, __int64 *data, const long maxCount);

	   long           readTaggedFloatArray(BChunkTag tag, long *count, float *data, const long maxCount);
	   long           readTaggedDoubleArray(BChunkTag tag, long *count, double *data, const long maxCount);

      long           readTaggedBoolArray(BChunkTag tag, long *count, bool *data, const long maxCount);
      long           readTaggedVectorArray(BChunkTag tag, long *count, BVector *data, const long maxCount);

      long           peekTaggedArrayLength(BChunkTag tag, long *count);           

   protected:
	   //Tags.
	   long           readTag( BChunkTag *tag, long *size );
	   long           readTag( BChunkTag *tag, long *size, long &chunkStartPosition );
	   long           readExpectedTag( BChunkTag tag, long *size );
      long           readTagged(BChunkTag tag, void *pdata, DWORD dwBytes);
      long           readTaggedArray(BChunkTag tag, long *count, void *data, long dwDataLen, long maxCount);

      //Misc Access methods.  YOU NEED TO KNOW WHAT YOU'RE DOING WHEN YOU CALL THESE.      
      long           skip( const long positionToSkipFrom, const long skipAmount );
      long           seek( const long seekPosition );

      long           openFile(long dirID, const char* pFilename);
      long           closeFile();
      void           handleCallback(long count);

      //Wrappers around fread.
      long                        read( void *data, long datalen );
      long                        readArray( void *data, long datalen, long *count, long maxCount);
      long                        readArrayHeader( const void *data, long *count, long maxCount);

      BStream*                            mpStream;
      long                                mFileSize;
      UIUPDATECALLBACK                    mUIUpdateCallback;
      BSimpleChunkReaderEntryArray        mChunks;
      long                                mLog;
      float                               mUIMaxPercent;
      bool                                mNoSkipTagMode;
      
      // UI callback metering.
      long                                mSizePerCallback;
      long                                mCurrentCallbackPos;

      char *mpStringBuffer;
      long mdwStringBuffer;

      DWORD mdwArrayLength;
      DWORD mdwPeekOffset;
      long  mPeekTagSize;
      BChunkTag mPeekTag;

      bool                                mManageFile;
      bool                                mStrict;

}; //class BChunkReader

//==============================================================================
// eof: ChunkReader.h
//==============================================================================

//==============================================================================
// chunkwriter.h
//
// Copyright (c) 1999-2007, Ensemble Studios
//==============================================================================

#pragma once

//==============================================================================
#include "chunker.h"

//==============================================================================

//==============================================================================
// BChunkWriter
//==============================================================================
class BChunkWriter
{
   public:
      //Constructors/Destructors.
      BChunkWriter();
      BChunkWriter(BStream* pStream, bool strict);
      BChunkWriter(long dirID, const char* pFilename, bool strict);
      ~BChunkWriter();

      //No skip tag mode.
      void           setNoSkipTagMode(bool val) { mNoSkipTagMode=val; }

      //"Skip" methods.  These are only to be used if you know what you're doing,
      //but they let you leave space for data writebacks at a later date in the file.
      long           skipAhead( long &position, long dataSize );
      long           skipAheadArray( long &position, long dataSize );
      long           writeLongData( long position, long data );
      long           writeDWORDData( long position, DWORD data );
      long           writeBYTEArrayData(long position, long count, const BYTE *data);

	   //Tags.
	   long           writeTag(BChunkTag tag, long size);
	   long           writeTagPostSized(BChunkTag tag, long &position);
      //Sizes.  Requires the handle used in writeTagPostSized.
	   long           writeSize(long position);

      //Simple writes.
      long           writeChar(char data);
      long           writeBYTE(BYTE data);
      long           writeSignedChar(const signed char data);

      long           writeShort(short data);
      long           writeWORD(WORD data);

      long           writeLong(long data);
      long           writeDWORD(DWORD data);      
      long           writeInt64(__int64 data);

      long           writeFloat(float data);
      long           writeDouble(double data);

      long           writeBool(bool data);
      long           writeVector(const BVector &data);

      long           writeMatrix(const BMatrix &data);

      bool           writeBString(const BString &string);
      bool           writeBSimString(const BSimString &string);

      //Simple, Tagged writes.  Writes out Tag, Size, and Data.
      long           writeTaggedChar(BChunkTag tag, char data);
      long           writeTaggedBYTE(BChunkTag tag, BYTE data);

      long           writeTaggedShort(BChunkTag tag, short data);
      long           writeTaggedWORD(BChunkTag tag, WORD data);

      long           writeTaggedLong(BChunkTag tag, long data);
      long           writeTaggedDWORD(BChunkTag tag, DWORD data);
      long           writeTaggedInt64(BChunkTag tag, __int64 data);

      long           writeTaggedFloat(BChunkTag tag, float data);
      long           writeTaggedDouble(BChunkTag tag, double data);

      long           writeTaggedBool(BChunkTag tag, bool data);
      long           writeTaggedVector(BChunkTag tag, const BVector &data);
   
      //Array writes.
      long           writeCharArray(long count, const char *data);
	   long           writeBYTEArray(long count, const BYTE *data);
	   long           writeSignedCharArray(long count, const signed char *data);

      long           writeShortArray(long count, const short *data);
      long           writeWORDArray(long count, const WORD *data);

      long           writeLongArray(long count, const long *data);
	   long           writeDWORDArray(long count, const DWORD *data);
      long           writeInt64Array(long count, const __int64 *data);

	   long           writeFloatArray(long count, const float *data);
	   long           writeDoubleArray(long count, const double *data);

      long           writeBoolArray(long count, const bool *data);
      long           writeVectorArray(long count, const BVector *data);

      //Tagged array writes.
      long           writeTaggedCharArray(BChunkTag tag, long count, const char *data);
	   long           writeTaggedBYTEArray(BChunkTag tag, long count, const BYTE *data);

      long           writeTaggedShortArray(BChunkTag tag, long count, const short *data);
	   long           writeTaggedWORDArray(BChunkTag tag, long count, const WORD *data);

      long           writeTaggedLongArray(BChunkTag tag, long count, const long *data);
	   long           writeTaggedDWORDArray(BChunkTag tag, long count, const DWORD *data);
      long           writeTaggedInt64Array(BChunkTag tag, long count, const __int64 *data);

	   long           writeTaggedFloatArray(BChunkTag tag, long count, const float *data) ;
	   long           writeTaggedDoubleArray(BChunkTag tag, long count, const double *data);

      long           writeTaggedBoolArray(BChunkTag tag, long count, const bool *data);
      long           writeTaggedVectorArray(BChunkTag tag, long count, const BVector *data);

      DWORD          getFilePosition(void) const;

   protected:
      //These "manage" the file that this class can manage.
      long                        openFile(long dirID, const char* pFilename );
      long                        closeFile();

      //Wrappers around fwrite.
      long                        write(const void *data, long datalen );
      long                        writeArray(const void *data, long datalen, long count );

      BStream*                            mpStream;
      bool                                mManageFile;
      bool                                mStrict;
      BDynamicSimLongArray                mPositionStack;
      BDynamicSimLongArray                mTagStack;
      long                                mLog;
      bool                                mNoSkipTagMode;

}; //class BChunkWriter

//==============================================================================
// eof: ChunkWriter.h
//==============================================================================
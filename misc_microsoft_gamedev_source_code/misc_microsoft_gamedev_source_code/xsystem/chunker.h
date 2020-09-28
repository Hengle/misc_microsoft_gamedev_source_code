//==============================================================================
// chunker.h
//
// Copyright (c) 1999-2007, Ensemble Studios
//==============================================================================

#pragma once 

//==============================================================================
//
//==============================================================================
typedef WORD BChunkTag;

//==============================================================================
//
//==============================================================================
// Macros
#define BCHUNKTAG(a) ((BChunkTag)(((int)(a[1]) << 8) + ((int)(a[0]))))

//==============================================================================
// macro for use by reader and writer
//==============================================================================
#define strictASSERT() { if (mStrict) BASSERT(0); }

//==============================================================================
// includes must go *after* the #defines above. *blargh*
//==============================================================================
#include "uiupdatecallback.h"
#include "chunkreader.h"
#include "chunkwriter.h"

class BChunkReader;
class BChunkWriter;
class BFile;

//==============================================================================
//
//==============================================================================
//Reader.
BChunkReader* getChunkReader(BStream* pStream, bool strict);
#define releaseChunkReader(x) { delete x; x = 0; }

//Writer.
BChunkWriter* getChunkWriter(BStream* pStream, bool strict);
#define releaseChunkWriter(x) { delete x; x = 0; }

//==============================================================================
//
//==============================================================================
// Easy read/write macros w/error reporting
// These all assume they can store the return value in "result"

#define CHUNKREADSAFE(reader,vartype,varname) \
   do \
   { \
      result = reader->read##vartype(&varname); \
      if (!result) \
      { \
         {setBlogError(4012); blogerror("ChunkerRead error for variable %s, type %s, on line %s, %d", #varname, #vartype, __FILE__,__LINE__);}\
         return(false); \
      } \
   } while (0)

#define CHUNKREADTAGGEDSAFE(reader,vartype,tag,varname) \
   do \
   { \
      result = reader->readTagged##vartype(tag,&varname); \
      if (!result) \
      { \
         {setBlogError(4013); blogerror("ChunkerReadTagged error for variable %s, type %s, tag %s, on line %s, %d", #varname, #vartype, #tag, __FILE__,__LINE__);}\
         return(false); \
      } \
   } while (0)

#define CHUNKREADARRAYSAFE(reader,vartype,count,varname,maxcount) \
   do \
   { \
      result = reader->read##vartype##Array(count,varname,maxcount); \
      if (!result) \
      { \
         {setBlogError(4014); blogerror("ChunkerReadArray error for variable %s, type %s, maxcount %d, on line %s, %d", #varname, #vartype, maxcount, __FILE__,__LINE__);}\
         return(false); \
      } \
   } while (0)

#define CHUNKREADTAGGEDARRAYSAFE(reader,vartype,tag,count,varname,maxcount) \
   do \
   { \
      result = reader->readTagged##vartype##Array(tag,count,varname,maxcount); \
      if (!result) \
      { \
         {setBlogError(4015); blogerror("ChunkerReadTaggedArray error for variable %s, type %s, tag %s, maxcount %d, on line %s, %d", #varname, #vartype, #tag, maxcount, __FILE__,__LINE__);}\
         return(false); \
      } \
   } while (0)

#define CHUNKWRITESAFE(writer,vartype,varname) \
   do \
   { \
      result = writer->write##vartype(varname); \
      if (!result) \
      { \
         {setBlogError(4016); blogerror("ChunkerWrite error for variable %s, type %s, on line %s, %d", #varname, #vartype, __FILE__,__LINE__);}\
         return(false); \
      } \
   } while (0)

#define CHUNKWRITETAGGEDSAFE(writer,vartype,tag,varname) \
   do \
   { \
      result = writer->writeTagged##vartype(tag,varname); \
      if (!result) \
      { \
         {setBlogError(4017); blogerror("ChunkerWriteTagged error for variable %s, type %s, tag %s, on line %s, %d", #varname, #vartype, #tag, __FILE__,__LINE__);}\
         return(false); \
      } \
   } while (0)

#define CHUNKWRITEARRAYSAFE(writer,vartype,count,varname) \
   do \
   { \
      result = writer->write##vartype##Array(count,varname); \
      if (!result) \
      { \
         {setBlogError(4018); blogerror("ChunkerWriteArray error for variable %s, type %s, count %d, on line %s, %d", #varname, #vartype, count, __FILE__,__LINE__);}\
         return(false); \
      } \
   } while (0)

#define CHUNKWRITETAGGEDARRAYSAFE(writer,vartype,tag,count,varname) \
   do \
   { \
      result = writer->writeTagged##vartype##Array(tag,count,varname); \
      if (!result) \
      { \
         {setBlogError(4019); blogerror("ChunkerWriteTaggedArray error for variable %s, type %s, tag %s, count %d, on line %s, %d", #varname, #vartype, #tag, count, __FILE__,__LINE__);}\
         return(false); \
      } \
   } while (0)


//==============================================================================

//==============================================================================
// eof: chunker.h
//==============================================================================

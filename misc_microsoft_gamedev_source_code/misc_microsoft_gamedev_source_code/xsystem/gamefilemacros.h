//==============================================================================
// gamefilemacros.h
//
// Copyright (c) 2008 Ensemble Studios
//==============================================================================

//==============================================================================
// Version helpers
//==============================================================================
#define GFDECLAREVERSION \
   static const DWORD cGameFileVersion; \
   static DWORD mGameFileVersion;

#define GFIMPLEMENTVERSION(class,version) \
   const DWORD class::cGameFileVersion = version; \
   DWORD class::mGameFileVersion = 0;

//==============================================================================
// General helpers
//==============================================================================
#define GFVERIFYCOUNT(varname,max) \
{ \
   if (varname > max) \
   { \
      {GFERROR("GameFile Error: verify count var %s, count %d, max %d, on line %s, %d", #varname, (int)(varname), (int)(max), __FILE__,__LINE__);} \
      return false; \
   } \
}

void gameFileError(const char *text, ...);
#define GFERROR gameFileError

//==============================================================================
// Write helpers
//==============================================================================
#define GFWRITEVAR(stream,vartype,varname) \
{ \
   if (stream->writeBytes(&(varname), sizeof(vartype)) != sizeof(vartype)) \
   { \
      {GFERROR("GameFile Error: write var %s, type %s, on line %s, %d", #varname, #vartype, __FILE__,__LINE__);} \
      return false; \
   } \
}

#define GFWRITEVAL(stream,vartype,valname) \
{ \
   vartype saveVal = (vartype)(valname); \
   if (stream->writeBytes(&saveVal, sizeof(vartype)) != sizeof(vartype)) \
   { \
      {GFERROR("GameFile Error: write val %s, type %s, on line %s, %d", #valname, #vartype, __FILE__,__LINE__);} \
      return false; \
   } \
}

#define GFWRITEVECTOR(stream,varname) \
{ \
   float data[4]; \
   data[0] = (varname).x; \
   data[1] = (varname).y; \
   data[2] = (varname).z; \
   data[3] = (varname).w; \
   if (stream->writeBytes(data, sizeof(float)*4) != sizeof(float)*4) \
   { \
      {GFERROR("GameFile Error: write BVector %s, on line %s, %d", #varname, __FILE__,__LINE__);} \
      return false; \
   } \
}

#define GFWRITEPTR(stream,size,ptr) \
{ \
   if (stream->writeBytes(ptr, size) != size) \
   { \
      {GFERROR("GameFile Error: write ptr var %s, size %d, on line %s, %d", #ptr, (int)(size), __FILE__,__LINE__);} \
      return false; \
   } \
}

#define GFWRITESTRING(stream,vartype,varname,maxsize) \
{ \
   int len = (varname).length(); \
   if (len > (int)maxsize) \
   { \
      {GFERROR("GameFile Error: write string %s length %d too long, on line %s, %d", #varname, len, __FILE__,__LINE__);} \
      return false; \
   } \
   if (stream->writeBytes(&len, sizeof(int)) != sizeof(int)) \
   { \
      {GFERROR("GameFile Error: write string %s len on line %s, %d", #varname, __FILE__,__LINE__);} \
      return false; \
   } \
   if (len > 0) \
   { \
      uint size = vartype::cBytesPerChar * len; \
      if (stream->writeBytes((varname).getPtr(), size) != size) \
      { \
         {GFERROR("GameFile Error: write string %s on line %s, %d", #varname, __FILE__,__LINE__);} \
         return false; \
      } \
   } \
}

#define GFWRITEBITBOOL(stream,varname) \
{ \
   bool boolVal = varname; \
   if (stream->writeBytes(&boolVal, sizeof(bool)) != sizeof(bool)) \
   { \
      {GFERROR("GameFile Error: write bitbool var %s on line %s, %d", #varname, __FILE__,__LINE__);} \
      return false; \
   } \
}

#define GFWRITEARRAY(stream,vartype,varname,counttype,maxsize) \
{ \
   if ((varname).size() > (uint)maxsize) \
   { \
      {GFERROR("GameFile Error: write array data %s size %u too big, on line %s, %d", #varname, (varname).size(), __FILE__,__LINE__);} \
      return false; \
   } \
   counttype writeCount = (counttype)((varname).size()); \
   if (stream->writeBytes(&writeCount, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: write array count %s, type %s, count %u, on line %s, %d", #varname, #vartype, (uint)writeCount, __FILE__,__LINE__);} \
      return false; \
   } \
   if (writeCount > 0) \
   { \
      uint writeSize = (uint)writeCount * sizeof(vartype); \
      if (stream->writeBytes((varname).getPtr(), writeSize) != writeSize) \
      { \
         {GFERROR("GameFile Error: write array data %s, type %s, count %u, on line %s, %d", #varname, #vartype, writeCount, __FILE__,__LINE__);} \
         return false; \
      } \
   } \
}

#define GFWRITEVECTORARRAY(stream,varname,counttype,maxsize) \
{ \
   if ((varname).size() > (uint)maxsize) \
   { \
      {GFERROR("GameFile Error: write vector array data %s size %u too big, on line %s, %d", #varname, (varname).size(), __FILE__,__LINE__);} \
      return false; \
   } \
   counttype writeCount = (counttype)((varname).size()); \
   if (stream->writeBytes(&writeCount, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: write vector array count %s, count %u, on line %s, %d", #varname, (uint)writeCount, __FILE__,__LINE__);} \
      return false; \
   } \
   if (writeCount > 0) \
   { \
      float data[4]; \
      for (counttype i=0; i<writeCount; i++) \
      { \
         const BVector& vec = (varname)[i]; \
         data[0] = vec.x; \
         data[1] = vec.y; \
         data[2] = vec.z; \
         data[3] = vec.w; \
         if (stream->writeBytes(data, sizeof(float)*4) != sizeof(float)*4) \
         { \
            {GFERROR("GameFile Error: write vectory array %s, index %u, on line %s, %d", #varname, (uint)i, __FILE__,__LINE__);} \
            return false; \
         } \
      } \
   } \
}

#define GFWRITEBITVECTOR(stream,varname) \
{ \
   long saveVal = (varname).getValue(); \
   if (stream->writeBytes(&saveVal, sizeof(long)) != sizeof(long)) \
   { \
      {GFERROR("GameFile Error: write bitvector %s, on line %s, %d", #varname, __FILE__,__LINE__);} \
      return false; \
   } \
}

#define GFWRITEUTBITVECTOR(stream,varname,counttype,maxsize) \
{ \
   if ((varname).getSize() > (long)(maxsize)) \
   { \
      {GFERROR("GameFile Error: write utbitvector %s size %d too big, max %d, on line %s, %d", #varname, (varname).getSize(), (int)(maxsize), __FILE__,__LINE__);} \
      return false; \
   } \
   counttype writeCount = (counttype)((varname).getSize()/8); \
   if (stream->writeBytes(&writeCount, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: write utbitvector count %s, count %u, on line %s, %d", #varname, (uint)writeCount, __FILE__,__LINE__);} \
      return false; \
   } \
   if (writeCount > 0) \
   { \
      uint writeSize = (uint)writeCount; \
      const char* writePtr = (varname).getRawValue(); \
      char* writePtr2 = const_cast<char*>(writePtr); \
      if (stream->writeBytes(writePtr2, writeSize) != writeSize) \
      { \
         {GFERROR("GameFile Error: write utbitvector data %s, count %u, on line %s, %d", #varname, writeCount, __FILE__,__LINE__);} \
         return false; \
      } \
   } \
}

#define GFWRITEBITARRAY(stream,varname,counttype,maxsize) \
{ \
   if ((varname).getNumberBytes() > ((int)maxsize)/8) \
   { \
      {GFERROR("GameFile Error: write bitarray %s size %d too big, max %d, on line %s, %d", #varname, (varname).getNumber(), (int)(maxsize), __FILE__,__LINE__);} \
      return false; \
   } \
   counttype writeCount = (counttype)((varname).getNumberBytes()); \
   if (stream->writeBytes(&writeCount, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: write bitarray count %s, count %u, on line %s, %d", #varname, (uint)writeCount, __FILE__,__LINE__);} \
      return false; \
   } \
   if (writeCount > 0) \
   { \
      uint writeSize = (uint)writeCount; \
      const unsigned char* writePtr = (varname).getBits(); \
      unsigned char* writePtr2 = const_cast<unsigned char*>(writePtr); \
      if (stream->writeBytes(writePtr2, writeSize) != writeSize) \
      { \
         {GFERROR("GameFile Error: write bitarray data %s, count %u, on line %s, %d", #varname, writeCount, __FILE__,__LINE__);} \
         return false; \
      } \
   } \
}

#define GFWRITECLASS(stream,savetype,varname) \
{ \
   if (!((varname).save(stream, savetype))) \
   { \
      {GFERROR("GameFile Error: write class %s, on line %s, %d", #varname, __FILE__,__LINE__);} \
      return false; \
   } \
}

#define GFWRITECLASSPTR(stream,savetype,varname) \
{ \
   if (!((varname)->save(stream, savetype))) \
   { \
      {GFERROR("GameFile Error: write class ptr %s, on line %s, %d", #varname, __FILE__,__LINE__);} \
      return false; \
   } \
}

#define GFWRITECLASSARRAY(stream,savetype,varname,counttype,maxsize) \
{ \
   counttype itemCount = (counttype)((varname).size()); \
   if (itemCount > maxsize) \
   { \
      {GFERROR("GameFile Error: write class array %s, size %d, on line %s, %d", #varname, (int)itemCount, __FILE__,__LINE__);} \
      return false; \
   } \
   if (stream->writeBytes(&itemCount, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: write class array %s, on line %s, %d", #varname, __FILE__,__LINE__);} \
      return false; \
   } \
   for (counttype i=0; i<itemCount; i++) \
   { \
      if (!(((varname)[i]).save(stream, savetype))) \
      { \
         {GFERROR("GameFile Error: write class array %s, index %u, on line %s, %d", #varname, (uint)i, __FILE__,__LINE__);} \
         return false; \
      } \
   } \
}

#define GFWRITECLASSPTRARRAY(stream,savetype,vartype,varname,counttype,maxsize) \
{ \
   counttype itemCount = (counttype)((varname).size()); \
   if (itemCount > maxsize) \
   { \
      {GFERROR("GameFile Error: write class ptr array size %s, size %d, on line %s, %d", #varname, (int)itemCount, __FILE__,__LINE__);} \
      return false; \
   } \
   if (stream->writeBytes(&itemCount, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: write class array ptr write size %s, on line %s, %d", #varname, __FILE__,__LINE__);} \
      return false; \
   } \
   for (counttype i=0; i<itemCount; i++) \
   { \
      vartype* pItem = (varname)[i]; \
      if (!pItem) \
         continue; \
      if (stream->writeBytes(&i, sizeof(counttype)) != sizeof(counttype)) \
      { \
         {GFERROR("GameFile Error: write class array ptr %s write index %u, on line %s, %d", #varname, (uint)i, __FILE__,__LINE__);} \
         return false; \
      } \
      if (!pItem->save(stream, savetype)) \
      { \
         {GFERROR("GameFile Error: write class array ptr %s save, index %u, on line %s, %d", #varname, (uint)i, __FILE__,__LINE__);} \
         return false; \
      } \
   } \
   counttype doneIndex = maxsize + 1; \
   if (stream->writeBytes(&doneIndex, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: write class array ptr %s write doneIndex, on line %s, %d", #varname, __FILE__,__LINE__);} \
      return false; \
   } \
}

#define GFWRITEFREELIST(stream,savetype,vartype,varname,counttype,maxsize) \
{ \
   counttype highWaterMark = (counttype)((varname).getHighWaterMark()); \
   if (highWaterMark > maxsize) \
   { \
      {GFERROR("GameFile Error: write freelist %s highWaterMark %u too big, on line %s, %d", #varname, (uint)highWaterMark, __FILE__,__LINE__);} \
      return false; \
   } \
   if (stream->writeBytes(&highWaterMark, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: write freelist highWaterMark write %s, type %s, on line %s, %d", #varname, #vartype, __FILE__,__LINE__);} \
      return false; \
   } \
   counttype numAllocated = (counttype)((varname).getNumberAllocated()); \
   if (numAllocated > maxsize) \
   { \
      {GFERROR("GameFile Error: write freelist %s numAllocated %u too big, on line %s, %d", #varname, (uint)numAllocated, __FILE__,__LINE__);} \
      return false; \
   } \
   if (stream->writeBytes(&numAllocated, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: write freelist numAllocated write %s, on line %s, %d", #varname, __FILE__,__LINE__);} \
      return false; \
   } \
   for (counttype i=0; i<highWaterMark; i++) \
   { \
      if ((varname).isInUse(i)) \
      { \
         if (stream->writeBytes(&i, sizeof(counttype)) != sizeof(counttype)) \
         { \
            {GFERROR("GameFile Error: write freelist index write %s, type %s, index %u, on line %s, %d", #varname, #vartype, (uint)i, __FILE__,__LINE__);} \
            return false; \
         } \
         const vartype* pVarItem = (varname).get(i); \
         if (!(pVarItem->save(stream, savetype))) \
         { \
            {GFERROR("GameFile Error: write freelist class save %s, type %s, index %u, on line %s, %d", #varname, #vartype, (uint)i, __FILE__,__LINE__);} \
            return false; \
         } \
      } \
   } \
}

#define GFWRITEFREELISTHEADER(stream,vartype,varname,counttype,maxsize) \
{ \
   counttype highWaterMark = (counttype)((varname).getHighWaterMark()); \
   if (highWaterMark > maxsize) \
   { \
      {GFERROR("GameFile Error: write freelist header %s highWaterMark %u too big, on line %s, %d", #varname, (uint)highWaterMark, __FILE__,__LINE__);} \
      return false; \
   } \
   if (stream->writeBytes(&highWaterMark, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: write freelist header highWaterMark write %s, type %s, on line %s, %d", #varname, #vartype, __FILE__,__LINE__);} \
      return false; \
   } \
   counttype numAllocated = (counttype)((varname).getNumberAllocated()); \
   if (numAllocated > maxsize) \
   { \
      {GFERROR("GameFile Error: write freelist header %s numAllocated %u too big, on line %s, %d", #varname, (uint)numAllocated, __FILE__,__LINE__);} \
      return false; \
   } \
   if (stream->writeBytes(&numAllocated, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: write freelist header numAllocated write %s, on line %s, %d", #varname, __FILE__,__LINE__);} \
      return false; \
   } \
   for (counttype i=0; i<highWaterMark; i++) \
   { \
      if ((varname).isInUse(i)) \
      { \
         if (stream->writeBytes(&i, sizeof(counttype)) != sizeof(counttype)) \
         { \
            {GFERROR("GameFile Error: write freelist header index write %s, type %s, index %u, on line %s, %d", #varname, #vartype, (uint)i, __FILE__,__LINE__);} \
            return false; \
         } \
      } \
   } \
}

#define GFWRITEFREELISTDATA(stream,savetype,vartype,varname,counttype,maxsize) \
{ \
   counttype highWaterMark = (counttype)(varname).getHighWaterMark(); \
   if (highWaterMark > maxsize) \
   { \
      {GFERROR("GameFile Error: write freelist data %s highWaterMark %u too big, on line %s, %d", #varname, (uint)highWaterMark, __FILE__,__LINE__);} \
      return false; \
   } \
   counttype numAllocated = (counttype)((varname).getNumberAllocated()); \
   if (numAllocated > maxsize) \
   { \
      {GFERROR("GameFile Error: write freelist data %s numAllocated %u too big, on line %s, %d", #varname, (uint)numAllocated, __FILE__,__LINE__);} \
      return false; \
   } \
   if (stream->writeBytes(&numAllocated, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: write freelist data numAllocated write %s, on line %s, %d", #varname, __FILE__,__LINE__);} \
      return false; \
   } \
   for (counttype i=0; i<highWaterMark; i++) \
   { \
      if ((varname).isInUse(i)) \
      { \
         if (stream->writeBytes(&i, sizeof(counttype)) != sizeof(counttype)) \
         { \
            {GFERROR("GameFile Error: write freelist data index write %s, index %u, on line %s, %d", #varname, (uint)i, __FILE__,__LINE__);} \
            return false; \
         } \
         const vartype* pVarItem = (varname).get(i); \
         if (!(pVarItem->save(stream, savetype))) \
         { \
            {GFERROR("GameFile Error: write freelist data class save %s, type %s, index %u, on line %s, %d", #varname, #vartype, (uint)i, __FILE__,__LINE__);} \
            return false; \
         } \
      } \
   } \
}

#define GFWRITEFREELISTITEMPTR(stream,vartype,varname) \
{ \
   bool haveItem = ((varname) != NULL); \
   uint index = UINT_MAX; \
   if (haveItem) \
   { \
      if (!vartype::mFreeList.getIndex((vartype*)(varname), index)) \
         haveItem = false; \
   } \
   if (stream->writeBytes(&haveItem, sizeof(bool)) != sizeof(bool)) \
   { \
      {GFERROR("GameFile Error: write freelistitemptr %s, on line %s, %d", #varname, __FILE__,__LINE__);} \
      return false; \
   } \
   if (haveItem) \
   { \
      if (stream->writeBytes(&index, sizeof(uint)) != sizeof(uint)) \
      { \
         {GFERROR("GameFile Error: write freelistitemptr %s, on line %s, %d", #varname, __FILE__,__LINE__);} \
         return false; \
      } \
   } \
}

#define GFWRITEVERSION(stream,class) \
{ \
   class::mGameFileVersion = class::cGameFileVersion; \
   if (stream->writeBytes(&(class::mGameFileVersion), sizeof(DWORD)) != sizeof(DWORD)) \
   { \
      {GFERROR("GameFile Error: write version of class %s on line %s, %d", #class, __FILE__,__LINE__);} \
      return false; \
   } \
}

#define GFWRITEMARKER(stream,marker) \
{ \
   uint16 writeMarker = (uint16)marker; \
   if (stream->writeBytes(&writeMarker, sizeof(uint16)) != sizeof(uint16)) \
   { \
      {GFERROR("GameFile Error: write marker %s on line %s, %d", #marker, __FILE__,__LINE__);} \
      return false; \
   } \
}

//==============================================================================
// Read helpers
//==============================================================================
#define GFREADVAR(stream,vartype,varname) \
{ \
   if (stream->readBytes(&(varname), sizeof(vartype)) != sizeof(vartype)) \
   { \
      {GFERROR("GameFile Error: read var %s, type %s, on line %s, %d", #varname, #vartype, __FILE__,__LINE__);} \
      return false; \
   } \
}

#define GFREADVAL(stream,vartype,valtype,valname) \
{ \
   vartype loadVal; \
   if (stream->readBytes(&loadVal, sizeof(vartype)) != sizeof(vartype)) \
   { \
      {GFERROR("GameFile Error: read val %s, type %s, on line %s, %d", #valname, #vartype, __FILE__,__LINE__);} \
      return false; \
   } \
   (valname)=(valtype)loadVal; \
}

#define GFREADTEMPVAL(stream,vartype) \
{ \
   vartype loadVal; \
   if (stream->readBytes(&loadVal, sizeof(vartype)) != sizeof(vartype)) \
   { \
      {GFERROR("GameFile Error: read temp var %s, on line %s, %d", #vartype, __FILE__,__LINE__);} \
      return false; \
   } \
}

#define GFREADVECTOR(stream,varname) \
{ \
   float data[4]; \
   if (stream->readBytes(data, sizeof(float)*4) != sizeof(float)*4) \
   { \
      {GFERROR("GameFile Error: read BVector %s, on line %s, %d", #varname, __FILE__,__LINE__);} \
      return false; \
   } \
   (varname).x = data[0]; \
   (varname).y = data[1]; \
   (varname).z = data[2]; \
   (varname).w = data[3]; \
}

#define GFREADPTR(stream,size,ptr) \
{ \
   if (stream->readBytes(ptr, size) != size) \
   { \
      {GFERROR("GameFile Error: read var ptr %s, size %d, on line %s, %d", #ptr, (int)(size), __FILE__,__LINE__);} \
      return false; \
   } \
}

#define GFREADSTRING(stream,vartype,varname,maxsize) \
{ \
   int len; \
   if (stream->readBytes(&len, sizeof(int)) != sizeof(int)) \
   { \
      {GFERROR("GameFile Error: read string %s len on line %s, %d", #varname, __FILE__,__LINE__);} \
      return false; \
   } \
   if (len > (int)maxsize) \
   { \
      {GFERROR("GameFile Error: read string %s length %d too long, on line %s, %d", #varname, len, __FILE__,__LINE__);} \
      return false; \
   } \
   if (len > 0) \
   { \
      uint size = vartype::cBytesPerChar * len; \
      (varname).makeRawString(len); \
      if (stream->readBytes((varname).getString(), size) != size) \
      { \
         {GFERROR("GameFile Error: read string %s, size %u, on line %s, %d", #varname, size, __FILE__,__LINE__);} \
         return false; \
      } \
      (varname).getString()[len]=NULL; \
   } \
}

#define GFREADBITBOOL(stream,varname) \
{ \
   bool boolVal; \
   if (stream->readBytes(&boolVal, sizeof(bool)) != sizeof(bool)) \
   { \
      {GFERROR("GameFile Error: read bitbool var %s on line %s, %d", #varname, __FILE__,__LINE__);} \
      return false; \
   } \
   varname = boolVal; \
}

#define GFREADARRAY(stream,vartype,varname,counttype,maxsize) \
{ \
   counttype readCount = 0; \
   if (stream->readBytes(&readCount, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: read array count %s, type %s, count %u, on line %s, %d", #varname, #vartype, (uint)readCount, __FILE__,__LINE__);} \
      return false; \
   } \
   if ((uint)readCount > (uint)maxsize) \
   { \
      {GFERROR("GameFile Error: read array %s size %u too big, on line %s, %d", #varname, (uint)readCount, __FILE__,__LINE__);} \
      return false; \
   } \
   (varname).setNumber(readCount); \
   if ((varname).size() != (uint)readCount) \
   { \
      {GFERROR("GameFile Error: read array %s error resizing array to %u, on line %s, %d", #varname, (uint)readCount, __FILE__,__LINE__);} \
      return false; \
   } \
   if (readCount > 0) \
   { \
      uint readSize = (uint)readCount * sizeof(vartype); \
      if (stream->readBytes((varname).getPtr(), readSize) != readSize) \
      { \
         {GFERROR("GameFile Error: read array data %s, type %s, count %u, on line %s, %d", #varname, #vartype, readCount, __FILE__,__LINE__);} \
         return false; \
      } \
   } \
}

#define GFREADVECTORARRAY(stream,varname,counttype,maxsize) \
{ \
   counttype readCount = 0; \
   if (stream->readBytes(&readCount, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: read vector array count %s, count %u, on line %s, %d", #varname, (uint)readCount, __FILE__,__LINE__);} \
      return false; \
   } \
   if ((uint)readCount > (uint)maxsize) \
   { \
      {GFERROR("GameFile Error: read vector array %s size %u too big, on line %s, %d", #varname, (uint)readCount, __FILE__,__LINE__);} \
      return false; \
   } \
   (varname).setNumber(readCount); \
   if ((varname).size() != (uint)readCount) \
   { \
      {GFERROR("GameFile Error: read vector array %s error resizing array to %u, on line %s, %d", #varname, (uint)readCount, __FILE__,__LINE__);} \
      return false; \
   } \
   if (readCount > 0) \
   { \
      float data[4]; \
      for (counttype i=0; i<readCount; i++) \
      { \
         if (stream->readBytes(data, sizeof(float)*4) != sizeof(float)*4) \
         { \
            {GFERROR("GameFile Error: read vector array %s, index %u, on line %s, %d", #varname, (uint)i, __FILE__,__LINE__);} \
            return false; \
         } \
         BVector& vec = (varname)[i]; \
         vec.x = data[0]; \
         vec.y = data[1]; \
         vec.z = data[2]; \
         vec.w = data[3]; \
      } \
   } \
}

#define GFREADBITVECTOR(stream,varname) \
{ \
   long loadVal = 0; \
   if (stream->readBytes(&loadVal, sizeof(long)) != sizeof(long)) \
   { \
      {GFERROR("GameFile Error: read bitvector %s, on line %s, %d", #varname, __FILE__,__LINE__);} \
      return false; \
   } \
   (varname).setAll(loadVal); \
}

#define GFREADUTBITVECTOR(stream,varname,counttype,maxsize) \
{ \
   counttype readCount = 0; \
   if (stream->readBytes(&readCount, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: read bitarray count %s, count %u, on line %s, %d", #varname, (uint)readCount, __FILE__,__LINE__);} \
      return false; \
   } \
   if ((uint)readCount > (uint)(maxsize/8) || (long)readCount > (varname).getSize()/8) \
   { \
      {GFERROR("GameFile Error: read bitarray %s size %u too big, on line %s, %d", #varname, (uint)readCount, __FILE__,__LINE__);} \
      return false; \
   } \
   (varname).zero(); \
   if (readCount > 0) \
   { \
      uint readSize = (uint)readCount; \
      const char* readPtr = (varname).getRawValue(); \
      char* readPtr2 = const_cast<char*>(readPtr); \
      if (stream->readBytes(readPtr2, readSize) != readSize) \
      { \
         {GFERROR("GameFile Error: read bitarray data %s, count %u, on line %s, %d", #varname, readCount, __FILE__,__LINE__);} \
         return false; \
      } \
   } \
}

#define GFREADBITARRAY(stream,varname,counttype,maxsize) \
{ \
   counttype readCount = 0; \
   if (stream->readBytes(&readCount, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: read bitarray count %s, count %u, on line %s, %d", #varname, (uint)readCount, __FILE__,__LINE__);} \
      return false; \
   } \
   if ((uint)readCount > (uint)(maxsize/8) || (long)readCount > (varname).getNumberBytes()) \
   { \
      {GFERROR("GameFile Error: read bitarray %s size %u too big, on line %s, %d", #varname, (uint)readCount, __FILE__,__LINE__);} \
      return false; \
   } \
   (varname).clear(); \
   if (readCount > 0) \
   { \
      uint readSize = (uint)readCount; \
      const unsigned char* readPtr = (varname).getBits(); \
      unsigned char* readPtr2 = const_cast<unsigned char*>(readPtr); \
      if (stream->readBytes(readPtr2, readSize) != readSize) \
      { \
         {GFERROR("GameFile Error: read bitarray data %s, count %u, on line %s, %d", #varname, readCount, __FILE__,__LINE__);} \
         return false; \
      } \
   } \
}

#define GFREADCLASS(stream,savetype,varname) \
{ \
   if (!((varname).load(stream, savetype))) \
   { \
      {GFERROR("GameFile Error: read class %s, on line %s, %d", #varname, __FILE__,__LINE__);} \
      return false; \
   } \
}

#define GFREADCLASSPTR(stream,savetype,varname) \
{ \
   if (!((varname)->load(stream, savetype))) \
   { \
      {GFERROR("GameFile Error: read class %s, on line %s, %d", #varname, __FILE__,__LINE__);} \
      return false; \
   } \
}

#define GFREADTEMPCLASS(stream,savetype,vartype) \
{ \
   vartype tempClass; \
   if (!tempClass.load(stream, savetype)) \
   { \
      {GFERROR("GameFile Error: read class %s, on line %s, %d", #vartype, __FILE__,__LINE__);} \
      return false; \
   } \
}

#define GFREADCLASSARRAY(stream,savetype,varname,counttype,maxsize) \
{ \
   counttype itemCount; \
   if (stream->readBytes(&itemCount, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: read class array %s, on line %s, %d", #varname, __FILE__,__LINE__);} \
      return false; \
   } \
   if (itemCount > maxsize) \
   { \
      {GFERROR("GameFile Error: read class array %s, size %d, on line %s, %d", #varname, (int)itemCount, __FILE__,__LINE__);} \
      return false; \
   } \
   (varname).setNumber(itemCount); \
   if ((varname).size() != (uint)itemCount) \
   { \
      {GFERROR("GameFile Error: read class array %s error resizing array to %d, on line %s, %d", #varname, (int)itemCount, __FILE__,__LINE__);} \
      return false; \
   } \
   for (counttype i=0; i<itemCount; i++) \
   { \
      if (!((varname)[i]).load(stream, savetype)) \
      { \
         {GFERROR("GameFile Error: read class array %s, index %u, on line %s, %d", #varname, (uint)i, __FILE__,__LINE__);} \
         return false; \
      } \
   } \
}

#define GFREADCLASSPTRARRAY(stream,savetype,vartype,varname,counttype,maxsize) \
{ \
   counttype itemCount; \
   if (stream->readBytes(&itemCount, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: read class array ptr %s, on line %s, %d", #varname, __FILE__,__LINE__);} \
      return false; \
   } \
   if (itemCount > maxsize) \
   { \
      {GFERROR("GameFile Error: read class array ptr %s, size %d, on line %s, %d", #varname, (int)itemCount, __FILE__,__LINE__);} \
      return false; \
   } \
   (varname).setNumber(itemCount); \
   if ((varname).size() != (uint)itemCount) \
   { \
      {GFERROR("GameFile Error: read class array ptr %s error resizing array to %d, on line %s, %d", #varname, (int)itemCount, __FILE__,__LINE__);} \
      return false; \
   } \
   for (counttype i=0; i<itemCount; i++) \
      (varname)[i] = NULL; \
   counttype loadedCount = 0; \
   counttype doneIndex = maxsize + 1; \
   for (;;) \
   { \
      counttype index; \
      GFREADVAR(pStream, counttype, index); \
      if (index == doneIndex) \
         break; \
      if (loadedCount >= itemCount) \
      { \
         {GFERROR("GameFile Error: read class array ptr %s, loadedCount %u, on line %s, %d", #varname, (uint)loadedCount, __FILE__,__LINE__);} \
         return false; \
      } \
      if (index >= itemCount) \
      { \
         {GFERROR("GameFile Error: read class array ptr %s, index %u, on line %s, %d", #varname, (uint)index, __FILE__,__LINE__);} \
         return false; \
      } \
      loadedCount++; \
      vartype* pItem = new vartype(); \
      if (!pItem) \
         continue; \
      (varname)[index] = pItem; \
      if (!pItem->load(stream, savetype)) \
      { \
         {GFERROR("GameFile Error: read class array ptr %s save, index %u, on line %s, %d", #varname, (uint)index, __FILE__,__LINE__);} \
         return false; \
      } \
   } \
}

#define GFREADFREELIST(stream,savetype,vartype,varname,counttype,maxsize) \
{ \
   counttype highWaterMark = 0; \
   if (stream->readBytes(&highWaterMark, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: read freelist highWaterMark %s, type %s, val %u, on line %s, %d", #varname, #vartype, (uint)highWaterMark, __FILE__,__LINE__);} \
      return false; \
   } \
   if ((uint)highWaterMark > (uint)maxsize) \
   { \
      {GFERROR("GameFile Error: read freelist %s highWaterMark %u too big, on line %s, %d", #varname, (uint)highWaterMark, __FILE__,__LINE__);} \
      return false; \
   } \
   counttype numAllocated = 0; \
   if (stream->readBytes(&numAllocated, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: read freelist numAllocated %s, type %s, val %u, on line %s, %d", #varname, #vartype, (uint)numAllocated, __FILE__,__LINE__);} \
      return false; \
   } \
   if ((uint)numAllocated > (uint)maxsize) \
   { \
      {GFERROR("GameFile Error: read freelist %s numAllocated %u too big, on line %s, %d", #varname, (uint)numAllocated, __FILE__,__LINE__);} \
      return false; \
   } \
   (varname).init(highWaterMark); \
   for (counttype i=0; i<numAllocated; i++) \
   { \
      counttype index = 0; \
      if (stream->readBytes(&index, sizeof(counttype)) != sizeof(counttype)) \
      { \
         {GFERROR("GameFile Error: read freelist index %s, type %s, index %u, on line %s, %d", #varname, #vartype, (uint)index, __FILE__,__LINE__);} \
         return false; \
      } \
      vartype* pVarItem = varname.acquireAtIndex((uint)index); \
      if (!pVarItem) \
      { \
         {GFERROR("GameFile Error: read freelist acquireAtIndex %s, type %s, index %u, on line %s, %d", #varname, #vartype, (uint)index, __FILE__,__LINE__);} \
         return false; \
      } \
      if (!pVarItem->load(stream, savetype)) \
      { \
         {GFERROR("GameFile Error: read freelist item load %s, type %s, index %u, on line %s, %d", #varname, #vartype, (uint)index, __FILE__,__LINE__);} \
         return false; \
      } \
   } \
}

#define GFREADFREELISTITEMPTR(stream,vartype,varname) \
{ \
   bool haveItem; \
   if (stream->readBytes(&haveItem, sizeof(bool)) != sizeof(bool)) \
   { \
      {GFERROR("GameFile Error: read freelistitemptr %s, on line %s, %d", #varname, __FILE__,__LINE__);} \
      return false; \
   } \
   if (haveItem) \
   { \
      uint index; \
      if (stream->readBytes(&index, sizeof(uint)) != sizeof(uint)) \
      { \
         {GFERROR("GameFile Error: read freelistitemptr %s, on line %s, %d", #varname, __FILE__,__LINE__);} \
         return false; \
      } \
      if (index == UINT_MAX) \
         (varname) = NULL; \
      else \
      { \
         if (!vartype::mFreeList.isValidIndex(index)) \
         { \
            {GFERROR("GameFile Error: read freelistitemptr %s, on line %s, %d", #varname, __FILE__,__LINE__);} \
            return false; \
         } \
         else \
            (varname) = vartype::mFreeList.get(index); \
      } \
   } \
}

#define GFREADFREELISTHEADER(stream,vartype,varname,counttype,maxsize) \
{ \
   counttype highWaterMark = 0; \
   if (stream->readBytes(&highWaterMark, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: read freelist header highWaterMark %s, type %s, val %u, on line %s, %d", #varname, #vartype, (uint)highWaterMark, __FILE__,__LINE__);} \
      return false; \
   } \
   if ((uint)highWaterMark > (uint)maxsize) \
   { \
      {GFERROR("GameFile Error: read freelist header %s highWaterMark %u too big, on line %s, %d", #varname, (uint)highWaterMark, __FILE__,__LINE__);} \
      return false; \
   } \
   counttype numAllocated = 0; \
   if (stream->readBytes(&numAllocated, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: read freelist header numAllocated %s, type %s, val %u, on line %s, %d", #varname, #vartype, (uint)numAllocated, __FILE__,__LINE__);} \
      return false; \
   } \
   if ((uint)numAllocated > (uint)maxsize) \
   { \
      {GFERROR("GameFile Error: read freelist header %s numAllocated %u too big, on line %s, %d", #varname, (uint)numAllocated, __FILE__,__LINE__);} \
      return false; \
   } \
   (varname).init(highWaterMark); \
   for (counttype i=0; i<numAllocated; i++) \
   { \
      counttype index = 0; \
      if (stream->readBytes(&index, sizeof(counttype)) != sizeof(counttype)) \
      { \
         {GFERROR("GameFile Error: read freelist header index %s, type %s, index %u, on line %s, %d", #varname, #vartype, (uint)index, __FILE__,__LINE__);} \
         return false; \
      } \
      vartype* pVarItem = varname.acquireAtIndex((uint)index); \
      if (!pVarItem) \
      { \
         {GFERROR("GameFile Error: read freelist header acquireAtIndex %s, type %s, index %u, on line %s, %d", #varname, #vartype, (uint)index, __FILE__,__LINE__);} \
         return false; \
      } \
   } \
}

#define GFREADFREELISTDATA(stream,savetype,vartype,varname,counttype,maxsize) \
{ \
   counttype numAllocated = 0; \
   if (stream->readBytes(&numAllocated, sizeof(counttype)) != sizeof(counttype)) \
   { \
      {GFERROR("GameFile Error: read freelist data numAllocated %s, type %s, val %u, on line %s, %d", #varname, #vartype, (uint)numAllocated, __FILE__,__LINE__);} \
      return false; \
   } \
   if ((uint)numAllocated > (uint)maxsize) \
   { \
      {GFERROR("GameFile Error: read freelist data %s numAllocated %u too big, on line %s, %d", #varname, (uint)numAllocated, __FILE__,__LINE__);} \
      return false; \
   } \
   for (counttype i=0; i<numAllocated; i++) \
   { \
      counttype index = 0; \
      if (stream->readBytes(&index, sizeof(counttype)) != sizeof(counttype)) \
      { \
         {GFERROR("GameFile Error: read freelist data index %s, type %s, counter %u, val %u, on line %s, %d", #varname, #vartype, (uint)i, (uint)numAllocated, __FILE__,__LINE__);} \
         return false; \
      } \
      if (!(varname).isInUse(index)) \
      { \
         {GFERROR("GameFile Error: read freelist data %s, index %u not in use, counter %u, on line %s, %d", #varname, (uint)index, (uint)i, __FILE__,__LINE__);} \
         return false; \
      } \
      vartype* pVarItem = (varname).get(index); \
      if (!(pVarItem->load(stream, savetype))) \
      { \
         {GFERROR("GameFile Error: read freelist data class save %s, type %s, index %u, counter %u, on line %s, %d", #varname, #vartype, (uint)index, (uint)i, __FILE__,__LINE__);} \
         return false; \
      } \
   } \
}

#define GFREADVERSION(stream,class) \
{ \
   if (stream->readBytes(&(class::mGameFileVersion), sizeof(DWORD)) != sizeof(DWORD)) \
   { \
      {GFERROR("GameFile Error: read version of class %s on line %s, %d", #class, __FILE__,__LINE__);} \
      return false; \
   } \
   if (class::mGameFileVersion > class::cGameFileVersion) \
   { \
      {GFERROR("GameFile Error: read version for class %s, version %d > %d, on line %s, %d", #class, class::mGameFileVersion, class::cGameFileVersion, __FILE__,__LINE__);} \
      return false; \
   } \
}

#define GFREADMARKER(stream,marker) \
{ \
   uint16 readMarker = 0; \
   if (stream->readBytes(&readMarker, sizeof(uint16)) != sizeof(uint16)) \
   { \
      {GFERROR("GameFile Error: read read marker %s on line %s, %d", #marker, __FILE__,__LINE__);} \
      return false; \
   } \
   if (readMarker != (uint16)marker) \
   { \
      {GFERROR("GameFile Error: read marker %s mismatch on line %s, %d", #marker, __FILE__,__LINE__);} \
      return false; \
   } \
}

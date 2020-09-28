//============================================================================
//
// File: ecfUtils.h
//
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#pragma once

// ECF file organization:
// Header/extra header data (total is always 4 byte aligned)
// Array of chunk header/chunk header extra data
// Aligned chunk 0 data 
// Aligned chunk 1 data 
// Aligned chunk 2 data 
// etc.
// Currently, ECF structures are always big endian in memory/on disk (even on the PC).

//============================================================================
// class BECFHeader
//============================================================================
#pragma pack(push)
#pragma pack(4)
class BECFHeader
{
public:
   enum { cECFHeaderMagic = 0xDABA7737, cECFInvertedHeaderMagic = 0x3777BADA };
   enum { cECFAdler32DWORDsToSkip = 3 };
   
   void clear(void) { Utils::ClearObj(*this); }
   
   DWORD getMagic(void) const { return Utils::GetValueBigEndian<DWORD>(&mHeaderMagic); }
   void setMagic(DWORD magic) { Utils::WriteValueBigEndian(&mHeaderMagic, magic); }

   // Header size
   DWORD getSize(void) const { return Utils::GetValueBigEndian<DWORD>(&mHeaderSize); }
   void setSize(DWORD size) { Utils::WriteValueBigEndian(&mHeaderSize, size); }
   
   DWORD getExtraDataSize(void) const { return getSize() - sizeof(BECFHeader); }
   const BYTE* getExtraDataPtr(void) const { return reinterpret_cast<const BYTE*>(this + 1); }

   DWORD getAdler32(void) const { return Utils::GetValueBigEndian<DWORD>(&mHeaderAdler32); }
   void setAdler32(DWORD adler32) { Utils::WriteValueBigEndian(&mHeaderAdler32, adler32); }   

   // Get/set user provided ID.
   DWORD getID(void) const { return Utils::GetValueBigEndian<DWORD>(&mID); }
   void setID(DWORD ID) { Utils::WriteValueBigEndian(&mID, ID); }   
   
   // Total file size, including header
   DWORD getFileSize(void) const { return Utils::GetValueBigEndian<DWORD>(&mFileSize); }
   void setFileSize(DWORD fileSize) { Utils::WriteValueBigEndian(&mFileSize, fileSize); }
         
   DWORD getNumChunks(void) const { return Utils::GetValueBigEndian<WORD>(&mNumChunks); }
   void setNumChunks(DWORD numChunks) { BDEBUG_ASSERT(numChunks < USHRT_MAX); Utils::WriteValueBigEndian<WORD>(&mNumChunks, static_cast<WORD>(numChunks)); }
   
   DWORD getFlags(void) const { return Utils::GetValueBigEndian<WORD>(&mFlags); }
   void setFlags(DWORD flags) { BDEBUG_ASSERT(flags <= USHRT_MAX); Utils::WriteValueBigEndian<WORD>(&mFlags, static_cast<WORD>(flags)); }
   
   DWORD getChunkExtraDataSize(void) const { return Utils::GetValueBigEndian<WORD>(&mChunkExtraDataSize); }
   void setChunkExtraDataSize(DWORD extraDataLen) { BDEBUG_ASSERT(extraDataLen <= USHRT_MAX); Utils::WriteValueBigEndian<WORD>(&mChunkExtraDataSize, static_cast<WORD>(extraDataLen)); }
      
private:   
   DWORD                mHeaderMagic;
   DWORD                mHeaderSize;
   DWORD                mHeaderAdler32;
   
   DWORD                mFileSize;
         
   WORD                 mNumChunks;
   WORD                 mFlags;
   
   DWORD                mID;   
   
   WORD                 mChunkExtraDataSize;
   WORD                 mPad0;
   
   uint                 mPad1;
};

const DWORD cECFMaxChunks = 32768;
const DWORD cECFMaxChunkSize = 1024U*1024U*1024U;

enum eECFChunkResourceFlags
{
   cECFChunkResFlagContiguous      = 0,
   cECFChunkResFlagWriteCombined   = 1,
   cECFChunkResFlagIsDeflateStream = 2,
   cECFChunkResFlagIsResourceTag   = 3,
};

//============================================================================
// class BECFChunkHeader
//============================================================================
class BECFChunkHeader
{  
public:
   void clear(void) { Utils::ClearObj(*this); }
   
   uint64 getID(void) const { return Utils::GetValueBigEndian<uint64>(&mID); }
   void setID(uint64 ID) { Utils::WriteValueBigEndian(&mID, ID); }   
   
   DWORD getOfs(void) const { return Utils::GetValueBigEndian<DWORD>(&mOfs); }
   void setOfs(DWORD ofs) { Utils::WriteValueBigEndian(&mOfs, ofs); }   
   
   DWORD getSize(void) const { return Utils::GetValueBigEndian<DWORD>(&mSize); }
   void setSize(DWORD size) { Utils::WriteValueBigEndian(&mSize, size); }   
   
   DWORD getAdler32(void) const { return Utils::GetValueBigEndian<DWORD>(&mAdler32); }
   void setAdler32(DWORD adler32) { Utils::WriteValueBigEndian(&mAdler32, adler32); }   
   
   // The chunk extra data size is located in the ECF header: getChunkExtraDataSize().
   const BYTE* getExtraDataPtr(void) const { return reinterpret_cast<const BYTE*>(this + 1); }
   
   DWORD getFlags(void) const { return mFlags; }
   void setFlags(DWORD flags) { BDEBUG_ASSERT(flags <= UCHAR_MAX); mFlags = static_cast<BYTE>(flags); }
      
   DWORD getAlignmentLog2(void) const { return mAlignmentLog2; }
   void setAlignmentLog2(DWORD alignmentLog2) { BDEBUG_ASSERT(alignmentLog2 <= UCHAR_MAX); mAlignmentLog2 = static_cast<BYTE>(alignmentLog2); }
   
   DWORD getResourceFlags(void) const { return Utils::GetValueBigEndian<WORD>(&mResourceFlags); }
   void setResourceFlags(DWORD flags) { BDEBUG_ASSERT(flags <= USHRT_MAX); Utils::WriteValueBigEndian(&mResourceFlags, static_cast<WORD>(flags)); }
       
   bool getResourceBitFlag(eECFChunkResourceFlags bitflag) const { return (getResourceFlags() & Utils::BBitMasks::get(bitflag)) != 0; }

   void setResourceBitFlag(eECFChunkResourceFlags bitflag, uint value)
   {
      const uint bitFlag = Utils::BBitMasks::get(bitflag);
      const uint curFlags = getResourceFlags();
      setResourceFlags(value ? (curFlags | bitFlag) : (curFlags & ~bitFlag));
   }
         
private:   
   uint64                mID;
   DWORD                 mOfs;
   DWORD                 mSize;
   DWORD                 mAdler32;
      
   BYTE                  mFlags;
   BYTE                  mAlignmentLog2;
   
   WORD                  mResourceFlags;
};

#pragma pack(pop)

typedef BDynamicArray<BECFChunkHeader> BECFChunkHeaderArray;

//============================================================================
// class BECFUtils
//============================================================================
class BECFUtils
{
public:
   static bool                   checkHeader(const BConstDataBuffer& ecfData, bool adler32Check, bool disableSizeCheck = false);
   static bool                   setHeader(const BDataBuffer& ecfData, const BECFHeader& header);
   static bool                   check(const BConstDataBuffer& ecfData);
   static bool                   checkChunkPtr(BECFChunkHeader* pECFChunk, const BDataBuffer& ecfData);
   static const BECFChunkHeader* getFirstChunk(const BConstDataBuffer& ecfData);
   static const BECFChunkHeader* getChunkByIndex(const BConstDataBuffer& ecfData, uint chunkIndex);
   static int                    findChunkByID(const BConstDataBuffer& ecfData, uint64 ID, uint startIndex = 0);
   static const BYTE*            getChunkData(const BConstDataBuffer& ecfData, uint chunkIndex);
   static void                   updateChunkAdler32(const BDataBuffer& ecfData, uint chunkIndex);
   static void                   test(void);
};   

//============================================================================
// class BECFFileReader
// BECFFileReader assumes the entire ECF file is present in memory.
//============================================================================
class BECFFileReader
{
public:
   BECFFileReader() : mECFData(NULL, 0) { }
   
   BECFFileReader(const BConstDataBuffer& ecfData) : mECFData(ecfData) { }
   
   const BConstDataBuffer& getDataBuffer(void) const  { return mECFData; }
         BConstDataBuffer& getDataBuffer(void)        { return mECFData; }   
   
   void setDataBuffer(const BConstDataBuffer& ecfData) { mECFData = ecfData; }
      
   bool checkHeader(bool adler32Check) const { return BECFUtils::checkHeader(mECFData, adler32Check); }
      
   bool check(void) const { return BECFUtils::check(mECFData); }
   
   const BECFHeader* getHeader(void) const { return reinterpret_cast<const BECFHeader*>(mECFData.getPtr()); }
   
   DWORD getID(void) const { return getHeader()->getID(); }
   
   uint getNumChunks(void) const { return getHeader()->getNumChunks(); }
   
   const BECFChunkHeader* getFirstChunk(void) const { return BECFUtils::getFirstChunk(mECFData); }
         
   const BECFChunkHeader* getChunkByIndex(uint chunkIndex) const { return BECFUtils::getChunkByIndex(mECFData, chunkIndex); }
      
   // Returns cInvalidIndex if chunk can't be found.
   int findChunkByID(uint64 ID, uint startIndex = 0) const { return BECFUtils::findChunkByID(mECFData, ID, startIndex); }
   
   const BYTE* getChunkDataByIndex(uint chunkIndex) const { return BECFUtils::getChunkData(mECFData, chunkIndex); }
   const BYTE* getChunkDataByIndex(uint chunkIndex, uint& len) const { len = getChunkDataLenByIndex(chunkIndex); return BECFUtils::getChunkData(mECFData, chunkIndex); }
   uint getChunkDataLenByIndex(uint chunkIndex) const { return getChunkByIndex(chunkIndex)->getSize(); }
   
   // Returns NULL if chunk can't be found.
   const BYTE* getChunkDataByID(uint64 ID) const { int index = findChunkByID(ID); if (index < 0) return NULL; return getChunkDataByIndex(index); }
   const BYTE* getChunkDataByID(uint64 ID, uint& len) const { len = 0; int index = findChunkByID(ID); if (index < 0) return NULL; len = getChunkDataLenByIndex(index); return getChunkDataByIndex(index); }
   
   // Returns 0 if chunk can't be found.
   uint getChunkDataLenByID(uint64 ID) const { int index = findChunkByID(ID); if (index < 0) return 0; return getChunkByIndex(index)->getSize(); }
   
private:    
   BConstDataBuffer mECFData;
};

//============================================================================
// class BECFFileStream
// BECFFileStream only reads the ECF headers into memory.
//============================================================================
class BECFFileStream
{
public:
   BECFFileStream();
   BECFFileStream(BStream* pStream);
   
   bool open(BStream* pStream);
   
   void close(void);
   void closeStream(void);
   
   bool getValid(void) const { return mValid; }
      
   BStream* getStream(void) const { return mpStream; }
   uint64 getStreamHeaderOfs(void) const { return mStreamHeaderOfs; }
   
   // Returns 0 if the stream is not open or valid.
   uint getNumChunks(void) const;
   
   // Returns false if the stream is not open or valid
   bool getHeaderPtr(const BECFHeader*& pHeader) const;
   bool getChunkHeaderPtr(const BECFChunkHeader*& pChunkHeader, uint index) const;
   
   // Asserts if the stream is not open or valid.      
   const BECFHeader& getHeader(void) const;
   const BECFChunkHeader& getChunkHeader(uint index) const;
      
   const BByteArray& getHeaderArray(void) const { return mHeader; }
   const BByteArray& getChunkHeadersArray(void) const { return mChunkHeaders; }
   
   // Returns cInvalidIndex on error.
   int findChunkByID(uint64 ID, uint startIndex = 0) const;
      
   // Returns cInvalidIndex on error.
   int64 getChunkOfsByID(uint64 ID, uint startIndex = 0);
   int64 getChunkOfsByIndex(uint chunkIndex);
   
   // Returns -1 on error.
   int64 getChunkSizeByIndex(uint chunkIndex);
   
   bool seekToChunk(uint chunkIndex, bool skipBytesOnly = false);
   
private:
   BStream*    mpStream;
   uint64      mStreamHeaderOfs;
      
   BByteArray  mHeader;
   BByteArray  mChunkHeaders;
         
   bool        mValid;
};

//============================================================================
// class BECFChunkData
// BECFChunkData is used while building ECF files in memory.
//============================================================================
class BECFChunkData
{
public:
   BECFChunkData() :
      mID(0),
      mFlags(0),
      mAlignmentLog2(2),
      mResourceFlags(0)
   {
   }
   
   BECFChunkData(uint64 ID, const BYTE* pData = NULL, uint dataLen = 0) :
      mID(ID),
      mFlags(0),
      mAlignmentLog2(2),
      mResourceFlags(0)
   { 
      if (pData) 
         mData.pushBack(pData, dataLen); 
   }
   
   BECFChunkData(uint64 ID, const BString& str) :
      mID(ID),
      mFlags(0),
      mAlignmentLog2(2),
      mResourceFlags(0)
   { 
      setData(str);
   }

   uint64 getID(void) const { return mID; }
   void setID(uint64 ID) { mID = ID; }

   uint getDataLen(void) const { return mData.getSize(); }
   
   const BYTE* getDataPtr(void) const  { return mData.getPtr(); }
         BYTE* getDataPtr(void)        { return mData.getPtr(); }
   
   uint getFlags(void) const { return mFlags; }
   void setFlags(uint flags) { BDEBUG_ASSERT(flags <= UCHAR_MAX); mFlags = static_cast<BYTE>(flags); }
   
   uint getResourceFlags(void) const { return mResourceFlags; }
   void setResourceFlags(uint flags) { BDEBUG_ASSERT(flags <= USHRT_MAX); mResourceFlags = static_cast<ushort>(flags); }
   
   bool getResourceBitFlag(eECFChunkResourceFlags bitflag) const { return (getResourceFlags() & Utils::BBitMasks::get(bitflag)) != 0; }

   void setResourceBitFlag(eECFChunkResourceFlags bitflag, uint value)
   {
      const uint bitFlag = Utils::BBitMasks::get(bitflag);
      const uint curFlags = getResourceFlags();
      setResourceFlags(value ? (curFlags | bitFlag) : (curFlags & ~bitFlag));
   }
   
   uint getAlignmentLog2(void) const { return mAlignmentLog2; }
   void setAlignmentLog2(uint alignmentLog2) { BDEBUG_ASSERT(alignmentLog2 <= UCHAR_MAX); mAlignmentLog2 = static_cast<BYTE>(alignmentLog2); }

   // Variable length data
   const BByteArray& getDataArray(void) const  { return mData; }
         BByteArray& getDataArray(void)        { return mData; }
         
   void setData(const BConstDataBuffer& dataBuffer) { mData.pushBack(dataBuffer.getPtr(), dataBuffer.getLen()); }         
   
   void setData(const BString& str);
   
   BConstDataBuffer getData(void) const { return mData.getConstDataBuffer(); }
   
   bool getData(BString& str);
   
   // Fixed length chunk header extra data
   const BByteArray& getExtraDataArray(void) const  { return mExtraData; }
         BByteArray& getExtraDataArray(void)        { return mExtraData; }

private:         
   uint64      mID;
   BByteArray  mExtraData;
   BByteArray  mData;
   ushort      mResourceFlags;
   BYTE        mFlags;
   BYTE        mAlignmentLog2;
};

typedef BDynamicArray<BECFChunkData> BECFChunkDataArray;

//============================================================================
// class BECFFileBuilder
// BECFFileBuilder helps build ECF files. All ECF data must be present in memory 
// to use this class.
//============================================================================
class BECFFileBuilder
{
public:
   BECFFileBuilder() : mID(0), mFlags(0), mChunkExtraDataSize(0) { }
   ~BECFFileBuilder() { }
   
   bool readFromStream(BStream& stream);
   bool writeToStream(BStream& stream);
   
   bool writeToFileInMemory(BByteArray& ECFData);
   
   void clear(void) { mChunks.resize(0); mHeaderData.resize(0); }
   
   uint getNumChunks(void) const { return mChunks.size(); }
   
   BECFChunkData& addChunk(const BECFChunkData& data) { mChunks.pushBack(data); return mChunks.back(); }
   BECFChunkData& addChunk(uint64 ID, const BYTE* pData = NULL, uint dataLen = 0, const BYTE* pExtraData = NULL, uint extraDataLen = 0);
   BECFChunkData& addChunk(uint64 ID, const BByteArray& data) { return addChunk(ID, data.getPtr(), data.getSize()); }
   BECFChunkData& addChunk(uint64 ID, const BString& string);
   
   void insertChunk(uint index, const BECFChunkData& data) { mChunks.insert(index, data); }
   
   void removeChunkByIndex(uint index) { mChunks.erase(index); }
   
   const BECFChunkData& getChunkByIndex(uint index) const  { return mChunks[index]; }
         BECFChunkData& getChunkByIndex(uint index)        { return mChunks[index]; }
         
   // -1 on failure
   int findChunkByID(uint64 ID, uint startIndex = 0) const;
         
   const BByteArray& getHeaderData(void) const  { return mHeaderData; }
         BByteArray& getHeaderData(void)        { return mHeaderData; }   
         
   uint getID(void) const { return mID; }
   void setID(uint ID) { mID = ID; }         
   
   uint getFlags(void) const { return mFlags; }
   void setFlags(uint flags) { mFlags = flags; }   
   
   uint getChunkExtraDataSize(void) const { return mChunkExtraDataSize; }
   void setChunkExtraDataSize(uint extraDataLen) { mChunkExtraDataSize = extraDataLen; }
         
   const BECFChunkDataArray& getChunkArray(void) const { return mChunks; }
         BECFChunkDataArray& getChunkArray(void)       { return mChunks; }
   
private:
   uint mID;
   uint mFlags;
   uint mChunkExtraDataSize;
   BByteArray mHeaderData;
   BECFChunkDataArray mChunks;
};

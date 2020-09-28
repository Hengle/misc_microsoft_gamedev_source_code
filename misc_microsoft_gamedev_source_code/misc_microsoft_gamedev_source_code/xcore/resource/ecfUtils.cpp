//============================================================================
//
// File: ecfUtils.cpp
//
//  Copyright (c) 2006, Ensemble Studios
//
//============================================================================
#include "xcore.h"
#include "stream\stream.h"
#include "ecfUtils.h"
#include "hash\adler32.h"

//============================================================================
// BECFUtils::checkHeader
//============================================================================
bool BECFUtils::checkHeader(const BConstDataBuffer& ecfData, bool adler32Check, bool disableSizeCheck)
{
   const BYTE* pECFData = ecfData.getPtr();
   const uint ECFDataSize = ecfData.getLen();
   
   if ((!pECFData) || (ECFDataSize < sizeof(BECFHeader)))
      return false;
      
   const BECFHeader* pInHeader = reinterpret_cast<const BECFHeader*>(pECFData);
   
   if (pInHeader->getMagic() != BECFHeader::cECFHeaderMagic)
      return false;
      
   if (pInHeader->getSize() < sizeof(BECFHeader))
      return false;

   if (!disableSizeCheck)
   {
      if (ECFDataSize < pInHeader->getFileSize())
         return false;
   }         
      
   if (pInHeader->getNumChunks() > cECFMaxChunks)
      return false;
         
   if (adler32Check)
   {
      if (pInHeader->getAdler32() != calcAdler32(
         pECFData + sizeof(DWORD) * BECFHeader::cECFAdler32DWORDsToSkip,
         pInHeader->getSize() - sizeof(DWORD) * BECFHeader::cECFAdler32DWORDsToSkip))
      {      
         return false;      
      }      
   }      
   
   return true;
}

//============================================================================
// BECFUtils::setHeader
//============================================================================
bool BECFUtils::setHeader(const BDataBuffer& ecfData, const BECFHeader& header)
{
   BYTE* pECFData = ecfData.getPtr();
   const uint ECFDataSize = ecfData.getLen();
   
   if ((!pECFData) || (ECFDataSize < header.getSize()))
      return false;

   if (reinterpret_cast<const BYTE*>(&header) != pECFData)      
   {
      memcpy(pECFData, &header, sizeof(header));
   }
   
   BECFHeader* pDstHeader = reinterpret_cast<BECFHeader*>(pECFData);
      
   pDstHeader->setAdler32(
      calcAdler32(pECFData + sizeof(DWORD) * BECFHeader::cECFAdler32DWORDsToSkip, pDstHeader->getSize() - sizeof(DWORD) * BECFHeader::cECFAdler32DWORDsToSkip)
      );

   return true;
}

//============================================================================
// BECFUtils::check
//============================================================================
bool BECFUtils::check(const BConstDataBuffer& ecfData)
{
   const BYTE* pECFData = ecfData.getPtr();
   const uint ECFDataSize = ecfData.getLen();
   
   if (!checkHeader(ecfData, true))
      return false;
      
   const BECFHeader& header = *reinterpret_cast<const BECFHeader*>(pECFData);
        
   for (uint i = 0; i < header.getNumChunks(); i++)
   {
      const BECFChunkHeader* pChunk = getChunkByIndex(ecfData, i);
      
      const DWORD chunkSize = pChunk->getSize();
      if (!chunkSize)
         continue;
         
      if ( (pChunk->getOfs() < sizeof(BECFHeader)) || ((pChunk->getOfs() + chunkSize) > ECFDataSize) )
         return false;
         
      if (pChunk->getSize() > cECFMaxChunkSize)         
         return false;

      const uint alignment = 1 << pChunk->getAlignmentLog2();
      if (pChunk->getOfs() & (alignment - 1))
         return false;
      
      const DWORD adler32 = pChunk->getAdler32();
      if (adler32 != calcAdler32(reinterpret_cast<const BYTE*>(pECFData) + pChunk->getOfs(), pChunk->getSize()))
         return false;
   }
         
   return true;
}

//============================================================================
// BECFUtils::getFirstChunk
//============================================================================
const BECFChunkHeader* BECFUtils::getFirstChunk(const BConstDataBuffer& ecfData)
{
   if (!ecfData.getPtr())
      return NULL;
      
   const BECFHeader& header = *reinterpret_cast<const BECFHeader*>(ecfData.getPtr());
      
   const BECFChunkHeader* pChunk = reinterpret_cast<const BECFChunkHeader*>(ecfData.getPtr() + header.getSize());
   
   return pChunk;
}

//============================================================================
// BECFUtils::getChunkByIndex
//============================================================================
const BECFChunkHeader* BECFUtils::getChunkByIndex(const BConstDataBuffer& ecfData, uint chunkIndex)
{
   if (!ecfData.getPtr())
      return NULL;
      
   const BECFHeader& header = *reinterpret_cast<const BECFHeader*>(ecfData.getPtr());
   BDEBUG_ASSERT(chunkIndex < header.getNumChunks());
   
   const BECFChunkHeader* pFirstChunk = getFirstChunk(ecfData);
      
   return reinterpret_cast<const BECFChunkHeader*>(
      reinterpret_cast<const BYTE*>(pFirstChunk) + (header.getChunkExtraDataSize() + sizeof(BECFChunkHeader)) * chunkIndex
      );
}

//============================================================================
// BECFUtils::findChunkByID
//============================================================================
int BECFUtils::findChunkByID(const BConstDataBuffer& ecfData, uint64 ID, uint startIndex)
{
   if (!ecfData.getPtr())
      return cInvalidIndex;
      
   const BECFHeader& header = *reinterpret_cast<const BECFHeader*>(ecfData.getPtr());
      
   for (uint i = startIndex; i < header.getNumChunks(); i++)
   {
      const BECFChunkHeader* pChunk = getChunkByIndex(ecfData, i);
      if (pChunk->getID() == ID)
         return i;
   }
         
   return cInvalidIndex;
}

//============================================================================
// BECFUtils::getChunkData
//============================================================================
const BYTE* BECFUtils::getChunkData(const BConstDataBuffer& ecfData, uint chunkIndex)
{
   if (!ecfData.getPtr())
      return NULL;
      
   const BECFChunkHeader* pChunk = getChunkByIndex(ecfData, chunkIndex);

   if (!pChunk->getSize())
      return NULL;

   return ecfData.getPtr() + pChunk->getOfs();
}

//============================================================================
// BECFUtils::updateChunkAdler32
//============================================================================
void BECFUtils::updateChunkAdler32(const BDataBuffer& ecfData, uint chunkIndex)
{
   BDEBUG_ASSERT(ecfData.getPtr());
      
   BECFChunkHeader* pChunk = (BECFChunkHeader*)getChunkByIndex(ecfData, chunkIndex);

   if (!pChunk->getSize())
   {
      pChunk->setOfs(0);
      pChunk->setAdler32(0);
   }
   else
   {
      pChunk->setAdler32(calcAdler32(getChunkData(ecfData, chunkIndex), pChunk->getSize()));
   }
}

//============================================================================
// BECFFileStream::BECFFileStream
//============================================================================
BECFFileStream::BECFFileStream() : 
   mpStream(NULL), 
   mStreamHeaderOfs(0), 
   mValid(false)
{ 
}

//============================================================================
// BECFFileStream::BECFFileStream
//============================================================================
BECFFileStream::BECFFileStream(BStream* pStream) : 
   mpStream(NULL), 
   mStreamHeaderOfs(0), 
   mValid(false)
{ 
   open(pStream);
}

//============================================================================
// BECFFileStream::open
//============================================================================
bool BECFFileStream::open(BStream* pStream)
{
   close();
   if (!pStream)
      return false;
   
   mpStream = pStream;
   
   mStreamHeaderOfs = pStream->curOfs();

   BECFHeader header;
   if (pStream->readBytes(&header, sizeof(header)) < sizeof(header))
   {
      close();
      return false;
   }

   if (header.getMagic() != BECFHeader::cECFHeaderMagic)
   {
      close();
      return false;
   }

   if (header.getSize() < sizeof(BECFHeader))
   {
      close();
      return false;      
   }

   mHeader.pushBack(reinterpret_cast<const BYTE*>(&header), sizeof(BECFHeader));
   
   const uint headerDataLen = header.getExtraDataSize();
   if (headerDataLen)
   {
      mHeader.enlarge(headerDataLen);
      if (pStream->readBytes(mHeader.getPtr() + sizeof(BECFHeader), headerDataLen) < headerDataLen)
      {
         close();
         return false;
      }
   }
   
   uint adler32 = calcAdler32(reinterpret_cast<const BYTE*>(mHeader.getPtr()) + sizeof(DWORD) * BECFHeader::cECFAdler32DWORDsToSkip, header.getSize() - sizeof(DWORD) * BECFHeader::cECFAdler32DWORDsToSkip);
   if (adler32 != header.getAdler32())      
   {
      close();
      return false;
   }

   const uint chunkExtraDataLen = header.getChunkExtraDataSize();
   const uint numChunks = header.getNumChunks();      
   if (numChunks)
   {
      mChunkHeaders.resize(numChunks * (chunkExtraDataLen + sizeof(BECFChunkHeader)));

      if (pStream->readBytes(mChunkHeaders.getPtr(), mChunkHeaders.getSize()) < mChunkHeaders.getSize())
      {
         close();
         return false;
      }
   }      
   
   mValid = true;

   return true;
}

//============================================================================
// BECFFileStream::close
//============================================================================
void BECFFileStream::close(void)
{
   mpStream = NULL;
   mHeader.clear();
   mChunkHeaders.clear();
   mValid = false;
   mStreamHeaderOfs = 0;
}

//============================================================================
// BECFFileStream::closeStream
//============================================================================
void BECFFileStream::closeStream(void)
{
   mpStream = NULL;
}

//============================================================================
// BECFFileStream::getNumChunks
//============================================================================
uint BECFFileStream::getNumChunks(void) const
{
   if (!mValid)
      return 0;
   return getHeader().getNumChunks();     
}

//============================================================================
// BECFFileStream::seekToChunk
//============================================================================
bool BECFFileStream::seekToChunk(uint chunkIndex, bool skipBytesOnly)
{
   if ((!mValid) || (chunkIndex >= getNumChunks()))
      return false;
      
   if (!mpStream)      
      return false;
         
   const uint64 streamCurOfs = mpStream->curOfs();
   
   const BECFChunkHeader& chunkHeader = getChunkHeader(chunkIndex);
   if (!chunkHeader.getSize())
      return false;
         
   const uint64 desiredOfs = getChunkHeader(chunkIndex).getOfs() + mStreamHeaderOfs;
   
   if (skipBytesOnly)
   {
      if (desiredOfs < streamCurOfs)
         return false;
      const uint64 numBytesToSkip = desiredOfs - streamCurOfs;
      if (mpStream->skipBytes(numBytesToSkip) < numBytesToSkip)
         return false;
   }
   else
   {
      if (mpStream->seek(desiredOfs) < 0)
         return false;
   }
   
   return true;
}

//============================================================================
// BECFFileStream::getChunkOfsByID
//============================================================================
int64 BECFFileStream::getChunkOfsByID(uint64 ID, uint startIndex)
{
   if (!mValid)
      return cInvalidIndex;
      
   int chunkIndex = findChunkByID(ID, startIndex);
   if (chunkIndex < 0)
      return cInvalidIndex;
   
   //const uint64 streamCurOfs = mpStream->curOfs();

   const BECFChunkHeader& chunkHeader = getChunkHeader(chunkIndex);
   if (!chunkHeader.getSize())
      return cInvalidIndex;

   const uint64 desiredOfs = getChunkHeader(chunkIndex).getOfs() + mStreamHeaderOfs;

   return desiredOfs;
}

//============================================================================
// BECFFileStream::getChunkOfsByIndex
//============================================================================
int64 BECFFileStream::getChunkOfsByIndex(uint chunkIndex)
{
   if (!mValid)
      return cInvalidIndex;

   if (chunkIndex >= getNumChunks())
      return cInvalidIndex;
   
   const BECFChunkHeader& chunkHeader = getChunkHeader(chunkIndex);
   if (!chunkHeader.getSize())
      return cInvalidIndex;

   const uint64 desiredOfs = getChunkHeader(chunkIndex).getOfs() + mStreamHeaderOfs;

   return desiredOfs;
}

//============================================================================
// BECFFileStream::getChunkSizeByIndex
//============================================================================
int64 BECFFileStream::getChunkSizeByIndex(uint chunkIndex)
{
   if (!mValid)
      return -1;

   if (chunkIndex >= getNumChunks())
      return -1;

   const BECFChunkHeader& chunkHeader = getChunkHeader(chunkIndex);
   return (int64)chunkHeader.getSize();
}

//============================================================================
// BECFFileStream::getHeader
//============================================================================
const BECFHeader& BECFFileStream::getHeader(void) const 
{ 
   BDEBUG_ASSERT(mValid); 
   return *reinterpret_cast<const BECFHeader*>(mHeader.getPtr()); 
}

//============================================================================
// BECFFileStream::getChunkHeader
//============================================================================
const BECFChunkHeader& BECFFileStream::getChunkHeader(uint index) const 
{ 
   BDEBUG_ASSERT(mValid && index < getNumChunks()); 
   const uint chunkOfs = index * (getHeader().getChunkExtraDataSize() + sizeof(BECFChunkHeader));
   return *reinterpret_cast<const BECFChunkHeader*>(mChunkHeaders.getPtr() + chunkOfs);
}      

//============================================================================
// BECFFileStream::getHeaderPtr
//============================================================================
bool BECFFileStream::getHeaderPtr(const BECFHeader*& pHeader) const
{
   pHeader = NULL;
   if (!mValid)
      return false;
   pHeader = &getHeader();
   return true;
}

//============================================================================
// BECFFileStream::getChunkHeaderPtr
//============================================================================
bool BECFFileStream::getChunkHeaderPtr(const BECFChunkHeader*& pChunkHeader, uint index) const
{
   pChunkHeader = NULL;
   if (!mValid)
      return false;
   pChunkHeader = &getChunkHeader(index);   
   return true;
}

//============================================================================
// BECFFileStream::findChunkByID
//============================================================================
int BECFFileStream::findChunkByID(uint64 ID, uint startIndex) const
{
   if (!mValid)
      return cInvalidIndex;
      
   for (uint i = startIndex; i < getNumChunks(); i++)
      if (getChunkHeader(i).getID() == ID)
         return i;
   
   return cInvalidIndex;
}

//============================================================================
// BECFChunkData::setData
//============================================================================
void BECFChunkData::setData(const BString& str) 
{ 
   const uint len = str.length();
   BDEBUG_ASSERT(len <= USHRT_MAX);
   mData.pushBack(static_cast<uchar>(len >> 8));
   mData.pushBack(static_cast<uchar>(len & 0xFF));
   if (len)
      mData.pushBack(reinterpret_cast<const BYTE*>(str.getPtr()), len);
}

//============================================================================
// BECFChunkData::getData
//============================================================================
bool BECFChunkData::getData(BString& str)
{
   if (mData.getSize() < 2)
      return false;

   const uint len = (mData[0] << 8) | mData[1];
   if (mData.getSize() < (2 + len))
      return false;

   str.set(reinterpret_cast<const char*>(&mData[2]), len);         
   return true;
}

//============================================================================
// BECFFileBuilder::readFromStream
//============================================================================
bool BECFFileBuilder::readFromStream(BStream& stream)
{
   clear();
   
   const uint64 streamStartOfs = stream.curOfs();
      
   BECFHeader header;
   if (stream.readBytes(&header, sizeof(header)) < sizeof(header))
      return false;
      
   if (header.getMagic() != BECFHeader::cECFHeaderMagic)
      return false;
      
   if (header.getSize() < sizeof(BECFHeader))
      return false;      
      
   mID = header.getID();   
   mFlags = header.getFlags();
   mChunkExtraDataSize = header.getChunkExtraDataSize();
    
   uint adler32 = calcAdler32(reinterpret_cast<const BYTE*>(&header) + sizeof(DWORD) * BECFHeader::cECFAdler32DWORDsToSkip, sizeof(BECFHeader) - sizeof(DWORD) * BECFHeader::cECFAdler32DWORDsToSkip);
   
   const uint headerDataLen = header.getExtraDataSize();
   if (headerDataLen)
   {
      mHeaderData.resize(headerDataLen);
      if (stream.readBytes(mHeaderData.getPtr(), headerDataLen) < headerDataLen)
         return false;

      adler32 = calcAdler32(mHeaderData.getPtr(), headerDataLen, adler32);         
   }

   if (adler32 != header.getAdler32())      
      return false;

   const uint numChunks = header.getNumChunks();      
   if (numChunks)
   {
      BByteArray chunkHeaderData(numChunks * (mChunkExtraDataSize + sizeof(BECFChunkHeader)));
      
      if (stream.readBytes(chunkHeaderData.getPtr(), chunkHeaderData.getSize()) < chunkHeaderData.getSize())
         return false;
               
      mChunks.resize(numChunks);
      
      for (uint i = 0; i < numChunks; i++)
      {
         const BECFChunkHeader& chunkHeader = *reinterpret_cast<const BECFChunkHeader*>(
            chunkHeaderData.getPtr() + i * (mChunkExtraDataSize + sizeof(BECFChunkHeader))
            );
            
         if (chunkHeader.getSize() > cECFMaxChunkSize)         
            return false;
         
         mChunks[i].setID(chunkHeader.getID());
         mChunks[i].setFlags(chunkHeader.getFlags());
         mChunks[i].setAlignmentLog2(chunkHeader.getAlignmentLog2());
         mChunks[i].setFlags(chunkHeader.getResourceFlags());
         mChunks[i].setResourceFlags(chunkHeader.getResourceFlags());
         
         if (mChunkExtraDataSize)
         {
            mChunks[i].getExtraDataArray().resize(mChunkExtraDataSize);
            
            Utils::FastMemCpy(mChunks[i].getExtraDataArray().getPtr(), &chunkHeader + 1, mChunkExtraDataSize);
         }
         
         if (!chunkHeader.getSize())
            continue;
            
         mChunks[i].getDataArray().resize(chunkHeader.getSize());
         
         const int64 bytesToSkip = chunkHeader.getOfs() - (stream.curOfs() - streamStartOfs);
         if (bytesToSkip < 0)
            return false;
         else if (bytesToSkip > 0)
         {
            if (stream.skipBytes(bytesToSkip) < static_cast<uint>(bytesToSkip))
               return false;
         }
         
         if (stream.readBytes(mChunks[i].getDataArray().getPtr(), chunkHeader.getSize()) < chunkHeader.getSize())
            return false;
               
         const uint adler32 = calcAdler32(mChunks[i].getDataArray().getPtr(), chunkHeader.getSize());
         if (adler32 != chunkHeader.getAdler32())
            return false;
      }    
   }      

   return true;
}

//============================================================================
// BECFFileBuilder::writeToStream
//============================================================================
bool BECFFileBuilder::writeToStream(BStream& stream)
{
   if (mChunks.size() > cECFMaxChunks)
      return false;
   
   const uint totalHeaderSize = Utils::AlignUpValue(sizeof(BECFHeader) + mHeaderData.getSize(), 16);
   
   BByteArray ECFData;
   ECFData.enlarge(totalHeaderSize);

   BECFHeader* pHeader = reinterpret_cast<BECFHeader*>(ECFData.getPtr());

   pHeader->setMagic(static_cast<DWORD>(BECFHeader::cECFHeaderMagic));      
   pHeader->setSize(totalHeaderSize);
   pHeader->setID(mID);
   pHeader->setNumChunks(mChunks.size());
   pHeader->setFlags(mFlags);
   pHeader->setChunkExtraDataSize(mChunkExtraDataSize);

   if (mHeaderData.getSize())      
      memcpy(pHeader + 1, mHeaderData.getPtr(), mHeaderData.getSize());

   const uint totalChunkHeadersSize = (sizeof(BECFChunkHeader) + mChunkExtraDataSize) * mChunks.size();

   const uint dstHeadersOfs = ECFData.getSize();

   ECFData.enlarge(totalChunkHeadersSize);
   
   uint totalFileSize = ECFData.getSize();

   for (uint i = 0; i < mChunks.size(); i++)
   {
      BECFChunkHeader* pDstHeader = reinterpret_cast<BECFChunkHeader*>(ECFData.getPtr() + dstHeadersOfs + (i * (sizeof(BECFChunkHeader) + mChunkExtraDataSize)));
      pDstHeader->setID(mChunks[i].getID());
      pDstHeader->setSize(mChunks[i].getDataLen());
      pDstHeader->setFlags(mChunks[i].getFlags());
      pDstHeader->setAlignmentLog2(mChunks[i].getAlignmentLog2());
      pDstHeader->setResourceFlags(mChunks[i].getResourceFlags());

      if (mChunkExtraDataSize)
      {
         if (mChunks[i].getExtraDataArray().getSize() != mChunkExtraDataSize)
            return false;

         memcpy(pDstHeader + 1, mChunks[i].getExtraDataArray().getPtr(), mChunkExtraDataSize);            
      }

      if (mChunks[i].getDataLen())
      {
         totalFileSize = Utils::AlignUpValue(totalFileSize, 1 << mChunks[i].getAlignmentLog2());
         
         pDstHeader->setOfs(totalFileSize);
         pDstHeader->setAdler32(calcAdler32(mChunks[i].getDataPtr(), mChunks[i].getDataLen()));

         totalFileSize += mChunks[i].getDataLen();
      }
   }

   pHeader = reinterpret_cast<BECFHeader*>(ECFData.getPtr());         
   pHeader->setFileSize(totalFileSize);

   BECFUtils::setHeader(BDataBuffer(reinterpret_cast<BYTE*>(pHeader), pHeader->getFileSize()), *pHeader);

   const uint64 startOfs = stream.curOfs();      
   
   if (stream.writeBytes(ECFData.getPtr(), ECFData.getSize()) < ECFData.getSize())
      return false;

   for (uint i = 0; i < mChunks.size(); i++)
   {
      if (mChunks[i].getDataLen())
      {
         const uint numBytesToAlign = Utils::BytesToAlignUpValue((uint)(stream.curOfs() - startOfs), 1 << mChunks[i].getAlignmentLog2());
         if (numBytesToAlign)
            stream.writeDuplicateBytes(0, numBytesToAlign);
               
         if (stream.writeBytes(mChunks[i].getDataPtr(), mChunks[i].getDataLen()) < mChunks[i].getDataLen())
            return false;
      }
   }

   return true;
}

//============================================================================
// BECFFileBuilder::writeToFileInMemory
//============================================================================
bool BECFFileBuilder::writeToFileInMemory(BByteArray& ECFData)
{
   if (mChunks.size() > cECFMaxChunks)
      return false;
      
   const uint startOfs = ECFData.getSize();   
   
   const uint totalHeaderSize = Utils::AlignUpValue(sizeof(BECFHeader) + mHeaderData.getSize(), 16);
   ECFData.enlarge(totalHeaderSize);
   
   BECFHeader* pHeader = reinterpret_cast<BECFHeader*>(ECFData.getPtr() + startOfs);

   pHeader->setMagic(static_cast<DWORD>(BECFHeader::cECFHeaderMagic));      
   pHeader->setSize(totalHeaderSize);
   pHeader->setID(mID);
   pHeader->setNumChunks(mChunks.size());
   pHeader->setFlags(mFlags);
   pHeader->setChunkExtraDataSize(mChunkExtraDataSize);

   if (mHeaderData.getSize())      
      memcpy(pHeader + 1, mHeaderData.getPtr(), mHeaderData.getSize());
   
   const uint totalChunkHeadersSize = (sizeof(BECFChunkHeader) + mChunkExtraDataSize) * mChunks.size();
   
   const uint dstHeadersOfs = ECFData.getSize();
   
   ECFData.enlarge(totalChunkHeadersSize);
                  
   for (uint i = 0; i < mChunks.size(); i++)
   {
      BECFChunkHeader* pDstHeader = reinterpret_cast<BECFChunkHeader*>(ECFData.getPtr() + dstHeadersOfs + (i * (sizeof(BECFChunkHeader) + mChunkExtraDataSize)));
      pDstHeader->setID(mChunks[i].getID());
      pDstHeader->setSize(mChunks[i].getDataLen());
      pDstHeader->setFlags(mChunks[i].getFlags());
      pDstHeader->setAlignmentLog2(mChunks[i].getAlignmentLog2());
      pDstHeader->setResourceFlags(mChunks[i].getResourceFlags());
      
      if (mChunkExtraDataSize)
      {
         if (mChunks[i].getExtraDataArray().getSize() != mChunkExtraDataSize)
            return false;
            
         memcpy(pDstHeader + 1, mChunks[i].getExtraDataArray().getPtr(), mChunkExtraDataSize);            
      }
      
      if (mChunks[i].getDataLen())
      {
         ECFData.enlarge(Utils::BytesToAlignUpValue(ECFData.getSize(), 1 << mChunks[i].getAlignmentLog2()));
         
         BECFChunkHeader* pDstHeader = reinterpret_cast<BECFChunkHeader*>(ECFData.getPtr() + dstHeadersOfs + (i * (sizeof(BECFChunkHeader) + mChunkExtraDataSize)));
         pDstHeader->setOfs(ECFData.getSize() - startOfs);
         pDstHeader->setAdler32(calcAdler32(mChunks[i].getDataPtr(), mChunks[i].getDataLen()));
      
         ECFData.pushBack(mChunks[i].getDataPtr(), mChunks[i].getDataLen());
      }
   }
   
   pHeader = reinterpret_cast<BECFHeader*>(ECFData.getPtr() + startOfs);         
   pHeader->setFileSize(ECFData.getSize() - startOfs);
   
   BECFUtils::setHeader(BDataBuffer(reinterpret_cast<BYTE*>(pHeader), pHeader->getFileSize()), *pHeader);
   
   return true;
}

//============================================================================
// BECFFileBuilder::findChunkByID
//============================================================================
int BECFFileBuilder::findChunkByID(uint64 ID, uint startIndex) const
{
   for (uint i = startIndex; i < mChunks.size(); i++)
      if (ID == mChunks[i].getID())
         return i;
   return cInvalidIndex;
}

//============================================================================
// BECFFileBuilder::addChunk
//============================================================================
BECFChunkData& BECFFileBuilder::addChunk(uint64 ID, const BYTE* pData, uint dataLen, const BYTE* pExtraData, uint extraDataLen)
{
   mChunks.enlarge(1);
   mChunks.back().setID(ID);
   
   if (pData)
      mChunks.back().getDataArray().pushBack(pData, dataLen);
      
   if (pExtraData)      
      mChunks.back().getExtraDataArray().pushBack(pExtraData, extraDataLen);
      
   return mChunks.back();      
}

//============================================================================
//============================================================================

#include "stream\byteStream.h"
#include "stream\dynamicStream.h"
#include "math\random.h"
void BECFUtils::test(void)
{
   Random random;

   for ( ; ; )
   {
      BECFFileBuilder ecfFileBuilder1;
      ecfFileBuilder1.setID(random.iRand(0, 999999));

      //uint numChunksToWrite = random.iRand(0, cECFMaxChunks);
      uint numChunksToWrite = random.iRand(0, 1000);

      uint extraLen = Utils::AlignUpValue(random.iRand(0, 64), 4);
      ecfFileBuilder1.setChunkExtraDataSize(extraLen);

      uint headerLen = random.iRand(0, 256);
      for (uint j = 0; j < headerLen; j++)
         ecfFileBuilder1.getHeaderData().pushBack((uchar)random.uRand());

      for (uint i  = 0; i < numChunksToWrite; i++)
      {
         BDynamicStream chunkStream0;
         uint len = random.iRand(0, 256);
         for (uint j = 0; j < len; j++)
            chunkStream0.writeObj((uchar)random.uRand());

         BDynamicStream chunkStream1;
         for (uint j = 0; j < extraLen; j++)
            chunkStream1.writeObj((uchar)random.uRand());

         ecfFileBuilder1.addChunk(
            random.iRand(0, 0xFFFFFFF), 
            chunkStream0.getBuf().getPtr(), chunkStream0.getBuf().getSize(),
            chunkStream1.getBuf().getPtr(), chunkStream1.getBuf().getSize() );

         const uint alignment = random.iRand(0, 13);         
         ecfFileBuilder1.getChunkByIndex(ecfFileBuilder1.getNumChunks() - 1).setAlignmentLog2(alignment);
         ecfFileBuilder1.getChunkByIndex(ecfFileBuilder1.getNumChunks() - 1).setFlags(random.uRand() & 0xFF);
      }         
   
      BByteArray tempStream;
      BByteArray* pStream = NULL;
      BDynamicStream dynStream;    
      
      if (random.uRand() & 1)
      {
         pStream = &tempStream;
         BVERIFY(ecfFileBuilder1.writeToFileInMemory(tempStream));
      }
      else
      {
         pStream = &dynStream.getBuf();
         BVERIFY(ecfFileBuilder1.writeToStream(dynStream));
      }
      
      BByteArray& stream = *pStream;
                  
      for (uint i = 0; i < numChunksToWrite; i++)
      {
         const BECFChunkHeader* pChunk = BECFUtils::getChunkByIndex(BDataBuffer(stream.getPtr(), stream.getSize()), i);
         const uint alignment = 1<<pChunk->getAlignmentLog2();
         BVERIFY( (pChunk->getOfs() & (alignment - 1)) == 0);
      }

      printf("%i bytes\n", stream.getSizeInBytes());

      BVERIFY(BECFUtils::check(BDataBuffer(stream.getPtr(), stream.getSize())));

      BECFFileBuilder ecfFileBuilder2;
      BVERIFY(ecfFileBuilder2.readFromStream(BByteStream(stream.getPtr(), stream.getSize())));

      BVERIFY(ecfFileBuilder1.getHeaderData().getSize() <= ecfFileBuilder2.getHeaderData().getSize());
      if (ecfFileBuilder1.getHeaderData().getSize())
      {
         BVERIFY( memcmp(ecfFileBuilder1.getHeaderData().getPtr(), ecfFileBuilder2.getHeaderData().getPtr(), ecfFileBuilder1.getHeaderData().getSize()) == 0 );
      }

      BVERIFY(ecfFileBuilder1.getNumChunks() == ecfFileBuilder2.getNumChunks());
      BVERIFY(ecfFileBuilder1.getChunkExtraDataSize() == ecfFileBuilder2.getChunkExtraDataSize());
      BVERIFY(ecfFileBuilder1.getID() == ecfFileBuilder2.getID());

      for (uint i = 0; i < ecfFileBuilder1.getNumChunks(); i++)
      {
         BECFChunkData& chunk0 = ecfFileBuilder1.getChunkByIndex(i);   
         BECFChunkData& chunk1 = ecfFileBuilder2.getChunkByIndex(i);   

         uint64 id0 = chunk0.getID();
         uint64 id1 = chunk1.getID();

         BVERIFY(id0 == id1);

         BVERIFY(chunk0.getAlignmentLog2() == chunk1.getAlignmentLog2());
         BVERIFY(chunk0.getFlags() == chunk1.getFlags());

         BVERIFY(chunk0.getDataArray().getSize() == chunk1.getDataArray().getSize());
         if (chunk0.getDataArray().getSize())
         {
            BVERIFY( memcmp(chunk0.getDataArray().getPtr(), chunk1.getDataArray().getPtr(), chunk0.getDataArray().getSize()) == 0);
         }

         BVERIFY(chunk0.getExtraDataArray().getSize() == chunk1.getExtraDataArray().getSize());
         if (chunk0.getExtraDataArray().getSize())
         {
            BVERIFY( memcmp(chunk0.getExtraDataArray().getPtr(), chunk1.getExtraDataArray().getPtr(), chunk0.getExtraDataArray().getSize()) == 0);
         }
      }
      
      BECFFileStream ecfFileStream;
      BByteStream ecfByteStream(stream.getPtr(), stream.getSize());
      BVERIFY(ecfFileStream.open(&ecfByteStream));
      for (uint i = 0; i < numChunksToWrite; i++)
      {
         BECFChunkData& chunk0 = ecfFileBuilder1.getChunkByIndex(i);   
         
         uint dataSize = ecfFileStream.getChunkHeader(i).getSize();
         if (!dataSize)
            continue;

         BVERIFY(ecfFileStream.seekToChunk(i));

         BByteArray buf(dataSize);
         BVERIFY(ecfByteStream.readBytes(buf.getPtr(), buf.getSize()) == buf.getSize());

         BVERIFY( memcmp(buf.getPtr(), chunk0.getDataArray().getPtr(), chunk0.getDataArray().getSize()) == 0 );

      }
   }      
}

#if 0
#include "compressedStream.h"
#include "stream\dynamicStream.h"
BECFFileBuilder ecfBuilder;

ecfBuilder.addChunk(BECFChunkData(1, "Popcorn"));
ecfBuilder.addChunk(BECFChunkData(2, "Salad"));
ecfBuilder.addChunk(BECFChunkData(3, "Chicken"));
ecfBuilder.addChunk(BECFChunkData(4, "Spaghetti"));

BDynamicStream compData;
BDeflateStream deflStream(compData);
BVERIFY(ecfBuilder.writeToStream(deflStream));

uint64 srcBytes = deflStream.size();
deflStream.close();

compData.seek(0);

printf("%I64u to %I64u bytes\n", srcBytes, compData.size());

BInflateStream inflStream(compData);

BECFFileBuilder ecfBuild2;
BVERIFY(ecfBuild2.readFromStream(inflStream));

for (uint i = 1; i <= 4; i++)
{
   int chunkIndex = ecfBuild2.findChunkByID(i);
   if (chunkIndex >= 0)
   {
      BECFChunkData& chunk = ecfBuild2.getChunkByIndex(chunkIndex);
      BString str;
      chunk.getData(str);
      printf("%s\n", str.getPtr());
   }
}
#endif
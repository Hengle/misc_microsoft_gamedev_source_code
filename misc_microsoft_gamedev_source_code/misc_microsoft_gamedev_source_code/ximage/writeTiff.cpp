//============================================================================
//
// File: writeTiff.cpp
// Copyright (c) 2005-2007, Ensemble Studios
//
//============================================================================
#include "ximage.h"
#include "writeTiff.h"

//============================================================================
// BTIFFWriter::BTIFFWriter
//============================================================================
BTIFFWriter::BTIFFWriter() :
   mpStream(NULL),
   mWidth(0),
   mHeight(0),
   mNumComponents(0),
   mBitsPerComponent(0),
   mBytesPerPixel(0),
   mRowsPerStrip(0),
   mTotalStrips(0),
   mCurRow(0),
   mCurStrip(0),
   mStreamStartOfs(0)
{
}

//============================================================================
// BTIFFWriter::~BTIFFWriter
//============================================================================
BTIFFWriter::~BTIFFWriter()
{
}

//============================================================================
// BTIFFWriter::open
//============================================================================
bool BTIFFWriter::open(BStream& stream, uint width, uint height, uint numComponents, uint bitsPerComponent)
{
   close();
   
   if ((width < 1) || (height < 1) || (width > 32767) || (height > 32767))
      return false;
   
   if ((numComponents < 3) || (numComponents > 4))
      return false;
   
   if ((bitsPerComponent != 8) && (bitsPerComponent != 16) && (bitsPerComponent != 32))
      return false;
   
   mpStream = &stream;
   mStreamStartOfs = stream.curOfs();
   mWidth = width;
   mHeight = height;
   mNumComponents = numComponents;
   mBitsPerComponent = bitsPerComponent;
   mBytesPerPixel = (numComponents * bitsPerComponent) >> 3;
      
   mRowsPerStrip = Math::Clamp(8192U / (mWidth * mBytesPerPixel), 1U, mHeight);
   mTotalStrips = (mHeight + (mRowsPerStrip - 1)) / mRowsPerStrip;
   
   mCurRow = 0;
   mCurStrip = 0;
   
   mStripOffsets.resize(mTotalStrips);
   mStripOffsets.setAll(0);
   mStripByteCount.resize(mTotalStrips);
   mStripByteCount.setAll(0);
   
   mTags.resize(0);
   mTagData.resize(0);
   
   defineWORDTag(cIMAGEWIDTH,       (WORD)mWidth);
   defineWORDTag(cIMAGEHEIGHT,      (WORD)mHeight);
   defineWORDTag(cCOMPRESSION,      (WORD)cENC_NONE);
   defineWORDTag(cPLANARCONFIG,     (WORD)1);
   
   defineWORDTag(cSAMPLESPERPIXEL,  (WORD)numComponents);
   //defineWORDTag(cBITSPERSAMPLE,    (WORD)bitsPerComponent);
   
   WORD bitsPerSampleArray[4] = { (WORD)bitsPerComponent, (WORD)bitsPerComponent, (WORD)bitsPerComponent, (WORD)bitsPerComponent };
   defineTag(cBITSPERSAMPLE, cSHORT, numComponents, bitsPerSampleArray);
   
   defineWORDTag(cPHOTOMETRIC,      (WORD)cPHOTO_RGB);
   
   defineDWORDTag(cROWSPERSTRIP,    (DWORD)mRowsPerStrip);
   defineTag(cSTRIPOFFSETS,    cLONG, mTotalStrips, NULL);
   defineTag(cSTRIPBYTECOUNTS, cLONG, mTotalStrips, NULL);
   
   if (bitsPerComponent == 32)
   {
      WORD sampleTypeArray[4] = { 3, 3, 3, 3 };
      defineTag(cSAMPLEFORMAT, cSHORT, numComponents, sampleTypeArray);
   }
   
   return writeIFHIFD();
}

//============================================================================
// BTIFFWriter::close
//============================================================================
bool BTIFFWriter::close(void)
{
   if (!mpStream)
      return false;
      
   if (mCurStrip != mTotalStrips)
   {
      mpStream = NULL;
      return false;
   }
      
   defineTag(cSTRIPOFFSETS,    cLONG, mTotalStrips, mStripOffsets.getPtr());
   defineTag(cSTRIPBYTECOUNTS, cLONG, mTotalStrips, mStripByteCount.getPtr());

   if (!writeIFHIFD())
      return false;
   
   mpStream = NULL;
   
   return true;
}

//============================================================================
// BTIFFWriter::writeLine
//============================================================================
bool BTIFFWriter::writeLine(const void* pScanLine)
{
   if (!mpStream)
      return false;
      
   if (!pScanLine)
      return false;

   if (mCurStrip >= mTotalStrips)
      return false;
      
   if (mCurRow == 0)
      mStripOffsets[mCurStrip] = (uint)(mpStream->curOfs() - mStreamStartOfs);

   const uint bytesPerLine = mBytesPerPixel * mWidth;
   if (!mpStream->writeBytes(pScanLine, bytesPerLine))
      return false;
   
   mStripByteCount[mCurStrip] += bytesPerLine;
   mCurRow++;
   if ((mCurRow == mRowsPerStrip) || ((mCurStrip * mRowsPerStrip + mCurRow) == mHeight))
   {
      mCurRow = 0;
      mCurStrip++;
   }
      
   return true;
}

//============================================================================
// BTIFFWriter::getSizeofDataType
//============================================================================
uint BTIFFWriter::getSizeofDataType(eDataType dataType)
{
   switch (dataType)
   {
      case cBYTE:    return 1;
      case cSHORT:   return 2;
      case cLONG:    return 4;
   }
   return 0;
}

//============================================================================
// BTIFFWriter::defineTag
//============================================================================
void BTIFFWriter::defineTag(eTagType type, eDataType dataType, uint num, const void* pValue)
{
   uint i;
   for (i = 0; i < mTags.getSize(); i++)
      if (mTags[i].mType == type) 
         break;

   if (i == mTags.getSize())
   {
      mTags.enlarge(1);
      mTagData.enlarge(1);
   }
      
   BTAG& tag = mTags[i];
   BByteArray& tagData = mTagData[i];
   
   tag.mType      = (WORD)type;
   tag.mDataType  = (WORD)dataType;
   tag.mLength    = num;
   tag.mValue     = 0;
   
   tagData.resize(0);
   
   if (pValue)
   {
      const uint sizeInBytes = getSizeofDataType(dataType) * num;   
   
      if (sizeInBytes <= sizeof(DWORD))
      {
         memcpy(&tag.mValue, pValue, sizeInBytes);
      }
      else
      {
         tagData.pushBack(static_cast<const uchar*>(pValue), sizeInBytes);
      }
   }         
}

//============================================================================
// BTIFFWriter::writeIFHIFD
//============================================================================
bool BTIFFWriter::writeIFHIFD(void)
{
   if (mpStream->seek(mStreamStartOfs) < 0)
      return false;
      
   if (!mpStream->writeBytes(cLittleEndianNative ? "II" : "MM", 2))
      return false;
   
   WORD wordVal = 0x2A;
   if (!mpStream->writeObjRaw(wordVal))
      return false;
   
   DWORD dwordVal = 8;
   if (!mpStream->writeObjRaw(dwordVal))
      return false;

   wordVal = (WORD)mTags.getSize();
   if (!mpStream->writeObjRaw(wordVal))
      return false;

   uint freeFilePos = 8 + 2 + (mTags.getSize() * 12) + 4;
   uint initialFreeFilePos = freeFilePos;
   initialFreeFilePos;
   
   for (uint i = 0; i < mTags.getSize(); i++)
   {
      BTAG& tag = mTags[i];
      
      const uint sizeInBytes = getSizeofDataType((eDataType)tag.mDataType) * tag.mLength;   
      
      if (sizeInBytes > sizeof(DWORD))
      {
         tag.mValue = freeFilePos;
         freeFilePos += Utils::AlignUpValue(sizeInBytes, 2);
      }
   }
   
   if (!mpStream->writeBytes(mTags.getPtr(), mTags.getSizeInBytes()))
      return false;   
      
   dwordVal = 0;
   if (!mpStream->writeObjRaw(dwordVal))
      return false;      

   BDEBUG_ASSERT((uint)(mpStream->curOfs() - mStreamStartOfs) == initialFreeFilePos);
   
   for (uint i = 0; i < mTags.getSize(); i++)
   {
      const BTAG& tag = mTags[i];
      
      const uint sizeInBytes = getSizeofDataType((eDataType)tag.mDataType) * tag.mLength;   

      if (sizeInBytes > sizeof(DWORD))
      {
         const BByteArray& tagData = mTagData[i];
         
         if (sizeInBytes == tagData.getSize())
         {
            if (!mpStream->writeBytes(tagData.getPtr(), tagData.getSize()))
               return false;
         }
         else
         {
            if (!mpStream->writeDuplicateBytes(0, sizeInBytes))
               return false; 
         }               
         
         if (sizeInBytes & 1)
         {
            if (!mpStream->writeDuplicateBytes(0, 1))
               return false; 
         }
      }
   }
   
   return true;
}








//============================================================================
//
// File: readTiff.cpp
// Copyright (c) 2005-2007, Ensemble Studios
//
//============================================================================
#include "ximage.h"
#include "readTiff.h"

enum eTiffTagType
{
   cT_IMAGEWIDTH            = 256, /* data is a short */
   cT_IMAGEHEIGHT           = 257, /* data is a short */
   cT_BITSPERSAMPLE         = 258, /* data is a short */
   cT_COMPRESSION           = 259, /* data is a short */
   cT_PHOTOMETRIC           = 262, /* data is short */
   cT_FILLORDER             = 266, /* data is short */
   cT_STRIPOFFSETS          = 273, /* data is short or long */
   cT_ORIENTATION           = 274, /* data is short */
   cT_SAMPLESPERPIXEL       = 277, /* data is short */
   cT_ROWSPERSTRIP          = 278, /* data is short or long */
   cT_PLANARCONFIG          = 284, /* data is short */
   cT_PREDICTOR             = 317, /* data is a short */
   cT_COLORMAP              = 320, /* data is 3 sets of shorts */
   cT_INKSET                = 332, /* data is 1 short */
   cT_Y_Cb_Cr_Sub_Samp      = 530 /* data is 1 short */
};

enum eTiffDataType
{
   cD_BYTE                  = 1,   /* data is unsigned 8 bit */
   cD_ASCII                 = 2,   /* data is ASCIIZ string */
   cD_SHORT                 = 3,   /* data is unsigned 16 bit */
   cD_LONG                  = 4,   /* data is unsigned 32 bit */
   cD_RATIONAL              = 5,   /* data is 2 LONGs */
   cD_SBYTE                 = 6,   /* data is signed 8 bit */
   cD_UNDEFINED             = 7,   /* data 8 bit anything */
   cD_SSHORT                = 8,   /* data is signed 16 bit */
   cD_SLONG                 = 9,   /* data is signed 32 bit */
   cD_SRATIONAL             = 10,  /* data is 2 SLONGs */
   cD_FLOAT                 = 11,  /* data is 4-byte IEEE format */
   cD_DOUBLE                = 12  /* data is 8-byte IEEE format */
};   

enum eTiffPhotoInterp
{
   cPHOTO_BIT0              = 0,
   cPHOTO_BIT1              = 1,
   cPHOTO_RGB               = 2,
   cPHOTO_PAL               = 3,
   cPHOTO_TRANS             = 4,
   cPHOTO_CMYK              = 5,
   cPHOTO_Y_Cb_Cr           = 6
};

enum eTiffEncoding
{
   cENC_NONE                = 1,
   cENC_G3_1D_MH            = 2,
   cENC_T4                  = 3,
   cENC_T6                  = 4,
   cENC_LZW                 = 5,
   cENC_PACKBITS            = -32763
};

BTIFFReader::BTIFFReader() 
{
   clear();
}

void BTIFFReader::clear(void)
{
   mpStream = NULL;
   mWidth = 0;
   mHeight = 0;
   mCurScanline = 0;
   
   mValid = false;
   mError = false;
   mXFlipped = false;
   mYFlipped = false;
   mLinesPerStrip = 0;
   mPhotoInterp = 0;
   mSamplesPerPixel = 0;
   mBitsPerSample = 0;
   mCompression = 0;
   mOrient = 0;
   mPlanarConfig = 0;
   mPredictor = 0;
   mTotalStrips = 0;
   mLinesLeftInStrip = 0;
   mCurStrip = 0;
}

BTIFFReader::~BTIFFReader()
{

}

bool BTIFFReader::init(BStream& stream)
{
   deinit();

   mpStream = &stream;
   
   if (readIFHAndIFD() != cNO_ERROR)
   {
      mError = true;
      return false;
   }
   
   if (scanIFD() != cNO_ERROR)
   {
      mError = true;
      return false;
   }
   
   mValid = true;
   
   return true;
}

void BTIFFReader::deinit(void)
{
   mpStream = NULL;
   
   mTags.clear();
   mTagData.clear();
   mStripOffsets.clear();
   mScanlineBuf.clear();
   mPixelBuf.clear();
      
   clear();
}

bool BTIFFReader::decode(const BRGBAColorF*& pScanOfs)
{
   pScanOfs = 0;
   
   if ((!mValid) || (mError) || (mCurScanline >= mHeight))
      return false;
   
   if (mLinesLeftInStrip == 0)
   {
      mLinesLeftInStrip = mLinesPerStrip;
      
      if (mCurStrip >= mTotalStrips)
      {
         mError = true;
         return false;
      }
      
      if (mpStream->seek(mStripOffsets[mCurStrip]) != mStripOffsets[mCurStrip])
      {
         mError = true;
         return false;
      }
      
      mCurStrip++;
   }  
   
   const uint bytesPerPixel = (mSamplesPerPixel * mBitsPerSample) / 8;
   mScanlineBuf.resize(mWidth * bytesPerPixel);
   
   if (mpStream->readBytes(mScanlineBuf.getPtr(), mScanlineBuf.getSize()) != mScanlineBuf.getSize())
   {  
      mError = true;
      return false;
   }
   
   mPixelBuf.resize(mWidth);
   pScanOfs = mPixelBuf.getPtr();

   const float* pSrc = reinterpret_cast<const float*>(mScanlineBuf.getPtr());
   for (uint x = 0; x < mWidth; x++)
   {
      for (uint c = 0; c < mSamplesPerPixel; c++)
      {
         mPixelBuf[x][c] = Utils::GetValue<float>(pSrc, mByteOrder == cMOTOROLA_BYTE_ORDER);
         pSrc++;
      }
      
      if (mSamplesPerPixel < 4)
         mPixelBuf[x][3] = 1.0f;
   }
   
   mCurScanline++;
   mLinesLeftInStrip--;
          
   return true;
}

uint BTIFFReader::getUShort(const void* q) const
{
   const uchar* p = static_cast<const uchar*>(q);
   
   uchar i = *(p + 0);
   uchar j = *(p + 1);

   if (mByteOrder == cINTEL_BYTE_ORDER)
      return (i + (((uint)j) << 8));
   else
      return ((((uint)i) << 8) + j);
}

uint BTIFFReader::getUInt(const void* q) const
{
   const uchar* p = static_cast<const uchar*>(q);
   
   uint i = *(p + 0);
   uint j = *(p + 1);
   uint k = *(p + 2);
   uint l = *(p + 3);

   if (mByteOrder == cINTEL_BYTE_ORDER)
   {
      i |= (j << 8);
      i |= (k << 16);
      i |= (l << 24);
   }
   else
   {
      i <<= 24;
      i |= (j << 16);
      i |= (k << 8);
      i |= l;
   }

   return i;
}

int BTIFFReader::getSizeOfTIFFDataType(int datatype) const
{
   switch (datatype)
   {
      case cD_BYTE:
         return 1;
      case cD_SHORT:
         return 2;
      case cD_FLOAT:
      case cD_LONG:
         return 4;
      case cD_DOUBLE:
         return 8;
      default:      /* anything else can go to hell */
         return 0;
   }
}

bool BTIFFReader::importantTagType(int type) const
{
   switch (type)
   {
      case cT_IMAGEWIDTH:
      case cT_IMAGEHEIGHT:
      case cT_BITSPERSAMPLE:
      case cT_COMPRESSION:
      case cT_PHOTOMETRIC:
      case cT_FILLORDER:
      case cT_STRIPOFFSETS:
      case cT_ORIENTATION:
      case cT_SAMPLESPERPIXEL:
      case cT_ROWSPERSTRIP:
      case cT_PLANARCONFIG:
      case cT_PREDICTOR:
      case cT_INKSET:
         return true;
   }
   return false;
}

BTIFFReader::eTiffStatus BTIFFReader::readIFHAndIFD(void)
{
   uchar buf[8];

   if (mpStream->readBytes(buf, 8) != 8)
      return cERR_READ_ERROR;
      
   if ((buf[0] == 'I') && (buf[1] == 'I'))
      mByteOrder = cINTEL_BYTE_ORDER;
   else if ((buf[0] == 'M') && (buf[1] == 'M'))
      mByteOrder = cMOTOROLA_BYTE_ORDER;
   else
      return cERR_NOT_TIFF;

   if (getUShort(&buf[2]) != 0x2A)
      return cERR_BAD_IFH_VER;

   uint ifd_ofs = getUInt(&buf[4]);

   if (ifd_ofs < 8)
      return cERR_BAD_IFD_OFS;

   if (mpStream->seek(ifd_ofs) != ifd_ofs)
      return cERR_BAD_IFD_OFS;
         
   if (mpStream->readBytes(buf, 2) != 2)
      return cERR_READ_ERROR;
   
   uint numTags = getUShort(buf);

   if ((numTags < 1) || (numTags > 256))
      return cERR_BAD_NUM_TAGS;

   mTags.resize(numTags);
   if (mpStream->readBytes(mTags.getPtr(), mTags.getSizeInBytes()) != mTags.getSizeInBytes())
      return cERR_READ_ERROR;
   
   for (uint tagIndex = 0; tagIndex < mTags.size(); tagIndex++)
   {
      BTAG& tag = mTags[tagIndex];
      
      tag.mType      = static_cast<WORD>(getUShort(&tag.mType));
      tag.mDataType  = static_cast<WORD>(getUShort(&tag.mDataType));
      tag.mLength    = getUInt(&tag.mLength);
                        
      if (!importantTagType(tag.mType))            
         continue;
             
      if (tag.mLength == 0)
         tag.mLength = 1;
      else if (tag.mType == cT_COLORMAP)
      {
         /* this tag is all fucked up */

         if (tag.mLength % 3)
            tag.mLength *= 3;
         else if (tag.mLength > 768)
            tag.mLength /= 2;
         
         if (tag.mDataType != cD_SHORT)
            tag.mDataType = cD_SHORT;
      }  
            
      const uint n = getSizeOfTIFFDataType(tag.mDataType);

      if (n != 0)   /* do we care about it? */
      {
         uint len = n * tag.mLength;
                           
         const uint dataOfs = mTagData.getSize();
         uchar* pData = mTagData.enlarge(len);               
         
         if (len > 4)
         {
            const uint val = getUInt(&tag.mValue);
            
            if (mpStream->seek(val) != val)
               return cERR_READ_ERROR;
            
            if (mpStream->readBytes(pData, len) != len)
               return cERR_READ_ERROR;
         }
         else
         {
            memcpy(pData, &tag.mValue, len);
         }
         
         tag.mValue = dataOfs;
      }
   }

   return cNO_ERROR;
}

const BTIFFReader::BTAG* BTIFFReader::locateTag(int type) const
{
   for (uint i = 0; i < mTags.size(); i++)
      if (mTags[i].mType == type)
         return &mTags[i];
   return NULL;
}

uint BTIFFReader::valueOfIntTagN(const BTAG* pTag, int n) const
{
   const uchar* pData = &mTagData[pTag->mValue];
   
   switch (pTag->mDataType)
   {
      case cD_BYTE:
         return *(pData + n);
      case cD_SHORT:
         return getUShort(pData + (n * 2));
      case cD_LONG:
         return getUInt(pData + (n * 4));
   }

   return 0;
}

uint BTIFFReader::valueOfIntTag(const BTAG* pTag) const
{
   return valueOfIntTagN(pTag, 0);
}

uint BTIFFReader::valueOfIntTagDef(int type, uint def) const
{
   const BTAG* pTag;
   if ((pTag = locateTag(type)) == NULL)
      return def;
   else
      return valueOfIntTag(pTag);
}

BTIFFReader::eTiffStatus BTIFFReader::scanIFD(void)
{
   const BTAG* pTagW;
   const BTAG* pTagH;
   const BTAG* pTagSO;

   pTagW  = locateTag(cT_IMAGEWIDTH);
   pTagH  = locateTag(cT_IMAGEHEIGHT);
   pTagSO = locateTag(cT_STRIPOFFSETS);

   if ((pTagW == NULL) || (pTagH == NULL) || (pTagSO == NULL))
      return cERR_MISSING_TAG;

   mWidth            = valueOfIntTag(pTagW);
   mHeight           = valueOfIntTag(pTagH);
   mPhotoInterp      = valueOfIntTagDef(cT_PHOTOMETRIC,     1);
   mLinesPerStrip    = valueOfIntTagDef(cT_ROWSPERSTRIP,    mHeight);
   mSamplesPerPixel  = valueOfIntTagDef(cT_SAMPLESPERPIXEL, 1);
   mBitsPerSample    = valueOfIntTagDef(cT_BITSPERSAMPLE,   1);
   mCompression      = valueOfIntTagDef(cT_COMPRESSION,     1);
   mOrient           = valueOfIntTagDef(cT_ORIENTATION,     1);
   mPlanarConfig     = valueOfIntTagDef(cT_PLANARCONFIG,    1);

   if ((mWidth <= 0) || (mWidth > 16384))
      return cERR_UNSUPPORTED_RES;

   if ((mHeight <= 0) || (mHeight > 16384))
      return cERR_UNSUPPORTED_RES;

   switch (mPhotoInterp)
   {
      case cPHOTO_RGB:
      {
         if (mSamplesPerPixel < 3)
            return cERR_RGB_BAD_SAMPLES_PER_PIXEL;
         else if (mSamplesPerPixel > 4)
            return cERR_RGB_UNSUPPORTED_SAMPLES_PER_PIXEL;
         else if (mBitsPerSample != 32)
            return cERR_RGB_UNSUPPORTED_BITS_PER_SAMPLE;
         
         break;
      }
      default:
      {
         return cERR_UNSUPPORTED_COLORSPACE;
      }
   }

   {
      if ( (mLinesPerStrip > mHeight) || (mLinesPerStrip == 0) )
      {           
         mLinesPerStrip = mHeight;
      }

      mTotalStrips = (mHeight + mLinesPerStrip - 1) / mLinesPerStrip;

      if (mPlanarConfig == 2)
         mTotalStrips *= mSamplesPerPixel;

      if (mTotalStrips > 16384)
         return cERR_TOO_MANY_STRIPS;

      mStripOffsets.resize(mTotalStrips);
      
      if (pTagSO->mLength < mTotalStrips)
         return cERR_BAD_STRIP_OFFSETS_LENGTH;

      for (uint i = 0; i < mTotalStrips; i++)
         mStripOffsets[i] = valueOfIntTagN(pTagSO, i);
   }

   if (mCompression != cENC_NONE) 
      return cERR_UNSUPPORTED_COMPRESSION;

   if (mOrient == 1)
      mYFlipped = false;
   else if (mOrient == 4)
      mYFlipped = true;
   else
      return cERR_UNSUPPORTED_ORIENT;

   if (valueOfIntTagDef(cT_FILLORDER, 1) != 1)
      return cERR_UNSUPPORTED_FILLORDER;

   if (mPlanarConfig != 1)
      return cERR_UNSUPPORTED_PLANAR;

   mPredictor = valueOfIntTagDef(cT_PREDICTOR, 1);

   if (mPredictor != 1)
      return cERR_UNSUPPORTED_PREDICTOR;

   return cNO_ERROR;      
}



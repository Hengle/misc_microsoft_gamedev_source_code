//------------------------------------------------------------------------------
// writetga.cpp
// Simple TGA writer -- handles 24-bit truecolor, 8-bit greyscale
//------------------------------------------------------------------------------
#include "xcore.h"
#include "writeTGA.h"
//------------------------------------------------------------------------------
BTGAWriter::BTGAWriter()
{
   mpStream = NULL;
   mWidth = mHeight = mBytesPerPixel = mBytesPerLine = 0;
   mImageType = cTGAImageTypeInvalid;
}
//------------------------------------------------------------------------------
BTGAWriter::~BTGAWriter()
{
   close();
}
//------------------------------------------------------------------------------
bool BTGAWriter::close(void)
{
   mWidth = mHeight = mBytesPerPixel = mBytesPerLine = 0;
   mImageType = cTGAImageTypeInvalid;
   mpStream = NULL;

   return true;
}
//------------------------------------------------------------------------------
bool BTGAWriter::open(
   BStream& stream,
   uint width, uint height,
   BTGAImageType mImageType)
{
   close();

   mpStream = &stream;

   this->mWidth = width;
   this->mHeight = height;
   this->mImageType = mImageType;

   uchar TGAHeader[18];
   memset(TGAHeader, 0, sizeof(TGAHeader));

   bool backwardsFlag = false;

   TGAHeader[12] = (uchar)(width & 0xFF);
   TGAHeader[13] = (uchar)((width >> 8) & 0xFF);
   TGAHeader[14] = (uchar)(height & 0xFF);
   TGAHeader[15] = (uchar)((height >> 8) & 0xFF);
   TGAHeader[17] = backwardsFlag ? 0x00 : 0x20;

   switch (mImageType)
   {
      case cTGAImageTypeBGR:
      {
         TGAHeader[2] = 2;
         TGAHeader[16] = 24;
         mBytesPerPixel = 3;
         break;
      }
      case cTGAImageTypeBGRA:
      {
         TGAHeader[2] = 2;
         TGAHeader[16] = 32;
         mBytesPerPixel = 4;
         break;
      }
      case cTGAImageTypeGray:
      {
         TGAHeader[2] = 3;
         TGAHeader[16] = 8;
         mBytesPerPixel = 1;
         break;
      }
      default:
         BDEBUG_ASSERT(false);
   }

   mBytesPerLine = width * mBytesPerPixel;

   if (mpStream->writeBytes(TGAHeader, sizeof(TGAHeader)) < sizeof(TGAHeader))
      return false;

   return true;
}
//------------------------------------------------------------------------------
bool BTGAWriter::writeLine(const void* pScanLine)
{
   if (!mpStream)
      return false;

   if (mpStream->writeBytes(pScanLine, mBytesPerLine) != mBytesPerLine)
      return false;

   return true;
}
//------------------------------------------------------------------------------


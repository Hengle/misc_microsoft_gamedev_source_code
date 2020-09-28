//--------------------------------------------------------------------------------------------------------------------------------------------
// File: ImageUtils.cpp
//--------------------------------------------------------------------------------------------------------------------------------------------
#include "ximage.h"
#include "containers\dynamicarray.h"
#include "colorutils.h"
#include "RGBAImage.h"
#include "ImageUtils.h"
#include "pixelFormat.h"
#include "readTGA.h"
#include "readHDR.h"
#include "writeHDR.h"
#include "readTiff.h"
#include "math\halfFloat.h"

//--------------------------------------------------------------------------------------------------------------------------------------------
// BImageUtils::ditherImage
//--------------------------------------------------------------------------------------------------------------------------------------------
void BImageUtils::ditherImage(BRGBAImage& ditheredImage, const BRGBAImage& image, int channel)
{
   ditheredImage.setSize(image.getWidth(), image.getHeight(), image.getPitch());

   const uchar matrix5[2][2] = { { 0, 6 }, { 4, 2 } };
   const uchar matrix6[2][2] = { { 0, 3 }, { 2, 1 } };

   for (uint y = 0; y < ditheredImage.getHeight(); y++)
   {
      for (uint x = 0; x < ditheredImage.getWidth(); x++)
      {
         const BRGBAColor& c = image(x, y);
         
         int cr = (c.r * 31) / 255;
         int cg = (c.g * 63) / 255;
         int cb = (c.b * 31) / 255;

         int er = c.r - (cr * 255) / 31;
         int eg = c.g - (cg * 255) / 63;
         int eb = c.b - (cb * 255) / 31;

         if (er > matrix5[x&1][y&1])
            cr++;
         if (eg > matrix6[x&1][y&1])
            cg++;
         if (eb > matrix5[x&1][y&1]) 
            cb++;

         if (channel == -1)
         {
            ditheredImage(x, y).set((cr*255+15)/31, (cg*255+31)/63, (cb*255+15)/31, c.a);
         }
         else
         {
            ditheredImage(x,y) = image(x,y);
            
            if (channel == 0)
               ditheredImage(x, y)[0] = static_cast<uchar>((cr*255+15)/31);
            else if (channel == 1)
               ditheredImage(x, y)[1] = static_cast<uchar>((cg*255+31)/63);
            else if (channel == 2)
               ditheredImage(x, y)[2] = static_cast<uchar>((cb*255+15)/31);
         }            
      }
   }
}

//--------------------------------------------------------------------------------------------------------------------------------------------
// BImageUtils::readTGA
//--------------------------------------------------------------------------------------------------------------------------------------------
bool BImageUtils::readTGA(BStream& stream, BRGBAImage& image, BPixelFormat* pTGAFormat)
{
   BTGAReader tgaReader;
   if (!tgaReader.init(stream))
      return false;

   if (pTGAFormat)
      *pTGAFormat = tgaReader.getFormat();
      
   image.setSize(tgaReader.getWidth(), tgaReader.getHeight());
   
   BPixelFormat::BTranslator translator(tgaReader.getFormat(), G_abgr_format);

   for (uint y = 0; y < tgaReader.getHeight(); y++)
   {
      const void* pScanOfs;
      uint scanLen;
      
      if (!tgaReader.decode(pScanOfs, scanLen))
         return false;

      const uint dstY = tgaReader.getYFlipped() ? (tgaReader.getHeight() - 1 - y) : y;
      translator.translate(&image(0, dstY), pScanOfs, tgaReader.getWidth());
      
      if (tgaReader.getXFlipped())
      {
         for (uint x = 0; x < tgaReader.getWidth() / 2; x++)
         {
            BRGBAColor a(image(x, dstY));
            BRGBAColor b(image(tgaReader.getWidth() - 1 - x, dstY));
            image(x, dstY) = b;
            image(tgaReader.getWidth() - 1 - x, dstY) = a;
         }
      }
   }
  
   return true;
}

//--------------------------------------------------------------------------------------------------------------------------------------------
// BImageUtils::writeTGA
//--------------------------------------------------------------------------------------------------------------------------------------------
bool BImageUtils::writeTGA(BStream& stream, const BRGBAImage& image, BTGAImageType type, int channel)
{
   BDEBUG_ASSERT((channel >= -1) && (channel <= 3));
   
   BTGAWriter tgaWriter;
   if (!tgaWriter.open(stream, image.getWidth(), image.getHeight(), type))
      return false;

   BByteArray buf(tgaWriter.getBytesPerLine());

   for (uint y = 0; y < image.getHeight(); y++)
   {
      for (uint x = 0; x < image.getWidth(); x++)
      {
         int r, g, b, a;
         if (channel != -1)
            r = g = b = a = image(x, y)[channel];
         else
         {
            r = image(x, y).r;
            g = image(x, y).g;
            b = image(x, y).b;
            a = image(x, y).a;
         }

         switch (type)
         {
            case cTGAImageTypeBGRA:
            {
               buf[x * 4 + 0] = static_cast<uchar>(b);
               buf[x * 4 + 1] = static_cast<uchar>(g);
               buf[x * 4 + 2] = static_cast<uchar>(r);
               buf[x * 4 + 3] = static_cast<uchar>(a);
               break;
            }
            case cTGAImageTypeBGR:
            {
               buf[x * 3 + 0] = static_cast<uchar>(b);
               buf[x * 3 + 1] = static_cast<uchar>(g);
               buf[x * 3 + 2] = static_cast<uchar>(r);
               break;
            }
            case cTGAImageTypeGray:
            {
               if (channel == -1)
                  buf[x] = static_cast<uchar>(BRGBAColor(r, g, b, a).getGrayscale());
               else
                  buf[x] = static_cast<uchar>(g);
               break;
            }
            default:
            {
               BASSERT(0);
               break;
            }
         }            
      }
      if (!tgaWriter.writeLine(&buf[0]))
         return false;
   }
   
   if (!tgaWriter.close())
      return false;

   return true;
}

//--------------------------------------------------------------------------------------------------------------------------------------------
// BImageUtils::writeHDR
//--------------------------------------------------------------------------------------------------------------------------------------------
bool BImageUtils::writeHDR(BStream& stream, const BRGBAFImage& image)
{
   BHDRWriter hdrWriter;

   if (!hdrWriter.open(stream, image.getWidth(), image.getHeight()))
      return false;

   for (uint y = 0; y < image.getHeight(); y++)
   {
      if (!hdrWriter.writeLine(image.getScanlinePtr(y), BHDRWriter::cInputRGBF))
         return false;
   }
   
   return true;
}

//--------------------------------------------------------------------------------------------------------------------------------------------
// BImageUtils::writeHDR
//--------------------------------------------------------------------------------------------------------------------------------------------
bool BImageUtils::writeHDR(BStream& stream, const BRGBA16Image& image)
{
   BHDRWriter hdrWriter;

   if (!hdrWriter.open(stream, image.getWidth(), image.getHeight()))
      return false;
      
   BDynamicArray<BRGBAColorF> scanlineBuf(image.getWidth());      

   for (uint y = 0; y < image.getHeight(); y++)
   {
      const BRGBAColor16* pSrcScanline = image.getScanlinePtr(y);
      
      for (uint x = 0; x < image.getWidth(); x++)
      {
         BRGBAColorF c;
         for (uint i = 0; i < 4; i++)
            c[i] = HalfFloat::HalfToFloat(pSrcScanline[x][i], true);
         scanlineBuf[x] = c;
      }
   
      if (!hdrWriter.writeLine(scanlineBuf.getPtr(), BHDRWriter::cInputRGBF))
         return false;
   }

   return true;
}

//--------------------------------------------------------------------------------------------------------------------------------------------
// BImageUtils::readHDR
//--------------------------------------------------------------------------------------------------------------------------------------------
bool BImageUtils::readHDR(BStream& stream, BRGBAFImage& image)
{
   BHDRReader hdrReader;
   if (!hdrReader.init(stream))
      return false;

   image.setSize(hdrReader.getWidth(), hdrReader.getHeight());
   
   for (uint y = 0; y < hdrReader.getHeight(); y++)
   {
      const BRGBAColor* pScanOfs;
      uint scanLen;

      if (!hdrReader.decode(pScanOfs, scanLen))
         return false;

      const uint dstY = hdrReader.getYFlipped() ? (hdrReader.getHeight() - 1 - y) : y;
      
      for (uint x = 0; x < hdrReader.getWidth(); x++)
      {
         const uint dstX = hdrReader.getXFlipped() ? (hdrReader.getWidth() - 1 - x) : x;
         
         BHDRColorUtils::unpackRGBE(pScanOfs + x, &image(dstX, dstY), 1);
      }
   }

   return true;
}

//--------------------------------------------------------------------------------------------------------------------------------------------
// BImageUtils::readTiff
//--------------------------------------------------------------------------------------------------------------------------------------------
bool BImageUtils::readTiff(BStream& stream, BRGBAFImage& image)
{
   BTIFFReader tiffReader;
   if (!tiffReader.init(stream))
      return false;

   image.setSize(tiffReader.getWidth(), tiffReader.getHeight());

   for (uint y = 0; y < tiffReader.getHeight(); y++)
   {
      const BRGBAColorF* pScanOfs;

      if (!tiffReader.decode(pScanOfs))
         return false;

      const uint dstY = tiffReader.getYFlipped() ? (tiffReader.getHeight() - 1 - y) : y;

      for (uint x = 0; x < tiffReader.getWidth(); x++)
      {
         const uint dstX = tiffReader.getXFlipped() ? (tiffReader.getWidth() - 1 - x) : x;

         image(dstX, dstY) = pScanOfs[x];
      }
   }

   return true;
}

//--------------------------------------------------------------------------------------------------------------------------------------------
// BImageUtils::computeMSE
//--------------------------------------------------------------------------------------------------------------------------------------------
bool BImageUtils::computeErrorMetrics(BErrorMetrics& errorMetrics, const BRGBAImage& a, const BRGBAImage& b, uint firstChannel, uint numChannels, bool lumaOnly)
{
   if ((a.getWidth() != b.getWidth()) || (a.getHeight() != b.getHeight()) || (numChannels < 1) || (numChannels > 4) || (firstChannel > 3))
      return false;
   
   if ((!a.getWidth()) || (!a.getHeight()))
      return false;

   uint hist[256];
   Utils::ClearObj(hist);

   for (uint y = 0; y < a.getHeight(); y++)
   {
      for (uint x = 0; x < a.getWidth(); x++)
      {
         if (lumaOnly)
         {
            const uint ay = BColorUtils::RGBToY(a(x, y));
            const uint by = BColorUtils::RGBToY(b(x, y));
            int d = ay - by;
            if (d < 0)
               d = -d;
            hist[d]++;
         }
         else
         {
            for (uint c = 0; c < numChannels; c++)
            {
               int d = a(x, y)[firstChannel + c] - b(x, y)[firstChannel + c];
               if (d < 0)
                  d = -d;
               hist[d]++;
            }
         }            
      }
   }
   
   uint maxError = 0;
   uint total = 0;
   uint totalSq = 0;
   
   for (uint i = 0; i < 256; i++)
   {
      if (hist[i])      
      {
         maxError = i;
         total += i * hist[i];
         totalSq += i * i * hist[i];
      }
   }
   
   const uint totalSize = a.getWidth() * a.getHeight() * (lumaOnly ? 1 : numChannels);
   
   errorMetrics.clear();
   errorMetrics.mMaxError = maxError;
   errorMetrics.mMeanError = total / float(totalSize);
   errorMetrics.mMSE = totalSq / float(totalSize);
   errorMetrics.mRMSE = sqrt(errorMetrics.mMSE);
   errorMetrics.mPSNR = errorMetrics.mRMSE ? 20.0f * log10(255.0f / errorMetrics.mRMSE) : 0.0f; 
   
   return true;
}

//------------------------------------------------------------------------------------------------------
// BImageUtils::swizzleAR
//------------------------------------------------------------------------------------------------------
void BImageUtils::swizzleAR(BRGBAImage& dst, const BRGBAImage& src)
{
   if (&dst != &src)
      dst.setSize(src.getWidth(), src.getHeight());

   for (uint y = 0; y < src.getHeight(); y++)
   {
      for (uint x = 0; x < src.getWidth(); x++)
      {
         const BRGBAColor& color(src(x, y));
         dst(x, y) = BRGBAColor(color.a, color.g, color.b, color.r);
      }
   }               
}

//------------------------------------------------------------------------------------------------------
// BImageUtils::renormalizeImage
//------------------------------------------------------------------------------------------------------
void BImageUtils::renormalizeImage(BRGBAImage& dst, const BRGBAImage& src, bool fixZeroVectors)
{
   dst.setSize(src.getWidth(), src.getHeight());

   for (uint y = 0; y < src.getHeight(); y++)
   {
      for (uint x = 0; x < src.getWidth(); x++)
      {
         BRGBAColor color(src(x, y));

         if ((color.r == 128) && (color.g == 128) && (color.b == 128))
         {
            if (fixZeroVectors)
               dst(x, y) = BRGBAColor(128, 128, 255, color.a);
            else  
               dst(x, y) = BRGBAColor(128, 128, 128, color.a);
         }
         else
         {
            float r = Math::Clamp(2.0f * (color.r / 255.0f) - 1.0f, -1.0f, 1.0f);
            float g = Math::Clamp(2.0f * (color.g / 255.0f) - 1.0f, -1.0f, 1.0f);
            float b = Math::Clamp(2.0f * (color.b / 255.0f) - 1.0f, -1.0f, 1.0f);

            float l = sqrt(r * r + g * g + b * b);
            if (l < .075f)
            {
               // rg [12/27/06] - I am not completely sure this is the best thing to do, but it does eliminate lots of edge artifacts on resampled normal maps.
               color.r = 128;
               color.g = 128;
               color.b = 255;
            }
            // rg [12/27/06] - Only modify the color if it's too far from normalized.
            else if (fabs(l - 1.0f) > .075f)
            {
               if (l != 0.0f)
               {
                  l = 1.0f / l;
                  r *= l;
                  g *= l;
                  b *= l;
               }      
                           
               color.r = (uchar)Math::Clamp(floor(.5f + (r + 1.0f) * .5f * 255.0f), 0.0f, 255.0f);
               color.g = (uchar)Math::Clamp(floor(.5f + (g + 1.0f) * .5f * 255.0f), 0.0f, 255.0f);
               color.b = (uchar)Math::Clamp(floor(.5f + (b + 1.0f) * .5f * 255.0f), 0.0f, 255.0f);

               if ((color.r == 128) && (color.g == 128))
               {
                  if (color.b < 128)
                     color.b = 1;
                  else
                     color.b = 255;
               }
            }               

            dst(x, y) = color;
         }            
      }
   }
}
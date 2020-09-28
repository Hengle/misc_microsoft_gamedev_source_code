// File: hdrCodec.cpp
// rg [6/9/06] - Created
#include "compression.h"
#include "xcore.h"

#include "hdrCodec.h"
#include "hdrUtils.h"
#include "math\halfFloat.h"

#include "dxtUnpacker.h"
#include "dxtPacker.h"

float BHDRCodec::packFloatToDXT5HPass1(const BRGBAFImage& srcImage, BRGBAImage& alphaImage, float hdrScale, float maxHDRScale)
{
   const uint width = srcImage.getWidth();
   const uint height = srcImage.getHeight();

   if (hdrScale < 0.0f)
   {
      hdrScale = BHDRUtils::getLargestComponent(srcImage, 3);
      hdrScale = Math::Clamp(hdrScale, .00000125f, maxHDRScale);
   }

   alphaImage.setSize(width, height);

   for (uint y = 0; y < height; y++)
   {
      for (uint x = 0; x < width; x++)
      {
         const BRGBAColorF& src = srcImage(x, y);

         double r = Math::Max(0.0f, src[0]);
         double g = Math::Max(0.0f, src[1]);
         double b = Math::Max(0.0f, src[2]);

         double m = sqrt(Math::Clamp<double>(Math::Max3(r, g, b) / hdrScale, 0.0f, 1.0f));

         int imultiplier = Math::Clamp(Math::FloatToIntRound(static_cast<float>(m * 255.0f)), 0, 255);

         BRGBAColor c;
         c[0] = 0;
         c[1] = 0;
         c[2] = 0;
         c[3] = static_cast<uchar>(imultiplier);

         alphaImage(x, y) = c;
      }
   }
   
   return hdrScale;
}   

float BHDRCodec::packFloatToDXT5HPass2(const BRGBAFImage& srcImage, const BRGBAImage& unpackedAlphaImage, BRGBAImage& compImage, float hdrScale, float maxHDRScale)
{
   const uint width = srcImage.getWidth();
   const uint height = srcImage.getHeight();

   if (hdrScale < 0.0f)
   {
      hdrScale = BHDRUtils::getLargestComponent(srcImage, 3);
      hdrScale = Math::Clamp(hdrScale, .00000125f, maxHDRScale);
   }
   
   compImage.setSize(width, height);

   for (uint y = 0; y < height; y++)
   {
      for (uint x = 0; x < width; x++)
      {
         const BRGBAColorF& src = srcImage(x, y);

         double r = Math::Max(0.0f, src[0]);
         double g = Math::Max(0.0f, src[1]);
         double b = Math::Max(0.0f, src[2]);

         const int imultiplier = unpackedAlphaImage(x, y)[3];

         double multiplier = (imultiplier / 255.0f) + 1.0f/255.0f;
         multiplier = multiplier * multiplier * hdrScale;

         r /= multiplier;
         g /= multiplier;
         b /= multiplier;

         r = sqrt(1.0f - Math::Min<double>(r, 1.0f));
         g = sqrt(1.0f - Math::Min<double>(g, 1.0f));
         b = sqrt(1.0f - Math::Min<double>(b, 1.0f));

         BRGBAColor c;

         c[0] = static_cast<uchar>(Math::iClampToByte(Math::FloatToIntRound(static_cast<float>(r * 255.0f))));
         c[1] = static_cast<uchar>(Math::iClampToByte(Math::FloatToIntRound(static_cast<float>(g * 255.0f))));
         c[2] = static_cast<uchar>(Math::iClampToByte(Math::FloatToIntRound(static_cast<float>(b * 255.0f))));
         c[3] = static_cast<uchar>(imultiplier);

         compImage(x, y) = c;
      }
   }
   
   return hdrScale;
}   

float BHDRCodec::packFloatToDXT5HImage(const BRGBAFImage& srcImage, BByteArray& outStream, float hdrScale, float maxHDRScale)
{
   const uint width = srcImage.getWidth();
   const uint height = srcImage.getHeight();
   
   BRGBAImage alphaImage;
   hdrScale = packFloatToDXT5HPass1(srcImage, alphaImage, hdrScale, maxHDRScale);
      
   BDXTPacker packer;
   BByteArray packedAlpha;
   packer.pack(alphaImage, cDXT5A, cDXTQualityNormal, false, false, packedAlpha, true);

   BRGBAImage unpackedAlpha;
   BDXTUnpacker unpacker;
   unpacker;
   unpacker.unpack(unpackedAlpha, packedAlpha.getPtr(), cDXT5A, width, height);
   
   BRGBAImage compImage;
   hdrScale = packFloatToDXT5HPass2(srcImage, unpackedAlpha, compImage, hdrScale, maxHDRScale);
   
   packer.pack(compImage, cDXT5, cDXTQualityNormal, false, false, outStream, true);

   return hdrScale;
   
#if 0
   const uint width = srcImage.getWidth();
   const uint height = srcImage.getHeight();
   
   if (hdrScale < 0.0f)
   {
      hdrScale = BHDRUtils::getLargestComponent(srcImage, 3);
      hdrScale = Math::Clamp(hdrScale, .00000125f, maxHDRScale);
   }
   
   BRGBAImage compImage(width, height);
   
   for (uint y = 0; y < height; y++)
   {
      for (uint x = 0; x < width; x++)
      {
         const BRGBAColorF& src = srcImage(x, y);
         
         double r = Math::Max(0.0f, src[0]);
         double g = Math::Max(0.0f, src[1]);
         double b = Math::Max(0.0f, src[2]);

         double m = sqrt(Math::Clamp<double>(Math::Max3(r, g, b) / hdrScale, 0.0f, 1.0f));

         int imultiplier = Math::Clamp(Math::FloatToIntRound(static_cast<float>(m * 255.0f)), 0, 255);

         BRGBAColor c;
         c[0] = 0;
         c[1] = 0;
         c[2] = 0;
         c[3] = static_cast<uchar>(imultiplier);

         compImage(x, y) = c;
      }
   }
   
   BDXTPacker packer;
   BByteArray packedAlpha;
   packer.pack(compImage, cDXT5A, cDXTQualityNormal, false, false, packedAlpha, true);

   BRGBAImage unpackedAlpha;
   BDXTUnpacker unpacker;
   unpacker;
   unpacker.unpack(unpackedAlpha, packedAlpha.getPtr(), cDXT5A, width, height);

   for (uint y = 0; y < height; y++)
   {
      for (uint x = 0; x < width; x++)
      {
         const BRGBAColorF& src = srcImage(x, y);
         
         double r = Math::Max(0.0f, src[0]);
         double g = Math::Max(0.0f, src[1]);
         double b = Math::Max(0.0f, src[2]);

         int imultiplier = unpackedAlpha(x, y)[3];

         double multiplier = (imultiplier / 255.0f) + 1.0f/255.0f;
         multiplier = multiplier * multiplier * hdrScale;

         r /= multiplier;
         g /= multiplier;
         b /= multiplier;
         
         r = sqrt(1.0f - Math::Min<double>(r, 1.0f));
         g = sqrt(1.0f - Math::Min<double>(g, 1.0f));
         b = sqrt(1.0f - Math::Min<double>(b, 1.0f));

         BRGBAColor c(compImage(x, y));
         
         c[0] = static_cast<uchar>(Math::iClampToByte(Math::FloatToIntRound(static_cast<float>(r * 255.0f))));
         c[1] = static_cast<uchar>(Math::iClampToByte(Math::FloatToIntRound(static_cast<float>(g * 255.0f))));
         c[2] = static_cast<uchar>(Math::iClampToByte(Math::FloatToIntRound(static_cast<float>(b * 255.0f))));

         compImage(x, y) = c;
      }
   }

   packer.pack(compImage, cDXT5, cDXTQualityNormal, false, false, outStream, true);
   
   return hdrScale;
#endif   
}

void BHDRCodec::unpackDXT5HToFloatImage(const BRGBAImage& sourceImage, float hdrScale, BRGBAFImage& outImage)
{
   const uint width = sourceImage.getWidth();
   const uint height = sourceImage.getHeight();
   
   outImage.setSize(width, height);

   for (uint y = 0; y < height; y++)
   {
      for (uint x = 0; x < width; x++)
      {
         const BRGBAColor& src = sourceImage(x, y);

         BRGBAColorF d(src[0]/255.0f, src[1]/255.0f, src[2]/255.0f, src[3]/255.0f);
                  
         d[3] += 1.0f/255.0f;

         float multiplier = d[3] * d[3] * hdrScale;

         d[0] = Math::Clamp(1.0f - d[0] * d[0], 0.0f, 1.0f);
         d[1] = Math::Clamp(1.0f - d[1] * d[1], 0.0f, 1.0f);
         d[2] = Math::Clamp(1.0f - d[2] * d[2], 0.0f, 1.0f);
         
         d[0] *= multiplier;
         d[1] *= multiplier;
         d[2] *= multiplier;
         d[3] = 1.0f;

         outImage(x, y) = d;
      }         
   }
}

void BHDRCodec::unpackDXT5HToFloatImage(const BConstDataBuffer& sourceData, uint width, uint height, float hdrScale, BRGBAFImage& outImage)
{
   BRGBAImage compImage;
   
   BDXTUnpacker unpacker;
   unpacker;
   unpacker.unpack(compImage, sourceData.getPtr(), cDXT5, width, height);

   return unpackDXT5HToFloatImage(compImage, hdrScale, outImage);
}








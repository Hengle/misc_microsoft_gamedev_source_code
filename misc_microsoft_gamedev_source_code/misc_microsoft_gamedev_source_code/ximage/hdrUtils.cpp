// File: hdrUtils.cpp
// rg [6/9/06] - Created
#include "xcore.h"
#include "hdrUtils.h"
#include "math\halfFloat.h"

float BHDRUtils::getLargestComponent(const BRGBAFImage& srcImage, uint numComps)
{
   float hdrScale = -1e+20f;

   for (uint y = 0; y < srcImage.getHeight(); y++)
      for (uint x = 0; x < srcImage.getWidth(); x++)
         for (uint c = 0; c < numComps; c++)
            hdrScale = Math::Max(hdrScale, srcImage(x, y)[c]);

   return hdrScale;
}

void BHDRUtils::packFloatToHalfImage(const BRGBAFImage& srcImage, BRGBA16Image& dstImage, uint numComps, bool littleEndian)
{
   dstImage.setSize(srcImage.getWidth(), srcImage.getHeight());

   const ushort one = littleEndian ? HalfFloat::FloatToHalfLittleEndian(1.0f) : HalfFloat::FloatToHalfBigEndian(1.0f);
   
   for (uint y = 0; y < srcImage.getHeight(); y++)
      for (uint x = 0; x < srcImage.getWidth(); x++)
      {
         for (uint c = 0; c < numComps; c++)
            dstImage(x, y)[c] = littleEndian ? HalfFloat::FloatToHalfLittleEndian(srcImage(x, y)[c]) : HalfFloat::FloatToHalfBigEndian(srcImage(x, y)[c]);
         
         for (uint c = numComps; c < 3; c++)
            dstImage(x, y)[c] = 0;
         
         if (numComps < 4)
            dstImage(x, y)[3] = one;
      }
}

void BHDRUtils::unpackHalfFloatImage(const BRGBA16Image& srcImage, BRGBAFImage& dstImage, uint numComps, bool littleEndian)
{
   dstImage.setSize(srcImage.getWidth(), srcImage.getHeight());

   for (uint y = 0; y < srcImage.getHeight(); y++)
      for (uint x = 0; x < srcImage.getWidth(); x++)
      {
         for (uint c = 0; c < numComps; c++)
            dstImage(x, y)[c] = littleEndian ? HalfFloat::LittleEndianHalfToFloat((ushort)srcImage(x, y)[c]) : HalfFloat::BigEndianHalfToFloat((ushort)srcImage(x, y)[c]);
         
         for (uint c = numComps; c < 3; c++)
            dstImage(x, y)[c] = 0;
            
         if (numComps < 4)
            dstImage(x, y)[3] = 1.0f;            
      }
}

void BHDRUtils::convertRGB8ToFloatImage(const BRGBAImage& srcImage, BRGBAFImage& dstImage, float gamma)
{
   float convTab[256];
   float gammaTab[256];         
   for (uint i = 0; i < 256; i++)
   {
      convTab[i] = i / 255.0f;
      gammaTab[i] = pow(i / 255.0f, gamma);
   }

   const uint width = srcImage.getWidth();
   const uint height = srcImage.getHeight();
   
   dstImage.setSize(width, height);

   for (uint y = 0; y < height; y++)
   {
      for (uint x = 0; x < width; x++)
      {
         for (uint c = 0; c < 3; c++)
            dstImage(x, y)[c] = gammaTab[srcImage(x, y)[c]];

         dstImage(x, y)[3] = convTab[srcImage(x, y)[3]];
      }
   }
}

void BHDRUtils::convertFloatToRGB8Image(const BRGBAFImage& srcImage, BRGBAImage& dstImage, float gamma, float exposure)
{
   float scale = pow(2.0f, exposure);
   
   uchar gammaTab[2048];         
   for (uint i = 0; i < 2048; i++)
      gammaTab[i] = static_cast<uchar>(Math::Clamp(Math::FloatToIntRound(pow(i / 2047.0f, 1.0f / gamma) * 255.0f), 0, 255));

   const uint width = srcImage.getWidth();
   const uint height = srcImage.getHeight();

   dstImage.setSize(width, height);

   for (uint y = 0; y < height; y++)
   {
      for (uint x = 0; x < width; x++)
      {
         for (uint c = 0; c < 3; c++)
         {
            float v = srcImage(x, y)[c] * scale;
            
            dstImage(x, y)[c] = gammaTab[ Math::Clamp(Math::FloatToIntRound(v * 2047.0f), 0, 2047) ];
         }

         dstImage(x, y)[3] = static_cast<uchar>(Math::Clamp(Math::FloatToIntRound(srcImage(x, y)[3] * 255.0f), 0, 255));
      }
   }
}

void BHDRUtils::convertRGB8ToHalfFloatImage(const BRGBAImage& srcImage, BRGBA16Image& dstImage, float gamma, bool littleEndian)
{
   BRGBAFImage floatImage;
   
   convertRGB8ToFloatImage(srcImage, floatImage, gamma);
   
   packFloatToHalfImage(floatImage, dstImage, 4, littleEndian);
}

void BHDRUtils::convertHalfFloatToRGB8Image(const BRGBA16Image& srcImage, BRGBAImage& dstImage, float gamma, float exposure, bool littleEndian)
{
   BRGBAFImage floatImage;
   
   unpackHalfFloatImage(srcImage, floatImage, 4, littleEndian);
   
   convertFloatToRGB8Image(floatImage, dstImage, gamma, exposure);
}








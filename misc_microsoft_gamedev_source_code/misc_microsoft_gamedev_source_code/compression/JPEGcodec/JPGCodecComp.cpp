// File: JPGCodecComp.cpp
#include "compression.h"
#include "xcore.h"
#include "containers\dynamicarray.h"
#include "colorutils.h"
#include "RGBAImage.h"
#include <algorithm>
#include "math\generalvector.h"
#include "DXTUtils.h"
#include "bytepacker.h"
#include "jpgcodec.h"
#include "jpgencode\jpge.h"

namespace 
{
   class BMemoryStream : public JPEG_Encoder::Stream
   {
      BByteArray& m_data;

   public:
      BMemoryStream(BByteArray& buf) : m_data(buf)
      {
      }
         
      virtual bool status(void) 
      { 
         return false; 
      }

      virtual void put(const void* Pbuf, int len)
      {
         m_data.pushBack(reinterpret_cast<const uchar*>(Pbuf), len);
      }

      int getDataSize()
      {
         return m_data.size();
      }
   };
} // anonymous namespace   

bool BJPEGCodec::compressImageJPEG(const BRGBAImage& image, BByteArray& stream, uint quality, eFormat format, bool visualQuant, int greyscaleChannel)
{
   JPEG_Encoder::Params params;
   params.quality = quality;
   params.two_pass_flag = true;
   params.aq_setting = 0;
   params.visual_quant = visualQuant;

   int numChannels = 0;
   switch (format)
   {
      case cH1V1:
      {
         params.subsampling = JPEG_Encoder::H1V1;
         numChannels = 3;
         break;
      }
      case cH1V1_RGB:
      {
         params.subsampling = JPEG_Encoder::H1V1_RGB;
         numChannels = 3;
         break;
      }
      case cH2V2:
      {
         params.subsampling = JPEG_Encoder::H2V2;
         numChannels = 3;
         break;
      }
      case cGreyscale:
      {
         params.subsampling = JPEG_Encoder::Y_ONLY;
         numChannels = 1;
         break;
      }
      default:
      {
         return false;
      }
   }
   
   BMemoryStream dstStream(stream);
   JPEG_Encoder dst_image(dstStream, params, image.getWidth(), image.getHeight(), numChannels);

   uint num_passes;
   if (params.two_pass_flag)
      num_passes = 2;
   else
      num_passes = 1;

   BByteArray buf(image.getWidth() * numChannels);
   
   for (uint pass = 0; pass < num_passes; pass++)
   {
      for (uint y = 0; y < image.getHeight(); y++)
      {
         const BRGBAColor* pSrc = &image(0, y);
         uchar* pDst = &buf[0];
         
         if (1 == numChannels)
         {
            if ((greyscaleChannel >= 0) && (greyscaleChannel <= 3))
            {
               for (uint x = 0; x < image.getWidth(); x++)
               {
                  pDst[0] = (*pSrc)[greyscaleChannel];
                  pSrc++;
                  pDst++;
               }
            }
            else
            {
               for (uint x = 0; x < image.getWidth(); x++)
               {
                  pDst[0] = static_cast<uchar>(BColorUtils::RGBToY(*pSrc));
                  pSrc++;
                  pDst++;
               }
            }               
         }
         else
         {
            for (uint x = 0; x < image.getWidth(); x++)
            {
               pDst[0] = pSrc->r;
               pDst[1] = pSrc->g;
               pDst[2] = pSrc->b;
               
               pSrc++;
               pDst += 3;
            }
         }            
         
         dst_image.scanline(&buf[0]);
      }

      dst_image.scanline(NULL);
   }
      
   return true;
}

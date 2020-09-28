#include "JPEGCodec\jpgmain.h"
#include <vector>
#include <algorithm>

#include "DXTMunge.h"
#include "JPEGCodec\jpgencode\jpge.h"

namespace 
{
   class BMemoryStream : public JPEG_Encoder::Stream
   {
      std::vector<uchar>& m_data;

   public:
      BMemoryStream(std::vector<uchar>& buf) : m_data(buf)
      {
      }
         
      virtual bool status(void) 
      { 
         return false; 
      }

      virtual void put(const void* Pbuf, int len)
      {
         m_data.insert(m_data.end(), reinterpret_cast<const char*>(Pbuf), reinterpret_cast<const char*>(Pbuf) + len);
      }

      int getDataSize()
      {
         return m_data.size();
      }
   };
} // anonymous namespace   

bool BDXTMunger::compressImageJPEG(const BRGBAImage& image, uint quality, bool subsampling, bool visualQuant, std::vector<uchar>& stream)
{
   JPEG_Encoder::Params params;
   params.quality = quality;
   params.subsampling = subsampling? JPEG_Encoder::H2V2 : JPEG_Encoder::H1V1;
   params.two_pass_flag = true;
   params.aq_setting = 0;
   params.visual_quant = visualQuant;

   const int NUM_CHANNELS = 3;
   BMemoryStream dstStream(stream);
   JPEG_Encoder dst_image(dstStream, params, image.getWidth(), image.getHeight(), NUM_CHANNELS);

   uint num_passes;
   if (params.two_pass_flag)
      num_passes = 2;
   else
      num_passes = 1;

   std::vector<uchar> buf(image.getWidth() * 3);
   
   for (uint pass = 0; pass < num_passes; pass++)
   {
      for (uint y = 0; y < image.getHeight(); y++)
      {
         const BRGBAColor* pSrc = &image(0, y);
         uchar* pDst = &buf[0];
         
         for (uint x = 0; x < image.getWidth(); x++)
         {
            pDst[0] = pSrc->r;
            pDst[1] = pSrc->g;
            pDst[2] = pSrc->b;
            
            pSrc++;
            pDst += 3;
         }
         
         dst_image.scanline(&buf[0]);
      }

      dst_image.scanline(NULL);
   }
      
   return true;
}
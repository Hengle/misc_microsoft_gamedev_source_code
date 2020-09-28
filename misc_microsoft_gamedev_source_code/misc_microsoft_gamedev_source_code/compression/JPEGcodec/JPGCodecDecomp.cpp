// JPGCodecDecomp.cpp
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
#include "jpgdecode\jpegdecoder.h"

namespace 
{
   class BJPEGDecoderVectorStream : public jpeg_decoder_stream
   {
      const uchar* mpBuf;
      uint mBufOfs;
      uint mBufSize;
            
   public:
      BJPEGDecoderVectorStream(const uchar* pBuf, int bufSize) : mpBuf(pBuf), mBufOfs(0), mBufSize(bufSize)
      {
      }
            
      virtual int read(uchar *Pbuf, int max_bytes_to_read, bool *Peof_flag)
      {
         int bytesToRead = max_bytes_to_read;
         if (max_bytes_to_read >= (int)(mBufSize - mBufOfs))
         {
            *Peof_flag = true;
            bytesToRead = mBufSize - mBufOfs;
         }
         
         Utils::FastMemCpy(Pbuf, mpBuf + mBufOfs, bytesToRead);
         
         mBufOfs += bytesToRead;
                  
         return (bytesToRead);
      }
      
      uint getOffset(void) const { return mBufOfs; }
   };
   
} // anonymous namespace   

bool BJPEGCodec::decompressImageJPEG(BRGBAImage& image, const uchar* pSrcData, uint srcDataSize, uint& srcDataRead, bool useH1V1RGB)
{
   BJPEGDecoderVectorStream* pInputStream = new BJPEGDecoderVectorStream(pSrcData, srcDataSize);
   
   Pjpeg_decoder Pd = new jpeg_decoder(pInputStream, false, useH1V1RGB);
      
   if (Pd->get_error_code())
      return false;

   image.setSize(Pd->get_width(), Pd->get_height());

#if 0   
   if ( ((static_cast<uint>(Pd->get_width()) != image.getWidth()) || (static_cast<uint>(Pd->get_height()) != image.getHeight())) )
   {
      delete Pd;
      delete pInputStream;
      
      return false;
   }
#endif   
   
   if (Pd->begin())
   {
      //printf("Error: Decoder failed! Error status: %i\n", Pd->get_error_code());

      delete Pd;
      delete pInputStream;
      
      return false;
   }
   
   uint lines_decoded = 0;

   for ( ; ; )
   {
      const void *Pscan_line_ofs;
      uint scan_line_len;

      if (Pd->decode(&Pscan_line_ofs, &scan_line_len))
         break;
      
      if (Pd->get_num_components() == 1)
      {
         const uchar* pSrc = reinterpret_cast<const uchar*>(Pscan_line_ofs);
         BRGBAColor* pDst = &image(0, lines_decoded);
         for (uint x = 0; x < image.getWidth(); x++)
         {
            pDst->r = pSrc[x];
            pDst->g = pSrc[x];
            pDst->b = pSrc[x];
            pDst->a = 255;
            pDst++;
         }
      }
      else
      {
         Utils::FastMemCpy(&image(0, lines_decoded), Pscan_line_ofs, image.getWidth() * 4);
      }
                        
      lines_decoded++;
   } 

   srcDataRead = pInputStream->getOffset();
   
   delete Pd;
   delete pInputStream;   
   
   if (lines_decoded != image.getHeight())
      return false;
         
   return true;
}

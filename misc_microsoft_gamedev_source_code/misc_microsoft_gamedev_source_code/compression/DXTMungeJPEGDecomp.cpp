#include "JPEGCodec\jpgmain.h"
#include <vector>
#include <algorithm>

#include "DXTMunge.h"
#include "JPEGCodec\jpgdecode\jpegdecoder.h"

namespace 
{
   class BJPEGDecoderVectorStream : public jpeg_decoder_stream
   {
      const std::vector<uchar>& mBuf;
      uint mBufOfs;
            
   public:
      BJPEGDecoderVectorStream(const std::vector<uchar>& buf, int bufOfs) : mBuf(buf), mBufOfs(bufOfs)
      {
      }
            
      virtual int read(uchar *Pbuf, int max_bytes_to_read, bool *Peof_flag)
      {
         uint bytesToRead = max_bytes_to_read;
         if (max_bytes_to_read >= static_cast<int>(mBuf.size() - mBufOfs))
         {
            *Peof_flag = true;
            bytesToRead = mBuf.size() - mBufOfs;
         }
         
         memcpy(Pbuf, &mBuf[mBufOfs], bytesToRead);
         mBufOfs += bytesToRead;
         
         return (bytesToRead);
      }
      
      uint getOffset(void) const { return mBufOfs; }
   };
   
} // anonymous namespace   

bool BDXTMunger::decompressImageJPEG(BRGBAImage& image, std::vector<uchar>& stream, uint streamOffset, uint& streamBytesRead)
{
   BJPEGDecoderVectorStream* pInputStream = new BJPEGDecoderVectorStream(stream, streamOffset);
   
   Pjpeg_decoder Pd = new jpeg_decoder(pInputStream, false);
   
   if ((Pd->get_width() != image.getWidth()) || (Pd->get_height() != image.getHeight()))
   {
      delete Pd;
      delete pInputStream;
      
      return false;
   }
   
   if (Pd->begin())
   {
      printf("Error: Decoder failed! Error status: %i\n", Pd->get_error_code());

      delete Pd;
      delete pInputStream;
      
      return false;
   }
   
   int lines_decoded = 0;

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
         memcpy(&image(0, lines_decoded), Pscan_line_ofs, image.getWidth() * 4);
      }
                        
      lines_decoded++;
   } 

   streamBytesRead = pInputStream->getOffset() - streamOffset;
   
   delete Pd;
   delete pInputStream;   
   
   if (lines_decoded != image.getHeight())
      return false;
         
   return true;
}

#include "compression.h"
#include "xcore.h"
#include "containers\dynamicarray.h"
#include "deflatecodec.h"
#include "inflate.h"
#include "deflate.h"
#include "hash\adler32.h"

bool BDeflateCodec::deflateData(const uchar* pData, uint dataLen, BByteArray& stream)
{
   if (dataLen & 0x80000000)
      return false;
   
   const uint streamOfs = stream.size();
         
   stream.pushBack(static_cast<uchar>(dataLen & 0xFF));
   stream.pushBack(static_cast<uchar>((dataLen>>8) & 0xFF));
   stream.pushBack(static_cast<uchar>((dataLen>>16) & 0xFF));
   stream.pushBack(static_cast<uchar>((dataLen>>24) & 0xFF));
   
   uint a = calcAdler32(pData, dataLen);
   stream.pushBack(static_cast<uchar>(a & 0xFF));
   stream.pushBack(static_cast<uchar>((a>>8) & 0xFF));
   stream.pushBack(static_cast<uchar>((a>>16) & 0xFF));
   stream.pushBack(static_cast<uchar>((a>>24) & 0xFF));
      
   BDeflate deflator;

   deflator.init();

   int dataOfs = 0;

   const int cOutBufSize = 2048;
   uchar outBuf[cOutBufSize];

   for ( ; ; )
   {
      int inBufSize = dataLen - dataOfs;

      int outBufSize = cOutBufSize;
      int status = deflator.compress(pData + dataOfs, &inBufSize, outBuf, &outBufSize, (inBufSize == 0) ? 1 : 0, 256, DEFL_ALL_BLOCKS, 0);

      stream.pushBack(outBuf, outBufSize);
      dataOfs += inBufSize;

      if (status == DEFL_STATUS_DONE)      
         break;
   }
   
   if ((stream.size() - streamOfs - 8) > dataLen)
   {
      stream[streamOfs + 3] |= 0x80;
      
      stream.resize(streamOfs + 8 + dataLen);
      Utils::FastMemCpy(&stream[streamOfs + 8], pData, dataLen);
   }

   return true;
}

bool BDeflateCodec::inflateData(const uchar* pData, uint dataLen, BByteArray& stream)
{
   if (dataLen < 8)
      return false;

   uint decompDataLen = pData[0] | (pData[1]<<8) | (pData[2]<<16) | (pData[3]<<24);
   const uint decompAdler32 = pData[4] | (pData[5]<<8) | (pData[6]<<16) | (pData[7]<<24);
   
   bool uncompressed = false;
   if (decompDataLen & 0x80000000)
   {
      decompDataLen &= 0x7FFFFFFF;
      uncompressed = true;
   }

#ifdef XBOX   
   // Fail on insane data sizes (probably a bad stream).
   // This is an arbitrary limit.
   if (decompDataLen > 250000000)
      return false;
#endif      
   
   const uint streamOfs = stream.size();
   stream.resize(streamOfs + decompDataLen);
//-- FIXING PREFIX BUG ID 6307
   const uchar* pDst = stream.getPtr() + streamOfs;
//--
   
   pData += 8;
   dataLen -= 8;
   
   if (uncompressed)
   {
      if (dataLen < decompDataLen)
         return false;
         
      Utils::FastMemCpy(&stream[streamOfs], pData, decompDataLen);
      
      if (calcAdler32(&stream[streamOfs], decompDataLen) != decompAdler32)
         return false;      
   }
   else
   {
      BInflate inflator;
      inflator.init();

      int outBufBytes = decompDataLen;
      int inBufBytes = dataLen;
      int status = inflator.decompress(pData, &inBufBytes, &stream[streamOfs], &outBufBytes, true, false);
            
      if (status != INFL_STATUS_DONE) 
         return false;

      if (static_cast<uint>(outBufBytes) != decompDataLen)
         return false;
         
      if (calcAdler32(pDst, outBufBytes) != decompAdler32)
         return false;
   }         
      
   return true;
}




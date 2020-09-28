// File: JPGCodec.h
#pragma once

class BJPEGCodec
{
public:
   enum eFormat
   {
      cH1V1,
      cH1V1_RGB,
      cH2V2,
      cGreyscale
   };
      
   static bool compressImageJPEG(const BRGBAImage& image, BByteArray& stream, uint quality = 85, eFormat format = cH2V2, bool visualQuant = true, int greyscaleChannel = -1);
   
   static bool decompressImageJPEG(BRGBAImage& image, const uchar* pSrcData, uint srcDataSize, uint& srcDataRead, bool useH1V1RGB = false);
};   
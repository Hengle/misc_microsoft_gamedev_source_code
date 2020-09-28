// File: EmbeddedDCTCodec.h
#pragma once

class BEmbeddedDCTCodec
{
public:
   static bool pack(const BRGBAImage& image, uint channel, float bitRate, BByteArray& stream);
   static void unpack(BRGBAImage& image, uint channel, const BByteArray& stream);
   
};

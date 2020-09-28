// File: deflatecodec.h
#pragma once

class BDeflateCodec
{
public:
   static bool deflateData(const uchar* pData, uint dataLen, BByteArray& stream);
   static bool inflateData(const uchar* pData, uint dataLen, BByteArray& stream);
};

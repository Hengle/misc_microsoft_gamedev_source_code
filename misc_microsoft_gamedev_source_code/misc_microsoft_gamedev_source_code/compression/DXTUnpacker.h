// File: DXTUnpacker.h
#pragma once

#include "dxtUtils.h"

class BDXTUnpacker
{
public:
   static bool unpack(BRGBAImage& image, const uchar* pDXTData, BDXTFormat dxtFormat, uint width, uint height);
};

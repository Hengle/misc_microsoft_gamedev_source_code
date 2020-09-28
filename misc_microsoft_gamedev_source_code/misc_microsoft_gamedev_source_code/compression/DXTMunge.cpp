// File: DXTMunge.cpp
#include "compression.h"
#include "xcore.h"
#include "bytepacker.h"
#include "containers\dynamicarray.h"
#include "colorutils.h"
#include "RGBAImage.h"
#include <algorithm>
#include "math\generalvector.h"
#include "dxtutils.h"
#include "containers\priorityQueue.h"
#include "dxtmunge.h"

bool BDXTMunger::decompress(uchar* pDXTData, const BDXTFormat& dxtFormat, uint width, uint height, const uchar* pSrcData, uint srcDataSize)
{
   if (srcDataSize < sizeof(BMungedDXTShortHeader))
      return false;

   if ((dxtFormat != cDXT1) && (dxtFormat != cDXT1A) && (dxtFormat != cDXT3))
      return false;

   if ((width & 3) || (height & 3))
      return false;
   
   const BMungedDXTShortHeader& header = *reinterpret_cast<const BMungedDXTShortHeader*>(pSrcData); 
   
   if (header.mHeaderSize < sizeof(BMungedDXTShortHeader))
      return false;
      
   if ((pSrcData[0] != BMungedDXTShortHeader::cMagic) && (pSrcData[0] != BMungedDXTHeader::cMagic))
      return false;
               
   if (header.mMethod == 1)
      return decompressMethod1(pDXTData, dxtFormat, width, height, pSrcData, srcDataSize);
   else if ((header.mMethod >= 2) && (header.mMethod <= 4))
      return decompressMethod2(pDXTData, dxtFormat, width, height, pSrcData, srcDataSize);
   
   return false;
}

bool BDXTMunger::imageHasAlphaData(const uchar* pSrcData, uint srcDataSize)
{
   if (srcDataSize < sizeof(BMungedDXTShortHeader))
      return false;

   const BMungedDXTShortHeader& header = *reinterpret_cast<const BMungedDXTShortHeader*>(pSrcData); 

   if ((pSrcData[0] != BMungedDXTShortHeader::cMagic) && (pSrcData[0] != BMungedDXTHeader::cMagic))
      return false;
      
   if (header.mHeaderSize < sizeof(BMungedDXTShortHeader))
      return false;
   
   return header.mAlphaBytes > 0;
}


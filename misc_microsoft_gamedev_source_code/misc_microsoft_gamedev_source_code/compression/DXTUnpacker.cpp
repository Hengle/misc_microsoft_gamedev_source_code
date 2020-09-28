// File: DXTUnpacker.cpp
#include "compression.h"
#include "xcore.h"
#include "containers\dynamicarray.h"
#include <algorithm>
#include "colorutils.h"
#include "RGBAImage.h"
#include "math\generalvector.h"
#include "DXTUtils.h"
#include "DXTUnpacker.h"

bool BDXTUnpacker::unpack(BRGBAImage& image, const uchar* pDXTData, BDXTFormat dxtFormat, uint width, uint height)
{
   if ((width < 1) || (height < 1))
      return false;
   
   if ((dxtFormat < cDXT1) || (dxtFormat > cDXN))
      return false;

   const uint cellsX = (width + cDXTBlockSizeMask) >> cDXTBlockSizeBits;
   const uint cellsY = (height + cDXTBlockSizeMask) >> cDXTBlockSizeBits;
   const uint bytesPerBlock = ((dxtFormat == cDXT1) || (dxtFormat == cDXT1A) || (dxtFormat == cDXT5A)) ? cDXT1BlockBytes : cDXT35NBlockBytes;
   const uint colorBlockOfs = (bytesPerBlock == cDXT1BlockBytes) ? 0 : cDXT1BlockBytes;
      
   BRGBAImage temp;   
   BRGBAImage* pDstImage = &image;
   if ((width & cDXTBlockSizeMask) || (height & cDXTBlockSizeMask))
      pDstImage = &temp;
   
   pDstImage->setSize((width + cDXTBlockSizeMask) & ~cDXTBlockSizeMask, (height + cDXTBlockSizeMask) & ~cDXTBlockSizeMask);
   
   bool hasColorBlock = true;
   if (dxtFormat == cDXN || dxtFormat == cDXT5A)
      hasColorBlock = false;
          
   for (uint cy = 0; cy < cellsY; cy++)
   {
      for (uint cx = 0; cx < cellsX; cx++)
      {
         BRGBAColor blockColors[cDXTColorSelectorValues];                  
         const BDXTUtils::BDXT1Cell& cell = *reinterpret_cast<const BDXTUtils::BDXT1Cell*>(pDXTData + (cx + cy * cellsX) * bytesPerBlock + colorBlockOfs);
         
         if (hasColorBlock)
         {
            WORD packedHighColor = cell.getColor0();
            WORD packedLowColor = cell.getColor1();
            
            BRGBAColor highColor, lowColor;
            lowColor.a = 255;
            highColor.a = 255;
            BColorUtils::unpackColor(packedHighColor, highColor, true);
            BColorUtils::unpackColor(packedLowColor, lowColor, true);
                        
            if (packedHighColor <= packedLowColor)
            {
               blockColors[0] = highColor;
               blockColors[1] = lowColor;
               blockColors[2] = BRGBAColor((lowColor.r + highColor.r) >> 1, (lowColor.g + highColor.g) >> 1, (lowColor.b + highColor.b) >> 1, 255);
               blockColors[3] = BRGBAColor(0, 0, 0, 0);
            }         
            else
            {
               blockColors[0] = highColor;
               blockColors[1] = lowColor;
               blockColors[2] = BRGBAColor((lowColor.r + highColor.r * 2) / 3, (lowColor.g + highColor.g * 2) / 3, (lowColor.b + highColor.b * 2) / 3, 255);
               blockColors[3] = BRGBAColor((lowColor.r * 2 + highColor.r) / 3, (lowColor.g * 2 + highColor.g) / 3, (lowColor.b * 2 + highColor.b) / 3, 255);
            }
         }            
                           
         if ((dxtFormat == cDXT1) || (dxtFormat == cDXT1A))
         {
            for (uint sy = 0; sy < cDXTBlockSize; sy++)
            {
               for (uint sx = 0; sx < cDXTBlockSize; sx++)
               {
                  const uint selector = cell.getSelector(sx, sy);
                  (*pDstImage)(cx * cDXTBlockSize + sx, cy * cDXTBlockSize + sy) = blockColors[selector];
               }
            }
         }
         else if (dxtFormat == cDXT3)
         {
            const BDXTUtils::BDXT3Cell& DXT3Cell = *reinterpret_cast<const BDXTUtils::BDXT3Cell*>(pDXTData + (cx + cy * cellsX) * 16);   
            
            for (uint sy = 0; sy < cDXTBlockSize; sy++)
            {
               for (uint sx = 0; sx < cDXTBlockSize; sx++)
               {
                  const uint selector = cell.getSelector(sx, sy);
                  BRGBAColor color(blockColors[selector]);
                  
                  color.a = DXT3Cell.getAlpha(sx, sy, true);
                  
                  (*pDstImage)(cx * cDXTBlockSize + sx, cy * cDXTBlockSize + sy) = color;
               }
            }
         }
         else if ((dxtFormat == cDXT5) || (dxtFormat == cDXN) || (dxtFormat == cDXT5A))
         {
            const uint numBlocks = (dxtFormat == cDXT5 || dxtFormat == cDXT5A) ? 1 : 2;
            for (uint blockIndex = 0; blockIndex < numBlocks; blockIndex++)
            {
               const BDXTUtils::BDXT5Cell& DXT5Cell = 
                  *reinterpret_cast<const BDXTUtils::BDXT5Cell*>(pDXTData + (cx + cy * cellsX) * bytesPerBlock + blockIndex * 8);   
               
               const uint alpha0 = DXT5Cell.mAlpha[0];
               const uint alpha1 = DXT5Cell.mAlpha[1];
               uint blockAlpha[8];
               if (alpha0 > alpha1)
               {
                  blockAlpha[0] = alpha0;
                  blockAlpha[1] = alpha1;
                  blockAlpha[2] = (6*alpha0 + 1*alpha1)/7;
                  blockAlpha[3] = (5*alpha0 + 2*alpha1)/7;
                  blockAlpha[4] = (4*alpha0 + 3*alpha1)/7;
                  blockAlpha[5] = (3*alpha0 + 4*alpha1)/7;
                  blockAlpha[6] = (2*alpha0 + 5*alpha1)/7;
                  blockAlpha[7] = (1*alpha0 + 6*alpha1)/7;
               }
               else
               {
                  blockAlpha[0] = alpha0;
                  blockAlpha[1] = alpha1;
                  blockAlpha[2] = (4*alpha0 + 1*alpha1)/5;
                  blockAlpha[3] = (3*alpha0 + 2*alpha1)/5;
                  blockAlpha[4] = (2*alpha0 + 3*alpha1)/5;
                  blockAlpha[5] = (1*alpha0 + 4*alpha1)/5;
                  blockAlpha[6] = 0;
                  blockAlpha[7] = 255;
               }
               
               for (uint sy = 0; sy < cDXTBlockSize; sy++)
               {
                  for (uint sx = 0; sx < cDXTBlockSize; sx++)
                  {
                     BRGBAColor color;
                     if (hasColorBlock)
                        color = blockColors[cell.getSelector(sx, sy)];
                     else if (blockIndex == 0)
                        color.set(0, 0);
                     else
                        color = (*pDstImage)(cx * cDXTBlockSize + sx, cy * cDXTBlockSize + sy);

                     const uint dstComponent = (dxtFormat == cDXT5 || dxtFormat == cDXT5A) ? 3 : blockIndex;
                        
                     color[dstComponent] = static_cast<uchar>(blockAlpha[DXT5Cell.getSelector(sx, sy)]);

                     (*pDstImage)(cx * cDXTBlockSize + sx, cy * cDXTBlockSize + sy) = color;
                  }
               }
            } // blockIndex
                           
         }            
      }
   }
   
   if ((width & cDXTBlockSizeMask) || (height & cDXTBlockSizeMask))      
   {
      image.setSize(width, height);
      
      for (uint y = 0; y < height; y++)
         for (uint x = 0; x < width; x++)
            image(x, y) = temp(x, y);
   }
 
   return true;
}

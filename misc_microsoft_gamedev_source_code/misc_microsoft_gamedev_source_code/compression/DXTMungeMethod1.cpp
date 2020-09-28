// File: DXTMungeMethod1.cpp
#include "compression.h"
#include "xcore.h"
#include "containers\dynamicarray.h"
#include <algorithm>
#include "colorutils.h"
#include "RGBAImage.h"
#include "math\generalvector.h"
#include "DXTUtils.h"
#include "bytepacker.h"
#include "containers\priorityQueue.h"
#include "dxtmunge.h"
#include "deflatecodec.h"
#include "JPEGCodec\jpgcodec.h"

bool BDXTMunger::compressMethod1(const BRGBAImage& image, bool hasAlpha, uint quality, BByteArray& stream)
{
   if (quality < 1) 
      quality = 85;
   else if (quality > 99)
      quality = 99;
      
   const uint width = image.getWidth();
   const uint height = image.getHeight();

   if ((width & 3) || (height & 3))
      return false;

   //const uint pitch = width * 3;
   const uint cellsX = width >> 2;
   const uint cellsY = height >> 2;
   const uint totalCells = cellsX * cellsY;

   BByteArray jpegData;

   const bool subsampling = true;
   const bool allowTransparentBlocks = true;

   bool success = BJPEGCodec::compressImageJPEG(image, jpegData, quality, subsampling ? BJPEGCodec::cH2V2 : BJPEGCodec::cH1V1);   
   if (!success)
      return false;

   BRGBAImage decompressedImage(width, height);
   uint streamBytesRead;
   success = BJPEGCodec::decompressImageJPEG(decompressedImage, &jpegData[0], jpegData.size(), streamBytesRead);
   if (!success)
      return false;

   BByteArray cellIndices(((cellsX + 1) >> 1) * cellsY);
   BByteArray cellTypes(totalCells);

   for (uint cy = 0; cy < cellsY; cy++)
   {
      for (uint cx = 0; cx < cellsX; cx++)
      {
         int bestUX = 0, bestUY = 0, bestTX = 0, bestTY = 0, bestType = 0;

         double bestVariance = 1e+30f;         
         for (int uy = 0; uy < 4; uy++)
         {
            for (int ux = 0; ux < 4; ux++)   
            {
               int r[4], g[4], b[4];

               const BRGBAColor& pixel = decompressedImage(cx * 4 + ux, cy * 4 + uy);

               r[3] = pixel.r;
               g[3] = pixel.g;
               b[3] = pixel.b;

               int furthestDist = -1;
               int trialTX = 0, trialTY = 0;
               for (int ty = 0; ty < 4; ty++)
               {
                  for (int tx = 0; tx < 4; tx++)   
                  {
                     const BRGBAColor& pixel = decompressedImage(cx * 4 + tx, cy * 4 + ty);
                     int dist = BColorUtils::colorDistanceWeighted(pixel.r, pixel.g, pixel.b, r[3], g[3], b[3]); 
                     if (dist > furthestDist)
                     {
                        furthestDist = dist;
                        trialTX = tx;
                        trialTY = ty;
                        r[0] = pixel.r;
                        g[0] = pixel.g;
                        b[0] = pixel.b;
                     }
                  }
               }

               for (int type = 0; type < (allowTransparentBlocks ? 2 : 1); type++)
               {
                  if (type == 0)
                  {
                     r[1] = (r[0] * 2 + r[3]) / 3;
                     g[1] = (g[0] * 2 + g[3]) / 3;
                     b[1] = (b[0] * 2 + b[3]) / 3;

                     r[2] = (r[3] * 2 + r[0]) / 3;
                     g[2] = (g[3] * 2 + g[0]) / 3;
                     b[2] = (b[3] * 2 + b[0]) / 3;
                  }
                  else
                  {
                     r[1] = (r[0] + r[3]) / 2;
                     g[1] = (g[0] + g[3]) / 2;
                     b[1] = (b[0] + b[3]) / 2;

                     r[2] = r[1];
                     g[2] = g[1];
                     b[2] = b[1];
                  }                           

                  double totalVariance = 0.0f;
                  int selectors[4][4];
                  for (int sy = 0; sy < 4; sy++)
                  {
                     for (int sx = 0; sx < 4; sx++)   
                     {
                        const BRGBAColor& pixel = decompressedImage(cx * 4 + sx, cy * 4 + sy);
                        int cr = pixel.r;
                        int cg = pixel.g;
                        int cb = pixel.b;

                        int bestDist = INT_MAX;
                        int bestI = 0;
                        for (int i = 0; i < 4; i++)
                        {
                           int trialDist = BColorUtils::colorDistanceWeighted(cr, cg, cb, r[i], g[i], b[i]);
                           if (trialDist < bestDist)
                           {
                              bestDist = trialDist;
                              bestI = i;
                           }
                        }

                        totalVariance += bestDist;
                        selectors[sx][sy] = bestI;
                     }
                  }

                  if (totalVariance < bestVariance)
                  {
                     bestVariance = totalVariance;

                     bestUX = ux;
                     bestUY = uy;
                     bestTX = trialTX;
                     bestTY = trialTY;

                     bestType = type;
                  }     

               } // type                        
            } // ux               
         }  // uy

         cellIndices[(cx >> 1) + cy * ((cellsX + 1) >> 1)] |= (bestUX + (bestUY << 2)) << (4 * (cx & 1));
         cellTypes[cx + cy * cellsX] = static_cast<uchar>(bestType);

      } // cx

      if ((cy & 3) == 0)
      {
         tracenocrlf("%i\n", cy);
      }

   } // cy   

   BByteArray compCellTypes;
   BDeflateCodec::deflateData(&cellTypes[0], totalCells, compCellTypes);

   {
      BByteArray decompCellTypes; //(totalCells);
      success = BDeflateCodec::inflateData(&compCellTypes[0], compCellTypes.size(), decompCellTypes);
      assert(success);
      if (!success)
         return false;

      bool equal = memcmp(&cellTypes[0], &decompCellTypes[0], totalCells) == 0;
      assert(equal);
      if (!equal)
         return false;
   }
   
   BByteArray compAlpha;
   if (hasAlpha)
   {
      BByteArray packedAlpha;
      packedAlpha.resize(totalCells * 8);
      for (uint cy = 0; cy < cellsY; cy++)
      {
         for (uint cx = 0; cx < cellsX; cx++)
         {
            BDXTUtils::BDXT3Cell& cell = *reinterpret_cast<BDXTUtils::BDXT3Cell*>(&packedAlpha[(cx + cy * cellsX) * 8]);
            
            for (uint sy = 0; sy < 4; sy++)
               for (uint sx = 0; sx < 4; sx++)
                  cell.setAlpha(sx, sy, image(cx * 4 + sx, cy * 4 + sy).a, true);
         }
      }
      
      BDeflateCodec::deflateData(&packedAlpha[0], packedAlpha.size(), compAlpha);
   }
   
   BMungedDXTHeader header;
   memset(&header, 0, sizeof(header));
   header.mShortHeader.mMagic = static_cast<BYTE>(BMungedDXTHeader::cMagic);
   header.mShortHeader.mHeaderSize = sizeof(header);
   header.mRequiredVersion = 1;
   header.mCreatedVersion = cDXTMungerVersion;
   header.mShortHeader.mWidthLog2 = static_cast<BYTE>(Math::iLog2(width));
   header.mShortHeader.mHeightLog2 = static_cast<BYTE>(Math::iLog2(height));
   header.mShortHeader.mMethod = 1;
   header.mJPEG0Bytes = jpegData.size();
   header.mCellIndicesBytes = cellIndices.size();
   header.mCellTypeBytes = compCellTypes.size();
   header.mShortHeader.mAlphaBytes = compAlpha.size();

   stream.pushBack((uchar*)&header, sizeof(header));
   if (jpegData.size()) 
      stream.pushBack(&jpegData[0], jpegData.size());
   if (cellIndices.size())
      stream.pushBack(&cellIndices[0], cellIndices.size());   
   if (compCellTypes.size())
      stream.pushBack(&compCellTypes[0], compCellTypes.size());
   if (compAlpha.size())
      stream.pushBack(&compAlpha[0], compAlpha.size());
   
   tracenocrlf("JPEG Size: %i, Cell Indices Size: %i, Cell Types Size: %i, Alpha Size: %i\n", jpegData.size(), cellIndices.size(), compCellTypes.size(), compAlpha.size());

   return true;
}

bool BDXTMunger::decompressMethod1(uchar* pDXTData, const BDXTFormat& dxtFormat, uint width, uint height, const uchar* pSrcData, uint srcDataSize)
{
   if (srcDataSize < sizeof(BMungedDXTHeader))
      return false;

   const BMungedDXTHeader& header = *reinterpret_cast<const BMungedDXTHeader*>(pSrcData); 

   if (BMungedDXTHeader::cMagic != header.mShortHeader.mMagic)
      return false;
    
    if (header.mShortHeader.mHeaderSize < sizeof(BMungedDXTHeader))
      return false;

   if ((width != (1U << header.mShortHeader.mWidthLog2)) || (height != (1U << header.mShortHeader.mHeightLog2)))
      return false;

   if (header.mRequiredVersion > cDXTMungerVersion) 
      return false;

   if (header.mShortHeader.mMethod != 1)
      return false;
      
   if ((dxtFormat != cDXT1) && (dxtFormat != cDXT1A) && (dxtFormat != cDXT3))
      return false;

   const uint cellsX = width >> 2;
   const uint cellsY = height >> 2;
   const uint totalCells = cellsX * cellsY;

   if (srcDataSize < header.mShortHeader.mHeaderSize + header.mJPEG0Bytes + header.mCellIndicesBytes + header.mCellTypeBytes) 
      return false;

   BRGBAImage decompressedImage(width, height);

   uint jpegStreamBytesRead = 0;
   bool success = BJPEGCodec::decompressImageJPEG(decompressedImage, pSrcData + header.mShortHeader.mHeaderSize, header.mJPEG0Bytes, jpegStreamBytesRead);
   if (!success)
      return false;

   LARGE_INTEGER startTime;
   LARGE_INTEGER endTime;
   QueryPerformanceCounter(&startTime);

   const uint cellsBytesPerRow = (cellsX + 1) >> 1;
   const uchar* pSelectorVectors = pSrcData + header.mShortHeader.mHeaderSize + header.mJPEG0Bytes;

   BByteArray decompCellTypes; //(totalCells);
   const uchar* pCompCellTypes = pSrcData + header.mShortHeader.mHeaderSize + header.mJPEG0Bytes + header.mCellIndicesBytes;
   success = BDeflateCodec::inflateData(pCompCellTypes, header.mCellTypeBytes, decompCellTypes);
   assert(success);
   if (!success)
      return false;
      
   BByteArray packedAlpha;
   
   if (dxtFormat != cDXT1)
   {
      if (header.mShortHeader.mAlphaBytes)
      {            
         const uchar* pCompAlpha = pCompCellTypes + header.mCellTypeBytes;
         success = BDeflateCodec::inflateData(pCompAlpha, header.mShortHeader.mAlphaBytes, packedAlpha);
         assert(success);
         if (!success)
            return false;
      }
      else
      {
         packedAlpha.resize(totalCells * 8);
         memset(&packedAlpha[0], 0xFF, packedAlpha.size());
      }
   }      
   
   uint bytesPerBlock = 8;
   uint colorBlockOfs = 0;
   
   if (dxtFormat == cDXT3)
   {
      bytesPerBlock = 16;
      colorBlockOfs = 8;
   }
   
   for (uint cy = 0; cy < cellsY; cy++)
   {
      for (uint cx = 0; cx < cellsX; cx++)
      {
         if (dxtFormat == cDXT3)
            memcpy(pDXTData + (cx + cy * cellsX) * 16, &packedAlpha[(cx + (cy * cellsX)) * 8], 8);
         
         BDXTUtils::BDXT1Cell& cell = *reinterpret_cast<BDXTUtils::BDXT1Cell*>(pDXTData + (cx + cy * cellsX) * bytesPerBlock + colorBlockOfs);
         uchar* pSelectors = &cell.mPixels0;

         int pixelU = (pSelectorVectors[(cx >> 1) + cy * cellsBytesPerRow] >> (4 * (cx & 1))) & 0xF;
         const BRGBAColor& highColor = decompressedImage(cx * 4 + (pixelU & 3), cy * 4 + (pixelU >> 2));
         WORD packedHighColor = BColorUtils::packColor(highColor.r, highColor.g, highColor.b, true);

         int pixelTX = 0;
         int pixelTY = 0;
         int furthestDist = -1;
         for (int sy = 0; sy < 4; sy++)
         {
            for (int sx = 0; sx < 4; sx++)
            {
               const BRGBAColor& color = decompressedImage(cx * 4 + sx, cy * 4 + sy);
               int dist = BColorUtils::colorDistanceWeighted(color.r, color.g, color.b, highColor.r, highColor.g, highColor.b);
               if (dist > furthestDist)
               {
                  furthestDist = dist;
                  pixelTX = sx;
                  pixelTY = sy;
               }
            }
         }

         const BRGBAColor& lowColor = decompressedImage(cx * 4 + pixelTX, cy * 4 + pixelTY);
         WORD packedLowColor = BColorUtils::packColor(lowColor.r, lowColor.g, lowColor.b, true);

         if (packedLowColor == packedHighColor)
         {
            int lr, lg, lb, hr, hg, hb;
            BColorUtils::unpackColor(packedLowColor, lr, lg, lb, false);
            BColorUtils::unpackColor(packedHighColor, hr, hg, hb, false);

            bool forceLow = false;
            if (hr != 31)
            {
               hr++;
               forceLow = true;
            }
            else if (hg != 63)
            {
               hg++;
               forceLow = true;
            }
            else if (hb != 31)
            {
               hb++;
               forceLow = true;
            }
            else if (lr != 0)
            {
               lr--;
            }
            else if (lg != 0)
            {
               lg--;
            }
            else if (lb != 0)
            {
               lb--;
            }
            else
            {
               assert(0);
            }

            cell.setColor0(BColorUtils::packColor(hr, hg, hb, false));
            cell.setColor1(BColorUtils::packColor(lr, lg, lb, false));

            assert(cell.getColor0() > cell.getColor1());

            if (forceLow)
               memset(pSelectors, 0x55, 4);
            else
               memset(pSelectors, 0x00, 4);
         }
         else 
         {
            int r[4], g[4], b[4], numColors;
//-- FIXING PREFIX BUG ID 6327
            const int* pSelectorTransTable;
//--

            if (decompCellTypes[cx + cy * cellsX])
            {
               // 3 color block
               if (packedHighColor > packedLowColor)
                  std::swap(packedHighColor, packedLowColor);

               BColorUtils::unpackColor(packedLowColor, r[0], g[0], b[0], true);
               BColorUtils::unpackColor(packedHighColor, r[2], g[2], b[2], true);

               r[1] = (r[0] + r[2]) >> 1;
               g[1] = (g[0] + g[2]) >> 1;
               b[1] = (b[0] + b[2]) >> 1;

               numColors = 3;

               static int selectorTransTable3[] = { 1, 2, 0 };
               pSelectorTransTable = selectorTransTable3;
            }
            else
            {
               // 4 color block
               if (packedHighColor < packedLowColor)
                  std::swap(packedHighColor, packedLowColor);

               BColorUtils::unpackColor(packedLowColor, r[0], g[0], b[0], true);
               BColorUtils::unpackColor(packedHighColor, r[3], g[3], b[3], true);

               r[1] = (r[0] * 2 + r[3]) / 3;
               g[1] = (g[0] * 2 + g[3]) / 3;
               b[1] = (b[0] * 2 + b[3]) / 3;

               r[2] = (r[3] * 2 + r[0]) / 3;
               g[2] = (g[3] * 2 + g[0]) / 3;
               b[2] = (b[3] * 2 + b[0]) / 3;

               numColors = 4;

               static int selectorTransTable4[] = { 1, 3, 2, 0 };
               pSelectorTransTable = selectorTransTable4;
            }

            cell.setColor0(packedHighColor);
            cell.setColor1(packedLowColor);

            memset(pSelectors, 0x00, 4);                                 

            for (int sy = 0; sy < 4; sy++)
            {
               for (int sx = 0; sx < 4; sx++)
               {
                  const BRGBAColor& pixel = decompressedImage(cx * 4 + sx, cy * 4 + sy);
                  int pr = pixel.r;
                  int pg = pixel.g;
                  int pb = pixel.b;

                  int bestDist = BColorUtils::colorDistanceWeighted(pr, pg, pb, r[0], g[0], b[0]);
                  int bestI = 0;

                  for (int i = 1; i < numColors; i++)
                  {
                     int dist = BColorUtils::colorDistanceWeighted(pr, pg, pb, r[i], g[i], b[i]);
                     if (dist < bestDist)
                     {
                        bestDist = dist;
                        bestI = i;
                     }
                  }

                  pSelectors[sy] |= (pSelectorTransTable[bestI] << (sx * 2));      
               }
            }
         }
      }
   }

   QueryPerformanceCounter(&endTime);

   LARGE_INTEGER freq;
   QueryPerformanceFrequency(&freq);

   DWORD time = static_cast<DWORD>(((*(unsigned __int64*)&endTime - *(unsigned __int64*)&startTime) * 1000) / *(unsigned __int64*)&freq);

   tracenocrlf("DXT1 Create Time: %ums\n", time);

   return true;
}
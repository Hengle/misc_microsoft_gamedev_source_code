// File: EmbeddedDCTCoder.cpp
// Experimental
#include "compression.h"
#include "xcore.h"
#include "containers\dynamicarray.h"
#include <algorithm>
#include "colorutils.h"
#include "RGBAImage.h"
#include "math\generalvector.h"
#include "math\generalmatrix.h"
#include "rangecoder.h"
#include "binaryentropycoder.h"
#include "bitcoder.h"

#include "ImageUtils.h"
#include "containers\bitarray2D.h"
#include "containers\dynamicarray2D.h"
#include "containers\staticArray2D.h"
#include "containers\staticBitArray2D.h"
#include "EmbeddedDCTCodec.h"
#include "DXTUtils.h"

void dct(int32 *data);
void idct(const int16 *data, uchar *Pdst_ptr, int block_max_zag);

namespace
{
   const float pi = 3.14159265f;
   const int FIX_SHIFT = 4;
   const float FIX_SCALE = 1 << FIX_SHIFT;
   const int c6 = (int)(FIX_SCALE * sqrt(2.0f) * cos(6.0f*pi/16.0f) + .5f);
   const int c2 = (int)(FIX_SCALE * sqrt(2.0f) * cos(2.0f*pi/16.0f) + .5f);
   const int hk = (int)(FIX_SCALE * sqrt(2.0f) + .5f);

   void dct4(const int* pX, int* pY)
   {
#if 0
      //1  1  1   1
      //1  1 -1  -1
      //k -k  0   0
      //0  0  k  -k
                             
      const int tX0 = pX[0];
      const int tX1 = pX[1];
      const int tX2 = pX[2];
      const int tX3 = pX[3];
      
      pY[0] = (tX0      + tX1       + tX2       + tX3) << FIX_SHIFT;
      pY[1] = (tX0      + tX1       - tX2       - tX3) << FIX_SHIFT;
      pY[2] = hk * tX0 - hk * tX1;
      pY[3] = hk * tX2 - hk * tX3;
#else
      const int tX0 = pX[0];
      const int tX1 = pX[1];
      const int tX2 = pX[2];
      const int tX3 = pX[3];

      pY[0] = (tX0      + tX1       + tX2       + tX3) << FIX_SHIFT;
      pY[1] =  tX0 * c2 + tX1 * c6  + tX2 * -c6 + tX3 * -c2;
      pY[2] = (tX0      - tX1       - tX2       + tX3) << FIX_SHIFT;
      pY[3] =  tX0 * c6 + tX1 * -c2 + tX2 * c2  + tX3 * -c6;
#endif      
   }

   void idct4(const int* pX, int* pY)
   {
#if 0
      //1  1  k 0
      //1  1 -k 0
      //1 -1  0 k
      //1 -1  0 -k
      
      const int tX0 = pX[0];
      const int tX1 = pX[1];
      const int tX2 = pX[2];
      const int tX3 = pX[3];
      
      pY[0] = tX0 + tX1 + ((tX2 * hk) >> FIX_SHIFT);
      pY[1] = tX0 + tX1 - ((tX2 * hk) >> FIX_SHIFT);
      pY[2] = tX0 - tX1 + ((tX3 * hk) >> FIX_SHIFT);
      pY[3] = tX0 - tX1 - ((tX3 * hk) >> FIX_SHIFT);
#else
      const int t10 = pX[0] + pX[2];
      const int t12 = pX[0] - pX[2];
      const int t2 = (c2 * pX[1] + c6 * pX[3]) >> FIX_SHIFT;
      const int t0 = (c6 * pX[1] - c2 * pX[3]) >> FIX_SHIFT;

      pY[0] = t10 + t2;
      pY[3] = t10 - t2;
      pY[1] = t12 + t0;
      pY[2] = t12 - t0;
#endif      
   }   

   struct BMatrix44I
   {
      int v[4][4];

      BMatrix44I() { }

      BMatrix44I(int i) 
      {
         for (uint y = 0; y < 4; y++)
            for (uint x = 0; x < 4; x++)
               v[x][y] = i;
      }

      int& operator() (uint i, uint j) { return v[i][j]; }
      int operator() (uint i, uint j) const { return v[i][j]; }

      void dct(void)
      {
         for (uint i = 0; i < 4; i++)
            dct4(&v[i][0], &v[i][0]);

         for (uint i = 0; i < 4; i++)
         {
            int r[4] = { v[0][i], v[1][i], v[2][i], v[3][i] };
            dct4(r, r);
            v[0][i] = (r[0] + (1 << (FIX_SHIFT + 1))) >> (FIX_SHIFT + 2);
            v[1][i] = (r[1] + (1 << (FIX_SHIFT + 1))) >> (FIX_SHIFT + 2);
            v[2][i] = (r[2] + (1 << (FIX_SHIFT + 1))) >> (FIX_SHIFT + 2);
            v[3][i] = (r[3] + (1 << (FIX_SHIFT + 1))) >> (FIX_SHIFT + 2);
         }
      }            

      void idct(void)
      {
         for (uint i = 0; i < 4; i++)
         {
            int r[4] = { v[0][i], v[1][i], v[2][i], v[3][i] };
            idct4(r, r);
            v[0][i] = r[0];
            v[1][i] = r[1];
            v[2][i] = r[2];
            v[3][i] = r[3];
         }

         for (uint i = 0; i < 4; i++)
         {
            idct4(&v[i][0], &v[i][0]);
            v[i][0] = (v[i][0] + (1 << (FIX_SHIFT + 1))) >> (FIX_SHIFT + 2);
            v[i][1] = (v[i][1] + (1 << (FIX_SHIFT + 1))) >> (FIX_SHIFT + 2);
            v[i][2] = (v[i][2] + (1 << (FIX_SHIFT + 1))) >> (FIX_SHIFT + 2);
            v[i][3] = (v[i][3] + (1 << (FIX_SHIFT + 1))) >> (FIX_SHIFT + 2);
         }

      }            
   };
   
   #define BLOCK_BITS 3U
   const uint BLOCK_SIZE = (1 << BLOCK_BITS);
   const uint BLOCK_MASK = BLOCK_SIZE - 1;
   
   const bool syncMarkers = false;

   struct BDiagnolScan
   {
      uchar num;
      uchar sx;
      uchar sy;
   };

   // HACK HACK
   BDiagnolScan gDiagnolScan[1024];
   uint gNumDiagnols;
  
   struct BZeroPred
   {
      char blockXOfs;
      char blockYOfs;
      char coeffXOfs;
      char coeffYOfs;
   };

   BZeroPred ACZeroPred[] = 
   {
      {  0,  0,  -1, -1 },
      {  0,  0,   0, -1 },
      {  0,  0,   1, -1 },
      {  0,  0,  -1,  0 },
      {  0,  0,  -1,  1 },
      {  0,  0,   0,  1 },
      { -1, -1,   0,  0 },
      {  0, -1,   0,  0 },
      { -1,  0,   0,  0 }
   };
   const uint numACZeroPred = sizeof(ACZeroPred)/sizeof(ACZeroPred[0]);

   BZeroPred DCZeroPred[] = 
   {
      { -1, -1 },
      {  0, -1 },
      {  1, -1 },

      { -1,  0 },
      {  1,  0 },

      { -1,  1 },
      {  0,  1 },
      {  1,  1 }
   };
   const uint numDCZeroPred = sizeof(DCZeroPred)/sizeof(DCZeroPred[0]);
           
   typedef short BCoeffType;           
   typedef BStaticArray2D<BCoeffType, BLOCK_SIZE, BLOCK_SIZE> BBlockMatrix;
   
   typedef BStaticBitArray2D<BLOCK_SIZE, BLOCK_SIZE> BBitMatrix;
         
} // anonymous namespace

bool BEmbeddedDCTCodec::pack(const BRGBAImage& image, uint channel, float bitRate, BByteArray& stream)
{  
   gNumDiagnols = 0;
   uint diagTotal = 0;
   for (uint i = 0; i < BLOCK_SIZE; i++)
   {
      gDiagnolScan[gNumDiagnols].num = static_cast<uchar>(i+1);
      diagTotal += i+1;
      gDiagnolScan[gNumDiagnols].sx = static_cast<uchar>(i);
      gDiagnolScan[gNumDiagnols++].sy = 0;
   }
   
   for (uint i = 1; i < BLOCK_SIZE; i++)
   {
      gDiagnolScan[gNumDiagnols].num = static_cast<uchar>(BLOCK_SIZE - i);
      diagTotal += BLOCK_SIZE - i;
      gDiagnolScan[gNumDiagnols].sx = static_cast<uchar>(BLOCK_SIZE - 1);
      gDiagnolScan[gNumDiagnols++].sy = static_cast<uchar>(i);
   }
   if (gNumDiagnols > sizeof(gDiagnolScan)/sizeof(gDiagnolScan[0]))
      DebugBreak();
      
   assert(diagTotal == BLOCK_SIZE*BLOCK_SIZE);
      
   const uint width = image.getWidth();
   const uint height = image.getHeight();
   const uint cellsX = width >> BLOCK_BITS;
   const uint cellsY = height >> BLOCK_BITS;
   const uint totalCells = cellsX * cellsY;
   
   if ((width < 1) || (height < 1) || (width & BLOCK_MASK) || (height & BLOCK_MASK))
      return false;
   
   BDynamicArray2D<BBlockMatrix> blocks(cellsX, cellsY);
   
   const BMatrixNxN<BLOCK_SIZE, BLOCK_SIZE> dctMatrix(BMatrixNxNCreateDCT< BMatrixNxN<BLOCK_SIZE, BLOCK_SIZE> >());
   
   for (uint cy = 0; cy < cellsY; cy++)
   {
      for (uint cx = 0; cx < cellsX; cx++)
      {
         BStaticArray2D<int32, BLOCK_SIZE, BLOCK_SIZE> temp;
         
         for (uint sy = 0; sy < BLOCK_SIZE; sy++)
            for (uint sx = 0; sx < BLOCK_SIZE; sx++)
               temp(sx, sy) = image(cx * BLOCK_SIZE + sx, cy * BLOCK_SIZE + sy)[channel] - 128;

         if (BLOCK_SIZE == 4)
         {
            ((BMatrix44I*)&temp)->dct();
         }
         else if (BLOCK_SIZE == 8)
         {               
            dct(&temp(0, 0));
         }
         else 
         {
            BMatrixNxN<BLOCK_SIZE, BLOCK_SIZE> tempf;
            
            for (uint sy = 0; sy < BLOCK_SIZE; sy++)
               for (uint sx = 0; sx < BLOCK_SIZE; sx++)
                  tempf[sx][sy] = static_cast<float>(temp(sx, sy));
            
            BMatrixNxN<BLOCK_SIZE, BLOCK_SIZE> tempfDCT(BMatrixNxNDCT< BMatrixNxN<BLOCK_SIZE, BLOCK_SIZE> >(tempf, dctMatrix));
            
            for (uint sy = 0; sy < BLOCK_SIZE; sy++)
               for (uint sx = 0; sx < BLOCK_SIZE; sx++)
                  temp(sx, sy) = Math::FloatToIntRound(tempfDCT[sx][sy] * 32.0f);
         }
         
         BBlockMatrix& blockMatrix = blocks(cx, cy);         
         for (uint sy = 0; sy < BLOCK_SIZE; sy++)
            for (uint sx = 0; sx < BLOCK_SIZE; sx++)
               blockMatrix(sx, sy) = static_cast<BCoeffType>(temp(sx, sy));                  
      } // cx
   } // cy

   double dcMeanD = 0;   
   for (uint blockIndex = 0; blockIndex < totalCells; blockIndex++)
      dcMeanD += blocks[blockIndex](0,0);
   dcMeanD /= totalCells;
   int dcMean = (int)dcMeanD;

   for (uint blockIndex = 0; blockIndex < totalCells; blockIndex++)
      blocks[blockIndex](0,0) = static_cast<BCoeffType>(blocks[blockIndex](0,0) - dcMean);

   int maxVal = 0;
   for (uint blockIndex = 0; blockIndex < totalCells; blockIndex++)
      for (uint sy = 0; sy < BLOCK_SIZE; sy++)
         for (uint sx = 0; sx < BLOCK_SIZE; sx++)
            maxVal = max(maxVal, labs(blocks[blockIndex](sx,sy)));

   uint size = 0;
   while (maxVal)
   {
      size++;
      maxVal >>= 1;
   }

   const int quant = size ? (1 << (size - 1)) : 0;
              
   uint numEOBBits = 0;
   uint numSignBits = 0;
   uint numZeroBits = 0;
   uint numRawBits = 0;
   uint totalEOFBits = 0;
   
   BBinaryEntropyCoder coder;
   coder.setBuf(&stream);
   coder.encodeStart();
   coder.encodeCode(cellsX, 10);
   coder.encodeCode(cellsY, 10);
   coder.encodeCode(quant & 0xFFFF, 16);
   coder.encodeCode(quant >> 16, 16);
   coder.encodeCode(dcMean & 0xFFFF, 16);
   coder.encodeCode((dcMean >> 16) & 0xFFFF, 16);
   
   BBitContexts EOBContexts(16384);
   BBitContexts ACZeroContexts(512);
   BBitContexts DCSignContexts(5);
   BBitContexts DCZeroContexts(256);
   
   const uint targetSize = static_cast<uint>((width * height * bitRate) / 8.0f);

   BBitArray2D<1> dcSigns(cellsX, cellsY);
      
   BDynamicArray2D<BBitMatrix> significant(cellsX, cellsY);
   
   int curQuant = quant;
   while (curQuant > 0)
   {
      BDynamicArray2D<BBitMatrix> newSignificant(significant);
      BBitArray2D<1> sentEOB(cellsX, cellsY);

      for (uint diagnol = 0; diagnol < gNumDiagnols; diagnol++)
      {
         if (syncMarkers)
            coder.encodeCode(0x1111, 16);

         // eob check
         if ((diagnol > 0) && (diagnol < gNumDiagnols - 1))
         {
            for (uint cy = 0; cy < cellsY; cy++)
            {
               for (uint cx = 0; cx < cellsX; cx++)
               {
                  //const uint blockIndex = cx + cy * cellsX;

                  if (sentEOB(cx, cy))
                     continue;

                  const BBlockMatrix& block = blocks(cx, cy);

                  bool foundSig = false;
                  for (uint probeDiagnol = diagnol; probeDiagnol < gNumDiagnols; probeDiagnol++)
                  {
                     uint sx = gDiagnolScan[probeDiagnol].sx;
                     uint sy = gDiagnolScan[probeDiagnol].sy;

                     for (uint n = 0; n < gDiagnolScan[probeDiagnol].num; n++, sx--, sy++)
                     {
                        if (!significant(cx, cy)(sx, sy))
                        {
                           if (labs(block(sx, sy)) >= curQuant)
                           {
                              foundSig = true;
                              break;
                           }
                        }
                     } // n

                     if (foundSig)
                        break;
                  }  // probeDiagnol 

                  const uint ul = sentEOB(Math::Clamp<int>(cx-1,0,cellsX-1), Math::Clamp<int>(cy-1,0,cellsY-1));
                  const uint u  = sentEOB(cx,                     Math::Clamp<int>(cy-1,0,cellsY-1));
                  const uint l  = sentEOB(Math::Clamp<int>(cx-1,0,cellsX-1), cy);
                  const uint bl = sentEOB(Math::Clamp<int>(cx-1,0,cellsX-1), Math::Clamp<int>(cy+1,0,cellsY-1));
                  const uint context = (diagnol * 16) + (ul + u * 2 + l * 4 + bl * 8);

                  numEOBBits++;

                  if (!foundSig)
                  {
                     sentEOB(cx, cy) = 1;

                     coder.encodeBit(EOBContexts[context], 1);
                  }
                  else
                  {
                     coder.encodeBit(EOBContexts[context], 0);
                  }
               } // cy
            } // cx
         } // if inner diagnol

         if (syncMarkers)
            coder.encodeCode(0xABCD, 16);

         uint sx = gDiagnolScan[diagnol].sx;
         uint sy = gDiagnolScan[diagnol].sy;
         for (uint n = 0; n < gDiagnolScan[diagnol].num; n++, sx--, sy++)
         {
            for (uint cy = 0; cy < cellsY; cy++)
            {
               for (uint cx = 0; cx < cellsX; cx++)
               {
                  //const uint blockIndex = cx + cy * cellsX;

                  if (sentEOB(cx, cy))
                     continue;

                  BBlockMatrix& block = blocks(cx, cy);
                  if (significant(cx, cy)(sx, sy))
                     continue;

                  if (syncMarkers)
                     coder.encodeCode(cx|(cy<<8), 16);

                  const bool DCFlag = (sx == 0) && (sy == 0);

                  uint zeroBitContext = 0;
                  if (DCFlag)
                  {
                     for (uint i = 0; i < numDCZeroPred; i++)
                     {
                        const int blockX = cx + DCZeroPred[i].blockXOfs;
                        const int blockY = cy + DCZeroPred[i].blockYOfs;

                        if ((blockX < 0) || (blockY < 0) || (blockX >= (int)cellsX) || (blockY >= (int)cellsY))
                           continue;

                        if (newSignificant(blockX, blockY)(0, 0))
                           zeroBitContext |= (1<<i);
                     }
                  }  
                  else
                  {
                     for (uint i = 0; i < numACZeroPred; i++)
                     {
                        const int blockX = cx + ACZeroPred[i].blockXOfs;
                        const int blockY = cy + ACZeroPred[i].blockYOfs;

                        if ((blockX < 0) || (blockY < 0) || (blockX >= (int)cellsX) || (blockY >= (int)cellsY))
                           continue;

                        const int x = sx + ACZeroPred[i].coeffXOfs;
                        const int y = sy + ACZeroPred[i].coeffYOfs;

                        if ((x < 0) || (y < 0) || (x > BLOCK_MASK) || (y > BLOCK_MASK))
                           continue;

                        if (newSignificant(blockX, blockY)(x, y))
                           zeroBitContext |= (1<<i);
                     }
                  }               

                  numZeroBits++;

                  if (syncMarkers)
                     coder.encodeCode(zeroBitContext, 16);

                  if (labs(block(sx, sy)) >= curQuant)
                  {
                     if (DCFlag)
                        coder.encodeBit(DCZeroContexts[zeroBitContext], 1);
                     else
                        coder.encodeBit(ACZeroContexts[zeroBitContext], 1);

                     numSignBits++;

                     const uchar signBit = (block(sx, sy) < 0) ? 1 : 0;
                     if (DCFlag)
                     {
                        uint numNegativeDC = 0;

                        if (cx > 0)
                        {
                           if (dcSigns(cx-1, cy))
                              numNegativeDC++;
                           
                           if (cy > 0)
                           {
                              if (dcSigns(cx-1,cy-1))
                                 numNegativeDC++;
                           }
                        }          

                        if (cy > 0)
                        {      
                           if (dcSigns(cx,cy-1))
                              numNegativeDC++;
                           
                           if (cx < (cellsX - 1))
                           {
                              if (dcSigns(cx+1,cy-1))
                                 numNegativeDC++;
                           }                                    
                        }

                        if (syncMarkers)
                           coder.encodeCode(numNegativeDC, 16);

                        coder.encodeBit(DCSignContexts[numNegativeDC], signBit);

                        if (signBit)
                           dcSigns(cx, cy) = 1;
                     }
                     else
                     {
                        // AC Sign
                        coder.encodeBit(signBit);
                     }

                     if (syncMarkers)
                     {
                        if (signBit)
                           coder.encodeCode(1, 1);
                        else
                           coder.encodeCode(0, 1);
                     }                                 

                     newSignificant(cx, cy)(sx, sy) = 1;
                  }
                  else
                  {
                     if (DCFlag)
                        coder.encodeBit(DCZeroContexts[zeroBitContext], 0);
                     else
                        coder.encodeBit(ACZeroContexts[zeroBitContext], 0);
                  }

               } // cx                
            } // cy
         } // n

         if (syncMarkers)
            coder.encodeCode(0xBAAD, 16);

         totalEOFBits++;
         if (coder.getBuf()->size() > targetSize)
         {
            coder.encodeBit(0);
            goto done;
         }
         coder.encodeBit(1);

      } // diagnol

      if (syncMarkers)
         coder.encodeCode(0x1234, 16);

      for (uint diagnol = 0; diagnol < gNumDiagnols; diagnol++)
      {     
         uint sx = gDiagnolScan[diagnol].sx;
         uint sy = gDiagnolScan[diagnol].sy;
         for (uint n = 0; n < gDiagnolScan[diagnol].num; n++, sx--, sy++)
         {
            for (uint cy = 0; cy < cellsY; cy++)
            {
               for (uint cx = 0; cx < cellsX; cx++)
               {
                  //const uint blockIndex = cx + cy * cellsX;

                  BBlockMatrix& block = blocks(cx, cy);
                  if ((!significant(cx, cy)(sx, sy)) && (newSignificant(cx, cy)(sx, sy)!=0))
                  {
                     if (block(sx, sy) < 0)
                        block(sx, sy) = static_cast<BCoeffType>(block(sx, sy) + curQuant);
                     else
                        block(sx, sy) = static_cast<BCoeffType>(block(sx, sy) - curQuant);
                  }
               } // cx
            } // cy
         } // n                                   
      } // diagnol         

      for (uint diagnol = 0; diagnol < gNumDiagnols; diagnol++)
      {     
         uint sx = gDiagnolScan[diagnol].sx;
         uint sy = gDiagnolScan[diagnol].sy;
         for (uint n = 0; n < gDiagnolScan[diagnol].num; n++, sx--, sy++)
         {
            for (uint blockIndex = 0; blockIndex < totalCells; blockIndex++)
            {
               BBlockMatrix& block = blocks[blockIndex];
               if (significant[blockIndex](sx, sy))
               {
                  numRawBits++;

                  if (labs(block(sx, sy)) >= curQuant)
                  {
                     coder.encodeBit(1);

                     if (block(sx, sy) < 0)
                        block(sx, sy) = static_cast<BCoeffType>(block(sx, sy) + curQuant);
                     else
                        block(sx, sy) = static_cast<BCoeffType>(block(sx, sy) - curQuant);
                  }
                  else
                  {
                     coder.encodeBit(0);
                  }
               }
            } // blockIndex
         } // n             

         totalEOFBits++;
         if (coder.getBuf()->size() > targetSize)
         {
            coder.encodeBit(0);
            goto done;
         }
         coder.encodeBit(1);  

      } // diagnol

      if (syncMarkers)
         coder.encodeCode(0x8856, 16);

      EOBContexts.reset();
      ACZeroContexts.reset();
      DCSignContexts.reset();
      DCZeroContexts.reset();

      significant = newSignificant; 

      curQuant >>= 1;
   }   

done:   

   coder.encodeCode(0x5566, 16);

   coder.encodeEnd();

   tracenocrlf("%i bytes, %f bits per block, %f bits per pixel\n", 
      coder.getBuf()->size(),
      (coder.getBuf()->size()*8.0f)/totalCells,
      (coder.getBuf()->size()*8.0f)/(width * height));

   return true;
}

void BEmbeddedDCTCodec::unpack(BRGBAImage& image, uint channel, const BByteArray& stream)
{
   BBinaryEntropyCoder coder;
   coder.setBuf(const_cast< BByteArray* >(&stream));
   coder.decodeStart();
      
   const uint cellsX = coder.decodeCode(10);
   const uint cellsY = coder.decodeCode(10);
   const uint totalCells = cellsX * cellsY;
   
   uint quant = coder.decodeCode(16);
   quant |= (coder.decodeCode(16) << 16);
   
   int dcMean = coder.decodeCode(16);
   dcMean |= (coder.decodeCode(16) << 16);

   BDynamicArray2D<BBlockMatrix> blocks(cellsX, cellsY);   
   blocks.setAll(BBlockMatrix(0));
   
   BBitContexts EOBContexts(16384);
   BBitContexts ACZeroContexts(512);
   BBitContexts DCSignContexts(5);
   BBitContexts DCZeroContexts(256);

   uint curQuant = quant;
   
   BDynamicArray2D<BBitMatrix> significant(cellsX, cellsY);
   
   curQuant = quant;
   while (curQuant > 0)
   {
      BDynamicArray2D<BBitMatrix> newSignificant(significant);
      BBitArray2D<1> gotEOB(cellsX, cellsY);

      for (uint diagnol = 0; diagnol < gNumDiagnols; diagnol++)
      {
         if (syncMarkers)
         {
            assert(0x1111 == coder.decodeCode(16));
         }

         if ((diagnol > 0) && (diagnol < gNumDiagnols - 1))
         {
            for (uint cy = 0; cy < cellsY; cy++)
            {
               for (uint cx = 0; cx < cellsX; cx++)
               {
                  //const uint blockIndex = cx + cy * cellsX;

                  if (!gotEOB(cx, cy))
                  {
                     const uint ul = gotEOB(Math::Clamp<int>(cx-1,0,cellsX-1), Math::Clamp<int>(cy-1,0,cellsY-1));
                     const uint u  = gotEOB(cx                    , Math::Clamp<int>(cy-1,0,cellsY-1));
                     const uint l  = gotEOB(Math::Clamp<int>(cx-1,0,cellsX-1), cy);
                     const uint bl = gotEOB(Math::Clamp<int>(cx-1,0,cellsX-1), Math::Clamp<int>(cy+1,0,cellsY-1));
                     const uint context = (diagnol * 16) + (ul + u * 2 + l * 4 + bl * 8);

                     const uint eob = coder.decodeBit(EOBContexts[context]);
                     if (eob)
                        gotEOB(cx,cy) = 1;
                  }               
               } // cx                  
            } // cy
         } // if inner diagnol

         if (syncMarkers)
         {
            assert(0xABCD == coder.decodeCode(16));
         }

         uint num = gDiagnolScan[diagnol].num;

         uint sx = gDiagnolScan[diagnol].sx;
         uint sy = gDiagnolScan[diagnol].sy;
         for (uint n = 0; n < num; n++, sx--, sy++)
         {
            for (uint cy = 0; cy < cellsY; cy++)
            {
               for (uint cx = 0; cx < cellsX; cx++)
               {
                  //const uint blockIndex = cx + cy * cellsX;

                  if (gotEOB(cx, cy))
                     continue;

                  BBlockMatrix& block = blocks(cx, cy);

                  if (significant(cx, cy)(sx, sy))
                     continue;

                  if (syncMarkers)   
                  {
                     assert(coder.decodeCode(16) == (cx|(cy<<8)));
                  }

                  const bool DCFlag = (sx == 0) && (sy == 0);

                  uint zeroBitContext = 0;
                  if (DCFlag)
                  {
                     for (uint i = 0; i < numDCZeroPred; i++)
                     {
                        const int blockX = cx + DCZeroPred[i].blockXOfs;
                        const int blockY = cy + DCZeroPred[i].blockYOfs;

                        if ((blockX < 0) || (blockY < 0) || (blockX >= (int)cellsX) || (blockY >= (int)cellsY))
                           continue;

                        if (newSignificant(blockX, blockY)(0, 0))
                           zeroBitContext |= (1<<i);
                     }
                  }  
                  else
                  {
                     for (uint i = 0; i < numACZeroPred; i++)
                     {
                        const int blockX = cx + ACZeroPred[i].blockXOfs;
                        const int blockY = cy + ACZeroPred[i].blockYOfs;

                        if ((blockX < 0) || (blockY < 0) || (blockX >= (int)cellsX) || (blockY >= (int)cellsY))
                           continue;

                        const int x = sx + ACZeroPred[i].coeffXOfs;
                        const int y = sy + ACZeroPred[i].coeffYOfs;

                        if ((x < 0) || (y < 0) || (x > BLOCK_MASK) || (y > BLOCK_MASK))
                           continue;

                        if (newSignificant(blockX, blockY)(x, y))
                           zeroBitContext |= (1<<i);
                     }
                  }               

                  if (syncMarkers)
                  {
                     assert(zeroBitContext == coder.decodeCode(16));
                  }

                  uint sigFlag;
                  if (DCFlag)
                     sigFlag = coder.decodeBit(DCZeroContexts[zeroBitContext]);
                  else
                     sigFlag = coder.decodeBit(ACZeroContexts[zeroBitContext]);

                  if (!sigFlag)
                     continue;

                  block(sx, sy) = static_cast<BCoeffType>(curQuant + (curQuant >> 1));

                  uint signFlag;
                  if (DCFlag)
                  {
                     uint numNegativeDC = 0;

                     if (cx > 0)
                     {
                        if (newSignificant(cx-1,cy)(0,0))
                        {
                           if (blocks(cx-1,cy)(0,0) < 0)
                              numNegativeDC++;
                        }

                        if (cy > 0)
                        {
                           if (newSignificant(cx-1,cy-1)(0,0))
                           {
                              if (blocks(cx-1,cy-1)(0,0) < 0)
                                 numNegativeDC++;
                           }
                        }
                     }          

                     if (cy > 0)
                     {      
                        if (newSignificant(cx,cy-1)(0,0))
                        {
                           if (blocks(cx,cy-1)(0,0) < 0)
                              numNegativeDC++;
                        }

                        if (cx < (cellsX - 1))
                        {
                           if (newSignificant(cx+1,cy-1)(0,0))
                           {
                              if (blocks(cx+1,cy-1)(0,0) < 0)
                                 numNegativeDC++;
                           }
                        }                                    
                     }

                     if (syncMarkers)
                     {
                        uint k = coder.decodeCode(16);
                        k;
                        assert(numNegativeDC == k);
                     }

                     signFlag = coder.decodeBit(DCSignContexts[numNegativeDC]);
                  }
                  else
                  {
                     signFlag = coder.decodeBit();
                  }

                  if (signFlag)
                     block(sx, sy) = -block(sx, sy);

                  if (syncMarkers)
                  {
                     uint k = coder.decodeCode(1);
                     if (k)
                     {
                        assert(block(sx, sy) < 0);
                     }
                     else
                     {
                        assert(block(sx, sy) >= 0);
                     }
                  }

                  newSignificant(cx,cy)(sx,sy) = 1;
               } // cx                  
            } // cy

         } // n

         if (syncMarkers)
         {
            assert(0xBAAD == coder.decodeCode(16));
         }  

         if (coder.decodeBit() == 0)
            goto done2;

      } // diagnol

      if (syncMarkers)
      {
         assert(0x1234 == coder.decodeCode(16));
      }

      for (uint diagnol = 0; diagnol < gNumDiagnols; diagnol++)
      {
         uint num = gDiagnolScan[diagnol].num;

         uint sx = gDiagnolScan[diagnol].sx;
         uint sy = gDiagnolScan[diagnol].sy;
         for (uint n = 0; n < num; n++, sx--, sy++)
         {
            for (uint blockIndex = 0; blockIndex < totalCells; blockIndex++)
            {
               BBlockMatrix& block = blocks[blockIndex];
               if (significant[blockIndex](sx, sy))
               {
                  uint rawBit = coder.decodeBit();
                  if (rawBit)
                  {
                     if (block(sx, sy) < 0)
                        block(sx, sy) = static_cast<BCoeffType>(block(sx, sy) - (curQuant >> 1));
                     else
                        block(sx, sy) = static_cast<BCoeffType>(block(sx, sy) + (curQuant >> 1));
                  }
                  else
                  {
                     if (block(sx, sy) < 0)
                        block(sx, sy) = static_cast<BCoeffType>(block(sx, sy) + (curQuant >> 1));
                     else
                        block(sx, sy) = static_cast<BCoeffType>(block(sx, sy) - (curQuant >> 1));
                  }
               }
            } 
         } // n            

         if (coder.decodeBit() == 0)
            goto done2;

      }  // diagnol

      if (syncMarkers)
      {
         assert(0x8856 == coder.decodeCode(16));
      }

      EOBContexts.reset();
      ACZeroContexts.reset();
      DCSignContexts.reset();
      DCZeroContexts.reset();

      significant = newSignificant; 

      curQuant >>= 1;
   }  

done2:   

   uint k = coder.decodeCode(16);
   if (0x5566 != k)
      DebugBreak();

   for (uint blockIndex = 0; blockIndex < totalCells; blockIndex++)
      blocks[blockIndex](0,0) = static_cast<BCoeffType>(blocks[blockIndex](0,0) + dcMean);

   image.setSize(cellsX * BLOCK_SIZE, cellsY * BLOCK_SIZE);
   
   const BMatrixNxN<BLOCK_SIZE, BLOCK_SIZE> dctMatrix(BMatrixNxNCreateDCT< BMatrixNxN<BLOCK_SIZE, BLOCK_SIZE> >());
   
   for (uint cy = 0; cy < cellsY; cy++)
   {
      for (uint cx = 0; cx < cellsX; cx++)
      {
         const BBlockMatrix& block = blocks(cx, cy);
                  
         uchar data[BLOCK_SIZE * BLOCK_SIZE];
         if (BLOCK_SIZE == 4)
         {
            BStaticArray2D<int32, BLOCK_SIZE, BLOCK_SIZE> temp;            
            for (uint sy = 0; sy < BLOCK_SIZE; sy++)
               for (uint sx = 0; sx < BLOCK_SIZE; sx++)
                  temp(sx, sy) = block(sx, sy);
            
            ((BMatrix44I*)&temp)->idct();
            
            for (uint sy = 0; sy < BLOCK_SIZE; sy++)
               for (uint sx = 0; sx < BLOCK_SIZE; sx++)
               {
                  int v = temp(sx, sy) + 128;
                  if (v < 0) v = 0; else if (v > 255) v = 255;
                  data[sx+sy*BLOCK_SIZE] = static_cast<uchar>(v);
               }
         }
         else if (BLOCK_SIZE == 8)
         {
            assert(sizeof(BCoeffType)==sizeof(int16));
            idct((int16*)&block(0,0), data, 64);
         }
         else 
         {
            BMatrixNxN<BLOCK_SIZE, BLOCK_SIZE> tempf;
            for (uint sy = 0; sy < BLOCK_SIZE; sy++)
               for (uint sx = 0; sx < BLOCK_SIZE; sx++)
                  tempf[sx][sy] = block(sx, sy) / 32.0f;
                        
            BMatrixNxN<BLOCK_SIZE, BLOCK_SIZE> tempfIDCT(BMatrixNxNIDCT< BMatrixNxN<BLOCK_SIZE, BLOCK_SIZE> >(tempf, dctMatrix));
            
            for (uint sy = 0; sy < BLOCK_SIZE; sy++)
               for (uint sx = 0; sx < BLOCK_SIZE; sx++)
               {
                  int v = (int)(tempfIDCT[sx][sy] + 128.0f);
                  if (v < 0) v = 0; else if (v > 255) v = 255;
                  data[sx+sy*BLOCK_SIZE] = static_cast<uchar>(v);
               }
         }
         
         for (uint sy = 0; sy < BLOCK_SIZE; sy++)
            for (uint sx = 0; sx < BLOCK_SIZE; sx++)
               image(cx*BLOCK_SIZE+sx, cy*BLOCK_SIZE+sy)[channel] = data[sx+sy*BLOCK_SIZE];
      } // cx
   } // cy
   
}


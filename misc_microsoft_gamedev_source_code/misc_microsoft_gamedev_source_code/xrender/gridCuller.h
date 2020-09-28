// File: gridCuller.h
#pragma once

template<uint cMaxObjects>
class BGridCuller
{
public:
   BGridCuller();
   ~BGridCuller();

   void init(float maxWorldCoord, float cellSize);
   void deinit(void);

   void clear(void);

   uint getGridWidth(void) const { return mGridWidth; }
   uint getGridHeight(void) const { return mGridHeight; }
   float getMaxWorldCoord(void) const { return mMaxWorldCoord; }
   float getCellSize(void) const { return mCellSize; }

   struct BCell
   {
      enum { cNumVecs = (cMaxObjects / 8) / 16 };
      XMVECTOR mBitFlags[cNumVecs];

      void clear(void)
      {
         Utils::FastMemSet(this, 0, sizeof(*this));
      }

      static void logicalOr(BCell* RESTRICT pResult, const BCell* RESTRICT pSrc)
      {
         for (uint i = 0; i < cNumVecs; i++)
            pResult->mBitFlags[i] = XMVectorOrInt(pResult->mBitFlags[i], pSrc->mBitFlags[i]);
      }
   };

   typedef XMVECTOR BGridCoord;
   typedef XMVECTOR BGridRect;

   BGridCoord getGridCoord(XMVECTOR point);

   const BCell& getCell(const BGridCoord coord) const;

   uint getCoordX(const BGridCoord coord) const { return reinterpret_cast<const uint*>(&coord)[0]; }
   uint getCoordY(const BGridCoord coord) const { return reinterpret_cast<const uint*>(&coord)[1]; }

   BGridRect getGridRect(XMVECTOR min, XMVECTOR max) const;
   
   BGridRect getGridRect(XMVECTOR center, float radius) const;
   
   BGridRect getGridRect(XMVECTOR centerRadius) const;

   uint getRectMinX(BGridRect rect) const { return reinterpret_cast<const uint*>(&rect)[0]; }
   uint getRectMinY(BGridRect rect) const { return reinterpret_cast<const uint*>(&rect)[1]; }
   uint getRectMaxX(BGridRect rect) const { return reinterpret_cast<const uint*>(&rect)[2]; }
   uint getRectMaxY(BGridRect rect) const { return reinterpret_cast<const uint*>(&rect)[3]; }

   void removeObject(const BGridRect rect, uint objectIndex);

   void addObject(const BGridRect rect, uint objectIndex);

   void prefetch(const BGridRect rect);

   const BCell& getCell(uint x, uint y) const   { BDEBUG_ASSERT((x < mGridWidth) && (y < mGridHeight)); return mCells[x + y * mGridWidth]; }
         BCell& getCell(uint x, uint y)         { BDEBUG_ASSERT((x < mGridWidth) && (y < mGridHeight)); return mCells[x + y * mGridWidth]; }

   void getAllObjects(BCell* RESTRICT pFlags, const BGridRect rect) const;
   
   template<class T>
   void getAllObjects(T* pResults, const BGridRect rect) const
   {
      BCell cell;
      cell.clear();
      getAllObjects(&cell, rect);

      const uint64* pVecs = reinterpret_cast<const uint64*>(&cell.mBitFlags[0]);

      for (uint i = 0; i < BCell::cNumVecs; i++)
      {
         uint64 l = ((const uint64*)pVecs)[i*2+0];
         uint64 h = ((const uint64*)pVecs)[i*2+1];
         
         if (l)
         {  
            do
            {
               uint lzc = _CountLeadingZeros64(l);
               pResults->pushBack( (ushort)(i * 128 + 63 - lzc) );
               l &= ~( ((uint64)1U) << (63U - lzc));
            } while (l);
         }
         
         if (h)
         {
            do
            {
               uint lzc = _CountLeadingZeros64(h);
               pResults->pushBack( (ushort)(i * 128 + 64 + 63 - lzc) );
               h &= ~( ((uint64)1U) << (63U - lzc));
            } while (h);
         }
                     
      }      
   }   

private:
   XMVECTOR mOOCellSize;
   XMVECTOR mMaxWorldCoordVec;

   typedef BDynamicArray<BCell, 128> BCellArray;
   BCellArray mCells;

   float mMaxWorldCoord;
   float mCellSize;
   uint mGridWidth;
   uint mGridHeight;
   uint mGridCells;
};

#include "gridCuller.inl"

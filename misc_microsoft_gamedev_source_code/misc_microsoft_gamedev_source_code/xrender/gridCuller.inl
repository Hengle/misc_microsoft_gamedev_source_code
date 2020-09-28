// File: gridCuller.inl

template<uint cMaxObjects>
inline BGridCuller<cMaxObjects>::BGridCuller() :
   mMaxWorldCoord(0.0f),
   mCellSize(0.0f),
   mGridWidth(0),
   mGridHeight(0),
   mGridCells(0)
{
   mOOCellSize = XMVectorZero();
   mMaxWorldCoordVec = XMVectorZero();
}

template<uint cMaxObjects>
inline BGridCuller<cMaxObjects>::~BGridCuller()
{
}

template<uint cMaxObjects>
inline void BGridCuller<cMaxObjects>::init(float maxWorldCoord, float cellSize)
{
   mMaxWorldCoord = ceil(maxWorldCoord / cellSize) * cellSize;
   mCellSize = cellSize;
   mGridWidth = (uint)ceil(mMaxWorldCoord / mCellSize);
   mGridHeight = (uint)ceil(mMaxWorldCoord / mCellSize);
   mOOCellSize = XMVectorReplicate(1.0f / mCellSize);
   mMaxWorldCoordVec = XMVectorReplicate(mMaxWorldCoord - .00125f);
   mGridCells = mGridWidth * mGridHeight;
   mCells.resize(mGridCells);
   
   clear();
}

template<uint cMaxObjects>
inline void BGridCuller<cMaxObjects>::deinit(void)
{
   mCells.clear();   
}

template<uint cMaxObjects>
inline void BGridCuller<cMaxObjects>::clear(void)
{
   Utils::FastMemSet(mCells.getPtr(), 0, mCells.size() * sizeof(BCell));
}

template<uint cMaxObjects>
inline typename BGridCuller<cMaxObjects>::BGridCoord BGridCuller<cMaxObjects>::getGridCoord(XMVECTOR point)
{
   XMVECTOR coord = XMVectorMax(coord, XMVectorZero());
   coord = XMVectorMin(coord, mMaxWorldCoordVec);
   coord = XMVectorMultiply(point, mOOCellSize);
   coord = XMConvertVectorFloatToUInt(coord, 0);
   return coord;
}

template<uint cMaxObjects>
inline const typename BGridCuller<cMaxObjects>::BCell& BGridCuller<cMaxObjects>::getCell(const BGridCoord coord) const 
{ 
   const uint* pCoord = reinterpret_cast<const uint*>(&coord);  
   BDEBUG_ASSERT((pCoord[0] < mGridWidth) && (pCoord[2] < mGridHeight));
   return mCells[pCoord[0] + pCoord[2] * mGridWidth]; 
}

template<uint cMaxObjects>
inline typename BGridCuller<cMaxObjects>::BGridRect BGridCuller<cMaxObjects>::getGridRect(XMVECTOR min, XMVECTOR max) const
{
   // 0123
   // 4567
   // 0246
   XMVECTOR coords = XMVectorPermute(min, max, XMVectorPermuteControl(0, 2, 4, 6));

   coords = XMVectorMax(coords, XMVectorZero());
   coords = XMVectorMin(coords, mMaxWorldCoordVec);

   coords = XMVectorMultiply(coords, mOOCellSize);
   coords = XMConvertVectorFloatToUInt(coords, 0);
   return coords;               
}

template<uint cMaxObjects>
inline typename BGridCuller<cMaxObjects>::BGridRect BGridCuller<cMaxObjects>::getGridRect(XMVECTOR center, float radius) const
{
   XMVECTOR vradius = XMVectorReplicate(center, radius);
   return getGridRect(XMVectorSubtract(center, vradius), XMVectorAdd(center, vradius));
}

template<uint cMaxObjects>
inline typename BGridCuller<cMaxObjects>::BGridRect BGridCuller<cMaxObjects>::getGridRect(XMVECTOR centerRadius) const
{
   XMVECTOR radius = XMVectorSplatW(centerRadius);
   return getGridRect(XMVectorSubtract(centerRadius, radius), XMVectorAdd(centerRadius, radius));
}

template<uint cMaxObjects>
inline void BGridCuller<cMaxObjects>::removeObject(const BGridRect rect, uint objectIndex)
{
   const uint* RESTRICT pBounds = reinterpret_cast<const uint*>(&rect);
   BDEBUG_ASSERT(pBounds[0] < mGridWidth && pBounds[1] < mGridHeight && pBounds[2] < mGridWidth && pBounds[3] < mGridHeight && pBounds[0] <= pBounds[2] && pBounds[1] <= pBounds[3]);

   BDEBUG_ASSERT(objectIndex < cMaxObjects);
   const uint qwordOfs = objectIndex >> 6;
   const uint64 bitMask = ~(((uint64)1U) << (objectIndex & 63U));

   for (uint y = pBounds[1]; y <= pBounds[3]; y++)
   {
      for (uint x = pBounds[0]; x <= pBounds[2]; x++)
      {
         BCell& cell = mCells[x + y * mGridWidth];

         uint64* pQWORDs = reinterpret_cast<uint64*>(&cell);

         pQWORDs[qwordOfs] &= bitMask;
      }
   }
}

template<uint cMaxObjects>
inline void BGridCuller<cMaxObjects>::addObject(const BGridRect rect, uint objectIndex)
{
   const uint* RESTRICT pBounds = reinterpret_cast<const uint*>(&rect);
   BDEBUG_ASSERT(pBounds[2] < mGridWidth && pBounds[3] < mGridHeight && pBounds[0] <= pBounds[2] && pBounds[1] <= pBounds[3]);

   BDEBUG_ASSERT(objectIndex < cMaxObjects);
   const uint qwordOfs = objectIndex >> 6;
   const uint64 bitMask = ((uint64)1U) << (objectIndex & 63U);

   for (uint y = pBounds[1]; y <= pBounds[3]; y++)
   {
      for (uint x = pBounds[0]; x <= pBounds[2]; x++)
      {
         BCell& cell = mCells[x + y * mGridWidth];

         uint64* pQWORDs = reinterpret_cast<uint64*>(&cell);

         pQWORDs[qwordOfs] |= bitMask;
      }
   }
}

template<uint cMaxObjects>
inline void BGridCuller<cMaxObjects>::prefetch(const BGridRect rect)
{
   const uint* RESTRICT pBounds = reinterpret_cast<const uint*>(&rect);
   BDEBUG_ASSERT(pBounds[2] < mGridWidth && pBounds[3] < mGridHeight && pBounds[0] <= pBounds[2] && pBounds[1] <= pBounds[3]);

   for (uint y = pBounds[1]; y <= pBounds[3]; y++)
   {
      for (uint x = pBounds[0]; x <= pBounds[2]; x++)
      {
         const BCell& cell = mCells[x + y * mGridWidth];

         __dcbt(0, &cell);
      }
   }
}

template<uint cMaxObjects>
inline void BGridCuller<cMaxObjects>::getAllObjects(BCell* RESTRICT pFlags, const BGridRect rect) const
{
   const uint* RESTRICT pBounds = reinterpret_cast<const uint*>(&rect);
   BDEBUG_ASSERT(pBounds[2] < mGridWidth && pBounds[3] < mGridHeight && pBounds[0] <= pBounds[2] && pBounds[1] <= pBounds[3]);

   for (uint y = pBounds[1]; y <= pBounds[3]; y++)
   {
      for (uint x = pBounds[0]; x <= pBounds[2]; x++)
      {
         const BCell& cell = mCells[x + y * mGridWidth];

         BCell::logicalOr(pFlags, &cell);
      }
   }   
}


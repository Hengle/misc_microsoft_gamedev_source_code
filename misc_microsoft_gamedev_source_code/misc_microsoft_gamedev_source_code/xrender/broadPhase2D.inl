// File: broadPhase2D.inl

template<uint cMaxObjects, uint cCellsPerAxis>
inline BBroadPhase2D<cMaxObjects, cCellsPerAxis>::BBroadPhase2D() :
   mMaxWorldCoord(0.0f),
   mCellSize(0.0f)
{
   mOOCellSize = XMVectorZero();
   mMaxWorldCoordVec = XMVectorZero();
}

template<uint cMaxObjects, uint cCellsPerAxis>
inline BBroadPhase2D<cMaxObjects, cCellsPerAxis>::~BBroadPhase2D()
{
}

template<uint cMaxObjects, uint cCellsPerAxis>
inline void BBroadPhase2D<cMaxObjects, cCellsPerAxis>::init(float maxWorldCoord, float cellSize)
{
   BDEBUG_ASSERT(maxWorldCoord <= cellSize * cCellsPerAxis);
   mMaxWorldCoord = ceil(maxWorldCoord / cellSize) * cellSize;
   mCellSize = cellSize;
   mOOCellSize = XMVectorReplicate(1.0f / mCellSize);
   mMaxWorldCoordVec = XMVectorReplicate(mMaxWorldCoord - .00125f);

   clear();
}

template<uint cMaxObjects, uint cCellsPerAxis>
inline void BBroadPhase2D<cMaxObjects, cCellsPerAxis>::deinit(void)
{
}

template<uint cMaxObjects, uint cCellsPerAxis>
inline void BBroadPhase2D<cMaxObjects, cCellsPerAxis>::clear(void)
{
   Utils::FastMemSet(mCellsX, 0, sizeof(mCellsX));
   Utils::FastMemSet(mCellsY, 0, sizeof(mCellsY));
}

template<uint cMaxObjects, uint cCellsPerAxis>
inline typename BBroadPhase2D<cMaxObjects, cCellsPerAxis>::BGridCoord BBroadPhase2D<cMaxObjects, cCellsPerAxis>::getGridCoord(XMVECTOR point)
{
   XMVECTOR coord = XMVectorMax(coord, XMVectorZero());
   coord = XMVectorMin(coord, mMaxWorldCoordVec);
   coord = XMVectorMultiply(point, mOOCellSize);
   coord = XMConvertVectorFloatToUInt(coord, 0);
   return coord;
}

template<uint cMaxObjects, uint cCellsPerAxis>
inline typename BBroadPhase2D<cMaxObjects, cCellsPerAxis>::BGridRect BBroadPhase2D<cMaxObjects, cCellsPerAxis>::getGridRect(XMVECTOR min, XMVECTOR max) const
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

template<uint cMaxObjects, uint cCellsPerAxis>
inline typename BBroadPhase2D<cMaxObjects, cCellsPerAxis>::BGridRect BBroadPhase2D<cMaxObjects, cCellsPerAxis>::getGridRect(XMVECTOR center, float radius) const
{
   XMVECTOR vradius = XMVectorReplicate(center, radius);
   return getGridRect(XMVectorSubtract(center, vradius), XMVectorAdd(center, vradius));
}

template<uint cMaxObjects, uint cCellsPerAxis>
inline typename BBroadPhase2D<cMaxObjects, cCellsPerAxis>::BGridRect BBroadPhase2D<cMaxObjects, cCellsPerAxis>::getGridRect(XMVECTOR centerRadius) const
{
   XMVECTOR radius = XMVectorSplatW(centerRadius);
   return getGridRect(XMVectorSubtract(centerRadius, radius), XMVectorAdd(centerRadius, radius));
}

template<uint cMaxObjects, uint cCellsPerAxis>
void BBroadPhase2D<cMaxObjects, cCellsPerAxis>::removeObject(const BGridRect rect, uint objectIndex)
{
   const uint* RESTRICT pBounds = reinterpret_cast<const uint*>(&rect);
   BDEBUG_ASSERT(pBounds[2] < cCellsPerAxis && pBounds[3] < cCellsPerAxis && pBounds[0] <= pBounds[2] && pBounds[1] <= pBounds[3]);
   BDEBUG_ASSERT(objectIndex < cMaxObjects);

   const uint qwordOfs = objectIndex >> 6;
   const uint64 bitMask = ~(((uint64)1U) << (objectIndex & 63U));

   for (uint x = pBounds[0]; x <= pBounds[2]; x++)
   {
      BCell& cell = mCellsX[x];
      uint64* pQWORDs = reinterpret_cast<uint64*>(&cell);
      pQWORDs[qwordOfs] &= bitMask;
   }

   for (uint y = pBounds[1]; y <= pBounds[3]; y++)
   {
      BCell& cell = mCellsY[y];
      uint64* pQWORDs = reinterpret_cast<uint64*>(&cell);
      pQWORDs[qwordOfs] &= bitMask;
   }
}

template<uint cMaxObjects, uint cCellsPerAxis>
void BBroadPhase2D<cMaxObjects, cCellsPerAxis>::addObject(const BGridRect rect, uint objectIndex)
{
   const uint* RESTRICT pBounds = reinterpret_cast<const uint*>(&rect);
   BDEBUG_ASSERT(pBounds[2] < cCellsPerAxis && pBounds[3] < cCellsPerAxis && pBounds[0] <= pBounds[2] && pBounds[1] <= pBounds[3]);
   BDEBUG_ASSERT(objectIndex < cMaxObjects);
   
   const uint qwordOfs = objectIndex >> 6;
   const uint64 bitMask = ((uint64)1U) << (objectIndex & 63U);

   for (uint x = pBounds[0]; x <= pBounds[2]; x++)
   {
      BCell& cell = mCellsX[x];
      uint64* pQWORDs = reinterpret_cast<uint64*>(&cell);
      pQWORDs[qwordOfs] |= bitMask;
   }
   
   for (uint y = pBounds[1]; y <= pBounds[3]; y++)
   {
      BCell& cell = mCellsY[y];
      uint64* pQWORDs = reinterpret_cast<uint64*>(&cell);
      pQWORDs[qwordOfs] |= bitMask;
   }
}

template<uint cMaxObjects, uint cCellsPerAxis>
void BBroadPhase2D<cMaxObjects, cCellsPerAxis>::getAllObjects(BCell* RESTRICT pFlags, const BGridRect rect) const
{
   const uint* RESTRICT pBounds = reinterpret_cast<const uint*>(&rect);
   BDEBUG_ASSERT(pBounds[2] < cCellsPerAxis && pBounds[3] < cCellsPerAxis && pBounds[0] <= pBounds[2] && pBounds[1] <= pBounds[3]);

   BCell flagsX;
   flagsX = mCellsX[pBounds[0]];
   for (uint x = pBounds[0] + 1; x <= pBounds[2]; x++)
      BCell::logicalOr(&flagsX, &mCellsX[x]);
   
   BCell flagsY;
   flagsY = mCellsY[pBounds[1]];
   for (uint y = pBounds[1] + 1; y <= pBounds[3]; y++)
      BCell::logicalOr(&flagsY, &mCellsY[y]);
      
   BCell::logicalAnd(pFlags, &flagsX, &flagsY);
}


// File: LBGQuantizer.cpp
#pragma once

template<typename BCellType>
class BNNSearchAccelerator
{
public:
   BNNSearchAccelerator() : 
      mpCells(NULL), 
      mNumCells(0)
   {
   }
         
   BNNSearchAccelerator(const BCellType* pCells, int numCells) :
      mpCells(pCells),
      mNumCells(numCells)
   {
      init();      
   }
   
   void init(const BCellType* pCells, int numCells)
   {
      mpCells = pCells;
      mNumCells = numCells;
      init();
   }
   
   uint findClosest(const BCellType& cell, int estimateHint, uint& numProbes, float& bestResultDist2) const
   {
      bestResultDist2 = 1e+30f;   
      uint bestResultIndex = 0;
      
      if (!mNumCells)
         return 0;
         
      const float cellDistToRef2 = cell.dist2(mRefVec.mVec);
      const float cellDistToRef = sqrt(cellDistToRef2);

      int low = 0;
      int high = mNumCells - 1;
      int bestIndex = 0;
      float bestDist = 1e+30f;
      
      if (-1 != estimateHint)
      {
         bestIndex = mRefVec.mCellLoc[estimateHint];
      }
      else
      {
         while (low <= high)
         {
            int mid = (low + high) >> 1;

            const float distToRef = mRefVec.mSortedCellRefs[mid].mDist;
            if (fabs(distToRef - cellDistToRef) < bestDist)
            {
               bestDist = fabs(distToRef - cellDistToRef);
               bestIndex = mid;
            }

            if (distToRef > cellDistToRef)
               high = mid - 1;
            else
               low = mid + 1;
         }
      }         

      const float cellDistToEstimate2 = cell.dist2(mpCells[mRefVec.mSortedCellRefs[bestIndex].mCellIndex]);
      const float cellDistToEstimate = sqrt(cellDistToEstimate2);

      const float lowRefDist = cellDistToRef - cellDistToEstimate;
      const float highRefDist = cellDistToRef + cellDistToEstimate;

      int curIndex = bestIndex;
      while ((curIndex >= 0) && (mRefVec.mSortedCellRefs[curIndex].mDist >= lowRefDist))
      {
         numProbes++;
         
         const uint probeIndex = mRefVec.mSortedCellRefs[curIndex].mCellIndex;
         const float probeDist2 = mpCells[probeIndex].dist2EarlyOut(cell, bestResultDist2);

         if (probeDist2 < bestResultDist2)
         {
            bestResultDist2 = probeDist2;
            bestResultIndex = probeIndex;
         }    
         
         curIndex--;
      }

      curIndex = bestIndex + 1;
      while ((curIndex < static_cast<int>(mNumCells)) && (mRefVec.mSortedCellRefs[curIndex].mDist <= highRefDist))
      {
         numProbes++;
         
         const uint probeIndex = mRefVec.mSortedCellRefs[curIndex].mCellIndex;
         const float probeDist2 = mpCells[probeIndex].dist2EarlyOut(cell, bestResultDist2);
         
         if (probeDist2 < bestResultDist2)
         {
            bestResultDist2 = probeDist2;
            bestResultIndex = probeIndex;
         }    
         
         curIndex++;
      }

      return bestResultIndex;
   }
   
private:
   const BCellType* mpCells;
   uint mNumCells;
   
   struct BCellRef
   {
      float mDist;
      uint mCellIndex;
      
      BCellRef() { }
      BCellRef(float dist, uint cellIndex) : mDist(dist), mCellIndex(cellIndex) { }
      
      void set(float dist, uint cellIndex)
      {  
         mDist = dist;
         mCellIndex = cellIndex;
      }
      
      bool operator< (const BCellRef& b) const
      {
         return mDist < b.mDist;
      }
   };
   
   struct BRefVec
   {
      BCellType mVec;
      typedef BDynamicArray<BCellRef> BCellRefArray;
      BCellRefArray mSortedCellRefs;
      BDynamicArray<ushort> mCellLoc;
   };
   
   BRefVec mRefVec;
   
   void init(void)
   {
      mRefVec.mVec.setZero();
      
      mRefVec.mSortedCellRefs.resize(mNumCells);
      
      for (uint i = 0; i < mNumCells; i++)
         mRefVec.mSortedCellRefs[i].set(sqrt(mRefVec.mVec.dist2(mpCells[i])), i);
      
      std::sort(&mRefVec.mSortedCellRefs[0], &mRefVec.mSortedCellRefs[0] + mNumCells);
      
      mRefVec.mCellLoc.resize(mNumCells);
      for (uint i = 0; i < mNumCells; i++)
         mRefVec.mCellLoc[mRefVec.mSortedCellRefs[i].mCellIndex] = static_cast<ushort>(i);
   }
};


template<typename BCellType, uint MaxCodebookSize>
class BLBGQuantizer 
{	
   struct BWeightedBCellType
   {
      BCellType cell;
      uint weight;
      BWeightedBCellType() { }
      BWeightedBCellType(const BCellType& c, uint w) : cell(c), weight(w) { }
   };

   uint mCodebookSize;
   BCellType mCodebook[MaxCodebookSize];

   typedef std::map<BCellType, uint> BCellTypeCont;
   mutable BCellTypeCont mHist;
   
   double mMSE;
   
   BNNSearchAccelerator<BCellType> mSearchAccel;
   
   bool mAllowRotations;
   bool mAllowFlips;
   
   static void debugPrintf(const char* pMsg, ...)
   {
      va_list args;
      va_start(args, pMsg);
      char buf[512];
      vsprintf_s(buf, sizeof(buf), pMsg, args);
      va_end(args);
      OutputDebugString(buf);
   }
   
   static void getInverseRotationFlip(int rot, bool xFlip, bool yFlip, int& inverseRot, bool& inverseXFlip, bool& inverseYFlip)
   {
      inverseRot = (4 - rot) & 3;
      
      if ((rot == 0) || (rot == 2))
      {
         inverseXFlip = xFlip;
         inverseYFlip = yFlip;
      }
      else //if ((rot == 1) || (rot == 3))
      {
         inverseXFlip = yFlip;
         inverseYFlip = xFlip;
      }
   }
   
   static uint encodeMatch(uint index, uint rot, bool xFlip, bool yFlip)
   {
      assert(!xFlip || !yFlip);
      
      const uint encoded = (index & 0xFFFF) | ((rot & 3) << 16) | ((xFlip & 1) << 18) | ((yFlip & 1) << 19);

#if 0
      {
         uint decodedRot;
         bool decodedXFlip;
         bool decodedYFlip;
         uint decodedIndex = decodeMatch(encoded, decodedRot, decodedXFlip, decodedYFlip);
         assert(decodedIndex == index);
         assert(decodedRot == rot);
         assert(decodedXFlip == xFlip);
         assert(decodedYFlip == yFlip);
      }
#endif      

      return encoded;
   }

   static uint decodeMatch(uint encoded, uint& rot, bool& xFlip, bool& yFlip)
   {
      uint index = encoded & 0xFFFF;
      rot = (encoded >> 16) & 3;
      xFlip = (encoded & (1 << 18)) != 0;
      yFlip = (encoded & (1 << 19)) != 0;
      return index;
   }
      
public:
   BLBGQuantizer(bool allowRotations = true, bool allowFlips = true) : 
      mCodebookSize(0), 
      mMSE(0.0f), 
      mAllowRotations(allowRotations), 
      mAllowFlips(allowFlips)
   {
      for (int i = 0; i < MaxCodebookSize; i++)
         mCodebook[i].setDefault();
   }
   
   void updateHistogram(const BCellType& cell, uint weight)
   {
      assert(weight > 0);
      std::pair<BCellTypeCont::iterator, bool> result = mHist.insert(BCellTypeCont::value_type(cell, 0));
      BCellTypeCont::iterator it = result.first;
      it->second = it->second + weight;	// update weight
   }
   
   void createCodebook(uint maxCodebookSize, const double minVariance = 0.0f)
   {
      if (maxCodebookSize > MaxCodebookSize)
         maxCodebookSize = MaxCodebookSize;
      
      if (!maxCodebookSize)
         return;
                     
      BDynamicArray<BWeightedBCellType> trainVecs;

      BCellType initial;
      initial.setZero();
      
      double codebookWeight = 0.0f;

      BCellTypeCont::iterator it;
      for (it = mHist.begin(); it != mHist.end(); it++)
      {
         const BCellType& cell = (*it).first;
         uint weight = (*it).second;

         assert(weight != 0);

         trainVecs.pushBack(BWeightedBCellType(cell, weight));

         initial += cell * float(weight);
         codebookWeight += weight;
      }

      if (codebookWeight == 0.0f)
         return;

      debugPrintf("%i unique training vecs\n", trainVecs.size());
      debugPrintf("total weight: %f\n", codebookWeight);

      BDynamicArray<BCellType> codebook(MaxCodebookSize);
      BDynamicArray<BCellType> tempCodebook(MaxCodebookSize);

      uint codebookSize;
      
      enum { cNumEstimateArrays = 4 * 3 };
      BDynamicArray< BDynamicArray<short> > hintEstimates(cNumEstimateArrays);
      for (int i = 0; i < cNumEstimateArrays; i++)
      {  
         hintEstimates[i].resize(trainVecs.size());
         std::fill(&hintEstimates[i][0], &hintEstimates[i][0] + trainVecs.size(), -1);
      }
      
      int furthestVecIndex[MaxCodebookSize];
      std::fill(furthestVecIndex, furthestVecIndex + MaxCodebookSize, -1);
      
      if (trainVecs.size() <= maxCodebookSize)
      {
         codebookSize = trainVecs.size();
         for (uint i = 0; i < trainVecs.size(); i++)
            codebook[i] = trainVecs[i].cell;
      }
      else
      {
         codebookSize = 1;
         codebook[0] = initial * float(1.0f / codebookWeight);
         codebook[0].normalize();
                  
         bool doneFlag = false;
                                    
         while ((codebookSize < maxCodebookSize) && (!doneFlag))
         {
            codebookSize *= 2;
            
            debugPrintf("codebookSize: %i\n", codebookSize);

            for (uint childIndex = 0; childIndex < codebookSize; childIndex++)
            {
               const uint parentIndex = childIndex >> 1;

               const float mul = (childIndex & 1) ? -1.0f : 1.0f;
               
               if (-1 == furthestVecIndex[parentIndex])
               {
                  tempCodebook[childIndex] = codebook[parentIndex].uniformPerturb(mul);
               }
               else
               {
                  BCellType dirToFurthest(trainVecs[furthestVecIndex[parentIndex]].cell - codebook[parentIndex]);
                  
                  tempCodebook[childIndex] = codebook[parentIndex] + dirToFurthest * (mul * .00125f);
               }
               
               tempCodebook[childIndex].normalize();
            }
            
            for (uint r = 0; r < cNumEstimateArrays; r++)
            {
               for (uint i = 0; i < trainVecs.size(); i++)
                  if (hintEstimates[r][i] != -1)
                     hintEstimates[r][i] = hintEstimates[r][i] * 2;
            }                     
                                    
            double totd1, totd2 = 10e+15f;
            int loopCounter = 0;

            for ( ; ; )
            {
               totd1 = 0.0f;
               
               BNNSearchAccelerator<BCellType> searchAccel(&tempCodebook[0], codebookSize);
               
               double totalWeight[MaxCodebookSize];
               double ttSum[MaxCodebookSize];
               for (uint i = 0; i < codebookSize; i++)
               {
                  codebook[i].setZero();
                  totalWeight[i] = 0.0f;
                  ttSum[i] = 0.0f;
               }
               
               float furthestVecDist[MaxCodebookSize];
               std::fill(furthestVecDist, furthestVecDist + MaxCodebookSize, 0.0f);
               std::fill(furthestVecIndex, furthestVecIndex + MaxCodebookSize, -1);

               uint totalProbes = 0;                              
               for (uint trainVecIndex = 0; trainVecIndex < trainVecs.size(); trainVecIndex++)
               {
                  const BCellType& trainVec = trainVecs[trainVecIndex].cell;
                  uint weight = trainVecs[trainVecIndex].weight;
                                 
                  float bestDist2 = 1e+30f;
                  int bestIndex = 0;
                  uint bestRotation = 0;

                  BCellType bestRotatedTrainVec;
                  for (uint f = 0; f < (mAllowFlips ? 2U : 1U); f++)
                  {
                     const bool xFlip = (f != 0);
                     const bool yFlip = false;
                  
                     for (uint r = 0; r < (mAllowRotations ? 4U : 1U); r++)
                     {
                        //debugPrintf("%i, %i, %i\n", r, xFlip, yFlip);
                     
                        BCellType rotatedTrainVec;
                        trainVec.rotated(rotatedTrainVec, r, xFlip, yFlip);
                                                                                                            
                        uint numProbes = 0;
                        float resultDist2;
                        uint resultIndex = searchAccel.findClosest(rotatedTrainVec, hintEstimates[r * 3 + f][trainVecIndex], numProbes, resultDist2);
                        totalProbes += numProbes;
                        hintEstimates[r * 3 + f][trainVecIndex] = static_cast<short>(resultIndex);

#if 0                        
                        {
                           int inverseR;
                           bool inverseXFlip, inverseYFlip;
                           getInverseRotationFlip(r, xFlip, yFlip, inverseR, inverseXFlip, inverseYFlip);
                           BCellType rotatedCodebookVec;
                           tempCodebook[bestIndex].rotated(rotatedCodebookVec, inverseR, inverseXFlip, inverseYFlip);
                           float e = rotatedCodebookVec.dist2(trainVec);
                           float f = rotatedTrainVec.dist2(tempCodebook[bestIndex]);
                           float g = tempCodebook[bestIndex].dist2(rotatedTrainVec);
                           assert(fabs(e-f) < .000125f);
                        }
#endif                        
                        
                        if (resultDist2 < bestDist2)
                        {
                           bestDist2 = resultDist2;
                           bestIndex = resultIndex;
                           bestRotation = r;
                           bestRotatedTrainVec = rotatedTrainVec;
                        }
                     }                     
                  }                     
                  
                  totalWeight[bestIndex] += weight;
                  codebook[bestIndex] += bestRotatedTrainVec * float(weight);
                  ttSum[bestIndex] += bestRotatedTrainVec.dot(bestRotatedTrainVec) * weight;
                  totd1 += tempCodebook[bestIndex].dist2(bestRotatedTrainVec) * weight;
                  
                  if (bestDist2 > furthestVecDist[bestIndex])
                  {
                     furthestVecDist[bestIndex] = bestDist2;
                     furthestVecIndex[bestIndex] = trainVecIndex;
                  }
               }

#ifdef BUILD_DEBUG               
               char buf[256];
               sprintf_s(buf, sizeof(buf), "%i vs. %i comps\n", totalProbes, trainVecs.size() * codebookSize * (mAllowFlips ? 2 : 1) * (mAllowRotations ? 4 : 1));
               OutputDebugString(buf);
#endif               
               
               double variance[MaxCodebookSize];
               double totalVariance = 0.0f;

               int unusedCount = 0;
               for (uint i = 0; i < codebookSize; i++)
               {
                  if (totalWeight[i] != 0.0f)
                  {
                     tempCodebook[i] = codebook[i] * float(1.0f / totalWeight[i]);
                     tempCodebook[i].normalize();

                     variance[i] = ttSum[i] - codebook[i].dot(codebook[i]) / totalWeight[i];
                        
                     totalVariance += variance[i];
                  }
                  else
                  {
                     unusedCount++;
                     variance[i] = -1e+30f;
                  }
               }

               if (unusedCount)
               {
                  debugPrintf("Warning: %i Empty cells -- reassigning\n", unusedCount);
                  for (uint i = 0; i < codebookSize; i++)
                  {
                     if (totalWeight[i] != 0.0f)
                        continue;

                     int maxJ = -1;
                     double maxVariance = -1e+30f;

                     uint j;
                     for (j = 0; j < codebookSize; j++)
                     {
                        if (variance[j] > maxVariance)
                        {
                           maxVariance = variance[j];
                           maxJ = j;
                        }
                     }

                     if (maxJ == -1)
                     {
                        double max_weight = -1e+30f;
                        for (j = 0; j < codebookSize; j++)
                        {
                           if (totalWeight[j] > max_weight)
                           {
                              max_weight = totalWeight[j];
                              maxJ = j;
                           }
                        }

                        if (maxJ == -1)
                        {
                           for (maxJ = 0; maxJ < static_cast<int>(codebookSize); maxJ++)
                              if (totalWeight[maxJ] == -1)
                                 break;
                        }
                        else
                           totalWeight[maxJ] = -1;
                     }
                     else
                        variance[maxJ] = -1e+30f;

                     assert(maxJ != -1);

                     tempCodebook[i] = tempCodebook[maxJ].randomPerturb();
                     tempCodebook[i].normalize();
                  }
               }

               totd1 /= codebookWeight; // mean squared distance
               double drel = (totd2 - totd1) / totd1;

               if (totalVariance <= minVariance)
               {
                  doneFlag = true;
                  break;
               }
                              
               totd2 = totd1;

               const double refineThreshold = .05f;	// percent
               if (drel < refineThreshold)
               {
                  debugPrintf("variance: %f totd2: %f totd1: %f drel: %f\n", totalVariance, totd2, totd1, drel);
                  break;
               }

               // should never happen, but just in case
               loopCounter++;
               const int LOOP_COUNTER_THRESH = 16;
               if (loopCounter >= LOOP_COUNTER_THRESH)
               {
                  debugPrintf("loopCounter >= LOOP_COUNTER_THRESH !\n");
                  break;
               }
            } // refinement loop

            memcpy(&codebook[0], &tempCodebook[0], sizeof(codebook[0]) * codebookSize);
         } 
      }

      mCodebookSize = codebookSize;

      for (uint i = 0; i < codebookSize; i++)
         mCodebook[i] = codebook[i];
         
      mMSE = 0.0f;
      
      mSearchAccel.init(&mCodebook[0], codebookSize);

      for (uint i = 0; i < trainVecs.size(); i++)
      {
         const BCellType& targetVec = trainVecs[i].cell;
         const uint weight = trainVecs[i].weight;
         
         it = mHist.find(targetVec);
         assert(it != mHist.end());
         if (it == mHist.end())
            continue;
                  
         float bestDist2 = 1e+30f;
         uint bestMatch = 0;
         uint bestR = 0;
         bool bestXFlip = false;
         bool bestYFlip = false;
         for (uint f = 0; f < (mAllowFlips ? 2U : 1U); f++)
         {
            const bool xFlip = (f != 0);
            const bool yFlip = false;
            
            for (uint r = 0; r < (mAllowRotations ? 4U : 1U); r++)
            {
               BCellType rotatedTargetVec;
               targetVec.rotated(rotatedTargetVec, r, xFlip, yFlip);
               
               uint numProbes = 0;
               float dist2;
               uint match = mSearchAccel.findClosest(rotatedTargetVec, hintEstimates[r * 3 + f][i], numProbes, dist2);
               if (dist2 < bestDist2)
               {
                  bestDist2 = dist2;
                  bestMatch = match;
                  bestR = r;
                  bestXFlip = xFlip;
                  bestYFlip = yFlip;
               }               
            }     
         }            
         
         int inverseR;
         bool inverseXFlip, inverseYFlip;
         getInverseRotationFlip(bestR, bestXFlip, bestYFlip, inverseR, inverseXFlip, inverseYFlip);
         
         (*it).second = encodeMatch(bestMatch, inverseR, inverseXFlip, inverseYFlip);
         
         BCellType rotatedCodebookCell;
         mCodebook[bestMatch].rotated(rotatedCodebookCell, inverseR, inverseXFlip, inverseYFlip);
         
         BCellType delta(rotatedCodebookCell - targetVec);
         mMSE += weight * delta.dot(delta);
      }
      
      mMSE = mMSE / codebookWeight;

      debugPrintf("Quantized MSE: %f\n", mMSE);
   }

   uint getCodebookSize(void) const
   {
      return mCodebookSize;
   }

   BCellType getCodebookEntry(uint entry) const
   {
      assert(entry < mCodebookSize);
      return mCodebook[entry];
   }

   uint findBestCodebookEntry(const BCellType& targetVec, uint& rot, bool& xFlip, bool& yFlip) const
   {
      BCellTypeCont::iterator it = mHist.find(targetVec);

      if (it == mHist.end())
      {
         std::pair<BCellTypeCont::iterator, bool> result = mHist.insert(BCellTypeCont::value_type(targetVec, 0));
         assert(result.second);
         it = result.first;
                           
         float bestDist2 = 1e+30f;
         uint bestMatch = 0;
         uint bestR = 0;
         bool bestXFlip = false;
         bool bestYFlip = false;
         for (uint f = 0; f < (mAllowFlips ? 2U : 1U); f++)
         {
            const bool xFlip = (f != 0);
            const bool yFlip = false;
            for (uint r = 0; r < (mAllowRotations ? 4U : 1U); r++)
            {
               BCellType rotatedTargetVec;
               targetVec.rotated(rotatedTargetVec, r, xFlip, yFlip);

               uint numProbes = 0;
               float dist2;
               uint match = mSearchAccel.findClosest(rotatedTargetVec, -1, numProbes, dist2);
               if (dist2 < bestDist2)
               {
                  bestDist2 = dist2;
                  bestMatch = match;
                  bestR = r;
                  bestXFlip = xFlip;
                  bestYFlip = yFlip;
               }               
            }     
         }            
         
         int inverseR;
         bool inverseXFlip, inverseYFlip;
         getInverseRotationFlip(bestR, bestXFlip, bestYFlip, inverseR, inverseXFlip, inverseYFlip);
                  
         it->second = encodeMatch(bestMatch, inverseR, inverseXFlip, inverseYFlip);
      }

      return decodeMatch(it->second, rot, xFlip, yFlip);
   }
      
};


//============================================================================
//
//  timelinesamplecodec.inl
//  
//  Copyright (c) 2005, Ensemble Studios
//
//============================================================================

//---------------------------------------------------------------------------------------------------
// BTimelineSampleCodec::BTimelineSampleCodec
//---------------------------------------------------------------------------------------------------
inline BTimelineSampleCodec::BTimelineSampleCodec() :
   mPrevLevel(-1),
   mPrevUserID(0),
   mPrevCPUStartTime(0),
   mPrevCPUEndTime(0),
   mPrevGPUStartTime(0),
   mPrevGPUEndTime(0),
   mPrevSectionID(0)
{
}

//---------------------------------------------------------------------------------------------------
// BTimelineSampleCodec::resetModel
//---------------------------------------------------------------------------------------------------
inline void BTimelineSampleCodec::resetModel(void)
{
   mPrevLevel = -1;
   mPrevUserID = 0;

   mPrevCPUStartTime = 0;
   mPrevCPUEndTime = 0;
   mPrevGPUStartTime = 0;
   mPrevGPUEndTime = 0;
   mPrevSectionID = 0;

   for (uint i = 0; i < mSectionPredDataArray.size(); i++)
      mSectionPredDataArray[i].clear();
}

//---------------------------------------------------------------------------------------------------
// BTimelineSampleCodec::codeSample
// false on failure
//---------------------------------------------------------------------------------------------------
template<typename BitPackerType>
inline bool BTimelineSampleCodec::codeSample(BitPackerType& bitPacker, const BRetiredProfileSample& sample, const BProfileSection* pSection)
{
   if (sample.mSectionID >= mSectionPredDataArray.size())
      mSectionPredDataArray.resize(sample.mSectionID + 1);

   const BCoderTime cpuStartTime = sample.mCPUStartScaledTime;
   const BCoderTime cpuEndTime = sample.mCPUEndScaledTime;
   BCoderTime gpuStartTime = 0;
   BCoderTime gpuEndTime = 0;
   if (!pSection->cpuOnly())
   {
      gpuStartTime = sample.mGPUStartScaledTime;
      gpuEndTime = sample.mGPUEndScaledTime;
   }
   BCoderTime cpuDuration = cpuEndTime - cpuStartTime;
   BCoderTime gpuDuration = gpuEndTime - gpuStartTime;

   if (-1 == mPrevLevel)
   {
      if (!mCoder.encodeOmega(bitPacker, sample.mSectionID))
         return false;
         
      // Model is empty - send uncompressed sample
      if (!mCoder.encodeOmega(bitPacker, sample.mLevel))              
         return false;

      if (!mCoder.encodeOmega(bitPacker, sample.mUserID))
         return false;

      if (!bitPacker.encode(cpuStartTime, 32))
         return false;         

      if (!bitPacker.encode(cpuEndTime, 32))
         return false;         

      if (!pSection->cpuOnly())
      {
         if (!bitPacker.encode(gpuStartTime, 32))
            return false;         

         if (!bitPacker.encode(gpuEndTime, 32))
            return false;         
      }
   }
   else
   {
      // Compress sample
      
      // Code the section ID delta
      int sectionIDDelta = sample.mSectionID - mSectionPredDataArray[mPrevSectionID].mNextSectionID;
      if (!mCoder.encodeOmegaSigned(bitPacker, sectionIDDelta))
         return false;
      
      // Code the level delta using 1 or 2 bits [-1, 0, or 1]
      const int levelDelta = sample.mLevel - mPrevLevel;
      if (!mCoder.encodeOmegaSigned(bitPacker, levelDelta))
         return false;
                        
      const int userIDDelta = sample.mUserID - mPrevUserID;
      if (userIDDelta == 0)
      {
         // Encode single bit to represent userIDDelta == 0.
         if (!bitPacker.encode(uint(0), uint(1)))
            return false;
      }
      else
      {
         if (!bitPacker.encode(uint(1), uint(1)))
            return false;
            
         if (!mCoder.encodeOmegaSigned(bitPacker, userIDDelta))
            return false;
      }

      uint predStartCPUTime = mPrevCPUEndTime;
      if (sample.mLevel > mPrevLevel)
         predStartCPUTime = mPrevCPUStartTime;

      
      //BDEBUG_ASSERT(predStartCPUTime <= cpuStartTime);

      const uint deltaStartCPUTime = (cpuStartTime - mPrevCPUStartTime) < (UINT_MAX-1) ? (cpuStartTime - mPrevCPUStartTime) : (UINT_MAX-1) ;
         
      if (!mCoder.encodeOmega(bitPacker, deltaStartCPUTime))
         return false;

      const int deltaCPUDuration = cpuDuration - mSectionPredDataArray[sample.mSectionID].mPrevCPUDuration;
      if (!mCoder.encodeOmegaSigned(bitPacker, deltaCPUDuration))
         return false;

      if (!pSection->cpuOnly())
      {
         uint predStartGPUTime = mPrevGPUEndTime;
         if (sample.mLevel > mPrevLevel)
            predStartGPUTime = mPrevGPUStartTime;

         if (!sample.hasGPUTimes())
         {
            // this should be very rare-- only when the gpu samples time out
            // we don't want to try encoding a negative start gpu time delta
            // instead, ensure the gpu times are both equal, which means the sample is invalid
            gpuStartTime = predStartGPUTime;
            gpuEndTime = predStartGPUTime;
            gpuDuration = 0;
         }
         
         BDEBUG_ASSERT(predStartGPUTime <= gpuStartTime);

         const uint deltaStartGPUTime = gpuStartTime - mPrevGPUStartTime;
         if (!mCoder.encodeOmega(bitPacker, deltaStartGPUTime))
            return false;

         const int deltaGPUDuration = gpuDuration - mSectionPredDataArray[sample.mSectionID].mPrevGPUDuration;
         if (!mCoder.encodeOmegaSigned(bitPacker, deltaGPUDuration))
            return false;   
      }
   }  

   // Update the model - only record those things we need for speed
   
   mSectionPredDataArray[sample.mSectionID].mPrevCPUDuration = cpuDuration;
   mSectionPredDataArray[sample.mSectionID].mPrevGPUDuration = gpuDuration;
   mSectionPredDataArray[mPrevSectionID].mNextSectionID = sample.mSectionID;
   
   mPrevLevel        = sample.mLevel;
   mPrevUserID       = sample.mUserID;
   mPrevSectionID    = sample.mSectionID;
   
   mPrevCPUStartTime = cpuStartTime;
   mPrevCPUEndTime   = cpuEndTime;
   mPrevGPUStartTime = gpuStartTime;
   mPrevGPUEndTime   = gpuEndTime;
      
   return true;
}

//---------------------------------------------------------------------------------------------------
// BTimelineSampleCodec::decodeSample
// false on failure
//---------------------------------------------------------------------------------------------------
template<typename BitPackerType>
inline bool BTimelineSampleCodec::decodeSample(BitPackerType& bitPacker, BRetiredProfileSample& decodedSample, const BDynamicArray<const BProfileSection*>& sections)
{
   uint sectionID;
   if (-1 == mPrevLevel)
   {
      if (!mCoder.decodeOmega(bitPacker, sectionID))
         return false;
   }
   else
   {
      int sectionIDDelta;
      if (!mCoder.decodeOmegaSigned(bitPacker, sectionIDDelta))
         return false;
      sectionID = static_cast<BSectionID>(sectionIDDelta + mSectionPredDataArray[mPrevSectionID].mNextSectionID);
   }
   
   decodedSample.mSectionID = static_cast<BSectionID>(sectionID);

   // Sanity check, we should never have more than a few hundred sections!              
   if (sectionID >= 512)
      return false;

   if (sectionID >= mSectionPredDataArray.size())
      mSectionPredDataArray.resize(sectionID + 1);

   // if this fails the passed in section array wasn't completely up to date
   if (sectionID >= sections.size())
      return false;

   const bool cpuOnly = sections[sectionID]->cpuOnly();

   BCoderTime cpuStartTime = 0;
   BCoderTime cpuEndTime;
   BCoderTime cpuDuration;
   BCoderTime gpuStartTime = 0;
   BCoderTime gpuEndTime = 0;
   BCoderTime gpuDuration = 0;

   if (-1 == mPrevLevel)
   {
      uint level;
      if (!mCoder.decodeOmega(bitPacker, level))              
         return false;
      decodedSample.mLevel = static_cast<uchar>(level);
      // sanity check
      if (level >= 128)
         return false;

      if (!mCoder.decodeOmega(bitPacker, decodedSample.mUserID))
         return false;

      if (!bitPacker.decode(cpuStartTime, 32))
         return false;         

      if (!bitPacker.decode(cpuEndTime, 32))
         return false;         

      if (!cpuOnly)
      {
         if (!bitPacker.decode(gpuStartTime, 32))
            return false;         

         if (!bitPacker.decode(gpuEndTime, 32))
            return false;         
      }

      cpuDuration = cpuEndTime - cpuStartTime;
      gpuDuration = gpuEndTime - gpuStartTime;
   }
   else
   {
      int levelDelta;
      if (!mCoder.decodeOmegaSigned(bitPacker, levelDelta))
         return false;
      
      decodedSample.mLevel = static_cast<uchar>(levelDelta + mPrevLevel);

      // Decode user ID - almost always 0, so read a single bit to determine if we need to omega decode the delta.
      int userIDDelta = 0;
      
      uint userIDDeltaNonZeroFlag;
      if (!bitPacker.decode(userIDDeltaNonZeroFlag, 1))
         return false;
      if (userIDDeltaNonZeroFlag)
      {
         if (!mCoder.decodeOmegaSigned(bitPacker, userIDDelta))
            return false;
      }            
      decodedSample.mUserID = userIDDelta + mPrevUserID;            

      // Decode the CPU start time.
      uint predStartCPUTime = mPrevCPUEndTime;
      if (decodedSample.mLevel > mPrevLevel)
         predStartCPUTime = mPrevCPUStartTime;

      uint deltaStartCPUTime;
      if (!mCoder.decodeOmega(bitPacker, deltaStartCPUTime))
         return false;
      cpuStartTime = deltaStartCPUTime + mPrevCPUStartTime;

      // Decode the CPU duration.
      int deltaCPUDuration;
      if (!mCoder.decodeOmegaSigned(bitPacker, deltaCPUDuration))
         return false;
      cpuDuration = deltaCPUDuration + mSectionPredDataArray[sectionID].mPrevCPUDuration;
      cpuEndTime = cpuStartTime + cpuDuration;

      if (!cpuOnly)
      {
         // Decode the GPU start time.
         uint predStartGPUTime = mPrevGPUEndTime;
         if (decodedSample.mLevel > mPrevLevel)
            predStartGPUTime = mPrevGPUStartTime;

         uint deltaStartGPUTime;
         if (!mCoder.decodeOmega(bitPacker, deltaStartGPUTime))
            return false;
         gpuStartTime = deltaStartGPUTime + mPrevGPUStartTime;

         // Decode the GPU end time.
         int deltaGPUDuration;
         if (!mCoder.decodeOmegaSigned(bitPacker, deltaGPUDuration))
            return false;

         gpuDuration = deltaGPUDuration + mSectionPredDataArray[sectionID].mPrevGPUDuration;
         gpuEndTime = gpuStartTime + gpuDuration;
      }
   }

   // Update the model - only record those things we need for speed
   mSectionPredDataArray[sectionID].mPrevCPUDuration = cpuDuration;
   mSectionPredDataArray[sectionID].mPrevGPUDuration = gpuDuration;
   mSectionPredDataArray[mPrevSectionID].mNextSectionID = sectionID;
   
   mPrevLevel        = decodedSample.mLevel;
   mPrevUserID       = decodedSample.mUserID;
   mPrevCPUStartTime = cpuStartTime;
   mPrevCPUEndTime   = cpuEndTime;
   mPrevGPUStartTime = gpuStartTime;
   mPrevGPUEndTime   = gpuEndTime;
   mPrevSectionID    = sectionID;
   
   decodedSample.mCPUStartScaledTime = cpuStartTime;
   decodedSample.mCPUEndScaledTime = cpuEndTime;
   decodedSample.mGPUStartScaledTime = 0;
   decodedSample.mGPUEndScaledTime = 0;
   if (!cpuOnly)
   {
      decodedSample.mGPUStartScaledTime = gpuStartTime;
      decodedSample.mGPUEndScaledTime = gpuEndTime;
   }

   return true;
}

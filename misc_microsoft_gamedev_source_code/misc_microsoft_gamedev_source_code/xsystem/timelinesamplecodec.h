//============================================================================
//
//  timelinesamplecodec.h
//  
//  Copyright (c) 2005, Ensemble Studios
//
//============================================================================
#pragma once

//---------------------------------------------------------------------------------------------------
// class BTimelineSampleCodec
//---------------------------------------------------------------------------------------------------
class BTimelineSampleCodec
{
public:
   BTimelineSampleCodec();
   
   void resetModel(void);
   
   // false on failure
   template<typename BitPackerType>
   bool codeSample(BitPackerType& bitPacker, const BRetiredProfileSample& sample, const BProfileSection* pSection);
   
   // false on failure
   template<typename BitPackerType>
   bool decodeSample(BitPackerType& bitPacker, BRetiredProfileSample& decodedSample, const BDynamicArray<const BProfileSection*>& sections);

private:
   BUniversalCoder mCoder;
   
   typedef uint BCoderTime;
                  
   // Section prediction data
   class BSectionPredData
   {
   public:
      BSectionPredData()
      {
         clear(); 
      }
      
      BSectionPredData(const BSectionPredData& other)
      {
         mPrevCPUDuration = other.mPrevCPUDuration;
         mPrevGPUDuration = other.mPrevGPUDuration;
         mNextSectionID   = other.mNextSectionID;
      }
      
      BSectionPredData& operator=(const BSectionPredData& rhs)
      {
         mPrevCPUDuration = rhs.mPrevCPUDuration;
         mPrevGPUDuration = rhs.mPrevGPUDuration;
         mNextSectionID   = rhs.mNextSectionID;
         return *this;
      }
      
      void clear(void)
      {
         mPrevCPUDuration = 50;
         mPrevGPUDuration = 50;
         mNextSectionID = 0;
      }
   
      // Prev CPU/GPU durations for this section.
      BCoderTime mPrevCPUDuration;
      BCoderTime mPrevGPUDuration;
            
      // The ID of the section that after this section.
      uint mNextSectionID;
   };
   
   BDynamicArray<BSectionPredData> mSectionPredDataArray;
   
   int mPrevLevel;
   uint mPrevUserID;
      
   BCoderTime mPrevCPUStartTime;
   BCoderTime mPrevCPUEndTime;
   BCoderTime mPrevGPUStartTime;
   BCoderTime mPrevGPUEndTime;
   uint mPrevSectionID;
};

#include "timelinesamplecodec.inl"


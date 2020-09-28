// File: runningAverage.h
// Reduces noise at a rate proportional to the square root of the number of samples in the average. See "Convolutions":
// http://www.ganssle.com/articles/aconvolv.htm
#pragma once

template<class SampleType=double, class AccumType=double>
class BRunningAverage
{
   AccumType mTotal;
   int mHead;
   int mTail;
   int mNumSamples;
   int mRingBufferSize;
   SampleType* mpRingBuffer;

   int nextElement(int i) const
   { 
      return (i + 1) % mRingBufferSize; 
   }

   void assign(const BRunningAverage& b)
   {		
      mTotal = b.mTotal;
      mHead = b.mHead;
      mTail = b.mTail;
      mNumSamples = b.mNumSamples;
      mRingBufferSize = b.mRingBufferSize;
      if (!mpRingBuffer)
         mpRingBuffer = new SampleType[mRingBufferSize];
               
      std::copy(b.mpRingBuffer, b.mpRingBuffer + mRingBufferSize, mpRingBuffer);
   }

public:
   BRunningAverage(const BRunningAverage& b) : 
      mpRingBuffer(NULL)
   {
      assign(b);
   }

   BRunningAverage(int bufferSize = 8)
   {
      BDEBUG_ASSERT(bufferSize >= 0);
      
      // One extra, because one entry is a sentinel/dummy.
      mRingBufferSize = bufferSize + 1; 
      mpRingBuffer = new SampleType[mRingBufferSize];
      
      clear();
   }

   ~BRunningAverage()
   {
      delete [] mpRingBuffer;
   }
   
   void set(int bufferSize)
   {
      if (bufferSize == mRingBufferSize)
         return;
         
      clear();
      
      mRingBufferSize = bufferSize + 1; 
      
      delete mpRingBuffer;
      mpRingBuffer = new SampleType[mRingBufferSize];
   }
   
   BRunningAverage& operator= (const BRunningAverage& b)
   {
      if (this == &b)
         return *this;

      if (mRingBufferSize < b.mRingBufferSize)
      {
         delete [] mpRingBuffer;
         mpRingBuffer = NULL;
      }

      assign(b);
      return *this;
   }
   
   void clear(void)
   {
      mTotal = 0;
      mNumSamples = mHead = mTail = 0;
      //std::fill(mpRingBuffer, mpRingBuffer + mRingBufferSize, 0);
   }

   void addSample(SampleType sample)
   {
      mTotal += (mpRingBuffer[mHead] = sample);
      mNumSamples++;

      if ((mHead = nextElement(mHead)) == mTail)
      {
         mTotal -= mpRingBuffer[mTail];
         if (mTotal < 0) 
            mTotal = 0; // due to FP rounding
         mNumSamples--;
         mTail = nextElement(mTail);
      }
   }

   AccumType getAverage(void) const
   {
      return mNumSamples ? (mTotal / static_cast<SampleType>(mNumSamples)) : 0;
   }

   int getNumSamples(void) const { return mNumSamples; }

   AccumType getTotal(void) const
   {
      return mTotal;
   }

   SampleType getMaximum(void) const
   {
      if (!mNumSamples)
         return 0;
         
      SampleType maxValue = static_cast<SampleType>(-Math::fNearlyInfinite);
      
      for (int i = 0, j = mTail; i < mNumSamples; i++, j = nextElement(j))
         maxValue = Math::Max(maxValue, mpRingBuffer[j]);
         
      return maxValue;
   }
   
};

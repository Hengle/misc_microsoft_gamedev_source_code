using System;
using System.Collections;

//typedef ushort BSectionID;

using RemoteTools;

namespace EnsembleStudios.RemoteGameDebugger
{

   //class BRetiredProfileSample{}
   public class BProfileSection
   {
      public bool cpuOnly()
      {
         return mCPUOnly;
      }
      public bool           mEnabled;
      public bool           mCPUOnly;
      public ushort         mID;
      public string         mName;

      //Application Data
      public bool mUIEnabled;

      public override string ToString()
      {
         return String.Format("{0} ID:{1} CPUOnly:{2} Enabled:{3}",mName,mID,mCPUOnly,mEnabled);

      }

   }

   public class BRetiredProfileSample
   {
      // Scaled CPU start/end times 
      public uint mCPUStartTime;
      public uint mCPUEndTime;
   
      // GPU start/end times - if 0, no GPU information is available for this sample
      public uint mGPUStartTime;
      public uint mGPUEndTime;           
   
      // User provided ID (from perfEndID for example).
      public uint mUserID;                 
   
      // Nesting level.
      public byte mLevel;                 
         
      public ushort/*BSectionID*/ mSectionID;
   
      // True if the GPU times are valid -- they could still be invalid even if the CPU section is NOT marked as CPU only!
      public bool hasGPUTimes() 
      {
         return mGPUStartTime != mGPUEndTime;
      }
      public override string ToString()
      {
         return String.Format("SecID:{0} Level:{1} Cstart:{2} Cend:{3} GStart:{4} GEnd:{5}", mSectionID,mLevel,mCPUStartTime,mCPUEndTime,mGPUStartTime,mGPUEndTime);
      }

   }

   public class BTimelineSampleCodec
   {
      public BTimelineSampleCodec()
      {
         mPrevLevel = -1;
         mPrevUserID = 0;
         mPrevCPUStartTime = 0;
         mPrevCPUEndTime = 0;
         mPrevGPUStartTime = 0;
         mPrevGPUEndTime = 0;
         mPrevSectionID = 0;   

         mSectionPredDataArray = new BSectionPredData[0];
         mCoder = new BUniversalCoder();
      }
      public void resetModel()
      {
         mPrevLevel = -1;
         mPrevUserID = 0;

         mPrevCPUStartTime = 0;
         mPrevCPUEndTime = 0;
         mPrevGPUStartTime = 0;
         mPrevGPUEndTime = 0;
         mPrevSectionID = 0;

         for (uint i = 0; i < size(mSectionPredDataArray); i++)
            mSectionPredDataArray[i].clear();      
      }
      
      //This is not efficient, but is compatible for the port
      private void resize(ref BSectionPredData[] buffer, int size)
      {
         if(buffer != null)
         {
            BSectionPredData[] newBuf = new BSectionPredData[size];
            //newBuf.Initialize();
            for (uint i = 0; i < size; i++)
               newBuf[i] = new BSectionPredData();              

            buffer.CopyTo(newBuf,0);
            buffer = newBuf;
         }
         else
         {
            buffer = new BSectionPredData[size];
         }
         //throw new System.Exception("implement this");
      }
      private uint size(BSectionPredData[] buffer)
      {
         return (uint)buffer.Length;
      }
      private uint size(BProfileSection[] buffer)
      {
         return (uint)buffer.Length;
      }

      // false on failure
      //template<typename BitPackerType>
      public bool codeSample(BitPacker bitPacker, /*const*/ ref BRetiredProfileSample sample, /*const*/ ref BProfileSection pSection)
      {
         if (sample.mSectionID >= size(mSectionPredDataArray))
            resize(ref mSectionPredDataArray,sample.mSectionID + 1);

         /*const*/ uint/*BCoderTime*/ cpuStartTime = sample.mCPUStartTime;
         /*const*/ uint/*BCoderTime*/ cpuEndTime = sample.mCPUEndTime;
         uint/*BCoderTime*/ gpuStartTime = 0;
         uint/*BCoderTime*/ gpuEndTime = 0;
         if (!pSection.cpuOnly())
         {
            gpuStartTime = sample.mGPUStartTime;
            gpuEndTime = sample.mGPUEndTime;
         }
         uint/*BCoderTime*/ cpuDuration = cpuEndTime - cpuStartTime;
         uint/*BCoderTime*/ gpuDuration = gpuEndTime - gpuStartTime;

         if (-1 == mPrevLevel)
         {
            if (!mCoder.encodeOmega(ref bitPacker, sample.mSectionID))
               return false;
         
            // Model is empty - send uncompressed sample
            if (!mCoder.encodeOmega(ref bitPacker, sample.mLevel))              
               return false;

            if (!mCoder.encodeOmega(ref bitPacker, sample.mUserID))
               return false;

            if (!bitPacker.encode(cpuStartTime, 32))
               return false;         

            if (!bitPacker.encode(cpuEndTime, 32))
               return false;         

            if (!pSection.cpuOnly())
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
            int sectionIDDelta = (int)(sample.mSectionID - mSectionPredDataArray[mPrevSectionID].mNextSectionID);
            if (!mCoder.encodeOmegaSigned(ref bitPacker, sectionIDDelta))
               return false;
      
            // Code the level delta using 1 or 2 bits [-1, 0, or 1]
            /*const*/ int levelDelta = sample.mLevel - mPrevLevel;
            if (!mCoder.encodeOmegaSigned(ref bitPacker, levelDelta))
               return false;
                        
            /*const*/ int userIDDelta = (int)(sample.mUserID - mPrevUserID);
            if (userIDDelta == 0)
            {
               // Encode single bit to represent userIDDelta == 0.
               if (!bitPacker.encode((uint)(0), (int)(1)))
                  return false;
            }
            else
            {
               if (!bitPacker.encode((uint)(1), (int)(1)))
                  return false;
            
               if (!mCoder.encodeOmegaSigned(ref bitPacker, userIDDelta))
                  return false;
            }

            uint predStartCPUTime = mPrevCPUEndTime;
            if (sample.mLevel > mPrevLevel)
               predStartCPUTime = mPrevCPUStartTime;

            //BDEBUG_ASSERT(predStartCPUTime <= cpuStartTime);

            /*const*/ uint deltaStartCPUTime = cpuStartTime - mPrevCPUStartTime;
            if (!mCoder.encodeOmega(ref bitPacker, deltaStartCPUTime))
               return false;

            /*const*/ int deltaCPUDuration = (int)(cpuDuration - mSectionPredDataArray[sample.mSectionID].mPrevCPUDuration);
            if (!mCoder.encodeOmegaSigned(ref bitPacker, deltaCPUDuration))
               return false;

            if (!pSection.cpuOnly())
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
         
               //BDEBUG_ASSERT(predStartGPUTime <= gpuStartTime);

               /*const*/ uint deltaStartGPUTime = gpuStartTime - mPrevGPUStartTime;
               if (!mCoder.encodeOmega(ref bitPacker, deltaStartGPUTime))
                  return false;

               /*const*/ int deltaGPUDuration = (int)(gpuDuration - mSectionPredDataArray[sample.mSectionID].mPrevGPUDuration);
               if (!mCoder.encodeOmegaSigned(ref bitPacker, deltaGPUDuration))
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
   
      // false on failure
      //template<typename BitPackerType>
      public bool decodeSample(BitPacker bitPacker, ref BRetiredProfileSample decodedSample, /*const*/ /*BAlignedArray<const BProfileSection*>&*/ BProfileSection[] sections)
      {

         if(sections == null)
         {
            ErrorHandler.Error("Can't decompress frame, sections == null!");
            return false;
         }

         uint sectionID;
         if (-1 == mPrevLevel)
         {
            if (!mCoder.decodeOmega(ref bitPacker, out sectionID))
               return false;
         }
         else
         {
            int sectionIDDelta;
            if (!mCoder.decodeOmegaSigned(ref bitPacker, out sectionIDDelta))
               return false;
            sectionID = /*static_cast<BSectionID>*/(ushort)(sectionIDDelta + mSectionPredDataArray[mPrevSectionID].mNextSectionID);
         }
   
         decodedSample.mSectionID = /*static_cast<BSectionID>*/(ushort)(sectionID);

         // Sanity check, we should never have more than a few hundred sections!              
         if (sectionID >= 512)
            return false;

         if (sectionID >= size(mSectionPredDataArray))
            resize(ref mSectionPredDataArray,(int)sectionID + 100);//the current resize operation is slow...

         // if this fails the passed in section array wasn't completely up to date
         if (sectionID >= size(sections))
            return false;

         bool cpuOnly = false;
         if (sections[sectionID] != null)
         {
            /*const*/
            cpuOnly = sections[sectionID].cpuOnly();
         }

         uint/*BCoderTime*/ cpuStartTime = 0;
         uint/*BCoderTime*/ cpuEndTime;
         uint/*BCoderTime*/ cpuDuration;
         uint/*BCoderTime*/ gpuStartTime = 0;
         uint/*BCoderTime*/ gpuEndTime = 0;
         uint/*BCoderTime*/ gpuDuration = 0;

         if (-1 == mPrevLevel)
         {
            uint level;
            if (!mCoder.decodeOmega(ref bitPacker, out level))              
               return false;
            decodedSample.mLevel = /*static_cast<uchar>*/(byte)(level);
            // sanity check
            if (level >= 128)
               return false;

            if (!mCoder.decodeOmega(ref bitPacker, out decodedSample.mUserID))
               return false;

            if (!bitPacker.decode(out cpuStartTime, 32))
               return false;         

            if (!bitPacker.decode(out cpuEndTime, 32))
               return false;         

            if (!cpuOnly)
            {
               if (!bitPacker.decode(out gpuStartTime, 32))
                  return false;         

               if (!bitPacker.decode(out gpuEndTime, 32))
                  return false;         
            }

            cpuDuration = cpuEndTime - cpuStartTime;
            gpuDuration = gpuEndTime - gpuStartTime;
         }
         else
         {
            int levelDelta;
            if (!mCoder.decodeOmegaSigned(ref bitPacker, out levelDelta))
               return false;
      
            decodedSample.mLevel = /*static_cast<uchar>*/(byte)(levelDelta + mPrevLevel);

            // Decode user ID - almost always 0, so read a single bit to determine if we need to omega decode the delta.
            int userIDDelta = 0;
      
            uint userIDDeltaNonZeroFlag;
            if (!bitPacker.decode(out userIDDeltaNonZeroFlag, 1))
               return false;
            if (userIDDeltaNonZeroFlag > 0)
            {
               if (!mCoder.decodeOmegaSigned(ref bitPacker, out userIDDelta))
                  return false;
            }            
            decodedSample.mUserID = (uint)(userIDDelta + mPrevUserID);

            // Decode the CPU start time.
            uint predStartCPUTime = mPrevCPUEndTime;
            if (decodedSample.mLevel > mPrevLevel)
               predStartCPUTime = mPrevCPUStartTime;

            uint deltaStartCPUTime;
            if (!mCoder.decodeOmega(ref bitPacker, out deltaStartCPUTime))
               return false;
            cpuStartTime = deltaStartCPUTime + mPrevCPUStartTime;

            // Decode the CPU duration.
            int deltaCPUDuration;
            if (!mCoder.decodeOmegaSigned(ref bitPacker, out deltaCPUDuration))
               return false;
            cpuDuration = (uint)(deltaCPUDuration + mSectionPredDataArray[sectionID].mPrevCPUDuration);
            cpuEndTime = cpuStartTime + cpuDuration;

            if (!cpuOnly)
            {
               // Decode the GPU start time.
               uint predStartGPUTime = mPrevGPUEndTime;
               if (decodedSample.mLevel > mPrevLevel)
                  predStartGPUTime = mPrevGPUStartTime;

               uint deltaStartGPUTime;
               if (!mCoder.decodeOmega(ref bitPacker, out deltaStartGPUTime))
                  return false;
               gpuStartTime = deltaStartGPUTime + mPrevGPUStartTime;

               // Decode the GPU end time.
               int deltaGPUDuration;
               if (!mCoder.decodeOmegaSigned(ref bitPacker, out deltaGPUDuration))
                  return false;

               gpuDuration = (uint)(deltaGPUDuration + mSectionPredDataArray[sectionID].mPrevGPUDuration);
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
   
         decodedSample.mCPUStartTime = cpuStartTime;
         decodedSample.mCPUEndTime = cpuEndTime;
         decodedSample.mGPUStartTime = 0;
         decodedSample.mGPUEndTime = 0;
         if (!cpuOnly)
         {
            decodedSample.mGPUStartTime = gpuStartTime;
            decodedSample.mGPUEndTime = gpuEndTime;
         }

         return true;
      }

      //private:
      BUniversalCoder mCoder;
   
      //typedef uint BCoderTime;
                   
      //BAlignedArray<BSectionPredData> mSectionPredDataArray;
      BSectionPredData[] mSectionPredDataArray;
         
      int mPrevLevel;
      uint mPrevUserID;
            
      uint/*BCoderTime*/ mPrevCPUStartTime;
      uint/*BCoderTime*/ mPrevCPUEndTime;
      uint/*BCoderTime*/ mPrevGPUStartTime;
      uint/*BCoderTime*/ mPrevGPUEndTime;
      uint mPrevSectionID;

      public void BDEBUG_ASSERT(bool val)
      {
         if(val == false)
            throw new System.Exception("//BDEBUG_ASSERT");
      }
   } 
   


   // Section prediction data
   public class BSectionPredData
   {
      //public:
      public BSectionPredData()
      {
         clear(); 
      }
      
      //         BSectionPredData(const BSectionPredData& other)
      //         {
      //            mPrevCPUDuration = other.mPrevCPUDuration;
      //            mPrevGPUDuration = other.mPrevGPUDuration;
      //            mNextSectionID   = other.mNextSectionID;
      //         }
      //      
      //         BSectionPredData& operator=(const BSectionPredData& rhs)
      //         {
      //            mPrevCPUDuration = rhs.mPrevCPUDuration;
      //            mPrevGPUDuration = rhs.mPrevGPUDuration;
      //            mNextSectionID   = rhs.mNextSectionID;
      //            return *this;
      //         }
      
      public void clear()
      {
         mPrevCPUDuration = 50;
         mPrevGPUDuration = 50;
         mNextSectionID = 0;
      }
   
      // Prev CPU/GPU durations for this section.
      public uint/*BCoderTime*/ mPrevCPUDuration;
      public uint/*BCoderTime*/ mPrevGPUDuration;
            
      // The ID of the section that after this section.
      public uint mNextSectionID;
   }

}
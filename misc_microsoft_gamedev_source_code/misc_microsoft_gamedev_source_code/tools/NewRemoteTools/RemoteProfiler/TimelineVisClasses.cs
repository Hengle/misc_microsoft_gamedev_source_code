using System;
using System.IO;
using System.Collections;
using System.Threading;


using RemoteTools;

namespace EnsembleStudios.RemoteGameDebugger
{

   #region Interfaces
   public interface PerformanceSampleConsumer
   {
      void insertIdentifier(BProfileSection identifier);
      void insertCompressedFrame(CompressedFrame frame);
   }
   public interface IFrameObserver
   {
      void NewFrame(Frame f);
   }
   #endregion
   
   #region Primary Data Classes

   public class CompressedFrame
   {
      public int mThreadID;
      public ulong mFrameNumber;
      public uint mNumSamples;
      public double mStartTime;
      ICoderBuffer mBuffer = null;

      public CompressedFrame()
      {
         mFrameNumber = 1;
      }
      public void SetBuffer(ICoderBuffer buffer)
      {
         mBuffer = buffer;
      }
      public ICoderBuffer GetBuffer()
      {
         return mBuffer;
      }
      public uint mTotalSize;

      public bool WriteFrame(BinaryWriter writer)
      {
         writer.Write(mThreadID);
         writer.Write(mFrameNumber);
         writer.Write(mNumSamples);
         writer.Write(mStartTime);
         writer.Write(mBuffer.getBuf().Length);
         writer.Write(mBuffer.getBuf());
         return true;
      }
      public bool ReadFrame(BinaryReader reader)
      {
         mThreadID = reader.ReadInt32();
         mFrameNumber = reader.ReadUInt64();
         mNumSamples = reader.ReadUInt32();
         mStartTime = reader.ReadDouble();
         int len = reader.ReadInt32();

         mBuffer = new BDynamicCoderBuf((int)len);
         reader.ReadBytes((int)len).CopyTo(mBuffer.getBuf(), 0);
         return true;
      }

   }
   public class Constants
   {
      public const int cUIntScaleToMS =   10000000;
      public const int cUIntScaletoSeconds = 10000;
   };

   public class Frame 
   {
      public ulong mFrameNumber;
      public uint mNumSamples;
      public double mStartTime;
      public int mThreadID = -1;
      public int mCPUID = -1;

      //Stats
      public bool mbHasGPUSamples;
      public uint mFirstGPUSampleIndex;
      public uint mLastGPUSampleIndex;
      public double mCPUStartTime;
      public double mCPUEndtime;
      public double mGPUStartTime;
      public double mGPUEndtime;
      public int mMaxStackHeight;
      public double mNextFrameStartTime;

      public uint[] mSectionStats = null;
      public uint[] mSectionStatsGpu = null;

      public DutyCycle mDutyCycle = new DutyCycle();
      public DutyCycle mGpuDutyCycle = new DutyCycle();


      public BRetiredProfileSample[] mSamples;

      public bool[] mCPUsUsed;

      CompressedFrame mCompressedFrame;

      private DynamicResource mFrameBuffer = new DynamicResource();

      //State
      public bool mbCollapsed;
 
      public BProfileSection[] mSections = null;

      public Frame(CompressedFrame compressedFrame, BProfileSection[] sections)
      {
         mCompressedFrame = compressedFrame;
         mFrameNumber = mCompressedFrame.mFrameNumber;
         mNumSamples = mCompressedFrame.mNumSamples - 1;
         
         mStartTime = mCompressedFrame.mStartTime;

         mFirstGPUSampleIndex = 0;
         mFirstGPUSampleIndex = 0;
         mbHasGPUSamples = false;
         mbCollapsed = false;

         mMaxStackHeight = 0;

         mCPUsUsed =  new bool[6];
         for (int i = 0; i<6; i++) 
         {
            mCPUsUsed[i] = false;
         }

         mThreadID = compressedFrame.mThreadID;

         mNextFrameStartTime = 0;
         mSections = sections;

         mSectionStats = new uint[sections.Length];
         mSectionStatsGpu = new uint[sections.Length];
         
      }

      int mTotalSamples = 0;
      public int TotalSamples()
      {
         if(mTotalSamples == 0)
         {
            mTotalSamples = (int)mNumSamples;
            foreach(Frame f in this.mThreadFrames)
            {
               mTotalSamples += (int)f.mNumSamples;
            }

         }


         return mTotalSamples;
      }

      public void Collapse()
      {
         mCompressedFrame = null;
         mbCollapsed = true;
      }

      public bool WriteFrame(BinaryWriter writer)
      {
         if(mCompressedFrame != null)
         {
            writer.Write(true);
            mCompressedFrame.WriteFrame(writer);
         }
         else
         {
            writer.Write(false);   
         }
         //write threadframes
         //writer.Write(mThreadFrames.Count);
         foreach(Frame threadFrame in mThreadFrames)
         {           
            threadFrame.WriteFrame(writer);
         }
         return true;
      }

      public bool ReadFrame(BinaryReader reader)
      {
         return false;
      }

      public bool DecompressFrame(BTimelineSampleCodec decomp, ref BRetiredProfileSample[] decodedSamplesBuffer)
      {
         BProfileSection[] sections = mSections;
         
         if(mCompressedFrame == null)
         {
            return false;
         }
         ICoderBuffer buffer = mCompressedFrame.GetBuffer();
         decomp.resetModel();
         buffer.setPos(0);
         BitPacker bitPacker = new BitPacker(buffer);
         bool success = bitPacker.decodeStart();
         BRetiredProfileSample decodedSample;
         int i;
         try
         {
            for(i=0; i<mNumSamples; i++)
            {
               decodedSample = decodedSamplesBuffer[i];
               success = decomp.decodeSample(bitPacker, ref decodedSample, sections);
               if(success == false)
               {
                  ErrorHandler.Error(String.Format("Error decompressing sample {0} from frame# {1}",i,this.mFrameNumber));
             //     return false;
               }
               ProcessSample(decodedSample,i);
            }
            mDutyCycle.BuildDutyCycle(ref decodedSamplesBuffer, mNumSamples, false);
            mDutyCycle.mStartTime = mStartTime;

            if(this.mbHasGPUSamples == true)
            {            
               mGpuDutyCycle.BuildDutyCycle(ref decodedSamplesBuffer, mNumSamples, true);
               mGpuDutyCycle.mStartTime = mStartTime;
            }
            mSamples = decodedSamplesBuffer;
         }
         catch(System.Exception ex)
         {
            ErrorHandler.Error(ex.ToString());
            return false;
         }
         return true;
      }
      
      #region DutyCycleComputations
      public class Line1D
      {
         public Line1D(uint start, uint end)
         {
            mStart = start; 
            mEnd = end;
         }
         public int Contains(uint point)
         {
            if(point < mStart)
            {
               return -1;
            }
            else if(point > mEnd)
            {
               return 1;
            }
            else 
            {
               return 0;
            }
         }        
         public uint mStart;
         public uint mEnd;
      };

      public class DutyCycle
      {      
         public ArrayList mSegmentList = new ArrayList();
         public uint mUnusedTime = 0;
         public double mStartTime;
         public int mOutOfOrderCount=0;

         public double Evaluate(double start, double end)
         {
            if(start < mStartTime)
               return -1;

            uint s = (uint)((start - mStartTime)*Constants.cUIntScaleToMS);
            uint e = (uint)((end - mStartTime)*Constants.cUIntScaleToMS);

            return Evaluate(s,e);

         }

         //deal with seconds better...
         public double Evaluate(uint start, uint end)
         {
            double total = 0;
            foreach(Line1D line in mSegmentList)
            {
               if((line.mEnd >= start) && (line.mStart <= end))
               {
                  if((line.mEnd > end) && (line.mStart < start))
                  {
                     total = end-start;
                     break;
                  }
                  else if(line.mEnd > end) 
                  {
                     total += (end - line.mStart);         
                     break;
                  }
                  else if(line.mStart < start)
                  {                 
                     total += (line.mEnd - start);
                  }
                  else 
                  {
                     total += (line.mEnd - line.mStart);
                  }
               }
            }
            return total;
         }

         public bool MergeDutyCycle(DutyCycle d)
         {
            double offset = d.mStartTime - mStartTime;

            if(d.mSegmentList.Count == 0)
               return true;

            if(offset >= 0)
            {            
               uint offs = (uint)(offset * Constants.cUIntScaleToMS);
               mSegmentList = DutyCycle.MergeDutyCycle(mSegmentList,d.mSegmentList,offs);
               return true;
            }
            else
            {

            }
            return false;
         }

         //hmm should use strong types here?...
         //uint offset could make this difficult to used do to ordering constraints
         static public ArrayList MergeDutyCycle(ArrayList a, ArrayList b, uint offset)
         {
            ArrayList mergedList = new ArrayList();

            int bIndex = 0;
            int aIndex = 0;
            Line1D bLine = null; 
            Line1D aLine = null;
            Line1D currentLine = null;

            bool bReprocessCurrentLine = false;

            while(aIndex < a.Count && bIndex < b.Count)
            {
               if(((a[aIndex] as Line1D).mStart) < ((b[bIndex] as Line1D).mStart + offset))
               {
                  currentLine = new Line1D((a[aIndex] as Line1D).mStart , (a[aIndex] as Line1D).mEnd);
                  aIndex++;
               }
               else if((currentLine == null) || ((b[bIndex] as Line1D).mEnd + offset > currentLine.mEnd))
               {
                  currentLine = new Line1D((b[bIndex] as Line1D).mStart + offset , (b[bIndex] as Line1D).mEnd + offset);
                  bIndex++;
               } 
               else
               {
                  bReprocessCurrentLine = true;
               }
               while(bIndex<b.Count)
               {
                  bLine = (b[bIndex] as Line1D);
                  if(currentLine.Contains(bLine.mStart + offset) == 0)
                  {
                     if(currentLine.Contains(bLine.mEnd + offset) != 0)
                     {
                        currentLine.mEnd = bLine.mEnd + offset;
                     }
                     bIndex++;
                  }
                  else
                  {
                     //CLM If this item isn't found, we break, causing an infite loop here... so increment bIndex
                     bIndex++;
                     break;
                  }
               }     
               while(aIndex<a.Count)
               {
                  aLine = (a[aIndex] as Line1D);
                  if(currentLine.Contains(aLine.mStart) == 0)
                  {
                     if(currentLine.Contains(aLine.mEnd) != 0)
                     {
                        currentLine.mEnd = aLine.mEnd;
                     }
                     aIndex++;
                  }
                  else
                  {
                     //CLM If this item isn't found, we break, causing an infite loop here... so increment aIndex
                     aIndex++;
                     break;
                  }
               }  
               if(!bReprocessCurrentLine)
                  mergedList.Add(currentLine);

               bReprocessCurrentLine = false;
            }
            while(aIndex < a.Count)
            {
               mergedList.Add(new Line1D((a[aIndex] as Line1D).mStart , (a[aIndex] as Line1D).mEnd));
               aIndex++;
            }
            while(bIndex < b.Count)
            {
               mergedList.Add(new Line1D((b[bIndex] as Line1D).mStart + offset , (b[bIndex] as Line1D).mEnd + offset));
               bIndex++;
            }
            return mergedList;
         }
         public void BuildDutyCycle(ref BRetiredProfileSample[] samples, uint numSamples, bool bGPU)
         {
            if((mSegmentList.Count != 0) || (samples.Length == 0))
               return;

            BRetiredProfileSample s = null;
            Line1D currentLine = null;
            uint sampleStart;
            uint sampleEnd;

            for(int sampleID=0;sampleID<numSamples;sampleID++)
            {
               s = samples[sampleID];
               if(bGPU == true)
               {
                  sampleStart = s.mGPUStartTime;
                  sampleEnd = s.mGPUEndTime;
                  if((sampleStart == sampleEnd)
                  || (sampleEnd == 0))
                     continue;
               }
               else
               {            
                  sampleStart = s.mCPUStartTime;
                  sampleEnd = s.mCPUEndTime;
               }
               if(currentLine == null)
               {
                  currentLine = new Line1D(sampleStart, sampleEnd);
                  mSegmentList.Add(currentLine);
               }
               else if(currentLine.Contains(sampleStart) == 0)
               {
                  if(currentLine.Contains(sampleEnd) == 0)
                  {
                     continue;
                  }
                  else
                  {
                     currentLine.mEnd = sampleEnd;
                     continue;
                  }
               }
               else
               {  
                  if(currentLine.Contains(sampleStart) < 0)
                  {
                     mOutOfOrderCount++;
                     //throw new Exception("samples out of order");
                  }

                  mUnusedTime += (sampleStart - currentLine.mEnd);
                  currentLine = new Line1D(sampleStart, sampleEnd);
                  mSegmentList.Add(currentLine);
               }
            }         
         }

   #endregion
//         class SortByStart : System.Collections.IComparer
//         {         
//            public int Compare(object x, object y)
//            {
//               return (x as Frame).mThreadID  - (y as Frame).mThreadID;
//            }         
//         };
      }

      
      public void ProcessSample(BRetiredProfileSample sample, int sampleID)
      {
         //Calculate a few stats as we go...
         if((sample.mGPUStartTime != 0) && (mbHasGPUSamples == false))
         {
            mbHasGPUSamples = true;
            mFirstGPUSampleIndex = (uint)sampleID;
         }
         else if(sample.mGPUStartTime != 0)
         {
            mbHasGPUSamples = true;
            mLastGPUSampleIndex = (uint)sampleID;
         }   
         if(sample.mLevel > mMaxStackHeight)
            mMaxStackHeight = sample.mLevel;

         if(sample.mUserID >= 0 && sample.mUserID < 6)
         {
            mCPUsUsed[sample.mUserID] = true;
            mCPUID = (int)sample.mUserID;
         }
         else
         {
            throw new Exception("invalid cpu id:" + sample.mUserID);
         }
 
         mSectionStats[sample.mSectionID] += (uint)(sample.mCPUEndTime - sample.mCPUEndTime);
         mSectionStatsGpu[sample.mSectionID] += (uint)(sample.mGPUEndTime - sample.mGPUEndTime);

         if (sample.mSectionID == 8)
         {
            mCPUID = mCPUID + 0;
         }

      }
      public bool hasGPUSamples()
      {
         return mbHasGPUSamples;
      }
      public void CalculateFinalStats()
      {
         //assume that "frame" is that start and stop of all cpu action...
         if(mSamples == null)
         {
            return;
         }
         mCPUStartTime = ((BRetiredProfileSample)(mSamples[0])).mCPUStartTime;
         mCPUEndtime = ((BRetiredProfileSample)(mSamples[0])).mCPUEndTime;

         mGPUStartTime = ((BRetiredProfileSample)(mSamples[mFirstGPUSampleIndex])).mGPUStartTime;
         mGPUEndtime = ((BRetiredProfileSample)(mSamples[mLastGPUSampleIndex])).mGPUEndTime;
      }
      public double getCPUStartTime()
      {
         return mCPUStartTime;
      }
      public double getCPUEndTime()
      {
         return mCPUEndtime;
      }      
      public double getGPUStartTime()
      {
         return mGPUStartTime;
      }
      public double getGPUEndTime()
      {
         return mGPUEndtime;
      }
      public override string ToString()
      {
         return String.Format("Frame count: {0} NumSamples: {1} StartTime: {2:.###}",this.mFrameNumber, this.mNumSamples, this.mStartTime);
      }

      public ArrayList mThreadFrames = new ArrayList();
      public Frame mNextFrame = null;
      public Frame mLastFrame = null;
      public void AddThreadFrame(Frame frame)
      {
         mThreadFrames.Add(frame);
      }
      public ResourceHandle SafeGetBufferHandle()
      {
         ResourceHandle resourceHandle = null;
         lock(this)
         {         
            if(mFrameBuffer == null)
               return null;
            resourceHandle = mFrameBuffer.GetHandle(mNumSamples);
            if(!mFrameBuffer.mIsInitialized)
            {
               SampleBuffer buffer = resourceHandle.Get() as SampleBuffer;
               DecompressFrame(buffer.mDecomp,ref buffer.mDecodedSamples);
               mFrameBuffer.mIsInitialized = true;
               mSamples = buffer.mDecodedSamples;
            }
            foreach(Frame f in mThreadFrames)
            {
               resourceHandle.AddDependentHandle(f.SafeGetBufferHandle());               
            }
         }
         return resourceHandle;
      }

      public class SortByThreadID : System.Collections.IComparer
      {         
         public int Compare(object x, object y)
         {
            return (x as Frame).mThreadID - (y as Frame).mThreadID;
         }         
      };

   }
   #endregion

   #region Core Code

   public class SampleBuffer
   {     
      public uint mSize = 1000;
      public BTimelineSampleCodec mDecomp = new BTimelineSampleCodec();
      public BRetiredProfileSample[] mDecodedSamples;
      public SampleBuffer(uint size)
      {
         //mSize = size;
         //mSize
         if (size >= 999)
         {
            mSize = size;
         }

         mDecodedSamples = new BRetiredProfileSample[mSize];
         for(int i = 0; i < mSize; i++)
         {
            mDecodedSamples[i] = new BRetiredProfileSample();
         }
      }
      public void Destroy()
      {
         for(int i = 0; i < mSize; i++)
         {
            mDecodedSamples[i] = null;
         }
         mDecodedSamples = null;
      }
   }
   public class SampleBufferFactory : Factory
   {
      public object NewResource(uint size)
      {
         return new SampleBuffer(size);
      }
   }
   #region Generic Resource Manager

   public class ResourcePool
   {
      public static ResourcePool GetPool()
      {
         return mPoolSingleton;
      }
      private static ResourcePool mPoolSingleton = new ResourcePool();
      
      public void SetFactory(Factory f)
      {
         mFactory = f;
      }
      public ResourceContainer Allocate(uint size)
      {
         lock(this)
         {
            if(mFactory == null)
               return null;
            ResourceContainer resource = null;
            
            foreach(ResourceContainer data in mPool)
            {
               if((data.mReferenceCount == 0) && (data.mSize >= size)) //??
               {
                  mReusecount++;
                  return data;                  
               }
            }
            //init Size
            uint initSize;
            if(size < 100)
            {
               initSize = 100;
            }
            else
            {            
               initSize = (uint)(size * 1.1);
            }
            if (size == 999)
            {
               initSize = initSize;
            }

            resource = new ResourceContainer(mFactory.NewResource(initSize),initSize);
            mPool.Add(resource);
            mAllocCount++;

            return resource;
         }
      }
      private Factory mFactory = null;
      private ArrayList mPool = new ArrayList();

      //just a few debug counters
      public int mReleaseCount = 0;
      public int mAllocCount = 0;
      public int mReusecount = 0;
      public int mRecyleCount = 0;
   }
   public class ResourceContainer
   {
      public ResourceContainer(object data, uint size){mData = data;mSize=size;}
      public object mData;
      public int mReferenceCount = 0;
      public uint mSize = 0;
      public DynamicResource mRevision = null;
   }
   public interface Factory
   {
      object NewResource(uint size);
   }
   public class DynamicResource
   {
      public ResourceHandle GetHandle(uint size)
      {
         lock(this)
         {
            if((mContainer != null) && (mContainer.mReferenceCount == 0) && (mContainer.mRevision == this))
            {
               //recyle  .. could be an mt issue later..
               mResource = mContainer.mData;
               mIsInitialized = true;
               ResourcePool.GetPool().mRecyleCount++;
            }
            else if(mResource == null)
            {
               mContainer = ResourcePool.GetPool().Allocate(size);
               mContainer.mRevision = this;
               mResource = mContainer.mData;
               mIsInitialized = false;
            }
            lock(mContainer)
            {            
               mContainer.mReferenceCount++;
            }         
         }
         return new ResourceHandle(this);
      }      
      public void ReleaseOneHandle()
      {
         lock(mContainer)
         {
            mContainer.mReferenceCount--;
            if(mContainer.mReferenceCount == 0)
               InvalidateHandle();
         }
      }
      private void InvalidateHandle()
      {
         mIsInitialized = false;
         mResource = null;
         ResourcePool.GetPool().mReleaseCount++;
      }
      private ResourceContainer mContainer;
      public object mResource = null;
      public bool mIsInitialized = false;
   }
   public class ResourceHandle
   {
      public ResourceHandle(DynamicResource resource)
      {
         mData = resource;
      }
      ~ResourceHandle()
      {
         mData.ReleaseOneHandle();
         mSubHandles.Clear();
      }
      private DynamicResource mData;
      public object Get(){return mData.mResource;}

      public void AddDependentHandle(ResourceHandle handle)
      {
         mSubHandles.Add(handle);
      }
      private ArrayList mSubHandles = new ArrayList();
   }
#endregion

   //This class builds frames out of samples and updates observers when there is a new frame available
   //This class also has a buffer of frame data, and frames get collapsed when more memory is needed.
   public class SampleManager : PerformanceSampleConsumer
   {
      protected ArrayList mFrameObservers;
   
      //Frame array 
      //protected long mNumFrames;
      public ArrayList mFrames;
      protected Frame mCurrentFrame;
      protected Frame mLastFrame;
      protected int mOldestFullFrame;
      protected int mNumFullFrames;
      protected int mFullFrameQuota;
      protected int mMaxFullFrames;

      //Section Identifiers
      protected int mMaxIdentifiers;
      public int mIdentiferCount;
      public BProfileSection[] mSections  = new BProfileSection[512];

      SampleBuffer mProcessSampleBuffer = new SampleBuffer(65000);//20000);  //was 2000....

      //Error Stuff
      int mFrameNetworkErrors;

      //stats stuff
      protected long mNumSamples;

      public SampleManager()
      {
         ResourcePool.GetPool().SetFactory(new SampleBufferFactory());

         mNumSamples = 0;

         mOldestFullFrame = 0;
         mNumFullFrames = 0;
         mFullFrameQuota = 10000;
         mMaxFullFrames = mFullFrameQuota + 2;

         mCurrentFrame = null;
         mLastFrame = null;
         mFrames = new ArrayList();
         mFrameObservers = new ArrayList();

         mMaxIdentifiers = 512;
         mIdentiferCount = 0;
         mFrameNetworkErrors = 0;
         
         ErrorHandler.OnError+=new ErrorEvent(ErrorHandler_OnError);

      }
      public void AddObserver(IFrameObserver observer)
      {
         mFrameObservers.Add(observer);
      }
      public void CleanupFrameMemory()
      {
         while((mNumFullFrames >= mFullFrameQuota) && (mOldestFullFrame < mFrames.Count))
         {
            mNumFullFrames--;
            ((Frame)mFrames[mOldestFullFrame]).Collapse();
            mOldestFullFrame++;
         }
      }

      //ArrayList mLostFrames
      ulong mLastFrameNumber = 1;
      ulong mMissingFrames = 0;
      ulong mRawFrameCount = 0;
      const int cFrameStatPeriod = 1;
      TimeTest mFrameRateTimer = null;
      double mAppFrameRate = 0;

      double mGameFrameRate = 0;

      StatBuffer mAppFrameRateAverage = new StatBuffer(5);
      StatBuffer mGameFrameRateAverage = new StatBuffer(5);


      public void FrameStats()
      {
         if(this.mCurrentFrame.mFrameNumber > (mLastFrameNumber + 1))
         {
            mMissingFrames = mMissingFrames + (mCurrentFrame.mFrameNumber - mLastFrameNumber) - 1;
         }
         mLastFrameNumber = this.mCurrentFrame.mFrameNumber;

         mRawFrameCount++;

         if((mRawFrameCount % cFrameStatPeriod) == 0)
         {
            if(mFrameRateTimer == null)
            {
               mFrameRateTimer = new TimeTest();
            }
            else
            {
               mAppFrameRate = cFrameStatPeriod * 1000 / mFrameRateTimer.GetElapsed();
               mAppFrameRateAverage.InsertValue(mAppFrameRate);
               mFrameRateTimer.Start();
            }
         }

         if(mLastFrameStartTime != 0)
         {
            mGameFrameRate = 1f / (mCurrentFrame.mStartTime - mLastFrameStartTime);

            mGameFrameRateAverage.InsertValue(mGameFrameRate);
         }
         mLastFrameStartTime = mCurrentFrame.mStartTime;
         //mCurrentFrame.
         //game frame rate from last start to start
         //game frame rate from measurement start measurement to measurement end
      }

      double mLastFrameStartTime = 0;
      private void ErrorHandler_OnError(string error)
      {
         if(error.IndexOf("Network Error") != -1)
         {
            mFrameNetworkErrors++;
         }
      }
      public string FrameReport()
      {
         return String.Format("MissingFrames(not sent): {0} Frames lost from network Errors:{1} ", mMissingFrames, mFrameNetworkErrors);
      }
      public string FrameRates()
      {
         return String.Format("AppFrameProcessRate:{0:.#} GameFrameRate:{1:.#}",mAppFrameRate, mGameFrameRate);
      }

      //add progress callback
      public bool SaveData(string fileName, int startFrame, int endFrame)
      {
         if(mFrames.Count == 0)
            return false;

         FileStream s = File.Create(fileName);
         BinaryWriter writer = new BinaryWriter(s);
         if(SaveSectionData(writer) == false)
            return false;

         //save frames
         startFrame = 0;
         endFrame = mFrames.Count - 1;

         writer.Write(startFrame);
         writer.Write(endFrame);

         for(int i = startFrame; i < endFrame; i++)
         {
            Frame f = (Frame)(mFrames[i]);
            if(f.WriteFrame(writer) == false)
               return false;
         }

         writer.Close();
         return true;
      }
 
      public bool SaveSectionData(BinaryWriter writer)
      {
         writer.Write(mIdentiferCount);
         for(int i=0; i<mIdentiferCount; i++)
         {
            BProfileSection section = mSections[i];
            writer.Write(section.mID);
            writer.Write(section.mName);
            writer.Write(section.mUIEnabled);
            writer.Write(section.mCPUOnly);
         }  
         return true;
      }
      public bool LoadData(string fileName)
      {
         FileStream s = File.OpenRead(fileName);
         BinaryReader reader = new BinaryReader(s);
         if(LoadSectionData(reader) == false)
            return false;

         int startFrame = reader.ReadInt32();
         int endFrame = reader.ReadInt32();

         //mFrames = new ArrayList();
         //for(int i = startFrame; i < endFrame; i++)
         try
         {
            while(true)
            {            
               
               bool bHasData = reader.ReadBoolean();
               if(bHasData == true)
               {
                  CompressedFrame frame = new CompressedFrame();
                  if(frame.ReadFrame(reader) == false)
                  {
                     return false;
                  }
                  else
                  {
                     insertCompressedFrame(frame);

                     //                  //read sub frames
                     //                  int numthreadFrames = reader.ReadInt32();
                     //                  for(int j=0; j<numthreadFrames; j++)
                     //                  {
                     //                     CompressedFrame subRame = new CompressedFrame();
                     //                     if(subRame.ReadFrame(reader) == true)
                     //                     {
                     //                        insertCompressedFrame(subRame);
                     //                     }
                     //                  }                  
                  }
                  
               }
            }

         }
         catch(System.IO.EndOfStreamException ex)
         {
            ex.ToString();           
         }         
         reader.Close();
         return true;
      }
      public bool LoadSectionData(BinaryReader reader)
      {
         mIdentiferCount = reader.ReadInt32();
         
         for(int i=0; i<mIdentiferCount; i++)
         {
            BProfileSection section = new BProfileSection();
            mSections[i] = section;
            section.mID = reader.ReadUInt16();
            section.mName = reader.ReadString();
            section.mUIEnabled = reader.ReadBoolean();
            section.mCPUOnly = reader.ReadBoolean();
         }
         return true;
      }
      //quick load...


//     public string TimeReport()
//     {
//         double m1 = (mLatestMeasurementTime - mFirstMeasurementTime) * 1000;
//         double m2 = ((System.TimeSpan)(mLastTime - mStartTime)).TotalMilliseconds;
//         return String.Format("Meas Elapsed: {0} App Elapsed:{1}  Difference:{2}",m1, m2, m2-m1);
//     }

      public void ClearSections()
      {
         mIdentiferCount = 0;
      }
      public void insertIdentifier(BProfileSection identifier)
      {
         //CLM we're getting crashes here. If we're indexing outside of the range, don't insert
         if (mIdentiferCount<0 || mIdentiferCount >= mSections.Length)
            return;

         BProfileSection sec = identifier;
         sec.mID = (ushort)mIdentiferCount; 
         sec.mEnabled = true;
         sec.mUIEnabled = true;
         mSections[mIdentiferCount] = sec;
         mIdentiferCount++;
      }

      //extract decompress and support storage of compressed frames...
      ArrayList mFutureFrameBuffer = new ArrayList();
      public void insertCompressedFrame(CompressedFrame frame)
      {
         //SampleBuffer buffer = mBuffers[mProcessSampleBufferID] as SampleBuffer;
         SampleBuffer buffer = mProcessSampleBuffer;

         if(frame.mNumSamples > buffer.mSize)
         {
            frame.mNumSamples = buffer.mSize;
         }
         mCurrentFrame = new Frame(frame, mSections);

         //if (mLastFrame != null && mLastFrame.mFrameNumber == 2363)
         //{
         //   int count = 3;
         //}

         if(mCurrentFrame.DecompressFrame(buffer.mDecomp, ref buffer.mDecodedSamples) == true)
         {
            if (buffer.mDecodedSamples[0].mSectionID < mSections.Length &&
                mSections[buffer.mDecodedSamples[0].mSectionID] != null &&
               (mSections[buffer.mDecodedSamples[0].mSectionID].mName == "Frame"))
            {
               mNumFullFrames++;          
               if(mNumFullFrames >= mMaxFullFrames)
               {
                  CleanupFrameMemory();
               }   
         
               if(mLastFrame != null)
               {
                  mLastFrame.mNextFrame = mCurrentFrame;
                  mCurrentFrame.mLastFrame = mLastFrame;

                  mLastFrame.CalculateFinalStats();
                  mLastFrame.mNextFrameStartTime = mCurrentFrame.mStartTime;
                  FrameStats();//mCurrentFrame;
                  OnNewFrame(mLastFrame);
                  mFrames.Add(mLastFrame);  
               }
               mLastFrame = mCurrentFrame;  
               ArrayList temp = new ArrayList();
               foreach (Frame t in mFutureFrameBuffer)
               {
                  if(t.mFrameNumber == mLastFrame.mFrameNumber)
                  {
                     mLastFrame.mThreadFrames.Add(t);
                  }
                  else
                  {
                     temp.Add(t);
                  }
               }
               mFutureFrameBuffer = temp;
               if(mFutureFrameBuffer.Count > 0)
               {
                  "asdf".ToString();
                  //oops
               }
                  
            }
            else if(mLastFrame != null)//if thread data
            {
               //mLastFrame.AddThreadFrame(mCurrentFrame);

               if(mLastFrame.mFrameNumber == mCurrentFrame.mFrameNumber)
               {
                  mLastFrame.AddThreadFrame(mCurrentFrame);
               }
               //Match up out of order data...
               else if(mLastFrame.mFrameNumber < mCurrentFrame.mFrameNumber)
               {
                  mFutureFrameBuffer.Add(mCurrentFrame);
               }
               else if(mLastFrame.mFrameNumber > mCurrentFrame.mFrameNumber)
               {
                  ulong back = mLastFrame.mFrameNumber - mCurrentFrame.mFrameNumber;  
                  Frame prev = mLastFrame.mLastFrame;
                  for(ulong i=0; i < back; i++)
                  {
                     if(prev == null) break;
                     if(prev.mThreadID == mCurrentFrame.mThreadID)
                     {
                        prev.AddThreadFrame(mCurrentFrame);
                     }      
                     prev = mLastFrame.mLastFrame;              
                  }
               }
            } 
         }
      }
 
      virtual protected void OnNewFrame(Frame f)
      {
         SendFrameToObservers(f);
      }
      protected void SendFrameToObservers(Frame f)
      {
         foreach(IFrameObserver o in mFrameObservers)
         {
            if(mCurrentFrame != null)
            {
               o.NewFrame(f);
            }
         }
      }
      public override string ToString()
      {
         return String.Format(" TotalNumSamples: {0}",mNumSamples);
         
      }
   }
   #endregion

   #region Helper stuff
   
   class StatBuffer
   {
      double[] mValues;
      int mBufferSize;
      int mNumSamples;
      int mCurrentIndex;
      public StatBuffer(int numEntries)
      {
         mBufferSize = numEntries;
         mNumSamples = 0;
         mCurrentIndex = 0;
         mValues = new double[numEntries];
      }
      public void InsertValue(double val)
      {
         mValues[mCurrentIndex] = val;
         mCurrentIndex++;

         mNumSamples++;
         if(mCurrentIndex >= mBufferSize)
         {
            mNumSamples = mBufferSize;
            mCurrentIndex = 0;
         }
         if(mNumSamples >= mBufferSize)
         {
            mNumSamples = mBufferSize;
         }

      }
      public double GetAverage()
      {
         double total = 0;

         for(int i=0; i<mNumSamples; i++)
         {
            total += mValues[i];
         }
         if(mNumSamples == 0)
            return 0;
         return total / mNumSamples;
      }
   }

//   public class Sets
//   {
//      public ArrayList Union(ArrayList a, ArrayList b, System.Collections.IComparer comparer)
//      {
//         ArrayList result = new ArrayList(a);
//         foreach(object bobj in b)
//         {            
//            if (comparer.Compare(aobj,bobj) == 0)
//            {
//               result.Add(aobj);
//            }
//         }      
//      }
//   };

   #endregion

   #region network stuff
      public class TimelineControlPacket
      {
         public enum TimelineControlValues
         {
            cStart = 1,
            cStop,
            cFrameAck,
            cConfigureSections
         }
         public TimelineControlPacket(Stream stream, TimelineControlValues controlValue)
         {
            WritePacket(stream, controlValue, 0, null, null);
         }
         public TimelineControlPacket(Stream stream, TimelineControlValues controlValue, ulong frameAck)
         {
            WritePacket(stream, controlValue, frameAck, null, null );
         }
         public TimelineControlPacket(Stream stream, TimelineControlValues controlValue, ArrayList SectionIDs, ArrayList Values)
         {
            WritePacket(stream, controlValue, 0 , SectionIDs, Values );
         }
         public void WritePacket(Stream stream, TimelineControlValues controlValue, ulong frameAck, ArrayList SectionIDs, ArrayList Values)
         {
            try
            {
               //BinaryWriter writer = new BinaryWriter(stream, System.Text.Encoding.Unicode);
               BinaryWriter writer = NetServiceConnection.GetBinaryWriter(stream);

               CTSPacketHeader header = new CTSPacketHeader(writer, (byte)TimelinePacketTypes.cTimelineControl);
               writer.Write((byte)controlValue);

               if(controlValue == TimelineControlPacket.TimelineControlValues.cFrameAck)
               {
                  //hmmm, ulong is only 4 bytes on xbox??
                  writer.Write((int)frameAck);
               }
               else if(controlValue == TimelineControlPacket.TimelineControlValues.cConfigureSections)
               {
                  writer.Write((int)SectionIDs.Count);
                  for(int i = 0; i < SectionIDs.Count; i++)
                  {
                     int id = (int)SectionIDs[i];
                     byte val = (byte)Values[i];
                     writer.Write(id);
                     writer.Write(val);
                  }
               }

               //-- get the size
               header.writeSize(writer);
            }
            catch (Exception e) 
            {
               Console.WriteLine(e.ToString());
            }
         }

      }
      public class SampleIdentifierPacket
      {
         public SampleIdentifierPacket(BinaryReader reader)
         {
            try
            {
               mIdentifiers = new ArrayList();
               mIdentifierCount = reader.ReadUInt16();
               mStartIdentifier = reader.ReadUInt16();

               for(int i=0; i<mIdentifierCount; i++)
               {   
                  BProfileSection identifier = new BProfileSection();
                  int length = reader.ReadInt16();
                  byte[] bytes = reader.ReadBytes(length);
                  identifier.mName = System.Text.Encoding.ASCII.GetString(bytes,0,length);
                  identifier.mCPUOnly = (reader.ReadByte() == 0)?false:true;
                  //todo add enabled state
                  mIdentifiers.Add(identifier);
               }
               if(mIdentifierCount != mIdentifiers.Count)
               {
                  throw new System.Exception(String.Format("Expected {0} identifiers, received {1}",mIdentifierCount,mIdentifiers.Count));
               }
            }
            catch (Exception e) 
            {
               Console.WriteLine(e.ToString());
            }
         }
         public ArrayList mIdentifiers;
         private ushort	mIdentifierCount;
         private ushort mStartIdentifier;  
      }
#if false   
   public class CompressedFramePacket
   {
      public CompressedFramePacket(BinaryReader reader)
      {
         try
         {
            mFrameNumber = (ulong)reader.ReadUInt32();//.ReadUInt64();
            mTotalSamples = reader.ReadUInt32();
            mTotalLength = reader.ReadUInt32();
            mLength = reader.ReadUInt32();
            mData = reader.ReadBytes((int)mLength);
         }
         catch (Exception e) 
         {
            Console.WriteLine(e.ToString());
         }
      }
      public ulong mFrameNumber;
      public uint mTotalSamples;
      public uint mTotalLength;
      public uint mLength;
      public byte[] mData;
   }
#endif
      public class ProfilerConnectionHandler
      {
         PerformanceSampleConsumer mObserver;

         public ProfilerConnectionHandler()
         {  
            mCurrentFrame = null;
         }
         public void AddObserver(PerformanceSampleConsumer observer)
         {
            mObserver = observer;
         }
         public void HandleMessage(byte type, BinaryReader reader)
         {
            if(type == (byte)TimelinePacketTypes.cSampleIdentifierPacket)
            {
               SampleIdentifierPacket packet = new SampleIdentifierPacket(reader);
               foreach(BProfileSection s in packet.mIdentifiers)
               {
                  mObserver.insertIdentifier(s);
               }
            }
            else if(type == (byte)TimelinePacketTypes.cCompressedFramePacket)
            {
               mThreadID = reader.ReadInt32();
               mFrameNumber = (ulong)reader.ReadUInt32();
               mStartTime = reader.ReadDouble();
               mTotalSamples = reader.ReadUInt32();
               mTotalLength = reader.ReadUInt32();
               mLength = reader.ReadUInt32();
               if((mCurrentFrame != null) 
                  && ((mBytesWritten == mCurrentFrame.mTotalSize)
                     || (mCurrentFrame.mFrameNumber != mFrameNumber)))
               {
                  mObserver.insertCompressedFrame(mCurrentFrame);
                  mCurrentFrame = null;
               }
               if(mCurrentFrame == null)
               {
                  mCurrentFrame = new CompressedFrame();
                  mCoderBuf = new BDynamicCoderBuf((int)mTotalLength);
                  mCurrentFrame.SetBuffer(mCoderBuf);
                  mCurrentFrame.mThreadID = mThreadID;
                  mCurrentFrame.mFrameNumber = mFrameNumber;
                  mCurrentFrame.mStartTime = mStartTime;
                  mCurrentFrame.mNumSamples = mTotalSamples;
                  mCurrentFrame.mTotalSize = mTotalLength;
                  mBytesWritten = 0;
               }
               reader.ReadBytes((int)mLength).CopyTo(mCoderBuf.getBuf(), mBytesWritten);
               mBytesWritten += mLength;
            }
         }
         CompressedFrame mCurrentFrame;
         ICoderBuffer mCoderBuf;
         uint mBytesWritten = 0;
         ulong mFrameNumber = 0;
         uint mTotalLength = 0;
         uint mTotalSamples = 0;
         uint mLength = 0;
         int mThreadID = 0;
         double mStartTime = 0;
      }
      #endregion

}


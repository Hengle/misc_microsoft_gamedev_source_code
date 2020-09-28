using System;
using System.IO;

namespace EnsembleStudios.RemoteGameDebugger.Profiler
{
	/// <summary>
	/// Summary description for CompressionTest.
	/// </summary>
	public class CompressionTest
	{
      StreamWriter log = new StreamWriter("compression.txt");
      public CompressionTest()
      {

      }

      public void SimpleTest()
      {
         mCurFrame = new BRetiredProfileSample[2];
         
         uint i = 0;
         mCurFrame[i].mCPUStartTime = 2000;
         mCurFrame[i].mCPUEndTime = 3000;
         mCurFrame[i].mGPUStartTime = 2000;
         mCurFrame[i].mGPUEndTime = 2000;
         mCurFrame[i].mLevel = 0;
         mCurFrame[i].mSectionID = 0;
         mCurFrame[i].mUserID = 0;
         i++;
         mCurFrame[i].mCPUStartTime = 3000 + i;
         mCurFrame[i].mCPUEndTime = 4000 + i;
         mCurFrame[i].mGPUStartTime = 2000 + i;
         mCurFrame[i].mGPUEndTime = 2000 + i;
         mCurFrame[i].mLevel = 0;
         mCurFrame[i].mSectionID = 0;
         mCurFrame[i].mUserID = 0;
        

         mSections = new BProfileSection[1];
         mSections[0].mCPUOnly = true;

         compressFrame();

         log.Flush();
      }

//bit packer test...
      BRetiredProfileSample[] mCurFrame;
      //BAlignedArray<const BProfileSection*> mSections;
      BProfileSection[] mSections;


//#ifdef TIMELINE_SAMPLE_CODEC_TEST   
      BTimelineSampleCodec mComp = new BTimelineSampleCodec();
      BTimelineSampleCodec mDecomp = new BTimelineSampleCodec();
//#endif

      void BASSERT(bool val)
      {
         if(val == false)
         {
            throw new System.Exception("BASSERT");
         }
      }
      void BASSERT(int val)
      {
         if(val == 0)
         {
            throw new System.Exception("BASSERT");
         }
      }
      void blogtrace(string text)
      {
         log.WriteLine(text);
      }
      private uint size(BRetiredProfileSample[] buffer)
      {
         return (uint)buffer.Length;
      }
      private uint size(BProfileSection[] buffer)
      {
         return (uint)buffer.Length;
      }

      UInt64 ReadCycleCounter(){return 0;}

      UInt64 totalTime = 0; 
      uint frameCount = 0;

      void compressFrame()
      {
      //#ifdef TIMELINE_SAMPLE_CODEC_TEST   
         
         //static BDynamicCoderBuf coderBuf(65536);
         //BBitPacker<BDynamicCoderBuf> bitPacker(&coderBuf);
         BDynamicCoderBuf coderBuf = new BDynamicCoderBuf(65536);
         BitPacker bitPacker = new BitPacker(coderBuf);

         
      //#ifdef DUMP_OVERHEAD_TIME_TO_TRACE         
         UInt64 startTime = ReadCycleCounter();
      //#endif
         
         // Reset the compressor's model to empty and compress all the samples to a buffer.
         mComp.resetModel();
            
         coderBuf.setPos(0);
         bitPacker.encodeStart();
         
         bool success;
         for (uint i = 0; i < size(mCurFrame); i++)
         {
            /*const BRetiredProfileSample&*/BRetiredProfileSample sample = mCurFrame[i];
            BASSERT(sample.mSectionID < size(mSections));
                  
            success = mComp.codeSample(bitPacker, ref sample, ref mSections[sample.mSectionID]);
            BASSERT(success);
         }
         
         /*const*/ uint totalInBytes = (uint)(size(mCurFrame) * 23/*sizeof(BRetiredProfileSample)*/);
         /*const*/ uint totalOutBytes = coderBuf.getPos();
         
         success = bitPacker.encodeEnd();
         BASSERT(success);

     //#if 1   
         // Now decompress the samples and compare them to the original samples.
         mDecomp.resetModel();  
         
         coderBuf.setPos(0);
            
         success = bitPacker.decodeStart();
         BASSERT(success);
               
         for (uint i = 0; i < size(mCurFrame); i++)
         {
            /*const BRetiredProfileSample&*/BRetiredProfileSample sample = mCurFrame[i];
            BASSERT(sample.mSectionID < size(mSections));
                        
            BRetiredProfileSample decodedSample = new BRetiredProfileSample();
            success = mDecomp.decodeSample(bitPacker, ref decodedSample, mSections);
            BASSERT(success);
            
            
            BASSERT(decodedSample.mSectionID == sample.mSectionID);
            BASSERT(decodedSample.mUserID == sample.mUserID);
            BASSERT(decodedSample.mLevel == sample.mLevel);
            
            BASSERT(decodedSample.mCPUStartTime == sample.mCPUStartTime);
            BASSERT(decodedSample.mCPUEndTime == sample.mCPUEndTime);
            if (sample.hasGPUTimes())
            {
               BASSERT(decodedSample.hasGPUTimes());
               BASSERT(decodedSample.mGPUStartTime == sample.mGPUStartTime);
               BASSERT(decodedSample.mGPUEndTime == sample.mGPUEndTime);
            }
            else
            {
               BASSERT(!decodedSample.hasGPUTimes());
            }
         }
      //#endif   
         
      //#ifdef DUMP_OVERHEAD_TIME_TO_TRACE         
         UInt64 endTime = ReadCycleCounter();

         /*static UInt64 totalTime;*/ 
         /*static uint frameCount;*/
         totalTime += (endTime - startTime);
         frameCount++;
         if (frameCount == 60)
         {
            //blogtrace(String.Format("Compress/decompress: %4.3f ms/frame", totalTime / (float)(frameCount) * gpProfileManager->getOOCompCPUFreq() * 1000.0f));

            frameCount = 0;
            totalTime = 0;
         }

         if (size(mCurFrame) > 0)
         {
            blogtrace(String.Format("Compression samples: %i, Bytes: %i, Compressed bytes: %i, Ave. bytes per sample: %f, Ave. bits per byte: %f", 
               size(mCurFrame),
               totalInBytes,
               totalOutBytes,
               (float)(totalOutBytes) / size(mCurFrame),
               (totalOutBytes * 8.0f) / totalInBytes));
         }  
      //#endif
         
      //#endif TIMELINE_SAMPLE_CODEC_TEST   
      }
	}
}

// File: vidToAvi.cpp
#include "xcore.h"
#include "xcoreLib.h"
#include "consoleOutput.h"
#include "containers\hashMap.h"
#include "utils\consoleAppHelper.h"
#include "stream\cfileStream.h"
#include "stream\byteStream.h"
#include "xdb\xdb.h"
#include "writeTGA.h"
#include "RGBAImage.h"
#include "ImageUtils.h"
#include "gpuDXTVideoHeaders.h"
#include "dxtUnpacker.h"
#include "imageUtils.h"
#include "file\win32FileUtils.h"
#include "aviMaker.h"
#include "wmvFile.h"
#include "xbdm.h"
#include <xgraphics.h>
#include <errno.h>

//-------------------------------------------------

const float cDefaultFrameRate = 30.0f;

class BVidToAviUtil
{
   BVidToAviUtil(const BVidToAviUtil&);
   BVidToAviUtil& operator= (const BVidToAviUtil&);
   
public:
   BVidToAviUtil() :
      mFrameRate(cDefaultFrameRate),
      mSpecifiedFrameRateFlag(false),
      mpAviMaker(NULL),
      mpWmvFile(NULL),
      mOutputImages(false),
      mOutputExactFrames(false),
      mNumInputFiles(0),
      mCurInputFileIndex(0)
   {
      char buf[MAX_PATH];
      buf[0] = '\0';
      GetModuleFileNameA(GetModuleHandle(NULL), buf, sizeof(buf));

      BString path, name;
      strPathSplit(BString(buf), path, name);
      mExecDirectory = path;
   }
   
   ~BVidToAviUtil()
   {
      cleanup();
   }
      
   bool process(BCommandLineParser::BStringArray& args)
   {
      cleanup();
      
      if (!parseArgs(args))
         return false;

      if (!copyFilesFromXbox())
         return false;
         
      if (!scanInputFiles())
         return false;
         
      if (!openInputFile())
         return false;
            
      if (!openOutputFile())
         return false;
      
      if (!processFile())
         return false;
            
      cleanup();
      
      gConsoleOutput.printf(" Total input frames: %u\n", mFrameCaptureTimes.getSize());
      gConsoleOutput.printf("Total output frames: %u\n", mCurOutputFrameIndex);
                  
      return true;
   }

private:
   BString                          mExecDirectory;
   
   BString                          mFilenames[2];
   BString                          mWMVProfile;
   bool                             mOutputImages;
   float                            mFrameRate;
   bool                             mSpecifiedFrameRateFlag;
   bool                             mOutputExactFrames;
   
   BAVIMaker*                       mpAviMaker;
   CwmvFile*                        mpWmvFile;
   BCFileStream                     mInputFile;
   BGPUDXTVideoFileHeader           mFirstFileHeader;
   BGPUDXTVideoFileHeader           mFileHeader;
   
   uint                             mNumInputFiles;
   uint                             mCurInputFileIndex;
   
   void printHelp(void)
   {
      gConsoleOutput.printf("Usage: vidToAvi inputfile.vid outputfile.avi/outputfile.wmv\n");
      gConsoleOutput.printf("Options:\n");
      gConsoleOutput.printf(" -frameRate X            Set output AVI file's framerate in Hz. (Default=30Hz)\n");
      gConsoleOutput.printf(" -outputImages           Write frames as individual .TGA files\n");
      gConsoleOutput.printf(" -wmvProfile profile.prx Use WMV compression profile file\n");
      gConsoleOutput.printf(" -outputExactFrames      Do not resample input video to fixed framerate\n");
      gConsoleOutput.printf("\n");
      BConsoleAppHelper::printHelp();
      gConsoleOutput.printf("\n");
      gConsoleOutput.printf("Notes:\n");
      gConsoleOutput.printf("If -outputExactFrames is specified, vidToAvi will write all input frames to the\n");
      gConsoleOutput.printf("output video unchanged. Otherwise, input frames will be skipped or duplicated\n");
      gConsoleOutput.printf("as needed to track the desired output frame rate.\n");
      gConsoleOutput.printf("\n");
      gConsoleOutput.printf("For WMV files: If -outputExactFrames is specified, each frame's\n");
      gConsoleOutput.printf("timestamp is copied directly from the input .VID file. The WMV header average\n");
      gConsoleOutput.printf("framerate will be set to the value of -frameRate, OR the value from the VID file\n");
      gConsoleOutput.printf("header if the framerate was locked via the EnableFPSLock config.\n");
      
   }
   
   bool parseArgs(BCommandLineParser::BStringArray& args)
   {
      mFrameRate = cDefaultFrameRate;
      mSpecifiedFrameRateFlag = false;
      mOutputImages = false;
      mWMVProfile.empty();
      
      const BCLParam params[] = 
      {
         { "frameRate", cCLParamTypeFloatPtr, &mFrameRate, 0, &mSpecifiedFrameRateFlag },
         { "outputImages", cCLParamTypeFlag, &mOutputImages },
         { "wmvProfile", cCLParamTypeBStringPtr, &mWMVProfile },
         { "outputExactFrames", cCLParamTypeFlag, &mOutputExactFrames }
      };
      const uint cNumCommandLineParams = sizeof(params)/sizeof(params[0]);
      
      BCommandLineParser parser(params, cNumCommandLineParams);
      const bool success = parser.parse(args, true, false);
      if (!success)
      {
         gConsoleOutput.error("%s\n", parser.getErrorString());
         return false;
      }
      
      const BDynamicArray<uint>& unparsedParams = parser.getUnparsedParams();
   
      uint numFilenames = 0;
      
      for (uint j = 0; j < unparsedParams.getSize(); j++)
      {
         const uint i = unparsedParams[j];
         
         if (args[i].isEmpty())
            continue;
            
         if (numFilenames >= 2)
         {
            gConsoleOutput.error("Too many filenames!\n");
            return false;  
         }
                           
         mFilenames[numFilenames++] = args[i];
      }
      
      if (numFilenames != 2) 
      {
         printHelp();
         return false;
      }
      
      mFrameRate = Math::Clamp(mFrameRate, 1.0f, 100.0f);
            
      return true;
   }
   
   void cleanup()
   {
      mInputFile.close();
      
      if (mpAviMaker)
      {
         delete mpAviMaker;
         mpAviMaker = NULL;
      }
      
      if (mpWmvFile)
      {
         delete mpWmvFile;
         mpWmvFile = NULL;
      }
   }
   
   bool isXboxPath(const char* pFilename)
   {
      // This is by no means complete - but should be good enough for this tool.
      // Recognizes only "xe:\" prefix.
      
      if (strlen(pFilename) < 4)
         return false;
         
      if ( (tolower(pFilename[0]) == 'x') && (tolower(pFilename[1]) == 'e') && (pFilename[2] == ':') && (pFilename[3] == '\\') )
         return true;
      
      return false;
   }
   
   bool copyFilesFromXbox(void)
   {
      if (!isXboxPath(mFilenames[0]))
         return true;
      
      uint i;
      for (i = 0; i < 1000; i++)
      {
         BString tempFilename;
         tempFilename.format("vidToAviTemp_%02i.vid", i);
         if (-1 == remove(tempFilename))
         {
            if (errno != EACCES)
               break;
            else
               gConsoleOutput.error("Unable to delete temporary file: %s\n", tempFilename.getPtr());
         }
         else
         {
            gConsoleOutput.printf("Deleted temporary file: %s\n", tempFilename.getPtr());
         }
      }
      
      for (i = 0; i < 1000; i++)
      {
         BString srcFilename(mFilenames[0]);
         srcFilename.right(1);
         strPathRemoveExtension(srcFilename);
         
         if (srcFilename.findLeft("_00") == (srcFilename.length() - 3))
            srcFilename.left(srcFilename.length() - 3);

         BString suffix;
         suffix.format("_%02i.vid", i);
         srcFilename += suffix;
      
         DM_FILE_ATTRIBUTES attribs;
         HRESULT hres = DmGetFileAttributes(srcFilename, &attribs);
         if (XBDM_NOERR == hres)
         {
            BString dstFilename;
            dstFilename.format("vidToAviTemp_%02i.vid", i);
                        
            gConsoleOutput.printf("Copying file \"%s\" to \"%s\"\n", srcFilename.getPtr(), dstFilename.getPtr());
            hres = DmReceiveFile(dstFilename, srcFilename);
            if (FAILED(hres))
            {
               gConsoleOutput.error("Failing copying file \"%s\" to \"%s\"!\n", srcFilename.getPtr(), dstFilename.getPtr());
               return false;
            }
         }
         else
            break;
      }
               
      mFilenames[0] = "vidToAviTemp";
      return true;
   }
   
   bool scanInputFiles(void)
   {
      uint i;
      for (i = 0; i < 1000; i++)
      {
         BString filename(mFilenames[0]);
         strPathRemoveExtension(filename);
         
         if (filename.findLeft("_00") == (filename.length() - 3))
            filename.left(filename.length() - 3);

         BString suffix;
         suffix.format("_%02i.vid", i);
         filename += suffix;
                                    
         if (!BWin32FileUtils::doesFileExist(filename))
         {
            if (i == 0)
            {
               gConsoleOutput.error("Unable to open file: %s\n", filename.getPtr());
               return false;
            }

            break;
         }
      }

      mNumInputFiles = i;
      mCurInputFileIndex = 0;
      
      gConsoleOutput.printf("Found %u input files\n", mNumInputFiles);
      
      return true;
   }
   
   bool openInputFile(void)
   {
      mInputFile.close();
      
      BString filename(mFilenames[0]);
      strPathRemoveExtension(filename);

      if (filename.findLeft("_00") == (filename.length() - 3))
         filename.left(filename.length() - 3);

      BString suffix;
      suffix.format("_%02i.vid", mCurInputFileIndex);
      filename += suffix;
      
      if (!mInputFile.open(filename, cSFReadable))
      {
         gConsoleOutput.error("Unable to open file: %s\n", filename.getPtr());
         return false;
      }
      
      if (mInputFile.readBytes(&mFileHeader, sizeof(mFileHeader)) != sizeof(mFileHeader))
      {
         gConsoleOutput.error("Failed reading from file: %s\n", filename.getPtr());
         return false;
      }
                  
      if (cLittleEndianNative)
         mFileHeader.endianSwap();
         
      if (mFileHeader.mSig != BGPUDXTVideoFileHeader::cSig)
      {
         gConsoleOutput.error("Unrecognized header: %s\n", filename.getPtr());
         return false;
      }
                  
      if ((mFileHeader.mWidth & 3) || (mFileHeader.mHeight & 3) || (!mFileHeader.mWidth) || (!mFileHeader.mHeight))
      {
         gConsoleOutput.error("Unrecognized header: %s\n", filename.getPtr());
         return false;
      }
      
      if (!mCurInputFileIndex)
         mFirstFileHeader = mFileHeader;
      else
      {
         if ((mFileHeader.mWidth != mFirstFileHeader.mWidth) || (mFileHeader.mHeight != mFirstFileHeader.mHeight))
         {
            gConsoleOutput.error("File is incompatible with first file in sequence: %s\n", filename.getPtr());
            return false;   
         }
      }
      
      gConsoleOutput.printf("Opened input file %s\n", filename.getPtr());
      
      mCurInputFileIndex++;
      
      return true;
   }
   
   bool openOutputFile(void)
   {
      if (mpAviMaker)
         delete mpAviMaker;
      
      if (mOutputImages)
      {
         gConsoleOutput.printf("Writing individual frames to TGA files\n");
         return true;
      }
                  
      if (!mSpecifiedFrameRateFlag)
      {
         if (mFileHeader.mAverageFPS > 0.0f)
         {
            gConsoleOutput.printf("Output framerate not specified. Using .VID file average framerate of %fHz\n", mFileHeader.mAverageFPS);
         
            mFrameRate = mFileHeader.mAverageFPS;
         }
      }
         
      if (mFilenames[1].contains(".wmv"))
      {
         gConsoleOutput.printf("Writing WMV file\n");
         
         if (mWMVProfile.isEmpty())
         {
            if (mFileHeader.mWidth == 1280)
               mWMVProfile = "720P_8MB.prx";
            else
               mWMVProfile = "640_2MB.prx";
            
            mWMVProfile = mExecDirectory + mWMVProfile;
         }
         
         BByteArray prxData;
         if (!BWin32FileUtils::readFileData(mWMVProfile, prxData))
         {
            gConsoleOutput.printf("Unable to read file: %s\n", mWMVProfile.getPtr());
            return false;
         }
         
         prxData.pushBack(0);         
         prxData.pushBack(0);
         
         IWMProfileManager* pProfileManager = NULL;
         HRESULT hres = WMCreateProfileManager(&pProfileManager);
         if (FAILED(hres))
         {
            gConsoleOutput.error("Failed creating profile manager\n");
            return false;
         }
         
         IWMProfileManager2* pProfileManager2 = NULL;
         if (SUCCEEDED(pProfileManager->QueryInterface(IID_IWMProfileManager2,(void**)&pProfileManager2)))
         {
            pProfileManager2->SetSystemProfileVersion(WMFORMAT_SDK_VERSION);
            pProfileManager2->Release();
         }            
         
         IWMProfile* pProfile = NULL;
         hres = pProfileManager->LoadProfileByData((const WCHAR*)prxData.getPtr(), &pProfile);
         if (FAILED(hres))
         {
            gConsoleOutput.error("Failed loading profile data\n");
            
            pProfileManager->Release();
            
            return false;
         }
         
         pProfileManager->Release();
               
         const GUID& guidProfileID = WMProfile_V80_384Video;
                  
         mpWmvFile = new CwmvFile(mFilenames[1].getPtr(), guidProfileID, mFrameRate, pProfile);
      }
      else
      {
         gConsoleOutput.printf("Writing AVI file\n");
         
         mpAviMaker = new BAVIMaker;
         if (!mpAviMaker->init(BConsoleAppHelper::getHWnd(), mFilenames[1], mFileHeader.mWidth, mFileHeader.mHeight, 1.0f / mFrameRate))
         {
            gConsoleOutput.error("Failed opening output file: %s\n", mFilenames[1].getPtr());
            return false;
         }
      }         
      
      return true;
   }
   
   struct BGameFrame
   {
      BGameFrame() { clear(); }
      
      void clear() { mpImage = NULL; mCaptureTime = 0.0f; mFrameIndex = 0; }
      
      BRGBAImage* mpImage;
      double      mCaptureTime;
      uint        mFrameIndex;
   };
   
   typedef BDynamicArray<BGameFrame> BGameFrames;

   BGameFrames             mFrames;
   
   BDynamicArray<double>   mFrameCaptureTimes;
   BDynamicArray<double>   mFrameDurations;

   double                  mCurOutputTime;
   uint                    mCurOutputFrameIndex;
   double                  mSecsPerOutputFrame;
   
   bool readFrame(BGameFrame* pFrame)
   {
      pFrame->mpImage = new BRGBAImage;
      
      BRGBAImage& image = *pFrame->mpImage;
      
      BGPUDXTVideoFrameHeader frameHeader;
      
      if (mInputFile.readBytes(&frameHeader, sizeof(frameHeader)) != sizeof(frameHeader))
      {
         gConsoleOutput.error("Failed reading from file: %s\n", mFilenames[0].getPtr());
         return false;
      }

      if (cLittleEndianNative)
         frameHeader.endianSwap();

      if (frameHeader.mSig != BGPUDXTVideoFrameHeader::cSig)
      {
         gConsoleOutput.error("Invalid frame header: %s\n", mFilenames[0].getPtr());
         return false;
      }
      
      if ((mFileHeader.mFlags & BGPUDXTVideoFileHeader::cFlagRaw) == 0)
      {
         const uint numBlocksX = mFileHeader.mWidth / 4;
         const uint numBlocksY = mFileHeader.mHeight / 4;
         uint expectedDataSize = numBlocksX * numBlocksY * 8;
         
         if (frameHeader.mDataSizeInBytes != expectedDataSize)
         {
            gConsoleOutput.error("Invalid frame size: %s\n", mFilenames[0].getPtr());
            return false;
         }
      }
      
      BByteArray buf(frameHeader.mDataSizeInBytes);
      
      if (mInputFile.readBytes(buf.getPtr(), buf.getSizeInBytes()) != buf.getSizeInBytes())
      {
         gConsoleOutput.error("Failed reading from file: %s\n", mFilenames[0].getPtr());
         return false;
      }
      
      if (calcAdler32(buf.getPtr(), buf.getSizeInBytes()) != frameHeader.mDataAdler32)
      {
         gConsoleOutput.error("Frame data failed Adler-32 check: %s\n", mFilenames[0].getPtr());
         return false;
      }
      
      if (mFileHeader.mFlags & BGPUDXTVideoFileHeader::cFlagRaw)
      {
         image.setSize(mFileHeader.mWidth, mFileHeader.mHeight);
         
         XGUntileTextureLevel(
            mFileHeader.mWidth,
            mFileHeader.mHeight,
            0,
            XGGetGpuFormat((D3DFORMAT)21758214),
            XGTILE_NONPACKED,
            image.getPtr(),
            image.getPitch() * sizeof(DWORD),
            NULL,
            buf.getPtr(),
            NULL);
            
         for (uint y = 0; y < mFileHeader.mHeight; y++)
         {
            BRGBAColor* p = image.getScanlinePtr(y);
            BRGBAColor* pEnd = p + image.getWidth();
            while (p != pEnd)
            {
               std::swap(p->r, p->b);
               p++;
            }
         }
      }
      else
      {
         if (cLittleEndianNative)
            EndianSwitchWords((WORD*)buf.getPtr(), buf.getSizeInBytes() / 2);
          
         BDXTUnpacker dxtUnpacker;
         dxtUnpacker;
         
         if (!dxtUnpacker.unpack(image, buf.getPtr(), cDXT1, mFileHeader.mWidth, mFileHeader.mHeight))
         {
            gConsoleOutput.error("Failed unpacking DXT data: %s\n", mFilenames[0].getPtr());
            return false;
         }
      }         
      
      pFrame->mCaptureTime = frameHeader.mScaledTime / 1000000.0f;
      pFrame->mFrameIndex = (uint)frameHeader.mFrameIndex;
      
      return true;
   }
   
   bool outputFrame(uint frameArrayIndex, double frameTime)
   {
      if (mOutputImages)
      {
         BString filename(mFilenames[1]);
         strPathRemoveExtension(filename);
         
         BString filenameSuffix;
         filenameSuffix.format("_%05u.tga", mCurOutputFrameIndex);
         
         filename += filenameSuffix;
         
         BCFileStream outputStream;
         if (!outputStream.open(filename, cSFWritable | cSFSeekable))
         {
            gConsoleOutput.error("Failed opening output file: %s\n", filename.getPtr());
            return false;  
         }
         
         if (!BImageUtils::writeTGA(outputStream, *mFrames[frameArrayIndex].mpImage, cTGAImageTypeBGR))
         {
            gConsoleOutput.error("Failed writing to output file: %s\n", filename.getPtr());
            return false;  
         }
         
         if (!outputStream.close())
         {
            gConsoleOutput.error("Failed writing to output file: %s\n", filename.getPtr());
            return false;  
         }
      }
      else if (mpWmvFile)
      {
         BByteArray buf(mFileHeader.mWidth * mFileHeader.mHeight * 3);
         
         BYTE* pDst = buf.getPtr();
         
         for (uint y = 0; y < mFileHeader.mHeight; y++)
         {
            const BRGBAColor* pSrc = mFrames[frameArrayIndex].mpImage->getScanlinePtr(mFileHeader.mHeight - 1 - y);
                           
            for (uint x = 0; x < mFileHeader.mWidth; x++)
            {
               pDst[0] = pSrc->b;
               pDst[1] = pSrc->g;
               pDst[2] = pSrc->r;
               pSrc++;
               pDst += 3;
            }
         }
         
         HRESULT hres = mpWmvFile->AppendNewFrame(mFileHeader.mWidth, mFileHeader.mHeight, buf.getPtr(), 24, frameTime);
         if (FAILED(hres))
         {
            gConsoleOutput.error("Failed writing to output file: %s\n", mFilenames[1].getPtr());
            return false;
         }
      }
      else
      {
         if (!mpAviMaker->addFrame(mFrames[frameArrayIndex].mpImage))
         {
            gConsoleOutput.error("Failed writing to output file: %s\n", mFilenames[1]);
            return false;  
         }
      }   
      
      mCurOutputFrameIndex++;
         
      return true;                        
   }
   
   bool createOutputFrames(bool noMoreFrames)
   {
      noMoreFrames;
      
      if (mOutputExactFrames)
      {
         for (uint frameArrayIndex = 0; frameArrayIndex < mFrames.getSize(); frameArrayIndex++)
         {
            const uint inputFrameIndex = mFrames[frameArrayIndex].mFrameIndex;
            double frameStartTime = mFrameCaptureTimes[inputFrameIndex] - mFrameCaptureTimes[0];  
            
            gConsoleOutput.printf("Input Frame: %i, Output Frame: %i, Time: %f [exact]\n", inputFrameIndex, mCurOutputFrameIndex, frameStartTime);
            
            if (!outputFrame(frameArrayIndex, frameStartTime))
               return false;
                           
            delete mFrames[frameArrayIndex].mpImage;
            mFrames[frameArrayIndex].mpImage = NULL;
         }
         
         mFrames.resize(0);
      }
      else
      {
         for ( ; ; )
         {
            int i;
            for (i = 0; i < (int)mFrames.getSize(); i++)
            {
               const uint inputFrameIndex = (uint)mFrames[i].mFrameIndex;
               
               if (!mFrameDurations[inputFrameIndex])
                  continue;
               
               double frameStartTime = mFrameCaptureTimes[inputFrameIndex] - mFrameCaptureTimes[0];
               double frameEndTime = frameStartTime + mFrameDurations[inputFrameIndex];
               
               if ((mCurOutputTime >= frameStartTime) && (mCurOutputTime < frameEndTime))              
                  break;
               
               if (mCurOutputTime >= frameEndTime)
               {
                  delete mFrames[i].mpImage;
                  mFrames[i].mpImage = NULL;
                  
                  mFrames.erase(i);
                  i--;
               }
            }
            
            if (i == (int)mFrames.getSize())
               break;
            
            gConsoleOutput.printf("Input Frame: %i, Output Frame: %i, Time: %f [resampled]\n", mFrames[i].mFrameIndex, mCurOutputFrameIndex, mCurOutputTime);
            
            if (!outputFrame(i, mCurOutputTime))
               return false;
                                                
            mCurOutputTime = mCurOutputFrameIndex * mSecsPerOutputFrame;
         }      
      }         
   
      return true;
   }
     
   bool processFile(void)
   {
      mFrames.reserve(32);
      mFrames.resize(0);
      mFrameCaptureTimes.resize(0);
      mFrameDurations.resize(0);

      mCurOutputTime = 0.0f;
      mCurOutputFrameIndex = 0;
      mSecsPerOutputFrame = 1.0f / mFrameRate;
      
      for (uint inputFileIndex = 0; inputFileIndex < mNumInputFiles; inputFileIndex++)
      {
         while (mInputFile.bytesLeft())
         {
            BGameFrame* pFrame = mFrames.enlarge(1);
                     
            if (!readFrame(pFrame))
               return false;
            
            if (pFrame->mFrameIndex != mFrameCaptureTimes.getSize())
            {
               gConsoleOutput.error("Invalid input frame: %s\n", mFilenames[0]);
               return false;
            }
            
            mFrameCaptureTimes.resize(pFrame->mFrameIndex + 1);
            mFrameDurations.resize(pFrame->mFrameIndex + 1);
            
            mFrameCaptureTimes[pFrame->mFrameIndex] = pFrame->mCaptureTime;
                     
            if (!pFrame->mFrameIndex)
               continue;
            
            mFrameDurations[pFrame->mFrameIndex - 1] = pFrame->mCaptureTime - mFrameCaptureTimes[pFrame->mFrameIndex - 1];
         
            if (!createOutputFrames(false))
               return false;
         }
                           
         if (inputFileIndex != (mNumInputFiles - 1))
         {
            if (!openInputFile())
               return false;
         }
      }         

      if (mFrameDurations.getSize() == 1)
         mFrameDurations[0] = mSecsPerOutputFrame;
      else if (mFrameDurations.getSize() >= 2)
         mFrameDurations[mFrameDurations.getSize() - 1] = mFrameDurations[mFrameDurations.getSize() - 2];
      
      return createOutputFrames(true);
   }
      
};

//-------------------------------------------------

int main(int argc, const char *argv[])
{
   CoInitialize(NULL);
   
   BCommandLineParser::BStringArray args;
   if (!BConsoleAppHelper::init(args, argc, argv))
   {
      CoUninitialize();
      
      return EXIT_FAILURE;
   }
   
   if (!BConsoleAppHelper::initD3D())
   {
      CoUninitialize();
      
      BConsoleAppHelper::deinit();
      return EXIT_FAILURE;
   }
   
   gConsoleOutput.printf("VidToAvi Compiled %s %s\n", __DATE__, __TIME__);
   
   BVidToAviUtil vidToAviUtil;
   if (!vidToAviUtil.process(args))
   {
      CoUninitialize();
      
      BConsoleAppHelper::deinit();
      return EXIT_FAILURE;
   }
   
   CoUninitialize();
   
   BConsoleAppHelper::deinit();
   return EXIT_SUCCESS;
}

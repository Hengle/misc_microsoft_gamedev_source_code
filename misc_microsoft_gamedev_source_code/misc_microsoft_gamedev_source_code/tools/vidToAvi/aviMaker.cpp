// File: aviMaker.cpp
#include "xcore.h"
#include "aviMaker.h"

#pragma comment(lib, "vfw32.lib")

//============================================================================
// BAVIMaker::BAVIMaker
//============================================================================
BAVIMaker::BAVIMaker(void) :
   mAVIFile(NULL),
   mPeriod(0),
   mVideoStream(NULL),
   mVideoStreamCompressed(NULL),
   mNextFrame(0),
   mNextSample(0),
   mWidth(0),
   mHeight(0),
   mTempBuffer(NULL)
{
   ZeroMemory(&mWaveFormat, sizeof(mWaveFormat));

   // Tell AVI lib to get going.
   AVIFileInit();
}


//============================================================================
// BAVIMaker::~BAVIMaker
//============================================================================
BAVIMaker::~BAVIMaker(void)
{
   // Clean up.
   cleanup();

   // Tell AVI lib we're done.
   AVIFileExit();
}


//============================================================================
// BAVIMaker::cleanup
//============================================================================
void BAVIMaker::cleanup(void)
{
   if(mVideoStream)
   {
      AVIStreamRelease(mVideoStream);
      mVideoStream = NULL;
   }

   if(mVideoStreamCompressed)
   {
      AVIStreamRelease(mVideoStreamCompressed);
      mVideoStreamCompressed = NULL;
   }

   if(mAVIFile)
   {
      AVIFileRelease(mAVIFile);
      mAVIFile = NULL;
   }

   mNextFrame = 0;
   mNextSample = 0;
   mPeriod = 0;
   mWidth = 0;
   mHeight = 0;
   ZeroMemory(&mWaveFormat, sizeof(mWaveFormat));

   if(mTempBuffer)
   {
      delete mTempBuffer;
      mTempBuffer = NULL;
   }
}


//============================================================================
// BAVIMaker::init
//============================================================================
bool BAVIMaker::init(HWND parentHWnd, const char *filename, DWORD width, DWORD height, float period)
{
   // Clean up for safety.
   cleanup();

   // Open the file.
   HRESULT hr = AVIFileOpenA(&mAVIFile, filename, OF_WRITE|OF_CREATE, NULL);
   if(FAILED(hr))
      return(false);

   // Remember the info.
   mWidth = width;
   mHeight = height;
   mPeriod = period;

   // Create the video stream
   AVISTREAMINFO aviStreamInfo; 
   ZeroMemory(&aviStreamInfo, sizeof(aviStreamInfo));
   aviStreamInfo.fccType = streamtypeVIDEO;// stream type
   aviStreamInfo.fccHandler = 0; 
   aviStreamInfo.dwScale = (DWORD)(mPeriod * 1000000 + .5f);
   aviStreamInfo.dwRate = 1000000;
   aviStreamInfo.dwSuggestedBufferSize = width*height*3;
   SetRect(&aviStreamInfo.rcFrame, 0, 0, width, height);
   hr = AVIFileCreateStream(mAVIFile, &mVideoStream, &aviStreamInfo);
   if(FAILED(hr))
   {
      cleanup();
      return(false);
   }

   // Uncompressed for now...
   AVICOMPRESSOPTIONS opts; 
   ZeroMemory(&opts, sizeof(opts));
   AVICOMPRESSOPTIONS *compressionOptions[1] = {&opts};

   // Get options.
   BOOL res = (BOOL)AVISaveOptions(parentHWnd, 0, 1, &mVideoStream, compressionOptions);
   if (!res) 
   {
      AVISaveOptionsFree(1, compressionOptions);
      return(false);
   }

   // Make the compressed stream.
   hr = AVIMakeCompressedStream(&mVideoStreamCompressed, mVideoStream, compressionOptions[0], NULL);
   if(FAILED(hr))
   {
      cleanup();
      return(false);
   }

   // Clean up options.
   AVISaveOptionsFree(1, compressionOptions);

   // Setup bitmap info with sizing, etc.  Hardcoded to 24bpp now.
   BITMAPINFOHEADER bmi;
   bmi.biSize = sizeof(bmi);
   bmi.biWidth = long(mWidth);
   bmi.biHeight = long(mHeight);
   bmi.biPlanes = 1; 
   bmi.biBitCount = 24;
   bmi.biCompression = BI_RGB;
   bmi.biSizeImage = 0;
   bmi.biXPelsPerMeter = 0; 
   bmi.biYPelsPerMeter = 0;
   bmi.biClrUsed = 0;
   bmi.biClrImportant = 0;

   // Set stream's format.
   hr = AVIStreamSetFormat(mVideoStreamCompressed, 0, &bmi, sizeof(bmi));
   if(FAILED(hr))
      return(false);

   // Get temp buffer that fits a single frame.
   mTempBuffer = new BYTE[mWidth*mHeight*3];

   // Success.
   return(true);   
}


//============================================================================
// BAVIMaker::addFrame
//============================================================================
bool BAVIMaker::addFrame(BRGBAImage *image)
{
   // Sanity.
   if(!image)
      return(false);

   // Confirm width/height match.  TBD: rescaling?
   if((DWORD(image->getWidth()) != mWidth) || (DWORD(image->getHeight()) != mHeight))
      return(false);

   // Convert into temp buffer.
   BYTE *dest = mTempBuffer;
   
   for(DWORD y=0; y<mHeight; y++)
   {
      const BRGBAImage::colorType* src = image->getScanlinePtr(mHeight - 1 - y);
      
      for(DWORD x=0; x<mWidth; x++)
      {
         *dest = (BYTE)src->b;
         dest++;
         *dest = (BYTE)src->g;
         dest++;
         *dest = (BYTE)src->r;
         dest++;
         src++;
      }
   }

   return(addFrame(mTempBuffer));
}


//============================================================================
// BAVIMaker::addFrame
//============================================================================
bool BAVIMaker::addFrame(void *data)
{
   // Sanity.
   if(!data || !mAVIFile)
      return(false);

   // Write out the frame.      
   HRESULT hr = AVIStreamWrite(mVideoStreamCompressed, mNextFrame, 1, data, mWidth*mHeight*3, AVIIF_KEYFRAME, NULL, NULL);
   if(FAILED(hr))
      return(false);

   // Next frame.
   mNextFrame++;

   // Success.
   return(true);
}


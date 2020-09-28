//==============================================================================
//
// File: gpuDXTVideo.h
//
// Copyright (c) 2006-2008 Ensemble Studios
//
//==============================================================================
#pragma once
#include "gpuDXTVideoHeaders.h"
#include "file\win32File.h"
#include "timer.h"
#include "threading\commandFIFO.h"

// Render thread only!
class BGPUDXTVideo 
{
public:
   BGPUDXTVideo();
   ~BGPUDXTVideo();
   
   bool init(const char* pFilename, bool downsample, float fpsLockRate, bool raw, ILowLevelFileIO* pLowLevelFileIO);
   void capture(void);
   void deinit(void);
   
   const BRenderString& getOutputFilename(void) const { return mOutputFilename; }

private:
   HANDLE            mWriterThread; 
   BWin32Event       mWriterExitEvent;
   
   uint              mWidth;
   uint              mHeight;
   
   BRenderString     mOutputFilename;
   BWin32File        mOutputFile;
   
   float             mFPSLockRate;
         
   BTimer            mTimer;
             
   enum { cNumPackBuffers = 3 };
  
   struct BPackBuffer
   {
      uint64               mCaptureTime;
      uint64               mTexDataReadyFence;
      DWORD                mFrameIndex;
      IDirect3DTexture9*   mpTex;
            
      void clear(void)
      {
         mCaptureTime = 0;
         mTexDataReadyFence = UINT64_MAX;
         mFrameIndex = 0;
         mpTex = NULL;
      }
   };
   
   BPackBuffer mPackBuffers[cNumPackBuffers];
   
   typedef BDynamicRenderArray<BYTE> BWriteBuffer;
   enum { cWriteFIFOSize = 8 };
   typedef BCommandFIFO<BWriteBuffer*, cWriteFIFOSize> BWriteFIFO;
   BWriteFIFO        mWriteFIFO;
   
   DWORD             mCurFrameIndex;
   DWORD             mNextFrameIndexToWrite;
   
   uint64            mTotalBytesWritten;
   uint              mCurFileIndex;
   
   bool              mRaw : 1;   
   bool              mOutputFileWriteFailed : 1;
   bool              mDownsample : 1;
   bool              mCapturing : 1;
      
   void flushPackBuf(uint packBufIndex);
   void servicePackBuffers(void);
   
   DWORD writerThread(void);
   static DWORD __stdcall writerThreadCallback(LPVOID lpThreadParameter);
};






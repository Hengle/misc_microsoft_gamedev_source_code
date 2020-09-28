//==============================================================================
//
// File: gpuDXTVideo.cpp
//
// Copyright (c) 2006-2008 Ensemble Studios
//
//==============================================================================
#include "xgameRender.h"
#include "gpuDXTPack.h"
#include "gpuDXTVideo.h"
#include "gpuHeap.h"
#include "renderDraw.h"
#include "hash\adler32.h"

//==============================================================================
// BGPUDXTVideo::BGPUDXTVideo
//==============================================================================
BGPUDXTVideo::BGPUDXTVideo() : 
   mWriterThread(INVALID_HANDLE_VALUE),
   mOutputFileWriteFailed(false),
   mDownsample(false),
   mCurFrameIndex(0),
   mNextFrameIndexToWrite(0),
   mCapturing(false),
   mTotalBytesWritten(0),
   mCurFileIndex(0),
   mWidth(0),
   mHeight(0),
   mFPSLockRate(-1.0f),
   mRaw(false)
{
   for (uint i = 0; i < cNumPackBuffers; i++)
      mPackBuffers[i].clear();
}

//==============================================================================
// BGPUDXTVideo::~BGPUDXTVideo
//==============================================================================
BGPUDXTVideo::~BGPUDXTVideo()
{
   deinit();
}

//==============================================================================
// BGPUDXTVideo::init
//==============================================================================
bool BGPUDXTVideo::init(const char* pFilename, bool downsample, float fpsLockRate, bool raw, ILowLevelFileIO* pLowLevelFileIO)
{
   deinit();
   
   if (raw)
      downsample = false;
      
   mOutputFilename.set(pFilename);
   mDownsample = downsample;
   mOutputFile.setLowLevelFileIO(pLowLevelFileIO);
   mFPSLockRate = fpsLockRate;
   mRaw = raw;
      
   mCurFileIndex = 0;
   
   for (uint i = 0; i < 64; i++)
   {
      BRenderString filename(mOutputFilename);
      strPathRemoveExtension(filename);
      BRenderString suffix;
      suffix.format("_%02u.vid", i);
      filename += suffix;
      DeleteFileA(filename);
   }

   BRenderString filename(mOutputFilename);
   strPathRemoveExtension(filename);
   BRenderString suffix;
   suffix.format("_%02u.vid", mCurFileIndex);
   
   filename += suffix;
   
   if (!mOutputFile.open(filename.getPtr(), BWin32File::cWriteAccess | BWin32File::cSequentialAccess | BWin32File::cCreateAlways))
   {
      gConsoleOutput.output(cMsgConsole, "Failed creating output file: %s", filename.getPtr());
      return false;
   }
   else
   {
      gConsoleOutput.output(cMsgConsole, "Created output file: %s", filename.getPtr());
   }
      
   DWORD threadID;
   mWriterThread = CreateThread(NULL, 65536, writerThreadCallback, this, CREATE_SUSPENDED, &threadID);
   if (NULL == mWriterThread)
   {
      gConsoleOutput.output(cMsgConsole, "Failed creating worker thread");
      return false;
   }

   XSetThreadProcessor(mWriterThread, 4);
   
   ResumeThread(mWriterThread);
   
   mTimer.start();
   
   mCurFrameIndex = 0;
   mNextFrameIndexToWrite = 0;
   mTotalBytesWritten = 0;
   mCurFileIndex = 0;
   
   mCapturing = true;
      
   return true;
}

//==============================================================================
// BGPUDXTVideo::deinit
//==============================================================================
void BGPUDXTVideo::deinit(void)
{
   if (!mCapturing)
      return;
      
   gRenderThread.blockUntilGPUIdle();
   
   if (!mOutputFileWriteFailed)
   {
      servicePackBuffers();
      
      BDEBUG_ASSERT(mNextFrameIndexToWrite == mCurFrameIndex);
   }
   
   mTimer.stop();
   
   if (mWriterThread != INVALID_HANDLE_VALUE)
   {
      for (uint i = 0; i < 15 * 30; i++)
      {  
         if (!mWriteFIFO.getCount())
            break;
         Sleep(30);
      }
   
      mWriterExitEvent.set();
      
      WaitForSingleObject(mWriterThread, INFINITE);
      
      CloseHandle(mWriterThread);
      mWriterThread = INVALID_HANDLE_VALUE;
   }
   
   mOutputFile.close();
         
   for (uint i = 0; i < cNumPackBuffers; i++)
   {
      if (mPackBuffers[i].mpTex)
         mPackBuffers[i].mpTex->Release();
      
      mPackBuffers[i].clear();
   }
   
   gConsoleOutput.output(cMsgConsole, "Wrote %u files, %u frames", mCurFileIndex + 1, mNextFrameIndexToWrite);
      
   mWidth = 0;
   mHeight = 0;
   
   mTotalBytesWritten = 0;
   mCurFileIndex = 0;
   
   mOutputFileWriteFailed = false;
   mDownsample = false;
   mCapturing = false;
   
   mFPSLockRate = -1.0f;
}

//==============================================================================
// BGPUDXTVideo::flushPackBuf
//==============================================================================
void BGPUDXTVideo::flushPackBuf(uint packBufIndex)
{
   BPackBuffer& packBuf = mPackBuffers[packBufIndex];
         
   D3DLOCKED_RECT lockedRect;
   packBuf.mpTex->LockRect(0, &lockedRect, NULL, D3DLOCK_READONLY);
   
   D3DSURFACE_DESC desc;
   packBuf.mpTex->GetLevelDesc(0, &desc);
    
   XGTEXTURE_DESC xgDesc;
   XGGetTextureDesc(packBuf.mpTex, 0, &xgDesc);
        
   //INT                 Pitch;
   //void*               pBits;
   
   uint srcTexBytes;
   if (mRaw)
   {
      srcTexBytes = xgDesc.SlicePitch;
   }
   else
   {
      const uint cBytesPerBlock = 8;
      const uint numBlocksX = desc.Width / 4;
      const uint numBlocksY = desc.Height / 4;
      srcTexBytes = cBytesPerBlock * numBlocksX * numBlocksY;
   }      
      
   BWriteBuffer* pWriteBuf = HEAP_NEW(BWriteBuffer, gRenderHeap);
   pWriteBuf->reserve(sizeof(BGPUDXTVideoFileHeader) + sizeof(BGPUDXTVideoFrameHeader) + srcTexBytes + 32);
         
   if (packBuf.mFrameIndex == 0)
   {
      mWidth = desc.Width;
      mHeight = desc.Height;
      
      BGPUDXTVideoFileHeader fileHeader;
      fileHeader.mSig = static_cast<DWORD>(BGPUDXTVideoFileHeader::cSig);
      fileHeader.mWidth = desc.Width;
      fileHeader.mHeight = desc.Height;
      fileHeader.mAverageFPS = mFPSLockRate;
      fileHeader.mFlags = 0;
      if (mRaw)
         fileHeader.mFlags |= BGPUDXTVideoFileHeader::cFlagRaw;
      
      pWriteBuf->pushBack(reinterpret_cast<const BYTE*>(&fileHeader), sizeof(fileHeader));            
   }
   
   BGPUDXTVideoFrameHeader frameHeader;
   frameHeader.mSig = static_cast<DWORD>(BGPUDXTVideoFrameHeader::cSig);
   frameHeader.mFrameIndex = packBuf.mFrameIndex;
   frameHeader.mScaledTime = packBuf.mCaptureTime;
   frameHeader.mDataSizeInBytes = srcTexBytes;
   frameHeader.mDataAdler32 = calcAdler32(lockedRect.pBits, srcTexBytes);
   
   pWriteBuf->pushBack(reinterpret_cast<const BYTE*>(&frameHeader), sizeof(frameHeader));
   pWriteBuf->pushBack(static_cast<const BYTE*>(lockedRect.pBits), srcTexBytes);
         
   packBuf.mpTex->UnlockRect(0);
   
   *mWriteFIFO.getBackPtr() = pWriteBuf;
   mWriteFIFO.pushBack();
   
   packBuf.mCaptureTime = 0;
   packBuf.mFrameIndex = 0;
   packBuf.mTexDataReadyFence = UINT64_MAX;
}

//==============================================================================
// BGPUDXTVideo::servicePackBuffers
//==============================================================================
void BGPUDXTVideo::servicePackBuffers(void)
{
   bool restartFlag = false;
   
   do
   {
      restartFlag = false;
      
      for (uint packBufIndex = 0; packBufIndex < cNumPackBuffers; packBufIndex++)
      {
         if (mPackBuffers[packBufIndex].mTexDataReadyFence != UINT64_MAX) 
         {
            if (mPackBuffers[packBufIndex].mFrameIndex == mNextFrameIndexToWrite)
            {
               if ((!BD3D::mpDev->IsFencePending((DWORD)mPackBuffers[packBufIndex].mTexDataReadyFence)) || (!BD3D::mpDev->IsBusy()))
               {
                  flushPackBuf(packBufIndex);
                  
                  mNextFrameIndexToWrite++;
                  
                  restartFlag = true;
                  break;
               }
            }            
         }            
      }
      
   }  while (restartFlag);
}

//==============================================================================
// BGPUDXTVideo::capture
//==============================================================================
void BGPUDXTVideo::capture(void)
{
   if (!mCapturing)
      return;
   
   if (mOutputFileWriteFailed)
      return;
      
   D3DSURFACE_DESC backBufDesc;
   gRenderDraw.getDevBackBuffer()->GetDesc(&backBufDesc);

   IDirect3DTexture9* pBackBufTex = NULL;
   
   if (!mRaw)
   {
      HRESULT hres = gGPUFrameHeap.createTexture(backBufDesc.Width, backBufDesc.Height, 1, 0, D3DFMT_A8R8G8B8, 0, &pBackBufTex, NULL);
      BVERIFY(SUCCEEDED(hres));
   
      BD3D::mpDev->Resolve(D3DRESOLVE_RENDERTARGET0, NULL, pBackBufTex, NULL, 0, 0, NULL, 0.0f, 0, NULL);
   }      
            
   uint packBufIndex;
      
   for ( ; ; )
   {
      servicePackBuffers();
      
      for (packBufIndex = 0; packBufIndex < cNumPackBuffers; packBufIndex++)
         if (mPackBuffers[packBufIndex].mTexDataReadyFence == UINT64_MAX)
            break;
      
      if (packBufIndex < cNumPackBuffers)
         break;
         
      gEventDispatcher.sleep(5);
   }
      
   BPackBuffer& packBuf = mPackBuffers[packBufIndex];
   
   if (mFPSLockRate > 0.0f)
      packBuf.mCaptureTime = static_cast<uint64>((mCurFrameIndex / mFPSLockRate) * 1000000.0f + .5f);
   else
      packBuf.mCaptureTime = static_cast<uint64>(mTimer.getElapsedSeconds() * 1000000.0f + .5f);
   
   if (!packBuf.mpTex)
   {
      if (mRaw)
      {
         #define GPUSWIZZLE_BGRA (GPUSWIZZLE_W | GPUSWIZZLE_X<<3 | GPUSWIZZLE_Y<<6 | GPUSWIZZLE_Z<<9)
         DWORD fmt = MAKED3DFMT(GPUTEXTUREFORMAT_8_8_8_8, GPUENDIAN_NONE, TRUE, GPUSIGN_ALL_UNSIGNED, GPUNUMFORMAT_FRACTION, GPUSWIZZLE_BGRA);
         #undef GPUSWIZZLE_BGRA
         
         HRESULT hres = gRenderDraw.createTexture(
            backBufDesc.Width, 
            backBufDesc.Height, 1, 0, (D3DFORMAT)fmt, 0, &packBuf.mpTex, NULL);

         BVERIFY(SUCCEEDED(hres));
      }
      else
      {
         HRESULT hres = gRenderDraw.createTexture(
            mDownsample ? (backBufDesc.Width / 2) : backBufDesc.Width,
            mDownsample ? (backBufDesc.Height / 2) : backBufDesc.Height, 1, 0, D3DFMT_LIN_DXT1, 0, &packBuf.mpTex, NULL);
         
         BVERIFY(SUCCEEDED(hres));
      }         
   }
   
   if (mRaw)
      BD3D::mpDev->Resolve(D3DRESOLVE_RENDERTARGET0, NULL, packBuf.mpTex, NULL, 0, 0, NULL, 0.0f, 0, NULL);
   else
   {
      gRenderDraw.pushRenderState(D3DRS_HALFPIXELOFFSET);
      
      BGPUDXTPack::getInstance().pack(pBackBufTex, packBuf.mpTex);
      
      gRenderDraw.popRenderState(D3DRS_HALFPIXELOFFSET);
      
      BD3D::mpDev->SetRenderTarget(0, gRenderDraw.getDevBackBuffer());
      BD3D::mpDev->SetRenderTarget(1, NULL);
      BD3D::mpDev->SetDepthStencilSurface(gRenderDraw.getDevDepthStencil());
   }      
   
   packBuf.mTexDataReadyFence = BD3D::mpDev->InsertFence();
   packBuf.mFrameIndex = mCurFrameIndex;
   
   mCurFrameIndex++;         
   
   if (pBackBufTex)
      gGPUFrameHeap.releaseD3DResource(pBackBufTex);
}

//==============================================================================
// BGPUDXTVideo::writerThread
//==============================================================================
DWORD BGPUDXTVideo::writerThread(void)
{
   for ( ; ; )
   {
      const BWriteFIFO::Element_Type* ppBuf = NULL;
      BWriteFIFO::eGetPtrStatus status = mWriteFIFO.getFrontPtr(ppBuf, mWriterExitEvent, INFINITE);
      
      if (BWriteFIFO::eHandleSignaled == status)
         break;
      else if (BWriteFIFO::eAcquiredPtr != status)
         continue;
         
      const BWriteBuffer* pBuf = *ppBuf;
         
      BDEBUG_ASSERT(pBuf);
      
      bool success = true;
      if ((!mOutputFileWriteFailed) && (pBuf->getSizeInBytes()))
      {
         if ( (mTotalBytesWritten + (uint64)pBuf->getSizeInBytes()) >= 0x7FFFFF00ULL )
         //if ( (mTotalBytesWritten + (uint64)pBuf->getSizeInBytes()) >= 8U*1024U*1024U )
         {
            mOutputFile.close();
            mTotalBytesWritten = 0;
            
            BRenderString filename(mOutputFilename);
            strPathRemoveExtension(filename);
            
            mCurFileIndex++;
                        
            BRenderString suffix;
            suffix.format("_%02u.vid", mCurFileIndex);
            filename += suffix;
            
            if (!mOutputFile.open(filename.getPtr(), BWin32File::cWriteAccess | BWin32File::cSequentialAccess | BWin32File::cCreateAlways))
            {
               success = false;
               gConsoleOutput.output(cMsgConsole, "Failed creating output file: %s", filename.getPtr());
            }
            else
            {
               gConsoleOutput.output(cMsgConsole, "Created output file: %s", filename.getPtr());
            
               BGPUDXTVideoFileHeader fileHeader;
               fileHeader.mSig = static_cast<DWORD>(BGPUDXTVideoFileHeader::cSig);
               fileHeader.mWidth = mWidth;
               fileHeader.mHeight = mHeight;
               fileHeader.mAverageFPS = mFPSLockRate;
               fileHeader.mFlags = 0;
               if (mRaw)
                  fileHeader.mFlags |= BGPUDXTVideoFileHeader::cFlagRaw;
               
               if (mOutputFile.write(&fileHeader, sizeof(fileHeader)) != sizeof(fileHeader))
               {
                  success = false;
                  
                  gConsoleOutput.output(cMsgConsole, "Failed writing to output video file");
               }
               else
                  mTotalBytesWritten = sizeof(fileHeader);
            }                  
         }
         
         if (success)
         {
            if (mOutputFile.write(pBuf->getPtr(), pBuf->getSizeInBytes()) != pBuf->getSizeInBytes())
            {
               success = false;
               
               gConsoleOutput.output(cMsgConsole, "Failed writing to output video file");
            }
            else         
               mTotalBytesWritten += pBuf->getSizeInBytes();
         }               
      }         
      
      if (!success)
         mOutputFileWriteFailed = true;
               
      HEAP_DELETE((BWriteBuffer*)pBuf, gRenderHeap);
      pBuf = NULL;
      
      mWriteFIFO.popFront();
   }

   return 0;
}

//==============================================================================
// BGPUDXTVideo::writerThreadCallback
//==============================================================================
DWORD __stdcall BGPUDXTVideo::writerThreadCallback(LPVOID lpThreadParameter)
{
   return static_cast<BGPUDXTVideo*>(lpThreadParameter)->writerThread();
}

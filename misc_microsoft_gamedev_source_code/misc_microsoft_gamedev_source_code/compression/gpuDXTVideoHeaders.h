//==============================================================================
//
// File: gpuDXTVideoHeaders.h
//
// Copyright (c) 2006-2008 Ensemble Studios
//
//==============================================================================
#pragma once

struct BGPUDXTVideoFileHeader
{
   enum { cSig = 0xAEC30003 };
   
   DWORD mSig;
   
   DWORD mWidth;
   DWORD mHeight;
   
   DWORD mFlags;
      
   enum 
   {
      cFlagRaw = 1
   };
   
   float mAverageFPS;
   
   void endianSwap(void)
   {
      EndianSwitchWorker(this, this + 1, "iiiif");
   }
};

struct BGPUDXTVideoFrameHeader
{
   enum { cSig = 0xC1D90001 };
   
   DWORD    mSig;
   DWORD    mUnused;
   uint64   mFrameIndex;
   uint64   mScaledTime;
   DWORD    mDataSizeInBytes;
   DWORD    mDataAdler32;
   
   void endianSwap(void)
   {
      EndianSwitchWorker(this, this + 1, "iiqqii");
   }
};

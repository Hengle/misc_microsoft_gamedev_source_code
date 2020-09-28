//==============================================================================
// File: DXTQTypes.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once
#include "bytePacker.h"

enum { cDXTQID = 0x1234997E };

//==============================================================================
// struct BDXTQHeader
//==============================================================================
#pragma pack(push, 1)
struct BDXTQHeader
{
   enum { cVersion = 0xED03 };
   BPacked16 mVersion;
   BPacked16 mD3DStructSize;

   BPacked32 mBaseBlocks;
   BPacked32 mMipBlocks;

   BPacked32 mColorCodebookOfs;
   BPacked16 mColorCodebookSize;
   BPacked16 mColorCodebookBytes;
   
   BPacked32 mColorSelectorCodebookOfs;
   BPacked16 mColorSelectorCodebookSize;
   BPacked16 mColorSelectorCodebookBytes;

   BPacked32 mAlphaCodebookOfs;
   BPacked16 mAlphaCodebookSize;
   BPacked16 mAlphaCodebookBytes;
   
   BPacked32 mAlphaSelectorCodebookOfs;
   BPacked16 mAlphaSelectorCodebookSize;
   BPacked16 mAlphaSelectorCodebookBytes;

   BPacked32 mD3DTexOfs;
   
   struct BSegment
   {
      BPacked32 mNumBlocks;
      BPacked32 mDataOfs;
      BPacked32 mDataLen;
   };

   BSegment mTables;
   BSegment mBaseIndices;
   BSegment mMipIndices;
};
#pragma pack(pop)


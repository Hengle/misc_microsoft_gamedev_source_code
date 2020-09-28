//==============================================================================
//
// File: ugxGeomRenderPackedBone.h
//
// Copyright (c) 2006 Ensemble Studios
//
//==============================================================================
#pragma once

//==============================================================================
// struct BUGXGeomRenderPackedBone
// Must be 32-byte aligned when copied to write combined memory.
//==============================================================================
struct BUGXGeomRenderPackedBone
{
   XMHALF4 mRow0; // ax ay az unused
   XMHALF4 mRow1; // bx by bz cx
   XMHALF2 mRow2; // cy cz
   XMFLOAT3 mTran;

   static void packMatrix(uchar* RESTRICT pDst, XMMATRIX modelToView);
   static void packMatrix(uchar* RESTRICT pDst, XMMATRIX worldToView, XMMATRIX modelToWorld);
   static void packMatrix4(uchar* RESTRICT pDst, const float* RESTRICT pAMatrices);

   static XMMATRIX unpackMatrix(const uchar* RESTRICT pSrc);
   
   // Packs 13 indices into the matrix like this:
   // 0123
   // 4567
   // 89
   // 10,11,12 
   static void packLightIndices(uchar* RESTRICT pDst, const short* RESTRICT pIndices);
};

const uint cUGXGeomRenderBytesPerPackedBone = sizeof(BUGXGeomRenderPackedBone);
const uint cUGXGeomRenderBytesPerPackedBoneLog2 = 5;
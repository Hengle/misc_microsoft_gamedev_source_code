//==============================================================================
//
// File: ugxGeomRenderPackedBone.cpp
// rg [2/20/06] - This module should be compiled with optimizations enabled in debug builds, otherwise it's too slow.
//
// Copyright (c) 2006 Ensemble Studios
//
//==============================================================================
#include "xgameRender.h"
#include "ugxGeomRenderPackedBone.h"
#include "math\VMXUtils.h"

// vrlimi shifting:
//   x y z w
// 0 x y z w
// 1 y z w x
// 2 z w x y
// 3 w x y z

// 2 2 2 2  2 2 2 2  2 2  X Y Z
// A A A    B B B C  C C

//==============================================================================
// BUGXGeomRenderPackedBone::packMatrix
//==============================================================================
void BUGXGeomRenderPackedBone::packMatrix(uchar* RESTRICT pDst, XMMATRIX modelToView)
{
   //  0 ax ay az cz
   //  8 bx by bz 00
   // 16 cx cy 
   // 20 tx ty tz
   // 32

   //XMStoreDHenN3( (XMDHENN3*)(pDst), modelToView.r[0] );
   //XMStoreDHenN3( (XMDHENN3*)(pDst+4), modelToView.r[1] );
   //XMStoreDHenN3( (XMDHENN3*)(pDst+8), modelToView.r[2] );

   XMStoreHalf4( (XMHALF4*)(pDst), __vrlimi(modelToView.r[0], modelToView.r[2], VRLIMI_CONST(0, 0, 0, 1), 3));
   XMStoreHalf4( (XMHALF4*)(pDst+8), modelToView.r[1] );
   XMStoreHalf2( (XMHALF2*)(pDst+16), modelToView.r[2] );
   XMStoreFloat3( (XMFLOAT3*)(pDst+20), modelToView.r[3] );
}

//==============================================================================
// BUGXGeomRenderPackedBone::packMatrix
//==============================================================================
void BUGXGeomRenderPackedBone::packMatrix(uchar* RESTRICT pDst, XMMATRIX worldToView, XMMATRIX modelToWorld)
{
   XMMATRIX modelToView = XMMatrixMultiply(modelToWorld, worldToView);

   XMStoreHalf4( (XMHALF4*)(pDst), __vrlimi(modelToView.r[0], modelToView.r[2], VRLIMI_CONST(0, 0, 0, 1), 3));
   XMStoreHalf4( (XMHALF4*)(pDst+8), modelToView.r[1] );
   XMStoreHalf2( (XMHALF2*)(pDst+16), modelToView.r[2] );
   XMStoreFloat3( (XMFLOAT3*)(pDst+20), modelToView.r[3] );
}

//==============================================================================
// BUGXGeomRenderPackedBone::packMatrix4
//==============================================================================
void BUGXGeomRenderPackedBone::packMatrix4(uchar* RESTRICT pDst, const float* RESTRICT pAMatrices)
{
   XMMATRIX modelToView0 = XMLoadFloat4x4A16( (const XMFLOAT4X4A16*)(pAMatrices)+0 );
   XMMATRIX modelToView1 = XMLoadFloat4x4A16( (const XMFLOAT4X4A16*)(pAMatrices)+1 );
   XMMATRIX modelToView2 = XMLoadFloat4x4A16( (const XMFLOAT4X4A16*)(pAMatrices)+2 );
   XMMATRIX modelToView3 = XMLoadFloat4x4A16( (const XMFLOAT4X4A16*)(pAMatrices)+3 );

   XMStoreHalf4( (XMHALF4*)(pDst), __vrlimi(modelToView0.r[0], modelToView0.r[2], VRLIMI_CONST(0, 0, 0, 1), 3));
   XMStoreHalf4( (XMHALF4*)(pDst+8), modelToView0.r[1] );
   XMStoreHalf2( (XMHALF2*)(pDst+16), modelToView0.r[2] );
   XMStoreFloat3( (XMFLOAT3*)(pDst+20), modelToView0.r[3] );

   XMStoreHalf4( (XMHALF4*)(pDst+32), __vrlimi(modelToView1.r[0], modelToView1.r[2], VRLIMI_CONST(0, 0, 0, 1), 3));
   XMStoreHalf4( (XMHALF4*)(pDst+8+32), modelToView1.r[1] );
   XMStoreHalf2( (XMHALF2*)(pDst+16+32), modelToView1.r[2] );
   XMStoreFloat3( (XMFLOAT3*)(pDst+20+32), modelToView1.r[3] );

   XMStoreHalf4( (XMHALF4*)(pDst+64), __vrlimi(modelToView2.r[0], modelToView2.r[2], VRLIMI_CONST(0, 0, 0, 1), 3));
   XMStoreHalf4( (XMHALF4*)(pDst+8+64), modelToView2.r[1] );
   XMStoreHalf2( (XMHALF2*)(pDst+16+64), modelToView2.r[2] );
   XMStoreFloat3( (XMFLOAT3*)(pDst+20+64), modelToView2.r[3] );

   XMStoreHalf4( (XMHALF4*)(pDst+96), __vrlimi(modelToView3.r[0], modelToView3.r[2], VRLIMI_CONST(0, 0, 0, 1), 3));
   XMStoreHalf4( (XMHALF4*)(pDst+8+96), modelToView3.r[1] );
   XMStoreHalf2( (XMHALF2*)(pDst+16+96), modelToView3.r[2] );
   XMStoreFloat3( (XMFLOAT3*)(pDst+20+96), modelToView3.r[3] );
}

//==============================================================================
// BUGXGeomRenderPackedBone::unpackMatrix
//==============================================================================
XMMATRIX BUGXGeomRenderPackedBone::unpackMatrix(const uchar* RESTRICT pSrc)
{
   //  0 ax ay az cz
   //  8 bx by bz 00
   // 16 cx cy 
   // 20 tx ty tz
   // 32

   //XMStoreHalf4( (XMHALF4*)(pDst), __vrlimi(modelToView.r[0], modelToView.r[2], VRLIMI_CONST(0, 0, 0, 1), 3));
   //XMStoreHalf4( (XMHALF4*)(pDst+8), modelToView.r[1] );
   //XMStoreHalf2( (XMHALF2*)(pDst+16), modelToView.r[2] );
   //XMStoreFloat3( (XMFLOAT3*)(pDst+20), modelToView.r[3] );

   // MPB TODO - This should be optimized, particularly the swap on last couple lines
   XMMATRIX temp = XMMatrixIdentity();
   temp.r[0] = XMLoadHalf4((XMHALF4*)(pSrc));
   temp.r[1] = XMLoadHalf4((XMHALF4*)(pSrc+8));
   temp.r[2] = __vrlimi(temp.r[2], XMLoadHalf2((XMHALF2*)(pSrc+16)), VRLIMI_CONST(1, 1, 0, 0), 0);
   temp.r[3] = __vrlimi(temp.r[3], XMLoadFloat3((XMFLOAT3*)(pSrc+20)), VRLIMI_CONST(1, 1, 1, 0), 0);
   temp._33 = temp._14;
   temp._14 = 0.0f;

   return temp;
}

//==============================================================================
// BUGXGeomRenderPackedBone::packLightIndices
//==============================================================================
void BUGXGeomRenderPackedBone::packLightIndices(uchar* RESTRICT pDst, const short* RESTRICT pIndices)
{
   XMVECTOR a = XMLoadShort4((const XMSHORT4*)(pIndices));
   XMVECTOR b = XMLoadShort4((const XMSHORT4*)(pIndices + 4));
   XMVECTOR c = XMLoadShort2((const XMSHORT2*)(pIndices + 8));
   XMVECTOR d = XMLoadShort4((const XMSHORT4*)(pIndices + 10));

   XMStoreHalf4( (XMHALF4*)(pDst), a);
   XMStoreHalf4( (XMHALF4*)(pDst+8), b);
   XMStoreHalf2( (XMHALF2*)(pDst+16), c);
   XMStoreFloat3( (XMFLOAT3*)(pDst+20), d);
}




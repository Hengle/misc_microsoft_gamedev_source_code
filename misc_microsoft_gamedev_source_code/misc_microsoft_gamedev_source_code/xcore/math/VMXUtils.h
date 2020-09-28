//==============================================================================
// File: VMXUtils.h
//
// Copyright (c) 2006 Ensemble Studios
//==============================================================================
#pragma once

#define VRLIMI_CONST(x, y, z, w) (((x)<<3)|((y)<<2)|((z)<<1)|(w))

extern CONST __declspec(selectany) XMVECTOR gXMOne = {1.0f, 1.0f, 1.0f, 1.0f};
extern CONST __declspec(selectany) XMVECTOR gXMNegOne = {-1.0f, -1.0f, -1.0f, -1.0f};
extern CONST __declspec(selectany) XMVECTOR gXMOneHalf = {0.5f, 0.5f, 0.5f, 0.5f};

//==============================================================================
// XMMatrixInverseByTranspose
//==============================================================================
// Matrix can only contain a 3x3 rotation and translation.
inline void XMMatrixInverseByTranspose(XMMATRIX& result, const XMMATRIX& matrix)
{
   XMMATRIX temp = matrix;
   temp.r[3] = XMVectorZero();

   XMMATRIX inverseMatrix = XMMatrixTranspose(temp);

   inverseMatrix.r[3] = XMVector3TransformNormal(-matrix.r[3], inverseMatrix);
   inverseMatrix.r[3] = XMVectorInsert(inverseMatrix.r[3], XMVectorSplatOne(), 0, 0, 0, 0, 1);

   result = inverseMatrix;
}

//==============================================================================
// XMVectorLoadFloatAndReplicate
// Loads a float without causing a LHS (unless, of course, you specify an 
// address that was recently stored to).
//==============================================================================
inline XMVECTOR XMVectorLoadFloatToX(const void* pFloat)
{
   return XMLoadScalar(pFloat);
}

//==============================================================================
// XMVectorLoadFloatAndReplicate
// Loads a float and splats to XYZW without causing a LHS (unless, of course,
// you specify an address that was recently stored to).
//==============================================================================
inline XMVECTOR XMVectorLoadFloatAndReplicate(const void* pFloat)
{
   return XMVectorSplatX(XMLoadScalar(pFloat));
}

//==============================================================================
// XMVectorSetWToOne
//==============================================================================
inline XMVECTOR XMVectorSetWToOne(XMVECTOR v)
{
   return __vrlimi(v, XMVectorSplatOne(), VRLIMI_CONST(0, 0, 0, 1), 0);
}

//==============================================================================
// XMVectorSetWToZero
//==============================================================================
inline XMVECTOR XMVectorSetWToZero(XMVECTOR v)
{
   return __vrlimi(v, XMVectorZero(), VRLIMI_CONST(0, 0, 0, 1), 0);
}


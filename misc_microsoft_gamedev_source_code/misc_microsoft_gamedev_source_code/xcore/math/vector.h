//==============================================================================
// Copyright (c) 1997-2005 Ensemble Studios
//
// Vector class
//==============================================================================

#ifndef _VECTOR_H_
#define _VECTOR_H_

//==============================================================================
// 
//==============================================================================
#include "shortfloat.h"
#include "simplevector.h"
#if defined(XBOX)
   #include "intrinsicvector.h"
#endif

#define USE_INTRINSIC_VECTORS

#if defined(XBOX) && defined(USE_INTRINSIC_VECTORS)
   typedef BIntrinsicVector BVector;
   typedef BIntrinsicVector2 BVector2;
   typedef BIntrinsicVector4 BVector4;
   typedef BIntrinsicShortVector BShortVector;
#else
   typedef BSimpleVector BVector;
   typedef BSimpleVector2 BVector2;
   typedef BSimpleVector4 BVector4;
   typedef BSimpleShortVector BShortVector;
#endif

//==============================================================================
// some handy constant vectors
//==============================================================================
__declspec(selectany) extern const BVector cOriginVector(0.0f, 0.0f, 0.0f);
__declspec(selectany) extern const BVector cXAxisVector(1.0f, 0.0f, 0.0f);
__declspec(selectany) extern const BVector cYAxisVector(0.0f, 1.0f, 0.0f);
__declspec(selectany) extern const BVector cZAxisVector(0.0f, 0.0f, 1.0f);
__declspec(selectany) extern const BVector cInvalidVector(-1.0f, -1.0f, -1.0f);

#endif // _VECTOR_H_

//==============================================================================
// eof: vector.h
//==============================================================================
//==============================================================================
// matrix.h
//
// Copyright (c) 1999 - 2005 Ensemble Studios
//==============================================================================


#ifndef _MATRIX_
#define _MATRIX_

//==============================================================================
// 
//==============================================================================
#include "vector.h"
#include "simplematrix.h"

#if defined(XBOX)
   #include "intrinsicmatrix.h"
#endif

#define USE_INTRINSIC_MATRICES

#if defined(XBOX) && defined(USE_INTRINSIC_MATRICES)
   typedef BIntrinsicMatrix BMatrix;
#else
   typedef BSimpleMatrix BMatrix;
#endif

//==============================================================================
#endif // _MATRIX_

//==============================================================================
// eof: matrix.h
//==============================================================================

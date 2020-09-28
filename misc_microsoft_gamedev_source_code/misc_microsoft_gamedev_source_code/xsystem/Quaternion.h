//==============================================================================
// quaternion.h
//
// Copyright (c) 1999 - 2005 Ensemble Studios
//==============================================================================

#ifndef _QUATERNION_
#define _QUATERNION_

#include "simplequaternion.h"
#if defined(XBOX)
   #include "math\intrinsicquaternion.h"
#endif

#define USE_INTRINSIC_QUATERNIONS

#if defined(XBOX) && defined(USE_INTRINSIC_QUATERNIONS)
   typedef BIntrinsicQuaternion BQuaternion;
#else
   typedef BSimpleQuaternion BQuaternion;
#endif

#endif // _QUATERNION_

//==============================================================================
// eof: quaternion.h
//==============================================================================

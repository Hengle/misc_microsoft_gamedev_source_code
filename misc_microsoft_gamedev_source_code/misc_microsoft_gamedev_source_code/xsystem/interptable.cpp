//==============================================================================
// interptable.cpp
//
// Copyright (c) 2004, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes

#include "xsystem.h"
#include "interptable.h"

void lerpFloats(const float time, const float& val1, const float& val2, float& result)
{
   result = (val1 + time * (val2 - val1));
}

void lerpVector(const float time, const BVector& val1, const BVector& val2, BVector& result)
{
   result.x = (val1.x + time * (val2.x - val1.x));
   result.y = (val1.y + time * (val2.y - val1.y));
   result.z = (val1.z + time * (val2.z - val1.z));
}

void lerpVector2(const float time, const BVector2& val1, const BVector2& val2, BVector2& result)
{
   result.x = (val1.x + time * (val2.x - val1.x));
   result.y = (val1.y + time * (val2.y - val1.y));
}

void lerpVector4(const float time, const BVector4& val1, const BVector4& val2, BVector4& result)
{
   result.x = (val1.x + time * (val2.x - val1.x));
   result.y = (val1.y + time * (val2.y - val1.y));
   result.z = (val1.z + time * (val2.z - val1.z));
   result.w = (val1.w + time * (val2.w - val1.w));
}


//==============================================================================
// eof: interptable.cpp
//==============================================================================

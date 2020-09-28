//-----------------------------------------------------------------------------
// File: vectorFont.h
//-----------------------------------------------------------------------------
#pragma once

#include "math\generalVector.h"

//-----------------------------------------------------------------------------
// class BVectorFont
//-----------------------------------------------------------------------------
class BVectorFont
{
public:
   static void renderText(
      const char* pStr, 
      const BVec3& pos, 
      const BVec3& up,
      const BVec3& right, 
      const DWORD color = 0xFFFFFFFF);
};


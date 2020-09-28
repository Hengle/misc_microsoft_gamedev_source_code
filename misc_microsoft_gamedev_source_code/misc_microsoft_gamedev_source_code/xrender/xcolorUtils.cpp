// File: xcolorUtils.cpp
#include "xrender.h"
#include "xcolorUtils.h"
#include "math\VMXUtils.h"

XMVECTOR BXColorUtils::D3DColorToLinear(DWORD c)
{
   XMVECTOR v = XMLoadColor(reinterpret_cast<const XMCOLOR*>(&c));

   const float g = 2.2f;
      
   return XMVectorPowEst(v, XMVectorSet(g, g, g, 1.0f));
}

XMVECTOR BXColorUtils::D3DColorToLinearApprox(DWORD c)
{
   XMVECTOR v = XMLoadColor(reinterpret_cast<const XMCOLOR*>(&c));

   XMVECTOR l = XMVectorMultiply(v, v);
   
   return __vrlimi(l, v, VRLIMI_CONST(0, 0, 0, 1), 0);
}

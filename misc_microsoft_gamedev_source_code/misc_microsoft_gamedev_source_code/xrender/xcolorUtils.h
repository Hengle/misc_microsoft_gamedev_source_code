// File: xcolorUtils.h
#pragma once

class BXColorUtils
{
public:
   // Returns all components divided by 255.0f.
   static XMVECTOR D3DColorToFractional(DWORD c) { return XMLoadColor(reinterpret_cast<const XMCOLOR*>(&c)); }
   
   // Uses a 2.2 power approximation to sRGB. Returns fractional alpha.
   static XMVECTOR D3DColorToLinear(DWORD c);
   
   // Uses a 2.0 power approximation to sRGB. Returns fractional alpha.
   static XMVECTOR D3DColorToLinearApprox(DWORD c);
};


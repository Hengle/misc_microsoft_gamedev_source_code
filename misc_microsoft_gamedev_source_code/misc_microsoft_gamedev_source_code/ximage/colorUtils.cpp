// File: colorutils.cpp
#include "ximage.h"
#include "colorutils.h"


const BRGBAColor gWhiteColor(255,255,255);
const BRGBAColor gBlackColor(0,0,0);
const BRGBAColor gGrayColor(128,128,128);
const BRGBAColor gRedColor(255,0,0);
const BRGBAColor gGreenColor(0,255,0);
const BRGBAColor gPurpleColor(255,0,255);
const BRGBAColor gBlueColor(0,0,255);
const BRGBAColor gYellowColor(255,255,0);
const BRGBAColor gMegentaColor(0,255,255);

void BColorUtils::RGBToYCoCg(int r, int g, int b, int& y, int& co, int& cg)
{
   y  =  (r >> 2) + (g >> 1) + (b >> 2);
   co =  (r >> 1)            - (b >> 1);
   cg = -(r >> 2) + (g >> 1) - (b >> 2);
}

void BColorUtils::YCoCgToRGB(int y, int co, int cg, int& r, int& g, int& b)
{
   g = y + cg;
   int t = y - cg;
   r = t + co;
   b = t - co;
}

void BColorUtils::RGBToYCoCgR(int r, int g, int b, int& y, int& co, int& cg)
{
   co = r - b;
   int t = b + (co >> 1);
   cg = g - t;
   y = t + (cg >> 1);
}

void BColorUtils::YCoCgRToRGB(int y, int co, int cg, int& r, int& g, int& b)
{
   int t = y - (cg >> 1);
   g = cg + t;
   b = t - (co >> 1);
   r = b + co;
}

void BColorUtils::RGBToYCoCgR(const BRGBAColor& rgb, BRGBAColor16& yCoCg)
{
   const int co = rgb.r - rgb.b;
   const int t = rgb.b + (co >> 1);
   const int cg = rgb.g - t;
   const int y = t + (cg >> 1);
   yCoCg(0) = static_cast<short>(y);
   yCoCg(1) = static_cast<short>(cg);
   yCoCg(2) = static_cast<short>(co);
   yCoCg(3) = rgb.a;
}

void BColorUtils::YCoCgRToRGB(const BRGBAColor16& yCoCg, BRGBAColor& rgb)
{
   const int y = Math::Clamp<int>(yCoCg(0), 0, 255);
   const int cg = Math::Clamp<int>(yCoCg(1), -255, 255);
   const int co = Math::Clamp<int>(yCoCg(2), -255, 255);
   const int t = y - (cg >> 1);
   int g = cg + t;
   int b = t - (co >> 1);
   int r = b + co;
   rgb.set(r, g, b, yCoCg.a);
//   rgb.set(y - (cg>>1) + (co>>1), y + (cg>>1), y - (cg>>1) - (co>>1), yCoCg.a);
}

void BColorUtils::RGBToYCoCg(const BRGBAColor& rgb, BRGBAColor16& yCoCg)
{
   const int r = rgb.r;
   const int g = rgb.g;
   const int b = rgb.b;
   yCoCg.r = static_cast<short>((r >> 2) + (g >> 1) + (b >> 2));
   yCoCg.b = static_cast<short>( (r >> 1)            - (b >> 1));
   yCoCg.g = static_cast<short>(-(r >> 2) + (g >> 1) - (b >> 2));
   yCoCg.a = rgb.a;
}

void BColorUtils::YCoCgToRGB(const BRGBAColor16& yCoCg, BRGBAColor& rgb)
{
   const int y = yCoCg.r;
   const int co = yCoCg.b;
   const int cg = yCoCg.g;
   const int t = y - cg;
   rgb.set(t + co, y + cg, t - co, yCoCg.a);
}

void BColorUtils::RGBToYCbCr(const BRGBAColor& rgb, BRGBAColor16& yCbCr)
{
   yCbCr(0) = (   299 * rgb.r +    587 * rgb.g +   114 * rgb.b + 500) / 1000;
   yCbCr(1) = static_cast<short>((-16874 * rgb.r + -33136 * rgb.g + 50000 * rgb.b) / 100000);
   yCbCr(2) = static_cast<short>(( 50000 * rgb.r + -41869 * rgb.g + -8131 * rgb.b) / 100000);
   yCbCr(3) = rgb.a;
}

uint BColorUtils::RGBToY(const BRGBAColor& rgb)
{
//   return (299 * rgb.r + 587 * rgb.g + 114 * rgb.b + 500) / 1000;
   return (212671U * rgb.r + 715160U * rgb.g + 72169U * rgb.b + 500000U) / 1000000U;
}

void BColorUtils::YCbCrToRGB(const BRGBAColor16& YCbCr, BRGBAColor& rgb)
{
   const int y = Math::Clamp<int>(YCbCr(0), 0, 255);
   const int cb = Math::Clamp<int>(YCbCr(1), -128, 127);
   const int cr = Math::Clamp<int>(YCbCr(2), -128, 127);
   int r = y + (cr * 1402 + 500) / 1000;
   int g = y + (cr * -71414 + cb * -34414 + 50000) / 100000;
   int b = y + (cb * 1772 + 500) / 1000;
   rgb.set(r, g, b, YCbCr.a);
}

void BColorUtils::unpackColor(WORD packed, int& r, int& g, int& b, bool scaled)
{
   if (scaled)
   {
      //r = (((packed >> 11) & 31) * 255 + 15) / 31;
      //g = (((packed >> 5) & 63) * 255 + 31) / 63;
      //b = ((packed & 31) * 255 + 15) / 31;
      
      r = (packed >> 11) & 31; 
      r = (r << 3) | (r >> 2);
      
      g = (packed >> 5) & 63;
      g = (g << 2) | (g >> 4);
      
      b = packed & 31;
      b = (b << 3) | (b >> 2);
   }
   else
   {
      r = (packed >> 11) & 31;
      g = (packed >> 5) & 63;
      b = packed & 31;
   }
}

WORD BColorUtils::packColor(int r, int g, int b, bool scaled, DWORD roundFlags)
{
   if (scaled)
   {
      int rBias = 127;
      int gBias = 127;
      int bBias = 127;
      if (roundFlags & cRCeil) rBias = 254; else if (roundFlags & cRFloor) rBias = 0;
      if (roundFlags & cGCeil) gBias = 254; else if (roundFlags & cGFloor) gBias = 0;
      if (roundFlags & cBCeil) bBias = 254; else if (roundFlags & cBFloor) bBias = 0;
      
      r = (r * 31 + rBias) / 255;
      g = (g * 63 + gBias) / 255;
      b = (b * 31 + bBias) / 255;
   }

   if (r < 0) r = 0; else if (r > 31) r = 31;
   if (g < 0) g = 0; else if (g > 63) g = 63;
   if (b < 0) b = 0; else if (b > 31) b = 31;

   return static_cast<WORD>((r << 11) | (g << 5) | b);
}

void BColorUtils::unpackColor(WORD packed, BRGBAColor& color, bool scaled)
{
   int r, g, b;
   unpackColor(packed, r, g, b, scaled);
   color.r = static_cast<uchar>(r);
   color.g = static_cast<uchar>(g);
   color.b = static_cast<uchar>(b);
}

WORD BColorUtils::packColor(const BRGBAColor& color, bool scaled, DWORD roundFlags)
{
   const int r = color.r;
   const int g = color.g;
   const int b = color.b;
   return packColor(r, g, b, scaled, roundFlags);
}

int BColorUtils::colorDistancePerceptual(int e1r, int e1g, int e1b, int e2r, int e2g, int e2b)
{
   int rmean = (e1r + e2r) / 2;
   int r = e1r - e2r;
   int g = e1g - e2g;
   int b = e1b - e2b;
   return (((512+rmean)*r*r)>>8) + 4*g*g + (((767-rmean)*b*b)>>8);
}

int BColorUtils::colorDistanceElucidian(int e1r, int e1g, int e1b, int e2r, int e2g, int e2b)
{
   int r = e1r - e2r;
   int g = e1g - e2g;
   int b = e1b - e2b;
   return r*r + g*g + b*b;
}

int BColorUtils::colorDistanceElucidian(int e1r, int e1g, int e1b, int e1a, int e2r, int e2g, int e2b, int e2a)
{
   int r = e1r - e2r;
   int g = e1g - e2g;
   int b = e1b - e2b;
   int a = e1a - e2a;
   return r*r + g*g + b*b + a*a;
}

int BColorUtils::colorDistancePerceptual(int e1r, int e1g, int e1b, int e1a, int e2r, int e2g, int e2b, int e2a)
{
   int rmean = (e1r + e2r) / 2;
   int r = e1r - e2r;
   int g = e1g - e2g;
   int b = e1b - e2b;
   int a = e1a - e2a;
   return (((512+rmean)*r*r)>>8) + 4*g*g + (((767-rmean)*b*b)>>8) + 4*a*a;
}

int BColorUtils::colorDistancePerceptual(const BRGBAColor& a, const BRGBAColor& b, bool includeAlpha)
{
   if (includeAlpha)
      return colorDistancePerceptual(a.r, a.g, a.b, a.a, b.r, b.g, b.b, b.a);
   else
      return colorDistancePerceptual(a.r, a.g, a.b, b.r, b.g, b.b);
}

int BColorUtils::colorDistanceElucidian(const BRGBAColor& a, const BRGBAColor& b, bool includeAlpha)
{
   if (includeAlpha)
      return colorDistanceElucidian(a.r, a.g, a.b, a.a, b.r, b.g, b.b, b.a);
   else
      return colorDistanceElucidian(a.r, a.g, a.b, b.r, b.g, b.b);
}

int BColorUtils::colorDistanceWeighted(int e1r, int e1g, int e1b, int e2r, int e2g, int e2b)
{
   const uint DistRWeight = 24;
   const uint DistGWeight = 73;//72;
   const uint DistBWeight = 3;//7;
   
   return (e1r - e2r) * (e1r - e2r) * DistRWeight + (e1g - e2g) * (e1g - e2g) * DistGWeight + (e1b - e2b) * (e1b - e2b) * DistBWeight;   
}

int BColorUtils::colorDistanceWeighted(const BRGBAColor& a, const BRGBAColor& b)
{
   return colorDistanceWeighted(a.r, a.g, a.b, b.r, b.g, b.b);
}

BRGBAColor& BHDRColorUtils::packRGBE(const BRGBAColorF& src, BRGBAColor& dst)
{
   float v = src.getMaxComponent3();
   if (v >= 1e-32)
   {
      int e;
      v = frexp(v, &e) * 255.9999f / v;
      dst.setComponent(0, Math::FloatToIntTrunc(src[0] * v));
      dst.setComponent(1, Math::FloatToIntTrunc(src[1] * v));
      dst.setComponent(2, Math::FloatToIntTrunc(src[2] * v));
      dst.setComponent(3, e + 128);
   }
   else
   {
      dst.clear();
   }
   return dst;
}

BRGBAColorF& BHDRColorUtils::unpackRGBE(const BRGBAColor& src, BRGBAColorF& dst)
{
   if (!src[3])
   {
      dst.set(0, 0, 0, 1.0f);
   }
   else 
   {
      double f = ldexp(1.0, static_cast<int>(src[3]) - (128 + 8));

      for (uint i = 0; i < 3; i++)
         dst.setComponent(i, static_cast<float>((src[i] + .5f) * f));
      
      dst[3] = 1.0f;         
   }
   return dst;
}

void BHDRColorUtils::unpackRGBE(const BRGBAColor* pSrc, BRGBAColorF* pDst, uint numPixels)
{
   while (numPixels)
   {
      unpackRGBE(*pSrc, *pDst);
      pSrc++;
      pDst++;
      numPixels--;
   }
}

void BHDRColorUtils::packRGBE(const BRGBAColorF* pSrc, BRGBAColor* pDst, uint numPixels)
{
   while (numPixels)
   {
      packRGBE(*pSrc, *pDst);
      pSrc++;
      pDst++;
      numPixels--;
   }
}


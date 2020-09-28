// File: cubemap.h
#pragma once

#include "generalVector.h"

template<typename PixelType>
class BCubemap
{
public:
   typedef BDynamicArray<PixelType> PixelTypeVec;
   
   BCubemap() :
      mWidth(0),
      mHeight(0),
      mFaces(6)
   {
   }      
               
   BCubemap(int width, int height) :
      mWidth(width),
      mHeight(height),
      mFaces(6)
   {
      for (int i = 0; i < 6; i++)
         mFaces[i].resize(mWidth * mHeight);
   }
   
   void setSize(int width, int height)
   {
      mWidth = width;
      mHeight = height;
      for (int i = 0; i < 6; i++)
      {
         mFaces[i].resize(width * height);
         mFaces[i].setAll(PixelType());
      }
   }
               
   int getWidth(void) const { return mWidth; }
   int getHeight(void) const { return mHeight; }
   
   const PixelTypeVec& getFace(int i) const { return mFaces[debugRangeCheck(i, 6)]; }
         PixelTypeVec& getFace(int i)       { return mFaces[debugRangeCheck(i, 6)]; }
         
   PixelType& getPixel(int i, int x, int y)
   {
      return mFaces[debugRangeCheck(i, 6)][debugRangeCheck(x, mWidth) + debugRangeCheck(y, mHeight) * mWidth];
   }
   
   const PixelType& getPixel(int i, int x, int y) const
   {
      return mFaces[debugRangeCheck(i, 6)][debugRangeCheck(x, mWidth) + debugRangeCheck(y, mHeight) * mWidth];
   }
   
   BVec3 getVector(int f, int x, int y, bool normalize = true, float ofsX = .5f, float ofsY = .5f) const
   {
      debugRangeCheck(f, 0, 6);
      debugRangeCheck(x, mWidth);
      debugRangeCheck(y, mHeight);

      float fx = (x + ofsX) / mWidth;
      float fy = (y + ofsY) / mHeight;

      fx = fx * 2.0f - 1.0f;
      fy = fy * 2.0f - 1.0f;

      BVec3 ret;
      switch (f)
      {
         case 0: ret[0] = 1.0f;  ret[1] = -fy;   ret[2] = -fx;   break;
         case 1: ret[0] = -1.0f; ret[1] = -fy;   ret[2] = fx;    break;
         case 2: ret[0] = fx;    ret[1] = 1.0f;  ret[2] = fy;    break;
         case 3: ret[0] = fx;    ret[1] = -1.0f; ret[2] = -fy;   break;
         case 4: ret[0] = fx;    ret[1] = -fy;   ret[2] = 1.0f;  break;
         case 5: ret[0] = -fx;   ret[1] = -fy;   ret[2] = -1.0f; break;
      }

      if (normalize)
         ret.normalize();

      return ret;
   }
               
   void getCoord(uint& f, uint& px, uint& py, const BVec3& dir) const
   {
      const float x = fabs(dir[0]);
      const float y = fabs(dir[1]);
      const float z = fabs(dir[2]);

      float xt, yt, m;

      if ((x >= y) && (x >= z))
      {
         m = x;
         yt = -dir[1];
         if (dir[0] >= 0.0f)
         {
            f = 0;
            xt = -dir[2];
         }
         else
         {
            f = 1;
            xt = dir[2];
         }
      }
      else if ((y >= x) && (y >= z))
      {
         m = y;
         xt = dir[0];
         if (dir[1] >= 0.0f)
         {
            f = 2;
            yt = dir[2];
         }
         else
         {
            f = 3;
            yt = -dir[2];
         }
      }
      else
      {
         m = z;
         yt = -dir[1];
         if (dir[2] >= 0.0f)
         {
            f = 4;
            xt = dir[0];
         }
         else
         {
            f = 5;
            xt = -dir[0];
         }
      }

      float oom = .5f / m;

      xt = ((xt * oom) + .5f) * mWidth + .5f;
      yt = ((yt * oom) + .5f) * mHeight + .5f;

      px = Math::Clamp(Math::FloatToIntTrunc(xt), 0, mWidth - 1);
      py = Math::Clamp(Math::FloatToIntTrunc(yt), 0, mHeight - 1);
   }
   
   // Point sampling
   const PixelType& getPixel(const BVec3& dir) const
   {
      uint f, px, py;
      
      getCoord(f, px, py, dir);
              
      return getPixel(f, px, py);
   }
   
   // Point sampling
   PixelType& getPixel(const BVec3& dir) 
   {
      uint f, px, py;

      getCoord(f, px, py, dir);

      return getPixel(f, px, py);
   }
   
   // returns solid angle of cubemap face (f) texel (x,y)
   // sum of all texels is 4*Pi
   double getSphericalArea(int f, int x, int y) const
   {
#if 0 
      double lat[5];
      double lng[5];

      int t = 0;
      for (int yc = 0; yc < 2; yc++)
      {
         for (int xc = 0; xc < 2; xc++)
         {
            BVec3 r(getVector(0, x, y, true, (xc ^ yc), yc));

            lng[t] = atan2(r[1], r[0]);
            lat[t] = asin(r[2]);
            t++;
         }
      }

      lng[t] = lng[t - 1];
      lat[t] = lat[t - 1];

      return (Math::fPi * SphericalPolyArea(lat, lng, 4)) / 180.0f;
#endif      

      // Check radiosity hemicube references for alternate methods?

      // Computes differential solid angle
      // Doesn't take projected shape into account, not as accurate ???
      // But SphericalPolyArea() suffers from accuracy problems!
      BVec3 r(getVector(0, x, y, false, .5f, .5f));
      BVec3 rNorm(r.normalized());

      BVec3 n;
      if (fabs(r[0]) == 1.0f)
         n = BVec3(-r[0], 0.0f, 0.0f);
      else if (fabs(r[1]) == 1.0f)
         n = BVec3(0.0f, -r[1], 0.0f);
      else
         n = BVec3(0.0f, 0.0f, -r[2]);

      float dA = (2.0f / mWidth) * (2.0f / mHeight);

      float cosB = -n * rNorm;

      float dw = (cosB * dA) / r.len2();

      return dw;
   }
               
protected:
   int mWidth, mHeight;
   BDynamicArray<PixelTypeVec> mFaces;
};


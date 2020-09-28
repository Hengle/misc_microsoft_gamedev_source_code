//----------------------------------------------------------------------------------------------------------------------
// Coherent noise class.
//----------------------------------------------------------------------------------------------------------------------
#pragma once
#include "generalVector.h"

class BCoherentNoise
{
public:
   static float noise1D(float p);
   
   // Tilable 1D coherent noise.
   // z should be [0,t]
   static float noise1DWrap(float z, float t);
   
   // Harmonic sum functions:
   // alpha - Harmonic weight, as this approaches 1 the function becomes more noisy.
   // beta - Harmonic scaling.
   // n - Number of octives to sum.
   static float noise1DHarmonicSum(float x, float alpha = 2.0f, float beta = 2.0f, int n = 3);

   static float noise2D(const BVec2& vec);
   
   // Tilable 2D coherent noise.
   // v should be [0,0]-[w,h]
   static float noise2DWrap(const BVec2& v, float w, float h);
   
   static float noise2DHarmonicSum(const BVec2& v, float alpha = 2.0f, float beta = 2.0f, int n = 3);
   
   static float noise3D(const BVec3& vec);
   
   // Tilable 3D coherent noise.
   // v should be [0,0]-[w,h]
   static float noise3Wrap(const BVec3& v, float w, float h, float d);
   
   static float noise3DHarmonicSum(const BVec3& v, float alpha = 2.0f, float beta = 2.0f, int n = 3);
   
   static void initTables(void);
               
private:
   enum 
   {
      TableSize         = 256,
      TableSizeMask     = TableSize - 1,
      
      Table3Size        = 16,
      Table3SizeMask    = Table3Size - 1
   };
   
   typedef uchar IndexType;
   
   static IndexType     mPermute[TableSize];
   static float         mGradient1[TableSize];
   static BVec2         mGradient2[TableSize];
   static BVec3         mGradient3[Table3Size];
   static bool          mInitialized;
   
   static int permute(int i) { return mPermute[i & TableSizeMask]; }
   
   static void calcInterpParams(float v, int& loIndex, int& hiIndex, float& loFract, float& hiFract)
   {
      const float bias = 1024.0f;
      const float t = v + bias;
      const int it = Math::FloatToIntTrunc(t);
      loIndex = it;
      hiIndex = it + 1;
      loFract = t - it;
      hiFract = loFract - 1.0f;
   }      
   
   static float interpFunc(float t) { return t * t * t * t * t * 6.0f - t * t * t * t * 15.0f + t * t * t * 10.0f; }
}; // BCoherentNoise

//----------------------------------------------------------------------------------------------------------------------
// Coherent noise class.
// This implementation follows the material in Ken Perlin's GDCHardCore Gamers Workshop '99 presentation:
// http://www.noisemachine.com/talk1/
// Includes improvements from:
// http://mrl.nyu.edu/~perlin/paper445.pdf  
//----------------------------------------------------------------------------------------------------------------------
#include "xcore.h"
#include "coherentNoise.h"
#include "random.h"

bool                       BCoherentNoise::mInitialized;
BCoherentNoise::IndexType  BCoherentNoise::mPermute[BCoherentNoise::TableSize];
float                      BCoherentNoise::mGradient1[BCoherentNoise::TableSize];
BVec2                      BCoherentNoise::mGradient2[BCoherentNoise::TableSize];
   
BVec3                      BCoherentNoise::mGradient3[Table3Size] = 
{
   BVec3( 1.0f, 1.0f, 0.0f),
   BVec3(-1.0f, 1.0f, 0.0f),
   BVec3( 1.0f,-1.0f, 0.0f),
   BVec3(-1.0f,-1.0f, 0.0f),
   
   BVec3( 1.0f, 0.0f, 1.0f),
   BVec3(-1.0f, 0.0f, 1.0f),
   BVec3( 1.0f, 0.0f,-1.0f),
   BVec3(-1.0f, 0.0f,-1.0f),
   
   BVec3( 0.0f, 1.0f, 1.0f),
   BVec3( 0.0f,-1.0f, 1.0f),
   BVec3( 0.0f, 1.0f,-1.0f),
   BVec3( 0.0f,-1.0f,-1.0f),
   
   BVec3( 1.0f, 1.0f, 0.0f),
   BVec3(-1.0f, 1.0f, 0.0f),
   BVec3( 0.0f,-1.0f, 1.0f),
   BVec3( 0.0f,-1.0f,-1.0f)
};

float BCoherentNoise::noise1D(float p)
{
   if (!mInitialized) 
      initTables();

   int loIndex, hiIndex;
   float loFract, hiFract;
   calcInterpParams(p, loIndex, hiIndex, loFract, hiFract);
            
   return Math::Lerp(loFract * mGradient1[permute(loIndex)], hiFract * mGradient1[permute(hiIndex)], interpFunc(loFract));
}
   
// Tilable 1D coherent noise.
float BCoherentNoise::noise1DWrap(float z, float t)
{
   return ((t - z) * noise1D(z) + z * noise1D(z - t)) / t;
}

float BCoherentNoise::noise1DHarmonicSum(float x, float alpha, float beta, int n)
{
   float p = x;
   float scale = 1.0f;
   float sum = 0.0f;
   
   for (int i = 0; i < n; i++) 
   {
      const float val = noise1D(p);
      sum += val / scale;
      scale *= alpha;
      p *= beta;
   }
   
   return sum;
}

float BCoherentNoise::noise2D(const BVec2& vec)
{
   if (!mInitialized) 
      initTables();

   int loIndex[2], hiIndex[2];
   BVec2 loFract, hiFract;
   for (int k = 0; k < 2; k++)
      calcInterpParams(vec[k], loIndex[k], hiIndex[k], loFract[k], hiFract[k]);
            
   const float sx = interpFunc(loFract[0]);
   
   // Bilinear interpolation
   float u = mGradient2[permute(permute(loIndex[0]) + loIndex[1])] * loFract;
   float v = mGradient2[permute(permute(hiIndex[0]) + loIndex[1])] * BVec2(hiFract[0], loFract[1]);
   const float a = Math::Lerp(u, v, sx);
   
   u = mGradient2[permute(permute(loIndex[0]) + hiIndex[1])] * BVec2(loFract[0], hiFract[1]);
   v = mGradient2[permute(permute(hiIndex[0]) + hiIndex[1])] * hiFract;
   const float b = Math::Lerp(u, v, sx);

   return Math::Lerp(a, b, interpFunc(loFract[1]));
}

// Tilable 2D coherent noise.
float BCoherentNoise::noise2DWrap(const BVec2& v, float w, float h)
{
   return (noise2D(v                        ) * (w - v[0]) * (h - v[1]) +
           noise2D(BVec2(v[0] - w, v[1])    ) * v[0]       * (h - v[1]) +
           noise2D(BVec2(v[0],     v[1] - h)) * (w - v[0]) * v[1] + 
           noise2D(BVec2(v[0] - w, v[1] - h)) * v[0]       * v[1]) / (w * h);
}

float BCoherentNoise::noise2DHarmonicSum(const BVec2& v, float alpha, float beta, int n)
{
   float val;
   float sum = 0.0f;
   BVec2 p(v);
   float scale = 1.0f;

   for (int i = 0; i < n; i++) 
   {
      val = noise2D(p);
      sum += val / scale;
      scale *= alpha;
      p *= beta;
   }
   
   return sum;
}

float BCoherentNoise::noise3D(const BVec3& vec)
{
   if (!mInitialized) 
      initTables();
         
   int loIndex[3], hiIndex[3];
   BVec3 loFract, hiFract;               
   for (int k = 0; k < 3; k++)
      calcInterpParams(vec[k], loIndex[k], hiIndex[k], loFract[k], hiFract[k]);
            
   const int i = permute(loIndex[0]);
   const int j = permute(hiIndex[0]);

   const int index00 = permute(i + loIndex[1]);
   const int index10 = permute(j + loIndex[1]);
   const int index01 = permute(i + hiIndex[1]);
   const int index11 = permute(j + hiIndex[1]);

   float t = interpFunc(loFract[0]);
   float sy = interpFunc(loFract[1]);
   
   // Trilinear interpolation
   float u = mGradient3[permute(index00 + loIndex[2]) & Table3SizeMask] * loFract;
   float v = mGradient3[permute(index10 + loIndex[2]) & Table3SizeMask] * BVec3(hiFract[0], loFract[1], loFract[2]);
   float a = Math::Lerp(u, v, t);

   u = mGradient3[permute(index01 + loIndex[2]) & Table3SizeMask] * BVec3(loFract[0], hiFract[1], loFract[2]);
   v = mGradient3[permute(index11 + loIndex[2]) & Table3SizeMask] * BVec3(hiFract[0], hiFract[1], loFract[2]);
   float b = Math::Lerp(u, v, t);

   const float c = Math::Lerp(a, b, sy);

   u = mGradient3[permute(index00 + hiIndex[2]) & Table3SizeMask] * BVec3(loFract[0], loFract[1], hiFract[2]);
   v = mGradient3[permute(index10 + hiIndex[2]) & Table3SizeMask] * BVec3(hiFract[0], loFract[1], hiFract[2]);
   a = Math::Lerp(u, v, t);

   u = mGradient3[permute(index01 + hiIndex[2]) & Table3SizeMask] * BVec3(loFract[0], hiFract[1], hiFract[2]);
   v = mGradient3[permute(index11 + hiIndex[2]) & Table3SizeMask] * hiFract;
   b = Math::Lerp(u, v, t);

   const float d = Math::Lerp(a, b, sy);

   return Math::Lerp(c, d, interpFunc(loFract[2]));
}

// Tilable 3D coherent noise.
float BCoherentNoise::noise3Wrap(const BVec3& v, float w, float h, float d)
{
   return (
      noise3D(BVec3(v[0],     v[1],     v[2]))     * (w - v[0]) * (h - v[1]) * (d - v[2]) +
      noise3D(BVec3(v[0] - w, v[1],     v[2]))     *      v[0]  * (h - v[1]) * (d - v[2]) +
      noise3D(BVec3(v[0] - w, v[1] - h, v[2]))     *      v[0]  *      v[1]  * (d - v[2]) +
      noise3D(BVec3(v[0],     v[1] - h, v[2]))     * (w - v[0]) *      v[1]  * (d - v[2]) +
      
      noise3D(BVec3(v[0],     v[1],     v[2] - d)) * (w - v[0]) * (h - v[1]) * v[2] +
      noise3D(BVec3(v[0] - w, v[1],     v[2] - d)) *      v[0]  * (h - v[1]) * v[2] +
      noise3D(BVec3(v[0],     v[1] - h, v[2] - d)) * (w - v[0]) *      v[1]  * v[2] +
      noise3D(BVec3(v[0] - w, v[1] - h, v[2] - d)) *      v[0]  *      v[1]  * v[2] 
      ) / (w * h * d);
}

float BCoherentNoise::noise3DHarmonicSum(const BVec3& v, float alpha, float beta, int n)
{
   float val;
   float sum = 0.0f;
   BVec3 p(v);
   float scale = 1.0f;
   
   for (int i = 0 ; i < n; i++) 
   {
      val = noise3D(p);
      sum += val / scale;
      scale *= alpha;
      p *= beta;
   }
   
   return sum;
}

void BCoherentNoise::initTables(void)
{
   Random rand;
   
   // Compute permutation table.
   for (int i = 0 ; i < TableSize; i++) 
      mPermute[i] = static_cast<IndexType>(i);

   for (int i = 0; i < TableSize; i++)
      std::swap(mPermute[i], mPermute[rand.iRand(0, TableSize)]);
   
   // Compute 1D, 2D, and 3D gradient tables.               
   for (int i = 0 ; i < TableSize; i++) 
   {
      mGradient1[i] = (rand.iRand(0, 2) == 0) ? -1.0f : 1.0f; 

      // Choose random points until a point is found inside the unit circle.
      do
      {
         mGradient2[i] = BVec2::makeRandomUniform(-1.0f, 1.0f);
      } while (mGradient2[i].len() > 1.0f);
      mGradient2[i].normalize();
   }
   
   mInitialized = true;
}
 
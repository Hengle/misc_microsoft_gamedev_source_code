//============================================================================
//
//  noise.cpp
//  
//  Copyright (c) 2003, Ensemble Studios
//
//============================================================================
#include "xsystem.h"
#include "noise.h"


//============================================================================
// global instance for convenience
//============================================================================
BNoise gNoise(23465);


//============================================================================
// Canonical gradient table
//
// http://mrl.nyu.edu/~perlin/paper445.pdf  
//============================================================================
const BVector gGradientTable[16] = 
{
   // These are the real 12 we need.
   BVector(1.0f,1.0f,0.0f),
   BVector(-1.0f,1.0f,0.0f),
   BVector(1.0f,-1.0f,0.0f),
   BVector(-1.0f,-1.0f,0.0f),
   BVector(1.0f,0.0f,1.0f),
   BVector(-1.0f,0.0f,1.0f),
   BVector(1.0f,0.0f,-1.0f),
   BVector(-1.0f,0.0f,-1.0f),
   BVector(0.0f,1.0f,1.0f),
   BVector(0.0f,-1.0f,1.0f),
   BVector(0.0f,1.0f,-1.0f),
   BVector(0.0f,-1.0f,-1.0f),

   // These are duplicates for padding out to an even power of two table size to allow &16 instead of %12
   BVector(1.0f,1.0f,0.0f),
   BVector(-1.0f,1.0f,0.0f),
   BVector(0.0f,-1.0f,1.0f),
   BVector(0.0f,-1.0f,-1.0f)
};


//============================================================================
// BNoise::BNoise
//============================================================================
BNoise::BNoise(long seed)
{
   // jce [10/14/2003] -- Use a rand class directly without going through random manager since
   // we don't need this to stick around, be synced, or any of that other junk.
   BRandom3 rnd;
   rnd.setSeed(seed);

   // Build random permutation table.

   // First, fill it in order.
   for(long i=0; i<cNoiseTableSize; i++)
      mPermutationTable[i] = (BYTE)i;

   // Now shuffle.
   for(long i=0; i<cNoiseTableSize; i++)
   {
      // Pick a random spot between here and the end of the array.
      long swapIndex = i + (rnd.getRandom() % (cNoiseTableSize-i));

      // Swap.
      bswap(mPermutationTable[i], mPermutationTable[swapIndex]);
   }
}


//============================================================================
// BNoise::gradientNoise
//============================================================================
float BNoise::gradientNoise(float x, float y, float z)
{
   // X components
   long ix = long(floor(x));
   float fx0 = x - ix;
   float fx1 = fx0 - 1;
   float wx = smoothStep(fx0);

   // Y components
   long iy = long(floor(y));
   float fy0 = y - iy;
   float fy1 = fy0 - 1;
   float wy = smoothStep(fy0);

   // Z components
   long iz = long(floor(z));
   float fz0 = z - iz;
   float fz1 = fz0 - 1;
   float wz = smoothStep(fz0);

   // Trilinearly interpolate the eight neighbors.
   float vx0 = gradientLattice(ix, iy, iz, fx0, fy0, fz0);
   float vx1 = gradientLattice(ix+1, iy, iz, fx1, fy0, fz0);
   float vy0 = lerp(wx, vx0, vx1);

   vx0 = gradientLattice(ix, iy+1, iz, fx0, fy1, fz0);
   vx1 = gradientLattice(ix+1, iy+1, iz, fx1, fy1, fz0);
   float vy1 = lerp(wx, vx0, vx1);
   float vz0 = lerp(wy, vy0, vy1);

   vx0 = gradientLattice(ix, iy, iz+1, fx0, fy0, fz1);
   vx1 = gradientLattice(ix+1, iy, iz+1, fx1, fy0, fz1);
   vy0 = lerp(wx, vx0, vx1);

   vx0 = gradientLattice(ix, iy+1, iz+1, fx0, fy1, fz1);
   vx1 = gradientLattice(ix+1, iy+1, iz+1, fx1, fy1, fz1);
   vy1 = lerp(wx, vx0, vx1);
   float vz1 = lerp(wy, vy0, vy1);

   float result = lerp(wz, vz0, vz1);
   return(result);
}


//============================================================================
// BNoise::gradientLattice
//============================================================================
float BNoise::gradientLattice(long ix, long iy, long iz, float fx, float fy, float fz) const
{
   // Get gradient vector.
   const BVector &gradient = gGradientTable[getIndex(ix, iy, iz)&0xF];

   // Dot with fractional portion to get result.
   return(gradient.x*fx + gradient.y*fy + gradient.z*fz);
}


//============================================================================
// BNoise::fractalSum
//============================================================================
float BNoise::fractalSum(float x, float y, float z, float minFrequency, long octaves, float persistence)
{
   // Add octaves.
   float noise = 0.0f;
   float freq = minFrequency;
   float amp = 1.0f;
   for(long i=0; i<octaves; i++)
   {
      noise += amp * gradientNoise(freq*x, freq*y, freq*z);
      freq *= 2.0f;
      amp *= persistence;
   }

   // Dumb clamping -- TODO: something better.
   if(noise < -1.0f)
      noise = -1.0f;
   if(noise > 1.0f)
      noise = 1.0f;

   return(noise);
}


//============================================================================
// BNoise::turbulence
//============================================================================
float BNoise::turbulence(float x, float y, float z, float minFrequency, long octaves, float persistence)
{
   // Add octaves.
   float noise = 0.0f;
   float freq = minFrequency;
   float amp = 1.0f;
   for(long i=0; i<octaves; i++)
   {
      noise += amp * float(fabs(gradientNoise(freq*x, freq*y, freq*z)));
      freq *= 2.0f;
      amp *= persistence;
   }

   // Dumb clamping -- TODO: something better.
   if(noise > 1.0f)
      noise = 1.0f;

   // Bring from 0 to 1 range into -1 to 1 range.
   noise = 2.0f*noise - 1.0f;
   return(noise);
}


//============================================================================
// eof: noise.cpp
//============================================================================

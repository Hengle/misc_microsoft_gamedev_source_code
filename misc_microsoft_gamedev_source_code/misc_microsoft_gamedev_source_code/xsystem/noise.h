//============================================================================
//
//  noise.h
//  
//  Copyright (c) 2003, Ensemble Studios
//
//============================================================================


#pragma once

#ifndef _NOISE_H_
#define _NOISE_H_

//============================================================================
// Some constants
static const DWORD cNoiseTableSize = 256;   // must be a power of 2, making this bigger means using more than a byte
static const DWORD cNoiseTableMask = cNoiseTableSize-1;


//============================================================================
// class BNoise
//============================================================================
class BNoise
{
   public:
                              BNoise(long seed);

      float                   gradientNoise(float x, float y, float z);
      float                   turbulence(float x, float y, float z, float minFrequency, long octaves, float persistence);
      float                   fractalSum(float x, float y, float z, float minFrequency, long octaves, float persistence);

   protected:
      __forceinline BYTE      getPermutation(long x) const {return(mPermutationTable[x&cNoiseTableMask]);}
      __forceinline BYTE      getIndex(long x, long y, long z) const {return(getPermutation(x + getPermutation(y) +getPermutation(z)));}
      __forceinline float     smoothStep(float t) const {return(6.0f*t*t*t*t*t - 15.0f*t*t*t*t + 10.0f*t*t*t);}
      __forceinline float     lerp(float t, float x0, float x1) const {return(x0+t*(x1-x0));}
      float                   gradientLattice(long ix, long iy, long iz, float fx, float fy, float fz) const;


      BYTE                    mPermutationTable[cNoiseTableSize];
};

// It's only 256 bytes, so make a global one.
extern BNoise gNoise;

#endif


//============================================================================
// eof: noise.h
//============================================================================

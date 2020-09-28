/**********************************************************************

Filename    :   GFxRandom.h
Content     :   Pseudo-random number generator
Created     :   June 27, 2005
Authors     :   

Notes       :   
History     :   

Copyright   :   (c) 1998-2006 Scaleform Corp. All Rights Reserved.

Licensees may use this file in accordance with the valid Scaleform
Commercial License Agreement provided with the software.

This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING 
THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR ANY PURPOSE.

**********************************************************************/

#include "GFxRandom.h"

#include "GTimer.h"

// Global generator.
static GFxRandom::Generator Generator_1;

UInt32  GFxRandom::NextRandom()
{
    return Generator_1.NextRandom();
}

void    GFxRandom::SeedRandom(UInt32 seed)
{
    Generator_1.SeedRandom(seed);
}

Float   GFxRandom::GetUnitFloat()
{
    return Generator_1.GetUnitFloat();
}


// PRNG code adapted from the complimentary-multiply-with-carry
// code in the article: George Marsaglia, "Seeds for Random Number
// Generators", Communications of the ACM, May 2003, Vol 46 No 5,
// pp90-93.
//
// The article says:
//
// "Any one of the choices for seed table size and multiplier will
// provide a RNG that has passed extensive tests of randomness,
// particularly those in [3], yet is simple and fast --
// approximately 30 million random 32-bit integers per second on a
// 850MHz PC.  The period is a*b^n, where a is the multiplier, n
// the size of the seed table and b=2^32-1.  (a is chosen so that
// b is a primitive root of the prime a*b^n + 1.)"
//
// [3] Marsaglia, G., Zaman, A., and Tsang, W.  Toward a universal
// random number generator.  _Statistics and Probability Letters
// 8 (1990), 35-39.

//  const UInt64    a = 123471786;  // for SEED_COUNT=1024
//  const UInt64    a = 123554632;  // for SEED_COUNT=512
//  const UInt64    a = 8001634;    // for SEED_COUNT=255
//  const UInt64    a = 8007626;    // for SEED_COUNT=128
//  const UInt64    a = 647535442;  // for SEED_COUNT=64
//  const UInt64    a = 547416522;  // for SEED_COUNT=32
//  const UInt64    a = 487198574;  // for SEED_COUNT=16
const UInt64    a = 716514398;  // for SEED_COUNT=8


GFxRandom::Generator::Generator()
    :
    C(362436),
    I(GFxRandom::Random_SeedCount - 1)
{
    //SeedRandom(987654321);

    // we set the seed to the current tick count
    SeedRandom(static_cast<UInt32>(GTimer::GetTicks()));
}


// Simple pseudo-random to reseed the seeds.
// Suggested by the above article.
void    GFxRandom::Generator::SeedRandom(UInt32 seed)
{
    UInt32  j = seed;
    for (int i = 0; i < Random_SeedCount; i++)
    {
        j = j ^ (j << 13);
        j = j ^ (j >> 17);
        j = j ^ (j << 5);
        Q[i] = j;
    }
}


// Return the next pseudo-random number in the sequence.
UInt32  GFxRandom::Generator::NextRandom()
{
    UInt64  t;
    UInt32  x;

    //static UInt32 c = 362436;
    //static UInt32 i = SEED_COUNT - 1;
    const UInt32    r = 0xFFFFFFFE;

    I = (I+1) & (Random_SeedCount - 1);
    t = a * Q[I] + C;
    C = (UInt32) (t >> 32);
    x = (UInt32) (t + C);
    if (x < C)
    {
        x++;
        C++;
    }
    
    UInt32  val = r - x;
    Q[I] = val;
    return val;
}


Float   GFxRandom::Generator::GetUnitFloat()
{
    UInt32  r = NextRandom();

    // 24 bits of precision.
    return Float(r >> 8) / (16777216.0f - 1.0f);
}



#ifdef TEST_RANDOM

// Compile with e.G.:
//
//  gcc -o TuRandomTest TuRandom.Cpp -I.. -g -DTEST_TU_RANDOM -lstdc++
//
// Generate a test file of random numbers for DIEHARD.
int main()
{
    const int   COUNT = 15000000 / 4;   // number of 4-byte words; DIEHARD needs ~80M bits

    for (int i = 0; i < COUNT; i++)
    {
        UInt32  val = GFxRandom::NextRandom();
        fwrite(&val, sizeof(val), 1, stdout);
    }
}


#endif // TEST_RANDOM

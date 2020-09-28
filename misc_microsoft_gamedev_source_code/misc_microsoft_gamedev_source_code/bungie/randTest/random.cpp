//============================================================================
// random.h
// Copyright (c) 1996-2006, Ensemble Studios
//
//============================================================================
#include "ens.h"

#include "random.h"
#include <algorithm>

namespace ens
{

// Random number generators from http://www.ciphersbyritter.com/NEWS4/RANDC.HTM

/* Any one of RND_KISS, RND_MWC, RND_FIB, RND_LFIB4, RND_SWB, RND_SHR3, or RND_CONG
can be used in an expression to provide a random 32-bit
integer.

The RND_KISS generator, (Keep It Simple Stupid), is
designed to combine the two multiply-with-carry
generators in RND_MWC with the 3-shift register RND_SHR3 and
the congruential generator RND_CONG, using addition and
exclusive-or. Period about 2^123.
It is one of my favorite generators.

The  RND_MWC generator concatenates two 16-bit multiply-
with-carry generators, x(n)=36969x(n-1)+carry,
y(n)=18000y(n-1)+carry  mod 2^16, has period about
2^60 and seems to pass all tests of randomness. A
favorite stand-alone generator---faster than RND_KISS,
which contains it.

RND_FIB is the classical Fibonacci sequence
x(n)=x(n-1)+x(n-2),but taken modulo 2^32.
Its period is 3*2^31 if one of its two seeds is odd
and not 1 mod 8. It has little worth as a RNG by
itself, but provides a simple and fast component for
use in combination generators.

RND_SHR3 is a 3-shift-register generator with period
2^32-1. It uses y(n)=y(n-1)(I+L^17)(I+R^13)(I+L^5),
with the y's viewed as binary vectors, L the 32x32
binary matrix that shifts a vector left 1, and R its
transpose.  RND_SHR3 seems to pass all except those
related to the binary rank test, since 32 successive
values, as binary vectors, must be linearly
independent, while 32 successive truly random 32-bit
integers, viewed as binary vectors, will be linearly
independent only about 29% of the time.

RND_CONG is a congruential generator with the widely used 69069
multiplier: x(n)=69069x(n-1)+1234567.  It has period
2^32. The leading half of its 32 bits seem to pass
tests, but bits in the last half are too regular.

RND_LFIB4 is an extension of what I have previously
defined as a lagged Fibonacci generator:
x(n)=x(n-r) op x(n-s), with the x's in a finite
set over which there is a binary operation op, such
as +,- on integers mod 2^32, * on odd such integers,
exclusive-or(xor) on binary vectors. Except for
those using multiplication, lagged Fibonacci
generators fail various tests of randomness, unless
the lags are very long. (See RND_SWB below).
To see if more than two lags would serve to overcome
the problems of 2-lag generators using +,- or xor, I
have developed the 4-lag generator RND_LFIB4 using
addition: x(n)=x(n-256)+x(n-179)+x(n-119)+x(n-55)
mod 2^32. Its period is 2^31*(2^256-1), about 2^287,
and it seems to pass all tests---in particular,
those of the kind for which 2-lag generators using
+,-,xor seem to fail.  For even more confidence in
its suitability,  RND_LFIB4 can be combined with RND_KISS,
with a resulting period of about 2^410: just use
(RND_KISS+RND_LFIB4) in any C expression.

RND_SWB is a subtract-with-borrow generator that I
developed to give a simple method for producing
extremely long periods:
x(n)=x(n-222)-x(n-237)- borrow mod 2^32.
The 'borrow' is 0, or set to 1 if computing x(n-1)
caused overflow in 32-bit integer arithmetic. This
generator has a very long period, 2^7098(2^480-1),
about 2^7578.   It seems to pass all tests of
randomness, except for the Birthday Spacings test,
which it fails badly, as do all lagged Fibonacci
generators using +,- or xor. I would suggest
combining RND_SWB with RND_KISS, RND_MWC, RND_SHR3, or RND_CONG.
RND_KISS+RND_SWB has period >2^7700 and is highly
recommended.
Subtract-with-borrow has the same local behaviour
as lagged Fibonacci using +,-,xor---the borrow
merely provides a much longer period.
RND_SWB fails the birthday spacings test, as do all
lagged Fibonacci and other generators that merely
combine two previous values by means of =,- or xor.
Those failures are for a particular case: m=512
birthdays in a year of n=2^24 days. There are
choices of m and n for which lags >1000 will also
fail the test.  A reasonable precaution is to always
combine a 2-lag Fibonacci or RND_SWB generator with
another kind of generator, unless the generator uses
*, for which a very satisfactory sequence of odd
32-bit integers results.

The classical Fibonacci sequence mod 2^32 from RND_FIB
fails several tests.  It is not suitable for use by
itself, but is quite suitable for combining with
other generators.

The last half of the bits of RND_CONG are too regular,
and it fails tests for which those bits play a
significant role. RND_CONG+RND_FIB will also have too much
regularity in trailing bits, as each does. But keep
in mind that it is a rare application for which
the trailing bits play a significant role.  RND_CONG
is one of the most widely used generators of the
last 30 years, as it was the system generator for
VAX and was incorporated in several popular
software packages, all seemingly without complaint.

Finally, because many simulations call for uniform
random variables in 0<x<1 or -1<x<1, I use #define
statements that permit inclusion of such variates
directly in expressions:  using RND_UNI will provide a
uniform random real (float) in (0,1), while RNV_VNI will
provide one in (-1,1).

All of these: RND_MWC, RND_SHR3, RND_CONG, RND_KISS, RND_LFIB4, RND_SWB, RND_FIB
RND_UNI and RNV_VNI, permit direct insertion of the desired
random quantity into an expression, avoiding the
time and space costs of a function call. I call
these in-line-define functions.  To use them, static
variables z,w,jsr,jcong,a and b should be assigned
seed values other than their initial values.  If
RND_LFIB4 or RND_SWB are used, the static table table[256] must
be initialized.

A note on timing:  It is difficult to provide exact
time costs for inclusion of one of these in-line-
define functions in an expression.  Times may differ
widely for different compilers, as the C operations
may be deeply nested and tricky. I suggest these
rough comparisons, based on averaging ten runs of a
routine that is essentially a long loop:
for(i=1;i<10000000;i++) L=RND_KISS; then with RND_KISS
replaced with RND_SHR3, RND_CONG,... or RND_KISS+RND_SWB, etc. The
times on my home PC, a Pentium 300MHz, in nanoseconds:
RND_FIB 49;RND_LFIB4 77;RND_SWB 80;RND_CONG 80;RND_SHR3 84;RND_MWC 93;RND_KISS 157;
RNV_VNI 417;RND_UNI 450;
*/

#define RND_UC    (uchar)  /*a cast operation*/
#define RND_ZNEW  (z=36969*(z&65535)+(z>>16))
#define RND_WNEW  (w=18000*(w&65535)+(w>>16))
#define RND_MWC   ((RND_ZNEW<<16)+RND_WNEW )
#define RND_SHR3  (jsr^=(jsr<<17), jsr^=(jsr>>13), jsr^=(jsr<<5))
#define RND_CONG  (jcong=69069*jcong+1234567)
#define RND_FIB   ((b=a+b),(a=b-a))
#define RND_KISS  ((RND_MWC^RND_CONG)+RND_SHR3)
#define RND_LFIB4 (c++,table[c]=table[c]+table[RND_UC(c+58)]+table[RND_UC(c+119)]+table[RND_UC(c+178)])
#define RND_SWB   (c++,bro=(x<y),table[c]=(x=table[RND_UC(c+34)])-(y=table[RND_UC(c+19)]+bro))
#define RND_UNI   (RND_KISS*2.328306e-10)
#define RNV_VNI   ((long) RND_KISS)*4.656613e-10

Random::Random()
{
   setSeed(12345, 65435, 34221, 12345, 9983651, 95746118);
}

uint Random::uRand(void) 
{
   return RND_KISS + RND_SWB;
}

double Random::dRand(double l, double h)
{
   double d;
   do 
   {
      const uint r = uRand();
      d = l + (h - l) * (r / 4294967296.0);
      // Probably not necessary, but I'm paranoid about roundoff error and I want to guarantee the output will always be in range.
   } while ((d < l) || (d >= h));
   return d;
}

float Random::fRand(float l, float h)
{
   if (l == h)
      return l;

   float f;
   do 
   {
      const uint r = uRand();
      f = static_cast<float>(l + (h - l) * (r * (1.0/4294967296.0)));
      // Probably not necessary, but I'm paranoid. This guarantees the output will always be in range.
   } while ((f < l) || (f >= h));
   return f;
}

int Random::iRand(int l, int h)
{
   assert(l < h);
   // This is done in FP to avoid biasing, see:
   // "Misconceptions about rand()"
   // http://www.azillionmonkeys.com/qed/random.html
   // Yes this is brutally slow, but we only every use this for testing.
   const float f = fRand(0.0f, 1.0f);
   const int r = Math::FloatToIntTrunc(l + (h - l) * f);
   assert((r >= l) && (r < h));
   return r;
}

float Random::fRandGaussian(float m, float s)
{
   float y1;
   if (usePrevGaussian)		        
   {
      y1 = prevGaussian;
      usePrevGaussian = false;
   }
   else
   {
      float x1, x2, w;

      do 
      {
         x1 = 2.0f * fRand() - 1.0f;
         x2 = 2.0f * fRand() - 1.0f;
         w = x1 * x1 + x2 * x2;
      } 
      while (w >= 1.0f);

      w = sqrt((-2.0f * log(w)) / w);

      // This transform outputs two at a time, so remember the 2nd for the next call.
      y1 = x1 * w;
      prevGaussian = x2 * w;
      usePrevGaussian = true;
   }

   return m + y1 * s;
}

void Random::setSeed(uint i1, uint i2, uint i3, uint i4, uint i5, uint i6)
{
   z = 362436069;
   w = 521288629;
   jsr = 123456789;
   jcong = 380116160;
   a = 224466889;
   b = 7584631;
   x = 0;
   y = 0;
   c = 0;
   usePrevGaussian = false;
   prevGaussian = 0.0f;
   setTable(i1, i2, i3, i4, i5, i6);
}

void Random::setSeed(uint seed)
{
   jcong = seed;

   DWORD s[6];
   for (int i = 0; i < 6; i++)
   {
      DWORD r0 = RND_CONG;
      DWORD r1 = RND_CONG;
      DWORD r2 = RND_CONG;
      DWORD r3 = RND_CONG;
      r0 >>= 24;
      r1 >>= 24;
      r2 >>= 24;
      r3 >>= 24;
      s[i] = r0 | (r1 << 8) | (r2 << 16) | (r3 << 24);
   }
   setSeed(s[0], s[1], s[2], s[3], s[4], s[5]);
}

void Random::setTable(uint i1, uint i2, uint i3,uint i4,uint i5, uint i6)
{ 
   z=i1;w=i2,jsr=i3; jcong=i4; a=i5; b=i6;
   for(int i=0;i<256;i=i+1)  
      table[i]=RND_KISS;
}

bool Random::test(void)
{
   int i; uint k = 0;
   setTable(12345,65435,34221,12345,9983651,95746118);

   // Should print 7 zeros
   bool passed = true;

   printf("Sequence verification (should print all 0's):\n");
   for(i=1;i<1000001;i++){k=RND_LFIB4;} 
   if (k-1064612766U) passed = false;
   printf("%u\n", k-1064612766U);

   for(i=1;i<1000001;i++){k=RND_SWB  ;} 
   if (k-627749721U) passed = false;
   printf("%u\n", k-627749721U);

   for(i=1;i<1000001;i++){k=RND_KISS ;} 
   if (k-1372460312U) passed = false;
   printf("%u\n", k-1372460312U);

   for(i=1;i<1000001;i++){k=RND_CONG ;} 
   if (k-1529210297U) passed = false;
   printf("%u\n", k-1529210297U);

   for(i=1;i<1000001;i++){k=RND_SHR3 ;} 
   if (k-2642725982U) passed = false;
   printf("%u\n", k-2642725982U);

   for(i=1;i<1000001;i++){k=RND_MWC  ;} 
   if (k-904977562U) passed = false;
   printf("%u\n", k-904977562U);

   for(i=1;i<1000001;i++){k=RND_FIB  ;} 
   if (k-3519793928U) passed = false;
   printf("%u\n", k-3519793928U);

   const int N = 32;
   int count[N];
   std::fill(count, count + N, 0);
   for (int i = 0; i < 5000000; i++)
   {
      count[iRand(0, N)]++;
      assert(iRand(0,1) == 0);
   }
   printf("Uniform:\n");
   for (int i = 0; i < N; i++)
      printf("%u\n", count[i]);
   printf("\n");

   std::fill(count, count + N, 0);
   for (int i = 0; i < 100000; i++)
   {
      float f = fRandGaussian(0.0f, 1.0f) / 6.0f;
      f = Math::Clamp(f, -1.0f, 1.0f);
      int k = Math::FloatToIntTrunc(f * (N/2) + (N/2));
      //debugRangeCheck(k, N);
      assert(k < N);
      count[k]++;
   }

   printf("Gaussian:\n");
   for (int i = 0; i < N; i++)
      printf("%u\n", count[i]);
   printf("\n");

   printf("Math::Random passed: %i\n", passed);

   return passed;
}

} // namespace ens

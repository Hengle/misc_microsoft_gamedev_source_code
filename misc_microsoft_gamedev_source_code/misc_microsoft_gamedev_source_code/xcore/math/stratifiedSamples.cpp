// File: stratifiedSamples.cpp
#include "xcore.h"
#include "generalVector.h"
#include "stratifiedSamples.h"

// n = -1 : random (sphere)
// n = 0 : proportional to solid angle (hemisphere)
// n = 1 : proportional to cosine-weighted solid angle (hemisphere)
// n > 1 : proportional to cosine lobe around normal, n = power (hemisphere)
void generateStratifiedSamples(BDynamicArray<BVec3>& samples, float n, int sqrtNumSamples)
{   
   if (n >= 0.0f)
      n = 1.0f / (1.0f + n);

   samples.resize(sqrtNumSamples*sqrtNumSamples);

   //   Scalar phi = TWO_PI * p.x;
   //   Scalar sint = sqrt( p.y );
   //   Scalar cost = sqrt( 1.0 - p.y );
   //   Vector3D a( cos(phi)*sint, sin(phi)*sint, cost );

   float q = 0.0f;   
   int index = 0;
   for(int i = 0; i < sqrtNumSamples; i++)
   {
      for(int j = 0; j < sqrtNumSamples; j++)
      {
         const double x = (i + Math::fRand(0.0f, 1.0f)) / sqrtNumSamples;
         const double y = (j + Math::fRand(0.0f, 1.0f)) / sqrtNumSamples;

         //         samples[index].set(	
         //            cos(2.0f*Math::fPi*x)*sqrt(1.0f-y),
         //            sin(2.0f*Math::fPi*x)*sqrt(1.0f-y),
         //            sqrt(y));

         double theta, phi;
         if (n < 0.0f)
            theta = acos(1.0f - 2.0f * x);
         else
            theta = acos(pow(x, (double)n));

         phi = 2.0f * Math::fPi * y;

         samples[index].set(	
            (float)(sin(theta) * cos(phi)),
            (float)(sin(theta) * sin(phi)),
            (float)cos(theta));

         q += (float)cos(theta);            

         index++;            
      }
   }                  
}

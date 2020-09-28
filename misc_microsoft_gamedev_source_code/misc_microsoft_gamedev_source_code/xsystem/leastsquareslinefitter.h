//==============================================================================
// leastsquareslinefitter.h
//
// Copyright (c) 2005 Ensemble Studios
//==============================================================================
#pragma once

template<typename FloatType>
struct BLeastSquaresLineFitter
{
   struct BPoint
   {
      FloatType x;
      FloatType y;
   };

   // Equation of returned line is y = b2 + b1 * x
   static void fit(FloatType& b1, FloatType& b2, const BPoint* pPoints, int numPoints)
   {
      if (!numPoints)
      {
         b1 = 0;
         b2 = 0;
         return;
      }
      
      FloatType a = 0.0f;
      for (int i = 0; i < numPoints; i++)
         a += pPoints[i].x * pPoints[i].y;
      a *= numPoints;

      FloatType xSum = 0.0f;
      for (int i = 0; i < numPoints; i++)
         xSum += pPoints[i].x;

      FloatType ySum = 0.0f;
      for (int i = 0; i < numPoints; i++)
         ySum += pPoints[i].y;

      a -= xSum * ySum;

      FloatType xSqSum = 0.0f;
      for (int i = 0; i < numPoints; i++)
         xSqSum += pPoints[i].x * pPoints[i].x;

      FloatType divisor = (numPoints * xSqSum - xSum * xSum);
      BASSERT(abs(divisor) > cFloatCompareEpsilon);
      b1 = a / divisor;
      b2 = (1.0f / numPoints) * (ySum - b1 * xSum);
   }
};

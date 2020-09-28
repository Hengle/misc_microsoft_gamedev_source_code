//============================================================================
//
//  bezier.cpp
//  
//  Copyright (c) 2003, Ensemble Studios
//
//============================================================================
#include "xsystem.h"
#include "bezier.h"



//============================================================================
// bezierEvaluate
//============================================================================
void bezierEvaluate(const BVector points[4], float t, BVector &result)
{
   float oneMinusT = 1.0f - t;
   float oneMinusT2 = oneMinusT*oneMinusT;
   float oneMinusT3 = oneMinusT*oneMinusT2;
   float t2 = t*t;
   float t3 = t2*t;

   result = oneMinusT3*points[0] + 3.0f*t*oneMinusT2*points[1] + 3.0f*t2*oneMinusT*points[2] + t3*points[3];
}


//============================================================================
// bezierEvaluateTangent
//============================================================================
void bezierEvaluateTangent(const BVector points[4], float t, BVector &result)
{
   BVector g = 3.0f*(points[1]-points[0]);
   BVector b = 3.0f*(points[2]-points[1]) - g;
   BVector a = points[3] - points[0] - b - g;
   result = 3.0f*a*t*t + 2*b*t + g;
   
   // Normalize, or if the length is zero, give back a "random" normalized vector.
   // This would happen, for example, if your curve had duplicate points.
   float length = result.length();
   if(length > cFloatCompareEpsilon)
      result.normalize();
   else
      result.set(1.0f, 0.0f, 0.0f);
}




//============================================================================
// BBezier::setPoint
//============================================================================
void BBezier::setPoint(long index, const BVector &point)
{
   // Range check.
   if(index<0 || index>3)
   {
      BFAIL("point index out of range.");
      return;
   }

   // Assign.
   mPoints[index] = point;
}


//============================================================================
// BBezier::setPoint
//============================================================================
void BBezier::setPoint(long index, float x, float y, float z)
{
   // Range check.
   if(index<0 || index>3)
   {
      BFAIL("point index out of range.");
      return;
   }

   // Assign.
   mPoints[index].x = x;
   mPoints[index].y = y;
   mPoints[index].z = z;
}


//============================================================================
// BBezier::setPoints
//============================================================================
void BBezier::setPoints(BVector points[4])
{
   // Sanity.
   if(!points)
   {
      BFAIL("Null point list.");
      return;
   }

   // Assign.
   memcpy(mPoints, points, 4*sizeof(mPoints[0]));
}


//============================================================================
// BBezier::setPoints
//============================================================================
void BBezier::setPoints(const BVector &p1, const BVector &p2, const BVector &tan1, const BVector &tan2)
{
   // Get delta between points.
   BVector delta;
   delta.assignDifference(p2, p1);
   float dist = delta.length();

   // First endpoint.
   mPoints[0] = p1;

   // Point along first tangent.
   mPoints[1] = p1 + cOneThird * dist * tan1;

   // Point along second tangent.
   mPoints[2] = p2 - cOneThird * dist * tan2;

   // Second endpoint.
   mPoints[3] = p2;
}


//============================================================================
// BBezier::evaluate
//============================================================================
void BBezier::evaluate(float t, BVector &result) const
{
   bezierEvaluate(mPoints, t, result);
}


//============================================================================
// BBezier::evaluateTangent
//============================================================================
void BBezier::evaluateTangent(float t, BVector &result) const
{
   bezierEvaluateTangent(mPoints, t, result);
}




//============================================================================
//BBezierLine::init
//============================================================================
bool BBezierLine::init(const BVector *points, long pointCount, const BVector *tangents, float *inLengths, float *outLengths)
{
   // Sanity.
   if(!points || pointCount<2)
   {
      mControlPoints.setNumber(0);
      return(false);
   }
   
   // We need a starting point and then three per curve since we share points.
   mControlPoints.setNumber(1 + 3*(pointCount-1));

   // Compute tangents if needed.
   BDynamicSimArray<BVector> tempTangents;
   if(!tangents)
   {
      tempTangents.setNumber(pointCount);

      // First tangent -- special case.  TODO: more configurable.
      tempTangents[0] = points[1] - points[0];
      tempTangents[0].normalize();

      // All the in-between tempTangents we'll choose as the direction from the previous point to the next point.
      for(long i=1; i<pointCount-1; i++)
      {
         tempTangents[i] = points[i+1] - points[i-1];
         tempTangents[i].normalize();
      }

      // Last tangent -- special case.  TODO: more configurable.
      tempTangents[pointCount-1] = points[pointCount-1] - points[pointCount-2];
      tempTangents[pointCount-1].normalize();
      
      tangents = tempTangents.getPtr();
   }


   // Build the control points.
   long i;
   for(i=0; i<pointCount-1; i++)
   {
      // Distance between this and next point.
      BVector delta = points[i+1] - points[i];
      float len = delta.length();

      mControlPoints[i*3] = points[i];
      
      if(inLengths)
         mControlPoints[i*3 + 1] = points[i] + inLengths[i]*len*tangents[i];
      else
         mControlPoints[i*3 + 1] = points[i] + cOneThird*len*tangents[i];
         
      if(outLengths)
         mControlPoints[i*3 + 2] = points[i+1] - outLengths[i]*len*tangents[i+1];
      else
         mControlPoints[i*3 + 2] = points[i+1] - cOneThird*len*tangents[i+1];
   }
   // Last point.
   mControlPoints[i*3] = points[pointCount-1];


   // Success.
   return(true);
}


//============================================================================
// BBezierLine::evaluate
//============================================================================
bool BBezierLine::evaluate(float t, BVector &result) const
{
   // Sanity.
   if(mControlPoints.getNumber() <= 0)
   {
      BFAIL("Bezier line not initialized properly.");
      result = cOriginVector;
      return(false);
   }

   // Find out which curve.
   long curveIndex = long(floor(t));

   // Deal with out of range stuff.
   // Negative gives you first control point.
   if(curveIndex<0)
   {
      result = mControlPoints[0];
      return(false);
   }

   // Too big gives you last control point.
   long numPoints = mControlPoints.getNumber();
   long maxCurveIndex = (numPoints-4)/3;
   if(curveIndex > maxCurveIndex)
   {
      result = mControlPoints[numPoints-1];
      return(false);
   }

   // Parameter for curve.
   float newT = t-float(curveIndex);

   // Evaluate.
   bezierEvaluate(mControlPoints.getPtr()+curveIndex*3, newT, result);

   // Success.
   return(true);
}


//============================================================================
// BBezierLine::evaluateTangent
//============================================================================
bool BBezierLine::evaluateTangent(float t, BVector &result) const
{
   // Sanity.
   if(mControlPoints.getNumber() <= 0)
   {
      BFAIL("Bezier line not initialized properly.");
      result = cOriginVector;
      return(false);
   }

   // Find out which curve.
   long curveIndex = long(floor(t));

   // Deal with out of range stuff.
   bool rVal = true;
   if(curveIndex<0)
   {
      t = 0.0f;
      curveIndex = 0;
      rVal = false;
   }

   // Too big gives you last control point.
   long numPoints = mControlPoints.getNumber();
   long maxCurveIndex = (numPoints-4)/3;
   if(curveIndex > maxCurveIndex)
   {
      curveIndex = maxCurveIndex;
      t = getMaxParameter();
      rVal = false;
   }

   // Parameter for curve.
   float newT = t-float(curveIndex);

   // Evaluate.
   bezierEvaluateTangent(mControlPoints.getPtr()+curveIndex*3, newT, result);

   // Success.
   return(rVal);
}



//============================================================================
// eof: bezier.cpp
//============================================================================

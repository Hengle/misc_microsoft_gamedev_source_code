//============================================================================
//
//  bezier.h
//  
//  Copyright (c) 2003, Ensemble Studios
//
//============================================================================


#pragma once

#ifndef _BEZIER_H_
#define _BEZIER_H_

//============================================================================
// class BBezier
// 
// Cubic Bezier curve
//============================================================================
class BBezier
{
   public:

      void                    setPoint(long index, const BVector &point);
      void                    setPoint(long index, float x, float y, float z);
      void                    setPoints(BVector points[4]);

      // Set based on endpoints and tangents.  Tangents are expected to be normalized.
      void                    setPoints(const BVector &p1, const BVector &p2, const BVector &tan1, const BVector &tan2);

      void                    evaluate(float t, BVector &result) const;
      void                    evaluateTangent(float t, BVector &result) const;

   protected:
      BVector                 mPoints[4];
};


//============================================================================
// class BBezierLine
//============================================================================
class BBezierLine
{
   public:
      bool                    init(const BVector *points, long pointCount, const BVector *tangents=NULL, float *inLength=NULL, float *outLengths=NULL);

      bool                    evaluate(float t, BVector &result) const;
      bool                    evaluateTangent(float t, BVector &result) const;
      
      long                    getPointCount(void) const {return(mControlPoints.getNumber());}
      float                   getMaxParameter(void) const {return(float((getPointCount()-1)/3));}

   protected:
      BDynamicSimArray<BVector>   mControlPoints;
};


#endif


//============================================================================
// eof: bezier.h
//============================================================================

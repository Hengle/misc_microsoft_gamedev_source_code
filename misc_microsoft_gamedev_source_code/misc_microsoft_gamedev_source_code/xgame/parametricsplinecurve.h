//==============================================================================
// parametricsplinecurve.h
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================

#pragma once

#include "stream\stream.h"

//==============================================================================
//==============================================================================
// Based off of: http://www.doc.ic.ac.uk/~dfg/AndysSplineTutorial/Parametrics.html
// and http://www.cs.wisc.edu/graphics/Courses/559-f2003/splines.pdf
class BParametricSplineCurve
{
   public:
      BParametricSplineCurve() : mValid(false) { }

      void reset();

      void           init(const BVector& start, const BVector& middle, const BVector& end);
      BVector        evaluate(float t);

      bool save(BStream* pStream, int saveType) const;
      bool load(BStream* pStream, int saveType);

   private:
      BVector        mA[3];
      bool           mValid;
};

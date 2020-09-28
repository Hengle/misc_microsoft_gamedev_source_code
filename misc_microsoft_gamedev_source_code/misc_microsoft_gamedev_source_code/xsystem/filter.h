//==============================================================================
// filter.h
//
// Copyright (c) 1997, 1998, 1999, Ensemble Studios
//==============================================================================

#ifndef _FILTER_H_
#define _FILTER_H_

//==============================================================================
// Includes

//==============================================================================
// Forward declarations

//==============================================================================
// Const declarations

//==============================================================================
class BFilter
{
   public:
                     BFilter(void);
                     
                     // Helper constructor to make a 3x3 filter.
                     BFilter(float w0, float w1, float w2, 
                             float w3, float w4, float w5, 
                             float w6, float w7, float w8);

                     // Helper constructor to make a 5x5 filter.  Blech.
                     BFilter(float w0, float w1, float w2, float w3, float w4, 
                             float w5, float w6, float w7, float w8, float w9,
                             float w10, float w11, float w12, float w13, float w14,
                             float w15, float w16, float w17, float w18, float w19,
                             float w20, float w21, float w22, float w23, float w24);

                     ~BFilter(void);

      bool           init(long sizeX, long sizeZ);
      long           getSizeX(void) const {return(mSizeX);}
      long           getSizeZ(void) const {return(mSizeZ);}

      void           setWeight(long x, long z, float weight);
      float          getWeight(long x, long z) const;

      float          getRecipTotalWeight(void) const {return(mRecipTotalWeight);}

   protected:
      void           clear(void);
      void           computeTotalWeight(void);

      float          *mWeights;
      long           mSizeX;
      long           mSizeZ;
      float          mRecipTotalWeight;
};


//==============================================================================
#endif // _FILTER_H_

//==============================================================================
// eof: filter.h
//==============================================================================

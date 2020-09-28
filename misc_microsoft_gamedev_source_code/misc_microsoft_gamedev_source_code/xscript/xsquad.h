//==============================================================================
// xsquad.h
//
// Copyright (c) 1999-2003, Ensemble Studios
//==============================================================================

#ifndef _XSQUAD_H_
#define _XSQUAD_H_

//==============================================================================
// Includes.


//==============================================================================
class BXSQuad  
{
   public:
      BXSQuad( long o=0, long f1=0, long f2=0, long f3=0 ) :
         mOpcode(o),
         mF1(f1),
         mF2(f2),
         mF3(f3)
         { }

      long                       mOpcode;
      long                       mF1;
      long                       mF2;
      long                       mF3;
};
typedef BDynamicSimArray<BXSQuad> BXSQuadArray;

//==============================================================================
#endif // _XSQUAD_H_

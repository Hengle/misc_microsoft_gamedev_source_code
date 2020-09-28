//==============================================================================
// xsoptimizer.h
//
// Copyright (c) 1999-2003, Ensemble Studios
//==============================================================================

#ifndef _XSOPTIMIZER_H_
#define _XSOPTIMIZER_H_

//==============================================================================
// Includes.
#include "xsquad.h"

//==============================================================================
// Forward Declarations.
class BXSMessenger;


//==============================================================================
class BXSOptimizer
{
   public:
      //Ctor and Dtor.
      BXSOptimizer( BXSMessenger *messenger );
      ~BXSOptimizer( void );

      //Optimization methods.
      bool                       optimizeQuads( BXSQuadArray &quads ); 
      long                       getNumberQuadsOptimized( void ) { return(mNumberQuadsOptimized); }
      long                       getNumberBytesSaved( void ) { return(mNumberBytesSaved); }

      //Misc.
      void                       clear( void ) { }

   protected:
      BXSMessenger*              mMessenger;
      long                       mNumberQuadsOptimized;
      long                       mNumberBytesSaved;
};


//==============================================================================
#endif // _XSOPTIMIZER_H_

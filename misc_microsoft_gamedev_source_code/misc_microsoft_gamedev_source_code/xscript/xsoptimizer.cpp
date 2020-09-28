//==============================================================================
// xsoptimizer.cpp
//
// Copyright (c) 1999-2003, Ensemble Studios
//==============================================================================

//==============================================================================
// Includes
#include "xscript.h"
#include "xsoptimizer.h"
#include "xsmessenger.h"
#include "xsopcodes.h"

//==============================================================================
// Defines


//==============================================================================
// BXSOptimizer::BXSOptimizer
//==============================================================================
BXSOptimizer::BXSOptimizer(BXSMessenger *messenger) :
   mMessenger(messenger),
   mNumberQuadsOptimized(0),
   mNumberBytesSaved(0)
{
}

//==============================================================================
// BXSOptimizer::~BXSOptimizer
//==============================================================================
BXSOptimizer::~BXSOptimizer(void)
{
}

//==============================================================================
// BXSOptimizer::optimizeQuads
//==============================================================================
bool BXSOptimizer::optimizeQuads(BXSQuadArray &quads)
{
   quads;
   return(true);
}

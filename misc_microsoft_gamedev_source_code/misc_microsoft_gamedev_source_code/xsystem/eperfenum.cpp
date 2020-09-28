//==============================================================================
// eperfenum.cpp
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

// Includes
#include "xsystem.h"
#include "eperfenum.h"

DEF_PERF(Update)
DEF_PERF(Render)
DEF_PERF(Test1)
DEF_PERF(Test2)
DEF_PERF(Test3)
DEF_PERF(Test4)
DEF_PERF(Test5)
DEF_PERF(RenderPresent)

void addEPerfEnums(void)
{
   ADD_PERF(Update, None, 0)
   ADD_PERF(Render, None, 0)
   ADD_PERF(Test1, None, 0)
   ADD_PERF(Test2, None, 0)
   ADD_PERF(Test3, None, 0)
   ADD_PERF(Test4, None, 0)
   ADD_PERF(Test5, None, 0)
   ADD_PERF(RenderPresent, Render, 0)
};


//==============================================================================
// Crazy junk to auto add these.
//==============================================================================
BEPerfEnum ePerfEnum;

//==============================================================================
// eof: eperfenum.cpp
//==============================================================================


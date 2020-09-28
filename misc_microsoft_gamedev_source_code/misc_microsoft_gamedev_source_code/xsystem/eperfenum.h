//==============================================================================
// eperfenum.h
//
// Copyright (c) 2003, Ensemble Studios
//==============================================================================

#pragma once 

#ifndef _EPERFENUM_H_
#define _EPERFENUM_H_

#include "perf.h"


EXT_PERF(Update)
EXT_PERF(Render)
EXT_PERF(Test1)
EXT_PERF(Test2)
EXT_PERF(Test3)
EXT_PERF(Test4)
EXT_PERF(Test5)
EXT_PERF(RenderPresent)


//==============================================================================
// Crazy junk to auto add these.
//==============================================================================
void addEPerfEnums(void);
class BEPerfEnum
{
   public:
      BEPerfEnum() {addEPerfEnums();}
};

//==============================================================================
#endif

//==============================================================================
// eof: eperfenum.h
//==============================================================================

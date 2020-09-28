// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef Optim_h
#define Optim_h

#include <limits>

class OptimSinglePrecisionFloat {
    // Switching floating-point state may have a significant penalty,
    //  at least on earlier processors (such as P3) where it may be
    //  on the order of 25 cycles for each of the two switches.
    // P4 may be fast for switching between only 2 states.
    // This speeds up only: divide and sqrt,
    //  i.e. not regular arithmetic and not trigonometric.
 public:
    OptimSinglePrecisionFloat() {
#if defined(__WIN32)
        _controlfp( _PC_24, MCW_PC ); // single precision
#endif
    }
    ~OptimSinglePrecisionFloat() {
#if defined(__WIN32)
        _controlfp( _CW_DEFAULT, 0xfffff ); // default precision
#endif
    }
};

#endif

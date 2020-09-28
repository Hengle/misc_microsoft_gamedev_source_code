// This may look like C code, but it is really -*- C++ -*-
// Copyright Microsoft Corporation.  Written by Hugues Hoppe.

#if defined(__WIN32) && _MSC_VER>1000
#pragma once
#endif
#ifndef PreciseTime_h
#define PreciseTime_h

// Return absolute time, in secs (accuracy at least ~.001)
extern double GetVeryPreciseTime();

#endif

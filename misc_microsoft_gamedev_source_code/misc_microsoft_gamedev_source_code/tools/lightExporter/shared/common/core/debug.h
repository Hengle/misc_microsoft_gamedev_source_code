//-----------------------------------------------------------------------------
// File: debug.h
// Debug related helper functions.
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#pragma once
#ifndef DEBUG_H
#define DEBUG_H

#include "common/core/core.h"

namespace gr
{
  template<class T>					 T DebugNull(T i);
	template<class T, class U> T DebugRange(T i, U l, U h);
  template<class T, class U> T DebugRange(T i, U h);
  template<class T, class U> T DebugRangeIncl(T i, U l, U h);
  template<class T, class U> T DebugRangeIncl(T i, U h);
} // namespace gr

#include "debug.inl"

#endif // DEBUG_H



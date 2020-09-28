//============================================================================
//
//  rangecheck.h
//  
// Copyright (c) 1999-2006, Ensemble Studios
//
//============================================================================
#pragma once
#ifndef _RANGE_CHECK_H_
#define _RANGE_CHECK_H_

// Debug inlines use BDEBUG_ASSERT() when ENABLE_DEBUG_RANGE_CHECKS is defined, otherwise they are no-ops

// i must be non-zero
template<class T>              T debugCheckNull(T i);

// i must be [i,h)
template<class T, class U>     T debugRangeCheck(T i, U l, U h);

// i must be [0,h)
template<class T, class U>     T debugRangeCheck(T i, U h);

// i must be [l,h]
template<class T, class U>     T debugRangeCheckIncl(T i, U l, U h);

// i must be [0,h]
template<class T, class U>     T debugRangeCheckIncl(T i, U h);

// Non-debug inlines use BASSERT() when ENABLE_NORMAL_RANGE_CHECKS is defined, otherwise they are no-ops

// i must be non-zero
template<class T>              T checkNull(T i);

// i must be [i,h)
template<class T, class U>     T rangeCheck(T i, U l, U h);

// i must be [0,h)
template<class T, class U>     T rangeCheck(T i, U h);

// i must be [l,h]
template<class T, class U>     T rangeCheckIncl(T i, U l, U h);

// i must be [0,h]
template<class T, class U>     T rangeCheckIncl(T i, U h);

#include "rangecheck.inl"

#endif // _RANGE_CHECK_H_


//============================================================================
//
//  ScanLine.h
//
//  Copyright (c) 1999-2001, Ensemble Studios
//
//============================================================================


#ifndef __SCAN_LINE_H__
#define __SCAN_LINE_H__


//----------------------------------------------------------------------------
//  Callback Prototypes
//----------------------------------------------------------------------------
typedef void (CALLBACK FORWARD_LINE_FUNC)(long x, long y, void* pParam);
typedef void (CALLBACK SYMMETRIC_LINE_FUNC)(long x, long y, bool isReverse, void* pParam);


//----------------------------------------------------------------------------
//  Line Functions
//----------------------------------------------------------------------------
void scanLineForward  (long x1, long y1, long x2, long y2, ClipRect* pClip, FORWARD_LINE_FUNC*   pFunc, void* pParam);
void scanLineSymmetric(long x1, long y1, long x2, long y2, ClipRect* pClip, SYMMETRIC_LINE_FUNC* pFunc, void* pParam);


#endif




#if !defined(GRANNY_BSPLINE_SOLVER_DEBUGGING_H)
#include "header_preamble.h"
// ========================================================================
// $File: //jeffr/granny/rt/granny_bspline_solver_debugging.h $
// $DateTime: 2006/10/16 14:57:23 $
// $Change: 13583 $
// $Revision: #4 $
//
// (C) Copyright 1999-2007 by RAD Game Tools, All Rights Reserved.
// ========================================================================

#if DEBUG_BSPLINE_SOLVER

#if !defined(GRANNY_TYPES_H)
#include "granny_types.h"
#endif

BEGIN_GRANNY_NAMESPACE;

#define DUMP_IN_MATRIX_FORM 1

#define DUMP_A   1
#define DUMP_ATA 1

#define DUMP_B   1
#define DUMP_ATB 1

#define DUMP_C   1
#define DUMP_FS  1
#define DUMP_BS  1

void DebugBeginSolve(void);
void DebugEndSolve(void);
void DebugDumpA(int32x KnotCount, int32x *KnotStart,
                int32x BandWidth, real32 *A);
void DebugDumpb(int32x bCount, int32x bWidth, real32 *b);
void DebugDumpATb(int32x KnotCount, int32x bWidth, real32 *ATb);
void DebugDumpATA(int32x ATn, int32x BandWidth, int32x *KnotStart,
                  real32 *Knots, real32 *ATA);
void DebugDumpC(int32x ATn, int32x BandWidth, real32 *ATA);
void DebugDumpFSATb(int32x ATn, int32x ATbStride, real32 *ATb);
void DebugDumpBSATb(int32x ATn, int32x ATbStride, real32 *ATb);

END_GRANNY_NAMESPACE;

#else

#define DebugBeginSolve()
#define DebugEndSolve()
#define DebugDumpA(a, b, c, d)
#define DebugDumpb(a, b, c)
#define DebugDumpATb(a, b, c)
#define DebugDumpATA(a, b, c, d, e)
#define DebugDumpC(a, b, c);
#define DebugDumpFSATb(a, b, c);
#define DebugDumpBSATb(a, b, c);

#endif

#include "header_postfix.h"
#define GRANNY_BSPLINE_SOLVER_DEBUGGING_H
#endif

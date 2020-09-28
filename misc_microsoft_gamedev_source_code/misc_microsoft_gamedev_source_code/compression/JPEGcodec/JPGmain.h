//------------------------------------------------------------------------------
// JPGmain.h
// Last updated: Nov. 16, 2000 v0.93
//------------------------------------------------------------------------------
#pragma once
#include "xcore.h"
#include <setjmp.h>

#if 0
#ifndef MAIN_H
#define MAIN_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <assert.h>

typedef   signed char  schar;       /*  8 bits     */
typedef unsigned char  uchar;       /*  8 bits     */
typedef   signed short int16;       /* 16 bits     */
typedef unsigned short uint16;      /* 16 bits     */
typedef unsigned short ushort;      /* 16 bits     */
typedef unsigned int   uint;        /* 16/32+ bits */
typedef unsigned long  uint;       /* 32 bits     */
typedef   signed int   int32;       /* 32+ bits    */

#ifndef max
#define max(a,b) (((a)>(b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a)<(b)) ? (a) : (b))
#endif

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#endif
#endif


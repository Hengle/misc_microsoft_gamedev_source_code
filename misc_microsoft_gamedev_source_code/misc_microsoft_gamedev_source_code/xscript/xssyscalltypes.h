//==============================================================================
// xssyscalltypes.h
//
// Copyright (c) 2005, Ensemble Studios
//==============================================================================

#ifndef _XSSYSCALLTYPES_H_
#define _XSSYSCALLTYPES_H_

enum
{
   cSyscallType_void,
   cSyscallType_d,
   cSyscallType_f,
   cSyscallType_b,
   cSyscallType_b1d1,
   cSyscallType_b1d2,
   cSyscallType_b1d6,
   cSyscallType_b1f1,
   cSyscallType_b1f2,
   cSyscallType_d1b1,
   cSyscallType_d1b1d1,
   cSyscallType_d1b2,
   cSyscallType_d1b2f4b2,
   cSyscallType_d1b3f1b1,
   cSyscallType_d1b4,
   cSyscallType_d1f1,
   cSyscallType_d1f1b1,
   cSyscallType_d1f1b1d1,
   cSyscallType_d1f1d1,
   cSyscallType_d1f1d1b1,
   cSyscallType_d1f1d1f2,
   cSyscallType_d1f1d1f4,
   cSyscallType_d1f1d3,
   cSyscallType_d1f2,
   cSyscallType_d1f2b1,
   cSyscallType_d1f2d1,
   cSyscallType_d1f2d1f1,
   cSyscallType_d1f3,
   cSyscallType_d1f3d1,
   cSyscallType_d1f4,
   cSyscallType_d1f4b1,
   cSyscallType_d1f4d2b1,
   cSyscallType_d1f5,
   cSyscallType_d1f5b1,
   cSyscallType_d1f7,
   cSyscallType_d2b1,
   cSyscallType_d2b1d1,
   cSyscallType_d2b1d1b1,
   cSyscallType_d2b1f1,
   cSyscallType_d2b2,
   cSyscallType_d2f1,
   cSyscallType_d2f1b1,
   cSyscallType_d2f1b2,
   cSyscallType_d2f1d1,
   cSyscallType_d2f1d1f1,
   cSyscallType_d2f2,
   cSyscallType_d2f2b1,
   cSyscallType_d2f2d1,
   cSyscallType_d2f3,
   cSyscallType_d2f3d1,
   cSyscallType_d2f3d2,
   cSyscallType_d2f4d1,
   cSyscallType_d3b1,
   cSyscallType_d3b1d2,
   cSyscallType_d3b2f3,
   cSyscallType_d3b3,
   cSyscallType_d3f1,
   cSyscallType_d3f1b1,
   cSyscallType_d3f2b1,
   cSyscallType_d3f3,
   cSyscallType_d3f3d1b1,
   cSyscallType_d4b1,
   cSyscallType_d4b1d3,
   cSyscallType_d4f1d1,
   cSyscallType_d4f2,
   cSyscallType_d5b1,
   cSyscallType_d5b1d1,
   cSyscallType_d5f1,
   cSyscallType_f1b1,
   cSyscallType_f1b1d4,
   cSyscallType_f1d1,
   cSyscallType_f1d1f1,
   cSyscallType_f1d1f2,
   cSyscallType_f2b1,
   cSyscallType_f2d1,
   cSyscallType_f2d2,
   cSyscallType_f3d1,
   cSyscallType_f3d1b1d2f1,
   cSyscallType_f3d1b2,
	cSyscallType_f3d1b2f1,
	cSyscallType_d2b2f1,
	cSyscallType_d2f4,
	cSyscallType_d3f4d1,
	cSyscallType_f5b1,
   cNumberSyscallTypes
};

typedef void (*XSPROC)(void);

typedef void (*XSPROC_d1) (long a1);
typedef void (*XSPROC_d2) (long a1, long a2);
typedef void (*XSPROC_d3) (long a1, long a2, long a3);
typedef void (*XSPROC_d4) (long a1, long a2, long a3, long a4);
typedef void (*XSPROC_d5) (long a1, long a2, long a3, long a4, long a5);
typedef void (*XSPROC_d6) (long a1, long a2, long a3, long a4, long a5, long a6);
typedef void (*XSPROC_d7) (long a1, long a2, long a3, long a4, long a5, long a6, long a7);
typedef void (*XSPROC_d8) (long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8);
typedef void (*XSPROC_d9) (long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9);
typedef void (*XSPROC_d10)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10);
typedef void (*XSPROC_d11)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11);
typedef void (*XSPROC_d12)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11, long a12);
typedef void (*XSPROC_d13)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11, long a12, long a13);
typedef void (*XSPROC_d14)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11, long a12, long a13, long a14);

typedef void (*XSPROC_f1) (float a1);
typedef void (*XSPROC_f2) (float a1, float a2);
typedef void (*XSPROC_f3) (float a1, float a2, float a3);
typedef void (*XSPROC_f4) (float a1, float a2, float a3, float a4);
typedef void (*XSPROC_f5) (float a1, float a2, float a3, float a4, float a5);
typedef void (*XSPROC_f6) (float a1, float a2, float a3, float a4, float a5, float a6);
typedef void (*XSPROC_f7) (float a1, float a2, float a3, float a4, float a5, float a6, float a7);
typedef void (*XSPROC_f8) (float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8);
typedef void (*XSPROC_f9) (float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9);
typedef void (*XSPROC_f10)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10);
typedef void (*XSPROC_f11)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11);
typedef void (*XSPROC_f12)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, float a12);
typedef void (*XSPROC_f13)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, float a12, float a13);
typedef void (*XSPROC_f14)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, float a12, float a13, float a14);

typedef void (*XSPROC_b1) (bool a1);
typedef void (*XSPROC_b2) (bool a1, bool a2);
typedef void (*XSPROC_b3) (bool a1, bool a2, bool a3);
typedef void (*XSPROC_b4) (bool a1, bool a2, bool a3, bool a4);
typedef void (*XSPROC_b5) (bool a1, bool a2, bool a3, bool a4, bool a5);
typedef void (*XSPROC_b6) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6);
typedef void (*XSPROC_b7) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7);
typedef void (*XSPROC_b8) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8);
typedef void (*XSPROC_b9) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9);
typedef void (*XSPROC_b10)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10);
typedef void (*XSPROC_b11)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11);
typedef void (*XSPROC_b12)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11, bool a12);
typedef void (*XSPROC_b13)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11, bool a12, bool a13);
typedef void (*XSPROC_b14)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11, bool a12, bool a13, bool a14);

typedef void (*XSPROC_b1d1)        (bool a1, long a2);
typedef void (*XSPROC_b1d2)        (bool a1, long a2, long a3);
typedef void (*XSPROC_b1d6)        (bool a1, long a2, long a3, long a4, long a5, long a6, long a7);
typedef void (*XSPROC_b1f1)        (bool a1, float a2);
typedef void (*XSPROC_b1f2)        (bool a1, float a2, float a3);
typedef void (*XSPROC_d1b1)        (long a1, bool a2);
typedef void (*XSPROC_d1b1d1)      (long a1, bool a2, long a3);
typedef void (*XSPROC_d1b2)        (long a1, bool a2, bool a3);
typedef void (*XSPROC_d1b2f4b2)    (long a1, bool a2, bool a3, float a4, float a5, float a6, float a7, bool a8, bool a9);
typedef void (*XSPROC_d1b3f1b1)    (long a1, bool a2, bool a3, bool a4, float a5, bool a6);
typedef void (*XSPROC_d1b4)        (long a1, bool a2, bool a3, bool a4, bool a5);
typedef void (*XSPROC_d1f1)        (long a1, float a2);
typedef void (*XSPROC_d1f1b1)      (long a1, float a2, bool a3);
typedef void (*XSPROC_d1f1b1d1)    (long a1, float a2, bool a3, long a4);
typedef void (*XSPROC_d1f1d1)      (long a1, float a2, long a3);
typedef void (*XSPROC_d1f1d1b1)    (long a1, float a2, long a3, bool a4);
typedef void (*XSPROC_d1f1d1f2)    (long a1, float a2, long a3, float a4, float a5);
typedef void (*XSPROC_d1f1d1f4)    (long a1, float a2, long a3, float a4, float a5, float a6, float a7);
typedef void (*XSPROC_d1f1d3)      (long a1, float a2, long a3, long a4, long a5);
typedef void (*XSPROC_d1f2)        (long a1, float a2, float a3);
typedef void (*XSPROC_d1f2b1)      (long a1, float a2, float a3, bool a4);
typedef void (*XSPROC_d1f2d1)      (long a1, float a2, float a3, long a4);
typedef void (*XSPROC_d1f2d1f1)    (long a1, float a2, float a3, long a4, float a5);
typedef void (*XSPROC_d1f3)        (long a1, float a2, float a3, float a4);
typedef void (*XSPROC_d1f3d1)      (long a1, float a2, float a3, float a4, long a5);
typedef void (*XSPROC_d1f4)        (long a1, float a2, float a3, float a4, float a5);
typedef void (*XSPROC_d1f4b1)      (long a1, float a2, float a3, float a4, float a5, bool a6);
typedef void (*XSPROC_d1f4d2b1)    (long a1, float a2, float a3, float a4, float a5, long a6, long a7, bool a8);
typedef void (*XSPROC_d1f5)        (long a1, float a2, float a3, float a4, float a5, float a6);
typedef void (*XSPROC_d1f5b1)      (long a1, float a2, float a3, float a4, float a5, float a6, bool a7);
typedef void (*XSPROC_d1f7)        (long a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8);
typedef void (*XSPROC_d2b1)        (long a1, long a2, bool a3);
typedef void (*XSPROC_d2b1d1)      (long a1, long a2, bool a3, long a4);
typedef void (*XSPROC_d2b1d1b1)    (long a1, long a2, bool a3, long a4, bool a5);
typedef void (*XSPROC_d2b1f1)      (long a1, long a2, bool a3, float a4);
typedef void (*XSPROC_d2b2)        (long a1, long a2, bool a3, bool a4);
typedef void (*XSPROC_d2f1)        (long a1, long a2, float a3);
typedef void (*XSPROC_d2f1b1)      (long a1, long a2, float a3, bool a4);
typedef void (*XSPROC_d2f1b2)      (long a1, long a2, float a3, bool a4, bool a5);
typedef void (*XSPROC_d2f1d1)      (long a1, long a2, float a3, long a4);
typedef void (*XSPROC_d2f1d1f1)    (long a1, long a2, float a3, long a4, float a5);
typedef void (*XSPROC_d2f2)        (long a1, long a2, float a3, float a4);
typedef void (*XSPROC_d2f2b1)      (long a1, long a2, float a3, float a4, bool a5);
typedef void (*XSPROC_d2f2d1)      (long a1, long a2, float a3, float a4, long a5);
typedef void (*XSPROC_d2f3)        (long a1, long a2, float a3, float a4, float a5);
typedef void (*XSPROC_d2f3d1)      (long a1, long a2, float a3, float a4, float a5, long a6);
typedef void (*XSPROC_d2f3d2)      (long a1, long a2, float a3, float a4, float a5, long a6, long a7);
typedef void (*XSPROC_d2f4d1)      (long a1, long a2, float a3, float a4, float a5, float a6, long a7);
typedef void (*XSPROC_d3b1)        (long a1, long a2, long a3, bool a4);
typedef void (*XSPROC_d3b1d2)      (long a1, long a2, long a3, bool a4, long a5, long a6);
typedef void (*XSPROC_d3b2f3)      (long a1, long a2, long a3, bool a4, bool a5, float a6, float a7, float a8);
typedef void (*XSPROC_d3b3)        (long a1, long a2, long a3, bool a4, bool a5, bool a6);
typedef void (*XSPROC_d3f1)        (long a1, long a2, long a3, float a4);
typedef void (*XSPROC_d3f1b1)      (long a1, long a2, long a3, float a4, bool a5);
typedef void (*XSPROC_d3f2b1)      (long a1, long a2, long a3, float a4, float a5, bool a6);
typedef void (*XSPROC_d3f3)        (long a1, long a2, long a3, float a4, float a5, float a6);
typedef void (*XSPROC_d3f3d1b1)    (long a1, long a2, long a3, float a4, float a5, float a6, long a7, bool a8);
typedef void (*XSPROC_d4b1)        (long a1, long a2, long a3, long a4, bool a5);
typedef void (*XSPROC_d4b1d3)      (long a1, long a2, long a3, long a4, bool a5, long a6, long a7, long a8);
typedef void (*XSPROC_d4f1d1)      (long a1, long a2, long a3, long a4, float a5, long a6);
typedef void (*XSPROC_d4f2)        (long a1, long a2, long a3, long a4, float a5, float a6);
typedef void (*XSPROC_d5b1)        (long a1, long a2, long a3, long a4, long a5, bool a6);
typedef void (*XSPROC_d5b1d1)      (long a1, long a2, long a3, long a4, long a5, bool a6, long a7);
typedef void (*XSPROC_d5f1)        (long a1, long a2, long a3, long a4, long a5, float a6);
typedef void (*XSPROC_f1b1)        (float a1, bool a2);
typedef void (*XSPROC_f1b1d4)      (float a1, bool a2, long a3, long a4, long a5, long a6);
typedef void (*XSPROC_f1d1)        (float a1, long a2);
typedef void (*XSPROC_f1d1f1)      (float a1, long a2, float a3);
typedef void (*XSPROC_f1d1f2)      (float a1, long a2, float a3, float a4);
typedef void (*XSPROC_f2b1)        (float a1, float a2, bool a3);
typedef void (*XSPROC_f2d1)        (float a1, float a2, long a3);
typedef void (*XSPROC_f2d2)        (float a1, float a2, long a3, long a4);
typedef void (*XSPROC_f3d1)        (float a1, float a2, float a3, long a4);
typedef void (*XSPROC_f3d1b1d2f1)  (float a1, float a2, float a3, long a4, bool a5, long a6, long a7, float a8);
typedef void (*XSPROC_f3d1b2)      (float a1, float a2, float a3, long a4, bool a5, bool a6);
typedef void (*XSPROC_f3d1b2f1)	  (float a1, float a2, float a3, long a4, bool a5, bool a6, float a7);
typedef void (*XSPROC_d2b2f1)		  (long a1, long a2, bool a3, bool a4, float a5);
typedef void (*XSPROC_d2f4)		  (long a1, long a2, float a3, float a4, float a5, float a6);
typedef void (*XSPROC_d3f4d1)		  (long a1, long a2, long a3, float a4, float a5, float a6, float a7, long a8);
typedef void (*XSPROC_f5b1)		  (float a1, float a2, float a3, float a4, float a5, bool a6);

typedef long (*XSFUNd)(void);

typedef long (*XSFUNd_d1) (long a1);
typedef long (*XSFUNd_d2) (long a1, long a2);
typedef long (*XSFUNd_d3) (long a1, long a2, long a3);
typedef long (*XSFUNd_d4) (long a1, long a2, long a3, long a4);
typedef long (*XSFUNd_d5) (long a1, long a2, long a3, long a4, long a5);
typedef long (*XSFUNd_d6) (long a1, long a2, long a3, long a4, long a5, long a6);
typedef long (*XSFUNd_d7) (long a1, long a2, long a3, long a4, long a5, long a6, long a7);
typedef long (*XSFUNd_d8) (long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8);
typedef long (*XSFUNd_d9) (long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9);
typedef long (*XSFUNd_d10)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10);
typedef long (*XSFUNd_d11)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11);
typedef long (*XSFUNd_d12)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11, long a12);
typedef long (*XSFUNd_d13)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11, long a12, long a13);
typedef long (*XSFUNd_d14)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11, long a12, long a13, long a14);

typedef long (*XSFUNd_f1) (float a1);
typedef long (*XSFUNd_f2) (float a1, float a2);
typedef long (*XSFUNd_f3) (float a1, float a2, float a3);
typedef long (*XSFUNd_f4) (float a1, float a2, float a3, float a4);
typedef long (*XSFUNd_f5) (float a1, float a2, float a3, float a4, float a5);
typedef long (*XSFUNd_f6) (float a1, float a2, float a3, float a4, float a5, float a6);
typedef long (*XSFUNd_f7) (float a1, float a2, float a3, float a4, float a5, float a6, float a7);
typedef long (*XSFUNd_f8) (float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8);
typedef long (*XSFUNd_f9) (float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9);
typedef long (*XSFUNd_f10)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10);
typedef long (*XSFUNd_f11)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11);
typedef long (*XSFUNd_f12)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, float a12);
typedef long (*XSFUNd_f13)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, float a12, float a13);
typedef long (*XSFUNd_f14)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, float a12, float a13, float a14);

typedef long (*XSFUNd_b1) (bool a1);
typedef long (*XSFUNd_b2) (bool a1, bool a2);
typedef long (*XSFUNd_b3) (bool a1, bool a2, bool a3);
typedef long (*XSFUNd_b4) (bool a1, bool a2, bool a3, bool a4);
typedef long (*XSFUNd_b5) (bool a1, bool a2, bool a3, bool a4, bool a5);
typedef long (*XSFUNd_b6) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6);
typedef long (*XSFUNd_b7) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7);
typedef long (*XSFUNd_b8) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8);
typedef long (*XSFUNd_b9) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9);
typedef long (*XSFUNd_b10)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10);
typedef long (*XSFUNd_b11)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11);
typedef long (*XSFUNd_b12)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11, bool a12);
typedef long (*XSFUNd_b13)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11, bool a12, bool a13);
typedef long (*XSFUNd_b14)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11, bool a12, bool a13, bool a14);

typedef long (*XSFUNd_b1d1)        (bool a1, long a2);
typedef long (*XSFUNd_b1d2)        (bool a1, long a2, long a3);
typedef long (*XSFUNd_b1d6)        (bool a1, long a2, long a3, long a4, long a5, long a6, long a7);
typedef long (*XSFUNd_b1f1)        (bool a1, float a2);
typedef long (*XSFUNd_b1f2)        (bool a1, float a2, float a3);
typedef long (*XSFUNd_d1b1)        (long a1, bool a2);
typedef long (*XSFUNd_d1b1d1)      (long a1, bool a2, long a3);
typedef long (*XSFUNd_d1b2)        (long a1, bool a2, bool a3);
typedef long (*XSFUNd_d1b2f4b2)    (long a1, bool a2, bool a3, float a4, float a5, float a6, float a7, bool a8, bool a9);
typedef long (*XSFUNd_d1b3f1b1)    (long a1, bool a2, bool a3, bool a4, float a5, bool a6);
typedef long (*XSFUNd_d1b4)        (long a1, bool a2, bool a3, bool a4, bool a5);
typedef long (*XSFUNd_d1f1)        (long a1, float a2);
typedef long (*XSFUNd_d1f1b1)      (long a1, float a2, bool a3);
typedef long (*XSFUNd_d1f1b1d1)    (long a1, float a2, bool a3, long a4);
typedef long (*XSFUNd_d1f1d1)      (long a1, float a2, long a3);
typedef long (*XSFUNd_d1f1d1b1)    (long a1, float a2, long a3, bool a4);
typedef long (*XSFUNd_d1f1d1f2)    (long a1, float a2, long a3, float a4, float a5);
typedef long (*XSFUNd_d1f1d1f4)    (long a1, float a2, long a3, float a4, float a5, float a6, float a7);
typedef long (*XSFUNd_d1f1d3)      (long a1, float a2, long a3, long a4, long a5);
typedef long (*XSFUNd_d1f2)        (long a1, float a2, float a3);
typedef long (*XSFUNd_d1f2b1)      (long a1, float a2, float a3, bool a4);
typedef long (*XSFUNd_d1f2d1)      (long a1, float a2, float a3, long a4);
typedef long (*XSFUNd_d1f2d1f1)    (long a1, float a2, float a3, long a4, float a5);
typedef long (*XSFUNd_d1f3)        (long a1, float a2, float a3, float a4);
typedef long (*XSFUNd_d1f3d1)      (long a1, float a2, float a3, float a4, long a5);
typedef long (*XSFUNd_d1f4)        (long a1, float a2, float a3, float a4, float a5);
typedef long (*XSFUNd_d1f4b1)      (long a1, float a2, float a3, float a4, float a5, bool a6);
typedef long (*XSFUNd_d1f4d2b1)    (long a1, float a2, float a3, float a4, float a5, long a6, long a7, bool a8);
typedef long (*XSFUNd_d1f5)        (long a1, float a2, float a3, float a4, float a5, float a6);
typedef long (*XSFUNd_d1f5b1)      (long a1, float a2, float a3, float a4, float a5, float a6, bool a7);
typedef long (*XSFUNd_d1f7)        (long a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8);
typedef long (*XSFUNd_d2b1)        (long a1, long a2, bool a3);
typedef long (*XSFUNd_d2b1d1)      (long a1, long a2, bool a3, long a4);
typedef long (*XSFUNd_d2b1d1b1)    (long a1, long a2, bool a3, long a4, bool a5);
typedef long (*XSFUNd_d2b1f1)      (long a1, long a2, bool a3, float a4);
typedef long (*XSFUNd_d2b2)        (long a1, long a2, bool a3, bool a4);
typedef long (*XSFUNd_d2f1)        (long a1, long a2, float a3);
typedef long (*XSFUNd_d2f1b1)      (long a1, long a2, float a3, bool a4);
typedef long (*XSFUNd_d2f1b2)      (long a1, long a2, float a3, bool a4, bool a5);
typedef long (*XSFUNd_d2f1d1)      (long a1, long a2, float a3, long a4);
typedef long (*XSFUNd_d2f1d1f1)    (long a1, long a2, float a3, long a4, float a5);
typedef long (*XSFUNd_d2f2)        (long a1, long a2, float a3, float a4);
typedef long (*XSFUNd_d2f2b1)      (long a1, long a2, float a3, float a4, bool a5);
typedef long (*XSFUNd_d2f2d1)      (long a1, long a2, float a3, float a4, long a5);
typedef long (*XSFUNd_d2f3)        (long a1, long a2, float a3, float a4, float a5);
typedef long (*XSFUNd_d2f3d1)      (long a1, long a2, float a3, float a4, float a5, long a6);
typedef long (*XSFUNd_d2f3d2)      (long a1, long a2, float a3, float a4, float a5, long a6, long a7);
typedef long (*XSFUNd_d2f4d1)      (long a1, long a2, float a3, float a4, float a5, float a6, long a7);
typedef long (*XSFUNd_d3b1)        (long a1, long a2, long a3, bool a4);
typedef long (*XSFUNd_d3b1d2)      (long a1, long a2, long a3, bool a4, long a5, long a6);
typedef long (*XSFUNd_d3b2f3)      (long a1, long a2, long a3, bool a4, bool a5, float a6, float a7, float a8);
typedef long (*XSFUNd_d3b3)        (long a1, long a2, long a3, bool a4, bool a5, bool a6);
typedef long (*XSFUNd_d3f1)        (long a1, long a2, long a3, float a4);
typedef long (*XSFUNd_d3f1b1)      (long a1, long a2, long a3, float a4, bool a5);
typedef long (*XSFUNd_d3f2b1)      (long a1, long a2, long a3, float a4, float a5, bool a6);
typedef long (*XSFUNd_d3f3)        (long a1, long a2, long a3, float a4, float a5, float a6);
typedef long (*XSFUNd_d3f3d1b1)    (long a1, long a2, long a3, float a4, float a5, float a6, long a7, bool a8);
typedef long (*XSFUNd_d4b1)        (long a1, long a2, long a3, long a4, bool a5);
typedef long (*XSFUNd_d4b1d3)      (long a1, long a2, long a3, long a4, bool a5, long a6, long a7, long a8);
typedef long (*XSFUNd_d4f1d1)      (long a1, long a2, long a3, long a4, float a5, long a6);
typedef long (*XSFUNd_d4f2)        (long a1, long a2, long a3, long a4, float a5, float a6);
typedef long (*XSFUNd_d5b1)        (long a1, long a2, long a3, long a4, long a5, bool a6);
typedef long (*XSFUNd_d5b1d1)      (long a1, long a2, long a3, long a4, long a5, bool a6, long a7);
typedef long (*XSFUNd_d5f1)        (long a1, long a2, long a3, long a4, long a5, float a6);
typedef long (*XSFUNd_f1b1)        (float a1, bool a2);
typedef long (*XSFUNd_f1b1d4)      (float a1, bool a2, long a3, long a4, long a5, long a6);
typedef long (*XSFUNd_f1d1)        (float a1, long a2);
typedef long (*XSFUNd_f1d1f1)      (float a1, long a2, float a3);
typedef long (*XSFUNd_f1d1f2)      (float a1, long a2, float a3, float a4);
typedef long (*XSFUNd_f2b1)        (float a1, float a2, bool a3);
typedef long (*XSFUNd_f2d1)        (float a1, float a2, long a3);
typedef long (*XSFUNd_f2d2)        (float a1, float a2, long a3, long a4);
typedef long (*XSFUNd_f3d1)        (float a1, float a2, float a3, long a4);
typedef long (*XSFUNd_f3d1b1d2f1)  (float a1, float a2, float a3, long a4, bool a5, long a6, long a7, float a8);
typedef long (*XSFUNd_f3d1b2)      (float a1, float a2, float a3, long a4, bool a5, bool a6);
typedef long (*XSFUNd_f3d1b2f1)	  (float a1, float a2, float a3, long a4, bool a5, bool a6, float a7);
typedef long (*XSFUNd_d2b2f1)		  (long a1, long a2, bool a3, bool a4, float a5);
typedef long (*XSFUNd_d2f4)		  (long a1, long a2, float a3, float a4, float a5, float a6);
typedef long (*XSFUNd_d3f4d1)		  (long a1, long a2, long a3, float a4, float a5, float a6, float a7, long a8);
typedef long (*XSFUNd_f5b1)		  (float a1, float a2, float a3, float a4, float a5, bool a6);

typedef float (*XSFUNf)(void);

typedef float (*XSFUNf_d1) (long a1);
typedef float (*XSFUNf_d2) (long a1, long a2);
typedef float (*XSFUNf_d3) (long a1, long a2, long a3);
typedef float (*XSFUNf_d4) (long a1, long a2, long a3, long a4);
typedef float (*XSFUNf_d5) (long a1, long a2, long a3, long a4, long a5);
typedef float (*XSFUNf_d6) (long a1, long a2, long a3, long a4, long a5, long a6);
typedef float (*XSFUNf_d7) (long a1, long a2, long a3, long a4, long a5, long a6, long a7);
typedef float (*XSFUNf_d8) (long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8);
typedef float (*XSFUNf_d9) (long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9);
typedef float (*XSFUNf_d10)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10);
typedef float (*XSFUNf_d11)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11);
typedef float (*XSFUNf_d12)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11, long a12);
typedef float (*XSFUNf_d13)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11, long a12, long a13);
typedef float (*XSFUNf_d14)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11, long a12, long a13, long a14);

typedef float (*XSFUNf_f1) (float a1);
typedef float (*XSFUNf_f2) (float a1, float a2);
typedef float (*XSFUNf_f3) (float a1, float a2, float a3);
typedef float (*XSFUNf_f4) (float a1, float a2, float a3, float a4);
typedef float (*XSFUNf_f5) (float a1, float a2, float a3, float a4, float a5);
typedef float (*XSFUNf_f6) (float a1, float a2, float a3, float a4, float a5, float a6);
typedef float (*XSFUNf_f7) (float a1, float a2, float a3, float a4, float a5, float a6, float a7);
typedef float (*XSFUNf_f8) (float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8);
typedef float (*XSFUNf_f9) (float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9);
typedef float (*XSFUNf_f10)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10);
typedef float (*XSFUNf_f11)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11);
typedef float (*XSFUNf_f12)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, float a12);
typedef float (*XSFUNf_f13)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, float a12, float a13);
typedef float (*XSFUNf_f14)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, float a12, float a13, float a14);

typedef float (*XSFUNf_b1) (bool a1);
typedef float (*XSFUNf_b2) (bool a1, bool a2);
typedef float (*XSFUNf_b3) (bool a1, bool a2, bool a3);
typedef float (*XSFUNf_b4) (bool a1, bool a2, bool a3, bool a4);
typedef float (*XSFUNf_b5) (bool a1, bool a2, bool a3, bool a4, bool a5);
typedef float (*XSFUNf_b6) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6);
typedef float (*XSFUNf_b7) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7);
typedef float (*XSFUNf_b8) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8);
typedef float (*XSFUNf_b9) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9);
typedef float (*XSFUNf_b10)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10);
typedef float (*XSFUNf_b11)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11);
typedef float (*XSFUNf_b12)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11, bool a12);
typedef float (*XSFUNf_b13)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11, bool a12, bool a13);
typedef float (*XSFUNf_b14)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11, bool a12, bool a13, bool a14);

typedef float (*XSFUNf_b1d1)        (bool a1, long a2);
typedef float (*XSFUNf_b1d2)        (bool a1, long a2, long a3);
typedef float (*XSFUNf_b1d6)        (bool a1, long a2, long a3, long a4, long a5, long a6, long a7);
typedef float (*XSFUNf_b1f1)        (bool a1, float a2);
typedef float (*XSFUNf_b1f2)        (bool a1, float a2, float a3);
typedef float (*XSFUNf_d1b1)        (long a1, bool a2);
typedef float (*XSFUNf_d1b1d1)      (long a1, bool a2, long a3);
typedef float (*XSFUNf_d1b2)        (long a1, bool a2, bool a3);
typedef float (*XSFUNf_d1b2f4b2)    (long a1, bool a2, bool a3, float a4, float a5, float a6, float a7, bool a8, bool a9);
typedef float (*XSFUNf_d1b3f1b1)    (long a1, bool a2, bool a3, bool a4, float a5, bool a6);
typedef float (*XSFUNf_d1b4)        (long a1, bool a2, bool a3, bool a4, bool a5);
typedef float (*XSFUNf_d1f1)        (long a1, float a2);
typedef float (*XSFUNf_d1f1b1)      (long a1, float a2, bool a3);
typedef float (*XSFUNf_d1f1b1d1)    (long a1, float a2, bool a3, long a4);
typedef float (*XSFUNf_d1f1d1)      (long a1, float a2, long a3);
typedef float (*XSFUNf_d1f1d1b1)    (long a1, float a2, long a3, bool a4);
typedef float (*XSFUNf_d1f1d1f2)    (long a1, float a2, long a3, float a4, float a5);
typedef float (*XSFUNf_d1f1d1f4)    (long a1, float a2, long a3, float a4, float a5, float a6, float a7);
typedef float (*XSFUNf_d1f1d3)      (long a1, float a2, long a3, long a4, long a5);
typedef float (*XSFUNf_d1f2)        (long a1, float a2, float a3);
typedef float (*XSFUNf_d1f2b1)      (long a1, float a2, float a3, bool a4);
typedef float (*XSFUNf_d1f2d1)      (long a1, float a2, float a3, long a4);
typedef float (*XSFUNf_d1f2d1f1)    (long a1, float a2, float a3, long a4, float a5);
typedef float (*XSFUNf_d1f3)        (long a1, float a2, float a3, float a4);
typedef float (*XSFUNf_d1f3d1)      (long a1, float a2, float a3, float a4, long a5);
typedef float (*XSFUNf_d1f4)        (long a1, float a2, float a3, float a4, float a5);
typedef float (*XSFUNf_d1f4b1)      (long a1, float a2, float a3, float a4, float a5, bool a6);
typedef float (*XSFUNf_d1f4d2b1)    (long a1, float a2, float a3, float a4, float a5, long a6, long a7, bool a8);
typedef float (*XSFUNf_d1f5)        (long a1, float a2, float a3, float a4, float a5, float a6);
typedef float (*XSFUNf_d1f5b1)      (long a1, float a2, float a3, float a4, float a5, float a6, bool a7);
typedef float (*XSFUNf_d1f7)        (long a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8);
typedef float (*XSFUNf_d2b1)        (long a1, long a2, bool a3);
typedef float (*XSFUNf_d2b1d1)      (long a1, long a2, bool a3, long a4);
typedef float (*XSFUNf_d2b1d1b1)    (long a1, long a2, bool a3, long a4, bool a5);
typedef float (*XSFUNf_d2b1f1)      (long a1, long a2, bool a3, float a4);
typedef float (*XSFUNf_d2b2)        (long a1, long a2, bool a3, bool a4);
typedef float (*XSFUNf_d2f1)        (long a1, long a2, float a3);
typedef float (*XSFUNf_d2f1b1)      (long a1, long a2, float a3, bool a4);
typedef float (*XSFUNf_d2f1b2)      (long a1, long a2, float a3, bool a4, bool a5);
typedef float (*XSFUNf_d2f1d1)      (long a1, long a2, float a3, long a4);
typedef float (*XSFUNf_d2f1d1f1)    (long a1, long a2, float a3, long a4, float a5);
typedef float (*XSFUNf_d2f2)        (long a1, long a2, float a3, float a4);
typedef float (*XSFUNf_d2f2b1)      (long a1, long a2, float a3, float a4, bool a5);
typedef float (*XSFUNf_d2f2d1)      (long a1, long a2, float a3, float a4, long a5);
typedef float (*XSFUNf_d2f3)        (long a1, long a2, float a3, float a4, float a5);
typedef float (*XSFUNf_d2f3d1)      (long a1, long a2, float a3, float a4, float a5, long a6);
typedef float (*XSFUNf_d2f3d2)      (long a1, long a2, float a3, float a4, float a5, long a6, long a7);
typedef float (*XSFUNf_d2f4d1)      (long a1, long a2, float a3, float a4, float a5, float a6, long a7);
typedef float (*XSFUNf_d3b1)        (long a1, long a2, long a3, bool a4);
typedef float (*XSFUNf_d3b1d2)      (long a1, long a2, long a3, bool a4, long a5, long a6);
typedef float (*XSFUNf_d3b2f3)      (long a1, long a2, long a3, bool a4, bool a5, float a6, float a7, float a8);
typedef float (*XSFUNf_d3b3)        (long a1, long a2, long a3, bool a4, bool a5, bool a6);
typedef float (*XSFUNf_d3f1)        (long a1, long a2, long a3, float a4);
typedef float (*XSFUNf_d3f1b1)      (long a1, long a2, long a3, float a4, bool a5);
typedef float (*XSFUNf_d3f2b1)      (long a1, long a2, long a3, float a4, float a5, bool a6);
typedef float (*XSFUNf_d3f3)        (long a1, long a2, long a3, float a4, float a5, float a6);
typedef float (*XSFUNf_d3f3d1b1)    (long a1, long a2, long a3, float a4, float a5, float a6, long a7, bool a8);
typedef float (*XSFUNf_d4b1)        (long a1, long a2, long a3, long a4, bool a5);
typedef float (*XSFUNf_d4b1d3)      (long a1, long a2, long a3, long a4, bool a5, long a6, long a7, long a8);
typedef float (*XSFUNf_d4f1d1)      (long a1, long a2, long a3, long a4, float a5, long a6);
typedef float (*XSFUNf_d4f2)        (long a1, long a2, long a3, long a4, float a5, float a6);
typedef float (*XSFUNf_d5b1)        (long a1, long a2, long a3, long a4, long a5, bool a6);
typedef float (*XSFUNf_d5b1d1)      (long a1, long a2, long a3, long a4, long a5, bool a6, long a7);
typedef float (*XSFUNf_d5f1)        (long a1, long a2, long a3, long a4, long a5, float a6);
typedef float (*XSFUNf_f1b1)        (float a1, bool a2);
typedef float (*XSFUNf_f1b1d4)      (float a1, bool a2, long a3, long a4, long a5, long a6);
typedef float (*XSFUNf_f1d1)        (float a1, long a2);
typedef float (*XSFUNf_f1d1f1)      (float a1, long a2, float a3);
typedef float (*XSFUNf_f1d1f2)      (float a1, long a2, float a3, float a4);
typedef float (*XSFUNf_f2b1)        (float a1, float a2, bool a3);
typedef float (*XSFUNf_f2d1)        (float a1, float a2, long a3);
typedef float (*XSFUNf_f2d2)        (float a1, float a2, long a3, long a4);
typedef float (*XSFUNf_f3d1)        (float a1, float a2, float a3, long a4);
typedef float (*XSFUNf_f3d1b1d2f1)  (float a1, float a2, float a3, long a4, bool a5, long a6, long a7, float a8);
typedef float (*XSFUNf_f3d1b2)      (float a1, float a2, float a3, long a4, bool a5, bool a6);
typedef float (*XSFUNf_f3d1b2f1)		(float a1, float a2, float a3, long a4, bool a5, bool a6, float a7);
typedef float (*XSFUNf_d2b2f1)		(long a1, long a2, bool a3, bool a4, float a5);
typedef float (*XSFUNf_d2f4)		  (long a1, long a2, float a3, float a4, float a5, float a6);
typedef float (*XSFUNf_d3f4d1)		  (long a1, long a2, long a3, float a4, float a5, float a6, float a7, long a8);
typedef float (*XSFUNf_f5b1)		  (float a1, float a2, float a3, float a4, float a5, bool a6);

typedef bool (*XSFUNb)(void);

typedef bool (*XSFUNb_d1) (long a1);
typedef bool (*XSFUNb_d2) (long a1, long a2);
typedef bool (*XSFUNb_d3) (long a1, long a2, long a3);
typedef bool (*XSFUNb_d4) (long a1, long a2, long a3, long a4);
typedef bool (*XSFUNb_d5) (long a1, long a2, long a3, long a4, long a5);
typedef bool (*XSFUNb_d6) (long a1, long a2, long a3, long a4, long a5, long a6);
typedef bool (*XSFUNb_d7) (long a1, long a2, long a3, long a4, long a5, long a6, long a7);
typedef bool (*XSFUNb_d8) (long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8);
typedef bool (*XSFUNb_d9) (long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9);
typedef bool (*XSFUNb_d10)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10);
typedef bool (*XSFUNb_d11)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11);
typedef bool (*XSFUNb_d12)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11, long a12);
typedef bool (*XSFUNb_d13)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11, long a12, long a13);
typedef bool (*XSFUNb_d14)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11, long a12, long a13, long a14);

typedef bool (*XSFUNb_f1) (float a1);
typedef bool (*XSFUNb_f2) (float a1, float a2);
typedef bool (*XSFUNb_f3) (float a1, float a2, float a3);
typedef bool (*XSFUNb_f4) (float a1, float a2, float a3, float a4);
typedef bool (*XSFUNb_f5) (float a1, float a2, float a3, float a4, float a5);
typedef bool (*XSFUNb_f6) (float a1, float a2, float a3, float a4, float a5, float a6);
typedef bool (*XSFUNb_f7) (float a1, float a2, float a3, float a4, float a5, float a6, float a7);
typedef bool (*XSFUNb_f8) (float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8);
typedef bool (*XSFUNb_f9) (float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9);
typedef bool (*XSFUNb_f10)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10);
typedef bool (*XSFUNb_f11)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11);
typedef bool (*XSFUNb_f12)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, float a12);
typedef bool (*XSFUNb_f13)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, float a12, float a13);
typedef bool (*XSFUNb_f14)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, float a12, float a13, float a14);

typedef bool (*XSFUNb_b1) (bool a1);
typedef bool (*XSFUNb_b2) (bool a1, bool a2);
typedef bool (*XSFUNb_b3) (bool a1, bool a2, bool a3);
typedef bool (*XSFUNb_b4) (bool a1, bool a2, bool a3, bool a4);
typedef bool (*XSFUNb_b5) (bool a1, bool a2, bool a3, bool a4, bool a5);
typedef bool (*XSFUNb_b6) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6);
typedef bool (*XSFUNb_b7) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7);
typedef bool (*XSFUNb_b8) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8);
typedef bool (*XSFUNb_b9) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9);
typedef bool (*XSFUNb_b10)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10);
typedef bool (*XSFUNb_b11)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11);
typedef bool (*XSFUNb_b12)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11, bool a12);
typedef bool (*XSFUNb_b13)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11, bool a12, bool a13);
typedef bool (*XSFUNb_b14)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11, bool a12, bool a13, bool a14);

typedef bool (*XSFUNb_b1d1)        (bool a1, long a2);
typedef bool (*XSFUNb_b1d2)        (bool a1, long a2, long a3);
typedef bool (*XSFUNb_b1d6)        (bool a1, long a2, long a3, long a4, long a5, long a6, long a7);
typedef bool (*XSFUNb_b1f1)        (bool a1, float a2);
typedef bool (*XSFUNb_b1f2)        (bool a1, float a2, float a3);
typedef bool (*XSFUNb_d1b1)        (long a1, bool a2);
typedef bool (*XSFUNb_d1b1d1)      (long a1, bool a2, long a3);
typedef bool (*XSFUNb_d1b2)        (long a1, bool a2, bool a3);
typedef bool (*XSFUNb_d1b2f4b2)    (long a1, bool a2, bool a3, float a4, float a5, float a6, float a7, bool a8, bool a9);
typedef bool (*XSFUNb_d1b3f1b1)    (long a1, bool a2, bool a3, bool a4, float a5, bool a6);
typedef bool (*XSFUNb_d1b4)        (long a1, bool a2, bool a3, bool a4, bool a5);
typedef bool (*XSFUNb_d1f1)        (long a1, float a2);
typedef bool (*XSFUNb_d1f1b1)      (long a1, float a2, bool a3);
typedef bool (*XSFUNb_d1f1b1d1)    (long a1, float a2, bool a3, long a4);
typedef bool (*XSFUNb_d1f1d1)      (long a1, float a2, long a3);
typedef bool (*XSFUNb_d1f1d1b1)    (long a1, float a2, long a3, bool a4);
typedef bool (*XSFUNb_d1f1d1f2)    (long a1, float a2, long a3, float a4, float a5);
typedef bool (*XSFUNb_d1f1d1f4)    (long a1, float a2, long a3, float a4, float a5, float a6, float a7);
typedef bool (*XSFUNb_d1f1d3)      (long a1, float a2, long a3, long a4, long a5);
typedef bool (*XSFUNb_d1f2)        (long a1, float a2, float a3);
typedef bool (*XSFUNb_d1f2b1)      (long a1, float a2, float a3, bool a4);
typedef bool (*XSFUNb_d1f2d1)      (long a1, float a2, float a3, long a4);
typedef bool (*XSFUNb_d1f2d1f1)    (long a1, float a2, float a3, long a4, float a5);
typedef bool (*XSFUNb_d1f3)        (long a1, float a2, float a3, float a4);
typedef bool (*XSFUNb_d1f3d1)      (long a1, float a2, float a3, float a4, long a5);
typedef bool (*XSFUNb_d1f4)        (long a1, float a2, float a3, float a4, float a5);
typedef bool (*XSFUNb_d1f4b1)      (long a1, float a2, float a3, float a4, float a5, bool a6);
typedef bool (*XSFUNb_d1f4d2b1)    (long a1, float a2, float a3, float a4, float a5, long a6, long a7, bool a8);
typedef bool (*XSFUNb_d1f5)        (long a1, float a2, float a3, float a4, float a5, float a6);
typedef bool (*XSFUNb_d1f5b1)      (long a1, float a2, float a3, float a4, float a5, float a6, bool a7);
typedef bool (*XSFUNb_d1f7)        (long a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8);
typedef bool (*XSFUNb_d2b1)        (long a1, long a2, bool a3);
typedef bool (*XSFUNb_d2b1d1)      (long a1, long a2, bool a3, long a4);
typedef bool (*XSFUNb_d2b1d1b1)    (long a1, long a2, bool a3, long a4, bool a5);
typedef bool (*XSFUNb_d2b1f1)      (long a1, long a2, bool a3, float a4);
typedef bool (*XSFUNb_d2b2)        (long a1, long a2, bool a3, bool a4);
typedef bool (*XSFUNb_d2f1)        (long a1, long a2, float a3);
typedef bool (*XSFUNb_d2f1b1)      (long a1, long a2, float a3, bool a4);
typedef bool (*XSFUNb_d2f1b2)      (long a1, long a2, float a3, bool a4, bool a5);
typedef bool (*XSFUNb_d2f1d1)      (long a1, long a2, float a3, long a4);
typedef bool (*XSFUNb_d2f1d1f1)    (long a1, long a2, float a3, long a4, float a5);
typedef bool (*XSFUNb_d2f2)        (long a1, long a2, float a3, float a4);
typedef bool (*XSFUNb_d2f2b1)      (long a1, long a2, float a3, float a4, bool a5);
typedef bool (*XSFUNb_d2f2d1)      (long a1, long a2, float a3, float a4, long a5);
typedef bool (*XSFUNb_d2f3)        (long a1, long a2, float a3, float a4, float a5);
typedef bool (*XSFUNb_d2f3d1)      (long a1, long a2, float a3, float a4, float a5, long a6);
typedef bool (*XSFUNb_d2f3d2)      (long a1, long a2, float a3, float a4, float a5, long a6, long a7);
typedef bool (*XSFUNb_d2f4d1)      (long a1, long a2, float a3, float a4, float a5, float a6, long a7);
typedef bool (*XSFUNb_d3b1)        (long a1, long a2, long a3, bool a4);
typedef bool (*XSFUNb_d3b1d2)      (long a1, long a2, long a3, bool a4, long a5, long a6);
typedef bool (*XSFUNb_d3b2f3)      (long a1, long a2, long a3, bool a4, bool a5, float a6, float a7, float a8);
typedef bool (*XSFUNb_d3b3)        (long a1, long a2, long a3, bool a4, bool a5, bool a6);
typedef bool (*XSFUNb_d3f1)        (long a1, long a2, long a3, float a4);
typedef bool (*XSFUNb_d3f1b1)      (long a1, long a2, long a3, float a4, bool a5);
typedef bool (*XSFUNb_d3f2b1)      (long a1, long a2, long a3, float a4, float a5, bool a6);
typedef bool (*XSFUNb_d3f3)        (long a1, long a2, long a3, float a4, float a5, float a6);
typedef bool (*XSFUNb_d3f3d1b1)    (long a1, long a2, long a3, float a4, float a5, float a6, long a7, bool a8);
typedef bool (*XSFUNb_d4b1)        (long a1, long a2, long a3, long a4, bool a5);
typedef bool (*XSFUNb_d4b1d3)      (long a1, long a2, long a3, long a4, bool a5, long a6, long a7, long a8);
typedef bool (*XSFUNb_d4f1d1)      (long a1, long a2, long a3, long a4, float a5, long a6);
typedef bool (*XSFUNb_d4f2)        (long a1, long a2, long a3, long a4, float a5, float a6);
typedef bool (*XSFUNb_d5b1)        (long a1, long a2, long a3, long a4, long a5, bool a6);
typedef bool (*XSFUNb_d5b1d1)      (long a1, long a2, long a3, long a4, long a5, bool a6, long a7);
typedef bool (*XSFUNb_d5f1)        (long a1, long a2, long a3, long a4, long a5, float a6);
typedef bool (*XSFUNb_f1b1)        (float a1, bool a2);
typedef bool (*XSFUNb_f1b1d4)      (float a1, bool a2, long a3, long a4, long a5, long a6);
typedef bool (*XSFUNb_f1d1)        (float a1, long a2);
typedef bool (*XSFUNb_f1d1f1)      (float a1, long a2, float a3);
typedef bool (*XSFUNb_f1d1f2)      (float a1, long a2, float a3, float a4);
typedef bool (*XSFUNb_f2b1)        (float a1, float a2, bool a3);
typedef bool (*XSFUNb_f2d1)        (float a1, float a2, long a3);
typedef bool (*XSFUNb_f2d2)        (float a1, float a2, long a3, long a4);
typedef bool (*XSFUNb_f3d1)        (float a1, float a2, float a3, long a4);
typedef bool (*XSFUNb_f3d1b1d2f1)  (float a1, float a2, float a3, long a4, bool a5, long a6, long a7, float a8);
typedef bool (*XSFUNb_f3d1b2)      (float a1, float a2, float a3, long a4, bool a5, bool a6);
typedef bool (*XSFUNb_f3d1b2f1)	  (float a1, float a2, float a3, long a4, bool a5, bool a6, float a7);
typedef bool (*XSFUNb_d2b2f1)		  (long a1, long a2, bool a3, bool a4, float a5);
typedef bool (*XSFUNb_d2f4)		  (long a1, long a2, float a3, float a4, float a5, float a6);
typedef bool (*XSFUNb_d3f4d1)		  (long a1, long a2, long a3, float a4, float a5, float a6, float a7, long a8);
typedef bool (*XSFUNb_f5b1)		  (float a1, float a2, float a3, float a4, float a5, bool a6);

typedef char* (*XSFUNs)(void);

typedef char* (*XSFUNs_d1) (long a1);
typedef char* (*XSFUNs_d2) (long a1, long a2);
typedef char* (*XSFUNs_d3) (long a1, long a2, long a3);
typedef char* (*XSFUNs_d4) (long a1, long a2, long a3, long a4);
typedef char* (*XSFUNs_d5) (long a1, long a2, long a3, long a4, long a5);
typedef char* (*XSFUNs_d6) (long a1, long a2, long a3, long a4, long a5, long a6);
typedef char* (*XSFUNs_d7) (long a1, long a2, long a3, long a4, long a5, long a6, long a7);
typedef char* (*XSFUNs_d8) (long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8);
typedef char* (*XSFUNs_d9) (long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9);
typedef char* (*XSFUNs_d10)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10);
typedef char* (*XSFUNs_d11)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11);
typedef char* (*XSFUNs_d12)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11, long a12);
typedef char* (*XSFUNs_d13)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11, long a12, long a13);
typedef char* (*XSFUNs_d14)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11, long a12, long a13, long a14);

typedef char* (*XSFUNs_f1) (float a1);
typedef char* (*XSFUNs_f2) (float a1, float a2);
typedef char* (*XSFUNs_f3) (float a1, float a2, float a3);
typedef char* (*XSFUNs_f4) (float a1, float a2, float a3, float a4);
typedef char* (*XSFUNs_f5) (float a1, float a2, float a3, float a4, float a5);
typedef char* (*XSFUNs_f6) (float a1, float a2, float a3, float a4, float a5, float a6);
typedef char* (*XSFUNs_f7) (float a1, float a2, float a3, float a4, float a5, float a6, float a7);
typedef char* (*XSFUNs_f8) (float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8);
typedef char* (*XSFUNs_f9) (float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9);
typedef char* (*XSFUNs_f10)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10);
typedef char* (*XSFUNs_f11)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11);
typedef char* (*XSFUNs_f12)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, float a12);
typedef char* (*XSFUNs_f13)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, float a12, float a13);
typedef char* (*XSFUNs_f14)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, float a12, float a13, float a14);

typedef char* (*XSFUNs_b1) (bool a1);
typedef char* (*XSFUNs_b2) (bool a1, bool a2);
typedef char* (*XSFUNs_b3) (bool a1, bool a2, bool a3);
typedef char* (*XSFUNs_b4) (bool a1, bool a2, bool a3, bool a4);
typedef char* (*XSFUNs_b5) (bool a1, bool a2, bool a3, bool a4, bool a5);
typedef char* (*XSFUNs_b6) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6);
typedef char* (*XSFUNs_b7) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7);
typedef char* (*XSFUNs_b8) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8);
typedef char* (*XSFUNs_b9) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9);
typedef char* (*XSFUNs_b10)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10);
typedef char* (*XSFUNs_b11)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11);
typedef char* (*XSFUNs_b12)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11, bool a12);
typedef char* (*XSFUNs_b13)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11, bool a12, bool a13);
typedef char* (*XSFUNs_b14)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11, bool a12, bool a13, bool a14);

typedef char* (*XSFUNs_b1d1)        (bool a1, long a2);
typedef char* (*XSFUNs_b1d2)        (bool a1, long a2, long a3);
typedef char* (*XSFUNs_b1d6)        (bool a1, long a2, long a3, long a4, long a5, long a6, long a7);
typedef char* (*XSFUNs_b1f1)        (bool a1, float a2);
typedef char* (*XSFUNs_b1f2)        (bool a1, float a2, float a3);
typedef char* (*XSFUNs_d1b1)        (long a1, bool a2);
typedef char* (*XSFUNs_d1b1d1)      (long a1, bool a2, long a3);
typedef char* (*XSFUNs_d1b2)        (long a1, bool a2, bool a3);
typedef char* (*XSFUNs_d1b2f4b2)    (long a1, bool a2, bool a3, float a4, float a5, float a6, float a7, bool a8, bool a9);
typedef char* (*XSFUNs_d1b3f1b1)    (long a1, bool a2, bool a3, bool a4, float a5, bool a6);
typedef char* (*XSFUNs_d1b4)        (long a1, bool a2, bool a3, bool a4, bool a5);
typedef char* (*XSFUNs_d1f1)        (long a1, float a2);
typedef char* (*XSFUNs_d1f1b1)      (long a1, float a2, bool a3);
typedef char* (*XSFUNs_d1f1b1d1)    (long a1, float a2, bool a3, long a4);
typedef char* (*XSFUNs_d1f1d1)      (long a1, float a2, long a3);
typedef char* (*XSFUNs_d1f1d1b1)    (long a1, float a2, long a3, bool a4);
typedef char* (*XSFUNs_d1f1d1f2)    (long a1, float a2, long a3, float a4, float a5);
typedef char* (*XSFUNs_d1f1d1f4)    (long a1, float a2, long a3, float a4, float a5, float a6, float a7);
typedef char* (*XSFUNs_d1f1d3)      (long a1, float a2, long a3, long a4, long a5);
typedef char* (*XSFUNs_d1f2)        (long a1, float a2, float a3);
typedef char* (*XSFUNs_d1f2b1)      (long a1, float a2, float a3, bool a4);
typedef char* (*XSFUNs_d1f2d1)      (long a1, float a2, float a3, long a4);
typedef char* (*XSFUNs_d1f2d1f1)    (long a1, float a2, float a3, long a4, float a5);
typedef char* (*XSFUNs_d1f3)        (long a1, float a2, float a3, float a4);
typedef char* (*XSFUNs_d1f3d1)      (long a1, float a2, float a3, float a4, long a5);
typedef char* (*XSFUNs_d1f4)        (long a1, float a2, float a3, float a4, float a5);
typedef char* (*XSFUNs_d1f4b1)      (long a1, float a2, float a3, float a4, float a5, bool a6);
typedef char* (*XSFUNs_d1f4d2b1)    (long a1, float a2, float a3, float a4, float a5, long a6, long a7, bool a8);
typedef char* (*XSFUNs_d1f5)        (long a1, float a2, float a3, float a4, float a5, float a6);
typedef char* (*XSFUNs_d1f5b1)      (long a1, float a2, float a3, float a4, float a5, float a6, bool a7);
typedef char* (*XSFUNs_d1f7)        (long a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8);
typedef char* (*XSFUNs_d2b1)        (long a1, long a2, bool a3);
typedef char* (*XSFUNs_d2b1d1)      (long a1, long a2, bool a3, long a4);
typedef char* (*XSFUNs_d2b1d1b1)    (long a1, long a2, bool a3, long a4, bool a5);
typedef char* (*XSFUNs_d2b1f1)      (long a1, long a2, bool a3, float a4);
typedef char* (*XSFUNs_d2b2)        (long a1, long a2, bool a3, bool a4);
typedef char* (*XSFUNs_d2f1)        (long a1, long a2, float a3);
typedef char* (*XSFUNs_d2f1b1)      (long a1, long a2, float a3, bool a4);
typedef char* (*XSFUNs_d2f1b2)      (long a1, long a2, float a3, bool a4, bool a5);
typedef char* (*XSFUNs_d2f1d1)      (long a1, long a2, float a3, long a4);
typedef char* (*XSFUNs_d2f1d1f1)    (long a1, long a2, float a3, long a4, float a5);
typedef char* (*XSFUNs_d2f2)        (long a1, long a2, float a3, float a4);
typedef char* (*XSFUNs_d2f2b1)      (long a1, long a2, float a3, float a4, bool a5);
typedef char* (*XSFUNs_d2f2d1)      (long a1, long a2, float a3, float a4, long a5);
typedef char* (*XSFUNs_d2f3)        (long a1, long a2, float a3, float a4, float a5);
typedef char* (*XSFUNs_d2f3d1)      (long a1, long a2, float a3, float a4, float a5, long a6);
typedef char* (*XSFUNs_d2f3d2)      (long a1, long a2, float a3, float a4, float a5, long a6, long a7);
typedef char* (*XSFUNs_d2f4d1)      (long a1, long a2, float a3, float a4, float a5, float a6, long a7);
typedef char* (*XSFUNs_d3b1)        (long a1, long a2, long a3, bool a4);
typedef char* (*XSFUNs_d3b1d2)      (long a1, long a2, long a3, bool a4, long a5, long a6);
typedef char* (*XSFUNs_d3b2f3)      (long a1, long a2, long a3, bool a4, bool a5, float a6, float a7, float a8);
typedef char* (*XSFUNs_d3b3)        (long a1, long a2, long a3, bool a4, bool a5, bool a6);
typedef char* (*XSFUNs_d3f1)        (long a1, long a2, long a3, float a4);
typedef char* (*XSFUNs_d3f1b1)      (long a1, long a2, long a3, float a4, bool a5);
typedef char* (*XSFUNs_d3f2b1)      (long a1, long a2, long a3, float a4, float a5, bool a6);
typedef char* (*XSFUNs_d3f3)        (long a1, long a2, long a3, float a4, float a5, float a6);
typedef char* (*XSFUNs_d3f3d1b1)    (long a1, long a2, long a3, float a4, float a5, float a6, long a7, bool a8);
typedef char* (*XSFUNs_d4b1)        (long a1, long a2, long a3, long a4, bool a5);
typedef char* (*XSFUNs_d4b1d3)      (long a1, long a2, long a3, long a4, bool a5, long a6, long a7, long a8);
typedef char* (*XSFUNs_d4f1d1)      (long a1, long a2, long a3, long a4, float a5, long a6);
typedef char* (*XSFUNs_d4f2)        (long a1, long a2, long a3, long a4, float a5, float a6);
typedef char* (*XSFUNs_d5b1)        (long a1, long a2, long a3, long a4, long a5, bool a6);
typedef char* (*XSFUNs_d5b1d1)      (long a1, long a2, long a3, long a4, long a5, bool a6, long a7);
typedef char* (*XSFUNs_d5f1)        (long a1, long a2, long a3, long a4, long a5, float a6);
typedef char* (*XSFUNs_f1b1)        (float a1, bool a2);
typedef char* (*XSFUNs_f1b1d4)      (float a1, bool a2, long a3, long a4, long a5, long a6);
typedef char* (*XSFUNs_f1d1)        (float a1, long a2);
typedef char* (*XSFUNs_f1d1f1)      (float a1, long a2, float a3);
typedef char* (*XSFUNs_f1d1f2)      (float a1, long a2, float a3, float a4);
typedef char* (*XSFUNs_f2b1)        (float a1, float a2, bool a3);
typedef char* (*XSFUNs_f2d1)        (float a1, float a2, long a3);
typedef char* (*XSFUNs_f2d2)        (float a1, float a2, long a3, long a4);
typedef char* (*XSFUNs_f3d1)        (float a1, float a2, float a3, long a4);
typedef char* (*XSFUNs_f3d1b1d2f1)  (float a1, float a2, float a3, long a4, bool a5, long a6, long a7, float a8);
typedef char* (*XSFUNs_f3d1b2)      (float a1, float a2, float a3, long a4, bool a5, bool a6);
typedef char* (*XSFUNs_f3d1b2f1)		(float a1, float a2, float a3, long a4, bool a5, bool a6, float a7);
typedef char* (*XSFUNs_d2b2f1)		(long a1, long a2, bool a3, bool a4, float a5);
typedef char* (*XSFUNs_d2f4)		  (long a1, long a2, float a3, float a4, float a5, float a6);
typedef char* (*XSFUNs_d3f4d1)		  (long a1, long a2, long a3, float a4, float a5, float a6, float a7, long a8);
typedef char* (*XSFUNs_f5b1)		  (float a1, float a2, float a3, float a4, float a5, bool a6);

typedef BVector (*XSFUNv)(void);

typedef BVector (*XSFUNv_d1) (long a1);
typedef BVector (*XSFUNv_d2) (long a1, long a2);
typedef BVector (*XSFUNv_d3) (long a1, long a2, long a3);
typedef BVector (*XSFUNv_d4) (long a1, long a2, long a3, long a4);
typedef BVector (*XSFUNv_d5) (long a1, long a2, long a3, long a4, long a5);
typedef BVector (*XSFUNv_d6) (long a1, long a2, long a3, long a4, long a5, long a6);
typedef BVector (*XSFUNv_d7) (long a1, long a2, long a3, long a4, long a5, long a6, long a7);
typedef BVector (*XSFUNv_d8) (long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8);
typedef BVector (*XSFUNv_d9) (long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9);
typedef BVector (*XSFUNv_d10)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10);
typedef BVector (*XSFUNv_d11)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11);
typedef BVector (*XSFUNv_d12)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11, long a12);
typedef BVector (*XSFUNv_d13)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11, long a12, long a13);
typedef BVector (*XSFUNv_d14)(long a1, long a2, long a3, long a4, long a5, long a6, long a7, long a8, long a9, long a10, long a11, long a12, long a13, long a14);

typedef BVector (*XSFUNv_f1) (float a1);
typedef BVector (*XSFUNv_f2) (float a1, float a2);
typedef BVector (*XSFUNv_f3) (float a1, float a2, float a3);
typedef BVector (*XSFUNv_f4) (float a1, float a2, float a3, float a4);
typedef BVector (*XSFUNv_f5) (float a1, float a2, float a3, float a4, float a5);
typedef BVector (*XSFUNv_f6) (float a1, float a2, float a3, float a4, float a5, float a6);
typedef BVector (*XSFUNv_f7) (float a1, float a2, float a3, float a4, float a5, float a6, float a7);
typedef BVector (*XSFUNv_f8) (float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8);
typedef BVector (*XSFUNv_f9) (float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9);
typedef BVector (*XSFUNv_f10)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10);
typedef BVector (*XSFUNv_f11)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11);
typedef BVector (*XSFUNv_f12)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, float a12);
typedef BVector (*XSFUNv_f13)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, float a12, float a13);
typedef BVector (*XSFUNv_f14)(float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, float a12, float a13, float a14);

typedef BVector (*XSFUNv_b1) (bool a1);
typedef BVector (*XSFUNv_b2) (bool a1, bool a2);
typedef BVector (*XSFUNv_b3) (bool a1, bool a2, bool a3);
typedef BVector (*XSFUNv_b4) (bool a1, bool a2, bool a3, bool a4);
typedef BVector (*XSFUNv_b5) (bool a1, bool a2, bool a3, bool a4, bool a5);
typedef BVector (*XSFUNv_b6) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6);
typedef BVector (*XSFUNv_b7) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7);
typedef BVector (*XSFUNv_b8) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8);
typedef BVector (*XSFUNv_b9) (bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9);
typedef BVector (*XSFUNv_b10)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10);
typedef BVector (*XSFUNv_b11)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11);
typedef BVector (*XSFUNv_b12)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11, bool a12);
typedef BVector (*XSFUNv_b13)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11, bool a12, bool a13);
typedef BVector (*XSFUNv_b14)(bool a1, bool a2, bool a3, bool a4, bool a5, bool a6, bool a7, bool a8, bool a9, bool a10, bool a11, bool a12, bool a13, bool a14);

typedef BVector (*XSFUNv_b1d1)        (bool a1, long a2);
typedef BVector (*XSFUNv_b1d2)        (bool a1, long a2, long a3);
typedef BVector (*XSFUNv_b1d6)        (bool a1, long a2, long a3, long a4, long a5, long a6, long a7);
typedef BVector (*XSFUNv_b1f1)        (bool a1, float a2);
typedef BVector (*XSFUNv_b1f2)        (bool a1, float a2, float a3);
typedef BVector (*XSFUNv_d1b1)        (long a1, bool a2);
typedef BVector (*XSFUNv_d1b1d1)      (long a1, bool a2, long a3);
typedef BVector (*XSFUNv_d1b2)        (long a1, bool a2, bool a3);
typedef BVector (*XSFUNv_d1b2f4b2)    (long a1, bool a2, bool a3, float a4, float a5, float a6, float a7, bool a8, bool a9);
typedef BVector (*XSFUNv_d1b3f1b1)    (long a1, bool a2, bool a3, bool a4, float a5, bool a6);
typedef BVector (*XSFUNv_d1b4)        (long a1, bool a2, bool a3, bool a4, bool a5);
typedef BVector (*XSFUNv_d1f1)        (long a1, float a2);
typedef BVector (*XSFUNv_d1f1b1)      (long a1, float a2, bool a3);
typedef BVector (*XSFUNv_d1f1b1d1)    (long a1, float a2, bool a3, long a4);
typedef BVector (*XSFUNv_d1f1d1)      (long a1, float a2, long a3);
typedef BVector (*XSFUNv_d1f1d1b1)    (long a1, float a2, long a3, bool a4);
typedef BVector (*XSFUNv_d1f1d1f2)    (long a1, float a2, long a3, float a4, float a5);
typedef BVector (*XSFUNv_d1f1d1f4)    (long a1, float a2, long a3, float a4, float a5, float a6, float a7);
typedef BVector (*XSFUNv_d1f1d3)      (long a1, float a2, long a3, long a4, long a5);
typedef BVector (*XSFUNv_d1f2)        (long a1, float a2, float a3);
typedef BVector (*XSFUNv_d1f2b1)      (long a1, float a2, float a3, bool a4);
typedef BVector (*XSFUNv_d1f2d1)      (long a1, float a2, float a3, long a4);
typedef BVector (*XSFUNv_d1f2d1f1)    (long a1, float a2, float a3, long a4, float a5);
typedef BVector (*XSFUNv_d1f3)        (long a1, float a2, float a3, float a4);
typedef BVector (*XSFUNv_d1f3d1)      (long a1, float a2, float a3, float a4, long a5);
typedef BVector (*XSFUNv_d1f4)        (long a1, float a2, float a3, float a4, float a5);
typedef BVector (*XSFUNv_d1f4b1)      (long a1, float a2, float a3, float a4, float a5, bool a6);
typedef BVector (*XSFUNv_d1f4d2b1)    (long a1, float a2, float a3, float a4, float a5, long a6, long a7, bool a8);
typedef BVector (*XSFUNv_d1f5)        (long a1, float a2, float a3, float a4, float a5, float a6);
typedef BVector (*XSFUNv_d1f5b1)      (long a1, float a2, float a3, float a4, float a5, float a6, bool a7);
typedef BVector (*XSFUNv_d1f7)        (long a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8);
typedef BVector (*XSFUNv_d2b1)        (long a1, long a2, bool a3);
typedef BVector (*XSFUNv_d2b1d1)      (long a1, long a2, bool a3, long a4);
typedef BVector (*XSFUNv_d2b1d1b1)    (long a1, long a2, bool a3, long a4, bool a5);
typedef BVector (*XSFUNv_d2b1f1)      (long a1, long a2, bool a3, float a4);
typedef BVector (*XSFUNv_d2b2)        (long a1, long a2, bool a3, bool a4);
typedef BVector (*XSFUNv_d2f1)        (long a1, long a2, float a3);
typedef BVector (*XSFUNv_d2f1b1)      (long a1, long a2, float a3, bool a4);
typedef BVector (*XSFUNv_d2f1b2)      (long a1, long a2, float a3, bool a4, bool a5);
typedef BVector (*XSFUNv_d2f1d1)      (long a1, long a2, float a3, long a4);
typedef BVector (*XSFUNv_d2f1d1f1)    (long a1, long a2, float a3, long a4, float a5);
typedef BVector (*XSFUNv_d2f2)        (long a1, long a2, float a3, float a4);
typedef BVector (*XSFUNv_d2f2b1)      (long a1, long a2, float a3, float a4, bool a5);
typedef BVector (*XSFUNv_d2f2d1)      (long a1, long a2, float a3, float a4, long a5);
typedef BVector (*XSFUNv_d2f3)        (long a1, long a2, float a3, float a4, float a5);
typedef BVector (*XSFUNv_d2f3d1)      (long a1, long a2, float a3, float a4, float a5, long a6);
typedef BVector (*XSFUNv_d2f3d2)      (long a1, long a2, float a3, float a4, float a5, long a6, long a7);
typedef BVector (*XSFUNv_d2f4d1)      (long a1, long a2, float a3, float a4, float a5, float a6, long a7);
typedef BVector (*XSFUNv_d3b1)        (long a1, long a2, long a3, bool a4);
typedef BVector (*XSFUNv_d3b1d2)      (long a1, long a2, long a3, bool a4, long a5, long a6);
typedef BVector (*XSFUNv_d3b2f3)      (long a1, long a2, long a3, bool a4, bool a5, float a6, float a7, float a8);
typedef BVector (*XSFUNv_d3b3)        (long a1, long a2, long a3, bool a4, bool a5, bool a6);
typedef BVector (*XSFUNv_d3f1)        (long a1, long a2, long a3, float a4);
typedef BVector (*XSFUNv_d3f1b1)      (long a1, long a2, long a3, float a4, bool a5);
typedef BVector (*XSFUNv_d3f2b1)      (long a1, long a2, long a3, float a4, float a5, bool a6);
typedef BVector (*XSFUNv_d3f3)        (long a1, long a2, long a3, float a4, float a5, float a6);
typedef BVector (*XSFUNv_d3f3d1b1)    (long a1, long a2, long a3, float a4, float a5, float a6, long a7, bool a8);
typedef BVector (*XSFUNv_d4b1)        (long a1, long a2, long a3, long a4, bool a5);
typedef BVector (*XSFUNv_d4b1d3)      (long a1, long a2, long a3, long a4, bool a5, long a6, long a7, long a8);
typedef BVector (*XSFUNv_d4f1d1)      (long a1, long a2, long a3, long a4, float a5, long a6);
typedef BVector (*XSFUNv_d4f2)        (long a1, long a2, long a3, long a4, float a5, float a6);
typedef BVector (*XSFUNv_d5b1)        (long a1, long a2, long a3, long a4, long a5, bool a6);
typedef BVector (*XSFUNv_d5b1d1)      (long a1, long a2, long a3, long a4, long a5, bool a6, long a7);
typedef BVector (*XSFUNv_d5f1)        (long a1, long a2, long a3, long a4, long a5, float a6);
typedef BVector (*XSFUNv_f1b1)        (float a1, bool a2);
typedef BVector (*XSFUNv_f1b1d4)      (float a1, bool a2, long a3, long a4, long a5, long a6);
typedef BVector (*XSFUNv_f1d1)        (float a1, long a2);
typedef BVector (*XSFUNv_f1d1f1)      (float a1, long a2, float a3);
typedef BVector (*XSFUNv_f1d1f2)      (float a1, long a2, float a3, float a4);
typedef BVector (*XSFUNv_f2b1)        (float a1, float a2, bool a3);
typedef BVector (*XSFUNv_f2d1)        (float a1, float a2, long a3);
typedef BVector (*XSFUNv_f2d2)        (float a1, float a2, long a3, long a4);
typedef BVector (*XSFUNv_f3d1)        (float a1, float a2, float a3, long a4);
typedef BVector (*XSFUNv_f3d1b1d2f1)  (float a1, float a2, float a3, long a4, bool a5, long a6, long a7, float a8);
typedef BVector (*XSFUNv_f3d1b2)      (float a1, float a2, float a3, long a4, bool a5, bool a6);
typedef BVector (*XSFUNv_f3d1b2f1)	  (float a1, float a2, float a3, long a4, bool a5, bool a6, float a7);
typedef BVector (*XSFUNv_d2b2f1)		  (long a1, long a2, bool a3, bool a4, float a5);
typedef BVector (*XSFUNv_d2f4)		  (long a1, long a2, float a3, float a4, float a5, float a6);
typedef BVector (*XSFUNv_d3f4d1)		  (long a1, long a2, long a3, float a4, float a5, float a6, float a7, long a8);
typedef BVector (*XSFUNv_f5b1)		  (float a1, float a2, float a3, float a4, float a5, bool a6);

#endif

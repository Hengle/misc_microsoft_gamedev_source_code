/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis 'TREMOR' CODEC SOURCE CODE.   *
 *                                                                  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis 'TREMOR' SOURCE CODE IS (C) COPYRIGHT 1994-2003    *
 * BY THE Xiph.Org FOUNDATION http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: normalized modified discrete cosine transform
           power of two length transform only [64 <= n ]
 last mod: $Id: mdct.c,v 1.9.6.5 2003/04/29 04:03:27 xiphmont Exp $

 Original algorithm adapted long ago from _The use of multirate filter
 banks for coding of high quality digital audio_, by T. Sporer,
 K. Brandenburg and B. Edler, collection of the European Signal
 Processing Conference (EUSIPCO), Amsterdam, June 1992, Vol.1, pp
 211-214

 The below code implements an algorithm that no longer looks much like
 that presented in the paper, but the basic structure remains if you
 dig deep enough to see it.

 This module DOES NOT INCLUDE code to generate/apply the window
 function.  Everybody has their own weird favorite including me... I
 happen to like the properties of y=sin(.5PI*sin^2(x)), but others may
 vehemently disagree.

 ********************************************************************/

#include "vorbis/ivorbiscodec.h"
#include "os.h"
#include "misc.h"
#include "mdct.h"
#include "mdct_lookup.h"

#include "ppcintrinsics.h"
	
// __vperm( vA, vB, Shuffler);
// this is ugly, but the X360 compiler does not allow any other way to initialise vectors with integers
#define	vA0	9.2557e-041		// 0x00010203
#define	vA1	1.56368425e-036	// 0x04050607
#define	vA2	4.12387433e-034	// 0x08090A0B
#define	vA3	1.08664755e-031	// 0x0C0D0E0F
#define	vB0	2.86101317e-029	// 0x10111213
#define	vB1	7.52693405e-027	// 0x14151617
#define	vB2	1.97879664e-024	// 0x18191A1B
#define	vB3	5.19858731e-022	// 0x1C1D1E1F

static const __vector4 vfsign1		= {  1.0f,  1.0f, -1.0f, -1.0f };
static const __vector4 vfsign2		= { -1.0f, -1.0f,  1.0f,  1.0f };
static const __vector4 vfsign3		= { -1.0f,  1.0f,  1.0f, -1.0f };
static const __vector4 vfsign4		= {  1.0f, -1.0f, -1.0f,  1.0f };
static const __vector4 vfsign5		= {  1.0f, -1.0f,  1.0f, -1.0f };
static const __vector4 vfsign6		= { -1.0f,  1.0f, -1.0f,  1.0f };
static const __vector4 vfPointFive	= {  0.5f,  0.5f,  0.5f,  0.5f };

// #define cPI3_8 (0x30fbc54d) -> 0.38268343237353648635257803199467 = cos(3 * pi / 8)
#define CosThreePiOverEight	0.38268343237353648635257803199467f

// #define cPI2_8 (0x5a82799a) -> 0.70710678152139614407038136574923 = cos(2 * pi / 8)
#define CosPiOverFour	0.70710678152139614407038136574923f

// #define cPI1_8 (0x7641af3d) -> 0.92387953303934984516322139891014 = cos(pi / 8)
#define CosPiOverEight	0.92387953303934984516322139891014f

static __vector4	vfcPI2_8	= { CosPiOverFour,CosPiOverFour,CosPiOverFour,CosPiOverFour };
static __vector4	vfcPI_3131	= { CosThreePiOverEight, CosPiOverEight, CosThreePiOverEight, CosPiOverEight };
static __vector4	vfcPI_1313	= { CosPiOverEight, CosThreePiOverEight, CosPiOverEight, CosThreePiOverEight };

//	__vperm([a0,a1,a2,a3], [b0,b1,b2,b3], S_b1b3a0a2) = [ b1, b3, a0, a2]

static const __vector4 S_Odd		= { vA1, vA3, vB1, vB3 };
static const __vector4 S_Even		= { vA0, vA2, vB0, vB2 };
static const __vector4 S_a2a3a2a3	= { vA2, vA3, vA2, vA3 };
static const __vector4 S_a0a1a0a1	= { vA0, vA1, vA0, vA1 };

static const __vector4 S_b0b2a1a3	= { vB0, vB2, vA1, vA3 };
static const __vector4 S_a0a3b0b2	= { vA0, vA3, vB0, vB2 };
static const __vector4 S_a1a2b1b3	= { vA1, vA2, vB1, vB3 };
static const __vector4 S_a2a1a3a0	= { vA2, vA1, vA3, vA0 };
static const __vector4 S_b1b3a0a2	= { vB1, vB3, vA0, vA2 };
static const __vector4 S_b0a1b2a3	= { vB0, vA1, vB2, vA3 };
static const __vector4 S_b1a0b3a2	= { vB1, vA0, vB3, vA2 };
static const __vector4 S_b0b2a0a3	= { vB0, vB2, vA0, vA3 };
static const __vector4 S_b1b3a1a2	= { vB1, vB3, vA1, vA2 };
static const __vector4 S_a0a3a1a2	= { vA0, vA3, vA1, vA2 };
static const __vector4 S_a0a3b1b3	= { vA0, vA3, vB1, vB3 };
static const __vector4 S_a1a2b0b2	= { vA1, vA2, vB0, vB2 };
static const __vector4 S_a0b1a2b3	= { vA0, vB1, vA2, vB3 };
static const __vector4 S_a1b0a3b2	= { vA1, vB0, vA3, vB2 };
static const __vector4 S_a0a0a0a0	= { vA0, vA0, vA0, vA0 };
static const __vector4 S_a1a1a1a1	= { vA1, vA1, vA1, vA1 };
static const __vector4 S_a1a2a0a3	= { vA1, vA2, vA0, vA3 };
static const __vector4 S_a0a2b0b3	= { vA0, vA2, vB0, vB3 };
static const __vector4 S_a1a3b1b2	= { vA1, vA3, vB1, vB2 };
static const __vector4 S_a0a3a0a3	= { vA0, vA3, vA0, vA3 };
static const __vector4 S_a1a2a1a2	= { vA1, vA2, vA1, vA2 };
static const __vector4 S_a0a2a0a3	= { vA0, vA2, vA0, vA3 };
static const __vector4 S_a1a3a1a2	= { vA1, vA3, vA1, vA2 };
static const __vector4 S_a0a2a1a3	= { vA0, vA2, vA1, vA3 };
static const __vector4 S_a1a3a0a2	= { vA1, vA3, vA0, vA2 };

static const __vector4 S_a0a0b0b0	= { vA0, vA0, vB0, vB0 };
static const __vector4 S_a1a1b1b1	= { vA1, vA1, vB1, vB1 };
static const __vector4 S_a2a2b2b2	= { vA2, vA2, vB2, vB2 };
static const __vector4 S_a3a3b3b3	= { vA3, vA3, vB3, vB3 };
static const __vector4 S_a1a0a3a2	= { vA1, vA0, vA3, vA2 };
static const __vector4 S_a2a3b0b1	= { vA2, vA3, vB0, vB1 };
static const __vector4 S_a3a2a1a0	= { vA3, vA2, vA1, vA0 };
static const __vector4 S_a0b3a2b1	= { vA0, vB3, vA2, vB1 };
static const __vector4 S_b2a1b0a3	= { vB2, vA1, vB0, vA3 };
static const __vector4 S_b3a0b1a2	= { vB3, vA0, vB1, vA2 };
static const __vector4 S_a1b2a3b0	= { vA1, vB2, vA3, vB0 };
static const __vector4 S_b1b1a1a1	= { vB1, vB1, vA1, vA1 };
static const __vector4 S_b0b0a0a0	= { vB0, vB0, vA0, vA0 };
static const __vector4 S_a2b1a0b3	= { vA2, vB1, vA0, vB3 };
static const __vector4 S_b0a3b2a1	= { vB0, vA3, vB2, vA1 };
static const __vector4 S_b1a2b3a0	= { vB1, vA2, vB3, vA0 };
static const __vector4 S_a3b0a1b2	= { vA3, vB0, vA1, vB2 };
static const __vector4 S_a1a0a1a0	= { vA1, vA0, vA1, vA0 };
//====================================================================================================
//====================================================================================================
// Now takes FLOAT as input, but leaves DATA_TYPE in the buffer
STIN void presymmetry(DATA_TYPE *in,int n2,int step)
{
	DATA_TYPE	*aX;
	DATA_TYPE	*bX;
	float		*T;
	int n4=n2>>1;

	aX	= in+n2-3;
	T	= fsincos_lookup0;

	do
	{
		float	fr0= ((float *)aX)[0];
		float	fr2= ((float *)aX)[2];
		fXPROD31( fr0, fr2, T[0], T[1], &aX[0], &aX[2] );

		T+=step;
		aX-=4;
	}
	while(aX>=in+n4);

	do
	{
		float	fr0= ((float *)aX)[0];
		float	fr2= ((float *)aX)[2];
		fXPROD31( fr0, fr2, T[1], T[0], &aX[0], &aX[2] );

		T-=step;
		aX-=4;
	}
	while(aX>=in);

	aX            = in+n2-4;
	bX            = in;
	T             = fsincos_lookup0;
	do
	{
		float	fri0= ((float *)aX)[0];
		float	fri2= ((float *)aX)[2];
		float	fro0= ((float *)bX)[0];
		float	fro2= ((float *)bX)[2];

		fXNPROD31( fro2, fro0, T[1], T[0], &aX[0], &aX[2] );
		T+=step;
		fXNPROD31( fri2, fri0, T[0], T[1], &bX[0], &bX[2] );

		aX-=4;
		bX+=4;
	}
	while(aX>=in+n4);
}
//====================================================================================================
//	mdct_butterfly_8(x);
//	mdct_butterfly_8(x+8);
//	mdct_butterfly_8(x+16);
//	mdct_butterfly_8(x+16+8);
//====================================================================================================
STIN void mdct_butterfly_8_ter(DATA_TYPE *x)
{
	AKASSERT(((AkUInt32)x & 0x0F) == 0);

	__vector4* vpx = (__vector4*)x;

	__vector4 vfA		= __vcfsx(vpx[0],0);	// x[0],x[1],x[2],x[3]
	__vector4 vfB		= __vcfsx(vpx[1],0);	// x[4],x[5],x[6],x[7]
	__vector4 vfA8		= __vcfsx(vpx[2],0);
	__vector4 vfB8		= __vcfsx(vpx[3],0);
	__vector4 vfA16		= __vcfsx(vpx[4],0);
	__vector4 vfB16		= __vcfsx(vpx[5],0);
	__vector4 vfA168	= __vcfsx(vpx[6],0);
	__vector4 vfB168	= __vcfsx(vpx[7],0);
//--------------------------------------------------------------------------------
//	REG_TYPE r0   = x[0] + x[1];
//	REG_TYPE r1   = x[0] - x[1];
//	REG_TYPE r2   = x[2] + x[3];
//	REG_TYPE r3   = x[2] - x[3];
//	REG_TYPE r4   = x[4] + x[5];
//	REG_TYPE r5   = x[4] - x[5];
//	REG_TYPE r6   = x[6] + x[7];
//	REG_TYPE r7   = x[6] - x[7];
//--------------------------------------------------------------------------------
	__vector4 vfOdd			= __vperm( vfA, vfB, S_Odd );
	__vector4 vfEven		= __vperm( vfA, vfB, S_Even );
	__vector4 vfOdd8		= __vperm( vfA8, vfB8, S_Odd );
	__vector4 vfEven8		= __vperm( vfA8, vfB8, S_Even );
	__vector4 vfOdd16		= __vperm( vfA16, vfB16, S_Odd );
	__vector4 vfEven16		= __vperm( vfA16, vfB16, S_Even );
	__vector4 vfOdd168		= __vperm( vfA168, vfB168, S_Odd );
	__vector4 vfEven168		= __vperm( vfA168, vfB168, S_Even );

	__vector4 vfREven		= __vaddfp( vfEven, vfOdd );		// vREven = [ r0, r2, r4, r6 ] = [ a, b, c, d ]
	__vector4 vfROdd		= __vsubfp( vfEven, vfOdd );		// vROdd = [ r1, r3, r5, r7 ] = [ e, f, g, h ]
	__vector4 vfREven8		= __vaddfp( vfEven8, vfOdd8 );
	__vector4 vfROdd8		= __vsubfp( vfEven8, vfOdd8 );
	__vector4 vfREven16		= __vaddfp( vfEven16, vfOdd16 );
	__vector4 vfROdd16		= __vsubfp( vfEven16, vfOdd16 );
	__vector4 vfREven168	= __vaddfp( vfEven168, vfOdd168 );
	__vector4 vfROdd168		= __vsubfp( vfEven168, vfOdd168 );
//--------------------------------------------------------------------------------
//	x[0] = r5   + r3;
//	x[1] = r7   - r1;
//	x[2] = r5   - r3;
//	x[3] = r7   + r1;
//	x[4] = r4   - r0;
//	x[5] = r6   - r2;
//	x[6] = r4   + r0;
//	x[7] = r6   + r2;
//--------------------------------------------------------------------------------
	__vector4 vfghgh	= __vperm(vfROdd, vfROdd, S_a2a3a2a3 );	// vghgh = [ g, h, g, h ]
	__vector4 vfghgh8	= __vperm(vfROdd8, vfROdd8, S_a2a3a2a3 );
	__vector4 vfghgh16	= __vperm(vfROdd16, vfROdd16, S_a2a3a2a3 );
	__vector4 vfghgh168	= __vperm(vfROdd168, vfROdd168, S_a2a3a2a3 );

	__vector4 vfefe		= __vperm(vfROdd, vfROdd, S_a1a0a1a0 );
	vfefe				= __vmulfp(vfefe,vfsign4);
	__vector4 vfefe8	= __vperm(vfROdd8, vfROdd8, S_a1a0a1a0 );
	vfefe8				= __vmulfp(vfefe8,vfsign4);
	__vector4 vfefe16	= __vperm(vfROdd16, vfROdd16, S_a1a0a1a0 );
	vfefe16				= __vmulfp(vfefe16,vfsign4);
	__vector4 vfefe168	= __vperm(vfROdd168, vfROdd168, S_a1a0a1a0 );
	vfefe168			= __vmulfp(vfefe168,vfsign4);

	vfA					= __vaddfp( vfghgh, vfefe );
	vfA8				= __vaddfp( vfghgh8, vfefe8 );
	vfA16				= __vaddfp( vfghgh16, vfefe16 );
	vfA168				= __vaddfp( vfghgh168, vfefe168 );

	__vector4 vfcdcd	= __vperm(vfREven, vfROdd, S_a2a3a2a3 );
	__vector4 vfcdcd8	= __vperm(vfREven8, vfROdd8, S_a2a3a2a3 );
	__vector4 vfcdcd16	= __vperm(vfREven16, vfROdd16, S_a2a3a2a3 );
	__vector4 vfcdcd168	= __vperm(vfREven168, vfROdd168, S_a2a3a2a3 );

	__vector4 vfabab	= __vperm(vfREven, vfREven, S_a0a1a0a1 );
	vfabab				= __vmulfp(vfabab,vfsign2);
	__vector4 vfabab8	= __vperm(vfREven8, vfREven8, S_a0a1a0a1 );
	vfabab8				= __vmulfp(vfabab8,vfsign2);
	__vector4 vfabab16	= __vperm(vfREven16, vfREven16, S_a0a1a0a1 );
	vfabab16			= __vmulfp(vfabab16,vfsign2);
	__vector4 vfabab168	= __vperm(vfREven168, vfREven168, S_a0a1a0a1 );
	vfabab168			= __vmulfp(vfabab168,vfsign2);

	vfB					= __vaddfp( vfcdcd, vfabab );				// vB = [ c, d, c+a, d+b ]
	vfB8				= __vaddfp( vfcdcd8, vfabab8 );
	vfB16				= __vaddfp( vfcdcd16, vfabab16 );
	vfB168				= __vaddfp( vfcdcd168, vfabab168 );

	vpx[0]				= __vctsxs(vfA,0);
	vpx[1]				= __vctsxs(vfB,0);
	vpx[2]				= __vctsxs(vfA8,0);
	vpx[3]				= __vctsxs(vfB8,0);
	vpx[4]				= __vctsxs(vfA16,0);
	vpx[5]				= __vctsxs(vfB16,0);
	vpx[6]				= __vctsxs(vfA168,0);
	vpx[7]				= __vctsxs(vfB168,0);
}
//====================================================================================================
//	mdct_butterfly_16(x);
//	mdct_butterfly_16(x+16);
//====================================================================================================
STIN void mdct_butterfly_16_bis(DATA_TYPE *x)
{
	AKASSERT(((AkUInt32)x & 0x0F) == 0);

	__vector4* vpx		= (__vector4*)x;

	__vector4* vpx0		= &vpx[0];
	__vector4* vpx4		= &vpx[1];
	__vector4* vpx8		= &vpx[2];
	__vector4* vpx12	= &vpx[3];

	__vector4 vx0		= vpx[0];
	__vector4 vx4		= vpx[1];
	__vector4 vx8		= vpx[2];
	__vector4 vx12		= vpx[3];

	__vector4* vpx16	= &vpx[4];
	__vector4* vpx20	= &vpx[5];
	__vector4* vpx24	= &vpx[6];
	__vector4* vpx28	= &vpx[7];

	__vector4 vx16		= vpx[4];
	__vector4 vx20		= vpx[5];
	__vector4 vx24		= vpx[6];
	__vector4 vx28		= vpx[7];

	__vector4	vtemp, vtemp2,vtemp16, vtemp216;
	__vector4	vr,vr16;
//--------------------------------------------------------------------------------
//	r[0]		= x[ 8] - x[ 9];
//	r[1]		= x[10] - x[11];
//	r[2]		= x[ 1] - x[ 0];
//	r[3]		= x[ 3] - x[ 2];
//--------------------------------------------------------------------------------
	vr			= __vperm(vx0, vx8, S_b0b2a1a3);	// vr = [ x8, x10, x1, x3 ]
	vtemp		= __vperm(vx0, vx8, S_b1b3a0a2);	// vtemp = [ x9, x11, x0, x2 ]
	vr			= __vsubsws(vr, vtemp);				// vr = [ x8-x9, x10-x11, x1-x0, x3-x2 ]
	vr16		= __vperm(vx16, vx24, S_b0b2a1a3);
	vtemp16		= __vperm(vx16, vx24, S_b1b3a0a2);
	vr16		= __vsubsws(vr16, vtemp16);
//--------------------------------------------------------------------------------
//	x[ 8]		= x[ 8] + x[ 9];
//	x[ 9]		= x[ 1] + x[ 0];
//	x[10]		= x[10] + x[11];
//	x[11]		= x[ 3] + x[ 2];
//--------------------------------------------------------------------------------
	vtemp2		= __vperm(vx0, vx8, S_b0a1b2a3);	// vtemp2 = [ x8, x1, x10, x3 ]
	vtemp		= __vperm(vx0, vx8, S_b1a0b3a2);	// vtemp = [ x9, x0, x11, x2 ]
	*vpx8		= __vaddsws(vtemp, vtemp2);			// vx8 = [ x8+x9, x1+x0, x10+x11, x3+x2 ]
	vtemp216	= __vperm(vx16, vx24, S_b0a1b2a3);
	vtemp16		= __vperm(vx16, vx24, S_b1a0b3a2);
	*vpx24		= __vaddsws(vtemp16, vtemp216);
//--------------------------------------------------------------------------------
//	x[ 0]		= MULT31((r[0] - r[1]) , cPI2_8);
//	x[ 1]		= MULT31((r[2] + r[3]) , cPI2_8);
//	x[ 2]		= MULT31((r[0] + r[1]) , cPI2_8);
//	x[ 3]		= MULT31((r[3] - r[2]) , cPI2_8);
//--------------------------------------------------------------------------------
	__vector4	vfr,vftemp,vftemp2;
	__vector4	vfr16,vftemp16,vftemp162;

	vfr			= __vcfsx(vr,0);
	vfr16		= __vcfsx(vr16,0);

	vftemp		= __vperm(vfr,vfr,S_a0a2a0a3);
	vftemp2		= __vperm(vfr,vfr,S_a1a3a1a2);
	vftemp16	= __vperm(vfr16,vfr16,S_a0a2a0a3);
	vftemp162	= __vperm(vfr16,vfr16,S_a1a3a1a2);

	vftemp		= __vmaddfp(vftemp2,vfsign3,vftemp);
	vftemp		= __vmulfp(vftemp, vfcPI2_8);
	vftemp16	= __vmaddfp(vftemp162,vfsign3,vftemp16);
	vftemp16	= __vmulfp(vftemp16, vfcPI2_8);

	*vpx0		= __vctsxs(vftemp,0);
	*vpx16		= __vctsxs(vftemp16,0);

	MB();
//--------------------------------------------------------------------------------
//	r[0]		= x[ 4] - x[ 5];
//	r[1]		= x[ 7] - x[ 6];
//	r[2]		= x[12] - x[13];
//	r[3]		= x[14] - x[15];
//--------------------------------------------------------------------------------
	vtemp		= __vperm(vx4,vx12,S_a0a3b0b2);		// vtemp = [ x4, x7, x12, x14 ]
	vtemp2		= __vperm(vx4,vx12,S_a1a2b1b3);		// vtemp2 = [ x5, x6, x13, x15 ]
	vr			= __vsubsws(vtemp, vtemp2);
	vtemp16		= __vperm(vx20,vx28,S_a0a3b0b2);
	vtemp216	= __vperm(vx20,vx28,S_a1a2b1b3);
	vr16		= __vsubsws(vtemp16, vtemp216);
//--------------------------------------------------------------------------------
//	x[12]		= x[12] + x[13];
//	x[13]		= x[ 5] + x[ 4];
//	x[14]		= x[14] + x[15];
//	x[15]		= x[ 7] + x[ 6];
//--------------------------------------------------------------------------------
	vtemp		= __vperm(vx4,vx12,S_b0a1b2a3);		// vtemp = [ x12, x5, x14, x7 ]
	vtemp2		= __vperm(vx4,vx12,S_b1a0b3a2);		// vtemp2 = [ x13, x4, x15, x6 ]
	*vpx12		= __vaddsws(vtemp, vtemp2);
	vtemp16		= __vperm(vx20,vx28,S_b0a1b2a3);
	vtemp216	= __vperm(vx20,vx28,S_b1a0b3a2);
	*vpx28		= __vaddsws(vtemp16, vtemp216);
//--------------------------------------------------------------------------------
//	x[ 4]		= r[2];
//	x[ 5]		= r[1]; 
//	x[ 6]		= r[3];
//	x[ 7]		= r[0];
//--------------------------------------------------------------------------------
	*vpx4		= __vperm(vr,vr,S_a2a1a3a0);
	*vpx20		= __vperm(vr16,vr16,S_a2a1a3a0);

	mdct_butterfly_8_ter(x);
}
//====================================================================================================
// 32 point butterfly (in place, 4 register)
//====================================================================================================
STIN void mdct_butterfly_32(DATA_TYPE *x)
{
	AKASSERT(((AkUInt32)x & 0x0F) == 0);

	__vector4* vpx = (__vector4*)x;
	__vector4* vpxbis = (__vector4*)(x+16);

	__vector4* vpx0		= &vpx[0];
	__vector4* vpx4		= &vpx[1];
	__vector4* vpx8		= &vpx[2];
	__vector4* vpx12	= &vpx[3];
	__vector4* vpx16	= &vpxbis[0];
	__vector4* vpx20	= &vpxbis[1];
	__vector4* vpx24	= &vpxbis[2];
	__vector4* vpx28	= &vpxbis[3];

	__vector4 vx0	= *vpx0;
	__vector4 vx4	= *vpx4;
	__vector4 vx8	= *vpx8;
	__vector4 vx12	= *vpx12;
	__vector4 vx16	= *vpx16;
	__vector4 vx20	= *vpx20;
	__vector4 vx24	= *vpx24;
	__vector4 vx28	= *vpx28;

	__vector4	vr;
	__vector4	vtemp, vtemp2;
	__vector4	vtemp3, vtemp4;
	__vector4	vfr,vftemp,vftemp2;

//--------------------------------------------------------------------------------
//	r0		= x[16] - x[17];
//	r1		= x[18] - x[19];
//	r2		= x[ 1] - x[ 0];
//	r3		= x[ 3] - x[ 2];
//	x[16]	= x[16] + x[17];
//	x[17]	= x[ 1] + x[ 0];
//	x[18]	= x[18] + x[19];
//	x[19]	= x[ 3] + x[ 2];
//--------------------------------------------------------------------------------
	vtemp		= __vperm(vx0,vx16,S_b0b2a1a3);			// vtemp = [ x16, x18, x1, x3 ]
	vtemp2		= __vperm(vx0,vx16,S_b1b3a0a2);			// vtemp2 = [ x17, x19, x0, x2 ]
	vtemp3		= __vperm(vx0,vx16,S_b0a1b2a3);			// vtemp = [ x16, x1, x18, x3 ]
	vtemp4		= __vperm(vx0,vx16,S_b1a0b3a2);			// vtemp2 = [ x17, x0, x19, x2 ]
	vr			= __vsubsws(vtemp, vtemp2);				// vr = [ x16-x17, x18-x19, x1-x0, x3-x2 ]
	vfr			= __vcfsx(vr,0);
//--------------------------------------------------------------------------------
//	XNPROD31( r0, r1, cPI3_8, cPI1_8, &x[ 0], &x[ 2] );
//	XPROD31 ( r2, r3, cPI1_8, cPI3_8, &x[ 1], &x[ 3] );
//--------------------------------------------------------------------------------
	vftemp		= __vperm(vfr,vfr,S_a0a2a1a3);			// vtemp = [ v0, v2, v1, v3 ]
	vftemp2		= __vperm(vfr,vfr,S_a1a3a0a2);			// vtemp2 = [ v1, v3, v0, v2 ]

	vftemp		= __vmulfp(vftemp, vfcPI_3131);			// vtemp2 = [ v0*c38, v2*c18, v1*c38, v3*c18 ]
	vftemp2		= __vmulfp(vftemp2, vfcPI_1313);		// vtemp2 = [ v1*c18, v3*c38, v0*c18, v2*c38 ]
	vftemp		= __vmaddfp(vftemp2,vfsign3,vftemp);	// vtemp += [ -(v1*c18), v3*c38, v0*c18, -(v2*c38) ]

	*vpx16		= __vaddsws(vtemp3, vtemp4);			// vx16 = [ x16+x17, x1+x0, x18+x19, x3+x2] 
	*vpx0		= __vctsxs(vftemp,0);

	MB();
//--------------------------------------------------------------------------------
//	r0		= x[20] - x[21];
//	r1		= x[22] - x[23];
//	r2		= x[ 5] - x[ 4];
//	r3		= x[ 7] - x[ 6];
//	x[20]	= x[20] + x[21];
//	x[21]	= x[ 5] + x[ 4];
//	x[22]	= x[22] + x[23];
//	x[23]	= x[ 7] + x[ 6];
//--------------------------------------------------------------------------------
	vtemp		= __vperm(vx4,vx20,S_b0b2a1a3);			// vtemp = [ x20, x22, x5, x7 ]
	vtemp2		= __vperm(vx4,vx20,S_b1b3a0a2);			// vtemp2 = [ x21, x23, x4, x6 ]
	vtemp3		= __vperm(vx4,vx20,S_b0a1b2a3);			// vtemp = [ x20, x5, x22, x7 ]
	vtemp4		= __vperm(vx4,vx20,S_b1a0b3a2);			// vtemp2 = [ x21, x4, x23, x6 ]
	vr			= __vsubsws(vtemp, vtemp2);				// vr = [ x20-x21, x22-x23, x5-x4, x7-x6 ]
//--------------------------------------------------------------------------------
//	x[ 4] = MULT31((r0 - r1) , cPI2_8);
//	x[ 5] = MULT31((r3 + r2) , cPI2_8);
//	x[ 6] = MULT31((r0 + r1) , cPI2_8);
//	x[ 7] = MULT31((r3 - r2) , cPI2_8);
//--------------------------------------------------------------------------------
	vfr			= __vcfsx(vr,0);
	vftemp		= __vperm(vfr,vfr,S_a0a3a0a3);
	vftemp2		= __vperm(vfr,vfr,S_a1a2a1a2);
	vftemp		= __vmaddfp(vftemp2,vfsign3,vftemp);
	vftemp		= __vmulfp(vftemp, vfcPI2_8);

	*vpx20		= __vaddsws(vtemp3, vtemp4);			// vx20 = [ x20+x21, x5+x4, x22+x23, x7+x6 ]
	*vpx4		= __vctsxs(vftemp,0);
	MB();
//--------------------------------------------------------------------------------
//	r0		= x[24] - x[25];
//	r1		= x[26] - x[27];
//	r2		= x[ 9] - x[ 8];
//	r3		= x[11] - x[10];
//	x[24]	= x[24] + x[25];
//	x[25]	= x[ 9] + x[ 8];
//	x[26]	= x[26] + x[27];
//	x[27]	= x[11] + x[10];
//--------------------------------------------------------------------------------
	vtemp		= __vperm(vx8,vx24,S_b0b2a1a3);			// vtemp = [ x24, x26, x9, x11 ]
	vtemp2		= __vperm(vx8,vx24,S_b1b3a0a2);			// vtemp2 = [ x25, x27, x8, x10 ]
	vtemp3		= __vperm(vx8,vx24,S_b0a1b2a3);			// vtemp = [ x24, x9, x26, x11 ]
	vtemp4		= __vperm(vx8,vx24,S_b1a0b3a2);			// vtemp2 = [ x25, x8, x27, x10 ]
	vr			= __vsubsws(vtemp, vtemp2);				// vr =  [ x24-x25, x26-x27, x9-x8, x11-x10 ]
	vfr			= __vcfsx(vr,0);
//--------------------------------------------------------------------------------
//	XNPROD31( r0, r1, cPI1_8, cPI3_8, &x[ 8], &x[10] );
//	XPROD31 ( r2, r3, cPI3_8, cPI1_8, &x[ 9], &x[11] );
//--------------------------------------------------------------------------------
	vftemp		= __vperm(vfr,vfr,S_a0a2a1a3);			// vtemp = [ r0, r2, r1, r3 ]
	vftemp2		= __vperm(vfr,vfr,S_a1a3a0a2);			// vtemp2 = [ r1, r3, r0, r2 ]

	vftemp		= vftemp * vfcPI_1313;					// vtemp2 = [ v0*c18, v2*c38, v1*c38, v3*c18 ]
	vftemp2		= vftemp2 * vfcPI_3131;					// vtemp2 = [ v1*c38, v3*c18, v0*c18, v2*c38 ]
	vftemp		= __vmaddfp(vftemp2,vfsign3,vftemp);	// vtemp += [ -(v1*c18), v3*c38, v0*c18, -(v2*c38) ]

	*vpx24		= __vaddsws(vtemp3, vtemp4);			// vx24 = [ x24+x25, x9+x8, x26+x27, x11+x10 ]
	*vpx8		= __vctsxs(vftemp,0);

	MB();
//--------------------------------------------------------------------------------
//	r0		= x[28] - x[29];
//	r1		= x[30] - x[31];
//	r2		= x[12] - x[13];
//	r3		= x[15] - x[14];
//	x[28]	= x[28] + x[29];
//	x[29]	= x[13] + x[12];
//	x[30]	= x[30] + x[31];
//	x[31]	= x[15] + x[14];
//--------------------------------------------------------------------------------
	vtemp		= __vperm(vx12,vx28,S_b0b2a0a3);		// vtemp = [ x28, x30, x12, x15 ]
	vtemp2		= __vperm(vx12,vx28,S_b1b3a1a2);		// vtemp2 = [ x29, x31, x13, x14 ]
	vtemp3		= __vperm(vx12,vx28,S_b0a1b2a3);		// vtemp = [ x28, x13, x30, x15 ]
	vtemp4		= __vperm(vx12,vx28,S_b1a0b3a2);		// vtemp2 = [ x29, x12, x31, x14 ]
	vr			= __vsubsws(vtemp, vtemp2);				// vr = [ x28-x29, x30-x31, x12-x13, x15-x14 ]
//--------------------------------------------------------------------------------
//	x[12]	= r0;
//	x[13]	= r3; 
//	x[14]	= r1;
//	x[15]	= r2;
//--------------------------------------------------------------------------------
	*vpx28		= __vaddsws(vtemp3, vtemp4);			// vx28 = [ x28+x29, x13+x12, x30+x31, x15+x14 ]
	*vpx12		= __vperm(vr,vr,S_a0a3a1a2);			// vtemp2 = [ r0, r3, r1, r2 ]

	MB();

	mdct_butterfly_16_bis(x);
}
//====================================================================================================
// N/stage point generic N stage butterfly (in place, 2 register)
//====================================================================================================
STIN void mdct_butterfly_generic(DATA_TYPE *x,int points,int step)
{
	AKASSERT(((AkUInt32)x & 0x0F) == 0);
	AKASSERT((step % 4) == 0);
	AKASSERT((points % 4) == 0);

	float				*pfT		= fsincos_lookup0;
	float				*LoopEnd	= fsincos_lookup0 + 1024;
	DATA_TYPE			*x1			= x + points - 4;
	DATA_TYPE			*x2			= x + (points>>1) - 4;
	__vector4			vr,vtemp,vtemp2;

	do
	{
		__vector4 *vpx1	= (__vector4*)x1;
		__vector4 *vpx2	= (__vector4*)x2;
		__vector4 vx1	= *(__vector4*)x1;
		__vector4 vx2	= *(__vector4*)x2;
//--------------------------------------------------------------------------------
//		r0		= x1[0] - x1[1];
//		r1		= x1[3] - x1[2];
//		r2		= x2[1] - x2[0];
//		r3		= x2[3] - x2[2];
//--------------------------------------------------------------------------------
		vr			= __vperm(vx1,vx2,S_a0a3b1b3);
		vtemp		= __vperm(vx1,vx2,S_a1a2b0b2);
		vr			= __vsubsws(vr, vtemp);
//--------------------------------------------------------------------------------
//		x1[0]	= x1[0] + x1[1];
//		x1[1]	= x2[1] + x2[0];
//		x1[2]	= x1[2] + x1[3];
//		x1[3]	= x2[3] + x2[2];
//--------------------------------------------------------------------------------
		vtemp		= __vperm(vx1,vx2,S_a0b1a2b3);
		vtemp2		= __vperm(vx1,vx2,S_a1b0a3b2);
		*vpx1		= __vaddsws(vtemp, vtemp2);
//--------------------------------------------------------------------------------
//		XPROD31( r1, r0, T[0], T[1], &x2[0], &x2[2] );
//		XPROD31( r2, r3, T[0], T[1], &x2[1], &x2[3] );
//--------------------------------------------------------------------------------
		__vector4	vfr,vfT,vfT0,vfT1,vftemp,vftemp2;

		vfr			= __vcfsx(vr,0);
		vfT			= *(__vector4*)pfT;						// vT = [ T0, T1, T2, T3 ]

		vfT0		= __vperm(vfT,vfT,S_a0a0a0a0);			// vT0 = [ T0, T0, T0, T0 ]
		vfT1		= __vperm(vfT,vfT,S_a1a1a1a1);			// vT1 = [ T1, T1, T1, T1 ]

		vftemp		= __vperm(vfr,vfr,S_a1a2a0a3);			// vtemp = [ v1, v2, v0, v3 ]
		vftemp		= __vmulfp(vftemp, vfT0);				// vtemp2 = [ v1*T0, v2*T0, v0*T0, v3*T0 ]

		vftemp2		= __vperm(vfr,vfr,S_a0a3a1a2);			// vtemp2 = [    v0,    v3,       v1,       v2 ]
		vftemp2		= __vmulfp(vftemp2, vfT1);				// vtemp2 = [ v0*T1, v3*T1,    v1*T1,    v2*T1 ]
		vftemp		= __vmaddfp(vftemp2,vfsign1,vftemp);	// vtemp += [ v0*T1, v3*T1, -(v1*T1), -(v2*T1) ]

		*vpx2		= __vctsxs(vftemp,0);

		pfT += step;
		x1 -= 4; 
		x2 -= 4;
	}
	while(pfT < LoopEnd);

	LoopEnd = fsincos_lookup0;
	do
	{
		__vector4 *vpx1	= (__vector4*)x1;
		__vector4 *vpx2	= (__vector4*)x2;
		__vector4 vx1	= *(__vector4*)x1;
		__vector4 vx2	= *(__vector4*)x2;
//--------------------------------------------------------------------------------
//		r0		= x1[0] - x1[1];
//		r1		= x1[2] - x1[3];
//		r2		= x2[0] - x2[1];
//		r3		= x2[3] - x2[2];
//--------------------------------------------------------------------------------
		vr			= __vperm(vx1,vx2,S_a0a2b0b3);
		vtemp		= __vperm(vx1,vx2,S_a1a3b1b2);
		vr			= __vsubsws(vr, vtemp);
//--------------------------------------------------------------------------------
//		x1[0]	= x1[0] + x1[1];
//		x1[1]	= x2[1] + x2[0];
//		x1[2]	= x1[2] + x1[3];
//		x1[3]	= x2[3] + x2[2];
//--------------------------------------------------------------------------------
		vtemp		= __vperm(vx1,vx2,S_a0b1a2b3);
		vtemp2		= __vperm(vx1,vx2,S_a1b0a3b2);
		*vpx1		= __vaddsws(vtemp, vtemp2);
//--------------------------------------------------------------------------------
//		XNPROD31( r0, r1, T[0], T[1], &x2[0], &x2[2] );
//		XNPROD31( r3, r2, T[0], T[1], &x2[1], &x2[3] );
//--------------------------------------------------------------------------------
		__vector4	vfr,vfT,vfT0,vfT1,vftemp,vftemp2;

		vfr			= __vcfsx(vr,0);
		vfT			= *(__vector4*)pfT;						// vT = [ T0, T1, T2, T3 ]

		vfT0		= __vperm(vfT,vfT,S_a0a0a0a0);			// vT0 = [ T0, T0, T0, T0 ]
		vfT1		= __vperm(vfT,vfT,S_a1a1a1a1);			// vT1 = [ T1, T1, T1, T1 ]

		vftemp		= __vperm(vfr,vfr,S_a0a3a1a2);			// vtemp = [ v0, v3, v1, v2 ]
		vftemp		= __vmulfp(vftemp, vfT0);				// vtemp2 = [ v0*T0, v3*T0, v1*T0, v2*T0 ]

		vftemp2		= __vperm(vfr,vfr,S_a1a2a0a3);			// vtemp2 = [       v1,       v2,    v0,    v3 ]
		vftemp2		= __vmulfp(vftemp2, vfT1);				// vtemp2 = [    v1*T1,    v2*T1, v0*T1, v3*T1 ]
		vftemp		= __vmaddfp(vftemp2,vfsign2,vftemp);	// vtemp += [ -(v1*T1), -(v2*T1), v0*T1, v3*T1 ]

		*vpx2		= __vctsxs(vftemp,0);

		pfT -= step;
		x1 -= 4; 
		x2 -= 4;
	}
	while(pfT > LoopEnd);
}
//====================================================================================================
//====================================================================================================
STIN void mdct_butterflies(DATA_TYPE *x,int points,int shift)
{
	int stages=8-shift;
	int i,j;

	for(i = 0 ; --stages > 0 ; i++)
	{
		for(j = 0 ; j < (1<<i) ; j++)
		{
			mdct_butterfly_generic(x+(points>>i)*j,points>>i,4<<(i+shift));
		}
	}

	for(j=0;j<points;j+=32)
	{
		mdct_butterfly_32(x+j);
	}
}
//====================================================================================================
//====================================================================================================
static unsigned char bitrev[16]={0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};

STIN int bitrev12(int x)
{
	return	bitrev[x>>8]
			|(bitrev[(x&0x0f0)>>4]<<4)
			|(((int)bitrev[x&0x00f])<<8);
}
//====================================================================================================
//====================================================================================================
STIN void mdct_bitreverse(DATA_TYPE *x,int n,int shift)
{
	int          bit   = 0;
	DATA_TYPE   *w     = x+(n>>1);

	do
	{
		DATA_TYPE  b     = bitrev12(bit++);
		DATA_TYPE *xx    = x + (b>>shift);
		REG_TYPE  r;

		w    -= 2;

		if(w>xx)
		{
			r      = xx[0];
			xx[0]  = w[0];
			w[0]   = r;

			r      = xx[1];
			xx[1]  = w[1];
			w[1]   = r;
		}
	}
	while(w>x);
}
//====================================================================================================
//====================================================================================================
STIN void mdct_step7(DATA_TYPE *x,int n,int step)
{
	DATA_TYPE   *w0 = x;
	DATA_TYPE   *w1 = x+(n>>1);
	float		*T = (step>=4)?(fsincos_lookup0+(step>>1)):fsincos_lookup1;
	float		*Ttop  = T+1024;

	do
	{
		w1 -= 4;

		__vector4 vr		= *(__vector4*)w0;
		__vector4 vfw0		= __vcfsx(vr,0);

		vr					= *(__vector4*)w1;
		__vector4 vfw1		= __vcfsx(vr,0);

		__vector4 vfT0		= __loadunalignedvector((__vector4*)T);
		T += step;
		__vector4 vfT1		= __loadunalignedvector((__vector4*)T);
		T+=step;
//--------------------------------------------------------------------------------
//		fw00	= ((w0[1] + w1[3]) + (((w0[0] + w1[2]) * pT0[1]) + ((w1[3] - w0[1]) * pT0[0]))) * 0.5f;
//		fw01	= ((w0[0] - w1[2]) + (((w1[3] - w0[1]) * pT0[1]) - ((w0[0] + w1[2]) * pT0[0]))) * 0.5f;
//		fw02	= ((w0[3] + w1[1]) + (((w0[2] + w1[0]) * pT1[1]) + ((w1[1] - w0[3]) * pT1[0]))) * 0.5f;
//		fw03	= ((w0[2] - w1[0]) + (((w1[1] - w0[3]) * pT1[1]) - ((w0[2] + w1[0]) * pT1[0]))) * 0.5f;
//--------------------------------------------------------------------------------
		// A
		__vector4 vfA		= __vperm(vfw0,vfw0,S_a1a0a3a2);	// vfA    = [       w0[1],       w0[0],       w0[3],       w0[2] ]
		__vector4 vftemp	= __vperm(vfw1,vfw1,S_a3a2a1a0);	// vftemp = [       w1[3],       w1[2],       w1[1],       w1[0] ]
		vfA					= __vmaddfp(vftemp,vfsign5,vfA);	// vfA    = [ w0[1]+w1[3], w0[0]-w1[2], w0[3]+w1[1], w0[2]-w1[0] ]

		// B
		__vector4 vfB		= __vperm(vfw0,vfw1,S_a0b3a2b1);	// vfB    = [        w0[0],              w1[3],              w0[2],              w1[1] ]
		vftemp				= __vperm(vfw0,vfw1,S_b2a1b0a3);	// vftemp = [        w1[2],              w0[1],              w1[0],              w0[3] ]
		vfB					= __vmaddfp(vftemp,vfsign5,vfB);	// vfB    = [  w0[0]+w1[2],        w1[3]-w0[1],        w0[2]+w1[0],        w1[1]-w0[3] ]
		vftemp				= __vperm(vfT0,vfT1,S_a1a1b1b1);	// vftemp = [               T0[1],              T0[1],              T1[1],              T1[1] ]
		vfB					= __vmulfp(vfB,vftemp);				// vfB    = [ (w0[0]+w1[2])*T0[1],(w1[3]-w0[1])*T0[1],(w0[2]+w1[0])*T1[1],)w1[1]-w0[3])*T1[1] ]

		// C
		__vector4 vfC		= __vperm(vfw0,vfw1,S_b3a0b1a2);	// vfC    = [       w1[3],       w0[0],       w1[1],       w0[2] ]
		vftemp				= __vperm(vfw0,vfw1,S_a1b2a3b0);	// vftemp = [       w0[1],       w1[2],       w0[3],       w1[0] ]
		vfC					= __vmaddfp(vftemp,vfsign6,vfC);	// vfC    = [ w1[3]-w0[1], w0[0]+w1[2], w1[1]-w0[3], w0[2]+w1[0] ]
		vftemp				= __vperm(vfT0,vfT1,S_a0a0b0b0);	// vftemp = [               T0[0],              T0[0],              T1[0],              T1[0] ]
		vfC					= __vmulfp(vfC,vftemp);				// vfC    = [ (w1[3]-w0[1])*T0[0],(w0[0]+w1[2])*T0[0],(w1[1]-w0[3])*T1[0],(w0[2]+w1[0])*T1[0] ]

		// A + B + C
		vfB					= __vmaddfp(vfC,vfsign5,vfB);
		vfA					= __vaddfp(vfA,vfB);
		vfA					= __vmulfp(vfA,vfPointFive);		// final result

		*(__vector4*)w0		= vfA;
//--------------------------------------------------------------------------------
//		fw10	= ( (w0[3] + w1[1]) - (((w0[2] + w1[0]) * pT1[1]) + ((w1[1] - w0[3]) * pT1[0]))) * 0.5f;
//		fw11	= (-(w0[2] - w1[0]) + (((w1[1] - w0[3]) * pT1[1]) - ((w0[2] + w1[0]) * pT1[0]))) * 0.5f;
//		fw12	= ( (w0[1] + w1[3]) - (((w0[0] + w1[2]) * pT0[1]) + ((w1[3] - w0[1]) * pT0[0]))) * 0.5f;
//		fw13	= (-(w0[0] - w1[2]) + (((w1[3] - w0[1]) * pT0[1]) - ((w0[0] + w1[2]) * pT0[0]))) * 0.5f;
//--------------------------------------------------------------------------------
		// A
		vfA					= __vperm(vfw0,vfw0,S_a3a2a1a0);	// vfA    = [       w0[3],       w0[2],       w0[1],       w0[0] ]
		vftemp				= __vperm(vfw1,vfw1,S_a1a0a3a2);	// vftemp = [       w1[1],       w1[0],       w1[3],       w1[2] ]
		vfA					= __vmaddfp(vftemp,vfsign5,vfA);	// vfA    = [ w0[3]+w1[3], w0[2]-w1[2], w0[1]+w1[1], w0[0]-w1[0] ]
		vfA					= __vmulfp(vfA,vfsign5);			// vfA    = [ w0[3]+w1[1],-w0[2]-w1[0], w0[1]+w1[3],-w0[0]-w1[2] ]

		// B
		vfB					= __vperm(vfw0,vfw1,S_a2b1a0b3);	// vfB    = [       w0[2],              w1[1],              w0[0],              w1[3] ]
		vftemp				= __vperm(vfw0,vfw1,S_b0a3b2a1);	// vftemp = [       w1[0],              w0[3],              w1[2],              w0[1] ]
		vfB					= __vmaddfp(vftemp,vfsign5,vfB);	// vfB    = [ w0[2]+w1[0],        w1[1]-w0[3],        w0[0]+w1[2],        w1[3]-w0[1] ]
		vftemp				= __vperm(vfT1,vfT0,S_a1a1b1b1);	// vftemp = [              T1[1],              T1[1],              T0[1],              T0[1] ]
		vfB					= __vmulfp(vfB,vftemp);				// vfB    = [(w0[2]+w1[0])*T1[1],(w1[1]-w0[3])*T1[1],(w0[0]+w1[2])*T0[1],(w1[3]-w0[1])*T0[1] ]

		// C
		vfC					= __vperm(vfw0,vfw1,S_b1a2b3a0);	// vfC    = [       w1[1],              w0[2],              w1[3],              w0[0] ]
		vftemp				= __vperm(vfw0,vfw1,S_a3b0a1b2);	// vftemp = [       w0[3],              w1[0],              w0[1],              w1[2] ]
		vfC					= __vmaddfp(vftemp,vfsign6,vfC);	// vfC    = [ w1[1]-w0[3],        w0[2]+w1[0],        w1[3]-w0[1],        w0[0]+w1[2] ]
		vftemp				= __vperm(vfT1,vfT0,S_a0a0b0b0);	// vftemp = [              T1[0],              T1[0],              T0[0],              T0[0] ]
		vfC					= __vmulfp(vfC,vftemp);				// vfC    = [(w1[1]-w0[3])*T1[0],(w0[2]+w1[0])*T1[0],(w1[3]-w0[1])*T0[0],(w0[0]+w1[2])*T0[0] ]

		// A + B + C
		vfB					= __vmaddfp(vfC,vfsign5,vfB);
		vfA					= __vmaddfp(vfB,vfsign6,vfA);
		vfA					= __vmulfp(vfA,vfPointFive);

		*(__vector4*)w1		= vfA;

		w0 += 4;
	}
	while(T<Ttop);

	do
	{
		w1 -= 4;

		__vector4 vr		= *(__vector4*)w0;
		__vector4 vfw0		= __vcfsx(vr,0);

		vr					= *(__vector4*)w1;
		__vector4 vfw1		= __vcfsx(vr,0);

		T -= step;
		__vector4 vfT0		= __loadunalignedvector((__vector4*)T);

		T-=step;
		__vector4 vfT1		= __loadunalignedvector((__vector4*)T);
//--------------------------------------------------------------------------------
//		fw00	= ((w0[1] + w1[3]) + ((w0[0] + w1[2]) * pT0[0] + (w1[3] - w0[1]) * pT0[1])) * 0.5f;
//		fw01	= ((w0[0] - w1[2]) + ((w1[3] - w0[1]) * pT0[0] - (w0[0] + w1[2]) * pT0[1])) * 0.5f;
//		fw02	= ((w0[3] + w1[1]) + ((w0[2] + w1[0]) * pT1[0] + (w1[1] - w0[3]) * pT1[1])) * 0.5f;
//		fw03	= ((w0[2] - w1[0]) + ((w1[1] - w0[3]) * pT1[0] - (w0[2] + w1[0]) * pT1[1])) * 0.5f;
//--------------------------------------------------------------------------------
		__vector4 vfA		= __vperm(vfw0,vfw0,S_a1a0a3a2);
		__vector4 vftemp	= __vperm(vfw1,vfw1,S_a3a2a1a0);
		vfA					= __vmaddfp(vftemp,vfsign5,vfA);

		__vector4 vfB		= __vperm(vfw0,vfw1,S_a0b3a2b1);
		vftemp				= __vperm(vfw0,vfw1,S_b2a1b0a3);
		vfB					= __vmaddfp(vftemp,vfsign5,vfB);
		vftemp				= __vperm(vfT0,vfT1,S_a0a0b0b0);
		vfB					= __vmulfp(vfB,vftemp);

		__vector4 vfC		= __vperm(vfw0,vfw1,S_b3a0b1a2);
		vftemp				= __vperm(vfw0,vfw1,S_a1b2a3b0);
		vfC					= __vmaddfp(vftemp,vfsign6,vfC);
		vftemp				= __vperm(vfT0,vfT1,S_a1a1b1b1);
		vfC					= __vmulfp(vfC,vftemp);

		vfB					= __vmaddfp(vfC,vfsign5,vfB);
		vfA					= __vaddfp(vfA,vfB);
		vfA					= __vmulfp(vfA,vfPointFive);

		*(__vector4*)w0		= vfA;
//--------------------------------------------------------------------------------
//		fw10	= ( (w0[3] + w1[1]) - ((w0[2] + w1[0]) * pT1[0] + (w1[1] - w0[3]) * pT1[1])) * 0.5f;
//		fw11	= (-(w0[2] - w1[0]) + ((w1[1] - w0[3]) * pT1[0] - (w0[2] + w1[0]) * pT1[1])) * 0.5f;
//		fw12	= ( (w0[1] + w1[3]) - ((w0[0] + w1[2]) * pT0[0] + (w1[3] - w0[1]) * pT0[1])) * 0.5f;
//		fw13	= (-(w0[0] - w1[2]) + ((w1[3] - w0[1]) * pT0[0] - (w0[0] + w1[2]) * pT0[1])) * 0.5f;
//--------------------------------------------------------------------------------
		vfA					= __vperm(vfw0,vfw0,S_a3a2a1a0);
		vftemp				= __vperm(vfw1,vfw1,S_a1a0a3a2);
		vfA					= __vmaddfp(vftemp,vfsign5,vfA);
		vfA					= __vmulfp(vfA,vfsign5);

		vfB					= __vperm(vfw0,vfw1,S_a2b1a0b3);
		vftemp				= __vperm(vfw0,vfw1,S_b0a3b2a1);
		vfB					= __vmaddfp(vftemp,vfsign5,vfB);
		vftemp				= __vperm(vfT1,vfT0,S_a0a0b0b0);
		vfB					= __vmulfp(vfB,vftemp);

		vfC					= __vperm(vfw0,vfw1,S_b1a2b3a0);
		vftemp				= __vperm(vfw0,vfw1,S_a3b0a1b2);
		vfC					= __vmaddfp(vftemp,vfsign6,vfC);
		vftemp				= __vperm(vfT1,vfT0,S_a1a1b1b1);
		vfC					= __vmulfp(vfC,vftemp);

		vfB					= __vmaddfp(vfC,vfsign5,vfB);
		vfA					= __vmaddfp(vfB,vfsign6,vfA);
		vfA					= __vmulfp(vfA,vfPointFive);

		*(__vector4*)w1		= vfA;

		w0 += 4;
	}
	while(w0<w1);
}
//====================================================================================================
//====================================================================================================
STIN void mdct_step8(float *x, int n, int step)
{
	float		*T;
	float		*V;
	float		*iX = x+(n>>1);
	step>>=2;

	switch(step)
	{
	default:
		T = (step>=4)?(fsincos_lookup0+(step>>1)):fsincos_lookup1;
		do
		{
//--------------------------------------------------------------------------------
//			XPROD31( x[0], -x[1],    T[0],      T[1], x[0], x[1]);
//			XPROD31( x[2], -x[3], T[step], T[step+1], x[2], x[3]);
//			XPROD31(a,b,t,v,*x,*y)
//			{
//				*x = (a * t) + (b * v);
//				*y = (b * t) - (a * v);
//			}
//
//			x[0] = ( x[0] *    T[0]) - ( x[1] *      T[1]);
//			x[1] = (-x[1] *    T[0]) - ( x[0] *      T[1]);
//			x[2] = ( x[2] * T[step]) - ( x[3] * T[step+1]);
//			x[3] = (-x[3] * T[step]) - ( x[2] * T[step+1]);
//--------------------------------------------------------------------------------
			__vector4 vfr		= *(__vector4*)x;							// cfr     = [      x[0],       x[1],          x[2],            x[3] ]
			__vector4 vftemp	= __vperm(vfr,vfr,S_a1a0a3a2);				// vftemp  = [      x[1],       x[0],          x[3],            x[2] ]
			vfr					= __vmulfp(vfr,vfsign5);					// vfr     = [      x[0],      -x[1],          x[2],           -x[3] ]

			__vector4 vfT0		= __loadunalignedvector((__vector4*)T);
			T += step;
			__vector4 vfT1		= __loadunalignedvector((__vector4*)T);
			T += step;

			__vector4 vftemp2	= __vperm(vfT0,vfT1,S_a0a0b0b0);			// vftemp2 = [      T[0],       T[0],        T[step],         T[step] ]
			vfr					= __vmulfp(vfr,vftemp2);					// vfr     = [ x[0]*T[0], -x[1]*T[0],   x[2]*T[step],   -x[3]*T[step] ]

			vftemp2				= __vperm(vfT0,vfT1,S_a1a1b1b1);			// vftemp2 = [      T[1],       T[1],      T[step+1],       T[step+1] ]
			vftemp				= __vmulfp(vftemp,vftemp2);					// vftemp  = [ x[1]*T[1],  x[0]*T[1], x[3]*T[step+1],  x[2]*T[step+1] ]
			vftemp				= __vsubfp(vfr,vftemp);

			*(__vector4*)x		= vftemp;
			x += 4;
		}
		while(x < iX);
		break;

	case 1: 
		{
			/* linear interpolation between table values: offset=0.5, step=1 */
			T		= fsincos_lookup0;
			V		= fsincos_lookup1;
			do
			{
//--------------------------------------------------------------------------------
//				fr[0]	= (float)x[0];
//				fr[1]	= (float)-x[1];
//				fr[2]	= (float)x[2];
//				fr[3]	= (float)-x[3];
//--------------------------------------------------------------------------------
				__vector4 vfr0		= *(__vector4*)x;							// vfr0    = [ x0,  x1, x2,  x3 ]
				vfr0				= __vmulfp(vfr0,vfsign5);					// vfr0    = [ x0, -x1, x2, -x3 ]
//--------------------------------------------------------------------------------
//				ft[0]	= (T[0] + V[0]) * 0.5f;
//				ft[1]	= (T[1] + V[1]) * 0.5f;
//				ft[2]	= (T[2] + V[2]) * 0.5f;
//				ft[3]	= (T[3] + V[3]) * 0.5f;
//--------------------------------------------------------------------------------
				__vector4 vfT		= __loadunalignedvector((__vector4*)T);		// vfT     = [         T0,          T1,          T2,          T3 ]
				T += 4;
				__vector4 vfV		= __loadunalignedvector((__vector4*)V);		// vfV     = [         V0,          V1,          V2,          V3 ]
				V += 4;
				__vector4 vft		= __vaddfp(vfT,vfV);						// vftemp  = [       T0+V,       T1+V1,       T2+V2,       T3+V3 ]
				vft					= __vmulfp(vft,vfPointFive);
//--------------------------------------------------------------------------------
//				fv[0]	= (V[0] + T[2]) * 0.5f;
//				fv[1]	= (V[1] + T[3]) * 0.5f;
//				fv[2]	= (V[2] + T[4]) * 0.5f;
//				fv[3]	= (V[3] + T[5]) * 0.5f;
//--------------------------------------------------------------------------------
				__vector4 vftemp	= __loadunalignedvector((__vector4*)T);		// vftemp  = [          T4,          T5,          T6,          T7 ]
				vftemp				= __vperm(vfT,vftemp,S_a2a3b0b1);			// vftemp  = [          T2,          T3,          T4,          T5 ]
				__vector4 vfv		= __vaddfp(vfV,vftemp);						// vfv     = [       V0+T2,       V1+T3,       V2+T4,       V3+T5 ]
				vfv					= __vmulfp(vfv,vfPointFive);
//--------------------------------------------------------------------------------
//				x[0]	= (ogg_int32_t)((fr[0] * ft[0]) + (fr[1] * ft[1]));
//				x[1]	= (ogg_int32_t)((fr[1] * ft[0]) - (fr[0] * ft[1]));
//				x[2]	= (ogg_int32_t)((fr[2] * fv[0]) + (fr[3] * fv[1]));
//				x[3]	= (ogg_int32_t)((fr[3] * fv[0]) - (fr[2] * fv[1]));
//--------------------------------------------------------------------------------
				vftemp				= __vperm(vft,vfv,S_a0a0b0b0);				// vftemp  = [          t0,          t0,          v0,          v0 ]
				vftemp				= __vmulfp(vfr0,vftemp);					// vftemp  = [       r0*t0,       r1*t0,       r2*v0,       r3*v0 ]
				__vector4 vftemp2	= __vperm(vft,vfv,S_a1a1b1b1);				// vftemp2 = [          t1,          t1,          v1,          v1 ]
				vfr0				= __vperm(vfr0,vfr0,S_a1a0a3a2);			// vfr0    = [          r1,          r0,          r3,          r2 ]
				vftemp2				= __vmulfp(vfr0,vftemp2);					// vftemp2 = [       r1*t1,       r0*t1,       r3*v1,       r2*v1 ]
				vftemp2				= __vmulfp(vftemp2,vfsign5);				// vftemp2 = [       r1*t1,      -r0*t1,       r3*v1,      -r2*v1 ]
				vftemp2				= __vaddfp(vftemp,vftemp2);					// vftemp2 = [ r0*t0+r1*t1, r1*t0-r0*t1, r2*v0+r3*v1, r3*v0-r2*v1 ]
				*(__vector4*)x		= vftemp2;
				x += 4;
//--------------------------------------------------------------------------------
//				fr[4]	= (float)x[4];
//				fr[5]	= (float)-x[5];
//				fr[6]	= (float)x[6];
//				fr[7]	= (float)-x[7];
//--------------------------------------------------------------------------------
				__vector4 vfr4		= *(__vector4*)x;							// vfr4    = [ x4,  x5, x6,  x7 ]
				vfr4				= __vmulfp(vfr4,vfsign5);					// vfr4    = [ x4, -x5, x6, -x7 ]
//--------------------------------------------------------------------------------
//				x[4]	= (ogg_int32_t)((fr[4] * ft[2]) + (fr[5] * ft[3]));
//				x[5]	= (ogg_int32_t)((fr[5] * ft[2]) - (fr[4] * ft[3]));
//				x[6]	= (ogg_int32_t)((fr[6] * fv[2]) + (fr[7] * fv[3]));
//				x[7]	= (ogg_int32_t)((fr[7] * fv[2]) - (fr[6] * fv[3]));
//--------------------------------------------------------------------------------
				vftemp				= __vperm(vft,vfv,S_a2a2b2b2);				// vftemp  = [          t2,          t2,          v2,          v2 ]
				vftemp				= __vmulfp(vfr4,vftemp);					// vftemp  = [       r4*t0,       r5*t0,       r6*v0,       r7*v0 ]
				vftemp2				= __vperm(vft,vfv,S_a3a3b3b3);				// vftemp2 = [          t3,          t3,          v3,          v3 ]
				vfr4				= __vperm(vfr4,vfr4,S_a1a0a3a2);			// vfr4    = [          r5,          r4,          r7,          r6 ]
				vftemp2				= __vmulfp(vfr4,vftemp2);					// vftemp2 = [       r5*t3,       r4*t3,       r7*v3,       r6*v3 ]
				vftemp2				= __vmulfp(vftemp2,vfsign5);				// vftemp2 = [       r5*t3,      -r4*t3,       r7*v3,      -r6*v3 ]
				vftemp2				= __vaddfp(vftemp,vftemp2);					// vftemp2 = [ r4*t0+r5*t3, r5*t0-r4*t3, r6*v0+r7*v3, r7*v0-r6*v3 ]
				*(__vector4*)x		= vftemp2;
				x += 4;
			}
			while(x < iX);
		}
		break;

	case 0: 
		{
//			PRINT("0\n");
			/* linear interpolation between table values: offset=0.25, step=0.5 */
			float		fq0,fq1,fr0,fr1,fv0,fv1,ft0,ft1;

			T	= fsincos_lookup0;
			V	= fsincos_lookup1;
			ft0	= *T++;
			ft1	= *T++;
			do
			{
				fv0  = *V++;
				fv1  = *V++;
				fq0 = (fv0 - ft0) * 0.25f;
				fq1 = (fv1 - ft1) * 0.25f;
				ft0 += fq0;
				ft1 += fq1;
				fr0  = x[0];
				fr1  = -x[1];	
				ffXPROD31( fr0, fr1, ft0, ft1, x, x+1 );

				ft0  = fv0 - fq0;
				ft1  = fv1 - fq1;
				fr0  = x[2];
				fr1  = -x[3];
				ffXPROD31( fr0, fr1, ft0, ft1, x+2, x+3 );

				ft0  = *T++;
				ft1  = *T++;
				fq0 = (ft0 - fv0) * 0.25f;
				fq1 = (ft1 - fv1) * 0.25f;
				fv0 += fq0;
				fv1 += fq1;
				fr0  = x[4];
				fr1  = -x[5];
				ffXPROD31( fr0, fr1, fv0, fv1, x+4, x+5 );

				fv0  = ft0 - fq0;
				fv1  = ft1 - fq1;
				fr0  = x[6];
				fr1  = -x[7];
				ffXPROD31( fr0, fr1, fv0, fv1, x+5, x+6 );

				x+=8;
			}
			while(x<iX);
		}
		break;
	}
}
//====================================================================================================
// partial; doesn't perform last-step deinterleave/unrolling.  That
//  can be done more efficiently during pcm output
//====================================================================================================
void mdct_backward(int n, DATA_TYPE *in)
{
	int shift;
	int step;

	for (shift=4;!(n&(1<<shift));shift++);
	shift=13-shift;
	step=2<<shift;

	presymmetry(in,n>>1,step);

	mdct_butterflies(in,n>>1,shift);

	mdct_bitreverse(in,n,shift);

	mdct_step7(in,n,step);

	mdct_step8((float *) in,n,step);
}
//====================================================================================================
//====================================================================================================
void mdct_shift_right(int n, DATA_TYPE *in, DATA_TYPE *right)
{
	int i;
	n>>=2;
	in+=1;

	for(i=0;i<n;i++)
	{
		right[i]=in[i<<1];
	}
}
//====================================================================================================
//====================================================================================================
#define ffpmax(a,b) __fself((a)-(b), a,b)
#define ffpmin(a,b) __fself((a)-(b), b,a)

void mdct_unroll_lap(	int			n0,
						int			n1,
						int			lW,
						int			W,
						float		*in,
						float		*right,
						float		*w0,
						float		*w1,
						ogg_int16_t	*out,
						int			step,
						int			start, /* samples, this frame */
						int			end    /* samples, this frame */)
{
	static const __vector4 vInterleave = { 9.8319e-041, 9.63069947e-038, 1.56460619e-036, 2.54128075e-035 };
	// 0x00011213, 0x02031617, 0x04051a1b, 0x06071e1f
		
	float *l=in+(W&&lW ? n1>>1 : n0>>1);
	float *r=right+(lW ? n1>>2 : n0>>2);
	float *post;
	float *wR=(W && lW ? w1+(n1>>1) : w0+(n0>>1));
	float *wL=(W && lW ? w1         : w0        );

	int preLap=(lW && !W ? (n1>>2)-(n0>>2) : 0 );
	int halfLap=(lW && W ? (n1>>2) : (n0>>2) );
	int postLap=(!lW && W ? (n1>>2)-(n0>>2) : 0 );
	int n,off;

	/* preceeding direct-copy lapping from previous frame, if any */
	if(preLap)
	{
		n      = (end<preLap?end:preLap);
		off    = (start<preLap?start:preLap);
		post   = r-n;
		r     -= off;
		start -= off;
		end   -= n;
/*		VECTOR PATH commented out due to overshoot in __storeunalignedvector
		int numVec = (r-post) / 4;
		if ( step == 1 ) 
		{
			while ( numVec-- > 0 )
			{
				r -= 4;
				__vector4 vr = __vpermwi( __loadunalignedvector((__vector4*)r), 0xe4 ); //invert
				static const __vector4 vDivisor = { 1/512.0f, 1/512.0f, 1/512.0f, 1/512.0f };
				__vector4 vProd = __vctsxs( __vmulfp( vr, vDivisor ), 0 );

				__storeunalignedvector( __vpkswss( vProd, vProd ), (__vector4*) out );
				out += 4;
			}
		}
		else
		{
			assert( step == 2 ); // need to extend for multi-channel
			while ( numVec-- > 0 )
			{
				r -= 4;
				__vector4 vr = __vpermwi( __loadunalignedvector((__vector4*)r), 0xe4 ); // invert
				static const __vector4 vDivisor = { 1/512.0f, 1/512.0f, 1/512.0f, 1/512.0f };
				__vector4 vProd = __vctsxs( __vmulfp( vr, vDivisor ), 0 );

				__vector4 vOut = __loadunalignedvector( (__vector4*) out );

				vOut = __vperm( __vpkswss( vProd, vProd ), vOut, vInterleave );

				__storeunalignedvector( vOut, (__vector4*) out );
				out += 8;
			}
		}
*/		while(r>post)
		{
			//CLIP_TO_15((*--r)>>9);
			float fSample = (*--r)*(1/512.0f);

			fSample = ffpmin( fSample, 32767.0f );
			fSample = ffpmax( fSample, -32768.0f );

			*out = fSample;
			out+=step;
		}
	}

	/* cross-lap; two halves due to wrap-around */
	n      = (end<halfLap?end:halfLap);
	off    = (start<halfLap?start:halfLap);
	post   = r-n;
	r     -= off;
	l     -= off*2;
	start -= off;
	wR    -= off;
	wL    += off;
	end   -= n;

/*	int numVec = (r-post) / 4;
	if ( step == 1 ) 
	{
		while ( numVec-- > 0 )
		{
			r -= 4;
			wR -= 4;
			__vector4 vr = __vpermwi( __loadunalignedvector((__vector4*)r), 0xe4 ); // invert
			__vector4 vwR = __vpermwi( __loadunalignedvector((__vector4*)wR), 0xe4 );
			__vector4 vwL = __loadunalignedvector((__vector4*)wL );
			wL += 4;
			l -= 4;
			__vector4 vl1 = __loadunalignedvector((__vector4*)l);
			l -= 4;
			__vector4 vl2 = __loadunalignedvector((__vector4*)l);

			static const __vector4 vPerm = { vA2, vA0, vB2, vB0 };
			__vector4 vl = __vperm( vl1, vl2, vPerm );

			// shuffle
			__vector4 vProd = __vmaddfp( vr, vwR, __vmulfp( vl, vwL ) );

			static const __vector4 vDivisor = { 1/512.0f, 1/512.0f, 1/512.0f, 1/512.0f };
			vProd = __vctsxs( __vmulfp( vProd, vDivisor ), 0 );

			__storeunalignedvector( __vpkswss( vProd, vProd ), (__vector4*) out );
			out += 4;
		}
	}
	else
	{
		assert( step == 2 ); // need to extend for multi-channel

		while ( numVec-- > 0 )
		{
			r -= 4;
			wR -= 4;
			__vector4 vr = __vpermwi( __loadunalignedvector((__vector4*)r), 0xe4 ); // invert
			__vector4 vwR = __vpermwi( __loadunalignedvector((__vector4*)wR), 0xe4 );
			__vector4 vwL = __loadunalignedvector((__vector4*)wL );
			wL += 4;
			l -= 4;
			__vector4 vl1 = __loadunalignedvector((__vector4*)l);
			l -= 4;
			__vector4 vl2 = __loadunalignedvector((__vector4*)l);

			static const __vector4 vPerm = { vA2, vA0, vB2, vB0 };
			__vector4 vl = __vperm( vl1, vl2, vPerm );

			// shuffle
			__vector4 vProd = __vmaddfp( vr, vwR, __vmulfp( vl, vwL ) );

			static const __vector4 vDivisor = { 1/512.0f, 1/512.0f, 1/512.0f, 1/512.0f };
			vProd = __vctsxs( __vmulfp( vProd, vDivisor ), 0 );

			__vector4 vOut = __loadunalignedvector( (__vector4*) out );

			vOut = __vperm( __vpkswss( vProd, vProd ), vOut, vInterleave );

			__storeunalignedvector( vOut, (__vector4*) out );
			out += 8;
		}
	}
*/
	while(r>post)
	{
		l-=2;
		//	*out = CLIP_TO_15((MULT31(*--r,*--wR) + MULT31(*l,*wL++))>>9);
		float fr = *--r;
		float fwR = *--wR;
		float fl = *l;
		float fwL = *wL++;

		float Prod	= (fr * fwR) + (fl * fwL);
		Prod		= Prod * (1/512.0f);

		Prod = ffpmin( Prod, 32767.0f );
		Prod = ffpmax( Prod, -32768.0f );

		*out = (ogg_int32_t)Prod;

		out+=step;
	}

	n      = (end<halfLap?end:halfLap);
	off    = (start<halfLap?start:halfLap);
	post   = r+n;
	r     += off;
	l     += off*2;
	start -= off;
	end   -= n;
	wR    -= off;
	wL    += off;
	
/*	numVec = (post-r) / 4;
	if ( step == 1 ) 
	{
		while ( numVec-- > 0 )
		{
			__vector4 vr = __loadunalignedvector((__vector4*)r);
			r += 4;
			wR -= 4;
			__vector4 vwR = __vpermwi( __loadunalignedvector((__vector4*)wR), 0xe4 );
			__vector4 vwL = __loadunalignedvector((__vector4*)wL );
			wL += 4;
			__vector4 vl1 = __loadunalignedvector((__vector4*)l);
			l += 4;
			__vector4 vl2 = __loadunalignedvector((__vector4*)l);
			l += 4;

			static const __vector4 vPerm = { vA0, vA2, vB0, vB2 };
			__vector4 vl = __vperm( vl1, vl2, vPerm );

			// shuffle
			__vector4 vProd = __vnmsubfp( vr, vwR, __vmulfp( vl, vwL ) );

			static const __vector4 vDivisor = { -1/512.0f, -1/512.0f, -1/512.0f, -1/512.0f };
			vProd = __vctsxs( __vmulfp( vProd, vDivisor ), 0 );

			__storeunalignedvector( __vpkswss( vProd, vProd ), (__vector4*) out );
			out += 4;
		}
	}
	else
	{
		assert( step == 2 ); // need to extend for multi-channel

		__vector4 vInterleave;
		vInterleave.u[0] = 0x00011213;
		vInterleave.u[1] = 0x02031617;
		vInterleave.u[2] = 0x04051a1b;
		vInterleave.u[3] = 0x06071e1f;

		while ( numVec-- > 0 )
		{
			__vector4 vr = __loadunalignedvector((__vector4*)r);
			r += 4;
			wR -= 4;
			__vector4 vwR = __vpermwi( __loadunalignedvector((__vector4*)wR), 0xe4 );
			__vector4 vwL = __loadunalignedvector((__vector4*)wL );
			wL += 4;
			__vector4 vl1 = __loadunalignedvector((__vector4*)l);
			l += 4;
			__vector4 vl2 = __loadunalignedvector((__vector4*)l);
			l += 4;

			static const __vector4 vPerm = { vA0, vA2, vB0, vB2 };
			__vector4 vl = __vperm( vl1, vl2, vPerm );

			// shuffle
			__vector4 vProd = __vnmsubfp( vr, vwR, __vmulfp( vl, vwL ) );

			static const __vector4 vDivisor = { -1/512.0f, -1/512.0f, -1/512.0f, -1/512.0f };
			vProd = __vctsxs( __vmulfp( vProd, vDivisor ), 0 );

			__vector4 vOut = __loadunalignedvector( (__vector4*) out );

			vOut = __vperm( __vpkswss( vProd, vProd ), vOut, vInterleave );

			__storeunalignedvector( vOut, (__vector4*) out );
			out += 8;
		}
	}
*/	
	while(r<post)
	{
		//    *out = CLIP_TO_15((MULT31(*r++,*--wR) - MULT31(*l,*wL++))>>9);
		float fr = *r++;
		float fwR = (float)*--wR;
		float fl = *l;
		float fwL = (float)*wL++;

		float Prod	= (fr * fwR) - (fl * fwL);
		Prod		= Prod * (1/512.0f);

		Prod = ffpmin( Prod, 32767.0f );
		Prod = ffpmax( Prod, -32768.0f );

		*out = (ogg_int32_t)Prod;

		out+=step;
		l+=2;
	}

	/* preceeding direct-copy lapping from previous frame, if any */
	if(postLap)
	{
		n      = (end<postLap?end:postLap);
		off    = (start<postLap?start:postLap);
		post   = l+n*2;
		l     += off*2;
/*		int numVec = (post-l) / (4*2); // vector_size * l stepping
		if ( step == 1 ) 
		{
			while ( numVec-- > 0 )
			{
				__vector4 vl1 = __loadunalignedvector((__vector4*)l);
				l += 4;
				__vector4 vl2 = __loadunalignedvector((__vector4*)l);
				l += 4;
				static const __vector4 vPerm = { vA0, vA2, vB0, vB2 };
				__vector4 vl = __vperm( vl1, vl2, vPerm );

				static const __vector4 vDivisor = { -1/512.0f, -1/512.0f, -1/512.0f, -1/512.0f };
				__vector4 vProd = __vctsxs( __vmulfp( vl, vDivisor ), 0 );

				__storeunalignedvector( __vpkswss( vProd, vProd ), (__vector4*) out );
				out += 4;
			}
		}
		else
		{
			assert( step == 2 ); // need to extend for multi-channel
			while ( numVec-- > 0 )
			{
				__vector4 vl1 = __loadunalignedvector((__vector4*)l);
				l += 4;
				__vector4 vl2 = __loadunalignedvector((__vector4*)l);
				l += 4;
				static const __vector4 vPerm = { vA0, vA2, vB0, vB2 };
				__vector4 vl = __vperm( vl1, vl2, vPerm );

				static const __vector4 vDivisor = { -1/512.0f, -1/512.0f, -1/512.0f, -1/512.0f };
				__vector4 vProd = __vctsxs( __vmulfp( vl, vDivisor ), 0 );

				__vector4 vOut = __loadunalignedvector( (__vector4*) out );

				vOut = __vperm( __vpkswss( vProd, vProd ), vOut, vInterleave );

				__storeunalignedvector( vOut, (__vector4*) out );
				out += 8;
			}
		}
*/		while(l<post)
		{
			//CLIP_TO_15((-*l)>>9);
			float fSample = *l*(-1/512.0f);

			fSample = ffpmin( fSample, 32767.0f );
			fSample = ffpmax( fSample, -32768.0f );

			*out = fSample;
			out+=step;
			l+=2;
		}
	}
}


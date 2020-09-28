// Reference:
// "Fast Scheme for Image Size Change in the Compressed Domain"
// http://vision.ai.uiuc.edu/~dugad/research/dct/index.html

namespace DCT_Upsample {

const int FRACT_BITS = 10;
const int SCALE = 1 << FRACT_BITS;

typedef int Temp_Type;
#define D(i) (((i) + (SCALE >> 1)) >> FRACT_BITS)
#define F(i) ((int)((i) * SCALE + .5f))

//typedef float Temp_Type;
//#define D(i) i
//#define F(i) i

// A good compiler will optimize this at compile time to a 0, or an array access.
#ifdef CODE_ANALYSIS_ENABLED // mjubran 9/17
const Temp_Type IsValidIndex(const int c, const int r, const BLOCK_TYPE* Psrc, const int NUM_COLS, const int NUM_ROWS)
{
	return ((((c)>=NUM_COLS)||((r)>=NUM_ROWS)) ? 0 : Psrc[(c)+(r)*8]);
}
#define AT(c, r) MY_AT(c, r, Psrc, NUM_COLS, NUM_ROWS)
#define MY_AT(c, r, Psrc, NUM_COLS, NUM_ROWS)	IsValidIndex(c, r, Psrc, NUM_COLS, NUM_ROWS)
#else
#define AT(c, r) ((((c)>=NUM_COLS)||((r)>=NUM_ROWS)) ? 0 : Psrc[(c)+(r)*8])
#endif

// NUM_ROWS/NUM_COLS = # of non-zero rows/cols in input matrix
template<int NUM_ROWS, int NUM_COLS>
struct P_Q
{
static void calc(Matrix44& P, Matrix44& Q, const BLOCK_TYPE* Psrc)
{
	// 4x8 = 4x8 times 8x8, matrix 0 is constant
	const Temp_Type X000 = AT(0, 0);
	const Temp_Type X001 = AT(0, 1);
	const Temp_Type X002 = AT(0, 2);
	const Temp_Type X003 = AT(0, 3);
	const Temp_Type X004 = AT(0, 4);
	const Temp_Type X005 = AT(0, 5);
	const Temp_Type X006 = AT(0, 6);
	const Temp_Type X007 = AT(0, 7);
	const Temp_Type X010 = D(F(0.415735f) * AT(1, 0) + F(0.791065f) * AT(3, 0) + F(-0.352443f) * AT(5, 0) + F(0.277785f) * AT(7, 0));
	const Temp_Type X011 = D(F(0.415735f) * AT(1, 1) + F(0.791065f) * AT(3, 1) + F(-0.352443f) * AT(5, 1) + F(0.277785f) * AT(7, 1));
	const Temp_Type X012 = D(F(0.415735f) * AT(1, 2) + F(0.791065f) * AT(3, 2) + F(-0.352443f) * AT(5, 2) + F(0.277785f) * AT(7, 2));
	const Temp_Type X013 = D(F(0.415735f) * AT(1, 3) + F(0.791065f) * AT(3, 3) + F(-0.352443f) * AT(5, 3) + F(0.277785f) * AT(7, 3));
	const Temp_Type X014 = D(F(0.415735f) * AT(1, 4) + F(0.791065f) * AT(3, 4) + F(-0.352443f) * AT(5, 4) + F(0.277785f) * AT(7, 4));
	const Temp_Type X015 = D(F(0.415735f) * AT(1, 5) + F(0.791065f) * AT(3, 5) + F(-0.352443f) * AT(5, 5) + F(0.277785f) * AT(7, 5));
	const Temp_Type X016 = D(F(0.415735f) * AT(1, 6) + F(0.791065f) * AT(3, 6) + F(-0.352443f) * AT(5, 6) + F(0.277785f) * AT(7, 6));
	const Temp_Type X017 = D(F(0.415735f) * AT(1, 7) + F(0.791065f) * AT(3, 7) + F(-0.352443f) * AT(5, 7) + F(0.277785f) * AT(7, 7));
	const Temp_Type X020 = AT(4, 0);
	const Temp_Type X021 = AT(4, 1);
	const Temp_Type X022 = AT(4, 2);
	const Temp_Type X023 = AT(4, 3);
	const Temp_Type X024 = AT(4, 4);
	const Temp_Type X025 = AT(4, 5);
	const Temp_Type X026 = AT(4, 6);
	const Temp_Type X027 = AT(4, 7);
	const Temp_Type X030 = D(F(0.022887f) * AT(1, 0) + F(-0.097545f) * AT(3, 0) + F(0.490393f) * AT(5, 0) + F(0.865723f) * AT(7, 0));
	const Temp_Type X031 = D(F(0.022887f) * AT(1, 1) + F(-0.097545f) * AT(3, 1) + F(0.490393f) * AT(5, 1) + F(0.865723f) * AT(7, 1));
	const Temp_Type X032 = D(F(0.022887f) * AT(1, 2) + F(-0.097545f) * AT(3, 2) + F(0.490393f) * AT(5, 2) + F(0.865723f) * AT(7, 2));
	const Temp_Type X033 = D(F(0.022887f) * AT(1, 3) + F(-0.097545f) * AT(3, 3) + F(0.490393f) * AT(5, 3) + F(0.865723f) * AT(7, 3));
	const Temp_Type X034 = D(F(0.022887f) * AT(1, 4) + F(-0.097545f) * AT(3, 4) + F(0.490393f) * AT(5, 4) + F(0.865723f) * AT(7, 4));
	const Temp_Type X035 = D(F(0.022887f) * AT(1, 5) + F(-0.097545f) * AT(3, 5) + F(0.490393f) * AT(5, 5) + F(0.865723f) * AT(7, 5));
	const Temp_Type X036 = D(F(0.022887f) * AT(1, 6) + F(-0.097545f) * AT(3, 6) + F(0.490393f) * AT(5, 6) + F(0.865723f) * AT(7, 6));
	const Temp_Type X037 = D(F(0.022887f) * AT(1, 7) + F(-0.097545f) * AT(3, 7) + F(0.490393f) * AT(5, 7) + F(0.865723f) * AT(7, 7));

	// 4x4 = 4x8 times 8x4, matrix 1 is constant
	P.at(0, 0) = X000;
	P.at(0, 1) = D(X001 * F(0.415735f) + X003 * F(0.791065f) + X005 * F(-0.352443f) + X007 * F(0.277785f));
	P.at(0, 2) = X004;
	P.at(0, 3) = D(X001 * F(0.022887f) + X003 * F(-0.097545f) + X005 * F(0.490393f) + X007 * F(0.865723f));
	P.at(1, 0) = X010;
	P.at(1, 1) = D(X011 * F(0.415735f) + X013 * F(0.791065f) + X015 * F(-0.352443f) + X017 * F(0.277785f));
	P.at(1, 2) = X014;
	P.at(1, 3) = D(X011 * F(0.022887f) + X013 * F(-0.097545f) + X015 * F(0.490393f) + X017 * F(0.865723f));
	P.at(2, 0) = X020;
	P.at(2, 1) = D(X021 * F(0.415735f) + X023 * F(0.791065f) + X025 * F(-0.352443f) + X027 * F(0.277785f));
	P.at(2, 2) = X024;
	P.at(2, 3) = D(X021 * F(0.022887f) + X023 * F(-0.097545f) + X025 * F(0.490393f) + X027 * F(0.865723f));
	P.at(3, 0) = X030;
	P.at(3, 1) = D(X031 * F(0.415735f) + X033 * F(0.791065f) + X035 * F(-0.352443f) + X037 * F(0.277785f));
	P.at(3, 2) = X034;
	P.at(3, 3) = D(X031 * F(0.022887f) + X033 * F(-0.097545f) + X035 * F(0.490393f) + X037 * F(0.865723f));
	// 40 muls 24 adds

	// 4x4 = 4x8 times 8x4, matrix 1 is constant
	Q.at(0, 0) = D(X001 * F(0.906127f) + X003 * F(-0.318190f) + X005 * F(0.212608f) + X007 * F(-0.180240f));
	Q.at(0, 1) = X002;
	Q.at(0, 2) = D(X001 * F(-0.074658f) + X003 * F(0.513280f) + X005 * F(0.768178f) + X007 * F(-0.375330f));
	Q.at(0, 3) = X006;
	Q.at(1, 0) = D(X011 * F(0.906127f) + X013 * F(-0.318190f) + X015 * F(0.212608f) + X017 * F(-0.180240f));
	Q.at(1, 1) = X012;
	Q.at(1, 2) = D(X011 * F(-0.074658f) + X013 * F(0.513280f) + X015 * F(0.768178f) + X017 * F(-0.375330f));
	Q.at(1, 3) = X016;
	Q.at(2, 0) = D(X021 * F(0.906127f) + X023 * F(-0.318190f) + X025 * F(0.212608f) + X027 * F(-0.180240f));
	Q.at(2, 1) = X022;
	Q.at(2, 2) = D(X021 * F(-0.074658f) + X023 * F(0.513280f) + X025 * F(0.768178f) + X027 * F(-0.375330f));
	Q.at(2, 3) = X026;
	Q.at(3, 0) = D(X031 * F(0.906127f) + X033 * F(-0.318190f) + X035 * F(0.212608f) + X037 * F(-0.180240f));
	Q.at(3, 1) = X032;
	Q.at(3, 2) = D(X031 * F(-0.074658f) + X033 * F(0.513280f) + X035 * F(0.768178f) + X037 * F(-0.375330f));
	Q.at(3, 3) = X036;
	// 40 muls 24 adds
}
};

template<int NUM_ROWS, int NUM_COLS>
struct R_S
{
static void calc(Matrix44& R, Matrix44& S, const BLOCK_TYPE* Psrc)
{
	// 4x8 = 4x8 times 8x8, matrix 0 is constant
	const Temp_Type X100 = D(F(0.906127f) * AT(1, 0) + F(-0.318190f) * AT(3, 0) + F(0.212608f) * AT(5, 0) + F(-0.180240f) * AT(7, 0));
	const Temp_Type X101 = D(F(0.906127f) * AT(1, 1) + F(-0.318190f) * AT(3, 1) + F(0.212608f) * AT(5, 1) + F(-0.180240f) * AT(7, 1));
	const Temp_Type X102 = D(F(0.906127f) * AT(1, 2) + F(-0.318190f) * AT(3, 2) + F(0.212608f) * AT(5, 2) + F(-0.180240f) * AT(7, 2));
	const Temp_Type X103 = D(F(0.906127f) * AT(1, 3) + F(-0.318190f) * AT(3, 3) + F(0.212608f) * AT(5, 3) + F(-0.180240f) * AT(7, 3));
	const Temp_Type X104 = D(F(0.906127f) * AT(1, 4) + F(-0.318190f) * AT(3, 4) + F(0.212608f) * AT(5, 4) + F(-0.180240f) * AT(7, 4));
	const Temp_Type X105 = D(F(0.906127f) * AT(1, 5) + F(-0.318190f) * AT(3, 5) + F(0.212608f) * AT(5, 5) + F(-0.180240f) * AT(7, 5));
	const Temp_Type X106 = D(F(0.906127f) * AT(1, 6) + F(-0.318190f) * AT(3, 6) + F(0.212608f) * AT(5, 6) + F(-0.180240f) * AT(7, 6));
	const Temp_Type X107 = D(F(0.906127f) * AT(1, 7) + F(-0.318190f) * AT(3, 7) + F(0.212608f) * AT(5, 7) + F(-0.180240f) * AT(7, 7));
	const Temp_Type X110 = AT(2, 0);
	const Temp_Type X111 = AT(2, 1);
	const Temp_Type X112 = AT(2, 2);
	const Temp_Type X113 = AT(2, 3);
	const Temp_Type X114 = AT(2, 4);
	const Temp_Type X115 = AT(2, 5);
	const Temp_Type X116 = AT(2, 6);
	const Temp_Type X117 = AT(2, 7);
	const Temp_Type X120 = D(F(-0.074658f) * AT(1, 0) + F(0.513280f) * AT(3, 0) + F(0.768178f) * AT(5, 0) + F(-0.375330f) * AT(7, 0));
	const Temp_Type X121 = D(F(-0.074658f) * AT(1, 1) + F(0.513280f) * AT(3, 1) + F(0.768178f) * AT(5, 1) + F(-0.375330f) * AT(7, 1));
	const Temp_Type X122 = D(F(-0.074658f) * AT(1, 2) + F(0.513280f) * AT(3, 2) + F(0.768178f) * AT(5, 2) + F(-0.375330f) * AT(7, 2));
	const Temp_Type X123 = D(F(-0.074658f) * AT(1, 3) + F(0.513280f) * AT(3, 3) + F(0.768178f) * AT(5, 3) + F(-0.375330f) * AT(7, 3));
	const Temp_Type X124 = D(F(-0.074658f) * AT(1, 4) + F(0.513280f) * AT(3, 4) + F(0.768178f) * AT(5, 4) + F(-0.375330f) * AT(7, 4));
	const Temp_Type X125 = D(F(-0.074658f) * AT(1, 5) + F(0.513280f) * AT(3, 5) + F(0.768178f) * AT(5, 5) + F(-0.375330f) * AT(7, 5));
	const Temp_Type X126 = D(F(-0.074658f) * AT(1, 6) + F(0.513280f) * AT(3, 6) + F(0.768178f) * AT(5, 6) + F(-0.375330f) * AT(7, 6));
	const Temp_Type X127 = D(F(-0.074658f) * AT(1, 7) + F(0.513280f) * AT(3, 7) + F(0.768178f) * AT(5, 7) + F(-0.375330f) * AT(7, 7));
	const Temp_Type X130 = AT(6, 0);
	const Temp_Type X131 = AT(6, 1);
	const Temp_Type X132 = AT(6, 2);
	const Temp_Type X133 = AT(6, 3);
	const Temp_Type X134 = AT(6, 4);
	const Temp_Type X135 = AT(6, 5);
	const Temp_Type X136 = AT(6, 6);
	const Temp_Type X137 = AT(6, 7);
	// 80 muls 48 adds

	// 4x4 = 4x8 times 8x4, matrix 1 is constant
	R.at(0, 0) = X100;
	R.at(0, 1) = D(X101 * F(0.415735f) + X103 * F(0.791065f) + X105 * F(-0.352443f) + X107 * F(0.277785f));
	R.at(0, 2) = X104;
	R.at(0, 3) = D(X101 * F(0.022887f) + X103 * F(-0.097545f) + X105 * F(0.490393f) + X107 * F(0.865723f));
	R.at(1, 0) = X110;
	R.at(1, 1) = D(X111 * F(0.415735f) + X113 * F(0.791065f) + X115 * F(-0.352443f) + X117 * F(0.277785f));
	R.at(1, 2) = X114;
	R.at(1, 3) = D(X111 * F(0.022887f) + X113 * F(-0.097545f) + X115 * F(0.490393f) + X117 * F(0.865723f));
	R.at(2, 0) = X120;
	R.at(2, 1) = D(X121 * F(0.415735f) + X123 * F(0.791065f) + X125 * F(-0.352443f) + X127 * F(0.277785f));
	R.at(2, 2) = X124;
	R.at(2, 3) = D(X121 * F(0.022887f) + X123 * F(-0.097545f) + X125 * F(0.490393f) + X127 * F(0.865723f));
	R.at(3, 0) = X130;
	R.at(3, 1) = D(X131 * F(0.415735f) + X133 * F(0.791065f) + X135 * F(-0.352443f) + X137 * F(0.277785f));
	R.at(3, 2) = X134;
	R.at(3, 3) = D(X131 * F(0.022887f) + X133 * F(-0.097545f) + X135 * F(0.490393f) + X137 * F(0.865723f));
	// 40 muls 24 adds
	// 4x4 = 4x8 times 8x4, matrix 1 is constant
	S.at(0, 0) = D(X101 * F(0.906127f) + X103 * F(-0.318190f) + X105 * F(0.212608f) + X107 * F(-0.180240f));
	S.at(0, 1) = X102;
	S.at(0, 2) = D(X101 * F(-0.074658f) + X103 * F(0.513280f) + X105 * F(0.768178f) + X107 * F(-0.375330f));
	S.at(0, 3) = X106;
	S.at(1, 0) = D(X111 * F(0.906127f) + X113 * F(-0.318190f) + X115 * F(0.212608f) + X117 * F(-0.180240f));
	S.at(1, 1) = X112;
	S.at(1, 2) = D(X111 * F(-0.074658f) + X113 * F(0.513280f) + X115 * F(0.768178f) + X117 * F(-0.375330f));
	S.at(1, 3) = X116;
	S.at(2, 0) = D(X121 * F(0.906127f) + X123 * F(-0.318190f) + X125 * F(0.212608f) + X127 * F(-0.180240f));
	S.at(2, 1) = X122;
	S.at(2, 2) = D(X121 * F(-0.074658f) + X123 * F(0.513280f) + X125 * F(0.768178f) + X127 * F(-0.375330f));
	S.at(2, 3) = X126;
	S.at(3, 0) = D(X131 * F(0.906127f) + X133 * F(-0.318190f) + X135 * F(0.212608f) + X137 * F(-0.180240f));
	S.at(3, 1) = X132;
	S.at(3, 2) = D(X131 * F(-0.074658f) + X133 * F(0.513280f) + X135 * F(0.768178f) + X137 * F(-0.375330f));
	S.at(3, 3) = X136;
	// 40 muls 24 adds	
}
};

} // end namespace DCT_Upsample

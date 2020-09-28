//============================================================================
//
// File: random.h
// Copyright (c) 2005-2006, Ensemble Studios
// 
//============================================================================
#pragma once

class Random
{   
public:
   Random();

   static uint uRandMax(void) { return 4294967295; }

   uint uRand(void);
   
   // uRandFast() should be much faster on 360, as it uses no multiplies, just constant shifts and table lookups.
   uint uRandFast(void);

   // Uniform distribution, half open interval [l,h)
   double dRand(double l = 0.0f, double h = 1.0f);
   float fRand(float l = 0.0f, float h = 1.0f);
   int iRand(int l, int h);

   // Box-Muller transform (polar form) with mean m, standard deviation s.
   float fRandGaussian(float m, float s);

   void setSeed(uint i1, uint i2, uint i3, uint i4, uint i5, uint i6);
   void setSeed(uint seed);
   void setSeed64(uint64 seed);
   bool test(void);
   
private:
   uint z, w, jsr, jcong;
   uint a, b, table[256];
   uint x, y, bro; 
   float prevGaussian;
   uchar c;
   bool usePrevGaussian;

   void setTable(uint i1, uint i2, uint i3, uint i4, uint i5, uint i6);
};

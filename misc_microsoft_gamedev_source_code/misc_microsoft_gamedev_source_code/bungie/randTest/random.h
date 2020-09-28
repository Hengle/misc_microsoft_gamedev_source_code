//============================================================================
// random.h
// Copyright (c) 1996-2006, Ensemble Studios
//
//============================================================================
#pragma once

namespace ens
{

class Random
{   
public:
   Random();

   static uint uRandMax(void) { return 4294967295; }

   uint uRand(void);

   // Uniform distribution, half open interval [l,h)
   double dRand(double l = 0.0f, double h = 1.0f);
   float fRand(float l = 0.0f, float h = 1.0f);
   int iRand(int l, int h);

   // Box-Muller transform (polar form) with mean m, standard deviation s.
   float fRandGaussian(float m, float s);

   void setSeed(uint i1, uint i2, uint i3, uint i4, uint i5, uint i6);
   void setSeed(uint seed);
   bool test(void);
   
private:
   uint z, w, jsr, jcong;
   uint a, b, table[256];
   uint x, y, bro; 
   uchar c;
   float prevGaussian;
   bool usePrevGaussian;

   void setTable(uint i1, uint i2, uint i3, uint i4, uint i5, uint i6);
};

} // namespace ens
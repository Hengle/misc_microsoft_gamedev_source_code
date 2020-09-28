//============================================================================
//
//  fastfloat.h
//  
//  Copyright (c) 2008, Ensemble Studios
//
//============================================================================
#pragma once

namespace FastFloat
{
   /*
   * Various fast float functions
   *
   * Credits :
   * [RANDY07]    Randy, Dillion, "Even Faster Floating Point Tricks", http://www.randydillon.org/Papers/2007/everfast.htm
   * [LAMONT06]   Lamont, Chris, "Floating-Point Tricks" GPG6
   * [NVIDIA]     Nvidia - fast_float.h
   * [DAWSON05]   Dawson, Bruce, "Comparing Floating-Point Numbers."
   * 
   * Scrubbing has occured to make them all match in syntax
   * 
   * As a general note, any edge cases, (QNAN, -0 etc) should be handled by the calling function
   * varaibles passed in as QNAN will result in undefined behavior
   * Adding error checking to these utilities will add un-needed bloat.
   */

//#define _USE_XBOX_ASM   //comment this out if you don't want to use the ASM versions of XBOX360 functions
//Note, this is currently disabled because some of the assembly versions cause LHSs, where their non-assemblied counterparts dont

#define _fval_to_int(fp)         (*(int *)&(fp))
#define _fval_to_dword(fp)       (*(DWORD *)&(fp))
#define _sgn_extend_mask(i) (-(int)(((unsigned int)(i))>>31))//Returns a value with the sign bit extended the entire length of the float


   //================================================
   //================================================
   //Compare Tests ==================================

   __inline bool isPositive(const float& a)
   {
      return (_fval_to_int(a) > 0);
   }

   __inline bool isPositiveOrZero(const float& a)
   {
      return (_fval_to_int(a) >= 0);
   }

   __inline bool isNegative(const float& a)
   {
      return (_fval_to_int(a) < 0);
   }

   __inline bool isNegativeOrZero(const float& a)
   {
      return (_fval_to_int(a) <= 0);
   }

   //===============================================
   //===============================================
   //Compare Zero ==================================

   // compare float against zero
   __inline bool compareZero(const float& a)
   {
      return (_fval_to_dword(a) == 0);
   }

   // compare float against zero with a tolerance
   // TOLERANCE MUST BE POSITIVE!!
   __inline bool compareZero(const float& a,const float& tolerance)
   {
      const DWORD pA = _fval_to_dword(a);
      const DWORD pT = _fval_to_dword(tolerance);

      return (pA & 0x7fffffff) <= pT;
   }

   //===============================================
   //===============================================
   //Compare A==B ==================================

   // compare two floats
   __inline bool compareEqual(const float& a,const float& b)
   {
      const DWORD pA = _fval_to_dword(a);
      const DWORD pB = _fval_to_dword(b);

      return pA == pB;
   }

   // compare two floats for equality with an integer tolerance of eachother
   // IE tolerance=1000, we compare as 'equal' any two floating-point values that are within 1000 of each other when their bits are viewed as int over the entire number range
   // see [DAWSON05] and [LAMONT07] for why this is preferred over a floating point tolerance unit
   __inline bool compareEqual(const float& a,const float& b,const int& tolerance)
   { 
      // solid, fast routine across all platforms
      // with constant time behavior
      int pA = _fval_to_int(a);
      int pB = _fval_to_int(b);

      int test = _sgn_extend_mask(pA^pB); 

      int diff = (pA ^ (test & 0x7fffffff)) - pB;
      int v1 = tolerance + diff;
      int v2 = tolerance - diff;

      return (v1|v2) >= 0;
   }




   //=================================================
   //=================================================
   //Compare A <> B ==================================

   //returns TRUE if a < b
   //You need to make sure that if you are considering 0 as one of your positive values, that it is not actually –0, 
   //(i.e. you don’t want to pass –0 and another negative value to the positive compare function)
   __inline bool compareLessThan(const float& a,const float& b)
   {
      const int pA = _fval_to_int(a);
      const int pB = _fval_to_int(b);
      if (pA >= 0)   // at least one input is positive
         return pA < pB;
      else           // at least one input is negative.
         return (unsigned int)pA > (unsigned int)pB;    
   }

   // compare two floats for less than with an integer tolerance of eachother
   // IE tolerance=1000, we compare as 'less than' any two floating-point values that are within 1000 of each other when their bits are viewed as int over the entire number range
   // see [DAWSON05] and [LAMONT06] for why this is preferred over a floating point tolerance unit
   __inline bool compareLessThan(const float& a,const float& b,const int& tolerance)
   {
      int pA = _fval_to_int(a);
      int pB = _fval_to_int(b);

      int testa = _sgn_extend_mask(pA);
      int testb = _sgn_extend_mask(pB);

      pA = (pA & 0x7fffffff) ^ testa;
      pB = (pB & 0x7fffffff) ^ testb;

      return  pA + tolerance < pB;
   } 

   //returns TRUE if a > b
   //You need to make sure that if you are considering 0 as one of your positive values, that it is not actually –0, 
   //(i.e. you don’t want to pass –0 and another negative value to the positive compare function)
   __inline bool compareGreaterThan(const float& a,const float& b)
   {
      const int pA = _fval_to_int(a);
      const int pB = _fval_to_int(b);
      if (pB >= 0)   // at least one input is positive
         return pB < pA;
      else           // at least one input is negative.
         return (unsigned int)pB > (unsigned int)pA;    
   }

   // compare two floats for greater than with an integer tolerance of eachother
   // IE tolerance=1000, we compare as 'greater than' any two floating-point values that are within 1000 of each other when their bits are viewed as int over the entire number range
   // see [DAWSON05] and [LAMONT06] for why this is preferred over a floating point tolerance unit
   __inline bool compareGreaterThan(const float& a,const float& b,const int& tolerance)
   {
      int pA = _fval_to_int(a);
      int pB = _fval_to_int(b);

      int testa = _sgn_extend_mask(pA);
      int testb = _sgn_extend_mask(pB);

      pA = (pA & 0x7fffffff) ^ testa;
      pB = (pB & 0x7fffffff) ^ testb;

      return  pB + tolerance < pA;
   } 






   //======================================================================
   //======================================================================
   //Compare Against Zero, Return A, or B==================================
   //Does certain simple forms of if-then-else constructions, without branching.
   //* These have been assembly optimized according to the PowerPC spec
   //*
   //*  __declspec( naked ) causes no parameters to be referenced by name (to the compiler)
   // so the compiler doesn't add any extra instructions


   //--------------------------------------------------------------------------------
   // if (comp >= 0) ? a : b
#ifdef XBOX	
   #ifdef _USE_XBOX_ASM

   __inline float __declspec( naked )compareGreaterEqualZeroSelect(const float& comp,const float& a, const float& b)
   {
      //Assembly version
      __asm
      {
         lfs     fr3, 0(r3)   ;  
         lfs     fr4, 0(r4)   ; 
         lfs     fr5, 0(r5)   ; 

         fsel     fr1, fr3, fr4, fr5;
         blr                  ;  //return results in fr1
      }
   }
   #else
      __inline float compareGreaterEqualZeroSelect(const float& comp,const float& a, const float& b)
      {
         return static_cast<float>(__fsel(comp, a, b));
      }

   #endif
#else
    __inline float compareGreaterEqualZeroSelect(const float& comp,const float& a, const float& b)
    {
      return (comp >= 0.0f) ? a : b;
    }
#endif
   
   //--------------------------------------------------------------------------------
   // if (comp > 0) ? a : b
#ifdef XBOX	
#ifdef _USE_XBOX_ASM

    __inline float __declspec( naked )compareGreaterZeroSelect(const float& comp,const float& a, const float& b)
    {
       //Assembly version
       __asm
       {
          lfs     fr3, 0(r3)   ;  
          lfs     fr4, 0(r4)   ; 
          lfs     fr5, 0(r5)   ; 
          fneg    fr7, fr3     ;
          fsel     fr1, fr7, fr5, fr4;
          blr                  ;  //return results in fr1
       }
    }
#else
    __inline float compareGreaterZeroSelect(const float& comp,const float& a, const float& b)
    {
       return static_cast<float>(__fsel(-comp, b, a));
    }

#endif
#else
    __inline float compareGreaterZeroSelect(const float& comp,const float& a, const float& b)
    {
       return (-comp >= 0.0f) ? b : a;
    }
#endif


   //--------------------------------------------------------------------------------
   // if (comp==0) ? a : b
#ifdef XBOX	
#ifdef _USE_XBOX_ASM

    __inline float __declspec( naked )compareEqualZeroSelect(const float& comp,const float& a, const float& b)
    {
       //Assembly version
       __asm
       {
          lfs     fr3, 0(r3)   ;  
          lfs     fr4, 0(r4)   ; 
          lfs     fr5, 0(r5)   ; 
          fsel    fr0, fr3, fr4, fr5;
          fneg    fr7, fr3     ;
          fsel     fr1, fr7, fr0, fr5;
          blr                  ;  //return results in fr1
       }
    }
#else
    __inline float compareEqualZeroSelect(const float& comp,const float& a, const float& b)
    {
       //NON assembly version
       const double temp = __fsel(comp, a, b);
       return static_cast<float>(__fsel(-comp, temp, b));
    }

#endif
#else
    __inline float compareEqualZeroSelect(const float& comp,const float& a, const float& b)
    {
       const float tmp = (comp >= 0.0f) ? a : b;
       return (-comp >= 0.0f) ? tmp : b;
    }
#endif


   //====================================================================
   //====================================================================
   //Compare A Against B, Return X or Z==================================
   //Does certain simple forms of if-then-else constructions, without branching.
   //* These have been assembly optimized according to the PowerPC spec
   //*
   //*  __declspec( naked ) causes no parameters to be referenced by name (to the compiler)
   // so the compiler doesn't add any extra instructions

    //--------------------------------------------------------------------------------
   // if (a>=b) ? x : z
#ifdef XBOX	
#ifdef _USE_XBOX_ASM

    __inline float __declspec( naked )compareGreaterEqualSelect(const float& a, const float& b,const float& x, const float& z)
    {
       //Assembly version
       __asm
       {
          lfs     fr3, 0(r3)   ;  
          lfs     fr4, 0(r4)   ; 
          lfs     fr5, 0(r5)   ; 
          lfs     fr6, 0(r6)   ; 

          fsub fr0, fr3, fr4
             fsel fr1, fr0, fr5, fr6

             blr                  ;  //return results in fr1
       }
    }
#else
    __inline float compareGreaterEqualSelect(const float& a, const float& b,const float& x, const float& z)
    {
       //NON assembly version
       return static_cast<float>(__fsel(a-b, x, z));
    }

#endif
#else
    __inline float compareGreaterEqualSelect(const float& a, const float& b,const float& x, const float& z)
    {
       return (a >= b) ? x : z;
    }
#endif


    //--------------------------------------------------------------------------------
    // if (a>b) ? x : z
#ifdef XBOX	
#ifdef _USE_XBOX_ASM

    __inline float __declspec( naked )compareGreaterSelect(const float& a, const float& b,const float& x, const float& z)
    {
       //Assembly version
       __asm
       {
         lfs     fr3, 0(r3)   ;  
         lfs     fr4, 0(r4)   ; 
         lfs     fr5, 0(r5)   ; 
         lfs     fr6, 0(r6)   ; 

         fsub fr0, fr4, fr3
         fsel fr1, fr0, fr6, fr5

         blr                  ;  //return results in fr1
       }
    }
#else
    __inline float compareGreaterSelect(const float& a, const float& b,const float& x, const float& z)
    {
       //NON assembly version
       return static_cast<float>(__fsel(b-a, z, x));
    }

#endif
#else
    __inline float compareGreaterSelect(const float& a, const float& b,const float& x, const float& z)
    {
       return (a > b) ? x : z;
    }
#endif


    //--------------------------------------------------------------------------------
    // if (a == b) ? x : z
#ifdef XBOX	
#ifdef _USE_XBOX_ASM

    __inline float __declspec( naked )compareEqualSelect(const float& a, const float& b,const float& x, const float& z)
    {
       //Assembly version
      __asm
      {
         lfs     fr3, 0(r3)   ;  
         lfs     fr4, 0(r4)   ; 
         lfs     fr5, 0(r5)   ; 
         lfs     fr6, 0(r6)   ; 

         fsub fr7, fr3, fr4
         fsel fr1, fr7, fr5, fr6
         fneg fr7, fr7
         fsel fr1, fr7, fr1, fr6

         blr                  ;  //return results in fr1
      }

        
    }
#else
    __inline float compareEqualSelect(const float& a, const float& b,const float& x, const float& z)
    {
       //NON assembly version
       const float div = a-b;
       const double temp = __fsel(div, x, z);
       return static_cast<float>(__fsel(-div, temp, z));
    }

#endif
#else
    __inline float compareEqualSelect(const float& a, const float& b,const float& x, const float& z)
    {
       return (a == b) ? x : z;
    }
#endif

}




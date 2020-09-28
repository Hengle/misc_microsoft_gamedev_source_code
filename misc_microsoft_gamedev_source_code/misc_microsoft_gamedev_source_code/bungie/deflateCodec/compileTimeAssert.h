//============================================================================
// compileTimeAssert.h
// Copyright (c) 1996-2006, Ensemble Studios
//
// rgeldreich@ensemblestudios.com
//============================================================================
#pragma once

namespace ens
{
   template <bool x> 
   struct BCompileTimeAssertionFailure;

   template <> 
   struct BCompileTimeAssertionFailure<true> 
   { 
      enum { anything = 1 }; 
   };

   template<int x> 
   struct BCompileTimeAssertTry
   {
   };

   // Macro hack to join together A and B, even when one is itself a macro. 
   // Macro expansion actually occurs in BJOINER(), not BJOINERFINAL().
   #define BJOINERFINAL(a, b)  a##b
   #define BJOINER(a, b)       BJOINERFINAL(a, b)
   #define BJOIN(a, b)         BJOINER(a, b)

   // If p is false, the sizeof() will fail because BCompileTimeAssertionFailure<false> is undefined.
   // The compiler's error message may be cryptic!
   #define BCOMPILETIMEASSERT(p) typedef ens::BCompileTimeAssertTry <  sizeof(ens::BCompileTimeAssertionFailure< (bool)(p) >) >  BJOIN(CompileTimeAssertTypedef, __COUNTER__)
         
} // namespace ens

//============================================================================
//
//  rangecheck.inl
//  
//  Copyright (c) 2004, Ensemble Studios
//
//============================================================================
// Evil: Disable signed/unsigned mismatch warnings
#pragma warning(push)
#pragma warning(disable:4018) 

#ifdef ENABLE_DEBUG_RANGE_CHECKS
  template<class T> __forceinline T debugCheckNull(T i)
  {
    BDEBUG_ASSERT(i);
    return i;
  }
  
  template<class T, class U> __forceinline T debugRangeCheck(T i, U l, U h)
  {
    BDEBUG_ASSERT((i >= l) && (i < h));
    return i;
  }
  
  template<class T, class U> __forceinline T debugRangeCheck(T i, U h)
  {
    BDEBUG_ASSERT((i >= 0) && (i < h));
    return i;
  }
        
  template<class T, class U> __forceinline T debugRangeCheckIncl(T i, U l, U h)
  {
    BDEBUG_ASSERT((i >= l) && (i <= h));
    return i;
  }
  
  template<class T, class U> __forceinline T debugRangeCheckIncl(T i, U h)
  {
    BDEBUG_ASSERT((i >= 0) && (i <= h));
    return i;
  }
#else // ENABLE_DEBUG_RANGE_CHECKS
  template<class T> __forceinline T debugCheckNull(T i)
  {
    return i;
  }
  
  template<class T, class U> __forceinline  T debugRangeCheck(T i, U l, U h)
  {
    l; 
    h;
    return i;
  }
  
  template<class T, class U> __forceinline  T debugRangeCheck(T i, U h)
  {
    h;
    return i;
  }
  
  template<class T, class U> __forceinline  T debugRangeCheckIncl(T i, U l, U h)
  {
    l;
    h;
    return i;
  }
  
  template<class T, class U> __forceinline  T debugRangeCheckIncl(T i, U h)
  {
    h;
    return i;
  }
#endif // ENABLE_DEBUG_RANGE_CHECKS

#ifdef ENABLE_NORMAL_RANGE_CHECKS
   template<class T> __forceinline T checkNull(T i)
   {
      BASSERT(i);
      return i;
   }

   template<class T, class U> __forceinline T rangeCheck(T i, U l, U h)
   {
      BASSERT((i >= l) && (i < h));
      return i;
   }
   
   template<class T, class U> __forceinline T rangeCheck(T i, U h)
   {
      BASSERT((i >= 0) && (i < h));
      return i;
   }
         
   template<class T, class U> __forceinline T rangeCheckIncl(T i, U l, U h)
   {
      BASSERT((i >= l) && (i <= h));
      return i;
   }

   template<class T, class U> __forceinline T rangeCheckIncl(T i, U h)
   {
      BASSERT((i >= 0) && (i <= h));
      return i;
   }
#else // ENABLE_NORMAL_RANGE_CHECKS
   template<class T> __forceinline T checkNull(T i)
   {
      return i;
   }

   template<class T, class U> __forceinline T rangeCheck(T i, U l, U h)
   {
      l;
      h;
      return i;
   }
   
   template<class T, class U> __forceinline T rangeCheck(T i, U h)
   {
      h;
      return i;
   }
         
   template<class T, class U> __forceinline T rangeCheckIncl(T i, U l, U h)
   {
      l;
      h;
      return i;
   }

   template<class T, class U> __forceinline T rangeCheckIncl(T i, U h)
   {
      h;
      return i;
   }

#endif // ENABLE_NORMAL_RANGE_CHECKS

#pragma warning(pop)

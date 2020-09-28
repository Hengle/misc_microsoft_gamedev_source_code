//============================================================================
//
// File: interlocked.h
// Copyright (c) 2005-2006, Ensemble Studios
//
// Good reference:
// http://www.linuxjournal.com/article/8212
//============================================================================
#pragma once

#ifdef XBOX
   // Declare compiler read/write barrier intrinsics.
   extern "C" void _ReadWriteBarrier();
   #pragma intrinsic(_ReadWriteBarrier)

   extern "C" void _ReadBarrier();
   #pragma intrinsic(_ReadBarrier)

   extern "C" void _WriteBarrier();
   #pragma intrinsic(_WriteBarrier)
#else
   // These intrinsics are unsupported in VS.Net 2005.
   inline void _ReadWriteBarrier() { }
   inline void _ReadBarrier() { }
   inline void _WriteBarrier() { }
#endif

namespace Sync
{
   //============================================================================
   // InterlockedIncrementExport
   //============================================================================
   inline LONG InterlockedIncrementExport(LONG volatile * lpAddend)
   {
   #ifdef XBOX   
      __lwsync();
   #endif
      return InterlockedIncrement(lpAddend);
   }                                                             
   
   //============================================================================
   // InterlockedDecrementExport
   //============================================================================
   inline LONG InterlockedDecrementExport(LONG volatile * lpAddend)
   {
   #ifdef XBOX
      __lwsync();
   #endif
      return InterlockedDecrement(lpAddend);
   }      
   
   //============================================================================
   // InterlockedExchangeExport
   //============================================================================
   inline LONG InterlockedExchangeExport(LONG volatile * Target, LONG Value)
   {
   #ifdef XBOX
      __lwsync();
   #endif
      return InterlockedExchange(Target, Value);
   }        
   
   //============================================================================
   // CompareAndSwapAcquire
   //============================================================================
   inline BOOL CompareAndSwapAcquire(LONG volatile* pValue, LONG compValue, LONG newValue)
   {
      const LONG initialValue = InterlockedCompareExchange(pValue, newValue, compValue);
      
#ifdef XBOX      
      __lwsync();
#endif         
      return (initialValue == compValue);
   }                                           
   
   //============================================================================
   // CompareAndSwapRelease
   //============================================================================
   inline BOOL CompareAndSwapRelease(LONG volatile* pValue, LONG compValue, LONG newValue)
   {
#ifdef XBOX      
      __lwsync();
#endif         
      const LONG initialValue = InterlockedCompareExchange(pValue, newValue, compValue);

      return (initialValue == compValue);
   }  
   
   //============================================================================
   // CompareAndSwapMB
   // This is a conservative implementation - it doesn't know if you are using
   // release or acquire semantics so it's a full memory barrier.
   //============================================================================
   inline BOOL CompareAndSwapMB(LONG volatile* pValue, LONG compValue, LONG newValue)
   {
#ifdef XBOX                  
      __lwsync();
#endif   
      const LONG initialValue = InterlockedCompareExchange(pValue, newValue, compValue);
#ifdef XBOX      
      __lwsync();
#endif         
      return (initialValue == compValue);
   }
   
   //============================================================================
   // CompareAndSwap
   // Full memory barriers!
   //============================================================================
   inline BOOL CompareAndSwap(LONG volatile* pValue, LONG compValue, LONG newValue)
   {
#ifdef XBOX                  
      __lwsync();
#endif   
      const LONG initialValue = InterlockedCompareExchange(pValue, newValue, compValue);
#ifdef XBOX      
      __lwsync();
#endif         
      return (initialValue == compValue);
   }
   
#ifdef XBOX   
   //============================================================================
   // CompareAndSwap64Acquire
   //============================================================================
   inline BOOL CompareAndSwap64Acquire(LONG64 volatile* pValue, LONG64 compValue, LONG64 newValue)
   {
      //BDEBUG_ASSERT(((DWORD)(pValue) & 7) == 0);

      const LONG64 initialValue = InterlockedCompareExchange64(pValue, newValue, compValue);
      
#ifdef XBOX      
      __lwsync();
#endif
         
      return (initialValue == compValue);
   }
   
   //============================================================================
   // CompareAndSwap64Release
   //============================================================================
   inline BOOL CompareAndSwap64Release(LONG64 volatile* pValue, LONG64 compValue, LONG64 newValue)
   {
      //BDEBUG_ASSERT(((DWORD)(pValue) & 7) == 0);

#ifdef XBOX      
      __lwsync();
#endif         

      const LONG64 initialValue = InterlockedCompareExchange64(pValue, newValue, compValue);

      return (initialValue == compValue);
   }
   
   //============================================================================
   // CompareAndSwap64MB
   // This is a conservative implementation - it doesn't know if you are using
   // release or acquire semantics so it's a full memory barrier.
   //============================================================================
   inline BOOL CompareAndSwap64MB(LONG64 volatile* pValue, LONG64 compValue, LONG64 newValue)
   {
      //BDEBUG_ASSERT(((DWORD)(pValue) & 7) == 0);
      
#ifdef XBOX                  
      __lwsync();
#endif   

      const LONG64 initialValue = InterlockedCompareExchange64(pValue, newValue, compValue);
      
#ifdef XBOX      
      __lwsync();
#endif         

      return (initialValue == compValue);
   }
   
   //============================================================================
   // CompareAndSwap64
   // No memory barriers!
   //============================================================================
   inline BOOL CompareAndSwap64(LONG64 volatile* pValue, LONG64 compValue, LONG64 newValue)
   {
      //BDEBUG_ASSERT(((DWORD)(pValue) & 7) == 0);

      const LONG64 initialValue = InterlockedCompareExchange64(pValue, newValue, compValue);

      return (initialValue == compValue);
   }
#endif   
   
}

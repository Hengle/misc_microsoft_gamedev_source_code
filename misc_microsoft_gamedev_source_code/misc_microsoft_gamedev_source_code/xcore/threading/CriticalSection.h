//============================================================================
//
//  CriticalSection.h
//
//  Copyright (c) 1999-2006, Ensemble Studios
// 
//============================================================================
#pragma once

#ifndef XBOX
   #if (_WIN32_WINNT < 0x0403)
      #error Please define _WIN32_WINNT to be 0x0403 or greater before including windows.h!
   #endif   
#endif

//----------------------------------------------------------------------------
//  Class BCriticalSection
//----------------------------------------------------------------------------
class BCriticalSection
{
public:
   //-- Construction/Destruction
   inline BCriticalSection(uint spinCount = 0) 
   { 
#ifdef BUILD_DEBUG
      mLockCount = 0;
#endif   
      InitializeCriticalSectionAndSpinCount(&mCrit, spinCount); 
   }
   
   inline ~BCriticalSection() 
   { 
#ifdef BUILD_DEBUG   
      MemoryBarrier();
      if (mLockCount) 
      {
         OutputDebugStringA("BCriticalSection::~BCriticalSection: WARNING: Deleting a critical section that may still be locked!\n");
      }
#endif      
      DeleteCriticalSection(&mCrit); 
   }
      
   //-- Interface
   inline void setSpinCount(uint spinCount)
   {
      SetCriticalSectionSpinCount(&mCrit, spinCount);
   }
   
   inline void lock(void) 
   { 
      EnterCriticalSection(&mCrit); 
#ifdef BUILD_DEBUG      
      mLockCount++;
#endif      
   }
   
   inline void unlock(void) 
   { 
#ifdef BUILD_DEBUG   
      if (mLockCount < 1) 
         DebugBreak();
      mLockCount--;               
#endif   
      LeaveCriticalSection(&mCrit); 
   }

private:
   //-- Disable copy constructor and equals operator.
   BCriticalSection(const BCriticalSection& crit);
   const BCriticalSection& operator = (const BCriticalSection& crit);

   //-- Private Data
   CRITICAL_SECTION mCrit;
   
#ifdef BUILD_DEBUG
   int mLockCount;
#endif   
};

//----------------------------------------------------------------------------
//  Class BScopedCriticalSection
//----------------------------------------------------------------------------
class BScopedCriticalSection
{
public:

   //-- Construction/Destruction
   inline BScopedCriticalSection (BCriticalSection& crit) : mCrit(crit) { mCrit.lock();   }
   inline ~BScopedCriticalSection()                                     { mCrit.unlock(); }

private:
   //-- Disable copy constructor and equals operator.
   BScopedCriticalSection(const BScopedCriticalSection& crit);
   BScopedCriticalSection& operator = (const BScopedCriticalSection& crit);

   //-- Private Data
   BCriticalSection& mCrit;
};

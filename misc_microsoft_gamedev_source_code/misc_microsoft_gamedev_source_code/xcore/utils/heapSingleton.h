//==============================================================================
// File: heapSingleton.h
//
// Copyright (c) 2005-2006, Ensemble Studios
//
//==============================================================================
#pragma once

template <typename T, typename Allocator = BPrimaryFixedHeapAllocator>
class BHeapSingleton
{
public:
   static void createInstance(void)
   {
      if (!mpInstance)
      {
         mpInstance = static_cast<T*>(mAllocator.alloc(sizeof(T)));
         if (!mpInstance)
         {
            BFATAL_FAIL("BHeapSingleton::getInstance: Allocation failed");
         }

         Utils::ConstructInPlace(mpInstance);
      }
   }

   static void destroyInstance(void)
   {
      if (mpInstance)
      {
         Utils::DestructInPlace(mpInstance);
         
         mAllocator.dealloc(mpInstance);
         mpInstance = NULL;
      }
   }
   
   static T& getInstance()
   {
      createInstance();
      
      return *mpInstance;
   }
   
   static T* getInstancePtr(void) { return mpInstance; }
   
protected:
   BHeapSingleton() { }
   
   static T* mpInstance;
   static Allocator mAllocator;

private:
   BHeapSingleton(const BHeapSingleton&);
   BHeapSingleton& operator= (const BHeapSingleton&);
};

template <typename T, typename Allocator> T*          BHeapSingleton<T, Allocator>::mpInstance;
template <typename T, typename Allocator> Allocator   BHeapSingleton<T, Allocator>::mAllocator;

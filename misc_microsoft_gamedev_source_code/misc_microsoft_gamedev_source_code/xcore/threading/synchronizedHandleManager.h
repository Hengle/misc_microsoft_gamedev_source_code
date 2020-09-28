// File: synchronizedHandleManager.h
#pragma once

#include "synchronizedBlockAllocator.h"

typedef uint BSynchronizedHandle;
const BSynchronizedHandle cInvalidSynchronizedHandle = static_cast<BSynchronizedHandle>(0xFFFFFFFFU);

template<uint cNumSlots, typename ValueType>
class BSynchronizedHandleManager
{
public:
   BSynchronizedHandleManager() { }
      
   // All methods are callable from any/multiple threads.

   // Allocates a new memory handle. 
   // Returns cInvalidSynchronizedHandle if out of handles.
   // A memory barrier (critical section, or a Win-32 sync API) must occur sometime before 
   // any other threads read this handle's associated data.
   BSynchronizedHandle alloc(ValueType value = ValueType(), bool failIfFull = true)
   {
      uint slotIndex;
      BHandleSlot* pSlot = mAllocator.alloc(slotIndex);
      if (!pSlot)
      {
         if (failIfFull)
         {
            BFATAL_FAIL("BSyncHandleManager: Handlespace full");
         }
         else
            return cInvalidSynchronizedHandle;
      }
         
      // Should be safe to set the values directly here, because we know only one thread has access to the slot.
      pSlot->mValue = value;
            
      return slotIndex;
   }
   
   // Frees a handle. 
   // Warning: As soon as a handle is freed because, assume it will be reused immediately
   // and asynchronously by another thread!
   void free(BSynchronizedHandle handle)
   {
      mAllocator.free(handle);
   }
   
   // Sets the handle's associated value. 
   // Use caution with this method, if another thread calls getValue() on this handle the results could be unpredictable.
   void setValue(BSynchronizedHandle handle, ValueType value)
   {
      BHandleSlot* pSlot = mAllocator.getSlotPtr(handle);
      pSlot->mValue = value;
   }
   
   // Gets the handle's associated value.
   ValueType getValue(BSynchronizedHandle handle)
   {
      BHandleSlot* pSlot = mAllocator.getSlotPtr(handle);
      return pSlot->mValue;
   }

private:
   struct BHandleSlot
   {
      volatile ValueType mValue;
   };

   typedef BSynchronizedBlockAllocator<BHandleSlot, cNumSlots, false> BBlockAllocator;
   BBlockAllocator mAllocator;   
};

//============================================================================
//
//  trackedObject.h
//
//  Copyright (c) 2007, Ensemble Studios
//
//============================================================================
#pragma once

#include "containers\linkedList.h"

template<typename ValueType, bool ThreadSafe = true>
class BTrackedObject
{
public:
   enum { threadSafe = ThreadSafe };
   
   typedef ValueType* BTrackedObjectPtr;
   typedef BLinkedList<BTrackedObjectPtr> BTrackedObjectPtrList;
   
   BTrackedObject()
   {
      lockObjectList();

      mpListPointer = getObjectList().pushBack(reinterpret_cast<BTrackedObjectPtr>(this)).getItem();

      unlockObjectList();
   }
   
   BTrackedObject(const BTrackedObject& other)
   {
      other;
      
      lockObjectList();

      mpListPointer = getObjectList().pushBack(reinterpret_cast<BTrackedObjectPtr>(this)).getItem();

      unlockObjectList();
   }
       
   BTrackedObject(BTrackedObjectPtr pObject)
   {
      lockObjectList();

      mpListPointer = getObjectList().pushBack(pObject).getItem();

      unlockObjectList();
   }

   ~BTrackedObject()
   {
      lockObjectList();
      
      getObjectList().remove(getObjectList().getIterator(mpListPointer));

      unlockObjectList();
   }

   BTrackedObject& operator= (const BTrackedObject& other)
   {
      other;
      return *this;
   }
                 
   static BTrackedObjectPtrList& getObjectList(void)
   {
      static BTrackedObjectPtrList trackedObjects;
      return trackedObjects;
   }
   
   static BCriticalSection& getObjectListCriticalSection(void)
   {
      static BCriticalSection criticalSection;
      return criticalSection;
   }
   
   static void lockObjectList(void) { if (ThreadSafe) getObjectListCriticalSection().lock(); }
   static void unlockObjectList(void) { if (ThreadSafe) getObjectListCriticalSection().unlock(); }

private:
   typename BTrackedObjectPtrList::valueType* mpListPointer;
};
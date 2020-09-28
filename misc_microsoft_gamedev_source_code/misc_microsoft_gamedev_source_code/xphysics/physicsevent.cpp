//==============================================================================
// PhysicsEvent.cpp
//
// Copyright (c) 2002, Ensemble Studios
//==============================================================================

#include "common.h"
#include "PhysicsEvent.h"



//==============================================================================
// pointer to pointer compare function  (this* comparison)
//==============================================================================
long CALLBACK simpleObserverCompare(const IPhysicsEventObserver &o1, const IPhysicsEventObserver &o2, void * /*pParam*/)
{
   //-- just compare the addresses
   if (&o1 == &o2)
      return (0);

   return (-1);
}

//==============================================================================
// BPhysicsEventSubject - constructor
//==============================================================================
BPhysicsEventSubject::BPhysicsEventSubject()
{
}

//==============================================================================
// BPhysicsEventSubject::~BPhysicsEventSubject
//==============================================================================
BPhysicsEventSubject::~BPhysicsEventSubject()
{
}

//==============================================================================
// BPhysicsEventSubject::attach
//==============================================================================
bool BPhysicsEventSubject::attach(IPhysicsEventObserver& observer)
{
   //-- make sure we are not in the list already
   BHandle hItem = mObservers.findItemForward(simpleObserverCompare, NULL, &observer);
   if (hItem)
      return (false);

   //-- register to listen
   mObservers.addToTail(&observer);
   return (true);
}

//==============================================================================
// BPhysicsEventSubject::detach
//==============================================================================
bool BPhysicsEventSubject::detach(IPhysicsEventObserver& observer)
{
   //-- find the item
   BHandle hItem = mObservers.findItemForward(simpleObserverCompare, NULL, &observer);
   if (!hItem)
      return (false);

   //-- stop listening
   mObservers.remove(hItem);
   return (true);
}

//==============================================================================
// BPhysicsEventSubject::notify
//==============================================================================
void BPhysicsEventSubject::notify( const BPhysicsEvent &PhysicsEvent ) const
{
   //-- send this PhysicsEvent on to all the observers
   BHandle hItem = NULL;
   IPhysicsEventObserver *pObserver = mObservers.getHead(hItem);
   while (pObserver)
   {
      pObserver->notification(PhysicsEvent);
      pObserver = mObservers.getNext(hItem);
   }
}

//==============================================================================
// SimOrderManager.cpp
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#include "common.h"
#include "SimOrderManager.h"

BSimOrderManager gSimOrderManager;



//==============================================================================
//==============================================================================
BSimOrderManager::BSimOrderManager()
{
}

//==============================================================================
//==============================================================================
BSimOrderManager::~BSimOrderManager()
{
}

//==============================================================================
//==============================================================================
bool BSimOrderManager::init()
{
   mOrders.clear();
   mDeleteOrders.clear();
   mOrderCounter=0;
   mOppCounter=0;
   return(true);
}

//==============================================================================
//==============================================================================
void BSimOrderManager::reset()
{
   for (uint i=0; i < mOrders.getSize(); i++)
      BSimOrder::releaseInstance(mOrders[i]);
   init();
}

//==============================================================================
//==============================================================================
void BSimOrderManager::update()
{
   //Go through all of the orders that are marked for deletion.  If they still
   //have a 0 ref count, nuke them.  If they have a ref count, take them off the
   //nuke list.
   for (uint i=0; i < mDeleteOrders.getSize(); i++)
   {
      if (mDeleteOrders[i]->getRefCount() > 0)
         continue;
      BSimOrder* pOrder=mDeleteOrders[i];

      //Remove it from the orders list.
      mOrders.removeValue(pOrder);

      //Return it.
      pOrder->init();
      BSimOrder::releaseInstance(pOrder);
   }

   //Clear out the delete list.
   mDeleteOrders.clear();
}

//==============================================================================
//==============================================================================
BSimOrder* BSimOrderManager::createOrder()
{
   BSimOrder* pOrder=BSimOrder::getInstance();
   if (!pOrder)
      return (NULL);
   
   //Init it.
   if (!pOrder->init())
   {
      BSimOrder::releaseInstance(pOrder);
      return(NULL);
   }

   //Set the ID.
   mOrderCounter++;
   pOrder->setID(mOrderCounter);
   //Save it.
   mOrders.add(pOrder);

   //Done.
   return(pOrder);
}

//==============================================================================
//==============================================================================
void BSimOrderManager::markForDelete(BSimOrder* pOrder)
{
   if (pOrder)
   {
      // ajl 4/10/08 - It's valid for the refCount to be > 0 in the case where a platoon
      // has two squads that share an order, then the squads get recommanded on the same
      // update in a way where they get put into two separate platoons. In that case the
      // squad delays it's removal of it's order which leaves the reference here until
      // the next update.
      //BASSERT(pOrder->getRefCount() == 0);
      mDeleteOrders.uniqueAdd(pOrder);
   }
}

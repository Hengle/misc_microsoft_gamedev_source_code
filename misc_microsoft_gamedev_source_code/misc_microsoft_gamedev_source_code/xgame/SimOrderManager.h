//==============================================================================
// SimOrderManager.h
// Copyright (c) 2007 Ensemble Studios
//==============================================================================

#pragma once
#include "SimOrder.h"


//==============================================================================
//==============================================================================
class BSimOrderManager
{
   public:
      BSimOrderManager();
      ~BSimOrderManager();

      //Init.
      bool                       init();
      void                       reset();
      //Update.
      void                       update();

      //Order.
      BSimOrder*                 createOrder();
      void                       markForDelete(BSimOrder* order);
      
      //Opp.
      uint                       getNextOppID() { mOppCounter++; return (mOppCounter); }

   protected:
      BSimOrderArray             mOrders;
      BSimOrderArray             mDeleteOrders;
      uint                       mOrderCounter;

      uint                       mOppCounter;
};

extern BSimOrderManager gSimOrderManager;

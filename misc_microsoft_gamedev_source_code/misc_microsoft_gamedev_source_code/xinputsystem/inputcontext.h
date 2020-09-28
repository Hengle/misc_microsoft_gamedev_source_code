//==============================================================================
// inputcontext.h
//
// Copyright (c) 2004, Ensemble Studios
//==============================================================================
#pragma once
#include "xmlreader.h"

// Forward declarations
class BInputControl;
class BInputEventDetail;
class IInputControlEventHandler;

//==============================================================================
// BInputContext
//==============================================================================
class BInputContext
{
   public:
                                       BInputContext();
                                       ~BInputContext();

      bool                             setup(BXMLNode node, IInputControlEventHandler* inputControlEventHandler);

      const BSimString&                getName() const { return mName; }

      long                             getPriority() const { return mPriority; }
      void                             setPriority(long val) { mPriority=val; }

      bool                             handleInput(long port, long event, long controlType, BInputEventDetail& detail);
      void                             reset();

      void                             clear();

   protected:
      BSimString                          mName;
      long                             mPriority;
      long                             mControlCount;
      BInputControl*                 mControlList;

};

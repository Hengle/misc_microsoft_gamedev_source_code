//==============================================================================
// BUIInputHandler.h
//
// Copyright (c) 2006-2007 Ensemble Studios
//==============================================================================
#pragma once

// Includes

class IInputControlEventHandler;

//==============================================================================
// 
//==============================================================================
class BUIInputHandler
{

public:
                  BUIInputHandler();
      virtual     ~BUIInputHandler();

      void        enterContext(const BCHAR_T* contextName);
      bool        loadControls(const BCHAR_T* name, IInputControlEventHandler* handler);
      bool        loadControls(BXMLNode controlsNode, IInputControlEventHandler* handler);

      bool        handleInput(long port, long event, long controlType, BInputEventDetail& detail);
      void        reset();


protected:
      BSmallDynamicSimArray<BInputContext*>  mContextList;
      BInputContext*                         mpActiveContext;
};

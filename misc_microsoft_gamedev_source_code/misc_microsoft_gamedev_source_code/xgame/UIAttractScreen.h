//============================================================================
// UIAttractScreen.h
// Ensemble Studios (c) 2008
//============================================================================

#pragma once 
#include "UIScreen.h"
#include "binkInterface.h"
#include "UITextFieldControl.h"
#include "UIGlobals.h"

class BUIAttractScreen : public BUIScreen, public BUIGlobals::yornHandlerInterface
{

public:
   BUIAttractScreen( void );
   virtual ~BUIAttractScreen( void );

   virtual bool init(BXMLNode dataNode);

   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

   void yornResult(uint result, DWORD userContext, int port);

protected:

   BUITextFieldControl  mESRBNotice;
   BUITextFieldControl  mTextField;
   BUITextFieldControl  mCopyright;

};
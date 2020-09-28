//============================================================================
// UIEndGame.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once

#include "UIScreen.h"
#include "UIButtonBarControl.h"
#include "UIGlobals.h"

class BUIEndGame : public BUIScreen, public BUIGlobals::yornHandlerInterface
{
public:
   enum { eResult_Stats, eResult_Quit };

public:
   BUIEndGame( void );
   virtual ~BUIEndGame( void );

   // BFlashScene
   virtual bool init( BXMLNode root );

   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual bool handleInput( int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail );
   virtual void setResult( const BString& civName, long result, const BUString& message);

   virtual void enter( void );
   virtual void update( float dt );
   virtual void leave( void );

   virtual void yornResult( uint result, DWORD userContext, int port );

protected:
   void fadeOut( void );

   long mGameType;
   bool bShown;
   bool bDoneWaiting;
   BUIButtonBarControl mButtonBar;
   float mAutoDelay;
};
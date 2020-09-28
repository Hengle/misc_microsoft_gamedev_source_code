//============================================================================
// UIGameSetupPlayerControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"

#include "UIGamerTagLongControl.h"

class BUIGameSetupPlayerControl : public BUIControl
{
public:
   enum Events
   {
      eLabelEvent = UIGameSetupPlayerControlID,
   };
   IMPLEMENT_CONTROL_TYPE_ACCESSORS( UIGameSetupPlayerControl );
public:

   BUIGameSetupPlayerControl( void );
   virtual ~BUIGameSetupPlayerControl( void );

   virtual bool init( IUIControlEventHandler* parent, const char* controlPath, int controlID = -1, BXMLNode* initData = NULL );

   // accessors
   BUIGamerTagLongControl& getGamerTag() { return mGamerTag; }

   // overrides
   virtual void focus( bool force=false );
   virtual void unfocus( bool force=false );

   virtual void playTransition(const char * transitionName);

   virtual void showSpinner(bool bShow);

protected:

   BUIGamerTagLongControl  mGamerTag;



};
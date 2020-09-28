//============================================================================
// UIMenuItemControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once
#include "UIControl.h"
#include "usermanager.h"
#include "UIGamerTagControl.h"

class BUser; 

class BUIGamerTagLongControl : public BUIGamerTagControl
{
public:

   enum
   {
      cGamerTagStatusActive,
      cGamerTagStatusOut
   };

   BUIGamerTagLongControl( void );
   virtual ~BUIGamerTagLongControl( void );

   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

   virtual void setLeaderPic(const BString& imgURL);
   virtual void setAIDifficulty(int difficulty);
   virtual void setStatus(int status);
   virtual void setReady(bool ready);

   virtual void clear();


protected:

   void  reset();

   BString  mLeaderPicURL;
   int      mAIDifficulty;
   int      mStatus;
   bool     mReady;
};
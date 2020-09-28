//============================================================================
// BUIServiceRecordSkirmishPanelControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once

#include "xcore.h"

#include "UIPanelControl.h"
#include "UITwoColumnScrollTextFieldControl.h"
class BPlayer;
class BUser;

//============================================================================
//============================================================================
class BUIServiceRecordSkirmishPanelControl : public BUIPanelControl, public BEventReceiver
{
   public:
   
      BUIServiceRecordSkirmishPanelControl( void );
      virtual ~BUIServiceRecordSkirmishPanelControl( void );

      virtual bool init( IUIControlEventHandler* parent, const char* controlPath, int controlID = -1, const BXMLNode* initData = NULL );

      bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
      virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

      bool handleUIControlEvent( BUIControlEvent& event );

      bool populateFromUser(BUser* pUser);


   protected:

      enum
      {
         cQueryEventTimer = cEventClassFirstUser,
      };

      void updateData(const BUString& dataString1, const BUString& dataString2);

      // helper functions to format text
      void appendLineBreak(BUString& s);
      void appendIndention(BUString& s, uint num);

      void populateSkirmishRecord(BUser* pUser, BUString& dataString1, BUString& dataString2);

      // BEventReceiver
      virtual bool receiveEvent(const BEvent& event, BThreadIndex threadIndex);

      BUITwoColumnScrollTextFieldControl mScrollList;

      // cache to see if we should update the page if this changes
      XUID mXuid;
      uint mLevel;

      BWin32WaitableTimer mQueryTimer;

      bool mQueryTimerSet : 1;
};

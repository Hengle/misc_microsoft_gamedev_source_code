//============================================================================
// BUIServiceRecordCampaignPanelControl.h
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
class BUIServiceRecordCampaignPanelControl : public BUIPanelControl
{
   public:
   
      BUIServiceRecordCampaignPanelControl( void );
      virtual ~BUIServiceRecordCampaignPanelControl( void );

      virtual bool init( IUIControlEventHandler* parent, const char* controlPath, int controlID = -1, const BXMLNode* initData = NULL );

      bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
      virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

      bool handleUIControlEvent( BUIControlEvent& event );

      bool populateFromUser(BUser* pUser);


protected:

      // helper functions to format text
      void appendLineBreak(BUString& s);
      void appendIndention(BUString& s, uint num);


      void populateMissionData(BUser* pUser, BUString& dataString1, BUString& dataString2);

      void updateData(const BUString& dataString1, const BUString& dataString2);
      

      BUITwoColumnScrollTextFieldControl mScrollList;
};
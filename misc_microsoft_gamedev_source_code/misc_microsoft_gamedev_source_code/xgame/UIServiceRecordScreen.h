//============================================================================
// UIServiceRecordScreen.h
// Ensemble Studios (c) 2008
//============================================================================

#pragma once 
#include "UIScreen.h"
#include "UITextFieldControl.h"
#include "UITwoColumnScrollTextFieldControl.h"
#include "UIButtonBarControl.h"
#include "UITabControl.h"
#include "UIGamerTagControl.h"
#include "UI.h"

class BUIPanelControl;
class BUITwoColumnScrollTextFieldControl;
class BUIServiceRecordCampaignPanelControl;
class BUIServiceRecordSkirmishPanelControl;

//============================================================================
//============================================================================
class BUIServiceRecordScreen : public BUIScreen
{
public:
   enum
   {
      cServiceRecordTabCampaign=0,
      cServiceRecordTabSkirmish,      
      cServiceRecordTabNumber,

   };
   BUIServiceRecordScreen( void );
   virtual ~BUIServiceRecordScreen( void );

   virtual bool init( const char* filename, const char* datafile );
   virtual bool init(BXMLNode dataNode);

   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual bool handleUIControlEvent( BUIControlEvent& event );

   void populateFromUser(BUser* pUser);
   bool displayButtons();

protected:
   
   bool addTab(const char * labelName, BUIPanelControl *panel, const WCHAR* labelText);   
   
   BUITextFieldControl  mTitle;
   BUITabControl        mTabControl;
   BUIGamerTagControl   mGamertag;

   BUIButtonBarControl  mButtonBar;
   BDynamicArray<BUIControl*> mControls;

   BUIServiceRecordCampaignPanelControl* mpPanelCampaign;
   BUIServiceRecordSkirmishPanelControl* mpPanelSkirmish;

};
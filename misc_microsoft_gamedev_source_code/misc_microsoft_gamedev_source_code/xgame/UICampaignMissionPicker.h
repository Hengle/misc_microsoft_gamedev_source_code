//============================================================================
// UICampaignMenu.h
// Ensemble Studios (c) 2008
//============================================================================

#pragma once 
#include "UIScreen.h"
#include "UIListControl.h"
#include "UIScrollableTextFieldControl.h"
#include "UIImageViewerControl.h"
#include "UIButtonBarControl.h"
#include "UIGamerTagControl.h"
#include "UIDifficultyDisplayControl.h"
#include "UILabelControl.h"
#include "UITextFieldControl.h"
#include "UIMenuItemControl.h"
class BCampaignNode;


class BUICampaignMissionPicker : public BUIScreen
{

public:

   enum
   {
      cMissionPickerMissionMenu,

      cMissionPickerControlCount
   };
   BUICampaignMissionPicker( void );
   virtual ~BUICampaignMissionPicker( void );

   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual bool handleUIControlEvent( BUIControlEvent& event );


   virtual bool init( const char* filename, const char* datafile );
   virtual bool init(BXMLNode dataNode);

   BUIDifficultyDisplayControl *getDifficultyDisplay() { return mInitialized ? &mDifficultyText : NULL; }

   void updateScoreDisplay();
   void updateRScrollButton();
   void updateTitle();

protected:

   void populateScreen();

   BCampaignNode* getCampaignNode();

   bool populateMissionMenu();


   BUIMenuItemControl* getMenuItemControl(int controlID);


   void initImageViewer();
   bool displayButtons();

   void setScoreText(const BUString &text);
   void setMedalText(const BUString &text);
   void setSkullText(const BUString &text);
   void setBlackBoxText(const BUString &text);
   void setCoopText(const BUString &text);
   void setParTimeText(const BUString &text);


   BUIListControl                mMissionMenu;

   BUIScrollableTextFieldControl mHelpText;
   BUIImageViewerControl         mImageViewer;

   BUIButtonBarControl           mButtonBar;
   BUIDifficultyDisplayControl   mDifficultyText;


   BUITextFieldControl           mTitleLabel;
   BUITextFieldControl           mDescriptionLabel;

   BUITextFieldControl           mScoreDisplay;
   BUITextFieldControl           mMedalDisplay;

   // Menu Items
   static const long cMAX_MENU_ITEMS = 20;
   BUIMenuItemControl mMissionMenuItems[cMAX_MENU_ITEMS];
   
   int                           mCurrentMission;

   bool                          mInitialized;
};
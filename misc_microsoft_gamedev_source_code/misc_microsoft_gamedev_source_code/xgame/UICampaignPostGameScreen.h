//============================================================================
// UICampaignPostGameScreen.h
// Ensemble Studios (c) 2008
//============================================================================

#pragma once 
#include "UIScreen.h"

#include "UIButtonBarControl.h"
#include "UITabControl.h"
#include "UITextFieldControl.h"
#include "UIGlobals.h"
#include "UIDifficultyDisplayControl.h"
#include "UIScrollableCalloutControl.h"


class BUIPanelControl;
class BUICpgSummaryPanelControl;
class BUICpgGradePanelControl;


class BUICampaignPostGameScreen : public BUIScreen, public BUIGlobals::yornHandlerInterface
{
public:
   enum
   {
      cCampaignPostgameTabSummary=0,
      cCampaignPostgameTabGrades,
      //cCampaignPostgameTabAwards,

      cCampaignPostgameTabNumber,

   };
   BUICampaignPostGameScreen( void );
   virtual ~BUICampaignPostGameScreen( void );

   virtual bool init( const char* filename, const char* datafile );
   virtual bool init(BXMLNode dataNode);

   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );
   virtual bool handleUIControlEvent( BUIControlEvent& event );

   void populate();

   bool displayButtons();

   virtual void enter( void );

   enum EResult { eResult_Quit, eResult_Continue, eResult_Replay, eResult_GoToAdvancedTutorial, eResult_None };

   virtual void yornResult(uint result, DWORD userContext, int port);

protected:

   bool addTab(const char * labelName, BUIPanelControl *panel, const WCHAR* labelText, bool visible);
   void populateMissionName();
   void populateTimeAndDifficulty();

   // My main controls
   BUITextFieldControl  mTitle;
   BUITextFieldControl  mGameTime;
   BUIDifficultyDisplayControl mDifficulty;

/*
   BUITextFieldControl  mMissionNameText;
   BUITextFieldControl  mTitleSmallRight;
*/

   BUITabControl        mTabControl;
   BUIButtonBarControl  mButtonBar;
   BDynamicArray<BUIControl*> mControls;

   BUICpgSummaryPanelControl* mpPanelSummary;
   BUICpgGradePanelControl*   mpPanelGrade;

   // call out controls
   BUIScrollableCalloutControl    mCallout;

   uint mAButtonIcon;
   BUString mAButtonText;
   long mAButtonResult;
   long mStartContext;

   XUID mSelectedXUID;

   bool mTabInitDone:1;
};
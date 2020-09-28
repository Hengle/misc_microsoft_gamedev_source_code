//============================================================================
// UICpgSummaryPanelControl.h
// Ensemble Studios (c) 2008
//============================================================================
#pragma once

#include "xcore.h"

#include "UIPanelControl.h"

#include "UIScrollingListControl.h"
#include "UIGamerTagLongControl.h"
#include "UIObjectiveControl.h"
#include "UILabelControl.h"
#include "UITextFieldControl.h"
#include "UIScrollableTextFieldControl.h"

class BPlayer;

class BUICpgSummaryPanelControl : public BUIPanelControl
{

public:

   enum
   {
      cControlIDList1=0,
      cControlIDList2,
   };

   enum
   {
      cMaxObjectives=11,
   };

   BUICpgSummaryPanelControl( void );
   virtual ~BUICpgSummaryPanelControl( void );

   virtual bool init( IUIControlEventHandler* parent, const char* controlPath, int controlID = -1, BXMLNode* initData = NULL );

   void show( bool force = false );
   void hide( bool force = false );

   bool handleInput(int port, BInputEventType event, BInputControlType controlType, BInputEventDetail& detail);
   virtual bool executeInputEvent( long port, long event, long controlType, BInputEventDetail& detail, const BSimString& command, const BInputControl* pInputControl );

   bool handleUIControlEvent( BUIControlEvent& event );

   bool populate();


protected:

   void populateMissionResult();
   void populatePlayers();
   void populatePlayer(BPlayer* player, BUIGamerTagLongControl& playerControl);
   void populateObjectives();
   bool addObjective(const WCHAR* menuText, int listID, int index, int controlID, int type, int status);
   BPlayer * getCoopPlayer(BPlayer* player);
   void showSelectedObjective();
   void updateRScrollButton();
   void selectFirstObjective();
   void releaseControls();


   // Completion Status
   BUITextFieldControl                 mMissionStatusText;

   BUITextFieldControl                 mDescription;
   BUIScrollableTextFieldControl       mHelpText;


   // gamer tags
   BUIGamerTagLongControl              mPlayer1;
   BUIGamerTagLongControl              mPlayer2;

   // we will also need award controls in here.

   // incomplete objectives
   BUILabelControl                     mObjectiveList1Label;
   BUIScrollingListControl             mObjectiveList1;
   BDynamicArray<BUIObjectiveControl*> mObjectiveList1Items;

   // complete objectives
   BUILabelControl                     mObjectiveList2Label;
   BUIScrollingListControl             mObjectiveList2;
   BDynamicArray<BUIObjectiveControl*> mObjectiveList2Items;

   BUIScrollingListControl*            mpLastObjectiveControlWithFocus;
   BUILabelControl*                    mpLastObjectiveLabelWithFocus;

   BDynamicArray<BUIControl*>          mControls;

   uint            mLastViewChangeTime;          //E3 hax - eric

   BUIControl* mpSelectedControl;

};